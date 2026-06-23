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
//
// Blood:
// - Check that VB's absorb increase is correctly implemented
// - Healing from Consumption damage done

#include <deque>

#include "config.hpp"

#include "action/action_callback.hpp"
#include "action/parse_effects.hpp"
#include "class_modules/apl/apl_death_knight.hpp"
#include "player/pet_spawner.hpp"
#include "report/charts.hpp"
#include "report/highchart.hpp"

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
action_t* get_action( std::string_view name, Actor* actor, Args&&... args )
{
  action_t* a = actor->find_action( name );
  if ( !a )
    a = new Action( name, actor, std::forward<Args>( args )... );
  assert( dynamic_cast<Action*>( a ) && a->name_str == name && a->background );
  return a;
}

// Only to be used with empowered release spells
template <typename Action, typename Actor, typename... Args>
action_t* get_empower_release_action( std::string_view name, Actor* actor, Args&&... args )
{
  action_t* a = actor->find_action( name );
  if ( !a )
    a = new Action( name, actor, std::forward<Args>( args )... );
  assert( dynamic_cast<Action*>( a ) && a->name_str == name && a->background == false );
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
struct lesser_ghoul_pet_t;
struct gargoyle_pet_t;
struct risen_skulker_pet_t;
struct dancing_rune_weapon_pet_t;
struct everlasting_bond_pet_t;
struct dance_of_midnight_pet_t;
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
void apocalypse( special_effect_t& );
void fallen_crusader( special_effect_t& );
void razorice( special_effect_t& );
void sanguination( special_effect_t& );
void spellwarding( special_effect_t& );
void stoneskin_gargoyle( special_effect_t& );
void unending_thirst( special_effect_t& );  // Effect only procs on killing blows, NYI
void init_runeforges();
}  // namespace runeforge

enum runeforge_apocalypse_e
{
  DEATH      = 0,
  FAMINE     = 1,
  PESTILENCE = 2,
  WAR        = 3,
  MAX        = 4
};

enum lesser_ghoul_e
{
  LESSER_DOOMED_BIDDING_COIL,
  LESSER_DOOMED_BIDDING_EPIDEMIC,
  LESSER_ARMY_OF_THE_DEAD,
  LESSER_FESTERING_STRIKE,
  LESSER_PUTREFY,
  LESSER_FORBIDDEN_KNOWLEDGE,
  LESSER_SOUL_REAPER,
};

enum magus_of_the_dead_e
{
  MAGUS_DARK_TRANSFORMATION,
  MAGUS_ARMY_OF_THE_DEAD,
  MAGUS_REANIMATION,
  MAGUS_REANIMATION_FK,
};

enum putrefy_source_e
{
  PUTREFY_SOURCE_NONE,
  PUTREFY_SOURCE_PUTREFY,
  PUTREFY_SOURCE_FORBIDDEN_KNOWLEDGE,
  PUTREFY_SOURCE_SOUL_REAPER
};

enum rider_of_the_apocalypse_e
{
  NONE       = -1,
  RANDOM     = 0,
  WHITEMANE  = 1,
  TROLLBANE  = 2,
  NAZGRIM    = 3,
  MOGRAINE   = 4,
  ALL_RIDERS = 5
};

enum runeforges_e
{
  RUNEFORGE_NONE = -1,
  RUNEFORGE_APOCALYPSE,
  RUNEFORGE_FALLEN_CRUSADER,
  RUNEFORGE_RAZORICE,
  RUNEFORGE_SANGUINATION,
  RUNEFORGE_SPELLWARDING,
  RUNEFORGE_STONESKIN_GARGOYLE,
  RUNEFORGE_UNENDING_THIRST,
  RUNEFORGE_ID_APOCALYPSE         = 6245,
  RUNEFORGE_ID_FALLEN_CRUSADER    = 3368,
  RUNEFORGE_ID_RAZORICE           = 3370,
  RUNEFORGE_ID_SANGUINATION       = 6241,
  RUNEFORGE_ID_SPELLWARDING       = 6242,
  RUNEFORGE_ID_STONESKIN_GARGOYLE = 3847,
  RUNEFORGE_ID_UNENDING_THIRST    = 6244,
};

enum drw_actions_e
{
  DRW_ACTION_BLOOD_BOIL      = 0,
  DRW_ACTION_DEATHS_CARESS   = 1,
  DRW_ACTION_DEATH_STRIKE    = 2,
  DRW_ACTION_HEART_STRIKE    = 3,
  DRW_ACTION_MARROWREND      = 4,
  DRW_ACTION_VAMPIRIC_STRIKE = 5,
  DRW_ACTION_MAX             = 6
};

enum empower_e
{
  EMPOWER_NONE = 0,
  EMPOWER_1    = 1,
  EMPOWER_2,
  EMPOWER_3,
  EMPOWER_4,
  EMPOWER_MAX
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

const double RUNIC_POWER_DECAY_RATE = -power_type_data_t::find( POWER_RUNIC_POWER ).regen_per_second( false );
const double RUNE_REGEN_BASE_SEC    = power_type_data_t::find( POWER_RUNE ).regen_per_second();
const double RUNE_REGEN_BASE        = ( 1 / RUNE_REGEN_BASE_SEC );

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
    propagate_const<dot_t*> virulent_plague;
    propagate_const<dot_t*> dread_plague;
    propagate_const<dot_t*> infected_claws;

    // Rider of the Apocalypse
    propagate_const<dot_t*> undeath;

    // Deathbringer
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
    propagate_const<buff_t*> abomination_limb;
    propagate_const<buff_t*> tightening_grasp;

    // Frost
    propagate_const<buff_t*> everfrost;
    propagate_const<buff_t*> frostreaper;

    // Unholy
    propagate_const<buff_t*> soul_reaper;

    // Rider of the Apocalypse
    propagate_const<buff_t*> chains_of_ice_trollbane_slow;
    propagate_const<buff_t*> chains_of_ice_trollbane_damage;

    // Deathbringer
    buff_t* reapers_mark;
    buff_t* wave_of_souls;

    // San'layn
    propagate_const<buff_t*> incite_terror;
  } debuff;

  struct flags_t
  {
    bool razorice_consumed;
  } flag;

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
  std::deque<ground_aoe_event_t*> active_dnds;
  event_t* runic_power_decay;

  // Expression warnings
  // for old dot.death_and_decay.x expressions
  bool deprecated_dnd_expression;
  // for runeforge.name expressions that call death knight runeforges instead of shadowlands legendary runeforges
  bool runeforge_expression_warning;

  // Counters
  unsigned int active_riders;     // Number of active Riders of the Apocalypse pets
  unsigned int magus_active;      // Number of active Magus of the Dead pets

  std::vector<player_t*> undeath_tl;

  // Buffs
  struct buffs_t
  {
    // Shared
    absorb_buff_t* antimagic_shell;
    absorb_buff_t* antimagic_shell_horsemen;
    buff_t* antimagic_shell_horsemen_icd;
    absorb_buff_t* antimagic_zone;
    propagate_const<buff_t*> blood_draw;
    propagate_const<buff_t*> icebound_fortitude;
    propagate_const<buff_t*> rune_mastery;
    propagate_const<buff_t*> icy_talons;
    propagate_const<buff_t*> lichborne;  // NYI
    propagate_const<buff_t*> death_and_decay;
    propagate_const<buff_t*> spellwarding;

    // Blood
    absorb_buff_t* blood_shield;
    propagate_const<buff_t*> bloodied_blade_stacks;
    propagate_const<buff_t*> bloodied_blade_final;
    propagate_const<buff_t*> blood_mist;
    propagate_const<buff_t*> boiling_point;
    propagate_const<buff_t*> boiling_point_echo;
    buff_t* bone_shield;
    propagate_const<buff_t*> coagulopathy;
    propagate_const<buff_t*> consumption;
    propagate_const<buff_t*> crimson_scourge;
    propagate_const<buff_t*> dancing_rune_weapon;
    propagate_const<buff_t*> hemostasis;
    propagate_const<buff_t*> ossuary;
    propagate_const<buff_t*> perseverance_of_the_ebon_blade;
    propagate_const<buff_t*> sanguine_ground;
    propagate_const<buff_t*> sanguinary_burst;
    propagate_const<buff_t*> vampiric_blood;
    propagate_const<buff_t*> voracious;

    propagate_const<buff_t*> dance_of_midnight_1;
    propagate_const<buff_t*> dance_of_midnight_2;

    // Frost
    propagate_const<buff_t*> breath_of_sindragosa;
    propagate_const<buff_t*> gathering_storm;
    propagate_const<buff_t*> inexorable_assault;
    propagate_const<buff_t*> empower_rune_weapon;
    propagate_const<buff_t*> killing_machine;
    buff_t* pillar_of_frost;
    propagate_const<buff_t*> remorseless_winter;
    propagate_const<buff_t*> rime;
    propagate_const<buff_t*> bonegrinder_crit;
    propagate_const<buff_t*> bonegrinder_frost;
    propagate_const<buff_t*> enduring_strength_builder;
    propagate_const<buff_t*> enduring_strength;
    propagate_const<buff_t*> frozen_dominion;
    buff_t* cryogenic_chamber;
    buff_t* frozen_dominion_remorseless_winter;
    propagate_const<buff_t*> frostbane;
    propagate_const<buff_t*> killing_streak;
    propagate_const<buff_t*> icy_onslaught;
    propagate_const<buff_t*> chosen_of_frostbrood_haste;
    propagate_const<buff_t*> chosen_of_frostbrood_fwf;
    // Tier Sets
    propagate_const<buff_t*> frost_mid1_4pc_buff;
    propagate_const<buff_t*> empowered_strikes;
    propagate_const<buff_t*> freezing_tempest;

    // Unholy
    propagate_const<buff_t*> dark_transformation;
    propagate_const<buff_t*> runic_corruption;
    propagate_const<buff_t*> sudden_doom;
    propagate_const<buff_t*> ghoulish_frenzy;
    propagate_const<buff_t*> commander_of_the_dead;
    propagate_const<buff_t*> commander_of_the_dead_damage;
    propagate_const<buff_t*> clawing_shadows;
    propagate_const<buff_t*> lesser_ghoul_ready;
    propagate_const<buff_t*> lesser_ghoul_counter;
    propagate_const<buff_t*> forbidden_knowledge;
    propagate_const<buff_t*> ancient_power;
    propagate_const<buff_t*> unholy_aura_haste;
    propagate_const<buff_t*> festering_scythe;
    propagate_const<buff_t*> festering_scythe_tt;
    propagate_const<buff_t*> forbidden_sacrifice;
    propagate_const<buff_t*> forbidden_ritual;
    propagate_const<buff_t*> reaping;
    propagate_const<buff_t*> blightfall;
    // Tier Sets
    propagate_const<buff_t*> blighted;

    // Rider of the Apocalypse
    propagate_const<buff_t*> a_feast_of_souls;
    buff_t* apocalyptic_conquest;
    propagate_const<buff_t*> mograines_might;

    // San'layn
    propagate_const<buff_t*> bloodsoaked_ground;
    propagate_const<buff_t*> essence_of_the_blood_queen;
    buff_t* gift_of_the_sanlayn;
    propagate_const<buff_t*> vampiric_strike;
    propagate_const<buff_t*> infliction_of_sorrow;
    propagate_const<buff_t*> visceral_strength;

    // Deathbringer
    propagate_const<buff_t*> bind_in_darkness;
    propagate_const<buff_t*> dark_talons_shadowfrost;
    propagate_const<buff_t*> dark_talons_icy_talons;
    propagate_const<buff_t*> exterminate;
    propagate_const<buff_t*> rune_carved_plates_physical_buff;
    propagate_const<buff_t*> rune_carved_plates_magical_buff;
    propagate_const<buff_t*> swift_and_painful;
    propagate_const<buff_t*> empowered_soul;

  } buffs;

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
    cooldown_t* blood_boil;
    cooldown_t* consumption;
    cooldown_t* plague_infusion_icd;
    cooldown_t* dancing_rune_weapon;
    propagate_const<cooldown_t*> vampiric_blood;
    cooldown_t* bloody_reflection_icd;
    // Frost
    propagate_const<cooldown_t*> inexorable_assault_icd;  // internal cooldown to prevent multiple procs during aoe
    propagate_const<cooldown_t*>
        enduring_strength_icd;  // internal cooldown that prevents several procs on the same dual-wield attacl
    propagate_const<cooldown_t*> pillar_of_frost;
    propagate_const<cooldown_t*> frostwyrms_fury;
    propagate_const<cooldown_t*> empower_rune_weapon;

    // Unholy
    propagate_const<cooldown_t*> army_of_the_dead;
    propagate_const<cooldown_t*> putrefy;
    propagate_const<cooldown_t*> soul_reaper;
  } cooldown;

  // Active Spells
  struct background_actions_t
  {
    // Shared

    // Class Tree
    propagate_const<action_t*> blood_draw;
    propagate_const<action_t*> permafrost;

    // Rider of the Apocalypse
    propagate_const<action_t*> undeath_dot;
    propagate_const<action_t*> trollbanes_icy_fury;

    // Sanlayn
    propagate_const<action_t*> vampiric_strike_heal;
    action_t* infliction_of_sorrow;
    action_t* the_blood_is_life;
    action_t* desecrate;

    // Blood
    action_t* heart_strike_bloodied_blade;
    action_t* blood_boil_boiling_point;
    action_t* bloody_reflection;
    action_t* blood_mist_tick;
    action_t* sanguinary_burst;

    // Deathbringer
    action_t* reapers_mark_explosion;
    action_t* wave_of_souls;
    action_t* soul_rupture;
    action_t* exterminate;
    action_t* exterminate_aoe;

    // Frost
    action_t* breath_of_sindragosa_tick;
    action_t* remorseless_winter_tick;
    action_t* frost_strike_main;
    action_t* frost_strike_offhand;
    action_t* frost_strike_sb_main;
    action_t* frost_strike_sb_offhand;
    propagate_const<action_t*> icy_death_torrent_damage;
    action_t* hyperpyrexia_damage;
    propagate_const<action_t*> erw_projectile;
    propagate_const<action_t*> frostreaper;
    propagate_const<action_t*> frozen_dominion_remorseless_winter;
    propagate_const<action_t*> arctic_assault_obliterate;
    propagate_const<action_t*> arctic_assault_frostscythe;

    // Unholy
    propagate_const<action_t*> virulent_plague;
    propagate_const<action_t*> outbreak_aoe;
    propagate_const<action_t*> death_coil_damage;
    propagate_const<action_t*> epidemic_main;
    propagate_const<action_t*> infected_claws;
    propagate_const<action_t*> coil_of_devastation;
    propagate_const<action_t*> virulent_plague_erupt;
    propagate_const<action_t*> dread_plague_erupt;
    propagate_const<action_t*> virulent_plague_erupt_ss;
    propagate_const<action_t*> dread_plague_erupt_ss;
    propagate_const<action_t*> virulent_plague_erupt_bb;
    propagate_const<action_t*> dread_plague_erupt_bb;
    propagate_const<action_t*> virulent_plague_erupt_pest;
    propagate_const<action_t*> dread_plague_erupt_pest;
    propagate_const<action_t*> dread_plague;
    propagate_const<action_t*> putrefy_st;
    propagate_const<action_t*> putrefy_aoe;
    propagate_const<action_t*> putrefy_fk_st;
    propagate_const<action_t*> putrefy_fk_aoe;
    propagate_const<action_t*> dread_plague_death;
  } background_actions;

  struct runeforge_actions_t
  {
    propagate_const<action_t*> apocalypse_pestilence;
    propagate_const<action_t*> fallen_crusader_heal;
    propagate_const<action_t*> razorice_damage;
    propagate_const<action_t*> sanguination_heal;
    propagate_const<action_t*> spellwarding_absorb;
  } runeforge_actions;

  struct pet_summon_actions_t
  {
    // Blood
    propagate_const<action_t*> bloodworm;

    // Unholy
    propagate_const<action_t*> army_ghoul;
    propagate_const<action_t*> fs_ghoul;
    propagate_const<action_t*> db_ghoul_coil;
    propagate_const<action_t*> db_ghoul_epi;
    propagate_const<action_t*> putrefy_ghoul;
    propagate_const<action_t*> fk_ghoul;
    propagate_const<action_t*> sr_ghoul;
    propagate_const<action_t*> reanimation_magus;
    propagate_const<action_t*> fk_reanimation_magus;
    propagate_const<action_t*> army_magus;
    propagate_const<action_t*> raise_skulker;

    // San'layn
    propagate_const<action_t*> blood_beast;

    // Rider of the Apocalypse
    action_t* summon_whitemane;
    action_t* summon_trollbane;
    action_t* summon_nazgrim;
    action_t* summon_mograine;
  } pet_summon;

  // Gains
  struct gains_t
  {
    // Shared
    propagate_const<gain_t*> antimagic_shell;  // RP from magic damage absorbed
    gain_t* rune;                              // Rune regeneration
    propagate_const<gain_t*> coldthirst;
    propagate_const<gain_t*> start_of_combat_overflow;

    // Blood
    propagate_const<gain_t*> blood_mist;
    propagate_const<gain_t*> consumption;
    propagate_const<gain_t*> drw_heart_strike;  // Blood Strike, Blizzard's hack to replicate HS rank 2 with DRW
    propagate_const<gain_t*> heartbreaker;
    propagate_const<gain_t*> blood_mid1_2pc;

    // Frost
    propagate_const<gain_t*> breath_of_sindragosa;
    propagate_const<gain_t*> empower_rune_weapon;
    propagate_const<gain_t*> frost_fever;  // RP generation per tick
    propagate_const<gain_t*> murderous_efficiency;
    propagate_const<gain_t*> obliteration;
    propagate_const<gain_t*> rage_of_the_frozen_champion;
    propagate_const<gain_t*> runic_attenuation;
    propagate_const<gain_t*> runic_empowerment;
    propagate_const<gain_t*> chosen_of_frostbrood;

    // Unholy
    propagate_const<gain_t*> forbidden_knowledge;
    propagate_const<gain_t*> superstrain;
    propagate_const<gain_t*> lesser_ghoul_energy;

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
    const spell_data_t* glacial_advance;

    // Unholy
    const spell_data_t* raise_dead;
    const spell_data_t* festering_strike;
    const spell_data_t* dark_transformation_2;
    const spell_data_t* outbreak;
    const spell_data_t* epidemic;
  } spec;

  // Mastery
  struct mastery_t
  {
    const spell_data_t* blood_shield;         // Blood
    const spell_data_t* frozen_heart;         // Frost
    const spell_data_t* dreadblade;           // Unholy
    const spell_data_t* dreadblade_pet_crit;  // Unholy, pet crit mastery
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
    player_talent_t unholy_momentum;
    player_talent_t control_undead;  // NYI
    player_talent_t enfeeble;        // NYI
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
    player_talent_t subduing_grasp;          // NYI
    player_talent_t will_of_the_necropolis;  // NYI
    // Row 10
    player_talent_t null_magic;  // NYI
    player_talent_t unyielding_will;
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
      player_talent_t relish_in_blood;
      // Row 5
      player_talent_t leeching_strike;
      player_talent_t heartbreaker;
      player_talent_t foul_bulwark;
      player_talent_t dancing_rune_weapon;
      player_talent_t hemostasis;
      player_talent_t perseverance_of_the_ebon_blade;
      player_talent_t bloodworms;
      // Row 6
      player_talent_t gorefiends_grasp;
      player_talent_t abomination_limb;
      player_talent_t improved_bone_shield;
      player_talent_t insatiable_blade;
      player_talent_t deadly_reach;
      player_talent_t rapid_decomposition;
      // Row 7
      player_talent_t boiling_point;
      player_talent_t lifeblood;
      player_talent_t everlasting_bond;
      player_talent_t voracious;
      player_talent_t blood_feast;
      // Row 8
      player_talent_t plague_infusion;
      player_talent_t bloody_reflection;
      player_talent_t iron_heart;
      player_talent_t bloodied_blade;
      player_talent_t coagulopathy;
      // Row 9
      player_talent_t blood_mist;
      player_talent_t sanguine_ground;
      player_talent_t bloodshot;
      player_talent_t consumption;
      player_talent_t red_thirst;
      // Row 10
      player_talent_t sanguinary_burst;
      player_talent_t purgatory;
      player_talent_t carnage;
      player_talent_t umbilicus_eternus;
      // Apex
      player_talent_t dance_of_midnight_1;
      player_talent_t dance_of_midnight_2;
      player_talent_t dance_of_midnight_3;
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
      player_talent_t empower_rune_weapon;
      player_talent_t frostscythe;
      // Row 4
      player_talent_t arctic_assault;
      player_talent_t runic_overflow;
      player_talent_t frostbound_will;
      player_talent_t runic_command;
      player_talent_t biting_cold;
      // Row 5
      player_talent_t frostreaper;
      player_talent_t pillar_of_frost;
      player_talent_t icy_onslaught;
      player_talent_t gathering_storm;
      // Row 6
      player_talent_t howling_blades;
      player_talent_t inexorable_assault;
      player_talent_t enduring_strength;
      player_talent_t frostwyrms_fury;
      player_talent_t frigid_executioner;
      // Row 7
      player_talent_t murderous_efficiency;
      player_talent_t cryogenic_chamber;
      player_talent_t rage_of_the_frozen_champion;
      player_talent_t frozen_dominion;
      player_talent_t everfrost;
      player_talent_t northwinds;
      // Row 8
      player_talent_t bonegrinder;
      player_talent_t smothering_offense;
      player_talent_t avalanche;
      player_talent_t icebreaker;
      // Row 9
      player_talent_t obliteration;
      player_talent_t icy_death_torrent;
      player_talent_t shattering_blade;
      player_talent_t hyperpyrexia;
      // Row 10
      player_talent_t killing_streak;
      player_talent_t the_long_winter;
      player_talent_t frostbane;
      player_talent_t breath_of_sindragosa;
      // Apex
      player_talent_t chosen_of_frostbrood_1;
      player_talent_t chosen_of_frostbrood_2;
      player_talent_t chosen_of_frostbrood_3;
    } frost;

    // Unholy
    struct
    {
      // Row 1
      player_talent_t outbreak;
      // Row 2
      player_talent_t scourge_strike;
      player_talent_t sudden_doom;
      // Row 3
      player_talent_t clawing_shadows;
      player_talent_t putrefy;
      player_talent_t dark_transformation;
      // Row 4
      player_talent_t foul_infections;
      player_talent_t plague_mastery;
      player_talent_t grave_mastery;
      player_talent_t necromancers_cunning;
      // Row 5
      player_talent_t ebon_fever;
      player_talent_t festering_scythe;
      player_talent_t superstrain;
      player_talent_t soul_reaper;
      player_talent_t magus_of_the_dead;
      player_talent_t infected_claws;
      // Row 6
      player_talent_t cycle_of_death;
      player_talent_t coil_of_devastation;
      player_talent_t army_of_the_dead;
      player_talent_t reaping;
      player_talent_t menacing_magus;
      player_talent_t ghoulish_frenzy;
      // Row 7
      player_talent_t morbidity;
      player_talent_t harbinger_of_doom;
      player_talent_t raise_abomination;
      player_talent_t summon_gargoyle;
      player_talent_t unholy_devotion;
      player_talent_t all_will_serve;
      // Row 8
      player_talent_t blightburst;
      player_talent_t putrid_echoes;
      player_talent_t doomed_bidding;
      // Row 9
      player_talent_t scourging;
      player_talent_t ancient_power;
      player_talent_t unholy_aura;
      player_talent_t commander_of_the_dead;
      // Row 10
      player_talent_t blightfall;
      player_talent_t reanimation;
      player_talent_t outnumber;
      // Apex
      player_talent_t forbidden_knowledge_1;
      player_talent_t forbidden_knowledge_2;
      player_talent_t forbidden_knowledge_3;
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
      player_talent_t ride_or_die;
      player_talent_t whitemanes_famine;
      player_talent_t nazgrims_conquest;
      player_talent_t trollbanes_icy_fury;
      player_talent_t let_terror_reign;
      player_talent_t hungering_thirst;
      player_talent_t fury_of_the_horsemen;
      player_talent_t a_feast_of_souls;
      player_talent_t mawsworn_menace;
      player_talent_t unholy_armaments;
      player_talent_t apocalypse_now;
    } rider;

    // Deathbringer
    struct
    {
      player_talent_t reapers_mark;
      player_talent_t wave_of_souls;
      player_talent_t wither_away;
      player_talent_t bind_in_darkness;
      player_talent_t frigid_resolve;
      player_talent_t soul_rupture;
      player_talent_t grim_reaper;
      player_talent_t pact_of_the_deathbringer;  // NYI
      player_talent_t rune_carved_plates;
      player_talent_t deathly_blows;
      player_talent_t swift_and_painful;
      player_talent_t dark_talons;
      player_talent_t reapers_onslaught;
      player_talent_t deaths_messenger;
      player_talent_t expelling_shield;  // NYI
      player_talent_t echoing_fury;      
      player_talent_t exterminate;
    } deathbringer;

    // San'layn
    struct
    {
      player_talent_t vampiric_strike;
      player_talent_t newly_turned;    // NYI
      player_talent_t vampiric_speed;  // NYI
      player_talent_t bloodsoaked_ground;
      player_talent_t desecrate;
      player_talent_t vampiric_aura;     // NYI
      player_talent_t bloody_fortitude;  // NYI
      player_talent_t thrill_of_blood;
      player_talent_t infliction_of_sorrow;
      player_talent_t frenzied_bloodthirst;
      player_talent_t the_blood_is_life;
      player_talent_t visceral_strength;
      player_talent_t inevitable;
      player_talent_t incite_terror;
      player_talent_t pact_of_the_sanlayn;
      player_talent_t sanguine_scent;
      player_talent_t transfusion;
      player_talent_t gift_of_the_sanlayn;
    } sanlayn;
  } talent;

  // Spells
  struct spells_t
  {
    // Shared
    const spell_data_t* brittle_debuff;
    const spell_data_t* dnd_buff;  // obliterate aoe increase while in death's due (nf covenant ability)
    const spell_data_t* rune_mastery_buff;
    const spell_data_t* coldthirst_gain;  // Coldthirst has a unique ID for the gain and cooldown reduction
    const spell_data_t* death_and_decay_damage;
    const spell_data_t* death_coil_damage;
    const spell_data_t* death_strike_heal;
    const spell_data_t* sacrificial_pact_damage;
    const spell_data_t* anti_magic_zone_buff;
    const spell_data_t* frost_shield_buff;

    // Diseases (because they're not stored in spec data, unlike frost fever's rp gen...)
    const spell_data_t* blood_plague;
    const spell_data_t* frost_fever;
    const spell_data_t* virulent_plague;

    // Blood
    const spell_data_t* abomination_limb_debuff;
    const spell_data_t* blood_shield;
    const spell_data_t* bloodied_blade_stacks_buff;
    const spell_data_t* bloodied_blade_final_buff;
    const spell_data_t* blood_mist_buff;
    const spell_data_t* blood_mist_damage;
    const spell_data_t* blood_mist_rp_gain;
    const spell_data_t* bloody_reflection_damage;
    const spell_data_t* boiling_point_buff;
    const spell_data_t* boiling_point_echo_buff;
    const spell_data_t* bone_shield;
    const spell_data_t* dance_of_midnight_1_buff;
    const spell_data_t* dance_of_midnight_2_buff;
    const spell_data_t* sanguine_ground;
    const spell_data_t* sanguinary_burst_buff;
    const spell_data_t* sanguinary_burst_damage;
    const spell_data_t* ossuary_buff;
    const spell_data_t* crimson_scourge_buff;
    const spell_data_t* heartbreaker_rp_gain;
    const spell_data_t* heart_strike_bloodied_blade;
    const spell_data_t* perserverence_of_the_ebon_blade_buff;
    const spell_data_t* tightening_grasp_debuff;
    const spell_data_t* voracious_buff;
    const spell_data_t* blood_draw_damage;
    const spell_data_t* blood_draw_cooldown;
    const spell_data_t* dancing_rune_weapon_buff;
    const spell_data_t* relish_in_blood_gains;
    const spell_data_t* leeching_strike_damage;
    const spell_data_t* consumption_damage;
    const spell_data_t* consumption_leech;
    const spell_data_t* everlasting_bond_summon;
    const spell_data_t* dance_of_midnight_summon;

    // Blood Tier Set Spells
    const spell_data_t* rejuvenating_blood; // 2pc rp gain

    // Frost
    const spell_data_t* runic_empowerment_gain;
    const spell_data_t* murderous_efficiency_gain;
    const spell_data_t* rage_of_the_frozen_champion;  // RP generation spell
    const spell_data_t* runic_empowerment_chance;
    const spell_data_t* gathering_storm_buff;
    const spell_data_t* inexorable_assault_buff;
    const spell_data_t* bonegrinder_crit_buff;
    const spell_data_t* bonegrinder_frost_buff;
    const spell_data_t* enduring_strength_buff;
    const spell_data_t* frozen_dominion_buff;
    const spell_data_t* inexorable_assault_damage;
    const spell_data_t* breath_of_sindragosa_rune_gen;
    const spell_data_t* death_strike_offhand;
    const spell_data_t* frostwyrms_fury_damage;
    const spell_data_t* glacial_advance_damage;
    const spell_data_t* avalanche_damage;
    const spell_data_t* enduring_strength_cooldown;
    const spell_data_t* frost_strike_2h;
    const spell_data_t* frost_strike_mh;
    const spell_data_t* frost_strike_oh;
    const spell_data_t* obliteration_gains;
    const spell_data_t* icy_death_torrent_damage;
    const spell_data_t* cryogenic_chamber_damage;
    const spell_data_t* cryogenic_chamber_buff;
    const spell_data_t* rime_buff;
    const spell_data_t* hyperpyrexia_damage;
    const spell_data_t* empower_rune_weapon_projectile;
    const spell_data_t* empower_rune_weapon_buff;
    const spell_data_t* frostreaper_debuff;
    const spell_data_t* frostreaper_damage;
    const spell_data_t* icy_onslaught_buff;
    const spell_data_t* first_howling_blades_damage;
    const spell_data_t* second_howling_blades_damage;
    const spell_data_t* frozen_dominion_remorseless_winter_buff;
    const spell_data_t* frostbane_buff;
    const spell_data_t* frostbane_driver;
    const spell_data_t* frostbane_damage;
    const spell_data_t* breath_of_sindragosa_erw_refund;
    const spell_data_t* killing_streak_buff;
    const spell_data_t* chosen_of_frostbrood_haste_buff;
    const spell_data_t* chosen_of_frostbrood_fwf_buff;
    const spell_data_t* chosen_of_frostbrood_delay;
    const spell_data_t* chosen_of_frostbrood_fwf_action;

    // Unholy
    const spell_data_t* runic_corruption;  // buff
    const spell_data_t* runic_corruption_chance;
    const spell_data_t* coil_of_devastation_debuff;
    const spell_data_t* ghoulish_frenzy_player;
    const spell_data_t* commander_of_the_dead;
    const spell_data_t* eternal_agony;
    const spell_data_t* epidemic_damage;
    const spell_data_t* outbreak_aoe;
    const spell_data_t* festering_scythe;
    const spell_data_t* dark_transformation_player_buff;
    const spell_data_t* reaping_buff;
    const spell_data_t* dread_plague;
    const spell_data_t* superstrain_energize;
    const spell_data_t* clawing_shadows_buff;
    const spell_data_t* lesser_ghoul_buff;
    const spell_data_t* lesser_ghoul_counter;
    const spell_data_t* lesser_ghoul_ticking;
    const spell_data_t* infected_claws_dot;
    const spell_data_t* infected_claws_driver;
    const spell_data_t* forbidden_knowledge_buff;
    const spell_data_t* forbidden_knowledge_energize;
    const spell_data_t* necrotic_coil_action;
    const spell_data_t* necrotic_coil_shadow;
    const spell_data_t* necrotic_coil_physical;
    const spell_data_t* graveyard_action;
    const spell_data_t* graveyard_damage;
    const spell_data_t* virulent_plague_erupt;
    const spell_data_t* dread_plague_erupt;
    const spell_data_t* ancient_runes_buff;
    const spell_data_t* disease_cloud_area;
    const spell_data_t* festering_scythe_debuff;
    const spell_data_t* dread_plague_death_damage;
    const spell_data_t* unholy_aura_mastery_buff;
    const spell_data_t* putrefy_st;
    const spell_data_t* putrefy_aoe;
    const spell_data_t* soul_reaper_debuff;
    const spell_data_t* festering_scythe_buff;
    const spell_data_t* blightfall;
    const spell_data_t* blightfall_buff;
    const spell_data_t* blightfall_damage;
    const spell_data_t* forbidden_sacrifice;
    const spell_data_t* forbidden_ritual;
    const spell_data_t* disease_cloud_damage;

    // Unholy Summon Spells
    const spell_data_t* summon_gargoyle;
    const spell_data_t* summon_abomination;
    const spell_data_t* summon_army_ghoul;
    const spell_data_t* summon_lesser_ghoul;
    const spell_data_t* summon_putrefy_ghoul;
    const spell_data_t* summon_magus;
    const spell_data_t* summon_reanimation_magus;
    const spell_data_t* raise_skulker;

    // Unholy Tier Set Spells
    const spell_data_t* blighted_buff;

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
    const spell_data_t* vampiric_strike_range;
    const spell_data_t* incite_terror_debuff;
    const spell_data_t* visceral_strength_buff;
    const spell_data_t* bloodsoaked_ground_buff;
    const spell_data_t* transfusion_buff;
    const spell_data_t* desecrate_damage;

    // Deathbringer spells
    const spell_data_t* reapers_mark_debuff;
    const spell_data_t* reapers_mark_explosion;
    const spell_data_t* reapers_mark_grim_reaper;
    const spell_data_t* wave_of_souls_damage;
    const spell_data_t* wave_of_souls_debuff;
    const spell_data_t* bind_in_darkness_buff;
    const spell_data_t* dark_talons_shadowfrost_buff;
    const spell_data_t* dark_talons_icy_talons_buff;
    const spell_data_t* soul_rupture_damage;
    const spell_data_t* exterminate_damage;
    const spell_data_t* exterminate_aoe;
    const spell_data_t* exterminate_buff;
    const spell_data_t* rune_carved_plates_physical_buff;
    const spell_data_t* rune_carved_plates_magical_buff;
    const spell_data_t* swift_and_painful_buff;
  } spell;

  struct runeforge_spells_t
  {
    const spell_data_t* apocalypse_death_debuff;
    const spell_data_t* apocalypse_famine_debuff;
    const spell_data_t* apocalypse_war_debuff;
    const spell_data_t* apocalypse_pestilence_damage;
    const spell_data_t* razorice_damage;
    const spell_data_t* razorice_debuff;
    const spell_data_t* sanguination_cooldown;
    const spell_data_t* sanguination_heal;
    const spell_data_t* spellwarding_driver;
    const spell_data_t* spellwarding_absorb;
    const spell_data_t* stoneskin_gargoyle;
    const spell_data_t* unending_thirst;
    const spell_data_t* unholy_strength;
  } runeforge_spell;

  // Pet Abilities
  struct pet_spells_t
  {
    // Shared
    const spell_data_t* grave_mastery_buff;
    // Raise dead ghoul
    const spell_data_t* ghoul_claw;
    const spell_data_t* sweeping_claws;
    const spell_data_t* gnaw;
    const spell_data_t* monstrous_blow;
    // Army of the dead
    const spell_data_t* army_claw;
    // All Ghouls
    const spell_data_t* pet_stun;
    const spell_data_t* leap;
    // Lesser Ghouls
    const spell_data_t* unholy_devotion_buff;
    const spell_data_t* lesser_sweeping_claws;
    const spell_data_t* ruptured_viscera;
    // Gargoyle
    const spell_data_t* gargoyle_strike;
    const spell_data_t* dark_empowerment;
    // All Will Serve
    const spell_data_t* blighted_arrow;
    const spell_data_t* blighted_arrow_aoe_buff;
    const spell_data_t* blighted_arrow_st_buff;
    // Magus of the Dead
    const spell_data_t* frostbolt;
    const spell_data_t* shadow_bolt;
    // Commander of the Dead Talent
    const spell_data_t* commander_of_the_dead;
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
    const spell_data_t* mograines_death_and_decay_aura;
    const spell_data_t* mograines_death_and_decay;
    const spell_data_t* mograines_might_buff;
    const spell_data_t* rider_ams;
    const spell_data_t* rider_ams_icd;
    const spell_data_t* whitemane_death_coil;
    const spell_data_t* whitemane_epidemic;
    const spell_data_t* mograine_heart_strike;
    const spell_data_t* trollbane_obliterate;
    const spell_data_t* trollbane_frostscythe;
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
    real_ppm_t* carnage;
    real_ppm_t* blood_beast;
    real_ppm_t* frostreaper;
  } rppm;

  struct pseudo_random_t
  {
    // Blood
    accumulated_rng_t* dance_of_midnight;
    // Frost
    accumulated_rng_t* killing_machine;
    // Unholy
    accumulated_rng_t* sudden_doom;
    accumulated_rng_t* forbidden_knowledge;
  } pseudo_random;

  // Pets and Guardians
  struct pets_t
  {
    // Shared
    spawner::pet_spawner_t<pets::ghoul_pet_t, death_knight_t> ghoul_pet;
    // Blood
    spawner::pet_spawner_t<pets::dancing_rune_weapon_pet_t, death_knight_t> dancing_rune_weapon_pet;
    spawner::pet_spawner_t<pets::dancing_rune_weapon_pet_t, death_knight_t> everlasting_bond_pet;
    spawner::pet_spawner_t<pets::dancing_rune_weapon_pet_t, death_knight_t> dance_of_midnight_pet;
    spawner::pet_spawner_t<pets::bloodworm_pet_t, death_knight_t> bloodworms;
    // Frost

    // Unholy
    spawner::pet_spawner_t<pets::lesser_ghoul_pet_t, death_knight_t> lesser_ghoul;
    spawner::pet_spawner_t<pets::lesser_ghoul_pet_t, death_knight_t> lesser_ghoul_army;
    spawner::pet_spawner_t<pets::lesser_ghoul_pet_t, death_knight_t> lesser_ghoul_db_coil;
    spawner::pet_spawner_t<pets::lesser_ghoul_pet_t, death_knight_t> lesser_ghoul_db_epi;
    spawner::pet_spawner_t<pets::lesser_ghoul_pet_t, death_knight_t> lesser_ghoul_fs;
    spawner::pet_spawner_t<pets::lesser_ghoul_pet_t, death_knight_t> lesser_ghoul_putrefy;
    spawner::pet_spawner_t<pets::lesser_ghoul_pet_t, death_knight_t> lesser_ghoul_fk;
    spawner::pet_spawner_t<pets::gargoyle_pet_t, death_knight_t> gargoyle;
    spawner::pet_spawner_t<pets::risen_skulker_pet_t, death_knight_t> risen_skulker;
    spawner::pet_spawner_t<pets::magus_pet_t, death_knight_t> magus_of_the_dead;
    spawner::pet_spawner_t<pets::magus_pet_t, death_knight_t> dt_magus;
    spawner::pet_spawner_t<pets::magus_pet_t, death_knight_t> army_magus;
    spawner::pet_spawner_t<pets::magus_pet_t, death_knight_t> reanimation_magus;
    spawner::pet_spawner_t<pets::abomination_pet_t, death_knight_t> abomination;
    // Rider of the Apocalypse
    spawner::pet_spawner_t<pets::mograine_pet_t, death_knight_t> mograine;
    spawner::pet_spawner_t<pets::whitemane_pet_t, death_knight_t> whitemane;
    spawner::pet_spawner_t<pets::trollbane_pet_t, death_knight_t> trollbane;
    spawner::pet_spawner_t<pets::nazgrim_pet_t, death_knight_t> nazgrim;
    // San'layn
    spawner::pet_spawner_t<pets::blood_beast_pet_t, death_knight_t> blood_beast;

    pets_t( death_knight_t* p )
      : ghoul_pet( "ghoul", p ),
        dancing_rune_weapon_pet( "dancing_rune_weapon", p ),
        everlasting_bond_pet( "everlasting_bond", p ),
        dance_of_midnight_pet( "dance_of_midnight", p ),
        bloodworms( "bloodworm", p ),
        lesser_ghoul( "lesser_ghoul", p ),
        lesser_ghoul_army( "lesser_ghoul_army", p ),
        lesser_ghoul_db_coil( "lesser_ghoul_db_coil", p ),
        lesser_ghoul_db_epi( "lesser_ghoul_db_epi", p ),
        lesser_ghoul_fs( "lesser_ghoul_fs", p ),
        lesser_ghoul_putrefy( "lesser_ghoul_putrefy", p ),
        lesser_ghoul_fk( "lesser_ghoul_forbidden_knowledge", p ),
        gargoyle( "gargoyle", p ),
        risen_skulker( "risen_skulker", p ),
        magus_of_the_dead( "magus_of_the_dead", p ),
        dt_magus( "dt_magus", p ),
        army_magus( "army_magus", p ),
        reanimation_magus( "reanimation_magus", p ),
        abomination( "abomination", p ),
        mograine( "mograine", p ),
        whitemane( "whitemane", p ),
        trollbane( "trollbane", p ),
        nazgrim( "nazgrim", p ),
        blood_beast( "blood_beast", p )
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
    propagate_const<proc_t*> km_from_grim_reaper;
    propagate_const<proc_t*> km_from_erw;
    propagate_const<proc_t*> km_from_howling_blades;
    propagate_const<proc_t*> km_from_exterminate;

    // Killing machine refreshed by
    propagate_const<proc_t*> km_from_crit_aa_wasted;
    propagate_const<proc_t*> km_from_obliteration_fs_wasted;  // Frost Strike during Obliteration
    propagate_const<proc_t*> km_from_obliteration_hb_wasted;  // Howling Blast during Obliteration
    propagate_const<proc_t*> km_from_obliteration_ga_wasted;  // Glacial Advance during Obliteration
    propagate_const<proc_t*> km_from_grim_reaper_wasted;
    propagate_const<proc_t*> km_from_erw_wasted;
    propagate_const<proc_t*> km_from_howling_blades_wasted;
    propagate_const<proc_t*> km_from_exterminate_wasted;

    // Razorice applied by
    propagate_const<proc_t*> razorice_from_arctic_assault;
    propagate_const<proc_t*> razorice_from_avalanche;
    propagate_const<proc_t*> razorice_from_glacial_advance;
    propagate_const<proc_t*> razorice_from_runeforge;

    // Lesser Ghoul Sources
    propagate_const<proc_t*> lesser_ghoul_db;
    propagate_const<proc_t*> lesser_ghoul_fs;
    propagate_const<proc_t*> lesser_ghoul_army;

    // Runic corruption triggered by
    propagate_const<proc_t*> rp_runic_corruption;

    // Visceral Strength RP spender procs
    propagate_const<proc_t*> coil_vs;
    propagate_const<proc_t*> epi_vs;

    // San'layn procs
    propagate_const<proc_t*> blood_beast;
    propagate_const<proc_t*> vampiric_strike;
    propagate_const<proc_t*> vampiric_strike_waste;

    // Deathbringer procs
    propagate_const<proc_t*> exterminate_reapers_mark;
  } procs;

  struct sample_data_t
  {
    std::unique_ptr<extended_sample_data_t> lesser_ghoul_duration;
    std::unique_ptr<extended_sample_data_t> lesser_ghouls_summoned;
    std::unique_ptr<extended_sample_data_t> lesser_ghouls_active;
    std::unique_ptr<extended_sample_data_t> magus_active;
    std::unique_ptr<extended_sample_data_t> blightfall_dp_dur;
    std::unique_ptr<extended_sample_data_t> blightfall_vp_dur;
  } sample_data;

  // Death Knight Options
  struct options_t
  {
    bool disable_aotd                     = false;
    bool split_ghoul_regen                = false;
    double ams_absorb_percent             = 0;
    bool individual_pet_reporting         = false;
    bool amz_specified                    = false;
    double average_cs_travel_time         = 0.4;
    timespan_t first_ams_cast             = 20_s;
    double horsemen_ams_absorb_percent    = 0.6;
    double average_mograines_might_uptime = 0.65;
    bool extra_unholy_reporting           = false;
    bool wcl_reporting_mode               = true;
  } options;

  // Runes
  runes_t _runes;
  // Other
  rider_of_the_apocalypse_e last_summoned_rider;
  std::vector<pets::death_knight_pet_t*> dk_active_pets;
  std::vector<pets::lesser_ghoul_pet_t*> active_lesser_ghouls;
  runeforges_e mh_runeforge;
  runeforges_e oh_runeforge;
  player_t* last_target;
  int lesser_ghouls_summoned;
  uptime_t* lesser_ghoul_uptimes;

  death_knight_t( sim_t* sim, std::string_view name, race_e r )
    : parse_player_effects_t( sim, DEATH_KNIGHT, name, r ),
      active_dnds(),
      runic_power_decay( nullptr ),
      deprecated_dnd_expression( false ),
      runeforge_expression_warning( false ),
      active_riders( 0 ),
      magus_active( 0 ),
      undeath_tl(),
      buffs(),
      background_actions(),
      runeforge_actions(),
      gains(),
      spec(),
      mastery(),
      talent(),
      spell(),
      pet_spell(),
      pseudo_random(),
      pets( this ),
      procs(),
      options(),
      _runes( this ),
      last_summoned_rider( rider_of_the_apocalypse_e::NONE ),
      dk_active_pets(),
      active_lesser_ghouls(),
      mh_runeforge( RUNEFORGE_NONE ),
      oh_runeforge( RUNEFORGE_NONE ),
      last_target( this ),
      lesser_ghouls_summoned( 0 ),
      lesser_ghoul_uptimes( nullptr )
  {
    // Shared
    // DnD - Default value, changed during action construction
    cooldown.death_and_decay_dynamic = get_cooldown( "death_and_decay" );
    cooldown.death_grip              = get_cooldown( "death_grip" );
    cooldown.mind_freeze             = get_cooldown( "mind_freeze" );

    // Blood
    cooldown.bone_shield_icd     = get_cooldown( "bone_shield_icd" );
    cooldown.blood_boil          = get_cooldown( "blood_boil" );
    cooldown.consumption         = get_cooldown( "consumption" );
    cooldown.plague_infusion_icd = get_cooldown( "plague_infusion_icd" );
    cooldown.dancing_rune_weapon = get_cooldown( "dancing_rune_weapon" );
    cooldown.vampiric_blood      = get_cooldown( "vampiric_blood" );
    cooldown.bloody_reflection_icd   = get_cooldown( "bloody_reflection_icd" );

    // Frost
    cooldown.inexorable_assault_icd = get_cooldown( "inexorable_assault_icd" );
    cooldown.pillar_of_frost        = get_cooldown( "pillar_of_frost" );
    cooldown.enduring_strength_icd  = get_cooldown( "enduring_strength" );
    cooldown.frostwyrms_fury        = get_cooldown( "frostwyrms_fury" );
    cooldown.empower_rune_weapon    = get_cooldown( "empower_rune_weapon" );

    // Unholy
    cooldown.army_of_the_dead = get_cooldown( "army_of_the_dead" );
    cooldown.putrefy          = get_cooldown( "putrefy" );
    cooldown.soul_reaper      = get_cooldown( "soul_reaper" );

    // Deathbringer

    // Rider of the Apocalypse

    // San'layn

    resource_regeneration = regen_type::DYNAMIC;
  }

  template <typename T>
  bool dot_or_debuff_active( T d, death_knight_td_t* t )
  {
    if constexpr ( std::is_invocable_v<T, death_knight_td_t::debuffs_t> )
      return std::invoke( d, t->debuff )->check() > 0;

    else if constexpr ( std::is_invocable_v<T, death_knight_td_t::dots_t> )
      return std::invoke( d, t->dot )->is_ticking();

    else
    {
      sim->error( SEVERE, "%s dot_or_debuff_active: Unsupported type passed.\n", name() );
      return false;
    }
  }

  // Character Definition overrides
  void init_spells() override;
  void init_action_list() override;
  void init_blizzard_action_list() override;
  parsed_assisted_combat_rule_t parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                            const assisted_combat_step_data_t& step ) const override;
  std::vector<std::string> action_names_from_spell_id( unsigned int spell_id ) const override;
  std::string aura_expr_from_spell_id( unsigned int spell_id, bool on_self ) const override;
  void init_rng() override;
  void init_base_stats() override;
  void init_scaling() override;
  void create_buffs() override;
  void init_gains() override;
  void init_procs() override;
  void init_uptimes() override;
  void init_special_effects() override;
  void init_finished() override;
  bool validate_fight_style( fight_style_e style ) const override;
  bool validate_actor() override;
  double composite_attribute( attribute_e ) const override;
  double composite_bonus_armor() const override;
  void combat_begin() override;
  void activate() override;
  void reset() override;
  void arise() override;
  void adjust_dynamic_cooldowns() override;
  void assess_damage( school_e, result_amount_type, action_state_t* ) override;
  void assess_damage_imminent( school_e, result_amount_type, action_state_t* ) override;
  void do_damage( action_state_t* ) override;
  void create_actions() override;
  action_t* create_action( std::string_view name, std::string_view options ) override;
  std::unique_ptr<expr_t> create_expression( std::string_view name ) override;
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
  void apply_action_effects( action_t* a, bool pet = false );
  void apply_target_action_effects( action_t* a, bool pet = false );
  void trigger_movement( double distance, movement_direction_type ) override;

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
  bool has_runeforge( runeforges_e rf ) const;
  void set_runeforges();
  void spell_lookups();
  void set_icds();
  bool in_death_and_decay() const;
  void parse_player_effects();
  const spell_data_t* conditional_spell_lookup( bool fn, int id );
  void trigger_rune_of_the_apocalypse( player_t* target );
  // Rider of the Apocalypse
  rider_of_the_apocalypse_e get_random_rider();
  void summon_rider( timespan_t duration, rider_of_the_apocalypse_e = rider_of_the_apocalypse_e::RANDOM );
  void extend_rider( double amount, pets::horseman_pet_t* rider );
  void trigger_whitemanes_famine( player_t* target );
  void spread_undeath( death_knight_td_t* main_td, player_t* new_target );
  void sort_undeath_targets( std::vector<player_t*> tl );
  void start_a_feast_of_souls();
  // San'layn
  void trigger_infliction_of_sorrow( player_t* target, bool is_vampiric );
  void trigger_vampiric_strike_proc( player_t* target );
  void trigger_sanlayn_execute_talents( bool is_vampiric, bool summoned_ghoul = false );
  // Deathbringer
  void trigger_reapers_mark_death( player_t* target );
  void reapers_mark_explosion_wrapper( player_t* target, player_t* source, int stacks );
  // Blood
  void bone_shield_handler( const action_state_t* ) const;
  void drw_action_execute( pets::dancing_rune_weapon_pet_t* drw, drw_actions_e action );
  void trigger_drw_action( drw_actions_e action );
  // Frost
  void trigger_killing_machine( bool predictable, proc_t* proc, proc_t* wasted_proc );
  void consume_killing_machine( proc_t* proc, timespan_t total_delay, action_t* aa_action );
  void trigger_runic_empowerment( double rpcost );
  void trigger_bonegrinder( int stacks );
  // Unholy
  void trigger_runic_corruption( proc_t* proc, double rpcost, double override_chance = -1.0,
                                 bool death_trigger = false );
  void trigger_dread_plague_death( player_t* t );
  void sudden_doom_execute_effects( bool coil = false );
  void sudden_doom_impact_effects( action_state_t* state, bool coil = false );
  void unholy_rp_execute_effects( bool sd, bool coil = false );
  void unholy_rp_impact_effects( action_state_t* state, bool sd, bool coil = false );
  // Start the repeated stacking of buffs, called at combat start
  void start_inexorable_assault();

  // Runeforge expression handling for Death Knight Runeforges (not legendary)
  std::unique_ptr<expr_t> create_runeforge_expression( std::string_view runeforge_name, bool warning );

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
  T_CONTAINER* get_data_entry( std::string_view name, std::vector<T_DATA*>& entries )
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
  struct
  {
    bool commander_of_the_dead = false;
    bool grave_mastery = false;
    bool mastery_dreadblade_crit = false;
  } affected_by;

  bool use_auto_attack, precombat_spawn, guardian;
  timespan_t precombat_spawn_adjust;
  bool is_magus;
  double commander_value;
  buff_t* grave_mastery;
  buff_t* mastery_dreadblade_crit;

  death_knight_pet_t( death_knight_t* player, std::string_view name, pet_e type = PET_DEATH_KNIGHT, bool guardian = true, bool auto_attack = true,
                      bool dynamic = true )
    : pet_t( player->sim, player, name, type, guardian, dynamic ),
      affected_by(),
      use_auto_attack( auto_attack ),
      precombat_spawn( false ),
      guardian( guardian ),
      precombat_spawn_adjust( 0_s ),
      is_magus( false ),
      commander_value( 0 ),
      grave_mastery( nullptr ),
      mastery_dreadblade_crit( nullptr )
  {
    if ( auto_attack )
    {
      main_hand_weapon.type = WEAPON_BEAST;
    }

    if ( player->talent.unholy.commander_of_the_dead.ok() )
    {
      commander_value = 1.0 + player->pet_spell.commander_of_the_dead->effectN( 1 ).percent();
    }

    affected_by.mastery_dreadblade_crit = player->mastery.dreadblade_pet_crit->ok();
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

  struct mastery_dreadblade_crit_t : public buff_t
  {
    mastery_dreadblade_crit_t( death_knight_pet_t* p )
      : buff_t( p, "mastery_dreadblade", p->dk()->mastery.dreadblade_pet_crit )
    {
      set_quiet( true );
    }

    death_knight_pet_t* pet() const
    {
      return debug_cast<death_knight_pet_t*>( source );
    }

    double value() override
    {
      double v = ( pet()->dk()->mastery.dreadblade->effectN( 6 ).percent() +
                   ( pet()->dk()->mastery.dreadblade->effectN( 6 ).sp_coeff() * pet()->dk()->composite_mastery_value() ) );

      return v;
    }

    double check_value() const override
    {
      double v = ( pet()->dk()->mastery.dreadblade->effectN( 6 ).percent() +
                   ( pet()->dk()->mastery.dreadblade->effectN( 6 ).sp_coeff() * pet()->dk()->composite_mastery_value() ) );

      return v;
    }
  };

  void init_finished() override
  {
    pet_t::init_finished();
    buffs.stunned->set_quiet( true );
    buffs.movement->set_quiet( true );
  }

  void update_stats() override
  {
    if ( owner_coeff.ap_from_ap > 0 )
    {
      current_pet_stats.attack_power_from_ap =
          owner->composite_total_attack_power_by_type( dk()->default_ap_type() ) * owner_coeff.ap_from_ap;
      sim->print_debug( "{} refreshed AP from owner (ap={})", name(), composite_melee_attack_power() );
    }

    current_pet_stats.composite_melee_crit = dk()->cache.attack_crit_chance();
    current_pet_stats.composite_spell_crit = dk()->cache.spell_crit_chance();
    sim->print_debug( "{} refreshed Critical Strike from owner (crit={})", name(),
                      current_pet_stats.composite_melee_crit, dk()->cache.attack_crit_chance() );

    current_pet_stats.composite_melee_haste = owner->cache.attack_haste();
    current_pet_stats.composite_spell_haste = owner->cache.spell_haste();
    sim->print_debug( "{} refreshed Haste from owner (haste={})", name(), current_pet_stats.composite_melee_haste,
                      owner->cache.attack_haste() );

    current_pet_stats.composite_melee_auto_attack_speed = current_pet_stats.composite_melee_haste;
    current_pet_stats.composite_spell_cast_speed        = dk()->cache.spell_cast_speed();
    this->adjust_dynamic_cooldowns();
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

  double composite_player_critical_damage_multiplier( const action_state_t* s, school_e school ) const override
  {
    double m = pet_t::composite_player_critical_damage_multiplier( s, school );

    if ( grave_mastery->check() )
      m *= 1.0 + grave_mastery->check_value();

    if ( mastery_dreadblade_crit->check() )
      m *= 1.0 + mastery_dreadblade_crit->check_value();

    return m;
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

  timespan_t available() const override
  {
    if ( is_moving() )
      return time_to_move();

    if ( buffs.stunned->check() )
      return buffs.stunned->remains();

    if ( primary_resource() == RESOURCE_ENERGY )
    {
      double energy = resources.current[ RESOURCE_ENERGY ];

      if ( energy >= resource_thresholds.front() )
        return pet_t::available();

      timespan_t time_to_next = timespan_t::from_seconds( ( resource_thresholds.front() - energy ) /
                                                          resource_regen_per_second( RESOURCE_ENERGY ) );

      return std::max( time_to_next, pet_t::available() );
    }

    return pet_t::available();
  }

  void arise() override
  {
    pet_t::arise();

    dk()->dk_active_pets.push_back( this );

    if ( dk()->mastery.dreadblade->ok() && affected_by.mastery_dreadblade_crit )
      mastery_dreadblade_crit->trigger();

    if ( dk()->talent.unholy.grave_mastery.ok() && affected_by.grave_mastery )
      grave_mastery->trigger();
  }

  void demise() override
  {
    range::erase_remove( dk()->dk_active_pets, this );
    pet_t::demise();

    // 2026-02-03: Dynamic pets by default dont seem to reset their auto attack on despawn, leading to incorrect
    // schedule_execute() durations for their first auto attack the next time they spawn.
    // Override that behavior here to get the correct behavior where they always execute their "first" auto attack
    // almost immediately after spawning (excluding movement time if applicable).
    if ( dynamic )
    {
      if ( main_hand_attack )
        main_hand_attack->reset();
      if ( off_hand_attack )
        off_hand_attack->reset();
    }
  }

  void trigger_pet_movement( double dist )
  {
    if ( dist == 0 )
      return;

    this->trigger_movement( dist, movement_direction_type::TOWARDS );
    auto dur = this->time_to_move();
    make_event( *sim, dur, [ &, dur ] { update_movement( dur ); } );
  }

  double composite_player_multiplier( school_e s ) const override
  {
    double m = pet_t::composite_player_multiplier( s );

    if ( dk()->specialization() == DEATH_KNIGHT_UNHOLY && affected_by.commander_of_the_dead &&
         dk()->buffs.commander_of_the_dead->check() )
    {
      m *= commander_value;
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
      trigger_gcd           = 0_ms;
      school                = SCHOOL_PHYSICAL;
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

  action_t* create_action( std::string_view name, std::string_view options_str ) override
  {
    if ( name == "auto_attack" )
      return new auto_attack_t( this );

    return pet_t::create_action( name, options_str );
  }

  void create_buffs() override
  {
    pet_t::create_buffs();
    grave_mastery = make_buff( this, "grave_mastery", dk()->pet_spell.grave_mastery_buff )
                        ->set_default_value_from_effect_type( A_MOD_CRIT_DAMAGE_MULTIPLIER )
                        ->set_quiet( true );

    transfusion = make_buff( this, "transfusion", dk()->spell.transfusion_buff )
                      ->set_default_value_from_effect( 1 )
                      ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );

    mastery_dreadblade_crit = make_buff<mastery_dreadblade_crit_t>( this );
  }

  void init_action_list() override
  {
    action_priority_list_t* def = get_action_priority_list( "default" );
    if ( use_auto_attack )
      def->add_action( "auto_attack" );

    pet_t::init_action_list();
  }

public:
  propagate_const<buff_t*> transfusion;
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
        else if ( a->harmful )
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

  debuff.frostbolt_debuff = make_debuff( p.is_magus, *this, "frostbolt_magus", p.dk()->pet_spell.frostbolt );
}

// ==========================================================================
// Base Death Knight Pet Action
// ==========================================================================

template <typename T_PET, typename Base>
struct pet_action_t : public parse_action_effects_t<Base>
{
  using action_base_t = parse_action_effects_t<Base>;
  using base_t        = pet_action_t<T_PET, Base>;

  pet_action_t( T_PET* p, std::string_view name, const spell_data_t* spell = spell_data_t::nil() )
    : action_base_t( name, p, spell )
  {
    this->special = true;

    if ( !this->data().flags( spell_attribute::SX_CANNOT_CRIT ) && this->harmful )
      this->may_crit = true;

    if ( this->data().flags( spell_attribute::SX_TICK_MAY_CRIT ) )
      this->tick_may_crit = true;

    if ( this->data().ok() )
    {
      dk()->apply_action_effects( this, true );
      if ( this->type == action_e::ACTION_SPELL || this->type == action_e::ACTION_ATTACK )
      {
        dk()->apply_target_action_effects( this, true );
      }
    }
  }

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
    return debug_cast<death_knight_t*>( pet()->owner );
  }

  template <typename T>
  target_filter_callback_t dk_dot_or_debuff_only( T d )
  {
    return [ &, d ]( const action_t*, player_t* target ) {
      return dk()->dot_or_debuff_active( d, dk()->get_target_data( target ) );
    };
  }

  void init() override
  {
    action_base_t::init();

    // Merge stats for pets sharing the same name
    if ( !this->player->sim->report_pets_separately )
    {
      auto it =
          range::find_if( dk()->pet_list, [ & ]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != dk()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }
  }

  bool ready() override
  {
    if ( this->player->is_moving() )
      return false;

    return action_base_t::ready();
  }
};

// ==========================================================================
// Base Death Knight Pet Melee Attack
// ==========================================================================

template <typename T_PET>
struct pet_melee_attack_t : public pet_action_t<T_PET, melee_attack_t>
{
  pet_melee_attack_t( T_PET* p, std::string_view name, const spell_data_t* spell = spell_data_t::nil() )
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
  pet_spell_t( T_PET* p, std::string_view name, const spell_data_t* spell = spell_data_t::nil() )
    : pet_action_t<T_PET, spell_t>( p, name, spell )
  {
    this->tick_may_crit = true;
    this->hasted_ticks  = false;
  }
};

// ==========================================================================
// Specialized Death Knight Pet Actions
// ==========================================================================

// Generic auto attack for meleeing pets
template <typename T = death_knight_pet_t>
struct auto_attack_melee_t : public pet_melee_attack_t<T>
{
  bool first;

  auto_attack_melee_t( T* p, std::string_view name = "auto_attack_mh" )
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

    if ( this->first )
      this->first = false;
  }

  void reset() override
  {
    pet_melee_attack_t<T>::reset();

    this->first = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = pet_melee_attack_t<T>::execute_time();

    // Randomize first swing time
    if ( this->first )
    {
      timespan_t delay = ( this->weapon->slot == SLOT_OFF_HAND ) ? pet()->rng().range( 10_ms, t * 0.5 ) : 0_ms;
      return delay;
    }

    return t;
  }

  T* pet() const
  {
    return debug_cast<T*>( this->player );
  }

  death_knight_t* dk() const
  {
    return debug_cast<death_knight_t*>( pet()->owner );
  }
};

// ==========================================================================
// Generic Unholy energy ghoul (main pet and Army/Apoc ghouls)
// ==========================================================================

struct base_ghoul_pet_t : public death_knight_pet_t
{
  double spawn_distance;
  double spawn_radius;

  base_ghoul_pet_t( death_knight_t* owner, std::string_view name, pet_e type, bool guardian = false, bool dynamic = true )
    : death_knight_pet_t( owner, name, type, guardian, true, dynamic ),
      spawn_distance( 0 ),
      spawn_radius( 0 )
  {
    main_hand_weapon.swing_time = 2.0_s;
    main_hand_weapon.type       = WEAPON_BEAST;
    spawn_radius                = dk()->spell.summon_lesser_ghoul->effectN( 1 ).radius();
  }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    ready_type = ready_e::READY_TRIGGER;

    resources.base[ RESOURCE_ENERGY ]                  = 100;
    resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10;
    resources.hasted[ RESOURCE_ENERGY ]                = true;
  }

  struct ghoul_auto_t final : public auto_attack_melee_t<base_ghoul_pet_t>
  {
    ghoul_auto_t( base_ghoul_pet_t* p, std::string_view name ) : auto_attack_melee_t( p, name )
    {
    }

    void impact( action_state_t* state ) override
    {
      auto_attack_melee_t<base_ghoul_pet_t>::impact( state );
      pet()->trigger_infected_claws( state->target );
    }
  };

  attack_t* create_main_hand_auto_attack() override
  {
    return new ghoul_auto_t( this, "auto_attack_mh" );
  }

  void arise() override
  {
    death_knight_pet_t::arise();

    double dist    = precombat_spawn ? 0 : rng().range( -spawn_radius, spawn_radius );
    spawn_distance = std::max( 0.0, dk()->base.distance + dist );
    trigger_pet_movement( spawn_distance );
  }

  void trigger_infected_claws( player_t* target )
  {
    if ( dk()->talent.unholy.infected_claws.ok() && rng().roll( dk()->spell.infected_claws_driver->proc_chance() ) )
      dk()->background_actions.infected_claws->execute_on_target( target );
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_ENERGY;
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
    dt_melee_ability_t( ghoul_pet_t* p, std::string_view name, const spell_data_t* spell = spell_data_t::nil(),
                        bool usable_in_dt = true )
      : pet_melee_attack_t( p, name, spell ),
        usable_in_dt( usable_in_dt ),
        triggers_apocalypse( false )
    {
    }

    void impact( action_state_t* state ) override
    {
      pet_melee_attack_t<ghoul_pet_t>::impact( state );

      pet()->trigger_infected_claws( state->target );

      if ( triggers_apocalypse  )
        dk()->trigger_rune_of_the_apocalypse( state->target );
    }

    bool ready() override
    {
      if ( usable_in_dt != pet()->dark_transformation->up() )
        return false;

      return pet_melee_attack_t<ghoul_pet_t>::ready();
    }

  private:
    bool usable_in_dt;

  public:
    bool triggers_infected_claws;
    bool triggers_apocalypse;
  };

  struct claw_base_t : public dt_melee_ability_t
  {
    claw_base_t( ghoul_pet_t* p, std::string_view name, const spell_data_t* spell = spell_data_t::nil(),
                 bool dt = false )
      : dt_melee_ability_t( p, name, spell, dt )
    {
      triggers_apocalypse = true;
      base_multiplier *= 0.85;
    }
  };

  struct claw_t final : public claw_base_t
  {
    claw_t( ghoul_pet_t* p, std::string_view options_str )
      : claw_base_t( p, "claw", p->dk()->pet_spell.ghoul_claw, false )
    {
      parse_options( options_str );
    }
  };

  struct sweeping_claws_aoe_t final : public claw_base_t
  {
    sweeping_claws_aoe_t( ghoul_pet_t* p )
      : claw_base_t( p, "sweeping_claws_aoe", p->dk()->pet_spell.sweeping_claws, true )
    {
      background              = true;
      aoe                     = -1;
      attack_power_mod.direct = data().effectN( 3 ).ap_coeff();
      target_filter_callback  = secondary_targets_only();
      base_costs[ RESOURCE_ENERGY ] = 0;
    }
  };

  struct sweeping_claws_st_t final : public claw_base_t
  {
    sweeping_claws_st_t( ghoul_pet_t* p, std::string_view options_str )
      : claw_base_t( p, "sweeping_claws", p->dk()->pet_spell.sweeping_claws, true )
    {
      parse_options( options_str );
      aoe                     = 0;
      attack_power_mod.direct = data().effectN( 2 ).ap_coeff();
      impact_action           = new sweeping_claws_aoe_t( p );
    }
  };

  struct gnaw_t final : public dt_melee_ability_t
  {
    gnaw_t( ghoul_pet_t* p, std::string_view options_str )
      : dt_melee_ability_t( p, "gnaw", p->dk()->pet_spell.gnaw, false )
    {
      parse_options( options_str );
      cooldown = p->get_cooldown( "gnaw" );
    }
  };

  struct monstrous_blow_t final : public dt_melee_ability_t
  {
    monstrous_blow_t( ghoul_pet_t* p, std::string_view options_str )
      : dt_melee_ability_t( p, "monstrous_blow", p->dk()->pet_spell.monstrous_blow, true )
    {
      parse_options( options_str );
      cooldown = p->get_cooldown( "gnaw" );
    }
  };

  ghoul_pet_t( death_knight_t* owner, bool guardian = true ) : base_ghoul_pet_t( owner, "ghoul", PET_GHOUL, guardian )
  {
    gnaw_cd                   = get_cooldown( "gnaw" );
    gnaw_cd->duration         = owner->pet_spell.gnaw->cooldown();
    affected_by.grave_mastery = true;
    owner_coeff.ap_from_ap    = 0.474;
    if ( owner->spec.raise_dead->ok() )
    {
      dynamic = false;
    }
  }

  void init_base_stats() override
  {
    base_ghoul_pet_t::init_base_stats();
    if ( dk()->talent.rider.unholy_armaments.ok() )
    {
      owner_coeff.sp_from_ap *= 1.0 + dk()->talent.rider.unholy_armaments->effectN( 4 ).percent();
      owner_coeff.ap_from_ap *= 1.0 + dk()->talent.rider.unholy_armaments->effectN( 3 ).percent();
    }
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = base_ghoul_pet_t::composite_player_multiplier( school );

    if ( dk()->specialization() == DEATH_KNIGHT_UNHOLY )
    {
      if ( dark_transformation->check() )
        m *= 1.0 + dark_transformation->check_value();

      if ( ghoulish_frenzy->check() )
        m *= 1.0 + ghoulish_frenzy->data().effectN( 2 ).percent();
    }

    return m;
  }

  double composite_melee_auto_attack_speed() const override
  {
    double haste = base_ghoul_pet_t::composite_melee_auto_attack_speed();

    if ( ghoulish_frenzy->check() )
      haste /= 1.0 + ghoulish_frenzy->data().effectN( 1 ).percent();

    if ( unholy_devotion->check() )
      haste /= 1.0 + unholy_devotion->check_stack_value();

    return haste;
  }

  double resource_regen_per_second( resource_e r ) const override
  {
    double reg = base_ghoul_pet_t::resource_regen_per_second( r );

    if ( unholy_devotion->check() )
      reg *= 1.0 + unholy_devotion->data().effectN( 3 ).percent() * unholy_devotion->check();

    return reg;
  }

  void init_gains() override
  {
    base_ghoul_pet_t::init_gains();

    dark_transformation_gain = get_gain( "Dark Transformation" );
  }

  void dismiss( bool expired = false ) override
  {
    base_ghoul_pet_t::dismiss( expired );
    if ( dk()->buffs.dark_transformation->check() )
      dk()->buffs.dark_transformation->expire();
  }

  void init_action_list() override
  {
    base_ghoul_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    if ( dk()->talent.unholy.dark_transformation.ok() )
    {
      def->add_action( "gnaw" );
      def->add_action( "monstrous_blow" );
      def->add_action( "sweeping_claws,if=energy>45" );
      def->add_action( "claw,if=energy>70" );
    }
    else
    {
      def->add_action( "gnaw" );
      def->add_action( "claw,if=energy>70" );
    }
  }

  action_t* create_action( std::string_view name, std::string_view options_str ) override
  {
    if ( name == "claw" )
      return new claw_t( this, options_str );
    if ( name == "gnaw" )
      return new gnaw_t( this, options_str );
    if ( name == "sweeping_claws" )
      return new sweeping_claws_st_t( this, options_str );
    if ( name == "monstrous_blow" )
      return new monstrous_blow_t( this, options_str );

    return base_ghoul_pet_t::create_action( name, options_str );
  }

  void create_buffs() override
  {
    base_ghoul_pet_t::create_buffs();

    dark_transformation = make_buff( this, "dark_transformation", dk()->talent.unholy.dark_transformation )
                              ->set_duration( 0_ms )  // Handled by the player buff
                              ->set_cooldown( 0_ms )  // Handled by the action
                              ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_DONE );

    ghoulish_frenzy = make_buff( this, "ghoulish_frenzy", dk()->pet_spell.ghoulish_frenzy )
                          ->set_default_value( dk()->pet_spell.ghoulish_frenzy->effectN( 1 ).percent() )
                          ->set_duration( 0_s )
                          ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                          ->add_invalidate( CACHE_AUTO_ATTACK_SPEED );

    unholy_devotion = make_buff( this, "unholy_devotion", dk()->pet_spell.unholy_devotion_buff )
                          ->set_default_value_from_effect_type( A_MOD_ATTACKSPEED_NORMALIZED )
                          ->set_disable_async_expire_events_removal( true );
  }

private:
  cooldown_t* gnaw_cd;  // shared cd between gnaw/monstrous_blow

public:
  propagate_const<gain_t*> dark_transformation_gain;
  propagate_const<buff_t*> dark_transformation;
  propagate_const<buff_t*> ghoulish_frenzy;
  propagate_const<buff_t*> unholy_devotion;
};

// ==========================================================================
// Army of the Dead and Apocalypse ghouls
// ==========================================================================

struct lesser_ghoul_pet_t final : public base_ghoul_pet_t
{
  struct lesser_ghoul_claw_base_t : public pet_melee_attack_t<lesser_ghoul_pet_t>
  {
    lesser_ghoul_claw_base_t( lesser_ghoul_pet_t* p, std::string_view name, const spell_data_t* spell )
      : pet_melee_attack_t( p, name, spell )
    {
      // Data has these as 0 cost. They cost 40 in game. 
      base_costs[ RESOURCE_ENERGY ] = 40;
      // Data has no gcd on these. They have a 1.5s gcd in game.
      trigger_gcd                   = 1.5_s;
    }

    void impact( action_state_t* s ) override
    {
      pet_melee_attack_t<lesser_ghoul_pet_t>::impact( s );
      pet()->trigger_infected_claws( s->target );

      dk()->trigger_rune_of_the_apocalypse( s->target );
    }

    bool ready() override
    {
      if ( pet()->putrefied )
        return false;

      return pet_melee_attack_t<lesser_ghoul_pet_t>::ready();
    }
  };

  struct lesser_ghoul_sweeping_claws_aoe_t : public lesser_ghoul_claw_base_t
  {
    lesser_ghoul_sweeping_claws_aoe_t( lesser_ghoul_pet_t* p )
      : lesser_ghoul_claw_base_t( p, "sweeping_claws_aoe", p->dk()->pet_spell.lesser_sweeping_claws )
    {
      background              = true;
      aoe                     = -1;
      attack_power_mod.direct = data().effectN( 3 ).ap_coeff();
      target_filter_callback  = secondary_targets_only();
      base_costs[ RESOURCE_ENERGY ] = 0;
      trigger_gcd                   = 0_ms;
    }
  };

  struct lesser_ghoul_sweeping_claws_main_t : public lesser_ghoul_claw_base_t
  {
    lesser_ghoul_sweeping_claws_main_t( lesser_ghoul_pet_t* p, std::string_view options_str )
      : lesser_ghoul_claw_base_t( p, "sweeping_claws", p->dk()->pet_spell.lesser_sweeping_claws )
    {
      parse_options( options_str );
      aoe                           = 0;
      attack_power_mod.direct       = data().effectN( 2 ).ap_coeff();
      impact_action                 = new lesser_ghoul_sweeping_claws_aoe_t( p );
    }

    bool ready() override
    {
      if ( !dk()->buffs.dark_transformation->check() && dk()->talent.unholy.outnumber.ok() )
        return false;

      return lesser_ghoul_claw_base_t::ready();
    }
  };

  struct lesser_ghoul_claw_t : public lesser_ghoul_claw_base_t
  {
    lesser_ghoul_claw_t( lesser_ghoul_pet_t* p, std::string_view options_str )
      : lesser_ghoul_claw_base_t( p, "claw", p->dk()->pet_spell.army_claw )
    {
      parse_options( options_str );
    }

    bool ready() override
    {
      if ( dk()->buffs.dark_transformation->check() && dk()->talent.unholy.outnumber.ok() )
        return false;

      return lesser_ghoul_claw_base_t::ready();
    }
  };

  struct ruptured_viscera_t : public pet_spell_t<lesser_ghoul_pet_t>
  {
    ruptured_viscera_t( std::string_view n, lesser_ghoul_pet_t* p )
      : pet_spell_t( p, n, p->dk()->pet_spell.ruptured_viscera )
    {
      background = true;
      aoe        = -1;
    }

    void impact( action_state_t* s ) override
    {
      pet_spell_t<lesser_ghoul_pet_t>::impact( s );

      dk()->trigger_rune_of_the_apocalypse( s->target );
    }
  };

  lesser_ghoul_pet_t( death_knight_t* owner, std::string_view name = "army_ghoul" )
    : base_ghoul_pet_t( owner, name, PET_LESSER_GHOUL, true ),
      ruptured_viscera( nullptr ),
      putrefied( false ),
      base_ap_from_ap( 0.192975 )
  {
    affected_by.commander_of_the_dead = true;
    affected_by.grave_mastery         = true;

    if ( dk()->talent.rider.unholy_armaments.ok() )
    {
      owner_coeff.sp_from_ap *= 1.0 + dk()->talent.rider.unholy_armaments->effectN( 6 ).percent();
      base_ap_from_ap *= 1.0 + dk()->talent.rider.unholy_armaments->effectN( 5 ).percent();
    }

    if ( name_str == "army_ghoul" )
      base_ap_from_ap *= 1.75;
  }

  void init_base_stats() override
  {
    base_ghoul_pet_t::init_base_stats();
    owner_coeff.ap_from_ap = base_ap_from_ap;
  }

  void init_gains() override
  {
    base_ghoul_pet_t::init_gains();

    // Merge gains for all lesser ghouls to cleanup reporting
    if( !dk()->gains.lesser_ghoul_energy )
      dk()->gains.lesser_ghoul_energy = gains.resource_regen[ RESOURCE_ENERGY ];
    else
      gains.resource_regen[ RESOURCE_ENERGY ] = dk()->gains.lesser_ghoul_energy;
  }

  void init_uptimes() override
  {
    base_ghoul_pet_t::init_uptimes();

    // Merge Uptimes for all lesser ghouls to cleanup reporting.
    if ( !dk()->lesser_ghoul_uptimes )
      dk()->lesser_ghoul_uptimes = uptimes.primary_resource_cap;
    else
      uptimes.primary_resource_cap = dk()->lesser_ghoul_uptimes;
  }

  void arise() override
  {
    if ( dk()->talent.unholy.commander_of_the_dead.ok() && dk()->options.extra_unholy_reporting )
      dk()->sample_data.lesser_ghoul_duration->add( duration.total_seconds() );

    dk()->lesser_ghouls_summoned++;

    base_ghoul_pet_t::arise();

    dk()->active_lesser_ghouls.push_back( this );
    dk()->buffs.lesser_ghoul_counter->trigger();
    if ( dk()->options.extra_unholy_reporting )
      dk()->sample_data.lesser_ghouls_active->add( as<unsigned>( dk()->active_lesser_ghouls.size() ) );
  }

  void demise() override
  {
    range::erase_remove( dk()->active_lesser_ghouls, this );

    dk()->buffs.lesser_ghoul_counter->decrement();

    base_ghoul_pet_t::demise();

    if ( !sim->event_mgr.canceled && dk()->options.extra_unholy_reporting )
      dk()->sample_data.lesser_ghouls_active->add( as<unsigned>( dk()->active_lesser_ghouls.size() ) );
  }

  void dismiss( bool expired = false ) override
  {
    // These expire before ruptured viscera triggers
    mastery_dreadblade_crit->expire();
    grave_mastery->expire();

    if ( dk()->talent.unholy.necromancers_cunning.ok() && expired && !sim->event_mgr.canceled )
      ruptured_viscera->execute();

    putrefied = false;

    base_ghoul_pet_t::dismiss( expired );
  }

  void putrefy_ghoul( putrefy_source_e source )
  {
    putrefied = true;
    timespan_t travel_time = timespan_t::from_seconds( spawn_distance / dk()->pet_spell.leap->missile_speed() );
    make_event( *sim, travel_time + rng().range( 0_ms, 100_ms ), [ &, source ]() {
      // RNG roll technically not needed as its a 100% chance, but, leaving this here in case it changes in the future
      bool reanimation_triggered          = rng().roll( dk()->talent.unholy.reanimation->effectN( 1 ).percent() );
      action_t* magus_summon_action       = dk()->pet_summon.reanimation_magus;
      timespan_t unholy_devotion_duration = dk()->pet_spell.unholy_devotion_buff->duration();
      timespan_t cycle_of_death_cdr       = -dk()->talent.unholy.cycle_of_death->effectN( 1 ).time_value();

      switch ( source )
      {
        case PUTREFY_SOURCE_FORBIDDEN_KNOWLEDGE:
          magus_summon_action = dk()->pet_summon.fk_reanimation_magus;
          unholy_devotion_duration *= dk()->talent.unholy.forbidden_knowledge_3->effectN( 2 ).percent();
          cycle_of_death_cdr *= dk()->talent.unholy.forbidden_knowledge_3->effectN( 2 ).percent();
          dk()->background_actions.putrefy_fk_st->execute_on_target( dk()->target );
          break;
        default:
          dk()->background_actions.putrefy_st->execute_on_target( dk()->target );
          break;
      }

      dismiss( false );

      if ( dk()->talent.unholy.reanimation.ok() && reanimation_triggered && magus_summon_action )
        magus_summon_action->execute();

      if ( dk()->talent.unholy.unholy_devotion.ok() && dk()->pets.ghoul_pet.active_pet() )
        dk()->pets.ghoul_pet.active_pet()->unholy_devotion->trigger( unholy_devotion_duration );

      if ( dk()->sets->has_set_bonus( DEATH_KNIGHT_UNHOLY, MID1, B4 ) )
        dk()->buffs.blighted->trigger();

      if ( dk()->talent.unholy.forbidden_knowledge_2.ok() )
      {
        dk()->buffs.lesser_ghoul_ready->trigger(
            as<int>( dk()->talent.unholy.forbidden_knowledge_2->effectN( 2 ).base_value() ) );
        dk()->buffs.forbidden_sacrifice->trigger();
      }

      if ( dk()->talent.unholy.cycle_of_death.ok() )
        dk()->cooldown.death_and_decay_dynamic->adjust( cycle_of_death_cdr );
    } );
  }

  void set_ap_multiplier( double multiplier )
  {
    owner_coeff.ap_from_ap = base_ap_from_ap * multiplier;
  }

  void init_action_list() override
  {
    base_ghoul_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    if ( dk()->talent.unholy.outnumber.ok() )
      def->add_action( "sweeping_claws" );
    def->add_action( "claw" );
  }

  void init_finished() override
  {
    base_ghoul_pet_t::init_finished();
    transfusion->set_quiet( true );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = base_ghoul_pet_t::composite_player_multiplier( school );

    if ( transfusion->check() )
      m *= 1.0 + transfusion->check_stack_value();

    return m;
  }

  void create_actions() override
  {
    base_ghoul_pet_t::create_actions();
    ruptured_viscera = get_action<ruptured_viscera_t>( "ruptured_viscera", this );
  }

  action_t* create_action( std::string_view name, std::string_view options_str ) override
  {
    if ( name == "claw" )
      return new lesser_ghoul_claw_t( this, options_str );

    if ( name == "sweeping_claws" )
      return new lesser_ghoul_sweeping_claws_main_t( this, options_str );

    return base_ghoul_pet_t::create_action( name, options_str );
  }

private:
  action_t* ruptured_viscera;
  bool putrefied;

public:
  double base_ap_from_ap;
};

// ==========================================================================
// Gargoyle
// ==========================================================================
struct gargoyle_pet_t : public death_knight_pet_t
{
  gargoyle_pet_t( death_knight_t* owner )
    : death_knight_pet_t( owner, "gargoyle", PET_GARGOYOLE, true, false ), gargoyle_strike( nullptr ), dark_empowerment( nullptr )
  {
    resource_regeneration             = regen_type::DISABLED;
    affected_by.commander_of_the_dead = true;
    affected_by.grave_mastery         = true;
  }

  struct gargoyle_strike_t : public pet_spell_t<gargoyle_pet_t>
  {
    gargoyle_strike_t( std::string_view n, gargoyle_pet_t* p ) : pet_spell_t( p, n, p->dk()->pet_spell.gargoyle_strike )
    {
      background = repeating = true;
    }

    double composite_crit_chance() const override
    {
      // Always Crits
      return 1.0;
    }
  };

  resource_e primary_resource() const override
  {
    return RESOURCE_NONE;
  }

  void arise() override
  {
    death_knight_pet_t::arise();
    timespan_t duration = 2.8_s;
    buffs.stunned->trigger( duration + rng().gauss<200, 25>() );
    stun();
    reschedule_gargoyle();
  }

  void init_finished() override
  {
    death_knight_pet_t::init_finished();
    buffs.stunned->set_expire_callback( [ this ]( buff_t*, int, timespan_t ) { reschedule_gargoyle(); } );
  }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 1.0;
  }

  double composite_player_multiplier( school_e s ) const override
  {
    double m = death_knight_pet_t::composite_player_multiplier( s );

    m *= 1.0 + dark_empowerment->check_stack_value();

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
    gargoyle_strike = get_action<gargoyle_strike_t>( "gargoyle_strike", this );
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
                      dk()->spell.summon_gargoyle->effectN( 4 ).base_value();

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
  propagate_const<action_t*> gargoyle_strike;

public:
  propagate_const<buff_t*> dark_empowerment;
};

// ==========================================================================
// Risen Skulker (All Will Serve talent)
// ==========================================================================

struct risen_skulker_pet_t : public death_knight_pet_t
{
  risen_skulker_pet_t( death_knight_t* owner ) : death_knight_pet_t( owner, "risen_skulker", PET_RISEN_SKULKER, true, false, false )
  {
    resource_regeneration       = regen_type::DISABLED;
    main_hand_weapon.type       = WEAPON_BEAST_RANGED;
    main_hand_weapon.swing_time = 3_s;

    affected_by.grave_mastery         = true;
    affected_by.commander_of_the_dead = true;

    // Using a background repeating action as a replacement for a foreground action. Change Ready Type to trigger so we
    // can wake up the pet when it needs to re-execute this action.
    ready_type = READY_TRIGGER;
  }

  struct blighted_arrow_t : public pet_spell_t<risen_skulker_pet_t>
  {
    blighted_arrow_t( std::string_view n, risen_skulker_pet_t* p )
      : pet_spell_t( p, n, p->dk()->pet_spell.blighted_arrow ), was_instant( false )
    {
      background = true;
      repeating  = true;
    }

    int n_targets() const override
    {
      if ( pet()->blighted_arrow_aoe_buff->check() )
        return as<int>( pet()->blighted_arrow_aoe_buff->data().effectN( 1 ).base_value() );
      return 0;
    }

    void schedule_execute( action_state_t* state ) override
    {
      pet_spell_t<risen_skulker_pet_t>::schedule_execute( state );
      // This pet uses a background repeating event with a ready type of READY_TRIGGER. Without constantly re-updating
      // the started waiting trigger_ready would never function.
      player->started_waiting = sim->current_time();
    }

    void execute() override
    {
      was_instant = false;

      pet_spell_t::execute();

      pet()->blighted_arrow_aoe_buff->consume( this, 1 );

      if ( pet()->blighted_arrow_st_buff->check() )
      {
        was_instant = true;
        pet()->blighted_arrow_st_buff->consume( this, 1 );
      }
    }

    double execute_time_pct_multiplier() const override
    {
      double m = pet_spell_t::execute_time_pct_multiplier();

      if ( pet()->blighted_arrow_st_buff->check() )
        m *= 0;

      return m;
    }

    double composite_da_multiplier( const action_state_t* state ) const override
    {
      double m = pet_spell_t::composite_da_multiplier( state );

      if ( was_instant )
        m *= dk()->talent.unholy.all_will_serve->effectN( 1 ).percent();

      return m;
    }

  private:
    bool was_instant;
  };

  void acquire_target( retarget_source event, player_t* context ) override
  {
    if ( blighted_arrow->execute_event && blighted_arrow->target->is_sleeping() )
    {
      event_t::cancel( blighted_arrow->execute_event );
      started_waiting = sim->current_time();
    }

    player_t::acquire_target( event, context );

    if ( !blighted_arrow->execute_event )
      trigger_ready();
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_NONE;
  }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 0.75;

    if ( dk()->talent.rider.unholy_armaments.ok() )
    {
      owner_coeff.sp_from_ap *= 1.0 + dk()->talent.rider.unholy_armaments->effectN( 8 ).percent();
      owner_coeff.ap_from_ap *= 1.0 + dk()->talent.rider.unholy_armaments->effectN( 7 ).percent();
    }
  }

  void create_buffs() override
  {
    death_knight_pet_t::create_buffs();

    blighted_arrow_aoe_buff = make_buff( this, "blighted_arrow_aoe", dk()->pet_spell.blighted_arrow_aoe_buff )
                                  ->set_consume_all_stacks( false );
    blighted_arrow_st_buff =
        make_buff( this, "blighted_arrow_st", dk()->pet_spell.blighted_arrow_st_buff )->set_consume_all_stacks( false );
  }

  void create_actions() override
  {
    death_knight_pet_t::create_actions();
    blighted_arrow = new blighted_arrow_t( "blighted_arrow", this );
  }

  void reschedule_skulker()
  {
    // Have to check the presecnce of a blighted_arrow->execute_event because this acts as our "executing" due to using
    // a background repeating action. We do not wish to have multiple of these.
    if ( executing || blighted_arrow->execute_event || buffs.movement->check() || buffs.stunned->check() )
      return;

    blighted_arrow->set_target( dk()->target );
    blighted_arrow->schedule_execute( nullptr );
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

public:
  blighted_arrow_t* blighted_arrow;
  propagate_const<buff_t*> blighted_arrow_aoe_buff;
  propagate_const<buff_t*> blighted_arrow_st_buff;
};

// ==========================================================================
// Dancing Rune Weapon
// ==========================================================================

struct dancing_rune_weapon_pet_t : public death_knight_pet_t
{
  template <typename T_ACTION>
  struct drw_action_t : public pet_action_t<dancing_rune_weapon_pet_t, T_ACTION>
  {
    drw_action_t( dancing_rune_weapon_pet_t* p, std::string_view name, const spell_data_t* s )
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
    blood_plague_t( std::string_view n, dancing_rune_weapon_pet_t* p )
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
    blood_boil_t( std::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t<spell_t>( p, n, p->dk()->talent.blood.blood_boil )
    {
      aoe                 = -1;
      cooldown->duration  = 0_ms;
      this->impact_action = p->ability.blood_plague;
    }
  };

  struct deaths_caress_t : public drw_action_t<spell_t>
  {
    int stack_gain;
    deaths_caress_t( std::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t( p, n, p->dk()->spec.deaths_caress ), stack_gain( as<int>( data().effectN( 3 ).base_value() ) )

    {
      this->impact_action = p->ability.blood_plague;
    }

    void impact( action_state_t* state ) override
    {
      drw_action_t::impact( state );

      dk()->buffs.bone_shield->trigger( stack_gain );
    }
  };

  struct death_strike_t : public drw_action_t<melee_attack_t>
  {
    death_strike_t( std::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t<melee_attack_t>( p, n, p->dk()->talent.death_strike )
    {
      // In simc, chain multiplier will reduce the damage per target hit, however in game
      // this affect seems to be a constant reduction across all secondary targets
      chain_multiplier = 1.0;
      base_aoe_multiplier = p->dk()->talent.death_strike->effectN( 1 ).chain_multiplier();
    }
  };

  struct heart_strike_t : public drw_action_t<melee_attack_t>
  {
    heart_strike_t( std::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t<melee_attack_t>( p, n, p->dk()->pet_spell.drw_heart_strike )
    {
    }

    int n_targets() const override
    {
      return dk()->buffs.death_and_decay->up() ? aoe + as<int>( dk()->spell.dnd_buff->effectN( 3 ).base_value() ) : aoe;
    }
  };

  struct vampiric_strike_t : public drw_action_t<melee_attack_t>
  {
    vampiric_strike_t( std::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t<melee_attack_t>( p, n, p->dk()->spell.vampiric_strike )
    {
      attack_power_mod.direct = data().effectN( 5 ).ap_coeff();
      aoe                     = 1;
    }
  };

  struct marrowrend_t : public drw_action_t<melee_attack_t>
  {
    int stack_gain;
    marrowrend_t( std::string_view n, dancing_rune_weapon_pet_t* p )
      : drw_action_t<melee_attack_t>( p, n, p->dk()->talent.blood.marrowrend ),
        stack_gain( as<int>( data().effectN( 3 ).base_value() ) )
    {
    }

    void impact( action_state_t* state ) override
    {
      drw_action_t::impact( state );

      dk()->buffs.bone_shield->trigger( stack_gain );
    }
  };

  struct drw_auto_attack_t : public auto_attack_melee_t<dancing_rune_weapon_pet_t>
  {
    drw_auto_attack_t( dancing_rune_weapon_pet_t* p ) : auto_attack_melee_t( p )
    {
      weapon            = &p->main_hand_weapon;
      weapon->slot      = SLOT_MAIN_HAND;
      base_execute_time = p->main_hand_weapon.swing_time;
    }
  };

  action_t* drw_auto_attack;

  struct abilities_t
  {
    action_t* blood_plague;
    action_t* blood_boil;
    action_t* deaths_caress;
    action_t* death_strike;
    action_t* heart_strike;
    action_t* marrowrend;
    action_t* vampiric_strike;
  } ability;

  dancing_rune_weapon_pet_t( death_knight_t* owner, std::string_view drw_name = "dancing_rune_weapon" )
    : death_knight_pet_t( owner, drw_name, PET_DANCING_RUNE_WEAPON, true, false ), ability()
  {
    // The pet wields the same weapon type as its owner for spells with weapon requirements
    main_hand_weapon.type       = owner->main_hand_weapon.type;
    main_hand_weapon.swing_time = 3.5_s;

    owner_coeff.ap_from_ap = 1 / 3.0;
    resource_regeneration  = regen_type::DISABLED;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = death_knight_pet_t::composite_player_multiplier( school );

    if ( transfusion->check() )
      m *= 1.0 + transfusion->check_stack_value();

    return m;
  }

  void create_actions() override
  {
    death_knight_pet_t::create_actions();

    drw_auto_attack = new drw_auto_attack_t( this );

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
    if ( dk()->talent.sanlayn.vampiric_strike.ok() )
    {
      ability.vampiric_strike = get_action<vampiric_strike_t>( "vampiric_strike", this );
    }
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_NONE;
  }

  void arise() override
  {
    death_knight_pet_t::arise();
    reschedule_drw();
    // We will let DRW and DOM drive the DRW buff
    if ( dk()->buffs.dancing_rune_weapon->remains() < duration)
        dk()->buffs.dancing_rune_weapon->trigger( duration );

    dk()->buffs.dance_of_midnight_2->trigger( duration );
  }

  void reschedule_drw()
  {
    if ( executing || is_sleeping() || buffs.movement->check() || buffs.stunned->check() )
      return;

    else
    {
      drw_auto_attack->set_target( dk()->target );
      drw_auto_attack->schedule_execute();
    }
  }

  void schedule_ready( timespan_t /* delta_time */, bool /* waiting */ ) override
  {
    reschedule_drw();
  }
};

// ==========================================================================
// Bloodworms
// ==========================================================================

struct bloodworm_pet_t : public death_knight_pet_t
{
  bloodworm_pet_t( death_knight_t* owner ) : death_knight_pet_t( owner, "bloodworm", PET_BLOODWORMS, true, true )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = 1.4_s;

    owner_coeff.ap_from_ap = 0.25;
    resource_regeneration  = regen_type::DISABLED;
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_NONE;
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
    magus_spell_t( magus_pet_t* p, std::string_view name, const spell_data_t* spell, std::string_view options_str )
      : pet_spell_t( p, name, spell )
    {
      parse_options( options_str );
      // There's a 1 energy cost in spelldata but it might as well be ignored
      base_costs[ RESOURCE_ENERGY ] = 0;
    }
  };

  struct frostbolt_magus_t final : public magus_spell_t
  {
    frostbolt_magus_t( magus_pet_t* p, std::string_view options_str )
      : magus_spell_t( p, "frostbolt", p->dk()->pet_spell.frostbolt, options_str )
    {
      // If the target is immune to slows, frostbolt seems to be used at most every 6 seconds
      cooldown->duration = dk()->pet_spell.frostbolt->duration();
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
    shadow_bolt_magus_t( magus_pet_t* p, std::string_view options_str )
      : magus_spell_t( p, "shadow_bolt", p->dk()->pet_spell.shadow_bolt, options_str )
    {
    }
  };

  magus_pet_t( death_knight_t* owner, std::string_view name = "dt_magus" )
    : death_knight_pet_t( owner, name, PET_MAGUS_OF_THE_DEAD, true, false )
  {
    resource_regeneration               = regen_type::DISABLED;
    affected_by.commander_of_the_dead   = true;
    affected_by.grave_mastery           = true;
    affected_by.mastery_dreadblade_crit = true;
    is_magus                            = true;
    npc_id                              = 163366;
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_NONE;
  }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();
    owner_coeff.ap_from_ap = 0.4664;
  }

  void arise() override
  {
    death_knight_pet_t::arise();
    trigger_pet_movement( 7 );
    dk()->buffs.unholy_aura_haste->trigger();
    if ( dk()->talent.unholy.forbidden_knowledge_3.ok() )
      dk()->buffs.forbidden_ritual->trigger();

    dk()->magus_active++;
    if ( dk()->options.extra_unholy_reporting )
      dk()->sample_data.magus_active->add( dk()->magus_active );
  }

  void demise() override
  {
    death_knight_pet_t::demise();
    dk()->buffs.unholy_aura_haste->decrement();
    if ( dk()->talent.unholy.forbidden_knowledge_3.ok() )
      dk()->buffs.forbidden_ritual->decrement();

    if ( !dk()->sim->event_mgr.canceled )
    {
      dk()->magus_active--;
      if ( dk()->options.extra_unholy_reporting )
        dk()->sample_data.magus_active->add( dk()->magus_active );
    }
  }

  void init_action_list() override
  {
    death_knight_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "frostbolt" );
    def->add_action( "shadow_bolt" );
  }

  action_t* create_action( std::string_view name, std::string_view options_str ) override
  {
    if ( name == "frostbolt" )
      return new frostbolt_magus_t( this, options_str );
    if ( name == "shadow_bolt" )
      return new shadow_bolt_magus_t( this, options_str );

    return death_knight_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Blood Beast
// ==========================================================================
struct blood_beast_pet_t : public death_knight_pet_t
{
  struct blood_beast_melee_t : public auto_attack_melee_t<blood_beast_pet_t>
  {
    blood_beast_melee_t( blood_beast_pet_t* p ) : auto_attack_melee_t<blood_beast_pet_t>( p )
    {
    }

    void impact( action_state_t* state ) override
    {
      auto_attack_melee_t<blood_beast_pet_t>::impact( state );
      if ( state->result_amount > 0 )
      {
        pet()->accumulator += state->result_amount;
      }
    }
  };

  struct corrupted_blood_t : public pet_melee_attack_t<blood_beast_pet_t>
  {
    corrupted_blood_t( blood_beast_pet_t* p, std::string_view options_str )
      : pet_melee_attack_t<blood_beast_pet_t>( p, "corrupted_blood", p->dk()->pet_spell.corrupted_blood )
    {
      parse_options( options_str );
      aoe = -1;
    }

    void impact( action_state_t* state ) override
    {
      pet_melee_attack_t<blood_beast_pet_t>::impact( state );
      if ( state->result_amount > 0 )
      {
        pet()->accumulator += state->result_amount;
      }
    }
  };

  blood_beast_pet_t( death_knight_t* owner ) : death_knight_pet_t( owner, "blood_beast", PET_BLOOD_BEAST, true, true ), accumulator( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = 1_s;
    npc_id                      = owner->find_spell( 434237 )->effectN( 1 ).misc_value1();
    owner_coeff.ap_from_ap      = owner->specialization() == DEATH_KNIGHT_UNHOLY ? 0.2325 : 0.2325;
    resource_regeneration       = regen_type::DISABLED;
    blood_beast_mod             = dk()->specialization() == DEATH_KNIGHT_BLOOD
                                      ? dk()->talent.sanlayn.the_blood_is_life->effectN( 1 ).percent()
                                      : dk()->talent.sanlayn.the_blood_is_life->effectN( 2 ).percent();

    affected_by.grave_mastery = true;
  }

  void demise() override
  {
    if ( !sim->event_mgr.canceled )
    {
      dk()->background_actions.the_blood_is_life->execute_on_target( dk()->target, accumulator * blood_beast_mod );
      accumulator = 0;
    }

    death_knight_pet_t::demise();
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_NONE;
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

  void init_action_list() override
  {
    death_knight_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "corrupted_blood" );
  }

  action_t* create_action( std::string_view name, std::string_view options_str ) override
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
    horseman_spell_t( horseman_pet_t* p, std::string_view name, const spell_data_t* spell )
      : pet_spell_t( p, name, spell )
    {
    }
  };

  struct horseman_melee_t : public pet_melee_attack_t<horseman_pet_t>
  {
    horseman_melee_t( horseman_pet_t* p, std::string_view name, const spell_data_t* spell )
      : pet_melee_attack_t( p, name, spell )
    {
    }
  };

  struct horseman_ams_t : public horseman_spell_t
  {
    horseman_ams_t( std::string_view name, horseman_pet_t* p, std::string_view options_str )
      : horseman_spell_t( p, name, p->dk()->pet_spell.rider_ams )
    {
      parse_options( options_str );
      trigger_gcd = 1_s;
      harmful     = false;
    }

    void execute() override
    {
      horseman_spell_t::execute();
      if ( !dk()->buffs.antimagic_shell_horsemen_icd->check() && dk()->talent.rider.horsemens_aid.ok() )
      {
        set_target( dk() );
        dk()->buffs.antimagic_shell_horsemen->trigger();
        dk()->buffs.antimagic_shell_horsemen_icd->trigger();
      }
      else
        set_target( pet() );
    }
  };

  horseman_pet_t( death_knight_t* owner, std::string_view name, pet_e type, bool guardian = true, bool dynamic = false )
    : death_knight_pet_t( owner, name, type, guardian, true, dynamic ), rp_spent( 0 ), current_pool( 0 )
  {
    main_hand_weapon.type     = WEAPON_BEAST_2H;
    owner_coeff.ap_from_ap    = 0.70125;
    resource_regeneration     = regen_type::DISABLED;
    auto_attack_multiplier    = 0.75;
    affected_by.grave_mastery = true;
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
    if ( !dk()->sim->event_mgr.canceled )
      dk()->active_riders--;
  }

  void reset() override
  {
    death_knight_pet_t::reset();
    rp_spent            = 0;
    current_pool        = 0;
    dk()->active_riders = 0;
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_NONE;
  }

  void init_action_list() override
  {
    death_knight_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    // Limit them casting AMS to only if this is talented and may impact player dps
    def->add_action( "antimagic_shell_horsemen" );
  }

  action_t* create_action( std::string_view name, std::string_view options_str ) override
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
      return new auto_attack_melee_t<horseman_pet_t>( this, "auto_attack_oh" );
    else
      return nullptr;
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
    dnd_damage_mograine_t( std::string_view name, horseman_pet_t* p )
      : horseman_spell_t( p, name, p->dk()->pet_spell.mograines_death_and_decay )
    {
      background = true;
      aoe        = -1;
    }

    void impact( action_state_t* s ) override
    {
      horseman_spell_t::impact( s );
      death_knight_td_t* td = dk()->get_target_data( s->target );
      timespan_t dur        = dk()->talent.rider.riders_champion->effectN( 2 ).time_value();
      std::vector<dot_t*> dots;
      switch ( dk()->specialization() )
      {
        case DEATH_KNIGHT_UNHOLY:
          if ( td->dot.virulent_plague->is_ticking() )
            dots.push_back( td->dot.virulent_plague );
          if ( td->dot.dread_plague->is_ticking() )
            dots.push_back( td->dot.dread_plague );
          break;
        case DEATH_KNIGHT_FROST:
          if ( td->dot.frost_fever->is_ticking() )
            dots.push_back( td->dot.frost_fever );
          break;
        default:
          break;
      }

      if ( !dots.empty() )
        for ( auto& dot : dots )
          dot->adjust_duration( dur );

      if ( dk()->talent.unholy.cycle_of_death.ok() && s->chain_target < 10 )
        dk()->cooldown.putrefy->adjust( -dk()->talent.unholy.cycle_of_death->effectN( 2 ).time_value() );
    }
  };

  struct dnd_aura_t final : public buff_t
  {
    dnd_aura_t( horseman_pet_t* p )
      : buff_t( p, "death_and_decay", p->dk()->pet_spell.mograines_death_and_decay_aura ),
        dk( p->dk() ),
        dnd_damage( get_action<dnd_damage_mograine_t>( "death_and_decay", p ) )
    {
      set_tick_zero( true );
      set_tick_callback( [ this ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ ) {
        dnd_damage->execute();
      } );
    }

    void start( int stacks, double value, timespan_t duration ) override
    {
      buff_t::start( stacks, value, duration );
      if ( dk->talent.rider.mograines_might.ok() )
      {
        timespan_t dur = mograine()->dnd_duration();
        dk->buffs.mograines_might->trigger( dur );
        dk->buffs.death_and_decay->trigger( dur + 4_s );
      }
    }

    mograine_pet_t* mograine() const
    {
      return debug_cast<mograine_pet_t*>( player );
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      buff_t::expire_override( expiration_stacks, remaining_duration );
      if ( mograine()->extended_by_apoc_now )
      {
        mograine()->extended_by_apoc_now = false;
        // Triggers again 100_ms after the buff expires
        make_event( *sim, 100_ms, [ & ]() { trigger(); } );
      }
    }

  private:
    death_knight_t* dk;
    propagate_const<action_t*> dnd_damage;
  };

  struct heart_strike_mograine_t final : public horseman_melee_t
  {
    heart_strike_mograine_t( std::string_view name, horseman_pet_t* p, std::string_view options_str )
      : horseman_melee_t( p, name, p->dk()->pet_spell.mograine_heart_strike )
    {
      parse_options( options_str );
    }
  };

  void create_buffs() override
  {
    death_knight_pet_t::create_buffs();
    dnd_aura = make_buff<dnd_aura_t>( this );
  }

  mograine_pet_t( death_knight_t* owner ) : horseman_pet_t( owner, "mograine", PET_MOGRAINE )
  {
    npc_id                      = owner->spell.summon_mograine->effectN( 1 ).misc_value1();
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = 2_s;
    off_hand_weapon.type        = WEAPON_BEAST;
    off_hand_weapon.swing_time  = 2_s;
  }

  timespan_t dnd_duration() const
  {
    timespan_t avg_dur =
        dk()->pet_spell.mograines_death_and_decay_aura->duration() * dk()->options.average_mograines_might_uptime;
    timespan_t dur = rng().gauss_b( avg_dur, 2_s, dk()->pet_spell.mograines_death_and_decay_aura->duration() );
    return dur;
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

  action_t* create_action( std::string_view name, std::string_view options_str ) override
  {
    if ( name == "heart_strike" )
      return new heart_strike_mograine_t( "heart_strike", this, options_str );

    return horseman_pet_t::create_action( name, options_str );
  }

public:
  propagate_const<buff_t*> dnd_aura;
  bool extended_by_apoc_now = false;
};

// ==========================================================================
// Whitemane Pet
// ==========================================================================
struct whitemane_pet_t final : public horseman_pet_t
{
  struct death_coil_whitemane_t final : public horseman_spell_t
  {
    death_coil_whitemane_t( std::string_view name, horseman_pet_t* p, std::string_view options_str )
      : horseman_spell_t( p, name, p->dk()->pet_spell.whitemane_death_coil )
    {
      parse_options( options_str );
    }
  };

  struct death_coil_whitemane_background_t final : public horseman_spell_t
  {
    death_coil_whitemane_background_t( std::string_view name, horseman_pet_t* p )
      : horseman_spell_t( p, name, p->dk()->pet_spell.whitemane_death_coil )
    {
      background         = true;
      base_multiplier    = dk()->talent.rider.let_terror_reign->effectN( 2 ).percent();
      cooldown->duration = 0_ms;  // Ignore the cooldown for the background casts
    }
  };

  struct epidemic_aoe_whitemane_t final : public horseman_spell_t
  {
    epidemic_aoe_whitemane_t( std::string_view name, horseman_pet_t* p )
      : horseman_spell_t( p, name, p->dk()->pet_spell.whitemane_epidemic ), soft_cap_multiplier( 1.0 )
    {
      background              = true;
      aoe                     = data().max_targets() - 1;
      attack_power_mod.direct = data().effectN( 2 ).ap_coeff();
      base_multiplier         = dk()->talent.rider.let_terror_reign->effectN( 2 ).percent();
      target_filter_callback  = secondary_targets_only();
    }

    double composite_aoe_multiplier( const action_state_t* state ) const override
    {
      double cam = horseman_spell_t::composite_aoe_multiplier( state );

      cam *= soft_cap_multiplier;

      return cam;
    }

  public:
    double soft_cap_multiplier;
  };

  struct epidemic_whitemane_main_t final : public horseman_spell_t
  {
    epidemic_whitemane_main_t( std::string_view name, horseman_pet_t* p )
      : horseman_spell_t( p, name, p->dk()->pet_spell.whitemane_epidemic ), soft_cap_multiplier( 1.0 )
    {
      background              = true;
      aoe                     = 0;
      attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
      base_multiplier         = dk()->talent.rider.let_terror_reign->effectN( 2 ).percent();
      impact_action           = get_action<epidemic_aoe_whitemane_t>( "epidemic_aoe", p );
    }

    double composite_aoe_multiplier( const action_state_t* state ) const override
    {
      double cam = horseman_spell_t::composite_aoe_multiplier( state );

      cam *= soft_cap_multiplier;

      return cam;
    }

  public:
    double soft_cap_multiplier;
  };

  struct epidemic_whitemane_t final : public horseman_spell_t
  {
    epidemic_whitemane_t( std::string_view name, horseman_pet_t* p )
      : horseman_spell_t( p, name, p->dk()->pet_spell.whitemane_epidemic ),
        custom_reduced_aoe_targets( 8.0 ),
        soft_cap_multiplier( 1.0 )
    {
      background              = true;
      aoe                     = 20;
      attack_power_mod.direct = 0;
      impact_action           = get_action<epidemic_whitemane_main_t>( "epidemic_main", p );
      target_filter_callback  = dk_dot_or_debuff_only( &death_knight_td_t::dots_t::virulent_plague );
    }

    void execute() override
    {
      // Reset target cache because of smart targetting
      target_cache.is_valid = false;
      horseman_spell_t::execute();
    }

    void impact( action_state_t* s ) override
    {
      // Set the multiplier for reduced aoe soft cap
      if ( s->n_targets > 0.0 && s->n_targets > custom_reduced_aoe_targets )
        soft_cap_multiplier = sqrt( custom_reduced_aoe_targets / std::min<int>( sim->max_aoe_enemies, s->n_targets ) );
      else
        soft_cap_multiplier = 1.0;

      debug_cast<epidemic_whitemane_main_t*>( impact_action )->soft_cap_multiplier               = soft_cap_multiplier;
      debug_cast<epidemic_aoe_whitemane_t*>( impact_action->impact_action )->soft_cap_multiplier = soft_cap_multiplier;

      horseman_spell_t::impact( s );
    }

  private:
    double custom_reduced_aoe_targets;  // Not in spelldata
    double soft_cap_multiplier;
  };

  struct undeath_whitemane_t final : public horseman_spell_t
  {
    undeath_whitemane_t( std::string_view name, horseman_pet_t* p, std::string_view options_str )
      : horseman_spell_t( p, name, p->dk()->pet_spell.undeath_range )
    {
      parse_options( options_str );
      impact_action      = p->dk()->background_actions.undeath_dot;
      cooldown->duration = p->dk()->pet_spell.undeath_dot->duration();
    }
  };

  whitemane_pet_t( death_knight_t* owner ) : horseman_pet_t( owner, "whitemane", PET_WHITEMANE )
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

  action_t* create_action( std::string_view name, std::string_view options_str ) override
  {
    if ( name == "death_coil" )
    {
      death_coil_foreground = new death_coil_whitemane_t( "death_coil", this, options_str );
      return death_coil_foreground;
    }
    if ( name == "undeath" )
      return new undeath_whitemane_t( "undeath", this, options_str );

    return horseman_pet_t::create_action( name, options_str );
  }

  void create_actions() override
  {
    death_knight_pet_t::create_actions();
    if ( dk()->talent.rider.let_terror_reign.ok() && dk()->specialization() == DEATH_KNIGHT_UNHOLY )
    {
      epidemic   = new epidemic_whitemane_t( "epidemic", this );
      death_coil = new death_coil_whitemane_background_t( "death_coil_let_terror_reign", this );
    }
  }

public:
  epidemic_whitemane_t* epidemic;
  death_coil_whitemane_background_t* death_coil;
  action_t* death_coil_foreground;
};

// ==========================================================================
// Trollbane Pet
// ==========================================================================
struct trollbane_pet_t final : public horseman_pet_t
{
  struct trollbane_chains_of_ice_t final : public horseman_spell_t
  {
    trollbane_chains_of_ice_t( std::string_view n, horseman_pet_t* p, std::string_view options_str )
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
    obliterate_trollbane_t( std::string_view name, horseman_pet_t* p, std::string_view options_str )
      : horseman_melee_t( p, name, p->dk()->pet_spell.trollbane_obliterate )
    {
      parse_options( options_str );
    }
  };

  struct obliterate_background_trollbane_t final : public horseman_melee_t
  {
    obliterate_background_trollbane_t( std::string_view name, horseman_pet_t* p )
      : horseman_melee_t( p, name, p->dk()->pet_spell.trollbane_obliterate )
    {
      base_multiplier    = dk()->talent.rider.let_terror_reign->effectN( 1 ).percent();
      cooldown->duration = 0_ms;  // Ignore the cooldown for the background casts
    }

    void execute() override
    {
      if ( consumed_km )
        set_school_override( SCHOOL_FROST );

      horseman_melee_t::execute();

      if ( consumed_km )
        clear_school_override();

      consumed_km = false;
    }

    double composite_da_multiplier( const action_state_t* state ) const override
    {
      double m = horseman_melee_t::composite_da_multiplier( state );
      // Copy of logic used in obliterate_strike_t to apply mastery
      if ( dk()->spec.frostreaper->ok() && get_school() == SCHOOL_FROST )
      {
        m *= 1.0 + dk()->cache.mastery_value();
      }

      return m;
    }

  public:
    bool consumed_km;
  };

  struct frostscythe_trollbane_t final : horseman_melee_t
  {
    frostscythe_trollbane_t( std::string_view name, horseman_pet_t* p )
      : horseman_melee_t( p, name, p->dk()->pet_spell.trollbane_frostscythe )
    {
      base_multiplier     = dk()->talent.rider.let_terror_reign->effectN( 1 ).percent();
      aoe                 = -1;
      reduced_aoe_targets = data().effectN( 5 ).base_value();
      background          = true;
    }
  };

  trollbane_pet_t( death_knight_t* owner ) : horseman_pet_t( owner, "trollbane", PET_TROLLBANE )
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

  action_t* create_action( std::string_view name, std::string_view options_str ) override
  {
    if ( name == "obliterate" )
      return new obliterate_trollbane_t( "obliterate", this, options_str );
    if ( name == "chains_of_ice" )
      return new trollbane_chains_of_ice_t( name, this, options_str );

    return horseman_pet_t::create_action( name, options_str );
  }

  void create_actions() override
  {
    death_knight_pet_t::create_actions();
    if ( dk()->talent.rider.let_terror_reign.ok() && dk()->specialization() == DEATH_KNIGHT_FROST )
    {
      obliterate  = new obliterate_background_trollbane_t( "obliterate_let_terror_reign", this );
      frostscythe = new frostscythe_trollbane_t( "frostscythe", this );
    }
  }

public:
  obliterate_background_trollbane_t* obliterate;
  action_t* frostscythe;
};

// ==========================================================================
// Nazgrim Pet
// ==========================================================================
struct nazgrim_pet_t final : public horseman_pet_t
{
  nazgrim_pet_t( death_knight_t* owner ) : horseman_pet_t( owner, "nazgrim", PET_NAZGRIM )
  {
    npc_id                      = owner->spell.summon_nazgrim->effectN( 1 ).misc_value1();
    main_hand_weapon.swing_time = 2_s;
  }

  struct scourge_strike_shadow_nazgrim_t final : public horseman_melee_t
  {
    scourge_strike_shadow_nazgrim_t( std::string_view name, horseman_pet_t* p )
      : horseman_melee_t( p, name, p->dk()->pet_spell.nazgrim_scourge_strike_shadow )
    {
      background = dual = true;
    }
  };

  struct scourge_strike_nazgrim_t final : public horseman_melee_t
  {
    scourge_strike_nazgrim_t( std::string_view name, horseman_pet_t* p, std::string_view options_str )
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

  action_t* create_action( std::string_view name, std::string_view options_str ) override
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
  struct disease_cloud_t final : public pet_spell_t<abomination_pet_t>
  {
    disease_cloud_t( std::string_view name, abomination_pet_t* p )
      : pet_spell_t( p, name, p->dk()->spell.disease_cloud_damage )
    {
      background = true;
      aoe        = -1;
      may_miss = may_dodge = may_parry = false;
    }
  };

  struct disease_cloud_event_t final : public event_t
  {
    disease_cloud_event_t( abomination_pet_t* p, timespan_t interval )
      : event_t( *p->sim, interval ), period( interval ), pet( p )
    {
    }

    const char* name() const override
    {
      return "disease_cloud_tick_event";
    }

    void execute() override
    {
      if ( pet->is_sleeping() )
        return;

      pet->disease_cloud->execute();

      make_event<disease_cloud_event_t>( *pet->sim, pet, period );
    }

  private:
    timespan_t period;
    abomination_pet_t* pet;
  };

  abomination_pet_t( death_knight_t* owner ) : death_knight_pet_t( owner, "abomination", PET_ABOMINATION )
  {
    npc_id                            = owner->spell.summon_abomination->effectN( 1 ).misc_value1();
    main_hand_weapon.type             = WEAPON_BEAST;
    main_hand_weapon.swing_time       = 3.6_s;
    affected_by.commander_of_the_dead = true;
    affected_by.grave_mastery         = true;
    owner_coeff.ap_from_ap            = 2.34;
    resource_regeneration             = regen_type::DISABLED;
  }

  void arise() override
  {
    // Period information hasnt been found in data, hard coding for now.
    timespan_t period = timespan_t::from_seconds( dk()->talent.unholy.raise_abomination->effectN( 1 ).base_value() );

    make_event<disease_cloud_event_t>( *sim, this, period );

    death_knight_pet_t::arise();
    // Assume precombat abominations have to walk far further than normal
    double dist = precombat_spawn ? 15 : rng().range( 0, dk()->spell.summon_abomination->effectN( 1 ).radius() );
    trigger_pet_movement( dist );
  }

  void create_actions() override
  {
    death_knight_pet_t::create_actions();
    disease_cloud = new disease_cloud_t( "disease_cloud", this );
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_NONE;
  }

  attack_t* create_main_hand_auto_attack() override
  {
    return new auto_attack_melee_t<abomination_pet_t>( this, "auto_attack_mh" );
  }

  public: 
    propagate_const<action_t*> disease_cloud;
};

}  // namespace pets

namespace
{  // UNNAMED NAMESPACE

// ==========================================================================
// Death Knight Custom Buff Structs
// ==========================================================================
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
// Death Knight Empowered actions setup
// ==========================================================================

template <typename Data, typename Base = action_state_t>
struct death_knight_empower_action_state_t : public Base, public Data
{
  static_assert( std::is_base_of_v<action_state_t, Base> );
  static_assert( std::is_default_constructible_v<Data> );  // required for initialize
  static_assert( std::is_copy_assignable_v<Data> );        // required for copy_state

  using Base::Base;

  void initialize() override
  {
    Base::initialize();
    *static_cast<Data*>( this ) = Data{};
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    Base::debug_str( s );
    if constexpr ( fmt::is_formattable<Data>::value )
      fmt::print( s, " {}", *static_cast<const Data*>( this ) );
    return s;
  }

  void copy_state( const action_state_t* o ) override
  {
    Base::copy_state( o );
    *static_cast<Data*>( this ) =
        *static_cast<const Data*>( static_cast<const death_knight_empower_action_state_t*>( o ) );
  }
};

struct empower_data_t
{
  empower_e empower;

  friend void sc_format_to( const empower_data_t& data, fmt::format_context::iterator out )
  {
    fmt::format_to( out, "empower_level={}", static_cast<int>( data.empower ) );
  }
};

template <class BASE>
struct death_knight_empowered_base_t : public BASE
{
protected:
  using state_t = death_knight_empower_action_state_t<empower_data_t>;

public:
  empower_e max_empower;

  death_knight_empowered_base_t( std::string_view name, death_knight_t* p, const spell_data_t* spell )
    : BASE( name, p, spell ), max_empower( empower_e::EMPOWER_3 )
  {
    BASE::can_have_one_button_penalty = false;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, BASE::target );
  }

  state_t* cast_state( action_state_t* s )
  {
    return static_cast<state_t*>( s );
  }

  const state_t* cast_state( const action_state_t* s ) const
  {
    return static_cast<const state_t*>( s );
  }
};

template <class BASE>
struct death_knight_empowered_release_t : public death_knight_empowered_base_t<BASE>
{
  using base = death_knight_empowered_base_t<BASE>;

  death_knight_empowered_release_t( std::string_view name, death_knight_t* p, const spell_data_t* spell )
    : death_knight_empowered_base_t<BASE>( name, p, spell )
  {
    base::dual = true;
  }

  empower_e empower_level( const action_state_t* s ) const
  {
    return base::cast_state( s )->empower;
  }

  int empower_value( const action_state_t* s ) const
  {
    return static_cast<int>( base::cast_state( s )->empower );
  }
};

template <class BASE>
struct death_knight_empowered_charge_t : public death_knight_empowered_base_t<BASE>
{
  using base = death_knight_empowered_base_t<BASE>;

  action_t* release_spell;
  stats_t* dummy_stat;  // used to hack channel tick time into execute time
  stats_t* orig_stat;
  int empower_to;
  timespan_t base_empower_duration;
  timespan_t lag;

  void setup_empower_stats( int empower_level )
  {
    assert( empower_level > 0 );
    assert( empower_level <= 5 );
    setup_empower_stats( static_cast<empower_e>( empower_level ) );
  }

  void setup_empower_stats( empower_e empower_level )
  {
    empower_to = empower_level;
    if ( static_cast<empower_e>( empower_to ) == EMPOWER_MAX )
    {
      base_empower_duration = max_hold_time();
    }
    else
    {
      empower_to            = std::min( static_cast<int>( base::max_empower ), empower_to );
      base_empower_duration = base_time_to_empower( static_cast<empower_e>( empower_to ) );
    }

    // apply parsed modifiers
    base::dot_duration      = base::player->get_passive_value( base::data(), "duration" );
    base::dot_duration.base = base_empower_duration;
    base::base_tick_time    = base::dot_duration;
  }

  death_knight_empowered_charge_t( std::string_view name, death_knight_t* p, const spell_data_t* spell,
                                   std::string_view options_str )
    : base( name, p, spell ),
      release_spell( nullptr ),
      dummy_stat( p->get_stats( "dummy_stat" ) ),
      orig_stat( base::stats ),
      empower_to( EMPOWER_MAX ),
      base_empower_duration( 0_ms ),
      lag( 0_ms )
  {
    base::channeled = true;
    base::add_option( opt_int( "empower_to", empower_to, EMPOWER_1, EMPOWER_MAX ) );

    base::parse_options( options_str );

    setup_empower_stats( empower_to );

    base::gcd_type = gcd_haste_type::NONE;
    if ( base::trigger_gcd > timespan_t::zero() )
      base::min_gcd = base::trigger_gcd;
  }

  template <typename T>
  void create_release_spell( std::string_view n )
  {
    static_assert( std::is_base_of_v<death_knight_empowered_release_t<BASE>, T>,
                   "Empowered release spell must be dervied from empowered_release_spell_t." );

    this->release_spell             = get_empower_release_action<T>( n, base::p() );
    this->release_spell->stats      = base::stats;
    this->release_spell->background = false;
  }

  timespan_t base_time_to_empower( empower_e emp ) const
  {
    // TODO: confirm these values and determine if they're set values or adjust based on a formula
    // Currently all empowered spells are 2.5s base and 3.25s with empower 4
    switch ( emp )
    {
      case empower_e::EMPOWER_1:
        return 1000_ms;
      case empower_e::EMPOWER_2:
        return 1750_ms;
      case empower_e::EMPOWER_3:
        return 2500_ms;
      case empower_e::EMPOWER_4:
        return 3250_ms;
      default:
        break;
    }

    return 0_ms;
  }

  timespan_t max_hold_time() const
  {
    // TODO: confirm if this is affected by duration mods/haste
    return base_time_to_empower( base::max_empower ) + 2_s;
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    return composite_dot_duration( s );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    auto dur = base::composite_dot_duration( s );

    // hack so we always have a non-zero duration in order to trigger last_tick()
    if ( dur == 0_ms )
      return 1_ms;

    return dur + lag;
  }

  timespan_t composite_time_to_empower( const action_state_t* s, empower_e emp ) const
  {
    auto base = base_time_to_empower( emp );
    auto mult = composite_dot_duration( s ) / base_empower_duration;

    return base * mult;
  }

  empower_e empower_level( const dot_t* d ) const
  {
    auto emp = empower_e::EMPOWER_NONE;

    if ( !d->is_ticking() )
      return emp;

    auto s       = d->state;
    auto elapsed = tick_time( s ) - d->time_to_next_full_tick();

    if ( elapsed >= composite_time_to_empower( s, empower_e::EMPOWER_4 ) )
      emp = empower_e::EMPOWER_4;
    else if ( elapsed >= composite_time_to_empower( s, empower_e::EMPOWER_3 ) )
      emp = empower_e::EMPOWER_3;
    else if ( elapsed >= composite_time_to_empower( s, empower_e::EMPOWER_2 ) )
      emp = empower_e::EMPOWER_2;
    else if ( elapsed >= composite_time_to_empower( s, empower_e::EMPOWER_1 ) )
      emp = empower_e::EMPOWER_1;

    return std::min( base::max_empower, emp );
  }

  void init() override
  {
    base::init();
    assert( release_spell && "Empowered charge spell must have a release spell." );
  }

  void execute() override
  {
    // pre-determine lag here per every execute
    lag = base::rng().gauss( base::sim->channel_lag );

    base::execute();
  }

  void tick( dot_t* d ) override
  {
    // For proper DPET analysis, we need to treat charge spells as non-channel, since channelled spells sum up tick
    // times to get the execute time, but this does not work for fire breath which also has a dot component. Instead we
    // hijack the stat obj during action_t:tick() causing the channel's tick to be recorded onto a throwaway stat obj.
    // We then record the corresponding tick time as execute time onto the original real stat obj. See further notes in
    // evoker_t::analyze().
    base::stats = dummy_stat;
    base::tick( d );
    base::stats = orig_stat;

    base::stats->iteration_total_execute_time += d->time_to_tick();
  }

  virtual player_t* get_release_target( dot_t* )
  {
    return base::target;
  }

  void last_tick( dot_t* d ) override
  {
    base::last_tick( d );

    auto release_target = get_release_target( d );

    // if ( empower_level( d ) == empower_e::EMPOWER_NONE || !release_target )
    // {
    //   base::p()->was_empowering = false;
    //   return;
    // }

    // If we have no valid targets, do not fire off the release spell
    if ( release_target == nullptr )
      return;

    release_spell->set_target( release_target );

    auto emp_state        = release_spell->get_state();
    emp_state->target     = release_target;
    release_spell->target = release_target;
    release_spell->snapshot_state( emp_state, release_spell->amount_type( emp_state ) );

    base::cast_state( emp_state )->empower = empower_level( d );

    release_spell->schedule_execute( emp_state );

    // hack to prevent dot_t::last_tick() from schedule_ready()'ing the player
    d->current_action = release_spell;
    // hack to prevent channel lag being added when player is schedule_ready()'d after the release spell execution
    base::p()->last_foreground_action = release_spell;
    // Start GCD - All Empowerw have a GCD of 0.5s after completion.
    base::start_gcd();
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

  propagate_const<gain_t*> gain;
  double rp_per_tick;
  std::vector<player_effect_t> runic_power_multiplier_effects;
  std::vector<player_effect_t> runic_power_flat_effects;

  action_t* replacement_action;
  buff_t* replacement_action_buff;
  bool always_replace;
  bool was_replaced;
  bool add_replacement_executes;

  struct
  {
  } affected_by;

  death_knight_action_t( std::string_view n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() )
    : action_base_t( n, p, s ),
      gain( nullptr ),
      rp_per_tick( 0 ),
      runic_power_multiplier_effects(),
      runic_power_flat_effects(),
      replacement_action( nullptr ),
      replacement_action_buff( nullptr ),
      always_replace( false ),
      was_replaced( false ),
      add_replacement_executes( false ),
      affected_by{}
  {
    this->may_glance = false;
    this->track_cd_waste = this->cooldown->duration > 0_s;
    this->may_crit = !this->data().flags( spell_attribute::SX_CANNOT_CRIT );
    this->tick_may_crit = this->data().flags( spell_attribute::SX_TICK_MAY_CRIT );
    this->rolling_periodic = this->data().flags( spell_attribute::SX_ROLLING_PERIODIC );

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
      p->apply_action_effects( this );

      if ( this->type == action_e::ACTION_SPELL || this->type == action_e::ACTION_ATTACK )
      {
        p->apply_target_action_effects( this );
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

  bool is_rp_energize( size_t idx )
  {
    if ( action_base_t::data().effects().size() < idx )
      return false;
    const spelleffect_data_t& eff = action_base_t::data().effectN( idx );
    return eff.type() == E_ENERGIZE && eff.misc_value1() == POWER_RUNIC_POWER;
  }

  std::vector<player_effect_t>* get_effect_vector( const spelleffect_data_t& eff, player_effect_t& tmp, double& val_mul,
                                                   std::string& str, bool& flat, bool force,
                                                   const pack_t<player_effect_t>& pack ) override
  {
    if ( ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER ) &&
         ( action_base_t::data().affected_by_all( eff ) || force ) )
    {
      int idx = util::effect_idx_from_property_type( static_cast<property_type_t>( eff.misc_value1() ) );
      if ( idx > 0 && is_rp_energize( idx ) )
      {
        str = "runic power multiplier";
        return &runic_power_multiplier_effects;
      }
    }

    else if ( ( eff.subtype() == A_ADD_FLAT_MODIFIER || eff.subtype() == A_ADD_FLAT_LABEL_MODIFIER ) &&
              ( action_base_t::data().affected_by_all( eff ) || force ) )
    {
      flat    = true;
      int idx = util::effect_idx_from_property_type( static_cast<property_type_t>( eff.misc_value1() ) );
      if ( idx > 0 && is_rp_energize( idx ) )
      {
        str = "runic power mod flat";
        return &runic_power_flat_effects;
      }
    }

    return action_base_t::get_effect_vector( eff, tmp, val_mul, str, flat, force, pack );
  }

  size_t total_effects_count() const override
  {
    return action_base_t::total_effects_count() + runic_power_multiplier_effects.size() +
           runic_power_flat_effects.size();
  }

  void print_parsed_custom_type( report::sc_html_stream& os ) const override
  {
    action_base_t::print_parsed_custom_type( os );

    action_base_t::template print_parsed_type<base_t>( os, &base_t::runic_power_multiplier_effects,
                                                       "RP Gen Multiplier" );
    action_base_t::template print_parsed_type<base_t>( os, &base_t::runic_power_flat_effects, "RP Gen Modifier" );
  }

  virtual double runic_power_generation_multiplier( const action_state_t* ) const
  {
    double m = 1.0;

    for ( const auto& i : runic_power_multiplier_effects )
      m *= 1.0 + base_t::get_effect_value( i );

    return m;
  }

  void set_replacement_action( action_t* a, buff_t* buff = nullptr, bool child = true )
  {
    if ( !a )
    {
      p()->sim->errorf( "%s Attempting to set null replacement action for %s. Ignoring.\n", p()->name(),
                        full_name().c_str() );
      return;
    }

    this->replacement_action = a;

    if ( buff )
    {
      this->replacement_action_buff = buff;
      if ( child )
      {
        this->add_child( a );
        this->add_replacement_executes = true;
      }
    }
    else
      this->always_replace = true;
  }

  void set_replacement_action( int id, buff_t* buff = nullptr )
  {
    action_t* a = find_action_by_id( id );
    if ( !a )
    {
      p()->sim->errorf(
          "%s Attempting to set replacement action by id %d for %s, but no such action exists. Ignoring.\n",
          p()->name(), id, full_name().c_str() );
      return;
    }
    set_replacement_action( a, buff );
  }

  void set_replacement_action( std::string_view name, buff_t* buff = nullptr )
  {
    action_t* a = p()->find_action( name );
    if ( !a )
    {
      p()->sim->errorf(
          "%s Attempting to set replacement action by name '%s' for %s, but no such action exists. Ignoring.\n",
          p()->name(), name.data(), full_name().c_str() );
      return;
    }
    set_replacement_action( a, buff );
  }

  action_t* find_action_by_id( int id )
  {
    for ( auto& a : p()->action_list )
    {
      if ( a->data().id() == id )
        return a;
    }
    return nullptr;
  }

  template <typename T>
  target_filter_callback_t dot_or_debuff_only( T d )
  {
    return [ &, d ]( const action_t*, player_t* target ) {
      return p()->dot_or_debuff_active( d, p()->get_target_data( target ) );
    };
  }

  target_filter_callback_t unholy_diseases_only()
  {
    return [ & ]( const action_t*, player_t* target ) {
      return p()->dot_or_debuff_active( &death_knight_td_t::dots_t::virulent_plague, p()->get_target_data( target ) ) ||
             p()->dot_or_debuff_active( &death_knight_td_t::dots_t::dread_plague, p()->get_target_data( target ) );
    };
  }

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
      for ( const auto& i : runic_power_flat_effects )
        amount += 1.0 + base_t::get_effect_value( i );

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

  double cost() const override
  {
    if ( !this->replacement_action )
      return action_base_t::cost();

    if ( this->always_replace || ( this->replacement_action_buff && this->replacement_action_buff->check() ) )
      return this->replacement_action->cost();

    return action_base_t::cost();
  }

  bool ready() override
  {
    if ( !this->replacement_action )
      return action_base_t::ready();

    if ( this->always_replace || ( this->replacement_action_buff && this->replacement_action_buff->check() ) )
      return this->replacement_action->ready();

    return action_base_t::ready();
  }

  bool action_ready() override
  {
    if ( !this->replacement_action || this->cooldown->duration == 0_ms )
      return action_base_t::action_ready();

    if ( this->always_replace || ( this->replacement_action_buff && this->replacement_action_buff->check() ) )
      return this->replacement_action->action_ready();

    return action_base_t::action_ready();
  }

  void queue_execute( execute_type et ) override
  {
    if ( !this->replacement_action || this->cooldown->duration == 0_ms )
      return action_base_t::queue_execute( et );

    if ( this->always_replace || ( this->replacement_action_buff && this->replacement_action_buff->check() ) )
      return this->replacement_action->queue_execute( et );

    return action_base_t::queue_execute( et );
  }

  void execute() override
  {
    if ( this->replacement_action )
    {
      if ( this->always_replace || ( this->replacement_action_buff && this->replacement_action_buff->check() ) )
      {
        this->replacement_action->set_target( this->target );
        this->replacement_action->execute();

        if ( !this->always_replace && this->add_replacement_executes )
          this->stats->add_execute( 0_ms, this->target );

        this->was_replaced = true;
        return;
      }
    }

    this->was_replaced = false;

    action_base_t::execute();
    // For non tank DK's, we proc the ability on CD, attached to thier own executes, to simulate it
    if ( p()->talent.blood_draw.ok() && p()->specialization() != DEATH_KNIGHT_BLOOD &&
         p()->background_actions.blood_draw->ready() && p()->in_combat )
    {
      p()->background_actions.blood_draw->execute();
    }
  }

  void impact( action_state_t* s ) override
  {
    action_base_t::impact( s );
    if ( !this->background && s->target != p() && s->target != nullptr && s->target->is_sleeping() &&
         s->chain_target == 0 )
      p()->last_target = s->target;

    if ( p()->talent.sanlayn.pact_of_the_sanlayn.ok() && p()->pets.blood_beast.active_pet() != nullptr &&
         dbc::has_common_school( this->get_school(), SCHOOL_SHADOW ) )
    {
      p()->pets.blood_beast.active_pet()->accumulator +=
          s->result_amount * p()->talent.sanlayn.pact_of_the_sanlayn->effectN( 1 ).percent();
    }

    if ( p()->talent.deathbringer.reapers_mark.ok() && s->result_raw != 0 &&
         this->data().id() != p()->spell.reapers_mark_explosion->id() &&
         this->data().id() != 66198 /* Obliterate offhand does not count */ &&
         this->data().id() != p()->spell.hyperpyrexia_damage->id() &&
         this->data().id() != p()->spell.icy_death_torrent_damage->id() )
    {
      death_knight_td_t* td = get_td( s->target );
      if ( td->debuff.reapers_mark->check() )
      {
        if ( this->get_school() == SCHOOL_SHADOWFROST ||
             ( p()->buffs.dark_talons_shadowfrost->check() &&
               // death strike is counted as shadowfrost, but the school remains physical to keep bloodshot functional
               ( this->data().id() == p()->talent.death_strike->id() ||
                 // 7/24/25 when dark talons is active, it will treat all sources of frost/shadow damage as if they
                 // are shadowfrost
                 ( p()->bugs && dbc::has_common_school( this->get_school(), SCHOOL_SHADOWFROST ) ) ) ) )
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
        else if ( this->get_school() == SCHOOL_SHADOW || this->get_school() == SCHOOL_FROST )
        {
          td->debuff.reapers_mark->increment( 1 );
        }
      }
    }
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = action_base_t::composite_da_multiplier( state );
    if ( p()->talent.deathbringer.wave_of_souls->ok() && this->get_school() == SCHOOL_SHADOWFROST )
    {
      death_knight_td_t* td = get_td( state->target );
      if ( td->debuff.wave_of_souls->check() )
      {
        m *= 1 + td->debuff.wave_of_souls->check_stack_value();
      }
    }
    // Death strike only appears to get the value of a single stack of wave of souls
    if ( p()->talent.deathbringer.wave_of_souls->ok() && p()->talent.deathbringer.dark_talons->ok() &&
          this->data().id() == p()->talent.death_strike->id() && p()->buffs.icy_talons->up() )
    {
      death_knight_td_t* td = get_td( state->target );
      if ( td->debuff.wave_of_souls->check() )
      {
        m *= 1 + td->debuff.wave_of_souls->value();
      }
    }
    return m;
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
  death_knight_melee_attack_t( std::string_view n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() )
    : death_knight_action_t( n, p, s )
  {
    special    = true;
    may_glance = false;
  }
};

// ==========================================================================
// Death Knight Spell
// ==========================================================================

struct death_knight_spell_t : public death_knight_action_t<spell_t>
{
  death_knight_spell_t( std::string_view n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() )
    : death_knight_action_t( n, p, s )
  {
  }
};

struct death_knight_empowered_charge_spell_t : public death_knight_empowered_charge_t<death_knight_spell_t>
{
  using base_t = death_knight_empowered_charge_spell_t;

  death_knight_empowered_charge_spell_t( std::string_view n, death_knight_t* p, const spell_data_t* s, std::string_view o )
    : death_knight_empowered_charge_t( n, p, s, o )
  {
  }

  player_t* get_release_target( dot_t* d ) override
  {
    auto t = d->state->target;

    if ( t->is_sleeping() )
      t = nullptr;

    return t;
  }
};

struct death_knight_empowered_release_spell_t : public death_knight_empowered_release_t<death_knight_spell_t>
{
  using base_t = death_knight_empowered_release_t<death_knight_spell_t>;

  death_knight_empowered_release_spell_t( std::string_view n, death_knight_t* p, const spell_data_t* s )
    : death_knight_empowered_release_t( n, p, s )
  {
  }
};

// ==========================================================================
// Death Knight Absorb
// ==========================================================================
struct death_knight_absorb_t : public absorb_t
{
  death_knight_absorb_t( std::string_view name, death_knight_t* p, const spell_data_t* s )
    : absorb_t( name, p, s ), absorb_buff( nullptr )
  {
    may_miss = callbacks = harmful = false;
    if ( absorb_buff != nullptr )
    {
      absorb_buff->set_absorb_source( stats );
      absorb_buff->set_absorb_gain( gain );
    }
  }

  death_knight_t* p() const
  {
    return debug_cast<death_knight_t*>( player );
  }

  void execute() override
  {
    if ( target->is_enemy() )
      target = player;

    absorb_t::execute();
  }

  absorb_buff_t* create_buff( const action_state_t* s ) override
  {
    if ( absorb_buff != nullptr )
      return absorb_buff;

    auto b = absorb_t::create_buff( s );

    return b;
  }

  absorb_buff_t* absorb_buff;
};

struct permafrost_t final : public death_knight_absorb_t
{
  permafrost_t( death_knight_t* p ) : death_knight_absorb_t( "frost_shield", p, p->spell.frost_shield_buff )
  {
    background = proc = true;
    may_crit          = false;
  }
};

// ==========================================================================
// Death Knight Runeforge Actions
// ==========================================================================

// Rune of Apocalpyse - Pestilence ==========================================
struct runeforge_apocalypse_pestilence_t final : public death_knight_spell_t
{
  runeforge_apocalypse_pestilence_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->runeforge_spell.apocalypse_pestilence_damage )
  {
    background = true;
  }
};

// Fallen Crusader Heal =====================================================
struct fallen_crusader_heal_t final : public heal_t
{
  fallen_crusader_heal_t( std::string_view name, death_knight_t* p )
    : heal_t( name, p, p->runeforge_spell.unholy_strength )
  {
    background = true;
    target     = p;
    harmful = callbacks = may_crit = false;
    base_pct_heal                  = data().effectN( 2 ).percent();
  }

  // Procs by default target the target of the action that procced them.
  void execute() override
  {
    target = player;
    heal_t::execute();
  }
};

// Unending Thirst Heal =====================================================
struct unending_thirst_heal_t final : public heal_t
{
  unending_thirst_heal_t( std::string_view name, death_knight_t* p )
    : heal_t( name, p, p->runeforge_spell.unending_thirst )
  {
    background = true;
    target     = p;
    harmful = callbacks = may_crit = false;
    base_pct_heal                  = data().effectN( 2 ).percent();
  }
  // Procs by default target the target of the action that procced them.
  void execute() override
  {
    target = player;
    heal_t::execute();
  }
};

// Razorice Attack ==========================================================
struct razorice_attack_t final : public death_knight_spell_t
{
  razorice_attack_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->runeforge_spell.razorice_damage )
  {
    may_miss = callbacks = false;
    background = proc = true;
    target_debuff     = p->runeforge_spell.razorice_debuff;
    // Note, razorice always attacks with the main hand weapon, regardless of which hand triggers it
    weapon = &p->main_hand_weapon;
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );
    death_knight_td_t* td = p()->get_target_data( s->target );
    td->debuff.razorice->trigger();
  }
};

// Sanguination Heal ========================================================
struct sanguination_heal_t final : public heal_t
{
  sanguination_heal_t( std::string_view name, death_knight_t* p )
    : heal_t( name, p, p->runeforge_spell.sanguination_heal )
  {
    background    = true;
    harmful       = false;
    tick_pct_heal = data().effectN( 1 ).percent();

    // Sated-type debuff, for simplicity the debuff's duration is used as a simple cooldown in simc
    cooldown->duration = p->runeforge_spell.sanguination_cooldown->duration();
    target             = p;
  }
};

// Spellwarding Absorb ======================================================
struct spellwarding_absorb_t final : public death_knight_absorb_t
{
  spellwarding_absorb_t( std::string_view name, death_knight_t* p )
    : death_knight_absorb_t( name, p, p->runeforge_spell.spellwarding_absorb )
  {
    target     = p;
    background = true;
    harmful    = false;
  }

  void execute() override
  {
    base_dd_min = base_dd_max =
        p()->runeforge_spell.spellwarding_absorb->effectN( 2 ).percent() * player->resources.max[ RESOURCE_HEALTH ];

    death_knight_absorb_t::execute();
  }
};

// ==========================================================================
// Death Knight Pet Summon Spell
// ==========================================================================
struct death_knight_summon_spell_t : public death_knight_spell_t
{
  death_knight_summon_spell_t( std::string_view n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() )
    : death_knight_spell_t( n, p, s ), duration( 0_s )
  {
    harmful = may_crit = may_miss = false;
  }

  void set_duration( timespan_t d )
  {
    this->duration = d;
  }

public:
  timespan_t duration;
};

struct bloodworm_summon_t : public death_knight_summon_spell_t
{
  bloodworm_summon_t( std::string_view n, death_knight_t* p )
    : death_knight_summon_spell_t( n, p, p->talent.blood.bloodworms->effectN( 1 ).trigger() )
  {
    background = true;
  }

  void execute() override
  {
    death_knight_summon_spell_t::execute();
    p()->procs.bloodworms->occur();
    p()->pets.bloodworms.spawn();
  }
};

struct blood_beast_summon_t : public death_knight_summon_spell_t
{
  blood_beast_summon_t( std::string_view n, death_knight_t* p )
    : death_knight_summon_spell_t( n, p, p->spell.blood_beast_summon )
  {
    background = true;
  }

  void execute() override
  {
    death_knight_summon_spell_t::execute();
    p()->procs.blood_beast->occur();
    p()->pets.blood_beast.spawn();
  }
};

struct summon_lesser_ghoul_t : public death_knight_summon_spell_t
{
  summon_lesser_ghoul_t( std::string_view n, death_knight_t* p, const spell_data_t* s, lesser_ghoul_e type )
    : death_knight_summon_spell_t( n, p, s ), source( type ), putrefy_source( PUTREFY_SOURCE_NONE ), ap_mult( 1.0 )
  {
    background = true;
    set_duration( data().duration() );
    putrefy_instantly = s == p->spell.summon_putrefy_ghoul;
    ap_mult           = s == p->spell.summon_army_ghoul ? 1.75 : 1.0;
    switch ( source )
    {
      case lesser_ghoul_e::LESSER_SOUL_REAPER:
        putrefy_source = PUTREFY_SOURCE_SOUL_REAPER;
        break;
      case lesser_ghoul_e::LESSER_PUTREFY:
        putrefy_source = PUTREFY_SOURCE_PUTREFY;
        break;
      case lesser_ghoul_e::LESSER_FORBIDDEN_KNOWLEDGE:
        putrefy_source = PUTREFY_SOURCE_FORBIDDEN_KNOWLEDGE;
        break;
      default:
        break;
    }
  }

  void execute() override
  {
    death_knight_summon_spell_t::execute();
    if ( source != LESSER_ARMY_OF_THE_DEAD )
      set_duration( data().duration() );

    if ( p()->talent.unholy.commander_of_the_dead.ok() )
      duration *=
          1.0 + ( p()->cache.mastery() / 100 ) * p()->talent.unholy.commander_of_the_dead->effectN( 1 ).percent();

    if ( !p()->options.wcl_reporting_mode )
    {
      switch ( source )
      {
        case lesser_ghoul_e::LESSER_DOOMED_BIDDING_COIL:
          p()->pets.lesser_ghoul_db_coil.spawn( duration );
          p()->procs.lesser_ghoul_db->occur();
          break;
        case lesser_ghoul_e::LESSER_DOOMED_BIDDING_EPIDEMIC:
          p()->pets.lesser_ghoul_db_epi.spawn( duration );
          p()->procs.lesser_ghoul_db->occur();
          break;
        case lesser_ghoul_e::LESSER_FESTERING_STRIKE:
          p()->pets.lesser_ghoul_fs.spawn( duration );
          p()->procs.lesser_ghoul_fs->occur();
          break;
        case lesser_ghoul_e::LESSER_ARMY_OF_THE_DEAD:
          p()->pets.lesser_ghoul_army.spawn( duration );
          p()->procs.lesser_ghoul_army->occur();
          break;
        case lesser_ghoul_e::LESSER_SOUL_REAPER:
          p()->pets.lesser_ghoul_putrefy.spawn( duration );
          break;
        case lesser_ghoul_e::LESSER_PUTREFY:
          p()->pets.lesser_ghoul_putrefy.spawn( duration );
          break;
        case lesser_ghoul_e::LESSER_FORBIDDEN_KNOWLEDGE:
          p()->pets.lesser_ghoul_fk.spawn( duration );
          break;
      }
    }
    else
    {
      p()->pets.lesser_ghoul.spawn( duration );
      p()->active_lesser_ghouls.back()->set_ap_multiplier( ap_mult );
    }

    if ( putrefy_instantly )
      p()->active_lesser_ghouls.back()->putrefy_ghoul( putrefy_source );
  }

private:
  lesser_ghoul_e source;
  putrefy_source_e putrefy_source;
  bool putrefy_instantly;
  double ap_mult;
};

struct summon_magus_t : public death_knight_summon_spell_t
{
  summon_magus_t( std::string_view n, death_knight_t* p, const spell_data_t* s, magus_of_the_dead_e type )
    : death_knight_summon_spell_t( n, p, s ), source( type )
  {
    background = true;
    switch ( type )
    {
      case MAGUS_DARK_TRANSFORMATION:
      case MAGUS_ARMY_OF_THE_DEAD:
        duration = p->talent.unholy.magus_of_the_dead->effectN( 1 ).time_value();
        break;
      case MAGUS_REANIMATION:
        duration = p->talent.unholy.reanimation->effectN( 2 ).time_value();
        break;
      case MAGUS_REANIMATION_FK:
        duration = p->talent.unholy.reanimation->effectN( 2 ).time_value() * p->talent.unholy.forbidden_knowledge_3->effectN( 2 ).percent();
        break;
      default:
        break;
    }
  }

  void execute() override
  {
    death_knight_summon_spell_t::execute();
    if ( !p()->options.wcl_reporting_mode )
    {
      switch ( source )
      {
        case magus_of_the_dead_e::MAGUS_DARK_TRANSFORMATION:
          p()->pets.dt_magus.spawn( duration );
          break;
        case magus_of_the_dead_e::MAGUS_ARMY_OF_THE_DEAD:
          p()->pets.army_magus.spawn( duration );
          break;
        case magus_of_the_dead_e::MAGUS_REANIMATION:
        case magus_of_the_dead_e::MAGUS_REANIMATION_FK:
          p()->pets.reanimation_magus.spawn( duration );
          break;
      }
    }
    else
      p()->pets.magus_of_the_dead.spawn( duration );
  }

private:
  magus_of_the_dead_e source;
};

struct summon_abomination_t : public death_knight_summon_spell_t
{
  summon_abomination_t( std::string_view n, death_knight_t* p )
    : death_knight_summon_spell_t( n, p, p->spell.summon_abomination )
  {
    background = true;
  }

  void execute() override
  {
    death_knight_summon_spell_t::execute();
    p()->pets.abomination.spawn( duration );
    set_duration( p()->pets.abomination.duration() );
  }
};

struct summon_gargoyle_t : public death_knight_summon_spell_t
{
  summon_gargoyle_t( std::string_view n, death_knight_t* p )
    : death_knight_summon_spell_t( n, p, p->spell.summon_gargoyle )
  {
    background = true;
  }

  void execute() override
  {
    death_knight_summon_spell_t::execute();
    p()->pets.gargoyle.spawn( duration );
    set_duration( p()->pets.gargoyle.duration() );
  }
};

struct raise_skulker_t : public death_knight_summon_spell_t
{
  raise_skulker_t( std::string_view n, death_knight_t* p ) : death_knight_summon_spell_t( n, p, p->spell.raise_skulker )
  {
    background = true;
  }

  void execute() override
  {
    death_knight_summon_spell_t::execute();
    p()->pets.risen_skulker.spawn();
  }
};

// Blood Shield =============================================================
struct blood_shield_buff_t final : public absorb_buff_t
{
  blood_shield_buff_t( death_knight_t* p ) : absorb_buff_t( p, "blood_shield", p->spell.blood_shield )
  {
    set_absorb_school( SCHOOL_PHYSICAL );
    set_absorb_source( p->get_stats( "blood_shield" ) );
  }

  void absorb_used( double absorbed, player_t* source ) override
  {
    absorb_buff_t::absorb_used( absorbed, source );

    death_knight_t* dk = debug_cast<death_knight_t*>( blood_shield_buff_t::source );

    if ( dk->rppm.carnage->trigger() )
    {
      dk->procs.carnage->occur();
      if ( dk->talent.blood.consumption.ok() )
        dk->cooldown.consumption->reset( true );
    }
  }
};

struct blood_shield_t final : public absorb_t
{
  blood_shield_t( std::string_view name, death_knight_t* p ) : absorb_t( name, p, p->spell.blood_shield )
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
  death_knight_heal_t( std::string_view n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() )
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
    if ( p()->buffs.vampiric_blood->up() )
      amount /= 1.0 + p()->talent.blood.vampiric_blood->effectN( 3 ).percent();

    amount *= 1.0 + p()->talent.blood.iron_heart->effectN( 2 ).percent();

    auto final_amount = amount + current_value;

    // Blood Shield caps at 50% max health
    if ( final_amount > ( player->resources.max[ RESOURCE_HEALTH ] * p()->mastery.blood_shield->effectN( 3 ).percent() ) )
      final_amount = player->resources.max[ RESOURCE_HEALTH ] * p()->mastery.blood_shield->effectN( 3 ).percent();

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
  death_knight_leech_damage_heal_t( std::string_view n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() )
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

// Inexorable Assault =======================================================
struct inexorable_assault_damage_t final : public death_knight_spell_t
{
  inexorable_assault_damage_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.inexorable_assault_damage )
  {
    background = true;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    // first stack is decremented by the consuming action, handle extra stacks here
    double m = death_knight_spell_t::composite_da_multiplier( s );

    int starting_stacks = p()->buffs.inexorable_assault->stack();

    p()->buffs.inexorable_assault->decrement(
        as<int>( p()->talent.frost.inexorable_assault->effectN( 1 ).base_value() ) - 1 );

    int extra_stacks_consumed = starting_stacks - std::max(p()->buffs.inexorable_assault->stack(), 1);

    m *= 1 + extra_stacks_consumed;

    return m;
  }
};

// Icy Death Torrent ========================================================
struct icy_death_torrent_t final : public death_knight_spell_t
{
  icy_death_torrent_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.icy_death_torrent_damage )
  {
    background = true;
    aoe        = -1;
  }
};

// Cryogennic Chamber =======================================================
struct cryogenic_chamber_t final : public death_knight_spell_t
{
  cryogenic_chamber_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.cryogenic_chamber_damage )
  {
    background = true;
    aoe        = -1;
  }
};

struct hyperpyrexia_damage_t final : public residual_action::residual_periodic_action_t<death_knight_spell_t>
{
  hyperpyrexia_damage_t( std::string_view name, death_knight_t* p )
    : residual_action::residual_periodic_action_t<death_knight_spell_t>( name, p, p->spell.hyperpyrexia_damage )
  {
    background = true;
    may_miss = hasted_ticks = false;
  }
};

// ==========================================================================
// Death Knight Buffs
// ==========================================================================
namespace buffs
{
using death_knight_buff_t = death_knight_buff_base_t<>;

// Breath of Sindragosa =====================================================
struct breath_of_sindragosa_buff_t : public death_knight_buff_t
{
  breath_of_sindragosa_buff_t( death_knight_t* p, std::string_view name, const spell_data_t* s )
    : death_knight_buff_t( p, name, s ),
      tick_period( p->talent.frost.breath_of_sindragosa->effectN( 1 ).period() ),
      rune_gen( as<int>( p->spell.breath_of_sindragosa_rune_gen->effectN( 1 ).base_value() ) )
  {
    cooldown->duration = 0_ms;  // Handled by the action
    set_tick_on_application( false );
    set_tick_zero( false );

    set_tick_callback( [ &, p ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ ) {
      // Default player target to begin with, if theres a valid last target, switch to that.
      player_t* bos_target = p->target;
      if ( !p->last_target->is_sleeping() && p->last_target != nullptr && p->last_target != source )
        bos_target = p->last_target;

      p->background_actions.breath_of_sindragosa_tick->execute_on_target( bos_target );
    } );
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
  const timespan_t tick_period;
  int rune_gen;
};

// Pillar of Frost Buff =====================================================
struct pillar_of_frost_buff_t final : public death_knight_buff_t
{
  pillar_of_frost_buff_t( death_knight_t* p, std::string_view n, const spell_data_t* s )
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
    extend_duration( timespan_t::from_seconds( added_duration ) );
  }

  void trigger_enduring_strength()
  {
    p()->buffs.enduring_strength->trigger();
    auto max_duration_extension = p()->talent.frost.enduring_strength->effectN( 4 ).time_value();
    p()->buffs.enduring_strength->extend_duration(
        std::min( p()->talent.frost.enduring_strength->effectN( 2 ).time_value() *
                      p()->buffs.enduring_strength_builder->stack(),
                  max_duration_extension ) );
    p()->buffs.enduring_strength_builder->expire();
  }

private:
  unsigned pillar_extension;
};

// Cryogennic Chamber =======================================================
struct cryogenic_chamber_buff_t final : public death_knight_buff_t
{
  cryogenic_chamber_buff_t( death_knight_t* p, std::string_view n, const spell_data_t* s )
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
    if ( remains > 0_ms )
    {
      cryogenic_chamber_damage->base_dd_min = cryogenic_chamber_damage->base_dd_max = damage;
      cryogenic_chamber_damage->execute();
    }

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
  dark_transformation_buff_t( death_knight_t* p, std::string_view n, const spell_data_t* s )
    : death_knight_buff_t( p, n, s )
  {
    set_default_value_from_effect( 1 );
    set_duration( p->talent.unholy.dark_transformation->duration() );
    cooldown->duration = 0_ms;  // Handled by the player ability
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    death_knight_buff_t::start( stacks, value, duration );

    pets::ghoul_pet_t* ghoul = p()->pets.ghoul_pet.active_pet();

    ghoul->dark_transformation->trigger();

    if ( p()->talent.unholy.ghoulish_frenzy.ok() )
    {
      p()->buffs.ghoulish_frenzy->trigger();
      ghoul->ghoulish_frenzy->trigger();
    }

    if ( p()->talent.sanlayn.gift_of_the_sanlayn.ok() )
      p()->buffs.gift_of_the_sanlayn->trigger();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    death_knight_buff_t::expire_override( expiration_stacks, remaining_duration );

    pets::ghoul_pet_t* ghoul = p()->pets.ghoul_pet.active_pet();

    if ( ghoul != nullptr )
    {
      ghoul->dark_transformation->expire();
      if ( p()->talent.unholy.ghoulish_frenzy.ok() )
        ghoul->ghoulish_frenzy->expire();
    }

    if ( p()->talent.unholy.ghoulish_frenzy.ok() )
      p()->buffs.ghoulish_frenzy->expire();

    if ( p()->talent.sanlayn.gift_of_the_sanlayn.ok() )
      p()->buffs.gift_of_the_sanlayn->expire();

    if ( p()->talent.sanlayn.the_blood_is_life.ok() && p()->pets.blood_beast.active_pet() != nullptr )
    {
      p()->pets.blood_beast.active_pet()->demise();
    }
  }
};

// Runic Corruption =========================================================
struct runic_corruption_buff_t final : public death_knight_buff_t
{
  runic_corruption_buff_t( death_knight_t* p, std::string_view name, const spell_data_t* spell )
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

    return initial_duration * ( p()->bugs ? 1.0 : p()->cache.attack_haste() );
  }
};

// Apocalyptic Conquest =======================================================
struct apocalyptic_conquest_buff_t final : public death_knight_buff_t
{
  apocalyptic_conquest_buff_t( death_knight_t* p, std::string_view name, const spell_data_t* spell )
    : death_knight_buff_t( p, name, spell ), nazgrims_conquest( 0 )
  {
    set_default_value( p->pet_spell.apocalyptic_conquest->effectN( 1 ).percent() );
    add_invalidate( CACHE_STRENGTH );
    set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );
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

// Gift of the San'layn ===================================================
struct gift_of_the_sanlayn_buff_t final : public death_knight_buff_t
{
  gift_of_the_sanlayn_buff_t( death_knight_t* p, std::string_view name, const spell_data_t* spell )
    : death_knight_buff_t( p, name, spell )
  {
    unsigned idx = p->specialization() == DEATH_KNIGHT_BLOOD ? 4 : 1;
    set_default_value_from_effect( idx );
    set_duration( 0_ms );  // Handled by DT and VB
    add_invalidate( CACHE_HASTE );
    if ( p->talent.sanlayn.thrill_of_blood )
      add_invalidate( CACHE_MASTERY );
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
};

// Essence of the Blood Queen ===============================================
struct essence_of_the_blood_queen_buff_t final : public death_knight_buff_t
{
  essence_of_the_blood_queen_buff_t( death_knight_t* p, std::string_view name, const spell_data_t* spell )
    : death_knight_buff_t( p, name, spell )
  {
    set_pct_buff_type( STAT_PCT_BUFF_MASTERY );
    add_invalidate( CACHE_MASTERY );
    size_t idx = p->specialization() == DEATH_KNIGHT_UNHOLY ? 1 : 2;
    set_default_value( p->talent.sanlayn.thrill_of_blood->effectN( idx ).base_value() / 10 );
  }

  // Override the value of the buff to properly capture Essence of the Blood Queens's buff behavior
  double value() override
  {
    double v = default_value;

    v *= 1.0 + p()->buffs.gift_of_the_sanlayn->check_value();

    return v;
  }

  double check_value() const override
  {
    double v = default_value;

    v *= 1.0 + p()->buffs.gift_of_the_sanlayn->check_value();

    return v;
  }
};

// Death and Decay ==========================================================
struct death_and_decay_buff_t : public death_knight_buff_t
{
  death_and_decay_buff_t( death_knight_t* p, std::string_view name, const spell_data_t* spell )
    : death_knight_buff_t( p, name, spell )
  {
    set_duration( 0_ms );  // Handled by things that trigger this buff.
    // Specifically use a stack change callback here due to when its called in buff_t::expire
    set_stack_change_callback( [ &, p ]( buff_t*, int, int new_ ) {
      if ( new_ == 0 && p->in_death_and_decay() )
        trigger();
    } );
  }

  void trigger_buffs()
  {
    if ( !p()->talent.blood.sanguine_ground.ok() && !p()->talent.sanlayn.bloodsoaked_ground.ok() )
      return;

    if ( p()->talent.blood.sanguine_ground.ok() && !p()->buffs.sanguine_ground->check() )
      p()->buffs.sanguine_ground->trigger();

    if ( p()->talent.sanlayn.bloodsoaked_ground.ok() && !p()->buffs.bloodsoaked_ground->check() )
      p()->buffs.bloodsoaked_ground->trigger();
  }

  void expire_buffs()
  {
    if ( !p()->talent.blood.sanguine_ground.ok() && !p()->talent.sanlayn.bloodsoaked_ground.ok() )
      return;

    if ( p()->talent.blood.sanguine_ground.ok() && p()->buffs.sanguine_ground->check() )
      p()->buffs.sanguine_ground->expire();

    if ( p()->talent.sanlayn.bloodsoaked_ground.ok() && p()->buffs.bloodsoaked_ground->check() )
      p()->buffs.bloodsoaked_ground->expire();
  }

  void start( int s, double v, timespan_t d ) override
  {
    death_knight_buff_t::start( s, v, d );
    trigger_buffs();
  }

  void expire_override( int s, timespan_t d ) override
  {
    death_knight_buff_t::expire_override( s, d );
    expire_buffs();
  }
};

// Chosen of Frostbrood Haste ===============================================
struct chosen_of_frostbrood_haste_buff_t : public death_knight_buff_t
{
  chosen_of_frostbrood_haste_buff_t( death_knight_t* p, std::string_view name, const spell_data_t* s )
    : death_knight_buff_t( p, name, s )
  {
    set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    add_invalidate( CACHE_HASTE );
    set_name_reporting( "Haste" );
  }

  double value() override
  {
    return default_value;
  }

  double check_value() const override
  {
    return default_value;
  }
};

// Anti-magic Shell =========================================================
struct ams_parent_buff_t : public death_knight_buff_base_t<absorb_buff_t>
{
  ams_parent_buff_t( death_knight_t* p, std::string_view name, const spell_data_t* spell, bool horsemen )
    : death_knight_buff_base_t<absorb_buff_t>( p, name, spell ),
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
    death_knight_buff_base_t<absorb_buff_t>::start( stacks, calc_absorb(), duration );

    if ( option > 0 )
    {
      double ticks = buff_duration() / tick_time();
      double pct   = p()->rng().gauss_ab( option, 0.3, 0.01, 1.0 ) / ticks;
      damage       = calc_absorb() * pct;
    }
  };

  void absorb_used( double absorbed, player_t* source ) override
  {
    death_knight_buff_base_t<absorb_buff_t>::absorb_used( absorbed, source );
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

    if ( horsemen )
    {
      max_absorb *= p()->talent.rider.horsemens_aid->effectN( 1 ).percent();
    }

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
  antimagic_shell_buff_t( death_knight_t* p, std::string_view name, const spell_data_t* spell )
    : ams_parent_buff_t( p, name, spell, false )
  {
    set_absorb_source( p->get_stats( "antimagic_shell" ) );
  }
};

struct antimagic_shell_buff_horseman_t : public ams_parent_buff_t
{
  antimagic_shell_buff_horseman_t( death_knight_t* p, std::string_view name, const spell_data_t* spell )
    : ams_parent_buff_t( p, name, spell, true )
  {
    set_absorb_source( p->get_stats( "antimagic_shell_horseman" ) );
  }
};

// Anti-magic Zone =========================================================
struct antimagic_zone_buff_t final : public death_knight_buff_base_t<absorb_buff_t>
{
  antimagic_zone_buff_t( death_knight_t* p, std::string_view name, const spell_data_t* spell )
    : death_knight_buff_base_t<absorb_buff_t>( p, name, spell )
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
    return death_knight_buff_base_t<absorb_buff_t>::consume( actual_consumed, s );
  }

  void start( int stacks, double, timespan_t duration ) override
  {
    death_knight_buff_base_t<absorb_buff_t>::start( stacks, calc_absorb(), duration );
  };

  double calc_absorb()
  {
    // HP Value doesnt appear in spell data, instead stored in a variable in spell ID 51052
    double max_absorb = p()->resources.max[ RESOURCE_HEALTH ] * 1.5;

    max_absorb *= 1.0 + p()->cache.heal_versatility();

    return max_absorb;
  }
};
}  // namespace buffs

namespace debuffs
{
using death_knight_debuff_t = death_knight_buff_base_t<buff_t>;

}  // namespace debuffs

// ==========================================================================
// Death Knight Attacks
// ==========================================================================

// Melee Attack =============================================================
struct melee_t : public death_knight_melee_attack_t
{
  melee_t( const char* name, death_knight_t* p, int sw )
    : death_knight_melee_attack_t( name, p ), sync_weapons( sw ), first( true ), idt_chance( 0 )
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

    if ( p->talent.frost.icy_death_torrent.ok() )
    {
      idt_chance = ( p->main_hand_weapon.group() == WEAPON_2H
                         ? 1.0
                         : 1.0 / ( p->talent.frost.icy_death_torrent->effectN( 1 ).base_value() * 0.1 ) ) *
                   p->talent.frost.icy_death_torrent->proc_chance();
    }

    p->apply_action_effects( this );
    p->apply_target_action_effects( this );

    // Dual wielders have a -19% chance to hit on melee attacks
    if ( p->dual_wield() )
      base_hit -= 0.19;
  }

  void reset() override
  {
    death_knight_melee_attack_t::reset();

    first = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = death_knight_melee_attack_t::execute_time();

    if ( first && !sync_weapons )
    {
      timespan_t delay = ( weapon->slot == SLOT_OFF_HAND ) ? p()->rng().range( 10_ms, t * 0.5 ) : 0_ms;
      return delay;
    }
    else
      return t;
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

    if ( result_is_hit( s->result ) )
    {
      if ( p()->talent.permafrost.ok() )
      {
        p()->background_actions.permafrost->execute_on_target(
            p(), s->result_amount * p()->talent.permafrost->effectN( 1 ).percent() );
      }

      if ( s->result == RESULT_CRIT )
      {
        if ( p()->talent.frost.killing_machine.ok() && p()->pseudo_random.killing_machine->trigger() )
          p()->trigger_killing_machine( false, p()->procs.km_from_crit_aa, p()->procs.km_from_crit_aa_wasted );

        // TODO: verify proc rate close to launch, as of build 55288 it is 100% for 2h and 50% for dw
        if ( p()->talent.frost.icy_death_torrent.ok() && rng().roll( idt_chance ) )
          p()->background_actions.icy_death_torrent_damage->execute();

        if ( p()->talent.frost.the_long_winter.ok() && p()->buffs.pillar_of_frost->check() )
        {
          debug_cast<buffs::pillar_of_frost_buff_t*>( p()->buffs.pillar_of_frost )->extend_pillar();
        }
      }

      // Crimson scourge doesn't proc if death and decay is ticking
      if ( get_td( s->target )->dot.blood_plague->is_ticking() && p()->active_dnds.empty() )
      {
        if ( p()->specialization() == DEATH_KNIGHT_BLOOD && p()->buffs.crimson_scourge->trigger() )
        {
          p()->cooldown.death_and_decay_dynamic->reset( true );
        }
      }
    }
  }

private:
  int sync_weapons;
  bool first;
  double idt_chance;
};

// Auto Attack ==============================================================

struct auto_attack_t final : public death_knight_melee_attack_t
{
  int sync_weapons;

  auto_attack_t( death_knight_t* p, std::string_view options_str )
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
// Common diseases code

struct death_knight_disease_t : public death_knight_spell_t
{
  death_knight_disease_t( std::string_view n, death_knight_t* p, const spell_data_t* s )
    : death_knight_spell_t( n, p, s ), inevitable_mult( 0 ), inevitable_idx( 0 )
  {
    background = true;
    may_miss = hasted_ticks = false;
    if ( p->talent.sanlayn.inevitable.ok() )
    {
      inevitable_idx    = p->specialization() == DEATH_KNIGHT_BLOOD ? 2 : 1;
      inevitable_mult = p->talent.sanlayn.inevitable->effectN( inevitable_idx ).percent() / 90.0;
    }
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

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double mult = death_knight_spell_t::composite_ta_multiplier( s );

    if ( p()->talent.sanlayn.inevitable.ok() )
      mult *= 1.0 + ( std::min( inevitable_mult * ( 100.0 - s->target->health_percentage() ),
                                p()->talent.sanlayn.inevitable->effectN( inevitable_idx ).percent() ) );

    return mult;
  }

private:
  double inevitable_mult;
  unsigned inevitable_idx;
};

// Blood Plague ============================================
struct blood_plague_heal_t final : public death_knight_leech_damage_heal_t
{
  blood_plague_heal_t( std::string_view name, death_knight_t* p )
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
  blood_plague_t( std::string_view name, death_knight_t* p )
    : death_knight_disease_t( name, p, p->spell.blood_plague )
  {
    heal = get_action<blood_plague_heal_t>( "blood_plague_heal", p );
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

    // Plague Infusion: Reduce Blood Boil CD on crit
    if ( d->state->result == RESULT_CRIT && p()->talent.blood.plague_infusion.ok() )
    {
      if ( p()->cooldown.plague_infusion_icd->up() )
      {
        p()->cooldown.blood_boil->adjust( p()->talent.blood.plague_infusion->effectN( 1 ).time_value() );
        p()->cooldown.plague_infusion_icd->start();
      }
    }
  }

private:
  propagate_const<action_t*> heal;
};

// Dread Plague ======================================================
struct dread_plague_death_t final : public death_knight_spell_t
{
  dread_plague_death_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.dread_plague_death_damage )
  {
    background = true;
    aoe        = -1;
    may_miss = may_dodge = may_parry = false;
  }
};

struct dread_plague_erupt_t final : public death_knight_spell_t
{
  dread_plague_erupt_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.dread_plague_erupt )
  {
    background = true;
    may_miss = may_dodge = may_parry = false;
  }
};

struct dread_plague_t final : public death_knight_disease_t
{
  dread_plague_t( std::string_view name, death_knight_t* p )
    : death_knight_disease_t( name, p, p->spell.dread_plague ), last_target( nullptr ), erupt( nullptr )
  {
    add_child( p->background_actions.dread_plague_death );

    if ( p->talent.unholy.forbidden_knowledge_3.ok() )
    {
      p->pets.lesser_ghoul_fk.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    }

    if ( p->talent.unholy.superstrain.ok() )
    {
      erupt                         = get_action<dread_plague_erupt_t>( "dread_plague_erupt_ss", p );
      erupt->target_filter_callback = secondary_targets_only();
      if ( !p->options.wcl_reporting_mode )
        add_child( erupt );
    }

    if ( p->options.wcl_reporting_mode )
      add_child( p->background_actions.dread_plague_erupt );
  }

  void impact( action_state_t* s ) override
  {
    death_knight_disease_t::impact( s );
    if ( !last_target )
      last_target = s->target;

    if ( s->target != last_target )
    {
      death_knight_td_t* td = get_td( last_target );
      td->dot.dread_plague->cancel();
    }

    last_target = s->target;
  }

  void tick( dot_t* d ) override
  {
    death_knight_disease_t::tick( d );

    if ( p()->talent.unholy.sudden_doom.ok() && p()->pseudo_random.sudden_doom->trigger() )
      p()->buffs.sudden_doom->trigger();

    // Proc rate testing shows this at ~50% chance with limited testing. Assuming effect 2 divided by 10 is the chance
    // for the time being. TODO: Test for longer... far longer
    if ( p()->talent.unholy.superstrain.ok() )
    {
      if ( rng().roll( p()->spell.superstrain_energize->effectN( 2 ).base_value() / 10 ) )
      {
        p()->resource_gain( RESOURCE_RUNIC_POWER,
                            p()->spell.superstrain_energize->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
                            p()->gains.superstrain, this );
      }
      if ( p()->sim->target_non_sleeping_list.size() > 1 )
      {
        auto target = rng().range( erupt->target_list() );
        erupt->execute_on_target( target,
                                  d->state->result_raw * p()->talent.unholy.superstrain->effectN( 2 ).percent() );
      }
    }

    if ( p()->talent.unholy.forbidden_knowledge_3.ok() && p()->pseudo_random.forbidden_knowledge->trigger() )
      p()->pet_summon.fk_ghoul->execute();
  }

private:
  player_t* last_target;
  action_t* erupt;
};

// Frost Fever =======================================================
struct frost_fever_t final : public death_knight_disease_t
{
  frost_fever_t( std::string_view name, death_knight_t* p )
    : death_knight_disease_t( name, p, p->spell.frost_fever ),
      rp_generation(
          as<int>( p->spec.frost_fever->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) ) )
  {
    ap_type = attack_power_type::WEAPON_BOTH;

    if ( p->main_hand_weapon.group() == WEAPON_2H )
    {
      ap_type = attack_power_type::WEAPON_MAINHAND;
      // There's a 0.98 modifier hardcoded in the tooltip if a 2H weapon is equipped, probably server side magic
      base_multiplier *= 0.98;
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
  }

private:
  int rp_generation;
};

// Infected Claws =====================================================
struct infected_claws_t final : public death_knight_spell_t
{
  infected_claws_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.infected_claws_dot )
  {
    background = true;
  }
};

// Virulent Plague ====================================================
struct virulent_plague_erupt_t final : public death_knight_spell_t
{
  virulent_plague_erupt_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.virulent_plague_erupt )
  {
    background = true;
    may_miss = may_dodge = may_parry = false;
  }
};

struct virulent_plague_t final : public death_knight_disease_t
{
  virulent_plague_t( std::string_view name, death_knight_t* p )
    : death_knight_disease_t( name, p, p->spell.virulent_plague )
  {
    if ( p->options.wcl_reporting_mode )
      add_child( p->background_actions.virulent_plague_erupt );
  }
};

// ==========================================================================
// Rider of the Apocalypse Abilities
// ==========================================================================
struct summon_rider_t : public death_knight_summon_spell_t
{
  summon_rider_t( std::string_view name, death_knight_t* p, const spell_data_t* s, timespan_t duration = 10_s,
                  bool random = true )
    : death_knight_summon_spell_t( name, p, s ), random( random )
  {
    background     = true;
    this->duration = duration;
  }

public:
  bool random;
};

struct undeath_dot_t final : public death_knight_spell_t
{
  undeath_dot_t( std::string_view name, death_knight_t* p ) : death_knight_spell_t( name, p, p->pet_spell.undeath_dot )
  {
    background = true;
    may_miss = may_dodge = may_parry = false;
    dot_behavior                     = DOT_NONE;
  }

  void tick( dot_t* d ) override
  {
    death_knight_spell_t::tick( d );
    auto td = p()->get_target_data( d->target );
    td->dot.undeath->increment( 1 );
  }
};

struct trollbanes_icy_fury_t final : public death_knight_spell_t
{
  trollbanes_icy_fury_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->pet_spell.trollbanes_icy_fury_ability )
  {
    background          = true;
    aoe                 = -1;
    reduced_aoe_targets = as<int>( p->talent.rider.trollbanes_icy_fury->effectN( 1 ).base_value() );
  }
};

struct summon_whitemane_t final : public summon_rider_t
{
  summon_whitemane_t( std::string_view name, death_knight_t* p ) : summon_rider_t( name, p, p->spell.summon_whitemane )
  {
    p->pets.whitemane.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    add_child( get_action<undeath_dot_t>( "undeath", p ) );
  }

  void execute() override
  {
    if ( p()->pets.whitemane.active_pet() == nullptr )
    {
      summon_rider_t::execute();
      p()->pets.whitemane.spawn( duration );
    }
    else if ( !random )
    {
      summon_rider_t::execute();
      p()->pets.whitemane.active_pet()->adjust_duration( duration );
    }
  }
};

struct summon_trollbane_t final : public summon_rider_t
{
  summon_trollbane_t( std::string_view name, death_knight_t* p ) : summon_rider_t( name, p, p->spell.summon_trollbane )
  {
    p->pets.trollbane.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
    add_child( get_action<trollbanes_icy_fury_t>( "trollbanes_icy_fury", p ) );
  }

  void execute() override
  {
    if ( p()->pets.trollbane.active_pet() == nullptr )
    {
      summon_rider_t::execute();
      p()->pets.trollbane.spawn( duration );
    }
    else if ( !random )
    {
      summon_rider_t::execute();
      p()->pets.trollbane.active_pet()->adjust_duration( duration );
    }
  }
};

struct summon_nazgrim_t final : public summon_rider_t
{
  summon_nazgrim_t( std::string_view name, death_knight_t* p ) : summon_rider_t( name, p, p->spell.summon_nazgrim )
  {
    p->pets.nazgrim.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
  }

  void execute() override
  {
    if ( p()->pets.nazgrim.active_pet() == nullptr )
    {
      summon_rider_t::execute();
      p()->pets.nazgrim.spawn( duration );
    }
    else if ( !random )
    {
      summon_rider_t::execute();
      p()->pets.nazgrim.active_pet()->adjust_duration( duration );
    }
  }
};

struct summon_mograine_t final : public summon_rider_t
{
  summon_mograine_t( std::string_view name, death_knight_t* p ) : summon_rider_t( name, p, p->spell.summon_mograine )
  {
    p->pets.mograine.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
  }

  void execute() override
  {
    if ( p()->pets.mograine.active_pet() == nullptr )
    {
      summon_rider_t::execute();
      p()->pets.mograine.spawn( duration );
    }
    else if ( !random )
    {
      summon_rider_t::execute();
      pets::mograine_pet_t* mograine = p()->pets.mograine.active_pet();
      mograine->adjust_duration( duration );
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
  vampiric_strike_heal_t( std::string_view name, death_knight_t* p )
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
  infliction_in_sorrow_t( std::string_view n, death_knight_t* p )
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
  exterminate_aoe_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.exterminate_aoe )
  {
    background              = true;
    cooldown->duration      = 0_ms;
    aoe                     = -1;
    reduced_aoe_targets     = p->talent.deathbringer.exterminate->effectN( 5 ).base_value();
    const int effect_idx    = p->specialization() == DEATH_KNIGHT_FROST ? 2 : 1;
    attack_power_mod.direct = data().effectN( effect_idx ).ap_coeff();

    if ( p->talent.deathbringer.wither_away->ok() )
    {
      if ( p->specialization() == DEATH_KNIGHT_FROST )
      {
        impact_action = get_action<frost_fever_t>( "frost_fever", p );
      }
      if ( p->specialization() == DEATH_KNIGHT_BLOOD )
      {
        impact_action = get_action<blood_plague_t>( "blood_plague", p );
      }
    }
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( p()->specialization() == DEATH_KNIGHT_BLOOD && p()->talent.permafrost.ok() && p()->talent.deathbringer.frigid_resolve.ok() )
    {
      p()->background_actions.permafrost->execute_on_target(
            p(), state->result_amount * p()->talent.deathbringer.frigid_resolve->effectN( 3 ).percent() );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    if ( empowered )
      empowered--;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( state );
    if ( empowered )
    {
      const double multiplier_effect = p()->specialization() == DEATH_KNIGHT_FROST
                                           ? p()->talent.deathbringer.echoing_fury->effectN( 3 ).percent()
                                           : p()->talent.deathbringer.echoing_fury->effectN( 6 ).percent();
      m *= multiplier_effect;
    }
    return m;
  }

public:
  int empowered;
};

struct exterminate_t final : public death_knight_spell_t
{
  exterminate_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.exterminate_damage ),
      second_hit( get_action<exterminate_aoe_t>( name_str + "_second_hit", p ) )
  {
    background              = true;
    cooldown->duration      = 0_ms;
    const int effect_idx    = p->specialization() == DEATH_KNIGHT_FROST ? 2 : 1;
    attack_power_mod.direct = data().effectN( effect_idx ).ap_coeff();

    add_child( second_hit );
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( p()->specialization() == DEATH_KNIGHT_BLOOD && p()->talent.permafrost.ok() && p()->talent.deathbringer.frigid_resolve.ok() )
    {
      p()->background_actions.permafrost->execute_on_target(
            p(), state->result_amount * p()->talent.deathbringer.frigid_resolve->effectN( 3 ).percent() );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p()->specialization() == DEATH_KNIGHT_FROST )
    {
      p()->trigger_killing_machine( true, p()->procs.km_from_exterminate, p()->procs.km_from_exterminate_wasted );
    }
    if ( empowered )
    {
      empowered--;
    }

    make_event<delayed_execute_event_t>( *sim, p(), second_hit, execute_state->target, 500_ms );
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( state );
    if ( empowered )
    {
      const double multiplier_effect = p()->specialization() == DEATH_KNIGHT_FROST
                                           ? p()->talent.deathbringer.echoing_fury->effectN( 2 ).percent()
                                           : p()->talent.deathbringer.echoing_fury->effectN( 5 ).percent();
      m *= multiplier_effect;
    }
    return m;
  }

public:
  int empowered;
  action_t* second_hit;
};

struct reapers_mark_explosion_t final : public death_knight_spell_t
{
  reapers_mark_explosion_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.reapers_mark_explosion ),
      stacks( 0 ),
      soul_rupture_effect_idx( p->specialization() == DEATH_KNIGHT_FROST ? 2 : 1 ),
      mod( 0.0 ),
      grim_reaper_max( 0 ),
      grim_reaper_threshold( 0 ),
      exterminate_stacks( as<int>( p->talent.deathbringer.exterminate->effectN( 3 ).base_value() ) )
  {
    background              = true;
    cooldown->duration      = 0_ms;
    attack_power_mod.direct = data().effectN( soul_rupture_effect_idx ).ap_coeff();
    stacks                  = 0;
    if ( p->talent.deathbringer.soul_rupture.ok() )
      mod = p->talent.deathbringer.soul_rupture->effectN( soul_rupture_effect_idx ).percent();
    if ( p->talent.deathbringer.grim_reaper.ok() )
    {
      grim_reaper_max       = p->talent.deathbringer.grim_reaper->effectN( 1 ).percent();
      grim_reaper_threshold = p->talent.deathbringer.grim_reaper->effectN( 2 ).base_value();
    }
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
      double missing_hp  = ( 1.0 - target->health_percentage() * 0.01 );
      double buff_amount = missing_hp * grim_reaper_max;
      m *= 1.0 + buff_amount;
    }

    return m;
  }

  void execute_wrapper( player_t* target, int stacks )
  {
    this->stacks = stacks;
    execute_on_target( target );

    if ( target != nullptr && p()->talent.deathbringer.exterminate->ok() )
    {
      p()->buffs.exterminate->trigger( exterminate_stacks );
    }

  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );
    if ( p()->talent.deathbringer.soul_rupture.ok() )
      p()->background_actions.soul_rupture->execute_on_target( state->target, state->result_amount * mod );
  }

private:
  int stacks;
  int soul_rupture_effect_idx;
  double mod;
  double grim_reaper_max;
  double grim_reaper_threshold;
  int exterminate_stacks;
};

struct wave_of_souls_t final : public death_knight_spell_t
{
  wave_of_souls_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.wave_of_souls_damage ),
      second_wave( false ),
      swift_and_painful_val( 0.0 ),
      swift_and_painful_mod( 0.0 )
  {
    background              = true;
    cooldown->duration      = 0_ms;
    aoe                     = -1;
    const int effect_idx    = p->specialization() == DEATH_KNIGHT_FROST ? 2 : 1;
    attack_power_mod.direct = data().effectN( effect_idx ).ap_coeff();

    if ( p->talent.deathbringer.swift_and_painful.ok() )
    {
      swift_and_painful_mod = p->talent.deathbringer.swift_and_painful->effectN( 2 ).percent();
      swift_and_painful_val = ( 1.0 + swift_and_painful_mod ) * p->spell.wave_of_souls_debuff->effectN( 1 ).percent();
    }
  }

  double composite_crit_chance() const override
  {
    double c = death_knight_spell_t::composite_crit_chance();
    if ( second_wave )
    {
      c = 1.0;
    }
    return c;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );
    if ( state->result == RESULT_CRIT )
    {
      if ( state->chain_target == 0 && p()->talent.deathbringer.swift_and_painful->ok() )
      {
        get_td( state->target )->debuff.wave_of_souls->trigger( 1, swift_and_painful_val );
      }
      else
      {
        get_td( state->target )->debuff.wave_of_souls->trigger();
      }
    }
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( state );

    if ( state->chain_target == 0 )
    {
      m *= 1.0 + swift_and_painful_mod;
    }

    return m;
  }

public:
  bool second_wave;

private:
  double swift_and_painful_val;
  double swift_and_painful_mod;
};

struct soul_rupture_t final : public death_knight_spell_t
{
  soul_rupture_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.soul_rupture_damage )
  {
    background             = true;
    cooldown->duration     = 0_ms;
    aoe                    = -1;
    target_filter_callback = secondary_targets_only();
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    if ( p()->talent.deathbringer.swift_and_painful->ok() && target_list().empty() )
    {
      p()->buffs.swift_and_painful->trigger();
    }
  }
};

struct reapers_mark_t final : public death_knight_spell_t
{
  reapers_mark_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "reapers_mark", p, p->talent.deathbringer.reapers_mark )
  {
    parse_options( options_str );
    const int effect_idx    = p->specialization() == DEATH_KNIGHT_FROST ? 4 : 1;
    attack_power_mod.direct = data().effectN( effect_idx ).ap_coeff();

    if ( p->talent.deathbringer.reapers_mark.ok() )
    {
      add_child( p->background_actions.reapers_mark_explosion );
    }
    if ( p->talent.deathbringer.wave_of_souls.ok() )
    {
      add_child( p->background_actions.wave_of_souls );
    }
    if ( p->talent.deathbringer.soul_rupture.ok() )
    {
      add_child( p->background_actions.soul_rupture );
    }
    if ( p->talent.deathbringer.exterminate.ok() || p->talent.deathbringer.echoing_fury.ok() )
    {
      add_child( p->background_actions.exterminate );
    }
    if ( p->talent.deathbringer.echoing_fury.ok() )
      exterm_stacks = as<int>( p->talent.deathbringer.echoing_fury->effectN( 1 ).base_value() );
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

    if ( p()->talent.deathbringer.grim_reaper.ok() )
    {
      if ( p()->specialization() == DEATH_KNIGHT_FROST )
      {
        p()->trigger_killing_machine( true, p()->procs.km_from_grim_reaper, p()->procs.km_from_grim_reaper_wasted );
      }
      else
      {
        p()->buffs.bone_shield->trigger( as<int>( p()->talent.deathbringer.grim_reaper->effectN( 3 ).base_value() ) );
      }
    }
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
        p()->background_actions.wave_of_souls->execute();
        timespan_t second    = rng().gauss<1000, 200>();
        wave_of_souls_t* wos = debug_cast<wave_of_souls_t*>( p()->background_actions.wave_of_souls );
        wos->second_wave     = true;
        make_repeating_event(
            *sim, second,
            [ this, wos ]() {
              p()->background_actions.wave_of_souls->execute();
              wos->second_wave = false;
            },
            1 );
      } );
    }

    if ( p()->talent.deathbringer.deathly_blows.ok() )
      p()->buffs.bonegrinder_crit->trigger(
          ( as<int>( p()->talent.deathbringer.deathly_blows->effectN( 4 ).base_value() ) ) );

    if ( p()->specialization() == DEATH_KNIGHT_FROST && p()->talent.deathbringer.echoing_fury.ok() )
    {
      p()->buffs.exterminate->trigger( exterm_stacks );
      debug_cast<exterminate_t*>( p()->background_actions.exterminate )->empowered         = exterm_stacks;
      debug_cast<exterminate_aoe_t*>( p()->background_actions.exterminate_aoe )->empowered = exterm_stacks;
    }

    // Tested and confirmed June 11th 2026.  Casting reapers mark gives 1 stack on blood, but should only apply to Frost
    if ( p()->bugs && p()->specialization() == DEATH_KNIGHT_BLOOD && p()->talent.deathbringer.echoing_fury.ok() )
    {
      p()->buffs.exterminate->trigger( exterm_stacks );
      debug_cast<exterminate_t*>( p()->background_actions.exterminate )->empowered         = exterm_stacks;
      debug_cast<exterminate_aoe_t*>( p()->background_actions.exterminate_aoe )->empowered = exterm_stacks;
    }  
  }

private:
  int exterm_stacks;
};

// ==========================================================================
// Death Knight Abilities
// ==========================================================================

// Abomination Limb =========================================================
struct abomination_limb_t : public death_knight_spell_t
{
  abomination_limb_t( death_knight_t* p, std::string_view options_str = "" )
    : death_knight_spell_t( "abomination_limb", p, p->talent.blood.abomination_limb )
  {
    harmful = false;
    parse_options( options_str );
  }

  void tick( dot_t* dot ) override
  {
    death_knight_spell_t::tick( dot );

    for ( auto& target : p()->sim->target_non_sleeping_list )
    {
      auto td = get_td( target );
      if ( !td->debuff.abomination_limb->check() )
      {
        td->debuff.tightening_grasp->trigger();
        td->debuff.abomination_limb->trigger();
        return;
      }
    }

    if ( p()->talent.blood.bone_collector.ok() )
      p()->buffs.bone_shield->trigger();
  }
};

// Dark Transformation ======================================================
// Blightfall
struct blightfall_t final : public death_knight_spell_t
{
  blightfall_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.blightfall ), damage_mult( 1.0 ), duration_mult( 1.0 )
  {
    aoe                    = -1;
    damage_mult            = p->talent.unholy.blightfall->effectN( 2 ).percent();
    duration_mult          = p->talent.unholy.blightfall->effectN( 1 ).percent();
    target_filter_callback = unholy_diseases_only();

    if ( !p->options.wcl_reporting_mode )
    {
      add_child( p->background_actions.virulent_plague_erupt_pest );
      add_child( p->background_actions.dread_plague_erupt_pest );
    }
  }

  void init_finished() override
  {
    death_knight_spell_t::init_finished();
    cooldown->set_max_charges( 1 );
    cooldown->duration = 0_ms;
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    p()->cooldown.putrefy->reset( false );
    p()->buffs.blightfall->expire();
  }

  void consume_dot( dot_t* dot, action_t* erupt )
  {
    if ( !dot->is_ticking() )
      return;

    double damage = 0.0;
    if ( duration_mult == 1.0 )
      damage = dot->tick_damage_over_remaining_time() * damage_mult;
    else
      damage = dot->tick_damage_over_time( dot->remains() * duration_mult ) * damage_mult;

    erupt->execute_on_target( dot->target, damage );
    dot->cancel();
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );
    death_knight_td_t* td = get_td( s->target );
    dot_t* vp             = td->dot.virulent_plague;
    dot_t* dp             = td->dot.dread_plague;

    if ( vp->is_ticking() )
    {
      if ( p()->options.extra_unholy_reporting )
        p()->sample_data.blightfall_vp_dur->add( vp->remains().total_seconds() );

      consume_dot( vp, p()->options.wcl_reporting_mode ? p()->background_actions.virulent_plague_erupt
                                                       : p()->background_actions.virulent_plague_erupt_pest );
    }
    if ( dp->is_ticking() )
    {
      if ( p()->options.extra_unholy_reporting )
        p()->sample_data.blightfall_dp_dur->add( dp->remains().total_seconds() );

      consume_dot( dp, p()->options.wcl_reporting_mode ? p()->background_actions.dread_plague_erupt
                                                       : p()->background_actions.dread_plague_erupt_pest );
    }
  }

private:
  double damage_mult;
  double duration_mult;
};

struct dark_transformation_t : public death_knight_spell_t
{
  dark_transformation_t( std::string_view n, death_knight_t* p, std::string_view options_str = "" )
    : death_knight_spell_t( n, p, p->talent.unholy.dark_transformation ), summon_magus( nullptr )
  {
    harmful = false;
    target  = p;
    parse_options( options_str );

    if ( p->talent.unholy.magus_of_the_dead.ok() )
    {
      summon_magus = get_action<summon_magus_t>( "dt_magus", p, p->spell.summon_magus,
                                                 magus_of_the_dead_e::MAGUS_DARK_TRANSFORMATION );
      p->pets.dt_magus.set_creation_event_callback( pets::parent_pet_action_fn( summon_magus ) );
      add_child( summon_magus );
    }

    if ( p->talent.sanlayn.the_blood_is_life.ok() )
    {
      p->pets.blood_beast.set_creation_event_callback( pets::parent_pet_action_fn( p->pet_summon.blood_beast ) );
      add_child( p->pet_summon.blood_beast );
      p->pet_summon.blood_beast->add_child( p->background_actions.the_blood_is_life );
    }

    if ( p->talent.unholy.blightfall.ok() )
      set_replacement_action( new blightfall_t( "blightfall", p ), p->buffs.blightfall );

    if ( !p->talent.unholy.blightfall.ok() )
      trigger_gcd = 0_ms;  // in data as 1.5s, only triggers this gcd if blightfall is talented.
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    if ( was_replaced )
      return;

    p()->pets.ghoul_pet.active_pet()->resource_gain( RESOURCE_ENERGY,
                                                     p()->talent.unholy.dark_transformation->effectN( 4 ).resource(),
                                                     p()->pets.ghoul_pet.active_pet()->dark_transformation_gain, this );

    p()->buffs.dark_transformation->trigger();

    if ( p()->talent.unholy.commander_of_the_dead.ok() )
      p()->buffs.commander_of_the_dead->trigger();

    if ( p()->talent.sanlayn.the_blood_is_life.ok() )
      p()->pet_summon.blood_beast->execute();

    if ( p()->talent.unholy.magus_of_the_dead.ok() )
      summon_magus->execute();

    if ( p()->talent.rider.ride_or_die.ok() )
    {
      timespan_t dur = timespan_t::from_seconds( p()->talent.rider.ride_or_die->effectN( 1 ).base_value() );
      p()->summon_rider( dur, rider_of_the_apocalypse_e::WHITEMANE );
    }

    if ( p()->talent.unholy.reaping.ok() && p()->talent.unholy.soul_reaper.ok() )
    {
      p()->buffs.reaping->trigger();
      p()->cooldown.soul_reaper->reset( false );
    }

    if ( p()->talent.unholy.blightfall.ok() )
      p()->buffs.blightfall->trigger();

    if ( p()->bugs && p()->talent.sanlayn.inevitable.ok() )
      p()->buffs.clawing_shadows->trigger( p()->buffs.clawing_shadows->max_stack() );
  }

  bool ready() override
  {
    if ( p()->buffs.blightfall->check() )
      return death_knight_spell_t::ready();

    if ( p()->pets.ghoul_pet.active_pet() == nullptr || p()->pets.ghoul_pet.active_pet()->is_sleeping() )
      return false;

    return death_knight_spell_t::ready();
  }

private:
  action_t* summon_magus;
};

// Army of the Dead =========================================================
struct army_of_the_dead_t final : public death_knight_summon_spell_t
{
  struct summon_army_event_t : public event_t
  {
    summon_army_event_t( death_knight_t* dk, int n, timespan_t interval, timespan_t duration )
      : event_t( *dk, interval ),
        n_ghoul( n ),
        max_ghouls( 0 ),
        summon_interval( interval ),
        summon_duration( duration ),
        p( dk ),
        summon_ghoul( dk->pet_summon.army_ghoul )
    {
      max_ghouls = as<int>( dk->talent.unholy.army_of_the_dead->effectN( 1 ).base_value() );
    }

    void execute() override
    {
      summon_lesser_ghoul_t* summon_ghoul_cast = debug_cast<summon_lesser_ghoul_t*>( summon_ghoul );
      summon_ghoul_cast->set_duration( summon_duration );
      summon_ghoul->execute();

      if ( ++n_ghoul < max_ghouls )
        make_event<summon_army_event_t>( sim(), p, n_ghoul, summon_interval, summon_duration );
    }

  private:
    int n_ghoul;
    int max_ghouls;
    timespan_t summon_interval;
    timespan_t summon_duration;
    death_knight_t* p;
    action_t* summon_ghoul;
  };

  army_of_the_dead_t( death_knight_t* p, std::string_view options_str )
    : death_knight_summon_spell_t( "army_of_the_dead", p, p->talent.unholy.army_of_the_dead ),
      precombat_time( 2.0 ),
      summon_duration( p->spell.summon_army_ghoul->duration() ),
      summon_abomination( nullptr ),
      abomination_duration( 0_s ),
      summon_gargoyle( nullptr ),
      gargoyle_duration( 0_s ),
      summon_magus( nullptr ),
      magus_duration( 0_s )
  {
    // disable_aotd=1 can be added to the profile to disable aotd usage, for example for specific dungeon simming

    if ( p->options.disable_aotd )
      background = true;

    // If used during precombat, army is cast around X seconds before the fight begins
    // This is done to save rune regeneration time once the fight starts
    // Default duration is 6, and can be changed by the user

    add_option( opt_float( "precombat_time", precombat_time ) );
    parse_options( options_str );

    harmful = false;
    target  = p;
    if ( p->talent.unholy.army_of_the_dead.ok() )
    {
      p->pets.lesser_ghoul_army.set_creation_event_callback( pets::parent_pet_action_fn( p->pet_summon.army_ghoul ) );
      add_child( p->pet_summon.army_ghoul );
    }
    if ( p->talent.unholy.raise_abomination.ok() )
    {
      summon_abomination = get_action<summon_abomination_t>( "raise_abomination", p );
      p->pets.abomination.set_creation_event_callback( pets::parent_pet_action_fn( summon_abomination ) );
      abomination_duration = p->spell.summon_abomination->duration();
      if( !p->options.wcl_reporting_mode )
        add_child( summon_abomination );
    }
    if ( p->talent.unholy.summon_gargoyle.ok() )
    {
      summon_gargoyle = get_action<summon_gargoyle_t>( "summon_gargoyle", p );
      p->pets.gargoyle.set_creation_event_callback( pets::parent_pet_action_fn( summon_gargoyle ) );
      gargoyle_duration = p->spell.summon_gargoyle->duration();
      if ( !p->options.wcl_reporting_mode )
        add_child( summon_gargoyle );
    }
    if ( p->talent.unholy.magus_of_the_dead.ok() )
    {
      summon_magus = get_action<summon_magus_t>( "magus_of_the_dead", p, p->spell.summon_magus,
                                                 magus_of_the_dead_e::MAGUS_ARMY_OF_THE_DEAD );
      p->pets.army_magus.set_creation_event_callback( pets::parent_pet_action_fn( summon_magus ) );
      magus_duration = p->talent.unholy.magus_of_the_dead->effectN( 1 ).time_value();
      if ( !p->options.wcl_reporting_mode )
        add_child( summon_magus );
    }
  }

  void init_finished() override
  {
    death_knight_summon_spell_t::init_finished();

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
    death_knight_summon_spell_t::execute();

    int n_ghoul                = 0;
    timespan_t abomination_dur = abomination_duration;
    timespan_t gargoyle_dur    = gargoyle_duration;
    timespan_t magus_dur       = magus_duration;

    // There's a 0.5s interval between each ghoul's spawn
    timespan_t const summon_interval = p()->talent.unholy.army_of_the_dead->effectN( 1 ).period();

    if ( !p()->in_combat && precombat_time > 0 )
    {
      // The first pet spawns after the interval timer
      timespan_t duration_penalty = timespan_t::from_seconds( precombat_time ) - summon_interval;
      while ( duration_penalty >= 0_s && n_ghoul < as<int>( data().effectN( 1 ).base_value() ) )
      {
        n_ghoul++;
        // Spawn with a duration penalty, and adjust the spawn/travel delay by the penalty
        auto pet = p()->pets.lesser_ghoul_army
                       .spawn( summon_duration + ( n_ghoul < 3 ? 1_s / n_ghoul : 0_s ) - duration_penalty, 1 )
                       .front();
        pet->precombat_spawn_adjust = duration_penalty;
        pet->precombat_spawn        = true;
        // For each pet, reduce the duration penalty by the 0.5s interval
        duration_penalty -= summon_interval;
      }

      if ( p()->talent.unholy.raise_abomination.ok() )
        abomination_dur -= timespan_t::from_seconds( precombat_time );

      if ( p()->talent.unholy.summon_gargoyle.ok() )
        gargoyle_dur -= timespan_t::from_seconds( precombat_time );

      if ( p()->talent.unholy.magus_of_the_dead.ok() )
        magus_dur -= timespan_t::from_seconds( precombat_time );

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
    if ( n_ghoul < as<int>( data().effectN( 1 ).base_value() ) )
      make_event<summon_army_event_t>( *sim, p(), n_ghoul, summon_interval, summon_duration );

    if ( p()->talent.unholy.raise_abomination.ok() )
    {
      timespan_t abom_summon_time = summon_interval * ( as<int>( data().effectN( 1 ).base_value() ) - n_ghoul - 1 );
      summon_abomination_t* summon_abomination_cast = debug_cast<summon_abomination_t*>( summon_abomination );
      summon_abomination_cast->set_duration( abomination_dur );
      // Abomination spawns with the final ghoul spawn
      make_event( *sim, abom_summon_time, [ this ]() { summon_abomination->execute(); } );
    }

    if ( p()->talent.unholy.summon_gargoyle.ok() )
    {
      timespan_t gargoyle_summon_time = summon_interval * ( as<int>( data().effectN( 1 ).base_value() ) - n_ghoul - 1 );
      summon_gargoyle_t* summon_gargoyle_cast = debug_cast<summon_gargoyle_t*>( summon_gargoyle );
      summon_gargoyle_cast->set_duration( gargoyle_dur );
      // Gargoyle spawns with the final ghoul spawn
      make_event( *sim, gargoyle_summon_time, [ this ]() { summon_gargoyle->execute(); } );
    }

    if ( p()->talent.unholy.magus_of_the_dead.ok() )
    {
      summon_magus_t* summon_magus_cast = debug_cast<summon_magus_t*>( summon_magus );
      summon_magus_cast->set_duration( magus_dur );
      summon_magus->execute();
    }

    if ( p()->talent.rider.apocalypse_now.ok() )
      p()->summon_rider(
          p()->talent.rider.apocalypse_now->effectN( 2 ).time_value() - timespan_t::from_seconds( precombat_time ),
          rider_of_the_apocalypse_e::ALL_RIDERS );

    if ( p()->talent.unholy.forbidden_knowledge_1.ok() )
      p()->buffs.forbidden_knowledge->trigger();
  }

private:
  double precombat_time;
  timespan_t summon_duration;
  action_t* summon_abomination;
  timespan_t abomination_duration;
  action_t* summon_gargoyle;
  timespan_t gargoyle_duration;
  action_t* summon_magus;
  timespan_t magus_duration;
};

// Blood Boil ===============================================================

struct blood_boil_t final : public death_knight_spell_t
{
  blood_boil_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "blood_boil", p, p->talent.blood.blood_boil )
  {
    parse_options( options_str );

    aoe              = -1;
    cooldown->hasted = true;
    impact_action    = get_action<blood_plague_t>( "blood_plague", p );
  }

  blood_boil_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->talent.blood.blood_boil )
    {
      aoe           = -1;
      background    = true;
      cooldown->duration = 0_ms;
      impact_action = get_action<blood_plague_t>( "blood_plague", p );
    }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->trigger_drw_action( DRW_ACTION_BLOOD_BOIL );

    if ( p()->buffs.boiling_point->up() )
    {
      // If boiling point echo is already up, we expire it to force it to fire, and queue up a new one
      // Of note, this means the casting BB, as well as the expired echo BB benefit from the buff
      if ( p()->buffs.boiling_point_echo->up() )
        p()->buffs.boiling_point_echo->expire();
      p()->buffs.boiling_point->expire();
      p()->buffs.boiling_point_echo->trigger();
    }

    if ( p()->sets->has_set_bonus( DEATH_KNIGHT_BLOOD, MID1, B2 ) )
      p()->resource_gain( RESOURCE_RUNIC_POWER, p()->spell.rejuvenating_blood->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ), p()->gains.blood_mid1_2pc, this );
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    p()->buffs.hemostasis->trigger();
  }
};

// Blood Draw =========================================================

struct blood_draw_t final : public death_knight_spell_t
{
  blood_draw_t( std::string_view name, death_knight_t* p ) : death_knight_spell_t( name, p, p->spell.blood_draw_damage )
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

// Blood Mist =========================================================

struct blood_mist_t final : public death_knight_spell_t
{
  blood_mist_t( std::string_view name, death_knight_t* p ) : death_knight_spell_t( name, p, p->spell.blood_mist_damage ),
  rp_gain( as<int>( p->spell.blood_mist_rp_gain->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) ) ),
  rp_gain_cap( as<int>( p->spell.blood_mist_rp_gain->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) * p->spell.blood_mist_buff->effectN( 3 ).base_value() ) ),
  rp_gained( 0 )
  {
    aoe                = -1;
    background         = true;
    reduced_aoe_targets = p->spell.blood_mist_buff->effectN( 4 ).base_value();
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( rp_gained < rp_gain_cap )
    {
      p()->resource_gain( RESOURCE_RUNIC_POWER, rp_gain, p()->gains.blood_mist, this );
      rp_gained += rp_gain;
    }
  }

  void execute() override
  {
    rp_gained = 0;
    death_knight_spell_t::execute();
  }

private:
  int rp_gain;
  int rp_gain_cap;
  int rp_gained;
};

struct bloody_reflection_t : public death_knight_spell_t
{
  bloody_reflection_t( std::string_view name, death_knight_t* p ) : death_knight_spell_t( name, p, p->spell.bloody_reflection_damage )
  {
    background = true;
  }
};

// The Blood is Life ========================================================

struct the_blood_is_life_t : public death_knight_spell_t
{
  the_blood_is_life_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->pet_spell.blood_eruption )
  {
    background          = true;
    aoe                 = -1;
    may_crit            = false;
    reduced_aoe_targets = p->talent.sanlayn.the_blood_is_life->effectN( 3 ).base_value();
  }
};

// Breath of Sindragosa =====================================================
struct breath_of_sindragosa_tick_t final : public death_knight_spell_t
{
  breath_of_sindragosa_tick_t( std::string_view n, death_knight_t* p )
    : death_knight_spell_t( n, p, p->talent.frost.breath_of_sindragosa->effectN( 1 ).trigger() ),
      hyperpyrexia_mod( 0.0 ),
      hyperpyrexia_chance( 0.0 )
  {
    aoe                 = -1;
    background          = true;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;
    ap_type             = attack_power_type::WEAPON_BOTH;

    if ( p->main_hand_weapon.group() == WEAPON_2H )
    {
      ap_type = attack_power_type::WEAPON_MAINHAND;
      // There's a 0.98 modifier hardcoded in the tooltip if a 2H weapon is equipped, probably server side magic
      base_multiplier *= 0.98;
    }

    if ( p->talent.frost.hyperpyrexia.ok() )
    {
      hyperpyrexia_mod    = p->talent.frost.hyperpyrexia->effectN( 1 ).percent();
      hyperpyrexia_chance = p->talent.frost.hyperpyrexia->proc_chance();
    }
  }

  double cost() const override
  {
    return 0;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( p()->talent.frost.hyperpyrexia->ok() && state->result_amount > 0 && p()->rng().roll( hyperpyrexia_chance ) )
    {
      residual_action::trigger( p()->background_actions.hyperpyrexia_damage, state->target,
                                state->result_amount * hyperpyrexia_mod );
    }
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( state );
    
    // Breath of Sindragosa has a hidden modifier to deal -20% to secondary targets
    if ( state->chain_target > 0 )
      m *= 0.8;

    return m;
  }

private:
  double hyperpyrexia_mod;
  double hyperpyrexia_chance;
};

struct breath_of_sindragosa_t final : public death_knight_spell_t
{
  breath_of_sindragosa_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "breath_of_sindragosa", p, p->talent.frost.breath_of_sindragosa ), rune_gen( 0 )
  {
    may_miss = may_dodge = may_parry = false;
    parse_options( options_str );

    if ( p->talent.frost.breath_of_sindragosa.ok() )
    {
      add_child( p->background_actions.breath_of_sindragosa_tick );
    }

    rune_gen = as<unsigned>( p->spell.breath_of_sindragosa_rune_gen->effectN( 1 ).base_value() );
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    p()->buffs.breath_of_sindragosa->trigger();
    p()->cooldown.empower_rune_weapon->reset(
        false, as<int>( p()->spell.breath_of_sindragosa_erw_refund->effectN( 1 ).base_value() ) );

    p()->background_actions.breath_of_sindragosa_tick->execute_on_target( target );
  }

private:
  int rune_gen;
};

// Chains of Ice ============================================================

struct chains_of_ice_t final : public death_knight_spell_t
{
  chains_of_ice_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "chains_of_ice", p, p->spec.chains_of_ice )
  {
    parse_options( options_str );
  }
};

// Consumption ==============================================================

struct consumption_leech_heal_t final : public death_knight_leech_damage_heal_t
{
  consumption_leech_heal_t( std::string_view name, death_knight_t* p )
    : death_knight_leech_damage_heal_t( name, p, p->spell.consumption_leech )
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
  }
};

struct consumption_leech_damage_t final : public death_knight_spell_t
{
  int empower_level;
  consumption_leech_damage_t( std::string_view name, death_knight_t* p )
   : death_knight_spell_t( name, p, p->spell.consumption_leech ),
   empower_level( 0 )
  {
    background = true;

    consumption_leech_heal = get_action<consumption_leech_heal_t>("consumption_leech_heal", p );
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    consumption_leech_heal->base_dd_min = consumption_leech_heal->base_dd_max = state->result_amount;
    consumption_leech_heal->execute();
  }

  void reset() override
  {
    death_knight_spell_t::reset();
    empower_level = 0;
  }
  private:
    action_t* consumption_leech_heal;
};

struct consumption_t final : public death_knight_empowered_charge_spell_t
{
  struct consumption_damage_t : public death_knight_empowered_release_spell_t
  {
    consumption_damage_t( std::string_view name, death_knight_t* p )
      : death_knight_empowered_release_spell_t( name, p, p->spell.consumption_damage ),
      leech_damage_accumulator( 0 ),
      bp_consumption_multi( 0 )
    {
      aoe = -1;
      reduced_aoe_targets = p->spell.consumption_damage->effectN( 3 ).base_value();
      background = false;

      consumption_leech_damage = get_action<consumption_leech_damage_t>("consumption_leech", p );
      add_child( consumption_leech_damage );
    }

    void impact( action_state_t* state ) override
    {
      leech_damage_accumulator = 0;
      death_knight_empowered_release_spell_t::impact( state );

      switch ( empower_value( state ) )
      {
        case EMPOWER_3:
          bp_consumption_multi = p()->talent.blood.consumption->effectN( 4 ).percent();
          break;
        case EMPOWER_2:
          bp_consumption_multi = p()->talent.blood.consumption->effectN( 3 ).percent();
          break;
        case EMPOWER_1:
          bp_consumption_multi = p()->talent.blood.consumption->effectN( 2 ).percent();
          break;
        default:
          bp_consumption_multi = p()->talent.blood.consumption->effectN( 2 ).percent();
      }

      // Player dots
      auto td = get_td( state->target );
      if ( td && td->dot.blood_plague->is_ticking() )
      {
        double leech_damage = td->dot.blood_plague->tick_damage_over_time( td->dot.blood_plague->remains() * bp_consumption_multi );
        leech_damage_accumulator += leech_damage;
        sim->print_debug( "Consumption blood plague consumes {} from {} with caster {}", leech_damage, state->target->name(),  td->dot.blood_plague->source->name() );
        td->dot.blood_plague->adjust_duration(- td->dot.blood_plague->remains() * bp_consumption_multi );
      }
      // DRW dots
      // Grab active DRW is we have one
      auto drw = p()->pets.dancing_rune_weapon_pet.active_pet();
      if ( drw )
      {
        auto drw_dot = drw->get_target_data( state->target )->dot.blood_plague;
        if ( drw_dot && drw_dot->is_ticking() )
        {
          double drw_leech = drw_dot->tick_damage_over_time( drw_dot->remains() * bp_consumption_multi );
          leech_damage_accumulator += drw_leech;
          sim->print_debug( "Consumption blood plague consumes {} from {} with caster {}", drw_leech, state->target->name(), drw_dot->source->name() );
          drw_dot->adjust_duration( -drw_dot->remains() * bp_consumption_multi );
        }
      }

      auto everlasting_bond = p()->pets.everlasting_bond_pet.active_pet();
      if ( everlasting_bond )
      {
        auto drw_dot = everlasting_bond->get_target_data( state->target )->dot.blood_plague;
        if ( drw_dot && drw_dot->is_ticking() )
        {
          double drw_leech = drw_dot->tick_damage_over_time( drw_dot->remains() * bp_consumption_multi );
          leech_damage_accumulator += drw_leech;
          sim->print_debug( "Consumption blood plague consumes {} from {} with caster {}", drw_leech, state->target->name(), drw_dot->source->name() );
          drw_dot->adjust_duration( -drw_dot->remains() * bp_consumption_multi );
        }
      }

      for ( auto& dom : p()->pets.dance_of_midnight_pet.active_pets() )
      {
        auto drw_dot = dom->get_target_data( state->target )->dot.blood_plague;
        if ( drw_dot && drw_dot->is_ticking() )
        {
          double drw_leech = drw_dot->tick_damage_over_time( drw_dot->remains() * bp_consumption_multi );
          leech_damage_accumulator += drw_leech;
          sim->print_debug( "Consumption blood plague consumes {} from {} with caster {}", drw_leech, state->target->name(), drw_dot->source->name() );
          drw_dot->adjust_duration( -drw_dot->remains() * bp_consumption_multi );
        }
      }

      debug_cast<consumption_leech_damage_t*>(consumption_leech_damage)->empower_level = empower_value( state );
      consumption_leech_damage->base_dd_min = consumption_leech_damage->base_dd_max = leech_damage_accumulator;
      consumption_leech_damage->execute_on_target( state->target );

      if ( p()->talent.blood.carnage.ok() )
      {
        double amount = state->result_raw * std::abs( p()->talent.blood.carnage->effectN( 1 ).percent() );
        double current_value = p()->buffs.blood_shield->current_value;

        double final_amount = amount + current_value;

        // Blood Shield caps at 50% max health
        if ( final_amount > ( player->resources.max[ RESOURCE_HEALTH ] * p()->mastery.blood_shield->effectN( 3 ).percent() ) )
          final_amount = player->resources.max[ RESOURCE_HEALTH ] * p()->mastery.blood_shield->effectN( 3 ).percent();

        sim->print_debug( "{} Blood Shield buff trigger, old_value={} added_value={} new_value={} from action={} (id={})",
                      player->name(), current_value, amount, final_amount, name(), this->data().id() );
        p()->buffs.blood_shield->trigger( 1, final_amount );
      }
    }

    void execute() override
    {
      leech_damage_accumulator = 0;
      bp_consumption_multi = 0;

      death_knight_empowered_release_spell_t::execute();
    }
    private:
      double leech_damage_accumulator;
      double bp_consumption_multi;
      action_t* consumption_leech_damage;
  };

  consumption_t( death_knight_t* p, std::string_view options_str )
    : death_knight_empowered_charge_spell_t( "consumption", p, p->talent.blood.consumption, options_str )
    {
      if ( !target )
        target = p->sim->target;
      create_release_spell<consumption_damage_t>( "consumption_release" );
    }

  player_t* get_release_target( dot_t* d ) override
  {
    auto t = d->state->target;

    if ( t->is_sleeping() )
    {
      t = nullptr;

      for ( auto enemy : p()->sim->target_non_sleeping_list )
      {
        if ( enemy->is_sleeping() || ( enemy->debuffs.invulnerable && enemy->debuffs.invulnerable->check() ) )
          continue;

        t = enemy;
        break;
      }
    }

    return t;
  }
};

// Dancing Rune Weapon ======================================================
struct dancing_rune_weapon_t final : public death_knight_spell_t
{
  dancing_rune_weapon_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "dancing_rune_weapon", p, p->talent.blood.dancing_rune_weapon ), bone_shield_stack_gain( 0 )
  {
    may_miss = may_crit = may_dodge = may_parry = harmful = false;
    target                                                = p;
    if ( p->talent.blood.insatiable_blade.ok() )
      bone_shield_stack_gain += as<int>( p->talent.blood.insatiable_blade->effectN( 2 ).base_value() );
    parse_options( options_str );

    p->pets.dancing_rune_weapon_pet.set_creation_event_callback( pets::parent_pet_action_fn( this ) );

    if ( p->talent.deathbringer.echoing_fury.ok() )
      exterm_stacks = as<int>( p->talent.deathbringer.echoing_fury->effectN( 4 ).base_value() );
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    if ( p()->talent.blood.insatiable_blade.ok() )
    {
      p()->buffs.bone_shield->trigger( bone_shield_stack_gain );
    }

    // As of Dec 29th, 2025 everlasting bond spawns first
    if ( p()->talent.blood.everlasting_bond.ok() )
      p()->pets.everlasting_bond_pet.spawn();

    p()->pets.dancing_rune_weapon_pet.spawn();

    if ( p()->talent.sanlayn.the_blood_is_life.ok() )
      p()->pet_summon.blood_beast->execute();

    if ( p()->talent.blood.blood_mist.ok() )
      p()->buffs.blood_mist->trigger();

    if ( p()->talent.sanlayn.gift_of_the_sanlayn.ok() )
      p()->buffs.gift_of_the_sanlayn->trigger( p()->talent.blood.dancing_rune_weapon->duration() );

    if ( p()->talent.deathbringer.echoing_fury.ok() )
    {
      p()->buffs.exterminate->trigger();
      debug_cast<exterminate_t*>( p()->background_actions.exterminate )->empowered         = exterm_stacks;
      debug_cast<exterminate_aoe_t*>( p()->background_actions.exterminate_aoe )->empowered = exterm_stacks;
    }
  }

private:
  int bone_shield_stack_gain;
  int exterm_stacks;
};

// Dark Command =============================================================

struct dark_command_t final : public death_knight_spell_t
{
  dark_command_t( death_knight_t* p, std::string_view options_str )
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

// Death and Decay ==========================================================
// Death and Decay direct damage spells
struct death_and_decay_damage_base_t : public death_knight_spell_t
{
  death_and_decay_damage_base_t( std::string_view name, death_knight_t* p, const spell_data_t* spell )
    : death_knight_spell_t( name, p, spell )
  {
    aoe        = -1;
    background = dual = true;
    tick_zero         = true;
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );

    if ( p()->talent.unholy.cycle_of_death.ok() && s->chain_target < 10 )
      p()->cooldown.putrefy->adjust( -p()->talent.unholy.cycle_of_death->effectN( 2 ).time_value() );
  }
};

struct death_and_decay_damage_t final : public death_and_decay_damage_base_t
{
  death_and_decay_damage_t( std::string_view name, death_knight_t* p, const spell_data_t* s = nullptr )
    : death_and_decay_damage_base_t( name, p, s == nullptr ? p->spell.death_and_decay_damage : s )
  {
  }
};

// Relish in Blood healing and RP generation
struct relish_in_blood_t final : public death_knight_heal_t
{
  relish_in_blood_t( std::string_view name, death_knight_t* p )
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
  death_and_decay_base_t( death_knight_t* p, std::string_view name, const spell_data_t* spell )
    : death_knight_spell_t( name, p, spell ), params(), damage( nullptr )
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

  void init_params( player_t* t )
  {
    double n_ticks    = ( data().duration() / compute_tick_time() ) + 1;
    bool partial_tick = std::trunc( n_ticks ) != n_ticks;
    if ( partial_tick )
    {
      n_ticks = std::ceil( n_ticks );
      params.expiration_pulse( ground_aoe_params_t::expiration_pulse_type::FULL_EXPIRATION_PULSE );
    }

    params.target( t );
    params.duration( data().duration() );
    params.action( damage );
    params.pulse_time( compute_tick_time() );
    params.n_pulses( as<int>( n_ticks ) );
    params.x( t->x_position );
    params.y( t->y_position );

    params.state_callback(
        [ & ]( ground_aoe_params_t::state_type type, ground_aoe_event_t* event ) {
          switch ( type )
          {
            case ground_aoe_params_t::EVENT_CREATED:
              p()->active_dnds.push_back( event );
              break;
            case ground_aoe_params_t::EVENT_STARTED:
              p()->buffs.death_and_decay->trigger();
              break;
            case ground_aoe_params_t::EVENT_STOPPED:
              break;
            case ground_aoe_params_t::EVENT_DESTRUCTED:
              p()->active_dnds.pop_front();
              break;
            default:
              break;
          }
        } );

    params.expiration_callback( [ & ]( const action_state_t* ) {
      // Need to expire the buff before we set it with an expiration time, as it does not seem to extend
      p()->buffs.death_and_decay->expire();
      p()->buffs.death_and_decay->expire( 4_s );
    } );
  }

  void init_finished() override
  {
    death_knight_spell_t::init_finished();
    add_child( damage );
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
        p()->buffs.visceral_strength->trigger();
    }

    init_params( target );
    make_event<ground_aoe_event_t>( *sim, p(), params, true /* Immediate pulse */ );
  }

private:
  timespan_t compute_tick_time() const
  {
    auto base = data().effectN( 3 ).period();

    return base;
  }
  propagate_const<action_t*> relish_in_blood;
  ground_aoe_params_t params;

public:
  action_t* damage;
};

struct death_and_decay_t final : public death_and_decay_base_t
{
  death_and_decay_t( death_knight_t* p, std::string_view options_str )
    : death_and_decay_base_t( p, "death_and_decay", p->spec.death_and_decay )
  {
    damage = get_action<death_and_decay_damage_t>( "death_and_decay_damage", p );
    parse_options( options_str );
  }
};

// Death Grip ===============================================================
struct death_grip_t final : public death_knight_spell_t
{
  death_grip_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "death_grip", p, p->spec.death_grip )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    harmful               = false;
    cooldown              = p->cooldown.death_grip;
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );
    death_knight_td_t* td = p()->get_target_data( s->target );
    td->debuff.tightening_grasp->trigger();
    if ( p()->talent.blood.bone_collector.ok() )
      p()->buffs.bone_shield->trigger();
  }
};

// Death's Caress ===========================================================

struct deaths_caress_t final : public death_knight_spell_t
{
  deaths_caress_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "deaths_caress", p, p->spec.deaths_caress ), stacks( 0 )
  {
    parse_options( options_str );
    impact_action = get_action<blood_plague_t>( "blood_plague", p );
    stacks        = as<int>( p->spec.deaths_caress->effectN( 3 ).base_value() );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->buffs.bone_shield->trigger( stacks );

    p()->trigger_drw_action( DRW_ACTION_DEATHS_CARESS );
  }

private:
  int stacks;
};

// Death Coil ===============================================================

struct coil_of_devastation_t final : public residual_action::residual_periodic_action_t<death_knight_spell_t>
{
  coil_of_devastation_t( std::string_view name, death_knight_t* p )
    : residual_action::residual_periodic_action_t<death_knight_spell_t>( name, p, p->spell.coil_of_devastation_debuff )
  {
    background = dual = true;
    may_miss = hasted_ticks = false;
  }
};

struct death_coil_damage_base_t : public death_knight_spell_t
{
  death_coil_damage_base_t( std::string_view name, death_knight_t* p, const spell_data_t* s )
    : death_knight_spell_t( name, p, s ), triggers_effects( true ), sudden_doom( false ), cod_mod( 0.0 ), reaping_hp( 0.0 ), reaping_mod( 0.0 )
  {
    if ( p->talent.unholy.coil_of_devastation.ok() )
      cod_mod = p->talent.unholy.coil_of_devastation->effectN( 1 ).percent();

    if ( p->talent.unholy.reaping.ok() )
    {
      reaping_hp  = p->talent.unholy.reaping->effectN( 2 ).base_value();
      reaping_mod = p->talent.unholy.reaping->effectN( 1 ).percent();
    }
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_spell_t::composite_target_multiplier( target );

    if ( p()->talent.unholy.reaping.ok() && target->health_percentage() < reaping_hp )
      m *= 1.0 + reaping_mod;

    return m;
  }

  void execute() override
  {
    sudden_doom = p()->buffs.sudden_doom->check();

    death_knight_spell_t::execute();

    if ( triggers_effects )
      p()->unholy_rp_execute_effects( sudden_doom, true );
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( p()->talent.unholy.coil_of_devastation.ok() )
      residual_action::trigger( p()->background_actions.coil_of_devastation, state->target,
                                state->result_amount * cod_mod );

    if( triggers_effects )
      p()->unholy_rp_impact_effects( state, sudden_doom, true );
  }

public:
  bool triggers_effects;
private:
  bool sudden_doom;
  double cod_mod;
  double reaping_hp;
  double reaping_mod;
};

struct necrotic_coil_shadow_t final : public death_coil_damage_base_t
{
  necrotic_coil_shadow_t( std::string_view name, death_knight_t* p )
    : death_coil_damage_base_t( name, p, p->spell.necrotic_coil_shadow )
  {
    background = true;
    aoe        = -1;
  }
};

struct necrotic_coil_shadowstrike_t final : public death_coil_damage_base_t
{
  necrotic_coil_shadowstrike_t( std::string_view name, death_knight_t* p )
    : death_coil_damage_base_t( name, p, p->spell.necrotic_coil_physical )
  {
    background       = true;
    aoe              = as<int>( p->talent.unholy.forbidden_knowledge_1->effectN( 2 ).base_value() );
    execute_action   = get_action<necrotic_coil_shadow_t>( "necrotic_coil_shadow", p );
    triggers_effects = false;
  }
};

struct death_coil_damage_t final : public death_coil_damage_base_t
{
  death_coil_damage_t( std::string_view name, death_knight_t* p )
    : death_coil_damage_base_t( name, p, p->spell.death_coil_damage )
  {
    background = dual = true;
  }
};

struct death_coil_base_t : public death_knight_spell_t
{
  death_coil_base_t( std::string_view n, death_knight_t* p, const spell_data_t* s ) : death_knight_spell_t( n, p, s )
  {
  }
};

struct necrotic_coil_t final : public death_coil_base_t
{
  necrotic_coil_t( std::string_view n, death_knight_t* p ) : death_coil_base_t( n, p, p->spell.necrotic_coil_action )
  {
    execute_action = get_action<necrotic_coil_shadowstrike_t>( "necrotic_coil_shadowstrike", p );
    add_child( execute_action );
    add_child( execute_action->execute_action );
  }
};

struct death_coil_t final : public death_coil_base_t
{
  death_coil_t( death_knight_t* p, std::string_view options_str )
    : death_coil_base_t( "death_coil", p, p->spec.death_coil )
  {
    parse_options( options_str );

    execute_action        = p->background_actions.death_coil_damage;
    execute_action->stats = stats;
    stats->action_list.push_back( execute_action );

    set_replacement_action( new necrotic_coil_t( "necrotic_coil", p ), p->buffs.forbidden_knowledge, !p->options.wcl_reporting_mode );

    if ( p->talent.unholy.coil_of_devastation.ok() )
      add_child( p->background_actions.coil_of_devastation );

    if ( p->talent.unholy.doomed_bidding.ok() )
    {
      p->pets.lesser_ghoul_db_coil.set_creation_event_callback(
          pets::parent_pet_action_fn( p->pet_summon.db_ghoul_coil ) );
      add_child( p->pet_summon.db_ghoul_coil );
    }
  }
};

// Death Strike =============================================================

struct death_strike_heal_t final : public death_knight_heal_t
{
  death_strike_heal_t( std::string_view name, death_knight_t* p )
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
      // Blood is scripted to 5%, other specs get parsed
      if ( p->specialization() == DEATH_KNIGHT_BLOOD )
      {
        // Min and max pulls from the same effect for blood.
        min_heal_multiplier *= 1.0 + p->talent.improved_death_strike->effectN( 4 ).percent();
        max_heal_multiplier *= 1.0 + p->talent.improved_death_strike->effectN( 4 ).percent();
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
    auto cur_heal =
        player->compute_incoming_damage( std::min( interval, ( sim->current_time() - last_buff_consumption ) ) ) *
        max_heal_multiplier;

    return std::max( min_heal, cur_heal );
  }

  double base_da_max( const action_state_t* ) const override
  {
    auto min_heal = player->resources.max[ RESOURCE_HEALTH ] * min_heal_multiplier;
    auto cur_heal =
        player->compute_incoming_damage( std::min( interval, ( sim->current_time() - last_buff_consumption ) ) ) *
        max_heal_multiplier;

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
    trigger_blood_shield( state );

    p()->buffs.voracious->trigger();

    death_knight_heal_t::impact( state );

    auto min_heal = player->resources.max[ RESOURCE_HEALTH ] * min_heal_multiplier;
    auto cur_heal = player->compute_incoming_damage( interval ) * max_heal_multiplier;

    // Reset timer if we healed for more than a min heal
    if ( min_heal < cur_heal )
      last_buff_consumption = sim->current_time();
  }

private:
  timespan_t interval;
  timespan_t last_buff_consumption;
  double min_heal_multiplier;
  double max_heal_multiplier;
};

struct death_strike_offhand_t final : public death_knight_melee_attack_t
{
  death_strike_offhand_t( std::string_view name, death_knight_t* p )
    : death_knight_melee_attack_t( name, p, p->spell.death_strike_offhand )
  {
    background = true;
    weapon     = &( p->off_hand_weapon );
  }
};

struct death_strike_t final : public death_knight_melee_attack_t
{
  death_strike_t( death_knight_t* p, std::string_view options_str )
    : death_knight_melee_attack_t( "death_strike", p, p->talent.death_strike ),
      heal( get_action<death_strike_heal_t>( "death_strike_heal", p ) ),
      oh_attack( nullptr ),
      improved_death_strike_reduction( 0 ),
      sanguination_pct( 0.0 )
  {
    parse_options( options_str );
    may_parry = false;

    weapon = &( p->main_hand_weapon );

    // In simc, chain multiplier will reduce the damage per target hit, however in game
    // this affect seems to be a constant reduction across all secondary targets
    chain_multiplier = 1.0;
    base_aoe_multiplier = p->talent.death_strike->effectN( 1 ).chain_multiplier();

    if ( p->dual_wield() )
    {
      oh_attack = get_action<death_strike_offhand_t>( "death_strike_offhand", p );
      add_child( oh_attack );
    }

    if ( p->talent.improved_death_strike.ok() && p->specialization() == DEATH_KNIGHT_BLOOD )
    {
      improved_death_strike_reduction += p->talent.improved_death_strike->effectN( 5 ).resource( RESOURCE_RUNIC_POWER );
    }
  }

  void init_finished() override
  {
    death_knight_melee_attack_t::init_finished();

    if ( p()->has_runeforge( RUNEFORGE_SANGUINATION ) )
    {
      sanguination_pct = 1 + ( 0.25 * ( 1 + p()->talent.unholy_bond->effectN( 1 ).percent() ) );
    }
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    // 2020-08-23: Seems to only affect main hand death strike, not OH hit, not DRW's spell
    // No spelldata either, just "increase damage based on target's missing health"
    // Update 2021-06-16 Changed from 1% to 1.25% damage increase
    // Testing shows a linear 1.25% damage increase for every 1% missing health
    // Unholy Bond increases this by 20% with the formula 1 + ( 0.25 * ( 1 + unholy_bond % ) )
    if ( p()->has_runeforge( RUNEFORGE_SANGUINATION ) )
    {
      m *= 1.0 + std::min( ( 1.0 - target->health_percentage() * 0.01 ) * sanguination_pct,
                           ( sanguination_pct * 80 ) * 0.01 );
      // Unholy bond gives a 20% bonus to damage, on top of the 20% bonus to the sanguination scaled damage
      m *= 1.0 + p()->talent.unholy_bond->effectN( 1 ).percent();
    }

    return m;
  }

  double cost_flat_modifier() const override
  {
    double c = death_knight_melee_attack_t::cost_flat_modifier();

    c += improved_death_strike_reduction;

    return c;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    p()->buffs.coagulopathy->trigger();
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( oh_attack )
      oh_attack->execute_on_target( execute_state->target );

    p()->trigger_drw_action( DRW_ACTION_DEATH_STRIKE );

    if ( hit_any_target )
    {
      heal->execute();
    }

    p()->buffs.hemostasis->expire();
    if ( p()->talent.sanlayn.vampiric_strike.ok() && !p()->buffs.gift_of_the_sanlayn->check() )
    {
      p()->trigger_vampiric_strike_proc( target );
    }

    if ( p()->sets->has_set_bonus( DEATH_KNIGHT_BLOOD, MID1, B4 ) &&
            p()->rng().roll( p()->sets->set( DEATH_KNIGHT_BLOOD, MID1, B4)->effectN( 2 ).percent() ) )
    {
      p()->cooldown.blood_boil->reset( false );
    }
  }

private:
  propagate_const<action_t*> heal;
  propagate_const<action_t*> oh_attack;
  double improved_death_strike_reduction;
  double sanguination_pct;
};

// Empower Rune Weapon ======================================================
struct empower_rune_weapon_projectile_t final : public death_knight_spell_t
{
  empower_rune_weapon_projectile_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->talent.frost.empower_rune_weapon->effectN( 1 ).trigger() )
  {
    background = quiet = true;
    harmful            = false;
  }
};

struct empower_rune_weapon_t final : public death_knight_spell_t
{
  empower_rune_weapon_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "empower_rune_weapon", p, p->talent.frost.empower_rune_weapon ),
      projectile( p->background_actions.erw_projectile )
  {
    parse_options( options_str );
    aoe                 = -1;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;

    internal_cooldown->duration = min_gcd;
    min_gcd = trigger_gcd = 0_ms;
  }

  bool ready() override
  {
    if ( p()->sets->has_set_bonus( DEATH_KNIGHT_FROST, MID1, B4 ) && p()->buffs.frost_mid1_4pc_buff->up() && internal_cooldown->is_ready() )
      return true;
    return death_knight_spell_t::ready();
  }

  void queue_execute( execute_type et ) override
  {
    // need to bypass the default behavior to account for the tier set granting a free charge
    // otherwise, erw sits there queued until the real cooldown is ready
    if ( et == execute_type::FOREGROUND && p()->buffs.frost_mid1_4pc_buff->up() )
    {
      schedule_execute();
    }
    else {
      death_knight_spell_t::queue_execute( et );
    }
  }

  void update_ready( timespan_t cd_duration ) override
  {
    if ( p()->buffs.frost_mid1_4pc_buff->up() )
    {
      p()->buffs.frost_mid1_4pc_buff->decrement();
      if ( internal_cooldown->duration > 0_ms )
      {
        internal_cooldown->start( this );

        sim->print_debug( "{} starts internal_cooldown for {} ({}). Will be ready at {}", *player, *this,
                          *internal_cooldown, internal_cooldown->ready );
      }
    }
    else
    {
      death_knight_spell_t::update_ready( cd_duration );
    }
  }

  // yoinked from evoker
  void reset() override
  {
    // Reset max charges to initial value, since it can get out of sync when previous iteration ends with charge-giving
    // buffs up. Do this before calling reset as that will also reset the cooldown.
    cooldown->charges = data().charges();
    death_knight_spell_t::reset();
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p()->buffs.empower_rune_weapon->cooldown->is_ready() )
    {
      if ( p()->buffs.pillar_of_frost->check() && p()->talent.frost.obliteration->ok() )
        p()->buffs.empower_rune_weapon->trigger();

      p()->resource_gain( RESOURCE_RUNIC_POWER,
                          p()->buffs.empower_rune_weapon->data().effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
                          p()->gains.empower_rune_weapon, this );
      p()->trigger_killing_machine( true, p()->procs.km_from_erw, p()->procs.km_from_erw_wasted );
    }

    if ( p()->sets->has_set_bonus( DEATH_KNIGHT_FROST, MID1, B2 ) )
    {
      p()->buffs.empowered_strikes->trigger();
    }
    if ( p()->sets->has_set_bonus( DEATH_KNIGHT_FROST, MID1, B4 ) ) 
    {
      p()->buffs.frost_mid1_4pc_buff->consume( this );
    }

    projectile->execute_on_target( execute_state->target );
  }

private:
  propagate_const<action_t*> projectile;
};

// Epidemic =================================================================
struct epidemic_damage_base_t : public death_knight_spell_t
{
  epidemic_damage_base_t( std::string_view name, death_knight_t* p, const spell_data_t* spell )
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

public:
  double soft_cap_multiplier;
};

struct epidemic_damage_aoe_t final : public epidemic_damage_base_t
{
  epidemic_damage_aoe_t( std::string_view name, death_knight_t* p )
    : epidemic_damage_base_t( name, p, p->spell.epidemic_damage )
  {
    // Main is one target, aoe is the other targets, so we take 1 off the max targets
    aoe                     = aoe - 1;
    attack_power_mod.direct = data().effectN( 2 ).ap_coeff();
    target_filter_callback  = secondary_targets_only();
  }
};

struct graveyard_damage_aoe_t final : public epidemic_damage_base_t
{
  graveyard_damage_aoe_t( std::string_view name, death_knight_t* p )
    : epidemic_damage_base_t( name, p, p->spell.graveyard_damage )
  {
    aoe                     = data().max_targets() - 1;
    attack_power_mod.direct = data().effectN( 2 ).ap_coeff();
  }
};

struct epidemic_damage_main_t final : public epidemic_damage_base_t
{
  epidemic_damage_main_t( std::string_view name, death_knight_t* p )
    : epidemic_damage_base_t( name, p, p->spell.epidemic_damage )
  {
    // Ignore spelldata for max targets for the main spell, as it is single target only
    aoe = 0;
    // this spell has both coefficients in it, and it seems like it is reading #2, the aoe portion, instead of #1
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();

    impact_action = get_action<epidemic_damage_aoe_t>( "epidemic_aoe", p );
    add_child( impact_action );
  }
};

struct graveyard_damage_main_t final : public epidemic_damage_base_t
{
  graveyard_damage_main_t( std::string_view name, death_knight_t* p )
    : epidemic_damage_base_t( name, p, p->spell.graveyard_damage )
  {
    // Ignore spelldata for max targets for the main spell, as it is single target only
    aoe = 0;
    // this spell has both coefficients in it, and it seems like it is reading #2, the aoe portion, instead of #1
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();

    impact_action = get_action<graveyard_damage_aoe_t>( "graveyard_aoe", p );
  }
};

struct epidemic_base_t : public death_knight_spell_t
{
  epidemic_base_t( std::string_view n, death_knight_t* p, const spell_data_t* s )
    : death_knight_spell_t( n, p, s ), custom_reduced_aoe_targets( 8.0 ), soft_cap_multiplier( 1.0 ), sd( false )
  {
    target_filter_callback = dot_or_debuff_only( &death_knight_td_t::dots_t::virulent_plague );
  }

  void execute() override
  {
    // Reset target cache because of smart targetting
    target_cache.is_valid = false;

    death_knight_spell_t::execute();

    if ( was_replaced )
      return;

    // Set the multiplier for reduced aoe soft cap
    int targets = std::min( num_targets(), n_targets() );
    if ( targets > 0 && targets > custom_reduced_aoe_targets )
      soft_cap_multiplier = sqrt( custom_reduced_aoe_targets / std::min<int>( sim->max_aoe_enemies, targets ) );
    else
      soft_cap_multiplier = 1.0;

    debug_cast<epidemic_damage_base_t*>( impact_action )->soft_cap_multiplier                = soft_cap_multiplier;
    debug_cast<epidemic_damage_base_t*>( impact_action->impact_action )->soft_cap_multiplier = soft_cap_multiplier;

    sd = p()->buffs.sudden_doom->up();
    p()->unholy_rp_execute_effects( sd );
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    p()->unholy_rp_impact_effects( state, sd );
  }

private:
  double custom_reduced_aoe_targets;  // Not in spelldata
  double soft_cap_multiplier;
  bool sd;
};

struct graveyard_t final : public epidemic_base_t
{
  graveyard_t( std::string_view n, death_knight_t* p ) : epidemic_base_t( n, p, p->spell.graveyard_action )
  {
    impact_action = get_action<graveyard_damage_main_t>( "graveyard_main", p );
    add_child( impact_action );
    add_child( impact_action->impact_action );
  }
};

struct epidemic_t final : public epidemic_base_t
{
  epidemic_t( death_knight_t* p, std::string_view options_str ) : epidemic_base_t( "epidemic", p, p->spec.epidemic )
  {
    parse_options( options_str );
    impact_action = p->background_actions.epidemic_main;

    set_replacement_action( new graveyard_t( "graveyard", p ), p->buffs.forbidden_knowledge );

    add_child( impact_action );

    if ( p->talent.unholy.doomed_bidding.ok() )
    {
      p->pets.lesser_ghoul_db_epi.set_creation_event_callback(
          pets::parent_pet_action_fn( p->pet_summon.db_ghoul_epi ) );
      add_child( p->pet_summon.db_ghoul_epi );
    }
  }
};

// Festering Strike and Scythe ==============================================
struct festering_base_t : public death_knight_melee_attack_t
{
  festering_base_t( std::string_view n, death_knight_t* p, const spell_data_t* s )
    : death_knight_melee_attack_t( n, p, s ), min_ghouls( 0 ), max_ghouls( 0 )
  {
    min_ghouls = as<int>( p->spec.festering_strike->effectN( 3 ).base_value() );
    // rng().range() does not include the max value, so we add 1 here
    max_ghouls = as<int>( p->spec.festering_strike->effectN( 4 ).base_value() + 1 );
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

  void execute() override
  {
    death_knight_melee_attack_t::execute();
    if ( was_replaced )
      return;

    int count = rng().range( min_ghouls, max_ghouls );
    p()->buffs.lesser_ghoul_ready->trigger( count );
  }

private:
  int min_ghouls;
  int max_ghouls;
};

struct festering_scythe_t final : public festering_base_t
{
  festering_scythe_t( std::string_view n, death_knight_t* p )
    : festering_base_t( n, p, p->spell.festering_scythe )
  {
    aoe             = -1;
    background      = true;
  }

  void execute() override
  {
    festering_base_t::execute();
    p()->buffs.festering_scythe->consume( this );
  }

  void impact( action_state_t* s ) override
  {
    festering_base_t::impact( s );
    p()->buffs.festering_scythe_tt->trigger();
  }
};

struct festering_strike_t final : public festering_base_t
{
  festering_strike_t( death_knight_t* p, std::string_view options_str )
    : festering_base_t( "festering_strike", p, p->spec.festering_strike )
  {
    parse_options( options_str );

    if ( p->talent.unholy.festering_scythe.ok() )
      set_replacement_action( get_action<festering_scythe_t>( "festering_scythe", p ), p->buffs.festering_scythe, !p->options.wcl_reporting_mode );
  }

  void execute() override
  {
    festering_base_t::execute();

    if ( was_replaced )
      return;

    if ( p()->talent.unholy.festering_scythe.ok() )
      p()->buffs.festering_scythe->trigger();
  }
};

// Frostscythe ==============================================================
struct frostscythe_base_t : public death_knight_melee_attack_t
{
  frostscythe_base_t( std::string_view n, death_knight_t* p, const spell_data_t* s )
    : death_knight_melee_attack_t( n, p, s )
  {
    inexorable_assault = get_action<inexorable_assault_damage_t>( "inexorable_assault", p );

    weapon              = &( player->main_hand_weapon );
    aoe                 = -1;
    reduced_aoe_targets = data().effectN( 5 ).base_value();
  }

  void execute() override
  {
    if ( p()->talent.rider.whitemanes_famine.ok() && p()->sim->target_non_sleeping_list.size() > 1 )
    {
      p()->sort_undeath_targets( target_list() );
    }

    death_knight_melee_attack_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    auto td = get_td( s->target );

    if ( p()->buffs.inexorable_assault->up() && p()->cooldown.inexorable_assault_icd->is_ready() )
    {
      inexorable_assault->set_target( target );
      inexorable_assault->schedule_execute();
      p()->buffs.inexorable_assault->decrement();
      p()->cooldown.inexorable_assault_icd->start();
    }

    if ( p()->talent.frost.enduring_strength.ok() && p()->buffs.pillar_of_frost->up() &&
         p()->cooldown.enduring_strength_icd->is_ready() && s->result == RESULT_CRIT )
    {
      p()->buffs.enduring_strength_builder->trigger();
      p()->cooldown.enduring_strength_icd->start();
    }

    if ( p()->talent.rider.trollbanes_icy_fury.ok() && td->debuff.chains_of_ice_trollbane_slow->check() &&
         p()->pets.trollbane.active_pet() != nullptr )
    {
      td->debuff.chains_of_ice_trollbane_slow->expire();
      p()->background_actions.trollbanes_icy_fury->execute_on_target( s->target );
    }

    if ( p()->talent.rider.whitemanes_famine.ok() && td->dot.undeath->is_ticking() )
    {
      p()->trigger_whitemanes_famine( s->target );
    }
  }

private:
  propagate_const<action_t*> inexorable_assault;
};

struct frostscythe_t : public frostscythe_base_t
{
  frostscythe_t( death_knight_t* p, std::string_view options_str )
    : frostscythe_base_t( "frostscythe", p, p->talent.frost.frostscythe ),
      aa_action( p->background_actions.arctic_assault_frostscythe )
  {
    parse_options( options_str );

    if ( p->talent.frost.arctic_assault.ok() )
    {
      add_child( aa_action );
    }
  }

  void execute() override
  {
    frostscythe_base_t::execute();

    if ( p()->buffs.killing_machine->up() )
    {
      // No spell data values found that match delay betwee FSC cast and KM consumption
      p()->consume_killing_machine( p()->procs.killing_machine_fsc, 50_ms, aa_action );
    }

    if ( p()->talent.frost.obliteration.ok() && p()->buffs.empower_rune_weapon->check() )
    {
      p()->buffs.empower_rune_weapon->expire();
    }

    if ( p()->talent.rider.let_terror_reign->ok() && p()->pets.trollbane.active_pet() != nullptr )
      // 11.2 TODO check delays again between patch launch and season launch
      make_event<delayed_execute_event_t>( *sim, p(), p()->pets.trollbane.active_pet()->frostscythe,
                                           execute_state->target, p()->rng().gauss( 200_ms, 50_ms ) );

    if ( p()->buffs.exterminate->up() )
    {
      p()->buffs.exterminate->decrement();
      make_event<delayed_execute_event_t>( *sim, p(), p()->background_actions.exterminate, execute_state->target,
                                           500_ms );
    }

    if ( p()->sets->has_set_bonus( DEATH_KNIGHT_FROST, MID1, B2 ) )
    {
      p()->buffs.empowered_strikes->consume( this, 1 );
    }
  }

private:
  propagate_const<action_t*> aa_action;
};

// Frostwyrm's Fury =========================================================
struct frostwyrms_fury_damage_t : public death_knight_spell_t
{
  frostwyrms_fury_damage_t( std::string_view name, death_knight_t* p, const spell_data_t* s )
    : death_knight_spell_t( name, p, s )
  {
    aoe                = -1;
    background         = true;
    cooldown->duration = 0_ms;  // handled by the actions
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( state );
    if ( p()->talent.frost.chosen_of_frostbrood_1.ok() && state->chain_target == 0 )
    {
      m *= 1.0 + p()->talent.frost.chosen_of_frostbrood_1->effectN( 1 ).percent();
    }
    return m;
  }
};

struct fwf_action_base_t : public death_knight_spell_t
{
  fwf_action_base_t( std::string_view name, death_knight_t* p, const spell_data_t* s, std::string_view options_str )
    : death_knight_spell_t( name, p, s ),
      rider_dur( 0_ms ),
      haste_val( 0 ),
      pillar_extension( 0_ms ),
      fwf_damage( nullptr )
  {
    parse_options( options_str );
    if ( p->talent.rider.apocalypse_now.ok() )
      rider_dur = p->talent.rider.apocalypse_now->effectN( 2 ).time_value();

    if ( p->talent.frost.chosen_of_frostbrood_1.ok() ) 
    {
      haste_val = p->spell.chosen_of_frostbrood_haste_buff->effectN( 1 ).percent();
      haste_duration = p->spell.chosen_of_frostbrood_haste_buff->duration();
    }
      

    if ( p->talent.frost.chosen_of_frostbrood_2.ok() )
      pillar_extension = p->talent.frost.chosen_of_frostbrood_2->effectN( 1 ).time_value();
  }

  void init_finished() override
  {
    death_knight_spell_t::init_finished();
    // Wait til init finished to get the fwf damage action
    fwf_damage->stats = stats;
    stats->action_list.push_back( fwf_damage );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( was_replaced )
      return;

    assert( fwf_damage && "Frostwyrms Fury Action missing Damage Action" );

    if ( fwf_damage )
      fwf_damage->execute();

    if ( p()->talent.rider.apocalypse_now.ok() )
      p()->summon_rider( rider_dur, rider_of_the_apocalypse_e::ALL_RIDERS );

    if ( p()->talent.frost.chosen_of_frostbrood_1.ok() )
    {
      p()->buffs.chosen_of_frostbrood_haste->set_refresh_behavior( buff_refresh_behavior::EXTEND );
      p()->buffs.chosen_of_frostbrood_haste->trigger( haste_duration );
      
    }

    if ( p()->talent.frost.chosen_of_frostbrood_2.ok() && p()->buffs.pillar_of_frost->check() )
      p()->buffs.pillar_of_frost->extend_duration( pillar_extension );
  }

public:
  timespan_t rider_dur;
  double haste_val;
  timespan_t haste_duration;
  timespan_t pillar_extension;
  action_t* fwf_damage;
};

struct chosen_of_frostbrood_fwf_t final : public fwf_action_base_t
{
  chosen_of_frostbrood_fwf_t( death_knight_t* p, std::string_view options_str )
    : fwf_action_base_t( "chosen_of_frostbrood", p, p->spell.chosen_of_frostbrood_fwf_action, options_str )
  {
    double chosen_mult = p->talent.frost.chosen_of_frostbrood_3->effectN( 2 ).percent();
    fwf_damage = get_action<frostwyrms_fury_damage_t>( "frostwyrms_fury_recall", p, p->spell.frostwyrms_fury_damage );
    fwf_damage->base_multiplier = chosen_mult;
    rider_dur *= chosen_mult;
    haste_val *= chosen_mult;
    haste_duration *= chosen_mult;
    pillar_extension *= chosen_mult;
    min_gcd = trigger_gcd = 0_ms;
  }

  void execute() override
  {
    fwf_action_base_t::execute();

    p()->buffs.chosen_of_frostbrood_fwf->expire();
  }
};

struct frostwyrms_fury_t final : public fwf_action_base_t
{
  frostwyrms_fury_t( death_knight_t* p, std::string_view options_str )
    : fwf_action_base_t( "frostwyrms_fury", p, p->talent.frost.frostwyrms_fury, options_str )
  {
    fwf_damage = get_action<frostwyrms_fury_damage_t>( "frostwyrms_fury_damage", p, p->spell.frostwyrms_fury_damage );
    set_replacement_action( new chosen_of_frostbrood_fwf_t( p, options_str ), p->buffs.chosen_of_frostbrood_fwf );
    // Stun is NYI
  }

  void execute() override
  {
    fwf_action_base_t::execute();

    if ( was_replaced )
      return;

    if ( p()->talent.frost.chosen_of_frostbrood_3.ok() )
      make_event( *sim, p()->spell.chosen_of_frostbrood_delay->duration(),
                  [ & ] { p()->buffs.chosen_of_frostbrood_fwf->trigger(); } );
  }
};

// Frost Strike =============================================================
struct frostreaper_t : public death_knight_spell_t
{
  frostreaper_t( std::string_view n, death_knight_t* p ) : death_knight_spell_t( n, p, p->spell.frostreaper_damage )
  {
    background = true;
  }
};

struct frost_strike_strike_t final : public death_knight_melee_attack_t
{
  frost_strike_strike_t( std::string_view n, death_knight_t* p, weapon_t* w, const spell_data_t* s,
                         bool shattering_blade )
    : death_knight_melee_attack_t( n, p, s ), sb( shattering_blade )
  {
    background = special = true;
    weapon               = w;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m      = death_knight_melee_attack_t::composite_da_multiplier( state );
    const auto ri = get_td( state->target )->debuff.razorice;

    if ( sb )
    {
      m *= 1.0 + p()->talent.frost.shattering_blade->effectN( 1 ).percent();
      m *= 1.0 + ri->default_value * ri->max_stack();
    }

    return m;
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    if ( p()->talent.frost.hyperpyrexia->ok() && s->result_amount > 0 &&
         p()->rng().roll( p()->talent.frost.hyperpyrexia->proc_chance() ) )
    {
      residual_action::trigger( p()->background_actions.hyperpyrexia_damage, s->target,
                                s->result_amount * p()->talent.frost.hyperpyrexia->effectN( 1 ).percent() );
    }
  }

public:
  bool sb;
};

struct frostbane_strike_t final : public death_knight_melee_attack_t
{
  frostbane_strike_t( std::string_view n, death_knight_t* p )
    : death_knight_melee_attack_t( n, p, p->spell.frostbane_damage )
  {
    dual       = true;
    background = true;
    aoe        = -1;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m      = death_knight_melee_attack_t::composite_da_multiplier( state );
    const auto ri = get_td( state->target )->flag.razorice_consumed;

    double target_reduction = 1 - ( state->chain_target * p()->talent.frost.frostbane->effectN( 4 ).percent() );
    target_reduction        = std::max( target_reduction, p()->talent.frost.frostbane->effectN( 5 ).percent() );

    m *= target_reduction;

    if ( state->chain_target == 0 && p()->talent.frost.shattering_blade->ok() && ri )
    {
      m *= 1.0 + p()->talent.frost.shattering_blade->effectN( 2 ).percent();
    }

    else if ( ri )
    {
      m *= 1.0 + p()->talent.frost.shattering_blade->effectN( 1 ).percent();
    }

    return m;
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    if ( p()->talent.frost.hyperpyrexia->ok() && s->result_amount > 0 &&
         p()->rng().roll( p()->talent.frost.hyperpyrexia->proc_chance() ) )
    {
      residual_action::trigger( p()->background_actions.hyperpyrexia_damage, s->target,
                                s->result_amount * p()->talent.frost.hyperpyrexia->effectN( 1 ).percent() );
    }
  }
};

struct frost_strike_base_t : public death_knight_melee_attack_t
{
  frost_strike_base_t( std::string_view n, death_knight_t* p, const spell_data_t* s )
    : death_knight_melee_attack_t( n, p, s ), frostreaper( nullptr )
  {
    if ( p->talent.frost.frostreaper.ok() )
      frostreaper = get_action<frostreaper_t>( "frostreaper", p );
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( was_replaced )
      return;

    // 6/21/25 IO buffs the frost strike that procs RE so we need to delay expiration and prevent
    // additional stacks til after the FS is resolved
    // In game this looks like: start FS cast, check if RE proced and stack IO if it did not, finish FS cast, calc
    // damage, expire IO

    // frostbane benefits from IO, and stacks it, but because its damage is delayed it will not get buffed
    // when frostbane procs RE
    if ( p()->talent.frost.icy_onslaught->ok() && p()->buffs.icy_onslaught->expiration_delay == nullptr && !p()->buffs.icy_onslaught->freeze_stacks )
      p()->buffs.icy_onslaught->trigger();
    p()->buffs.icy_onslaught->set_freeze_stacks( false );


    if ( p()->talent.frost.frostreaper.ok() )
    {
      const auto td = get_td( target );

      if ( td->debuff.frostreaper->up() )
      {
        frostreaper->execute_on_target( target );
        for ( auto t : target_list() )
          get_td( t )->debuff.frostreaper->expire();
      }
    }

    if ( p()->buffs.pillar_of_frost->up() && p()->talent.frost.obliteration.ok() )
    {
      p()->trigger_killing_machine( true, p()->procs.km_from_obliteration_fs,
                                    p()->procs.km_from_obliteration_fs_wasted );

      // Obliteration's rune generation
      if ( rng().roll( p()->talent.frost.obliteration->effectN( 2 ).percent() ) )
      {
        p()->replenish_rune( as<int>( p()->spell.obliteration_gains->effectN( 1 ).base_value() ),
                             p()->gains.obliteration );
      }
    }

    p()->buffs.rime->trigger();
  }

private:
  action_t* frostreaper;
};

struct frostbane_t final : public frost_strike_base_t
{
  frostbane_t( death_knight_t* p, std::string_view options_str )
    : frost_strike_base_t( "frostbane", p, p->spell.frostbane_driver ),
      frostbane_strike( new frostbane_strike_t( "frostbane_strike", p ) ),
      delay_1( 0_ms ),
      delay_2( 0_ms )
  {
    parse_options( options_str );
    aoe = -1;
    if ( data().ok() )
    {
      frostbane_strike->stats = stats;
      stats->action_list.push_back( frostbane_strike );
      delay_1 = timespan_t::from_millis( data().effectN( 4 ).misc_value1() );
      delay_2 = timespan_t::from_millis( data().effectN( 5 ).misc_value1() );
    }
  }

  void impact( action_state_t* s ) override
  {
    frost_strike_base_t::impact( s );

    auto td                    = get_td( s->target );
    td->flag.razorice_consumed = false;

    if ( td->debuff.razorice->at_max_stacks() )
    {
      // if frostbane hits a target with 5 stacks of razorice, it consumes the stacks and gets the shattering bonus
      // however, it does not get the hidden bonus that shattering blade gets from razorice
      td->debuff.razorice->expire();
      td->flag.razorice_consumed = true;
    }
  }

  void execute() override
  {
    frost_strike_base_t::execute();

    make_event<delayed_execute_event_t>( *sim, p(), frostbane_strike, target, delay_1 );
    make_event<delayed_execute_event_t>( *sim, p(), frostbane_strike, target, delay_2 );
    p()->buffs.frostbane->expire();
  }

  bool ready() override
  {
    if ( !p()->buffs.frostbane->up() )
      return false;

    return frost_strike_base_t::ready();
  }

private:
  action_t* frostbane_strike;
  timespan_t delay_1;
  timespan_t delay_2;
};

struct frost_strike_t final : public frost_strike_base_t
{
  frost_strike_t( death_knight_t* p, std::string_view options_str )
    : frost_strike_base_t( "frost_strike", p, p->talent.frost.frost_strike ),
      mh( p->background_actions.frost_strike_main ),
      oh( p->background_actions.frost_strike_offhand ),
      mh_sb( p->background_actions.frost_strike_sb_main ),
      oh_sb( p->background_actions.frost_strike_sb_offhand ),
      mh_delay( 0_ms ),
      oh_delay( 0_ms ),
      sb( false )
  {
    parse_options( options_str );

    dual = true;

    if ( data().ok() )
    {
      if ( p->main_hand_weapon.group() == WEAPON_2H )
        mh_delay = timespan_t::from_millis( as<int>( data().effectN( 4 ).misc_value1() ) );

      if ( p->off_hand_weapon.type != WEAPON_NONE )
        oh_delay = timespan_t::from_millis( as<int>( data().effectN( 3 ).misc_value1() ) );

      mh->stats = stats;
      stats->action_list.push_back( mh );

      if ( p->talent.frost.shattering_blade.ok() )
        add_child( mh_sb );

      if ( p->off_hand_weapon.type != WEAPON_NONE && p->main_hand_weapon.group() != WEAPON_2H )
      {
        add_child( oh );
        if ( p->talent.frost.shattering_blade.ok() )
          add_child( oh_sb );
      }
      if ( p->talent.frost.frostbane.ok() )
      {
        if ( p->find_action( "frostbane" ) )
          set_replacement_action( "frostbane", p->buffs.frostbane );
        else
          set_replacement_action( new frostbane_t( p, options_str ), p->buffs.frostbane );
      }

    }
  }

  void execute() override
  {

    frost_strike_base_t::execute();

    if ( was_replaced )
      return;

    auto td = get_td( target );
    if ( p()->talent.frost.shattering_blade.ok() && td->debuff.razorice->at_max_stacks() )
    {
      sb = true;
      td->debuff.razorice->expire();
    }

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
  glacial_advance_damage_t( std::string_view name, death_knight_t* p, bool aa = false )
    : death_knight_spell_t( name, p, p->spell.glacial_advance_damage ),
      is_arctic_assault( aa ),
      targets_max_razorice( 0 )
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
    if ( is_arctic_assault )
    {
      base_multiplier *= p->talent.frost.arctic_assault->effectN( 1 ).percent();
    }
  }

  void execute() override
  {
    targets_max_razorice = 0;
    death_knight_spell_t::execute();

    // Killing Machine glacial advcances currently does not trigger Obliteration rune generation
    if ( is_arctic_assault )
    {
      if ( p()->talent.icy_talons.ok() )
      {
        p()->buffs.icy_talons->trigger();
      }
    }

    if ( execute_state && p()->talent.frost.frostbane && !p()->buffs.frostbane->up() )
    {
      // 11.2 TODO find actual proc chance
      // This is a very dumb formula that is only here to emulate a very high proc chance when 3+ targets have 5 stacks
      double chance = .195;

      if (execute_state->n_targets > 2)
        chance = std::min( ( .09 * (execute_state->n_targets - targets_max_razorice) ) + ( .32 * targets_max_razorice ),
                           .925 );

      if ( p()->rng().roll( chance ) )
        p()->buffs.frostbane->trigger();
    }
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    auto razorice = get_td( state->target )->debuff.razorice;

    razorice->trigger();
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
      residual_action::trigger( p()->background_actions.hyperpyrexia_damage, state->target,
                                state->result_amount * p()->talent.frost.hyperpyrexia->effectN( 1 ).percent() );
    }

    if ( razorice->at_max_stacks() )
    {
      targets_max_razorice++;
    }
  }

private:
  bool is_arctic_assault;
  int targets_max_razorice;
};

struct glacial_advance_t final : public death_knight_spell_t
{
  glacial_advance_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "glacial_advance", p, p->spec.glacial_advance ),
      damage_action( get_action<glacial_advance_damage_t>( "glacial_advance_damage", p ) )
  {
    parse_options( options_str );

    weapon = &( p->main_hand_weapon );

    add_child( damage_action );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    // 6/21/25 IO buffs the frost strike that procs RE so we need to delay expiration and prevent
    // additional stacks til after the FS is resolved
    // In game this looks like: start FS cast, check if RE proced and stack IO if it did not, finish FS cast, calc
    // damage, expire IO

    // frostbane benefits from IO, and stacks it, but because its damage is delayed it will not get buffed
    // when frostbane procs RE
    if ( p()->talent.frost.icy_onslaught->ok() && p()->buffs.icy_onslaught->expiration_delay == nullptr )
    {
      p()->buffs.icy_onslaught->trigger();
    }
    p()->buffs.icy_onslaught->set_freeze_stacks( false );

    // 12.0 TODO drive the delay from likely misc values
    make_event<delayed_execute_event_t>( *sim, p(), damage_action, execute_state->target, 100_ms );

    if ( p()->buffs.pillar_of_frost->up() && p()->talent.frost.obliteration.ok() )
    {
      p()->trigger_killing_machine( true, p()->procs.km_from_obliteration_ga,
                                    p()->procs.km_from_obliteration_ga_wasted );

      // Obliteration's rune generation
      if ( rng().roll( p()->talent.frost.obliteration->effectN( 2 ).percent() ) )
      {
        p()->replenish_rune( as<int>( p()->spell.obliteration_gains->effectN( 1 ).base_value() ),
                             p()->gains.obliteration );
      }
    }

    p()->buffs.rime->trigger();
  }

  private:
  action_t* damage_action;
};

// Gorefiend's Grasp ========================================================

struct gorefiends_grasp_t final : public death_knight_spell_t
{
  gorefiends_grasp_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "gorefiends_grasp", p, p->talent.blood.gorefiends_grasp )
  {
    parse_options( options_str );
    aoe = -1;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    auto td = get_td( state->target );
    if ( td )
      td->debuff.tightening_grasp->trigger();

    if ( p()->talent.blood.bone_collector.ok() )
      p()->buffs.bone_shield->trigger();
  }
};

// Heart Strike =============================================================

struct leeching_strike_t final : public death_knight_heal_t
{
  leeching_strike_t( std::string_view name, death_knight_t* p )
    : death_knight_heal_t( name, p, p->spell.leeching_strike_damage )
  {
    background = true;
    may_crit = callbacks = false;
    target               = p;
    // As of July 20 2024, heals for 2.5% of health, represented as 0.25 in spelldata
    base_pct_heal = data().effectN( 1 ).base_value() * 0.1;
  }
};

struct heart_strike_base_t : public death_knight_melee_attack_t
{
  heart_strike_base_t( std::string_view n, death_knight_t* p, const spell_data_t* s )
    : death_knight_melee_attack_t( n, p, s ),
      heartbreaker_rp_gen( p->spell.heartbreaker_rp_gain->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) ),
      boiling_point_proc_attempts( 0 )
  {
    aoe             = 2;
    weapon          = &( p->main_hand_weapon );
    leeching_strike = get_action<leeching_strike_t>( "leeching_strike", p );

    if ( p->talent.blood.boiling_point.ok() )
      boiling_point_proc_chance = 0.15;
  }

  int n_targets() const override
  {
    return p()->buffs.death_and_decay->up() ? aoe + as<int>( p()->spell.dnd_buff->effectN( 3 ).base_value() ) : aoe;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( p()->talent.deathbringer.dark_talons.ok() && p()->talent.icy_talons->ok() &&
         rng().roll( p()->talent.deathbringer.dark_talons->effectN( 1 ).percent() ) )
    {
      p()->buffs.dark_talons_icy_talons->trigger(
          as<int>( p()->talent.deathbringer.dark_talons->effectN( 2 ).base_value() ) );
    }

    p()->trigger_sanlayn_execute_talents( this->data().id() == p()->spell.vampiric_strike->id() );

    // For some reason, boiling point seems to be on a two roll system.
    // First roll checks only for first target
    if ( p()->talent.blood.boiling_point.ok() && rng().roll( boiling_point_proc_chance ) )
      p()->buffs.boiling_point->trigger();

    // Second roll, multiplies proc chance by number of targets -1
    if ( p()->talent.blood.boiling_point.ok() && p()->sim->target_non_sleeping_list.size() > 1 &&
            p()->rng().roll( boiling_point_proc_chance * ( p()->sim->target_non_sleeping_list.size() - 1 ) ) )
      p()->buffs.boiling_point->trigger();

    if ( p()->talent.blood.dance_of_midnight_1.ok() )
      p()->buffs.dance_of_midnight_1->expire();
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

  void reset() override
  {
    death_knight_melee_attack_t::reset();

    boiling_point_proc_attempts = 0;
  }

private:
  propagate_const<action_t*> leeching_strike;
  double heartbreaker_rp_gen;
  double boiling_point_proc_chance;
  int boiling_point_proc_attempts;
};

struct vampiric_strike_blood_t : public heart_strike_base_t
{
  vampiric_strike_blood_t( std::string_view n, death_knight_t* p, bool bloodied_blade_triggered )
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
        add_child( p->background_actions.infliction_of_sorrow );
      }
      // In game bloodied blade vamp strikes will proc this. But I have no desire to re-write this all right now, and
      // expect this to change due to how buggy it is.
      if ( p->talent.sanlayn.the_blood_is_life.ok() )
      {
        p->pets.blood_beast.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
        add_child( p->background_actions.the_blood_is_life );
      }
    }
  }

  void execute() override
  {
    heart_strike_base_t::execute();

    p()->trigger_drw_action( DRW_ACTION_VAMPIRIC_STRIKE );
  }
};

struct heart_strike_t : public heart_strike_base_t
{
  heart_strike_t( std::string_view n, death_knight_t* p, std::string_view options_str )
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

    p()->trigger_drw_action( DRW_ACTION_HEART_STRIKE );
  }

private:
  vampiric_strike_blood_t* vampiric_strike;
  double vampiric_strike_cost;
};

struct heart_strike_bloodied_blade_t : public death_knight_melee_attack_t
{
  heart_strike_bloodied_blade_t( std::string_view n, death_knight_t* p )
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
    return p()->buffs.death_and_decay->up() ? aoe + as<int>( p()->spell.dnd_buff->effectN( 3 ).base_value() ) : aoe;
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

// Howling Blast ============================================================

struct avalanche_t final : public death_knight_spell_t
{
  avalanche_t( std::string_view name, death_knight_t* p ) : death_knight_spell_t( name, p, p->spell.avalanche_damage )
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

struct howling_blades_t final : public death_knight_spell_t
{
  howling_blades_t( std::string_view name, death_knight_t* p, const spell_data_t* data )
    : death_knight_spell_t( name, p, data )
  {
    background = true;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( p()->rng().roll( p()->talent.frost.howling_blades->effectN( 1 ).percent() ) )
    {
      p()->trigger_killing_machine( false, p()->procs.km_from_howling_blades,
                                    p()->procs.km_from_howling_blades_wasted );
    }
  }
};

struct howling_blast_t final : public death_knight_spell_t
{
  howling_blast_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "howling_blast", p, p->talent.frost.howling_blast )
  {
    parse_options( options_str );

    aoe                 = -1;
    reduced_aoe_targets = 1.0;
    full_amount_targets = p->talent.frost.northwinds->ok() ? 2 : 1;

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
    if ( p->talent.frost.howling_blades.ok() )
    {
      first_howling_blades =
          get_action<howling_blades_t>( "howling_blades_first", p, p->spell.first_howling_blades_damage );
      add_child( first_howling_blades );
      second_howling_blades =
          get_action<howling_blades_t>( "howling_blades_second", p, p->spell.second_howling_blades_damage );
      add_child( second_howling_blades );
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

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( state );

    bool is_northwinds_target = p()->talent.frost.northwinds->ok() && state->chain_target == 1;

    if ( p()->buffs.rime->check() && ( state->chain_target == 0 || is_northwinds_target ) )
    {
      if ( p()->talent.frost.icebreaker.ok() )
      {
        m *= 1.0 + p()->talent.frost.icebreaker->effectN( 1 ).percent();
      }
      if ( p()->buffs.bind_in_darkness->check() )
      {
        m *= 1.0 + p()->talent.deathbringer.bind_in_darkness->effectN( 4 ).percent();
      }
    }
    if ( p()->talent.frost.everfrost->ok() && p()->buffs.rime->check() &&
         ( state->chain_target > 0 && !is_northwinds_target ) )
    {
      m *= 1.0 + p()->talent.frost.everfrost->effectN( 2 ).percent();
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

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( p()->talent.frost.cryogenic_chamber.ok() && p()->buffs.rime->up() )
    {
      if ( !p()->buffs.cryogenic_chamber->at_max_stacks() )
      {
        debug_cast<buffs::cryogenic_chamber_buff_t*>( p()->buffs.cryogenic_chamber )->damage +=
            state->result_amount * p()->talent.frost.cryogenic_chamber->effectN( 1 ).percent();
        p()->buffs.cryogenic_chamber->trigger();
      }
      else if ( p()->buffs.cryogenic_chamber->check() )
      {
        p()->buffs.cryogenic_chamber->refresh();
      }
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    if ( p()->buffs.pillar_of_frost->up() && p()->talent.frost.obliteration.ok() )
    {
      p()->trigger_killing_machine( true, p()->procs.km_from_obliteration_hb,
                                    p()->procs.km_from_obliteration_hb_wasted );

      // Obliteration's rune generation
      if ( rng().roll( p()->talent.frost.obliteration->effectN( 2 ).percent() ) )
      {
        p()->replenish_rune( as<int>( p()->spell.obliteration_gains->effectN( 1 ).base_value() ),
                             p()->gains.obliteration );
      }
    }

    if ( p()->buffs.rime->up() )
    {
      if ( p()->talent.frost.avalanche.ok() )
        avalanche->execute_on_target( target );

      if ( p()->talent.frost.rage_of_the_frozen_champion.ok() )
      {
        p()->resource_gain( RESOURCE_RUNIC_POWER,
                            p()->spell.rage_of_the_frozen_champion->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
                            p()->gains.rage_of_the_frozen_champion );
      }

      if ( p()->talent.frost.frostbound_will->ok() )
      {
        p()->cooldown.empower_rune_weapon->adjust( p()->talent.frost.frostbound_will->effectN( 1 ).time_value() );
      }

      if ( p()->talent.deathbringer.dark_talons.ok() && p()->talent.icy_talons->ok() &&
           rng().roll( p()->talent.deathbringer.dark_talons->effectN( 1 ).percent() ) )
      {
        p()->buffs.dark_talons_icy_talons->trigger(
            as<int>( p()->talent.deathbringer.dark_talons->effectN( 2 ).base_value() ) );
      }

      if ( p()->talent.frost.breath_of_sindragosa.ok() && p()->buffs.breath_of_sindragosa->check() )
      {
        timespan_t base_extension = p()->talent.frost.breath_of_sindragosa->effectN( 3 ).time_value();
        p()->buffs.breath_of_sindragosa->extend_duration( base_extension );
      }

      if ( p()->talent.frost.howling_blades->ok() )
      {
        make_event<delayed_execute_event_t>( *sim, p(), first_howling_blades, execute_state->target, 500_ms );
        make_event<delayed_execute_event_t>( *sim, p(), second_howling_blades, execute_state->target, 500_ms );
      }

      p()->buffs.rime->decrement();
    }
  }

private:
  propagate_const<action_t*> avalanche;
  propagate_const<action_t*> first_howling_blades;
  propagate_const<action_t*> second_howling_blades;
};

// Marrowrend ===============================================================

struct marrowrend_t final : public death_knight_melee_attack_t
{
  marrowrend_t( death_knight_t* p, std::string_view options_str )
    : death_knight_melee_attack_t( "marrowrend", p, p->talent.blood.marrowrend )
  {
    parse_options( options_str );
    weapon = &( p->main_hand_weapon );
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    p()->trigger_drw_action( DRW_ACTION_MARROWREND );

    if ( p()->talent.deathbringer.dark_talons.ok() && p()->talent.icy_talons->ok() &&
         rng().roll( p()->talent.deathbringer.dark_talons->effectN( 1 ).percent() ) )
    {
      p()->buffs.dark_talons_icy_talons->trigger(
          as<int>( p()->talent.deathbringer.dark_talons->effectN( 2 ).base_value() ) );
    }

    if ( p()->buffs.exterminate->up() )
    {
      p()->buffs.exterminate->decrement();
      make_event<delayed_execute_event_t>( *sim, p(), p()->background_actions.exterminate, execute_state->target,
                                           500_ms );
    }
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    p()->buffs.bone_shield->trigger( as<int>( data().effectN( 3 ).base_value() ) );
  }
};

// Mind Freeze ==============================================================

struct mind_freeze_t final : public death_knight_spell_t
{
  mind_freeze_t( death_knight_t* p, std::string_view options_str )
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
      p()->resource_gain( RESOURCE_RUNIC_POWER, p()->spell.coldthirst_gain->effectN( 1 ).base_value() * 0.1,
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
  obliterate_strike_t( std::string_view name, death_knight_t* p, weapon_t* w, const spell_data_t* s )
    : death_knight_melee_attack_t( name, p, s )
  {
    background = true;
    special    = true;
    may_miss   = false;
    weapon     = w;

    inexorable_assault = get_action<inexorable_assault_damage_t>( "inexorable_assault", p );
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_melee_attack_t::composite_da_multiplier( state );
    // The phsyical portion of Obliterate does not list Frozen Heart in it's list of affecting spells so needs manual intervention when frostreaper applies.
    if ( p()->spec.frostreaper->ok() && get_school() == SCHOOL_FROST &&
         !data().affected_by( p()->mastery.frozen_heart ) )
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
    if ( p()->spec.frostreaper->ok() && get_school() == SCHOOL_FROST )
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
      inexorable_assault->execute_on_target( state->target );
      p()->buffs.inexorable_assault->decrement();
      p()->cooldown.inexorable_assault_icd->start();
    }

    if ( p()->talent.frost.frostreaper->ok() && p()->rppm.frostreaper->trigger() )
    {
      auto td = get_td( state->target );
      td->debuff.frostreaper->trigger();

      std::vector<player_t*>& current_targets = target_list();
      int chains_remaining                    = p()->spell.frostreaper_debuff->effectN( 1 ).chain_target() - 1;
      for ( auto t : current_targets )
      {
        if ( chains_remaining <= 0 )
        {
          break;
        }
        if ( t != state->target )
        {
          get_td( t )->debuff.frostreaper->trigger();
          --chains_remaining;
        }
      }
    }

    if ( p()->talent.rider.trollbanes_icy_fury.ok() && td->debuff.chains_of_ice_trollbane_slow->check() &&
         p()->pets.trollbane.active_pet() != nullptr )
    {
      td->debuff.chains_of_ice_trollbane_slow->expire();
      p()->background_actions.trollbanes_icy_fury->execute_on_target( state->target );
    }
  }

private:
  propagate_const<action_t*> inexorable_assault;
};

struct obliterate_t final : public death_knight_melee_attack_t
{
  obliterate_t( death_knight_t* p, std::string_view options_str = {} )
    : death_knight_melee_attack_t( "obliterate", p, p->talent.frost.obliterate ),
      mh( nullptr ),
      oh( nullptr ),
      mh_delay( 0_ms ),
      oh_delay( 0_ms ),
      total_delay( 0_ms ),
      aa_action( p->background_actions.arctic_assault_obliterate )
  {
    parse_options( options_str );
    dual = true;

    const spell_data_t* mh_data =
        p->main_hand_weapon.group() == WEAPON_2H ? data().effectN( 4 ).trigger() : data().effectN( 2 ).trigger();

    const spell_data_t* frost_mh_data =
        p->main_hand_weapon.group() == WEAPON_2H ? data().effectN( 9 ).trigger() : data().effectN( 7 ).trigger();

    // Misc_value1 contains the delay in milliseconds for the spells being executed
    mh_delay = timespan_t::from_millis( as<int>( p->main_hand_weapon.group() == WEAPON_2H
                                                     ? data().effectN( 4 ).misc_value1()
                                                     : data().effectN( 2 ).misc_value1() ) );
    if ( p->off_hand_weapon.type != WEAPON_NONE )
      oh_delay = timespan_t::from_millis( as<int>( data().effectN( 3 ).misc_value1() ) );

    // Snag total delay to schedule Killing Machine for after the final hit
    total_delay = mh_delay + oh_delay;

    mh        = get_action<obliterate_strike_t>( "obliterate_damage", p, &( p->main_hand_weapon ), mh_data );
    mh->stats = stats;
    stats->action_list.push_back( mh );

    mh->execute_action =
        get_action<obliterate_strike_t>( "obliterate_frost", p, &( p->main_hand_weapon ), frost_mh_data );
    add_child( mh->execute_action );

    if ( p->off_hand_weapon.type != WEAPON_NONE )
    {
      oh = get_action<obliterate_strike_t>( "obliterate_offhand", p, &( p->off_hand_weapon ),
                                            data().effectN( 3 ).trigger() );
      add_child( oh );

      oh->execute_action = get_action<obliterate_strike_t>( "obliterate_offhand_frost", p, &( p->off_hand_weapon ),
                                                            data().effectN( 8 ).trigger() );
      add_child( oh->execute_action );
    }

    if ( p->talent.frost.arctic_assault.ok() )
      add_child( aa_action );
  }

  void execute() override
  {
    if ( p()->talent.rider.whitemanes_famine.ok() && p()->sim->target_non_sleeping_list.size() > 1 )
      p()->sort_undeath_targets( target_list() );

    death_knight_melee_attack_t::execute();

    if ( hit_any_target )
    {
      make_event<delayed_execute_event_t>( *sim, p(), mh, execute_state->target, mh_delay );
      if ( oh )
        make_event<delayed_execute_event_t>( *sim, p(), oh, execute_state->target, oh_delay );
    }

    if ( p()->buffs.exterminate->up() )
    {
      p()->buffs.exterminate->decrement();
      make_event<delayed_execute_event_t>( *sim, p(), p()->background_actions.exterminate, execute_state->target,
                                           500_ms );
    }

    if ( p()->talent.rider.let_terror_reign->ok() && p()->pets.trollbane.active_pet() != nullptr )
    {
      p()->pets.trollbane.active_pet()->obliterate->consumed_km = p()->buffs.killing_machine->up();
      p()->pets.trollbane.active_pet()->obliterate->execute_on_target( target );
    }

    if ( p()->talent.rider.whitemanes_famine.ok() && get_td( execute_state->target )->dot.undeath->is_ticking() )
      p()->trigger_whitemanes_famine( execute_state->target );

    if ( p()->buffs.killing_machine->up() )
      p()->consume_killing_machine( p()->procs.killing_machine_oblit, total_delay, aa_action );

    if ( p()->talent.frost.obliteration.ok() && p()->buffs.empower_rune_weapon->check() )
      p()->buffs.empower_rune_weapon->expire();

    if ( p()->sets->has_set_bonus( DEATH_KNIGHT_FROST, MID1, B2 ) )
      make_event( *sim, total_delay, [ this ]() { p()->buffs.empowered_strikes->consume( this, 1 ); } );
  }

  // Allow on-cast procs
  bool has_amount_result() const override
  {
    return true;
  }

private:
  action_t *mh, *oh;
  timespan_t mh_delay;
  timespan_t oh_delay;
  timespan_t total_delay;
  propagate_const<action_t*> aa_action;
};

// Outbreak ================================================================
struct outbreak_aoe_t final : public death_knight_spell_t
{
  outbreak_aoe_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.outbreak_aoe ),
      vp( get_action<virulent_plague_t>( "virulent_plague", p ) )
  {
    aoe        = -1;
    radius     = data().effectN( 1 ).radius_max();
    background = true;
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );
    vp->execute_on_target( s->target );
  }

private:
  propagate_const<action_t*> vp;
};

struct outbreak_t final : public death_knight_spell_t
{
  outbreak_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "outbreak", p, p->talent.unholy.outbreak )
  {
    parse_options( options_str );
    aoe = 0;

    if ( !p->talent.unholy.blightburst.ok() && !p->options.wcl_reporting_mode )
    {
      add_child( p->background_actions.virulent_plague );
      add_child( p->background_actions.dread_plague );
    }
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );
    p()->background_actions.outbreak_aoe->execute_on_target( s->target );
    p()->background_actions.dread_plague->execute_on_target( s->target );
  }
};

// Pillar of Frost ==========================================================
struct pillar_of_frost_t final : public death_knight_spell_t
{
  pillar_of_frost_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "pillar_of_frost", p, p->talent.frost.pillar_of_frost )
  {
    parse_options( options_str );

    harmful = false;
    target  = p;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->buffs.pillar_of_frost->trigger();

    if ( p()->talent.frost.frozen_dominion->ok() )
    {
      p()->background_actions.frozen_dominion_remorseless_winter->execute();
    }

    if ( p()->talent.rider.ride_or_die.ok() )
    {
      timespan_t dur = timespan_t::from_seconds( p()->talent.rider.ride_or_die->effectN( 1 ).base_value() );
      p()->summon_rider( dur, rider_of_the_apocalypse_e::TROLLBANE );
    }

    if ( p()->sets->has_set_bonus( DEATH_KNIGHT_FROST, MID1, B4 ) )
    {
      p()->buffs.frost_mid1_4pc_buff->trigger();
    }
  }
};

// Putrefy ==================================================================
struct putrefy_damage_base_t : public death_knight_spell_t
{
  putrefy_damage_base_t( std::string_view n, death_knight_t* p, const spell_data_t* s, putrefy_source_e ps )
    : death_knight_spell_t( n, p, s ), source( ps ), blightburst_dur( 0_s ), blightburst_mult( 1.0 )
  {
    if ( p->talent.unholy.blightburst.ok() )
    {
      blightburst_dur  = p->talent.unholy.blightburst->effectN( 1 ).time_value();
      blightburst_mult = p->talent.unholy.blightburst->effectN( 2 ).percent();
    }

    switch ( source )
    {
      case PUTREFY_SOURCE_FORBIDDEN_KNOWLEDGE:
        blightburst_dur *= p->talent.unholy.forbidden_knowledge_3->effectN( 2 ).percent();
        base_multiplier *= p->talent.unholy.forbidden_knowledge_3->effectN( 2 ).percent();
        break;
      default:
        break;
    }
  }

  void set_source( putrefy_source_e s )
  {
    source = s;
  }

  void trigger_blightburst( player_t* t, death_knight_td_t* td, dot_t* dot )
  {
    if ( !dot->is_ticking() )
    {
      action_t* dot_applicator =
          dot == td->dot.dread_plague ? p()->background_actions.dread_plague : p()->background_actions.virulent_plague;
      dot_applicator->execute_on_target( t );
    }
    else
    {
      action_t* background_action = nullptr;
      if ( !p()->options.wcl_reporting_mode )
      {
        if ( dot == td->dot.dread_plague )
          background_action = p()->background_actions.dread_plague_erupt_bb;
        else
          background_action = p()->background_actions.virulent_plague_erupt_bb;
      }
      else
      {
        if ( dot == td->dot.dread_plague )
          background_action = p()->background_actions.dread_plague_erupt;
        else
          background_action = p()->background_actions.virulent_plague_erupt;
      }
      background_action->execute_on_target( t, dot->tick_damage_over_time( blightburst_dur ) * blightburst_mult );

      dot->adjust_duration( blightburst_dur );
    }
  }

public:
  putrefy_source_e source;
  timespan_t blightburst_dur;
  double blightburst_mult;
};

struct putrefy_aoe_t : public putrefy_damage_base_t
{
  putrefy_aoe_t( std::string_view n, death_knight_t* p, putrefy_source_e s )
    : putrefy_damage_base_t( n, p, p->spell.putrefy_aoe, s )
  {
    aoe        = -1;
    radius     = data().effectN( 1 ).radius_max();
    background = true;
  }

  void impact( action_state_t* s ) override
  {
    putrefy_damage_base_t::impact( s );
    if ( p()->talent.unholy.blightburst.ok() )
    {
      death_knight_td_t* td = p()->get_target_data( s->target );
      trigger_blightburst( s->target, td, td->dot.virulent_plague );
    }

    p()->trigger_rune_of_the_apocalypse( s->target );
  }
};

struct putrefy_fk_aoe_t : public putrefy_aoe_t
{
  putrefy_fk_aoe_t( std::string_view n, death_knight_t* p ) : putrefy_aoe_t( n, p, PUTREFY_SOURCE_FORBIDDEN_KNOWLEDGE )
  {
  }
};

struct putrefy_st_t : public putrefy_damage_base_t
{
  putrefy_st_t( std::string_view n, death_knight_t* p, putrefy_source_e s )
    : putrefy_damage_base_t( n, p, p->spell.putrefy_st, s )
  {
    background         = true;
    cooldown->duration = 0_ms;;
    execute_action = p->background_actions.putrefy_aoe;
  }

  void impact( action_state_t* s ) override
  {
    putrefy_damage_base_t::impact( s );

    if ( p()->talent.unholy.blightburst.ok() )
    {
      death_knight_td_t* td = p()->get_target_data( s->target );
      trigger_blightburst( s->target, td, td->dot.dread_plague );
    }
  }
};

struct putrefy_fk_st_t : public putrefy_st_t
{
  putrefy_fk_st_t( std::string_view n, death_knight_t* p ) : putrefy_st_t( n, p, PUTREFY_SOURCE_FORBIDDEN_KNOWLEDGE )
  {
    execute_action = p->background_actions.putrefy_fk_aoe;
  }
};

struct putrefy_t final : public death_knight_spell_t
{
  putrefy_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "putrefy", p, p->talent.unholy.putrefy )
  {
    parse_options( options_str );

    cooldown = p->cooldown.putrefy;

    if ( p->talent.unholy.reanimation.ok() )
    {
      p->pets.reanimation_magus.set_creation_event_callback(
          pets::parent_pet_action_fn( p->pet_summon.reanimation_magus ) );
      add_child( p->pet_summon.reanimation_magus );
    }

    p->pets.lesser_ghoul_putrefy.set_creation_event_callback(
        pets::parent_pet_action_fn( p->pet_summon.putrefy_ghoul ) );
    add_child( p->pet_summon.putrefy_ghoul );
  }

  void init_finished() override
  {
    death_knight_spell_t::init_finished();
    add_child( p()->background_actions.putrefy_st );
    add_child( p()->background_actions.putrefy_aoe );

    if ( p()->talent.unholy.blightburst.ok() && !p()->options.wcl_reporting_mode )
    {
      add_child( p()->background_actions.virulent_plague );
      add_child( p()->background_actions.dread_plague );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p()->pet_summon.putrefy_ghoul->execute();

    if ( p()->talent.unholy.putrid_echoes.ok() && std::floor( cooldown->charges_fractional() ) > 1 )
    {
      p()->pet_summon.putrefy_ghoul->execute();
      cooldown->start( this );
    }
  }
};

// Raise Dead ===============================================================

struct raise_dead_t final : public death_knight_summon_spell_t
{
  raise_dead_t( death_knight_t* p, std::string_view options_str )
    : death_knight_summon_spell_t( "raise_dead", p,
                                   p->spec.raise_dead->ok() ? p->spec.raise_dead : p->talent.raise_dead )
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
      p->pets.risen_skulker.set_creation_event_callback( pets::parent_pet_action_fn( p->pet_summon.raise_skulker ) );
      add_child( p->pet_summon.raise_skulker );
    }
  }

  void execute() override
  {
    death_knight_summon_spell_t::execute();

    // If the action is done in precombat and the pet is permanent
    // Assume that the player used it long enough before pull that the cooldown is ready again
    if ( is_precombat && p()->spec.raise_dead->ok() )
    {
      cooldown->reset( false );
    }

    // Summon for the duration specified in spelldata if there's one (no data = permanent pet)
    p()->pets.ghoul_pet.spawn( data().duration() );

    if ( p()->talent.unholy.all_will_serve.ok() && p()->pets.risen_skulker.active_pet() == nullptr )
      p()->pet_summon.raise_skulker->execute();
  }

  bool ready() override
  {
    if ( p()->pets.ghoul_pet.active_pet() != nullptr )
    {
      return false;
    }

    return death_knight_summon_spell_t::ready();
  }
};

// Remorseless Winter =======================================================

struct remorseless_winter_damage_t final : public death_knight_spell_t
{
  remorseless_winter_damage_t( std::string_view n, death_knight_t* p, const spell_data_t* data )
    : death_knight_spell_t( n, p, data ),
      biting_cold_target_threshold( 0 ),
      triggered_biting_cold( false ),
      triggered_frozen_dominion( false ),
      frozen_dominion_impacted{ true }
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

    if ( p->talent.frost.frozen_dominion.ok() )
      p->register_on_kill_callback( [ & ]( player_t* t ) { clear_frozen_dominion_impacted( t ); } );
  }

  void reset() override
  {
    death_knight_spell_t::reset();
    clear_state();
  }

  void clear_state()
  {
    triggered_biting_cold = false;
    if ( p()->talent.frost.frozen_dominion.ok() )
    {
      triggered_frozen_dominion = false;
      for ( auto spawn_index : frozen_dominion_impacted.get_entries() )
      {
        if ( spawn_index != nullptr )
          *spawn_index = false;
      }
    }
  }

  void clear_frozen_dominion_impacted( const player_t* target )
  {
    if ( frozen_dominion_impacted[ target ] != nullptr )
      *frozen_dominion_impacted[ target ] = false;
  }

  bool mark_frozen_dominion_impacted( const player_t* target )
  {
    if ( frozen_dominion_impacted[ target ] != nullptr )
    {
      if ( *frozen_dominion_impacted[ target ] )
        return false;
    }
    else
    {
      frozen_dominion_impacted[ target ] = new bool;
    }

    *frozen_dominion_impacted[ target ] = true;
    return true;
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    if ( hit_any_target && p()->sets->has_set_bonus( DEATH_KNIGHT_FROST, MID2, B2 ) )
    {
      p()->buffs.freezing_tempest->trigger();
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

    if ( p()->talent.frost.frozen_dominion.ok() &&
         ( !triggered_frozen_dominion || p()->buffs.frozen_dominion->check() ) &&
         mark_frozen_dominion_impacted( state->target ) )
    {
      triggered_frozen_dominion = true;
      p()->buffs.frozen_dominion->trigger();
    }
  }

private:
  double biting_cold_target_threshold;
  bool triggered_biting_cold;
  bool triggered_frozen_dominion;
  target_specific_t<bool> frozen_dominion_impacted;
};

struct remorseless_winter_base_t : public death_knight_spell_t
{
  remorseless_winter_base_t( std::string_view name, death_knight_t* p, const spell_data_t* data,
                             buff_t* remorseless_winter_buff )
    : death_knight_spell_t( name, p, data ),
      damage( p->background_actions.remorseless_winter_tick ),
      remorseless_winter_buff( remorseless_winter_buff )
  {
    may_miss = may_dodge = may_parry = false;

    // Periodic behavior handled by the buff
    dot_duration = base_tick_time = 0_ms;

    if ( data->ok() )
    {
      add_child( damage );

      if ( p->talent.frost.cryogenic_chamber.ok() )
        add_child( get_action<cryogenic_chamber_t>( "cryogenic_chamber", p ) );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    debug_cast<remorseless_winter_damage_t*>( damage )->clear_state();
    if ( p()->sets->has_set_bonus( DEATH_KNIGHT_FROST, MID2, B2 ) )
    {
      p()->buffs.freezing_tempest->expire();
    }
    remorseless_winter_buff->trigger();

    if ( p()->talent.frost.cryogenic_chamber.ok() && p()->buffs.cryogenic_chamber->check() )
    {
      p()->buffs.cryogenic_chamber->expire();
    }
  }

private:
  action_t*& damage;
  buff_t* remorseless_winter_buff;
};

struct frozen_dominion_remorseless_winter_t final : public remorseless_winter_base_t
{
  frozen_dominion_remorseless_winter_t( std::string_view name, death_knight_t* p )
    : remorseless_winter_base_t( name, p, p->spell.frozen_dominion_remorseless_winter_buff,
                                 p->buffs.frozen_dominion_remorseless_winter )
  {
    background = true;
  }
};

struct remorseless_winter_t final : public remorseless_winter_base_t
{
  remorseless_winter_t( death_knight_t* p, std::string_view options_str )
    : remorseless_winter_base_t( "remorseless_winter", p, p->spec.remorseless_winter, p->buffs.remorseless_winter )
  {
    parse_options( options_str );
  }
};

// Sanguinary Burst =========================================================
struct sanguinary_burst_t : public death_knight_spell_t
{
  sanguinary_burst_t( std::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p->spell.sanguinary_burst_damage )
    {
      background = true;
      aoe = data().max_targets();
    }

    void execute() override
    {
      death_knight_spell_t::execute();

      p()->buffs.sanguinary_burst->expire();
    }
};

// Scourge Strike and Clawing Shadows =======================================

struct scourge_strike_base_t : public death_knight_melee_attack_t
{
  scourge_strike_base_t( std::string_view name, death_knight_t* p, const spell_data_t* spell )
    : death_knight_melee_attack_t( name, p, spell ), summon_ghoul( nullptr ), errupt_mult( 1.0 )
  {
    errupt_mult = p->talent.unholy.scourge_strike->effectN( 2 ).percent();

    if ( p->talent.unholy.scourging.ok() )
      errupt_mult += p->talent.unholy.scourging->effectN( 1 ).percent();

    weapon = &( player->main_hand_weapon );
    aoe    = 1;

    summon_ghoul = p->pet_summon.fs_ghoul;
  }

  int n_targets() const override
  {
    return p()->buffs.clawing_shadows->check() ? aoe + as<int>( p()->buffs.clawing_shadows->check_stack_value() ) : aoe;
  }

  std::vector<player_t*>& target_list() const override
  {
    std::vector<player_t*>& current_targets = death_knight_melee_attack_t::target_list();
    // Don't bother ordering the list if all the valid targets will be hit
    if ( current_targets.size() <= as<size_t>( n_targets() ) )
      return current_targets;

    // first target, the action target, needs to be left in place
    std::sort( current_targets.begin() + 1, current_targets.end(), [ this ]( player_t* a, player_t* b ) {
      return get_td( a )->dot.virulent_plague->is_ticking() && !get_td( b )->dot.virulent_plague->is_ticking();
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

    trigger_disease_effects( state, td, td->dot.virulent_plague );
    trigger_disease_effects( state, td, td->dot.dread_plague );

    if ( !td->dot.virulent_plague->is_ticking() )
      p()->background_actions.virulent_plague->execute_on_target( state->target );

    if ( p()->talent.rider.trollbanes_icy_fury.ok() && td->debuff.chains_of_ice_trollbane_slow->check() &&
         p()->pets.trollbane.active_pet() != nullptr )
    {
      td->debuff.chains_of_ice_trollbane_slow->expire();
      p()->background_actions.trollbanes_icy_fury->execute_on_target( state->target );
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
      p()->sort_undeath_targets( target_list() );

    death_knight_melee_attack_t::execute();
    if ( was_replaced )
      return;

    bool summoned_ghoul = false;

    if ( p()->buffs.lesser_ghoul_ready->check() )
    {
      p()->buffs.lesser_ghoul_ready->consume( this, 1 );
      summon_ghoul->execute();
      summoned_ghoul = true;

      if ( p()->talent.unholy.harbinger_of_doom.ok() )
        p()->cooldown.putrefy->adjust( -p()->talent.unholy.harbinger_of_doom->effectN( 3 ).time_value(), false );
    }

    if ( p()->talent.unholy.clawing_shadows.ok() )
      p()->buffs.clawing_shadows->trigger();

    p()->trigger_sanlayn_execute_talents( this->data().id() == p()->spell.vampiric_strike->id(), summoned_ghoul );
  }

  void trigger_disease_effects( const action_state_t* s, const death_knight_td_t* td, dot_t* dot )
  {
    if ( !dot->is_ticking() || s->target->is_sleeping() )
      return;

    timespan_t dur = dot->current_action->tick_time( s );

    double dam = dot->tick_damage_over_time( dur ) * errupt_mult;

    if ( dot == td->dot.virulent_plague )
    {
      if ( !p()->options.wcl_reporting_mode )
        p()->background_actions.virulent_plague_erupt_ss->execute_on_target( s->target, dam );
      else
        p()->background_actions.virulent_plague_erupt->execute_on_target( s->target, dam );
      for ( auto& target : target_list() )
      {
        if ( get_td( target )->dot.virulent_plague->is_ticking() )
          continue;

        td->dot.virulent_plague->copy( target, DOT_COPY_CLONE );
        return;
      }
    }
    else
    {
      if ( !p()->options.wcl_reporting_mode )
        p()->background_actions.dread_plague_erupt_ss->execute_on_target( s->target, dam );
      else
        p()->background_actions.dread_plague_erupt->execute_on_target( s->target, dam );
    }
  }

private:
  action_t* summon_ghoul;

public:
  double errupt_mult;
};

struct vampiric_strike_unholy_t : public scourge_strike_base_t
{
  vampiric_strike_unholy_t( std::string_view n, death_knight_t* p )
    : scourge_strike_base_t( n, p, p->spell.vampiric_strike )
  {
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
    energize_amount         = std::fabs( data().powerN( 3 ).cost() );

    if ( p->talent.sanlayn.infliction_of_sorrow.ok() )
      errupt_mult *= 1.0 + p->talent.sanlayn.infliction_of_sorrow->effectN( 6 ).percent();
  }
};

struct scourge_strike_t final : public scourge_strike_base_t
{
  scourge_strike_t( std::string_view n, death_knight_t* p, std::string_view options_str )
    : scourge_strike_base_t( n, p, p->talent.unholy.scourge_strike )
  {
    parse_options( options_str );

    if ( p->talent.sanlayn.vampiric_strike.ok() )
      set_replacement_action( new vampiric_strike_unholy_t( "vampiric_strike", p ), p->buffs.vampiric_strike );

    if ( data().ok() )
    {
      p->pets.lesser_ghoul_fs.set_creation_event_callback( pets::parent_pet_action_fn( p->pet_summon.fs_ghoul ) );
      add_child( p->pet_summon.fs_ghoul );

      if ( !p->options.wcl_reporting_mode )
      {
        add_child( p->background_actions.virulent_plague_erupt_ss );
        add_child( p->background_actions.dread_plague_erupt_ss );
      }
    }

    if ( p->talent.sanlayn.infliction_of_sorrow.ok() )
      add_child( p->background_actions.infliction_of_sorrow );
  }
};

// Soul Reaper ==============================================================
struct soul_reaper_t final : public death_knight_spell_t
{
  soul_reaper_t( death_knight_t* p, std::string_view options_str )
    : death_knight_spell_t( "soul_reaper", p, p->talent.unholy.soul_reaper )
  {
    parse_options( options_str );
    p->cooldown.soul_reaper = cooldown;
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    p()->buffs.reaping->decrement();
    int charges = std::min( 2, as<int>( std::floor( p()->cooldown.putrefy->charges_fractional() ) ) );
    for ( int i = 0; i < charges; i++ )
    {
      p()->pet_summon.sr_ghoul->execute();
      p()->cooldown.putrefy->start( nullptr );
    }
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );
    get_td( s->target )->debuff.soul_reaper->trigger();
  }

  bool target_ready( player_t* tar ) override
  {
    if ( tar->health_percentage() > data().effectN( 2 ).base_value() && !p()->buffs.reaping->check() )
      return false;

    return death_knight_spell_t::target_ready( tar );
  }
};

// ==========================================================================
// Death Knight Defensive Abilities
// ==========================================================================

// Anti-Magic Shell ========================================================
struct antimagic_shell_t final : public death_knight_absorb_t
{
  antimagic_shell_t( death_knight_t* p, std::string_view options_str )
    : death_knight_absorb_t( "antimagic_shell", p, p->spec.antimagic_shell ),
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
    absorb_buff = p->buffs.antimagic_shell;

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

    death_knight_absorb_t::execute();
  }

private:
  double min_interval;
  double interval;
  double interval_stddev;
  double interval_stddev_opt;
  double damage;
};

// Anti-Magic Zone =========================================================
struct antimagic_zone_t final : public death_knight_absorb_t
{
  antimagic_zone_t( death_knight_t* p, std::string_view options_str )
    : death_knight_absorb_t( "antimagic_zone", p, p->talent.antimagic_zone )
  {
    harmful = may_crit = may_miss = false;
    base_dd_min = base_dd_max = 0;
    target                    = p;
    parse_options( options_str );
    absorb_buff = p->buffs.antimagic_zone;
  }
};

// Icebound Fortitude =======================================================

struct icebound_fortitude_t final : public death_knight_spell_t
{
  icebound_fortitude_t( death_knight_t* p, std::string_view options_str )
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

// Vampiric Blood ===========================================================
struct vampiric_blood_t final : public death_knight_spell_t
{
  vampiric_blood_t( death_knight_t* p, std::string_view options_str )
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

// ==========================================================================
// Death Knight Proc Callbacks
// ==========================================================================
struct death_knight_proc_callback_t : public dbc_proc_callback_t
{
  death_knight_proc_callback_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
  {
    initialize();
    activate();
  }

  death_knight_t* p() const
  {
    return debug_cast<death_knight_t*>( listener );
  }
  death_knight_t* p()
  {
    return debug_cast<death_knight_t*>( listener );
  }
};

void runic_attenuation_proc( const special_effect_t& e )
{
  struct runic_attenuation_proc : public death_knight_proc_callback_t
  {
    runic_attenuation_proc( const special_effect_t& e ) : death_knight_proc_callback_t( e )
    {
    }

    void execute( const spell_data_t*, player_t*, action_state_t* s ) override
    {
      p()->resource_gain(
          RESOURCE_RUNIC_POWER,
          p()->talent.runic_attenuation->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
          p()->gains.runic_attenuation, s->action );
    }
  };

  new runic_attenuation_proc( e );
}

}  // UNNAMED NAMESPACE

// Runeforges ===============================================================
void runeforge::fallen_crusader( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "unholy_strength" } ) )
  {
    return;
  }

  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* dk = debug_cast<death_knight_t*>( effect.player );
  auto buff          = buff_t::find( effect.player, "unholy_strength" );
  if ( !buff )
  {
    buff = make_buff( dk, "unholy_strength", dk->runeforge_spell.unholy_strength )
               ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
               ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );
  }

  effect.custom_buff    = buff;
  effect.execute_action = get_action<fallen_crusader_heal_t>( "unholy_strength", dk );

  new dbc_proc_callback_t( effect.player, effect );
}

void runeforge::razorice( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  effect.proc_flags2_ = PF2_ALL_HIT;
  effect.proc_chance_ = 1.01;  // Always proc, as the proc chance is 100%

  death_knight_t* dk = debug_cast<death_knight_t*>( effect.player );
  switch ( effect.item->slot )
  {
    case SLOT_MAIN_HAND:
      effect.execute_action = get_action<razorice_attack_t>( "razorice_mh", dk );
      effect.proc_flags_    = PF_MAINHAND | PF_MELEE_ABILITY | PF_MELEE;
      break;
    case SLOT_OFF_HAND:
      effect.execute_action = get_action<razorice_attack_t>( "razorice_oh", dk );
      effect.proc_flags_    = PF_OFFHAND | PF_MELEE_ABILITY | PF_MELEE;
      break;
    default:
      break;
  }

  new dbc_proc_callback_t( effect.player, effect );
}

void runeforge::stoneskin_gargoyle( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }
}

void runeforge::apocalypse( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }
}

void runeforge::sanguination( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* dk = debug_cast<death_knight_t*>( effect.player );

  effect.player->callbacks.register_callback_trigger_function(
      effect.spell_id, dbc_proc_callback_t::trigger_fn_type::CONDITION,
      [ effect ]( const dbc_proc_callback_t*, const proc_data_t&, player_t*, const action_state_t*,
                  proc_trigger_type_e ) {
        return effect.player->health_percentage() > effect.driver()->effectN( 1 ).base_value();
      } );

  effect.proc_flags2_   = PF2_ALL_HIT;
  effect.execute_action = get_action<sanguination_heal_t>( "rune_of_sanguination", dk );
  effect.cooldown_      = dk->runeforge_spell.sanguination_cooldown->duration();
  new dbc_proc_callback_t( effect.player, effect );
}

void runeforge::spellwarding( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* dk = debug_cast<death_knight_t*>( effect.player );

  effect.execute_action = get_action<spellwarding_absorb_t>( "rune_of_spellwarding", dk );

  new dbc_proc_callback_t( effect.player, effect );
}

void runeforge::unending_thirst( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* dk = debug_cast<death_knight_t*>( effect.player );

  auto buff = make_buff( dk, "rune_of_unending_thirst", dk->runeforge_spell.unending_thirst )
                  ->set_default_value_from_effect_type( A_HASTE_ALL )
                  ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  auto heal = get_action<unending_thirst_heal_t>( "rune_of_unending_thirst", dk );

  dk->register_on_kill_callback( [ buff, heal ]( player_t* ) {
    buff->trigger();
    heal->execute();
  } );
}

void runeforge::init_runeforges()
{
  unique_gear::register_special_effect( 50401, runeforge::razorice );
  unique_gear::register_special_effect( 166441, runeforge::fallen_crusader, true );
  unique_gear::register_special_effect( 62157, runeforge::stoneskin_gargoyle );
  unique_gear::register_special_effect( 327087, runeforge::apocalypse );
  unique_gear::register_special_effect( 326801, runeforge::sanguination );
  unique_gear::register_special_effect( 326864, runeforge::spellwarding );
  unique_gear::register_special_effect( 326982, runeforge::unending_thirst );
}

// Resource Manipulation ====================================================

double death_knight_t::resource_gain( resource_e resource_type, double amount, gain_t* g, action_t* action )
{
  double actual_amount = player_t::resource_gain( resource_type, amount, g, action );

  if ( resource_type == RESOURCE_RUNE && talent.deathbringer.rune_carved_plates.ok() && amount > 0 )
  {
    buffs.rune_carved_plates_physical_buff->trigger( as<int>( amount ) );
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

    if ( talent.deathbringer.rune_carved_plates.ok() && amount > 0 )
    {
      buffs.rune_carved_plates_magical_buff->trigger( as<int>( amount ) );
    }

    if ( talent.blood.dance_of_midnight_3.ok() )
    {
      if ( pseudo_random.dance_of_midnight->trigger() )
        pets.dance_of_midnight_pet.spawn();
    }
  }

  // Procs from runes spent
  if ( resource_type == RESOURCE_RUNE && action )
  {
    int base_cost = as<int>( action->base_costs[ RESOURCE_RUNE ].base );
    // Gathering Storm triggers a stack and extends RW duration by 0.5s
    // for each spell cast that normally consumes a rune (even if it ended up free)
    // But it doesn't count the original Relentless Winter cast
    if ( talent.frost.gathering_storm.ok() &&
         ( buffs.remorseless_winter->check() || buffs.frozen_dominion_remorseless_winter->check() ) &&
         action->data().id() != spec.remorseless_winter->id() )
    {
      buffs.gathering_storm->trigger( base_cost );
      timespan_t base_extension =
          timespan_t::from_seconds( talent.frost.gathering_storm->effectN( 1 ).base_value() * 0.1 );
      buffs.remorseless_winter->extend_duration( base_extension * base_cost );
      buffs.frozen_dominion_remorseless_winter->extend_duration( base_extension * base_cost );
    }

    if ( talent.rune_mastery.ok() )
      buffs.rune_mastery->trigger();

    // Scourge Strike with blighted uses base cost rather than actual spend.
    if ( talent.rider.nazgrims_conquest.ok() && buffs.apocalyptic_conquest->check() &&
         action->data().id() == talent.unholy.scourge_strike->id() )
    {
      debug_cast<buffs::apocalyptic_conquest_buff_t*>( buffs.apocalyptic_conquest )->nazgrims_conquest += base_cost;
      invalidate_cache( CACHE_STRENGTH );
    }

    // Proc Chance does not appear to be in data, using testing data that is current as of 4/19/2024
    if ( talent.rider.riders_champion.ok() && rng().roll( 0.2 ) )
      summon_rider( spell.summon_whitemane_2->duration() );

    // Effects that require the player to actually spend runes
    if ( actual_amount > 0 )
    {
      std::array<unsigned, 2> ignored_actions = { spec.remorseless_winter->id(), talent.unholy.scourge_strike->id() };
      bool uses_actual_amount = action && !range::contains( ignored_actions, action->data().id() );
      if ( talent.rider.nazgrims_conquest.ok() && buffs.apocalyptic_conquest->check() && uses_actual_amount )
      {
        debug_cast<buffs::apocalyptic_conquest_buff_t*>( buffs.apocalyptic_conquest )->nazgrims_conquest +=
            as<int>( actual_amount );
        invalidate_cache( CACHE_STRENGTH );
      }
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

    // If an action is linked, fetch its base cost.
    if ( action )
      base_rp_cost = action->base_costs[ RESOURCE_RUNIC_POWER ].base;

    double calc_rp_cost = std::max( base_rp_cost, actual_amount );

    // 2020-12-16 - Melekus: Based on testing with both Frost Strike and Breath of Sindragosa during Hypothermic
    // Presence, RE is using the ability's base cost for its proc chance calculation, just like Runic Corruption
    // 2025-07-28 If an ability costs more than its base_cost, RE takes the higher cost.
    trigger_runic_empowerment( calc_rp_cost );
    trigger_runic_corruption( procs.rp_runic_corruption, calc_rp_cost, false );

    if ( talent.unholy.summon_gargoyle.ok() && pets.gargoyle.active_pet() != nullptr )
    {
      // 2019-02-12 Death Strike buffs gargoyle as if it had its full cost, even though it received a -10rp cost in 8.1
      // Assuming it's a bug for now, using base_costs instead of last_resource_cost won't affect free spells here
      // Free Death Coils are still handled in the action
      pets.gargoyle.active_pet()->increase_power( calc_rp_cost );
    }

    if ( talent.icy_talons.ok() )
    {
      buffs.icy_talons->trigger();
    }

    if ( talent.rider.fury_of_the_horsemen.ok() )
    {
      if ( pets.whitemane.active_pet() != nullptr )
        extend_rider( calc_rp_cost, pets.whitemane.active_pet() );
      if ( pets.mograine.active_pet() != nullptr )
        extend_rider( calc_rp_cost, pets.mograine.active_pet() );
      if ( pets.nazgrim.active_pet() != nullptr )
        extend_rider( calc_rp_cost, pets.nazgrim.active_pet() );
      if ( pets.trollbane.active_pet() != nullptr )
        extend_rider( calc_rp_cost, pets.trollbane.active_pet() );
    }

    if ( talent.unholy.ancient_power.ok() )
      buffs.ancient_power->trigger();

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

      if ( talent.blood.sanguinary_burst.ok() && buffs.blood_mist->check() )
        buffs.sanguinary_burst->trigger( as<int>( final_spend ) );
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
  add_option( opt_float( "deathknight.ams_absorb_percent", options.ams_absorb_percent, 0.0, 1.0 ) );
  add_option( opt_bool( "deathknight.individual_pet_reporting", options.individual_pet_reporting ) );
  add_option( opt_float( "deathknight.average_cs_travel_time", options.average_cs_travel_time, 0.0, 5.0 ) );
  add_option(
      opt_timespan( "deathknight.first_ams_cast", options.first_ams_cast, timespan_t::zero(), timespan_t::max() ) );
  add_option( opt_float( "deathknight.horsemen_ams_absorb_percent", options.horsemen_ams_absorb_percent, 0.0, 1.0 ) );
  add_option(
      opt_float( "deathknight.average_mograines_might_uptime", options.average_mograines_might_uptime, 0.0, 1.0 ) );
  add_option( opt_bool( "deathknight.extra_unholy_reporting", options.extra_unholy_reporting ) );
  add_option( opt_bool( "deathknight.wcl_reporting_mode", options.wcl_reporting_mode ) );
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

  if ( dk.options.extra_unholy_reporting )
  {
    if ( talent.unholy.commander_of_the_dead.ok() )
      sample_data.lesser_ghoul_duration->merge( *dk.sample_data.lesser_ghoul_duration );

    sample_data.lesser_ghouls_summoned->merge( *dk.sample_data.lesser_ghouls_summoned );
    sample_data.lesser_ghouls_active->merge( *dk.sample_data.lesser_ghouls_active );
    sample_data.magus_active->merge( *dk.sample_data.magus_active );
    if ( talent.unholy.blightfall.ok() )
    {
      sample_data.blightfall_dp_dur->merge( *dk.sample_data.blightfall_dp_dur );
      sample_data.blightfall_vp_dur->merge( *dk.sample_data.blightfall_vp_dur );
    }
  }
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
  if ( options.extra_unholy_reporting )
  {
    if ( talent.unholy.commander_of_the_dead.ok() )
      sample_data.lesser_ghoul_duration->analyze();

    sample_data.lesser_ghouls_summoned->analyze();
    sample_data.lesser_ghouls_active->analyze();
    sample_data.magus_active->analyze();

    if ( talent.unholy.blightfall.ok() )
    {
      sample_data.blightfall_dp_dur->analyze();
      sample_data.blightfall_vp_dur->analyze();
    }
  }
}

bool death_knight_t::in_death_and_decay() const
{
  if ( talent.rider.mograines_might.ok() && buffs.mograines_might->check() )
    return true;

  if ( !sim->distance_targeting_enabled )
    return !active_dnds.empty();

  return get_ground_aoe_distance( *active_dnds.front()->pulse_state) <= active_dnds.front()->pulse_state->action->radius;
}

unsigned death_knight_t::replenish_rune( unsigned n, gain_t* gain )
{
  unsigned replenished = 0;

  while ( n-- )
  {
    rune_t* rune = _runes.first_depleted_rune();
    if ( !rune )
      rune = _runes.first_regenerating_rune();

    if ( !rune && gain )
      gain->add( RESOURCE_RUNE, 0, 1 );
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

// Helper function to trigger Killing Machine
void death_knight_t::trigger_killing_machine( bool predictable, proc_t* proc, proc_t* wasted_proc )
{
  bool wasted = buffs.killing_machine->at_max_stacks();

  buffs.killing_machine->trigger();

  if ( predictable )
    buffs.killing_machine->predict();

  // Use procs to track sources and wastes
  if ( !wasted )
    proc->occur();
  else
    wasted_proc->occur();
}

void death_knight_t::consume_killing_machine( proc_t* proc, timespan_t total_delay, action_t* aa_action )
{
  // Killing Machine is consumed shortly after casting Obliterate.
  make_event( sim, total_delay, [ this, aa_action, proc ] {
    if ( !buffs.killing_machine->check() )
      return;

    const int decrement_count = talent.frost.killing_streak.ok() ? buffs.killing_machine->check() : 1;
    buffs.killing_machine->decrement( decrement_count );

    if ( talent.frost.killing_streak.ok() )
      buffs.killing_streak->trigger( decrement_count );

    if ( talent.frost.breath_of_sindragosa.ok() && buffs.breath_of_sindragosa->check() )
    {
      timespan_t base_extension = talent.frost.breath_of_sindragosa->effectN( 3 ).time_value();
      buffs.breath_of_sindragosa->extend_duration( base_extension * decrement_count );
    }


    trigger_bonegrinder( decrement_count );

    if ( talent.frost.arctic_assault.ok() )
    {
      // Arctic Assault fires on a delay after consuming Killing Machine.
      // Uncertain from logs if its tied to the Obliterate execute or the consumption, leaving it here for now.
      make_event( *sim, 500_ms, [ aa_action, decrement_count ]() {
        aa_action->base_multiplier *= decrement_count;
        aa_action->execute();
        aa_action->base_multiplier /= decrement_count;
      } );
    }

    for ( int i = decrement_count; i > 0; --i )
    {
      proc->occur();

      if ( rng().roll( talent.frost.murderous_efficiency->effectN( 1 ).percent() ) )
      {
        replenish_rune( as<int>( spell.murderous_efficiency_gain->effectN( 1 ).base_value() ),
                        gains.murderous_efficiency );
      }
    }

    if ( talent.deathbringer.dark_talons.ok() && talent.icy_talons->ok() &&
         rng().roll( talent.deathbringer.dark_talons->effectN( 1 ).percent() ) )
    {
      buffs.dark_talons_icy_talons->trigger( as<int>( talent.deathbringer.dark_talons->effectN( 2 ).base_value() ) );
    }
  } );
}

void death_knight_t::trigger_runic_empowerment( double rpcost )
{
  if ( !spec.frost_death_knight->ok() )
    return;

  double base_chance = spell.runic_empowerment_chance->effectN( 1 ).percent() * 0.1;

  if ( !rng().roll( base_chance * rpcost ) )
    return;

  int regenerated = replenish_rune( as<int>( spell.runic_empowerment_gain->effectN( 1 ).resource( RESOURCE_RUNE ) ),
                                    gains.runic_empowerment );

  sim->print_debug( "{} Runic Empowerment regenerated {} rune", name(), regenerated );
  log_rune_status( this );

  if ( talent.frost.frigid_executioner->ok() )
  {
    if ( rng().roll( talent.frost.frigid_executioner->effectN( 2 ).percent() ) )
    {
      int regenerated = replenish_rune( as<int>( spell.runic_empowerment_gain->effectN( 1 ).resource( RESOURCE_RUNE ) ),
                                        gains.runic_empowerment );

      sim->print_debug( "{} Runic Empowerment (Frigid Executioner) regenerated {} rune", name(), regenerated );
      log_rune_status( this );
    }
  }

  if ( talent.frost.icy_onslaught->ok() )
  {
    buffs.icy_onslaught->set_freeze_stacks( true ); // hacky abuse of bool to stop IO from gaining a stack when RE procs at 0 IO stacks
    buffs.icy_onslaught->expire(
        100_ms );  // Delay needed to push icy_onslaughts expiration to after frost strike is resolved
  }
}

void death_knight_t::trigger_bonegrinder( int stacks )
{
  if ( !talent.frost.bonegrinder.ok() )
    return;

  if ( bugs && buffs.bonegrinder_frost->check() && buffs.bonegrinder_crit->check() )
  {
    buffs.bonegrinder_frost->trigger();
    buffs.bonegrinder_crit->expire();
  }

  if ( buffs.bonegrinder_frost->check() )
    return;

  buffs.bonegrinder_crit->trigger( stacks );
  if ( buffs.bonegrinder_crit->at_max_stacks() )
  {
    buffs.bonegrinder_frost->trigger();
    buffs.bonegrinder_crit->expire();
  }
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
                    spell.runic_corruption_chance->effectN( 1 ).percent() * rpcost * 0.01;
  // Buff duration and refresh behavior handled in runic_corruption_buff_t
  if ( buffs.runic_corruption->trigger( 1, buff_t::DEFAULT_VALUE(), proc_chance ) && proc )
    proc->occur();
}

void death_knight_t::trigger_dread_plague_death( player_t* t )
{
  if ( sim->target_non_sleeping_list.size() <= 1 || sim->event_mgr.canceled )
    return;

  std::vector<player_t*> tl;
  for ( auto& tar : sim->target_non_sleeping_list )
  {
    if ( tar != t )
      tl.push_back( tar );
  }

  if ( tl.empty() )
    return;

  background_actions.dread_plague_death->execute();

  std::sort( tl.begin(), tl.end(),
             []( player_t* a, player_t* b ) { return a->health_percentage() < b->health_percentage(); } );

  background_actions.dread_plague->execute_on_target( tl.front() );
}

void death_knight_t::sudden_doom_execute_effects( bool coil )
{
  if ( talent.sanlayn.visceral_strength.ok() )
    buffs.visceral_strength->trigger();

  if ( talent.unholy.all_will_serve.ok() && pets.risen_skulker.active_pet() != nullptr )
  {
    pets::risen_skulker_pet_t* skulker = pets.risen_skulker.active_pet();
    if ( coil )
      skulker->blighted_arrow_st_buff->trigger();
    else
      skulker->blighted_arrow_aoe_buff->trigger();
  }

  if ( talent.unholy.doomed_bidding.ok() )
  {
    if ( coil )
      pet_summon.db_ghoul_coil->execute();
    else
      pet_summon.db_ghoul_epi->execute();
  }
}

void death_knight_t::sudden_doom_impact_effects( action_state_t* /*state*/, bool coil )
{
  if ( !coil )
    return;
}

void death_knight_t::unholy_rp_execute_effects( bool sd, bool coil )
{
  if ( buffs.dark_transformation->up() )
    buffs.dark_transformation->extend_duration( spell.eternal_agony->effectN( 1 ).time_value() );

  if ( talent.sanlayn.vampiric_strike.ok() && !buffs.gift_of_the_sanlayn->check() )
    trigger_vampiric_strike_proc( target );

  if ( talent.rider.let_terror_reign.ok() && pets.whitemane.active_pet() != nullptr )
  {
    if ( coil )
      pets.whitemane.active_pet()->death_coil->execute_on_target( target );
    else
      pets.whitemane.active_pet()->epidemic->execute();
  }

  if ( sd )
  {
    sudden_doom_execute_effects( coil );
    // Delay Sudden Doom decrement to ~100ms after the damage is done. Coil has a travel time, hence the higher delay.
    make_event( *sim, coil ? 210_ms : 100_ms, [ & ]() { buffs.sudden_doom->decrement(); } );
  }
}

void death_knight_t::unholy_rp_impact_effects( action_state_t* state, bool sd, bool coil )
{
  if ( !state->action->result_is_hit( state->result ) )
    return;

  death_knight_td_t* td     = get_target_data( state->target );
  timespan_t extension_time = spec.unholy_death_knight->effectN( 11 ).time_value();

  if ( td->dot.dread_plague->is_ticking() )
    td->dot.dread_plague->adjust_duration( extension_time );

  if ( td->dot.virulent_plague->is_ticking() )
    td->dot.virulent_plague->adjust_duration( extension_time );

  if ( sd )
    sudden_doom_impact_effects( state, coil );
}

// Launches the repeating event for the Inexorable Assault talent
void death_knight_t::start_inexorable_assault()
{
  if ( !talent.frost.inexorable_assault.ok() )
    return;

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

rider_of_the_apocalypse_e death_knight_t::get_random_rider()
{
  // If all riders are active, dont bother running the rest of the function, no random riders can be spawned.
  if ( pets.mograine.active_pet() != nullptr && pets.nazgrim.active_pet() != nullptr &&
       pets.trollbane.active_pet() != nullptr && pets.whitemane.active_pet() != nullptr )
    return rider_of_the_apocalypse_e::NONE;

  std::vector<rider_of_the_apocalypse_e> available_riders;
  available_riders.reserve( rider_of_the_apocalypse_e::ALL_RIDERS );

  for ( auto& rider : { rider_of_the_apocalypse_e::MOGRAINE, rider_of_the_apocalypse_e::NAZGRIM,
                        rider_of_the_apocalypse_e::TROLLBANE, rider_of_the_apocalypse_e::WHITEMANE } )
  {
    if ( last_summoned_rider != rider )
      available_riders.push_back( rider );
  }

  rider_of_the_apocalypse_e n = rng().range( available_riders );
  last_summoned_rider         = n;
  return n;
}

void death_knight_t::summon_rider( timespan_t duration, rider_of_the_apocalypse_e rider )
{
  rider_of_the_apocalypse_e n = rider;
  bool random                 = false;
  if ( n == rider_of_the_apocalypse_e::RANDOM )
  {
    random = true;
    n      = get_random_rider();
  }

  if ( n == rider_of_the_apocalypse_e::NONE )
    return;

  std::vector<action_t*> summon_riders;
  if ( n == rider_of_the_apocalypse_e::ALL_RIDERS )
    summon_riders.reserve( 4 );

  switch ( n )
  {
    case rider_of_the_apocalypse_e::MOGRAINE:
      summon_riders.push_back( pet_summon.summon_mograine );
      break;
    case rider_of_the_apocalypse_e::NAZGRIM:
      summon_riders.push_back( pet_summon.summon_nazgrim );
      break;
    case rider_of_the_apocalypse_e::TROLLBANE:
      summon_riders.push_back( pet_summon.summon_trollbane );
      break;
    case rider_of_the_apocalypse_e::WHITEMANE:
      summon_riders.push_back( pet_summon.summon_whitemane );
      break;
    case rider_of_the_apocalypse_e::ALL_RIDERS:
      summon_riders.push_back( pet_summon.summon_mograine );
      summon_riders.push_back( pet_summon.summon_nazgrim );
      summon_riders.push_back( pet_summon.summon_trollbane );
      summon_riders.push_back( pet_summon.summon_whitemane );
      break;
    default:
      break;
  }

  for ( auto& rider : summon_riders )
  {
    debug_cast<summon_rider_t*>( rider )->set_duration( duration );
    debug_cast<summon_rider_t*>( rider )->random = random;
    rider->execute();
  }
}

void death_knight_t::extend_rider( double amount, pets::horseman_pet_t* rider )
{
  double threshold    = talent.rider.fury_of_the_horsemen->effectN( 1 ).base_value();
  double max_time     = talent.rider.fury_of_the_horsemen->effectN( 2 ).base_value();
  timespan_t duration = timespan_t::from_seconds( talent.rider.fury_of_the_horsemen->effectN( 3 ).base_value() );
  double limit        = threshold * max_time;

  if ( rider->rp_spent >= limit )
    return;

  rider->rp_spent += amount;
  rider->current_pool += amount;
  if ( rider->current_pool >= threshold )
  {
    rider->current_pool -= threshold;
    rider->adjust_duration( duration );
  }
}

void death_knight_t::sort_undeath_targets( std::vector<player_t*> tl )
{
  undeath_tl = tl;

  std::sort( undeath_tl.begin(), undeath_tl.end(), [ this ]( player_t* a, player_t* b ) {
    return get_target_data( a )->dot.undeath->current_stack() < get_target_data( b )->dot.undeath->current_stack();
  } );
}

void death_knight_t::trigger_whitemanes_famine( player_t* main_target )
{
  auto td = get_target_data( main_target );

  td->dot.undeath->increment( as<int>( pet_spell.undeath_dot->effectN( 3 ).base_value() ) );

  if ( sim->target_non_sleeping_list.size() == 1 )
    return;

  std::vector<player_t*> tl = undeath_tl;
  range::erase_remove( tl, [ main_target ]( player_t* t ) { return t == main_target; } );
  spread_undeath( td, tl[ 0 ] );
}

void death_knight_t::spread_undeath( death_knight_td_t* main_td, player_t* new_target )
{
  death_knight_td_t* td = get_target_data( new_target );

  if ( td->dot.undeath->is_ticking() )
    td->dot.undeath->increment( as<int>( pet_spell.undeath_dot->effectN( 3 ).base_value() ) );
  else
    main_td->dot.undeath->copy( new_target, DOT_COPY_CLONE );

  std::rotate( undeath_tl.begin(), undeath_tl.begin() + 1, undeath_tl.end() );
}

void death_knight_t::start_a_feast_of_souls()
{
  unsigned threshold = as<unsigned>( talent.rider.a_feast_of_souls->effectN( 1 ).base_value() );
  timespan_t period  = talent.rider.a_feast_of_souls->effectN( 1 ).period();
  timespan_t first   = timespan_t::from_millis( rng().range( 0, as<int>( period.total_millis() ) ) );

  make_event( *sim, first, [ this, threshold, period ]() {
    if ( !buffs.a_feast_of_souls->check() && active_riders >= threshold )
    {
      buffs.a_feast_of_souls->trigger();
    }
    make_repeating_event( *sim, period, [ this, threshold ]() {
      if ( !buffs.a_feast_of_souls->check() && active_riders >= threshold )
      {
        buffs.a_feast_of_souls->trigger();
      }
      if ( buffs.a_feast_of_souls->check() && active_riders < threshold )
      {
        buffs.a_feast_of_souls->expire();
      }
    } );
  } );
}

void death_knight_t::trigger_infliction_of_sorrow( player_t* t, bool is_vampiric )
{
  if ( !is_vampiric )
    return;

  auto base_td                   = get_target_data( t );
  std::vector<dot_t*> disease_td = {};
  disease_td.reserve( 2 );
  double disease_remaining_damage = 0;
  double mod                      = 0;
  switch ( specialization() )
  {
    case DEATH_KNIGHT_BLOOD:
      disease_td.push_back( base_td->dot.blood_plague );
      break;
    case DEATH_KNIGHT_UNHOLY:
      disease_td.push_back( base_td->dot.dread_plague );
      disease_td.push_back( base_td->dot.virulent_plague );
      break;
    default:
      break;
  }

  for ( auto& disease : disease_td )
    disease_remaining_damage += disease->tick_damage_over_remaining_time();

  if ( disease_remaining_damage == 0 || disease_td.empty() )
    return;

  if ( is_vampiric )
  {
    timespan_t extension = 0_s;
    switch ( specialization() )
    {
      case DEATH_KNIGHT_BLOOD:
        extension = talent.sanlayn.infliction_of_sorrow->effectN( 3 ).time_value();
        break;
      case DEATH_KNIGHT_UNHOLY:
        extension = talent.sanlayn.infliction_of_sorrow->effectN( 5 ).time_value();
        break;
      default:
        break;
    }
    mod = talent.sanlayn.infliction_of_sorrow->effectN( 2 ).percent();

    for ( auto& disease : disease_td )
      if ( disease->is_ticking() )
        disease->adjust_duration( extension );

    if ( specialization() == DEATH_KNIGHT_BLOOD && disease_remaining_damage > 0 )
    {
      background_actions.infliction_of_sorrow->execute_on_target( t, disease_remaining_damage * mod );
    }
  }
}

void death_knight_t::trigger_vampiric_strike_proc( player_t* target )
{
  double chance    = talent.sanlayn.vampiric_strike->effectN( 1 ).percent();
  double target_hp = target->health_percentage();

  if ( talent.sanlayn.sanguine_scent.ok() && target_hp <= talent.sanlayn.sanguine_scent->effectN( 1 ).base_value() )
    chance += talent.sanlayn.sanguine_scent->effectN( 2 ).percent();

  if ( buffs.bloodsoaked_ground->up() )
    chance += talent.sanlayn.bloodsoaked_ground->effectN( 2 ).percent();

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

void death_knight_t::trigger_sanlayn_execute_talents( bool is_vampiric, bool summoned_ghoul )
{
  if ( !is_vampiric )
    return;

  background_actions.vampiric_strike_heal->execute();
  buffs.essence_of_the_blood_queen->trigger();

  if ( !buffs.gift_of_the_sanlayn->check() )
    buffs.vampiric_strike->expire();

  if ( talent.sanlayn.transfusion.ok() )
  {
    if ( specialization() == DEATH_KNIGHT_UNHOLY && summoned_ghoul )
      active_lesser_ghouls.back()->transfusion->trigger();

    else if ( specialization() == DEATH_KNIGHT_BLOOD )
    {
      if ( pets.dancing_rune_weapon_pet.active_pet() != nullptr )
        pets.dancing_rune_weapon_pet.active_pet()->transfusion->trigger();
      if ( pets.everlasting_bond_pet.active_pet() != nullptr )
        pets.everlasting_bond_pet.active_pet()->transfusion->trigger();
      for ( auto& dom : pets.dance_of_midnight_pet.active_pets() )
        dom->transfusion->trigger();
    }
  }

  if ( talent.sanlayn.inevitable.ok() && specialization() == DEATH_KNIGHT_UNHOLY )
    buffs.clawing_shadows->trigger( buffs.clawing_shadows->max_stack() );
}

void death_knight_t::trigger_reapers_mark_death( player_t* target )
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( sim->event_mgr.canceled || !talent.deathbringer.reapers_mark.ok() )
    return;

  buff_t* reapers_mark = get_target_data( target )->debuff.reapers_mark;

  if ( !reapers_mark->check() )
    return;

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

void death_knight_t::reapers_mark_explosion_wrapper( player_t* target, player_t* source, int stacks )
{
  if ( target != nullptr && !target->is_sleeping() && stacks > 0 && !sim->event_mgr.canceled && source != nullptr )
  {
    debug_cast<reapers_mark_explosion_t*>( background_actions.reapers_mark_explosion )
        ->execute_wrapper( target, stacks );
  }
}

void death_knight_t::drw_action_execute( pets::dancing_rune_weapon_pet_t* drw, drw_actions_e action )
{
  switch ( action )
  {
    case DRW_ACTION_BLOOD_BOIL:
      drw->ability.blood_boil->execute();
      break;
    case DRW_ACTION_DEATHS_CARESS:
      drw->ability.deaths_caress->execute();
      break;
    case DRW_ACTION_DEATH_STRIKE:
      drw->ability.death_strike->execute();
      break;
    case DRW_ACTION_HEART_STRIKE:
      drw->ability.heart_strike->execute();
      break;
    case DRW_ACTION_MARROWREND:
      drw->ability.marrowrend->execute();
      break;
    case DRW_ACTION_VAMPIRIC_STRIKE:
      drw->ability.vampiric_strike->execute();
      break;
    default:
      assert( false && "DRW Action Does not Exist" );
      break;
  }
}

void death_knight_t::trigger_drw_action( drw_actions_e action )
{
  if ( specialization() != DEATH_KNIGHT_BLOOD || !talent.blood.dancing_rune_weapon.ok() )
    return;

  if ( pets.dancing_rune_weapon_pet.active_pet() )
    drw_action_execute( pets.dancing_rune_weapon_pet.active_pet(), action );

  if ( talent.blood.everlasting_bond.ok() && pets.everlasting_bond_pet.active_pet() )
    drw_action_execute( pets.everlasting_bond_pet.active_pet(), action );

  if ( talent.blood.dance_of_midnight_3.ok() )
  {
    for ( auto& dom : pets.dance_of_midnight_pet.active_pets() )
    {
      drw_action_execute( dom, action );
    }
  }
}

void death_knight_t::trigger_rune_of_the_apocalypse( player_t* t )
{
  if ( !has_runeforge( RUNEFORGE_APOCALYPSE ) )
    return;

  int n = as<int>( std::floor( rng().range( 0, runeforge_apocalypse_e::MAX ) ) );

  death_knight_td_t* td = get_target_data( t );

  switch ( n )
  {
    case runeforge_apocalypse_e::DEATH:
      td->debuff.apocalypse_death->trigger();
      break;
    case runeforge_apocalypse_e::FAMINE:
      td->debuff.apocalypse_famine->trigger();
      break;
    case runeforge_apocalypse_e::PESTILENCE:
      runeforge_actions.apocalypse_pestilence->execute_on_target( t );
      break;
    case runeforge_apocalypse_e::WAR:
      td->debuff.apocalypse_war->trigger();
      break;
  }
}

const spell_data_t* death_knight_t::conditional_spell_lookup( bool fn, int id )
{
  if ( !fn )
    return spell_data_t::not_found();

  return find_spell( id );
}

bool death_knight_t::has_runeforge( runeforges_e rf ) const
{
  return mh_runeforge == rf || oh_runeforge == rf;
}

void death_knight_t::set_runeforges()
{
  switch ( items[ SLOT_MAIN_HAND ].parsed.enchant_id )
  {
    case RUNEFORGE_ID_APOCALYPSE:
      mh_runeforge = RUNEFORGE_APOCALYPSE;
      break;
    case RUNEFORGE_ID_FALLEN_CRUSADER:
      mh_runeforge = RUNEFORGE_FALLEN_CRUSADER;
      break;
    case RUNEFORGE_ID_RAZORICE:
      mh_runeforge = RUNEFORGE_RAZORICE;
      break;
    case RUNEFORGE_ID_SANGUINATION:
      mh_runeforge = RUNEFORGE_SANGUINATION;
      break;
    case RUNEFORGE_ID_SPELLWARDING:
      mh_runeforge = RUNEFORGE_SPELLWARDING;
      break;
    case RUNEFORGE_ID_STONESKIN_GARGOYLE:
      mh_runeforge = RUNEFORGE_STONESKIN_GARGOYLE;
      break;
    case RUNEFORGE_ID_UNENDING_THIRST:
      mh_runeforge = RUNEFORGE_UNENDING_THIRST;
      break;
    default:
      mh_runeforge = RUNEFORGE_NONE;
      break;
  }

  if ( off_hand_weapon.type != WEAPON_NONE )
  {
    switch ( items[ SLOT_OFF_HAND ].parsed.enchant_id )
    {
      case RUNEFORGE_ID_APOCALYPSE:
        oh_runeforge = RUNEFORGE_APOCALYPSE;
        break;
      case RUNEFORGE_ID_FALLEN_CRUSADER:
        oh_runeforge = RUNEFORGE_FALLEN_CRUSADER;
        break;
      case RUNEFORGE_ID_RAZORICE:
        oh_runeforge = RUNEFORGE_RAZORICE;
        break;
      case RUNEFORGE_ID_SANGUINATION:
        oh_runeforge = RUNEFORGE_SANGUINATION;
        break;
      case RUNEFORGE_ID_SPELLWARDING:
        oh_runeforge = RUNEFORGE_SPELLWARDING;
        break;
      case RUNEFORGE_ID_STONESKIN_GARGOYLE:
        oh_runeforge = RUNEFORGE_STONESKIN_GARGOYLE;
        break;
      case RUNEFORGE_ID_UNENDING_THIRST:
        oh_runeforge = RUNEFORGE_UNENDING_THIRST;
        break;
      default:
        oh_runeforge = RUNEFORGE_NONE;
        break;
    }
  }
}

// ==========================================================================
// Death Knight Character Definition
// ==========================================================================

// death_knight_t::create_actions ===========================================

void death_knight_t::create_actions()
{
  background_actions.death_coil_damage = get_action<death_coil_damage_t>( "death_coil_damage", this );

  // Runeforges
  if ( has_runeforge( RUNEFORGE_APOCALYPSE ) )
  {
    runeforge_actions.apocalypse_pestilence =
        get_action<runeforge_apocalypse_pestilence_t>( "pestilence_runeforge", this );
    runeforge_actions.apocalypse_pestilence->name_str_reporting = "pestilence";
  }

  if ( has_runeforge( RUNEFORGE_SPELLWARDING ) )
    runeforge_actions.spellwarding_absorb = get_action<spellwarding_absorb_t>( "spellwarding", this );
  if ( has_runeforge( RUNEFORGE_SANGUINATION ) )
    runeforge_actions.sanguination_heal = get_action<sanguination_heal_t>( "sanguination_heal", this );

  // Class talents
  if ( talent.blood_draw.ok() )
  {
    background_actions.blood_draw = get_action<blood_draw_t>( "blood_draw", this );
  }

  if ( talent.permafrost.ok() )
  {
    background_actions.permafrost = new permafrost_t( this );
  }

  // Rider of the Apocalypse
  if ( talent.rider.riders_champion.ok() )
  {
    pet_summon.summon_whitemane    = get_action<summon_whitemane_t>( "summon_whitemane", this );
    background_actions.undeath_dot = get_action<undeath_dot_t>( "undeath", this );

    pet_summon.summon_mograine = get_action<summon_mograine_t>( "summon_mograine", this );

    pet_summon.summon_trollbane            = get_action<summon_trollbane_t>( "summon_trollbane", this );
    background_actions.trollbanes_icy_fury = get_action<trollbanes_icy_fury_t>( "trollbanes_icy_fury", this );

    pet_summon.summon_nazgrim = get_action<summon_nazgrim_t>( "summon_nazgrim", this );

    // Dummy Action for reporting purposes
    action_t* riders_champion =
        new action_t( action_e::ACTION_OTHER, "riders_champion", this, talent.rider.riders_champion );
    riders_champion->add_child( pet_summon.summon_whitemane );
    riders_champion->add_child( pet_summon.summon_mograine );
    riders_champion->add_child( pet_summon.summon_trollbane );
    riders_champion->add_child( pet_summon.summon_nazgrim );
  }

  // San'layn
  if ( talent.sanlayn.vampiric_strike.ok() )
  {
    background_actions.vampiric_strike_heal = get_action<vampiric_strike_heal_t>( "vampiric_strike_heal", this );
  }

  if ( talent.sanlayn.infliction_of_sorrow.ok() )
  {
    background_actions.infliction_of_sorrow = get_action<infliction_in_sorrow_t>( "infliction_of_sorrow", this );
  }

  if ( talent.sanlayn.the_blood_is_life.ok() )
  {
    background_actions.the_blood_is_life = get_action<the_blood_is_life_t>( "the_blood_is_life", this );
    pet_summon.blood_beast               = get_action<blood_beast_summon_t>( "blood_beast", this );
  }

  // Deathbringer
  if ( talent.deathbringer.reapers_mark.ok() )
  {
    background_actions.reapers_mark_explosion = get_action<reapers_mark_explosion_t>( "reapers_mark_explosion", this );
  }
  if ( talent.deathbringer.wave_of_souls.ok() )
  {
    background_actions.wave_of_souls = get_action<wave_of_souls_t>( "wave_of_souls", this );
  }
  if ( talent.deathbringer.soul_rupture.ok() )
  {
    background_actions.soul_rupture = get_action<soul_rupture_t>( "reapers_mark_soul_rupture", this );
  }
  if ( talent.deathbringer.exterminate.ok() || talent.deathbringer.echoing_fury.ok() )
  {
    background_actions.exterminate = get_action<exterminate_t>( "exterminate", this );
  }
  if ( talent.deathbringer.exterminate.ok() || talent.deathbringer.echoing_fury.ok() )
  {
    background_actions.exterminate_aoe = get_action<exterminate_aoe_t>( "exterminate_second_hit", this );
  }

  // Blood
  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    if ( talent.blood.bloodied_blade.ok() )
    {
      background_actions.heart_strike_bloodied_blade =
          get_action<heart_strike_bloodied_blade_t>( "heart_strike_bloodied_blade", this );
    }
    if ( talent.blood.bloodworms.ok() )
    {
      pet_summon.bloodworm = get_action<bloodworm_summon_t>( "bloodworm_summon", this );
    }
    if ( talent.blood.boiling_point.ok() )
      background_actions.blood_boil_boiling_point = get_action<blood_boil_t>( "blood_boil_boiling_point", this );
    if ( talent.blood.blood_mist.ok() )
      background_actions.blood_mist_tick = get_action<blood_mist_t>( "blood_mist", this );
    if ( talent.blood.bloody_reflection.ok() )
      background_actions.bloody_reflection = get_action<bloody_reflection_t>( "bloody_reflection", this );
    if ( talent.blood.sanguinary_burst.ok() )
      background_actions.sanguinary_burst = get_action<sanguinary_burst_t>( "sanguinary_burst", this );
  }

  // Unholy
  else if ( specialization() == DEATH_KNIGHT_UNHOLY )
  {
    if ( options.wcl_reporting_mode )
    {
      // Dummy Actions for reporting purposes
      action_t* summon_lesser_ghoul =
          new action_t( action_e::ACTION_OTHER, "lesser_ghoul", this, spell.summon_lesser_ghoul );
      pets.lesser_ghoul.set_creation_event_callback( pets::parent_pet_action_fn( summon_lesser_ghoul ) );

      action_t* summon_magus = new action_t( action_e::ACTION_OTHER, "army_of_the_damned", this, spell.summon_magus );
      pets.magus_of_the_dead.set_creation_event_callback( pets::parent_pet_action_fn( summon_magus ) );
    }
    

    if ( talent.unholy.outbreak.ok() )
    {
      background_actions.dread_plague_death    = get_action<dread_plague_death_t>( "dread_plague_death", this );
      background_actions.virulent_plague_erupt = get_action<virulent_plague_erupt_t>( "virulent_plague_erupt", this );
      background_actions.dread_plague_erupt    = get_action<dread_plague_erupt_t>( "dread_plague_erupt", this );
    }

    if ( talent.unholy.outbreak.ok() )
    {
      background_actions.outbreak_aoe    = get_action<outbreak_aoe_t>( "outbreak_aoe", this );
      background_actions.virulent_plague = get_action<virulent_plague_t>( "virulent_plague", this );
      background_actions.dread_plague    = get_action<dread_plague_t>( "dread_plague", this );
    }

    if ( talent.unholy.scourge_strike.ok() )
    {
      background_actions.virulent_plague_erupt_ss =
          get_action<virulent_plague_erupt_t>( "virulent_plague_erupt", this );
      background_actions.dread_plague_erupt_ss = get_action<dread_plague_erupt_t>( "dread_plague_erupt", this );
      pet_summon.fs_ghoul = get_action<summon_lesser_ghoul_t>( "fs_ghoul", this, spell.summon_lesser_ghoul,
                                                               lesser_ghoul_e::LESSER_FESTERING_STRIKE );
    }

    if ( talent.unholy.blightburst.ok() )
    {
      background_actions.virulent_plague_erupt_bb =
          get_action<virulent_plague_erupt_t>( "virulent_plague_erupt_bb", this );
      background_actions.dread_plague_erupt_bb = get_action<dread_plague_erupt_t>( "dread_plague_erupt_bb", this );
    }

    if ( talent.unholy.blightfall.ok() )
    {
      background_actions.virulent_plague_erupt_pest =
          get_action<virulent_plague_erupt_t>( "virulent_plague_erupt_blightfall", this );
      background_actions.dread_plague_erupt_pest = get_action<dread_plague_erupt_t>( "dread_plague_erupt_blightfall", this );
    }

    if ( spec.epidemic->ok() )
      background_actions.epidemic_main = get_action<epidemic_damage_main_t>( "epidemic_main", this );

    if ( talent.unholy.putrefy.ok() )
    {
      background_actions.putrefy_aoe = get_action<putrefy_aoe_t>( "putrefy_aoe", this, PUTREFY_SOURCE_PUTREFY );
      background_actions.putrefy_st = get_action<putrefy_st_t>( "putrefy_st", this, PUTREFY_SOURCE_PUTREFY );
      pet_summon.putrefy_ghoul   = get_action<summon_lesser_ghoul_t>( "putrefy_ghoul", this, spell.summon_putrefy_ghoul,
                                                                      lesser_ghoul_e::LESSER_PUTREFY );
    }

    if ( talent.unholy.army_of_the_dead.ok() )
      pet_summon.army_ghoul = get_action<summon_lesser_ghoul_t>( "army_ghoul", this, spell.summon_army_ghoul,
                                                                 lesser_ghoul_e::LESSER_ARMY_OF_THE_DEAD );

    if ( talent.unholy.infected_claws.ok() )
      background_actions.infected_claws = get_action<infected_claws_t>( "infected_claws", this );

    if ( talent.unholy.doomed_bidding.ok() )
    {
      pet_summon.db_ghoul_coil = get_action<summon_lesser_ghoul_t>( "coil_db_ghoul", this, spell.summon_lesser_ghoul,
                                                                    lesser_ghoul_e::LESSER_DOOMED_BIDDING_COIL );
      pet_summon.db_ghoul_epi  = get_action<summon_lesser_ghoul_t>( "epi_db_ghoul", this, spell.summon_lesser_ghoul,
                                                                    lesser_ghoul_e::LESSER_DOOMED_BIDDING_EPIDEMIC );
    }

    if ( talent.unholy.reanimation.ok() )
    {
      pet_summon.reanimation_magus = get_action<summon_magus_t>(
          "reanimation_magus", this, spell.summon_reanimation_magus, magus_of_the_dead_e::MAGUS_REANIMATION );
      if ( talent.unholy.forbidden_knowledge_3.ok() )
        pet_summon.fk_reanimation_magus = get_action<summon_magus_t>(
            "fk_reanimation_magus", this, spell.summon_reanimation_magus, magus_of_the_dead_e::MAGUS_REANIMATION_FK );
    }

    if ( talent.unholy.coil_of_devastation.ok() )
      background_actions.coil_of_devastation = get_action<coil_of_devastation_t>( "coil_of_devastation", this );

    if ( talent.unholy.all_will_serve.ok() )
      pet_summon.raise_skulker = get_action<raise_skulker_t>( "raise_skulker", this );

    if ( talent.unholy.forbidden_knowledge_3.ok() )
    {
      background_actions.putrefy_fk_aoe = new putrefy_fk_aoe_t( "putrefy_aoe", this );
      background_actions.putrefy_fk_st  = new putrefy_fk_st_t( "putrefy_st", this );
      pet_summon.fk_ghoul = get_action<summon_lesser_ghoul_t>( "fk_ghoul", this, spell.summon_putrefy_ghoul,
                                                               lesser_ghoul_e::LESSER_FORBIDDEN_KNOWLEDGE );
    }

    if ( talent.unholy.soul_reaper.ok() )
    {
      pet_summon.sr_ghoul = get_action<summon_lesser_ghoul_t>( "sr_ghoul", this, spell.summon_putrefy_ghoul,
                                                                         lesser_ghoul_e::LESSER_SOUL_REAPER );
    }
  }

  else if ( specialization() == DEATH_KNIGHT_FROST )
  {
    if ( talent.frost.breath_of_sindragosa.ok() )
    {
      background_actions.breath_of_sindragosa_tick =
          get_action<breath_of_sindragosa_tick_t>( "breath_of_sindragosa_damage", this );
    }

    if ( spec.remorseless_winter->ok() || talent.frost.frozen_dominion->ok() )
    {
      const spell_data_t* rw_data = spec.remorseless_winter->ok()
                                        ? spec.remorseless_winter->effectN( 2 ).trigger()
                                        : spell.frozen_dominion_remorseless_winter_buff->effectN( 2 ).trigger();
      background_actions.remorseless_winter_tick =
          get_action<remorseless_winter_damage_t>( "remorseless_winter_damage", this, rw_data );
    }

    if ( talent.frost.frost_strike.ok() )
    {
      const spell_data_t* mh_data =
          main_hand_weapon.group() == WEAPON_2H ? spell.frost_strike_2h : spell.frost_strike_mh;
      background_actions.frost_strike_main =
          get_action<frost_strike_strike_t>( "frost_strike_damage", this, &( main_hand_weapon ), mh_data, false );
      if ( off_hand_weapon.type != WEAPON_NONE )
      {
        background_actions.frost_strike_offhand = get_action<frost_strike_strike_t>(
            "frost_strike_offhand", this, &( off_hand_weapon ), spell.frost_strike_oh, false );
        if ( talent.frost.shattering_blade.ok() )
        {
          background_actions.frost_strike_sb_offhand = get_action<frost_strike_strike_t>(
              "frost_strike_offhand_sb", this, &( off_hand_weapon ), spell.frost_strike_oh, true );
        }
      }
      if ( talent.frost.shattering_blade.ok() )
      {
        background_actions.frost_strike_sb_main =
            get_action<frost_strike_strike_t>( "frost_strike_sb", this, &( main_hand_weapon ), mh_data, true );
      }
      if ( talent.frost.icy_death_torrent.ok() )
      {
        background_actions.icy_death_torrent_damage = get_action<icy_death_torrent_t>( "icy_death_torrent", this );
      }
      if ( talent.frost.hyperpyrexia.ok() )
      {
        background_actions.hyperpyrexia_damage = get_action<hyperpyrexia_damage_t>( "hyperpyrexia", this );
      }
    }

    if ( talent.frost.empower_rune_weapon.ok() )
    {
      background_actions.erw_projectile =
          get_action<empower_rune_weapon_projectile_t>( "empower_rune_weapon_projectile", this );
    }

    if ( talent.frost.frostreaper.ok() )
    {
      background_actions.frostreaper = get_action<frostreaper_t>( "frostreaper", this );
    }

    if ( talent.frost.frozen_dominion.ok() )
    {
      background_actions.frozen_dominion_remorseless_winter =
          get_action<frozen_dominion_remorseless_winter_t>( "remorseless_winter_frozen_dominion", this );
    }

    if ( talent.frost.arctic_assault.ok() )
    {
      background_actions.arctic_assault_obliterate =
          get_action<glacial_advance_damage_t>( "glacial_advance_arctic_assault_obliterate", this, true );
      background_actions.arctic_assault_frostscythe =
          get_action<glacial_advance_damage_t>( "glacial_advance_arctic_assault_frostscythe", this, true );
    }
  }

  player_t::create_actions();
}

// death_knight_t::create_action  ===========================================

action_t* death_knight_t::create_action( std::string_view name, std::string_view options_str )
{
  // Deathbringer Actions
  if ( name == "reapers_mark" )
    return new reapers_mark_t( this, options_str );

  // General Actions
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
  if ( name == "death_grip" )
    return new death_grip_t( this, options_str );

  // Blood Actions
  if ( name == "abomination_limb" )
    return new abomination_limb_t( this, options_str );
  if ( name == "blood_boil" )
    return new blood_boil_t( this, options_str );
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
  if ( name == "marrowrend" )
    return new marrowrend_t( this, options_str );
  if ( name == "vampiric_blood" )
    return new vampiric_blood_t( this, options_str );

  // Frost Actions
  if ( name == "breath_of_sindragosa" )
    return new breath_of_sindragosa_t( this, options_str );
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
  if ( name == "howling_blast" )
    return new howling_blast_t( this, options_str );
  if ( name == "obliterate" )
    return new obliterate_t( this, options_str );
  if ( name == "pillar_of_frost" )
    return new pillar_of_frost_t( this, options_str );
  if ( name == "remorseless_winter" )
    return new remorseless_winter_t( this, options_str );
  if (name == "frostbane" )
    return new frostbane_t( this, options_str );

  // Unholy Actions
  if ( name == "army_of_the_dead" )
    return new army_of_the_dead_t( this, options_str );
  if ( name == "dark_transformation" )
    return new dark_transformation_t( name, this, options_str );
  if ( name == "death_and_decay" )
    return new death_and_decay_t( this, options_str );
  if ( name == "death_coil" )
    return new death_coil_t( this, options_str );
  if ( name == "epidemic" )
    return new epidemic_t( this, options_str );
  if ( name == "festering_strike" )
    return new festering_strike_t( this, options_str );
  if ( name == "outbreak" )
    return new outbreak_t( this, options_str );
  if ( name == "scourge_strike" )
    return new scourge_strike_t( name, this, options_str );
  if ( name == "putrefy" )
    return new putrefy_t( this, options_str );
  if ( name == "soul_reaper" )
    return new soul_reaper_t( this, options_str );

  // Dynamic actions
  // any_dnd and dnd_any return defile if talented, or death and decay otherwise
  if ( name == "any_dnd" || name == "dnd_any" )
  {
    return create_action( "death_and_decay", options_str );
  }

  return player_t::create_action( name, options_str );
}

// death_knight_t::create_expression ========================================

// Equipped death knight runeforge expressions
std::unique_ptr<expr_t> death_knight_t::create_runeforge_expression( std::string_view runeforge_name,
                                                                     bool warning = false )
{
  // Razorice, looks for the damage procs related to MH and OH
  if ( util::str_compare_ci( runeforge_name, "razorice" ) )
    return expr_t::create_constant( "razorice_runforge_expression", has_runeforge( RUNEFORGE_RAZORICE ) );

  // Razorice MH and OH expressions (this can matter for razorice application)
  if ( util::str_compare_ci( runeforge_name, "razorice_mh" ) )
    return expr_t::create_constant( "razorice_mh_runforge_expression", mh_runeforge == RUNEFORGE_RAZORICE );
  if ( util::str_compare_ci( runeforge_name, "razorice_oh" ) )
    return expr_t::create_constant( "razorice_oh_runforge_expression", oh_runeforge == RUNEFORGE_RAZORICE );

  // Fallen Crusader, looks for the unholy strength healing action
  if ( util::str_compare_ci( runeforge_name, "fallen_crusader" ) )
    return expr_t::create_constant( "fallen_crusader_runforge_expression", has_runeforge( RUNEFORGE_FALLEN_CRUSADER ) );

  // Stoneskin Gargoyle
  if ( util::str_compare_ci( runeforge_name, "stoneskin_gargoyle" ) )
    return expr_t::create_constant( "stoneskin_gargoyle_runforge_expression",
                                    has_runeforge( RUNEFORGE_STONESKIN_GARGOYLE ) );

  // Apocalypse
  if ( util::str_compare_ci( runeforge_name, "apocalypse" ) )
    return expr_t::create_constant( "apocalypse_runforge_expression", has_runeforge( RUNEFORGE_APOCALYPSE ) );

  // Sanguination
  if ( util::str_compare_ci( runeforge_name, "sanguination" ) )
    return expr_t::create_constant( "sanguination_runeforge_expression", has_runeforge( RUNEFORGE_SANGUINATION ) );

  // Spellwarding
  if ( util::str_compare_ci( runeforge_name, "spellwarding" ) )
    return expr_t::create_constant( "spellwarding_runeforge_expression", has_runeforge( RUNEFORGE_SPELLWARDING ) );

  // Unending Thirst, effect NYI
  if ( util::str_compare_ci( runeforge_name, "unending_thirst" ) )
    return expr_t::create_constant( "unending_thirst_runeforge_expression",
                                    has_runeforge( RUNEFORGE_UNENDING_THIRST ) );

  // Only throw an error with death_knight.runeforge expressions
  // runeforge.x already spits out a warning for relevant runeforges and has to send a runeforge legendary if needed
  if ( !warning )
    throw sc_invalid_apl_argument( fmt::format( "Unknown Death Knight runeforge name '{}'.", runeforge_name ) );

  return nullptr;
}

std::unique_ptr<expr_t> death_knight_t::create_expression( std::string_view name_str )
{
  auto splits = util::string_split<std::string_view>( name_str, "." );

  if ( util::str_compare_ci( splits[ 0 ], "rune" ) && splits.size() > 1 )
  {
    // rune.time_to_x returns the number of seconds before X runes will be ready at the current generation rate
    if ( util::str_in_str_ci( splits[ 1 ], "time_to_" ) )
    {
      auto n_char = splits[ 1 ][ splits[ 1 ].size() - 1 ];
      auto n      = n_char - '0';
      if ( n <= 0 || as<size_t>( n ) > MAX_RUNES )
      {
        throw sc_invalid_apl_argument( fmt::format( "Invalid rune amount in 'rune.time_to_{}.'", n_char ) );
      }

      return make_fn_expr( "rune_time_to_x", [ this, n ]() { return _runes.time_to_regen( as<unsigned>( n ) ); } );
    }
  }

  // Death Knight special expressions
  if ( util::str_compare_ci( splits[ 0 ], "death_knight" ) && splits.size() > 1 )
  {
    // Returns the value of the disable_aotd option
    if ( util::str_compare_ci( splits[ 1 ], "disable_aotd" ) && splits.size() == 2 )
      return expr_t::create_constant( "disable_aotd_expression", this->options.disable_aotd );

    // Returns if the given death knight runeforge is equipped or not
    if ( util::str_compare_ci( splits[ 1 ], "runeforge" ) && splits.size() == 3 )
    {
      auto runeforge_expr = create_runeforge_expression( splits[ 2 ] );
      if ( runeforge_expr )
        return runeforge_expr;
    }

    // Expose AMS Absorb Percent to the APL to enable decisions based on anticipated resource generation
    if ( util::str_compare_ci( splits[ 1 ], "ams_absorb_percent" ) && splits.size() == 2 )
      return expr_t::create_constant( "ams_absorb_percent", options.ams_absorb_percent );

    // Expose AMZ Specified to the APL to prevent its use.
    if ( util::str_compare_ci( splits[ 1 ], "amz_specified" ) && splits.size() == 2 )
      return expr_t::create_constant( "amz_specified", options.amz_specified );

    // Expose first AMS cast to the APL to prevent its use.
    if ( util::str_compare_ci( splits[ 1 ], "first_ams_cast" ) && splits.size() == 2 )
      return expr_t::create_constant( "first_ams_cast", options.first_ams_cast.total_seconds() );

    throw sc_invalid_apl_argument( fmt::format( "Unknown death_knight expression '{}'.", splits[ 1 ] ) );
  }

  if ( util::str_compare_ci( splits[ 0 ], "drw" ) && splits.size() > 1 )
  {
    if ( util::str_compare_ci( splits[ 1 ], "bp_ticking" ) && splits.size() == 2 )
      return make_fn_expr( "drw_blood_plague_ticking_expression", [ this ]() {
        return pets.dancing_rune_weapon_pet.active_pet() != nullptr &&
               pets.dancing_rune_weapon_pet.active_pet()->get_target_data( target )->dot.blood_plague->is_ticking();
      } );
  }

  // Death and Decay/Defile expressions
  if ( util::str_compare_ci( splits[ 0 ], "death_and_decay" ) && splits.size() == 2 )
  {
    // Returns true if there's an active dnd
    if ( util::str_compare_ci( splits[ 1 ], "ticking" ) || util::str_compare_ci( splits[ 1 ], "up" ) )
    {
      return make_fn_expr( "dnd_ticking", [ this ]() { return !active_dnds.empty(); } );
    }

    // Returns the remaining value on the active dnd, or 0 if there's no dnd
    if ( util::str_compare_ci( splits[ 1 ], "remains" ) )
    {
      return make_fn_expr( "dnd_remains", [ this ]() {
        return active_dnds.empty() ? 0 : active_dnds.back()->remaining_time().total_seconds();
      } );
    }

    // Returns true if there's an active dnd AND the player is inside it
    if ( util::str_compare_ci( splits[ 1 ], "active" ) )
    {
      return make_fn_expr( "dnd_active", [ this ]() { return in_death_and_decay(); } );
    }

    // Returns the remaining value on the active dnd if the player is inside it, or 0 otherwise
    if ( util::str_compare_ci( splits[ 1 ], "active_remains" ) )
    {
      return make_fn_expr( "dnd_active_remains", [ this ]() {
        return in_death_and_decay() ? active_dnds.back()->remaining_time().total_seconds() : 0;
      } );
    }

    throw sc_invalid_apl_argument( fmt::format( "Unknown dnd expression '{}'.", splits[ 1 ] ) );
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
      []( death_knight_t* p ) { return new pets::ghoul_pet_t( p, !p->spec.raise_dead->ok() ); } );
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
    pets.lesser_ghoul.set_creation_callback(
        []( death_knight_t* p ) { return new pets::lesser_ghoul_pet_t( p, "lesser_ghoul" ); } );
    pets.magus_of_the_dead.set_creation_callback(
        []( death_knight_t* p ) { return new pets::magus_pet_t( p, "magus_of_the_dead" ); } );

    if ( spec.festering_strike->ok() )
    {
      pets.lesser_ghoul_fs.set_creation_callback(
          []( death_knight_t* p ) { return new pets::lesser_ghoul_pet_t( p, "fs_ghoul" ); } );
      pets.lesser_ghoul_fs.set_default_duration( spell.summon_lesser_ghoul->duration() );
    }

    if ( talent.unholy.putrefy.ok() )
    {
      pets.lesser_ghoul_putrefy.set_creation_callback(
          []( death_knight_t* p ) { return new pets::lesser_ghoul_pet_t( p, "putrefy_ghoul" ); } );
    }

    if ( talent.unholy.summon_gargoyle.ok() )
    {
      pets.gargoyle.set_creation_callback( []( death_knight_t* p ) { return new pets::gargoyle_pet_t( p ); } );
      pets.gargoyle.set_default_duration( spell.summon_gargoyle->duration() );
      pets.gargoyle.set_max_pets( 1 );
    }

    if ( talent.unholy.all_will_serve.ok() )
    {
      pets.risen_skulker.set_creation_callback(
          []( death_knight_t* p ) { return new pets::risen_skulker_pet_t( p ); } );
      pets.risen_skulker.set_max_pets( 1 );
      pets.risen_skulker.set_replacement_strategy( spawner::pet_replacement_strategy::NO_REPLACE );
    }

    if ( talent.unholy.army_of_the_dead.ok() )
    {
      pets.lesser_ghoul_army.set_creation_callback(
          []( death_knight_t* p ) { return new pets::lesser_ghoul_pet_t( p, "army_ghoul" ); } );
      pets.lesser_ghoul_army.set_default_duration( spell.summon_army_ghoul->duration() );
      pets.lesser_ghoul_army.set_max_pets( 8 );
    }

    if ( talent.unholy.magus_of_the_dead.ok() )
    {
      pets.dt_magus.set_creation_callback( []( death_knight_t* p ) { return new pets::magus_pet_t( p, "dt_magus" ); } );
      pets.dt_magus.set_default_duration( talent.unholy.magus_of_the_dead->effectN( 1 ).time_value() );
      pets.dt_magus.set_max_pets( 1 );
      if ( talent.unholy.army_of_the_dead.ok() )
      {
        pets.army_magus.set_creation_callback(
            []( death_knight_t* p ) { return new pets::magus_pet_t( p, "army_magus" ); } );
        pets.army_magus.set_default_duration( talent.unholy.magus_of_the_dead->effectN( 1 ).time_value() );
        pets.army_magus.set_max_pets( 1 );
      }
    }

    if ( talent.unholy.doomed_bidding.ok() )
    {
      timespan_t doomed_bidding_duration = spell.summon_lesser_ghoul->duration();

      pets.lesser_ghoul_db_coil.set_creation_callback(
          []( death_knight_t* p ) { return new pets::lesser_ghoul_pet_t( p, "db_ghoul_coil" ); } );
      pets.lesser_ghoul_db_coil.set_default_duration( doomed_bidding_duration );

      pets.lesser_ghoul_db_epi.set_creation_callback(
          []( death_knight_t* p ) { return new pets::lesser_ghoul_pet_t( p, "db_ghoul_epi" ); } );
      pets.lesser_ghoul_db_epi.set_default_duration( doomed_bidding_duration );
    }

    if ( talent.unholy.reanimation.ok() )
    {
      timespan_t reanimation_duration = talent.unholy.reanimation->effectN( 2 ).time_value();
      pets.reanimation_magus.set_creation_callback(
          []( death_knight_t* p ) { return new pets::magus_pet_t( p, "reanimation_magus" ); } );
      pets.reanimation_magus.set_default_duration( reanimation_duration );
    }

    if ( talent.unholy.raise_abomination.ok() )
    {
      pets.abomination.set_creation_callback( []( death_knight_t* p ) { return new pets::abomination_pet_t( p ); } );
      pets.abomination.set_default_duration( spell.summon_abomination->duration() );
      pets.abomination.set_max_pets( 1 );
    }

    if ( talent.unholy.forbidden_knowledge_2.ok() )
      pets.lesser_ghoul_fk.set_creation_callback(
          []( death_knight_t* p ) { return new pets::lesser_ghoul_pet_t( p, "fk_ghoul" ); } );
  }

  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    if ( talent.blood.dancing_rune_weapon.ok() )
    {
      pets.dancing_rune_weapon_pet.set_creation_callback(
          []( death_knight_t* p ) { return new pets::dancing_rune_weapon_pet_t( p, "dancing_rune_weapon" ); } );
      pets.dancing_rune_weapon_pet.set_default_duration( talent.blood.dancing_rune_weapon->duration() );
      // As of Dec 19 2025, the first rune weapon does not get the 4s extension from everlasting bond.  Only the everlasting bond weapon does
      if ( bugs && talent.blood.everlasting_bond.ok() )
        pets.dancing_rune_weapon_pet.set_default_duration( talent.blood.dancing_rune_weapon->duration() );
      pets.dancing_rune_weapon_pet.set_max_pets( 1 );

      if ( talent.blood.everlasting_bond.ok() )
      {
        pets.everlasting_bond_pet.set_creation_callback(
            []( death_knight_t* p ) { return new pets::dancing_rune_weapon_pet_t( p, "everlasting_bond" ); } );
        pets.everlasting_bond_pet.set_default_duration( spell.everlasting_bond_summon->duration() );
        pets.everlasting_bond_pet.set_max_pets( 1 );
      }

      if ( talent.blood.dance_of_midnight_3.ok() )
      {
        pets.dance_of_midnight_pet.set_creation_callback(
            []( death_knight_t* p ) { return new pets::dancing_rune_weapon_pet_t( p, "dance_of_midnight" ); } );
        pets.dance_of_midnight_pet.set_default_duration( spell.dance_of_midnight_summon->duration() );
        pets.dance_of_midnight_pet.set_max_pets( 10 );
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

  rppm.carnage     = get_rppm( "carnage", talent.blood.carnage );
  rppm.blood_beast = get_rppm( "blood_beast", talent.sanlayn.the_blood_is_life );
  rppm.frostreaper = get_rppm( "frostreaper", talent.frost.frostreaper );

  // Blood
  pseudo_random.dance_of_midnight =
      get_accumulated_rng( "dance_of_midnight", prd::find_constant( 0.125 ) );  // 12.5% chance, not in data.
  // Frost
  pseudo_random.killing_machine = get_accumulated_rng(
      "killing_machine", spec.might_of_the_frozen_wastes->ok() && main_hand_weapon.group() == WEAPON_2H ? 1.0 : 0.3 );
  // Unholy
  pseudo_random.sudden_doom = get_accumulated_rng(
      "sudden_doom", prd::find_constant( talent.unholy.sudden_doom->effectN( 2 ).percent() *
                                         ( 1.0 + talent.unholy.harbinger_of_doom->effectN( 2 ).percent() ) *
                                         ( 1.0 + talent.unholy.ebon_fever->effectN( 1 ).percent() ) ) );

  pseudo_random.forbidden_knowledge = get_accumulated_rng(
      "forbidden_knowledge", prd::find_constant( talent.unholy.forbidden_knowledge_3->effectN( 3 ).percent() ) );
}

// death_knight_t::init_base ================================================

void death_knight_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 5;

  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility  = 0.0;
  base.spell_power_per_intellect = 1.0;

  player_t::init_base_stats();
}

// death_knight_t::init_spells ==============================================
void death_knight_t::init_spells()
{
  player_t::init_spells();
  set_runeforges();
  // Specialization

  // Generic baselines
  spec.plate_specialization = find_specialization_spell( "Plate Specialization" );
  spec.death_knight         = find_spell( 137005 );  // "Death Knight" passive
  spec.death_knight_2       = find_spell( 462062 );  // Second "Death Knight" passive
  spec.death_coil           = find_class_spell( "Death Coil" );
  spec.death_grip           = find_class_spell( "Death Grip" );
  spec.dark_command         = find_class_spell( "Dark Command" );
  spec.death_and_decay      = find_spell( 43265 );
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
  spec.glacial_advance            = find_specialization_spell( "Glacial Advance" );

  // Unholy Baselines
  spec.unholy_death_knight   = find_specialization_spell( "Unholy Death Knight" );
  spec.raise_dead            = find_specialization_spell( "Raise Dead" );
  spec.festering_strike      = find_specialization_spell( "Festering Strike" );
  spec.unholy_death_knight_2 = conditional_spell_lookup( spec.unholy_death_knight->ok(), 462064 );
  spec.dark_transformation_2 = conditional_spell_lookup( spec.unholy_death_knight->ok(), 325554 );
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

  talent.unholy_momentum = find_talent_spell( talent_tree::CLASS, "Unholy Momentum" );
  talent.control_undead  = find_talent_spell( talent_tree::CLASS, "Control Undead" );
  talent.enfeeble        = find_talent_spell( talent_tree::CLASS, "Enfeeble" );
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
  talent.subduing_grasp         = find_talent_spell( talent_tree::CLASS, "Subduing Grasp" );
  talent.will_of_the_necropolis = find_talent_spell( talent_tree::CLASS, "Will of the Necropolis" );
  // Row 10
  talent.null_magic      = find_talent_spell( talent_tree::CLASS, "Null Magic" );
  talent.unyielding_will = find_talent_spell( talent_tree::CLASS, "Unyielding Will" );
  talent.deaths_echo     = find_talent_spell( talent_tree::CLASS, "Death's Echo" );
  talent.vestigial_shell = find_talent_spell( talent_tree::CLASS, "Vestigial Shell" );

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
  talent.blood.relish_in_blood         = find_talent_spell( talent_tree::SPECIALIZATION, "Relish in Blood" );
  // Row 5
  talent.blood.leeching_strike     = find_talent_spell( talent_tree::SPECIALIZATION, "Leeching Strike" );
  talent.blood.heartbreaker        = find_talent_spell( talent_tree::SPECIALIZATION, "Heartbreaker" );
  talent.blood.foul_bulwark        = find_talent_spell( talent_tree::SPECIALIZATION, "Foul Bulwark" );
  talent.blood.dancing_rune_weapon = find_talent_spell( talent_tree::SPECIALIZATION, "Dancing Rune Weapon" );
  talent.blood.hemostasis          = find_talent_spell( talent_tree::SPECIALIZATION, "Hemostasis" );
  talent.blood.perseverance_of_the_ebon_blade =
      find_talent_spell( talent_tree::SPECIALIZATION, "Perseverance of the Ebon Blade" );
  talent.blood.bloodworms = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodworms" );
  // Row 6
  talent.blood.gorefiends_grasp     = find_talent_spell( talent_tree::SPECIALIZATION, "Gorefiend's Grasp" );
  talent.blood.abomination_limb     = find_talent_spell( talent_tree::SPECIALIZATION, "Abomination Limb" );
  talent.blood.improved_bone_shield = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Bone Shield" );
  talent.blood.insatiable_blade     = find_talent_spell( talent_tree::SPECIALIZATION, "Insatiable Blade" );
  talent.blood.deadly_reach         = find_talent_spell( talent_tree::SPECIALIZATION, "Deadly Reach" );
  talent.blood.rapid_decomposition  = find_talent_spell( talent_tree::SPECIALIZATION, "Rapid Decomposition" );
  // Row 7
  talent.blood.boiling_point    = find_talent_spell( talent_tree::SPECIALIZATION, "Boiling Point" );
  talent.blood.lifeblood        = find_talent_spell( talent_tree::SPECIALIZATION, "Lifeblood" );
  talent.blood.everlasting_bond = find_talent_spell( talent_tree::SPECIALIZATION, "Everlasting Bond" );
  talent.blood.voracious        = find_talent_spell( talent_tree::SPECIALIZATION, "Voracious" );
  talent.blood.blood_feast      = find_talent_spell( talent_tree::SPECIALIZATION, "Blood Feast" );
  // Row 8
  talent.blood.plague_infusion = find_talent_spell( talent_tree::SPECIALIZATION, "Plague Infusion" );
  talent.blood.bloody_reflection = find_talent_spell( talent_tree::SPECIALIZATION, "Bloody Reflection" );
  talent.blood.iron_heart      = find_talent_spell( talent_tree::SPECIALIZATION, "Iron Heart" );
  talent.blood.bloodied_blade = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodied Blade" );
  talent.blood.coagulopathy   = find_talent_spell( talent_tree::SPECIALIZATION, "Coagulopathy" );
  // Row 9
  talent.blood.blood_mist      = find_talent_spell( talent_tree::SPECIALIZATION, "Blood Mist" );
  talent.blood.sanguine_ground = find_talent_spell( talent_tree::SPECIALIZATION, "Sanguine Ground" );
  talent.blood.bloodshot         = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodshot" );
  talent.blood.consumption    = find_talent_spell( talent_tree::SPECIALIZATION, "Consumption" );
  talent.blood.red_thirst      = find_talent_spell( talent_tree::SPECIALIZATION, "Red Thirst" );
  // Row 10
  talent.blood.sanguinary_burst  = find_talent_spell( talent_tree::SPECIALIZATION, "Sanguinary Burst" );
  talent.blood.purgatory         = find_talent_spell( talent_tree::SPECIALIZATION, "Purgatory" );
  talent.blood.carnage         = find_talent_spell( talent_tree::SPECIALIZATION, "Carnage" );
  talent.blood.umbilicus_eternus = find_talent_spell( talent_tree::SPECIALIZATION, "Umbilicus Eternus" );
  // Apex
  talent.blood.dance_of_midnight_1 = find_talent_spell( talent_tree::SPECIALIZATION, "Dance of Midnight", 1 );
  talent.blood.dance_of_midnight_2 = find_talent_spell( talent_tree::SPECIALIZATION, "Dance of Midnight", 2 );
  talent.blood.dance_of_midnight_3 = find_talent_spell( talent_tree::SPECIALIZATION, "Dance of Midnight", 3 );

  //////// Frost
  // Row 1
  talent.frost.frost_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Frost Strike" );
  // Row 2
  talent.frost.obliterate    = find_talent_spell( talent_tree::SPECIALIZATION, "Obliterate" );
  talent.frost.howling_blast = find_talent_spell( talent_tree::SPECIALIZATION, "Howling Blast" );
  // Row 3
  talent.frost.killing_machine     = find_talent_spell( talent_tree::SPECIALIZATION, "Killing Machine" );
  talent.frost.empower_rune_weapon = find_talent_spell( talent_tree::SPECIALIZATION, "Empower Rune Weapon" );
  talent.frost.frostscythe         = find_talent_spell( talent_tree::SPECIALIZATION, "Frostscythe" );
  // Row 4
  talent.frost.arctic_assault  = find_talent_spell( talent_tree::SPECIALIZATION, "Arctic Assault" );
  talent.frost.runic_overflow  = find_talent_spell( talent_tree::SPECIALIZATION, "Runic Overflow" );
  talent.frost.frostbound_will = find_talent_spell( talent_tree::SPECIALIZATION, "Frostbound Will" );
  talent.frost.runic_command   = find_talent_spell( talent_tree::SPECIALIZATION, "Runic Command" );
  talent.frost.biting_cold     = find_talent_spell( talent_tree::SPECIALIZATION, "Biting Cold" );
  // Row 5
  talent.frost.frostreaper     = find_talent_spell( talent_tree::SPECIALIZATION, "Frostreaper" );
  talent.frost.pillar_of_frost = find_talent_spell( talent_tree::SPECIALIZATION, "Pillar of Frost" );
  talent.frost.icy_onslaught   = find_talent_spell( talent_tree::SPECIALIZATION, "Icy Onslaught" );
  talent.frost.gathering_storm = find_talent_spell( talent_tree::SPECIALIZATION, "Gathering Storm" );
  // Row 6
  talent.frost.howling_blades     = find_talent_spell( talent_tree::SPECIALIZATION, "Howling Blades" );
  talent.frost.inexorable_assault = find_talent_spell( talent_tree::SPECIALIZATION, "Inexorable Assault" );
  talent.frost.enduring_strength  = find_talent_spell( talent_tree::SPECIALIZATION, "Enduring Strength" );
  talent.frost.frostwyrms_fury    = find_talent_spell( talent_tree::SPECIALIZATION, "Frostwyrm's Fury" );
  talent.frost.frigid_executioner = find_talent_spell( talent_tree::SPECIALIZATION, "Frigid Executioner" );
  // Row 7
  talent.frost.murderous_efficiency = find_talent_spell( talent_tree::SPECIALIZATION, "Murderous Efficiency" );
  talent.frost.cryogenic_chamber    = find_talent_spell( talent_tree::SPECIALIZATION, "Cryogenic Chamber" );
  talent.frost.rage_of_the_frozen_champion =
      find_talent_spell( talent_tree::SPECIALIZATION, "Rage of the Frozen Champion" );
  talent.frost.frozen_dominion = find_talent_spell( talent_tree::SPECIALIZATION, "Frozen Dominion" );
  talent.frost.everfrost       = find_talent_spell( talent_tree::SPECIALIZATION, "Everfrost" );
  talent.frost.northwinds      = find_talent_spell( talent_tree::SPECIALIZATION, "Northwinds" );
  // Row 8
  talent.frost.bonegrinder        = find_talent_spell( talent_tree::SPECIALIZATION, "Bonegrinder" );
  talent.frost.smothering_offense = find_talent_spell( talent_tree::SPECIALIZATION, "Smothering Offense" );
  talent.frost.avalanche          = find_talent_spell( talent_tree::SPECIALIZATION, "Avalanche" );
  talent.frost.icebreaker         = find_talent_spell( talent_tree::SPECIALIZATION, "Icebreaker" );
  // Row 9
  talent.frost.obliteration      = find_talent_spell( talent_tree::SPECIALIZATION, "Obliteration" );
  talent.frost.icy_death_torrent = find_talent_spell( talent_tree::SPECIALIZATION, "Icy Death Torrent" );
  talent.frost.shattering_blade  = find_talent_spell( talent_tree::SPECIALIZATION, "Shattering Blade" );
  talent.frost.hyperpyrexia      = find_talent_spell( talent_tree::SPECIALIZATION, "Hyperpyrexia" );
  // Row 10
  talent.frost.killing_streak       = find_talent_spell( talent_tree::SPECIALIZATION, "Killing Streak" );
  talent.frost.the_long_winter      = find_talent_spell( talent_tree::SPECIALIZATION, "The Long Winter" );
  talent.frost.frostbane            = find_talent_spell( talent_tree::SPECIALIZATION, "Frostbane" );
  talent.frost.breath_of_sindragosa = find_talent_spell( talent_tree::SPECIALIZATION, "Breath of Sindragosa" );
  // Apex
  talent.frost.chosen_of_frostbrood_1 = find_talent_spell( talent_tree::SPECIALIZATION, "Chosen of Frostbrood", 1 );
  talent.frost.chosen_of_frostbrood_2 = find_talent_spell( talent_tree::SPECIALIZATION, "Chosen of Frostbrood", 2 );
  talent.frost.chosen_of_frostbrood_3 = find_talent_spell( talent_tree::SPECIALIZATION, "Chosen of Frostbrood", 3 );

  //////// Unholy
  // Row 1
  talent.unholy.outbreak = find_talent_spell( talent_tree::SPECIALIZATION, "Outbreak" );
  // Row 2
  talent.unholy.scourge_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Scourge Strike" );
  talent.unholy.sudden_doom    = find_talent_spell( talent_tree::SPECIALIZATION, "Sudden Doom" );
  // Row 3
  talent.unholy.clawing_shadows = find_talent_spell( talent_tree::SPECIALIZATION, "Clawing Shadows" );
  talent.unholy.putrefy         = find_talent_spell( talent_tree::SPECIALIZATION, "Putrefy" );
  talent.unholy.dark_transformation = find_talent_spell( talent_tree::SPECIALIZATION, "Dark Transformation" );
  // Row 4
  talent.unholy.foul_infections     = find_talent_spell( talent_tree::SPECIALIZATION, "Foul Infections" );
  talent.unholy.plague_mastery      = find_talent_spell( talent_tree::SPECIALIZATION, "Plague Mastery" );
  talent.unholy.grave_mastery       = find_talent_spell( talent_tree::SPECIALIZATION, "Grave Mastery" );
  talent.unholy.necromancers_cunning = find_talent_spell( talent_tree::SPECIALIZATION, "Necromancer's Cunning" );
  // Row 5
  talent.unholy.ebon_fever        = find_talent_spell( talent_tree::SPECIALIZATION, "Ebon Fever" );
  talent.unholy.festering_scythe  = find_talent_spell( talent_tree::SPECIALIZATION, "Festering Scythe" );
  talent.unholy.superstrain       = find_talent_spell( talent_tree::SPECIALIZATION, "Superstrain" );
  talent.unholy.soul_reaper       = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Reaper" );
  talent.unholy.magus_of_the_dead = find_talent_spell( talent_tree::SPECIALIZATION, "Magus of the Dead" );
  talent.unholy.infected_claws    = find_talent_spell( talent_tree::SPECIALIZATION, "Infected Claws" );
  // Row 6
  talent.unholy.cycle_of_death      = find_talent_spell( talent_tree::SPECIALIZATION, "Cycle of Death" );
  talent.unholy.coil_of_devastation = find_talent_spell( talent_tree::SPECIALIZATION, "Coil of Devastation" );
  talent.unholy.army_of_the_dead    = find_talent_spell( talent_tree::SPECIALIZATION, "Army of the Dead" );
  talent.unholy.reaping             = find_talent_spell( talent_tree::SPECIALIZATION, "Reaping" );
  talent.unholy.menacing_magus      = find_talent_spell( talent_tree::SPECIALIZATION, "Menacing Magus" );
  talent.unholy.ghoulish_frenzy     = find_talent_spell( talent_tree::SPECIALIZATION, "Ghoulish Frenzy" );
  // Row 7
  talent.unholy.morbidity         = find_talent_spell( talent_tree::SPECIALIZATION, "Morbidity" );
  talent.unholy.harbinger_of_doom = find_talent_spell( talent_tree::SPECIALIZATION, "Harbinger of Doom" );
  talent.unholy.raise_abomination = find_talent_spell( talent_tree::SPECIALIZATION, "Raise Abomination" );
  talent.unholy.summon_gargoyle   = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Gargoyle" );
  talent.unholy.unholy_devotion   = find_talent_spell( talent_tree::SPECIALIZATION, "Unholy Devotion" );
  talent.unholy.all_will_serve    = find_talent_spell( talent_tree::SPECIALIZATION, "All Will Serve" );
  // Row 8
  talent.unholy.blightburst          = find_talent_spell( talent_tree::SPECIALIZATION, "Blightburst" );
  talent.unholy.putrid_echoes        = find_talent_spell( talent_tree::SPECIALIZATION, "Putrid Echoes" );
  talent.unholy.doomed_bidding       = find_talent_spell( talent_tree::SPECIALIZATION, "Doomed Bidding" );
  // Row 9
  talent.unholy.scourging             = find_talent_spell( talent_tree::SPECIALIZATION, "Scourging" );
  talent.unholy.ancient_power         = find_talent_spell( talent_tree::SPECIALIZATION, "Ancient Power" );
  talent.unholy.unholy_aura           = find_talent_spell( talent_tree::SPECIALIZATION, "Unholy Aura" );
  talent.unholy.commander_of_the_dead = find_talent_spell( talent_tree::SPECIALIZATION, "Commander of the Dead" );
  // Row 10
  talent.unholy.blightfall  = find_talent_spell( talent_tree::SPECIALIZATION, "Blightfall" );
  talent.unholy.reanimation = find_talent_spell( talent_tree::SPECIALIZATION, "Reanimation" );
  talent.unholy.outnumber   = find_talent_spell( talent_tree::SPECIALIZATION, "Outnumber" );
  // Apex
  talent.unholy.forbidden_knowledge_1 = find_talent_spell( talent_tree::SPECIALIZATION, "Forbidden Knowledge", 1 );
  talent.unholy.forbidden_knowledge_2 = find_talent_spell( talent_tree::SPECIALIZATION, "Forbidden Knowledge", 2 );
  talent.unholy.forbidden_knowledge_3 = find_talent_spell( talent_tree::SPECIALIZATION, "Forbidden Knowledge", 3 );

  //////// Rider of the Apocalypse
  talent.rider.riders_champion        = find_talent_spell( talent_tree::HERO, "Rider's Champion" );
  talent.rider.on_a_paler_horse       = find_talent_spell( talent_tree::HERO, "On a Paler Horse" );
  talent.rider.death_charge           = find_talent_spell( talent_tree::HERO, "Death Charge" );
  talent.rider.ride_or_die            = find_talent_spell( talent_tree::HERO, "Ride or Die!" );
  talent.rider.mograines_might        = find_talent_spell( talent_tree::HERO, "Mograine's Might" );
  talent.rider.horsemens_aid          = find_talent_spell( talent_tree::HERO, "Horsemen's Aid" );
  talent.rider.pact_of_the_apocalypse = find_talent_spell( talent_tree::HERO, "Pact of the Apocalypse" );
  talent.rider.whitemanes_famine      = find_talent_spell( talent_tree::HERO, "Whitemane's Famine" );
  talent.rider.nazgrims_conquest      = find_talent_spell( talent_tree::HERO, "Nazgrim's Conquest" );
  talent.rider.trollbanes_icy_fury    = find_talent_spell( talent_tree::HERO, "Trollbane's Icy Fury" );
  talent.rider.let_terror_reign       = find_talent_spell( talent_tree::HERO, "Let Terror Reign" );
  talent.rider.hungering_thirst       = find_talent_spell( talent_tree::HERO, "Hungering Thirst" );
  talent.rider.fury_of_the_horsemen   = find_talent_spell( talent_tree::HERO, "Fury of the Horsemen" );
  talent.rider.a_feast_of_souls       = find_talent_spell( talent_tree::HERO, "A Feast of Souls" );
  talent.rider.mawsworn_menace        = find_talent_spell( talent_tree::HERO, "Mawsworn Menace" );
  talent.rider.unholy_armaments       = find_talent_spell( talent_tree::HERO, "Unholy Armaments" );
  talent.rider.apocalypse_now         = find_talent_spell( talent_tree::HERO, "Apocalypse Now" );

  //////// Deathbringer
  talent.deathbringer.reapers_mark             = find_talent_spell( talent_tree::HERO, "Reaper's Mark" );
  talent.deathbringer.wave_of_souls            = find_talent_spell( talent_tree::HERO, "Wave of Souls" );
  talent.deathbringer.wither_away              = find_talent_spell( talent_tree::HERO, "Wither Away" );
  talent.deathbringer.bind_in_darkness         = find_talent_spell( talent_tree::HERO, "Bind in Darkness" );
  talent.deathbringer.frigid_resolve           = find_talent_spell( talent_tree::HERO, "Frigid Resolve" );
  talent.deathbringer.soul_rupture             = find_talent_spell( talent_tree::HERO, "Soul Rupture" );
  talent.deathbringer.grim_reaper              = find_talent_spell( talent_tree::HERO, "Grim Reaper" );
  talent.deathbringer.pact_of_the_deathbringer = find_talent_spell( talent_tree::HERO, "Pact of the Deathbringer" );
  talent.deathbringer.rune_carved_plates       = find_talent_spell( talent_tree::HERO, "Rune Carved Plates" );
  talent.deathbringer.deathly_blows            = find_talent_spell( talent_tree::HERO, "Deathly Blows" );
  talent.deathbringer.swift_and_painful        = find_talent_spell( talent_tree::HERO, "Swift and Painful" );
  talent.deathbringer.dark_talons              = find_talent_spell( talent_tree::HERO, "Dark Talons" );
  talent.deathbringer.reapers_onslaught        = find_talent_spell( talent_tree::HERO, "Reaper's Onslaught" );
  talent.deathbringer.deaths_messenger         = find_talent_spell( talent_tree::HERO, "Death's Messenger" );
  talent.deathbringer.expelling_shield         = find_talent_spell( talent_tree::HERO, "Expelling Shield" );
  talent.deathbringer.echoing_fury             = find_talent_spell( talent_tree::HERO, "Echoing Fury" );
  talent.deathbringer.exterminate              = find_talent_spell( talent_tree::HERO, "Exterminate" );

  ///////// San'layn
  talent.sanlayn.vampiric_strike      = find_talent_spell( talent_tree::HERO, "Vampiric Strike" );
  talent.sanlayn.newly_turned         = find_talent_spell( talent_tree::HERO, "Newly Turned" );
  talent.sanlayn.vampiric_speed       = find_talent_spell( talent_tree::HERO, "Vampiric Speed" );
  talent.sanlayn.bloodsoaked_ground   = find_talent_spell( talent_tree::HERO, "Blood-soaked Ground" );
  talent.sanlayn.desecrate            = find_talent_spell( talent_tree::HERO, "Desecrate" );
  talent.sanlayn.thrill_of_blood      = find_talent_spell( talent_tree::HERO, "Thrill of Blood" );
  talent.sanlayn.vampiric_aura        = find_talent_spell( talent_tree::HERO, "Vampiric Aura" );
  talent.sanlayn.bloody_fortitude     = find_talent_spell( talent_tree::HERO, "Bloody Fortitude" );
  talent.sanlayn.infliction_of_sorrow = find_talent_spell( talent_tree::HERO, "Infliction of Sorrow" );
  talent.sanlayn.frenzied_bloodthirst = find_talent_spell( talent_tree::HERO, "Frenzied Bloodthirst" );
  talent.sanlayn.the_blood_is_life    = find_talent_spell( talent_tree::HERO, "The Blood is Life" );
  talent.sanlayn.visceral_strength    = find_talent_spell( talent_tree::HERO, "Visceral Strength" );
  talent.sanlayn.inevitable           = find_talent_spell( talent_tree::HERO, "Inevitable" );
  talent.sanlayn.incite_terror        = find_talent_spell( talent_tree::HERO, "Incite Terror" );
  talent.sanlayn.pact_of_the_sanlayn  = find_talent_spell( talent_tree::HERO, "Pact of the San'layn" );
  talent.sanlayn.sanguine_scent       = find_talent_spell( talent_tree::HERO, "Sanguine Scent" );
  talent.sanlayn.transfusion          = find_talent_spell( talent_tree::HERO, "Transfusion" );
  talent.sanlayn.gift_of_the_sanlayn  = find_talent_spell( talent_tree::HERO, "Gift of the San'layn" );

  // Shared
  spec.frost_fever = find_specialization_spell( "Frost Fever" );  // RP generation only

  mastery.blood_shield        = find_mastery_spell( DEATH_KNIGHT_BLOOD );
  mastery.frozen_heart        = find_mastery_spell( DEATH_KNIGHT_FROST );
  mastery.dreadblade          = find_mastery_spell( DEATH_KNIGHT_UNHOLY );
  mastery.dreadblade_pet_crit = conditional_spell_lookup( mastery.dreadblade->ok(), 1250728 );  // Dreadblade's Pet Crit

  spell_lookups();
  set_icds();

  // Passives that modify effects
  register_passive_effect_mask( talent.improved_death_strike, specialization() == DEATH_KNIGHT_BLOOD
                                                                  ? effect_mask_t( true ).disable( 1, 2, 3 )
                                                                  : effect_mask_t( true ).disable( 4, 5 ) );

  register_passive_effect_mask( talent.deathbringer.frigid_resolve, specialization() == DEATH_KNIGHT_BLOOD
                                                                        ? effect_mask_t( true ).disable( 2 )
                                                                        : effect_mask_t( true ).disable( 1, 3) );

  if ( specialization() == DEATH_KNIGHT_BLOOD )
    register_passive_effect_mask( talent.cleaving_strikes, effect_mask_t( true ).disable( 4 ) );

  if ( main_hand_weapon.group() != WEAPON_2H )
    deregister_passive_spell( spec.might_of_the_frozen_wastes );

  if ( has_runeforge( RUNEFORGE_STONESKIN_GARGOYLE ) )
  {
    parse_passive_effects( runeforge_spell.stoneskin_gargoyle );
    if ( mh_runeforge == RUNEFORGE_STONESKIN_GARGOYLE && oh_runeforge == RUNEFORGE_STONESKIN_GARGOYLE )
    {
      register_passive_effect_override( runeforge_spell.stoneskin_gargoyle->effectN( 1 ),
                                        runeforge_spell.stoneskin_gargoyle->effectN( 1 ).base_value() * 2 );
      register_passive_effect_override( runeforge_spell.stoneskin_gargoyle->effectN( 2 ),
                                        runeforge_spell.stoneskin_gargoyle->effectN( 2 ).base_value() * 2 );
    }
  }

  // Handle Rune Regen Rate effects manually due to the unique nature of runes
  deregister_passive_spell( talent.frost.runic_command );

  parse_all_class_passives();
  parse_all_passive_talents();
  parse_all_passive_sets();
  parse_raid_buffs();

  if ( specialization() == DEATH_KNIGHT_UNHOLY )
    parse_passive_effects( spell.vampiric_strike_range );
}

void death_knight_t::spell_lookups()
{
  // Generic spells
  // Shared
  spell.brittle_debuff         = conditional_spell_lookup( talent.brittle.ok(), 374557 );
  spell.dnd_buff               = conditional_spell_lookup( spec.death_and_decay->ok(), 188290 );
  spell.runic_corruption       = conditional_spell_lookup( spec.death_knight->ok(), 51460 );
  spell.runic_empowerment_gain = conditional_spell_lookup( spec.frost_death_knight->ok(), 193486 );
  spell.rune_mastery_buff      = conditional_spell_lookup( talent.rune_mastery.ok(), 374585 );
  spell.coldthirst_gain        = conditional_spell_lookup( talent.coldthirst.ok(), 378849 );
  spell.death_and_decay_damage = conditional_spell_lookup( spec.death_and_decay->ok(), 52212 );
  spell.death_coil_damage      = conditional_spell_lookup( spec.death_coil->ok(), 47632 );
  spell.death_strike_heal      = conditional_spell_lookup( talent.death_strike.ok(), 45470 );
  spell.anti_magic_zone_buff   = conditional_spell_lookup( talent.antimagic_zone.ok(), 396883 );
  spell.frost_shield_buff      = conditional_spell_lookup( talent.permafrost.ok(), 207203 );
  spell.blood_draw_damage      = conditional_spell_lookup( talent.blood_draw.ok(), 374606 );
  spell.blood_draw_cooldown    = conditional_spell_lookup( talent.blood_draw.ok(), 374609 );

  // Runeforges
  runeforge_spell.apocalypse_death_debuff  = conditional_spell_lookup( has_runeforge( RUNEFORGE_APOCALYPSE ), 327095 );
  runeforge_spell.apocalypse_famine_debuff = conditional_spell_lookup( has_runeforge( RUNEFORGE_APOCALYPSE ), 327092 );
  runeforge_spell.apocalypse_war_debuff    = conditional_spell_lookup( has_runeforge( RUNEFORGE_APOCALYPSE ), 327096 );
  runeforge_spell.apocalypse_pestilence_damage = conditional_spell_lookup( has_runeforge( RUNEFORGE_APOCALYPSE ), 327093 );
  runeforge_spell.razorice_damage       = conditional_spell_lookup( has_runeforge( RUNEFORGE_RAZORICE ), 50401 );
  runeforge_spell.razorice_debuff       = conditional_spell_lookup( spec.glacial_advance->ok() || has_runeforge( RUNEFORGE_RAZORICE ), 51714 );
  runeforge_spell.sanguination_cooldown = conditional_spell_lookup( has_runeforge( RUNEFORGE_SANGUINATION ), 326809 );
  runeforge_spell.sanguination_heal     = conditional_spell_lookup( has_runeforge( RUNEFORGE_SANGUINATION ), 326808 );
  runeforge_spell.spellwarding_driver   = conditional_spell_lookup( has_runeforge( RUNEFORGE_SPELLWARDING ), 326864 );
  runeforge_spell.spellwarding_absorb   = conditional_spell_lookup( has_runeforge( RUNEFORGE_SPELLWARDING ), 326867 );
  runeforge_spell.stoneskin_gargoyle = conditional_spell_lookup( has_runeforge( RUNEFORGE_STONESKIN_GARGOYLE ), 62157 );
  runeforge_spell.unending_thirst    = conditional_spell_lookup( has_runeforge( RUNEFORGE_UNENDING_THIRST ), 326984 );
  runeforge_spell.unholy_strength    = conditional_spell_lookup( has_runeforge( RUNEFORGE_FALLEN_CRUSADER ), 53365 );

  // Diseases
  spell.blood_plague =
      conditional_spell_lookup( spec.blood_death_knight->ok() || talent.unholy.superstrain.ok(), 55078 );
  spell.frost_fever =
      conditional_spell_lookup( talent.frost.howling_blast.ok() || talent.unholy.superstrain.ok(), 55095 );
  spell.virulent_plague = conditional_spell_lookup( talent.unholy.outbreak.ok(), 191587 );

  // Blood
  spell.abomination_limb_debuff     = conditional_spell_lookup( talent.blood.abomination_limb.ok(), 1263566 );
  spell.blood_shield                = conditional_spell_lookup( mastery.blood_shield->ok(), 77535 );
  spell.bloodied_blade_stacks_buff  = conditional_spell_lookup( talent.blood.bloodied_blade.ok(), 460499 );
  spell.bloodied_blade_final_buff   = conditional_spell_lookup( talent.blood.bloodied_blade.ok(), 460500 );
  spell.blood_mist_buff             = conditional_spell_lookup( talent.blood.blood_mist.ok(), 1263729 );
  spell.blood_mist_damage           = conditional_spell_lookup( talent.blood.blood_mist.ok(), 1263752 );
  spell.blood_mist_rp_gain          = conditional_spell_lookup( talent.blood.blood_mist.ok(), 1263774 );
  spell.bloody_reflection_damage    = conditional_spell_lookup( talent.blood.bloody_reflection.ok(), 1279656 );
  spell.boiling_point_buff          = conditional_spell_lookup( talent.blood.boiling_point.ok(), 1265968 );
  spell.boiling_point_echo_buff     = conditional_spell_lookup( talent.blood.boiling_point.ok(), 1265982 );
  spell.bone_shield                 = conditional_spell_lookup( spec.blood_death_knight->ok(), 195181 );
  spell.dance_of_midnight_1_buff    = conditional_spell_lookup( talent.blood.dance_of_midnight_1.ok(), 1264568 );
  spell.dance_of_midnight_2_buff    = conditional_spell_lookup( talent.blood.dance_of_midnight_2.ok(), 1264407 );
  spell.sanguine_ground             = conditional_spell_lookup( talent.blood.sanguine_ground.ok(), 391459 );
  spell.sanguinary_burst_buff       = conditional_spell_lookup( talent.blood.sanguinary_burst.ok(), 1263789 );
  spell.sanguinary_burst_damage     = conditional_spell_lookup( talent.blood.sanguinary_burst.ok(), 1263786 );
  spell.ossuary_buff                = conditional_spell_lookup( talent.blood.ossuary.ok(), 219788 );
  spell.crimson_scourge_buff        = conditional_spell_lookup( spec.crimson_scourge->ok(), 81141 );
  spell.heartbreaker_rp_gain        = conditional_spell_lookup( talent.blood.heartbreaker.ok(), 210738 );
  spell.tightening_grasp_debuff     = conditional_spell_lookup( talent.blood.gorefiends_grasp.ok(), 374776 );
  spell.heart_strike_bloodied_blade = conditional_spell_lookup( talent.blood.bloodied_blade.ok(), 460501 );
  spell.perserverence_of_the_ebon_blade_buff =
      conditional_spell_lookup( talent.blood.perseverance_of_the_ebon_blade.ok(), 374748 );
  spell.voracious_buff           = conditional_spell_lookup( talent.blood.voracious.ok(), 274009 );
  spell.dancing_rune_weapon_buff = conditional_spell_lookup( talent.blood.dancing_rune_weapon.ok(), 81256 );
  spell.relish_in_blood_gains    = conditional_spell_lookup( talent.blood.relish_in_blood.ok(), 317614 );
  spell.leeching_strike_damage   = conditional_spell_lookup( talent.blood.leeching_strike.ok(), 377633 );
  spell.consumption_damage       = conditional_spell_lookup( talent.blood.consumption.ok(), 1263825 );
  spell.consumption_leech        = conditional_spell_lookup( talent.blood.consumption.ok(), 1263872 );
  spell.everlasting_bond_summon  = conditional_spell_lookup( talent.blood.everlasting_bond.ok(), 1237128 );
  spell.dance_of_midnight_summon = conditional_spell_lookup( talent.blood.dance_of_midnight_3.ok(), 1264353 );

  // Blood Tier set spells
  spell.rejuvenating_blood       = conditional_spell_lookup( sets->has_set_bonus( DEATH_KNIGHT_BLOOD, MID1, B2 ), 1271198 );

  // Frost
  spell.murderous_efficiency_gain   = conditional_spell_lookup( talent.frost.murderous_efficiency.ok(), 207062 );
  spell.rage_of_the_frozen_champion = conditional_spell_lookup( talent.frost.rage_of_the_frozen_champion.ok(), 377077 );
  spell.runic_empowerment_chance    = conditional_spell_lookup( spec.frost_death_knight->ok(), 81229 );
  spell.gathering_storm_buff        = conditional_spell_lookup( talent.frost.gathering_storm.ok(), 211805 );
  spell.inexorable_assault_buff     = conditional_spell_lookup( talent.frost.inexorable_assault.ok(), 253595 );
  spell.bonegrinder_crit_buff       = conditional_spell_lookup( talent.frost.bonegrinder.ok(), 377101 );
  spell.bonegrinder_frost_buff      = conditional_spell_lookup( talent.frost.bonegrinder.ok(), 377103 );
  spell.enduring_strength_buff      = conditional_spell_lookup( talent.frost.enduring_strength.ok(), 377195 );
  spell.inexorable_assault_damage   = conditional_spell_lookup( talent.frost.inexorable_assault.ok(), 253597 );
  spell.breath_of_sindragosa_rune_gen = conditional_spell_lookup( talent.frost.breath_of_sindragosa.ok(), 1234304 );
  spell.death_strike_offhand =
      conditional_spell_lookup( talent.death_strike.ok() && off_hand_weapon.type != WEAPON_NONE, 66188 );
  spell.frostwyrms_fury_damage = conditional_spell_lookup( talent.frost.frostwyrms_fury.ok(), 279303 );
  spell.frozen_dominion_buff   = conditional_spell_lookup( talent.frost.frozen_dominion.ok(), 377253 );
  spell.glacial_advance_damage =
      conditional_spell_lookup( spec.glacial_advance->ok() || talent.frost.arctic_assault.ok(), 195975 );
  spell.avalanche_damage           = conditional_spell_lookup( talent.frost.avalanche.ok(), 207150 );
  spell.enduring_strength_cooldown = conditional_spell_lookup( talent.frost.enduring_strength.ok(), 377192 );
  spell.obliteration_gains         = conditional_spell_lookup( talent.frost.obliteration.ok(), 281327 );
  spell.frost_strike_2h =
      conditional_spell_lookup( talent.frost.frost_strike.ok() && main_hand_weapon.group() == WEAPON_2H, 325464 );
  spell.frost_strike_mh =
      conditional_spell_lookup( talent.frost.frost_strike.ok() && main_hand_weapon.group() == WEAPON_1H, 222026 );
  spell.frost_strike_oh =
      conditional_spell_lookup( talent.frost.frost_strike.ok() && off_hand_weapon.type != WEAPON_NONE, 66196 );
  spell.icy_death_torrent_damage     = conditional_spell_lookup( talent.frost.icy_death_torrent.ok(), 439539 );
  spell.cryogenic_chamber_damage     = conditional_spell_lookup( talent.frost.cryogenic_chamber.ok(), 456371 );
  spell.cryogenic_chamber_buff       = conditional_spell_lookup( talent.frost.cryogenic_chamber.ok(), 456370 );
  spell.rime_buff                    = conditional_spell_lookup( spec.rime->ok(), 59052 );
  spell.hyperpyrexia_damage          = conditional_spell_lookup( talent.frost.hyperpyrexia.ok(), 458169 );
  spell.empower_rune_weapon_buff     = conditional_spell_lookup( talent.frost.empower_rune_weapon.ok(), 1230959 );
  spell.frostreaper_debuff           = conditional_spell_lookup( talent.frost.frostreaper.ok(), 1233351 );
  spell.frostreaper_damage           = conditional_spell_lookup( talent.frost.frostreaper.ok(), 1233619 );
  spell.icy_onslaught_buff           = conditional_spell_lookup( talent.frost.icy_onslaught.ok(), 1230273 );
  spell.first_howling_blades_damage  = conditional_spell_lookup( talent.frost.howling_blades.ok(), 1231083 );
  spell.second_howling_blades_damage = conditional_spell_lookup( talent.frost.howling_blades.ok(), 1231082 );
  spell.frozen_dominion_remorseless_winter_buff =
      conditional_spell_lookup( talent.frost.frozen_dominion.ok(), 1233152 );
  spell.frostbane_buff                  = conditional_spell_lookup( talent.frost.frostbane.ok(), 1229310 );
  spell.frostbane_driver                = conditional_spell_lookup( talent.frost.frostbane.ok(), 1228433 );
  spell.frostbane_damage                = conditional_spell_lookup( talent.frost.frostbane.ok(), 1228443 );
  spell.killing_streak_buff             = conditional_spell_lookup( talent.frost.killing_streak.ok(), 1230916 );
  spell.breath_of_sindragosa_erw_refund = conditional_spell_lookup( talent.frost.breath_of_sindragosa.ok(), 303753 );
  // Frost Apex
  spell.chosen_of_frostbrood_haste_buff = conditional_spell_lookup( talent.frost.chosen_of_frostbrood_1.ok(), 1265630 );
  spell.chosen_of_frostbrood_fwf_buff   = conditional_spell_lookup( talent.frost.chosen_of_frostbrood_3.ok(), 1265639 );
  spell.chosen_of_frostbrood_delay      = conditional_spell_lookup( talent.frost.chosen_of_frostbrood_3.ok(), 1265640 );
  spell.chosen_of_frostbrood_fwf_action = conditional_spell_lookup( talent.frost.chosen_of_frostbrood_3.ok(), 1265384 );

  // Unholy
  spell.runic_corruption_chance         = conditional_spell_lookup( spec.unholy_death_knight->ok(), 51462 );
  spell.coil_of_devastation_debuff      = conditional_spell_lookup( talent.unholy.coil_of_devastation.ok(), 390271 );
  spell.ghoulish_frenzy_player          = conditional_spell_lookup( talent.unholy.ghoulish_frenzy.ok(), 377588 );
  spell.commander_of_the_dead           = conditional_spell_lookup( talent.unholy.commander_of_the_dead.ok(), 390260 );
  spell.eternal_agony                   = conditional_spell_lookup( talent.unholy.dark_transformation.ok(), 390268 );
  spell.epidemic_damage                 = conditional_spell_lookup( spec.epidemic->ok(), 212739 );
  spell.outbreak_aoe                    = conditional_spell_lookup( talent.unholy.outbreak.ok(), 196780 );
  spell.festering_scythe                = conditional_spell_lookup( talent.unholy.festering_scythe.ok(), 458128 );
  spell.dark_transformation_player_buff = conditional_spell_lookup( talent.unholy.dark_transformation.ok(), 1235391 );
  spell.dread_plague                    = conditional_spell_lookup( talent.unholy.outbreak.ok(), 1240996 );
  spell.superstrain_energize            = conditional_spell_lookup( talent.unholy.superstrain.ok(), 1242078 );
  spell.clawing_shadows_buff            = conditional_spell_lookup( talent.unholy.clawing_shadows.ok(), 1241569 );
  spell.lesser_ghoul_buff               = conditional_spell_lookup( spec.festering_strike->ok(), 1254252 );
  spell.lesser_ghoul_counter            = conditional_spell_lookup( spec.festering_strike->ok(), 1242998 );
  spell.lesser_ghoul_ticking            = conditional_spell_lookup( spec.festering_strike->ok(), 1255830 );
  spell.infected_claws_dot              = conditional_spell_lookup( talent.unholy.infected_claws.ok(), 1241786 );
  spell.infected_claws_driver           = conditional_spell_lookup( talent.unholy.infected_claws.ok(), 1241792 );
  spell.virulent_plague_erupt           = conditional_spell_lookup( talent.unholy.outbreak.ok(), 1241167 );
  spell.dread_plague_erupt              = conditional_spell_lookup( talent.unholy.outbreak.ok(), 1241171 );
  spell.ancient_runes_buff              = conditional_spell_lookup( talent.unholy.ancient_power.ok(), 377591 );
  spell.disease_cloud_area              = conditional_spell_lookup( talent.unholy.raise_abomination.ok(), 1244103 );
  spell.festering_scythe_debuff         = conditional_spell_lookup( talent.unholy.festering_scythe.ok(), 1241077 );
  spell.dread_plague_death_damage       = conditional_spell_lookup( talent.unholy.outbreak.ok(), 1242564 );
  spell.unholy_aura_mastery_buff        = conditional_spell_lookup( talent.unholy.unholy_aura.ok(), 1268917 );
  spell.putrefy_st                      = conditional_spell_lookup( talent.unholy.putrefy.ok(), 1277016 );
  spell.putrefy_aoe                     = conditional_spell_lookup( talent.unholy.putrefy.ok(), 390220 );
  spell.soul_reaper_debuff              = conditional_spell_lookup( talent.unholy.soul_reaper.ok(), 1241521 );
  spell.festering_scythe_buff           = conditional_spell_lookup( talent.unholy.festering_scythe.ok(), 458123 );
  spell.blightfall                      = conditional_spell_lookup( talent.unholy.blightfall.ok(), 1271967 );
  spell.blightfall_buff                 = conditional_spell_lookup( talent.unholy.blightfall.ok(), 1271975 );
  spell.blightfall_damage               = conditional_spell_lookup( talent.unholy.blightfall.ok(), 1272116 );
  spell.reaping_buff                    = conditional_spell_lookup( talent.unholy.reaping.ok(), 1242654 );
  spell.disease_cloud_damage            = conditional_spell_lookup( talent.unholy.raise_abomination.ok(), 1244347 );
  // Unholy Apex
  spell.forbidden_knowledge_buff = conditional_spell_lookup( talent.unholy.forbidden_knowledge_1.ok(), 1242223 );
  spell.necrotic_coil_action     = conditional_spell_lookup( talent.unholy.forbidden_knowledge_1.ok(), 1242174 );
  spell.necrotic_coil_shadow     = conditional_spell_lookup( talent.unholy.forbidden_knowledge_1.ok(), 1242178 );
  spell.necrotic_coil_physical   = conditional_spell_lookup( talent.unholy.forbidden_knowledge_1.ok(), 1242172 );
  spell.graveyard_action         = conditional_spell_lookup( talent.unholy.forbidden_knowledge_1.ok(), 383269 );
  spell.graveyard_damage         = conditional_spell_lookup( talent.unholy.forbidden_knowledge_1.ok(), 383313 );
  spell.forbidden_sacrifice      = conditional_spell_lookup( talent.unholy.forbidden_knowledge_2.ok(), 1256576 );
  spell.forbidden_ritual         = conditional_spell_lookup( talent.unholy.forbidden_knowledge_3.ok(), 1282570 );

  // Unholy Summon Spells
  spell.summon_gargoyle          = conditional_spell_lookup( talent.unholy.summon_gargoyle.ok(), 49206 );
  spell.summon_abomination       = conditional_spell_lookup( talent.unholy.raise_abomination.ok(), 288853 );
  spell.summon_army_ghoul        = conditional_spell_lookup( talent.unholy.army_of_the_dead.ok(), 1282535 );
  spell.summon_lesser_ghoul      = conditional_spell_lookup( talent.unholy.scourge_strike.ok(), 275430 );
  spell.summon_putrefy_ghoul     = conditional_spell_lookup( talent.unholy.putrefy.ok(), 1277098 );
  spell.summon_magus             = conditional_spell_lookup( talent.unholy.magus_of_the_dead.ok(), 317776 );
  spell.summon_reanimation_magus = conditional_spell_lookup( talent.unholy.reanimation.ok(), 1242294 );
  spell.raise_skulker            = conditional_spell_lookup( talent.unholy.all_will_serve.ok(), 196910 );

  // Unholy Tier Set Spells
  spell.blighted_buff = conditional_spell_lookup( sets->has_set_bonus( DEATH_KNIGHT_UNHOLY, MID1, B4 ), 1271199 );

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
  spell.vampiric_strike_range =
      conditional_spell_lookup( talent.sanlayn.vampiric_strike.ok() && spec.unholy_death_knight->ok(), 445669 );
  spell.incite_terror_debuff          = conditional_spell_lookup( talent.sanlayn.incite_terror.ok(), 458478 );
  spell.visceral_strength_buff        = conditional_spell_lookup( talent.sanlayn.visceral_strength.ok(),
                                                           specialization() == DEATH_KNIGHT_BLOOD ? 461130 : 434159 );
  spell.bloodsoaked_ground_buff = conditional_spell_lookup( talent.sanlayn.bloodsoaked_ground.ok(), 434034 );
  spell.transfusion_buff              = conditional_spell_lookup( talent.sanlayn.transfusion.ok(),
                                                     specialization() == DEATH_KNIGHT_BLOOD ? 1265577 : 1280386 );
  spell.desecrate_damage              = conditional_spell_lookup( talent.sanlayn.desecrate.ok(), 1232346 );

  // Deathbringer Spells
  spell.reapers_mark_debuff          = conditional_spell_lookup( talent.deathbringer.reapers_mark.ok(), 434765 );
  spell.reapers_mark_explosion       = conditional_spell_lookup( talent.deathbringer.reapers_mark.ok(), 436304 );
  spell.wave_of_souls_damage         = conditional_spell_lookup( talent.deathbringer.wave_of_souls.ok(), 435802 );
  spell.wave_of_souls_debuff         = conditional_spell_lookup( talent.deathbringer.wave_of_souls.ok(), 443404 );
  spell.bind_in_darkness_buff        = conditional_spell_lookup( talent.deathbringer.bind_in_darkness.ok(), 443532 );
  spell.dark_talons_shadowfrost_buff = conditional_spell_lookup( talent.deathbringer.dark_talons.ok(), 443586 );
  spell.dark_talons_icy_talons_buff  = conditional_spell_lookup( talent.deathbringer.dark_talons.ok(), 443595 );
  spell.soul_rupture_damage          = conditional_spell_lookup( talent.deathbringer.soul_rupture.ok(), 439594 );
  spell.exterminate_damage =
      conditional_spell_lookup( talent.deathbringer.exterminate.ok() || talent.deathbringer.echoing_fury.ok(), 441424 );
  spell.exterminate_aoe =
      conditional_spell_lookup( talent.deathbringer.exterminate.ok() || talent.deathbringer.echoing_fury.ok(), 441426 );
  spell.exterminate_buff =
      conditional_spell_lookup( talent.deathbringer.exterminate.ok() || talent.deathbringer.echoing_fury.ok(), 441416 );
  spell.rune_carved_plates_physical_buff =
      conditional_spell_lookup( talent.deathbringer.rune_carved_plates.ok(), 440289 );
  spell.rune_carved_plates_magical_buff =
      conditional_spell_lookup( talent.deathbringer.rune_carved_plates.ok(), 440290 );
  spell.swift_and_painful_buff = conditional_spell_lookup( talent.deathbringer.swift_and_painful.ok(), 469169 );

  // Pet abilities
  // Shared
  pet_spell.grave_mastery_buff    = conditional_spell_lookup( talent.unholy.grave_mastery.ok(), 1238902 );
  pet_spell.commander_of_the_dead = conditional_spell_lookup( talent.unholy.commander_of_the_dead.ok(), 390264 );
  // Raise Dead abilities, used for both rank 1 and rank 2
  pet_spell.ghoul_claw           = conditional_spell_lookup( talent.raise_dead.ok(), 91776 );
  pet_spell.sweeping_claws       = conditional_spell_lookup( talent.unholy.dark_transformation.ok(), 91778 );
  pet_spell.gnaw                 = conditional_spell_lookup( talent.raise_dead.ok(), 91800 );
  pet_spell.monstrous_blow       = conditional_spell_lookup( talent.unholy.dark_transformation.ok(), 91797 );
  pet_spell.unholy_devotion_buff = conditional_spell_lookup( talent.unholy.unholy_devotion.ok(), 1270491 );
  pet_spell.ghoulish_frenzy      = conditional_spell_lookup( talent.unholy.ghoulish_frenzy.ok(), 377589 );
  // Army of the dead
  pet_spell.army_claw =
      conditional_spell_lookup( talent.unholy.army_of_the_dead.ok() || talent.unholy.doomed_bidding.ok(), 199373 );
  // All Ghouls
  pet_spell.pet_stun = find_spell( 47466 );
  pet_spell.leap     = find_spell( 91809 );
  // Lesser Ghoul
  pet_spell.lesser_sweeping_claws = conditional_spell_lookup( talent.unholy.outnumber.ok(), 1278150 );
  pet_spell.ruptured_viscera      = conditional_spell_lookup( talent.unholy.necromancers_cunning.ok(), 1247379 );
  // Gargoyle
  pet_spell.gargoyle_strike  = conditional_spell_lookup( talent.unholy.summon_gargoyle.ok(), 51963 );
  pet_spell.dark_empowerment = conditional_spell_lookup( talent.unholy.summon_gargoyle.ok(), 211947 );
  // Risen Skulker (all will serve)
  pet_spell.blighted_arrow          = conditional_spell_lookup( talent.unholy.all_will_serve.ok(), 1239356 );
  pet_spell.blighted_arrow_aoe_buff = conditional_spell_lookup( talent.unholy.all_will_serve.ok(), 1239385 );
  pet_spell.blighted_arrow_st_buff  = conditional_spell_lookup( talent.unholy.all_will_serve.ok(), 1239422 );
  // Magus of the dead
  pet_spell.frostbolt =
      conditional_spell_lookup( talent.unholy.magus_of_the_dead.ok() || talent.unholy.doomed_bidding.ok(), 317792 );
  pet_spell.shadow_bolt =
      conditional_spell_lookup( talent.unholy.magus_of_the_dead.ok() || talent.unholy.doomed_bidding.ok(), 317791 );
  // DRW Spells
  pet_spell.drw_heart_strike =
      conditional_spell_lookup( talent.blood.heart_strike.ok() && talent.blood.dancing_rune_weapon.ok(), 228645 );
  pet_spell.drw_heart_strike_rp_gen =
      conditional_spell_lookup( talent.blood.heart_strike.ok() && talent.blood.dancing_rune_weapon.ok(), 220890 );
  // Rider of the Apocalypse Pet Spells
  pet_spell.apocalyptic_conquest             = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444763 );
  pet_spell.trollbanes_chains_of_ice_ability = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444826 );
  pet_spell.trollbanes_chains_of_ice_debuff  = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444828 );
  pet_spell.trollbanes_icy_fury_ability    = conditional_spell_lookup( talent.rider.trollbanes_icy_fury.ok(), 444834 );
  pet_spell.undeath_dot                    = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444633 );
  pet_spell.undeath_range                  = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444634 );
  pet_spell.mograines_death_and_decay_aura = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444474 );
  pet_spell.mograines_death_and_decay      = conditional_spell_lookup( talent.rider.riders_champion.ok(), 1251951 );
  pet_spell.mograines_might_buff           = conditional_spell_lookup( talent.rider.mograines_might.ok(), 444505 );
  pet_spell.rider_ams                      = conditional_spell_lookup( talent.rider.riders_champion.ok(), 444741 );
  pet_spell.rider_ams_icd                  = conditional_spell_lookup( talent.rider.horsemens_aid.ok(), 451777 );
  pet_spell.whitemane_death_coil           = conditional_spell_lookup( talent.rider.riders_champion.ok(), 445513 );
  pet_spell.whitemane_epidemic             = conditional_spell_lookup( talent.rider.riders_champion.ok(), 1237172 );
  pet_spell.mograine_heart_strike          = conditional_spell_lookup( talent.rider.riders_champion.ok(), 445504 );
  pet_spell.trollbane_obliterate           = conditional_spell_lookup( talent.rider.riders_champion.ok(), 445507 );
  pet_spell.trollbane_frostscythe          = conditional_spell_lookup( talent.rider.let_terror_reign.ok(), 1237388 );
  pet_spell.nazgrim_scourge_strike_phys    = conditional_spell_lookup( talent.rider.riders_champion.ok(), 445508 );
  pet_spell.nazgrim_scourge_strike_shadow  = conditional_spell_lookup( talent.rider.riders_champion.ok(), 445509 );
  // San'layn Pet Spells
  pet_spell.corrupted_blood = conditional_spell_lookup( talent.sanlayn.the_blood_is_life.ok(), 434574 );
  pet_spell.blood_eruption  = conditional_spell_lookup( talent.sanlayn.the_blood_is_life.ok(), 434246 );
  // Abomination Spells
  pet_spell.abomination_disease_cloud = conditional_spell_lookup( talent.unholy.raise_abomination.ok(), 290577 );
}

void death_knight_t::set_icds()
{
  // Custom/Internal cooldowns default durations
  cooldown.bone_shield_icd->duration = spell.bone_shield->internal_cooldown();

  if ( talent.blood.bloody_reflection.ok() )
    cooldown.bloody_reflection_icd->duration = 500_ms;  // Not in spelldata, found via logs Dec 12 2025

  if ( talent.blood.plague_infusion.ok() )
    cooldown.plague_infusion_icd->duration = talent.blood.plague_infusion->internal_cooldown();

  if ( talent.frost.enduring_strength.ok() )
    cooldown.enduring_strength_icd->duration = spell.enduring_strength_cooldown->internal_cooldown();

  if ( talent.frost.inexorable_assault.ok() )
    cooldown.inexorable_assault_icd->duration =
        spell.inexorable_assault_buff->internal_cooldown();  // Inexorable Assault buff spell id
}

// death_knight_t::init_action_list =========================================

void death_knight_t::init_action_list()
{
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

// death_knight_t::init_blizzard_action_list ================================
void death_knight_t::init_blizzard_action_list()
{
  action_priority_list_t* default_ = get_action_priority_list( "default" );
  default_->add_action( "auto_attack" );  // Add before generating the other actions so its always the highest priority
  player_t::init_blizzard_action_list();

  action_priority_list_t* pre_c = get_action_priority_list( "precombat" );
  if ( specialization() == DEATH_KNIGHT_UNHOLY )
    pre_c->add_action( "raise_dead" );

  action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );

  switch ( specialization() )
  {
    case DEATH_KNIGHT_BLOOD:
      cooldowns->add_action( "vampiric_blood" );
      cooldowns->add_action( "raise_dead" );
      break;
    case DEATH_KNIGHT_FROST:
      cooldowns->add_action( "breath_of_sindragosa" );
      cooldowns->add_action( "raise_dead" );
      break;
    case DEATH_KNIGHT_UNHOLY:
      break;
    default:
      break;
  }
}

// death_knight_t::parse_assisted_combat_rule ===============================
parsed_assisted_combat_rule_t death_knight_t::parse_assisted_combat_rule(
    const assisted_combat_rule_data_t& rule, const assisted_combat_step_data_t& step ) const
{
  if ( rule.condition_type == AC_AURA_ON_PLAYER && rule.condition_value_1 == spell.lesser_ghoul_counter->id() )
  {
    if ( rule.condition_value_2 >= 1 )
      return { "buff.lesser_ghoul_counter.up" };
    if ( rule.condition_value_2 == 0 )
      return { "buff.lesser_ghoul_counter.down" };
  }

  if ( rule.condition_type == AC_AURA_ON_PLAYER && rule.condition_value_1 == spell.lesser_ghoul_buff->id() )
  {
    if ( rule.condition_value_2 >= 1 )
      return { "buff.lesser_ghoul_ready.up" };
    if ( rule.condition_value_2 == 0 )
      return { "buff.lesser_ghoul_ready.down" };
  }

  if ( rule.condition_type == AC_AURA_MISSING_TARGET && rule.condition_value_1 == spell.festering_scythe_debuff->id() )
  {
    if ( rule.condition_value_2 >= 1 )
      return { "buff.festering_scythe_tt.up" };
    if ( rule.condition_value_2 == 0 )
      return { "buff.festering_scythe_tt.down" };
  }

  if ( rule.condition_type == AC_AURA_ON_TARGET && rule.condition_value_1 == 194310 )
    return { "target.health.pct<=0",
             "Leftover line from old Unholy, references Festering Wounds. Since this can never trigger in game, "
             "giving it a condition thatll never be met.",
             true };

  return player_t::parse_assisted_combat_rule( rule, step );
}

// death_knight_t::action_names_from_spell_id ===============================
std::vector<std::string> death_knight_t::action_names_from_spell_id( unsigned int spell_id ) const
{
  if ( spell_id == 316239 )  // Rune Strike
  {
    switch ( specialization() )
    {
      case DEATH_KNIGHT_BLOOD:
        spell_id = talent.blood.heart_strike->id();
        break;
      case DEATH_KNIGHT_FROST:
        spell_id = talent.frost.obliterate
                       ->id();  // Seems they use Obliterate as a replacement for Rune Strike rather than Frost Strike
        break;
      case DEATH_KNIGHT_UNHOLY:
        spell_id = spec.festering_strike->id();
        break;
      default:
        break;
    }
  }

  return player_t::action_names_from_spell_id( spell_id );
}

std::string death_knight_t::aura_expr_from_spell_id( unsigned int spell_id, bool on_self ) const
{
  std::string aura_expr = player_t::aura_expr_from_spell_id( spell_id, on_self );
  if ( aura_expr == "debuff.reapers_mark" )
    aura_expr.append( "_debuff" );

  return aura_expr;
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
  dot.dread_plague    = target.get_dot( "dread_plague", &p );

  // Rider of the Apocalypse
  dot.undeath = target.get_dot( "undeath", &p );

  // Deathbringer

  using namespace debuffs;

  // Shared
  debuff.brittle = make_debuff( p.talent.brittle.ok(), *this, "brittle", p.spell.brittle_debuff )
                       ->set_default_value_from_effect( 1 );

  debuff.razorice = make_debuff( p.spec.glacial_advance->ok() || p.talent.frost.avalanche->ok() ||
                                     p.talent.frost.arctic_assault->ok() || p.has_runeforge( RUNEFORGE_RAZORICE ),
                                 *this, "razorice", p.runeforge_spell.razorice_debuff )
                        ->set_default_value_from_effect( 1 )
                        ->disable_ticking( true );

  debuff.apocalypse_death = make_debuff( p.has_runeforge( RUNEFORGE_APOCALYPSE ), *this, "death",
                                         p.runeforge_spell.apocalypse_death_debuff );  // Effect not implemented

  debuff.apocalypse_famine = make_debuff( p.has_runeforge( RUNEFORGE_APOCALYPSE ), *this, "famine",
                                          p.runeforge_spell.apocalypse_famine_debuff )
                                 ->set_default_value_from_effect( 1 );

  debuff.apocalypse_war =
      make_debuff( p.has_runeforge( RUNEFORGE_APOCALYPSE ), *this, "war", p.runeforge_spell.apocalypse_war_debuff )
          ->set_default_value_from_effect( 1 );

  // Blood
  debuff.abomination_limb = make_debuff( p.talent.blood.abomination_limb.ok(), *this, "abomination_limb", p.spell.abomination_limb_debuff );
  debuff.tightening_grasp = make_debuff( p.talent.blood.gorefiends_grasp.ok(), *this, "tightening_grasp", p.spell.tightening_grasp_debuff );

  // Frost
  debuff.everfrost =
      make_debuff( p.talent.frost.everfrost.ok(), *this, "everfrost", p.talent.frost.everfrost->effectN( 1 ).trigger() )
          ->set_default_value( p.talent.frost.everfrost->effectN( 1 ).percent() );

  debuff.frostreaper = make_debuff( p.talent.frost.frostreaper.ok(), *this, "frostreaper", p.spell.frostreaper_debuff )
                           ->set_refresh_duration_callback( [ & ]( const buff_t* b, timespan_t time ) {
                             p.background_actions.frostreaper->execute_on_target( b->player );
                             return time;
                           } );

  flag.razorice_consumed = false;

  // Unholy
  dot.infected_claws = target.get_dot( "infected_claws", &p );

  debuff.soul_reaper =
      make_debuff( p.talent.unholy.soul_reaper.ok(), *this, "soul_reaper_debuff", p.spell.soul_reaper_debuff );

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
            if ( !p.sim->event_mgr.canceled )
              p.reapers_mark_explosion_wrapper( buff->player, buff->source, stacks );
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
  buffs.antimagic_shell = make_buff<antimagic_shell_buff_t>( this, "antimagic_shell", spec.antimagic_shell );

  buffs.antimagic_zone = make_buff<antimagic_zone_buff_t>( this, "antimagic_zone", spell.anti_magic_zone_buff );

  buffs.blood_draw =
      make_fallback( talent.blood_draw.ok(), this, "blood_draw", talent.blood_draw->effectN( 4 ).trigger() );

  buffs.icebound_fortitude =
      make_fallback( talent.icebound_fortitude.ok(), this, "icebound_fortitude", talent.icebound_fortitude )
          ->set_duration( talent.icebound_fortitude->duration() )
          ->set_cooldown( 0_ms );  // Handled by the action

  buffs.rune_mastery = make_fallback( talent.rune_mastery.ok(), this, "rune_mastery", spell.rune_mastery_buff )
                           ->set_chance( 0.15 )  // This was found through testing 2022 July 21.  Not in spelldata.
                           ->set_default_value( talent.rune_mastery->effectN( 1 ).percent() );

  buffs.icy_talons =
      make_fallback( talent.icy_talons.ok(), this, "icy_talons", talent.icy_talons->effectN( 1 ).trigger() )
          ->set_default_value( talent.icy_talons->effectN( 1 ).percent() )
          ->set_cooldown( talent.icy_talons->internal_cooldown() )
          ->set_trigger_spell( talent.icy_talons )
          ->increase_max_stack_uptime( as<int>( talent.deathbringer.dark_talons->effectN( 2 ).base_value() ) )
          ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
            if ( talent.deathbringer.dark_talons.ok() )
            {
              if ( new_ >= 1 )
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

  buffs.death_and_decay =
      make_fallback<death_and_decay_buff_t>( spec.death_and_decay->ok(), this, "death_and_decay", spell.dnd_buff );

  buffs.runic_corruption = make_fallback<runic_corruption_buff_t>( specialization() == DEATH_KNIGHT_UNHOLY, this,
                                                                   "runic_corruption", spell.runic_corruption );

  // Rider of the Apocalypse
  buffs.antimagic_shell_horsemen =
      make_buff<antimagic_shell_buff_horseman_t>( this, "antimagic_shell_horsemen", pet_spell.rider_ams );

  buffs.antimagic_shell_horsemen_icd =
      make_fallback( talent.rider.horsemens_aid.ok(), this, "antimagic_shell_horsemen_icd", pet_spell.rider_ams_icd )
          ->set_quiet( true );

  buffs.apocalyptic_conquest =
      make_fallback<apocalyptic_conquest_buff_t>( talent.rider.riders_champion.ok(), this, "apocalyptic_conquest",
                                                  pet_spell.apocalyptic_conquest )
          ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  buffs.mograines_might =
      make_fallback( talent.rider.mograines_might.ok(), this, "mograines_might", pet_spell.mograines_might_buff )
          ->set_default_value_from_effect( 1 )
          ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  buffs.a_feast_of_souls =
      make_fallback( talent.rider.a_feast_of_souls.ok(), this, "a_feast_of_souls", spell.a_feast_of_souls_buff )
          ->set_duration( 0_ms )
          ->set_default_value_from_effect( 1 );

  // Deathbringer

  buffs.bind_in_darkness =
      make_fallback( talent.deathbringer.bind_in_darkness.ok(), this, "bind_in_darkness", spell.bind_in_darkness_buff )
          ->set_trigger_spell( talent.deathbringer.bind_in_darkness );

  buffs.dark_talons_shadowfrost = make_fallback( talent.deathbringer.dark_talons.ok(), this, "dark_talons_shadowfrost",
                                                 spell.dark_talons_shadowfrost_buff )
                                      ->set_quiet( true );

  buffs.dark_talons_icy_talons =
      make_fallback( talent.deathbringer.dark_talons.ok(), this, "dark_talons", spell.dark_talons_icy_talons_buff )
          ->set_stack_change_callback( [ this ]( buff_t*, int old_, int new_ ) {
            int it_stack_modifier = as<int>( talent.deathbringer.dark_talons->effectN( 2 ).base_value() );
            if ( new_ > old_ )
            {
              buffs.icy_talons->set_max_stack( buffs.icy_talons->max_stack() + it_stack_modifier );
              buffs.icy_talons->increment( it_stack_modifier );
            }
            else if ( new_ < old_ )
            {
              buffs.icy_talons->set_max_stack( buffs.icy_talons->max_stack() -
                                               ( ( old_ - new_ ) * it_stack_modifier ) );
              buffs.icy_talons->decrement( it_stack_modifier );
            }
          } );

  buffs.exterminate = make_fallback( talent.deathbringer.exterminate.ok() || talent.deathbringer.echoing_fury.ok(),
                                     this, "exterminate", spell.exterminate_buff );

  buffs.swift_and_painful = make_fallback( talent.deathbringer.swift_and_painful.ok(), this, "swift_and_painful",
                                           spell.swift_and_painful_buff );

  buffs.rune_carved_plates_physical_buff =
      make_fallback( talent.deathbringer.rune_carved_plates.ok(), this, "rune_carved_plates_physical",
                     spell.rune_carved_plates_physical_buff )
          ->set_default_value( spell.rune_carved_plates_physical_buff->effectN( 1 ).base_value() / 1000 );

  buffs.rune_carved_plates_magical_buff =
      make_fallback( talent.deathbringer.rune_carved_plates.ok(), this, "rune_carved_plates_magical",
                     spell.rune_carved_plates_magical_buff )
          ->set_default_value( spell.rune_carved_plates_magical_buff->effectN( 1 ).base_value() / 1000 );

  // San'layn
  buffs.essence_of_the_blood_queen = make_fallback<essence_of_the_blood_queen_buff_t>(
      talent.sanlayn.vampiric_strike.ok(), this, "essence_of_the_blood_queen", spell.essence_of_the_blood_queen_buff );

  buffs.gift_of_the_sanlayn = make_fallback<gift_of_the_sanlayn_buff_t>(
      talent.sanlayn.gift_of_the_sanlayn.ok(), this, "gift_of_the_sanlayn", spell.gift_of_the_sanlayn_buff );

  buffs.vampiric_strike =
      make_fallback( talent.sanlayn.vampiric_strike.ok(), this, "vampiric_strike", spell.vampiric_strike_buff );

  buffs.visceral_strength =
      make_fallback( talent.sanlayn.visceral_strength.ok(), this, "visceral_strength", spell.visceral_strength_buff )
          ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
          ->add_invalidate( CACHE_STRENGTH )
          ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );

  buffs.bloodsoaked_ground = make_fallback( talent.sanlayn.bloodsoaked_ground.ok(), this, "bloodsoaked_ground",
                                            spell.bloodsoaked_ground_buff );

  // Blood
  if ( this->specialization() == DEATH_KNIGHT_BLOOD )
  {
    buffs.blood_shield = make_buff<blood_shield_buff_t>( this );

    buffs.boiling_point = make_buff( this, "boiling_point", spell.boiling_point_buff );
    buffs.boiling_point_echo = make_buff( this, "boiling_point_echo", spell.boiling_point_echo_buff )
              ->set_expire_callback( [ this ]( buff_t*, int, timespan_t ) {
                        background_actions.blood_boil_boiling_point->execute();
              } );

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
            ->set_cooldown( 0_ms );

    buffs.bloodied_blade_stacks =
        make_buff( this, "bloodied_blade_stacks", spell.bloodied_blade_stacks_buff )
            ->set_default_value( spell.bloodied_blade_stacks_buff->effectN( 1 ).percent() * 0.1 )
            ->add_invalidate( CACHE_STRENGTH )
            ->set_cooldown( spell.bloodied_blade_stacks_buff->internal_cooldown() )
            ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );

    buffs.bloodied_blade_final = make_buff( this, "bloodied_blade_final", spell.bloodied_blade_final_buff )
                                     ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
                                     ->add_invalidate( CACHE_STRENGTH )
                                     ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );

    buffs.blood_mist = make_fallback( talent.blood.blood_mist->ok(), this, "blood_mist", spell.blood_mist_buff )
          ->set_tick_callback(
              [ this ]( buff_t*, int, timespan_t ) { background_actions.blood_mist_tick->execute(); } )
          ->set_expire_callback(
              [ this ]( buff_t*, int, timespan_t) { if ( talent.blood.sanguinary_burst.ok() ) background_actions.sanguinary_burst->execute(); } );

    buffs.ossuary = make_buff( this, "ossuary", spell.ossuary_buff )->set_default_value_from_effect( 1, 0.1 );

    buffs.coagulopathy = make_buff( this, "coagulopathy", talent.blood.coagulopathy->effectN( 2 ).trigger() )
                             ->set_trigger_spell( talent.blood.coagulopathy )
                             ->set_default_value_from_effect( 1 );

    buffs.consumption = make_buff( this, "consumption", talent.blood.consumption );

    buffs.crimson_scourge =
        make_buff( this, "crimson_scourge", spell.crimson_scourge_buff )->set_trigger_spell( spec.crimson_scourge );

    buffs.dancing_rune_weapon =
        make_buff( this, "dancing_rune_weapon", spell.dancing_rune_weapon_buff )
            ->set_cooldown( 0_ms )
            ->set_refresh_behavior( buff_refresh_behavior::DURATION )
            ->set_default_value_from_effect_type( A_MOD_PARRY_PERCENT )
            ->set_expire_callback( [ this ]( buff_t*, int, timespan_t ) {
              if ( talent.sanlayn.the_blood_is_life.ok() && pets.blood_beast.active_pet() != nullptr )
                pets.blood_beast.active_pet()->demise();
            } );

    buffs.hemostasis = make_buff( this, "hemostasis", talent.blood.hemostasis->effectN( 1 ).trigger() )
                           ->set_trigger_spell( talent.blood.hemostasis )
                           ->set_default_value_from_effect( 1 );

    buffs.perseverance_of_the_ebon_blade =
        make_buff( this, "perseverance_of_the_ebon_blade", spell.perserverence_of_the_ebon_blade_buff );

    buffs.sanguine_ground = make_buff( this, "sanguine_ground", spell.sanguine_ground )
                                ->set_default_value_from_effect( 1 )
                                ->set_duration( 0_ms )  // Handled by trigger_dnd_buffs() & expire_dnd_buffs()
                                ->set_schools_from_effect( 1 );

    buffs.sanguinary_burst = make_fallback( talent.blood.sanguinary_burst.ok(), this, "sanguinary_burst", spell.sanguinary_burst_buff )
                                  ->set_max_stack( 999 );  // Set to 1 in spelldata

    buffs.vampiric_blood =
        make_buff( this, "vampiric_blood", talent.blood.vampiric_blood )
            ->set_cooldown( 0_ms )
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
              }
            } );

    buffs.voracious = make_buff( this, "voracious", spell.voracious_buff )->set_trigger_spell( talent.blood.voracious );

    buffs.dance_of_midnight_1 = make_fallback( talent.blood.dance_of_midnight_1.ok(), this, "dance_of_midnight_1", spell.dance_of_midnight_1_buff )
                                  ->set_chance( 1.0 )
                                  ->set_cooldown( talent.blood.dance_of_midnight_1->internal_cooldown() );
                            
    buffs.dance_of_midnight_2 = make_fallback( talent.blood.dance_of_midnight_2.ok(), this, "dance_of_midnight_2", spell.dance_of_midnight_2_buff )
                                  ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                                  ->set_disable_async_expire_events_removal( true );

    // Tier Sets
  }

  // Frost
  buffs.breath_of_sindragosa = make_fallback<breath_of_sindragosa_buff_t>(
      talent.frost.breath_of_sindragosa.ok(), this, "breath_of_sindragosa", talent.frost.breath_of_sindragosa );

  buffs.gathering_storm =
      make_fallback( talent.frost.gathering_storm.ok(), this, "gathering_storm", spell.gathering_storm_buff )
          ->set_trigger_spell( talent.frost.gathering_storm )
          ->set_default_value_from_effect( 1 );

  buffs.inexorable_assault =
      make_fallback( talent.frost.inexorable_assault.ok(), this, "inexorable_assault", spell.inexorable_assault_buff )
          ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

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

  buffs.empower_rune_weapon = make_fallback( talent.frost.empower_rune_weapon.ok(), this, "empower_rune_weapon",
                                             spell.empower_rune_weapon_buff );

  buffs.pillar_of_frost = make_fallback<pillar_of_frost_buff_t>( talent.frost.pillar_of_frost.ok(), this,
                                                                 "pillar_of_frost", talent.frost.pillar_of_frost );

  buffs.remorseless_winter =
      make_fallback( spec.remorseless_winter->ok() && !talent.frost.frozen_dominion->ok(), this, "remorseless_winter",
                     spec.remorseless_winter )
          ->set_cooldown( 0_ms )  // Handled by the action
          ->set_refresh_behavior( buff_refresh_behavior::DURATION )
          ->set_partial_tick( true )
          ->set_tick_callback(
              [ this ]( buff_t*, int, timespan_t ) { background_actions.remorseless_winter_tick->execute(); } )
          ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
            if ( !new_ )
            {
              buffs.gathering_storm->expire();
            }
            else
            {
              debug_cast<remorseless_winter_damage_t*>( background_actions.remorseless_winter_tick )->clear_state();
            }
          } );

  buffs.frozen_dominion_remorseless_winter =
      make_fallback( talent.frost.frozen_dominion->ok(), this, "remorseless_winter",
                     spell.frozen_dominion_remorseless_winter_buff )
          ->set_cooldown( 0_ms )  // Handled by the action
          ->set_refresh_behavior( buff_refresh_behavior::DURATION )
          ->set_partial_tick( true )
          ->set_tick_callback(
              [ this ]( buff_t*, int, timespan_t ) { background_actions.remorseless_winter_tick->execute(); } )
          ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
            if ( !new_ )
            {
              buffs.gathering_storm->expire();
            }
            else
            {
              debug_cast<remorseless_winter_damage_t*>( background_actions.remorseless_winter_tick )->clear_state();
            }
          } );

  buffs.rime = make_fallback( spec.rime->ok(), this, "rime", spell.rime_buff )
                   ->set_trigger_spell( spec.rime )
                   ->set_chance( spec.rime->effectN( 2 ).percent() )
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
          ->set_cooldown( talent.frost.bonegrinder->internal_cooldown() )
          ->set_max_stack( spell.bonegrinder_crit_buff->max_stacks() - 1 );

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

  buffs.frozen_dominion =
      make_fallback( talent.frost.frozen_dominion.ok(), this, "frozen_dominion", spell.frozen_dominion_buff )
          ->set_default_value( spell.frozen_dominion_buff->effectN( 1 ).base_value() )
          ->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buffs.cryogenic_chamber = make_fallback<cryogenic_chamber_buff_t>(
      talent.frost.cryogenic_chamber.ok(), this, "cryogenic_chamber", spell.cryogenic_chamber_buff );

  buffs.icy_onslaught =
      make_fallback( talent.frost.icy_onslaught.ok(), this, "icy_onslaught", spell.icy_onslaught_buff );

  buffs.frostbane = make_fallback( talent.frost.frostbane.ok(), this, "frostbane", spell.frostbane_buff );

  buffs.killing_streak =
      make_fallback( talent.frost.killing_streak.ok(), this, "killing_streak", spell.killing_streak_buff )
          ->set_default_value( spell.killing_streak_buff->effectN( 1 ).percent() / 10 )
          ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
          ->set_disable_async_expire_events_removal( true );

  buffs.chosen_of_frostbrood_haste =
      make_fallback<chosen_of_frostbrood_haste_buff_t>( talent.frost.chosen_of_frostbrood_1.ok(), this,
                                                        "chosen_of_frostbrood_haste",
                                                        spell.chosen_of_frostbrood_haste_buff )
          ->set_default_value( spell.chosen_of_frostbrood_haste_buff->effectN( 1 ).percent() );

  buffs.chosen_of_frostbrood_fwf = make_fallback( talent.frost.chosen_of_frostbrood_3.ok(), this,
                                                  "chosen_of_frostbrood_fwf", spell.chosen_of_frostbrood_fwf_buff )
                                       ->set_name_reporting( "FWF" );

  buffs.empowered_strikes =
      make_fallback( sets->has_set_bonus( DEATH_KNIGHT_FROST, MID1, B2 ), this, "empowered_strikes",
                     sets->set( DEATH_KNIGHT_FROST, MID1, B2 )->effectN( 1 ).trigger() );

  buffs.frost_mid1_4pc_buff = make_fallback( sets->has_set_bonus( DEATH_KNIGHT_FROST, MID1, B4 ), this, "mid1_4pc_buff",
                                             sets->set( DEATH_KNIGHT_FROST, MID1, B4 )->effectN( 2 ).trigger() );

  buffs.freezing_tempest = make_fallback( sets->has_set_bonus( DEATH_KNIGHT_FROST, MID2, B2 ), this, "freezing_tempest",
                                          sets->set( DEATH_KNIGHT_FROST, MID2, B2 )->effectN( 1 ).trigger() );


  // Unholy
  buffs.dark_transformation = make_fallback<dark_transformation_buff_t>(
      talent.unholy.dark_transformation.ok(), this, "dark_transformation", spell.dark_transformation_player_buff );

  buffs.sudden_doom = make_fallback( talent.unholy.sudden_doom.ok(), this, "sudden_doom",
                                     talent.unholy.sudden_doom->effectN( 1 ).trigger() )
                          ->set_trigger_spell( talent.unholy.sudden_doom )
                          ->set_consume_all_stacks( false );

  buffs.ghoulish_frenzy =
      make_fallback( talent.unholy.ghoulish_frenzy.ok(), this, "ghoulish_frenzy", spell.ghoulish_frenzy_player )
          ->set_duration( 0_ms )  // Handled by DT
          ->set_default_value( spell.ghoulish_frenzy_player->effectN( 1 ).percent() );

  buffs.commander_of_the_dead = make_fallback( talent.unholy.commander_of_the_dead.ok(), this, "commander_of_the_dead",
                                               spell.commander_of_the_dead );

  buffs.clawing_shadows =
      make_fallback( talent.unholy.clawing_shadows.ok(), this, "clawing_shadows", spell.clawing_shadows_buff )
          ->set_default_value_from_effect( 1 );

  buffs.lesser_ghoul_ready =
      make_fallback( spec.festering_strike->ok(), this, "lesser_ghoul_ready", spell.lesser_ghoul_buff )
          ->set_name_reporting( "lesser_ghoul" );

  buffs.lesser_ghoul_counter =
      make_fallback( talent.unholy.outnumber.ok(), this, "lesser_ghoul_counter", spell.lesser_ghoul_counter )
          ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  buffs.forbidden_knowledge = make_fallback( talent.unholy.forbidden_knowledge_1.ok(), this, "forbidden_knowledge",
                                             spell.forbidden_knowledge_buff );

  buffs.ancient_power =
      make_fallback( talent.unholy.ancient_power.ok(), this, "ancient_power", spell.ancient_runes_buff )
          ->set_default_value( talent.unholy.ancient_power->effectN( 1 ).percent() / 100 )
          ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
          ->add_invalidate( CACHE_STRENGTH )
          ->set_disable_async_expire_events_removal( true );

  buffs.unholy_aura_haste =
      make_fallback( talent.unholy.unholy_aura.ok(), this, "unholy_aura", spell.unholy_aura_mastery_buff )
          ->set_default_value( talent.unholy.unholy_aura->effectN( 1 ).percent() )
          ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
          ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  buffs.festering_scythe =
      make_fallback( talent.unholy.festering_scythe.ok(), this, "festering_scythe", spell.festering_scythe_buff );

  buffs.festering_scythe_tt =
      make_fallback( talent.unholy.festering_scythe.ok(), this, "festering_scythe_tt", spell.festering_scythe_debuff );

  buffs.blightfall = make_fallback( talent.unholy.blightfall.ok(), this, "blightfall", spell.blightfall_buff );

  buffs.forbidden_sacrifice =
      make_fallback( talent.unholy.forbidden_knowledge_2.ok(), this, "forbidden_sacrifice", spell.forbidden_sacrifice )
          ->set_default_value( talent.unholy.forbidden_knowledge_2->effectN( 1 ).base_value() / 1.8 )
          ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
          ->add_invalidate( CACHE_MASTERY )
          ->set_disable_async_expire_events_removal( true );

  buffs.forbidden_ritual =
      make_fallback( talent.unholy.forbidden_knowledge_3.ok(), this, "forbidden_ritual", spell.forbidden_ritual )
          ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  buffs.reaping = make_fallback( talent.unholy.reaping.ok(), this, "reaping", spell.reaping_buff );

  // Tier Sets
  buffs.blighted =
      make_fallback( sets->has_set_bonus( DEATH_KNIGHT_UNHOLY, MID1, B4 ), this, "blighted", spell.blighted_buff );
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

  // Blood
  gains.blood_mist       = get_gain( "Blood Mist" );
  gains.consumption      = get_gain( "Consumption" );
  gains.drw_heart_strike = get_gain( "Rune Weapon Heart Strike" );
  gains.heartbreaker     = get_gain( "Heartbreaker" );
  gains.blood_mid1_2pc   = get_gain( "Blood MID1 2pc");

  // Frost
  gains.breath_of_sindragosa        = get_gain( "Breath of Sindragosa" );
  gains.empower_rune_weapon         = get_gain( "Empower Rune Weapon" );
  gains.frost_fever                 = get_gain( "Frost Fever" );
  gains.murderous_efficiency        = get_gain( "Murderous Efficiency" );
  gains.obliteration                = get_gain( "Obliteration" );
  gains.rage_of_the_frozen_champion = get_gain( "Rage of the Frozen Champion" );
  gains.runic_attenuation           = get_gain( "Runic Attenuation" );
  gains.runic_empowerment           = get_gain( "Runic Empowerment" );
  gains.chosen_of_frostbrood        = get_gain( "Chosen of Frostbrood" );

  // Unholy
  gains.forbidden_knowledge = get_gain( "Forbidden Knowledge" );
  gains.superstrain         = get_gain( "Superstrain" );

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
  procs.km_from_grim_reaper     = get_proc( "Killing Machine: Grim Reaper" );
  procs.km_from_erw             = get_proc( "Killing Machine: Empower Rune Weapon" );
  procs.km_from_howling_blades  = get_proc( "Killing Machine: Howling Blades" );
  procs.km_from_exterminate     = get_proc( "Killing Machine: Exterminate" );

  procs.km_from_crit_aa_wasted         = get_proc( "Killing Machine wasted: Critical auto attacks" );
  procs.km_from_obliteration_fs_wasted = get_proc( "Killing Machine wasted: Frost Strike" );
  procs.km_from_obliteration_hb_wasted = get_proc( "Killing Machine wasted: Howling Blast" );
  procs.km_from_obliteration_ga_wasted = get_proc( "Killing Machine wasted: Glacial Advance" );
  procs.km_from_grim_reaper_wasted     = get_proc( "Killing Machine wasted: Grim Reaper" );
  procs.km_from_erw_wasted             = get_proc( "Killing Machine wasted: Empower Rune Weapon" );
  procs.km_from_howling_blades_wasted  = get_proc( "Killing Machine wasted: Howling Blades" );
  procs.km_from_exterminate_wasted     = get_proc( "Killing Machine wasted: Exterminate" );

  procs.razorice_from_arctic_assault  = get_proc( "Razorice from Arctic Assault" );
  procs.razorice_from_avalanche       = get_proc( "Razorice from Avalanche" );
  procs.razorice_from_runeforge       = get_proc( "Razorice from Runeforge" );
  procs.razorice_from_glacial_advance = get_proc( "Razorice from Glacial Advance" );

  procs.ready_rune = get_proc( "Rune ready" );

  procs.lesser_ghoul_army = get_proc( "Lesser Ghoul from Army" );
  procs.lesser_ghoul_db   = get_proc( "Lesser Ghoul from Doomed Bidding" );
  procs.lesser_ghoul_fs   = get_proc( "Lesser Ghoul from Scourge Strike" );

  procs.rp_runic_corruption = get_proc( "Runic Corruption from Runic Power Spent" );

  procs.bloodworms = get_proc( "Bloodworms" );
  procs.carnage    = get_proc( "Carnage" );

  procs.coil_vs = get_proc( "Coils cast by Visceral Strength Proc" );
  procs.epi_vs  = get_proc( "Epidemic cast by Visceral Strength Proc" );

  procs.blood_beast           = get_proc( "Blood Beast" );
  procs.vampiric_strike       = get_proc( "Vampiric Strike Proc" );
  procs.vampiric_strike_waste = get_proc( "Vampiric Strike Proc Wasted" );
}

// death_knight_t::init_uptimes =============================================
void death_knight_t::init_uptimes()
{
  player_t::init_uptimes();
  if ( options.extra_unholy_reporting )
  {
    if ( talent.unholy.commander_of_the_dead.ok() )
      sample_data.lesser_ghoul_duration = std::make_unique<extended_sample_data_t>( "Lesser Ghoul Duration", false );

    sample_data.lesser_ghouls_summoned = std::make_unique<extended_sample_data_t>( "Lesser Ghouls Summoned", false );
    sample_data.lesser_ghouls_active   = std::make_unique<extended_sample_data_t>( "Lesser Ghouls Active", false );
    sample_data.magus_active           = std::make_unique<extended_sample_data_t>( "Magus Active", false );

    if ( talent.unholy.blightfall.ok() )
    {
      sample_data.blightfall_dp_dur = std::make_unique<extended_sample_data_t>( "Dread Plague Consume Duration", false );
      sample_data.blightfall_vp_dur = std::make_unique<extended_sample_data_t>( "Virulent Plague Consume Duration", false );
    }
  }

  uptimes.primary_resource_cap->uptime_instance.change_mode( false );
  uptimes.primary_resource_cap->uptime_sum.change_mode( false );
}

// death_knight_t::init_special_effects =====================================
void death_knight_t::init_special_effects()
{
  player_t::init_special_effects();

  if ( talent.runic_attenuation.ok() )
  {
    auto runic_attenuation      = new special_effect_t( this );
    runic_attenuation->name_str = "runic_attenuation";
    runic_attenuation->spell_id = talent.runic_attenuation->id();
    runic_attenuation->disable_action();
    special_effects.push_back( runic_attenuation );

    runic_attenuation_proc( *runic_attenuation );
  }

  if ( talent.blood.bloodworms.ok() )
  {
    auto bloodworms            = new special_effect_t( this );
    bloodworms->name_str       = "bloodworms";
    bloodworms->spell_id       = talent.blood.bloodworms->id();
    bloodworms->execute_action = pet_summon.bloodworm;
    special_effects.push_back( bloodworms );

    new death_knight_proc_callback_t( *bloodworms );
  }
}

// death_knight_t::init_finished ============================================

void death_knight_t::init_finished()
{
  parse_player_effects();

  if ( main_hand_weapon.type != WEAPON_NONE && mh_runeforge == RUNEFORGE_NONE )
    sim->error( TRIVIAL, "Player {} has no Main-Hand Runeforge enchanted.", name() );

  if ( off_hand_weapon.type != WEAPON_NONE && oh_runeforge == RUNEFORGE_NONE )
    sim->error( TRIVIAL, "Player {} has no Off-Hand Runeforge enchanted.", name() );

  if ( specialization() == DEATH_KNIGHT_UNHOLY && mh_runeforge != RUNEFORGE_APOCALYPSE )
    sim->error( TRIVIAL, "Player {} has a Main-Hand Runeforge that is not Apocalypse enchanted.", name() );

  if ( specialization() == DEATH_KNIGHT_UNHOLY )
    magus_active = 0;

  active_riders = 0;

  player_t::init_finished();
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
      case FIGHT_STYLE_DUNGEON_ROUTE:
        return true;
      case FIGHT_STYLE_DUNGEON_SLICE:
        if ( talent.sanlayn.vampiric_strike.ok() )
          return false;
        return true;
      default:
        return false;
    }
  }
  return false;
}

// death_kight_t::validate_actor ======================================
bool death_knight_t::validate_actor()
{
  if ( talent.frost.frostbane.ok() )
    sim->error( error_level_e::SEVERE, "The precise proc chance of Frostbane is unknown. Results will be incorrect." );

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

  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    throw sc_invalid_player_argument(
        fmt::format( "Player {} has no weapon equipped in the Main-Hand slot.", name() ) );
    return false;
  }

  if ( main_hand_weapon.group() == WEAPON_2H && off_hand_weapon.type != WEAPON_NONE )
  {
    throw sc_invalid_player_argument( fmt::format( "Player {} has an Off-Hand weapon equipped with a 2h.", name() ) );
    return false;
  }

  return true;
}

// death_knight_t::activate =================================================

void death_knight_t::activate()
{
  player_t::activate();

  // Reserve space in the vector based on the maximum number of pets that can be active at once.
  switch ( specialization() )
  {
    case DEATH_KNIGHT_BLOOD:
      dk_active_pets.reserve( 9 );
      // Add some extra space for cantrip item pets.
      active_pets.reserve( 12 );
      break;
    case DEATH_KNIGHT_FROST:
      dk_active_pets.reserve( 6 );
      // Add some extra space for cantrip item pets.
      active_pets.reserve( 10 );
      break;
    case DEATH_KNIGHT_UNHOLY:
      dk_active_pets.reserve( 32 );
      // Add some extra space for cantrip item pets.
      active_pets.reserve( 36 );
      active_lesser_ghouls.reserve( 24 );
      break;
    default:
      break;
  }

  register_on_combat_state_callback( [ this ]( player_t*, bool c ) {
    if ( !c && !sim->event_mgr.canceled )
    {
      runic_power_decay = make_event( *sim, 20_s, [ this ]() {
        runic_power_decay = nullptr;
        make_event<runic_power_decay_event_t>( *sim, this );
      } );

      // This should probably be core to ground_aoe_event_t, canceling the event when leaving combat
      if ( !active_dnds.empty() )
        for ( auto& dnd : active_dnds )
          dnd->expired = true;
    }
    else
    {
      event_t::cancel( runic_power_decay );
    }
  } );

  if ( talent.deathbringer.reapers_mark.ok() )
    register_on_kill_callback( [ this ]( player_t* t ) { trigger_reapers_mark_death( t ); } );

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

  if ( talent.unholy.outbreak.ok() )
    register_on_kill_callback( [ this ]( player_t* t ) {
      if ( get_target_data( t )->dot.dread_plague->is_ticking() )
        trigger_dread_plague_death( t );
    } );
}

// death_knight_t::reset ====================================================

void death_knight_t::reset()
{
  parse_player_effects_t::reset();

  _runes.reset();
  runic_power_decay = nullptr;
  active_riders     = 0;
  magus_active      = 0;
  if ( lesser_ghouls_summoned > 0 && options.extra_unholy_reporting )
    sample_data.lesser_ghouls_summoned->add( lesser_ghouls_summoned );
  lesser_ghouls_summoned = 0;
  dk_active_pets.clear();
  active_lesser_ghouls.clear();
  active_dnds.clear();
}

// death_knight_t::assess_damage ============================================

void death_knight_t::assess_damage( school_e school, result_amount_type type, action_state_t* s )
{
  parse_player_effects_t::assess_damage( school, type, s );

  // As of Mar 19 2026 bloody reflection only reflects if blood shield isn't consumed by the hit.
  // Consumption of the absorb will happen in the parent call to assess_damage, so if it is still up
  // at this point, then we can trigger the reflect
  if ( talent.blood.bloody_reflection.ok() && cooldown.bloody_reflection_icd->up() &&
        buffs.blood_shield->up() && s->result_type == result_amount_type::DMG_DIRECT )
  {
    background_actions.bloody_reflection->execute_on_target( target );
    cooldown.bloody_reflection_icd->start();
  }

  if ( specialization() == DEATH_KNIGHT_BLOOD && s->result == RESULT_PARRY )
  {
    if ( talent.blood.bloodied_blade->ok() )
    {
      if ( buffs.bloodied_blade_stacks->at_max_stacks() )
      {
        buffs.bloodied_blade_stacks->expire();
        buffs.bloodied_blade_final->trigger();
        background_actions.heart_strike_bloodied_blade->execute_on_target( target );
      }
      else if ( !buffs.bloodied_blade_final->check() )  // Can not proc while the final 10% str boost is up
        buffs.bloodied_blade_stacks->trigger();
    }

    if ( talent.blood.dance_of_midnight_1.ok() && buffs.dancing_rune_weapon->up() )
      buffs.dance_of_midnight_1->trigger();
  }
}

// death_knight_t::assess_damage_imminent ===================================

void death_knight_t::bone_shield_handler( const action_state_t* state ) const
{
  if ( ( ( specialization() == DEATH_KNIGHT_BLOOD && !buffs.bone_shield->check() ) || !cooldown.bone_shield_icd->up() ||
         state->action->special ) )
  {
    return;
  }

  sim->print_log( "{} took a successful auto attack and lost a bone shield charge", name() );
  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    buffs.bone_shield->decrement();
  }
  cooldown.bone_shield_icd->start();
}

void death_knight_t::assess_damage_imminent( school_e, result_amount_type, action_state_t* s )
{
  bone_shield_handler( s );
}

// death_knight_t::do_damage ================================================

void death_knight_t::do_damage( action_state_t* state )
{
  player_t::do_damage( state );

  if ( talent.blood_draw.ok() && specialization() == DEATH_KNIGHT_BLOOD && background_actions.blood_draw->ready() )
  {
    background_actions.blood_draw->execute();
  }
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
        // if ( buffs.bloodied_blade_final->check() )
        //   a += dbc->race_base( race ).strength +
        //        dbc->attribute_base( type, level() ).strength * buffs.bloodied_blade_final->check_value();
        break;
      case DEATH_KNIGHT_UNHOLY:
        break;
      case DEATH_KNIGHT_FROST:
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
      if ( specialization() == DEATH_KNIGHT_UNHOLY )
      {
        player_t::invalidate_cache( CACHE_PET_DAMAGE_MULTIPLIER );
        player_t::invalidate_cache( CACHE_GUARDIAN_DAMAGE_MULTIPLIER );
      }
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

void death_knight_t::trigger_movement( double distance, movement_direction_type t )
{
  player_t::trigger_movement( distance, t );
  // Expire all active dnds if moving more than 10 yards
  if ( !active_dnds.empty() && distance > 10 )
  {
    for ( auto& dnd : active_dnds )
      dnd->expired = true;
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
    rps *= 1.0 + spell.runic_corruption->effectN( 1 ).percent();

  if ( talent.frost.runic_command->ok() )
    rps *= 1.0 + talent.frost.runic_command->effectN( 2 ).percent();

  return rps;
}

inline double death_knight_t::rune_regen_coefficient() const
{
  auto coeff = cache.attack_haste();

  // Runic corruption doubles rune regeneration speed
  if ( buffs.runic_corruption->check() )
    coeff /= 1.0 + spell.runic_corruption->effectN( 1 ).percent();

  if ( talent.frost.runic_command->ok() )
    coeff /= 1.0 + talent.frost.runic_command->effectN( 2 ).percent();

  return coeff;
}

// death_knight_t::arise ====================================================
void death_knight_t::arise()
{
  runic_power_decay            = nullptr;
  active_riders                = 0;
  magus_active                 = 0;
  dk_active_pets.clear();
  active_lesser_ghouls.clear();
  active_dnds.clear();

  player_t::arise();
  start_inexorable_assault();

  if ( talent.rider.a_feast_of_souls.ok() )
    start_a_feast_of_souls();

  // Exclude Blood from recklessness checks
  if ( specialization() != DEATH_KNIGHT_BLOOD )
  {
    std::array<stat_e, 4> offensive_stats = { STAT_CRIT_RATING, STAT_HASTE_RATING, STAT_MASTERY_RATING,
                                              STAT_VERSATILITY_RATING };
    if ( ( util::str_compare_ci( potion_str, "potion_of_recklessness" ) ||
           util::str_compare_ci( potion_str, "potion_of_recklessness_2" ) ) &&
         util::highest_stat( this, offensive_stats ) != STAT_MASTERY_RATING )
      sim->error( MODERATE,
                  "Player {} has selected Potion of Recklessness but does not have Mastery as their highest offensive "
                  "stat. Results may be inaccurate.",
                  name() );
  }
}

void death_knight_t::adjust_dynamic_cooldowns()
{
  player_t::adjust_dynamic_cooldowns();

  _runes.update_coefficient();
}

void death_knight_t::apply_action_effects( action_t* a, bool pet )
{
  auto action = dynamic_cast<parse_action_base_t*>( a );
  assert( action );

  // Shared
  action->parse_effects( buffs.blood_draw );

  switch ( specialization() )
    {
    case DEATH_KNIGHT_BLOOD:
      // Don't auto parse coag, since there is some snapshot behavior when the DRW dies
      if ( !pet )
        action->parse_effects( buffs.coagulopathy );
      action->parse_effects( buffs.blood_shield );
      action->parse_effects( buffs.boiling_point );
      action->parse_effects( buffs.dance_of_midnight_1 );
      action->parse_effects( buffs.dance_of_midnight_2 );
      action->parse_effects( buffs.consumption );
      action->parse_effects( buffs.crimson_scourge );
      action->parse_effects( buffs.sanguine_ground );
      action->parse_effects( buffs.sanguinary_burst );
      action->parse_effects( buffs.hemostasis );
      action->parse_effects( buffs.ossuary );
      break;
    case DEATH_KNIGHT_FROST:
      action->parse_effects( buffs.rime );
      action->parse_effects( buffs.gathering_storm );
      action->parse_effects( buffs.killing_machine );
      action->parse_effects( mastery.frozen_heart );
      action->parse_effects( buffs.icy_onslaught );
      action->parse_effects( buffs.remorseless_winter );
      action->parse_effects( buffs.frozen_dominion_remorseless_winter );
      action->parse_effects( buffs.empower_rune_weapon, talent.frost.obliteration->effectN( 1 ).trigger() );
      if ( talent.frost.smothering_offense.ok() )
        action->parse_effects( buffs.icy_talons );
      action->parse_effects( buffs.empowered_strikes );
      action->parse_effects( buffs.frost_mid1_4pc_buff );
      action->parse_effects( buffs.freezing_tempest );
      break;
    case DEATH_KNIGHT_UNHOLY:
      action->parse_effects( buffs.sudden_doom );
      action->parse_effects( buffs.commander_of_the_dead );
      // Dont parse effect 6 due to the way this effect works.
      action->parse_effects( mastery.dreadblade, effect_mask_t( true ).disable( 6 ) );
      // Sets
      action->parse_effects( buffs.blighted, CONSUME_BUFF );
      action->parse_effects( buffs.forbidden_ritual );
      action->parse_effects( buffs.festering_scythe_tt );
      break;
    default:
      break;
  }

  // Might break if Blizzard ever adds other types of subtrees.
  for ( auto& tree : player_sub_trees )
  {
    hero_tree_e hero_tree = static_cast<hero_tree_e>( tree );
    switch ( hero_tree )
    {
      case HERO_DEATHBRINGER:
        action->parse_effects( buffs.dark_talons_shadowfrost );
        action->parse_effects( buffs.bind_in_darkness );
        action->parse_effects( buffs.exterminate );
        break;
      case HERO_RIDER_OF_THE_APOCALYPSE:
        action->parse_effects( buffs.mograines_might );
        action->parse_effects( buffs.a_feast_of_souls );
        break;
      case HERO_SANLAYN:
        action->parse_effects( buffs.essence_of_the_blood_queen, [ & ]( double v ) {
          if ( buffs.gift_of_the_sanlayn->check() )
            v *= 1.0 + buffs.gift_of_the_sanlayn->check_value();
          return v;
        } );
        break;
      default:
        break;
    }
  }
}

void death_knight_t::apply_target_action_effects( action_t* a, bool pet )
{
  auto action = dynamic_cast<parse_action_base_t*>( a );
  assert( action );

  /* NOTE NOTE NOTE NOTE NOTE
  As of 2024 Aug 18th, while testing for TWW we observed that if the pet applies the debuff, like DRW does for blood
  plague they are considered the caster, and as such, they get the benefit of the casters amps (aura 271).  If the
  player applies the debuff the pet does not gain the benefit of the caster debuff, but does gain the benefit for
  pet/guardian auras (aura 380/381) if they exist.

  Auras 380 and 381 get applied in parse_player_effects of the DK.

  Below we should only have debuffs that are cast by pets and guardians, that apply aura 271.
  */
  if ( pet )
  {
    action->parse_target_effects( pets::d_fn( &pets::death_knight_pet_td_t::dots_t::blood_plague ),
                                  spell.blood_plague );
  }
  else
  {
    // Shared
    action->parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::apocalypse_war ),
                                  runeforge_spell.apocalypse_war_debuff );
    action->parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::razorice ), runeforge_spell.razorice_debuff );
    action->parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::brittle ), spell.brittle_debuff );

    switch ( specialization() )
    {
      case DEATH_KNIGHT_BLOOD:
        action->parse_target_effects( d_fn( &death_knight_td_t::dots_t::blood_plague ), spell.blood_plague );
        break;
      case DEATH_KNIGHT_FROST:
        action->parse_target_effects( d_fn( &death_knight_td_t::dots_t::frost_fever ), spell.frost_fever );
        action->parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::everfrost ),
                                      talent.frost.everfrost->effectN( 1 ).trigger(), talent.frost.everfrost );
        break;
      case DEATH_KNIGHT_UNHOLY:
        action->parse_target_effects( d_fn( &death_knight_td_t::dots_t::virulent_plague ), spell.virulent_plague );
        action->parse_target_effects( d_fn( &death_knight_td_t::dots_t::dread_plague ), spell.dread_plague );
        action->parse_target_effects( d_fn( &death_knight_td_t::dots_t::infected_claws ), spell.infected_claws_dot );
        action->parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::soul_reaper ), spell.soul_reaper_debuff );
        break;
      default:
        break;
    }

    // Might break if Blizzard ever adds other types of subtrees.
    for ( auto& tree : player_sub_trees )
    {
      hero_tree_e hero_tree = static_cast<hero_tree_e>( tree );
      switch ( hero_tree )
      {
        case HERO_DEATHBRINGER:
          break;
        case HERO_RIDER_OF_THE_APOCALYPSE:
          break;
        case HERO_SANLAYN:
          action->parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::incite_terror ),
                                        spell.incite_terror_debuff );
          break;
        default:
          break;
      }
    }
  }
}

void death_knight_t::parse_player_effects()
{
  // Shared
  parse_effects( buffs.blood_draw );
  parse_effects( buffs.icy_talons, talent.icy_talons );
  parse_effects( buffs.rune_mastery );
  parse_effects( buffs.antimagic_shell );
  parse_effects( buffs.icebound_fortitude );

  if ( has_runeforge( RUNEFORGE_SPELLWARDING ) )
  {
    double val = ( ( mh_runeforge == RUNEFORGE_SPELLWARDING ) + ( oh_runeforge == RUNEFORGE_SPELLWARDING ) ) *
                 runeforge_spell.spellwarding_driver->effectN( 2 ).percent();
    parse_effects( runeforge_spell.spellwarding_driver, val, PARSE_PASSIVE );
  }

  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::brittle ), spell.brittle_debuff );
  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::apocalypse_war ), runeforge_spell.apocalypse_war_debuff );
  parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::apocalypse_famine ), runeforge_spell.apocalypse_famine_debuff );

  switch ( specialization() )
  {
    case DEATH_KNIGHT_BLOOD:
      parse_effects( mastery.blood_shield );
      parse_effects( buffs.blood_mist );
      parse_effects( buffs.voracious );
      parse_effects( buffs.dancing_rune_weapon );
      parse_effects( buffs.vampiric_blood, effect_mask_t( true ).disable( 2, 4 ) );
      parse_effects( buffs.sanguine_ground );
      parse_effects( buffs.bone_shield, IGNORE_STACKS );
      parse_effects( buffs.perseverance_of_the_ebon_blade );
      parse_effects( buffs.dance_of_midnight_2 );
      break;
    case DEATH_KNIGHT_FROST:
      parse_effects( buffs.bonegrinder_frost );
      parse_effects( buffs.bonegrinder_crit );
      parse_effects( buffs.frozen_dominion );
      parse_effects( buffs.enduring_strength );
      parse_effects( buffs.swift_and_painful );
      parse_effects( buffs.freezing_tempest );
      break;
    case DEATH_KNIGHT_UNHOLY:
      parse_effects( mastery.dreadblade );
      parse_effects( buffs.ghoulish_frenzy );
      parse_effects( buffs.lesser_ghoul_counter );

      parse_target_effects( d_fn( &death_knight_td_t::dots_t::virulent_plague ), spell.virulent_plague );
      parse_target_effects( d_fn( &death_knight_td_t::dots_t::dread_plague ), spell.dread_plague );
      parse_target_effects( d_fn( &death_knight_td_t::dots_t::infected_claws ), spell.infected_claws_dot );
      parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::soul_reaper ), spell.soul_reaper_debuff );
      break;
    default:
      break;
  }

  auto mograines_might_mask = effect_mask_t( true );
  if ( specialization() == DEATH_KNIGHT_UNHOLY )
    mograines_might_mask.disable( 4 );

  // Might break if Blizzard ever adds other types of subtrees.
  for ( auto& tree : player_sub_trees )
  {
    hero_tree_e hero_tree = static_cast<hero_tree_e>( tree );
    switch ( hero_tree )
    {
      case HERO_DEATHBRINGER:
        if ( specialization() == DEATH_KNIGHT_FROST )
          parse_effects( buffs.empowered_soul, effect_mask_t( false ).enable( 1 ) );
        else if ( specialization() == DEATH_KNIGHT_BLOOD )
          parse_effects( buffs.empowered_soul, effect_mask_t( false ).enable( 2 ) );
        break;
      case HERO_RIDER_OF_THE_APOCALYPSE:
        parse_effects( buffs.mograines_might, mograines_might_mask );
        parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::chains_of_ice_trollbane_damage ),
                              pet_spell.trollbanes_chains_of_ice_debuff );
        break;
      case HERO_SANLAYN:
        parse_effects( buffs.bloodsoaked_ground );
        parse_effects( buffs.essence_of_the_blood_queen, effect_mask_t( true ).disable( 3 ), [ & ]( double v ) {
          v *= 0.1;  // Divides by 10 in spell data
          if ( buffs.gift_of_the_sanlayn->check() )
            v *= 1.0 + buffs.gift_of_the_sanlayn->check_value();
          return v;
        } );
        parse_target_effects( d_fn( &death_knight_td_t::debuffs_t::incite_terror ), spell.incite_terror_debuff );
        break;
      default:
        break;
    }
  }
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
    // Basic Table
    os << "<div class=\"player-section custom_section\">\n";
    os << "<h3 class=\"toggle\">Rune waste details</h3>\n"
       << "<div class=\"toggle-content hide\">\n";

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

    // Bar Charts
    auto& d         = p._runes.rune_waste;
    int num_buckets = std::min( 24, static_cast<int>( 2 * ( d.max() - d.min() ) ) + 1 );
    d.create_histogram( num_buckets );

    highchart::histogram_chart_t chart( highchart::build_id( p, "time_spent_wasting" ), *p.sim );
    if ( chart::generate_distribution( chart, &p, d.distribution, "Seconds Spent Wasting Per Rune", d.mean(), d.min(),
                                       d.max() ) )
    {
      chart.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart.set( "chart.width", std::to_string( 80 + num_buckets * 20 ) );
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }

    auto& r       = p._runes.cumulative_waste;
    int n_buckets = std::min( 24, static_cast<int>( 2 * ( r.max() - r.min() ) ) + 1 );
    r.create_histogram( n_buckets );

    highchart::histogram_chart_t total_chart( highchart::build_id( p, "total_time_spent_wasting" ), *p.sim );
    if ( chart::generate_distribution( total_chart, &p, r.distribution, "Total Seconds Spent Wasting Runes", r.mean(),
                                       r.min(), r.max() ) )
    {
      total_chart.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      total_chart.set( "chart.width", std::to_string( 80 + n_buckets * 20 ) );
      os << total_chart.to_target_div();
      p.sim->add_chart_data( total_chart );
    }

    os << "</div>\n";
    os << "<div class=\"clear\"></div>\n";
    os << "</div>\n";
  }

  void html_rp_waste( report::sc_html_stream& os ) const
  {
    auto& d = p.get_uptime( "Runic Power Cap" )->uptime_instance;
    if ( d.distribution.size() == 0 )
      return;

    os << "<div class=\"player-section custom_section\">\n"
          "<h3 class=\"toggle\">Runic Power Waste Details</h3>\n"
          "<div class=\"toggle-content hide\">\n";

    int num_buckets = std::min( 24, static_cast<int>( 2 * ( d.max() - d.min() ) ) + 1 );
    d.create_histogram( num_buckets );

    highchart::histogram_chart_t chart( highchart::build_id( p, "rp_wasted_time" ), *p.sim );
    if ( chart::generate_distribution( chart, &p, d.distribution, "Total Seconds Spent Wasting Runic Power", d.mean(),
                                       d.min(), d.max() ) )
    {
      chart.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart.set( "chart.width", std::to_string( 80 + num_buckets * 20 ) );
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }

    os << "</div>\n"
          "</div>\n";
  }

  void lesser_ghoul_duration_chart( report::sc_html_stream& os )
  {
    if ( !p.talent.unholy.commander_of_the_dead.ok() )
      return;

    auto& d = *p.sample_data.lesser_ghoul_duration;
    if ( d.count() == 0 )
      return;

    int num_buckets = std::min( 30, static_cast<int>( 2 * ( d.max() - d.min() ) ) + 1 );
    d.create_histogram( num_buckets );

    highchart::histogram_chart_t chart( highchart::build_id( p, "lesser_ghoul_duration" ), *p.sim );
    if ( chart::generate_distribution( chart, &p, d.distribution, "Lesser Ghoul Duration (Seconds)", d.mean(), d.min(),
                                       d.max() ) )
    {
      chart.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart.set( "chart.width", std::to_string( 80 + num_buckets * 20 ) );
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }

    os << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th colspan=\"5\">Statistics</th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th>Minimum</th>\n"
       << "<th>5<sup>th</sup> percentile</th>\n"
       << "<th>Mean / Median</th>\n"
       << "<th>75<sup>th</sup> percentile</th>\n"
       << "<th>95<sup>th</sup> percentile</th>\n"
       << "<th>Maximum</th>\n"
       << "</tr>\n";

    os << "<tr>\n";
    os.printf( "<td class=\"right\">%.3f</td>", d.min() );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .05 ) );
    os.printf( "<td class=\"right\">%.3f / %.3f</td>", d.mean(), d.percentile( .5 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .75 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .95 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.max() );
    os << "</tr>\n";
    os << "</table>\n";
  }

    void lesser_ghoul_summoned_chart( report::sc_html_stream& os )
  {
    auto& d = *p.sample_data.lesser_ghouls_summoned;
    if ( d.count() == 0 )
      return;

    int num_buckets = std::min( 30, static_cast<int>( 2 * ( d.max() - d.min() ) ) + 1 );
    d.create_histogram( num_buckets );

    highchart::histogram_chart_t chart( highchart::build_id( p, "lesser_ghouls_summoned" ), *p.sim );
    if ( chart::generate_distribution( chart, &p, d.distribution, "Lesser Ghouls Summoned", d.mean(), d.min(),
                                       d.max() ) )
    {
      chart.set( "chart.width", std::to_string( 80 + num_buckets * 20 ) );
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }

    os << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th colspan=\"5\">Statistics</th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th>Minimum</th>\n"
       << "<th>5<sup>th</sup> percentile</th>\n"
       << "<th>Mean / Median</th>\n"
       << "<th>75<sup>th</sup> percentile</th>\n"
       << "<th>95<sup>th</sup> percentile</th>\n"
       << "<th>Maximum</th>\n"
       << "</tr>\n";

    os << "<tr>\n";
    os.printf( "<td class=\"right\">%.3f</td>", d.min() );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .05 ) );
    os.printf( "<td class=\"right\">%.3f / %.3f</td>", d.mean(), d.percentile( .5 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .75 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .95 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.max() );
    os << "</tr>\n";
    os << "</table>\n";
  }

  void lesser_ghoul_active_chart( report::sc_html_stream& os )
  {
    auto& d = *p.sample_data.lesser_ghouls_active;
    if ( d.count() == 0 )
      return;

    int num_buckets = std::min( 30, static_cast<int>( d.max() ) );
    d.create_histogram( num_buckets );

    highchart::histogram_chart_t chart( highchart::build_id( p, "lesser_ghoul_active" ), *p.sim );
    if ( chart::generate_distribution( chart, &p, d.distribution, "Lesser Ghouls Active", d.mean(), d.min(), d.max() ) )
    {
      chart.set( "chart.width", std::to_string( 80 + num_buckets * 20 ) );
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }

    os << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th colspan=\"5\">Statistics</th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th>Minimum</th>\n"
       << "<th>5<sup>th</sup> percentile</th>\n"
       << "<th>Mean / Median</th>\n"
       << "<th>75<sup>th</sup> percentile</th>\n"
       << "<th>95<sup>th</sup> percentile</th>\n"
       << "<th>Maximum</th>\n"
       << "</tr>\n";

    os << "<tr>\n";
    os.printf( "<td class=\"right\">%.3f</td>", d.min() );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .05 ) );
    os.printf( "<td class=\"right\">%.3f / %.3f</td>", d.mean(), d.percentile( .5 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .75 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .95 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.max() );
    os << "</tr>\n";
    os << "</table>\n";
  }

  void lesser_ghoul_charts( report::sc_html_stream& os )
  {
    if ( p.specialization() != DEATH_KNIGHT_UNHOLY )
      return;

    highchart::pie_chart_t lesser_ghoul_sources( highchart::build_id( p, "lesser_ghoul_sources" ), *p.sim );
    lesser_ghoul_sources.set_title( "Lesser Ghoul Summon Sources" );
    lesser_ghoul_sources.set( "plotOptions.pie.dataLabels.format", "{point.name}: {point.percentage:.1f}%" );

    std::array<proc_t*, 3> sources = { p.procs.lesser_ghoul_army, p.procs.lesser_ghoul_db, p.procs.lesser_ghoul_fs };

    double sum = 0.0;

    for ( auto source : sources )
      sum += source->count.mean();

    range::sort( sources, [ & ]( proc_t* a, proc_t* b ) {
      if ( a->count.mean() == b->count.mean() )
        return a->name() < b->name();
      return a->count.mean() > b->count.mean();
    } );

    range::for_each( sources, [ this, &lesser_ghoul_sources, sum ]( proc_t* source ) {
      if ( source->count.mean() == 0.0 )
        return;

      color::rgb color = color::school_color( SCHOOL_SHADOWSTORM );

      js::sc_js_t e;
      e.set( "color", color.str() );
      e.set( "y", util::round( source->count.mean() / sum * 100, p.sim->report_precision ) );
      e.set( "name", report_decorators::decorate_html_string( source->name(), color ) );

      lesser_ghoul_sources.add( "series.0.data", e );
    } );

    os << "<div class=\"player-section custom_section\">\n"
          "<h3 class=\"toggle\">Lesser Ghouls</h3>\n"
          "<div class=\"toggle-content hide\">\n";

    os << lesser_ghoul_sources.to_target_div();
    p.sim->add_chart_data( lesser_ghoul_sources );
    lesser_ghoul_summoned_chart( os );
    lesser_ghoul_duration_chart( os );
    lesser_ghoul_active_chart( os );

    os << "</div>\n"
          "</div>\n";
  }

  void magus_active_chart( report::sc_html_stream& os )
  {
    if ( p.specialization() != DEATH_KNIGHT_UNHOLY )
      return;

    auto& d = *p.sample_data.magus_active;
    if ( d.count() == 0 )
      return;

    os << "<div class=\"player-section custom_section\">\n"
          "<h3 class=\"toggle\">Magus of the Dead</h3>\n"
          "<div class=\"toggle-content hide\">\n";

    int num_buckets = std::min( 30, static_cast<int>( d.max() ) );
    d.create_histogram( num_buckets );

    highchart::histogram_chart_t chart( highchart::build_id( p, "magus_active" ), *p.sim );
    if ( chart::generate_distribution( chart, &p, d.distribution, "Magus' Active", d.mean(), d.min(), d.max() ) )
    {
      chart.set( "chart.width", std::to_string( 80 + num_buckets * 20 ) );
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }

    os << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th colspan=\"5\">Statistics</th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th>Minimum</th>\n"
       << "<th>5<sup>th</sup> percentile</th>\n"
       << "<th>Mean / Median</th>\n"
       << "<th>75<sup>th</sup> percentile</th>\n"
       << "<th>95<sup>th</sup> percentile</th>\n"
       << "<th>Maximum</th>\n"
       << "</tr>\n";

    os << "<tr>\n";
    os.printf( "<td class=\"right\">%.3f</td>", d.min() );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .05 ) );
    os.printf( "<td class=\"right\">%.3f / %.3f</td>", d.mean(), d.percentile( .5 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .75 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .95 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.max() );
    os << "</tr>\n";
    os << "</table>\n";

    os << "</div>\n"
          "</div>\n";
  }

  void dread_plague_dur_chart( report::sc_html_stream& os )
  {
    auto& d = *p.sample_data.blightfall_dp_dur;
    if ( d.count() == 0 )
      return;

    int num_buckets = std::min( 30, static_cast<int>( d.max() ) );
    d.create_histogram( num_buckets );

    highchart::histogram_chart_t chart( highchart::build_id( p, "dp_dur" ), *p.sim );
    if ( chart::generate_distribution( chart, &p, d.distribution, "Dread Plague Consumed Duration", d.mean(), d.min(),
                                       d.max() ) )
    {
      chart.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart.set( "chart.width", std::to_string( 80 + num_buckets * 20 ) );
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }

    os << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th colspan=\"5\">Statistics</th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th>Minimum</th>\n"
       << "<th>5<sup>th</sup> percentile</th>\n"
       << "<th>Mean / Median</th>\n"
       << "<th>75<sup>th</sup> percentile</th>\n"
       << "<th>95<sup>th</sup> percentile</th>\n"
       << "<th>Maximum</th>\n"
       << "</tr>\n";

    os << "<tr>\n";
    os.printf( "<td class=\"right\">%.3f</td>", d.min() );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .05 ) );
    os.printf( "<td class=\"right\">%.3f / %.3f</td>", d.mean(), d.percentile( .5 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .75 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .95 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.max() );
    os << "</tr>\n";
    os << "</table>\n";

  }

  void virulent_plague_dur_chart( report::sc_html_stream& os )
  {
    auto& d = *p.sample_data.blightfall_vp_dur;
    if ( d.count() == 0 )
      return;

    int num_buckets = std::min( 30, static_cast<int>( d.max() ) );
    d.create_histogram( num_buckets );

    highchart::histogram_chart_t chart( highchart::build_id( p, "vp_dur" ), *p.sim );
    if ( chart::generate_distribution( chart, &p, d.distribution, "Virulent Plague Consumed Duration", d.mean(), d.min(),
                                       d.max() ) )
    {
      chart.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart.set( "chart.width", std::to_string( 80 + num_buckets * 20 ) );
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }

    os << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th colspan=\"5\">Statistics</th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th>Minimum</th>\n"
       << "<th>5<sup>th</sup> percentile</th>\n"
       << "<th>Mean / Median</th>\n"
       << "<th>75<sup>th</sup> percentile</th>\n"
       << "<th>95<sup>th</sup> percentile</th>\n"
       << "<th>Maximum</th>\n"
       << "</tr>\n";

    os << "<tr>\n";
    os.printf( "<td class=\"right\">%.3f</td>", d.min() );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .05 ) );
    os.printf( "<td class=\"right\">%.3f / %.3f</td>", d.mean(), d.percentile( .5 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .75 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.percentile( .95 ) );
    os.printf( "<td class=\"right\">%.3f</td>", d.max() );
    os << "</tr>\n";
    os << "</table>\n";
  }

  void blightfall_charts( report::sc_html_stream& os )
  {
    if ( p.specialization() != DEATH_KNIGHT_UNHOLY )
      return;

    if ( !p.talent.unholy.blightfall.ok() )
      return;

    os << "<div class=\"player-section custom_section\">\n"
          "<h3 class=\"toggle\">Blightfall</h3>\n"
          "<div class=\"toggle-content hide\">\n";

    dread_plague_dur_chart( os );
    virulent_plague_dur_chart( os );

    os << "</div>\n"
          "</div>\n";
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    if ( p.sim->report_details == 1 )
    {
      if ( p._runes.cumulative_waste.percentile( .5 ) > 0 )
        html_rune_waste( os );
      if ( p.options.extra_unholy_reporting )
      {
        lesser_ghoul_charts( os );
        magus_active_chart( os );
        blightfall_charts( os );
      }
      html_rp_waste( os );
    }
  }

private:
  death_knight_t& p;
};

namespace live_death_knight
{
// #include "class_modules/sc_death_knight_live.inc"
}

// DEATH_KNIGHT MODULE INTERFACE ============================================

struct death_knight_module_t : public module_t
{
  death_knight_module_t() : module_t( DEATH_KNIGHT )
  {
  }

  player_t* create_player( sim_t* sim, std::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p              = new death_knight_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new death_knight_report_t( *p ) );
    return p;
  }

  void static_init() const override
  {
    runeforge::init_runeforges();
    // If the module is split between live and ptr, it needs to call the live init instead.
    // Make sure the wowv check is correct.
    /*
     * if( sim->dbc->wowv() >= wowv_t( 12, 0, 0 ) )
     *   runeforge::init_runeforges();
     * else
     *   live_death_knight::runeforge::init_runeforges();
     */
  }

  void register_hotfixes() const override
  {
    // hotfix::register_effect( "Death Knight", "2026-5-1", "Shadow Bolt Nerfed 15%", 803165,
    //                          hotfix::HOTFIX_FLAG_LIVE )
    //     .field( "ap_coefficient" )
    //     .operation( hotfix::HOTFIX_SET )
    //     .modifier( 0.6587568 )
    //     .verification_value( 0.775008 );

    // hotfix::register_effect( "Death Knight", "2026-5-1", "Graveyard (main) nerfed 15%", 1015149,
    //                          hotfix::HOTFIX_FLAG_LIVE )
    //     .field( "ap_coefficient" )
    //     .operation( hotfix::HOTFIX_SET )
    //     .modifier( 0.9546775 )
    //     .verification_value( 1.12315 );

    // hotfix::register_effect( "Death Knight", "2026-5-1", "Graveyard (AoE) nerfed 15%", 1274362,
    //                          hotfix::HOTFIX_FLAG_LIVE )
    //     .field( "ap_coefficient" )
    //     .operation( hotfix::HOTFIX_SET )
    //     .modifier( 0.3818761 )
    //     .verification_value( 0.449266 );

    // hotfix::register_effect( "Death Knight", "2026-5-1", "Whitemane Epidemic (main) nerfed 25%", 1233789,
    //                          hotfix::HOTFIX_FLAG_LIVE )
    //     .field( "ap_coefficient" )
    //     .operation( hotfix::HOTFIX_SET )
    //     .modifier( 0.494484 )
    //     .verification_value( 0.659312 );

    // hotfix::register_effect( "Death Knight", "2026-5-1", "Whitemane Epidemic (AoE) nerfed 25%", 1233790,
    //                          hotfix::HOTFIX_FLAG_LIVE )
    //     .field( "ap_coefficient" )
    //     .operation( hotfix::HOTFIX_SET )
    //     .modifier( 0.19779525 )
    //     .verification_value( 0.263727 );

    // hotfix::register_effect( "Death Knight", "2026-5-1", "Trollbanes icy Fury nerfed 25%", 1141463,
    //                          hotfix::HOTFIX_FLAG_LIVE )
    //     .field( "ap_coefficient" )
    //     .operation( hotfix::HOTFIX_SET )
    //     .modifier( 0.78975 )
    //     .verification_value( 1.053 );

    // hotfix::register_effect( "Death Knight", "2026-5-1", "Thrill of Blood DP damage increased to 10%", 1319202,
    //                          hotfix::HOTFIX_FLAG_LIVE )
    //     .field( "base_value" )
    //     .operation( hotfix::HOTFIX_SET )
    //     .modifier( 10 )
    //     .verification_value( 5 );
    // hotfix::register_effect( "Death Knight", "2026-05-11", "Dancing Rune Weapon grants 25% Parry", 68684,
    //                           hotfix::HOTFIX_FLAG_LIVE )
    //       .field( "base_value" )
    //       .operation( hotfix::HOTFIX_SET )
    //       .modifier( 25 )
    //       .verification_value( 20 );
    // hotfix::register_effect( "Death Knight", "2026-05-11", "Blood Fortification increased to 40%", 1000722,
    //                           hotfix::HOTFIX_FLAG_LIVE )
    //       .field( "base_value" )
    //       .operation( hotfix::HOTFIX_SET )
    //       .modifier( 40 )
    //       .verification_value( 35 );
    // hotfix::register_effect( "Death Knight", "2026-05-11", "Improved Death Strike healing increased to 15%", 1000063,
    //                           hotfix::HOTFIX_FLAG_LIVE )
    //       .field( "base_value" )
    //       .operation( hotfix::HOTFIX_SET )
    //       .modifier( 15 )
    //       .verification_value( 5 );
    // hotfix::register_spell( "Death Knight", "2026-05-11", "Dance of Midnight grants DRW for 8 seconds", 1264353,
    //                           hotfix::HOTFIX_FLAG_LIVE )
    //       .field( "duration" )
    //       .operation( hotfix::HOTFIX_SET )
    //       .modifier( 8000 )
    //       .verification_value( 6000 );
  }

  void register_actor_initializers( sim_t* ) const override
  {
  }
  bool valid() const override
  {
    return true;
  }
};

}  // UNNAMED NAMESPACE

const module_t* module_t::death_knight()
{
  static death_knight_module_t m;
  return &m;
}
