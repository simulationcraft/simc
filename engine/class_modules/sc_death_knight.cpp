// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// TODO as of 2023-11-02
// Class:
// Killing Blow based mechanics (free Death Strike, Rune of Unending Thirst)
// Disable noise from healing/defensive actions when simming a single, dps role, character
// Automate Rune energize in death_knight_action_t::execute() instead of per spell overrides
// Look into Death Strike OH handling (p -> dual_wield()?) and see if it can apply to other DW attacks
// Unholy:
// - Predict the first two Festering wounds on FS and use reaction time on the third?
// Blood:
// - Check that VB's absorb increase is correctly implemented
// - Healing from Consumption damage done

#include "config.hpp"

#include "action/action_callback.hpp"
#include "action/parse_effects.hpp"
#include "class_modules/apl/apl_death_knight.hpp"
#include "player/pet_spawner.hpp"

#include "simulationcraft.hpp"

namespace
{  // UNNAMED NAMESPACE

// Finds an action with the given name. If no action exists, a new one will
// be created.
//
// Use this with secondary background actions to ensure the player only has
// one copy of the action.
// Shamelessly borrowed from the mage module
template <typename Action, typename Actor, typename... Args>
action_t* get_action( util::string_view name, Actor* actor, Args&&... args )
{
  action_t* a = actor->find_action( name );
  if ( !a )
    a = new Action( name, actor, std::forward<Args>( args )... );
  assert( dynamic_cast<Action*>( a ) && a->name_str == name && a->background );
  return a;
}

template <typename V>
static const spell_data_t* resolve_spell_data( V data )
{
  if constexpr ( std::is_invocable_v<decltype( &spell_data_t::ok ), V> )
    return data;
  else if constexpr ( std::is_invocable_v<decltype( &buff_t::data ), V> )
    return &data->data();
  else if constexpr ( std::is_invocable_v<decltype( &action_t::data ), V> )
    return &data->data();

  assert( false && "Could not resolve find_effect argument to spell data." );
  return nullptr;
}

// finds a spell effect
// 1) first argument can be either player_talent_t, spell_data_t*, buff_t*, action_t*
// 2) if the second argument is player_talent_t, spell_data_t*, buff_t*, or action_t* then only effects that affect it
// are returned 3) if the third (or second if the above does not apply) argument is effect subtype, then the type is
// assumed to be E_APPLY_AURA further arguments can be given to filter for full type + subtype + property
template <typename T, typename U, typename... Ts>
static const spelleffect_data_t& find_effect( T val, U type, Ts&&... args )
{
  const spell_data_t* data = resolve_spell_data<T>( val );

  if constexpr ( std::is_same_v<U, effect_subtype_t> )
    return spell_data_t::find_spelleffect( *data, E_APPLY_AURA, type, std::forward<Ts>( args )... );
  else if constexpr ( std::is_same_v<U, effect_type_t> )
    return spell_data_t::find_spelleffect( *data, type, std::forward<Ts>( args )... );
  else
  {
    const spell_data_t* affected = resolve_spell_data<U>( type );

    if constexpr ( std::is_same_v<std::tuple_element_t<0, std::tuple<Ts...>>, effect_subtype_t> )
      return spell_data_t::find_spelleffect( *data, *affected, E_APPLY_AURA, std::forward<Ts>( args )... );
    else if constexpr ( std::is_same_v<std::tuple_element_t<0, std::tuple<Ts...>>, effect_type_t> )
      return spell_data_t::find_spelleffect( *data, *affected, std::forward<Ts>( args )... );
    else
      return spell_data_t::find_spelleffect( *data, *affected, E_APPLY_AURA );
  }

  assert( false && "Could not resolve find_effect argument to type/subtype." );
  return spelleffect_data_t::nil();
}

template <typename T, typename U, typename... Ts>
static size_t find_effect_index( T val, U type, Ts&&... args )
{
  return find_effect( val, type, std::forward<Ts>( args )... ).index() + 1;
}

// finds the first effect with a trigger spell
// argument can be either player_talent_t, spell_data_t*, buff_t*, action_t*
template <typename T>
static const spelleffect_data_t& find_trigger( T val )
{
  const spell_data_t* data = resolve_spell_data<T>( val );

  for ( const auto& eff : data->effects() )
  {
    switch ( eff.type() )
    {
      case E_TRIGGER_SPELL:
      case E_TRIGGER_SPELL_WITH_VALUE:
        return eff;
      case E_APPLY_AURA:
      case E_APPLY_AREA_AURA_PARTY:
        switch ( eff.subtype() )
        {
          case A_PROC_TRIGGER_SPELL:
          case A_PROC_TRIGGER_SPELL_WITH_VALUE:
          case A_PERIODIC_TRIGGER_SPELL:
          case A_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
            return eff;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }

  return spelleffect_data_t::nil();
}

using namespace unique_gear;

// Foorward Declarations
struct death_knight_t;
struct runes_t;
struct rune_t;

namespace pets
{
struct death_knight_pet_t;
struct ghoul_pet_t;
struct army_ghoul_pet_t;
struct gargoyle_pet_t;
struct risen_skulker_pet_t;
struct dancing_rune_weapon_pet_t;
struct everlasting_bond_pet_t;
struct bloodworm_pet_t;
struct magus_pet_t;
struct blood_beast_pet_t;
struct horseman_pet_t;
struct mograine_pet_t;
struct whitemane_pet_t;
struct trollbane_pet_t;
struct nazgrim_pet_t;
struct abomination_pet_t;
}  // namespace pets

namespace runeforge
{
void hysteria( special_effect_t& );
void apocalypse( special_effect_t& );
void fallen_crusader( special_effect_t& );
void razorice( special_effect_t& );
void sanguination( special_effect_t& );
void spellwarding( special_effect_t& );
void stoneskin_gargoyle( special_effect_t& );
void unending_thirst( special_effect_t& );  // Effect only procs on killing blows, NYI
}  // namespace runeforge

enum runeforge_apocalypse
{
  DEATH,
  FAMINE,
  PESTILENCE,
  WAR,
  MAX
};

// ==========================================================================
// Death Knight Runes ( part 1 )
// ==========================================================================

enum rune_state
{
  STATE_DEPLETED,
  STATE_REGENERATING,
  STATE_FULL
};

const double RUNIC_POWER_DECAY_RATE = 1.0;
const double RUNE_REGEN_BASE        = 10;
const double RUNE_REGEN_BASE_SEC    = ( 1 / RUNE_REGEN_BASE );

const size_t MAX_RUNES              = 6;
const size_t MAX_REGENERATING_RUNES = 3;
const double MAX_START_OF_COMBAT_RP = 20;

template <typename T>
struct dynamic_event_t : public event_t
{
  double m_coefficient;
  T** m_ptr;

  dynamic_event_t( sim_t* s ) : event_t( *s ), m_coefficient( 1.0 ), m_ptr( nullptr )
  {
  }

  const char* name() const override
  {
    return "Dynamic-Event-Base";
  }

  static T* create( sim_t* s )
  {
    return new ( *s ) T( s );
  }

  virtual T* clone() = 0;

  virtual timespan_t adjust( timespan_t by_time )
  {
    auto this_ = this;
    // Execute early and cancel the event, if the adjustment would trigger the event
    if ( by_time >= remains() )
    {
      execute();
      event_t::cancel( this_ );
      return 0_ms;
    }

    auto new_remains = remains() - by_time;

    sim().print_debug( "{} adjust time by {}, remains= {}, new_remains={}", name(), by_time.total_seconds(),
                       remains().total_seconds(), new_remains.total_seconds() );

    // Otherwise, just clone this event and schedule it with the new time, bypassing the coefficient
    // adjustment
    create_clone()->schedule( new_remains, false );

    event_t::cancel( this_ );
    return new_remains;
  }

  // Create a clone of this event
  virtual T* create_clone()
  {
    auto cloned = clone();
    if ( m_ptr )
    {
      cloned->ptr( *m_ptr );
      *m_ptr = cloned;
    }
    cloned->coefficient( m_coefficient );

    return cloned;
  }

  virtual void execute_event() = 0;

  // Update the duration coefficient, causing an adjustment to the remaining event duration to be
  // made
  virtual T* update_coefficient( double new_coefficient )
  {
    if ( new_coefficient == 0 || new_coefficient == m_coefficient )
    {
      return cast();
    }

    auto ratio        = new_coefficient / m_coefficient;
    auto remains      = this->remains();
    auto new_duration = remains * ratio;

    sim().print_debug( "{} coefficient change, remains={} old_coeff={} new_coeff={} ratio={} new_remains={}", name(),
                       remains.total_seconds(), m_coefficient, new_coefficient, ratio, new_duration.total_seconds() );

    // Duration increases, so reschedule the event
    if ( ratio > 1 )
    {
      reschedule( new_duration );
      m_coefficient = new_coefficient;
      return cast();
    }
    // Duration decreases, cannot reschedule so clone the event and schedule it with the new
    // remaining duration, bypassing the coefficient adjustment (since it is already included in the
    // duration)
    else
    {
      auto cloned = create_clone();
      cloned->coefficient( new_coefficient )->schedule( new_duration, false );

      // Cancel wants ref to a ptr, but we don't really care
      auto this_ = this;
      event_t::cancel( this_ );
      return cast( cloned );
    }
  }

  static T* cast( event_t* event )
  {
    return debug_cast<T*>( event );
  }

  T* cast() const
  {
    return debug_cast<T*>( this );
  }

  T* cast()
  {
    return debug_cast<T*>( this );
  }

  T* ptr( T*& value )
  {
    m_ptr = &value;
    return cast();
  }

  T* ptr() const
  {
    return m_ptr ? *m_ptr : nullptr;
  }

  T* coefficient( double value )
  {
    m_coefficient = value;
    return cast();
  }

  double coefficient() const
  {
    return m_coefficient;
  }

  // Actually schedules the event into the core event system, by default, applies the coefficient
  // associated with the event to the duration given
  T* schedule( timespan_t duration, bool use_coeff = true )
  {
    event_t::schedule( duration * ( use_coeff ? coefficient() : 1.0 ) );
    return cast();
  }

  void execute() override
  {
    execute_event();

    if ( m_ptr )
    {
      *m_ptr = nullptr;
    }
  }
};

struct rune_event_t : public dynamic_event_t<rune_event_t>
{
  using super = dynamic_event_t<rune_event_t>;

  rune_t* m_rune;

  rune_event_t( sim_t* sim ) : super( sim ), m_rune( nullptr )
  {
  }

  const char* name() const override
  {
    return "Rune-Regen-Event";
  }

  rune_event_t* clone() override
  {
    return create( &sim() )->rune( *m_rune );
  }

  rune_t* rune() const
  {
    return m_rune;
  }

  rune_event_t* rune( rune_t& r )
  {
    m_rune = &r;
    return this;
  }

  void execute_event() override;
};

struct rune_t
{
  runes_t* runes;          // Back reference to runes_t array so we can adjust rune state
  rune_state state;        // DEPLETED, REGENERATING, FULL
  rune_event_t* event;     // Regen event
  timespan_t regen_start;  // Start time of the regeneration
  timespan_t regenerated;  // Timestamp when rune regenerated to full

  rune_t() : runes( nullptr ), state( STATE_FULL ), event( nullptr ), regen_start( timespan_t::min() )
  {
  }

  rune_t( runes_t* r ) : runes( r ), state( STATE_FULL ), event( nullptr ), regen_start( timespan_t::min() )
  {
  }

  bool is_ready() const
  {
    return state == STATE_FULL;
  }
  bool is_regenerating() const
  {
    return state == STATE_REGENERATING;
  }
  bool is_depleted() const
  {
    return state == STATE_DEPLETED;
  }

  double fill_level() const;

  void update_coefficient();

  // Consume this rune and adjust internal rune state
  void consume();
  // Fill this rune and adjust internal rune state
  void fill_rune( gain_t* gain = nullptr );
  // Start regenerating the rune
  void start_regenerating();

  // Directly adjust regeneration by time units
  void adjust_regen_event( timespan_t adjustment );

  void reset()
  {
    regen_start = timespan_t::min();
    regenerated = 0_ms;
    event       = nullptr;
    state       = STATE_FULL;
  }
};

struct runes_t
{
  death_knight_t* dk;
  std::array<rune_t, MAX_RUNES> slot;
  timespan_t waste_start;
  // Cumulative waste per iteration in seconds
  extended_sample_data_t cumulative_waste;
  // Individual waste times per rune in seconds
  extended_sample_data_t rune_waste;
  // Per iteration waste counter, added into cumulative_waste on reset
  timespan_t iteration_waste_sum;
  // Cached regenerating runes array
  std::vector<const rune_t*> regenerating_runes;

  runes_t( death_knight_t* p );

  void update_coefficient()
  {
    range::for_each( slot, []( rune_t& r ) { r.update_coefficient(); } );
  }

  void reset()
  {
    range::for_each( slot, []( rune_t& r ) { r.reset(); } );

    waste_start = 0_ms;
    if ( iteration_waste_sum > 0_ms )
    {
      cumulative_waste.add( iteration_waste_sum.total_seconds() );
    }
    iteration_waste_sum = 0_ms;
  }

  std::string string_representation() const
  {
    std::string rune_str;
    std::string rune_val_str;

    for ( const auto& rune : slot )
    {
      char rune_letter;
      if ( rune.is_ready() )
      {
        rune_letter = 'F';
      }
      else if ( rune.is_depleted() )
      {
        rune_letter = 'd';
      }
      else
      {
        rune_letter = 'r';
      }

      std::string rune_val = util::to_string( rune.fill_level(), 2 );

      rune_str += rune_letter;
      rune_val_str += '[' + rune_val + ']';
    }
    return rune_str + " " + rune_val_str;
  }

  // Return the number of runes in specific state
  unsigned runes_in_state( rune_state s ) const
  {
    return std::accumulate( slot.begin(), slot.end(), 0U,
                            [ s ]( const unsigned& v, const rune_t& r ) { return v + ( r.state == s ); } );
  }

  // Return the first rune in a specific state. If no rune in specific state found, return nullptr.
  rune_t* first_rune_in_state( rune_state s )
  {
    auto it = range::find_if( slot, [ s ]( const rune_t& rune ) { return rune.state == s; } );
    if ( it != slot.end() )
    {
      return &( *it );
    }

    return nullptr;
  }

  unsigned runes_regenerating() const
  {
    return runes_in_state( STATE_REGENERATING );
  }

  unsigned runes_depleted() const
  {
    return runes_in_state( STATE_DEPLETED );
  }

  unsigned runes_full() const
  {
    return runes_in_state( STATE_FULL );
  }

  rune_t* first_depleted_rune()
  {
    return first_rune_in_state( STATE_DEPLETED );
  }

  rune_t* first_regenerating_rune()
  {
    return first_rune_in_state( STATE_REGENERATING );
  }

  rune_t* first_full_rune()
  {
    return first_rune_in_state( STATE_FULL );
  }

  void consume( unsigned runes );

  // Perform seconds of rune regeneration time instantaneously
  void regenerate_immediate( timespan_t seconds );

  // Time it takes with the current rune regeneration speed to regenerate n_runes by the Death
  // Knight.
  timespan_t time_to_regen( unsigned n_runes );
};

// ==========================================================================
// Death Knight
// ==========================================================================

struct death_knight_td_t : public actor_target_data_t
{
  struct dots_t
  {
    // Blood
    propagate_const<dot_t*> blood_plague;
    // Frost
    propagate_const<dot_t*> frost_fever;
    // Unholy
    propagate_const<dot_t*> soul_reaper;
    propagate_const<dot_t*> virulent_plague;
    propagate_const<dot_t*> unholy_blight;

    // Rider of the Apocalypse
    propagate_const<dot_t*> undeath;
  } dot;

  struct debuffs_t
  {
    // Runeforges
    propagate_const<buff_t*> apocalypse_death;  // Dummy debuff, healing reduction not implemented
    propagate_const<buff_t*> apocalypse_war;
    propagate_const<buff_t*> apocalypse_famine;
    buff_t* razorice;

    // General Talents
    propagate_const<buff_t*> brittle;

    // Blood
    propagate_const<buff_t*> mark_of_blood;

    // Frost
    propagate_const<buff_t*> piercing_chill;
    propagate_const<buff_t*> everfrost;
    propagate_const<buff_t*> chill_streak;

    // Unholy
    propagate_const<buff_t*> festering_wound;
    propagate_const<buff_t*> rotten_touch;
    propagate_const<buff_t*> death_rot;
    propagate_const<buff_t*> unholy_aura;
    buff_t* decomposition;

    // Rider of the Apocalypse
    propagate_const<buff_t*> chains_of_ice_trollbane_slow;
    propagate_const<buff_t*> chains_of_ice_trollbane_damage;

    // Deathbringer
    buff_t* reapers_mark;
    buff_t* wave_of_souls;

    // San'layn
    propagate_const<buff_t*> incite_terror;
  } debuff;

  death_knight_td_t( player_t& target, death_knight_t& p );

  template <typename Buff = buff_t, typename... Args>
  inline buff_t* make_debuff( bool b, Args&&... args );
};

using data_t        = std::pair<std::string, simple_sample_data_with_min_max_t>;
using simple_data_t = std::pair<std::string, simple_sample_data_t>;

// utility to create target_effect_t compatible functions from death_knight_td_t member references
template <typename T>
static std::function<int( actor_target_data_t* )> d_fn( T d, bool stack = true )
{
  if constexpr ( std::is_invocable_v<T, death_knight_td_t::debuffs_t> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<death_knight_td_t*>( t )->debuff )->check();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<death_knight_td_t*>( t )->debuff )->check() > 0;
      };
  }
  else if constexpr ( std::is_invocable_v<T, death_knight_td_t::dots_t> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<death_knight_td_t*>( t )->dot )->current_stack();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<death_knight_td_t*>( t )->dot )->is_ticking();
      };
  }
  else
  {
    static_assert( static_false<T>, "Not a valid member of death_knight_td_t" );
    return nullptr;
  }
}

struct death_knight_t : public parse_player_effects_t
{
public:
  // Stores the currently active death and decay ground event
  ground_aoe_event_t* active_dnd;
  event_t* runic_power_decay;

  // Expression warnings
  // for old dot.death_and_decay.x expressions
  bool deprecated_dnd_expression;
  // for runeforge.name expressions that call death knight runeforges instead of shadowlands legendary runeforges
  bool runeforge_expression_warning;

  // Counters
  unsigned int km_proc_attempts;               // critical auto attacks since the last KM proc
  unsigned int festering_wounds_target_count;  // cached value of the current number of enemies affected by FW
  unsigned int
      bone_shield_charges_consumed;  // Counts how many bone shield charges have been consumed for T29 4pc blood
  unsigned int active_riders;        // Number of active Riders of the Apocalypse pets

  std::vector<player_t*> undeath_tl;

  // Buffs
  struct buffs_t
  {
    // Shared
    buff_t* antimagic_shell;
    buff_t* antimagic_shell_horsemen;
    buff_t* antimagic_shell_horsemen_icd;
    buff_t* antimagic_zone;
    propagate_const<buff_t*> blood_draw;
    propagate_const<buff_t*> icebound_fortitude;
    propagate_const<buff_t*> rune_mastery;
    propagate_const<buff_t*> unholy_ground;
    propagate_const<buff_t*> icy_talons;
    propagate_const<buff_t*> abomination_limb;
    propagate_const<buff_t*> lichborne;  // NYI
    buff_t* frost_shield;
    propagate_const<buff_t*> death_and_decay;

    // Runeforges
    propagate_const<buff_t*> rune_of_hysteria;
    propagate_const<buff_t*> stoneskin_gargoyle;
    propagate_const<buff_t*> unholy_strength;  // runeforge of the fallen crusader

    // Blood
    absorb_buff_t* blood_shield;
    propagate_const<buff_t*> bloodied_blade_stacks;
    propagate_const<buff_t*> bloodied_blade_final;
    buff_t* bone_shield;
    propagate_const<buff_t*> coagulopathy;
    propagate_const<buff_t*> consumption;
    propagate_const<buff_t*> crimson_scourge;
    propagate_const<buff_t*> dancing_rune_weapon;
    propagate_const<buff_t*> heartrend;
    propagate_const<buff_t*> hemostasis;
    propagate_const<buff_t*> ossuary;
    buff_t* ossified_vitriol;
    propagate_const<buff_t*> perseverance_of_the_ebon_blade;
    propagate_const<buff_t*> rune_tap;
    propagate_const<buff_t*> sanguine_ground;
    propagate_const<buff_t*> tombstone;
    propagate_const<buff_t*> vampiric_blood;
    propagate_const<buff_t*> voracious;
    // Tier Sets
    propagate_const<buff_t*> unbreakable_tww1_2pc;
    buff_t* unbroken_tww1_2pc;
    buff_t* piledriver_tww1_4pc;

    // Frost
    propagate_const<buff_t*> breath_of_sindragosa;
    propagate_const<buff_t*> cold_heart;
    propagate_const<buff_t*> gathering_storm;
    propagate_const<buff_t*> inexorable_assault;
    propagate_const<buff_t*> empower_rune_weapon;
    propagate_const<buff_t*> killing_machine;
    buff_t* pillar_of_frost;
    propagate_const<buff_t*> remorseless_winter;
    propagate_const<buff_t*> rime;
    propagate_const<buff_t*> unleashed_frenzy;
    propagate_const<buff_t*> bonegrinder_crit;
    propagate_const<buff_t*> bonegrinder_frost;
    propagate_const<buff_t*> enduring_strength_builder;
    propagate_const<buff_t*> enduring_strength;
    propagate_const<buff_t*> frostwhelps_aid;
    buff_t* cryogenic_chamber;
    // Tier Sets
    propagate_const<buff_t*> icy_vigor;

    // Unholy
    propagate_const<buff_t*> dark_transformation;
    propagate_const<buff_t*> runic_corruption;
    propagate_const<buff_t*> sudden_doom;
    propagate_const<buff_t*> unholy_assault;
    propagate_const<buff_t*> unholy_pact;
    propagate_const<buff_t*> festermight;
    propagate_const<buff_t*> ghoulish_frenzy;
    propagate_const<buff_t*> plaguebringer;
    propagate_const<buff_t*> commander_of_the_dead;
    propagate_const<buff_t*> festering_scythe;
    propagate_const<buff_t*> festering_scythe_stacks;
    // Tier Sets
    propagate_const<buff_t*> unholy_commander;

    // Rider of the Apocalypse
    propagate_const<buff_t*> a_feast_of_souls;
    buff_t* apocalyptic_conquest;
    propagate_const<buff_t*> mograines_might;

    // San'layn
    propagate_const<buff_t*> bloodsoaked_ground;
    propagate_const<buff_t*> essence_of_the_blood_queen;
    propagate_const<buff_t*> essence_of_the_blood_queen_damage;
    buff_t* gift_of_the_sanlayn;
    propagate_const<buff_t*> vampiric_strike;
    propagate_const<buff_t*> infliction_of_sorrow;
    propagate_const<buff_t*> visceral_strength;

    // Deathbringer
    propagate_const<buff_t*> grim_reaper;
    propagate_const<buff_t*> bind_in_darkness;
    propagate_const<buff_t*> dark_talons_shadowfrost;
    propagate_const<buff_t*> dark_talons_icy_talons;
    propagate_const<buff_t*> exterminate;
    propagate_const<buff_t*> exterminate_painful_death;

  } buffs;

  struct runeforge_t
  {
    bool rune_of_hysteria;
    bool rune_of_apocalypse;
    // Razorice has one for each weapon because they don't proc from the same abilities
    bool rune_of_razorice_mh, rune_of_razorice_oh;
    bool rune_of_sanguination;
    bool rune_of_the_fallen_crusader;
    bool rune_of_the_stoneskin_gargoyle;
    bool rune_of_unending_thirst;

    // Spellwarding is simpler to store as a double to check whether it's stacked or not
    double rune_of_spellwarding;
  } runeforge;

  // Cooldowns
  struct cooldowns_t
  {
    // Shared
    propagate_const<cooldown_t*>
        death_and_decay_dynamic;  // Shared cooldown object for death and decay, defile and death's due
    propagate_const<cooldown_t*> death_grip;
    propagate_const<cooldown_t*> mind_freeze;

    // Blood
    cooldown_t* bone_shield_icd;  // internal cooldown between bone shield stack consumption
    cooldown_t* blood_tap;
    cooldown_t* blooddrinker;
    cooldown_t* consumption;
    cooldown_t* dancing_rune_weapon;
    propagate_const<cooldown_t*> vampiric_blood;
    // Frost
    propagate_const<cooldown_t*> inexorable_assault_icd;  // internal cooldown to prevent multiple procs during aoe
    propagate_const<cooldown_t*>
        frigid_executioner_icd;  // internal cooldown that prevents several procs on the same dual-wield attack
    propagate_const<cooldown_t*>
        enduring_strength_icd;  // internal cooldown that prevents several procs on the same dual-wield attacl
    propagate_const<cooldown_t*> pillar_of_frost;
    propagate_const<cooldown_t*> frostwyrms_fury;
    propagate_const<cooldown_t*> chill_streak;
    propagate_const<cooldown_t*> empower_rune_weapon;
    propagate_const<cooldown_t*> frostscythe;

    // Unholy
    propagate_const<cooldown_t*> apocalypse;
    propagate_const<cooldown_t*> army_of_the_dead;
    propagate_const<cooldown_t*> dark_transformation;
    propagate_const<cooldown_t*> vile_contagion;

    // Rider of the Apocalypse
    propagate_const<target_specific_cooldown_t*> undeath_spread;
    propagate_const<cooldown_t*> whitemane_ams_cd;
    propagate_const<cooldown_t*> trollbane_ams_cd;
    propagate_const<cooldown_t*> nazgrim_ams_cd;
    propagate_const<cooldown_t*> mograine_ams_cd;

  } cooldown;

  // Active Spells
  struct active_spells_t
  {
    // Shared
    propagate_const<action_t*> runeforge_pestilence;
    propagate_const<action_t*> runeforge_razorice;
    propagate_const<action_t*> runeforge_sanguination;
    action_t* abomination_limb_damage;

    // Class Tree
    propagate_const<action_t*> blood_draw;

    // Rider of the Apocalypse
    action_t* summon_whitemane;
    action_t* summon_trollbane;
    action_t* summon_nazgrim;
    action_t* summon_mograine;
    action_t* last_rider_summoned;
    std::vector<action_t*> rider_summon_spells;
    propagate_const<action_t*> undeath_dot;
    propagate_const<action_t*> trollbanes_icy_fury;
    propagate_const<action_t*> mograines_death_and_decay;

    // Sanlayn
    propagate_const<action_t*> vampiric_strike_heal;
    action_t* infliction_of_sorrow;
    action_t* the_blood_is_life;

    // Blood
    propagate_const<action_t*> mark_of_blood_heal;
    action_t* shattering_bone;
    action_t* heart_strike_bloodied_blade;
    propagate_const<action_t*> soul_reaper_execute_expired_drw;

    // Deathbringer
    action_t* reapers_mark_explosion;
    action_t* wave_of_souls;
    action_t* soul_rupture;
    action_t* grim_reaper_soul_reaper;
    action_t* exterminate;

    // Frost
    action_t* breath_of_sindragosa_tick;
    action_t* remorseless_winter_tick;
    action_t* frost_strike_main;
    action_t* frost_strike_offhand;
    action_t* frost_strike_sb_main;
    action_t* frost_strike_sb_offhand;
    action_t* chill_streak_damage;
    propagate_const<action_t*> icy_death_torrent_damage;
    action_t* hyperpyrexia_damage;

    // Unholy
    propagate_const<action_t*> bursting_sores;
    propagate_const<action_t*> festering_wound;
    propagate_const<action_t*> festering_wound_application;
    propagate_const<action_t*> virulent_eruption;
    propagate_const<action_t*> virulent_plague;
    propagate_const<action_t*> ruptured_viscera;
    propagate_const<action_t*> outbreak_aoe;
    propagate_const<action_t*> unholy_blight;
    action_t* unholy_pact_damage;
    action_t* decomposition_damage;

  } active_spells;

  // Gains
  struct gains_t
  {
    // Shared
    propagate_const<gain_t*> rune_of_hysteria;
    propagate_const<gain_t*> antimagic_shell;  // RP from magic damage absorbed
    gain_t* rune;                              // Rune regeneration
    propagate_const<gain_t*> coldthirst;
    propagate_const<gain_t*> start_of_combat_overflow;

    // Blood
    propagate_const<gain_t*> bonestorm;
    propagate_const<gain_t*> blood_tap;
    propagate_const<gain_t*> consumption;
    propagate_const<gain_t*> drw_heart_strike;  // Blood Strike, Blizzard's hack to replicate HS rank 2 with DRW
    propagate_const<gain_t*> heartbreaker;
    propagate_const<gain_t*> tombstone;

    // Frost
    propagate_const<gain_t*> breath_of_sindragosa;
    propagate_const<gain_t*> empower_rune_weapon;
    propagate_const<gain_t*> frost_fever;  // RP generation per tick
    propagate_const<gain_t*> horn_of_winter;
    propagate_const<gain_t*> frigid_executioner;  // Rune refund chance
    propagate_const<gain_t*> murderous_efficiency;
    propagate_const<gain_t*> obliteration;
    propagate_const<gain_t*> rage_of_the_frozen_champion;
    propagate_const<gain_t*> runic_attenuation;
    propagate_const<gain_t*> runic_empowerment;

    // Unholy
    propagate_const<gain_t*> apocalypse;
    propagate_const<gain_t*> festering_wound;

    // Rider of the Apocalypse
    propagate_const<gain_t*> antimagic_shell_horsemen;  // RP from magic damage absorbed
  } gains;

  // Specialization
  struct specialization_t
  {
    // Class/spec auras
    const spell_data_t* death_knight;
    const spell_data_t* death_knight_2;
    const spell_data_t* plate_specialization;
    const spell_data_t* riposte;
    const spell_data_t* blood_fortification;
    const spell_data_t* blood_death_knight;
    const spell_data_t* blood_death_knight_2;
    const spell_data_t* frost_death_knight;
    const spell_data_t* frost_death_knight_2;
    const spell_data_t* unholy_death_knight;
    const spell_data_t* unholy_death_knight_2;

    // Shared
    const spell_data_t* frost_fever;  // The RP energize spell is a spec ability in spelldata
    const spell_data_t* death_coil;
    const spell_data_t* death_grip;
    const spell_data_t* dark_command;
    const spell_data_t* death_and_decay;
    const spell_data_t* rune_strike;
    const spell_data_t* antimagic_shell;
    const spell_data_t* chains_of_ice;

    // Blood
    const spell_data_t* crimson_scourge;
    const spell_data_t* deaths_caress;

    // Frost
    const spell_data_t* remorseless_winter;
    const spell_data_t* might_of_the_frozen_wastes;
    const spell_data_t* frostreaper;
    const spell_data_t* rime;

    // Unholy
    const spell_data_t* dark_transformation_2;
    const spell_data_t* festering_wound;
    const spell_data_t* outbreak;
    const spell_data_t* epidemic;

  } spec;

  // Mastery
  struct mastery_t
  {
    const spell_data_t* blood_shield;  // Blood
    const spell_data_t* frozen_heart;  // Frost
    const spell_data_t* dreadblade;    // Unholy
  } mastery;

  // Talents
  struct talents_t
  {
    // Shared Class Tree
    player_talent_t icebound_fortitude;
    player_talent_t death_strike;
    player_talent_t raise_dead;
    // Row 2
    player_talent_t runic_attenuation;
    player_talent_t improved_death_strike;
    player_talent_t cleaving_strikes;
    // Row 3
    player_talent_t mindfreeze;
    player_talent_t blinding_sleet;  // NYI
    player_talent_t antimagic_barrier;
    player_talent_t march_of_darkness;  // NYI
    player_talent_t unholy_ground;
    player_talent_t control_undead;  // NYI
    player_talent_t enfeeble;        // NYI
    player_talent_t sacrificial_pact;
    // Row 4
    player_talent_t coldthirst;
    player_talent_t proliferating_chill;
    player_talent_t permafrost;
    player_talent_t veteran_of_the_third_war;
    player_talent_t death_pact;  // NYI
    player_talent_t brittle;
    player_talent_t deaths_reach;  // NYI
    // Row 5
    player_talent_t icy_talons;
    player_talent_t antimagic_zone;
    player_talent_t unholy_bond;
    // Row 6
    player_talent_t ice_prison;  // NYI
    player_talent_t gloom_ward;
    player_talent_t asphyxiate;  // NYI
    player_talent_t assimilation;
    player_talent_t wraith_walk;       // NYI
    player_talent_t grip_of_the_dead;  // NYI
    player_talent_t soul_reaper;
    // Row 7
    player_talent_t suppression;  // NYI
    player_talent_t blood_scent;
    player_talent_t unholy_endurance;  // NYI
    // Row 8
    player_talent_t osmosis;
    player_talent_t insidious_chill;  // NYI
    player_talent_t runic_protection;
    player_talent_t blood_draw;
    // Row 9
    player_talent_t rune_mastery;
    player_talent_t subuding_grasp;          // NYI
    player_talent_t will_of_the_necropolis;  // NYI
    // Row 10
    player_talent_t null_magic;  // NYI
    player_talent_t unyielding_will;
    player_talent_t abomination_limb;
    player_talent_t deaths_echo;
    player_talent_t vestigial_shell;  // NYI

    // Blood
    struct
    {
      // Row 1
      player_talent_t heart_strike;
      // Row 2
      player_talent_t marrowrend;
      player_talent_t blood_boil;
      // Row 3
      player_talent_t vampiric_blood;
      player_talent_t bone_collector;
      // Row 4
      player_talent_t ossuary;
      player_talent_t improved_vampiric_blood;
      player_talent_t improved_heart_strike;
      player_talent_t ossified_vitriol;
      // Row 5
      player_talent_t leeching_strike;
      player_talent_t heartbreaker;
      player_talent_t foul_bulwark;
      player_talent_t dancing_rune_weapon;
      player_talent_t hemostasis;
      player_talent_t perseverance_of_the_ebon_blade;
      player_talent_t relish_in_blood;
      // Row 6
      player_talent_t rune_tap;
      player_talent_t gorefiends_grasp;
      player_talent_t insatiable_blade;
      player_talent_t reinforced_bones;
      player_talent_t rapid_decomposition;
      // Row 7
      player_talent_t blood_tap;
      player_talent_t improved_bone_shield;
      player_talent_t tightening_grasp;
      player_talent_t everlasting_bond;
      player_talent_t voracious;
      player_talent_t coagulopathy;
      player_talent_t bloodworms;
      // Row 8
      player_talent_t blood_feast;
      player_talent_t mark_of_blood;
      player_talent_t tombstone;
      player_talent_t blooddrinker;
      player_talent_t consumption;
      player_talent_t bloodied_blade;
      player_talent_t sanguine_ground;
      // Row 9
      player_talent_t shattering_bone;
      player_talent_t heartrend;
      player_talent_t carnage;
      player_talent_t iron_heart;
      player_talent_t red_thirst;
      // Row 10
      player_talent_t bonestorm;
      player_talent_t purgatory;
      player_talent_t bloodshot;
      player_talent_t umbilicus_eternus;
    } blood;

    // Frost
    struct
    {
      // Row 1
      player_talent_t frost_strike;
      // Row 2
      player_talent_t obliterate;
      player_talent_t howling_blast;
      // Row 3
      player_talent_t killing_machine;
      player_talent_t everfrost;
      // Row 4
      player_talent_t unleashed_frenzy;
      player_talent_t runic_command;
      player_talent_t improved_frost_strike;
      player_talent_t improved_rime;
      // Row 5
      player_talent_t improved_obliterate;
      player_talent_t glacial_advance;
      player_talent_t pillar_of_frost;
      player_talent_t frostscythe;
      player_talent_t biting_cold;
      // Row 6
      player_talent_t rage_of_the_frozen_champion;
      player_talent_t frigid_executioner;
      player_talent_t cold_heart;
      player_talent_t horn_of_winter;
      player_talent_t enduring_strength;
      player_talent_t icecap;
      player_talent_t frostwhelps_aid;
      player_talent_t empower_rune_weapon;
      player_talent_t chill_streak;
      // Row 7
      player_talent_t murderous_efficiency;
      player_talent_t inexorable_assault;
      player_talent_t frostwyrms_fury;
      player_talent_t gathering_storm;
      player_talent_t cryogenic_chamber;
      player_talent_t enduring_chill;
      player_talent_t piercing_chill;
      // Row 8
      player_talent_t bonegrinder;
      player_talent_t smothering_offense;
      player_talent_t absolute_zero;
      player_talent_t avalanche;
      player_talent_t icebreaker;
      // Row 9
      player_talent_t obliteration;
      player_talent_t icy_death_torrent;
      player_talent_t shattering_blade;
      player_talent_t hyperpyrexia;
      // Row 10
      player_talent_t arctic_assault;
      player_talent_t the_long_winter;
      player_talent_t shattered_frost;
      player_talent_t breath_of_sindragosa;
    } frost;

    // Unholy
    struct
    {
      // Row 1
      player_talent_t festering_strike;
      // Row 2
      player_talent_t scourge_strike;
      player_talent_t raise_dead;
      // Row 3
      player_talent_t sudden_doom;
      player_talent_t dark_transformation;
      // Row 4
      player_talent_t foul_infections;
      player_talent_t improved_festering_strike;
      player_talent_t runic_mastery;
      player_talent_t eternal_agony;
      // Row 5
      player_talent_t defile;
      player_talent_t unholy_blight;
      player_talent_t festering_scythe;
      player_talent_t apocalypse;
      player_talent_t plaguebringer;
      player_talent_t clawing_shadows;
      player_talent_t reaping;
      player_talent_t all_will_serve;
      // Row 6
      player_talent_t ebon_fever;
      player_talent_t bursting_sores;
      player_talent_t ghoulish_frenzy;
      player_talent_t magus_of_the_dead;
      player_talent_t improved_death_coil;
      player_talent_t harbinger_of_doom;
      player_talent_t unholy_pact;
      // Row 7
      player_talent_t vile_contagion;
      player_talent_t pestilence;
      player_talent_t infected_claws;
      player_talent_t menacing_magus;
      player_talent_t coil_of_devastation;
      player_talent_t rotten_touch;
      player_talent_t ruptured_viscera;
      // Row 8
      player_talent_t death_rot;
      player_talent_t army_of_the_dead;
      player_talent_t summon_gargoyle;
      player_talent_t doomed_bidding;
      // Row 9
      player_talent_t morbidity;
      player_talent_t festermight;
      player_talent_t raise_abomination;
      player_talent_t decomposition;
      player_talent_t unholy_aura;
      // Row 10
      player_talent_t superstrain;
      player_talent_t unholy_assault;
      player_talent_t commander_of_the_dead;
    } unholy;

    // Rider of the Apocalypse
    struct
    {
      player_talent_t riders_champion;
      player_talent_t on_a_paler_horse;  // NYI
      player_talent_t death_charge;      // NYI
      player_talent_t mograines_might;
      player_talent_t horsemens_aid;
      player_talent_t pact_of_the_apocalypse;  // NYI
      player_talent_t whitemanes_famine;
      player_talent_t nazgrims_conquest;
      player_talent_t trollbanes_icy_fury;
      player_talent_t hungering_thirst;
      player_talent_t fury_of_the_horsemen;
      player_talent_t a_feast_of_souls;
      player_talent_t mawsworn_menace;
      player_talent_t apocalypse_now;
    } rider;

    // Deathbringer
    struct
    {
      player_talent_t reapers_mark;
      player_talent_t wave_of_souls;
      player_talent_t blood_fever;
      player_talent_t bind_in_darkness;
      player_talent_t soul_rupture;
      player_talent_t grim_reaper;
      player_talent_t pact_of_the_deathbringer; // NYI
      player_talent_t rune_carved_plates;       // NYI
      player_talent_t swift_end;
      player_talent_t painful_death;
      player_talent_t dark_talons;
      player_talent_t wither_away;
      player_talent_t deaths_messenger;
      player_talent_t expelling_shield;  // NYI
      player_talent_t exterminate;
    } deathbringer;

    // San'layn
    struct
    {
      player_talent_t vampiric_strike;
      player_talent_t newly_turned;    // NYI
      player_talent_t vampiric_speed;  // NYI
      player_talent_t bloodsoaked_ground;
      player_talent_t vampiric_aura;     // NYI
      player_talent_t bloody_fortitude;  // NYI
      player_talent_t infliction_of_sorrow;
      player_talent_t frenzied_bloodthirst;
      player_talent_t the_blood_is_life;
      player_talent_t visceral_strength;
      player_talent_t incite_terror;
      player_talent_t pact_of_the_sanlayn;
      player_talent_t sanguine_scent;
      player_talent_t gift_of_the_sanlayn;
    } sanlayn;
  } talent;

  // Spells
  struct spells_t
  {
    // Shared
    const spell_data_t* brittle_debuff;
    const spell_data_t* dnd_buff;  // obliterate aoe increase while in death's due (nf covenant ability)
    const spell_data_t* razorice_debuff;
    const spell_data_t* rune_mastery_buff;
    const spell_data_t* coldthirst_gain;  // Coldthirst has a unique ID for the gain and cooldown reduction
    const spell_data_t* unholy_strength_buff;
    const spell_data_t* unholy_ground_buff;
    const spell_data_t* apocalypse_death_debuff;
    const spell_data_t* apocalypse_famine_debuff;
    const spell_data_t* apocalypse_war_debuff;
    const spell_data_t* apocalypse_pestilence_damage;
    const spell_data_t* razorice_damage;
    const spell_data_t* death_and_decay_damage;
    const spell_data_t* death_coil_damage;
    const spell_data_t* death_strike_heal;
    const spell_data_t* sacrificial_pact_damage;
    const spell_data_t* soul_reaper_execute;
    const spell_data_t* sanguination_cooldown;
    const spell_data_t* spellwarding_absorb;
    const spell_data_t* anti_magic_zone_buff;
    const spell_data_t* frost_shield_buff;
    const spell_data_t* rune_of_hysteria_buff;

    // Diseases (because they're not stored in spec data, unlike frost fever's rp gen...)
    const spell_data_t* blood_plague;
    const spell_data_t* frost_fever;
    const spell_data_t* virulent_plague;
    const spell_data_t* virulent_erruption;

    // Blood
    const spell_data_t* blood_shield;
    const spell_data_t* bloodied_blade_stacks_buff;
    const spell_data_t* bloodied_blade_final_buff;
    const spell_data_t* bone_shield;
    const spell_data_t* sanguine_ground;
    const spell_data_t* ossuary_buff;
    const spell_data_t* ossified_vitriol_buff;
    const spell_data_t* crimson_scourge_buff;
    const spell_data_t* heartbreaker_rp_gain;
    const spell_data_t* heartrend_buff;
    const spell_data_t* heart_strike_bloodied_blade;
    const spell_data_t* perserverence_of_the_ebon_blade_buff;
    const spell_data_t* voracious_buff;
    const spell_data_t* blood_draw_damage;
    const spell_data_t* blood_draw_cooldown;
    const spell_data_t* bonestorm_heal;
    const spell_data_t* dancing_rune_weapon_buff;
    const spell_data_t* relish_in_blood_gains;
    const spell_data_t* leeching_strike_damage;
    const spell_data_t* shattering_bone_damage;
    // Tier Sets
    const spell_data_t* unbreakable_tww1_2pc;
    const spell_data_t* unbroken_tww1_2pc;
    const spell_data_t* piledriver_tww1_4pc;

    // Frost
    const spell_data_t* runic_empowerment_gain;
    const spell_data_t* murderous_efficiency_gain;
    const spell_data_t* rage_of_the_frozen_champion;  // RP generation spell
    const spell_data_t* piercing_chill_debuff;
    const spell_data_t* runic_empowerment_chance;
    const spell_data_t* gathering_storm_buff;
    const spell_data_t* inexorable_assault_buff;
    const spell_data_t* bonegrinder_crit_buff;
    const spell_data_t* bonegrinder_frost_buff;
    const spell_data_t* enduring_strength_buff;
    const spell_data_t* frostwhelps_aid_buff;
    const spell_data_t* inexorable_assault_damage;
    const spell_data_t* breath_of_sindragosa_rune_gen;
    const spell_data_t* cold_heart_damage;
    const spell_data_t* chill_streak_damage;
    const spell_data_t* death_strike_offhand;
    const spell_data_t* frostwyrms_fury_damage;
    const spell_data_t* glacial_advance_damage;
    const spell_data_t* avalanche_damage;
    const spell_data_t* frostwhelps_aid_damage;
    const spell_data_t* enduring_strength_cooldown;
    const spell_data_t* frost_strike_2h;
    const spell_data_t* frost_strike_mh;
    const spell_data_t* frost_strike_oh;
    const spell_data_t* obliteration_gains;
    const spell_data_t* shattered_frost;
    const spell_data_t* icy_death_torrent_damage;
    const spell_data_t* cryogenic_chamber_damage;
    const spell_data_t* cryogenic_chamber_buff;
    const spell_data_t* rime_buff;
    const spell_data_t* hyperpyrexia_damage;
    // Tier Sets
    const spell_data_t* icy_vigor;

    // Unholy
    const spell_data_t* runic_corruption;  // buff
    const spell_data_t* runic_corruption_chance;
    const spell_data_t* festering_wound_debuff;
    const spell_data_t* rotten_touch_debuff;
    const spell_data_t* death_rot_debuff;
    const spell_data_t* coil_of_devastation_debuff;
    const spell_data_t* ghoulish_frenzy_player;
    const spell_data_t* plaguebringer_buff;
    const spell_data_t* festermight_buff;
    const spell_data_t* unholy_blight;
    const spell_data_t* unholy_blight_dot;
    const spell_data_t* commander_of_the_dead;
    const spell_data_t* ruptured_viscera_chance;
    const spell_data_t* apocalypse_duration;
    const spell_data_t* apocalypse_rune_gen;
    const spell_data_t* unholy_pact_damage;
    const spell_data_t* dark_transformation_damage;
    const spell_data_t* defile_damage;
    const spell_data_t* epidemic_damage;
    const spell_data_t* bursting_sores_damage;
    const spell_data_t* festering_wound_damage;
    const spell_data_t* outbreak_aoe;
    const spell_data_t* unholy_aura_debuff;
    const spell_data_t* decomposition_buff;
    const spell_data_t* decomposition_damage;
    const spell_data_t* festering_scythe;
    const spell_data_t* festering_scythe_buff;
    const spell_data_t* festering_scythe_stacking_buff;
    // Tier Sets
    const spell_data_t* unholy_commander;

    // Rider of the Apocalypse non-talent spells
    const spell_data_t* a_feast_of_souls_buff;
    const spell_data_t* summon_mograine;
    const spell_data_t* summon_whitemane;
    const spell_data_t* summon_trollbane;
    const spell_data_t* summon_nazgrim;
    const spell_data_t* summon_mograine_2;
    const spell_data_t* summon_whitemane_2;
    const spell_data_t* summon_trollbane_2;
    const spell_data_t* summon_nazgrim_2;
    const spell_data_t* apocalypse_now_data;
    const spell_data_t* death_charge_action;

    // San'layn non-talent spells
    const spell_data_t* vampiric_strike;
    const spell_data_t* vampiric_strike_buff;
    const spell_data_t* essence_of_the_blood_queen_buff;
    const spell_data_t* gift_of_the_sanlayn_buff;
    const spell_data_t* vampiric_strike_heal;
    const spell_data_t* infliction_of_sorrow_damage;
    const spell_data_t* infliction_of_sorrow_buff;
    const spell_data_t* blood_beast_summon;
    const spell_data_t* vampiric_strike_clawing_shadows;
    const spell_data_t* incite_terror_debuff;
    const spell_data_t* visceral_strength_buff;
    const spell_data_t* bloodsoaked_ground_buff;

    // Deathbringer spells
    const spell_data_t* reapers_mark_debuff;
    const spell_data_t* reapers_mark_explosion;
    const spell_data_t* grim_reaper;
    const spell_data_t* wave_of_souls_damage;
    const spell_data_t* wave_of_souls_debuff;
    const spell_data_t* blood_fever_damage;
    const spell_data_t* bind_in_darkness_buff;
    const spell_data_t* dark_talons_shadowfrost_buff;
    const spell_data_t* dark_talons_icy_talons_buff;
    const spell_data_t* soul_rupture_damage;
    const spell_data_t* grim_reaper_soul_reaper;
    const spell_data_t* exterminate_damage;
    const spell_data_t* exterminate_aoe;
    const spell_data_t* exterminate_buff;
    const spell_data_t* exterminate_buff_painful_death;

  } spell;

  // Pet Abilities
  struct pet_spells_t
  {
    // Raise dead ghoul
    const spell_data_t* ghoul_claw;
    const spell_data_t* sweeping_claws;
    const spell_data_t* gnaw;
    const spell_data_t* monstrous_blow;
    // Army of the dead
    const spell_data_t* army_claw;
    // All Ghouls
    const spell_data_t* pet_stun;
    // Gargoyle
    const spell_data_t* gargoyle_strike;
    const spell_data_t* dark_empowerment;
    // All Will Serve
    const spell_data_t* skulker_shot;
    // Magus of the Dead
    const spell_data_t* frostbolt;
    const spell_data_t* shadow_bolt;
    // Commander of the Dead Talent
    const spell_data_t* commander_of_the_dead;
    // Ruptured Viscera Talent
    const spell_data_t* ruptured_viscera;
    // Ghoulish Frenzy
    const spell_data_t* ghoulish_frenzy;
    // DRW Spells
    const spell_data_t* drw_heart_strike;
    const spell_data_t* drw_heart_strike_rp_gen;
    // Rider of the Apocalypse Pet Spells
    const spell_data_t* apocalyptic_conquest;
    const spell_data_t* trollbanes_chains_of_ice_ability;
    const spell_data_t* trollbanes_chains_of_ice_debuff;
    const spell_data_t* trollbanes_icy_fury_ability;
    const spell_data_t* undeath_dot;
    const spell_data_t* undeath_range;
    const spell_data_t* mograines_death_and_decay;
    const spell_data_t* mograines_might_buff;
    const spell_data_t* rider_ams;
    const spell_data_t* rider_ams_icd;
    const spell_data_t* whitemane_death_coil;
    const spell_data_t* mograine_heart_strike;
    const spell_data_t* trollbane_obliterate;
    const spell_data_t* nazgrim_scourge_strike_phys;
    const spell_data_t* nazgrim_scourge_strike_shadow;
    // San'layn Blood Beast Spells
    const spell_data_t* corrupted_blood;
    const spell_data_t* blood_eruption;
    // Abomiantion Spells
    const spell_data_t* abomination_disease_cloud;
  } pet_spell;

  // RPPM
  struct rppm_t
  {
    real_ppm_t* bloodworms;
    real_ppm_t* carnage;
    real_ppm_t* runic_attenuation;
    real_ppm_t* blood_beast;
    real_ppm_t* tww1_fdk_4pc;
  } rppm;

  // Pets and Guardians
  struct pets_t
  {
    spawner::pet_spawner_t<pets::ghoul_pet_t, death_knight_t> ghoul_pet;
    spawner::pet_spawner_t<pets::army_ghoul_pet_t, death_knight_t> army_ghouls;
    spawner::pet_spawner_t<pets::army_ghoul_pet_t, death_knight_t> apoc_ghouls;
    spawner::pet_spawner_t<pets::gargoyle_pet_t, death_knight_t> gargoyle;
    spawner::pet_spawner_t<pets::risen_skulker_pet_t, death_knight_t> risen_skulker;
    spawner::pet_spawner_t<pets::dancing_rune_weapon_pet_t, death_knight_t> dancing_rune_weapon_pet;
    spawner::pet_spawner_t<pets::dancing_rune_weapon_pet_t, death_knight_t> everlasting_bond_pet;
    spawner::pet_spawner_t<pets::bloodworm_pet_t, death_knight_t> bloodworms;
    spawner::pet_spawner_t<pets::magus_pet_t, death_knight_t> army_magus;
    spawner::pet_spawner_t<pets::magus_pet_t, death_knight_t> apoc_magus;
    spawner::pet_spawner_t<pets::magus_pet_t, death_knight_t> doomed_bidding_magus_coil;
    spawner::pet_spawner_t<pets::magus_pet_t, death_knight_t> doomed_bidding_magus_epi;
    spawner::pet_spawner_t<pets::blood_beast_pet_t, death_knight_t> blood_beast;
    spawner::pet_spawner_t<pets::mograine_pet_t, death_knight_t> mograine;
    spawner::pet_spawner_t<pets::whitemane_pet_t, death_knight_t> whitemane;
    spawner::pet_spawner_t<pets::trollbane_pet_t, death_knight_t> trollbane;
    spawner::pet_spawner_t<pets::nazgrim_pet_t, death_knight_t> nazgrim;
    spawner::pet_spawner_t<pets::abomination_pet_t, death_knight_t> abomination;

    pets_t( death_knight_t* p )
      : ghoul_pet( "ghoul", p ),
        army_ghouls( "army_ghoul", p ),
        apoc_ghouls( "apoc_ghoul", p ),
        gargoyle( "gargoyle", p ),
        risen_skulker( "risen_skulker", p ),
        dancing_rune_weapon_pet( "dancing_rune_weapon", p ),
        everlasting_bond_pet( "everlasting_bond", p ),
        bloodworms( "bloodworm", p ),
        army_magus( "army_magus", p ),
        apoc_magus( "apoc_magus", p ),
        doomed_bidding_magus_coil( "doomed_bidding_magus_coil", p ),
        doomed_bidding_magus_epi( "doomed_bidding_magus_epi", p ),
        blood_beast( "blood_beast", p ),
        mograine( "mograine", p ),
        whitemane( "whitemane", p ),
        trollbane( "trollbane", p ),
        nazgrim( "nazgrim", p ),
        abomination( "abomination", p )
    {
    }
  } pets;

  // Procs
  struct procs_t
  {
    // Normal rune regeneration proc
    propagate_const<proc_t*> ready_rune;

    propagate_const<proc_t*> bloodworms;
    propagate_const<proc_t*> carnage;

    // Killing Machine spent on
    propagate_const<proc_t*> killing_machine_oblit;
    propagate_const<proc_t*> killing_machine_fsc;

    // Killing machine triggered by
    propagate_const<proc_t*> km_from_crit_aa;
    propagate_const<proc_t*> km_from_obliteration_fs;  // Frost Strike during Obliteration
    propagate_const<proc_t*> km_from_obliteration_hb;  // Howling Blast during Obliteration
    propagate_const<proc_t*> km_from_obliteration_ga;  // Glacial Advance during Obliteration
    propagate_const<proc_t*> km_from_obliteration_sr;  // Soul Reaper during Obliteration

    // Killing machine refreshed by
    propagate_const<proc_t*> km_from_crit_aa_wasted;
    propagate_const<proc_t*> km_from_obliteration_fs_wasted;  // Frost Strike during Obliteration
    propagate_const<proc_t*> km_from_obliteration_hb_wasted;  // Howling Blast during Obliteration
    propagate_const<proc_t*> km_from_obliteration_ga_wasted;  // Glacial Advance during Obliteration
    propagate_const<proc_t*> km_from_obliteration_sr_wasted;  // Soul Reaper during Obliteration

    // Razorice applied by
    propagate_const<proc_t*> razorice_from_arctic_assault;
    propagate_const<proc_t*> razorice_from_avalanche;
    propagate_const<proc_t*> razorice_from_glacial_advance;
    propagate_const<proc_t*> razorice_from_runeforge;

    // Runic corruption triggered by
    propagate_const<proc_t*> rp_runic_corruption;  // from RP spent
    propagate_const<proc_t*> sr_runic_corruption;  // from soul reaper

    // Festering Wound applied by
    propagate_const<proc_t*> fw_festering_strike;
    propagate_const<proc_t*> fw_festering_scythe;
    propagate_const<proc_t*> fw_infected_claws;
    propagate_const<proc_t*> fw_pestilence;
    propagate_const<proc_t*> fw_unholy_assault;
    propagate_const<proc_t*> fw_vile_contagion;
    propagate_const<proc_t*> fw_ruptured_viscera;
    propagate_const<proc_t*> fw_abomination;

    // Festering Wound consumed by
    propagate_const<proc_t*> fw_apocalypse;
    propagate_const<proc_t*> fw_wound_spender;
    propagate_const<proc_t*> fw_sudden_doom;
    propagate_const<proc_t*> fw_death;

    // Chill Streak related procs
    propagate_const<proc_t*> enduring_chill;  // Extra bounces given by Enduring Chill

    // Decomposition 
    propagate_const<proc_t*> decomposition;

    // San'layn procs
    propagate_const<proc_t*> blood_beast;
    propagate_const<proc_t*> vampiric_strike;
    propagate_const<proc_t*> vampiric_strike_waste;

    // Deathbringer procs
    propagate_const<proc_t*> exterminate_reapers_mark;
  } procs;

  // Death Knight Options
  struct options_t
  {
    bool disable_aotd             = false;
    bool split_ghoul_regen        = false;
    bool split_obliterate_schools = true;
    double ams_absorb_percent     = 0;
    bool individual_pet_reporting = false;
    bool amz_specified                 = false;
    double average_cs_travel_time      = 0.4;
    timespan_t first_ams_cast          = 20_s;
    double horsemen_ams_absorb_percent = 0.6;
  } options;

  // Runes
  runes_t _runes;

  auto_dispose<std::vector<modified_spell_data_t*>> modified_spells;
  std::vector<pets::death_knight_pet_t*> dk_active_pets;

  death_knight_t( sim_t* sim, util::string_view name, race_e r )
    : parse_player_effects_t( sim, DEATH_KNIGHT, name, r ),
      active_dnd( nullptr ),
      runic_power_decay( nullptr ),
      deprecated_dnd_expression( false ),
      runeforge_expression_warning( false ),
      km_proc_attempts( 0 ),
      festering_wounds_target_count( 0 ),
      bone_shield_charges_consumed( 0 ),
      active_riders( 0 ),
      undeath_tl(),
      buffs(),
      runeforge(),
      active_spells(),
      gains(),
      spec(),
      mastery(),
      talent(),
      spell(),
      pet_spell(),
      pets( this ),
      procs(),
      options(),
      _runes( this ),
      dk_active_pets()
  {
    cooldown.apocalypse          = get_cooldown( "apocalypse" );
    cooldown.army_of_the_dead    = get_cooldown( "army_of_the_dead" );
    cooldown.blood_tap           = get_cooldown( "blood_tap" );
    cooldown.blooddrinker        = get_cooldown( "blooddrinker" );
    cooldown.bone_shield_icd     = get_cooldown( "bone_shield_icd" );
    cooldown.consumption         = get_cooldown( "consumption" );
    cooldown.dancing_rune_weapon = get_cooldown( "dancing_rune_weapon" );
    cooldown.dark_transformation = get_cooldown( "dark_transformation" );
    cooldown.death_and_decay_dynamic =
        get_cooldown( "death_and_decay" );  // Default value, changed during action construction
    cooldown.death_grip             = get_cooldown( "death_grip" );
    cooldown.inexorable_assault_icd = get_cooldown( "inexorable_assault_icd" );
    cooldown.frigid_executioner_icd = get_cooldown( "frigid_executioner_icd" );
    cooldown.pillar_of_frost        = get_cooldown( "pillar_of_frost" );
    cooldown.vampiric_blood         = get_cooldown( "vampiric_blood" );
    cooldown.enduring_strength_icd  = get_cooldown( "enduring_strength" );
    cooldown.mind_freeze            = get_cooldown( "mind_freeze" );
    cooldown.frostwyrms_fury        = get_cooldown( "frostwyrms_fury_driver" );
    cooldown.chill_streak           = get_cooldown( "chill_streak" );
    cooldown.empower_rune_weapon    = get_cooldown( "empower_rune_weapon" );
    cooldown.frostscythe            = get_cooldown( "frostscythe" );

    // Target Specific
    cooldown.undeath_spread = get_target_specific_cooldown( "undeath_spread" );

    resource_regeneration = regen_type::DYNAMIC;
  }

  // Character Definition overrides
  void init_spells() override;
  void init_action_list() override;
  void init_rng() override;
  void init_base_stats() override;
  void init_scaling() override;
  void create_buffs() override;
  void init_gains() override;
  void init_procs() override;
  void init_finished() override;
  bool validate_fight_style( fight_style_e style ) const override;
  double composite_attribute( attribute_e ) const override;
  double composite_bonus_armor() const override;
  void combat_begin() override;
  void activate() override;
  void reset() override;
  void arise() override;
  void adjust_dynamic_cooldowns() override;
  void assess_heal( school_e, result_amount_type, action_state_t* ) override;
  void assess_damage( school_e, result_amount_type, action_state_t* ) override;
  void assess_damage_imminent( school_e, result_amount_type, action_state_t* ) override;
  void target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  void do_damage( action_state_t* ) override;
  void create_actions() override;
  action_t* create_action( util::string_view name, util::string_view options ) override;
  std::unique_ptr<expr_t> create_expression( util::string_view name ) override;
  void create_options() override;
  void create_pets() override;
  resource_e primary_resource() const override
  {
    return RESOURCE_RUNIC_POWER;
  }
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void invalidate_cache( cache_e ) override;
  double resource_gain( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  double resource_loss( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  void copy_from( player_t* source ) override;
  void merge( player_t& other ) override;
  void datacollection_begin() override;
  void datacollection_end() override;
  void analyze( sim_t& sim ) override;
  void apply_affecting_auras( buff_t& );
  void apply_affecting_auras( action_t& action ) override;

  // Default consumables
  std::string default_flask() const override
  {
    return death_knight_apl::flask( this );
  }
  std::string default_food() const override
  {
    return death_knight_apl::food( this );
  }
  std::string default_potion() const override
  {
    return death_knight_apl::potion( this );
  }
  std::string default_rune() const override
  {
    return death_knight_apl::rune( this );
  }
  std::string default_temporary_enchant() const override
  {
    return death_knight_apl::temporary_enchant( this );
  }

  // Create Profile options
  std::string create_profile( save_e ) override;

  // Death Knight specific methods
  // Rune related methods
  double runes_per_second() const;
  double rune_regen_coefficient() const;
  unsigned replenish_rune( unsigned n, gain_t* gain = nullptr );
  // Shared
  modified_spell_data_t* get_modified_spell( const spell_data_t* );
  bool in_death_and_decay() const;
  void parse_player_effects();
  const spell_data_t* conditional_spell_lookup( bool fn, int id );
  double tick_damage_over_time( timespan_t duration, const dot_t* dot ) const;
  // Rider of the Apocalypse
  void summon_rider( timespan_t duration, bool random );
  void extend_rider( double amount, pets::horseman_pet_t* rider );
  void trigger_whitemanes_famine( player_t* target );
  void sort_undeath_targets( std::vector<player_t*> tl );
  void start_a_feast_of_souls();
  // San'layn
  void trigger_infliction_of_sorrow( player_t* target, bool is_vampiric );
  void trigger_vampiric_strike_proc( player_t* target );
  void trigger_sanlayn_execute_talents( bool is_vampiric );
  // Deathbringer
  void trigger_reapers_mark_death( player_t* target );
  void reapers_mark_explosion_wrapper( player_t* target, player_t* source, int stacks );
  // Blood
  void bone_shield_handler( const action_state_t* ) const;
  // Frost
  void trigger_killing_machine( double chance, proc_t* proc, proc_t* wasted_proc );
  void consume_killing_machine( proc_t* proc, timespan_t total_delay );
  void trigger_runic_empowerment( double rpcost );
  void chill_streak_bounce( player_t& target );
  // Unholy
  void trigger_festering_wound( const action_state_t* state, unsigned n_stacks = 1, proc_t* proc = nullptr );
  void burst_festering_wound( player_t* target, unsigned n = 1, proc_t* action = nullptr );
  void trigger_runic_corruption( proc_t* proc, double rpcost, double override_chance = -1.0,
                                 bool death_trigger = false );
  void trigger_bursting_sores( player_t* target, unsigned n = 1 );
  // Start the repeated stacking of buffs, called at combat start
  void start_cold_heart();
  void start_inexorable_assault();
  // On-target-death triggers
  void trigger_festering_wound_death( player_t* );
  void trigger_soul_reaper_death( player_t* );
  void trigger_virulent_plague_death( player_t* );

  // Runeforge expression handling for Death Knight Runeforges (not legendary)
  std::unique_ptr<expr_t> create_runeforge_expression( util::string_view runeforge_name, bool warning );

  target_specific_t<death_knight_td_t> target_data;

  death_knight_td_t* get_target_data( player_t* target ) const override
  {
    assert( target );
    death_knight_td_t*& td = target_data[ target ];
    if ( !td )
      td = new death_knight_td_t( *target, const_cast<death_knight_t&>( *this ) );

    return td;
  }

  // Cooldown Tracking
  template <typename T_CONTAINER, typename T_DATA>
  T_CONTAINER* get_data_entry( util::string_view name, std::vector<T_DATA*>& entries )
  {
    for ( size_t i = 0; i < entries.size(); i++ )
    {
      if ( entries[ i ]->first == name )
      {
        return &( entries[ i ]->second );
      }
    }

    entries.push_back( new T_DATA( name, T_CONTAINER() ) );
    return &( entries.back()->second );
  }
};

// ==========================================================================
// Death Knight Runes ( part 2 )
// ==========================================================================

inline void rune_event_t::execute_event()
{
  sim().print_debug( "{} regenerates a rune, start_time={}, regen_time={} current_coeff={}", m_rune->runes->dk->name(),
                     m_rune->regen_start.total_seconds(),
                     ( sim().current_time() - m_rune->regen_start ).total_seconds(), m_coefficient );

  m_rune->fill_rune();
}

static void log_rune_status( const death_knight_t* p )
{
  if ( !p->sim->debug )
    return;
  std::string rune_string = p->_runes.string_representation();
  p->sim->print_debug( "{} runes: {}", p->name(), rune_string );
}

inline runes_t::runes_t( death_knight_t* p )
  : dk( p ),
    cumulative_waste( dk->name_str + "_Iteration_Rune_Waste", false ),
    rune_waste( dk->name_str + "_Rune_Waste", false ),
    iteration_waste_sum( 0_ms )
{
  for ( auto& rune : slot )
  {
    rune = rune_t( this );
  }
}

inline void runes_t::consume( unsigned runes )
{
  // We should never get there, ready checks should catch resource constraints
#ifndef NDEBUG
  if ( runes_full() < runes )
  {
    assert( 0 );
  }
#endif
  auto n_full_runes   = runes_full();
  int n_wasting_runes = n_full_runes - MAX_REGENERATING_RUNES;
  int disable_waste   = n_full_runes - runes <= MAX_REGENERATING_RUNES;

  while ( runes-- )
  {
    auto rune = first_full_rune();
    if ( n_wasting_runes > 0 )
    {
      // Waste time for a given rune is determined as the time the actor went past the maximum
      // regenerating runesa (waste_start), or if later in time, the time this specific rune
      // replenished (rune -> regenerated).
      auto wasted_time = dk->sim->current_time() - std::max( rune->regenerated, waste_start );
      if ( wasted_time > 0_ms )
      {
        assert( wasted_time > 0_ms );
        iteration_waste_sum += wasted_time;
        rune_waste.add( wasted_time.total_seconds() );

        dk->sim->print_debug( "{} rune waste, n_full_runes={} rune_regened={} waste_started={} wasted_time={}",
                              dk->name(), n_full_runes, rune->regenerated.total_seconds(), waste_start.total_seconds(),
                              wasted_time.total_seconds() );

        n_wasting_runes--;
      }
    }
    rune->consume();
  }

  log_rune_status( dk );

  // Full runes will be going below the maximum number of regenerating runes, so there's no longer
  // going to be any waste time.
  if ( disable_waste && waste_start >= 0_ms )
  {
    dk->sim->print_debug( "{} rune waste, waste ended, n_full_runes={}", dk->name(), runes_full() );

    waste_start = timespan_t::min();
  }
}

inline void runes_t::regenerate_immediate( timespan_t seconds )
{
  if ( seconds <= 0_ms )
  {
    dk->sim->errorf( "%s warning, regenerating runes with an invalid immediate value (%.3f)", dk->name(),
                     seconds.total_seconds() );
    return;
  }

  dk->sim->print_debug( "{} regenerating runes with an immediate value of {}", dk->name(), seconds.total_seconds() );
  log_rune_status( dk );

  // Collect regenerating and depleted runes
  std::vector<rune_t*> regenerating_runes;
  std::vector<rune_t*> depleted_runes;
  range::for_each( slot, [ &regenerating_runes, &depleted_runes ]( rune_t& r ) {
    if ( r.is_regenerating() )
    {
      regenerating_runes.push_back( &( r ) );
    }
    else if ( r.is_depleted() )
    {
      depleted_runes.push_back( &( r ) );
    }
  } );

  // Sort regenerating runes by ascending remaining time
  range::sort( regenerating_runes, []( const rune_t* l, const rune_t* r ) {
    timespan_t lv = l->event->remains();
    timespan_t rv = r->event->remains();
    // Use pointers as tiebreaker
    if ( lv == rv )
    {
      return l < r;
    }

    return lv < rv;
  } );

  timespan_t seconds_left = seconds;
  // Regenerate runes in time chunks, until all immediate regeneration time is consumed
  while ( seconds_left > 0_ms )
  {
    // Pop out all full runes from regenerating runes. Can happen if below the call to
    // adjust_regen_event causes the rune to actually fill up.
    while ( !regenerating_runes.empty() && regenerating_runes.front()->is_ready() )
    {
      regenerating_runes.erase( regenerating_runes.begin() );
    }

    // Move any new regenerating runes from depleted runes to regenerating runes. They can be placed
    // at the back, since they will automatically have the longest regen time. This can occur if
    // there are depleted runes, and a regenerating rune fills up (causing a depleted rune to start
    // regenerating)
    while ( true )
    {
      auto it = range::find_if( depleted_runes, []( rune_t* r ) { return r->is_regenerating(); } );
      if ( it != depleted_runes.end() )
      {
        regenerating_runes.push_back( *it );
        depleted_runes.erase( it );
      }
      else
      {
        break;
      }
    }

    // Break out early if all runes are filled up
    if ( regenerating_runes.empty() )
    {
      break;
    }

    // The first rune in the regenerating runes list is always going to be the one with the least
    // remaining regeneration time left, so use that as a clamp to determine how large a chunk can
    // be regenerated during this iteration of the loop.
    auto first_left = regenerating_runes[ 0 ]->event->remains();

    // Clamp regenerating time units to the minimum of total regeneration time left, and the first
    // regenerating rune's time left
    timespan_t units_regened = std::min( seconds_left, first_left );

    // Regenerate all regenerating runes by units_regened
    for ( auto& rune : regenerating_runes )
    {
      rune->adjust_regen_event( -units_regened );
    }

    seconds_left -= units_regened;
  }

  log_rune_status( dk );
}

timespan_t runes_t::time_to_regen( unsigned n_runes )
{
  if ( n_runes == 0 )
  {
    return 0_ms;
  }

  if ( n_runes > MAX_RUNES )
  {
    return timespan_t::max();
  }

  // If we have the runes, no need to check anything.
  if ( dk->resources.current[ RESOURCE_RUNE ] >= as<double>( n_runes ) )
  {
    return 0_ms;
  }

  // First, collect regenerating runes into an array
  regenerating_runes.clear();
  range::for_each( slot, [ this ]( const rune_t& r ) {
    if ( r.is_regenerating() )
    {
      regenerating_runes.push_back( &( r ) );
    }
  } );

  // Sort by ascending remaining time
  range::sort( regenerating_runes, []( const rune_t* l, const rune_t* r ) {
    timespan_t lv = l->event->remains();
    timespan_t rv = r->event->remains();
    // Use pointers as tiebreaker
    if ( lv == rv )
    {
      return l < r;
    }

    return lv < rv;
  } );

  // Number of unsatisfied runes
  unsigned n_unsatisfied = n_runes - as<unsigned int>( dk->resources.current[ RESOURCE_RUNE ] );

  // If we can satisfy the remaining unsatisfied runes with regenerating runes, return the N - 1th
  // remaining regeneration time
  if ( n_unsatisfied <= regenerating_runes.size() )
  {
    return regenerating_runes[ n_unsatisfied - 1 ]->event->remains();
  }

  // Which regenerating rune time should be picked when we have more unsatisfied runes than
  // currently regenerating ones.
  auto nth_regenerating_rune = n_unsatisfied - regenerating_runes.size();

  // Otherwise, the time is going to be the nth rune regen time plus the time it takes to regen a
  // depleted rune
  return regenerating_runes[ nth_regenerating_rune - 1 ]->event->remains() +
         timespan_t::from_seconds( 1 / dk->runes_per_second() );
}

inline double rune_t::fill_level() const
{
  if ( state == STATE_FULL )
  {
    return 1.0;
  }
  else if ( state == STATE_DEPLETED )
  {
    return 0.0;
  }

  auto regen_time_elapsed = runes->dk->sim->current_time() - regen_start;
  auto total_rune_time    = regen_time_elapsed + event->remains();

  return 1.0 - event->remains() / total_rune_time;
}

inline void rune_t::update_coefficient()
{
  if ( event )
  {
    event->update_coefficient( runes->dk->rune_regen_coefficient() );
  }
}

inline void rune_t::consume()
{
  assert( state == STATE_FULL && event == nullptr );

  state = STATE_DEPLETED;

  // Immediately update the state of the next regenerating rune, since rune_t::regen_rune presumes
  // that the internal state of each invidual rune is always consistent with the rune regeneration
  // rules
  if ( runes->runes_regenerating() < MAX_REGENERATING_RUNES )
  {
    runes->first_depleted_rune()->start_regenerating();
  }

  // Internal state consistency for current rune regeneration rules
  assert( runes->runes_regenerating() <= MAX_REGENERATING_RUNES );
  assert( runes->runes_depleted() == MAX_RUNES - runes->runes_full() - runes->runes_regenerating() );
}

inline void rune_t::fill_rune( gain_t* gain )
{
  // Cancel regeneration event if this rune was regenerating
  if ( state == STATE_REGENERATING )
  {
    assert( event );
    event_t::cancel( event );
  }

  if ( state != STATE_FULL )
  {
    runes->dk->procs.ready_rune->occur();
  }

  state       = STATE_FULL;
  regenerated = runes->dk->sim->current_time();

  // Update actor rune resources, so we can re-use a lot of the normal resource mechanisms that the
  // sim core offers
  runes->dk->resource_gain( RESOURCE_RUNE, 1, gain ? gain : runes->dk->gains.rune );

  // Immediately update the state of the next regenerating rune, since rune_t::regen_rune presumes
  // that the internal state of each invidual rune is always consistent with the rune regeneration
  // rules
  if ( runes->runes_depleted() > 0 && runes->runes_regenerating() < MAX_REGENERATING_RUNES )
  {
    runes->first_depleted_rune()->start_regenerating();
  }

  // Internal state consistency for current rune regeneration rules
  assert( runes->runes_regenerating() <= MAX_REGENERATING_RUNES );
  assert( runes->runes_depleted() == MAX_RUNES - runes->runes_full() - runes->runes_regenerating() );

  // If the actor goes past the maximum number of regenerating runes, mark the waste start
  if ( runes->waste_start < 0_ms && runes->runes_full() > MAX_REGENERATING_RUNES )
  {
    runes->dk->sim->print_debug( "{} rune waste, waste started, n_full_runes={}", runes->dk->name(),
                                 runes->runes_full() );
    runes->waste_start = runes->dk->sim->current_time();
  }
}

inline void rune_t::start_regenerating()
{
  assert( event == nullptr );
  state       = STATE_REGENERATING;
  regen_start = runes->dk->sim->current_time();

  // Create a new regen event with proper parameters
  event = rune_event_t::create( runes->dk->sim )
              ->rune( *this )
              ->ptr( event )
              ->coefficient( runes->dk->rune_regen_coefficient() )
              ->schedule( timespan_t::from_seconds( RUNE_REGEN_BASE ) );
}

inline void rune_t::adjust_regen_event( timespan_t adjustment )
{
  if ( !event )
  {
    return;
  }

  auto new_remains = event->remains() + adjustment;

  // Reduce remaining rune regeneration time by adjustment seconds.
  if ( adjustment < 0_ms )
  {
    // Filled the rune through the adjustment
    if ( new_remains <= 0_ms )
    {
      fill_rune();
    }
    // Cut out adjustment amount of time from the rune regeneration, recreate the event with a
    // shorter remaining event time.
    else
    {
      auto e = event;
      event  = rune_event_t::create( runes->dk->sim )
                  ->rune( *this )
                  ->ptr( event )
                  ->coefficient( event->coefficient() )
                  ->schedule( new_remains, false );  // Note, sheduled using new_remains and no coefficient

      event_t::cancel( e );

      if ( adjustment < 0_ms )
      {
        regen_start += adjustment;
      }
    }
  }
  // Adjustment is positive, reschedule the regeneration event to the new remaining time.
  else if ( adjustment > 0_ms )
  {
    event->reschedule( new_remains );
  }
}

namespace pets
{
// ==========================================================================
// Death Knight Pet Target Data
// ==========================================================================
struct death_knight_pet_td_t : public actor_target_data_t
{
  struct debuffs_t
  {
    buff_t* frostbolt_debuff;
  } debuff;

  struct dots_t
  {
    dot_t* blood_plague;
  } dot;

  death_knight_pet_td_t( player_t& target, death_knight_pet_t& p );

  template <typename Buff = buff_t, typename... Args>
  inline buff_t* make_debuff( bool b, Args&&... args );
};

// utility to create target_effect_t compatible functions from death_knight_td_t member references
template <typename T>
static std::function<int( actor_target_data_t* )> d_fn( T d, bool stack = true )
{
  if constexpr ( std::is_invocable_v<T, death_knight_pet_td_t::debuffs_t> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<death_knight_pet_td_t*>( t )->debuff )->check();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<death_knight_pet_td_t*>( t )->debuff )->check() > 0;
      };
  }
  else if constexpr ( std::is_invocable_v<T, death_knight_pet_td_t::dots_t> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<death_knight_pet_td_t*>( t )->dot )->current_stack();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<death_knight_pet_td_t*>( t )->dot )->is_ticking();
      };
  }
  else
  {
    static_assert( static_false<T>, "Not a valid member of death_knight_pet_td_t" );
    return nullptr;
  }
}

// ==========================================================================
// Generic DK pet
// ==========================================================================

struct death_knight_pet_t : public pet_t
{
  bool use_auto_attack, precombat_spawn, affected_by_commander_of_the_dead, guardian;
  timespan_t precombat_spawn_adjust;
  double army_ghoul_ap_mod;
  timespan_t decomposition_extended;
  timespan_t decomposition_extend_limit;
  bool decomposition_can_extend;
  bool tww1_4pc_proc;
  bool is_dk_pet;
  bool is_magus;

  death_knight_pet_t( death_knight_t* player, util::string_view name, bool guardian = true, bool auto_attack = true,
                      bool dynamic = true )
    : pet_t( player->sim, player, name, guardian, dynamic ),
      use_auto_attack( auto_attack ),
      precombat_spawn( false ),
      affected_by_commander_of_the_dead( false ),
      guardian( guardian ),
      precombat_spawn_adjust( 0_s ),
      army_ghoul_ap_mod( 0 ),
      decomposition_extended( 0_s ),
      decomposition_extend_limit( 0_s ),
      decomposition_can_extend( false ),
      tww1_4pc_proc( false ),
      is_dk_pet( true ),
      is_magus( false )
  {
    if ( auto_attack )
    {
      main_hand_weapon.type = WEAPON_BEAST;
    }

    // Not in spell data, storing here to ensure parity between magus/apoc/army ghouls.
    army_ghoul_ap_mod = 0.4664;

    if ( player->talent.unholy.decomposition.ok() )
    {
      decomposition_extend_limit =
          timespan_t::from_millis( player->talent.unholy.decomposition->effectN( 3 ).base_value() );
    }
  }

  target_specific_t<death_knight_pet_td_t> target_data;

  death_knight_pet_td_t* get_target_data( player_t* target ) const override
  {
    assert( target );
    death_knight_pet_td_t*& td = target_data[ target ];
    if ( !td )
      td = new death_knight_pet_td_t( *target, const_cast<death_knight_pet_t&>( *this ) );

    return td;
  }

  void init_finished() override
  {
    pet_t::init_finished();
    buffs.stunned->set_quiet( true );
    buffs.movement->set_quiet( true );
  }

  // DK pets dont care about armor, return 0 for speed
  double composite_bonus_armor() const override
  {
    return 0;
  }

  // No DK Pets (currently) heal, return 0 for speed
  double composite_heal_versatility() const override
  {
    return 0;
  }

  // DK pets dont care about incoming damage, return 0 for speed
  double composite_mitigation_versatility() const override
  {
    return 0;
  }

  double composite_melee_auto_attack_speed() const override
  {
    return current_pet_stats.composite_melee_haste;
  }

  death_knight_t* dk() const
  {
    return debug_cast<death_knight_t*>( owner );
  }

  virtual attack_t* create_main_hand_auto_attack()
  {
    return nullptr;
  }

  virtual attack_t* create_off_hand_auto_attack()
  {
    return nullptr;
  }

  void arise() override
  {
    pet_t::arise();
    dk()->dk_active_pets.push_back( this );
    if ( decomposition_can_extend )
    {
      decomposition_extended = 0_s;
    }

    if ( dk()->sets->has_set_bonus( DEATH_KNIGHT_UNHOLY, TWW1, B4 ) && tww1_4pc_proc )
    {
      dk()->buffs.unholy_commander->trigger();
    }
  }

  void demise() override
  {
    range::erase_remove( dk()->dk_active_pets, [ this ]( player_t* ) { return this; } );
    pet_t::demise();
    if ( decomposition_can_extend )
    {
      decomposition_extended = 0_s;
    }
  }

  void reset() override
  {
    pet_t::reset();
    dk()->dk_active_pets.clear();
    if ( decomposition_can_extend )
    {
      decomposition_extended = 0_s;
    }
  }

  void apply_affecting_auras( action_t& action ) override
  {
    player_t::apply_affecting_auras( action );

    // Hack to apply BDK aura to DRW abilities
    // 2021-01-29: The bdk aura is currently set to 0,
    // Keep an eye on this in case blizz changes the value as double dipping may happen (both ingame and here)
    dk()->apply_affecting_auras( action );
  }

  double composite_player_multiplier( school_e s ) const override
  {
    double m = pet_t::composite_player_multiplier( s );

    if ( dk()->specialization() == DEATH_KNIGHT_UNHOLY && dk()->buffs.commander_of_the_dead->up() &&
         affected_by_commander_of_the_dead )
    {
      m *= 1.0 + dk()->pet_spell.commander_of_the_dead->effectN( 1 ).percent();
    }

    return m;
  }

  // Standard Death Knight pet actions
  struct auto_attack_t final : public melee_attack_t
  {
    auto_attack_t( death_knight_pet_t* p ) : melee_attack_t( "auto_attack", p )
    {
      assert( p->main_hand_weapon.type != WEAPON_NONE );
      p->main_hand_attack                    = p->create_main_hand_auto_attack();
      p->main_hand_attack->weapon            = &( p->main_hand_weapon );
      p->main_hand_attack->base_execute_time = p->main_hand_weapon.swing_time;

      if ( p->off_hand_weapon.type != WEAPON_NONE )
      {
        p->off_hand_attack                    = p->create_off_hand_auto_attack();
        p->off_hand_attack->weapon            = &( p->off_hand_weapon );
        p->off_hand_attack->base_execute_time = p->off_hand_weapon.swing_time;
        p->off_hand_attack->id                = 1;
      }

      ignore_false_positive = true;
      trigger_gcd = 0_ms;
      school = SCHOOL_PHYSICAL;
    }

    void execute() override
    {
      player->main_hand_attack->schedule_execute();
      if ( player->off_hand_attack )
      {
        player->off_hand_attack->schedule_execute();
      }
    }

    bool ready() override
    {
      if ( player->is_moving() )
        return false;
      return ( player->main_hand_attack->execute_event == nullptr );
    }
  };

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this );

    return pet_t::create_action( name, options_str );
  }

  void init_action_list() override
  {
    action_priority_list_t* def = get_action_priority_list( "default" );
    if ( use_auto_attack )
      def->add_action( "auto_attack" );

    pet_t::init_action_list();
  }
};

// ==========================================================================
// Death Knight Pet parent action function
// ==========================================================================
std::function<void( death_knight_pet_t* )> parent_pet_action_fn( action_t* parent )
{
  return [ parent ]( death_knight_pet_t* p ) {
    if ( !p->dk()->options.individual_pet_reporting )
    {
      for ( auto& a : p->action_list )
      {
        auto it = range::find( parent->child_action, a->name_str, &action_t::name_str );
        if ( it != parent->child_action.end() )
          a->stats = ( *it )->stats;
        else
          parent->add_child( a );
      }
    }
  };
}

// ==========================================================================
// Death Knight Pet Target Data DoT & Debuff Creation
// ==========================================================================
template <typename Buff, typename... Args>
inline buff_t* death_knight_pet_td_t::make_debuff( bool b, Args&&... args )
{
  return buff_t::make_buff_fallback<Buff>( target->is_enemy() && b, std::forward<Args>( args )... );
}
inline death_knight_pet_td_t::death_knight_pet_td_t( player_t& target, death_knight_pet_t& p )
  : actor_target_data_t( &target, &p )
{
  dot.blood_plague = target.get_dot( "blood_plague", &p );

  debuff.frostbolt_debuff = make_debuff( p.npc_id == 163366, *this, "frostbolt_magus_of_the_dead", p.dk()->pet_spell.frostbolt );
}

// ==========================================================================
// Base Death Knight Pet Action
// ==========================================================================

template <typename T_PET, typename Base>
struct pet_action_t : public parse_action_effects_t<Base>
{
  using action_base_t = parse_action_effects_t<Base>;
  using base_t        = pet_action_t<T_PET, Base>;

  pet_action_t( T_PET* p, util::string_view name, const spell_data_t* spell = spell_data_t::nil() )
    : action_base_t( name, p, spell )
  {
    this->special = this->may_crit = true;
    if ( this->data().ok() )
    {
      apply_pet_action_effects();
      if ( this->type == action_e::ACTION_SPELL || this->type == action_e::ACTION_ATTACK )
      {
        apply_pet_target_effects();
      }
    }
  }

  void apply_pet_action_effects();
  void apply_pet_target_effects();

  template <typename... Ts>
  void parse_effects( Ts&&... args )
  {
    action_base_t::parse_effects( std::forward<Ts>( args )... );
  }
  template <typename... Ts>
  void parse_target_effects( Ts&&... args )
  {
    action_base_t::parse_target_effects( std::forward<Ts>( args )... );
  }

  T_PET* pet() const
  {
    return debug_cast<T_PET*>( this->player );
  }

  death_knight_t* dk() const
  {
    return debug_cast<death_knight_t*>( debug_cast<pet_t*>( this->player )->owner );
  }

  void init() override
  {
    action_base_t::init();

    // Merge stats for pets sharing the same name
    if ( !this->player->sim->report_pets_separately )
    {
      auto it =
          range::find_if( dk()->pet_list, [ this ]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != dk()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }
  }
};

// ==========================================================================
// Base Death Knight Pet Melee Attack
// ==========================================================================

template <typename T_PET>
struct pet_melee_attack_t : public pet_action_t<T_PET, melee_attack_t>
{
  pet_melee_attack_t( T_PET* p, util::string_view name, const spell_data_t* spell = spell_data_t::nil() )
    : pet_action_t<T_PET, melee_attack_t>( p, name, spell )
  {
    if ( this->school == SCHOOL_NONE )
    {
      this->school        = SCHOOL_PHYSICAL;
      this->stats->school = SCHOOL_PHYSICAL;
    }
  }

  void init() override
  {
    pet_action_t<T_PET, melee_attack_t>::init();

    // Default to a 1s gcd
    if ( !this->background && this->trigger_gcd == 0_ms )
    {
      this->trigger_gcd = 1_s;
    }
  }
};

// ==========================================================================
// Base Death Knight Pet Spell
// ==========================================================================

template <typename T_PET>
struct pet_spell_t : public pet_action_t<T_PET, spell_t>
{
  pet_spell_t( T_PET* p, util::string_view name, const spell_data_t* spell = spell_data_t::nil() )
    : pet_action_t<T_PET, spell_t>( p, name, spell )
  {
    this->tick_may_crit = true;
    this->hasted_ticks  = false;
  }
};

// Ruptured Viscera ========================================================
template <typename T_PET>
struct ruptured_viscera_t final : public pet_spell_t<T_PET>
{
  ruptured_viscera_t( T_PET* p, bool magus = false )
    : pet_spell_t<T_PET>( p, "ruptured_viscera", p->dk()->pet_spell.ruptured_viscera ), magus( magus )
  {
    this->aoe        = -1;
    this->background = true;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    pet_spell_t<T_PET>::available_targets( tl );
    if ( this->magus )
    {
      for ( auto& t : tl )
      {
        // Give magus an incredibly low chance to hit any target with Ruptured Viscera
        if ( this->rng().roll( 0.95 ) )
        {
          auto it = range::find( tl, t );
          if ( it != tl.end() )
          {
            tl.erase( it );
          }
        }
      }
    }

    return tl.size();
  }

  void impact( action_state_t* s ) override
  {
    pet_spell_t<T_PET>::impact( s );
    if ( this->dk()->rng().roll( this->dk()->spell.ruptured_viscera_chance->effectN( 1 ).percent() ) )
    {
      this->dk()->trigger_festering_wound( s, 1, this->dk()->procs.fw_ruptured_viscera );
    }
  }

private:
  bool magus;
};

// ==========================================================================
// Specialized Death Knight Pet Actions
// ==========================================================================

// Generic auto attack for meleeing pets
template <typename T>
struct auto_attack_melee_t : public pet_melee_attack_t<T>
{
  bool first;

  auto_attack_melee_t( T* p, util::string_view name = "auto_attack_mh" )
    : pet_melee_attack_t<T>( p, name ), first( true )
  {
    this->background = this->repeating = true;
    this->not_a_proc = this->may_crit = true;
    this->special                     = false;
    this->weapon_multiplier           = 1.0;
    this->trigger_gcd                 = 0_ms;
  }

  void init_finished() override
  {
    pet_melee_attack_t<T>::init_finished();
    if ( this->weapon->slot == SLOT_OFF_HAND )
    {
      // Currently all DK dual wield pets dont appear to have the 0.5x offhand penalty
      this->weapon_multiplier = 2.0;
    }
  }

  void execute() override
  {
    // If we're casting, we should clip a swing
    if ( this->player->executing )
      this->schedule_execute();
    else
      pet_melee_attack_t<T>::execute();

    if ( first )
      first = false;
  }

  void reset() override
  {
    pet_melee_attack_t<T>::reset();

    this->first = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = pet_melee_attack_t<T>::execute_time();

    if ( this->first && this->weapon->slot == SLOT_OFF_HAND )
      return t / 2;
    else
      return t;
  }

  T* pet() const
  {
    return debug_cast<T*>( this->player );
  }

  death_knight_t* dk() const
  {
    return debug_cast<death_knight_t*>( debug_cast<pet_t*>( this->player )->owner );
  }

  // Override a bunch of stuff that attack_t overrides to prevent multiple cache hits
  double composite_hit() const override
  {
    return action_t::composite_hit() + dk()->cache.attack_hit();
  };
  double composite_crit_chance() const override
  {
    return action_t::composite_crit_chance() + pet()->current_pet_stats.composite_melee_crit;
  };
  double composite_haste() const override
  {
    return action_t::composite_haste() + pet()->current_pet_stats.composite_melee_haste;
  };
  double composite_versatility( const action_state_t* state ) const override
  {
    return action_t::composite_versatility( state ) + dk()->cache.damage_versatility();
  };
};

// ==========================================================================
// Generic Unholy energy ghoul (main pet and Army/Apoc ghouls)
// ==========================================================================

struct base_ghoul_pet_t : public death_knight_pet_t
{
  base_ghoul_pet_t( death_knight_t* owner, util::string_view name, bool guardian = false, bool dynamic = true )
    : death_knight_pet_t( owner, name, guardian, true, dynamic )
  {
    main_hand_weapon.swing_time = 2.0_s;
  }

  attack_t* create_main_hand_auto_attack() override
  {
    return new auto_attack_melee_t<base_ghoul_pet_t>( this );
  }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    resources.base[ RESOURCE_ENERGY ]                  = 100;
    resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10;
  }

  void arise() override
  {
    death_knight_pet_t::arise();
    timespan_t duration = dk()->pet_spell.pet_stun->duration();
    if ( precombat_spawn_adjust > 0_s && precombat_spawn )
    {
      duration = duration - precombat_spawn_adjust;
      buffs.stunned->trigger( duration );
      stun();
    }
    else if ( !precombat_spawn )
    {
      buffs.stunned->trigger( duration );
      stun();
    }
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_ENERGY;
  }

  timespan_t available() const override
  {
    double energy = resources.current[ RESOURCE_ENERGY ];
    timespan_t time_to_next =
        timespan_t::from_seconds( ( 40 - energy ) / resource_regen_per_second( RESOURCE_ENERGY ) );

    // Cheapest Ability need 40 Energy
    if ( energy > 40 )
    {
      return 100_ms;
    }
    else
    {
      return std::max( time_to_next, 100_ms );
    }
  }
};

// ===============================================================================
// Raise Dead ghoul (both temporary blood/frost ghouls and permanent unholy ghoul)
// ===============================================================================

struct ghoul_pet_t final : public base_ghoul_pet_t
{
  // Generic Dark Transformation pet ability
  struct dt_melee_ability_t : public pet_melee_attack_t<ghoul_pet_t>
  {
    dt_melee_ability_t( ghoul_pet_t* p, util::string_view name, const spell_data_t* spell = spell_data_t::nil(),
                        bool usable_in_dt = true )
      : pet_melee_attack_t( p, name, spell ),
        usable_in_dt( usable_in_dt ),
        triggers_infected_claws( false ),
        triggers_apocalypse( false )
    {
    }

    void execute() override
    {
      pet_melee_attack_t<ghoul_pet_t>::execute();
      if ( triggers_apocalypse && dk()->runeforge.rune_of_apocalypse && hit_any_target )
      {
        int n = as<int>( pet()->rng().range( 0, runeforge_apocalypse::MAX ) );

        death_knight_td_t* td = dk()->get_target_data( target );

        switch ( n )
        {
          case runeforge_apocalypse::DEATH:
            td->debuff.apocalypse_death->trigger();
            break;
          case runeforge_apocalypse::FAMINE:
            td->debuff.apocalypse_famine->trigger();
            break;
          case runeforge_apocalypse::PESTILENCE:
            dk()->active_spells.runeforge_pestilence->execute_on_target( target );
            break;
          case runeforge_apocalypse::WAR:
            td->debuff.apocalypse_war->trigger();
            break;
        }
      }
    }

    void impact( action_state_t* state ) override
    {
      pet_melee_attack_t<ghoul_pet_t>::impact( state );

      if ( triggers_infected_claws && dk()->talent.unholy.infected_claws.ok() &&
           rng().roll( dk()->talent.unholy.infected_claws->effectN( 1 ).percent() ) )
      {
        dk()->trigger_festering_wound( state, 1, dk()->procs.fw_infected_claws );
      }
    }

    bool ready() override
    {
      if ( usable_in_dt !=
           ( dk()->specialization() == DEATH_KNIGHT_UNHOLY && dk()->buffs.dark_transformation->check() > 0 ) )
      {
        return false;
      }

      return pet_melee_attack_t<ghoul_pet_t>::ready();
    }

  private:
    bool usable_in_dt;

  public:
    bool triggers_infected_claws;
    bool triggers_apocalypse;
  };

  struct claw_t final : public dt_melee_ability_t
  {
    claw_t( ghoul_pet_t* p, util::string_view options_str )
      : dt_melee_ability_t( p, "claw", p->dk()->pet_spell.ghoul_claw, false )
    {
      parse_options( options_str );
      triggers_infected_claws = triggers_apocalypse = true;
    }
  };

  struct sweeping_claws_t final : public dt_melee_ability_t
  {
    sweeping_claws_t( ghoul_pet_t* p, util::string_view options_str )
      : dt_melee_ability_t( p, "sweeping_claws", p->dk()->pet_spell.sweeping_claws, true )
    {
      parse_options( options_str );
      aoe                     = -1;
      triggers_infected_claws = triggers_apocalypse = true;
    }
  };

  struct gnaw_t final : public dt_melee_ability_t
  {
    gnaw_t( ghoul_pet_t* p, util::string_view options_str )
      : dt_melee_ability_t( p, "gnaw", p->dk()->pet_spell.gnaw, false )
    {
      parse_options( options_str );
      cooldown = p->get_cooldown( "gnaw" );
    }
  };

  struct monstrous_blow_t final : public dt_melee_ability_t
  {
    monstrous_blow_t( ghoul_pet_t* p, util::string_view options_str )
      : dt_melee_ability_t( p, "monstrous_blow", p->dk()->pet_spell.monstrous_blow, true )
    {
      parse_options( options_str );
      cooldown = p->get_cooldown( "gnaw" );
    }
  };

  ghoul_pet_t( death_knight_t* owner, bool guardian = true ) : base_ghoul_pet_t( owner, "ghoul", guardian )
  {
    gnaw_cd                = get_cooldown( "gnaw" );
    gnaw_cd->duration      = owner->pet_spell.gnaw->cooldown();
    owner_coeff.ap_from_ap = 0.6318;
    if ( owner->talent.unholy.raise_dead.ok() && !owner->talent.sacrificial_pact.ok() )
    {
      dynamic = false;
    }
  }

  attack_t* create_main_hand_auto_attack() override
  {
    return new auto_attack_melee_t( this );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = base_ghoul_pet_t::composite_player_multiplier( school );

    if ( dk()->specialization() == DEATH_KNIGHT_UNHOLY )
    {
      m *= 1.0 + dk()->buffs.dark_transformation->value();

      if ( ghoulish_frenzy->check() )
        m *= 1.0 + ghoulish_frenzy->value();
    }

    return m;
  }

  double composite_melee_auto_attack_speed() const override
  {
    double haste = base_ghoul_pet_t::composite_melee_auto_attack_speed();

    if ( ghoulish_frenzy->check() )
      haste *= 1.0 / ( 1.0 + ghoulish_frenzy->data().effectN( 2 ).percent() );

    return haste;
  }

  void init_gains() override
  {
    base_ghoul_pet_t::init_gains();

    dark_transformation_gain = get_gain( "Dark Transformation" );
  }

  void init_action_list() override
  {
    base_ghoul_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    if ( dk()->talent.unholy.dark_transformation.ok() )
    {
      def->add_action( "sweeping_claws" );
      def->add_action( "claw,if=energy>70" );
      def->add_action( "monstrous_blow" );
      def->add_action( "Gnaw" );
    }
    else
    {
      def->add_action( "claw,if=energy>70" );
      def->add_action( "Gnaw" );
    }
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "claw" )
      return new claw_t( this, options_str );
    if ( name == "gnaw" )
      return new gnaw_t( this, options_str );
    if ( name == "sweeping_claws" )
      return new sweeping_claws_t( this, options_str );
    if ( name == "monstrous_blow" )
      return new monstrous_blow_t( this, options_str );

    return base_ghoul_pet_t::create_action( name, options_str );
  }

  void create_buffs() override
  {
    base_ghoul_pet_t::create_buffs();

    ghoulish_frenzy = make_buff( this, "ghoulish_frenzy", dk()->pet_spell.ghoulish_frenzy )
                          ->set_default_value( dk()->pet_spell.ghoulish_frenzy->effectN( 1 ).percent() )
                          ->apply_affecting_aura( dk()->talent.unholy.ghoulish_frenzy )
                          ->set_duration( 0_s )
                          ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                          ->add_invalidate( CACHE_AUTO_ATTACK_SPEED );
  }

private:
  cooldown_t* gnaw_cd;  // shared cd between gnaw/monstrous_blow

public:
  gain_t* dark_transformation_gain;
  buff_t* ghoulish_frenzy;
};

// ==========================================================================
// Army of the Dead and Apocalypse ghouls
// ==========================================================================

struct army_ghoul_pet_t final : public base_ghoul_pet_t
{
  struct army_claw_t : public pet_melee_attack_t<army_ghoul_pet_t>
  {
    army_claw_t( army_ghoul_pet_t* p, util::string_view options_str )
      : pet_melee_attack_t( p, "claw", p->dk()->pet_spell.army_claw )
    {
      parse_options( options_str );
    }
  };

  army_ghoul_pet_t( death_knight_t* owner, util::string_view name = "army_ghoul" )
    : base_ghoul_pet_t( owner, name, true )
  {
    affected_by_commander_of_the_dead = true;
    decomposition_can_extend          = true;
    tww1_4pc_proc                     = true;
  }

  void init_base_stats() override
  {
    base_ghoul_pet_t::init_base_stats();

    // Shares a value with Magus of the Dead, stored in death_knight_pet_t
    // Ensures parity between all pets that share this ap_from_ap mod.
    owner_coeff.ap_from_ap = army_ghoul_ap_mod;

    if ( name_str == "army_ghoul" )
    {
      // Currently has a 0.75x modiier, doesnt appear to be in spell data anywhere
      owner_coeff.ap_from_ap *= 0.75;
    }

    if ( name_str == "apoc_ghoul" )
    {
      // Currently has a 1.12x modifier, also not in spell data
      owner_coeff.ap_from_ap *= 0.9996;
    }
  }

  void init_spells() override
  {
    base_ghoul_pet_t::init_spells();

    if ( dk()->talent.unholy.ruptured_viscera.ok() )
    {
      ruptured_viscera = new ruptured_viscera_t<army_ghoul_pet_t>( this );
    }
  }

  void init_gains() override
  {
    base_ghoul_pet_t::init_gains();

    // Group Energy regen for the same pets together to reduce report clutter
    if ( !dk()->options.split_ghoul_regen && dk()->find_pet( name_str ) )
    {
      this->gains.resource_regen = dk()->find_pet( name_str )->gains.resource_regen;
    }
  }

  void init_action_list() override
  {
    base_ghoul_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "claw" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "claw" )
      return new army_claw_t( this, options_str );

    return base_ghoul_pet_t::create_action( name, options_str );
  }

  void dismiss( bool expired = false ) override
  {
    if ( !sim->event_mgr.canceled && dk()->talent.unholy.ruptured_viscera.ok() )
    {
      ruptured_viscera->execute_on_target( target );
    }

    death_knight_pet_t::dismiss( expired );
  }

private:
  pet_spell_t<army_ghoul_pet_t>* ruptured_viscera;
};

// ==========================================================================
// Gargoyle
// ==========================================================================
struct gargoyle_pet_t : public death_knight_pet_t
{
  gargoyle_pet_t( death_knight_t* owner )
    : death_knight_pet_t( owner, "gargoyle", true, false ), gargoyle_strike( nullptr ), dark_empowerment( nullptr )
  {
    resource_regeneration             = regen_type::DISABLED;
    affected_by_commander_of_the_dead = true;
    decomposition_can_extend          = true;
    tww1_4pc_proc                     = true;
  }

  struct gargoyle_strike_t : public pet_spell_t<gargoyle_pet_t>
  {
    gargoyle_strike_t( gargoyle_pet_t* p ) : pet_spell_t( p, "gargoyle_strike", p->dk()->pet_spell.gargoyle_strike )
    {
      background = repeating = true;
    }
  };

  void arise() override
  {
    death_knight_pet_t::arise();
    timespan_t duration = 2.8_s;
    buffs.stunned->trigger( duration + rng().gauss<200, 25>() );
    stun();
    reschedule_gargoyle();
    // Gargoyle procs 2 stacks of this oddly, duplicate it here to emulate that
    if ( dk()->sets->has_set_bonus( DEATH_KNIGHT_UNHOLY, TWW1, B4 ) && tww1_4pc_proc )
    {
      dk()->buffs.unholy_commander->trigger();
    }
  }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 1.0;
  }

  double composite_player_multiplier( school_e s ) const override
  {
    double m = death_knight_pet_t::composite_player_multiplier( s );

    m *= 1.0 + dark_empowerment->stack_value();

    return m;
  }

  void create_buffs() override
  {
    death_knight_pet_t::create_buffs();

    dark_empowerment = make_buff( this, "dark_empowerment", dk()->pet_spell.dark_empowerment )
                           ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  void create_actions() override
  {
    death_knight_pet_t::create_actions();
    gargoyle_strike = new gargoyle_strike_t( this );
  }

  void reschedule_gargoyle()
  {
    if ( executing || is_sleeping() || buffs.movement->check() || buffs.stunned->check() )
      return;

    else
    {
      gargoyle_strike->set_target( dk()->target );
      gargoyle_strike->schedule_execute();
    }
  }

  void schedule_ready( timespan_t /* delta_time */, bool /* waiting */ ) override
  {
    reschedule_gargoyle();
  }

  // Function that increases the gargoyle's dark empowerment buff value based on RP spent
  void increase_power( double rp_spent )
  {
    if ( is_sleeping() )
    {
      return;
    }

    double increase = rp_spent * dark_empowerment->data().effectN( 1 ).percent() /
                      dk()->talent.unholy.summon_gargoyle->effectN( 4 ).base_value();

    if ( !dark_empowerment->check() )
    {
      dark_empowerment->trigger( 1, increase );
    }
    else
    {
      sim->print_debug( "{} increasing shadow_empowerment power by {}", name(), increase );
      dark_empowerment->current_value += increase;
    }
  }

private:
  pet_spell_t<gargoyle_pet_t>* gargoyle_strike;

public:
  buff_t* dark_empowerment;
};

// ==========================================================================
// Risen Skulker (All Will Serve talent)
// ==========================================================================

struct risen_skulker_pet_t : public death_knight_pet_t
{
  risen_skulker_pet_t( death_knight_t* owner )
    : death_knight_pet_t( owner, "risen_skulker", true, false, false ), skulker_shot( nullptr )
  {
    resource_regeneration       = regen_type::DISABLED;
    main_hand_weapon.type       = WEAPON_BEAST_RANGED;
    main_hand_weapon.swing_time = 2.7_s;

    // Using a background repeating action as a replacement for a foreground action. Change Ready Type to trigger so we
    // can wake up the pet when it needs to re-execute this action.
    ready_type = READY_TRIGGER;
  }

  struct skulker_shot_t : public pet_spell_t<risen_skulker_pet_t>
  {
    skulker_shot_t( risen_skulker_pet_t* p )
      : pet_spell_t<risen_skulker_pet_t>( p, "skulker_shot", p->dk()->pet_spell.skulker_shot )
    {
      weapon     = &( p->main_hand_weapon );
      background = true;
      // For some reason, Risen Skulker deals double damage to its main target, and normal damage to the other targets
      base_multiplier *= 2.0;
      aoe                 = -1;
      base_aoe_multiplier = 0.5;
      repeating           = true;
    }

    void schedule_execute( action_state_t* state ) override
    {
      pet_spell_t<risen_skulker_pet_t>::schedule_execute( state );
      // This pet uses a background repeating event with a ready type of READY_TRIGGER. Without constantly re-updating
      // the started waiting trigger_ready would never function.
      player->started_waiting = sim->current_time();
    }
  };

  void acquire_target( retarget_source event, player_t* context ) override
  {
    if ( skulker_shot->execute_event && skulker_shot->target->is_sleeping() )
    {
      event_t::cancel( skulker_shot->execute_event );
      started_waiting = sim->current_time();
    }

    player_t::acquire_target( event, context );

    if ( !skulker_shot->execute_event )
      trigger_ready();
  }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 1.0;
  }

  void create_actions() override
  {
    death_knight_pet_t::create_actions();
    skulker_shot = new skulker_shot_t( this );
  }

  void reschedule_skulker()
  {
    // Have to check the presecnce of a skulker_shot->execute_event because this acts as our "executing" due to using a
    // background repeating action. We do not wish to have multiple of these.
    if ( executing || skulker_shot->execute_event || is_sleeping() || buffs.movement->check() ||
         buffs.stunned->check() )
      return;

    skulker_shot->set_target( dk()->target );
    skulker_shot->schedule_execute();
  }

  void arise() override
  {
    death_knight_pet_t::arise();
    reschedule_skulker();
  }

  void schedule_ready( timespan_t /* delta_time */, bool /* waiting */ ) override
  {
    reschedule_skulker();
  }

private:
  pet_spell_t<risen_skulker_pet_t>* skulker_shot;
};

// ==========================================================================
// Dancing Rune Weapon
// ==========================================================================

struct dancing_rune_weapon_pet_t : public death_knight_pet_t
{
  template <typename T_ACTION>
  struct drw_action_t : public pet_action_t<dancing_rune_weapon_pet_t, T_ACTION>
  {
    drw_action_t( dancing_rune_weapon_pet_t* p, util::string_view name, const spell_data_t* s )
      : pet_action_t<dancing_rune_weapon_pet_t, T_ACTION>( p, name, s )
    {
      this->background = true;
      this->weapon     = &( p->main_hand_weapon );
    }

    // Override verify actor spec, the pet's abilities are blood's abilities and require blood spec in spelldata
    // The pet can only be spawned in blood spec and all the method does is set the ability to background=true
    // Which doesn't stop us from using DRW abilities anyway (they're called directly with -> execute())
    bool verify_actor_spec() const override
    {
      return true;
    }
  };

  struct blood_plague_t : public drw_action_t<spell_t>
  {
    double snapshot_coagulopathy;
    blood_plague_t( util::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t<spell_t>( p, n, p->dk()->spell.blood_plague )
    {
      snapshot_coagulopathy = 0.0;
    }

    double composite_ta_multiplier( const action_state_t* state ) const override
    {
      double m = drw_action_t::composite_ta_multiplier( state );

      // DRW snapshots the coag stacks when the weapon demises, so if DRW is down, use the snapshot value
      if ( dk()->buffs.dancing_rune_weapon->up() )
        m *= 1.0 + dk()->buffs.coagulopathy->stack_value();
      else
        m *= 1.0 + snapshot_coagulopathy;

      return m;
    }

    void tick( dot_t* dot ) override
    {
      drw_action_t::tick( dot );

      // Snapshot damage amp, as it's constant if drw demises
      if ( dk()->buffs.dancing_rune_weapon->up() )
        snapshot_coagulopathy = dk()->buffs.coagulopathy->stack_value();
    }

    void execute() override
    {
      drw_action_t::execute();
      snapshot_coagulopathy = 0.0;
    }
  };

  struct blood_boil_t : public drw_action_t<spell_t>
  {
    blood_boil_t( util::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t<spell_t>( p, n, p->dk()->talent.blood.blood_boil )
    {
      aoe                 = -1;
      cooldown->duration  = 0_ms;
      this->impact_action = p->ability.blood_plague;
    }
  };

  struct consumption_t : public drw_action_t<melee_attack_t>
  {
    consumption_t( util::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t( p, n, p->dk()->talent.blood.consumption )
    {
      aoe                 = -1;
      reduced_aoe_targets = data().effectN( 3 ).base_value();
    }
  };

  struct deaths_caress_t : public drw_action_t<spell_t>
  {
    int stack_gain;
    deaths_caress_t( util::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t( p, n, p->dk()->spec.deaths_caress ), stack_gain( as<int>( data().effectN( 3 ).base_value() ) )

    {
      this->impact_action = p->ability.blood_plague;
    }

    void impact( action_state_t* state ) override
    {
      drw_action_t::impact( state );

      dk()->buffs.bone_shield->trigger( stack_gain );
      if ( dk() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, TWW1, B4 ) )
        dk() -> buffs.piledriver_tww1_4pc->trigger( stack_gain );
    }
  };

  struct death_strike_t : public drw_action_t<melee_attack_t>
  {
    death_strike_t( util::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t<melee_attack_t>( p, n, p->dk()->talent.death_strike )
    {
    }
  };

  struct heart_strike_t : public drw_action_t<melee_attack_t>
  {
    double blood_strike_rp_generation;

    heart_strike_t( util::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t<melee_attack_t>( p, n, p->dk()->pet_spell.drw_heart_strike ),
        // DRW is still using an old spell called "Blood Strike" for the 5 additional RP generation on Heart Strike
        blood_strike_rp_generation(
            dk()->pet_spell.drw_heart_strike_rp_gen->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) )
    {
    }

    int n_targets() const override
    {
      return dk()->in_death_and_decay() ? aoe + as<int>( dk()->talent.cleaving_strikes->effectN( 3 ).base_value() )
                                        : aoe;
    }

    void execute() override
    {
      drw_action_t::execute();

      dk()->resource_gain( RESOURCE_RUNIC_POWER, blood_strike_rp_generation, dk()->gains.drw_heart_strike, this );
    }
  };

  struct vampiric_strike_t : public drw_action_t<melee_attack_t>
  {
    vampiric_strike_t( util::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t<melee_attack_t>( p, n, p->dk()->spell.vampiric_strike )
    {
      attack_power_mod.direct = data().effectN( 5 ).ap_coeff();
    }
  };

  struct marrowrend_t : public drw_action_t<melee_attack_t>
  {
    int stack_gain;
    marrowrend_t( util::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t<melee_attack_t>( p, n, p->dk()->talent.blood.marrowrend ),
        stack_gain( as<int>( data().effectN( 3 ).base_value() ) )
    {
    }

    void impact( action_state_t* state ) override
    {
      drw_action_t::impact( state );

      dk()->buffs.bone_shield->trigger( stack_gain );
      if ( dk() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, TWW1, B4 ) )
        dk() -> buffs.piledriver_tww1_4pc->trigger( stack_gain );
    }
  };

  struct soul_reaper_execute_t : public drw_action_t<spell_t>
  {
    soul_reaper_execute_t( util::string_view name, dancing_rune_weapon_pet_t* p )
      : drw_action_t<spell_t>( p, name, p->dk()->spell.soul_reaper_execute )
    {
      background = true;
    }
  };

  struct soul_reaper_t : public drw_action_t<melee_attack_t>
  {
    soul_reaper_t( util::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t<melee_attack_t>( p, n, p->dk()->talent.soul_reaper ),
        soul_reaper_execute( get_action<soul_reaper_execute_t>( "soul_reaper_execute", p ) )
    {
      if ( dk()->options.individual_pet_reporting )
      {
        add_child( soul_reaper_execute );
      }
      hasted_ticks = false;
      dot_behavior = DOT_EXTEND;
    }

    void tick( dot_t* dot ) override
    {
      if( dot->target->health_percentage() < data().effectN( 3 ).base_value() )
      {
        // If DRW is still up, fire drw soul reaper execute, otherwise fire from the DK, with full DK damage mods
        if ( dk()->buffs.dancing_rune_weapon->up() )
          soul_reaper_execute->execute_on_target( dot->target );
        else
          dk()->active_spells.soul_reaper_execute_expired_drw->execute_on_target( dot->target );
      }
    }

  private:
    propagate_const<action_t*> soul_reaper_execute;
  };

  struct soul_reaper_grim_reaper_t : public drw_action_t<melee_attack_t>
  {
    soul_reaper_grim_reaper_t( util::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t<melee_attack_t>( p, n, p->dk()->spell.grim_reaper_soul_reaper ),
        soul_reaper_execute( get_action<soul_reaper_execute_t>( "soul_reaper_grim_reaper_execute", p ) )
    {
      if ( dk()->options.individual_pet_reporting )
      {
        add_child( soul_reaper_execute );
      }
      hasted_ticks = false;
      dot_behavior = DOT_EXTEND;
    }

    void tick( dot_t* dot ) override
    {
     if( dot->target->health_percentage() < data().effectN( 3 ).base_value() )
      {
        // If DRW is still up, fire drw soul reaper execute, otherwise fire from the DK, with full DK damage mods
        if ( dk()->buffs.dancing_rune_weapon->up() )
          soul_reaper_execute->execute_on_target( dot->target );
        else
          dk()->active_spells.soul_reaper_execute_expired_drw->execute_on_target( dot->target );
      }
    }

  private:
    propagate_const<action_t*> soul_reaper_execute;
  };

  struct abilities_t
  {
    action_t* blood_plague;
    action_t* blood_boil;
    action_t* deaths_caress;
    action_t* death_strike;
    action_t* heart_strike;
    action_t* marrowrend;
    action_t* soul_reaper;
    action_t* grim_reaper_soul_reaper;
    action_t* consumption;
    action_t* vampiric_strike;
  } ability;

  dancing_rune_weapon_pet_t( death_knight_t* owner, util::string_view drw_name = "dancing_rune_weapon" )
    : death_knight_pet_t( owner, drw_name, true, true ), ability()
  {
    // The pet wields the same weapon type as its owner for spells with weapon requirements
    main_hand_weapon.type       = owner->main_hand_weapon.type;
    main_hand_weapon.swing_time = 3.5_s;

    owner_coeff.ap_from_ap = 1 / 3.0;
    resource_regeneration  = regen_type::DISABLED;
  }

  void init_spells() override
  {
    death_knight_pet_t::init_spells();

    // Dont init spells that dont exist, breaks reporting for auto's
    if ( dk()->talent.blood.blood_boil.ok() )
    {
      ability.blood_plague = get_action<blood_plague_t>( "blood_plague", this );
      ability.blood_boil   = get_action<blood_boil_t>( "blood_boil", this );
    }
    if ( dk()->spec.deaths_caress->ok() )
    {
      ability.deaths_caress = get_action<deaths_caress_t>( "deaths_caress", this );
    }
    if ( dk()->talent.death_strike.ok() )
    {
      ability.death_strike = get_action<death_strike_t>( "death_strike", this );
    }
    if ( dk()->talent.blood.heart_strike.ok() )
    {
      ability.heart_strike = get_action<heart_strike_t>( "heart_strike", this );
    }
    if ( dk()->talent.blood.marrowrend.ok() )
    {
      ability.marrowrend = get_action<marrowrend_t>( "marrowrend", this );
    }
    if ( dk()->talent.soul_reaper.ok() )
    {
      ability.soul_reaper = get_action<soul_reaper_t>( "soul_reaper", this );
    }
    if ( dk()->talent.deathbringer.grim_reaper.ok() )
    {
      ability.grim_reaper_soul_reaper = get_action<soul_reaper_grim_reaper_t>( "soul_reaper_grim_reaper", this );
    }
    if ( dk()->talent.blood.consumption.ok() )
    {
      ability.consumption = get_action<consumption_t>( "consumption", this );
    }
    if ( dk()->talent.sanlayn.vampiric_strike.ok() )
    {
      ability.vampiric_strike = get_action<vampiric_strike_t>( "vampiric_strike", this );
    }
  }

  void arise() override
  {
    death_knight_pet_t::arise();
    dk()->buffs.dancing_rune_weapon->trigger();
  }

  void demise() override
  {
    death_knight_pet_t::demise();
    dk()->buffs.dancing_rune_weapon->expire();
  }

  attack_t* create_main_hand_auto_attack() override
  {
    return new auto_attack_melee_t<dancing_rune_weapon_pet_t>( this );
  }
};

// ==========================================================================
// Bloodworms
// ==========================================================================

struct bloodworm_pet_t : public death_knight_pet_t
{
  bloodworm_pet_t( death_knight_t* owner ) : death_knight_pet_t( owner, "bloodworm", true, true )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = 1.4_s;

    owner_coeff.ap_from_ap = 0.25;
    resource_regeneration  = regen_type::DISABLED;
  }

  attack_t* create_main_hand_auto_attack() override
  {
    return new auto_attack_melee_t<bloodworm_pet_t>( this );
  }
};

// ==========================================================================
// Magus of the Dead (Talent)
// ==========================================================================

struct magus_pet_t : public death_knight_pet_t
{
  struct magus_spell_t : public pet_spell_t<magus_pet_t>
  {
    magus_spell_t( magus_pet_t* p, util::string_view name, const spell_data_t* spell, util::string_view options_str )
      : pet_spell_t( p, name, spell )
    {
      parse_options( options_str );
    }

    // There's a 1 energy cost in spelldata but it might as well be ignored
    double cost() const override
    {
      return 0;
    }
  };

  struct frostbolt_magus_t final : public magus_spell_t
  {
    frostbolt_magus_t( magus_pet_t* p, util::string_view options_str )
      : magus_spell_t( p, "frostbolt", p->dk()->pet_spell.frostbolt, options_str )
    {
      // If the target is immune to slows, frostbolt seems to be used at most every 6 seconds
      cooldown->duration = dk()->pet_spell.frostbolt->duration();
    }

    void execute() override
    {
      // Magus of the Dead waits a little bit after executing Frost Bolt
      trigger_gcd = execute_time() + rng().range( 0_ms, 410_ms );
      magus_spell_t::execute();
    }

    // Frostbolt applies a slowing debuff on non-boss targets
    // This is needed because Frostbolt won't ever target an enemy affected by the debuff
    void impact( action_state_t* state ) override
    {
      magus_spell_t::impact( state );

      if ( result_is_hit( state->result ) && state->target->type == ENEMY_ADD )
      {
        pet()->get_target_data( state->target )->debuff.frostbolt_debuff->trigger();
      }
    }
  };

  struct shadow_bolt_magus_t final : public magus_spell_t
  {
    shadow_bolt_magus_t( magus_pet_t* p, util::string_view options_str )
      : magus_spell_t( p, "shadow_bolt", p->dk()->pet_spell.shadow_bolt, options_str )
    {
    }
  };

  magus_pet_t( death_knight_t* owner, util::string_view name = "army_magus" )
    : death_knight_pet_t( owner, name, true, false )
  {
    resource_regeneration             = regen_type::DISABLED;
    affected_by_commander_of_the_dead = true;
    decomposition_can_extend          = true;
    tww1_4pc_proc                     = true;
    is_magus                          = true;
    npc_id                            = 163366;
  }

  void init_spells() override
  {
    death_knight_pet_t::init_spells();
    if ( dk()->talent.unholy.ruptured_viscera.ok() )
    {
      ruptured_viscera = new ruptured_viscera_t<magus_pet_t>( this, true );
    }
  }

  void dismiss( bool expired = false ) override
  {
    if ( !sim->event_mgr.canceled && dk()->talent.unholy.ruptured_viscera.ok() )
    {
      ruptured_viscera->execute_on_target( target );
    }

    death_knight_pet_t::dismiss( expired );
  }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    // Shares a value with Army of the Dead/Apocalypse, stored in death_knight_pet_t
    // Ensures parity between all pets that share this ap_from_ap mod.
    owner_coeff.ap_from_ap = army_ghoul_ap_mod;
    owner_coeff.ap_from_ap *= 0.85;
  }

  void init_action_list() override
  {
    death_knight_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "frostbolt" );
    def->add_action( "shadow_bolt" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "frostbolt" )
      return new frostbolt_magus_t( this, options_str );
    if ( name == "shadow_bolt" )
      return new shadow_bolt_magus_t( this, options_str );

    return death_knight_pet_t::create_action( name, options_str );
  }

private:
  pet_spell_t<magus_pet_t>* ruptured_viscera;
};

// ==========================================================================
// Blood Beast
// ==========================================================================
struct blood_beast_pet_t : public death_knight_pet_t
{
  struct blood_beast_melee_t : public auto_attack_melee_t<blood_beast_pet_t>
  {
    blood_beast_melee_t( blood_beast_pet_t* p ) : auto_attack_melee_t( p )
    {
    }

    void impact( action_state_t* state ) override
    {
      auto_attack_melee_t::impact( state );
      if ( state->result_amount > 0 )
      {
        pet()->accumulator += state->result_amount;
      }
    }
  };
  struct corrupted_blood_t : public pet_melee_attack_t<blood_beast_pet_t>
  {
    corrupted_blood_t( blood_beast_pet_t* p, util::string_view options_str )
      : pet_melee_attack_t( p, "corrupted_blood", p->dk()->pet_spell.corrupted_blood )
    {
      parse_options( options_str );
      aoe = -1;
      cooldown->duration = 8_s; // Emulating odd behavior where the pet casts this very infrequently
    }

    void impact( action_state_t* state ) override
    {
      pet_melee_attack_t::impact( state );
      if ( state->result_amount > 0 )
      {
        pet()->accumulator += state->result_amount;
      }
    }
  };

  blood_beast_pet_t( death_knight_t* owner ) : death_knight_pet_t( owner, "blood_beast", true, true ), accumulator( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = 1_s;
    npc_id                      = owner->find_spell( 434237 )->effectN( 1 ).misc_value1();
    owner_coeff.ap_from_ap      = 0.5565;
    resource_regeneration       = regen_type::DISABLED;
    blood_beast_mod             = dk()->specialization() == DEATH_KNIGHT_BLOOD
                                      ? dk()->talent.sanlayn.the_blood_is_life->effectN( 1 ).percent()
                                      : dk()->talent.sanlayn.the_blood_is_life->effectN( 2 ).percent();
  }

  void dismiss( bool expired = false ) override
  {
    if ( !sim->event_mgr.canceled )
    {
      dk()->active_spells.the_blood_is_life->execute_on_target( dk()->target, accumulator * blood_beast_mod );
      accumulator = 0;
    }

    pet_t::dismiss( expired );
  }

  void arise() override
  {
    death_knight_pet_t::arise();
    accumulator = 0;
  }

  void reset() override
  {
    death_knight_pet_t::reset();
    accumulator = 0;
  }

  void init_spells() override
  {
    death_knight_pet_t::init_spells();
  }

  void init_action_list() override
  {
    death_knight_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "corrupted_blood" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "corrupted_blood" )
      return new corrupted_blood_t( this, options_str );

    return death_knight_pet_t::create_action( name, options_str );
  }

  attack_t* create_main_hand_auto_attack() override
  {
    return new blood_beast_melee_t( this );
  }

public:
  double accumulator;

private:
  double blood_beast_mod;
};

// ==========================================================================
// Horsemen Parent Class
// ==========================================================================
struct horseman_pet_t : public death_knight_pet_t
{
  struct horseman_spell_t : public pet_spell_t<horseman_pet_t>
  {
    horseman_spell_t( horseman_pet_t* p, util::string_view name, const spell_data_t* spell )
      : pet_spell_t( p, name, spell )
    {
    }
  };

  struct horseman_melee_t : public pet_melee_attack_t<horseman_pet_t>
  {
    horseman_melee_t( horseman_pet_t* p, util::string_view name, const spell_data_t* spell )
      : pet_melee_attack_t( p, name, spell )
    {
    }
  };

  struct horseman_ams_t : public horseman_spell_t
  {
    horseman_ams_t( util::string_view name, horseman_pet_t* p, util::string_view options_str )
      : horseman_spell_t( p, name, p->dk()->pet_spell.rider_ams )
    {
      parse_options( options_str );
      trigger_gcd = 1_s;
    }

    void init_finished() override
    {
      horseman_spell_t::init_finished();
      if ( pet()->npc_id == pet()->dk()->spell.summon_whitemane->effectN( 1 ).misc_value1() )
      {
        pet()->dk()->cooldown.whitemane_ams_cd           = pet()->dk()->get_cooldown( "whitemane_ams_cd", this );
        pet()->dk()->cooldown.whitemane_ams_cd->duration = cooldown->duration;
        cooldown                                         = pet()->dk()->cooldown.whitemane_ams_cd;
      }
      if ( pet()->npc_id == pet()->dk()->spell.summon_mograine->effectN( 1 ).misc_value1() )
      {
        pet()->dk()->cooldown.mograine_ams_cd           = pet()->dk()->get_cooldown( "mograine_ams_cd", this );
        pet()->dk()->cooldown.mograine_ams_cd->duration = cooldown->duration;
        cooldown                                        = pet()->dk()->cooldown.mograine_ams_cd;
      }
      if ( pet()->npc_id == pet()->dk()->spell.summon_nazgrim->effectN( 1 ).misc_value1() )
      {
        pet()->dk()->cooldown.nazgrim_ams_cd           = pet()->dk()->get_cooldown( "nazgrim_ams_cd", this );
        pet()->dk()->cooldown.nazgrim_ams_cd->duration = cooldown->duration;
        cooldown                                       = pet()->dk()->cooldown.nazgrim_ams_cd;
      }
      if ( pet()->npc_id == pet()->dk()->spell.summon_trollbane->effectN( 1 ).misc_value1() )
      {
        pet()->dk()->cooldown.trollbane_ams_cd           = pet()->dk()->get_cooldown( "trollbane_ams_cd", this );
        pet()->dk()->cooldown.trollbane_ams_cd->duration = cooldown->duration;
        cooldown                                         = pet()->dk()->cooldown.trollbane_ams_cd;
      }
    }

    void execute() override
    {
      horseman_spell_t::execute();
      if ( !pet()->dk()->buffs.antimagic_shell_horsemen_icd->check() && dk()->talent.rider.horsemens_aid.ok() )
      {
        set_target( pet()->dk() );
        pet()->dk()->buffs.antimagic_shell_horsemen->trigger();
        pet()->dk()->buffs.antimagic_shell_horsemen_icd->trigger();
      }
      else
        set_target( pet() );
    }
  };

  horseman_pet_t( death_knight_t* owner, util::string_view name, bool guardian = true, bool dynamic = true )
    : death_knight_pet_t( owner, name, guardian, true, dynamic ), rp_spent( 0 ), current_pool( 0 )
  {
    main_hand_weapon.type  = WEAPON_BEAST_2H;
    owner_coeff.ap_from_ap = 0.85;
    resource_regeneration  = regen_type::DISABLED;
    auto_attack_multiplier = 1.25;
  }

  void arise() override
  {
    death_knight_pet_t::arise();
    rp_spent     = 0;
    current_pool = 0;
    dk()->active_riders++;
  }

  void demise() override
  {
    death_knight_pet_t::demise();
    rp_spent     = 0;
    current_pool = 0;
    dk()->active_riders--;
  }

  void reset() override
  {
    death_knight_pet_t::reset();
    rp_spent     = 0;
    current_pool = 0;
  }

  void init_action_list() override
  {
    death_knight_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    // Limit them casting AMS to only if this is talented and may impact player dps
    def->add_action( "antimagic_shell_horsemen" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "antimagic_shell_horsemen" )
      return new horseman_ams_t( name, this, options_str );

    return death_knight_pet_t::create_action( name, options_str );
  }

  attack_t* create_main_hand_auto_attack() override
  {
    return new auto_attack_melee_t<horseman_pet_t>( this );
  }

  attack_t* create_off_hand_auto_attack() override
  {
    if ( off_hand_weapon.type != WEAPON_NONE )
    {
      return new auto_attack_melee_t<horseman_pet_t>( this, "auto_attack_oh" );
    }
    else
    {
      return nullptr;
    }
  }

public:
  double rp_spent, current_pool;
};

// ==========================================================================
// Mograine Pet
// ==========================================================================
struct mograine_pet_t final : public horseman_pet_t
{
  struct dnd_damage_mograine_t final : public horseman_spell_t
  {
    dnd_damage_mograine_t( util::string_view name, horseman_pet_t* p )
      : horseman_spell_t( p, name, p->dk()->spell.death_and_decay_damage )
    {
      background = true;
      aoe        = -1;
    }
  };

  struct dnd_aura_t final : public buff_t
  {
    dnd_aura_t( horseman_pet_t* p )
      : buff_t( p, "death_and_decay", p->dk()->pet_spell.mograines_death_and_decay ), dk( p->dk() )
    {
      set_tick_zero( true );
      set_tick_callback( [ this ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ ) {
        dk->active_spells.mograines_death_and_decay->execute();
      } );
    }

    void start( int stacks, double value, timespan_t duration ) override
    {
      buff_t::start( stacks, value, duration );
      if ( dk->talent.rider.mograines_might.ok() )
      {
        dk->buffs.mograines_might->trigger();
        dk->buffs.death_and_decay->trigger();
      }
    }

    mograine_pet_t* mograine() const
    {
      return debug_cast<mograine_pet_t*>( player );
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      buff_t::expire_override( expiration_stacks, remaining_duration );
      if ( dk->talent.rider.mograines_might.ok() )
      {
        dk->buffs.mograines_might->expire();
        dk->buffs.death_and_decay->expire();
      }
      if ( mograine()->extended_by_apoc_now )
      {
        mograine()->extended_by_apoc_now = false;
        // Triggers again 100_ms after the buff expires
        make_event( *sim, 100_ms, [ & ]() { trigger(); } );
      }
    }

  private:
    death_knight_t* dk;
  };

  struct heart_strike_mograine_t final : public horseman_melee_t
  {
    heart_strike_mograine_t( util::string_view name, horseman_pet_t* p, util::string_view options_str )
      : horseman_melee_t( p, name, p->dk()->pet_spell.mograine_heart_strike )
    {
      parse_options( options_str );
    }
  };

  void create_buffs() override
  {
    death_knight_pet_t::create_buffs();
    dnd_aura = new dnd_aura_t( this );
  }

  mograine_pet_t( death_knight_t* owner ) : horseman_pet_t( owner, "mograine" )
  {
    npc_id                      = owner->spell.summon_mograine->effectN( 1 ).misc_value1();
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = 2_s;
    off_hand_weapon.type        = WEAPON_BEAST;
    off_hand_weapon.swing_time  = 2_s;
  }

  void init_spells() override
  {
    horseman_pet_t::init_spells();
    dk()->active_spells.mograines_death_and_decay = get_action<dnd_damage_mograine_t>( "death_and_decay", this );
  }

  void arise() override
  {
    horseman_pet_t::arise();
    dnd_aura->trigger();
  }

  void init_action_list() override
  {
    horseman_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "heart_strike" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "heart_strike" )
      return new heart_strike_mograine_t( "heart_strike", this, options_str );

    return horseman_pet_t::create_action( name, options_str );
  }

public:
  buff_t* dnd_aura;
  bool extended_by_apoc_now = false;
};

// ==========================================================================
// Whitemane Pet
// ==========================================================================
struct whitemane_pet_t final : public horseman_pet_t
{
  struct death_coil_whitemane_t final : public horseman_spell_t
  {
    death_coil_whitemane_t( util::string_view name, horseman_pet_t* p, util::string_view options_str )
      : horseman_spell_t( p, name, p->dk()->pet_spell.whitemane_death_coil )
    {
      parse_options( options_str );
    }
  };

  struct undeath_whitemane_t final : public horseman_spell_t
  {
    undeath_whitemane_t( util::string_view name, horseman_pet_t* p, util::string_view options_str )
      : horseman_spell_t( p, name, p->dk()->pet_spell.undeath_range )
    {
      parse_options( options_str );
      impact_action = p->dk()->active_spells.undeath_dot;
      cooldown->duration = p->dk()->pet_spell.undeath_dot->duration();
    }
  };

  whitemane_pet_t( death_knight_t* owner ) : horseman_pet_t( owner, "whitemane" )
  {
    npc_id                      = owner->spell.summon_whitemane->effectN( 1 ).misc_value1();
    main_hand_weapon.swing_time = 2_s;
  }

  void init_action_list() override
  {
    horseman_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "undeath" );
    def->add_action( "death_coil" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "death_coil" )
      return new death_coil_whitemane_t( "death_coil", this, options_str );
    if ( name == "undeath" )
      return new undeath_whitemane_t( "undeath", this, options_str );

    return horseman_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Trollbane Pet
// ==========================================================================
struct trollbane_pet_t final : public horseman_pet_t
{
  struct trollbane_chains_of_ice_t final : public horseman_spell_t
  {
    trollbane_chains_of_ice_t( util::string_view n, horseman_pet_t* p, util::string_view options_str )
      : horseman_spell_t( p, n, p->dk()->pet_spell.trollbanes_chains_of_ice_ability )
    {
      parse_options( options_str );
      trigger_gcd        = 1_s;
      cooldown->duration = 4_s;
      cooldown->hasted   = true;
    }

    void impact( action_state_t* a ) override
    {
      horseman_spell_t::impact( a );
      auto dk_td = dk()->get_target_data( a->target );
      dk_td->debuff.chains_of_ice_trollbane_damage->trigger();
      dk_td->debuff.chains_of_ice_trollbane_slow->trigger();
    }
  };

  struct obliterate_trollbane_t final : public horseman_melee_t
  {
    obliterate_trollbane_t( util::string_view name, horseman_pet_t* p, util::string_view options_str )
      : horseman_melee_t( p, name, p->dk()->pet_spell.trollbane_obliterate )
    {
      parse_options( options_str );
    }
  };

  trollbane_pet_t( death_knight_t* owner ) : horseman_pet_t( owner, "trollbane" )
  {
    npc_id                      = owner->spell.summon_trollbane->effectN( 1 ).misc_value1();
    main_hand_weapon.swing_time = 2_s;
  }

  void init_action_list() override
  {
    horseman_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "chains_of_ice" );
    def->add_action( "obliterate" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "obliterate" )
      return new obliterate_trollbane_t( "obliterate", this, options_str );
    if ( name == "chains_of_ice" )
      return new trollbane_chains_of_ice_t( name, this, options_str );

    return horseman_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Nazgrim Pet
// ==========================================================================
struct nazgrim_pet_t final : public horseman_pet_t
{
  nazgrim_pet_t( death_knight_t* owner ) : horseman_pet_t( owner, "nazgrim" )
  {
    npc_id                      = owner->spell.summon_nazgrim->effectN( 1 ).misc_value1();
    main_hand_weapon.swing_time = 2_s;
  }

  struct scourge_strike_shadow_nazgrim_t final : public horseman_melee_t
  {
    scourge_strike_shadow_nazgrim_t( util::string_view name, horseman_pet_t* p )
      : horseman_melee_t( p, name, p->dk()->pet_spell.nazgrim_scourge_strike_shadow )
    {
      background = dual = true;
    }
  };

  struct scourge_strike_nazgrim_t final : public horseman_melee_t
  {
    scourge_strike_nazgrim_t( util::string_view name, horseman_pet_t* p, util::string_view options_str )
      : horseman_melee_t( p, name, p->dk()->pet_spell.nazgrim_scourge_strike_phys ),
        scourge_strike_shadow( get_action<scourge_strike_shadow_nazgrim_t>( "scourge_strike_shadow", p ) )
    {
      parse_options( options_str );
      impact_action = scourge_strike_shadow;
    }

  private:
    propagate_const<action_t*> scourge_strike_shadow;
  };

  void arise() override
  {
    horseman_pet_t::arise();
    dk()->buffs.apocalyptic_conquest->trigger();
  }

  void demise() override
  {
    horseman_pet_t::demise();
    dk()->buffs.apocalyptic_conquest->expire();
  }

  void init_action_list() override
  {
    horseman_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "scourge_strike" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "scourge_strike" )
      return new scourge_strike_nazgrim_t( "scourge_strike", this, options_str );

    return horseman_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Abomination Pet
// ==========================================================================
struct abomination_pet_t : public death_knight_pet_t
{
  struct disease_cloud_t : public pet_spell_t<abomination_pet_t>
  {
    disease_cloud_t( util::string_view name, abomination_pet_t* p )
      : pet_spell_t( p, name, p->dk()->pet_spell.abomination_disease_cloud )
    {
      background    = true;
      aoe           = 0;
      impact_action = dk()->active_spells.virulent_plague;
    }
  };

  struct abomination_melee_t : public auto_attack_melee_t<abomination_pet_t>
  {
    abomination_melee_t( abomination_pet_t* p, util::string_view name ) : auto_attack_melee_t( p, name )
    {
    }

    void impact( action_state_t* state ) override
    {
      pet_melee_attack_t::impact( state );
      if ( state->result_amount > 0 )
      {
        pet()->dk()->trigger_festering_wound( state, 1, pet()->dk()->procs.fw_abomination );
      }
    }
  };

  abomination_pet_t( death_knight_t* owner ) : death_knight_pet_t( owner, "abomination" )
  {
    npc_id                            = owner->talent.unholy.raise_abomination->effectN( 2 ).misc_value1();
    main_hand_weapon.type             = WEAPON_BEAST;
    main_hand_weapon.swing_time       = 3.6_s;
    affected_by_commander_of_the_dead = true;
    decomposition_can_extend          = true;
    tww1_4pc_proc                     = true;
    owner_coeff.ap_from_ap            = 2.4;
    resource_regeneration             = regen_type::DISABLED;
  }

  void arise() override
  {
    death_knight_pet_t::arise();
    for ( auto& t : sim->target_non_sleeping_list )
    {
      disease_cloud->execute_on_target( t );
    }
  }

  void create_actions() override
  {
    death_knight_pet_t::create_actions();
    disease_cloud = get_action<disease_cloud_t>( "disease_cloud", this );

    sim->target_non_sleeping_list.register_callback( [ & ]( player_t* t ) {
      if ( !t->is_enemy() || dk()->pets.abomination.active_pet() == nullptr )
        return;

      disease_cloud->execute_on_target( t );
    } );
  }

  attack_t* create_main_hand_auto_attack() override
  {
    return new abomination_melee_t( this, "main_hand" );
  }

private:
  action_t* disease_cloud;
};

}  // namespace pets

namespace
{  // UNNAMED NAMESPACE

// ==========================================================================
// Death Knight Custom Buff Structs
// ==========================================================================
//
template <typename Base = buff_t, typename = std::enable_if_t<std::is_base_of_v<buff_t, Base>>>
struct death_knight_buff_base_t : public Base
{
protected:
  using base_t = death_knight_buff_base_t<Base>;

public:
  death_knight_buff_base_t( death_knight_t* p, std::string_view n, const spell_data_t* s = spell_data_t::nil(),
                            const item_t* item = nullptr )
    : Base( p, n, s, item )
  {
  }

  death_knight_buff_base_t( death_knight_td_t& td, std::string_view n, const spell_data_t* s = spell_data_t::nil(),
                            const item_t* item = nullptr )
    : Base( td, n, s, item )
  {
  }

  death_knight_t* p() const
  {
    return debug_cast<death_knight_t*>( Base::source );
  }
};

// ==========================================================================
// Death Knight Actions
// ==========================================================================

// Template for common death knight action code. See priest_action_t.
template <class Base>
struct death_knight_action_t : public parse_action_effects_t<Base>
{
  using action_base_t = parse_action_effects_t<Base>;
  using base_t        = death_knight_action_t<Base>;

  gain_t* gain;
  bool hasted_gcd;
  double rp_per_tick;

  death_knight_action_t( util::string_view n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() )
    : action_base_t( n, p, s ), gain( nullptr ), hasted_gcd( false ), rp_per_tick( 0 )
  {
    this->may_glance = false;
    if ( this->cooldown->duration > 0_s )
    {
      this->track_cd_waste = true;
    }

    if ( !this->data().flags( spell_attribute::SX_CANNOT_CRIT ) )
    {
      this->may_crit = true;
    }

    if ( this->data().flags( spell_attribute::SX_TICK_MAY_CRIT ) )
    {
      this->tick_may_crit = true;
    }

    // Death Knights have unique snowflake mechanism for RP energize. Base actions indicate the
    // amount as a negative value resource cost in spell data, so abuse that.
    if ( this->base_costs[ RESOURCE_RUNIC_POWER ] < 0 )
    {
      double rp_gain = std::fabs( this->base_costs[ RESOURCE_RUNIC_POWER ] );

      // action_t parse_effect_data() runs before this, and if the spell has a energize on tick amount it will set it to
      // that type This results in the RP gain on cast being set to the incorrect value, so we need to account for it
      // here.
      if ( this->energize_type == action_energize::PER_TICK )
      {
        this->rp_per_tick     = this->energize_amount;
        this->energize_amount = rp_gain;
      }
      else
      {
        this->energize_amount += rp_gain;
      }

      this->energize_type                      = action_energize::ON_CAST;
      this->energize_resource                  = RESOURCE_RUNIC_POWER;
      this->base_costs[ RESOURCE_RUNIC_POWER ] = 0;
    }
    // Always rely on custom handling via replenish_rune() for Rune energize
    if ( this->energize_resource == RESOURCE_RUNE )
    {
      this->energize_type     = action_energize::NONE;
      this->energize_resource = RESOURCE_NONE;
    }

    if ( this->data().ok() )
    {
      apply_action_effects();

      if ( this->type == action_e::ACTION_SPELL || this->type == action_e::ACTION_ATTACK )
      {
        apply_target_effects();
      }

      if ( this->data().flags( spell_attribute::SX_ABILITY ) || this->trigger_gcd > 0_ms )
      {
        this->not_a_proc = true;
      }
    }
  }

  std::string full_name() const
  {
    std::string n = action_base_t::data().name_cstr();
    return n.empty() ? action_base_t::name_str : n;
  }

  death_knight_t* p() const
  {
    return debug_cast<death_knight_t*>( this->player );
  }

  death_knight_td_t* get_td( player_t* t ) const
  {
    return p()->get_target_data( t );
  }

  virtual double runic_power_generation_multiplier( const action_state_t* /* s */ ) const
  {
    return 1.0;
  }

  void apply_action_effects();
  void apply_target_effects();

  template <typename... Ts>
  void parse_effects( Ts&&... args )
  {
    action_base_t::parse_effects( std::forward<Ts>( args )... );
  }
  template <typename... Ts>
  void parse_target_effects( Ts&&... args )
  {
    action_base_t::parse_target_effects( std::forward<Ts>( args )... );
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double amount = action_base_t::composite_energize_amount( s );

    if ( this->energize_resource_() == RESOURCE_RUNIC_POWER )
    {
      amount *= this->runic_power_generation_multiplier( s );
    }

    return amount;
  }

  void init() override
  {
    action_base_t::init();
  }

  void init_finished() override
  {
    action_base_t::init_finished();

    if ( this->base_costs[ RESOURCE_RUNE ] || this->base_costs[ RESOURCE_RUNIC_POWER ] )
    {
      gain = this->player->get_gain( util::inverse_tokenize( this->name_str ) );
    }
  }

  timespan_t gcd() const override
  {
    timespan_t base_gcd = action_base_t::gcd();
    if ( base_gcd == 0_ms )
    {
      return 0_ms;
    }

    if ( hasted_gcd )
    {
      base_gcd *= this->composite_haste();
    }

    if ( base_gcd < this->min_gcd )
    {
      base_gcd = this->min_gcd;
    }

    return base_gcd;
  }

  void tick( dot_t* d ) override
  {
    action_base_t::tick( d );

    if ( this->rp_per_tick > 0 )
    {
      p()->resource_gain( RESOURCE_RUNIC_POWER, this->rp_per_tick, this->gain, this );
    }
  }

  void execute() override
  {
    action_base_t::execute();

    // For non tank DK's, we proc the ability on CD, attached to thier own executes, to simulate it
    if ( p()->talent.blood_draw.ok() && p()->specialization() != DEATH_KNIGHT_BLOOD &&
         p()->active_spells.blood_draw->ready() && p()->in_combat )
    {
      p()->active_spells.blood_draw->execute();
    }
  }

  void impact( action_state_t* s ) override
  {
    action_base_t::impact( s );
    if ( p()->talent.sanlayn.pact_of_the_sanlayn.ok() && p()->pets.blood_beast.active_pet() != nullptr &&
         dbc::is_school( this->get_school(), SCHOOL_SHADOW ) )
    {
      p()->pets.blood_beast.active_pet()->accumulator +=
          s->result_amount * p()->talent.sanlayn.pact_of_the_sanlayn->effectN( 1 ).percent();
    }

    if ( p()->talent.deathbringer.reapers_mark.ok() && this->data().id() != p()->spell.reapers_mark_explosion->id() &&
         this->data().id() != 66198 )  // TODO-TWW verify if offhand obliterate bug is fixed
    {
      death_knight_td_t* td = get_td( s->target );
      if ( td->debuff.reapers_mark->check() )
      {
        if ( this->get_school() == SCHOOL_SHADOW || this->get_school() == SCHOOL_FROST )
        {
          td->debuff.reapers_mark->increment( 1 );
        }
        else if ( this->get_school() == SCHOOL_SHADOWFROST || ( p()->buffs.dark_talons_shadowfrost->check() &&
                                                                this->data().id() == p()->talent.death_strike->id() ) )
        {
          if ( p()->talent.deathbringer.bind_in_darkness->ok() )
          {
            td->debuff.reapers_mark->increment( s->result == RESULT_CRIT ? 4 : 2 );
          }
          else
          {
            td->debuff.reapers_mark->increment( 1 );
          }
        }
      }
    }
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = action_base_t::composite_da_multiplier( state );
    if ( p()->talent.deathbringer.wave_of_souls->ok() && this->data().id() != p()->spell.wave_of_souls_damage->id() )
    {
      death_knight_td_t* td = get_td( state->target );
      if ( dbc::is_school( this->get_school(), SCHOOL_SHADOWFROST ) && td->debuff.wave_of_souls->check() )
      {
        m *= 1 + td->debuff.wave_of_souls->check_stack_value();
      }
    }
    return m;
  }

  void update_ready( timespan_t cd ) override
  {
    action_base_t::update_ready( cd );
  }
};

// Delayed Execute Event ====================================================
struct delayed_execute_event_t : public event_t
{
  action_t* action;
  player_t* target;

  delayed_execute_event_t( death_knight_t* p, action_t* a, player_t* t, timespan_t delay )
    : event_t( *p->sim, delay ), action( a ), target( t )
  {
    assert( action->background );
  }

  const char* name() const override
  {
    return action->name();
  }

  void execute() override
  {
    if ( !target->is_sleeping() )
    {
      action->set_target( target );
      action->execute();
    }
  }
};

// Runic Power Decay Event ==================================================
struct runic_power_decay_event_t : public event_t
{
  runic_power_decay_event_t( death_knight_t* p ) : event_t( *p, 1000_ms ), player( p )
  {
  }

  const char* name() const override
  {
    return "runic_power_decay";
  }

  void execute() override
  {
    if ( !player->in_combat )
    {
      auto cur = player->resources.current[ RESOURCE_RUNIC_POWER ];
      if ( cur > RUNIC_POWER_DECAY_RATE )
      {
        player->resource_loss( RESOURCE_RUNIC_POWER, RUNIC_POWER_DECAY_RATE, nullptr, nullptr );
        make_event<runic_power_decay_event_t>( sim(), player );
      }
      else
      {
        player->resource_loss( RESOURCE_RUNIC_POWER, cur, nullptr, nullptr );
      }
    }
  }

private:
  death_knight_t* player;
};

// ==========================================================================
// Death Knight Attack
// ==========================================================================

struct death_knight_melee_attack_t : public death_knight_action_t<melee_attack_t>
{
  death_knight_melee_attack_t( util::string_view n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() )
    : death_knight_action_t( n, p, s )
  {
    special    = true;
    may_glance = false;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_action_t::impact( state );

    if ( state->result_amount > 0 && callbacks && weapon &&
         ( ( p()->runeforge.rune_of_razorice_mh && weapon->slot == SLOT_MAIN_HAND ) ||
           ( p()->runeforge.rune_of_razorice_oh && weapon->slot == SLOT_OFF_HAND ) ) )
    {
      // Razorice is executed after the attack that triggers it
      p()->active_spells.runeforge_razorice->set_target( state->target );
      p()->active_spells.runeforge_razorice->schedule_execute();
    }
  }
};

// ==========================================================================
// Death Knight Spell
// ==========================================================================

struct death_knight_spell_t : public death_knight_action_t<spell_t>
{
  death_knight_spell_t( util::string_view n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() )
    : death_knight_action_t( n, p, s )
  {
  }
};

struct blood_shield_buff_t final : public absorb_buff_t
{
  blood_shield_buff_t( death_knight_t* p ) : absorb_buff_t( p, "blood_shield", p->spell.blood_shield )
  {
    set_absorb_school( SCHOOL_PHYSICAL );
    set_absorb_source( p->get_stats( "blood_shield" ) );
    modify_duration( p->talent.blood.iron_heart->effectN( 1 ).time_value() );
  }

  void absorb_used( double absorbed, player_t* source ) override
  {
    absorb_buff_t::absorb_used( absorbed, source );

    death_knight_t* dk = debug_cast<death_knight_t*>( blood_shield_buff_t::source );

    if ( dk->rppm.carnage->trigger() )
    {
      dk->procs.carnage->occur();
      if ( dk->talent.blood.blooddrinker.ok() )
        dk->cooldown.blooddrinker->reset( true );
      if ( dk->talent.blood.consumption.ok() )
        dk->cooldown.consumption->reset( true );
    }
  }
};

struct blood_shield_t final : public absorb_t
{
  blood_shield_t( util::string_view name, death_knight_t* p ) : absorb_t( name, p, p->spell.blood_shield )
  {
    may_miss = may_crit = callbacks = false;
    background = proc = true;
  }

  // Self only so we can do this in a simple way
  absorb_buff_t* create_buff( const action_state_t* ) override
  {
    return debug_cast<death_knight_t*>( player )->buffs.blood_shield;
  }

  void init() override
  {
    absorb_t::init();

    snapshot_flags = update_flags = 0;
  }
};

struct death_knight_heal_t : public death_knight_action_t<heal_t>
{
  death_knight_heal_t( util::string_view n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() )
    : death_knight_action_t( n, p, s ),
      blood_shield( p->specialization() == DEATH_KNIGHT_BLOOD ? get_action<blood_shield_t>( "blood_shield", p )
                                                              : nullptr )
  {
  }

  void trigger_blood_shield( action_state_t* state )
  {
    if ( p()->specialization() != DEATH_KNIGHT_BLOOD )
      return;

    double current_value                = 0;
    blood_shield_t* blood_shield_lookup = debug_cast<blood_shield_t*>( blood_shield );

    if ( blood_shield_lookup->target_specific[ state->target ] )
      current_value = blood_shield_lookup->target_specific[ state->target ]->current_value;

    double amount = state->result_raw;

    if ( p()->mastery.blood_shield->ok() )
      amount *= p()->cache.mastery_value();

    // https://www.wowhead.com/news/upcoming-tank-tuning-in-the-war-within-nerfs-to-self-sustain-and-survivability-345239
    // Per above wowhead post, vamp blood no longer affects blood shield
    // if ( p()->buffs.vampiric_blood->up() )
    //   amount *= 1.0 + p()->talent.blood.vampiric_blood->effectN( 3 ).percent();

    amount *= 1.0 + p()->talent.blood.iron_heart->effectN( 2 ).percent();

    amount *= 1.0 + p()->talent.gloom_ward->effectN( 1 ).percent();

    auto final_amount = amount + current_value;

    // Blood Shield caps at 50% max health
    if ( final_amount > ( player->resources.max[ RESOURCE_HEALTH ] / 2 ) )
      final_amount = player->resources.max[ RESOURCE_HEALTH ] / 2;

    sim->print_debug( "{} Blood Shield buff trigger, old_value={} added_value={} new_value={} from action={} (id={})",
                      player->name(), current_value, amount, final_amount, name(), this->data().id() );

    blood_shield->base_dd_min = blood_shield->base_dd_max = final_amount;
    blood_shield->execute();
  }

private:
  action_t* blood_shield;
};

struct death_knight_leech_damage_heal_t : public death_knight_heal_t
// Leech tick damage and leech direct damage such as blood plague, consumption, and blood drinker
// do not include a bunch of the damage modifiers, and need to be based off the actual damage of the ability
// so we strip out the mods here.
{
  death_knight_leech_damage_heal_t( util::string_view n, death_knight_t* p,
                                    const spell_data_t* s = spell_data_t::nil() )
    : death_knight_heal_t( n, p, s )
  {
    da_value_mod = 1.0;
    ta_value_mod = 1.0;

    for ( auto& effect : this->data().effects() )
    {
      if ( effect.type() == E_HEALTH_LEECH )
      {
        da_value_mod = effect.m_value();
      }
      if ( effect.type() == E_APPLY_AURA && effect.subtype() == A_PERIODIC_LEECH )
      {
        ta_value_mod = effect.m_value();
      }
    }
  }

  double composite_versatility( const action_state_t* /* s */ ) const override
  {
    return 1.0;
  }

  // This ensures that bloodshot does not affect the results, as consumption is a physical attack
  double composite_player_multiplier( const action_state_t* ) const override
  {
    return 1.0;
  }

  double composite_da_multiplier( const action_state_t* /* s */ ) const override
  {
    return 1.0 * da_value_mod;
  }

  double composite_ta_multiplier( const action_state_t* /* s */ ) const override
  {
    return 1.0 * ta_value_mod;
  }

private:
  double da_value_mod;
  double ta_value_mod;
};

// ==========================================================================
// Death Knight Secondary Abilities
// ==========================================================================

// Razorice Attack ==========================================================
struct razorice_attack_t final : public death_knight_melee_attack_t
{
  razorice_attack_t( util::string_view name, death_knight_t* player )
    : death_knight_melee_attack_t( name, player, player->spell.razorice_damage )
  {
    may_miss = callbacks = false;
    background = proc = true;

    // Note, razorice always attacks with the main hand weapon, regardless of which hand triggers it
    weapon = &( player->main_hand_weapon );
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );
    get_td( s->target )->debuff.razorice->trigger();
    p()->procs.razorice_from_runeforge->occur();
  }
};

// Inexorable Assault =======================================================
struct inexorable_assault_damage_t final : public death_knight_spell_t
{
  inexorable_assault_damage_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.inexorable_assault_damage )
  {
    background = true;
  }
};

// Icy Death Torrent ========================================================
struct icy_death_torrent_t final : public death_knight_spell_t
{
  icy_death_torrent_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.icy_death_torrent_damage )
  {
    background = true;
    aoe        = -1;
  }
};

// Cryogennic Chamber =======================================================
struct cryogenic_chamber_t final : public death_knight_spell_t
{
  cryogenic_chamber_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.cryogenic_chamber_damage )
  {
    background = true;
    aoe        = -1;
  }
};

struct hyperpyrexia_damage_t final : public residual_action::residual_periodic_action_t<death_knight_spell_t>
{
  hyperpyrexia_damage_t( util::string_view name, death_knight_t* p )
    : residual_action::residual_periodic_action_t<death_knight_spell_t>( name, p, p->spell.hyperpyrexia_damage )
  {
    background = true;
    may_miss = hasted_ticks = false;
  }
};

// Unholy Pact ==============================================================
struct unholy_pact_damage_t final : public death_knight_spell_t
{
  unholy_pact_damage_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.unholy_pact_damage )
  {
    background = true;
    aoe        = -1;
  }
};

// Decomposition ==============================================================
struct decomposition_damage_t final : public death_knight_spell_t
{
  decomposition_damage_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.decomposition_damage )
  {
    background = true;
  }
};

// Shattering Bone ==========================================================

struct shattering_bone_t final : public death_knight_spell_t
{
  shattering_bone_t( util::string_view n, death_knight_t* p )
    : death_knight_spell_t( n, p, p->spell.shattering_bone_damage )
  {
    background = true;
    aoe        = -1;
    base_multiplier *= 1.0 + p->talent.blood.shattering_bone->effectN( 2 ).percent();
    boneshield_charges_consumed = 1.0;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( state );

    if ( p()->in_death_and_decay() )
    {
      m *= p()->talent.blood.shattering_bone->effectN( 1 ).base_value();
    }

    m *= boneshield_charges_consumed;

    return m;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    // Reset charges consumed to default
    boneshield_charges_consumed = 1.0;
  }

public:
  double boneshield_charges_consumed;
};

// ==========================================================================
// Death Knight Buffs
// ==========================================================================
namespace buffs
{
using death_knight_buff_t = death_knight_buff_base_t<>;

// Base Death Knight Absorb Buff struct
struct death_knight_absorb_buff_t : public death_knight_buff_base_t<absorb_buff_t>
{
protected:
  using base_t = death_knight_absorb_buff_t;

public:
  death_knight_absorb_buff_t( death_knight_t* p, util::string_view name, const spell_data_t* spell )
    : death_knight_buff_base_t( p, name, spell )
  {
    set_absorb_gain( p->get_gain( absorb_name() ) );
    set_absorb_source( p->get_stats( absorb_name() ) );
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    auto ret = death_knight_buff_base_t::trigger( s, v, c, d );
    if ( ret && !quiet )
      absorb_source->add_execute( 0_ms, player );

    return ret;
  }

  std::string absorb_name() const
  {
    return util::inverse_tokenize( name_str ) + " (absorb)";
  }
};

// Breath of Sindragosa =====================================================
struct breath_of_sindragosa_buff_t : public death_knight_buff_t
{
  breath_of_sindragosa_buff_t( death_knight_t* p, util::string_view name, const spell_data_t* s )
    : death_knight_buff_t( p, name, s ),
      bos_damage( p->active_spells.breath_of_sindragosa_tick ),
      ticking_cost( 0.0 ),
      tick_period( p->talent.frost.breath_of_sindragosa->effectN( 1 ).period() ),
      rune_gen( as<int>( p->spell.breath_of_sindragosa_rune_gen->effectN( 1 ).base_value() ) )
  {
    tick_zero          = true;
    cooldown->duration = 0_ms;  // Handled by the action

    // Extract the cost per tick from spelldata
    for ( size_t idx = 1; idx <= data().power_count(); idx++ )
    {
      const spellpower_data_t& power = data().powerN( idx );
      if ( power.aura_id() == 0 || player->dbc->spec_by_spell( power.aura_id() ) == player->specialization() )
      {
        ticking_cost = power.cost_per_tick();
      }
    }

    set_tick_callback( [ &, p ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ ) {
      // TODO: Target the last enemy targeted by the player's foreground abilities
      // Currently use the player's target which is the first non invulnerable, active enemy found.
      player_t* bos_target = p->target;

      // On cast execute damage for no cost, and generate runes
      if ( current_tick == 0 )
      {
        bos_damage->execute_on_target( bos_target );
        p->replenish_rune( rune_gen, p->gains.breath_of_sindragosa );
        return;
      }

      // If the player doesn't have enough RP to fuel this tick, BoS is cancelled and no RP is consumed
      // This can happen if the player uses another RP spender between two ticks and is left with < 15 RP
      if ( !p->resource_available( RESOURCE_RUNIC_POWER, ticking_cost ) )
      {
        sim->print_log(
            "Player {} doesn't have the {} Runic Power required for current tick. Breath of Sindragosa was cancelled.",
            p->name_str, ticking_cost );

        // Separate the expiration event to happen immediately after tick processing
        make_event( *sim, 0_ms, [ & ]() { expire(); } );
        return;
      }

      // Else, consume the resource and update the damage tick's resource stats
      p->resource_loss( RESOURCE_RUNIC_POWER, ticking_cost, nullptr, bos_damage );
      bos_damage->stats->consume_resource( RESOURCE_RUNIC_POWER, ticking_cost );

      // If the player doesn't have enough RP to fuel the next tick, BoS is cancelled
      // after the RP consumption and before the damage event
      // This is the normal BoS expiration scenario
      if ( !p->resource_available( RESOURCE_RUNIC_POWER, ticking_cost ) )
      {
        sim->print_log(
            "Player {} doesn't have the {} Runic Power required for next tick. Breath of Sindragosa was cancelled.",
            p->name_str, ticking_cost );

        // Separate the expiration event to happen immediately after tick processing
        make_event( *sim, 0_ms, [ & ]() { expire(); } );
        return;
      }

      // If there's enough resources for another tick, deal damage
      bos_damage->execute_on_target( bos_target );
    } );
  }

  // Breath of Sindragosa always ticks every one second, not affected by haste
  timespan_t tick_time() const override
  {
    return tick_period;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    death_knight_buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( !p()->sim->event_mgr.canceled )
    {
      // BoS generates 2 runes when it expires
      p()->replenish_rune( rune_gen, p()->gains.breath_of_sindragosa );
    }
  }

private:
  action_t*& bos_damage;
  double ticking_cost;
  const timespan_t tick_period;
  int rune_gen;
};

// Pillar of Frost Buff =====================================================
struct pillar_of_frost_buff_t final : public death_knight_buff_t
{
  pillar_of_frost_buff_t( death_knight_t* p, util::string_view n, const spell_data_t* s )
    : death_knight_buff_t( p, n, s ), pillar_extension( 0 )
  {
    cooldown->duration = 0_ms;  // Controlled by the action
    set_default_value( p->talent.frost.pillar_of_frost->effectN( 1 ).percent() );
    set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    death_knight_buff_t::start( stacks, value, duration );
    pillar_extension = 0;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    death_knight_buff_t::expire_override( expiration_stacks, remaining_duration );
    pillar_extension = 0;
    if ( p()->talent.frost.enduring_strength.ok() )
    {
      trigger_enduring_strength();
    }
  }

  void refresh( int stacks, double value, timespan_t duration ) override
  {
    death_knight_buff_t::refresh( stacks, value, duration );
    pillar_extension = 0;
    if ( p()->talent.frost.enduring_strength.ok() )
    {
      trigger_enduring_strength();
    }
  }

  void extend_pillar()
  {
    if ( pillar_extension >= as<unsigned>( p()->talent.frost.the_long_winter->effectN( 2 ).base_value() ) )
    {
      return;
    }

    int added_duration = as<unsigned>( p()->talent.frost.the_long_winter->effectN( 1 ).base_value() );
    pillar_extension += added_duration;
    extend_duration( p(), timespan_t::from_seconds( added_duration ) );
  }

  void trigger_enduring_strength()
  {
    p()->buffs.enduring_strength->trigger();
    p()->buffs.enduring_strength->extend_duration( p(), p()->talent.frost.enduring_strength->effectN( 2 ).time_value() *
                                                            p()->buffs.enduring_strength_builder->stack() );
    p()->buffs.enduring_strength_builder->expire();
  }

private:
  unsigned pillar_extension;
};

// Cryogennic Chamber =======================================================
struct cryogenic_chamber_buff_t final : public death_knight_buff_t
{
  cryogenic_chamber_buff_t( death_knight_t* p, util::string_view n, const spell_data_t* s )
    : death_knight_buff_t( p, n, s ),
      damage( 0 ),
      cryogenic_chamber_damage( get_action<cryogenic_chamber_t>( "cryogenic_chamber", p ) )
  {
    if ( p->talent.frost.cryogenic_chamber.ok() )
    {
      set_max_stack( as<int>( p->talent.frost.cryogenic_chamber->effectN( 2 ).base_value() ) );
    }
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    death_knight_buff_t::start( stacks, value, duration );
    damage = 0;
  }

  void expire_override( int stacks, timespan_t remains ) override
  {
    death_knight_buff_t::expire_override( stacks, remains );
    cryogenic_chamber_damage->base_dd_min = cryogenic_chamber_damage->base_dd_max = damage;
    cryogenic_chamber_damage->execute();
    damage = 0;
  }

public:
  double damage;

private:
  action_t* cryogenic_chamber_damage;
};

// Dark Transformation ======================================================
// Even though the buff is tied to the pet ingame, it's simpler to add it to the player
struct dark_transformation_buff_t final : public death_knight_buff_t
{
  dark_transformation_buff_t( death_knight_t* p, util::string_view n, const spell_data_t* s )
    : death_knight_buff_t( p, n, s )
  {
    set_default_value_from_effect( 1 );
    cooldown->duration = 0_ms;  // Handled by the player ability
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    death_knight_buff_t::start( stacks, value, duration );
    if ( p()->talent.unholy.ghoulish_frenzy.ok() )
    {
      p()->buffs.ghoulish_frenzy->trigger();
      p()->pets.ghoul_pet.active_pet()->ghoulish_frenzy->trigger();
    }
    if ( p()->talent.sanlayn.gift_of_the_sanlayn.ok() )
    {
      p()->buffs.gift_of_the_sanlayn->trigger();
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    death_knight_buff_t::expire_override( expiration_stacks, remaining_duration );
    if ( p()->talent.unholy.ghoulish_frenzy.ok() )
    {
      p()->buffs.ghoulish_frenzy->expire();
      if ( p()->pets.ghoul_pet.active_pet() != nullptr )
      {
        p()->pets.ghoul_pet.active_pet()->ghoulish_frenzy->expire();
      }
    }
    if ( p()->talent.sanlayn.gift_of_the_sanlayn.ok() )
    {
      p()->buffs.gift_of_the_sanlayn->expire();
    }
  }
};

// Runic Corruption =========================================================
struct runic_corruption_buff_t final : public death_knight_buff_t
{
  runic_corruption_buff_t( death_knight_t* p, util::string_view name, const spell_data_t* spell )
    : death_knight_buff_t( p, name, spell )
  {
    // Runic Corruption refreshes to remaining time + buff duration
    refresh_behavior = buff_refresh_behavior::EXTEND;
    set_affects_regen( true );
    set_stack_change_callback( [ p ]( buff_t*, int, int ) { p->_runes.update_coefficient(); } );
  }

  // Runic Corruption duration is reduced by haste so it always regenerates
  // the equivalent of 0.9 of a rune ( 3 / 10 seconds on 3 regenerating runes )
  timespan_t buff_duration() const override
  {
    timespan_t initial_duration = death_knight_buff_t::buff_duration();

    return initial_duration * p()->cache.attack_haste();
  }
};

// Apocalyptic Conquest =======================================================
struct apocalyptic_conquest_buff_t final : public death_knight_buff_t
{
  apocalyptic_conquest_buff_t( death_knight_t* p, util::string_view name, const spell_data_t* spell )
    : death_knight_buff_t( p, name, spell ), nazgrims_conquest( 0 )
  {
    set_default_value( p->pet_spell.apocalyptic_conquest->effectN( 1 ).percent() );
    add_invalidate( CACHE_STRENGTH );
    // set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );  TODO: bugged should be A_MOD_TOTAL_STAT_PERCENTAGE (137)
  }

  // Override the value of the buff to properly capture Apocalyptic Conquest's strength buff behavior
  double value() override
  {
    return p()->pet_spell.apocalyptic_conquest->effectN( 1 ).percent() +
           ( nazgrims_conquest * p()->talent.rider.nazgrims_conquest->effectN( 2 ).percent() );
  }

  double check_value() const override
  {
    return p()->pet_spell.apocalyptic_conquest->effectN( 1 ).percent() +
           ( nazgrims_conquest * p()->talent.rider.nazgrims_conquest->effectN( 2 ).percent() );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    death_knight_buff_t::start( stacks, value, duration );
    nazgrims_conquest = 0;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    death_knight_buff_t::expire_override( expiration_stacks, remaining_duration );
    nazgrims_conquest = 0;
  }

  void refresh( int stacks, double value, timespan_t duration ) override
  {
    death_knight_buff_t::refresh( stacks, value, duration );
    nazgrims_conquest = 0;
  }

public:
  int nazgrims_conquest;
};

// Essence of the Blood Queen ================================================
struct essence_of_the_blood_queen_haste_buff_t final : public death_knight_buff_t
{
  essence_of_the_blood_queen_haste_buff_t( death_knight_t* p, util::string_view name, const spell_data_t* spell )
    : death_knight_buff_t( p, name, spell )
  {
    set_default_value( p->spell.essence_of_the_blood_queen_buff->effectN( 1 ).percent() / 10 );
    set_stack_change_callback( [ p ]( buff_t*, int old_, int new_ ) {
      if ( new_ > old_ )
      {
        p->buffs.essence_of_the_blood_queen_damage->increment();
      }
    } );
    set_expire_callback(
        [ p ]( buff_t*, double, timespan_t ) { p->buffs.essence_of_the_blood_queen_damage->expire(); } );
    set_pct_buff_type( STAT_PCT_BUFF_HASTE );
  }

  // Override the value of the buff to properly capture Essence of the Blood Queens's buff behavior
  double value() override
  {
    return ( p()->spell.essence_of_the_blood_queen_buff->effectN( 1 ).percent() / 10 ) *
           ( 1.0 + p()->buffs.gift_of_the_sanlayn->check_value() );
  }

  double check_value() const override
  {
    return ( p()->spell.essence_of_the_blood_queen_buff->effectN( 1 ).percent() / 10 ) *
           ( 1.0 + p()->buffs.gift_of_the_sanlayn->check_value() );
  }
};

struct essence_of_the_blood_queen_damage_buff_t final : public death_knight_buff_t
{
  modified_spell_data_t* m_data;
  essence_of_the_blood_queen_damage_buff_t( death_knight_t* p, util::string_view name, const spell_data_t* spell )
    : death_knight_buff_t( p, name, spell ), m_data( p->get_modified_spell( &data() ) )
  {
    m_data->parse_effects( p->talent.sanlayn.frenzied_bloodthirst );
    set_default_value( m_data->effectN( 2 ).percent() );
    set_duration( 0_ms );  // Handled by the haste buff
  }

  // Override the value of the buff to properly capture Essence of the Blood Queens's buff behavior
  double value() override
  {
    return ( m_data->effectN( 2 ).percent() ) * ( 1.0 + p()->buffs.gift_of_the_sanlayn->check_value() );
  }

  double check_value() const override
  {
    return ( m_data->effectN( 2 ).percent() ) * ( 1.0 + p()->buffs.gift_of_the_sanlayn->check_value() );
  }
};

struct gift_of_the_sanlayn_buff_t final : public death_knight_buff_t
{
  gift_of_the_sanlayn_buff_t( death_knight_t* p, util::string_view name, const spell_data_t* spell )
    : death_knight_buff_t( p, name, spell ), idx( p->specialization() == DEATH_KNIGHT_BLOOD ? 4 : 1 )
  {
    set_default_value_from_effect( idx );
    set_duration( 0_ms );  // Handled by DT and VB
    add_invalidate( CACHE_HASTE );
    set_expire_callback( [ p ]( buff_t*, int, timespan_t ) {
      p->buffs.vampiric_strike->expire();
      if ( p->talent.sanlayn.infliction_of_sorrow.ok() )
      {
        p->buffs.infliction_of_sorrow->trigger();
      }
    } );
    set_stack_change_callback( [ p ]( buff_t*, int, int new_ ) {
      if ( new_ )
      {
        if ( p->buffs.vampiric_strike->check() )
        {
          p->procs.vampiric_strike_waste->occur();
        }
        p->buffs.vampiric_strike->predict();
        p->buffs.vampiric_strike->trigger();
      }
    } );
  }

private:
  unsigned idx;
};

// Death and Decay ==========================================================
struct death_and_decay_buff_t : public death_knight_buff_t
{
  death_and_decay_buff_t( death_knight_t* p, util::string_view name, const spell_data_t* spell )
    : death_knight_buff_t( p, name, spell ), leway_buff( false )
  {
    set_duration( 0_ms );  // Handled by things that trigger this buff.
    // Specifically use a stack change callback here due to when its called in buff_t::expire
    set_stack_change_callback( [ &, p ]( buff_t*, int, int new_ ) {
      if ( new_ == 0 && p->in_death_and_decay() )
      {
        trigger();
      }
      if ( new_ == 0 && !leway_buff && !p->in_death_and_decay() )
      {
        trigger( 4_s );
      }
    } );
  }

  void trigger_buffs()
  {
    if ( !p()->talent.unholy_ground.ok() && !p()->talent.blood.sanguine_ground.ok() )
      return;

    if ( p()->talent.unholy_ground.ok() && !p()->buffs.unholy_ground->check() )
      p()->buffs.unholy_ground->trigger();

    if ( p()->talent.blood.sanguine_ground.ok() && !p()->buffs.sanguine_ground->check() )
      p()->buffs.sanguine_ground->trigger();

    if ( p()->talent.sanlayn.bloodsoaked_ground.ok() && !p()->buffs.bloodsoaked_ground->check() )
      p()->buffs.bloodsoaked_ground->trigger();
  }

  void expire_buffs()
  {
    if ( !p()->talent.unholy_ground.ok() && !p()->talent.blood.sanguine_ground.ok() )
      return;

    if ( p()->talent.unholy_ground.ok() && p()->buffs.unholy_ground->check() )
      p()->buffs.unholy_ground->expire();

    if ( p()->talent.blood.sanguine_ground.ok() && p()->buffs.sanguine_ground->check() )
      p()->buffs.sanguine_ground->expire();

    if ( p()->talent.sanlayn.bloodsoaked_ground.ok() && p()->buffs.bloodsoaked_ground->check() )
      p()->buffs.bloodsoaked_ground->expire();
  }

  void start( int s, double v, timespan_t d ) override
  {
    death_knight_buff_t::start( s, v, d );
    trigger_buffs();

    if ( d == 4_s )
      leway_buff = true;
    else
      leway_buff = false;
  }

  void expire_override( int s, timespan_t d ) override
  {
    death_knight_buff_t::expire_override( s, d );
    expire_buffs();
  }

private:
  bool leway_buff;
};

// Anti-magic Shell =========================================================
struct ams_parent_buff_t : public death_knight_absorb_buff_t
{
  ams_parent_buff_t( death_knight_t* p, util::string_view name, const spell_data_t* spell, bool horsemen )
    : death_knight_absorb_buff_t( p, name, spell ),
      damage( 0 ),
      horsemen( horsemen ),
      option( horsemen ? p->options.horsemen_ams_absorb_percent : p->options.ams_absorb_percent )
  {
    cooldown->duration = 0_ms;
    set_absorb_school( SCHOOL_MAGIC );
    if ( option > 0 )
    {
      set_period( 1_s );
      set_tick_time_behavior( buff_tick_time_behavior::HASTED );
      set_tick_callback( [ &, p ]( buff_t*, int, timespan_t ) {
        if ( p->specialization() == DEATH_KNIGHT_UNHOLY || p->specialization() == DEATH_KNIGHT_FROST )
        {
          consume( damage );
        }
      } );
    }
  }

  void start( int stacks, double, timespan_t duration ) override
  {
    death_knight_absorb_buff_t::start( stacks, calc_absorb(), duration );

    if ( option > 0 )
    {
      double ticks = buff_duration() / tick_time();
      double pct   = option / ticks;
      damage       = calc_absorb() * pct;
    }
  };

  void absorb_used( double absorbed, player_t* source ) override
  {
    death_knight_absorb_buff_t::absorb_used( absorbed, source );
    // AMS generates 0.833~ runic power per percentage max health absorbed.
    double rp_generated = absorbed / p()->resources.max[ RESOURCE_HEALTH ] * 83.333;

    player->resource_gain( RESOURCE_RUNIC_POWER, util::round( rp_generated ),
                           horsemen ? p()->gains.antimagic_shell_horsemen : p()->gains.antimagic_shell );
  }

  double calc_absorb()
  {
    double max_absorb = p()->resources.max[ RESOURCE_HEALTH ] * p()->spec.antimagic_shell->effectN( 2 ).percent();

    max_absorb *= 1.0 + p()->talent.antimagic_barrier->effectN( 2 ).percent();

    max_absorb *= 1.0 + p()->cache.heal_versatility();

    max_absorb *= 1.0 + p()->talent.gloom_ward->effectN( 1 ).percent();

    if ( horsemen )
      max_absorb *= 0.8;

    return max_absorb;
  }

private:
  double damage;
  bool horsemen;

public:
  double option;
};

struct antimagic_shell_buff_t : public ams_parent_buff_t
{
  antimagic_shell_buff_t( death_knight_t* p, util::string_view name, const spell_data_t* spell )
    : ams_parent_buff_t( p, name, spell, false )
  {
    set_absorb_source( p->get_stats( "antimagic_shell" ) );
  }
};

struct antimagic_shell_buff_horseman_t : public ams_parent_buff_t
{
  antimagic_shell_buff_horseman_t( death_knight_t* p, util::string_view name, const spell_data_t* spell )
    : ams_parent_buff_t( p, name, spell, true )
  {
    set_absorb_source( p->get_stats( "antimagic_shell_horseman" ) );
  }
};

// Anti-magic Zone =========================================================
struct antimagic_zone_buff_t final : public death_knight_absorb_buff_t
{
  antimagic_zone_buff_t( death_knight_t* p, util::string_view name, const spell_data_t* spell )
    : death_knight_absorb_buff_t( p, name, spell )
  {
    set_absorb_school( SCHOOL_MAGIC );
    set_absorb_source( p->get_stats( "antimagic_zone" ) );
  }

  double consume( double consumed, action_state_t* s ) override
  {
    double actual_consumed = consumed;
    actual_consumed *= 0.2;  // AMZ only absorbs 20% of incoming damage
    s->result_absorbed *= 0.2;
    s->self_absorb_amount -= consumed - actual_consumed;
    s->result_amount += consumed - actual_consumed;
    return death_knight_absorb_buff_t::consume( actual_consumed, s );
  }

  void start( int stacks, double, timespan_t duration ) override
  {
    death_knight_absorb_buff_t::start( stacks, calc_absorb(), duration );
  };

  double calc_absorb()
  {
    // HP Value doesnt appear in spell data, instead stored in a variable in spell ID 51052
    double max_absorb = p()->resources.max[ RESOURCE_HEALTH ] * 1.5;

    max_absorb *= 1.0 + p()->talent.assimilation->effectN( 1 ).percent();

    max_absorb *= 1.0 + p()->cache.heal_versatility();

    max_absorb *= 1.0 + p()->talent.gloom_ward->effectN( 1 ).percent();

    return max_absorb;
  }
};
}  // namespace buffs

namespace debuffs
{
using death_knight_debuff_t = death_knight_buff_base_t<buff_t>;

// Decomposition =========================================================
struct decomposition_debuff_t final : public death_knight_debuff_t
{
  decomposition_debuff_t( death_knight_td_t& td, util::string_view n, const spell_data_t* s )
    : death_knight_debuff_t( td, n, s ),
      stored_damage( 0 ),
      last_period( 0 ),
      damage( nullptr ),
      tar( td.target ),
      decomposition_extend_duration( 0_s )
  {
    damage = p()->active_spells.decomposition_damage;
    decomposition_extend_duration =
        timespan_t::from_millis( p()->talent.unholy.decomposition->effectN( 2 ).base_value() );
    set_tick_callback( [ & ]( buff_t*, int, timespan_t ) {
      last_period   = stored_damage;
      stored_damage = 0;
      if ( rng().roll( 0.25 ) )
      {
        p()->procs.decomposition->occur();
        execute_damage();
        // Cant use the main `active_pets` vector here, breaks non dk pets.
        for ( auto& pet : p()->dk_active_pets )
        {
          extend_pet( pet );
        }
      }
    } );
  }

  void extend_pet( pets::death_knight_pet_t* pet )
  {
    if ( pet->decomposition_can_extend && pet->decomposition_extend_limit > pet->decomposition_extended )
    {
      pet->adjust_duration( decomposition_extend_duration );
      pet->decomposition_extended += decomposition_extend_duration;
    }
  }

  void execute_damage()
  {
    damage->base_dd_min = damage->base_dd_max = last_period * p()->talent.unholy.decomposition->effectN( 1 ).percent();
    damage->execute_on_target( tar );
  }

  void reset() override
  {
    death_knight_debuff_t::reset();
    stored_damage = 0;
    last_period   = 0;
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    death_knight_debuff_t::start( stacks, value, duration );
    stored_damage = 0;
    last_period   = 0;
  };

  void expire_override( int stacks, timespan_t duration ) override
  {
    death_knight_debuff_t::expire_override( stacks, duration );
    stored_damage = 0;
    last_period   = 0;
  }

public:
  double stored_damage;

private:
  double last_period;
  action_t* damage;
  player_t* tar;
  timespan_t decomposition_extend_duration;
};
}  // namespace debuffs

// ==========================================================================
// Death Knight Attacks
// ==========================================================================

// Melee Attack =============================================================

struct melee_t : public death_knight_melee_attack_t
{
  int sync_weapons;
  bool first;
  int autos_since_last_proc;
  double sd_chance;

  melee_t( const char* name, death_knight_t* p, int sw )
    : death_knight_melee_attack_t( name, p ),
      sync_weapons( sw ),
      first( true ),
      autos_since_last_proc( 0 ),
      sd_chance( 0 )
  {
    school                    = SCHOOL_PHYSICAL;
    may_crit                  = true;
    may_glance                = true;
    background                = true;
    allow_class_ability_procs = true;
    not_a_proc                = true;
    repeating                 = true;
    trigger_gcd               = 0_ms;
    special                   = false;
    weapon_multiplier         = 1.0;
    if ( p->talent.unholy.sudden_doom.ok() )
    {
      sd_chance = pseudo_random_c_from_p( p->talent.unholy.sudden_doom->effectN( 2 ).percent() *
                                          ( 1 + p->talent.unholy.harbinger_of_doom->effectN( 2 ).percent() ) );
    }

    // Dual wielders have a -19% chance to hit on melee attacks
    if ( p->dual_wield() )
      base_hit -= 0.19;
  }

  void reset() override
  {
    death_knight_melee_attack_t::reset();

    first = true;
    autos_since_last_proc = 0;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = death_knight_melee_attack_t::execute_time();

    if ( first )
      return ( weapon->slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, 0_ms ) : t / 2 ) : 0_ms;
    else
      return t;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = death_knight_melee_attack_t::composite_da_multiplier( s );

    if ( p()->talent.frost.smothering_offense.ok() )
    {
      m *= 1.0 + ( p()->buffs.icy_talons->check() * p()->talent.frost.smothering_offense->effectN( 2 ).percent() );
    }

    return m;
  }

  void execute() override
  {
    if ( first )
      first = false;

    death_knight_melee_attack_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    if ( p()->talent.runic_attenuation.ok() )
    {
      trigger_runic_attenuation( s );
    }

    if ( result_is_hit( s->result ) )
    {
      if ( p()->specialization() == DEATH_KNIGHT_UNHOLY && p()->talent.unholy.sudden_doom.ok() )
      {
        if ( rng().roll( sd_chance * ++autos_since_last_proc ) )
        {
          p()->buffs.sudden_doom->trigger();
          autos_since_last_proc = 0;
        }
      }

      if ( p()->talent.permafrost.ok() )
      {
        trigger_permafrost( s );
      }

      if ( s->result == RESULT_CRIT )
      {
        if ( p()->talent.frost.killing_machine.ok() )
        {
          p()->trigger_killing_machine( 0, p()->procs.km_from_crit_aa, p()->procs.km_from_crit_aa_wasted );
        }

        // TODO: verify proc rate close to launch, as of build 55288 it is 100% for 2h and 50% for dw
        if ( p()->talent.frost.icy_death_torrent.ok() )
        {
          double chance_mult = p()->main_hand_weapon.group() == WEAPON_2H
                                   ? 1
                                   : 1 / ( p()->talent.frost.icy_death_torrent->effectN( 1 ).base_value() / 10 );

          if ( rng().roll( p()->talent.frost.icy_death_torrent->proc_chance() * chance_mult ) )
          {
            p()->active_spells.icy_death_torrent_damage->execute();
          }
        }

        if ( p()->talent.frost.the_long_winter.ok() && p()->buffs.pillar_of_frost->check() )
        {
          debug_cast<buffs::pillar_of_frost_buff_t*>( p()->buffs.pillar_of_frost )->extend_pillar();
        }
      }

      // Crimson scourge doesn't proc if death and decay is ticking
      if ( get_td( s->target )->dot.blood_plague->is_ticking() && !p()->active_dnd )
      {
        if ( p()->specialization() == DEATH_KNIGHT_BLOOD && p()->buffs.crimson_scourge->trigger() )
        {
          p()->cooldown.death_and_decay_dynamic->reset( true );
        }
      }

      if ( p()->talent.blood.bloodworms.ok() )
      {
        trigger_bloodworm();
      }
    }
  }

  double psuedo_random_p_from_c( double c )
  {
    double p_proc_on_n       = 0;
    double p_proc_by_n       = 0;
    double sum_n_p_proc_on_n = 0;

    int max_fails = as<int>( std::ceil( 1 / c ) );
    for ( int n = 1; n <= max_fails; ++n )
    {
      p_proc_on_n = std::min( 1.0, n * c ) * ( 1 - p_proc_by_n );
      p_proc_by_n += p_proc_on_n;
      sum_n_p_proc_on_n += n * p_proc_on_n;
    }

    return ( 1 / sum_n_p_proc_on_n );
  }

  double pseudo_random_c_from_p( double p )
  {
    double c_upper = p;
    double c_lower = 0;
    double c_mid;
    double p1;
    double p2 = 1;
    while ( true )
    {
      c_mid = ( c_upper + c_lower ) / 2;
      p1    = psuedo_random_p_from_c( c_mid );
      if ( std::abs( p1 - p2 ) <= 0 )
        break;

      if ( p1 > p )
      {
        c_upper = c_mid;
      }
      else
      {
        c_lower = c_mid;
      }

      p2 = p1;
    }

    return c_mid;
  }

  void trigger_runic_attenuation( action_state_t* s )
  {
    if ( !p()->rppm.runic_attenuation->trigger() )
    {
      return;
    }

    p()->resource_gain(
        RESOURCE_RUNIC_POWER,
        p()->talent.runic_attenuation->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
        p()->gains.runic_attenuation, s->action );
  }

  void trigger_bloodworm()
  {
    if ( !p()->rppm.bloodworms->trigger() )
    {
      return;
    }
    p()->procs.bloodworms->occur();

    // TODO: check whether spelldata is wrong or tooltip is using the wrong spelldata
    // Spelldata used in the tooltip: 15s
    p()->pets.bloodworms.spawn();
    // Pet spawn spelldata: 16s
    // p() -> pets.bloodworms.spawn( p() -> talent.bloodworms -> effectN( 1 ).trigger() -> duration(), 1 );
  }

  void trigger_permafrost( action_state_t* state )
  {
    if ( !p()->talent.permafrost.ok() || p()->buffs.frost_shield->check() )
      return;

    double amount = state->result_amount * p()->talent.permafrost->effectN( 1 ).percent();

    amount *= 1.0 + p()->talent.gloom_ward->effectN( 1 ).percent();

    p()->buffs.frost_shield->trigger( 1, amount, -1.0 );
  }
};

// Auto Attack ==============================================================

struct auto_attack_t final : public death_knight_melee_attack_t
{
  int sync_weapons;

  auto_attack_t( death_knight_t* p, util::string_view options_str )
    : death_knight_melee_attack_t( "auto_attack", p ), sync_weapons( 1 )
  {
    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    parse_options( options_str );
    ignore_false_positive = true;

    assert( p->main_hand_weapon.type != WEAPON_NONE );

    p->main_hand_attack                    = new melee_t( "auto_attack_mh", p, sync_weapons );
    p->main_hand_attack->weapon            = &( p->main_hand_weapon );
    p->main_hand_attack->base_execute_time = p->main_hand_weapon.swing_time;

    if ( p->off_hand_weapon.type != WEAPON_NONE )
    {
      p->off_hand_attack                    = new melee_t( "auto_attack_oh", p, sync_weapons );
      p->off_hand_attack->weapon            = &( p->off_hand_weapon );
      p->off_hand_attack->base_execute_time = p->off_hand_weapon.swing_time;
      p->off_hand_attack->id                = 1;
    }

    trigger_gcd = 0_ms;
  }

  void execute() override
  {
    player->main_hand_attack->schedule_execute();
    if ( player->off_hand_attack )
    {
      player->off_hand_attack->schedule_execute();
    }
  }

  bool ready() override
  {
    if ( player->is_moving() )
      return false;

    return ( player->main_hand_attack->execute_event == nullptr );  // not swinging
  }
};

// ==========================================================================
// Death Knight Diseases
// ==========================================================================
// Deathbringer
struct blood_fever_t final : public death_knight_spell_t
{
  blood_fever_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.blood_fever_damage )
  {
    background         = true;
    cooldown->duration = 0_ms;
  }
};
// Common diseases code

struct death_knight_disease_t : public death_knight_spell_t
{
  death_knight_disease_t( util::string_view n, death_knight_t* p, const spell_data_t* s )
    : death_knight_spell_t( n, p, s )
  {
    background = true;
    may_miss = hasted_ticks = false;
  }

  void tick( dot_t* d ) override
  {
    death_knight_spell_t::tick( d );
    auto td = get_td( d->target );
    if ( p()->talent.brittle.ok() && rng().roll( p()->talent.brittle->proc_chance() ) )
    {
      td->debuff.brittle->trigger();
    }
  }
};

// Blood Plague ============================================
struct blood_plague_heal_t final : public death_knight_leech_damage_heal_t
{
  blood_plague_heal_t( util::string_view name, death_knight_t* p )
    : death_knight_leech_damage_heal_t( name, p, p->spell.blood_plague )
  {
    callbacks  = false;
    background = true;
    target     = p;
    // Tick time, duration and healing amount handled by the damage
    attack_power_mod.direct = attack_power_mod.tick = 0;
    dot_duration = base_tick_time = 0_ms;
  }
};

struct blood_plague_t final : public death_knight_disease_t
{
  blood_plague_t( util::string_view name, death_knight_t* p, bool superstrain = false )
    : death_knight_disease_t( name, p, p->spell.blood_plague )
  {
    heal = get_action<blood_plague_heal_t>( "blood_plague_heal", p );

    if ( superstrain )
    {
      // It looks like the legendary modifier from superstrain is still being applied to blood plague, but not frost
      // fever.
      if ( p->bugs )
        base_multiplier *= 0.75;
    }
    if ( p->talent.deathbringer.blood_fever->ok() )
    {
      blood_fever = get_action<blood_fever_t>( "blood_fever", p );
      add_child( blood_fever );
    }
  }

  void tick( dot_t* d ) override
  {
    death_knight_disease_t::tick( d );

    if ( d->state->result_amount > 0 )
    {
      // Healing is based off damage done, increased by Rapid Decomposition if talented
      heal->base_dd_min = heal->base_dd_max =
          d->state->result_amount * ( 1.0 + p()->talent.blood.rapid_decomposition->effectN( 3 ).percent() );
      heal->execute();
    }

    if ( blood_fever && p()->rng().roll( p()->talent.deathbringer.blood_fever->proc_chance() ) )
    {
      blood_fever->execute_on_target(
          d->target, d->state->result_amount * p()->talent.deathbringer.blood_fever->effectN( 1 ).percent() );
    }
  }

private:
  propagate_const<action_t*> heal;
  propagate_const<action_t*> blood_fever;
};

// Frost Fever =======================================================
struct frost_fever_t final : public death_knight_disease_t
{
  frost_fever_t( util::string_view name, death_knight_t* p )
    : death_knight_disease_t( name, p, p->spell.frost_fever ),
      rp_generation(
          as<int>( p->spec.frost_fever->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) +
                   ( p->talent.unholy.superstrain->effectN( 3 ).base_value() / 10 ) ) )
  {
    ap_type = attack_power_type::WEAPON_BOTH;

    if ( p->main_hand_weapon.group() == WEAPON_2H )
    {
      ap_type = attack_power_type::WEAPON_MAINHAND;
      // There's a 0.98 modifier hardcoded in the tooltip if a 2H weapon is equipped, probably server side magic
      base_multiplier *= 0.98;
    }

    if ( p->talent.deathbringer.blood_fever.ok() )
    {
      blood_fever = get_action<blood_fever_t>( "blood_fever", p );
      add_child( blood_fever );
    }
  }

  void tick( dot_t* d ) override
  {
    death_knight_disease_t::tick( d );

    // TODO: Melekus, 2019-05-15: Frost fever proc chance and ICD have been removed from spelldata on PTR
    // Figure out what is up with the "30% proc chance, diminishing beyond the first target" from blue post.
    // https://us.forums.blizzard.com/en/wow/t/upcoming-ptr-class-changes-4-23/158332

    // 2020-05-05: It would seem that the proc chance is 0.30 * sqrt(FeverCount) / FeverCount
    unsigned ff_count = p()->get_active_dots( d );
    double chance     = 0.30;

    chance *= std::sqrt( ff_count ) / ff_count;

    if ( rng().roll( chance ) )
    {
      p()->resource_gain( RESOURCE_RUNIC_POWER, rp_generation, p()->gains.frost_fever, this );
    }

    if ( blood_fever && p()->rng().roll( p()->talent.deathbringer.blood_fever->proc_chance() ) )
    {
      blood_fever->execute_on_target(
          d->target,
          d->state->result_amount * ( p()->talent.deathbringer.blood_fever->effectN( 1 ).default_value() / 100 ) );
    }

    if ( p()->talent.frost.cryogenic_chamber.ok() && !p()->buffs.cryogenic_chamber->at_max_stacks() )
    {
      debug_cast<buffs::cryogenic_chamber_buff_t*>( p()->buffs.cryogenic_chamber )->damage +=
          d->state->result_amount * p()->talent.frost.cryogenic_chamber->effectN( 1 ).percent();
      p()->buffs.cryogenic_chamber->trigger();
    }
  }

private:
  int rp_generation;
  propagate_const<action_t*> blood_fever;
};

// Virulent Plague ====================================================
struct virulent_eruption_t final : public death_knight_disease_t
{
  virulent_eruption_t( util::string_view n, death_knight_t* p )
    : death_knight_disease_t( n, p, p->spell.virulent_erruption )
  {
    split_aoe_damage = true;
    aoe              = -1;
  }
};

struct virulent_plague_t final : public death_knight_disease_t
{
  virulent_plague_t( util::string_view name, death_knight_t* p )
    : death_knight_disease_t( name, p, p->spell.virulent_plague )
  {
  }

  void tick( dot_t* d ) override
  {
    death_knight_disease_t::tick( d );
    if ( p()->talent.unholy.decomposition.ok() )
    {
      auto td = get_td( d->target );
      debug_cast<debuffs::decomposition_debuff_t*>( td->debuff.decomposition )->stored_damage +=
          d->state->result_amount;
    }
  }

  void impact( action_state_t* s ) override
  {
    death_knight_disease_t::impact( s );

    if ( p()->talent.unholy.decomposition.ok() )
    {
      auto td = get_td( s->target );
      td->debuff.decomposition->trigger( td->dot.virulent_plague->duration() );
    }
  }
};

// Unholy Blight DoT ====================================================
struct unholy_blight_dot_t final : public death_knight_disease_t
{
  unholy_blight_dot_t( util::string_view name, death_knight_t* p )
    : death_knight_disease_t( name, p, p->spell.unholy_blight_dot )
  {
    tick_on_application = true;
  }
};

struct unholy_blight_t final : public death_knight_spell_t
{
  unholy_blight_t( util::string_view n, death_knight_t* p )
    : death_knight_spell_t( n, p, p->spell.unholy_blight ),
      dot( get_action<unholy_blight_dot_t>( "unholy_blight_dot", p ) ),
      vp( get_action<virulent_plague_t>( "virulent_plague", p ) ),
      ff( get_action<frost_fever_t>( "frost_fever", p ) ),
      bp( get_action<blood_plague_t>( "blood_plague", p, true ) ),
      diseases()
  {
    may_dodge = may_parry = harmful = false;
    tick_zero = background = true;
    target                 = p;
    radius                 = p->spell.unholy_blight_dot->effectN( 1 ).radius_max();
    aoe                    = -1;
    if ( p->talent.unholy.superstrain.ok() )
    {
      diseases.push_back( vp );
      diseases.push_back( ff );
      diseases.push_back( bp );
    }
  }

  void tick( dot_t* d ) override
  {
    death_knight_spell_t::tick( d );
    dot->execute_on_target( d->state->target );

    if ( p()->talent.unholy.superstrain.ok() )
    {
      // Randomize the order the diseases are applied to emulate the random tick order in game.
      // Only really matters for RPPM effects.
      rng().shuffle( diseases.begin(), diseases.end() );
      for ( auto& dis : diseases )
      {
        dis->execute_on_target( d->state->target );
      }
    }
    else
    {
      vp->execute_on_target( d->state->target );
    }
  }

private:
  propagate_const<action_t*> dot;
  propagate_const<action_t*> vp;
  propagate_const<action_t*> ff;
  propagate_const<action_t*> bp;
  std::vector<action_t*> diseases;
};

// ==========================================================================
// Rider of the Apocalypse Abilities
// ==========================================================================
struct summon_rider_t : public death_knight_spell_t
{
  summon_rider_t( util::string_view name, death_knight_t* p, const spell_data_t* s, timespan_t duration = 10_s,
                  bool random = true )
    : death_knight_spell_t( name, p, s ), duration( duration ), random( random )
  {
    background = true;
  }

public:
  timespan_t duration;
  bool random;
};

struct undeath_dot_t final : public death_knight_spell_t
{
  undeath_dot_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->pet_spell.undeath_dot ),
      sqrt_targets( p->pet_spell.undeath_dot->effectN( 2 ).base_value() )
  {
    background = true;
    may_miss = may_dodge = may_parry          = false;
    dot_behavior                              = DOT_NONE;
  }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m     = death_knight_spell_t::composite_ta_multiplier( state );
    /*
    * TWW-TODO: Believe this is what effect2's value is for, but, havent been able to confirm yet. 
    * Leaving this here for if its confirmed. 
    auto counter = p()->get_active_dots( p()->active_spells.undeath_dot->get_dot( nullptr ) );

    if ( counter > sqrt_targets )
    {
      m *= sqrt( sqrt_targets / counter );
    }
    */
    return m;
  }

  void tick( dot_t* d ) override
  {
    death_knight_spell_t::tick( d );
    auto td = p()->get_target_data( d->target );
    td->dot.undeath->increment( 1 );
  }

private:
  double sqrt_targets;
};

struct trollbanes_icy_fury_t final : public death_knight_spell_t
{
  trollbanes_icy_fury_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->pet_spell.trollbanes_icy_fury_ability )
  {
    background          = true;
    aoe                 = -1;
    reduced_aoe_targets = as<int>( p->talent.rider.trollbanes_icy_fury->effectN( 1 ).base_value() );
  }
};

struct summon_whitemane_t final : public summon_rider_t
{
  summon_whitemane_t( util::string_view name, death_knight_t* p ) : summon_rider_t( name, p, p->spell.summon_whitemane )
  {
    p->pets.whitemane.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    add_child( get_action<undeath_dot_t>( "undeath", p ) );
  }

  void execute() override
  {
    if ( p()->pets.whitemane.active_pet() == nullptr )
    {
      death_knight_spell_t::execute();
      p()->pets.whitemane.spawn( duration );
    }
    else if ( !random )
    {
      death_knight_spell_t::execute();
      p()->pets.whitemane.active_pet()->adjust_duration( duration );
      p()->pets.whitemane.active_pet()->rp_spent = 0;
      if ( !p()->bugs )
      {
        auto td = p()->get_target_data( target );
        if ( td && td->dot.undeath->is_ticking() )
        {
          td->dot.undeath->increment( 1 );
          td->dot.undeath->adjust_duration( p()->pet_spell.undeath_dot->duration() - td->dot.undeath->remains() );
        }
        else
        {
          p()->active_spells.undeath_dot->execute_on_target( target );
        }
      }
    }
  }
};

struct summon_trollbane_t final : public summon_rider_t
{
  summon_trollbane_t( util::string_view name, death_knight_t* p ) : summon_rider_t( name, p, p->spell.summon_trollbane )
  {
    p->pets.trollbane.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    add_child( get_action<trollbanes_icy_fury_t>( "trollbanes_icy_fury", p ) );
  }

  void execute() override
  {
    if ( p()->pets.trollbane.active_pet() == nullptr )
    {
      death_knight_spell_t::execute();
      p()->pets.trollbane.spawn( duration );
    }
    else if ( !random )
    {
      death_knight_spell_t::execute();
      p()->pets.trollbane.active_pet()->adjust_duration( duration );
      p()->pets.trollbane.active_pet()->rp_spent = 0;
    }
  }
};

struct summon_nazgrim_t final : public summon_rider_t
{
  summon_nazgrim_t( util::string_view name, death_knight_t* p ) : summon_rider_t( name, p, p->spell.summon_nazgrim )
  {
    p->pets.nazgrim.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
  }

  void execute() override
  {
    if ( p()->pets.nazgrim.active_pet() == nullptr )
    {
      death_knight_spell_t::execute();
      p()->pets.nazgrim.spawn( duration );
    }
    else if ( !random )
    {
      death_knight_spell_t::execute();
      p()->pets.nazgrim.active_pet()->adjust_duration( duration );
      p()->pets.nazgrim.active_pet()->rp_spent = 0;
    }
  }
};

struct summon_mograine_t final : public summon_rider_t
{
  summon_mograine_t( util::string_view name, death_knight_t* p ) : summon_rider_t( name, p, p->spell.summon_mograine )
  {
    p->pets.mograine.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
  }

  void execute() override
  {
    if ( p()->pets.mograine.active_pet() == nullptr )
    {
      death_knight_spell_t::execute();
      p()->pets.mograine.spawn( duration );
    }
    else if ( !random )
    {
      death_knight_spell_t::execute();
      pets::mograine_pet_t* mograine = p()->pets.mograine.active_pet();
      mograine->adjust_duration( duration );
      mograine->rp_spent = 0;
      if ( mograine->dnd_aura->check() )
        mograine->extended_by_apoc_now = true;
      else
        mograine->dnd_aura->trigger();
    }
  }
};

// ==========================================================================
// San'layn Abilities
// ==========================================================================

// Vampiric Strike ==========================================================
struct vampiric_strike_heal_t : public death_knight_heal_t
{
  vampiric_strike_heal_t( util::string_view name, death_knight_t* p )
    : death_knight_heal_t( name, p, p->spell.vampiric_strike_heal )
  {
    background = true;
    may_miss = may_dodge = may_parry = false;
    target                           = p;
    int idx                          = p->specialization() == DEATH_KNIGHT_BLOOD ? 2 : 3;
    base_pct_heal                    = data().effectN( idx ).percent();
  }
};

struct infliction_in_sorrow_t : public death_knight_spell_t
{
  infliction_in_sorrow_t( util::string_view n, death_knight_t* p )
    : death_knight_spell_t( n, p, p->spell.infliction_of_sorrow_damage )
  {
    background = true;
    may_miss = may_dodge = may_parry = false;
  }
};

// ==========================================================================
// Deathbringer Abilities
// ==========================================================================
struct exterminate_aoe_t final : public death_knight_spell_t
{
  exterminate_aoe_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.exterminate_aoe )
  {
    background          = true;
    cooldown->duration  = 0_ms;
    aoe                 = -1;
    reduced_aoe_targets = p->talent.deathbringer.exterminate->effectN( 5 ).base_value();
    const int effect_idx    = p->specialization() == DEATH_KNIGHT_FROST ? 2 : 1;
    attack_power_mod.direct = data().effectN( effect_idx ).ap_coeff();

    if ( p->specialization() == DEATH_KNIGHT_FROST )
    {
      impact_action = get_action<frost_fever_t>( "frost_fever", p );
    }
    if ( p->specialization() == DEATH_KNIGHT_BLOOD )
    {
      impact_action = get_action<blood_plague_t>( "blood_plague", p );
    }
  }
};

struct exterminate_t final : public death_knight_spell_t
{
  exterminate_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.exterminate_damage ),
      second_hit( get_action<exterminate_aoe_t>( name_str + "_second_hit", p ) )
  {
    background         = true;
    cooldown->duration = 0_ms;
    const int effect_idx    = p->specialization() == DEATH_KNIGHT_FROST ? 2 : 1;
    attack_power_mod.direct = data().effectN( effect_idx ).ap_coeff();

    add_child( second_hit );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    double chance = p()->talent.deathbringer.painful_death->ok()
                        ? p()->talent.deathbringer.painful_death->effectN( 2 ).percent()
                        : p()->talent.deathbringer.exterminate->effectN( 2 ).percent();

    buff_t* rm = get_td( execute_state->target )->debuff.reapers_mark;
    if ( !rm->up() && p()->rng().roll( chance ) )
    {
      rm->trigger();
      p()->procs.exterminate_reapers_mark->occur();
    }

    make_event<delayed_execute_event_t>( *sim, p(), second_hit, execute_state->target, 500_ms );
  }

private:
  action_t* second_hit;
};

struct reapers_mark_explosion_t final : public death_knight_spell_t
{
  reapers_mark_explosion_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.reapers_mark_explosion )
  {
    background              = true;
    cooldown->duration      = 0_ms;
    const int effect_idx    = p->specialization() == DEATH_KNIGHT_FROST ? 2 : 1;
    attack_power_mod.direct = data().effectN( effect_idx ).ap_coeff();
    stacks                  = 0;
    soul_rupture_effect_idx = p->specialization() == DEATH_KNIGHT_FROST ? 2 : 1;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( state );
    return m * stacks;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_spell_t::composite_target_multiplier( target );
    if ( p()->talent.deathbringer.grim_reaper->ok() )
    {
      double possible_increase = p()->talent.deathbringer.grim_reaper->effectN( 1 ).percent();
      double missing_hp        = ( 1.0 - target->health_percentage() / 100 );
      double buff_amount       = missing_hp * possible_increase;
      m *= 1.0 + buff_amount;
    }

    return m;
  }

  void execute_wrapper( player_t* target, int stacks )
  {
    this->stacks = stacks;
    execute_on_target( target );

    if ( target != nullptr )
    {
      if ( p()->talent.deathbringer.grim_reaper->ok() &&
           target->health_percentage() < p()->talent.deathbringer.grim_reaper->effectN( 2 ).base_value() )
      {
        p()->active_spells.grim_reaper_soul_reaper->execute_on_target( target );
      }

      if ( p()->talent.deathbringer.exterminate->ok() )
      {
        p()->buffs.exterminate->trigger();
      }
    }
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );
    if ( p()->talent.deathbringer.soul_rupture.ok() )
      p()->active_spells.soul_rupture->execute_on_target(
          state->target, state->result_amount * p()->talent.deathbringer.soul_rupture->effectN( soul_rupture_effect_idx ).percent() );
  }

private:
  int stacks;
  int soul_rupture_effect_idx;
};

struct wave_of_souls_t final : public death_knight_spell_t
{
  wave_of_souls_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.wave_of_souls_damage )
  {
    background         = true;
    cooldown->duration = 0_ms;
    aoe                = -1;
    const int effect_idx    = p->specialization() == DEATH_KNIGHT_FROST ? 2 : 1;
    attack_power_mod.direct = data().effectN( effect_idx ).ap_coeff();
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );
    if ( state->result == RESULT_CRIT )
    {
      get_td( state->target )->debuff.wave_of_souls->trigger();
    }
  }
};

struct soul_rupture_t final : public death_knight_spell_t
{
  soul_rupture_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.soul_rupture_damage )
  {
    background         = true;
    cooldown->duration = 0_ms;
    aoe                = -1;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    death_knight_spell_t::available_targets( tl );

    auto it = range::find( tl, target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    return tl.size();
  }
};

struct reapers_mark_t final : public death_knight_spell_t
{
  reapers_mark_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "reapers_mark", p, p->talent.deathbringer.reapers_mark )
  {
    parse_options( options_str );
    const int effect_idx    = p->specialization() == DEATH_KNIGHT_FROST ? 4 : 1;
    attack_power_mod.direct = data().effectN( effect_idx ).ap_coeff();
    
    if ( p->talent.deathbringer.reapers_mark.ok() )
    {
      add_child( p->active_spells.reapers_mark_explosion );
    }
    if ( p->talent.deathbringer.wave_of_souls.ok() )
    {
      add_child( p->active_spells.wave_of_souls );
    }
    if ( p->talent.deathbringer.soul_rupture.ok() )
    {
      add_child( p->active_spells.soul_rupture );
    }
    if ( p->talent.deathbringer.grim_reaper.ok() )
    {
      add_child( p->active_spells.grim_reaper_soul_reaper );
    }
    if ( p->talent.deathbringer.exterminate.ok() )
    {
      add_child( p->active_spells.exterminate );
    }
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );
    // TODO-TWW implement 10ms delay
    buff_t* rm = get_td( state->target )->debuff.reapers_mark;
    if ( rm->up() )
    {
      rm->expire();
    }
    rm->trigger();
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p()->talent.deathbringer.wave_of_souls->ok() )
    {
      // 5/12/24 The initial hit can have some slight delay depending on how far away the player is
      // the return hit does not have a consistent travel time, starting with roughly a second for now
      timespan_t first = timespan_t::from_millis( rng().range( 0, 100 ) );
      make_event( *sim, first, [ this ]() {
        p()->active_spells.wave_of_souls->execute();
        timespan_t second = rng().gauss<1000, 200>();
        make_repeating_event(
            *sim, second, [ this ]() { p()->active_spells.wave_of_souls->execute(); }, 1 );
      } );
    }
  }
};

// ==========================================================================
// Death Knight Abilities
// ==========================================================================

// Abomination Limb =========================================================

struct abomination_limb_damage_t final : public death_knight_spell_t
{
  abomination_limb_damage_t( util::string_view n, death_knight_t* p )
    : death_knight_spell_t( n, p, p->talent.abomination_limb->effectN( 2 ).trigger() )
  {
    background          = true;
    aoe                 = -1;
    reduced_aoe_targets = p->talent.abomination_limb->effectN( 5 ).base_value();
  }
};

struct abomination_limb_t : public death_knight_spell_t
{
  abomination_limb_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "abomination_limb", p, p->talent.abomination_limb )
  {
    may_miss = may_dodge = may_parry = false;

    parse_options( options_str );

    // Periodic behavior handled by the buff
    dot_duration = base_tick_time = 0_ms;

    if ( p->talent.abomination_limb->ok() )
    {
      add_child( p->active_spells.abomination_limb_damage );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    // Pull affect for this ability is NYI

    p()->buffs.abomination_limb->trigger();
  }
};

// Apocalypse ===============================================================

struct apocalypse_t final : public death_knight_melee_attack_t
{
  apocalypse_t( death_knight_t* p, util::string_view options_str )
    : death_knight_melee_attack_t( "apocalypse", p, p->talent.unholy.apocalypse ),
      rune_generation( as<int>( p->spell.apocalypse_rune_gen->effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );
    p->pets.apoc_ghouls.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    if ( p->talent.unholy.magus_of_the_dead.ok() )
    {
      p->pets.apoc_magus.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    }
  }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );
    const death_knight_td_t* td = get_td( state->target );
    assert( td && "apocalypse impacting without any target data" );  // td should should exist because the debuff is a
    // condition of target_ready()
    auto n_wounds = std::min( as<int>( data().effectN( 2 ).base_value() ), td->debuff.festering_wound->check() );

    p()->burst_festering_wound( state->target, n_wounds, p()->procs.fw_apocalypse );
    p()->pets.apoc_ghouls.spawn( as<int>( data().effectN( 2 ).base_value() ) );

    if ( p()->talent.unholy.magus_of_the_dead.ok() )
    {
      p()->pets.apoc_magus.spawn();
    }

    if ( p()->talent.unholy.apocalypse.ok() )
    {
      p()->replenish_rune( rune_generation, p()->gains.apocalypse );
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    const death_knight_td_t* td = get_td( candidate_target );

    // In-game limitation: you can't use Apocalypse on a target that isn't affected by Festering Wounds
    if ( !td || !td->debuff.festering_wound->check() )
      return false;

    return death_knight_melee_attack_t::target_ready( candidate_target );
  }

private:
  int rune_generation;
};

// Army of the Dead =========================================================

struct army_of_the_dead_t final : public death_knight_spell_t
{
  struct summon_army_event_t : public event_t
  {
    summon_army_event_t( death_knight_t* dk, int n, timespan_t interval, timespan_t duration )
      : event_t( *dk, interval ), n_ghoul( n ), summon_interval( interval ), summon_duration( duration ), p( dk )
    {
    }

    void execute() override
    {
      ++n_ghoul;
      if ( n_ghoul < 3 )
      {
        // First 2 ghouls seem to have an odd duration offset, +1s for the first ghoul.
        // +0.5s for the 2nd ghoul, increasing summon duration
        summon_duration += 1_s / n_ghoul;
      }
      p->pets.army_ghouls.spawn( summon_duration, 1 );
      if ( n_ghoul < 8 )
      {
        make_event<summon_army_event_t>( sim(), p, n_ghoul, summon_interval, summon_duration );
      }
    }

  private:
    int n_ghoul;
    timespan_t summon_interval;
    timespan_t summon_duration;
    death_knight_t* p;
  };

  army_of_the_dead_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "army_of_the_dead", p, p->talent.unholy.army_of_the_dead ),
      precombat_time( 2.0 ),
      summon_duration( p->talent.unholy.army_of_the_dead->effectN( 1 ).trigger()->duration() )
  {
    // disable_aotd=1 can be added to the profile to disable aotd usage, for example for specific dungeon simming

    if ( p->options.disable_aotd || p->talent.unholy.raise_abomination.ok() )
      background = true;

    // If used during precombat, army is cast around X seconds before the fight begins
    // This is done to save rune regeneration time once the fight starts
    // Default duration is 6, and can be changed by the user

    add_option( opt_float( "precombat_time", precombat_time ) );
    parse_options( options_str );

    harmful = false;
    target  = p;
    p->pets.army_ghouls.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    if ( p->talent.unholy.magus_of_the_dead.ok() && !p->talent.unholy.raise_abomination.ok() )
    {
      p->pets.army_magus.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    }
  }

  void init_finished() override
  {
    death_knight_spell_t::init_finished();

    if ( is_precombat )
    {
      double MIN_TIME = player->base_gcd.total_seconds();  // the player's base unhasted gcd: 1.5s
      double MAX_TIME =
          10;  // Using 10s as highest value because it's the time to recover the rune cost of AOTD at 0 haste

      // Ensure that we're using a positive value
      if ( precombat_time < 0 )
        precombat_time = -precombat_time;

      // Limit Army of the dead precast option between 10s before pull (because rune regeneration time is 10s at 0
      // haste) and on pull
      if ( precombat_time > MAX_TIME )
      {
        precombat_time = MAX_TIME;
        sim->error( "{} tried to cast army of the dead more than {} seconds before combat begins, setting to {}",
                    player->name(), MAX_TIME, MAX_TIME );
      }
      else if ( precombat_time < MIN_TIME )
      {
        precombat_time = MIN_TIME;
        sim->error(
            "{} tried to cast army of the dead less than {} before combat begins, setting to {} (has to be >= base "
            "gcd)",
            player->name(), MIN_TIME, MIN_TIME );
      }
    }
    else
      precombat_time = 0;
  }

  // Army of the Dead should always cost resources
  double cost() const override
  {
    return base_costs[ RESOURCE_RUNE ];
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    int n_ghoul = 0;

    // There's a 0.5s interval between each ghoul's spawn
    timespan_t const summon_interval = p()->talent.unholy.army_of_the_dead->effectN( 1 ).period();

    if ( !p()->in_combat && precombat_time > 0 )
    {
      // The first pet spawns after the interval timer
      timespan_t duration_penalty = timespan_t::from_seconds( precombat_time ) - summon_interval;
      while ( duration_penalty >= 0_s && n_ghoul < 8 )
      {
        n_ghoul++;
        // Spawn with a duration penalty, and adjust the spawn/travel delay by the penalty
        auto pet =
            p()->pets.army_ghouls.spawn( summon_duration + ( n_ghoul < 3 ? 1_s / n_ghoul : 0_s ) - duration_penalty, 1 )
                .front();
        pet->precombat_spawn_adjust = duration_penalty;
        pet->precombat_spawn        = true;
        // For each pet, reduce the duration penalty by the 0.5s interval
        duration_penalty -= summon_interval;
      }

      // Adjust the cooldown based on the precombat time
      p()->cooldown.army_of_the_dead->adjust( timespan_t::from_seconds( -precombat_time ), false );

      // Simulate RP decay and rune regeneration
      p()->resource_loss( RESOURCE_RUNIC_POWER, RUNIC_POWER_DECAY_RATE * precombat_time, nullptr, nullptr );
      p()->_runes.regenerate_immediate( timespan_t::from_seconds( precombat_time ) );

      sim->print_debug(
          "{} used Army of the Dead in precombat with precombat time = {}, adjusting pets' duration and remaining "
          "cooldown.",
          player->name(), precombat_time );
    }

    // If precombat didn't summon every ghoul (due to interval between each spawn)
    // Or if the cast isn't during precombat
    // Summon the rest
    if ( n_ghoul < 8 )
      make_event<summon_army_event_t>( *sim, p(), n_ghoul, summon_interval, summon_duration );

    if ( p()->talent.unholy.magus_of_the_dead.ok() )
    {
      // Magus of the Dead seems to have a 31s duration, rather than the 30 listed in spell data
      p()->pets.army_magus.spawn( ( summon_duration + 1_s ) - timespan_t::from_seconds( precombat_time ), 1 );
    }

    if ( p()->talent.rider.apocalypse_now.ok() )
    {
      p()->summon_rider( p()->spell.apocalypse_now_data->duration() - timespan_t::from_seconds( precombat_time ),
                         false );
    }
  }

private:
  double precombat_time;
  timespan_t summon_duration;
};

// Raise Abomination =========================================================
struct raise_abomination_t final : public death_knight_spell_t
{
  raise_abomination_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "raise_abomination", p, p->talent.unholy.raise_abomination ), precombat_time( 0_ms )
  {
    harmful = false;
    target  = p;
    add_option( opt_timespan( "precombat_time", precombat_time, trigger_gcd, 10_s ) );
    parse_options( options_str );
    p->pets.abomination.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    if ( p->talent.unholy.magus_of_the_dead.ok() && p->talent.unholy.raise_abomination.ok() )
    {
      p->pets.army_magus.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    }
  }

  void precombat_check()
  {
    duration       = data().duration();
    rider_duration = p()->spell.apocalypse_now_data->duration();

    if ( precombat_time > 0_ms && !p()->in_combat )
    {
      duration -= precombat_time;
      rider_duration -= precombat_time;
      cooldown->adjust( -precombat_time, false );
      p()->resource_loss( RESOURCE_RUNIC_POWER, RUNIC_POWER_DECAY_RATE * timespan_t::to_native( precombat_time ),
                          nullptr, nullptr );
      p()->_runes.regenerate_immediate( precombat_time );
    }
  }

  void execute() override
  {
    precombat_check();
    death_knight_spell_t::execute();
    p()->pets.abomination.spawn();
    if ( p()->talent.unholy.magus_of_the_dead.ok() )
    {
      p()->pets.army_magus.spawn();
    }
    if ( p()->talent.rider.apocalypse_now.ok() )
    {
      p()->summon_rider( rider_duration, false );
    }
  }

private:
  timespan_t duration;
  timespan_t rider_duration;
  timespan_t precombat_time;
};

// Blood Boil ===============================================================

struct blood_boil_t final : public death_knight_spell_t
{
  blood_boil_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "blood_boil", p, p->talent.blood.blood_boil )
  {
    parse_options( options_str );

    aoe              = -1;
    cooldown->hasted = true;
    impact_action    = get_action<blood_plague_t>( "blood_plague", p );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p()->pets.dancing_rune_weapon_pet.active_pet() != nullptr )
    {
      p()->pets.dancing_rune_weapon_pet.active_pet()->ability.blood_boil->execute_on_target( target );
    }

    if ( p()->talent.blood.everlasting_bond.ok() )
    {
      if ( p()->pets.everlasting_bond_pet.active_pet() != nullptr )
      {
        p()->pets.everlasting_bond_pet.active_pet()->ability.blood_boil->execute_on_target( target );
      }
    }
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    p()->buffs.hemostasis->trigger();
  }
};

// Blood Tap ================================================================

struct blood_tap_t final : public death_knight_spell_t
{
  blood_tap_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "blood_tap", p, p->talent.blood.blood_tap )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->replenish_rune( as<int>( p()->talent.blood.blood_tap->effectN( 1 ).resource( RESOURCE_RUNE ) ),
                         p()->gains.blood_tap );
  }
};

// Blood Draw =========================================================

struct blood_draw_t final : public death_knight_spell_t
{
  blood_draw_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.blood_draw_damage )
  {
    aoe                = -1;
    background         = true;
    cooldown->duration = p->spell.blood_draw_cooldown->duration();
    // Force set threshold to 100% health, to make the spell proc on cooldown
    health_threshold = 100;
    // TODO make health_threshold configurable
    // If role is set to tank, we can use the proper health % value
    if ( p->primary_role() == ROLE_TANK )
      health_threshold = p->talent.blood_draw->effectN( 1 ).base_value();
  }

  bool ready() override
  {
    if ( p()->health_percentage() > health_threshold )
      return false;

    return death_knight_spell_t::ready();
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->buffs.blood_draw->trigger();
  }

private:
  double health_threshold;
};

// Blooddrinker =============================================================

struct blooddrinker_heal_t final : public death_knight_leech_damage_heal_t
{
  blooddrinker_heal_t( util::string_view name, death_knight_t* p )
    : death_knight_leech_damage_heal_t( name, p, p->talent.blood.blooddrinker )
  {
    background = true;
    callbacks = may_crit = may_miss = false;
    base_costs[ RESOURCE_RUNE ]     = 0;
    energize_type                   = action_energize::NONE;
    cooldown->duration              = 0_ms;
    target                          = p;
    attack_power_mod.direct = attack_power_mod.tick = 0;
    dot_duration = base_tick_time = 0_ms;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_leech_damage_heal_t::impact( state );

    if ( p()->talent.blood.carnage.ok() )
      trigger_blood_shield( state );
  }
};

// TODO: Implement the defensive stuff somehow
struct blooddrinker_t final : public death_knight_spell_t
{
  blooddrinker_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "blooddrinker", p, p->talent.blood.blooddrinker ),
      heal( get_action<blooddrinker_heal_t>( "blooddrinker_heal", p ) )
  {
    parse_options( options_str );
    channeled = hasted_ticks = tick_zero = true;
    base_tick_time                       = 1.0_s;
  }

  void tick( dot_t* d ) override
  {
    death_knight_spell_t::tick( d );

    if ( d->state->result_amount > 0 )
    {
      heal->base_dd_min = heal->base_dd_max = d->state->result_amount;
      heal->execute();
    }
  }

private:
  propagate_const<action_t*> heal;
};

// The Blood is Life ========================================================

struct the_blood_is_life_t : public death_knight_spell_t
{
  the_blood_is_life_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->pet_spell.blood_eruption )
  {
    background = true;
    aoe        = -1;
    may_crit   = false;
    reduced_aoe_targets = 8;
  }
};

// Bonestorm ================================================================

struct bonestorm_heal_t : public death_knight_heal_t
{
  bonestorm_heal_t( util::string_view name, death_knight_t* p )
    : death_knight_heal_t( name, p, p->spell.bonestorm_heal )
  {
    background = true;
    target     = p;
  }
};

struct bonestorm_damage_t final : public death_knight_spell_t
{
  bonestorm_damage_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->talent.blood.bonestorm->effectN( 3 ).trigger() ),
      heal( get_action<bonestorm_heal_t>( "bonestorm_heal", p ) ),
      heal_count( 0 )
  {
    background          = true;
    aoe                 = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
  }

  void execute() override
  {
    heal_count = 0;
    death_knight_spell_t::execute();
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( result_is_hit( state->result ) )
    {
      // Healing is limited at 5 occurnces per tick, regardless of enemies hit
      if ( heal_count < p()->talent.blood.bonestorm->effectN( 4 ).base_value() )
      {
        heal->execute();
        heal_count++;
      }
    }
  }

private:
  propagate_const<action_t*> heal;
  int heal_count;
};

struct bonestorm_t final : public death_knight_spell_t
{
  bonestorm_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "bonestorm", p, p->talent.blood.bonestorm )
  {
    parse_options( options_str );
    hasted_ticks = false;
    tick_action  = get_action<bonestorm_damage_t>( "bonestorm_damage", p );
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    int charges =
        std::min( p()->buffs.bone_shield->check(), as<int>( p()->talent.blood.bonestorm->effectN( 4 ).base_value() ) );
    p()->buffs.bone_shield->decrement( charges );

    if ( charges > 0 )
    {
      if ( p()->talent.blood.ossified_vitriol->ok() )
        p()->buffs.ossified_vitriol->trigger( charges );

      if ( p()->talent.blood.insatiable_blade->ok() )
        p()->cooldown.dancing_rune_weapon->adjust( p()->talent.blood.insatiable_blade->effectN( 1 ).time_value() *
                                                   charges );

      if ( p()->talent.blood.shattering_bone.ok() )
      {
        // Set the number of charges of BS consumed, as it's used as a multiplier in shattering bone
        debug_cast<shattering_bone_t*>( p()->active_spells.shattering_bone )->boneshield_charges_consumed = charges;
        p()->active_spells.shattering_bone->execute_on_target( target );
      }
    }    

    p()->sim->print_debug( "Bonestorm consumed {} charges of bone shield", charges );
    return p()->talent.blood.bonestorm->duration() * charges;
  }

  void tick( dot_t* d ) override
  {
    death_knight_spell_t::tick( d );
    p()->buffs.bone_shield->trigger();

    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, TWW1, B4 ) )
        p() -> buffs.piledriver_tww1_4pc->trigger();

    if ( p()->talent.icy_talons.ok() )
    {
      p()->buffs.icy_talons->trigger();
    }
  }

  bool ready() override
  {
    if ( !p()->buffs.bone_shield->check() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Breath of Sindragosa =====================================================
struct breath_of_sindragosa_tick_t final : public death_knight_spell_t
{
  breath_of_sindragosa_tick_t( util::string_view n, death_knight_t* p )
    : death_knight_spell_t( n, p, p->talent.frost.breath_of_sindragosa->effectN( 1 ).trigger() ), ticking_cost( 0.0 )
  {
    aoe                 = -1;
    background          = true;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;
    // Extract the cost per tick from spelldata
    for ( size_t idx = 1; idx <= p->buffs.breath_of_sindragosa->data().power_count(); idx++ )
    {
      const spellpower_data_t& power = p->buffs.breath_of_sindragosa->data().powerN( idx );
      if ( power.aura_id() == 0 || player->dbc->spec_by_spell( power.aura_id() ) == player->specialization() )
      {
        ticking_cost = power.cost_per_tick();
      }
    }
    base_costs[ RESOURCE_RUNIC_POWER ] = ticking_cost;
    ap_type                            = attack_power_type::WEAPON_BOTH;

    if ( p->main_hand_weapon.group() == WEAPON_2H )
    {
      ap_type = attack_power_type::WEAPON_MAINHAND;
      // There's a 0.98 modifier hardcoded in the tooltip if a 2H weapon is equipped, probably server side magic
      base_multiplier *= 0.98;
    }
  }

  // Resource cost/loss handled by buff
  double cost() const override
  {
    return 0;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( p()->talent.frost.hyperpyrexia->ok() && state->result_amount > 0 &&
         p()->rng().roll( p()->talent.frost.hyperpyrexia->proc_chance() ) )
    {
      residual_action::trigger( p()->active_spells.hyperpyrexia_damage, state->target,
                                state->result_amount * p()->talent.frost.hyperpyrexia->effectN( 1 ).percent() );
    }
  }

private:
  double ticking_cost;
};

struct breath_of_sindragosa_t final : public death_knight_spell_t
{
  breath_of_sindragosa_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "breath_of_sindragosa", p, p->talent.frost.breath_of_sindragosa )
  {
    parse_options( options_str );
    base_tick_time = 0_ms;  // Handled by the buff
    add_child( get_action<breath_of_sindragosa_tick_t>( "breath_of_sindragosa_damage", p ) );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->buffs.breath_of_sindragosa->trigger();
  }

  // Breath of Sindragosa can not be used if there isn't enough resources available for one tick
  bool ready() override
  {
    if ( !p()->resource_available( RESOURCE_RUNIC_POWER, this->base_costs_per_tick[ RESOURCE_RUNIC_POWER ] ) )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Chains of Ice ============================================================

// Cold Heart damage
struct cold_heart_damage_t final : public death_knight_spell_t
{
  cold_heart_damage_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.cold_heart_damage )
  {
    background = true;
  }

  double action_multiplier() const override
  {
    double m = death_knight_spell_t::action_multiplier();

    m *= p()->buffs.cold_heart->check();

    return m;
  }
};

struct chains_of_ice_t final : public death_knight_spell_t
{
  chains_of_ice_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "chains_of_ice", p, p->spec.chains_of_ice )
  {
    parse_options( options_str );
    cold_heart = get_action<cold_heart_damage_t>( "cold_heart", p );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p()->buffs.cold_heart->check() > 0 )
    {
      cold_heart->execute_on_target( target );
      p()->buffs.cold_heart->expire();
    }
  }

private:
  propagate_const<action_t*> cold_heart;
};

// Chill Streak =============================================================

struct chill_streak_damage_t final : public death_knight_spell_t
{
  chill_streak_damage_t( util::string_view n, death_knight_t* p )
    : death_knight_spell_t( n, p, p->spell.chill_streak_damage ), current( 0 )
  {
    background = proc = true;
    max_bounces       = as<int>( p->talent.frost.chill_streak->effectN( 1 ).base_value() );
  }

  void execute() override
  {
    // Setting a variable min travel time to more accurately emulate in game variance
    min_travel_time = rng().gauss( p()->options.average_cs_travel_time, 0.2 );
    death_knight_spell_t::execute();
  }

  void impact( action_state_t* state ) override
  {
    if ( state->target->is_player() )
    {
      state->result_raw = state->result_amount = state->result_total = 0;
    }
    death_knight_spell_t::impact( state );
    current++;

    if ( !state->action->result_is_hit( state->result ) )
    {
      return;
    }

    if ( p()->talent.frost.enduring_chill.ok() &&
         rng().roll( p()->talent.frost.enduring_chill->effectN( 1 ).percent() ) )
    {
      current--;
      p()->procs.enduring_chill->occur();
    }

    auto td = get_td( state->target );

    if ( current < max_bounces )
    {
      td->debuff.chill_streak->trigger();
    }

    if ( p()->talent.frost.piercing_chill.ok() )
    {
      td->debuff.piercing_chill->trigger();
    }
  }

private:
  int max_bounces;

public:
  int current;
};

struct chill_streak_t final : public death_knight_spell_t
{
  chill_streak_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "chill_streak", p, p->talent.frost.chill_streak ),
      damage( get_action<chill_streak_damage_t>( "chill_streak_damage", p ) )
  {
    parse_options( options_str );
    add_child( damage );
    impact_action = damage;
    aoe           = 0;
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    debug_cast<chill_streak_damage_t*>( damage )->current = 0;
  }

private:
  action_t* damage;
};

// Consumption ==============================================================

struct consumption_heal_t final : public death_knight_leech_damage_heal_t
{
  consumption_heal_t( util::string_view name, death_knight_t* p )
    : death_knight_leech_damage_heal_t( name, p, p->talent.blood.consumption )
  {
    background = true;
    harmful    = false;
    callbacks = may_crit = may_miss = false;
    energize_type                   = action_energize::NONE;
    cooldown->duration              = 0_ms;
    target                          = p;
    attack_power_mod.direct = attack_power_mod.tick = 0;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_leech_damage_heal_t::impact( state );

    if ( p()->talent.blood.carnage.ok() )
      trigger_blood_shield( state );
  }
};

struct consumption_t final : public death_knight_melee_attack_t
{
  consumption_t( death_knight_t* p, util::string_view options_str )
    : death_knight_melee_attack_t( "consumption", p, p->talent.blood.consumption ),
      heal( get_action<consumption_heal_t>( "consumption_heal", p ) )
  {
    parse_options( options_str );
    aoe                 = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();
  }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    if ( state->result_amount > 0 )
    {
      heal->base_dd_min = heal->base_dd_max = state->result_amount;
      heal->execute();
    }
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    p()->buffs.consumption->trigger();
    p()->replenish_rune( as<unsigned int>( data().effectN( 4 ).base_value() ), p()->gains.consumption );

    if ( p()->pets.dancing_rune_weapon_pet.active_pet() != nullptr )
    {
      p()->pets.dancing_rune_weapon_pet.active_pet()->ability.consumption->execute_on_target( target );
    }

    if ( p()->talent.blood.everlasting_bond.ok() )
    {
      if ( p()->pets.everlasting_bond_pet.active_pet() != nullptr )
      {
        p()->pets.everlasting_bond_pet.active_pet()->ability.consumption->execute_on_target( target );
      }
    }
  }

private:
  propagate_const<action_t*> heal;
};

// Dancing Rune Weapon ======================================================
struct dancing_rune_weapon_t final : public death_knight_spell_t
{
  dancing_rune_weapon_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "dancing_rune_weapon", p, p->talent.blood.dancing_rune_weapon ), bone_shield_stack_gain( 0 )
  {
    may_miss = may_crit = may_dodge = may_parry = harmful = false;
    target                                                = p;
    if ( p->talent.blood.insatiable_blade.ok() )
      bone_shield_stack_gain += as<int>( p->talent.blood.insatiable_blade->effectN( 2 ).base_value() );
    parse_options( options_str );

    p->pets.dancing_rune_weapon_pet.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    if ( p->talent.blood.everlasting_bond.ok() )
    {
      p->pets.everlasting_bond_pet.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    if ( p()->talent.blood.insatiable_blade.ok() )
    {
      p()->buffs.bone_shield->trigger( bone_shield_stack_gain );
      if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, TWW1, B4 ) )
        p() -> buffs.piledriver_tww1_4pc->trigger( bone_shield_stack_gain );
    }

    // Only summon the rune weapons if the buff is down.
    if ( !p()->buffs.dancing_rune_weapon->up() )
    {
      p()->pets.dancing_rune_weapon_pet.spawn();

      if ( p()->talent.blood.everlasting_bond.ok() )
      {
        p()->pets.everlasting_bond_pet.spawn();
      }
    }
  }

private:
  int bone_shield_stack_gain;
};

// Dark Command =============================================================

struct dark_command_t final : public death_knight_spell_t
{
  dark_command_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "dark_command", p, p->find_class_spell( "Dark Command" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    use_off_gcd           = true;
  }

  void impact( action_state_t* s ) override
  {
    if ( s->target->is_enemy() )
      target->taunt( player );

    death_knight_spell_t::impact( s );
  }
};

// Dark Transformation ======================================================
struct dark_transformation_damage_t final : public death_knight_spell_t
{
  dark_transformation_damage_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.dark_transformation_damage )
  {
    background = dual  = true;
    name_str_reporting = "dark_transformation";
    aoe                = as<int>( data().effectN( 2 ).base_value() );
  }
};

struct dark_transformation_t final : public death_knight_spell_t
{
  dark_transformation_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "dark_transformation", p, p->talent.unholy.dark_transformation )
  {
    harmful = false;
    target  = p;

    execute_action = get_action<dark_transformation_damage_t>( "dark_transformation_damage", p );
    add_child( execute_action );
    execute_action->stats = stats;

    parse_options( options_str );

    if ( p->talent.unholy.unholy_blight.ok() )
    {
      add_child( get_action<unholy_blight_dot_t>( "unholy_blight_dot", p ) );
    }
    if ( p->talent.unholy.unholy_pact.ok() )
    {
      add_child( get_action<unholy_pact_damage_t>( "unholy_pact_damage", p ) );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->buffs.dark_transformation->trigger();

    // Rank 2 still exists for unholy as a baseline
    if ( p()->spec.dark_transformation_2->ok() )
    {
      p()->pets.ghoul_pet.active_pet()->resource_gain(
          RESOURCE_ENERGY, p()->spec.dark_transformation_2->effectN( 1 ).base_value(),
          p()->pets.ghoul_pet.active_pet()->dark_transformation_gain, this );
    }

    if ( p()->talent.unholy.unholy_pact.ok() )
    {
      p()->buffs.unholy_pact->trigger();
    }

    if ( p()->talent.unholy.commander_of_the_dead.ok() )
    {
      p()->buffs.commander_of_the_dead->trigger();
    }

    if ( p()->talent.unholy.unholy_blight.ok() )
    {
      p()->active_spells.unholy_blight->execute();
    }
  }

  bool ready() override
  {
    if ( p()->pets.ghoul_pet.active_pet() == nullptr )
    {
      return false;
    }

    return death_knight_spell_t::ready();
  }
};

// Death and Decay and Defile ===============================================

// Death and Decay direct damage spells
struct death_and_decay_damage_base_t : public death_knight_spell_t
{
  death_and_decay_damage_base_t( util::string_view name, death_knight_t* p, const spell_data_t* spell )
    : death_knight_spell_t( name, p, spell )
  {
    aoe        = -1;
    background = dual = true;
    tick_zero         = true;
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );
    if ( p()->talent.unholy.pestilence.ok() && rng().roll( p()->talent.unholy.pestilence->effectN( 1 ).percent() ) )
    {
      p()->trigger_festering_wound( s, 1, p()->procs.fw_pestilence );
    }
  }
};

struct death_and_decay_damage_t final : public death_and_decay_damage_base_t
{
  death_and_decay_damage_t( util::string_view name, death_knight_t* p, const spell_data_t* s = nullptr )
    : death_and_decay_damage_base_t( name, p, s == nullptr ? p->spell.death_and_decay_damage : s )
  {
  }
};

struct defile_damage_t final : public death_and_decay_damage_base_t
{
  defile_damage_t( util::string_view name, death_knight_t* p )
    : death_and_decay_damage_base_t( name, p, p->spell.defile_damage ),
      // Testing shows a 1.06 multiplicative damage increase for every tick of defile that hits an enemy
      // Can't seem to find it anywhere in defile's spelldata
      defile_tick_multiplier( 1.06 ),
      active_defile_multiplier( 1.0 )
  {
  }

  double action_multiplier() const override
  {
    double m = death_knight_spell_t::action_multiplier();

    m *= active_defile_multiplier;

    return m;
  }

  void execute() override
  {
    death_and_decay_damage_base_t::execute();
    // Increase damage of next ticks if it damages at least an enemy
    // Yes, it is multiplicative
    if ( hit_any_target )
    {
      active_defile_multiplier *= defile_tick_multiplier;
    }
  }

private:
  const double defile_tick_multiplier;

public:
  double active_defile_multiplier;
};

// Relish in Blood healing and RP generation
struct relish_in_blood_t final : public death_knight_heal_t
{
  relish_in_blood_t( util::string_view name, death_knight_t* p )
    : death_knight_heal_t( name, p, p->spell.relish_in_blood_gains )
  {
    background = true;
    target     = p;
  }

  double action_multiplier() const override
  {
    double m = death_knight_heal_t::action_multiplier();

    m *= p()->buffs.bone_shield->check();

    return m;
  }
};

// Main Death and Decay spells

struct death_and_decay_base_t : public death_knight_spell_t
{
  death_and_decay_base_t( death_knight_t* p, util::string_view name, const spell_data_t* spell )
    : death_knight_spell_t( name, p, spell ), damage( nullptr )
  {
    base_tick_time = dot_duration = 0_ms;  // Handled by event
    ignore_false_positive         = true;  // TODO: Is this necessary?
    // Note, radius and ground_aoe flag needs to be set in base so spell_targets expression works
    ground_aoe = true;
    radius     = data().effectN( 1 ).radius_max();
    // Set the player-stored death and decay cooldown to this action's cooldown
    p->cooldown.death_and_decay_dynamic = cooldown;

    if ( p->talent.blood.relish_in_blood.ok() )
      relish_in_blood = get_action<relish_in_blood_t>( "relish_in_blood", p );
  }

  void init_finished() override
  {
    death_knight_spell_t::init_finished();
    // Merge stats with the damage object
    damage->stats = stats;
    stats->action_list.push_back( damage );
  }

  double runic_power_generation_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::runic_power_generation_multiplier( state );

    if ( p()->specialization() == DEATH_KNIGHT_BLOOD && p()->buffs.crimson_scourge->check() )
    {
      m *= 1.0 + p()->buffs.crimson_scourge->data().effectN( 2 ).percent();
    }

    return m;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    // If bone shield isn't up, Relish in Blood doesn't heal or generate any RP
    if ( p()->specialization() == DEATH_KNIGHT_BLOOD && p()->buffs.crimson_scourge->up() &&
         p()->talent.blood.relish_in_blood.ok() && p()->buffs.bone_shield->up() )
    {
      // The heal's energize data automatically handles RP generation
      relish_in_blood->execute();
    }

    if ( p()->specialization() == DEATH_KNIGHT_BLOOD && p()->buffs.crimson_scourge->up() )
    {
      p()->buffs.crimson_scourge->decrement();
      if ( p()->talent.blood.perseverance_of_the_ebon_blade.ok() )
        p()->buffs.perseverance_of_the_ebon_blade->trigger();
      if ( p()->talent.sanlayn.visceral_strength )
      {
        p()->buffs.visceral_strength->trigger();
      }
    }

    make_event<ground_aoe_event_t>(
        *sim, p(),
        ground_aoe_params_t()
            .target( target )
            .duration( data().duration() )
            .pulse_time( compute_tick_time() )
            .action( damage )
            .x( target->x_position )
            .y( target->y_position )
            // Keep track of on-going dnd events
            .state_callback( [ this ]( ground_aoe_params_t::state_type type, ground_aoe_event_t* event ) {
              switch ( type )
              {
                case ground_aoe_params_t::EVENT_STARTED:
                  p()->active_dnd = event;
                  p()->buffs.death_and_decay->trigger();
                  break;
                case ground_aoe_params_t::EVENT_STOPPED:
                  p()->active_dnd = nullptr;
                  p()->buffs.death_and_decay->expire();
                  break;
                default:
                  break;
              }
            } ),
        true /* Immediate pulse */ );
  }

private:
  timespan_t compute_tick_time() const
  {
    auto base = data().effectN( 3 ).period();

    return base;
  }
  propagate_const<action_t*> relish_in_blood;

public:
  action_t* damage;
};

struct death_and_decay_t final : public death_and_decay_base_t
{
  death_and_decay_t( death_knight_t* p, util::string_view options_str )
    : death_and_decay_base_t( p, "death_and_decay", p->spec.death_and_decay )
  {
    damage = get_action<death_and_decay_damage_t>( "death_and_decay_damage", p );

    parse_options( options_str );

    // Disable when Defile or Death's Due are taken
    if ( p->talent.unholy.defile.ok() )
      background = true;
  }
};

struct defile_t final : public death_and_decay_base_t
{
  defile_t( death_knight_t* p, util::string_view options_str )
    : death_and_decay_base_t( p, "defile", p->talent.unholy.defile )
  {
    damage = get_action<defile_damage_t>( "defile_damage", p );

    parse_options( options_str );
  }

  void execute() override
  {
    // Reset the damage component's increasing multiplier
    debug_cast<defile_damage_t*>( damage )->active_defile_multiplier = 1.0;

    death_and_decay_base_t::execute();
  }
};

// Death's Caress ===========================================================

struct deaths_caress_t final : public death_knight_spell_t
{
  deaths_caress_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "deaths_caress", p, p->spec.deaths_caress )
  {
    parse_options( options_str );
    impact_action = get_action<blood_plague_t>( "blood_plague", p );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->buffs.bone_shield->trigger( as<int>( p()->spec.deaths_caress->effectN( 3 ).base_value() ) );
    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, TWW1, B4 ) )
        p() -> buffs.piledriver_tww1_4pc->trigger( as<int>( p()->spec.deaths_caress->effectN( 3 ).base_value() ) );

    if ( p()->pets.dancing_rune_weapon_pet.active_pet() != nullptr )
    {
      p()->pets.dancing_rune_weapon_pet.active_pet()->ability.deaths_caress->execute_on_target( target );
    }

    if ( p()->talent.blood.everlasting_bond.ok() )
    {
      if ( p()->pets.everlasting_bond_pet.active_pet() != nullptr )
      {
        p()->pets.everlasting_bond_pet.active_pet()->ability.deaths_caress->execute_on_target( target );
      }
    }
  }
};

// Death Coil ===============================================================

struct coil_of_devastation_t final : public residual_action::residual_periodic_action_t<death_knight_spell_t>
{
  coil_of_devastation_t( util::string_view name, death_knight_t* p )
    : residual_action::residual_periodic_action_t<death_knight_spell_t>( name, p, p->spell.coil_of_devastation_debuff )
  {
    background = dual = true;
    may_miss = hasted_ticks = false;
  }
};

struct death_coil_damage_t final : public death_knight_spell_t
{
  death_coil_damage_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.death_coil_damage ), coil_of_devastation( nullptr )
  {
    background = dual = true;
    aoe = 0;

    if ( p->talent.unholy.coil_of_devastation.ok() )
    {
      coil_of_devastation = get_action<coil_of_devastation_t>( "coil_of_devastation", p );
    }
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_spell_t::composite_target_multiplier( target );

    if ( p()->talent.unholy.reaping.ok() &&
         target->health_percentage() < p()->talent.unholy.reaping->effectN( 2 ).base_value() )
    {
      m *= 1.0 + p()->talent.unholy.reaping->effectN( 1 ).percent();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );
    if ( p()->talent.unholy.coil_of_devastation.ok() )
    {
      residual_action::trigger( coil_of_devastation, state->target,
                                state->result_amount * p()->talent.unholy.coil_of_devastation->effectN( 1 ).percent() );
    }
  }

private:
  propagate_const<action_t*> coil_of_devastation;
};

struct death_coil_t final : public death_knight_spell_t
{
  death_coil_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "death_coil", p, p->spec.death_coil )
  {
    parse_options( options_str );

    impact_action = get_action<death_coil_damage_t>( "death_coil_damage", p );
    impact_action->stats = stats;
    stats->action_list.push_back( impact_action );

    aoe = 1 + as<int>( p->talent.unholy.improved_death_coil->effectN( 2 ).base_value() );

    if ( p->talent.unholy.coil_of_devastation.ok() )
      add_child( get_action<coil_of_devastation_t>( "coil_of_devastation", p ) );

    if ( p->talent.unholy.doomed_bidding.ok() )
    {
      p->pets.doomed_bidding_magus_coil.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p()->buffs.sudden_doom->check() )
    {
      if ( p()->talent.unholy.doomed_bidding.ok() )
      {
        p()->pets.doomed_bidding_magus_coil.spawn();
      }

      if ( p()->talent.sanlayn.visceral_strength )
      {
        p()->buffs.visceral_strength->trigger();
      }
    }

    if ( p()->buffs.dark_transformation->up() && p()->talent.unholy.eternal_agony.ok() )
    {
      p()->buffs.dark_transformation->extend_duration(
          p(), timespan_t::from_seconds( p()->talent.unholy.eternal_agony->effectN( 1 ).base_value() ) );
    }

    if ( p()->talent.sanlayn.vampiric_strike.ok() && !p()->buffs.gift_of_the_sanlayn->check() )
    {
      p()->trigger_vampiric_strike_proc( target );
    }

    p()->buffs.sudden_doom->decrement();
  }


  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( p()->talent.unholy.death_rot.ok() )
    {
      get_td( state->target )->debuff.death_rot->trigger( 1 + p()->buffs.sudden_doom->check() );
    }

    if ( p()->talent.unholy.rotten_touch.ok() && p()->buffs.sudden_doom->check() )
    {
      get_td( state->target )->debuff.rotten_touch->trigger();
    }

    if ( p()->buffs.sudden_doom->check() )
    {
      p()->burst_festering_wound( state->target, as<int>( p()->talent.unholy.sudden_doom->effectN( 3 ).base_value() ),
                                  p()->procs.fw_sudden_doom );
    }
  }
};

// Death Strike =============================================================

struct death_strike_heal_t final : public death_knight_heal_t
{
  death_strike_heal_t( util::string_view name, death_knight_t* p )
    : death_knight_heal_t( name, p, p->spell.death_strike_heal ),
      interval( timespan_t::from_seconds( p->talent.death_strike->effectN( 4 ).base_value() ) ),
      last_buff_consumption( timespan_t::zero() ),
      min_heal_multiplier( p->talent.death_strike->effectN( 3 ).percent() ),
      max_heal_multiplier( p->talent.death_strike->effectN( 2 ).percent() )
  {
    may_crit = callbacks = false;
    background           = true;
    target               = p;

    if ( p->talent.improved_death_strike.ok() )
    {
      // Blood only gets 10%, dps specs get 60%
      if ( p->specialization() == DEATH_KNIGHT_BLOOD )
      {
        // Min and max pulls from the same effect for blood.
        min_heal_multiplier *= 1.0 + p->talent.improved_death_strike->effectN( 4 ).percent();
        max_heal_multiplier *= 1.0 + p->talent.improved_death_strike->effectN( 4 ).percent();
      }
      else
      {
        min_heal_multiplier *= 1.0 + p->talent.improved_death_strike->effectN( 2 ).percent();
        max_heal_multiplier *= 1.0 + p->talent.improved_death_strike->effectN( 1 ).percent();
      }
    }
  }

  void init() override
  {
    death_knight_heal_t::init();

    snapshot_flags |= STATE_MUL_DA;
  }

  double base_da_min( const action_state_t* ) const override
  {
    auto min_heal = player->resources.max[ RESOURCE_HEALTH ] * min_heal_multiplier;
    auto cur_heal = player->compute_incoming_damage( std::min(interval, ( sim->current_time() - last_buff_consumption ) ) ) * max_heal_multiplier;

    return std::max( min_heal, cur_heal );
  }

  double base_da_max( const action_state_t* ) const override
  {
    auto min_heal = player->resources.max[ RESOURCE_HEALTH ] * min_heal_multiplier;
    auto cur_heal = player->compute_incoming_damage( std::min( interval, ( sim->current_time() - last_buff_consumption ) ) ) * max_heal_multiplier;

    return std::max( min_heal, cur_heal );
  }

  double action_multiplier() const override
  {
    double m = death_knight_heal_t::action_multiplier();

    m *= 1.0 + p()->buffs.hemostasis->stack_value();

    return m;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_heal_t::impact( state );

    auto min_heal = player->resources.max[ RESOURCE_HEALTH ] * min_heal_multiplier;
    auto cur_heal = player->compute_incoming_damage( interval ) * max_heal_multiplier;

    // Reset timer if we healed for more than a min heal
    if ( min_heal < cur_heal )
      last_buff_consumption = sim->current_time();

    trigger_blood_shield( state );
  }

private:
  timespan_t interval;
  timespan_t last_buff_consumption;
  double min_heal_multiplier;
  double max_heal_multiplier;
};

struct death_strike_offhand_t final : public death_knight_melee_attack_t
{
  death_strike_offhand_t( util::string_view name, death_knight_t* p )
    : death_knight_melee_attack_t( name, p, p->spell.death_strike_offhand )
  {
    background = true;
    weapon     = &( p->off_hand_weapon );
  }
};

struct death_strike_t final : public death_knight_melee_attack_t
{
  death_strike_t( death_knight_t* p, util::string_view options_str )
    : death_knight_melee_attack_t( "death_strike", p, p->talent.death_strike ),
      heal( get_action<death_strike_heal_t>( "death_strike_heal", p ) ),
      oh_attack( nullptr ),
      improved_death_strike_reduction( 0 )
  {
    parse_options( options_str );
    may_parry = false;

    weapon = &( p->main_hand_weapon );

    if ( p->dual_wield() )
    {
      oh_attack = get_action<death_strike_offhand_t>( "death_strike_offhand", p );
      add_child( oh_attack );
    }

    if ( p->talent.improved_death_strike.ok() )
    {
      if ( p->specialization() == DEATH_KNIGHT_BLOOD )
        improved_death_strike_reduction +=
            p->talent.improved_death_strike->effectN( 5 ).resource( RESOURCE_RUNIC_POWER );
      else
        improved_death_strike_reduction +=
            p->talent.improved_death_strike->effectN( 3 ).resource( RESOURCE_RUNIC_POWER );
    }
  }

  double action_multiplier() const override
  {
    double m = death_knight_melee_attack_t::action_multiplier();

    // Death Strike is affected by bloodshot, even for the applying DS.  This is because in game, blood shield gets
    // applied before DS damage is calculated. So we apply the modifier here, but only if bloodshield is not up, to
    // avoid double dip by the base multipler when blood shield is up and bloodshot is talented.
    if ( p()->talent.blood.bloodshot.ok() && !p()->buffs.blood_shield->up() )
    {
      m *= 1.0 + p()->talent.blood.bloodshot->effectN( 1 ).percent();
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    // 2020-08-23: Seems to only affect main hand death strike, not OH hit, not DRW's spell
    // No spelldata either, just "increase damage based on target's missing health"
    // Update 2021-06-16 Changed from 1% to 1.25% damage increase
    // Testing shows a linear 1.25% damage increase for every 1% missing health
    // Unholy Bond increases this by 10% per point, with the formula 1 + ( 0.25 * ( 1 + unholy_bond % ) )
    if ( p()->runeforge.rune_of_sanguination )
    {
      auto increase    = 1 + ( 0.25 * ( 1 + p()->talent.unholy_bond->effectN( 1 ).percent() ) );
      auto buff_amount = ( 1.0 - target->health_percentage() / 100 ) * increase;
      buff_amount      = std::min( buff_amount, ( increase * 80 ) / 100 );  // Caps at 80% of target's hp missing
      m *= 1.0 + buff_amount;
    }

    return m;
  }

  double cost_flat_modifier() const override
  {
    double c = death_knight_melee_attack_t::cost_flat_modifier();

    c += improved_death_strike_reduction;

    return c;
  }

  void execute() override
  {
    p()->buffs.voracious->trigger();

    death_knight_melee_attack_t::execute();

    p()->buffs.coagulopathy->trigger();

    if ( oh_attack )
      oh_attack->execute_on_target( execute_state->target );

    if ( p()->pets.dancing_rune_weapon_pet.active_pet() != nullptr )
    {
      p()->pets.dancing_rune_weapon_pet.active_pet()->ability.death_strike->execute_on_target( target );
    }

    if ( p()->talent.blood.everlasting_bond.ok() )
    {
      if ( p()->pets.everlasting_bond_pet.active_pet() != nullptr )
      {
        p()->pets.everlasting_bond_pet.active_pet()->ability.death_strike->execute_on_target( target );
      }
    }

    if ( hit_any_target )
    {
      heal->execute();
    }

    p()->buffs.hemostasis->expire();
    p()->buffs.heartrend->expire();
    if ( p()->talent.sanlayn.vampiric_strike.ok() && !p()->buffs.gift_of_the_sanlayn->check() )
    {
      p()->trigger_vampiric_strike_proc( target );
    }
  }

private:
  propagate_const<action_t*> heal;
  propagate_const<action_t*> oh_attack;
  double improved_death_strike_reduction;
};

// Empower Rune Weapon ======================================================
struct empower_rune_weapon_t final : public death_knight_spell_t
{
  empower_rune_weapon_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "empower_rune_weapon", p, p->talent.frost.empower_rune_weapon )
  {
    parse_options( options_str );

    harmful = false;
    target  = p;

    // Buff handles the ticking, this one just triggers the buff
    dot_duration = base_tick_time = 0_ms;
    cooldown->charges =
        1;  // Data appears to be messed up since they removed the class tree version, lists as 0 charges
    cooldown->duration = p->talent.frost.empower_rune_weapon->charge_cooldown();
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->buffs.empower_rune_weapon->trigger();
  }
};

// Epidemic =================================================================
struct epidemic_damage_base_t : public death_knight_spell_t
{
  epidemic_damage_base_t( util::string_view name, death_knight_t* p, const spell_data_t* spell )
    : death_knight_spell_t( name, p, spell ), soft_cap_multiplier( 1.0 )
  {
    background = true;
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double cam = death_knight_spell_t::composite_aoe_multiplier( state );

    cam *= soft_cap_multiplier;

    return cam;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( state );

    if ( p()->talent.unholy.harbinger_of_doom.ok() && p()->buffs.sudden_doom->check() )
    {
      m *= 1.0 + p()->talent.unholy.harbinger_of_doom->effectN( 4 ).percent() * p()->buffs.sudden_doom->check();
    }

    return m;
  }

public:
  double soft_cap_multiplier;
};
struct epidemic_damage_main_t final : public epidemic_damage_base_t
{
  epidemic_damage_main_t( util::string_view name, death_knight_t* p )
    : epidemic_damage_base_t( name, p, p->spell.epidemic_damage )
  {
    // Ignore spelldata for max targets for the main spell, as it is single target only
    aoe = 0;
    // this spell has both coefficients in it, and it seems like it is reading #2, the aoe portion, instead of #1
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
  }
};

struct epidemic_damage_aoe_t final : public epidemic_damage_base_t
{
  epidemic_damage_aoe_t( util::string_view name, death_knight_t* p )
    : epidemic_damage_base_t( name, p, p->spell.epidemic_damage )
  {
    // Main is one target, aoe is the other targets, so we take 1 off the max targets
    aoe = aoe - 1;

    attack_power_mod.direct = data().effectN( 2 ).ap_coeff();
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    death_knight_spell_t::available_targets( tl );

    auto it = range::find( tl, target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    return tl.size();
  }
};

struct epidemic_t final : public death_knight_spell_t
{
  epidemic_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "epidemic", p, p->spec.epidemic ),
      custom_reduced_aoe_targets( 8.0 ),
      soft_cap_multiplier( 1.0 ),
      sd( false )
  {
    parse_options( options_str );

    impact_action                = get_action<epidemic_damage_main_t>( "epidemic_main", p );
    impact_action->impact_action = get_action<epidemic_damage_aoe_t>( "epidemic_aoe", p );

    add_child( impact_action );
    add_child( impact_action->impact_action );
    if ( p->talent.unholy.doomed_bidding.ok() )
    {
      p->pets.doomed_bidding_magus_epi.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    }
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    death_knight_spell_t::available_targets( tl );

    // Remove enemies that are not affected by virulent plague
    range::erase_remove( tl, [ this ]( player_t* t ) { return !get_td( t )->dot.virulent_plague->is_ticking(); } );

    return tl.size();
  }

  void execute() override
  {
    // Reset target cache because of smart targetting
    target_cache.is_valid = false;

    death_knight_spell_t::execute();

    if ( p()->buffs.dark_transformation->up() && p()->talent.unholy.eternal_agony.ok() )
    {
      p()->buffs.dark_transformation->extend_duration(
          p(), timespan_t::from_seconds( p()->talent.unholy.eternal_agony->effectN( 1 ).base_value() ) );
    }

    if ( p()->talent.unholy.doomed_bidding.ok() && p()->buffs.sudden_doom->check() )
    {
      p()->pets.doomed_bidding_magus_epi.spawn();
    }

    if ( p()->talent.sanlayn.visceral_strength && p()->buffs.sudden_doom->check() )
    {
      p()->buffs.visceral_strength->trigger();
    }

    if ( p()->buffs.sudden_doom->check() )
    {
      sd = true;
      p()->buffs.sudden_doom->decrement();
    }
    else
    {
      sd = false;
    }

    if ( p()->talent.sanlayn.vampiric_strike.ok() && !p()->buffs.gift_of_the_sanlayn->check() )
    {
      p()->trigger_vampiric_strike_proc( target );
    }
  }

  void impact( action_state_t* state ) override
  {
    // Set the multiplier for reduced aoe soft cap
    if ( state->n_targets > 0.0 && state->n_targets > custom_reduced_aoe_targets )
      soft_cap_multiplier =
          sqrt( custom_reduced_aoe_targets / std::min<int>( sim->max_aoe_enemies, state->n_targets ) );
    else
      soft_cap_multiplier = 1.0;

    debug_cast<epidemic_damage_main_t*>( impact_action )->soft_cap_multiplier               = soft_cap_multiplier;
    debug_cast<epidemic_damage_aoe_t*>( impact_action->impact_action )->soft_cap_multiplier = soft_cap_multiplier;

    death_knight_spell_t::impact( state );

    if ( p()->talent.unholy.death_rot.ok() && result_is_hit( state->result ) )
    {
      get_td( state->target )->debuff.death_rot->trigger();

      if ( sd )
      {
        get_td( state->target )->debuff.death_rot->trigger();
      }
    }
  }

private:
  double custom_reduced_aoe_targets;  // Not in spelldata
  double soft_cap_multiplier;
  bool sd;
};

// Festering Strike and Wounds ===============================================

struct bursting_sores_t final : public death_knight_spell_t
{
  bursting_sores_t( util::string_view n, death_knight_t* p )
    : death_knight_spell_t( n, p, p->spell.bursting_sores_damage )
  {
    background          = true;
    aoe                 = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();
  }

  // Bursting sores have a slight delay ingame, but nothing really significant
  // The travel time is overriden to zero to make it trigger properly on enemy death
  timespan_t travel_time() const override
  {
    return 0_ms;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    death_knight_spell_t::available_targets( tl );

    // Does not hit the main target
    auto it = range::find( tl, target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    return tl.size();
  }
};

struct festering_wound_application_t final : public death_knight_spell_t
{
  festering_wound_application_t( util::string_view n, death_knight_t* p )
    : death_knight_spell_t( n, p, p->spec.festering_wound )
  {
    background = true;
  }
};

struct festering_wound_t final : public death_knight_spell_t
{
  festering_wound_t( util::string_view n, death_knight_t* p )
    : death_knight_spell_t( n, p, p->spell.festering_wound_damage ),
      bursting_sores( get_action<bursting_sores_t>( "bursting_sores", p ) )
  {
    background = true;
    if ( p->talent.unholy.bursting_sores.ok() )
    {
      add_child( bursting_sores );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->resource_gain(
        RESOURCE_RUNIC_POWER,
        p()->spec.festering_wound->effectN( 1 ).trigger()->effectN( 2 ).resource( RESOURCE_RUNIC_POWER ),
        p()->gains.festering_wound, this );
  }

private:
  action_t* bursting_sores;
};

struct festering_base_t : public death_knight_melee_attack_t
{
  festering_base_t( util::string_view n, death_knight_t* p, const spell_data_t* s )
    : death_knight_melee_attack_t( n, p, s )
  {
  }

  void impact( action_state_t* s ) override
  {
    static const std::array<unsigned, 2> fw_proc_stacks = { { 2, 3 } };

    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      size_t n          = rng().range( size_t(), fw_proc_stacks.size() );
      unsigned n_stacks = fw_proc_stacks[ n ];

      p()->trigger_festering_wound( s, n_stacks, this->data().id() == p()->spell.festering_scythe->id() ? p()->procs.fw_festering_scythe : p()->procs.fw_festering_strike);
    }
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    if ( p()->talent.unholy.reaping.ok() &&
         target->health_percentage() < p()->talent.unholy.reaping->effectN( 2 ).base_value() )
    {
      m *= 1.0 + p()->talent.unholy.reaping->effectN( 1 ).percent();
    }

    return m;
  }
};

struct festering_scythe_t final : public festering_base_t
{
  festering_scythe_t( death_knight_t* p ) : festering_base_t( "festering_scythe", p, p->spell.festering_scythe )
  {
    aoe = -1;
  }

  void execute() override
  {
    festering_base_t::execute();
    p()->buffs.festering_scythe->expire();
  }
};

struct festering_strike_t final : public festering_base_t
{
  festering_strike_t( death_knight_t* p, util::string_view options_str )
    : festering_base_t( "festering_strike", p, p->talent.unholy.festering_strike ),
      festering_scythe( nullptr ),
      festering_scythe_cost( 0 )
  {
    parse_options( options_str );
    if ( p->talent.unholy.festering_scythe.ok() )
    {
      festering_scythe      = new festering_scythe_t( p );
      festering_scythe_cost = festering_scythe->data().cost( POWER_RUNE );
      add_child( festering_scythe );
    }
  }

  double cost() const override
  {
    if ( p()->talent.unholy.festering_scythe.ok() && p()->buffs.festering_scythe->check() )
    {
      return festering_scythe_cost;
    }
    else
    {
      return base_costs[ RESOURCE_RUNE ];
    }
  }

  void execute() override
  {
    if ( p()->talent.unholy.festering_scythe.ok() && p()->buffs.festering_scythe->check() )
    {
      festering_scythe->execute_on_target( target );
      stats->add_execute( 0_ms, target );
      return;
    }
    festering_base_t::execute();
  }

private:
  festering_scythe_t* festering_scythe;
  double festering_scythe_cost;
};

// Frostscythe ==============================================================

struct frostscythe_t final : public death_knight_melee_attack_t
{
  frostscythe_t( death_knight_t* p, util::string_view options_str )
    : death_knight_melee_attack_t( "frostscythe", p, p->talent.frost.frostscythe )
  {
    parse_options( options_str );

    inexorable_assault = get_action<inexorable_assault_damage_t>( "inexorable_assault", p );

    weapon              = &( player->main_hand_weapon );
    aoe                 = -1;
    reduced_aoe_targets = data().effectN( 5 ).base_value();
    base_crit           = 1.0;
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    if ( p()->buffs.inexorable_assault->up() && p()->cooldown.inexorable_assault_icd->is_ready() )
    {
      inexorable_assault->set_target( target );
      inexorable_assault->schedule_execute();
      p()->buffs.inexorable_assault->decrement();
      p()->cooldown.inexorable_assault_icd->start();
    }

    if ( p()->talent.frost.enduring_strength.ok() && p()->buffs.pillar_of_frost->up() &&
         p()->cooldown.enduring_strength_icd->is_ready() )
    {
      p()->buffs.enduring_strength_builder->trigger();
      p()->cooldown.enduring_strength_icd->start();
    }
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    // Frostscythe procs rime at half the chance of Obliterate
    p()->buffs.rime->trigger( 1, buff_t::DEFAULT_VALUE(), p()->buffs.rime->manual_chance / 2.0 );
  }

private:
  propagate_const<action_t*> inexorable_assault;
};

// Frostwyrm's Fury =========================================================
struct frostwyrms_fury_damage_t : public death_knight_spell_t
{
  frostwyrms_fury_damage_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.frostwyrms_fury_damage )
  {
    aoe                = -1;
    background         = true;
    cooldown->duration = 0_ms;  // handled by the actions
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p()->talent.rider.apocalypse_now.ok() )
    {
      p()->summon_rider( p()->spell.apocalypse_now_data->duration(), false );
    }
  }
};

struct frostwyrms_fury_t final : public death_knight_spell_t
{
  frostwyrms_fury_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "frostwyrms_fury_driver", p, p->talent.frost.frostwyrms_fury )
  {
    parse_options( options_str );
    execute_action = get_action<frostwyrms_fury_damage_t>( "frostwyrms_fury", p );
    // Stun is NYI
  }
};

// Frost Strike =============================================================

struct shattered_frost_t final : public death_knight_melee_attack_t
{
  shattered_frost_t( util::string_view n, death_knight_t* p )
    : death_knight_melee_attack_t( n, p, p->spell.shattered_frost )
  {
    background          = true;
    aoe                 = -1;
    reduced_aoe_targets = as<int>( data().effectN( 2 ).base_value() );
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    death_knight_melee_attack_t::available_targets( tl );

    auto it = range::find( tl, target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    return tl.size();
  }
};

struct frost_strike_strike_t final : public death_knight_melee_attack_t
{
  frost_strike_strike_t( util::string_view n, death_knight_t* p, weapon_t* w, const spell_data_t* s,
                         bool shattering_blade )
    : death_knight_melee_attack_t( n, p, s ),
      sb( shattering_blade ),
      weapon_hand( w ),
      damage( 0 ),
      shattered_frost( nullptr )
  {
    background = special = true;
    weapon               = w;
    if ( p->talent.frost.shattered_frost.ok() )
    {
      shattered_frost = get_action<shattered_frost_t>( "shattered_frost", p );
    }
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_melee_attack_t::composite_da_multiplier( state );

    if ( sb )
    {
      m *= 1.0 + p()->talent.frost.shattering_blade->effectN( 1 ).percent();
    }

    return m;
  }

  void trigger_shattered_frost( double hit_damage, bool final_hit )
  {
    damage += hit_damage;
    if ( final_hit )
    {
      damage *= p()->talent.frost.shattered_frost->effectN( 1 ).percent();
      shattered_frost->base_dd_min = shattered_frost->base_dd_max = damage;
      shattered_frost->execute();
      damage = 0;
    }
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );
    if ( p()->talent.frost.shattered_frost->ok() && s->result_amount > 0 && sb )
    {
      if ( weapon_hand->group() == WEAPON_2H )
      {
        trigger_shattered_frost( s->result_amount, true );
      }
      if ( weapon_hand->group() == WEAPON_1H )
      {
        if ( weapon_hand->slot == SLOT_MAIN_HAND )
        {
          trigger_shattered_frost( s->result_amount,
                                   true /* TODO-TWW check if still bugged p()->off_hand_weapon.type == WEAPON_NONE */ );
        }
        /*if ( weapon_hand->slot == SLOT_OFF_HAND )
        {
          trigger_shattered_frost( s->result_amount, true );
        }*/
      }
    }

    if ( p()->talent.frost.hyperpyrexia->ok() && s->result_amount > 0 &&
         p()->rng().roll( p()->talent.frost.hyperpyrexia->proc_chance() ) )
    {
      residual_action::trigger( p()->active_spells.hyperpyrexia_damage, s->target,
                                s->result_amount * p()->talent.frost.hyperpyrexia->effectN( 1 ).percent() );
    }
  }

public:
  bool sb;

private:
  weapon_t* weapon_hand;
  double damage;
  action_t* shattered_frost;
};

struct frost_strike_t final : public death_knight_melee_attack_t
{
  frost_strike_t( death_knight_t* p, util::string_view options_str )
    : death_knight_melee_attack_t( "frost_strike", p, p->talent.frost.frost_strike ),
      mh( p->active_spells.frost_strike_main ),
      oh( p->active_spells.frost_strike_offhand ),
      mh_sb( p->active_spells.frost_strike_sb_main ),
      oh_sb( p->active_spells.frost_strike_sb_offhand ),
      mh_delay( 0_ms ),
      oh_delay( 0_ms ),
      sb( false )
  {
    parse_options( options_str );

    dual = true;

    if ( data().ok() )
    {
      if ( p->main_hand_weapon.group() == WEAPON_2H )
      {
        mh_delay = timespan_t::from_millis( as<int>( data().effectN( 4 ).misc_value1() ) );
      }

      if ( p->off_hand_weapon.type != WEAPON_NONE )
      {
        oh_delay = timespan_t::from_millis( as<int>( data().effectN( 3 ).misc_value1() ) );
      }

      add_child( mh );
      if ( p->talent.frost.shattering_blade.ok() )
      {
        add_child( mh_sb );
      }

      if ( p->off_hand_weapon.type != WEAPON_NONE && p->main_hand_weapon.group() != WEAPON_2H )
      {
        add_child( oh );
        if ( p->talent.frost.shattering_blade.ok() )
        {
          add_child( oh_sb );
        }
      }

      if ( p->talent.frost.shattered_frost.ok() )
      {
        add_child( get_action<shattered_frost_t>( "shattered_frost", p ) );
      }
    }
  }

  void execute() override
  {
    const auto td = get_td( target );

    if ( p()->talent.frost.shattering_blade.ok() && td->debuff.razorice->at_max_stacks() )
    {
      sb = true;
      td->debuff.razorice->expire();
    }

    death_knight_melee_attack_t::execute();

    if ( hit_any_target )
    {
      if ( !sb )
      {
        make_event<delayed_execute_event_t>( *sim, p(), mh, execute_state->target, mh_delay );
        if ( oh )
        {
          make_event<delayed_execute_event_t>( *sim, p(), oh, execute_state->target, oh_delay );
        }
      }
      if ( sb )
      {
        make_event<delayed_execute_event_t>( *sim, p(), mh_sb, execute_state->target, mh_delay );
        if ( oh_sb )
        {
          make_event<delayed_execute_event_t>( *sim, p(), oh_sb, execute_state->target, oh_delay );
        }
        sb = false;
      }
    }

    if ( p()->buffs.pillar_of_frost->up() && p()->talent.frost.obliteration.ok() )
    {
      p()->trigger_killing_machine( 1.0, p()->procs.km_from_obliteration_fs,
                                    p()->procs.km_from_obliteration_fs_wasted );

      // Obliteration's rune generation
      if ( rng().roll( p()->talent.frost.obliteration->effectN( 2 ).percent() ) )
      {
        p()->replenish_rune( as<int>( p()->spell.obliteration_gains->effectN( 1 ).base_value() ),
                             p()->gains.obliteration );
      }
    }
  }

private:
  action_t *&mh, *&oh, *&mh_sb, *&oh_sb;
  timespan_t mh_delay;
  timespan_t oh_delay;
  bool sb;
};

// Glacial Advance ==========================================================

struct glacial_advance_damage_t final : public death_knight_spell_t
{
  glacial_advance_damage_t( util::string_view name, death_knight_t* p, bool aa = false )
    : death_knight_spell_t( name, p, p->spell.glacial_advance_damage ), is_arctic_assault( aa )
  {
    aoe        = -1;  // TODO: Fancier targeting .. make it aoe for now
    background = true;
    ap_type    = attack_power_type::WEAPON_BOTH;
    if ( p->main_hand_weapon.group() == WEAPON_2H )
    {
      ap_type = attack_power_type::WEAPON_MAINHAND;
      // There's a 0.98 modifier hardcoded in the tooltip if a 2H weapon is equipped, probably server side magic
      base_multiplier *= 0.98;
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    // Killing Machine glacial advcances trigger Unleashed Frenzy without spending Runic Power
    // Currently does not trigger Obliteration rune generation
    if ( is_arctic_assault )
    {
      if ( p()->talent.frost.unleashed_frenzy.ok() )
      {
        p()->buffs.unleashed_frenzy->trigger();
      }
      if ( p()->talent.icy_talons.ok() )
      {
        p()->buffs.icy_talons->trigger();
      }

      // TWW-TODO: Re-verify what RP effects Arctic Assault procs
      if ( p()->sets->has_set_bonus( DEATH_KNIGHT_FROST, TWW1, B4 ) && p()->rppm.tww1_fdk_4pc->trigger() )
      {
        p()->buffs.icy_vigor->trigger();
      }
    }
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );
  
    get_td( state->target )->debuff.razorice->trigger();
    if ( is_arctic_assault )
    {
      p()->procs.razorice_from_arctic_assault->occur();
    }
    else
    {
      p()->procs.razorice_from_glacial_advance->occur();
    }   

    if ( p()->talent.frost.hyperpyrexia->ok() && state->result_amount > 0 &&
         p()->rng().roll( p()->talent.frost.hyperpyrexia->proc_chance() ) )
    {
      residual_action::trigger( p()->active_spells.hyperpyrexia_damage, state->target,
                                state->result_amount * p()->talent.frost.hyperpyrexia->effectN( 1 ).percent() );
    }
  }

private:
  bool is_arctic_assault;
};

struct glacial_advance_t final : public death_knight_spell_t
{
  glacial_advance_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "glacial_advance", p, p->talent.frost.glacial_advance )
  {
    parse_options( options_str );

    weapon = &( p->main_hand_weapon );

    execute_action = get_action<glacial_advance_damage_t>( "glacial_advance_damage", p );
    add_child( execute_action );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p()->buffs.pillar_of_frost->up() && p()->talent.frost.obliteration.ok() )
    {
      p()->trigger_killing_machine( 1.0, p()->procs.km_from_obliteration_ga,
                                    p()->procs.km_from_obliteration_ga_wasted );

      // Obliteration's rune generation
      if ( rng().roll( p()->talent.frost.obliteration->effectN( 2 ).percent() ) )
      {
        p()->replenish_rune( as<int>( p()->spell.obliteration_gains->effectN( 1 ).base_value() ),
                             p()->gains.obliteration );
      }
    }
  }
};

// Gorefiend's Grasp ========================================================

struct gorefiends_grasp_t final : public death_knight_spell_t
{
  gorefiends_grasp_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "gorefiends_grasp", p, p->talent.blood.gorefiends_grasp )
  {
    parse_options( options_str );
    aoe = -1;
  }
};

// Heart Strike =============================================================

struct leeching_strike_t final : public death_knight_heal_t
{
  leeching_strike_t( util::string_view name, death_knight_t* p )
    : death_knight_heal_t( name, p, p->spell.leeching_strike_damage )
  {
    background = true;
    may_crit = callbacks = false;
    target               = p;
    // As of July 20 2024, heals for 2.5% of health, represented as 0.25 in spelldata
    base_pct_heal        = data().effectN( 1 ).base_value() / 10;
  }
};

struct heart_strike_base_t : public death_knight_melee_attack_t
{
  heart_strike_base_t( util::string_view n, death_knight_t* p, const spell_data_t* s )
    : death_knight_melee_attack_t( n, p, s ),
      heartbreaker_rp_gen( p->spell.heartbreaker_rp_gain->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) )
  {
    aoe             = 2;
    weapon          = &( p->main_hand_weapon );
    leeching_strike = get_action<leeching_strike_t>( "leeching_strike", p );
  }

  int n_targets() const override
  {
    return p()->in_death_and_decay() ? aoe + as<int>( p()->talent.cleaving_strikes->effectN( 3 ).base_value() ) : aoe;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( p()->talent.blood.heartrend.ok() )
    {
      p()->buffs.heartrend->trigger();
    }

    if ( p()->talent.deathbringer.dark_talons.ok() && p()->buffs.icy_talons->check() &&
         rng().roll( p()->talent.deathbringer.dark_talons->effectN( 1 ).percent() ) )
    {
      p()->buffs.dark_talons_icy_talons->trigger();
    }

    p()->trigger_sanlayn_execute_talents( this->data().id() == p()->spell.vampiric_strike->id() );
  }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    if ( p()->talent.blood.heartbreaker.ok() && result_is_hit( state->result ) )
    {
      p()->resource_gain( RESOURCE_RUNIC_POWER, heartbreaker_rp_gen, p()->gains.heartbreaker, this );
    }

    if ( p()->talent.blood.leeching_strike.ok() && get_td( state->target )->dot.blood_plague->is_ticking() )
    {
      leeching_strike->execute();
    }

    if ( p()->talent.sanlayn.incite_terror.ok() )
    {
      auto td = get_td( state->target );
      td->debuff.incite_terror->trigger();
    }

    if ( p()->talent.sanlayn.infliction_of_sorrow.ok() )
    {
      p()->trigger_infliction_of_sorrow( state->target, this->data().id() == p()->spell.vampiric_strike->id() );
    }
  }

private:
  propagate_const<action_t*> leeching_strike;
  double heartbreaker_rp_gen;
};

struct vampiric_strike_blood_t : public heart_strike_base_t
{
  vampiric_strike_blood_t( util::string_view n, death_knight_t* p, bool bloodied_blade_triggered )
    : heart_strike_base_t( n, p, p->spell.vampiric_strike )
  {
    attack_power_mod.direct = data().effectN( 5 ).ap_coeff();
    energize_amount         = std::fabs( data().powerN( 2 ).cost() );

    if ( bloodied_blade_triggered )
    {
      background = true;
    }
    else
    {
      if ( p->talent.sanlayn.infliction_of_sorrow.ok() )
      {
        // We only can have this be a child of a single thing, so parent it to the main vampiric strike
        add_child( p->active_spells.infliction_of_sorrow );
      }
      // In game bloodied blade vamp strikes will proc this. But I have no desire to re-write this all right now, and
      // expect this to change due to how buggy it is.
      if ( p->talent.sanlayn.the_blood_is_life.ok() )
      {
        p->pets.blood_beast.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
        add_child( p->active_spells.the_blood_is_life );
      }
    }
  }

  void execute() override
  {
    heart_strike_base_t::execute();

    if ( p()->pets.dancing_rune_weapon_pet.active_pet() != nullptr )
    {
      p()->pets.dancing_rune_weapon_pet.active_pet()->ability.vampiric_strike->execute_on_target( target );
    }

    if ( p()->talent.blood.everlasting_bond.ok() )
    {
      if ( p()->pets.everlasting_bond_pet.active_pet() != nullptr )
      {
        p()->pets.everlasting_bond_pet.active_pet()->ability.vampiric_strike->execute_on_target( target );
      }
    }
  }
};

struct heart_strike_t : public heart_strike_base_t
{
  heart_strike_t( util::string_view n, death_knight_t* p, util::string_view options_str )
    : heart_strike_base_t( n, p, p->talent.blood.heart_strike ), vampiric_strike( nullptr ), vampiric_strike_cost( 0 )
  {
    parse_options( options_str );
    if ( p->talent.sanlayn.vampiric_strike.ok() )
    {
      vampiric_strike      = new vampiric_strike_blood_t( "vampiric_strike", p, false );
      vampiric_strike_cost = p->spell.vampiric_strike->cost( POWER_RUNE );
      add_child( vampiric_strike );
    }
  }

  double cost() const override
  {
    if ( p()->talent.sanlayn.vampiric_strike.ok() && p()->buffs.vampiric_strike->check() )
    {
      return vampiric_strike_cost;
    }
    else
    {
      return base_costs[ RESOURCE_RUNE ];
    }
  }

  void execute() override
  {
    if ( p()->talent.sanlayn.vampiric_strike.ok() && p()->buffs.vampiric_strike->check() )
    {
      vampiric_strike->execute_on_target( target );
      stats->add_execute( 0_ms, target );
      return;
    }
    heart_strike_base_t::execute();

    if ( p()->pets.dancing_rune_weapon_pet.active_pet() != nullptr )
    {
      p()->pets.dancing_rune_weapon_pet.active_pet()->ability.heart_strike->execute_on_target( target );
    }

    if ( p()->talent.blood.everlasting_bond.ok() )
    {
      if ( p()->pets.everlasting_bond_pet.active_pet() != nullptr )
      {
        p()->pets.everlasting_bond_pet.active_pet()->ability.heart_strike->execute_on_target( target );
      }
    }
  }

private:
  vampiric_strike_blood_t* vampiric_strike;
  double vampiric_strike_cost;
};

struct heart_strike_bloodied_blade_t : public death_knight_melee_attack_t
{
  heart_strike_bloodied_blade_t( util::string_view n, death_knight_t* p )
    : death_knight_melee_attack_t( n, p, p->spell.heart_strike_bloodied_blade )
  {
    background = true;
    aoe        = 2;
    weapon     = &( p->main_hand_weapon );

    if ( p->talent.sanlayn.vampiric_strike.ok() )
    {
      vampiric_strike      = new vampiric_strike_blood_t( "vampiric_strike_bloodied_blade", p, true );
      vampiric_strike_cost = p->spell.vampiric_strike->cost( POWER_RUNE );
      add_child( vampiric_strike );
    }
  }

  int n_targets() const override
  {
    return p()->in_death_and_decay() ? aoe + as<int>( p()->talent.cleaving_strikes->effectN( 3 ).base_value() )
                                      : aoe;
  }

  double cost() const override
  {
    if ( p()->talent.sanlayn.vampiric_strike.ok() && p()->buffs.vampiric_strike->check() )
    {
      return vampiric_strike_cost;
    }
    else
      return 0;
  }

  void execute() override
  {
    if ( p()->talent.sanlayn.vampiric_strike.ok() && p()->buffs.vampiric_strike->check() )
    {
      // If we don't have any runes available, we don't want to execute vampiric strike, and we basically just bail out.
      if ( p()->_runes.runes_full() == 0 )
        return;
      vampiric_strike->execute_on_target( target );
      stats->add_execute( 0_ms, target );
      return;
    }
    death_knight_melee_attack_t::execute();
    p()->trigger_sanlayn_execute_talents( false );
  }

  private:
  vampiric_strike_blood_t* vampiric_strike;
  double vampiric_strike_cost;
};

// Horn of Winter ===========================================================

struct horn_of_winter_t final : public death_knight_spell_t
{
  horn_of_winter_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "horn_of_winter", p, p->talent.frost.horn_of_winter )
  {
    parse_options( options_str );
    harmful = false;
    target  = p;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->resource_gain( RESOURCE_RUNIC_POWER, data().effectN( 2 ).resource( RESOURCE_RUNIC_POWER ),
                        p()->gains.horn_of_winter, this );

    p()->replenish_rune( as<unsigned int>( data().effectN( 1 ).base_value() ), p()->gains.horn_of_winter );
  }
};

// Howling Blast ============================================================

struct avalanche_t final : public death_knight_spell_t
{
  avalanche_t( util::string_view name, death_knight_t* p ) : death_knight_spell_t( name, p, p->spell.avalanche_damage )
  {
    aoe        = -1;
    background = true;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    get_td( state->target )->debuff.razorice->trigger();
    p()->procs.razorice_from_avalanche->occur();
  }
};

struct howling_blast_t final : public death_knight_spell_t
{
  howling_blast_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "howling_blast", p, p->talent.frost.howling_blast )
  {
    parse_options( options_str );

    aoe                 = -1;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;

    impact_action = get_action<frost_fever_t>( "frost_fever", p );

    ap_type = attack_power_type::WEAPON_BOTH;
    if ( p->main_hand_weapon.group() == WEAPON_2H )
    {
      ap_type = attack_power_type::WEAPON_MAINHAND;
      // There's a 0.98 modifier hardcoded in the tooltip if a 2H weapon is equipped, probably server side magic
      base_multiplier *= 0.98;
    }

    if ( p->talent.frost.avalanche.ok() )
    {
      avalanche = get_action<avalanche_t>( "avalanche", p );
      add_child( avalanche );
    }
  }

  double runic_power_generation_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::runic_power_generation_multiplier( state );

    if ( p()->buffs.rime->check() )
    {
      m *= 1.0 + p()->buffs.rime->data().effectN( 3 ).percent();
    }

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = death_knight_spell_t::composite_target_multiplier( t );

    if ( p()->talent.frost.icebreaker.ok() && p()->buffs.rime->check() && this->target == t )
    {
      m *= 1.0 + p()->talent.frost.icebreaker->effectN( 1 ).percent();
    }

    return m;
  }

  void schedule_execute( action_state_t* state ) override
  {
    // If we have rime, and no runes left, and rime is expiring on the same ms timestamp as we would howling blast,
    // do not queue howling blast.  This avoids a crash/assert where the buff would get removed during the actions
    // executing, before howling blast goes off.  However, if we do have runes left, we will allow howling blast to
    // get fired off, to more accurately reflect what a player would do
    if ( p()->buffs.rime->check() && p()->_runes.runes_full() == 0 && p()->buffs.rime->remains() == 0_ms )
    {
      sim->print_debug( "{} action={} attempted to schedule howling blast on the same tick rime expires {}", *player,
                        *this, *target );

      if ( state )
      {
        action_state_t::release( state );
      }
      return;
    }
    death_knight_spell_t::schedule_execute( state );
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    if ( p()->buffs.pillar_of_frost->up() && p()->talent.frost.obliteration.ok() )
    {
      p()->trigger_killing_machine( 1.0, p()->procs.km_from_obliteration_hb,
                                    p()->procs.km_from_obliteration_hb_wasted );

      // Obliteration's rune generation
      if ( rng().roll( p()->talent.frost.obliteration->effectN( 2 ).percent() ) )
      {
        p()->replenish_rune( as<int>( p()->spell.obliteration_gains->effectN( 1 ).base_value() ),
                             p()->gains.obliteration );
      }
    }

    if ( p()->talent.frost.avalanche.ok() && p()->buffs.rime->up() )
      avalanche->execute_on_target( target );

    if ( p()->buffs.rime->check() && p()->talent.frost.rage_of_the_frozen_champion.ok() )
    {
      p()->resource_gain( RESOURCE_RUNIC_POWER,
                          p()->spell.rage_of_the_frozen_champion->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
                          p()->gains.rage_of_the_frozen_champion );
    }

    if ( p()->talent.deathbringer.dark_talons.ok() && p()->buffs.rime->check() && p()->buffs.icy_talons->check() &&
         rng().roll( p()->talent.deathbringer.dark_talons->effectN( 1 ).percent() ) )
    {
      p()->buffs.dark_talons_icy_talons->trigger();
    }

    p()->buffs.rime->decrement();
  }

private:
  propagate_const<action_t*> avalanche;
};

// Marrowrend ===============================================================

struct marrowrend_t final : public death_knight_melee_attack_t
{
  marrowrend_t( death_knight_t* p, util::string_view options_str )
    : death_knight_melee_attack_t( "marrowrend", p, p->talent.blood.marrowrend )
  {
    parse_options( options_str );
    weapon = &( p->main_hand_weapon );
  }

  double cost() const override
  {
    double c = death_knight_melee_attack_t::cost();
    if ( p()->buffs.exterminate->up() )
    {
      double m = p()->talent.deathbringer.painful_death.ok()
                     ? p()->buffs.exterminate_painful_death->data().effectN( 2 ).percent()
                     : p()->buffs.exterminate->data().effectN( 2 ).percent();
      c += c * m;
    }

    return c;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( p()->pets.dancing_rune_weapon_pet.active_pet() != nullptr )
    {
      p()->pets.dancing_rune_weapon_pet.active_pet()->ability.marrowrend->execute_on_target( target );
    }

    if ( p()->talent.blood.everlasting_bond.ok() )
    {
      if ( p()->pets.everlasting_bond_pet.active_pet() != nullptr )
      {
        p()->pets.everlasting_bond_pet.active_pet()->ability.marrowrend->execute_on_target( target );
      }
    }

    if ( p()->talent.deathbringer.dark_talons.ok() && p()->buffs.icy_talons->check() &&
         rng().roll( p()->talent.deathbringer.dark_talons->effectN( 1 ).percent() ) )
    {
      p()->buffs.dark_talons_icy_talons->trigger();
    }

    if ( p()->buffs.exterminate->up() )
    {
      p()->buffs.exterminate->expire();
      make_event<delayed_execute_event_t>( *sim, p(), p()->active_spells.exterminate, execute_state->target, 500_ms );
      if ( p()->talent.deathbringer.painful_death->ok() )
      {
        p()->buffs.exterminate_painful_death->trigger();
      }
    }
    else if ( p()->buffs.exterminate_painful_death->up() )
    {
      p()->buffs.exterminate_painful_death->expire();
      make_event<delayed_execute_event_t>( *sim, p(), p()->active_spells.exterminate, execute_state->target, 500_ms );
    }

    if ( p()->buffs.ossified_vitriol->up() )
      p()->buffs.ossified_vitriol->expire();
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    p()->buffs.bone_shield->trigger( as<int>( data().effectN( 3 ).base_value() ) );
    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, TWW1, B4 ) )
        p() -> buffs.piledriver_tww1_4pc->trigger( as<int>( data().effectN( 3 ).base_value() ) );
  }
};

// Mind Freeze ==============================================================

struct mind_freeze_t final : public death_knight_spell_t
{
  mind_freeze_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "mind_freeze", p, p->find_class_spell( "Mind Freeze" ) )
  {
    parse_options( options_str );
    ignore_false_positive = is_interrupt = true;
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p()->talent.coldthirst.ok() )
    {
      p()->resource_gain( RESOURCE_RUNIC_POWER, p()->spell.coldthirst_gain->effectN( 1 ).base_value() / 10,
                          p()->gains.coldthirst, this );

      p()->cooldown.mind_freeze->adjust( p()->spell.coldthirst_gain->effectN( 2 ).time_value() );
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->debuffs.casting || !candidate_target->debuffs.casting->check() )
      return false;

    return death_knight_spell_t::target_ready( candidate_target );
  }
};

// Obliterate ===============================================================

struct obliterate_strike_t final : public death_knight_melee_attack_t
{
  obliterate_strike_t( death_knight_t* p, util::string_view name, weapon_t* w, const spell_data_t* s )
    : death_knight_melee_attack_t( name, p, s )
  {
    background = special = true;
    may_miss             = false;
    weapon               = w;

    // To support Cleaving strieks affecting Obliterate in Dragonflight:
    // - obliterate damage spells have gained a value of 1 in their chain target data
    // - the death and decay buff now has an effect that modifies obliterate's chain target with a value of 0
    // - cleaving strikes increases the aforementionned death and decay buff effect by 1
    cleaving_strikes_targets = data().effectN( 1 ).chain_target() +
                               as<int>( p->spell.dnd_buff->effectN( 4 ).base_value() ) +
                               as<int>( p->talent.cleaving_strikes->effectN( 2 ).base_value() );

    inexorable_assault = get_action<inexorable_assault_damage_t>( "inexorable_assault", p );
  }

  int n_targets() const override
  {
    if ( p()->in_death_and_decay() )
    {
      if ( p()->talent.cleaving_strikes.ok() )
        return cleaving_strikes_targets;
    }

    return death_knight_melee_attack_t::n_targets();
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_melee_attack_t::composite_da_multiplier( state );
    // Obliterate does not list Frozen Heart in it's list of affecting spells.  So mastery does not get applied
    // automatically.
    if ( p()->spec.frostreaper->ok() && p()->buffs.killing_machine->up() )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    const death_knight_td_t* td = get_td( target );
    // Obliterate does not list razorice in it's list of affecting spells, so debuff does not get applied automatically.
    if ( td && p()->spec.frostreaper->ok() && p()->buffs.killing_machine->up() )
    {
      m *= 1.0 + td->debuff.razorice->check_stack_value();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    auto td = get_td( state->target );

    if ( p()->talent.frost.enduring_strength.ok() && p()->buffs.pillar_of_frost->up() &&
         p()->cooldown.enduring_strength_icd->is_ready() && state->result == RESULT_CRIT )
    {
      p()->buffs.enduring_strength_builder->trigger();
      p()->cooldown.enduring_strength_icd->start();
    }

    if ( p()->buffs.inexorable_assault->up() && p()->cooldown.inexorable_assault_icd->is_ready() )
    {
      inexorable_assault->set_target( target );
      inexorable_assault->schedule_execute();
      p()->buffs.inexorable_assault->decrement();
      p()->cooldown.inexorable_assault_icd->start();
    }

    if ( p()->talent.rider.trollbanes_icy_fury.ok() && td->debuff.chains_of_ice_trollbane_slow->check() &&
         p()->pets.trollbane.active_pet() != nullptr )
    {
      td->debuff.chains_of_ice_trollbane_slow->expire();
      p()->active_spells.trollbanes_icy_fury->execute_on_target( state->target );
    }

    if ( p()->talent.rider.whitemanes_famine.ok() && td->dot.undeath->is_ticking() )
    {
      p()->trigger_whitemanes_famine( state->target );
    }
  }

  void execute() override
  {
    if ( !p()->options.split_obliterate_schools && p()->spec.frostreaper->ok() && p()->buffs.killing_machine->up() )
    {
      school = SCHOOL_FROST;
    }

    if ( p()->talent.rider.whitemanes_famine.ok() && p()->sim->target_non_sleeping_list.size() > 1 )
    {
      p()->sort_undeath_targets( target_list() );
    }

    death_knight_melee_attack_t::execute();

    // Improved Killing Machine - revert school after the hit
    if ( !p()->options.split_obliterate_schools )
      school = SCHOOL_PHYSICAL;
  }

private:
  propagate_const<action_t*> inexorable_assault;
  int cleaving_strikes_targets;
};

struct obliterate_t final : public death_knight_melee_attack_t
{
  obliterate_t( death_knight_t* p, util::string_view options_str = {} )
    : death_knight_melee_attack_t( "obliterate", p, p->talent.frost.obliterate ),
      mh( nullptr ),
      oh( nullptr ),
      km_mh( nullptr ),
      km_oh( nullptr ),
      mh_delay( 0_ms ),
      oh_delay( 0_ms ),
      total_delay( 0_ms )
  {
    parse_options( options_str );
    dual = true;

    const spell_data_t* mh_data =
        p->main_hand_weapon.group() == WEAPON_2H ? data().effectN( 4 ).trigger() : data().effectN( 2 ).trigger();

    // Misc_value1 contains the delay in milliseconds for the spells being executed
    mh_delay = timespan_t::from_millis( as<int>( p->main_hand_weapon.group() == WEAPON_2H
                                                     ? data().effectN( 4 ).misc_value1()
                                                     : data().effectN( 2 ).misc_value1() ) );
    if ( p->off_hand_weapon.type != WEAPON_NONE )
    {
      oh_delay = timespan_t::from_millis( as<int>( data().effectN( 3 ).misc_value1() ) );
    }

    // Snag total delay to schedule Killing Machine for after the final hit
    total_delay = mh_delay + oh_delay;

    mh = new obliterate_strike_t( p, "obliterate", &( p->main_hand_weapon ), mh_data );
    add_child( mh );

    if ( p->off_hand_weapon.type != WEAPON_NONE )
    {
      oh = new obliterate_strike_t( p, "obliterate_offhand", &( p->off_hand_weapon ), data().effectN( 3 ).trigger() );
      add_child( oh );
    }
    if ( p->options.split_obliterate_schools && p->spec.frostreaper->ok() )
    {
      km_mh         = new obliterate_strike_t( p, "obliterate_km", &( p->main_hand_weapon ), mh_data );
      km_mh->school = SCHOOL_FROST;
      add_child( km_mh );
      if ( p->off_hand_weapon.type != WEAPON_NONE )
      {
        km_oh         = new obliterate_strike_t( p, "obliterate_offhand_km", &( p->off_hand_weapon ),
                                                 data().effectN( 3 ).trigger() );
        km_oh->school = SCHOOL_FROST;
        add_child( km_oh );
      }
    }

    if ( p->talent.frost.arctic_assault.ok() )
    {
      add_child( get_action<glacial_advance_damage_t>( "glacial_advance_arctic_assault", p, true ) );
    }
  }

  double cost() const override
  {
    double c = death_knight_melee_attack_t::cost();
    if ( p()->buffs.exterminate->up() )
    {
      double m = p()->talent.deathbringer.painful_death.ok()
                     ? p()->buffs.exterminate_painful_death->data().effectN( 2 ).percent()
                     : p()->buffs.exterminate->data().effectN( 2 ).percent();
      c += c * m;
    }

    return c;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( hit_any_target )
    {
      if ( p()->talent.frost.frigid_executioner.ok() && p()->cooldown.frigid_executioner_icd->is_ready() )
      {
        if ( p()->rng().roll( p()->talent.frost.frigid_executioner->proc_chance() ) )
        {
          // # of runes to restore was stored in a secondary affect
          p()->replenish_rune(
              as<unsigned int>(
                  p()->talent.frost.frigid_executioner->effectN( 1 ).trigger()->effectN( 1 ).base_value() ),
              p()->gains.frigid_executioner );
          p()->cooldown.frigid_executioner_icd->start();
        }
      }
      if ( km_mh && p()->buffs.killing_machine->up() )
      {
        make_event<delayed_execute_event_t>( *sim, p(), km_mh, execute_state->target, mh_delay );
        if ( oh && km_oh )
          make_event<delayed_execute_event_t>( *sim, p(), km_oh, execute_state->target, oh_delay );
      }
      else
      {
        make_event<delayed_execute_event_t>( *sim, p(), mh, execute_state->target, mh_delay );
        if ( oh )
          make_event<delayed_execute_event_t>( *sim, p(), oh, execute_state->target, oh_delay );
      }

      p()->buffs.rime->trigger();
    }

    if ( p()->buffs.exterminate->up() )
    {
      p()->buffs.exterminate->expire();
      make_event<delayed_execute_event_t>( *sim, p(), p()->active_spells.exterminate, execute_state->target, 500_ms );
      if ( p()->talent.deathbringer.painful_death->ok() )
      {
        p()->buffs.exterminate_painful_death->trigger();
      }
    }
    else if ( p()->buffs.exterminate_painful_death->up() )
    {
      p()->buffs.exterminate_painful_death->expire();
      make_event<delayed_execute_event_t>( *sim, p(), p()->active_spells.exterminate, execute_state->target, 500_ms );
    }

    if ( p()->buffs.killing_machine->up() )
    {
      p()->consume_killing_machine( p()->procs.killing_machine_oblit, total_delay );
    }
  }

  // Allow on-cast procs
  bool has_amount_result() const override
  {
    return true;
  }

private:
  obliterate_strike_t *mh, *oh, *km_mh, *km_oh;
  timespan_t mh_delay;
  timespan_t oh_delay;
  timespan_t total_delay;
};

// Outbreak ================================================================
struct outbreak_aoe_t final : public death_knight_spell_t
{
  outbreak_aoe_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.outbreak_aoe ),
      vp( get_action<virulent_plague_t>( "virulent_plague", p ) ),
      ff( get_action<frost_fever_t>( "frost_fever", p ) ),
      bp( get_action<blood_plague_t>( "blood_plague", p, true ) ),
      diseases()
  {
    aoe        = -1;
    radius     = data().effectN( 1 ).radius_max();
    background = true;
    if ( p->talent.unholy.superstrain.ok() )
    {
      diseases.push_back( vp );
      diseases.push_back( ff );
      diseases.push_back( bp );
    }
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );
    if ( p()->talent.unholy.superstrain.ok() )
    {
      // Randomize the order the diseases are applied to emulate the random tick order in game.
      // Only really matters for RPPM effects.
      rng().shuffle( diseases.begin(), diseases.end() );
      for ( auto& d : diseases )
      {
        d->execute_on_target( s->target );
      }
    }
    else
    {
      vp->execute_on_target( s->target );
    }
  }

private:
  propagate_const<action_t*> vp;
  propagate_const<action_t*> ff;
  propagate_const<action_t*> bp;
  std::vector<action_t*> diseases;
};

struct outbreak_t final : public death_knight_spell_t
{
  outbreak_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "outbreak", p, p->spec.outbreak )
  {
    parse_options( options_str );
    impact_action = p->active_spells.outbreak_aoe;
  }
};

// Rune of Apocalpyse - Pestilence ==========================================
struct runeforge_apocalypse_pestilence_t final : public death_knight_spell_t
{
  runeforge_apocalypse_pestilence_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.apocalypse_pestilence_damage )
  {
    background = true;
  }
};

// Pillar of Frost ==========================================================
struct frostwhelps_aid_t final : public death_knight_spell_t
{
  frostwhelps_aid_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.frostwhelps_aid_damage )
  {
    aoe        = -1;
    background = true;
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );
    p()->buffs.frostwhelps_aid->trigger();
  }
};

struct pillar_of_frost_t final : public death_knight_spell_t
{
  pillar_of_frost_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "pillar_of_frost", p, p->talent.frost.pillar_of_frost )
  {
    parse_options( options_str );

    harmful = false;
    target  = p;

    if ( p->talent.frost.frostwhelps_aid.ok() )
    {
      whelp = get_action<frostwhelps_aid_t>( "frostwhelps_aid", p );
      add_child( whelp );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    if ( p()->talent.frost.frostwhelps_aid.ok() )
    {
      whelp->execute();
    }

    p()->buffs.pillar_of_frost->trigger();
  }

private:
  propagate_const<action_t*> whelp;
};

// Vile Contagion =============================================================

struct vile_contagion_t final : public death_knight_spell_t
{
  vile_contagion_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "vile_contagion", p, p->talent.unholy.vile_contagion )
  {
    parse_options( options_str );
    aoe = as<int>( p->talent.unholy.vile_contagion->effectN( 1 ).base_value() );
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    death_knight_spell_t::available_targets( tl );

    // Does not hit the main target
    auto it = range::find( tl, target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    return tl.size();
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );

    if ( result_is_hit( s->result ) && get_td( target )->debuff.festering_wound->stack() > 0 )
    {
      unsigned n_stacks = get_td( target )->debuff.festering_wound->stack();

      p()->trigger_festering_wound( s, n_stacks, p()->procs.fw_vile_contagion );
    }
  }
};

// Raise Dead ===============================================================

struct raise_dead_t final : public death_knight_spell_t
{
  raise_dead_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "raise_dead", p,
                            p->talent.unholy.raise_dead.ok() ? p->talent.unholy.raise_dead : p->talent.raise_dead )
  {
    parse_options( options_str );

    harmful = false;
    target  = p;
    p->pets.ghoul_pet.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    p->pets.ghoul_pet.set_event_callback(
        spawner::pet_event_type::PRE_SPAWN,
        [ this ]( spawner::pet_event_type, pets::ghoul_pet_t* p ) { p->precombat_spawn = is_precombat; } );
    if ( p->talent.unholy.all_will_serve.ok() )
    {
      p->pets.risen_skulker.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    // If the action is done in precombat and the pet is permanent
    // Assume that the player used it long enough before pull that the cooldown is ready again
    if ( is_precombat && p()->talent.unholy.raise_dead.ok() )
    {
      cooldown->reset( false );
    }

    // Summon for the duration specified in spelldata if there's one (no data = permanent pet)
    p()->pets.ghoul_pet.spawn( data().duration() );

    if ( p()->talent.unholy.all_will_serve.ok() )
    {
      p()->pets.risen_skulker.spawn();
    }
  }

  bool ready() override
  {
    if ( p()->pets.ghoul_pet.active_pet() != nullptr )
    {
      return false;
    }

    return death_knight_spell_t::ready();
  }
};

// Remorseless Winter =======================================================

struct remorseless_winter_damage_t final : public death_knight_spell_t
{
  remorseless_winter_damage_t( util::string_view n, death_knight_t* p )
    : death_knight_spell_t( n, p, p->spec.remorseless_winter->effectN( 2 ).trigger() ),
      biting_cold_target_threshold( 0 ),
      triggered_biting_cold( false )
  {
    background = true;
    aoe        = -1;

    ap_type = attack_power_type::WEAPON_BOTH;

    if ( p->main_hand_weapon.group() == WEAPON_2H )
    {
      ap_type = attack_power_type::WEAPON_MAINHAND;
      // There's a 0.98 modifier hardcoded in the tooltip if a 2H weapon is equipped, probably server side magic
      base_multiplier *= 0.98;
    }

    if ( p->talent.frost.biting_cold.ok() )
    {
      biting_cold_target_threshold = p->talent.frost.biting_cold->effectN( 1 ).base_value();
    }
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( state->n_targets >= biting_cold_target_threshold && p()->talent.frost.biting_cold.ok() &&
         !triggered_biting_cold )
    {
      p()->buffs.rime->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
      triggered_biting_cold = true;
    }

    if ( p()->talent.frost.everfrost.ok() )
      get_td( state->target )->debuff.everfrost->trigger();
  }

private:
  double biting_cold_target_threshold;

public:
  bool triggered_biting_cold;
};

struct remorseless_winter_t final : public death_knight_spell_t
{
  remorseless_winter_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "remorseless_winter", p, p->spec.remorseless_winter ),
      damage( p->active_spells.remorseless_winter_tick )
  {
    may_miss = may_dodge = may_parry = false;

    parse_options( options_str );

    // Periodic behavior handled by the buff
    dot_duration = base_tick_time = 0_ms;
    if ( p->spec.remorseless_winter->ok() )
    {
      add_child( damage );
    }
    if ( p->talent.frost.cryogenic_chamber.ok() )
    {
      add_child( get_action<cryogenic_chamber_t>( "cryogenic_chamber", p ) );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    debug_cast<remorseless_winter_damage_t*>( damage )->triggered_biting_cold = false;
    p()->buffs.remorseless_winter->trigger();
    if ( p()->talent.frost.cryogenic_chamber.ok() && p()->buffs.cryogenic_chamber->check() )
    {
      p()->buffs.cryogenic_chamber->expire();
    }
  }

private:
  action_t*& damage;
};

// Sacrificial Pact =========================================================

struct sacrificial_pact_damage_t final : public death_knight_spell_t
{
  sacrificial_pact_damage_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.sacrificial_pact_damage )
  {
    background          = true;
    aoe                 = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
  }
};

struct sacrificial_pact_t final : public death_knight_heal_t
{
  sacrificial_pact_t( death_knight_t* p, util::string_view options_str )
    : death_knight_heal_t( "sacrificial_pact", p, p->talent.sacrificial_pact )
  {
    target        = p;
    base_pct_heal = data().effectN( 1 ).percent();
    parse_options( options_str );
    cooldown->duration = p->talent.sacrificial_pact->cooldown();
    damage             = get_action<sacrificial_pact_damage_t>( "sacrificial_pact_damage", p );
  }

  void execute() override
  {
    death_knight_heal_t::execute();

    damage->execute_on_target( execute_state->target );
    p()->pets.ghoul_pet.active_pet()->dismiss();
  }

  bool ready() override
  {
    if ( p()->pets.ghoul_pet.active_pet() == nullptr || p()->pets.ghoul_pet.active_pet()->is_sleeping() )
    {
      return false;
    }

    return death_knight_heal_t::ready();
  }

private:
  propagate_const<action_t*> damage;
};

// Scourge Strike and Clawing Shadows =======================================

struct wound_spender_base_t : public death_knight_melee_attack_t
{
  wound_spender_base_t( util::string_view name, death_knight_t* p, const spell_data_t* spell )
    : death_knight_melee_attack_t( name, p, spell ),
      dnd_cleave_targets( as<int>( p->talent.unholy.scourge_strike->effectN( 4 ).base_value() ) )
  {
    weapon = &( player->main_hand_weapon );
  }

  // The death and decay target cap is displayed both in scourge strike's effects
  // And in SS and CS' max_targets data entry. Using the latter
  int n_targets() const override
  {
    if ( p()->talent.cleaving_strikes.ok() )
      return p()->in_death_and_decay() ? dnd_cleave_targets : 0;
    return 0;
  }

  std::vector<player_t*>& target_list() const override  // smart targeting to targets with wounds when cleaving SS
  {
    std::vector<player_t*>& current_targets = death_knight_melee_attack_t::target_list();
    // Don't bother ordering the list if all the valid targets will be hit
    if ( current_targets.size() <= as<size_t>( n_targets() ) )
      return current_targets;

    // first target, the action target, needs to be left in place
    std::sort( current_targets.begin() + 1, current_targets.end(), [ this ]( player_t* a, player_t* b ) {
      return get_td( a )->debuff.festering_wound->up() && !get_td( b )->debuff.festering_wound->up();
    } );

    return current_targets;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    if ( p()->talent.unholy.reaping.ok() &&
         target->health_percentage() < p()->talent.unholy.reaping->effectN( 2 ).base_value() )
    {
      m *= 1.0 + p()->talent.unholy.reaping->effectN( 1 ).percent();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );
    auto td = get_td( state->target );

    p()->burst_festering_wound( state->target, 1, p()->procs.fw_wound_spender );

    if ( p()->talent.unholy.plaguebringer.ok() )
    {
      p()->buffs.plaguebringer->trigger();
    }

    if ( p()->talent.rider.trollbanes_icy_fury.ok() && td->debuff.chains_of_ice_trollbane_slow->check() &&
         p()->pets.trollbane.active_pet() != nullptr )
    {
      td->debuff.chains_of_ice_trollbane_slow->expire();
      p()->active_spells.trollbanes_icy_fury->execute_on_target( state->target );
    }

    if ( p()->talent.rider.whitemanes_famine.ok() && td->dot.undeath->is_ticking() )
    {
      p()->trigger_whitemanes_famine( state->target );
    }

    if ( p()->talent.sanlayn.incite_terror.ok() )
    {
      td->debuff.incite_terror->trigger();
    }

    if ( p()->talent.sanlayn.infliction_of_sorrow.ok() )
    {
      p()->trigger_infliction_of_sorrow( state->target, this->data().id() == p()->spell.vampiric_strike->id() );
    }
  }

  void execute() override
  {
    if ( p()->talent.rider.whitemanes_famine.ok() && p()->sim->target_non_sleeping_list.size() > 1 )
    {
      p()->sort_undeath_targets( target_list() );
    }
    death_knight_melee_attack_t::execute();

    p()->trigger_sanlayn_execute_talents( this->data().id() == p()->spell.vampiric_strike->id() );
  }

private:
  int dnd_cleave_targets;  // For when in dnd how many targets we can cleave
};

struct vampiric_strike_unholy_t : public wound_spender_base_t
{
  vampiric_strike_unholy_t( util::string_view n, death_knight_t* p )
    : wound_spender_base_t( n, p, p->spell.vampiric_strike )
  {
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
    energize_amount         = std::fabs( data().powerN( 3 ).cost() );
    if ( p->talent.sanlayn.infliction_of_sorrow.ok() )
    {
      add_child( p->active_spells.infliction_of_sorrow );
    }
    if ( p->talent.sanlayn.the_blood_is_life.ok() )
    {
      p->pets.blood_beast.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
      add_child( p->active_spells.the_blood_is_life );
    }
  }
};

struct clawing_shadows_t final : public wound_spender_base_t
{
  clawing_shadows_t( util::string_view n, death_knight_t* p, util::string_view options_str )
    : wound_spender_base_t( n, p, p->talent.unholy.clawing_shadows ),
      vampiric_strike( nullptr ),
      vampiric_strike_cost( 0 )
  {
    parse_options( options_str );
    if ( p->talent.sanlayn.vampiric_strike.ok() )
    {
      vampiric_strike      = new vampiric_strike_unholy_t( "vampiric_strike", p );
      vampiric_strike_cost = p->spell.vampiric_strike->cost( POWER_RUNE );
      add_child( vampiric_strike );
    }
  }

  double cost() const override
  {
    if ( p()->talent.sanlayn.vampiric_strike.ok() && p()->buffs.vampiric_strike->check() )
    {
      return vampiric_strike_cost;
    }
    else
    {
      return base_costs[ RESOURCE_RUNE ];
    }
  }

  void execute() override
  {
    if ( p()->talent.sanlayn.vampiric_strike.ok() && p()->buffs.vampiric_strike->check() )
    {
      vampiric_strike->execute_on_target( target );
      stats->add_execute( 0_ms, target );
      return;
    }
    wound_spender_base_t::execute();
  }

private:
  vampiric_strike_unholy_t* vampiric_strike;
  double vampiric_strike_cost;
};

struct scourge_strike_shadow_t final : public death_knight_melee_attack_t
{
  scourge_strike_shadow_t( util::string_view name, death_knight_t* p )
    : death_knight_melee_attack_t( name, p, p->talent.unholy.scourge_strike->effectN( 3 ).trigger() )
  {
    may_miss = may_parry = may_dodge = false;
    background = proc = dual = true;
    weapon                   = &( player->main_hand_weapon );
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    if ( p()->talent.unholy.reaping.ok() &&
         target->health_percentage() < p()->talent.unholy.reaping->effectN( 2 ).base_value() )
    {
      m *= 1.0 + p()->talent.unholy.reaping->effectN( 1 ).percent();
    }

    return m;
  }
};

struct scourge_strike_t final : public wound_spender_base_t
{
  scourge_strike_t( util::string_view n, death_knight_t* p, util::string_view options_str )
    : wound_spender_base_t( n, p, p->talent.unholy.scourge_strike ),
      vampiric_strike( nullptr ),
      vampiric_strike_cost( 0 )
  {
    parse_options( options_str );
    impact_action = get_action<scourge_strike_shadow_t>( "scourge_strike_shadow", p );
    add_child( impact_action );
    if ( p->talent.unholy.clawing_shadows.ok() )
    {
      background = true;  // Prevent executing this through the APL with Clawing Shadows talented
    }
    if ( p->talent.sanlayn.vampiric_strike.ok() )
    {
      vampiric_strike      = new vampiric_strike_unholy_t( "vampiric_strike", p );
      vampiric_strike_cost = p->spell.vampiric_strike->cost( POWER_RUNE );
      add_child( vampiric_strike );
    }
  }

  double cost() const override
  {
    if ( p()->talent.sanlayn.vampiric_strike.ok() && p()->buffs.vampiric_strike->check() )
    {
      return vampiric_strike_cost;
    }
    else
    {
      return base_costs[ RESOURCE_RUNE ];
    }
  }

  void execute() override
  {
    if ( p()->talent.sanlayn.vampiric_strike.ok() && p()->buffs.vampiric_strike->check() )
    {
      vampiric_strike->execute_on_target( target );
      stats->add_execute( 0_ms, target );
      return;
    }
    wound_spender_base_t::execute();
    p()->trigger_sanlayn_execute_talents( false );
  }

private:
  vampiric_strike_unholy_t* vampiric_strike;
  double vampiric_strike_cost;
};

// Soul Reaper ==============================================================

struct soul_reaper_execute_t : public death_knight_spell_t
{
  soul_reaper_execute_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.soul_reaper_execute )
  {
    background = dual = true;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_spell_t::composite_target_multiplier( target );

    if ( p()->talent.unholy.reaping.ok() &&
         target->health_percentage() < p()->talent.unholy.reaping->effectN( 2 ).base_value() )
    {
      m *= 1.0 + p()->talent.unholy.reaping->effectN( 1 ).percent();
    }

    return m;
  }
};

struct soul_reaper_t : public death_knight_melee_attack_t
{
  soul_reaper_t( util::string_view name, death_knight_t* p, const spell_data_t* data )
    : death_knight_melee_attack_t( name, p, data ),
      soul_reaper_execute( get_action<soul_reaper_execute_t>( name_str + "_execute", p ) )
  {
    if ( p->talent.soul_reaper.ok() || p->talent.deathbringer.grim_reaper.ok() )
    {
      add_child( soul_reaper_execute );
    }

    hasted_ticks = false;
    dot_behavior = DOT_EXTEND;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    if ( p()->talent.unholy.reaping.ok() &&
         target->health_percentage() < p()->talent.unholy.reaping->effectN( 2 ).base_value() )
    {
      m *= 1.0 + p()->talent.unholy.reaping->effectN( 1 ).percent();
    }

    return m;
  }

  void tick( dot_t* dot ) override
  {
    if ( dot->target->health_percentage() < data().effectN( 3 ).base_value() )
      soul_reaper_execute->execute_on_target( dot->target );
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    if ( p()->specialization() == DEATH_KNIGHT_FROST && p()->buffs.pillar_of_frost->up() &&
         p()->talent.frost.obliteration.ok() )
    {
      p()->trigger_killing_machine( 1.0, p()->procs.km_from_obliteration_sr,
                                    p()->procs.km_from_obliteration_sr_wasted );

      // Obliteration's rune generation
      if ( rng().roll( p()->talent.frost.obliteration->effectN( 2 ).percent() ) )
      {
        p()->replenish_rune( as<int>( p()->spell.obliteration_gains->effectN( 1 ).base_value() ),
                             p()->gains.obliteration );
      }
    }
  }

private:
  propagate_const<action_t*> soul_reaper_execute;
};

struct soul_reaper_action_t final : public soul_reaper_t
{
  soul_reaper_action_t( util::string_view n, death_knight_t* p, util::string_view options_str )
    : soul_reaper_t( n, p, p->talent.soul_reaper )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    soul_reaper_t::execute();
    if ( p()->specialization() == DEATH_KNIGHT_BLOOD )
    {
      if ( p()->pets.dancing_rune_weapon_pet.active_pet() != nullptr )
      {
        p()->pets.dancing_rune_weapon_pet.active_pet()->ability.soul_reaper->execute_on_target( execute_state->target );
      }

      if ( p()->talent.blood.everlasting_bond.ok() )
      {
        if ( p()->pets.everlasting_bond_pet.active_pet() != nullptr )
        {
          p()->pets.everlasting_bond_pet.active_pet()->ability.soul_reaper->execute_on_target( execute_state->target );
        }
      }
    }
  }
};

struct grim_reaper_soul_reaper_t final : public soul_reaper_t
{
  grim_reaper_soul_reaper_t( util::string_view name, death_knight_t* p )
    : soul_reaper_t( name, p, p->spell.grim_reaper_soul_reaper )
  {
    background                         = true;
    base_costs[ RESOURCE_RUNIC_POWER ] = 0;
    base_costs[ RESOURCE_RUNE ]        = 0;
    trigger_gcd                        = 0_ms;
    cooldown->duration                 = 0_ms;
    energize_amount                    = 0;
  }

  void execute() override
  {
    soul_reaper_t::execute();
    if ( p()->specialization() == DEATH_KNIGHT_BLOOD )
    {
      if ( p()->pets.dancing_rune_weapon_pet.active_pet() != nullptr )
      {
        p()->pets.dancing_rune_weapon_pet.active_pet()->ability.grim_reaper_soul_reaper->execute_on_target( execute_state->target );
      }

      if ( p()->talent.blood.everlasting_bond.ok() )
      {
        if ( p()->pets.everlasting_bond_pet.active_pet() != nullptr )
        {
          p()->pets.everlasting_bond_pet.active_pet()->ability.grim_reaper_soul_reaper->execute_on_target( execute_state->target );
        }
      }
    }
  }
};

// Summon Gargoyle ==========================================================

struct summon_gargoyle_t final : public death_knight_spell_t
{
  summon_gargoyle_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "summon_gargoyle", p, p->talent.unholy.summon_gargoyle )
  {
    parse_options( options_str );
    harmful = false;
    target  = p;
    p->pets.gargoyle.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->pets.gargoyle.spawn();
  }
};

// Tombstone ================================================================
// Not with Defensive Abilities because of the reliable RP generation

struct tombstone_t final : public death_knight_spell_t
{
  tombstone_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "tombstone", p, p->talent.blood.tombstone )
  {
    parse_options( options_str );

    harmful = may_crit = false;
    target             = p;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    int charges = std::min( p()->buffs.bone_shield->check(), as<int>( data().effectN( 5 ).base_value() ) );

    double power  = charges * data().effectN( 3 ).base_value();
    double shield = charges * data().effectN( 4 ).percent();

    p()->resource_gain( RESOURCE_RUNIC_POWER, power, p()->gains.tombstone, this );
    p()->buffs.tombstone->trigger( 1, shield * p()->resources.max[ RESOURCE_HEALTH ] );
    p()->buffs.bone_shield->decrement( charges );

    if( p() -> talent.blood.ossified_vitriol -> ok() )
      p()->buffs.ossified_vitriol->trigger( charges );

    if ( p()->talent.blood.insatiable_blade->ok() )
      p()->cooldown.dancing_rune_weapon->adjust( p()->talent.blood.insatiable_blade->effectN( 1 ).time_value() * charges );

    if ( p()->talent.blood.blood_tap.ok() )
    {
      p()->cooldown.blood_tap->adjust(
          -1.0 * timespan_t::from_seconds( p()->talent.blood.blood_tap->effectN( 2 ).base_value() ) * charges );
    }

    if ( charges > 0 && p()->talent.blood.shattering_bone.ok() )
    {
      // Set the number of charges of BS consumed, as it's used as a multiplier in shattering bone
      debug_cast<shattering_bone_t*>( p()->active_spells.shattering_bone )->boneshield_charges_consumed = charges;
      p()->active_spells.shattering_bone->execute_on_target( target );
    }
  }

  bool ready() override
  {
    if ( !p()->buffs.bone_shield->check() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Unholy Assault ============================================================

struct unholy_assault_t final : public death_knight_melee_attack_t
{
  unholy_assault_t( death_knight_t* p, util::string_view options_str )
    : death_knight_melee_attack_t( "unholy_assault", p, p->talent.unholy.unholy_assault )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    p()->buffs.unholy_assault->trigger();
    death_knight_melee_attack_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    p()->trigger_festering_wound( s, as<int>( p()->talent.unholy.unholy_assault->effectN( 3 ).base_value() ),
                                  p()->procs.fw_unholy_assault );
  }
};

// ==========================================================================
// Death Knight Defensive Abilities
// ==========================================================================

// Anti-Magic Shell ========================================================
struct antimagic_shell_t final : public death_knight_spell_t
{
  antimagic_shell_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "antimagic_shell", p, p->spec.antimagic_shell ),
      min_interval( 60 ),
      interval( 60 ),
      interval_stddev( 0.05 ),
      interval_stddev_opt( 0 ),
      damage( p->options.ams_absorb_percent )
  {
    harmful = may_crit = may_miss = false;
    base_dd_min = base_dd_max = 0;
    target                    = p;
    parse_options( options_str );

    min_interval += p->talent.antimagic_barrier->effectN( 1 ).base_value() / 1000;
    interval += p->talent.antimagic_barrier->effectN( 1 ).base_value() / 1000;

    // Don't allow lower than AMS cd intervals
    if ( interval < min_interval )
    {
      sim->errorf( "%s minimum interval for Anti-Magic Shell is %f seconds.", player->name(), min_interval );
      interval = min_interval;
    }

    // Less than a second standard deviation is translated to a percent of
    // interval
    if ( interval_stddev_opt < 1 )
      interval_stddev = interval * interval_stddev_opt;
    // >= 1 seconds is used as a standard deviation normally
    else
      interval_stddev = interval_stddev_opt;
  }

  void execute() override
  {
    if ( damage > 0 )
    {
      timespan_t new_cd =
          timespan_t::from_seconds( std::max( min_interval, rng().gauss( interval, interval_stddev ) ) );
      cooldown->duration = new_cd;
    }

    death_knight_spell_t::execute();

    p()->buffs.antimagic_shell->trigger();
  }

private:
  double min_interval;
  double interval;
  double interval_stddev;
  double interval_stddev_opt;
  double damage;
};

// Anti-Magic Zone =========================================================
struct antimagic_zone_t final : public death_knight_spell_t
{
  antimagic_zone_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "antimagic_zone", p, p->talent.antimagic_zone )
  {
    harmful = may_crit = may_miss = false;
    base_dd_min = base_dd_max = 0;
    target                    = p;
    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->buffs.antimagic_zone->trigger();
  }
};

// Icebound Fortitude =======================================================

struct icebound_fortitude_t final : public death_knight_spell_t
{
  icebound_fortitude_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "icebound_fortitude", p, p->talent.icebound_fortitude )
  {
    parse_options( options_str );
    harmful = false;
    target  = p;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->buffs.icebound_fortitude->trigger();
  }
};

// Mark of Blood ============================================================

struct mark_of_blood_heal_t final : public death_knight_heal_t
{
  mark_of_blood_heal_t( util::string_view n, death_knight_t* p )
    :  // The data is removed and switched to the talent spell on PTR 8.3.0.32805
      death_knight_heal_t( n, p, p->talent.blood.mark_of_blood )
  {
    may_crit = callbacks = false;
    background = dual = true;
    target            = p;
    base_pct_heal     = data().effectN( 1 ).percent();
  }
};

struct mark_of_blood_t final : public death_knight_spell_t
{
  mark_of_blood_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "mark_of_blood", p, p->talent.blood.mark_of_blood )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    get_td( target )->debuff.mark_of_blood->trigger();
  }
};

// Rune Tap =================================================================

struct rune_tap_t final : public death_knight_spell_t
{
  rune_tap_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "rune_tap", p, p->talent.blood.rune_tap )
  {
    parse_options( options_str );
    use_off_gcd = true;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->buffs.rune_tap->trigger();
  }
};

// Vampiric Blood ===========================================================
struct vampiric_blood_t final : public death_knight_spell_t
{
  vampiric_blood_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "vampiric_blood", p, p->talent.blood.vampiric_blood )
  {
    parse_options( options_str );

    harmful     = false;
    target      = p;
    base_dd_min = base_dd_max = 0;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->buffs.vampiric_blood->trigger();
  }
};

}  // UNNAMED NAMESPACE

// Runeforges ===============================================================

void runeforge::fallen_crusader( special_effect_t& effect )
{
  struct fallen_crusader_heal_t final : public death_knight_heal_t
  {
    fallen_crusader_heal_t( util::string_view name, death_knight_t* p, const spell_data_t* data )
      : death_knight_heal_t( name, p, data )
    {
      background = true;
      target     = player;
      callbacks = may_crit = false;
      base_pct_heal        = data->effectN( 2 ).percent();
      base_pct_heal *= 1.0 + p->talent.unholy_bond->effectN( 2 ).percent();
    }

    // Procs by default target the target of the action that procced them.
    void execute() override
    {
      target = player;

      death_knight_heal_t::execute();
    }
  };

  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* p = debug_cast<death_knight_t*>( effect.player );

  // Create unholy strength heal if necessary, buff is always created for APL support
  p->runeforge.rune_of_the_fallen_crusader = true;

  effect.custom_buff = p->buffs.unholy_strength;
  effect.execute_action =
      get_action<fallen_crusader_heal_t>( "unholy_strength", p, effect.driver()->effectN( 1 ).trigger() );

  new dbc_proc_callback_t( effect.player, effect );
}

void runeforge::razorice( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* p = debug_cast<death_knight_t*>( effect.player );

  if ( !p->active_spells.runeforge_razorice )
    p->active_spells.runeforge_razorice = get_action<razorice_attack_t>( "razorice", p );

  // Store in which hand razorice is equipped, as it affects which abilities proc it
  switch ( effect.item->slot )
  {
    case SLOT_MAIN_HAND:
      p->runeforge.rune_of_razorice_mh = true;
      break;
    case SLOT_OFF_HAND:
      p->runeforge.rune_of_razorice_oh = true;
      break;
    default:
      break;
  }
}

void runeforge::stoneskin_gargoyle( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* p = debug_cast<death_knight_t*>( effect.player );

  p->runeforge.rune_of_the_stoneskin_gargoyle = true;

  p->buffs.stoneskin_gargoyle = make_buff( p, "stoneskin_gargoyle", effect.driver() )
                                    ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE );

  // Change the player's base armor multiplier
  double am = effect.driver()->effectN( 1 ).percent();
  am *= 1.0 + p->talent.unholy_bond->effectN( 2 ).percent();
  p->base.armor_multiplier *= 1.0 + am;

  // This buff can only be applied on a 2H weapon, stacking mechanic is unknown territory

  // The buff isn't shown ingame, leave it visible in the sim for clarity
  // p -> quiet = true;
}

void runeforge::apocalypse( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* p = debug_cast<death_knight_t*>( effect.player );
  // Nothing happens if the runeforge is applied on both weapons
  if ( p->runeforge.rune_of_apocalypse )
    return;

  p->spell.apocalypse_death_debuff      = p->find_spell( 327095 );
  p->spell.apocalypse_famine_debuff     = p->find_spell( 327092 );
  p->spell.apocalypse_war_debuff        = p->find_spell( 327096 );
  p->spell.apocalypse_pestilence_damage = p->find_spell( 327093 );
  // Triggering the effects is handled in pet_melee_attack_t::impact()
  p->runeforge.rune_of_apocalypse = true;
  // Even though a pet procs it, the damage from Pestilence belongs directly to the player in logs
  p->active_spells.runeforge_pestilence = get_action<runeforge_apocalypse_pestilence_t>( "pestilence", p );
}

void runeforge::sanguination( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* p = debug_cast<death_knight_t*>( effect.player );
  // This runeforge doesn't stack
  if ( p->runeforge.rune_of_sanguination )
    return;

  p->spell.sanguination_cooldown = p->find_spell( 326809 );

  struct sanguination_heal_t final : public death_knight_heal_t
  {
    sanguination_heal_t( special_effect_t& effect )
      : death_knight_heal_t( "rune_of_sanguination", debug_cast<death_knight_t*>( effect.player ),
                             effect.driver()->effectN( 1 ).trigger() ),
        health_threshold( effect.driver()->effectN( 1 ).base_value() )
    {
      background    = true;
      tick_pct_heal = data().effectN( 1 ).percent();
      tick_pct_heal *= 1.0 + p()->talent.unholy_bond->effectN( 1 ).percent();
      // Sated-type debuff, for simplicity the debuff's duration is used as a simple cooldown in simc
      cooldown->duration = p()->spell.sanguination_cooldown->duration();
    }

    bool ready() override
    {
      if ( p()->health_percentage() > health_threshold )
        return false;

      return death_knight_heal_t::ready();
    }

  private:
    double health_threshold;
  };

  p->runeforge.rune_of_sanguination = true;

  p->active_spells.runeforge_sanguination = new sanguination_heal_t( effect );
}

void runeforge::spellwarding( special_effect_t& effect )
{
  struct spellwarding_absorb_t final : public absorb_t
  {
    spellwarding_absorb_t( util::string_view name, death_knight_t* p, const spell_data_t* data )
      : absorb_t( name, p, data ), health_percentage( p->spell.spellwarding_absorb->effectN( 2 ).percent() )
    // The absorb amount is hardcoded in the effect tooltip, the only data is in the runeforging action spell
    {
      target     = p;
      background = true;
      harmful    = false;
    }

    void execute() override
    {
      base_dd_min = base_dd_max = health_percentage * player->resources.max[ RESOURCE_HEALTH ];

      absorb_t::execute();
    }

  private:
    double health_percentage;
  };

  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* p = debug_cast<death_knight_t*>( effect.player );

  p->spell.spellwarding_absorb = p->find_spell( 326855 );

  // Stacking the rune doubles the damage reduction, and seems to create a second proc
  p->runeforge.rune_of_spellwarding += effect.driver()->effectN( 2 ).percent();
  effect.execute_action =
      get_action<spellwarding_absorb_t>( "rune_of_spellwarding", p, effect.driver()->effectN( 1 ).trigger() );

  new dbc_proc_callback_t( effect.player, effect );
}

// NYI
void runeforge::unending_thirst( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  // Placeholder for APL tracking purpose, effect NYI
  debug_cast<death_knight_t*>( effect.player )->runeforge.rune_of_unending_thirst = true;
}

void runeforge::hysteria( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* p = debug_cast<death_knight_t*>( effect.player );

  if ( !p->runeforge.rune_of_hysteria )
  {
    p->runeforge.rune_of_hysteria = true;
  }

  // The RP cap increase stacks
  p->resources.base[ RESOURCE_RUNIC_POWER ] += effect.driver()->effectN( 2 ).resource( RESOURCE_RUNIC_POWER ) *
                                               ( 1.0 + p->talent.unholy_bond->effectN( 2 ).percent() );

  // The buff doesn't stack and doesn't have an increased effect
  // but the proc rate is increased and it has been observed to proc twice on the same damage event (2020-08-23)
  effect.custom_buff = p->buffs.rune_of_hysteria;

  new dbc_proc_callback_t( effect.player, effect );
}

// Resource Manipulation ====================================================

double death_knight_t::resource_gain( resource_e resource_type, double amount, gain_t* g, action_t* action )
{
  double actual_amount = player_t::resource_gain( resource_type, amount, g, action );

  if ( runeforge.rune_of_hysteria && resource_type == RESOURCE_RUNIC_POWER )
  {
    // Unholy Bond is implemented in the buff, so no need to boost it here.
    double bonus_rp = amount * buffs.rune_of_hysteria->value();
    actual_amount += player_t::resource_gain( resource_type, bonus_rp, gains.rune_of_hysteria, action );
  }

  return actual_amount;
}

double death_knight_t::resource_loss( resource_e resource_type, double amount, gain_t* g, action_t* action )
{
  double actual_amount = player_t::resource_loss( resource_type, amount, g, action );

  if ( resource_type == RESOURCE_RUNE )
  {
    _runes.consume( as<int>( amount ) );
    // Ensure rune state is consistent with the actor resource state for runes
    assert( _runes.runes_full() == resources.current[ RESOURCE_RUNE ] );
  }

  // Procs from runes spent
  if ( resource_type == RESOURCE_RUNE && action )
  {
    // Gathering Storm triggers a stack and extends RW duration by 0.5s
    // for each spell cast that normally consumes a rune (even if it ended up free)
    // But it doesn't count the original Relentless Winter cast
    if ( talent.frost.gathering_storm.ok() && buffs.remorseless_winter->check() &&
         action->data().id() != spec.remorseless_winter->id() )
    {
      unsigned consumed = static_cast<unsigned>( action->base_costs[ RESOURCE_RUNE ] );
      buffs.gathering_storm->trigger( consumed );
      timespan_t base_extension =
          timespan_t::from_seconds( talent.frost.gathering_storm->effectN( 1 ).base_value() / 10.0 );
      buffs.remorseless_winter->extend_duration( this, base_extension * consumed );
    }

    if ( talent.rune_mastery.ok() )
    {
      buffs.rune_mastery->trigger();
    }

    // Proc Chance does not appear to be in data, using testing data that is current as of 4/19/2024
    if ( talent.rider.riders_champion.ok() && rng().roll( 0.2 ) )
    {
      summon_rider( spell.summon_whitemane_2->duration(), true );
    }

    if ( talent.rider.nazgrims_conquest.ok() && buffs.apocalyptic_conquest->check() )
    {
      debug_cast<buffs::apocalyptic_conquest_buff_t*>( buffs.apocalyptic_conquest )->nazgrims_conquest +=
          as<int>( amount );
      invalidate_cache( CACHE_STRENGTH );
    }

    // Effects that require the player to actually spend runes
    if ( actual_amount > 0 )
    {
    }
  }

  // Procs from RP spent
  if ( resource_type == RESOURCE_RUNIC_POWER && action )
  {
    // 2019-02-12: Runic Empowerment, Runic Corruption, Red Thirst and gargoyle's shadow empowerment
    // seem to be using the base cost of the ability rather than the last resource cost
    // Bug? Intended?
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/396
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/397
    // Some abilities use the actual RP spent by the ability, others use the base RP cost
    double base_rp_cost = actual_amount;

    // If an action is linked, fetch its base cost. Exclude Bonestorm from this otherwise it uses the base cost for
    // Insatiable Hunger instead of the actual rp spent
    if ( action && action->id != 194844 )
      base_rp_cost = action->base_costs[ RESOURCE_RUNIC_POWER ];

    // 2020-12-16 - Melekus: Based on testing with both Frost Strike and Breath of Sindragosa during Hypothermic
    // Presence, RE is using the ability's base cost for its proc chance calculation, just like Runic Corruption
    trigger_runic_empowerment( base_rp_cost );
    trigger_runic_corruption( procs.rp_runic_corruption, base_rp_cost, false );

    if ( talent.unholy.summon_gargoyle.ok() )
    {
      // 2019-02-12 Death Strike buffs gargoyle as if it had its full cost, even though it received a -10rp cost in 8.1
      // Assuming it's a bug for now, using base_costs instead of last_resource_cost won't affect free spells here
      // Free Death Coils are still handled in the action
      for ( auto& gargoyle : pets.gargoyle )
      {
        gargoyle->increase_power( base_rp_cost );
      }
    }

    if ( talent.icy_talons.ok() )
    {
      buffs.icy_talons->trigger();
    }

    if ( talent.frost.unleashed_frenzy.ok() )
    {
      buffs.unleashed_frenzy->trigger();
    }

    if ( talent.rider.fury_of_the_horsemen.ok() )
    {
      if ( pets.whitemane.active_pet() != nullptr )
        extend_rider( amount, pets.whitemane.active_pet() );
      if ( pets.mograine.active_pet() != nullptr )
        extend_rider( amount, pets.mograine.active_pet() );
      if ( pets.nazgrim.active_pet() != nullptr )
        extend_rider( amount, pets.nazgrim.active_pet() );
      if ( pets.trollbane.active_pet() != nullptr )
        extend_rider( amount, pets.trollbane.active_pet() );
    }

    // Effects that only trigger if resources were spent
    if ( actual_amount > 0 )
    {
      auto final_spend = actual_amount;
      // If we are using actual amount for things, and the triggering action was death strike, the talent
      // improved death strike is not counted toward actual spent
      if ( action->id == 49998 )
      {
        if ( specialization() == DEATH_KNIGHT_BLOOD )
          final_spend -= talent.improved_death_strike->effectN( 5 ).resource( RESOURCE_RUNIC_POWER );
        else
          final_spend -= talent.improved_death_strike->effectN( 3 ).resource( RESOURCE_RUNIC_POWER );
      }

      if ( talent.blood.red_thirst.ok() )
      {
        timespan_t sec = talent.blood.red_thirst->effectN( 1 ).time_value() * final_spend /
                         talent.blood.red_thirst->effectN( 2 ).base_value();
        cooldown.vampiric_blood->adjust( -sec );
      }
    }
  }

  return actual_amount;
}

// death_knight_t::create_options ===========================================

void death_knight_t::create_options()
{
  player_t::create_options();

  add_option( opt_bool( "deathknight.disable_aotd", options.disable_aotd ) );
  add_option( opt_bool( "deathknight.split_ghoul_regen", options.split_ghoul_regen ) );
  add_option( opt_bool( "deathknight.split_obliterate_schools", options.split_obliterate_schools ) );
  add_option( opt_float( "deathknight.ams_absorb_percent", options.ams_absorb_percent, 0.0, 1.0 ) );
  add_option( opt_bool( "deathknight.individual_pet_reporting", options.individual_pet_reporting ) );
  add_option( opt_float( "deathknight.average_cs_travel_time", options.average_cs_travel_time, 0.0, 5.0 ) );
  add_option(
      opt_timespan( "deathknight.first_ams_cast", options.first_ams_cast, timespan_t::zero(), timespan_t::max() ) );
  add_option( opt_float( "deathknight.horsemen_ams_absorb_percent", options.horsemen_ams_absorb_percent, 0.0, 1.0 ) );
}

void death_knight_t::copy_from( player_t* source )
{
  player_t::copy_from( source );
  options = debug_cast<death_knight_t*>( source )->options;
}

void death_knight_t::merge( player_t& other )
{
  player_t::merge( other );

  death_knight_t& dk = dynamic_cast<death_knight_t&>( other );

  _runes.rune_waste.merge( dk._runes.rune_waste );
  _runes.cumulative_waste.merge( dk._runes.cumulative_waste );
}

std::string death_knight_t::create_profile( save_e type )
{
  std::string term;
  std::string profile_str = player_t::create_profile( type );
  term                    = "\n";
  if ( type & SAVE_PLAYER )
  {
    if ( options.ams_absorb_percent > 0 )
    {
      profile_str += "deathknight.ams_absorb_percent=" + util::to_string( options.ams_absorb_percent ) + term;
    }
    if ( options.first_ams_cast != 20_s )
    {
      profile_str += "deathknight.first_ams_cast=" + util::to_string( options.first_ams_cast.total_seconds() ) + term;
    }
  }
  return profile_str;
}

// death_knight_t::datacollection_begin ===========================================

void death_knight_t::datacollection_begin()
{
  player_t::datacollection_begin();
}

// death_knight_t::datacollection_end =============================================

void death_knight_t::datacollection_end()
{
  player_t::datacollection_end();
}

void death_knight_t::analyze( sim_t& s )
{
  player_t::analyze( s );

  _runes.rune_waste.analyze();
  _runes.cumulative_waste.analyze();
}

void death_knight_t::trigger_soul_reaper_death( player_t* target )
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( sim->event_mgr.canceled )
  {
    return;
  }

  if ( !talent.soul_reaper.ok() )
  {
    return;
  }

  if ( get_target_data( target )->dot.soul_reaper->is_ticking() )
  {
    sim->print_log( "Target {} died while affected by Soul Reaper, player {} gains Runic Corruption buff.",
                    target->name(), name() );

    trigger_runic_corruption( procs.sr_runic_corruption, 0, 1.0, true );
    return;
  }
}

void death_knight_t::trigger_festering_wound_death( player_t* target )
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( sim->event_mgr.canceled || !spec.festering_wound->ok() )
  {
    return;
  }

  const death_knight_td_t* td = get_target_data( target );
  if ( !td )
    return;

  int n_wounds = td->debuff.festering_wound->check();

  // If the target wasn't affected by festering wound, return
  if ( n_wounds == 0 )
    return;

  // Generate the RP you'd have gained if you popped the wounds manually
  resource_gain( RESOURCE_RUNIC_POWER,
                 spec.festering_wound->effectN( 1 ).trigger()->effectN( 2 ).resource( RESOURCE_RUNIC_POWER ) * n_wounds,
                 gains.festering_wound );

  for ( int i = 0; i < n_wounds; i++ )
  {
    procs.fw_death->occur();
  }

  trigger_bursting_sores( this, n_wounds );

  if ( talent.unholy.festermight.ok() )
  {
    buffs.festermight->trigger( n_wounds );
  }

  if ( talent.unholy.festering_scythe.ok() )
  {
    if ( !buffs.festering_scythe->check() )
    {
      buffs.festering_scythe_stacks->trigger( n_wounds );
    }
  }
}

void death_knight_t::trigger_virulent_plague_death( player_t* target )
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( sim->event_mgr.canceled )
  {
    return;
  }

  if ( !get_target_data( target )->dot.virulent_plague->is_ticking() )
  {
    return;
  }

  // Schedule an execute for Virulent Eruption
  active_spells.virulent_eruption->schedule_execute();
}

bool death_knight_t::in_death_and_decay() const
{
  if ( ( talent.rider.mograines_might.ok() && buffs.mograines_might->check() ) || buffs.death_and_decay->check() )
  {
    return true;
  }

  if ( !sim->distance_targeting_enabled || !active_dnd )
    return active_dnd != nullptr;

  return get_ground_aoe_distance( *active_dnd->pulse_state ) <= active_dnd->pulse_state->action->radius;
}

unsigned death_knight_t::replenish_rune( unsigned n, gain_t* gain )
{
  unsigned replenished = 0;

  while ( n-- )
  {
    rune_t* rune = _runes.first_depleted_rune();
    if ( !rune )
    {
      rune = _runes.first_regenerating_rune();
    }

    if ( !rune && gain )
    {
      gain->add( RESOURCE_RUNE, 0, 1 );
    }
    else if ( rune )
    {
      rune->fill_rune( gain );
      ++replenished;
    }
  }

  // Ensure internal state is consistent with the actor and runees
  assert( _runes.runes_full() == resources.current[ RESOURCE_RUNE ] );

  return replenished;
}

// Helper function to trigger Killing Machine, whether it's from a "forced" proc, a % chance, or the regular rppm proc
void death_knight_t::trigger_killing_machine( double chance, proc_t* proc, proc_t* wasted_proc )
{
  bool triggered = false;
  bool wasted    = buffs.killing_machine->at_max_stacks();
  // If the given chance is 0, use the auto attack proc mechanic (new system, or rppm if the option is selected)
  // Melekus, 2020-06-03: It appears that Killing Machine now procs from auto attacks following a custom system
  // Every critical auto attack has a 30% * number of missed proc attempts to trigger Killing Machine
  // Originally found by Bicepspump, made public on 2020-05-17
  // This may have been added to the game on patch 8.2, when rppm data from Killing Machine was removed from the game
  // 2022-01-29 During 9.2 beta cycle, it was noticed that during most, if not all of shadowlands, the KM forumula
  // for 1h weapons was incorrect.  This new version seems to match testing done by Bicepspump, via wcl log pull.
  if ( chance == 0 )
  {
    // If we are using a 1H, km_proc_attempts*0.3
    // with 2H it looks to be km_proc_attempts*1.0 through testing
    double km_proc_chance = 0.13;
    if ( spec.might_of_the_frozen_wastes->ok() && main_hand_weapon.group() == WEAPON_2H )
    {
      km_proc_chance = 1.0;
    }
    else
    {
      km_proc_chance = ++km_proc_attempts * 0.3;
    }

    if ( rng().roll( km_proc_chance ) )
    {
      triggered        = true;
      km_proc_attempts = 0;
    }
  }
  // Else, use RNG
  else
  {
    triggered = rng().roll( chance );
  }

  // If Killing Machine didn't proc, there's no waste and nothing else to do
  if ( !triggered )
  {
    return;
  }

  buffs.killing_machine->trigger();

  // If the proc is guaranteed, allow the player to instantly react to it
  if ( chance == 1.0 )
  {
    buffs.killing_machine->predict();
  }

  // Use procs to track sources and wastes
  if ( !wasted )
  {
    proc->occur();
  }
  else
  {
    wasted_proc->occur();
  }
}

void death_knight_t::consume_killing_machine( proc_t* proc, timespan_t total_delay )
{
  if ( !buffs.killing_machine->up() )
  {
    return;
  }

  proc->occur();

  // Killing Machine Consumption appears to be delayed by 20ms after the final consumption action was executed
  make_event( sim, total_delay + 20_ms, [ this ] {
    buffs.killing_machine->decrement();

    if ( rng().roll( talent.frost.murderous_efficiency->effectN( 1 ).percent() ) )
    {
      replenish_rune( as<int>( spell.murderous_efficiency_gain->effectN( 1 ).base_value() ),
                      gains.murderous_efficiency );
    }

    if ( talent.frost.bonegrinder.ok() && !buffs.bonegrinder_frost->up() )
    {
      buffs.bonegrinder_crit->trigger();
      if ( buffs.bonegrinder_crit->at_max_stacks() )
      {
        buffs.bonegrinder_frost->trigger();
        buffs.bonegrinder_crit->expire();
      }
    }

    if ( talent.frost.arctic_assault.ok() )
    {
      get_action<glacial_advance_damage_t>( "glacial_advance_arctic_assault", this, true )->execute();
    }

    if ( talent.frost.frostscythe.ok() )
    {
      cooldown.frostscythe->adjust( timespan_t::from_millis( -talent.frost.frostscythe->effectN( 1 ).base_value() ) );
    }

    if ( talent.deathbringer.dark_talons.ok() && buffs.icy_talons->check() &&
         rng().roll( talent.deathbringer.dark_talons->effectN( 1 ).percent() ) )
    {
      buffs.dark_talons_icy_talons->trigger();
    }
  } );
}

void death_knight_t::trigger_runic_empowerment( double rpcost )
{
  if ( !spec.frost_death_knight->ok() )
    return;

  double base_chance = spell.runic_empowerment_chance->effectN( 1 ).percent() / 10.0;

  if ( !rng().roll( base_chance * rpcost ) )
    return;

  int regenerated = replenish_rune( as<int>( spell.runic_empowerment_gain->effectN( 1 ).resource( RESOURCE_RUNE ) ),
                                    gains.runic_empowerment );

  sim->print_debug( "{} Runic Empowerment regenerated {} rune", name(), regenerated );
  log_rune_status( this );
}

void death_knight_t::trigger_runic_corruption( proc_t* proc, double rpcost, double override_chance, bool death_trigger )
{
  if ( !spec.unholy_death_knight->ok() && death_trigger == false )
    return;
  double proc_chance = 0.0;

  // Use the overriden chance if there's one and RP cost is 0
  proc_chance = ( !rpcost && override_chance != -1.0 )
                    ? override_chance
                    :
                    // Else, use the general proc chance ( 1.6 per RP * RP / 100 as of patch 9.0.2 )
                    spell.runic_corruption_chance->effectN( 1 ).percent() * rpcost / 100.0;
  // Buff duration and refresh behavior handled in runic_corruption_buff_t
  if ( buffs.runic_corruption->trigger( 1, buff_t::DEFAULT_VALUE(), proc_chance ) && proc )
    proc->occur();
}

void death_knight_t::trigger_festering_wound( const action_state_t* state, unsigned n, proc_t* proc )
{
  if ( !state->action->result_is_hit( state->result ) )
  {
    return;
  }

  get_target_data( state->target )->debuff.festering_wound->trigger( n );
  while ( n-- > 0 )
  {
    proc->occur();
    active_spells.festering_wound_application->execute_on_target( state->target );
  }
}

void death_knight_t::trigger_bursting_sores( player_t* target, unsigned n )
{
  struct bs_event_t : public event_t
  {
    unsigned n;
    player_t* t;
    player_t* dk;

    bs_event_t( player_t* p, player_t* target, unsigned n ) : event_t( *p, 0_ms ), n( n ), t( target ), dk( p )
    {
    }

    const char* name() const override
    {
      return "bursting_sores_event";
    }

    death_knight_t* p() const
    {
      return debug_cast<death_knight_t*>( dk );
    }

    void execute() override
    {
      for ( unsigned i = 0; i < n; ++i )
      {
        if ( p()->talent.unholy.bursting_sores.ok() && p()->active_spells.bursting_sores->target_list().size() != 0 )
        {
          p()->active_spells.bursting_sores->execute_on_target( t );
        }
      }
    }
  };

  // Don't bother creating the event if n is 0, the target has no wounds, or is scheduled to demise
  if ( !talent.unholy.bursting_sores.ok() || sim->event_mgr.canceled )
  {
    return;
  }

  make_event<bs_event_t>( *sim, this, target, n );
}

void death_knight_t::burst_festering_wound( player_t* target, unsigned n, proc_t* proc )
{
  struct fs_burst_t : public event_t
  {
    unsigned n;
    player_t* target;
    player_t* dk;
    proc_t* proc;

    fs_burst_t( player_t* p, player_t* target, unsigned n, proc_t* proc )
      : event_t( *p, 0_ms ), n( n ), target( target ), dk( p ), proc( proc )
    {
    }

    const char* name() const override
    {
      return "festering_wounds_burst";
    }

    death_knight_t* p() const
    {
      return debug_cast<death_knight_t*>( dk );
    }

    void execute() override
    {
      death_knight_td_t* td = p()->get_target_data( target );

      unsigned n_executes = std::min( n, as<unsigned>( td->debuff.festering_wound->check() ) );
      for ( unsigned i = 0; i < n_executes; ++i )
      {
        p()->active_spells.festering_wound->execute_on_target( target );
        proc->occur();
      }

      p()->trigger_bursting_sores( p(), n_executes );

      if ( p()->talent.unholy.festermight.ok() )
      {
        p()->buffs.festermight->trigger( n_executes );
      }

      if ( p()->talent.unholy.festering_scythe.ok() )
      {
        if( !p()->buffs.festering_scythe->check() )
        {
          p()->buffs.festering_scythe_stacks->trigger( n_executes );
        }
      }

      td->debuff.festering_wound->decrement( n_executes );
    }
  };

  const death_knight_td_t* td = get_target_data( target );

  // Don't bother creating the event if n is 0, the target has no wounds, or is scheduled to demise
  if ( !spec.festering_wound->ok() || !n || !td || !td->debuff.festering_wound->check() || target->demise_event )
  {
    return;
  }

  make_event<fs_burst_t>( *sim, this, target, n, proc );
}

// Launches the repeating event for the Inexorable Assault talent
void death_knight_t::start_inexorable_assault()
{
  if ( !talent.frost.inexorable_assault.ok() )
  {
    return;
  }

  buffs.inexorable_assault->trigger( buffs.inexorable_assault->max_stack() );

  // Inexorable assault keeps ticking out of combat and when it's at max stacks
  // We solve that by picking a random point at which the buff starts ticking
  timespan_t first = timespan_t::from_millis(
      rng().range( 0, as<int>( talent.frost.inexorable_assault->effectN( 1 ).period().total_millis() ) ) );

  make_event( *sim, first, [ this ]() {
    buffs.inexorable_assault->trigger();
    make_repeating_event( *sim, talent.frost.inexorable_assault->effectN( 1 ).period(),
                          [ this ]() { buffs.inexorable_assault->trigger(); } );
  } );
}

// Launches the repeting event for the cold heart talent
void death_knight_t::start_cold_heart()
{
  if ( !talent.frost.cold_heart.ok() )
  {
    return;
  }

  buffs.cold_heart->trigger( buffs.cold_heart->max_stack() );

  // Cold Heart keeps ticking out of combat and when it's at max stacks
  // We solve that by picking a random point at which the buff starts ticking
  timespan_t first = timespan_t::from_millis(
      rng().range( 0, as<int>( talent.frost.cold_heart->effectN( 1 ).period().total_millis() ) ) );

  make_event( *sim, first, [ this ]() {
    buffs.cold_heart->trigger();
    make_repeating_event( *sim, talent.frost.cold_heart->effectN( 1 ).period(),
                          [ this ]() { buffs.cold_heart->trigger(); } );
  } );
}

void death_knight_t::chill_streak_bounce( player_t& t )
{
  struct cs_bounce_t : public event_t
  {
    player_t* t;
    death_knight_t* dk;

    cs_bounce_t( death_knight_t* dk, player_t* target ) : event_t( *dk, 0_ms ), t( target ), dk( dk )
    {
    }

    const char* name() const override
    {
      return "chill_streak_bounce";
    }

    void execute() override
    {
      vector_with_callback<player_t*> target_list = dk->sim->target_non_sleeping_list;
      if ( target_list.size() == 1 )
      {
        target_list.push_back( dk );
      }

      for ( const auto target : target_list )
      {
        if ( target != t )
        {
          dk->active_spells.chill_streak_damage->set_target( target );
          dk->active_spells.chill_streak_damage->schedule_execute();
          return;
        }
      }
    }
  };

  make_event<cs_bounce_t>( *sim, this, &t );
}

void death_knight_t::summon_rider( timespan_t duration, bool random )
{
  std::vector<action_t*> random_rider_spells = active_spells.rider_summon_spells;
  // Can not randomly summon the same rider twice in a row
  if ( random && active_spells.last_rider_summoned != nullptr && active_spells.last_rider_summoned == random_rider_spells[ 0 ] )
  {
    auto it = std::find( random_rider_spells.begin(), random_rider_spells.end(), active_spells.last_rider_summoned );
    random_rider_spells.erase( it );
  }

  for ( auto& rider : random_rider_spells )
  {
    debug_cast<summon_rider_t*>( rider )->duration = duration;
    debug_cast<summon_rider_t*>( rider )->random   = random;
    rider->execute();
    if ( random )
    {
      active_spells.last_rider_summoned = rider;
      rng().shuffle( active_spells.rider_summon_spells.begin(), active_spells.rider_summon_spells.end() );
      return;
    }
  }
}

void death_knight_t::extend_rider( double amount, pets::horseman_pet_t* rider )
{
  double threshold    = talent.rider.fury_of_the_horsemen->effectN( 1 ).base_value();
  double max_time     = talent.rider.fury_of_the_horsemen->effectN( 2 ).base_value();
  timespan_t duration = timespan_t::from_seconds( talent.rider.fury_of_the_horsemen->effectN( 3 ).base_value() );
  double limit        = threshold * max_time;

  if ( rider->rp_spent < limit )
  {
    rider->rp_spent += amount;
    rider->current_pool += amount;
    if ( rider->current_pool >= threshold )
    {
      rider->current_pool -= threshold;
      rider->adjust_duration( duration );
    }
  }
}

void death_knight_t::sort_undeath_targets( std::vector<player_t*> tl )
{
  undeath_tl = tl;

  std::sort( undeath_tl.begin(), undeath_tl.end(), [ this ]( player_t* a, player_t* b ) {
    return get_target_data( a )->dot.undeath->current_stack() > get_target_data( b )->dot.undeath->current_stack();
  } );
}

void death_knight_t::trigger_whitemanes_famine( player_t* main_target )
{
  auto td = get_target_data( main_target );
  td->dot.undeath->increment( as<int>( pet_spell.undeath_dot->effectN( 3 ).base_value() ) );
  auto cd = cooldown.undeath_spread->get_cooldown( main_target );

  if ( !cd->down() && sim->target_non_sleeping_list.size() > 1 )
  {
    std::vector<player_t*> tl = undeath_tl;
    auto it = range::find( tl, main_target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    player_t* undeath_target = tl[ 0 ];

    auto undeath_td  = get_target_data( undeath_target );

    if ( undeath_td->dot.undeath->is_ticking() )
    {
      undeath_td->dot.undeath->increment( as<int>( pet_spell.undeath_dot->effectN( 3 ).base_value() ) );
    }
    else
    {
      td->dot.undeath->copy( undeath_target, DOT_COPY_CLONE );
    }

    cd->start();

    std::rotate( undeath_tl.begin(), undeath_tl.begin() + 1, undeath_tl.end() );
  }
}

void death_knight_t::start_a_feast_of_souls()
{
  double threshold  = talent.rider.a_feast_of_souls->effectN( 1 ).base_value();
  timespan_t period = talent.rider.a_feast_of_souls->effectN( 1 ).period();
  timespan_t first  = timespan_t::from_millis( rng().range( 0, as<int>( period.total_millis() ) ) );

  make_event( *sim, first, [ this, threshold, period ]() {
    if ( active_riders >= threshold && !buffs.a_feast_of_souls->check() )
    {
      buffs.a_feast_of_souls->trigger();
    }
    make_repeating_event( *sim, period, [ this, threshold ]() {
      if ( active_riders >= threshold && !buffs.a_feast_of_souls->check() )
      {
        buffs.a_feast_of_souls->trigger();
      }
      if ( active_riders < threshold && buffs.a_feast_of_souls->check() )
      {
        buffs.a_feast_of_souls->expire();
      }
    } );
  } );
}

double death_knight_t::tick_damage_over_time( timespan_t duration, const dot_t* dot ) const
{
  if ( !dot->is_ticking() )
  {
    return 0.0;
  }
  action_state_t* state = dot->current_action->get_state( dot->state );
  dot->current_action->calculate_tick_amount( state, dot->current_stack() );
  double tick_base_damage  = state->result_raw;
  timespan_t dot_tick_time = dot->current_action->tick_time( state );
  double ticks_left        = duration / dot_tick_time;
  double total_damage      = ticks_left * tick_base_damage;
  action_state_t::release( state );
  return total_damage;
}

void death_knight_t::trigger_infliction_of_sorrow( player_t* target, bool is_vampiric )
{
  if( !is_vampiric && !buffs.infliction_of_sorrow->check() )
    return;

  auto base_td    = get_target_data( target );
  auto disease_td = specialization() == DEATH_KNIGHT_BLOOD ? base_td->dot.blood_plague : base_td->dot.virulent_plague;
  double disease_remaining_damage = 0;
  double mod                      = 0;

  disease_remaining_damage += tick_damage_over_time( disease_td->remains(), disease_td );

  if ( disease_remaining_damage == 0 )
    return;

  if ( is_vampiric )
  {
    timespan_t extension = timespan_t::from_seconds( talent.sanlayn.infliction_of_sorrow->effectN( 3 ).base_value() );
    mod                  = talent.sanlayn.infliction_of_sorrow->effectN( 2 ).percent();
    if ( disease_td->is_ticking() )
    {
      disease_td->adjust_duration( extension );
      if ( talent.unholy.decomposition.ok() )
      {
        base_td->debuff.decomposition->extend_duration( target, extension );
      }
    }
  }
  else if ( buffs.infliction_of_sorrow->check() )
  {
    // The talent itself references 100% damage done, and it's stored in the below effect
    // mod = talent.sanlayn.infliction_of_sorrow->effectN( 1 ).percent();
    // However, the buff that is on the player still has 200% set, and in game testing shows the explosion to be 200%
    mod = spell.infliction_of_sorrow_buff->effectN( 1 ).percent();

    buffs.infliction_of_sorrow->expire();
    if ( disease_td->is_ticking() )
    {
      disease_td->cancel();
      if ( talent.unholy.decomposition.ok() )
      {
        base_td->debuff.decomposition->expire();
      }
    }
  }

  if ( disease_remaining_damage > 0 )
  {
    active_spells.infliction_of_sorrow->execute_on_target( target, disease_remaining_damage * mod );
  }
}

void death_knight_t::trigger_vampiric_strike_proc( player_t* target )
{
  double chance    = talent.sanlayn.vampiric_strike->effectN( 1 ).percent();
  double target_hp = target->health_percentage();

  if ( talent.sanlayn.sanguine_scent.ok() && target_hp <= talent.sanlayn.sanguine_scent->effectN( 1 ).base_value() )
  {
    chance += talent.sanlayn.sanguine_scent->effectN( 2 ).percent();
  }
  if ( talent.sanlayn.bloodsoaked_ground.ok() && in_death_and_decay() )
  {
    chance += talent.sanlayn.bloodsoaked_ground->effectN( 2 ).percent();
  }

  if ( chance >= 0.9 )
  {
    buffs.vampiric_strike->predict();
  }

  if ( rng().roll( chance ) )
  {
    if ( buffs.vampiric_strike->check() )
    {
      procs.vampiric_strike_waste->occur();
    }
    buffs.vampiric_strike->trigger();
    procs.vampiric_strike->occur();
  }
}

void death_knight_t::trigger_sanlayn_execute_talents( bool is_vampiric )
{
  if ( is_vampiric )
  {
    active_spells.vampiric_strike_heal->execute();
    if ( rppm.blood_beast->trigger() )
    {
      procs.blood_beast->occur();
      pets.blood_beast.spawn();
    }
    buffs.essence_of_the_blood_queen->trigger();
    if ( !buffs.gift_of_the_sanlayn->check() )
    {
      buffs.vampiric_strike->expire();
    }
  }
}

void death_knight_t::trigger_reapers_mark_death( player_t* target )
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( sim->event_mgr.canceled )
  {
    return;
  }

  if ( !talent.deathbringer.reapers_mark.ok() )
  {
    return;
  }

  buff_t* reapers_mark = get_target_data( target )->debuff.reapers_mark;

  if ( reapers_mark->check() )
  {
    for ( auto& new_target : target->sim->target_non_sleeping_list )
    {
      if ( new_target != target )
      {
        sim->print_debug(
            "Target {} died while affected by reapers_mark with {} stacks with {} duration left, jumping to target {}.",
            target->name(), reapers_mark->check(), reapers_mark->remains(), new_target->name() );
        get_target_data( new_target )->debuff.reapers_mark->trigger( reapers_mark->check(), reapers_mark->remains() );
        reapers_mark->cancel();
        return;
      }
    }
  }
}

void death_knight_t::reapers_mark_explosion_wrapper( player_t* target, player_t* source, int stacks )
{
  if ( target != nullptr && !target->is_sleeping() && stacks > 0 && !sim->event_mgr.canceled && source != nullptr )
  {
    debug_cast<reapers_mark_explosion_t*>( active_spells.reapers_mark_explosion )->execute_wrapper( target, stacks );
  }
}

const spell_data_t* death_knight_t::conditional_spell_lookup( bool fn, int id )
{
  if ( !fn )
  {
    return spell_data_t::not_found();
  }

  return find_spell( id );
}

modified_spell_data_t* death_knight_t::get_modified_spell( const spell_data_t* s )
{
  if ( s && s->ok() )
  {
    for ( auto& m : modified_spells )
      if ( m->_spell.id() == s->id() )
        return m;

    return modified_spells.emplace_back( new modified_spell_data_t( s ) );
  }

  return modified_spell_data_t::nil();
}
// ==========================================================================
// Death Knight Character Definition
// ==========================================================================

// death_knight_t::create_actions ===========================================

void death_knight_t::create_actions()
{
  // Class talents
  if ( talent.blood_draw.ok() )
  {
    active_spells.blood_draw = get_action<blood_draw_t>( "blood_draw", this );
  }

  if ( talent.abomination_limb.ok() )
  {
    active_spells.abomination_limb_damage = get_action<abomination_limb_damage_t>( "abomination_limb_damage", this );
  }

  // Rider of the Apocalypse
  if ( talent.rider.riders_champion.ok() )
  {
    active_spells.summon_whitemane = get_action<summon_whitemane_t>( "summon_whitemane", this );
    active_spells.rider_summon_spells.push_back( active_spells.summon_whitemane );
    active_spells.undeath_dot = get_action<undeath_dot_t>( "undeath", this );

    active_spells.summon_mograine = get_action<summon_mograine_t>( "summon_mograine", this );
    active_spells.rider_summon_spells.push_back( active_spells.summon_mograine );

    active_spells.summon_trollbane = get_action<summon_trollbane_t>( "summon_trollbane", this );
    active_spells.rider_summon_spells.push_back( active_spells.summon_trollbane );
    active_spells.trollbanes_icy_fury = get_action<trollbanes_icy_fury_t>( "trollbanes_icy_fury", this );

    active_spells.summon_nazgrim = get_action<summon_nazgrim_t>( "summon_nazgrim", this );
    active_spells.rider_summon_spells.push_back( active_spells.summon_nazgrim );
  }

  // San'layn
  if ( talent.sanlayn.vampiric_strike.ok() )
  {
    active_spells.vampiric_strike_heal = get_action<vampiric_strike_heal_t>( "vampiric_strike_heal", this );
  }

  if ( talent.sanlayn.infliction_of_sorrow.ok() )
  {
    active_spells.infliction_of_sorrow = get_action<infliction_in_sorrow_t>( "infliction_of_sorrow", this );
  }

  if ( talent.sanlayn.the_blood_is_life.ok() )
  {
    active_spells.the_blood_is_life = get_action<the_blood_is_life_t>( "the_blood_is_life", this );
  }

  // Deathbringer
  if ( talent.deathbringer.reapers_mark.ok() )
  {
    active_spells.reapers_mark_explosion = get_action<reapers_mark_explosion_t>( "reapers_mark_explosion", this );
  }
  if ( talent.deathbringer.wave_of_souls.ok() )
  {
    active_spells.wave_of_souls = get_action<wave_of_souls_t>( "wave_of_souls", this );
  }
  if ( talent.deathbringer.soul_rupture.ok() )
  {
    active_spells.soul_rupture = get_action<soul_rupture_t>( "reapers_mark_soul_rupture", this );
  }
  if ( talent.deathbringer.grim_reaper.ok() )
  {
    active_spells.grim_reaper_soul_reaper = get_action<grim_reaper_soul_reaper_t>( "soul_reaper_grim_reaper", this );
  }
  if ( talent.deathbringer.exterminate.ok() )
  {
    active_spells.exterminate = get_action<exterminate_t>( "exterminate", this );
  }

  // Blood
  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    if ( talent.blood.mark_of_blood.ok() )
    {
      active_spells.mark_of_blood_heal = get_action<mark_of_blood_heal_t>( "mark_of_blood_heal", this );
    }
    if ( talent.blood.shattering_bone.ok() )
    {
      active_spells.shattering_bone = get_action<shattering_bone_t>( "shattering_bone", this );
    }
    if ( talent.blood.bloodied_blade.ok() )
    {
      active_spells.heart_strike_bloodied_blade = get_action<heart_strike_bloodied_blade_t>( "heart_strike_bloodied_blade", this );
    }
    if ( talent.soul_reaper.ok() || talent.deathbringer.grim_reaper.ok() )
    {
      active_spells.soul_reaper_execute_expired_drw = get_action<soul_reaper_execute_t>( "soul_reaper_execute_expired_drw", this );
    }
  }

  // Unholy
  else if ( specialization() == DEATH_KNIGHT_UNHOLY )
  {
    if ( spec.outbreak->ok() )
    {
      active_spells.outbreak_aoe = get_action<outbreak_aoe_t>( "outbreak_aoe", this );
      active_spells.virulent_plague = get_action<virulent_plague_t>( "virulent_plague", this );
    }
    if ( spec.festering_wound->ok() )
    {
      active_spells.festering_wound = get_action<festering_wound_t>( "festering_wound", this );
      active_spells.festering_wound_application =
          get_action<festering_wound_application_t>( "festering_wound_application", this );
    }

    if ( talent.unholy.bursting_sores.ok() )
    {
      active_spells.bursting_sores = get_action<bursting_sores_t>( "bursting_sores", this );
    }

    if ( talent.unholy.unholy_pact.ok() )
    {
      active_spells.unholy_pact_damage = get_action<unholy_pact_damage_t>( "unholy_pact_damage", this );
    }

    if ( spec.outbreak->ok() || talent.unholy.unholy_blight.ok() )
    {
      active_spells.virulent_eruption = get_action<virulent_eruption_t>( "virulent_eruption", this );
    }

    if ( talent.unholy.decomposition.ok() )
    {
      active_spells.decomposition_damage = get_action<decomposition_damage_t>( "decomposition", this );
    }

    if ( talent.unholy.unholy_blight.ok() )
    {
      active_spells.unholy_blight = get_action<unholy_blight_t>( "unholy_blight", this );
    }
  }

  else if ( specialization() == DEATH_KNIGHT_FROST )
  {
    if ( talent.frost.breath_of_sindragosa.ok() )
    {
      active_spells.breath_of_sindragosa_tick =
          get_action<breath_of_sindragosa_tick_t>( "breath_of_sindragosa_damage", this );
    }

    if ( spec.remorseless_winter->ok() )
    {
      active_spells.remorseless_winter_tick =
          get_action<remorseless_winter_damage_t>( "remorseless_winter_damage", this );
    }

    if ( talent.frost.frost_strike.ok() )
    {
      const spell_data_t* mh_data =
          main_hand_weapon.group() == WEAPON_2H ? spell.frost_strike_2h : spell.frost_strike_mh;
      active_spells.frost_strike_main =
          get_action<frost_strike_strike_t>( "frost_strike", this, &( main_hand_weapon ), mh_data, false );
      if ( off_hand_weapon.type != WEAPON_NONE )
      {
        active_spells.frost_strike_offhand = get_action<frost_strike_strike_t>(
            "frost_strike_offhand", this, &( off_hand_weapon ), spell.frost_strike_oh, false );
        if ( talent.frost.shattering_blade.ok() )
        {
          active_spells.frost_strike_sb_offhand = get_action<frost_strike_strike_t>(
              "frost_strike_offhand_sb", this, &( off_hand_weapon ), spell.frost_strike_oh, true );
        }
      }
      if ( talent.frost.shattering_blade.ok() )
      {
        active_spells.frost_strike_sb_main =
            get_action<frost_strike_strike_t>( "frost_strike_sb", this, &( main_hand_weapon ), mh_data, true );
      }
      if ( talent.frost.icy_death_torrent.ok() )
      {
        active_spells.icy_death_torrent_damage = get_action<icy_death_torrent_t>( "icy_death_torrent", this );
      }
      if ( talent.frost.hyperpyrexia.ok() )
      {
        active_spells.hyperpyrexia_damage = get_action<hyperpyrexia_damage_t>( "hyperpyrexia", this );
      }
    }

    if ( talent.frost.chill_streak.ok() )
    {
      active_spells.chill_streak_damage = get_action<chill_streak_damage_t>( "chill_streak_damage", this );
    }
  }

  player_t::create_actions();
}

// death_knight_t::create_action  ===========================================

action_t* death_knight_t::create_action( util::string_view name, util::string_view options_str )
{
  // Deathbringer Actions
  if ( name == "reapers_mark" )
    return new reapers_mark_t( this, options_str );
  // General Actions
  if ( name == "abomination_limb" )
    return new abomination_limb_t( this, options_str );
  if ( name == "antimagic_shell" )
    return new antimagic_shell_t( this, options_str );
  if ( name == "antimagic_zone" )
    return new antimagic_zone_t( this, options_str );
  if ( name == "auto_attack" )
    return new auto_attack_t( this, options_str );
  if ( name == "chains_of_ice" )
    return new chains_of_ice_t( this, options_str );
  if ( name == "death_strike" )
    return new death_strike_t( this, options_str );
  if ( name == "icebound_fortitude" )
    return new icebound_fortitude_t( this, options_str );
  if ( name == "mind_freeze" )
    return new mind_freeze_t( this, options_str );
  if ( name == "raise_dead" )
    return new raise_dead_t( this, options_str );
  if ( name == "sacrificial_pact" )
    return new sacrificial_pact_t( this, options_str );

  // Blood Actions
  if ( name == "blood_boil" )
    return new blood_boil_t( this, options_str );
  if ( name == "blood_tap" )
    return new blood_tap_t( this, options_str );
  if ( name == "blooddrinker" )
    return new blooddrinker_t( this, options_str );
  if ( name == "bonestorm" )
    return new bonestorm_t( this, options_str );
  if ( name == "consumption" )
    return new consumption_t( this, options_str );
  if ( name == "dancing_rune_weapon" )
    return new dancing_rune_weapon_t( this, options_str );
  if ( name == "dark_command" )
    return new dark_command_t( this, options_str );
  if ( name == "deaths_caress" )
    return new deaths_caress_t( this, options_str );
  if ( name == "gorefiends_grasp" )
    return new gorefiends_grasp_t( this, options_str );
  if ( name == "heart_strike" )
    return new heart_strike_t( name, this, options_str );
  if ( name == "mark_of_blood" )
    return new mark_of_blood_t( this, options_str );
  if ( name == "marrowrend" )
    return new marrowrend_t( this, options_str );
  if ( name == "rune_tap" )
    return new rune_tap_t( this, options_str );
  if ( name == "tombstone" )
    return new tombstone_t( this, options_str );
  if ( name == "vampiric_blood" )
    return new vampiric_blood_t( this, options_str );

  // Frost Actions
  if ( name == "breath_of_sindragosa" )
    return new breath_of_sindragosa_t( this, options_str );
  if ( name == "chill_streak" )
    return new chill_streak_t( this, options_str );
  if ( name == "empower_rune_weapon" )
    return new empower_rune_weapon_t( this, options_str );
  if ( name == "frost_strike" )
    return new frost_strike_t( this, options_str );
  if ( name == "frostscythe" )
    return new frostscythe_t( this, options_str );
  if ( name == "frostwyrms_fury" )
    return new frostwyrms_fury_t( this, options_str );
  if ( name == "glacial_advance" )
    return new glacial_advance_t( this, options_str );
  if ( name == "horn_of_winter" )
    return new horn_of_winter_t( this, options_str );
  if ( name == "howling_blast" )
    return new howling_blast_t( this, options_str );
  if ( name == "obliterate" )
    return new obliterate_t( this, options_str );
  if ( name == "pillar_of_frost" )
    return new pillar_of_frost_t( this, options_str );
  if ( name == "remorseless_winter" )
    return new remorseless_winter_t( this, options_str );

  // Unholy Actions
  if ( name == "army_of_the_dead" )
    return new army_of_the_dead_t( this, options_str );
  if ( name == "apocalypse" )
    return new apocalypse_t( this, options_str );
  if ( name == "clawing_shadows" )
    return new clawing_shadows_t( name, this, options_str );
  if ( name == "dark_transformation" )
    return new dark_transformation_t( this, options_str );
  if ( name == "death_and_decay" )
    return new death_and_decay_t( this, options_str );
  if ( name == "death_coil" )
    return new death_coil_t( this, options_str );
  if ( name == "defile" )
    return new defile_t( this, options_str );
  if ( name == "epidemic" )
    return new epidemic_t( this, options_str );
  if ( name == "festering_strike" )
    return new festering_strike_t( this, options_str );
  if ( name == "outbreak" )
    return new outbreak_t( this, options_str );
  if ( name == "raise_abomination" )
    return new raise_abomination_t( this, options_str );
  if ( name == "scourge_strike" )
    return new scourge_strike_t( name, this, options_str );
  if ( name == "soul_reaper" )
    return new soul_reaper_action_t( name, this, options_str );
  if ( name == "summon_gargoyle" )
    return new summon_gargoyle_t( this, options_str );
  if ( name == "unholy_assault" )
    return new unholy_assault_t( this, options_str );
  if ( name == "vile_contagion" )
    return new vile_contagion_t( this, options_str );

  // Dynamic actions
  // any_dnd and dnd_any return defile if talented, or death and decay otherwise
  if ( name == "any_dnd" || name == "dnd_any" )
  {
    if ( talent.unholy.defile.ok() )
    {
      return create_action( "defile", options_str );
    }
    return create_action( "death_and_decay", options_str );
  }

  // wound_spender will return clawing shadows if talented, scourge strike if it's not
  if ( name == "wound_spender" )
    return create_action( talent.unholy.clawing_shadows.ok() ? "clawing_shadows" : "scourge_strike", options_str );

  return player_t::create_action( name, options_str );
}

// death_knight_t::create_expression ========================================

// Equipped death knight runeforge expressions
std::unique_ptr<expr_t> death_knight_t::create_runeforge_expression( util::string_view runeforge_name,
                                                                     bool warning = false )
{
  // Razorice, looks for the damage procs related to MH and OH
  if ( util::str_compare_ci( runeforge_name, "razorice" ) )
    return expr_t::create_constant( "razorice_runforge_expression",
                                    runeforge.rune_of_razorice_mh || runeforge.rune_of_razorice_oh );

  // Razorice MH and OH expressions (this can matter for razorice application)
  if ( util::str_compare_ci( runeforge_name, "razorice_mh" ) )
    return expr_t::create_constant( "razorice_mh_runforge_expression", runeforge.rune_of_razorice_mh );
  if ( util::str_compare_ci( runeforge_name, "razorice_oh" ) )
    return expr_t::create_constant( "razorice_oh_runforge_expression", runeforge.rune_of_razorice_oh );

  // Fallen Crusader, looks for the unholy strength healing action
  if ( util::str_compare_ci( runeforge_name, "fallen_crusader" ) )
    return expr_t::create_constant( "fallen_crusader_runforge_expression", runeforge.rune_of_the_fallen_crusader );

  // Stoneskin Gargoyle
  if ( util::str_compare_ci( runeforge_name, "stoneskin_gargoyle" ) )
    return expr_t::create_constant( "stoneskin_gargoyle_runforge_expression",
                                    runeforge.rune_of_the_stoneskin_gargoyle );

  // Apocalypse
  if ( util::str_compare_ci( runeforge_name, "apocalypse" ) )
    return expr_t::create_constant( "apocalypse_runforge_expression", runeforge.rune_of_apocalypse );

  // Sanguination
  if ( util::str_compare_ci( runeforge_name, "sanguination" ) )
    return expr_t::create_constant( "sanguination_runeforge_expression", runeforge.rune_of_sanguination );

  // Spellwarding
  if ( util::str_compare_ci( runeforge_name, "spellwarding" ) )
    return expr_t::create_constant( "spellwarding_runeforge_expression", runeforge.rune_of_spellwarding != 0 );

  // Unending Thirst, effect NYI
  if ( util::str_compare_ci( runeforge_name, "unending_thirst" ) )
    return expr_t::create_constant( "unending_thirst_runeforge_expression", runeforge.rune_of_unending_thirst );

  // Hysteria
  if ( util::str_compare_ci( runeforge_name, "hysteria" ) )
    return expr_t::create_constant( "hysteria_runeforge_expression", runeforge.rune_of_hysteria );

  // Only throw an error with death_knight.runeforge expressions
  // runeforge.x already spits out a warning for relevant runeforges and has to send a runeforge legendary if needed
  if ( !warning )
    throw std::invalid_argument( fmt::format( "Unknown Death Knight runeforge name '{}'", runeforge_name ) );
  return nullptr;
}

std::unique_ptr<expr_t> death_knight_t::create_expression( util::string_view name_str )
{
  auto splits = util::string_split<util::string_view>( name_str, "." );

  if ( util::str_compare_ci( splits[ 0 ], "rune" ) && splits.size() > 1 )
  {
    // rune.time_to_x returns the number of seconds before X runes will be ready at the current generation rate
    if ( util::str_in_str_ci( splits[ 1 ], "time_to_" ) )
    {
      auto n_char = splits[ 1 ][ splits[ 1 ].size() - 1 ];
      auto n      = n_char - '0';
      if ( n <= 0 || as<size_t>( n ) > MAX_RUNES )
        throw std::invalid_argument(
            fmt::format( "Error in rune.time_to expression, please enter a valid amount of runes" ) );

      return make_fn_expr( "rune_time_to_x", [ this, n ]() { return _runes.time_to_regen( as<unsigned>( n ) ); } );
    }
  }

  // Check if IQD execute is disabled
  if ( util::str_compare_ci( splits[ 0 ], "death_knight" ) && splits.size() > 1 )
  {
    if ( util::str_compare_ci( splits[ 1 ], "disable_iqd_execute" ) && splits.size() == 2 )
      return expr_t::create_constant( " disable_iqd_execute_expression ", sim->shadowlands_opts.disable_iqd_execute );
  }

  // Expose AMS Absorb Percent to the APL to enable decisions based on anticipated resource generation
  if ( util::str_compare_ci( splits[ 0 ], "death_knight" ) && splits.size() > 1 )
  {
    if ( util::str_compare_ci( splits[ 1 ], "ams_absorb_percent" ) && splits.size() == 2 )
      return expr_t::create_constant( "ams_absorb_percent", options.ams_absorb_percent );
  }

  // Expose AMZ Specified to the APL to prevent its use.
  if ( util::str_compare_ci( splits[ 0 ], "death_knight" ) && splits.size() > 1 )
  {
    if ( util::str_compare_ci( splits[ 1 ], "amz_specified" ) && splits.size() == 2 )
      return expr_t::create_constant( "amz_specified", options.amz_specified );
  }

  // Expose first AMS cast to the APL to prevent its use.
  if ( util::str_compare_ci( splits[ 0 ], "death_knight" ) && splits.size() > 1 )
  {
    if ( util::str_compare_ci( splits[ 1 ], "first_ams_cast" ) && splits.size() == 2 )
      return expr_t::create_constant( "first_ams_cast", options.first_ams_cast.total_seconds() );
  }

  // Death Knight special expressions
  if ( util::str_compare_ci( splits[ 0 ], "death_knight" ) && splits.size() > 1 )
  {
    // Returns the value of the disable_aotd option
    if ( util::str_compare_ci( splits[ 1 ], "disable_aotd" ) && splits.size() == 2 )
      return expr_t::create_constant( "disable_aotd_expression", this->options.disable_aotd );

    // Returns the number of targets currently affected by the festering wound debuff
    if ( util::str_compare_ci( splits[ 1 ], "fwounded_targets" ) && splits.size() == 2 )
      return make_fn_expr( "festering_wounds_target_count_expression",
                           [ this ]() { return this->festering_wounds_target_count; } );

    // Returns if the given death knight runeforge is equipped or not
    if ( util::str_compare_ci( splits[ 1 ], "runeforge" ) && splits.size() == 3 )
    {
      auto runeforge_expr = create_runeforge_expression( splits[ 2 ] );
      if ( runeforge_expr )
        return runeforge_expr;
    }

    throw std::invalid_argument( fmt::format( "Unknown death_knight expression '{}'", splits[ 1 ] ) );
  }

  // Death and Decay/Defile expressions
  if ( ( util::str_compare_ci( splits[ 0 ], "defile" ) || util::str_compare_ci( splits[ 0 ], "death_and_decay" ) ) &&
       splits.size() == 2 )
  {
    // Returns true if there's an active dnd
    if ( util::str_compare_ci( splits[ 1 ], "ticking" ) || util::str_compare_ci( splits[ 1 ], "up" ) )
    {
      return make_fn_expr( "dnd_ticking", [ this ]() { return active_dnd ? 1 : 0; } );
    }

    // Returns the remaining value on the active dnd, or 0 if there's no dnd
    if ( util::str_compare_ci( splits[ 1 ], "remains" ) )
    {
      return make_fn_expr( "dnd_remains",
                           [ this ]() { return active_dnd ? active_dnd->remaining_time().total_seconds() : 0; } );
    }

    // Returns true if there's an active dnd AND the player is inside it
    if ( util::str_compare_ci( splits[ 1 ], "active" ) )
    {
      return make_fn_expr( "dnd_ticking", [ this ]() { return in_death_and_decay() ? 1 : 0; } );
    }

    // Returns the remaining value on the active dnd if the player is inside it, or 0 otherwise
    if ( util::str_compare_ci( splits[ 1 ], "active_remains" ) )
    {
      return make_fn_expr( "dnd_remains", [ this ]() {
        return in_death_and_decay() ? active_dnd->remaining_time().total_seconds() : 0;
      } );
    }

    throw std::invalid_argument( fmt::format( "Unknown dnd expression '{}'", splits[ 1 ] ) );
  }

  if ( util::str_compare_ci( splits[ 0 ], "runeforge" ) && splits.size() == 2 )
  {
    auto runeforge_expr = create_runeforge_expression( splits[ 1 ], true );
    // Properly handle dk runeforge expressions using runeforge.name
    // instead of death_knight.runeforge.name, but warn the user
    if ( runeforge_expr )
    {
      runeforge_expression_warning = true;
      return runeforge_expr;
    }
  }

  return player_t::create_expression( name_str );
}

// death_knight_t::create_pets ==============================================

void death_knight_t::create_pets()
{
  // Created unconditionally for APL purpose
  // Only the permanent version with raise dead 2 is a pet, others are guardians
  pets.ghoul_pet.set_creation_callback(
      []( death_knight_t* p ) { return new pets::ghoul_pet_t( p, !p->talent.unholy.raise_dead.ok() ); } );
  pets.ghoul_pet.set_max_pets( 1 );

  if ( talent.rider.riders_champion.ok() )
  {
    pets.whitemane.set_creation_callback( []( death_knight_t* p ) { return new pets::whitemane_pet_t( p ); } );
    pets.whitemane.set_default_duration( spell.summon_whitemane_2->duration() );
    pets.whitemane.set_max_pets( 1 );
    pets.mograine.set_creation_callback( []( death_knight_t* p ) { return new pets::mograine_pet_t( p ); } );
    pets.mograine.set_default_duration( spell.summon_mograine_2->duration() );
    pets.mograine.set_max_pets( 1 );
    pets.trollbane.set_creation_callback( []( death_knight_t* p ) { return new pets::trollbane_pet_t( p ); } );
    pets.trollbane.set_default_duration( spell.summon_trollbane_2->duration() );
    pets.trollbane.set_max_pets( 1 );
    pets.nazgrim.set_creation_callback( []( death_knight_t* p ) { return new pets::nazgrim_pet_t( p ); } );
    pets.nazgrim.set_default_duration( spell.summon_nazgrim_2->duration() );
    pets.nazgrim.set_max_pets( 1 );
  }

  if ( talent.sanlayn.the_blood_is_life.ok() )
  {
    pets.blood_beast.set_creation_callback( []( death_knight_t* p ) { return new pets::blood_beast_pet_t( p ); } );
    pets.blood_beast.set_default_duration( spell.blood_beast_summon->duration() );
    pets.blood_beast.set_max_pets( 1 );
    pets.blood_beast.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );
  }

  if ( specialization() == DEATH_KNIGHT_UNHOLY )
  {
    // Initialized even if the talent isn't picked for APL purpose
    pets.gargoyle.set_creation_callback( []( death_knight_t* p ) { return new pets::gargoyle_pet_t( p ); } );
    pets.gargoyle.set_default_duration( talent.unholy.summon_gargoyle->duration() );

    if ( talent.unholy.all_will_serve.ok() )
    {
      pets.risen_skulker.set_creation_callback(
          []( death_knight_t* p ) { return new pets::risen_skulker_pet_t( p ); } );
      pets.risen_skulker.set_max_pets( 1 );
      pets.risen_skulker.set_replacement_strategy( spawner::pet_replacement_strategy::NO_REPLACE );
    }

    if ( talent.unholy.army_of_the_dead.ok() && !talent.unholy.raise_abomination.ok() )
    {
      pets.army_ghouls.set_creation_callback(
          []( death_knight_t* p ) { return new pets::army_ghoul_pet_t( p, "army_ghoul" ); } );
      pets.army_ghouls.set_default_duration( talent.unholy.army_of_the_dead->effectN( 1 ).trigger()->duration() );
      pets.army_ghouls.set_max_pets( 8 );
    }

    if ( talent.unholy.magus_of_the_dead.ok() &&
         ( talent.unholy.army_of_the_dead.ok() || talent.unholy.raise_abomination.ok() ) )
    {
      pets.army_magus.set_creation_callback(
          []( death_knight_t* p ) { return new pets::magus_pet_t( p, "army_magus" ); } );
      pets.army_magus.set_default_duration( talent.unholy.army_of_the_dead->effectN( 1 ).trigger()->duration() );
      pets.army_magus.set_max_pets( 1 );
    }

    if ( talent.unholy.apocalypse.ok() )
    {
      pets.apoc_ghouls.set_creation_callback(
          []( death_knight_t* p ) { return new pets::army_ghoul_pet_t( p, "apoc_ghoul" ); } );
      pets.apoc_ghouls.set_default_duration( spell.apocalypse_duration->duration() );
      pets.apoc_ghouls.set_max_pets( 4 );

      if ( talent.unholy.magus_of_the_dead.ok() )
      {
        pets.apoc_magus.set_creation_callback(
            []( death_knight_t* p ) { return new pets::magus_pet_t( p, "apoc_magus" ); } );
        pets.apoc_magus.set_default_duration( spell.apocalypse_duration->duration() );
        pets.apoc_magus.set_max_pets( 1 );
      }
    }

    if ( talent.unholy.doomed_bidding.ok() )
    {
      timespan_t doomed_bidding_duration =
          timespan_t::from_millis( talent.unholy.doomed_bidding->effectN( 1 ).base_value() );
      pets.doomed_bidding_magus_coil.set_creation_callback(
          []( death_knight_t* p ) { return new pets::magus_pet_t( p, "doomed_bidding_magus_coil" ); } );
      pets.doomed_bidding_magus_coil.set_default_duration( doomed_bidding_duration );
      pets.doomed_bidding_magus_epi.set_creation_callback(
          []( death_knight_t* p ) { return new pets::magus_pet_t( p, "doomed_bidding_magus_epi" ); } );
      pets.doomed_bidding_magus_epi.set_default_duration( doomed_bidding_duration );
    }

    if ( talent.unholy.raise_abomination.ok() )
    {
      pets.abomination.set_creation_callback( []( death_knight_t* p ) { return new pets::abomination_pet_t( p ); } );
      pets.abomination.set_default_duration( talent.unholy.raise_abomination->duration() );
      pets.abomination.set_max_pets( 1 );
    }
  }

  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    if ( talent.blood.dancing_rune_weapon.ok() )
    {
      pets.dancing_rune_weapon_pet.set_creation_callback(
          []( death_knight_t* p ) { return new pets::dancing_rune_weapon_pet_t( p, "dancing_rune_weapon" ); } );
      pets.dancing_rune_weapon_pet.set_default_duration( talent.blood.dancing_rune_weapon->duration() +
                                                         talent.blood.everlasting_bond->effectN( 2 ).time_value() );
      pets.dancing_rune_weapon_pet.set_max_pets( 1 );

      if ( talent.blood.everlasting_bond.ok() )
      {
        pets.everlasting_bond_pet.set_creation_callback(
            []( death_knight_t* p ) { return new pets::dancing_rune_weapon_pet_t( p, "everlasting_bond" ); } );
        pets.everlasting_bond_pet.set_default_duration( talent.blood.dancing_rune_weapon->duration() +
                                                        talent.blood.everlasting_bond->effectN( 2 ).time_value() );
        pets.everlasting_bond_pet.set_max_pets( 1 );
      }
    }

    if ( talent.blood.bloodworms.ok() )
    {
      pets.bloodworms.set_creation_callback( []( death_knight_t* p ) { return new pets::bloodworm_pet_t( p ); } );
      pets.bloodworms.set_default_duration(
          talent.blood.bloodworms->effectN( 2 ).trigger()->effectN( 1 ).trigger()->duration() );
      pets.bloodworms.set_max_pets( 5 );
    }
  }
}

// death_knight_t::init_rng =================================================

void death_knight_t::init_rng()
{
  player_t::init_rng();

  rppm.bloodworms        = get_rppm( "bloodworms", talent.blood.bloodworms );
  rppm.carnage           = get_rppm( "carnage", talent.blood.carnage );
  rppm.runic_attenuation = get_rppm( "runic_attenuation", talent.runic_attenuation );
  rppm.blood_beast       = get_rppm( "blood_beast", talent.sanlayn.the_blood_is_life );
  rppm.tww1_fdk_4pc      = get_rppm( "tww1_fdk_4pc", sets->set( DEATH_KNIGHT_FROST, TWW1, B4 ) );
}

// death_knight_t::init_base ================================================

void death_knight_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 5;

  player_t::init_base_stats();

  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility  = 0.0;
  base.spell_power_per_intellect = 1.0;
  base.leech += talent.blood_scent->effectN( 1 ).percent();

  resources.base[ RESOURCE_RUNIC_POWER ] = 100;
  resources.base[ RESOURCE_RUNIC_POWER ] += spec.blood_death_knight->effectN( 10 ).resource( RESOURCE_RUNIC_POWER );

  if ( talent.blood.ossuary.ok() )
    resources.base[ RESOURCE_RUNIC_POWER ] += talent.blood.ossuary->effectN( 2 ).resource( RESOURCE_RUNIC_POWER );

  if ( talent.frost.runic_command.ok() )
    resources.base[ RESOURCE_RUNIC_POWER ] += talent.frost.runic_command->effectN( 1 ).resource( RESOURCE_RUNIC_POWER );

  if ( talent.unholy.runic_mastery.ok() )
    resources.base[ RESOURCE_RUNIC_POWER ] +=
        talent.unholy.runic_mastery->effectN( 1 ).resource( RESOURCE_RUNIC_POWER );

  resources.base[ RESOURCE_RUNE ] = MAX_RUNES;
}

// death_knight_t::init_spells ==============================================
void death_knight_t::init_spells()
{
  player_t::init_spells();

  // Specialization

  // Generic baselines
  spec.plate_specialization = find_specialization_spell( "Plate Specialization" );
  spec.death_knight         = find_spell( 137005 );  // "Death Knight" passive
  spec.death_knight_2       = find_spell( 462062 );  // Second "Death Knight" passive
  spec.death_coil           = find_class_spell( "Death Coil" );
  spec.death_grip           = find_class_spell( "Death Grip" );
  spec.dark_command         = find_class_spell( "Dark Command" );
  spec.death_and_decay      = find_class_spell( "Death and Decay" );
  spec.rune_strike          = find_class_spell( "Rune Strike" );
  spec.chains_of_ice        = find_class_spell( "Chains of Ice" );
  spec.antimagic_shell      = find_class_spell( "Anti-Magic Shell" );

  // Blood Baselines
  spec.blood_death_knight   = find_specialization_spell( "Blood Death Knight" );
  spec.blood_death_knight_2 = conditional_spell_lookup( spec.blood_death_knight->ok(), 462061 );
  spec.riposte              = find_specialization_spell( "Riposte" );
  spec.blood_fortification  = find_specialization_spell( "Blood Fortification" );
  spec.crimson_scourge      = find_specialization_spell( "Crimson Scourge" );
  spec.deaths_caress        = find_specialization_spell( "Death's Caress" );

  // Frost Baselines
  spec.frost_death_knight         = find_specialization_spell( "Frost Death Knight" );
  spec.frost_death_knight_2       = conditional_spell_lookup( spec.frost_death_knight->ok(), 462063 );
  spec.remorseless_winter         = find_specialization_spell( "Remorseless Winter" );
  spec.might_of_the_frozen_wastes = find_specialization_spell( "Might of the Frozen Wastes" );
  spec.frostreaper                = find_specialization_spell( "Frostreaper" );
  spec.rime                       = find_specialization_spell( "Rime" );

  // Unholy Baselines
  spec.unholy_death_knight   = find_specialization_spell( "Unholy Death Knight" );
  spec.unholy_death_knight_2 = conditional_spell_lookup( spec.unholy_death_knight->ok(), 462064 );
  spec.festering_wound       = conditional_spell_lookup( spec.unholy_death_knight->ok(), 197147 );
  spec.dark_transformation_2 = conditional_spell_lookup( spec.unholy_death_knight->ok(), 325554 );
  spec.outbreak              = find_specialization_spell( "Outbreak" );
  spec.epidemic              = find_specialization_spell( "Epidemic" );

  //////// Class Talent Tree
  // Row 1
  talent.icebound_fortitude = find_talent_spell( talent_tree::CLASS, "Icebound Fortitude" );
  talent.death_strike       = find_talent_spell( talent_tree::CLASS, "Death Strike" );
  talent.raise_dead         = find_talent_spell( talent_tree::CLASS, "Raise Dead" );
  // Row 2
  talent.runic_attenuation     = find_talent_spell( talent_tree::CLASS, "Runic Attenuation" );
  talent.improved_death_strike = find_talent_spell( talent_tree::CLASS, "Improved Death Strike" );
  talent.cleaving_strikes      = find_talent_spell( talent_tree::CLASS, "Cleaving Strikes" );
  // Row 3
  talent.mindfreeze        = find_talent_spell( talent_tree::CLASS, "Mind Freeze" );
  talent.blinding_sleet    = find_talent_spell( talent_tree::CLASS, "Blinding Sleet" );
  talent.antimagic_barrier = find_talent_spell( talent_tree::CLASS, "Anti-magic Barrier" );
  talent.march_of_darkness = find_talent_spell( talent_tree::CLASS, "March of Darkness" );
  talent.unholy_ground     = find_talent_spell( talent_tree::CLASS, "Unholy Ground" );
  talent.control_undead    = find_talent_spell( talent_tree::CLASS, "Control Undead" );
  talent.enfeeble          = find_talent_spell( talent_tree::CLASS, "Enfeeble" );
  talent.sacrificial_pact  = find_talent_spell( talent_tree::CLASS, "Sacrificial Pact" );
  // Row 4
  talent.coldthirst               = find_talent_spell( talent_tree::CLASS, "Coldthirst" );
  talent.proliferating_chill      = find_talent_spell( talent_tree::CLASS, "Proliferating Chill" );
  talent.permafrost               = find_talent_spell( talent_tree::CLASS, "Permafrost" );
  talent.veteran_of_the_third_war = find_talent_spell( talent_tree::CLASS, "Veteran of the Third War" );
  talent.death_pact               = find_talent_spell( talent_tree::CLASS, "Death Pact" );
  talent.brittle                  = find_talent_spell( talent_tree::CLASS, "Brittle" );
  talent.deaths_reach             = find_talent_spell( talent_tree::CLASS, "Death's Reach" );
  // Row 5
  talent.icy_talons     = find_talent_spell( talent_tree::CLASS, "Icy Talons" );
  talent.antimagic_zone = find_talent_spell( talent_tree::CLASS, "Anti-magic Zone" );
  talent.unholy_bond    = find_talent_spell( talent_tree::CLASS, "Unholy Bond" );
  // Row 6
  talent.ice_prison       = find_talent_spell( talent_tree::CLASS, "Ice Prison" );
  talent.gloom_ward       = find_talent_spell( talent_tree::CLASS, "Gloom Ward" );
  talent.asphyxiate       = find_talent_spell( talent_tree::CLASS, "Asphyxiate" );
  talent.assimilation     = find_talent_spell( talent_tree::CLASS, "Assimilation" );
  talent.wraith_walk      = find_talent_spell( talent_tree::CLASS, "Wraith Walk" );
  talent.grip_of_the_dead = find_talent_spell( talent_tree::CLASS, "Grip of the Dead" );
  talent.soul_reaper      = find_talent_spell( talent_tree::CLASS, "Soul Reaper" );
  // Row 7
  talent.suppression      = find_talent_spell( talent_tree::CLASS, "Suppression" );
  talent.blood_scent      = find_talent_spell( talent_tree::CLASS, "Blood Scent" );
  talent.unholy_endurance = find_talent_spell( talent_tree::CLASS, "Unholy Endurance" );
  // Row 8
  talent.osmosis          = find_talent_spell( talent_tree::CLASS, "Osmosis" );
  talent.insidious_chill  = find_talent_spell( talent_tree::CLASS, "Insidious Chill" );
  talent.runic_protection = find_talent_spell( talent_tree::CLASS, "Runic Protection" );
  talent.blood_draw       = find_talent_spell( talent_tree::CLASS, "Blood Draw" );
  // Row 9
  talent.rune_mastery           = find_talent_spell( talent_tree::CLASS, "Rune Mastery" );
  talent.subuding_grasp         = find_talent_spell( talent_tree::CLASS, "Subduing Grasp" );
  talent.will_of_the_necropolis = find_talent_spell( talent_tree::CLASS, "Will of the Necropolis" );
  // Row 10
  talent.null_magic       = find_talent_spell( talent_tree::CLASS, "Null Magic" );
  talent.unyielding_will  = find_talent_spell( talent_tree::CLASS, "Unyielding Will" );
  talent.abomination_limb = find_talent_spell( talent_tree::CLASS, "Abomination Limb" );
  talent.deaths_echo      = find_talent_spell( talent_tree::CLASS, "Death's Echo" );
  talent.vestigial_shell  = find_talent_spell( talent_tree::CLASS, "Vestigial Shell" );

  //////// Blood
  // Row 1
  talent.blood.heart_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Heart Strike" );
  // Row 2
  talent.blood.marrowrend = find_talent_spell( talent_tree::SPECIALIZATION, "Marrowrend" );
  talent.blood.blood_boil = find_talent_spell( talent_tree::SPECIALIZATION, "Blood Boil" );
  // Row 3
  talent.blood.vampiric_blood = find_talent_spell( talent_tree::SPECIALIZATION, "Vampiric Blood" );
  talent.blood.bone_collector = find_talent_spell( talent_tree::SPECIALIZATION, "Bone Collector" );

  // Row 4
  talent.blood.ossuary                 = find_talent_spell( talent_tree::SPECIALIZATION, "Ossuary" );
  talent.blood.improved_vampiric_blood = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Vampiric Blood" );
  talent.blood.improved_heart_strike   = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Heart Strike" );
  talent.blood.ossified_vitriol        = find_talent_spell( talent_tree::SPECIALIZATION, "Ossified Vitriol" );

  // Row 5
  talent.blood.leeching_strike     = find_talent_spell( talent_tree::SPECIALIZATION, "Leeching Strike" );
  talent.blood.heartbreaker        = find_talent_spell( talent_tree::SPECIALIZATION, "Heartbreaker" );
  talent.blood.foul_bulwark        = find_talent_spell( talent_tree::SPECIALIZATION, "Foul Bulwark" );
  talent.blood.dancing_rune_weapon = find_talent_spell( talent_tree::SPECIALIZATION, "Dancing Rune Weapon" );
  talent.blood.hemostasis          = find_talent_spell( talent_tree::SPECIALIZATION, "Hemostasis" );
  talent.blood.perseverance_of_the_ebon_blade =
      find_talent_spell( talent_tree::SPECIALIZATION, "Perseverance of the Ebon Blade" );
  talent.blood.relish_in_blood = find_talent_spell( talent_tree::SPECIALIZATION, "Relish in Blood" );
  // Row 6
  talent.blood.rune_tap            = find_talent_spell( talent_tree::SPECIALIZATION, "Rune Tap" );
  talent.blood.gorefiends_grasp    = find_talent_spell( talent_tree::SPECIALIZATION, "Gorefiend's Grasp" );
  talent.blood.insatiable_blade    = find_talent_spell( talent_tree::SPECIALIZATION, "Insatiable Blade" );
  talent.blood.reinforced_bones    = find_talent_spell( talent_tree::SPECIALIZATION, "Reinforced Bones" );
  talent.blood.rapid_decomposition = find_talent_spell( talent_tree::SPECIALIZATION, "Rapid Decomposition" );
  // Row 7
  talent.blood.blood_tap            = find_talent_spell( talent_tree::SPECIALIZATION, "Blood Tap" );
  talent.blood.improved_bone_shield = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Bone Shield" );
  talent.blood.tightening_grasp     = find_talent_spell( talent_tree::SPECIALIZATION, "Tightening Grasp" );
  talent.blood.everlasting_bond     = find_talent_spell( talent_tree::SPECIALIZATION, "Everlasting Bond" );
  talent.blood.voracious            = find_talent_spell( talent_tree::SPECIALIZATION, "Voracious" );
  talent.blood.coagulopathy         = find_talent_spell( talent_tree::SPECIALIZATION, "Coagulopathy" );
  talent.blood.bloodworms           = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodworms" );
  // Row 8
  talent.blood.blood_feast     = find_talent_spell( talent_tree::SPECIALIZATION, "Blood Feast" );
  talent.blood.mark_of_blood   = find_talent_spell( talent_tree::SPECIALIZATION, "Mark of Blood" );
  talent.blood.tombstone       = find_talent_spell( talent_tree::SPECIALIZATION, "Tombstone" );
  talent.blood.blooddrinker    = find_talent_spell( talent_tree::SPECIALIZATION, "Blooddrinker" );
  talent.blood.consumption     = find_talent_spell( talent_tree::SPECIALIZATION, "Consumption" );
  talent.blood.bloodied_blade  = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodied Blade" );
  talent.blood.sanguine_ground = find_talent_spell( talent_tree::SPECIALIZATION, "Sanguine Ground" );
  // Row 9
  talent.blood.shattering_bone = find_talent_spell( talent_tree::SPECIALIZATION, "Shattering Bone" );
  talent.blood.heartrend       = find_talent_spell( talent_tree::SPECIALIZATION, "Heartrend" );
  talent.blood.carnage         = find_talent_spell( talent_tree::SPECIALIZATION, "Carnage" );
  talent.blood.iron_heart      = find_talent_spell( talent_tree::SPECIALIZATION, "Iron Heart" );
  talent.blood.red_thirst      = find_talent_spell( talent_tree::SPECIALIZATION, "Red Thirst" );
  // Row 10
  talent.blood.bonestorm         = find_talent_spell( talent_tree::SPECIALIZATION, "Bonestorm" );
  talent.blood.purgatory         = find_talent_spell( talent_tree::SPECIALIZATION, "Purgatory" );
  talent.blood.bloodshot         = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodshot" );
  talent.blood.umbilicus_eternus = find_talent_spell( talent_tree::SPECIALIZATION, "Umbilicus Eternus" );

  //////// Frost
  // Row 1
  talent.frost.frost_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Frost Strike" );
  // Row 2
  talent.frost.obliterate    = find_talent_spell( talent_tree::SPECIALIZATION, "Obliterate" );
  talent.frost.howling_blast = find_talent_spell( talent_tree::SPECIALIZATION, "Howling Blast" );
  // Row 3
  talent.frost.killing_machine = find_talent_spell( talent_tree::SPECIALIZATION, "Killing Machine" );
  talent.frost.everfrost       = find_talent_spell( talent_tree::SPECIALIZATION, "Everfrost" );
  // Row 4
  talent.frost.unleashed_frenzy      = find_talent_spell( talent_tree::SPECIALIZATION, "Unleashed Frenzy" );
  talent.frost.runic_command         = find_talent_spell( talent_tree::SPECIALIZATION, "Runic Command" );
  talent.frost.improved_frost_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Frost Strike" );
  talent.frost.improved_rime         = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Rime" );
  // Row 5
  talent.frost.improved_obliterate = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Obliterate" );
  talent.frost.glacial_advance     = find_talent_spell( talent_tree::SPECIALIZATION, "Glacial Advance" );
  talent.frost.pillar_of_frost     = find_talent_spell( talent_tree::SPECIALIZATION, "Pillar of Frost" );
  talent.frost.frostscythe         = find_talent_spell( talent_tree::SPECIALIZATION, "Frostscythe" );
  talent.frost.biting_cold         = find_talent_spell( talent_tree::SPECIALIZATION, "Biting Cold" );
  // Row 6
  talent.frost.rage_of_the_frozen_champion =
      find_talent_spell( talent_tree::SPECIALIZATION, "Rage of the Frozen Champion" );
  talent.frost.frigid_executioner  = find_talent_spell( talent_tree::SPECIALIZATION, "Frigid Executioner" );
  talent.frost.cold_heart          = find_talent_spell( talent_tree::SPECIALIZATION, "Cold Heart" );
  talent.frost.horn_of_winter      = find_talent_spell( talent_tree::SPECIALIZATION, "Horn of Winter" );
  talent.frost.enduring_strength   = find_talent_spell( talent_tree::SPECIALIZATION, "Enduring Strength" );
  talent.frost.icecap              = find_talent_spell( talent_tree::SPECIALIZATION, "Icecap" );
  talent.frost.frostwhelps_aid     = find_talent_spell( talent_tree::SPECIALIZATION, "Frostwhelp's Aid" );
  talent.frost.empower_rune_weapon = find_talent_spell( talent_tree::SPECIALIZATION, "Empower Rune Weapon" );
  talent.frost.chill_streak        = find_talent_spell( talent_tree::SPECIALIZATION, "Chill Streak" );
  // Row 7
  talent.frost.murderous_efficiency = find_talent_spell( talent_tree::SPECIALIZATION, "Murderous Efficiency" );
  talent.frost.inexorable_assault   = find_talent_spell( talent_tree::SPECIALIZATION, "Inexorable Assault" );
  talent.frost.frostwyrms_fury      = find_talent_spell( talent_tree::SPECIALIZATION, "Frostwyrm's Fury" );
  talent.frost.gathering_storm      = find_talent_spell( talent_tree::SPECIALIZATION, "Gathering Storm" );
  talent.frost.cryogenic_chamber    = find_talent_spell( talent_tree::SPECIALIZATION, "Cryogenic Chamber" );
  talent.frost.enduring_chill       = find_talent_spell( talent_tree::SPECIALIZATION, "Enduring Chill" );
  talent.frost.piercing_chill       = find_talent_spell( talent_tree::SPECIALIZATION, "Piercing Chill" );
  // Row 8
  talent.frost.bonegrinder        = find_talent_spell( talent_tree::SPECIALIZATION, "Bonegrinder" );
  talent.frost.smothering_offense = find_talent_spell( talent_tree::SPECIALIZATION, "Smothering Offense" );
  talent.frost.absolute_zero      = find_talent_spell( talent_tree::SPECIALIZATION, "Absolute Zero" );
  talent.frost.avalanche          = find_talent_spell( talent_tree::SPECIALIZATION, "Avalanche" );
  talent.frost.icebreaker         = find_talent_spell( talent_tree::SPECIALIZATION, "Icebreaker" );
  // Row 9
  talent.frost.obliteration      = find_talent_spell( talent_tree::SPECIALIZATION, "Obliteration" );
  talent.frost.icy_death_torrent = find_talent_spell( talent_tree::SPECIALIZATION, "Icy Death Torrent" );
  talent.frost.shattering_blade  = find_talent_spell( talent_tree::SPECIALIZATION, "Shattering Blade" );
  talent.frost.hyperpyrexia      = find_talent_spell( talent_tree::SPECIALIZATION, "Hyperpyrexia" );
  // Row 10
  talent.frost.arctic_assault       = find_talent_spell( talent_tree::SPECIALIZATION, "Arctic Assault" );
  talent.frost.the_long_winter      = find_talent_spell( talent_tree::SPECIALIZATION, "The Long Winter" );
  talent.frost.shattered_frost      = find_talent_spell( talent_tree::SPECIALIZATION, "Shattered Frost" );
  talent.frost.breath_of_sindragosa = find_talent_spell( talent_tree::SPECIALIZATION, "Breath of Sindragosa" );

  //////// Unholy
  // Row 1
  talent.unholy.festering_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Festering Strike" );
  // Row 2
  talent.unholy.scourge_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Scourge Strike" );
  talent.unholy.raise_dead     = find_talent_spell( talent_tree::SPECIALIZATION, "Raise Dead" );
  // Row 3
  talent.unholy.sudden_doom         = find_talent_spell( talent_tree::SPECIALIZATION, "Sudden Doom" );
  talent.unholy.dark_transformation = find_talent_spell( talent_tree::SPECIALIZATION, "Dark Transformation" );
  // Row 4
  talent.unholy.foul_infections = find_talent_spell( talent_tree::SPECIALIZATION, "Foul Infections" );
  talent.unholy.improved_festering_strike =
      find_talent_spell( talent_tree::SPECIALIZATION, "Improved Festering Strike" );
  talent.unholy.runic_mastery = find_talent_spell( talent_tree::SPECIALIZATION, "Runic Mastery" );
  talent.unholy.eternal_agony = find_talent_spell( talent_tree::SPECIALIZATION, "Eternal Agony" );
  // Row 5
  talent.unholy.defile           = find_talent_spell( talent_tree::SPECIALIZATION, "Defile" );
  talent.unholy.unholy_blight    = find_talent_spell( talent_tree::SPECIALIZATION, "Unholy Blight" );
  talent.unholy.festering_scythe = find_talent_spell( talent_tree::SPECIALIZATION, "Festering Scythe" );
  talent.unholy.apocalypse       = find_talent_spell( talent_tree::SPECIALIZATION, "Apocalypse" );
  talent.unholy.plaguebringer    = find_talent_spell( talent_tree::SPECIALIZATION, "Plaguebringer" );
  talent.unholy.clawing_shadows  = find_talent_spell( talent_tree::SPECIALIZATION, "Clawing Shadows" );
  talent.unholy.reaping          = find_talent_spell( talent_tree::SPECIALIZATION, "Reaping" );
  talent.unholy.all_will_serve   = find_talent_spell( talent_tree::SPECIALIZATION, "All Will Serve" );
  // Row 6
  talent.unholy.ebon_fever          = find_talent_spell( talent_tree::SPECIALIZATION, "Ebon Fever" );
  talent.unholy.bursting_sores      = find_talent_spell( talent_tree::SPECIALIZATION, "Bursting Sores" );
  talent.unholy.ghoulish_frenzy     = find_talent_spell( talent_tree::SPECIALIZATION, "Ghoulish Frenzy" );
  talent.unholy.magus_of_the_dead   = find_talent_spell( talent_tree::SPECIALIZATION, "Magus of the Dead" );
  talent.unholy.improved_death_coil = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Death Coil" );
  talent.unholy.harbinger_of_doom   = find_talent_spell( talent_tree::SPECIALIZATION, "Harbinger of Doom" );
  talent.unholy.unholy_pact         = find_talent_spell( talent_tree::SPECIALIZATION, "Unholy Pact" );
  // Row 7
  talent.unholy.vile_contagion      = find_talent_spell( talent_tree::SPECIALIZATION, "Vile Contagion" );
  talent.unholy.pestilence          = find_talent_spell( talent_tree::SPECIALIZATION, "Pestilence" );
  talent.unholy.infected_claws      = find_talent_spell( talent_tree::SPECIALIZATION, "Infected Claws" );
  talent.unholy.menacing_magus      = find_talent_spell( talent_tree::SPECIALIZATION, "Menacing Magus" );
  talent.unholy.coil_of_devastation = find_talent_spell( talent_tree::SPECIALIZATION, "Coil of Devastation" );
  talent.unholy.rotten_touch        = find_talent_spell( talent_tree::SPECIALIZATION, "Rotten Touch" );
  talent.unholy.ruptured_viscera    = find_talent_spell( talent_tree::SPECIALIZATION, "Ruptured Viscera" );
  // Row 8
  talent.unholy.death_rot        = find_talent_spell( talent_tree::SPECIALIZATION, "Death Rot" );
  talent.unholy.army_of_the_dead = find_talent_spell( talent_tree::SPECIALIZATION, "Army of the Dead" );
  talent.unholy.summon_gargoyle  = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Gargoyle" );
  talent.unholy.doomed_bidding   = find_talent_spell( talent_tree::SPECIALIZATION, "Doomed Bidding" );
  // Row 9
  talent.unholy.morbidity         = find_talent_spell( talent_tree::SPECIALIZATION, "Morbidity" );  // ITS MORBIN TIME
  talent.unholy.festermight       = find_talent_spell( talent_tree::SPECIALIZATION, "Festermight" );
  talent.unholy.raise_abomination = find_talent_spell( talent_tree::SPECIALIZATION, "Raise Abomination" );
  talent.unholy.decomposition     = find_talent_spell( talent_tree::SPECIALIZATION, "Decomposition" );
  talent.unholy.unholy_aura       = find_talent_spell( talent_tree::SPECIALIZATION, "Unholy Aura" );
  // Row 10
  talent.unholy.superstrain           = find_talent_spell( talent_tree::SPECIALIZATION, "Superstrain" );
  talent.unholy.unholy_assault        = find_talent_spell( talent_tree::SPECIALIZATION, "Unholy Assault" );
  talent.unholy.commander_of_the_dead = find_talent_spell( talent_tree::SPECIALIZATION, "Commander of the Dead" );

  //////// Rider of the Apocalypse
  talent.rider.riders_champion        = find_talent_spell( talent_tree::HERO, "Rider's Champion" );
  talent.rider.on_a_paler_horse       = find_talent_spell( talent_tree::HERO, "On a Paler Horse" );
  talent.rider.death_charge           = find_talent_spell( talent_tree::HERO, "Death Charge" );
  talent.rider.mograines_might        = find_talent_spell( talent_tree::HERO, "Mograine's Might" );
  talent.rider.horsemens_aid          = find_talent_spell( talent_tree::HERO, "Horsemen's Aid" );
  talent.rider.pact_of_the_apocalypse = find_talent_spell( talent_tree::HERO, "Pact of the Apocalypse" );
  talent.rider.whitemanes_famine      = find_talent_spell( talent_tree::HERO, "Whitemane's Famine" );
  talent.rider.nazgrims_conquest      = find_talent_spell( talent_tree::HERO, "Nazgrim's Conquest" );
  talent.rider.trollbanes_icy_fury    = find_talent_spell( talent_tree::HERO, "Trollbane's Icy Fury" );
  talent.rider.hungering_thirst       = find_talent_spell( talent_tree::HERO, "Hungering Thirst" );
  talent.rider.fury_of_the_horsemen   = find_talent_spell( talent_tree::HERO, "Fury of the Horsemen" );
  talent.rider.a_feast_of_souls       = find_talent_spell( talent_tree::HERO, "A Feast of Souls" );
  talent.rider.mawsworn_menace        = find_talent_spell( talent_tree::HERO, "Mawsworn Menace" );
  talent.rider.apocalypse_now         = find_talent_spell( talent_tree::HERO, "Apocalypse Now" );

  //////// Deathbringer
  talent.deathbringer.reapers_mark              = find_talent_spell( talent_tree::HERO, "Reaper's Mark" );
  talent.deathbringer.wave_of_souls             = find_talent_spell( talent_tree::HERO, "Wave of Souls" );
  talent.deathbringer.blood_fever               = find_talent_spell( talent_tree::HERO, "Blood Fever" );
  talent.deathbringer.bind_in_darkness          = find_talent_spell( talent_tree::HERO, "Bind in Darkness" );
  talent.deathbringer.soul_rupture              = find_talent_spell( talent_tree::HERO, "Soul Rupture" );
  talent.deathbringer.grim_reaper               = find_talent_spell( talent_tree::HERO, "Grim Reaper" );
  talent.deathbringer.pact_of_the_deathbringer  = find_talent_spell( talent_tree::HERO, "Pact of the Deathbringer" );
  talent.deathbringer.rune_carved_plates        = find_talent_spell( talent_tree::HERO, "Rune Carved Plates" );
  talent.deathbringer.swift_end                 = find_talent_spell( talent_tree::HERO, "Swift End" );
  talent.deathbringer.painful_death             = find_talent_spell( talent_tree::HERO, "Painful Death" );
  talent.deathbringer.dark_talons               = find_talent_spell( talent_tree::HERO, "Dark Talons" );
  talent.deathbringer.wither_away               = find_talent_spell( talent_tree::HERO, "Wither Away" );
  talent.deathbringer.deaths_messenger          = find_talent_spell( talent_tree::HERO, "Death's Messenger" );
  talent.deathbringer.expelling_shield          = find_talent_spell( talent_tree::HERO, "Expelling Shield" );
  talent.deathbringer.exterminate               = find_talent_spell( talent_tree::HERO, "Exterminate" );

  ///////// San'layn
  talent.sanlayn.vampiric_strike      = find_talent_spell( talent_tree::HERO, "Vampiric Strike" );
  talent.sanlayn.newly_turned         = find_talent_spell( talent_tree::HERO, "Newly Turned" );
  talent.sanlayn.vampiric_speed       = find_talent_spell( talent_tree::HERO, "Vampiric Speed" );
  talent.sanlayn.bloodsoaked_ground   = find_talent_spell( talent_tree::HERO, "Blood-soaked Ground" );
  talent.sanlayn.vampiric_aura        = find_talent_spell( talent_tree::HERO, "Vampiric Aura" );
  talent.sanlayn.bloody_fortitude     = find_talent_spell( talent_tree::HERO, "Bloody Fortitude" );
  talent.sanlayn.infliction_of_sorrow = find_talent_spell( talent_tree::HERO, "Infliction of Sorrow" );
  talent.sanlayn.frenzied_bloodthirst = find_talent_spell( talent_tree::HERO, "Frenzied Bloodthirst" );
  talent.sanlayn.the_blood_is_life    = find_talent_spell( talent_tree::HERO, "The Blood is Life" );
  talent.sanlayn.visceral_strength    = find_talent_spell( talent_tree::HERO, "Visceral Strength" );
  talent.sanlayn.incite_terror        = find_talent_spell( talent_tree::HERO, "Incite Terror" );
  talent.sanlayn.pact_of_the_sanlayn  = find_talent_spell( talent_tree::HERO, "Pact of the San'layn" );
  talent.sanlayn.sanguine_scent       = find_talent_spell( talent_tree::HERO, "Sanguine Scent" );
  talent.sanlayn.gift_of_the_sanlayn  = find_talent_spell( talent_tree::HERO, "Gift of the San'layn" );

  // Shared
  spec.frost_fever = find_specialization_spell( "Frost Fever" );  // RP generation only

  mastery.blood_shield = find_mastery_spell( DEATH_KNIGHT_BLOOD );
  mastery.frozen_heart = find_mastery_spell( DEATH_KNIGHT_FROST );
  mastery.dreadblade   = find_mastery_spell( DEATH_KNIGHT_UNHOLY );

  // Generic spells
  // Shared
  spell.unholy_strength_buff = find_spell( 53365 );
  spell.brittle_debuff       = conditional_spell_lookup( talent.brittle.ok(), 374557 );
  spell.dnd_buff         = conditional_spell_lookup( spec.death_and_decay->ok() || talent.unholy.defile.ok(), 188290 );
  spell.runic_corruption = conditional_spell_lookup( spec.death_knight->ok(), 51460 );
  spell.runic_empowerment_gain = conditional_spell_lookup( spec.frost_death_knight->ok(), 193486 );
  spell.rune_mastery_buff      = conditional_spell_lookup( talent.rune_mastery.ok(), 374585 );
  spell.coldthirst_gain        = conditional_spell_lookup( talent.coldthirst.ok(), 378849 );
  spell.unholy_ground_buff     = conditional_spell_lookup( talent.unholy_ground.ok(), 374271 );
  spell.death_and_decay_damage =
      conditional_spell_lookup( spec.death_and_decay->ok() || talent.rider.riders_champion.ok(), 52212 );
  spell.death_coil_damage       = conditional_spell_lookup( spec.death_coil->ok(), 47632 );
  spell.death_strike_heal       = conditional_spell_lookup( talent.death_strike.ok(), 45470 );
  spell.sacrificial_pact_damage = conditional_spell_lookup( talent.sacrificial_pact.ok(), 327611 );
  spell.soul_reaper_execute =
      conditional_spell_lookup( talent.soul_reaper.ok() || talent.deathbringer.grim_reaper.ok(), 343295 );
  spell.anti_magic_zone_buff    = conditional_spell_lookup( talent.antimagic_zone.ok(), 396883 );
  spell.frost_shield_buff       = conditional_spell_lookup( talent.permafrost.ok(), 207203 );
  spell.blood_draw_damage       = conditional_spell_lookup( talent.blood_draw.ok(), 374606 );
  spell.blood_draw_cooldown     = conditional_spell_lookup( talent.blood_draw.ok(), 374609 );
  spell.razorice_debuff         = find_spell( 51714 );
  spell.razorice_damage         = find_spell( 50401 );
  spell.rune_of_hysteria_buff   = find_spell( 326918 );

  // Diseases
  spell.blood_plague =
      conditional_spell_lookup( spec.blood_death_knight->ok() || talent.unholy.superstrain.ok(), 55078 );
  spell.frost_fever =
      conditional_spell_lookup( talent.frost.howling_blast.ok() || talent.unholy.superstrain.ok(), 55095 );
  spell.virulent_plague    = conditional_spell_lookup( spec.outbreak->ok(), 191587 );
  spell.virulent_erruption = conditional_spell_lookup( spec.outbreak->ok(), 191685 );

  // Blood
  spell.blood_shield                = conditional_spell_lookup( mastery.blood_shield->ok(), 77535 );
  spell.bloodied_blade_stacks_buff  = conditional_spell_lookup( talent.blood.bloodied_blade->ok(), 460499 );
  spell.bloodied_blade_final_buff   = conditional_spell_lookup( talent.blood.bloodied_blade->ok(), 460500 );
  spell.bone_shield                 = conditional_spell_lookup( spec.blood_death_knight->ok(), 195181 );
  spell.sanguine_ground             = conditional_spell_lookup( talent.blood.sanguine_ground.ok(), 391459 );
  spell.ossuary_buff                = conditional_spell_lookup( talent.blood.ossuary.ok(), 219788 );
  spell.ossified_vitriol_buff       = conditional_spell_lookup( talent.blood.ossified_vitriol.ok(), 458745 );
  spell.crimson_scourge_buff        = conditional_spell_lookup( spec.crimson_scourge->ok(), 81141 );
  spell.heartbreaker_rp_gain        = conditional_spell_lookup( talent.blood.heartbreaker.ok(), 210738 );
  spell.heartrend_buff              = conditional_spell_lookup( talent.blood.heartrend.ok(), 377656 );
  spell.heart_strike_bloodied_blade = conditional_spell_lookup( talent.blood.bloodied_blade.ok(), 460501 );
  spell.perserverence_of_the_ebon_blade_buff =
      conditional_spell_lookup( talent.blood.perseverance_of_the_ebon_blade.ok(), 374748 );
  spell.voracious_buff           = conditional_spell_lookup( talent.blood.voracious.ok(), 274009 );
  spell.bonestorm_heal           = conditional_spell_lookup( talent.blood.bonestorm.ok(), 196545 );
  spell.dancing_rune_weapon_buff = conditional_spell_lookup( talent.blood.dancing_rune_weapon.ok(), 81256 );
  spell.relish_in_blood_gains    = conditional_spell_lookup( talent.blood.relish_in_blood.ok(), 317614 );
  spell.leeching_strike_damage   = conditional_spell_lookup( talent.blood.leeching_strike.ok(), 377633 );
  spell.shattering_bone_damage   = conditional_spell_lookup( talent.blood.shattering_bone.ok(), 377642 );
  // Tier Sets
  spell.unbreakable_tww1_2pc = conditional_spell_lookup( sets->has_set_bonus( DEATH_KNIGHT_BLOOD, TWW1, B2 ), 457468 );
  spell.unbroken_tww1_2pc    = conditional_spell_lookup( sets->has_set_bonus( DEATH_KNIGHT_BLOOD, TWW1, B2 ), 457473 );
  spell.piledriver_tww1_4pc  = conditional_spell_lookup( sets->has_set_bonus( DEATH_KNIGHT_BLOOD, TWW1, B4 ), 457506 );

  // Frost
  spell.murderous_efficiency_gain   = conditional_spell_lookup( talent.frost.murderous_efficiency.ok(), 207062 );
  spell.rage_of_the_frozen_champion = conditional_spell_lookup( talent.frost.rage_of_the_frozen_champion.ok(), 341725 );
  spell.runic_empowerment_chance    = conditional_spell_lookup( spec.frost_death_knight->ok(), 81229 );
  spell.gathering_storm_buff        = conditional_spell_lookup( talent.frost.gathering_storm.ok(), 211805 );
  spell.inexorable_assault_buff     = conditional_spell_lookup( talent.frost.inexorable_assault.ok(), 253595 );
  spell.bonegrinder_crit_buff       = conditional_spell_lookup( talent.frost.bonegrinder.ok(), 377101 );
  spell.bonegrinder_frost_buff      = conditional_spell_lookup( talent.frost.bonegrinder.ok(), 377103 );
  spell.enduring_strength_buff      = conditional_spell_lookup( talent.frost.enduring_strength.ok(), 377195 );
  spell.frostwhelps_aid_buff        = conditional_spell_lookup( talent.frost.frostwhelps_aid.ok(), 377253 );
  spell.inexorable_assault_damage   = conditional_spell_lookup( talent.frost.inexorable_assault.ok(), 253597 );
  spell.breath_of_sindragosa_rune_gen = conditional_spell_lookup( talent.frost.breath_of_sindragosa.ok(), 303753 );
  spell.cold_heart_damage             = conditional_spell_lookup( talent.frost.cold_heart.ok(), 281210 );
  spell.chill_streak_damage           = conditional_spell_lookup( talent.frost.chill_streak.ok(), 204167 );
  spell.death_strike_offhand =
      conditional_spell_lookup( talent.death_strike.ok() && off_hand_weapon.type != WEAPON_NONE, 66188 );
  spell.frostwyrms_fury_damage = conditional_spell_lookup( talent.frost.frostwyrms_fury.ok(), 279303 );
  spell.glacial_advance_damage =
      conditional_spell_lookup( talent.frost.glacial_advance.ok() || talent.frost.arctic_assault.ok(), 195975 );
  spell.avalanche_damage           = conditional_spell_lookup( talent.frost.avalanche.ok(), 207150 );
  spell.frostwhelps_aid_damage     = conditional_spell_lookup( talent.frost.frostwhelps_aid.ok(), 377245 );
  spell.enduring_strength_cooldown = conditional_spell_lookup( talent.frost.enduring_strength.ok(), 377192 );
  spell.piercing_chill_debuff      = conditional_spell_lookup( talent.frost.piercing_chill.ok(), 377359 );
  spell.obliteration_gains         = conditional_spell_lookup( talent.frost.obliteration.ok(), 281327 );
  spell.frost_strike_2h =
      conditional_spell_lookup( talent.frost.frost_strike.ok() && main_hand_weapon.group() == WEAPON_2H, 325464 );
  spell.frost_strike_mh =
      conditional_spell_lookup( talent.frost.frost_strike.ok() && main_hand_weapon.group() == WEAPON_1H, 222026 );
  spell.frost_strike_oh =
      conditional_spell_lookup( talent.frost.frost_strike.ok() && off_hand_weapon.type != WEAPON_NONE, 66196 );
  spell.shattered_frost          = conditional_spell_lookup( talent.frost.shattered_frost.ok(), 455996 );
  spell.icy_death_torrent_damage = conditional_spell_lookup( talent.frost.icy_death_torrent.ok(), 439539 );
  spell.cryogenic_chamber_damage = conditional_spell_lookup( talent.frost.cryogenic_chamber.ok(), 456371 );
  spell.cryogenic_chamber_buff   = conditional_spell_lookup( talent.frost.cryogenic_chamber.ok(), 456370 );
  spell.rime_buff                = conditional_spell_lookup( spec.rime->ok(), 59052 );
  spell.hyperpyrexia_damage      = conditional_spell_lookup( talent.frost.hyperpyrexia.ok(), 458169 );
  // Tier Sets
  spell.icy_vigor       = conditional_spell_lookup( sets->has_set_bonus( DEATH_KNIGHT_FROST, TWW1, B4 ), 457189 );

  // Unholy
  spell.runic_corruption_chance        = conditional_spell_lookup( spec.unholy_death_knight->ok(), 51462 );
  spell.festering_wound_debuff         = conditional_spell_lookup( spec.festering_wound->ok(), 194310 );
  spell.rotten_touch_debuff            = conditional_spell_lookup( talent.unholy.rotten_touch.ok(), 390276 );
  spell.death_rot_debuff               = conditional_spell_lookup( talent.unholy.death_rot.ok(), 377540 );
  spell.coil_of_devastation_debuff     = conditional_spell_lookup( talent.unholy.coil_of_devastation.ok(), 390271 );
  spell.ghoulish_frenzy_player         = conditional_spell_lookup( talent.unholy.ghoulish_frenzy.ok(), 377588 );
  spell.plaguebringer_buff             = conditional_spell_lookup( talent.unholy.plaguebringer.ok(), 390178 );
  spell.festermight_buff               = conditional_spell_lookup( talent.unholy.festermight.ok(), 377591 );
  spell.unholy_blight                  = conditional_spell_lookup( talent.unholy.unholy_blight.ok(), 115989 );
  spell.unholy_blight_dot              = conditional_spell_lookup( talent.unholy.unholy_blight.ok(), 115994 );
  spell.commander_of_the_dead          = conditional_spell_lookup( talent.unholy.commander_of_the_dead.ok(), 390260 );
  spell.ruptured_viscera_chance        = conditional_spell_lookup( talent.unholy.ruptured_viscera.ok(), 390236 );
  spell.apocalypse_duration            = conditional_spell_lookup( talent.unholy.apocalypse.ok(), 221180 );
  spell.apocalypse_rune_gen            = conditional_spell_lookup( talent.unholy.apocalypse.ok(), 343758 );
  spell.unholy_pact_damage             = conditional_spell_lookup( talent.unholy.unholy_pact.ok(), 319236 );
  spell.dark_transformation_damage     = conditional_spell_lookup( talent.unholy.dark_transformation.ok(), 344955 );
  spell.defile_damage                  = conditional_spell_lookup( talent.unholy.defile.ok(), 156000 );
  spell.epidemic_damage                = conditional_spell_lookup( spec.epidemic->ok(), 212739 );
  spell.bursting_sores_damage          = conditional_spell_lookup( talent.unholy.bursting_sores.ok(), 207267 );
  spell.festering_wound_damage         = conditional_spell_lookup( spec.festering_wound->ok(), 194311 );
  spell.outbreak_aoe                   = conditional_spell_lookup( spec.outbreak->ok(), 196780 );
  spell.unholy_aura_debuff             = conditional_spell_lookup( talent.unholy.unholy_aura.ok(), 377445 );
  spell.decomposition_buff             = conditional_spell_lookup( talent.unholy.decomposition.ok(), 458233 );
  spell.decomposition_damage           = conditional_spell_lookup( talent.unholy.decomposition.ok(), 458264 );
  spell.festering_scythe               = conditional_spell_lookup( talent.unholy.festering_scythe.ok(), 458128 );
  spell.festering_scythe_buff          = conditional_spell_lookup( talent.unholy.festering_scythe.ok(), 458123 );
  spell.festering_scythe_stacking_buff = conditional_spell_lookup( talent.unholy.festering_scythe.ok(), 459238 );
  // Set Bonuses
  spell.unholy_commander  = conditional_spell_lookup( sets->has_set_bonus( DEATH_KNIGHT_UNHOLY, TWW1, B4 ), 456698 );

  // Rider of the Apocalypse Spells
  spell.a_feast_of_souls_buff = conditional_spell_lookup( talent.rider.a_feast_of_souls.ok(), 440861 );
  spell.summon_mograine       = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444248 );
  spell.summon_whitemane      = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444251 );
  spell.summon_trollbane      = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444254 );
  spell.summon_nazgrim        = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444252 );
  spell.summon_mograine_2     = conditional_spell_lookup( talent.rider.riders_champion.ok(), 454393 );
  spell.summon_whitemane_2    = conditional_spell_lookup( talent.rider.riders_champion.ok(), 454389 );
  spell.summon_trollbane_2    = conditional_spell_lookup( talent.rider.riders_champion.ok(), 454390 );
  spell.summon_nazgrim_2      = conditional_spell_lookup( talent.rider.riders_champion.ok(), 454392 );
  spell.apocalypse_now_data   = conditional_spell_lookup( talent.rider.apocalypse_now.ok(), 444244 );
  spell.death_charge_action   = conditional_spell_lookup( talent.rider.death_charge.ok(), 444347 );

  // San'layn Spells
  spell.vampiric_strike                 = conditional_spell_lookup( talent.sanlayn.vampiric_strike.ok(), 433895 );
  spell.vampiric_strike_buff            = conditional_spell_lookup( talent.sanlayn.vampiric_strike.ok(), 433899 );
  spell.essence_of_the_blood_queen_buff = conditional_spell_lookup( talent.sanlayn.vampiric_strike.ok(), 433925 );
  spell.gift_of_the_sanlayn_buff        = conditional_spell_lookup( talent.sanlayn.gift_of_the_sanlayn.ok(), 434153 );
  spell.vampiric_strike_heal            = conditional_spell_lookup( talent.sanlayn.vampiric_strike.ok(), 434422 );
  spell.infliction_of_sorrow_damage     = conditional_spell_lookup( talent.sanlayn.infliction_of_sorrow.ok(), 434144 );
  spell.infliction_of_sorrow_buff       = conditional_spell_lookup( talent.sanlayn.infliction_of_sorrow.ok(), 460049 );
  spell.blood_beast_summon              = conditional_spell_lookup( talent.sanlayn.the_blood_is_life.ok(), 434237 );
  spell.vampiric_strike_clawing_shadows =
      conditional_spell_lookup( talent.sanlayn.vampiric_strike.ok() && talent.unholy.clawing_shadows.ok(), 445669 );
  spell.incite_terror_debuff   = conditional_spell_lookup( talent.sanlayn.incite_terror.ok(), 458478 );
  spell.visceral_strength_buff = conditional_spell_lookup( talent.sanlayn.visceral_strength.ok(),
                                                           specialization() == DEATH_KNIGHT_BLOOD ? 461130 : 434159 );
  spell.bloodsoaked_ground_buff = conditional_spell_lookup( talent.sanlayn.bloodsoaked_ground.ok(), 434034 );

  // Deathbringer Spells
  spell.reapers_mark_debuff            = conditional_spell_lookup( talent.deathbringer.reapers_mark.ok(), 434765 );
  spell.reapers_mark_explosion         = conditional_spell_lookup( talent.deathbringer.reapers_mark.ok(), 436304 );
  spell.grim_reaper                    = conditional_spell_lookup( talent.deathbringer.grim_reaper.ok(), 443761 );
  spell.wave_of_souls_damage           = conditional_spell_lookup( talent.deathbringer.wave_of_souls.ok(), 435802 );
  spell.wave_of_souls_debuff           = conditional_spell_lookup( talent.deathbringer.wave_of_souls.ok(), 443404 );
  spell.blood_fever_damage             = conditional_spell_lookup( talent.deathbringer.blood_fever.ok(), 440005 );
  spell.bind_in_darkness_buff          = conditional_spell_lookup( talent.deathbringer.bind_in_darkness.ok(), 443532 );
  spell.dark_talons_shadowfrost_buff   = conditional_spell_lookup( talent.deathbringer.dark_talons.ok(), 443586 );
  spell.dark_talons_icy_talons_buff    = conditional_spell_lookup( talent.deathbringer.dark_talons.ok(), 443595 );
  spell.soul_rupture_damage            = conditional_spell_lookup( talent.deathbringer.soul_rupture.ok(), 439594 );
  spell.grim_reaper_soul_reaper        = conditional_spell_lookup( talent.deathbringer.grim_reaper.ok(), 448229 );
  spell.exterminate_damage             = conditional_spell_lookup( talent.deathbringer.exterminate.ok(), 441424 );
  spell.exterminate_aoe                = conditional_spell_lookup( talent.deathbringer.exterminate.ok(), 441426 );
  spell.exterminate_buff               = conditional_spell_lookup( talent.deathbringer.exterminate.ok(), 441416 );
  spell.exterminate_buff_painful_death = conditional_spell_lookup( talent.deathbringer.exterminate.ok(), 447954 );

  // Pet abilities
  // Raise Dead abilities, used for both rank 1 and rank 2
  pet_spell.ghoul_claw     = conditional_spell_lookup( talent.raise_dead.ok(), 91776 );
  pet_spell.sweeping_claws = conditional_spell_lookup( talent.unholy.dark_transformation.ok(), 91778 );
  pet_spell.gnaw           = conditional_spell_lookup( talent.raise_dead.ok(), 91800 );
  pet_spell.monstrous_blow = conditional_spell_lookup( talent.unholy.dark_transformation.ok(), 91797 );
  // Army of the dead
  pet_spell.army_claw =
      conditional_spell_lookup( talent.unholy.army_of_the_dead.ok() || talent.unholy.apocalypse.ok(), 199373 );
  // All Ghouls
  pet_spell.pet_stun = find_spell( 47466 );
  // Gargoyle
  pet_spell.gargoyle_strike  = conditional_spell_lookup( talent.unholy.summon_gargoyle.ok(), 51963 );
  pet_spell.dark_empowerment = conditional_spell_lookup( talent.unholy.summon_gargoyle.ok(), 211947 );
  // Risen Skulker (all will serve)
  pet_spell.skulker_shot = conditional_spell_lookup( talent.unholy.all_will_serve.ok(), 212423 );
  // Magus of the dead (army of the damned)
  pet_spell.frostbolt =
      conditional_spell_lookup( talent.unholy.magus_of_the_dead.ok() || talent.unholy.doomed_bidding.ok(), 317792 );
  pet_spell.shadow_bolt =
      conditional_spell_lookup( talent.unholy.magus_of_the_dead.ok() || talent.unholy.doomed_bidding.ok(), 317791 );
  // Commander of the Dead Talent
  pet_spell.commander_of_the_dead = conditional_spell_lookup( talent.unholy.commander_of_the_dead.ok(), 390264 );
  // Ruptured Viscera Talent
  pet_spell.ruptured_viscera = conditional_spell_lookup( talent.unholy.ruptured_viscera.ok(), 390220 );
  // Ghoulish Frenzy
  pet_spell.ghoulish_frenzy = conditional_spell_lookup( talent.unholy.ghoulish_frenzy.ok(), 377589 );
  // DRW Spells
  pet_spell.drw_heart_strike =
      conditional_spell_lookup( talent.blood.heart_strike.ok() && talent.blood.dancing_rune_weapon.ok(), 228645 );
  pet_spell.drw_heart_strike_rp_gen =
      conditional_spell_lookup( talent.blood.heart_strike.ok() && talent.blood.dancing_rune_weapon.ok(), 220890 );
  // Rider of the Apocalypse Pet Spells
  pet_spell.apocalyptic_conquest             = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444763 );
  pet_spell.trollbanes_chains_of_ice_ability = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444826 );
  pet_spell.trollbanes_chains_of_ice_debuff  = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444828 );
  pet_spell.trollbanes_icy_fury_ability   = conditional_spell_lookup( talent.rider.trollbanes_icy_fury.ok(), 444834 );
  pet_spell.undeath_dot                   = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444633 );
  pet_spell.undeath_range                 = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444634 );
  pet_spell.mograines_death_and_decay     = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444474 );
  pet_spell.mograines_might_buff          = conditional_spell_lookup( talent.rider.mograines_might.ok(), 444505 );
  pet_spell.rider_ams                     = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444741 );
  pet_spell.rider_ams_icd                 = conditional_spell_lookup( talent.rider.horsemens_aid.ok(), 451777 );
  pet_spell.whitemane_death_coil          = conditional_spell_lookup( talent.rider.riders_champion.ok(), 445513 );
  pet_spell.mograine_heart_strike         = conditional_spell_lookup( talent.rider.riders_champion.ok(), 445504 );
  pet_spell.trollbane_obliterate          = conditional_spell_lookup( talent.rider.riders_champion.ok(), 445507 );
  pet_spell.nazgrim_scourge_strike_phys   = conditional_spell_lookup( talent.rider.riders_champion.ok(), 445508 );
  pet_spell.nazgrim_scourge_strike_shadow = conditional_spell_lookup( talent.rider.riders_champion.ok(), 445509 );
  // San'layn Pet Spells
  pet_spell.corrupted_blood = conditional_spell_lookup( talent.sanlayn.the_blood_is_life.ok(), 434574 );
  pet_spell.blood_eruption  = conditional_spell_lookup( talent.sanlayn.the_blood_is_life.ok(), 434246 );
  // Abomination Spells
  pet_spell.abomination_disease_cloud = conditional_spell_lookup( talent.unholy.raise_abomination.ok(), 290577 );

  // Custom/Internal cooldowns default durations
  cooldown.bone_shield_icd->duration = spell.bone_shield->internal_cooldown();

  if ( talent.frost.enduring_strength.ok() )
    cooldown.enduring_strength_icd->duration = spell.enduring_strength_cooldown->internal_cooldown();

  if ( talent.frost.inexorable_assault.ok() )
    cooldown.inexorable_assault_icd->duration =
        spell.inexorable_assault_buff->internal_cooldown();  // Inexorable Assault buff spell id

  if ( talent.frost.frigid_executioner.ok() )
    cooldown.frigid_executioner_icd->duration = talent.frost.frigid_executioner->internal_cooldown();

  if( talent.rider.whitemanes_famine.ok() )
    cooldown.undeath_spread->base_duration = pet_spell.undeath_dot->internal_cooldown();
}

// death_knight_t::init_action_list =========================================

void death_knight_t::init_action_list()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim->errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( main_hand_weapon.group() == WEAPON_2H && off_hand_weapon.type != WEAPON_NONE )
  {
    if ( !quiet )
      sim->errorf( "Player %s has an Off-Hand weapon equipped with a 2h.", name() );
    quiet = true;
    return;
  }

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();

  switch ( specialization() )
  {
    case DEATH_KNIGHT_UNHOLY:
      death_knight_apl::unholy( this );
      break;
    case DEATH_KNIGHT_FROST:
      death_knight_apl::frost( this );
      break;
    case DEATH_KNIGHT_BLOOD:
      death_knight_apl::blood( this );
      break;
    default:
      break;
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// death_knight_t::init_scaling =============================================

void death_knight_t::init_scaling()
{
  player_t::init_scaling();

  if ( off_hand_weapon.type != WEAPON_NONE )
    scaling->enable( STAT_WEAPON_OFFHAND_DPS );

  if ( specialization() == DEATH_KNIGHT_BLOOD )
    scaling->enable( STAT_BONUS_ARMOR );

  scaling->disable( STAT_AGILITY );
}

// Create DoTs and Debuffs ===================================================
template <typename Buff, typename... Args>
inline buff_t* death_knight_td_t::make_debuff( bool b, Args&&... args )
{
  return buff_t::make_buff_fallback<Buff>( target->is_enemy() && b, std::forward<Args>( args )... );
}
inline death_knight_td_t::death_knight_td_t( player_t& target, death_knight_t& p ) : actor_target_data_t( &target, &p )
{
  // Diseases
  dot.blood_plague    = target.get_dot( "blood_plague", &p );
  dot.frost_fever     = target.get_dot( "frost_fever", &p );
  dot.virulent_plague = target.get_dot( "virulent_plague", &p );
  dot.unholy_blight   = target.get_dot( "unholy_blight_dot", &p );
  // Other dots
  dot.soul_reaper = target.get_dot( "soul_reaper", &p );

  // Rider of the Apocalypse
  dot.undeath = target.get_dot( "undeath", &p );

  using namespace debuffs;
  // General Talents
  debuff.brittle = make_debuff( p.talent.brittle.ok(), *this, "brittle", p.spell.brittle_debuff )
                       ->set_default_value_from_effect( 1 );

  // Blood
  debuff.mark_of_blood =
      make_debuff( p.talent.blood.mark_of_blood.ok(), *this, "mark_of_blood", p.talent.blood.mark_of_blood )
          ->set_cooldown( 0_ms );  // Handled by the action

  // Frost
  debuff.razorice = make_debuff( p.runeforge.rune_of_razorice_mh || p.runeforge.rune_of_razorice_oh ||
                                     p.talent.frost.avalanche->ok() || p.talent.frost.glacial_advance->ok() ||
                                     p.talent.frost.arctic_assault->ok(),
                                 *this, "razorice", p.spell.razorice_debuff )
                        ->set_default_value_from_effect( 1 )
                        ->set_period( 0_ms )
                        ->apply_affecting_aura( p.talent.unholy_bond );

  debuff.piercing_chill =
      make_debuff( p.talent.frost.piercing_chill.ok(), *this, "piercing_chill", p.spell.piercing_chill_debuff )
          ->set_default_value_from_effect( 1 );

  debuff.everfrost =
      make_debuff( p.talent.frost.everfrost.ok(), *this, "everfrost", p.talent.frost.everfrost->effectN( 1 ).trigger() )
          ->set_default_value( p.talent.frost.everfrost->effectN( 1 ).percent() );

  debuff.chill_streak =
      make_buff_fallback( p.talent.frost.chill_streak.ok(), *this, "chill_streak", p.spell.chill_streak_damage )
          ->set_quiet( true )
          ->set_expire_callback( [ & ]( buff_t*, int, timespan_t d ) {
            // Chill streak doesnt bounce if the target dies before the debuff expires
            if ( d == timespan_t::zero() )
            {
              p.chill_streak_bounce( target );
            }
          } );

  // Unholy
  debuff.festering_wound =
      make_debuff( p.spec.festering_wound->ok(), *this, "festering_wound", p.spell.festering_wound_debuff )
          ->set_cooldown( 0_ms )  // Handled by death_knight_t::trigger_festering_wound()
          ->set_stack_change_callback( [ & ]( buff_t*, int old_, int new_ ) {
            // Update the FW target count if needed
            if ( old_ == 0 )
              p.festering_wounds_target_count++;
            else if ( new_ == 0 )
              p.festering_wounds_target_count--;
          } );

  debuff.rotten_touch =
      make_debuff( p.talent.unholy.rotten_touch.ok(), *this, "rotten_touch", p.spell.rotten_touch_debuff )
          ->set_default_value_from_effect( 1 );

  debuff.death_rot = make_debuff( p.talent.unholy.death_rot.ok(), *this, "death_rot", p.spell.death_rot_debuff )
                         ->set_default_value_from_effect( 1 );

  debuff.unholy_aura = make_debuff( p.talent.unholy.unholy_aura.ok(), *this, "unholy_aura", p.spell.unholy_aura_debuff )
                           ->set_duration( 0_ms )  // Handled by a combat state callback trigger
                           ->apply_affecting_aura( p.talent.unholy.unholy_aura );

  debuff.decomposition = make_buff_fallback<decomposition_debuff_t>( p.talent.unholy.decomposition.ok(), *this,
                                                                     "decomposition", p.spell.decomposition_buff );

  // Apocalypse Death Knight Runeforge Debuffs
  debuff.apocalypse_death = make_debuff( p.runeforge.rune_of_apocalypse, *this, "death",
                                         p.spell.apocalypse_death_debuff )  // Effect not implemented
                                ->apply_affecting_aura( p.talent.unholy_bond );

  debuff.apocalypse_famine =
      make_debuff( p.runeforge.rune_of_apocalypse, *this, "famine", p.spell.apocalypse_famine_debuff )
          ->set_default_value_from_effect( 1 )
          ->apply_affecting_aura( p.talent.unholy_bond );

  debuff.apocalypse_war = make_debuff( p.runeforge.rune_of_apocalypse, *this, "war", p.spell.apocalypse_war_debuff )
                              ->set_default_value_from_effect( 1 )
                              ->apply_affecting_aura( p.talent.unholy_bond );

  // Rider of the Apocalypse Debuffs
  debuff.chains_of_ice_trollbane_slow =
      make_debuff( p.talent.rider.riders_champion.ok(), *this, "chains_of_ice_trollbane_slow",
                   p.pet_spell.trollbanes_chains_of_ice_ability );

  debuff.chains_of_ice_trollbane_damage =
      make_debuff( p.talent.rider.riders_champion.ok(), *this, "chains_of_ice_trollbane_debuff",
                   p.pet_spell.trollbanes_chains_of_ice_debuff )
          ->set_default_value_from_effect( 1 );

  // Deathbringer
  // TODO-TWW confirm refresh behavior with swift end?
  debuff.reapers_mark =
      make_debuff( p.talent.deathbringer.reapers_mark.ok(), *this, "reapers_mark_debuff", p.spell.reapers_mark_debuff )
          ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
          ->set_expire_at_max_stack( true )
          ->set_can_cancel( true )
          ->set_freeze_stacks( true )
          ->set_expire_callback( [ & ]( buff_t* buff, int stacks, timespan_t ) {
            if( !p.sim->event_mgr.canceled )
              p.reapers_mark_explosion_wrapper( buff->player, buff->source, stacks );
          } )
          ->set_tick_callback( [ & ]( buff_t* buff, int, timespan_t ) {
            // 5/7/24 the 35% appears to be in a server script
            // the explosion is triggering anytime the target is below 35%, if the hidden grim reaper buff is down, instantly popping fresh marks
            // trigger is on a one second dummy periodic, giving a slight window to acquire stacks
            size_t targets = p.sim->target_non_sleeping_list.size();
            if ( !p.buffs.grim_reaper->check() && targets == 1 && buff->player->health_percentage() < 35 )
            {
              p.sim->print_debug( "reapers_mark go boom" );
              p.buffs.grim_reaper->trigger();
              make_event( p.sim, 0_ms, [ buff ]() { buff->expire(); } );
            }
          } );

  debuff.wave_of_souls = make_debuff( p.talent.deathbringer.wave_of_souls.ok(), *this, "wave_of_souls_debuff",
                                      p.spell.wave_of_souls_debuff )
                             ->set_default_value_from_effect( 1 );

  // San'layn
  debuff.incite_terror =
      make_debuff( p.talent.sanlayn.incite_terror.ok(), *this, "incite_terror", p.spell.incite_terror_debuff );
}

// death_knight_t::create_buffs ===============================================
template <typename Buff = buffs::death_knight_buff_t, typename... Args>
static inline buff_t* make_fallback( Args&&... args )
{
  return buff_t::make_buff_fallback<Buff>( std::forward<Args>( args )... );
}

void death_knight_t::create_buffs()
{
  player_t::create_buffs();
  using namespace buffs;

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  // Shared
  buffs.antimagic_shell = make_fallback<antimagic_shell_buff_t>( spec.antimagic_shell->ok(), this, "antimagic_shell",
                                                                 spec.antimagic_shell );

  buffs.antimagic_zone = make_fallback<antimagic_zone_buff_t>( talent.antimagic_zone.ok(), this, "antimagic_zone",
                                                               spell.anti_magic_zone_buff );

  buffs.blood_draw =
      make_fallback( talent.blood_draw.ok(), this, "blood_draw", talent.blood_draw->effectN( 4 ).trigger() );

  buffs.icebound_fortitude =
      make_fallback( talent.icebound_fortitude.ok(), this, "icebound_fortitude", talent.icebound_fortitude )
          ->set_duration( talent.icebound_fortitude->duration() )
          ->set_cooldown( 0_ms );  // Handled by the action

  buffs.rune_mastery = make_fallback( talent.rune_mastery.ok(), this, "rune_mastery", spell.rune_mastery_buff )
                           ->set_chance( 0.15 )  // This was found through testing 2022 July 21.  Not in spelldata.
                           ->set_default_value( talent.rune_mastery->effectN( 1 ).percent() );

  buffs.unholy_strength = make_buff( this, "unholy_strength", spell.unholy_strength_buff )
                              ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE );

  buffs.unholy_ground = make_fallback( talent.unholy_ground.ok(), this, "unholy_ground", spell.unholy_ground_buff )
                            ->set_default_value_from_effect( 1 )
                            ->set_duration( 0_ms );  // Handled by trigger_dnd_buffs() & expire_dnd_buffs()

  buffs.abomination_limb =
      make_fallback( talent.abomination_limb.ok(), this, "abomination_limb", talent.abomination_limb )
          ->set_cooldown( 0_ms )  // Handled by the action
          ->set_partial_tick( true )
          ->set_tick_callback( [ this ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ ) {
            active_spells.abomination_limb_damage->execute();
          } );

  buffs.icy_talons =
      make_fallback( talent.icy_talons.ok(), this, "icy_talons", talent.icy_talons->effectN( 1 ).trigger() )
          ->set_default_value( talent.icy_talons->effectN( 1 ).percent() )
          ->set_cooldown( talent.icy_talons->internal_cooldown() )
          ->set_trigger_spell( talent.icy_talons )
          ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
            if ( talent.deathbringer.dark_talons.ok() )
            {
              if ( new_ == 1 )
              {
                buffs.dark_talons_shadowfrost->trigger();
              }
              else if ( new_ == 0 )
              {
                buffs.dark_talons_shadowfrost->expire();
                buffs.dark_talons_icy_talons->expire();
              }
            }
          } );

  buffs.death_and_decay = make_fallback<death_and_decay_buff_t>(
      spec.death_and_decay->ok() || talent.unholy.defile.ok(), this, "death_and_decay", spell.dnd_buff );

  buffs.frost_shield = make_fallback<death_knight_absorb_buff_t>( talent.permafrost.ok(), this, "frost_shield",
                                                                  spell.frost_shield_buff );
  if ( talent.permafrost.ok() )
  {
    // Mildly annoying workaround since make_fallback is of type buff_t, not absorb_buff_t.
    debug_cast<death_knight_absorb_buff_t*>( buffs.frost_shield )->set_absorb_high_priority( true );
  }

  buffs.runic_corruption =
      make_fallback<runic_corruption_buff_t>( talent.soul_reaper.ok() || specialization() == DEATH_KNIGHT_UNHOLY, this,
                                              "runic_corruption", spell.runic_corruption );

  buffs.rune_of_hysteria = make_buff( this, "rune_of_hysteria", spell.rune_of_hysteria_buff )
                               ->set_default_value_from_effect( 1 )
                               ->apply_affecting_aura( talent.unholy_bond );

  // Rider of the Apocalypse
  buffs.antimagic_shell_horsemen = make_fallback<antimagic_shell_buff_horseman_t>(
      talent.rider.horsemens_aid.ok(), this, "antimagic_shell_horsemen", pet_spell.rider_ams );

  buffs.antimagic_shell_horsemen_icd =
      make_fallback( talent.rider.horsemens_aid.ok(), this, "antimagic_shell_horsemen_icd", pet_spell.rider_ams_icd )
          ->set_quiet( true );

  buffs.apocalyptic_conquest = make_fallback<apocalyptic_conquest_buff_t>(
      talent.rider.riders_champion.ok(), this, "apocalyptic_conquest", pet_spell.apocalyptic_conquest );

  buffs.mograines_might =
      make_fallback( talent.rider.mograines_might.ok(), this, "mograines_might", pet_spell.mograines_might_buff )
          ->set_default_value_from_effect( 1 );

  buffs.a_feast_of_souls =
      make_fallback( talent.rider.a_feast_of_souls.ok(), this, "a_feast_of_souls", spell.a_feast_of_souls_buff )
          ->set_duration( 0_ms )
          ->set_default_value_from_effect( 1 );

  // Deathbringer
  buffs.grim_reaper =
      make_fallback( talent.deathbringer.grim_reaper.ok(), this, "grim_reaper", spell.grim_reaper )->set_quiet( true );

  buffs.bind_in_darkness =
      make_fallback( talent.deathbringer.bind_in_darkness.ok(), this, "bind_in_darkness", spell.bind_in_darkness_buff )
          ->set_trigger_spell( talent.deathbringer.bind_in_darkness );

  buffs.dark_talons_shadowfrost = make_fallback(
      talent.deathbringer.dark_talons.ok(), this, "dark_talons_shadowfrost", spell.dark_talons_shadowfrost_buff )
          ->set_quiet( true );

  buffs.dark_talons_icy_talons =
      make_fallback( talent.deathbringer.dark_talons.ok(), this, "dark_talons", spell.dark_talons_icy_talons_buff )
          ->set_stack_change_callback( [ this ]( buff_t*, int old_, int new_ ) {
            int it_stack_modifier = as<int>( talent.deathbringer.dark_talons->effectN( 2 ).base_value() );
            if ( new_ > old_ )
            {
              buffs.icy_talons->set_max_stack( buffs.icy_talons->max_stack() + it_stack_modifier );
            }
            else if ( new_ < old_ )
            {
              buffs.icy_talons->set_max_stack( buffs.icy_talons->max_stack() -
                                               ( ( old_ - new_ ) * it_stack_modifier ) );
            }
          } );

  buffs.exterminate = make_fallback( talent.deathbringer.exterminate.ok(), this, "exterminate", spell.exterminate_buff );

  buffs.exterminate_painful_death = make_fallback( talent.deathbringer.painful_death.ok(), this, "exterminate_painful_death",
                                       spell.exterminate_buff_painful_death );

  // San'layn
  buffs.essence_of_the_blood_queen = make_fallback<essence_of_the_blood_queen_haste_buff_t>(
      talent.sanlayn.vampiric_strike.ok(), this, "essence_of_the_blood_queen", spell.essence_of_the_blood_queen_buff );

  buffs.essence_of_the_blood_queen_damage =
      make_fallback<essence_of_the_blood_queen_damage_buff_t>( talent.sanlayn.vampiric_strike.ok(), this,
                                                               "essence_of_the_blood_queen_damage",
                                                               spell.essence_of_the_blood_queen_buff )
          ->set_quiet( true );

  buffs.gift_of_the_sanlayn = make_fallback<gift_of_the_sanlayn_buff_t>( talent.sanlayn.gift_of_the_sanlayn.ok(), this, "gift_of_the_sanlayn",
                                             spell.gift_of_the_sanlayn_buff );


  buffs.vampiric_strike =
      make_fallback( talent.sanlayn.vampiric_strike.ok(), this, "vampiric_strike", spell.vampiric_strike_buff );

  buffs.infliction_of_sorrow = make_fallback( talent.sanlayn.infliction_of_sorrow, this, "infliction_of_sorrow",
                                              spell.infliction_of_sorrow_buff );

  buffs.visceral_strength =
      make_fallback( talent.sanlayn.visceral_strength, this, "visceral_strength", spell.visceral_strength_buff )
          ->set_default_value_from_effect_type( A_MOD_PERCENT_STAT )
          ->add_invalidate( CACHE_STRENGTH);
          // ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );  // TODO bugged should be A_MOD_TOTAL_STAT_PERCENTAGE (137)

  buffs.bloodsoaked_ground = make_fallback( talent.sanlayn.bloodsoaked_ground.ok(), this, "bloodsoaked_ground",
                                            spell.bloodsoaked_ground_buff );

  // Blood
  if ( this->specialization() == DEATH_KNIGHT_BLOOD )
  {
    buffs.blood_shield = make_buff<blood_shield_buff_t>( this );

    buffs.bone_shield =
        make_buff( this, "bone_shield", spell.bone_shield )
            ->set_default_value_from_effect_type( A_MOD_ARMOR_BY_PRIMARY_STAT_PCT )
            ->set_stack_change_callback( [ this ]( buff_t*, int old_stacks, int new_stacks ) {
              if ( talent.blood.foul_bulwark.ok() )  // Change player's max health if FB is talented
              {
                double fb_health     = talent.blood.foul_bulwark->effectN( 1 ).percent();
                double health_change = ( 1.0 + new_stacks * fb_health ) / ( 1.0 + old_stacks * fb_health );

                resources.initial_multiplier[ RESOURCE_HEALTH ] *= health_change;

                recalculate_resource_max( RESOURCE_HEALTH );
              }

              // Trigger/cancel ossuary
              // Trigger is done regardless of current stacks to feed the 'refresh' data, but requires a stack gain
              if ( talent.blood.ossuary.ok() && new_stacks > old_stacks &&
                   new_stacks >= talent.blood.ossuary->effectN( 1 ).base_value() )
                buffs.ossuary->trigger();
              // Only expire if the buff is already up
              else if ( buffs.ossuary->up() && new_stacks < talent.blood.ossuary->effectN( 1 ).base_value() )
                buffs.ossuary->expire();

              // If the buff starts or expires, invalidate relevant caches
              if ( ( !old_stacks && new_stacks ) || ( old_stacks && !new_stacks ) )
              {
                invalidate_cache( CACHE_BONUS_ARMOR );
              }
            } )
            // The internal cd in spelldata is for stack loss, handled in bone_shield_handler
            ->set_cooldown( 0_ms )
            ->set_max_stack( spell.bone_shield->max_stacks() + as<int>( talent.blood.reinforced_bones.ok() ? talent.blood.reinforced_bones->effectN( 2 ).base_value() : 0 ) );  // TODO: Remove this if they fix reinforced bones effect2 to be APPLY_AURA instead of E_APPLY_AREA_AURA_PARTY

    buffs.bloodied_blade_stacks = make_buff( this, "bloodied_blade_stacks", spell.bloodied_blade_stacks_buff )
                                      ->set_default_value( spell.bloodied_blade_stacks_buff->effectN( 1 ).base_value() / 10 )
                                      ->add_invalidate( CACHE_STRENGTH )
                                      ->set_cooldown( spell.bloodied_blade_stacks_buff->internal_cooldown() );
                                      // ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH ); // TODO bugged should be A_MOD_TOTAL_STAT_PERCENTAGE (137)

    buffs.bloodied_blade_final  = make_buff( this, "bloodied_blade_final", spell.bloodied_blade_final_buff )
                                      ->set_default_value_from_effect_type( A_MOD_PERCENT_STAT )  // TODO bugged should be A_MOD_TOTAL_STAT_PERCENTAGE (137)
                                      ->add_invalidate( CACHE_STRENGTH );
                                      // ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );


    buffs.ossuary = make_buff( this, "ossuary", spell.ossuary_buff )->set_default_value_from_effect( 1, 0.1 );

    buffs.ossified_vitriol = make_buff( this, "ossified_vitriol", spell.ossified_vitriol_buff );

    buffs.coagulopathy = make_buff( this, "coagulopathy", talent.blood.coagulopathy->effectN( 2 ).trigger() )
                             ->set_trigger_spell( talent.blood.coagulopathy )
                             ->set_default_value_from_effect( 1 );

    buffs.consumption = make_buff( this, "consumption", talent.blood.consumption );

    buffs.crimson_scourge =
        make_buff( this, "crimson_scourge", spell.crimson_scourge_buff )->set_trigger_spell( spec.crimson_scourge );

    buffs.dancing_rune_weapon = make_buff( this, "dancing_rune_weapon", spell.dancing_rune_weapon_buff )
                                    ->set_cooldown( 0_ms )
                                    ->set_duration( 0_ms )
                                    ->set_default_value_from_effect_type( A_MOD_PARRY_PERCENT );

    buffs.heartrend = make_buff( this, "heartrend", spell.heartrend_buff )
                          ->set_default_value( talent.blood.heartrend->effectN( 1 ).percent() )
                          ->set_chance( 0.10 );  // Sept 20 2022.  Chance was found via testing for 30 mins.  Other
    // people have mentioned the same proc rate.  Not in spelldata.

    buffs.hemostasis = make_buff( this, "hemostasis", talent.blood.hemostasis->effectN( 1 ).trigger() )
                           ->set_trigger_spell( talent.blood.hemostasis )
                           ->set_default_value_from_effect( 1 );

    buffs.perseverance_of_the_ebon_blade =
        make_buff( this, "perseverance_of_the_ebon_blade", spell.perserverence_of_the_ebon_blade_buff );

    buffs.rune_tap =
        make_buff( this, "rune_tap", talent.blood.rune_tap )->set_cooldown( 0_ms );  // Handled by the action

    buffs.sanguine_ground = make_buff( this, "sanguine_ground", spell.sanguine_ground )
                                ->set_default_value_from_effect( 1 )
                                ->set_duration( 0_ms )  // Handled by trigger_dnd_buffs() & expire_dnd_buffs()
                                ->set_schools_from_effect( 1 );

    buffs.tombstone = make_buff<absorb_buff_t>( this, "tombstone", talent.blood.tombstone )
                          ->set_cooldown( 0_ms );  // Handled by the action

    buffs.vampiric_blood =
        make_buff( this, "vampiric_blood", talent.blood.vampiric_blood )
            ->set_cooldown( 0_ms )
            ->set_duration( talent.blood.vampiric_blood->duration() +
                            talent.blood.improved_vampiric_blood->effectN( 3 ).time_value() )
            ->set_default_value_from_effect( 5 )
            ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
              double old_health     = resources.current[ RESOURCE_HEALTH ];
              double old_max_health = resources.max[ RESOURCE_HEALTH ];
              auto health_change    = talent.blood.vampiric_blood->effectN( 4 ).percent();

              if ( new_ )
              {
                resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + health_change;
                recalculate_resource_max( RESOURCE_HEALTH );
                resources.current[ RESOURCE_HEALTH ] *=
                    1.0 + health_change;  // Update health after the maximum is increased

                sim->print_debug(
                    "{} gains Vampiric Blood: health pct change {}%, current health: {} -> {}, max: {} -> {}", name(),
                    health_change * 100.0, old_health, resources.current[ RESOURCE_HEALTH ], old_max_health,
                    resources.max[ RESOURCE_HEALTH ] );
                if ( talent.sanlayn.gift_of_the_sanlayn.ok() )
                {
                  buffs.gift_of_the_sanlayn->trigger();
                }
              }
              else
              {
                resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + health_change;
                resources.current[ RESOURCE_HEALTH ] /=
                    1.0 + health_change;  // Update health before the maximum is reduced
                recalculate_resource_max( RESOURCE_HEALTH );

                sim->print_debug(
                    "{} loses Vampiric Blood: health pct change {}%, current health: {} -> {}, max: {} -> {}", name(),
                    health_change * 100.0, old_health, resources.current[ RESOURCE_HEALTH ], old_max_health,
                    resources.max[ RESOURCE_HEALTH ] );
                if ( talent.sanlayn.gift_of_the_sanlayn.ok() )
                {
                  buffs.gift_of_the_sanlayn->expire();
                }
              }
            } );

    buffs.voracious = make_buff( this, "voracious", spell.voracious_buff )->set_trigger_spell( talent.blood.voracious );

    // Tier Sets
    // TWW1
    buffs.unbreakable_tww1_2pc = make_buff( this, "unbreakable", spell.unbreakable_tww1_2pc );
    buffs.unbroken_tww1_2pc    = make_buff( this, "unbroken", spell.unbroken_tww1_2pc )
                                    ->set_chance( 0.15 );  // TODO Verify this number.  Was found through manual testing, not in spelldata
    buffs.piledriver_tww1_4pc  = make_buff( this, "piledriver", spell.piledriver_tww1_4pc );
  }

  // Frost
  buffs.breath_of_sindragosa = make_fallback<breath_of_sindragosa_buff_t>(
      talent.frost.breath_of_sindragosa.ok(), this, "breath_of_sindragosa", talent.frost.breath_of_sindragosa );

  buffs.cold_heart = make_fallback( talent.frost.cold_heart.ok(), this, "cold_heart",
                                    talent.frost.cold_heart->effectN( 1 ).trigger() );

  buffs.gathering_storm =
      make_fallback( talent.frost.gathering_storm.ok(), this, "gathering_storm", spell.gathering_storm_buff )
          ->set_trigger_spell( talent.frost.gathering_storm )
          ->set_default_value_from_effect( 1 );

  buffs.inexorable_assault =
      make_fallback( talent.frost.inexorable_assault.ok(), this, "inexorable_assault", spell.inexorable_assault_buff );

  buffs.killing_machine = make_fallback( talent.frost.killing_machine.ok(), this, "killing_machine",
                                         talent.frost.killing_machine->effectN( 1 ).trigger() )
                              ->set_chance( 1.0 )
                              ->set_default_value_from_effect( 1 )
                              ->set_stack_change_callback( [ this ]( buff_t* buff_, int old_, int new_ ) {
                                // in 10.0.7 killing machine has a behavior where dropping from 2 -> 1 stacks will
                                // also refresh your buff in 10.1.0 this behavior changed, KM now no longer refreshes,
                                // unsure if this is intended
                                if ( !bugs && new_ > 0 && buff_->check() && old_ > new_ )
                                {
                                  buff_->refresh();
                                }
                              } );

  buffs.empower_rune_weapon =
      make_fallback( talent.frost.empower_rune_weapon.ok(), this, "empower_rune_weapon",
                     talent.frost.empower_rune_weapon )
          ->set_tick_zero( true )
          ->set_cooldown( 0_ms )
          ->set_period( talent.frost.empower_rune_weapon->effectN( 1 ).period() )
          ->set_default_value_from_effect( 3 )
          ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
          ->set_tick_behavior( buff_tick_behavior::REFRESH )
          ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
            replenish_rune( as<unsigned int>( b->data().effectN( 1 ).base_value() ), gains.empower_rune_weapon );
            resource_gain( RESOURCE_RUNIC_POWER, b->data().effectN( 2 ).resource( RESOURCE_RUNIC_POWER ),
                           gains.empower_rune_weapon );
          } );

  buffs.pillar_of_frost = make_fallback<pillar_of_frost_buff_t>( talent.frost.pillar_of_frost.ok(), this,
                                                                 "pillar_of_frost", talent.frost.pillar_of_frost );

  buffs.remorseless_winter =
      make_fallback( spec.remorseless_winter->ok(), this, "remorseless_winter", spec.remorseless_winter )
          ->set_cooldown( 0_ms )  // Handled by the action
          ->set_refresh_behavior( buff_refresh_behavior::DURATION )
          ->set_partial_tick( true )
          ->set_tick_callback(
              [ this ]( buff_t*, int, timespan_t ) { active_spells.remorseless_winter_tick->execute(); } )
          ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
            if ( !new_ )
            {
              buffs.gathering_storm->expire();
            }
            else
            {
              debug_cast<remorseless_winter_damage_t*>( active_spells.remorseless_winter_tick )->triggered_biting_cold =
                  false;
            }
          } );

  buffs.rime = make_fallback( spec.rime->ok(), this, "rime", spell.rime_buff )
                   ->set_trigger_spell( spec.rime )
                   ->set_chance( spec.rime->effectN( 2 ).percent() +
                                 talent.frost.rage_of_the_frozen_champion->effectN( 1 ).percent() )
                   ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
                     if ( talent.deathbringer.bind_in_darkness.ok() )
                     {
                       if ( new_ )
                         buffs.bind_in_darkness->trigger();
                       else
                         buffs.bind_in_darkness->expire();
                     }
                   } );

  buffs.bonegrinder_crit =
      make_fallback( talent.frost.bonegrinder.ok(), this, "bonegrinder_crit", spell.bonegrinder_crit_buff )
          ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
          ->set_cooldown( talent.frost.bonegrinder->internal_cooldown() );

  buffs.bonegrinder_frost =
      make_fallback( talent.frost.bonegrinder.ok(), this, "bonegrinder_frost", spell.bonegrinder_frost_buff )
          ->set_default_value( talent.frost.bonegrinder->effectN( 1 ).percent() )
          ->set_schools_from_effect( 1 );

  buffs.enduring_strength_builder =
      make_fallback( talent.frost.enduring_strength.ok(), this, "enduring_strength_builder",
                     talent.frost.enduring_strength->effectN( 1 ).trigger() );

  buffs.enduring_strength =
      make_fallback( talent.frost.enduring_strength.ok(), this, "enduring_strength", spell.enduring_strength_buff )
          ->set_default_value( spell.enduring_strength_buff->effectN( 1 ).percent() );

  buffs.frostwhelps_aid =
      make_fallback( talent.frost.frostwhelps_aid.ok(), this, "frostwhelps_aid", spell.frostwhelps_aid_buff )
          ->set_default_value( spell.frostwhelps_aid_buff->effectN( 1 ).base_value() );

  buffs.unleashed_frenzy = make_fallback( talent.frost.unleashed_frenzy.ok(), this, "unleashed_frenzy",
                                          talent.frost.unleashed_frenzy->effectN( 1 ).trigger() )
                               ->set_cooldown( talent.frost.unleashed_frenzy->internal_cooldown() )
                               ->set_default_value( talent.frost.unleashed_frenzy->effectN( 1 ).percent() );

  buffs.cryogenic_chamber = make_fallback<cryogenic_chamber_buff_t>(
      talent.frost.cryogenic_chamber.ok(), this, "cryogenic_chamber", spell.cryogenic_chamber_buff );

  buffs.icy_vigor =
      make_fallback( sets->has_set_bonus( DEATH_KNIGHT_FROST, TWW1, B4 ), this, "icy_vigor", spell.icy_vigor );

  // Unholy
  buffs.dark_transformation = make_fallback<dark_transformation_buff_t>(
      talent.unholy.dark_transformation.ok(), this, "dark_transformation", talent.unholy.dark_transformation );

  buffs.sudden_doom = make_fallback( talent.unholy.sudden_doom.ok(), this, "sudden_doom",
                                     talent.unholy.sudden_doom->effectN( 1 ).trigger() )
                          ->set_trigger_spell( talent.unholy.sudden_doom );

  buffs.unholy_assault =
      make_fallback( talent.unholy.unholy_assault.ok(), this, "unholy_assault", talent.unholy.unholy_assault )
          ->set_cooldown( 0_ms )  // Handled by the action
          ->set_default_value_from_effect( 4 );

  buffs.unholy_pact =
      make_fallback( talent.unholy.unholy_pact.ok(), this, "unholy_pact",
                     talent.unholy.unholy_pact->effectN( 1 ).trigger()->effectN( 1 ).trigger() )
          ->set_tick_zero( true )
          ->set_period( 1.0_s )
          ->set_tick_behavior( buff_tick_behavior::CLIP )
          ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) { active_spells.unholy_pact_damage->execute(); } )
          // Unholy pact ticks twice on buff expiration, hence the following override
          // https://github.com/SimCMinMax/WoW-BugTracker/issues/675
          ->set_expire_callback( [ this ]( buff_t*, int, timespan_t ) {
            active_spells.unholy_pact_damage->execute();
            active_spells.unholy_pact_damage->execute();
          } );

  buffs.ghoulish_frenzy =
    make_fallback( talent.unholy.ghoulish_frenzy.ok(), this, "ghoulish_frenzy", spell.ghoulish_frenzy_player )
    ->set_duration( 0_ms )  // Handled by DT
    ->set_default_value( spell.ghoulish_frenzy_player->effectN( 1 ).percent() );

  buffs.plaguebringer =
      make_fallback( talent.unholy.plaguebringer.ok(), this, "plaguebringer", spell.plaguebringer_buff )
          ->set_cooldown( talent.unholy.plaguebringer->internal_cooldown() )
          ->set_default_value( talent.unholy.plaguebringer->effectN( 1 ).percent() )
          ->set_max_stack( 1 );

  buffs.festermight = make_fallback( talent.unholy.festermight.ok(), this, "festermight", spell.festermight_buff )
                          ->set_default_value( talent.unholy.festermight->effectN( 1 ).percent() )
                          ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );

  buffs.commander_of_the_dead = make_fallback( talent.unholy.commander_of_the_dead.ok(), this, "commander_of_the_dead",
                                               spell.commander_of_the_dead );

  buffs.festering_scythe =
      make_fallback( talent.unholy.festering_scythe.ok(), this, "festering_scythe", spell.festering_scythe_buff );

  buffs.festering_scythe_stacks = make_fallback( talent.unholy.festering_scythe.ok(), this, "festering_scythe_stacks",
                                                 spell.festering_scythe_stacking_buff )
                                      ->set_expire_at_max_stack( true )
                                      ->set_expire_callback( [ this ]( buff_t*, int, timespan_t remains ) {
                                        if ( remains > 0_ms )
                                          buffs.festering_scythe->trigger();
                                      } );

  buffs.unholy_commander = make_fallback( sets->has_set_bonus( DEATH_KNIGHT_UNHOLY, TWW1, B4 ), this,
                                          "unholy_commander", spell.unholy_commander );

}

// death_knight_t::init_gains ===============================================

void death_knight_t::init_gains()
{
  player_t::init_gains();

  // Shared
  gains.antimagic_shell          = get_gain( "Antimagic Shell" );
  gains.rune                     = get_gain( "Rune Regeneration" );
  gains.start_of_combat_overflow = get_gain( "Start of Combat Overflow" );
  gains.coldthirst               = get_gain( "Coldthirst" );
  gains.rune_of_hysteria         = get_gain( "Rune of Hysteria" );

  // Blood
  gains.bonestorm        = get_gain( "Bonestorm" );
  gains.blood_tap        = get_gain( "Blood Tap" );
  gains.consumption      = get_gain( "Consumption" );
  gains.drw_heart_strike = get_gain( "Rune Weapon Heart Strike" );
  gains.heartbreaker     = get_gain( "Heartbreaker" );
  gains.tombstone        = get_gain( "Tombstone" );

  // Frost
  gains.breath_of_sindragosa        = get_gain( "Breath of Sindragosa" );
  gains.empower_rune_weapon         = get_gain( "Empower Rune Weapon" );
  gains.frost_fever                 = get_gain( "Frost Fever" );
  gains.horn_of_winter              = get_gain( "Horn of Winter" );
  gains.murderous_efficiency        = get_gain( "Murderous Efficiency" );
  gains.obliteration                = get_gain( "Obliteration" );
  gains.rage_of_the_frozen_champion = get_gain( "Rage of the Frozen Champion" );
  gains.runic_attenuation           = get_gain( "Runic Attenuation" );
  gains.runic_empowerment           = get_gain( "Runic Empowerment" );
  gains.frigid_executioner          = get_gain( "Frigid Executioner" );

  // Unholy
  gains.apocalypse      = get_gain( "Apocalypse" );
  gains.festering_wound = get_gain( "Festering Wound" );

  // Rider of the Apocalypse
  gains.antimagic_shell_horsemen = get_gain( "Antimagic Shell Horsemen" );
}

// death_knight_t::init_procs ===============================================

void death_knight_t::init_procs()
{
  player_t::init_procs();

  procs.killing_machine_oblit = get_proc( "Killing Machine spent on Obliterate" );
  procs.killing_machine_fsc   = get_proc( "Killing Machine spent on Frostscythe" );

  procs.km_from_crit_aa         = get_proc( "Killing Machine: Critical auto attacks" );
  procs.km_from_obliteration_fs = get_proc( "Killing Machine: Frost Strike" );
  procs.km_from_obliteration_hb = get_proc( "Killing Machine: Howling Blast" );
  procs.km_from_obliteration_ga = get_proc( "Killing Machine: Glacial Advance" );
  procs.km_from_obliteration_sr = get_proc( "Killing Machine: Soul Reaper" );

  procs.km_from_crit_aa_wasted         = get_proc( "Killing Machine wasted: Critical auto attacks" );
  procs.km_from_obliteration_fs_wasted = get_proc( "Killing Machine wasted: Frost Strike" );
  procs.km_from_obliteration_hb_wasted = get_proc( "Killing Machine wasted: Howling Blast" );
  procs.km_from_obliteration_ga_wasted = get_proc( "Killing Machine wasted: Glacial Advance" );
  procs.km_from_obliteration_sr_wasted = get_proc( "Killing Machine wasted: Soul Reaper" );

  procs.razorice_from_arctic_assault  = get_proc( "Razorice from Arctic Assault" );
  procs.razorice_from_avalanche       = get_proc( "Razorice from Avalanche" );
  procs.razorice_from_glacial_advance = get_proc( "Razorice from Glacial Advance" );
  procs.razorice_from_runeforge       = get_proc( "Razorice from Runeforge" );

  procs.ready_rune = get_proc( "Rune ready" );

  procs.rp_runic_corruption = get_proc( "Runic Corruption from Runic Power Spent" );
  procs.sr_runic_corruption = get_proc( "Runic Corruption from Soul Reaper" );

  procs.bloodworms = get_proc( "Bloodworms" );
  procs.carnage    = get_proc( "Carnage" );

  procs.fw_festering_strike = get_proc( "Festering Wound from Festering Strike" );
  procs.fw_festering_scythe = get_proc( "Festering Wound from Festering Scythe" );
  procs.fw_infected_claws   = get_proc( "Festering Wound from Infected Claws" );
  procs.fw_pestilence       = get_proc( "Festering Wound from Pestilence" );
  procs.fw_unholy_assault   = get_proc( "Festering Wound from Unholy Assault" );
  procs.fw_vile_contagion   = get_proc( "Festering Wound from Vile Contagion" );
  procs.fw_ruptured_viscera = get_proc( "Festering Wound from Ruptured Viscera" );
  procs.fw_abomination      = get_proc( "Festering Wound from Abomination" );

  procs.fw_apocalypse    = get_proc( "Festering Wound Burst by Apocalypse" );
  procs.fw_death         = get_proc( "Festering Wound Burst by Target Death" );
  procs.fw_wound_spender = get_proc( "Festering Wound Burst by Wound Spender" );
  procs.fw_sudden_doom   = get_proc( "Festering Wound Burst by Sudden Doom" );

  procs.enduring_chill = get_proc( "Enduring Chill extra bounces" );

  procs.decomposition = get_proc( "Decomposition" );

  procs.blood_beast           = get_proc( "Blood Beast" );
  procs.vampiric_strike       = get_proc( "Vampiric Strike Proc" );
  procs.vampiric_strike_waste = get_proc( "Vampiric Strike Proc Wasted" );

  procs.exterminate_reapers_mark = get_proc( "Reaper's Mark from Exterminate" );
}

// death_knight_t::init_finished ============================================

void death_knight_t::init_finished()
{
  for ( auto& b : buff_list )
  {
    if ( b->data().ok() )
      apply_affecting_auras( *b );
  }

  parse_player_effects();

  player_t::init_finished();

  if ( deprecated_dnd_expression )
  {
    sim->errorf(
        "Player %s, Death and Decay and Defile expressions of the form "
        "'dot.death_and_decay.X' have been deprecated. Use 'death_and_decay.ticking' "
        "or death_and_decay.remains' instead.",
        name() );
  }

  if ( runeforge_expression_warning )
  {
    sim->errorf(
        "Player %s, Death Knight runeforge expressions of the form "
        "runeforge.name are to be used with Shadowlands Runeforge legendaries only. "
        "Use death_knight.runeforge.name instead.",
        name() );
  }
}

// death_knight_t::validate_fight_style =====================================
bool death_knight_t::validate_fight_style( fight_style_e fight ) const
{
  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    switch ( fight )
    {
      case FIGHT_STYLE_PATCHWERK:
      case FIGHT_STYLE_CASTING_PATCHWERK:
        return true;
      default:
        return false;
    }
  }
  if ( specialization() == DEATH_KNIGHT_FROST )
  {
    switch ( fight )
    {
      case FIGHT_STYLE_PATCHWERK:
      case FIGHT_STYLE_CASTING_PATCHWERK:
      case FIGHT_STYLE_DUNGEON_SLICE:
      case FIGHT_STYLE_DUNGEON_ROUTE:
        return true;
      default:
        return false;
    }
  }
  if ( specialization() == DEATH_KNIGHT_UNHOLY )
  {
    switch ( fight )
    {
      case FIGHT_STYLE_PATCHWERK:
      case FIGHT_STYLE_CASTING_PATCHWERK:
      case FIGHT_STYLE_DUNGEON_SLICE:
      case FIGHT_STYLE_DUNGEON_ROUTE:
        return true;
      default:
        return false;
    }
  }
  return false;
}

// death_knight_t::activate =================================================

void death_knight_t::activate()
{
  player_t::activate();

  register_on_combat_state_callback( [ this ]( player_t*, bool c ) {
    if ( !c )
    {
      runic_power_decay = make_event( *sim, 20_s, [ this ]() {
        runic_power_decay = nullptr;
        make_event<runic_power_decay_event_t>( *sim, this );
      } );
    }
    else
    {
      event_t::cancel( runic_power_decay );
    }
  } );

  if ( talent.soul_reaper.ok() )
    register_on_kill_callback( [ this ]( player_t* t ) { trigger_soul_reaper_death( t ); } );

  if ( talent.deathbringer.reapers_mark.ok() )
    register_on_kill_callback( [ this ]( player_t* t ) { trigger_reapers_mark_death( t ); } );

  if ( specialization() == DEATH_KNIGHT_UNHOLY && spec.festering_wound->ok() )
    register_on_kill_callback( [ this ]( player_t* t ) { trigger_festering_wound_death( t ); } );

  if ( talent.deaths_reach.ok() )
    register_on_kill_callback( [ this ]( player_t* ) { cooldown.death_grip->reset( true ); } );

  if ( talent.rider.nazgrims_conquest.ok() )
  {
    int stacks = as<int>( talent.rider.nazgrims_conquest->effectN( 1 ).base_value() );
    register_on_kill_callback( [ this, stacks ]( player_t* ) {
      if ( !sim->event_mgr.canceled && buffs.apocalyptic_conquest->check() )
      {
        debug_cast<buffs::apocalyptic_conquest_buff_t*>( buffs.apocalyptic_conquest )->nazgrims_conquest += stacks;
        invalidate_cache( CACHE_STRENGTH );
      }
    } );
  }

  if ( spec.outbreak->ok() || talent.unholy.unholy_blight.ok() )
    register_on_kill_callback( [ this ]( player_t* t ) { trigger_virulent_plague_death( t ); } );

  if ( talent.unholy.unholy_aura.ok() )
  {
    sim->target_non_sleeping_list.register_callback( [ & ]( player_t* t ) {
      if ( !t->is_enemy() )
        return;

      auto enemy_td = get_target_data( t );
      if ( !enemy_td->debuff.unholy_aura->check() )
      {
        make_event( *sim, timespan_t::from_millis( rng().range( 0, 2000 ) ),
                    [ enemy_td ]() { enemy_td->debuff.unholy_aura->trigger(); } );
      }
    } );
  }
}

// death_knight_t::reset ====================================================

void death_knight_t::reset()
{
  player_t::reset();

  _runes.reset();
  active_dnd                    = nullptr;
  runic_power_decay             = nullptr;
  km_proc_attempts              = 0;
  bone_shield_charges_consumed  = 0;
  active_riders                 = 0;
}

// death_knight_t::assess_heal ==============================================

void death_knight_t::assess_heal( school_e school, result_amount_type t, action_state_t* s )
{
  if ( specialization() == DEATH_KNIGHT_BLOOD && buffs.vampiric_blood->up() )
    s->result_total *= 1.0 + buffs.vampiric_blood->data().effectN( 1 ).percent() +
                       talent.blood.improved_vampiric_blood->effectN( 1 ).percent();

  if ( talent.blood.sanguine_ground.ok() && in_death_and_decay() )
    s->result_total *= 1.0 + spell.sanguine_ground->effectN( 2 ).percent();

  if ( talent.osmosis.ok() && buffs.antimagic_shell->check() )
    s->result_total *= 1.0 + buffs.antimagic_shell->data().effectN( 4 ).percent();

  player_t::assess_heal( school, t, s );
}

// death_knight_t::assess_damage ============================================

void death_knight_t::assess_damage( school_e school, result_amount_type type, action_state_t* s )
{
  player_t::assess_damage( school, type, s );

  if ( specialization() == DEATH_KNIGHT_BLOOD && s-> result == RESULT_PARRY && talent.blood.bloodied_blade->ok() )
  {
    if ( buffs.bloodied_blade_stacks->at_max_stacks() )
    {
      buffs.bloodied_blade_stacks->expire();
      buffs.bloodied_blade_final->trigger();
      active_spells.heart_strike_bloodied_blade->execute_on_target( target );
    }
    else if ( ! buffs.bloodied_blade_final->check() )  // Can not proc while the final 10% str boost is up
      buffs.bloodied_blade_stacks->trigger();
  }
}

// death_knight_t::assess_damage_imminent ===================================

void death_knight_t::bone_shield_handler( const action_state_t* state ) const
{
  //  TWW1 4pc handling
  if ( specialization() == DEATH_KNIGHT_BLOOD &&
        sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, TWW1, B4 ) &&
        buffs.bone_shield->stack() < sets -> set ( DEATH_KNIGHT_BLOOD, TWW1, B4 )->effectN( 2 ).base_value() &&
        rng().roll( sets -> set ( DEATH_KNIGHT_BLOOD, TWW1, B4 )->effectN( 5 ).percent() ) )
  {
    if ( rng().roll( 0.5 ) ) // 50% chance to roll 1 or 2 charges
      buffs.piledriver_tww1_4pc->trigger( as<int>( sets -> set ( DEATH_KNIGHT_BLOOD, TWW1, B4 )->effectN( 3 ).base_value() ) );
    else
      buffs.piledriver_tww1_4pc->trigger( as<int>( sets -> set ( DEATH_KNIGHT_BLOOD, TWW1, B4 )->effectN( 4 ).base_value() ) );
  }

  if ( ( ( specialization() == DEATH_KNIGHT_BLOOD && !buffs.bone_shield->check() ) || !cooldown.bone_shield_icd->up() ||
         state->action->special ) )
  {
    return;
  }

  sim->print_log( "{} took a successful auto attack and lost a bone shield charge", name() );
  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    buffs.bone_shield->decrement();
    if( talent.blood.ossified_vitriol -> ok() )
      buffs.ossified_vitriol->trigger();
    if ( sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, TWW1, B2 ) )
      buffs.unbroken_tww1_2pc->trigger();
    if ( sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, TWW1, B4 ) )
      buffs.piledriver_tww1_4pc->decrement();
  }
  cooldown.bone_shield_icd->start();
  // Blood tap spelldata is a bit weird, it's not in milliseconds like other time values, and is positive even though it
  // reduces a cooldown
  if ( talent.blood.blood_tap.ok() )
  {
    cooldown.blood_tap->adjust( -1.0 * timespan_t::from_seconds( talent.blood.blood_tap->effectN( 2 ).base_value() ) );
  }

  if ( talent.blood.shattering_bone.ok() )
    active_spells.shattering_bone->execute_on_target( target );

  cooldown.dancing_rune_weapon->adjust( talent.blood.insatiable_blade->effectN( 1 ).time_value() );
}

void death_knight_t::assess_damage_imminent( school_e, result_amount_type, action_state_t* s )
{
  bone_shield_handler( s );
}

// death_knight_t::do_damage ================================================

void death_knight_t::do_damage( action_state_t* state )
{
  player_t::do_damage( state );

  if ( state->result_amount > 0 && talent.blood.mark_of_blood.ok() && !state->action->special )
  {
    const death_knight_td_t* td = get_target_data( state->action->player );
    if ( td && td->debuff.mark_of_blood->check() )
      active_spells.mark_of_blood_heal->execute();
  }

  if ( runeforge.rune_of_sanguination )
  {
    // Health threshold and internal cooldown are both handled in ready()
    if ( active_spells.runeforge_sanguination->ready() )
      active_spells.runeforge_sanguination->execute();
  }

  if ( talent.blood_draw.ok() && specialization() == DEATH_KNIGHT_BLOOD && active_spells.blood_draw->ready() )
  {
    active_spells.blood_draw->execute();
  }
}

// death_knight_t::target_mitigation ========================================

void death_knight_t::target_mitigation( school_e school, result_amount_type type, action_state_t* state )
{
  if ( specialization() == DEATH_KNIGHT_BLOOD )
    state->result_amount *= 1.0 + spec.blood_fortification->effectN( 2 ).percent();

  if ( specialization() == DEATH_KNIGHT_BLOOD && buffs.rune_tap->up() )
    state->result_amount *= 1.0 + buffs.rune_tap->data().effectN( 1 ).percent();

  if ( buffs.icebound_fortitude->up() )
    state->result_amount *= 1.0 + buffs.icebound_fortitude->data().effectN( 3 ).percent();

  if ( buffs.bloodsoaked_ground->up() )
    state->result_amount *= 1.0 + buffs.bloodsoaked_ground->data().effectN( 1 ).percent();

  const death_knight_td_t* td = get_target_data( state->action->player );
  if ( td && runeforge.rune_of_apocalypse )
    state->result_amount *= 1.0 + td->debuff.apocalypse_famine->check_stack_value();

  if ( dbc::is_school( school, SCHOOL_MAGIC ) && runeforge.rune_of_spellwarding )
    state->result_amount *= 1.0 + runeforge.rune_of_spellwarding;

  player_t::target_mitigation( school, type, state );
}

// death_knight_t::composite_bonus_armor =========================================

double death_knight_t::composite_bonus_armor() const
{
  double ba = player_t::composite_bonus_armor();

  if ( specialization() == DEATH_KNIGHT_BLOOD && buffs.bone_shield->check() )
  {
    ba += buffs.bone_shield->value() * cache.strength();
  }

  return ba;
}

// death_knight_t::composite_attribute ======================================

double death_knight_t::composite_attribute( attribute_e attr ) const
{
  auto a = player_t::composite_attribute( attr );

  // TODO: remove if fixed.  This implements effect type 80 ( Modify Attribute% )
  if ( attr == ATTR_STRENGTH )
  {
    switch ( specialization() )
    {
      case DEATH_KNIGHT_BLOOD:
        if ( buffs.bloodied_blade_stacks->check() )
          a += base.stats.attribute[ attr ] * buffs.bloodied_blade_stacks->check_value();
        if ( buffs.bloodied_blade_final->check() )
          a += base.stats.attribute[ attr ] * buffs.bloodied_blade_final->check_value();
        if ( buffs.visceral_strength->check() )
          a += base.stats.attribute[ attr ] * buffs.visceral_strength->check_value();
        break;
      case DEATH_KNIGHT_UNHOLY:
        if ( buffs.visceral_strength->check() )
          a += base.stats.attribute[ attr ] * buffs.visceral_strength->check_value();
        if ( buffs.apocalyptic_conquest->check() )
          a += base.stats.attribute[ attr ] * buffs.apocalyptic_conquest->check_value();
        break;
      case DEATH_KNIGHT_FROST:
        if ( buffs.apocalyptic_conquest->check() )
          a += base.stats.attribute[ attr ] * buffs.apocalyptic_conquest->check_value();
        break;
      default:
        break;
    }
  }

  return a;
}

// death_knight_t::combat_begin =============================================

void death_knight_t::combat_begin()
{
  player_t::combat_begin();
  auto rp_overflow = resources.current[ RESOURCE_RUNIC_POWER ] - MAX_START_OF_COMBAT_RP;
  if ( rp_overflow > 0 )
  {
    resource_loss( RESOURCE_RUNIC_POWER, rp_overflow, gains.start_of_combat_overflow );
  }
}

// death_knight_t::invalidate_cache =========================================

void death_knight_t::invalidate_cache( cache_e c )
{
  parse_player_effects_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_MASTERY:
      if ( specialization() == DEATH_KNIGHT_BLOOD )
        player_t::invalidate_cache( CACHE_ATTACK_POWER );
      break;
    case CACHE_STRENGTH:
      if ( spell.bone_shield->ok() )
        player_t::invalidate_cache( CACHE_BONUS_ARMOR );
      break;
    default:
      break;
  }
}

// death_knight_t::primary_role =============================================

role_e death_knight_t::primary_role() const
{
  if ( player_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_ATTACK )
    return ROLE_ATTACK;

  if ( specialization() == DEATH_KNIGHT_BLOOD )
    return ROLE_TANK;

  return ROLE_ATTACK;
}

// death_knight_t::convert_hybrid_stat ==============================================

stat_e death_knight_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
    // This is a guess at how AGI/INT will work for DKs, TODO: confirm
    case STAT_AGI_INT:
      return STAT_NONE;
    case STAT_STR_AGI_INT:
    case STAT_STR_AGI:
    case STAT_STR_INT:
      return STAT_STRENGTH;
    case STAT_SPIRIT:
      return STAT_NONE;
    case STAT_BONUS_ARMOR:
      if ( specialization() == DEATH_KNIGHT_BLOOD )
        return s;
      else
        return STAT_NONE;
    default:
      return s;
  }
}

// death_knight_t::runes_per_second =========================================

// Base rune regen rate is 10 seconds; we want the per-second regen
// rate, so divide by 10.0.  Haste is a multiplier (so 30% haste
// means composite_attack_haste is 1/1.3), so we invert it.  Haste
// linearly scales regen rate -- 100% haste means a rune regens in 5
// seconds, etc.
inline double death_knight_t::runes_per_second() const
{
  double rps = RUNE_REGEN_BASE_SEC / cache.attack_haste();
  // Runic corruption doubles rune regeneration speed
  if ( buffs.runic_corruption->check() )
  {
    rps *= 1.0 + spell.runic_corruption->effectN( 1 ).percent() + talent.unholy.runic_mastery->effectN( 2 ).percent();
  }

  return rps;
}

inline double death_knight_t::rune_regen_coefficient() const
{
  auto coeff = cache.attack_haste();
  // Runic corruption doubles rune regeneration speed
  if ( buffs.runic_corruption->check() )
  {
    coeff /= 1.0 + spell.runic_corruption->effectN( 1 ).percent() + talent.unholy.runic_mastery->effectN( 2 ).percent();
  }

  return coeff;
}

// death_knight_t::arise ====================================================
void death_knight_t::arise()
{
  player_t::arise();
  if ( runeforge.rune_of_the_stoneskin_gargoyle )
    buffs.stoneskin_gargoyle->trigger();

  start_inexorable_assault();
  start_cold_heart();

  if ( talent.rider.riders_champion.ok() )
  {
    rng().shuffle( active_spells.rider_summon_spells.begin(), active_spells.rider_summon_spells.end() );
  }

  if ( talent.rider.a_feast_of_souls.ok() )
  {
    start_a_feast_of_souls();
  }
}

void death_knight_t::adjust_dynamic_cooldowns()
{
  player_t::adjust_dynamic_cooldowns();

  _runes.update_coefficient();
}

// Basic Parse Effects implementation for pets. Mostly applies to Dancing Rune Weapon currently
// Other pets (such as Mograine from Riders) also benefit from this due to executing spells contained in whitelists
template <class T_PET, class Base>
void pets::pet_action_t<T_PET, Base>::apply_pet_action_effects()
{
  // Blood
  parse_effects( dk()->buffs.consumption );
  parse_effects( dk()->buffs.crimson_scourge );
  parse_effects( dk()->buffs.ossified_vitriol );
  parse_effects( dk()->buffs.sanguine_ground );
  parse_effects( dk()->buffs.heartrend, dk()->talent.blood.heartrend );
  parse_effects( dk()->buffs.hemostasis );
  parse_effects( dk()->buffs.ossuary );

  // Don't auto parse coag, since there is some snapshot behavior when the weapon dies
  // parse_effects( dk()->buffs.coagulopathy );

  // Unholy
  parse_effects( dk()->buffs.unholy_assault );
  parse_effects( dk()->mastery.dreadblade );

  // Rider of the Apocalypse
  parse_effects( dk()->buffs.mograines_might );
  parse_effects( dk()->buffs.a_feast_of_souls );
}

template <class T_PET, class Base>
void pets::pet_action_t<T_PET, Base>::apply_pet_target_effects()
{
  using namespace pets;
  /* NOTE NOTE NOTE NOTE NOTE
  As of 2024 Aug 18th, while testing for TWW we observed that if the pet applies the debuff, like DRW does for blood plague
  they are considered the caster, and as such, they get the benefit of the casters amps (aura 271).  If the player applies the debuff
  the pet does not gain the benefit of the caster debuff, but does gain the benefit for pet/guardian auras (aura 380/381) if they exist.

  Auras 380 and 381 get applied in parse_player_effects of the DK.

  Below we should only have debuffs that are cast by pets and guardians, that apply aura 271.
  */
  // Shared
  parse_target_effects( d_fn( &death_knight_pet_td_t::dots_t::blood_plague ), dk()->spell.blood_plague,
                        dk()->talent.unholy.morbidity, dk()->talent.blood.coagulopathy );

  // Blood

  // Frost

  // Unholy

  // Rider of the Apocalypse

  // Deathbringer

  // San'layn
}

template <class Base>
void death_knight_action_t<Base>::apply_action_effects()
{
  // Shared
  parse_effects( p()->buffs.blood_draw );

  // Blood
  parse_effects( p()->buffs.coagulopathy );
  parse_effects( p()->buffs.consumption );
  parse_effects( p()->buffs.crimson_scourge );
  parse_effects( p()->buffs.ossified_vitriol );
  parse_effects( p()->buffs.sanguine_ground );
  parse_effects( p()->buffs.heartrend, p()->talent.blood.heartrend );
  parse_effects( p()->buffs.hemostasis );
  parse_effects( p()->buffs.ossuary );

  // Frost
  parse_effects( p()->buffs.rime, p()->talent.frost.improved_rime );
  parse_effects( p()->buffs.gathering_storm );
  parse_effects( p()->buffs.killing_machine, effect_mask_t( true ).disable( 2 ) );
  parse_effects( p()->mastery.frozen_heart );

  // Unholy
  parse_effects( p()->buffs.unholy_assault );
  parse_effects( p()->buffs.sudden_doom, p()->talent.unholy.harbinger_of_doom );
  parse_effects( p()->buffs.plaguebringer, p()->talent.unholy.plaguebringer );
  parse_effects( p()->mastery.dreadblade );

  // Rider of the Apocalypse
  parse_effects( p()->buffs.mograines_might );
  parse_effects( p()->buffs.a_feast_of_souls );

  // Deathbringer
  parse_effects( p()->buffs.dark_talons_shadowfrost, p()->talent.deathbringer.dark_talons );
  parse_effects( p()->buffs.bind_in_darkness, p()->talent.deathbringer.bind_in_darkness );
  parse_effects( p()->buffs.exterminate, effect_mask_t( true ).disable( 2 ) );
  parse_effects( p()->buffs.exterminate_painful_death, effect_mask_t( true ).disable( 2 ) );

  // San'layn
  parse_effects( p()->buffs.essence_of_the_blood_queen_damage, USE_CURRENT, p()->talent.sanlayn.frenzied_bloodthirst );
}

template <class Base>
void death_knight_action_t<Base>::apply_target_effects()
{
  // Shared
  parse_target_effects( d_fn( &death_knight_td_t::dots_t::virulent_plague ), p()->spell.virulent_plague,
                        p()->talent.unholy.morbidity );
  parse_target_effects( d_fn( &death_knight_td_t::dots_t::frost_fever ), p()->spell.frost_fever,
                        p()->talent.unholy.morbidity );
  parse_target_effects( d_fn( &death_knight_td_t::dots_t::blood_plague ), p()->spell.blood_plague,
                        p()->talent.unholy.morbidity, p()->talent.blood.coagulopathy );
  parse_target_effects( d_fn( &death_knight_td_t::dots_t::unholy_blight, false ), p()->spell.unholy_blight_dot,
                        p()->talent.unholy.morbidity );
  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::apocalypse_war ), p()->spell.apocalypse_war_debuff,
                        p()->talent.unholy_bond );
  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::razorice ), p()->spell.razorice_debuff,
                        p()->talent.unholy_bond );
  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::brittle ), p()->spell.brittle_debuff );

  // Blood

  // Frost
  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::everfrost ),
                        p()->talent.frost.everfrost->effectN( 1 ).trigger(), p()->talent.frost.everfrost );
  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::piercing_chill ), p()->spell.piercing_chill_debuff );

  // Unholy
  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::death_rot ), p()->spell.death_rot_debuff );
  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::rotten_touch ), p()->spell.rotten_touch_debuff );

  // Rider of the Apocalypse

  // Deathbringer

  // San'layn
  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::incite_terror ), p()->spell.incite_terror_debuff );
}

void death_knight_t::parse_player_effects()
{
  // Shared
  parse_effects( spec.death_knight );
  parse_effects( spec.death_knight_2 );
  parse_effects( spec.plate_specialization );
  parse_effects( buffs.blood_draw, talent.blood_draw );
  parse_effects( buffs.icy_talons, talent.icy_talons );
  parse_effects( buffs.rune_mastery, talent.rune_mastery );
  parse_effects( buffs.unholy_strength, talent.unholy_bond );
  parse_effects( buffs.unholy_ground, talent.unholy_ground );
  parse_effects( buffs.stoneskin_gargoyle, talent.unholy_bond );
  parse_effects( talent.veteran_of_the_third_war, spec.blood_death_knight );
  parse_effects( talent.runic_protection );
  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::brittle ), spell.brittle_debuff );
  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::apocalypse_war ), spell.apocalypse_war_debuff,
                        talent.unholy_bond );

  // Blood
  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    parse_effects( spec.blood_death_knight );
    parse_effects( spec.blood_death_knight_2 );
    parse_effects( spec.blood_fortification );
    parse_effects( spec.riposte );
    parse_effects( mastery.blood_shield );
    parse_effects( buffs.blood_shield, talent.blood.bloodshot );
    parse_effects( buffs.voracious, talent.blood.voracious );
    parse_effects( buffs.dancing_rune_weapon );
    parse_effects( buffs.bone_shield, IGNORE_STACKS, talent.blood.improved_bone_shield, talent.blood.reinforced_bones );
    parse_effects( buffs.perseverance_of_the_ebon_blade );

    // Tier Sets
    parse_effects( buffs.unbreakable_tww1_2pc, [ this ] { return buffs.bone_shield->check(); } );
    parse_effects( buffs.unbroken_tww1_2pc );
    parse_effects( buffs.piledriver_tww1_4pc );
  }

  // Frost
  if ( specialization() == DEATH_KNIGHT_FROST )
  {
    parse_effects( spec.frost_death_knight );
    parse_effects( spec.frost_death_knight_2 );
    parse_effects( buffs.empower_rune_weapon, talent.frost.empower_rune_weapon );
    parse_effects( buffs.frostwhelps_aid, talent.frost.frostwhelps_aid );
    parse_effects( buffs.bonegrinder_frost, talent.frost.bonegrinder );
    parse_effects( buffs.bonegrinder_crit, talent.frost.bonegrinder );
    parse_effects( buffs.enduring_strength, talent.frost.enduring_strength );
    parse_effects( buffs.unleashed_frenzy, talent.frost.unleashed_frenzy );
    parse_effects( buffs.icy_vigor );
  }

  // Unholy
  if ( specialization() == DEATH_KNIGHT_UNHOLY )
  {
    parse_effects( spec.unholy_death_knight );
    parse_effects( spec.unholy_death_knight_2 );
    parse_effects( mastery.dreadblade );
    parse_effects( buffs.unholy_assault, talent.unholy.unholy_assault );
    parse_effects( buffs.ghoulish_frenzy, talent.unholy.ghoulish_frenzy );
    parse_effects( buffs.festermight, talent.unholy.festermight );
    parse_effects( buffs.unholy_commander );
    parse_effects( sets->set( DEATH_KNIGHT_UNHOLY, TWW1, B2 ) );
    parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::unholy_aura ), spell.unholy_aura_debuff,
                          talent.unholy.unholy_aura );
    parse_target_effects( d_fn( &death_knight_td_t::dots_t::virulent_plague ), spell.virulent_plague,
                          talent.unholy.morbidity );
    parse_target_effects( d_fn( &death_knight_td_t::dots_t::frost_fever ), spell.frost_fever, talent.unholy.morbidity );
    parse_target_effects( d_fn( &death_knight_td_t::dots_t::blood_plague ), spell.blood_plague,
                          talent.unholy.morbidity );
    parse_target_effects( d_fn( &death_knight_td_t::dots_t::unholy_blight, false ), spell.unholy_blight_dot,
                          talent.unholy.morbidity );
  }

  // Rider of the Apocalypse
  parse_effects( buffs.mograines_might, talent.rider.mograines_might );
  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::chains_of_ice_trollbane_damage ),
                        pet_spell.trollbanes_chains_of_ice_debuff );

  // San'layn
  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::incite_terror ), spell.incite_terror_debuff );
}

void death_knight_t::apply_affecting_auras( buff_t& buff )
{
  // Spec and Class Auras
  buff.apply_affecting_aura( spec.death_knight );
  buff.apply_affecting_aura( spec.death_knight_2 );
  buff.apply_affecting_aura( spec.blood_death_knight );
  buff.apply_affecting_aura( spec.blood_death_knight_2 );
  buff.apply_affecting_aura( spec.frost_death_knight );
  buff.apply_affecting_aura( spec.frost_death_knight_2 );
  buff.apply_affecting_aura( spec.unholy_death_knight );
  buff.apply_affecting_aura( spec.unholy_death_knight_2 );

  // Shared
  buff.apply_affecting_aura( talent.antimagic_barrier );
  buff.apply_affecting_aura( talent.osmosis );
  buff.apply_affecting_aura( talent.unholy_bond );

  // Blood
  buff.apply_affecting_aura( talent.blood.reinforced_bones );

  // Frost
  buff.apply_affecting_aura( talent.frost.smothering_offense );

  // Unholy
  buff.apply_affecting_aura( talent.unholy.harbinger_of_doom );
  buff.apply_affecting_aura( talent.unholy.ghoulish_frenzy );

  // Rider of the Apocalypse

  // San'layn
  buff.apply_affecting_aura( talent.sanlayn.frenzied_bloodthirst );

  // Deathbringer
}

void death_knight_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  // Spec and Class Auras
  action.apply_affecting_aura( spec.death_knight );
  action.apply_affecting_aura( spec.death_knight_2 );
  action.apply_affecting_aura( spec.blood_death_knight );
  action.apply_affecting_aura( spec.blood_death_knight_2 );
  action.apply_affecting_aura( spec.frost_death_knight );
  action.apply_affecting_aura( spec.frost_death_knight_2 );
  action.apply_affecting_aura( spec.unholy_death_knight );
  action.apply_affecting_aura( spec.unholy_death_knight_2 );

  // Shared
  action.apply_affecting_aura( talent.antimagic_barrier );
  action.apply_affecting_aura( talent.assimilation );
  action.apply_affecting_aura( talent.unholy_bond );
  action.apply_affecting_aura( talent.deaths_echo );
  action.apply_affecting_aura( talent.deaths_reach );
  action.apply_affecting_aura( talent.gloom_ward );
  action.apply_affecting_aura( talent.proliferating_chill );
  action.apply_affecting_aura( talent.unyielding_will );
  action.apply_affecting_aura( talent.ice_prison );

  // Blood
  action.apply_affecting_aura( talent.blood.improved_heart_strike );
  action.apply_affecting_aura( talent.blood.tightening_grasp );
  action.apply_affecting_aura( talent.blood.voracious );
  action.apply_affecting_aura( talent.blood.rapid_decomposition );

  // Frost
  action.apply_affecting_aura( talent.frost.improved_frost_strike );
  action.apply_affecting_aura( talent.frost.improved_obliterate );
  action.apply_affecting_aura( talent.frost.frigid_executioner );
  action.apply_affecting_aura( talent.frost.biting_cold );
  action.apply_affecting_aura( talent.frost.absolute_zero );
  action.apply_affecting_aura( talent.frost.icecap );
  if ( spec.might_of_the_frozen_wastes->ok() && main_hand_weapon.group() == WEAPON_2H )
  {
    action.apply_affecting_aura( spec.might_of_the_frozen_wastes );
  }
  action.apply_affecting_aura( sets->set( DEATH_KNIGHT_FROST, TWW1, B2 ) );

  // Unholy
  action.apply_affecting_aura( talent.unholy.ebon_fever );
  action.apply_affecting_aura( talent.unholy.improved_festering_strike );
  action.apply_affecting_aura( talent.unholy.improved_death_coil );
  action.apply_affecting_aura( talent.unholy.bursting_sores );
  action.apply_affecting_aura( talent.unholy.superstrain );
  action.apply_affecting_aura( talent.unholy.foul_infections );
  action.apply_affecting_aura( talent.unholy.menacing_magus );

  // Rider of the Apocalypse
  action.apply_affecting_aura( talent.rider.mawsworn_menace );
  action.apply_affecting_aura( talent.rider.hungering_thirst );

  // San'layn
  if ( talent.unholy.clawing_shadows.ok() )
  {
    action.apply_affecting_aura( spell.vampiric_strike_clawing_shadows );
  }

  // Deathbringer
  action.apply_affecting_aura( talent.deathbringer.bind_in_darkness );
  action.apply_affecting_aura( talent.deathbringer.wither_away );
  action.apply_affecting_aura( talent.deathbringer.deaths_messenger );
  action.apply_affecting_aura( talent.deathbringer.swift_end );
  action.apply_affecting_aura( talent.deathbringer.painful_death );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class death_knight_report_t : public player_report_extension_t
{
public:
  death_knight_report_t( death_knight_t& player ) : p( player )
  {
  }

  void html_rune_waste( report::sc_html_stream& os ) const
  {
    os << "<h3 class=\"toggle open\">Rune waste details (<strong>experimental</strong>)</h3>\n"
       << "<div class=\"toggle-content\">\n";

    os << "<p style=\"width: 75%\">"
       << "In the table below, &quot;Seconds per Rune&quot; denotes the time in seconds an individual "
       << "rune is wasting regeneration. The &quot;Total Seconds per Iteration&quot; denotes the cumulative "
       << "time in seconds all runes wasted during a single iteration."
       << "</p>\n";

    os << "<table class=\"sc\" style=\"float: left;margin-right: 10px;\">\n"
       << "<tr>\n"
       << "<th colspan=\"5\">Seconds per Rune (n=" << p._runes.rune_waste.count() << ")</th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th>Minimum</th>\n"
       << "<th>5<sup>th</sup> percentile</th>\n"
       << "<th>Mean / Median</th>\n"
       << "<th>95<sup>th</sup> percentile</th>\n"
       << "<th>Maximum</th>\n"
       << "</tr>\n";

    os << "<tr>\n";
    os.printf( "<td class=\"right\">%.3f</td>", p._runes.rune_waste.min() );
    os.printf( "<td class=\"right\">%.3f</td>", p._runes.rune_waste.percentile( .05 ) );
    os.printf( "<td class=\"right\">%.3f / %.3f</td>", p._runes.rune_waste.mean(),
               p._runes.rune_waste.percentile( .5 ) );
    os.printf( "<td class=\"right\">%.3f</td>", p._runes.rune_waste.percentile( .95 ) );
    os.printf( "<td class=\"right\">%.3f</td>", p._runes.rune_waste.max() );
    os << "</tr>\n";
    os << "</table>\n";

    os << "<table class=\"sc\" style=\"float: left;margin-right: 10px;\">\n"
       << "<tr>\n"
       << "<th colspan=\"5\">Total Seconds per Iteration (n=" << p._runes.cumulative_waste.count() << ")</th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th>Minimum</th>\n"
       << "<th>5<sup>th</sup> percentile</th>\n"
       << "<th>Mean / Median</th>\n"
       << "<th>95<sup>th</sup> percentile</th>\n"
       << "<th>Maximum</th>\n"
       << "</tr>\n";

    os.printf( "<td class=\"right\">%.3f</td>", p._runes.cumulative_waste.min() );
    os.printf( "<td class=\"right\">%.3f</td>", p._runes.cumulative_waste.percentile( .05 ) );
    os.printf( "<td class=\"right\">%.3f / %.3f</td>", p._runes.cumulative_waste.mean(),
               p._runes.cumulative_waste.percentile( .5 ) );
    os.printf( "<td class=\"right\">%.3f</td>", p._runes.cumulative_waste.percentile( .95 ) );
    os.printf( "<td class=\"right\">%.3f</td>", p._runes.cumulative_waste.max() );
    os << "</tr>\n";

    os << "</table>\n";

    os << "</div>\n";
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    os << "<div class=\"player-section custom_section\">\n";
    if ( p._runes.cumulative_waste.percentile( .5 ) > 0 )
    {
      html_rune_waste( os );
    }
    os << "<div class=\"clear\"></div>\n";
    p.parsed_effects_html( os );
    os << "</div>\n";
  }

private:
  death_knight_t& p;
};

// DEATH_KNIGHT MODULE INTERFACE ============================================

struct death_knight_module_t : public module_t
{
  death_knight_module_t() : module_t( DEATH_KNIGHT )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p              = new death_knight_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new death_knight_report_t( *p ) );
    return p;
  }

  void static_init() const override
  {
    unique_gear::register_special_effect( 50401, runeforge::razorice );
    unique_gear::register_special_effect( 166441, runeforge::fallen_crusader );
    unique_gear::register_special_effect( 62157, runeforge::stoneskin_gargoyle );
    unique_gear::register_special_effect( 327087, runeforge::apocalypse );
    unique_gear::register_special_effect( 326801, runeforge::sanguination );
    unique_gear::register_special_effect( 326864, runeforge::spellwarding );
    unique_gear::register_special_effect( 326982, runeforge::unending_thirst );
    unique_gear::register_special_effect( 326913, runeforge::hysteria );
  }
  
  void register_hotfixes() const override
  {
    hotfix::register_effect( "Death Knight", "2024-09-6", "Unholy Commander Nerfed by 50%", 1161082,
                             hotfix::HOTFIX_FLAG_LIVE )
        .field( "base_value" )
        .operation( hotfix::HOTFIX_SET )
        .modifier( 1 )
        .verification_value( 2 );

    hotfix::register_effect( "Death Knight", "2024-09-6", "Unholy Commander Buff Nerfed by 50%", 1155631,
                             hotfix::HOTFIX_FLAG_LIVE )
        .field( "base_value" )
        .operation( hotfix::HOTFIX_SET )
        .modifier( 1 )
        .verification_value( 2 );

    hotfix::register_effect( "Death Knight", "2024-09-6", "Unholy Specilization Aura Direct Damage buffed by 5%", 179690,
                             hotfix::HOTFIX_FLAG_LIVE )
        .field( "base_value" )
        .operation( hotfix::HOTFIX_SET )
        .modifier( -5 )
        .verification_value( -10 );

    hotfix::register_effect( "Death Knight", "2024-09-6", "Unholy Specilization Periodic Aura buffed by 5%", 191170,
                             hotfix::HOTFIX_FLAG_LIVE )
        .field( "base_value" )
        .operation( hotfix::HOTFIX_SET )
        .modifier( -5 )
        .verification_value( -10 );

    hotfix::register_effect( "Death Knight", "2024-09-6", "Unholy Specilization Pet Aura buffed by 5%", 191171,
                             hotfix::HOTFIX_FLAG_LIVE )
        .field( "base_value" )
        .operation( hotfix::HOTFIX_SET )
        .modifier( -5 )
        .verification_value( -10 );

    hotfix::register_effect( "Death Knight", "2024-09-6", "Unholy Specilization Guardian Aura buffed by 5%", 1032341,
                             hotfix::HOTFIX_FLAG_LIVE )
        .field( "base_value" )
        .operation( hotfix::HOTFIX_SET )
        .modifier( -5 )
        .verification_value( -10 );
  }

  void init( player_t* ) const override
  {
  }
  bool valid() const override
  {
    return true;
  }
  void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
};

}  // UNNAMED NAMESPACE

const module_t* module_t::death_knight()
{
  static death_knight_module_t m;
  return &m;
}
