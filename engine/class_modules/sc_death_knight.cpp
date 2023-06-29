// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// TODO as of 2020-09-06
// Class:
// Killing Blow based mechanics (free Death Strike, Rune of Unending Thirst)
// Defensives: Anti-Magic Zone (+ group wide effect), Lichborne
// Disable noise from healing/defensive actions when simming a single, dps role, character
// Automate Rune energize in death_knight_action_t::execute() instead of per spell overrides
// utilize stat_pct_buffs instead of overriding player_t methods
// Add get_action() handling to diseases, obliterate/frost strike, active_spells.x
// Standardize debug_cast<T>() over other types of casting where possible
// Look into Death Strike OH handling (p -> dual_wield()?) and see if it can apply to other DW attacks
// Unholy:
// - Predict the first two Festering wounds on FS and use reaction time on the third?
// Blood:
// - Check that VB's absorb increase is correctly implemented
// - Healing from Consumption damage done
// Frost:
// - Revisit Deaths Due, Koltiras to verify 1H vs 2H behavior, especially with DD cleave
// - Test Frost's damage with atypical weapon setups (single 1H/OH, etc.)
//     on abilities with the 2H penalty or combined AP type
// - Figure out what to do with Obliterate/Frost Strike strikes, reporting, etc.
// TOCHECK: accurate spawn/travel delay timers for all dynamic pets
// - DRW, Magus of the dead, Bloodworms have no delay at all
// - Army ghoul, Apoc ghoul, Raise dead ghoul have a different delay (currently estimated at 4.5s with a 0.1 stddev)
//   TODO: even more variable travel time for apoc/army based on distance from boss on spawn?
// - Gargoyle has its own delay too

#include "simulationcraft.hpp"
#include "player/pet_spawner.hpp"
#include "action/action_callback.hpp"
#include "class_modules/apl/apl_death_knight.hpp"

namespace { // UNNAMED NAMESPACE

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

using namespace unique_gear;

struct death_knight_t;
struct runes_t;
struct rune_t;

namespace pets {
  struct death_knight_pet_t;
  struct army_ghoul_pet_t;
  struct bloodworm_pet_t;
  struct dancing_rune_weapon_pet_t;
  struct everlasting_bond_pet_t;
  struct gargoyle_pet_t;
  struct ghoul_pet_t;
  struct magus_pet_t;
}

namespace runeforge {
  void apocalypse( special_effect_t& );
  void fallen_crusader( special_effect_t& );
  void hysteria( special_effect_t& );
  void razorice( special_effect_t& );
  void sanguination( special_effect_t& );
  void spellwarding( special_effect_t& );
  void stoneskin_gargoyle( special_effect_t& );
  void unending_thirst( special_effect_t& ); // Effect only procs on killing blows, NYI
}

enum runeforge_apocalypse { DEATH, FAMINE, PESTILENCE, WAR, MAX };

// ==========================================================================
// Death Knight Runes ( part 1 )
// ==========================================================================

enum rune_state { STATE_DEPLETED, STATE_REGENERATING, STATE_FULL };

const double RUNIC_POWER_DECAY_RATE = 1.0;
const double RUNE_REGEN_BASE = 10;
const double RUNE_REGEN_BASE_SEC = ( 1 / RUNE_REGEN_BASE );

const size_t MAX_RUNES = 6;
const size_t MAX_REGENERATING_RUNES = 3;
const double MAX_START_OF_COMBAT_RP = 20;

template <typename T>
struct dynamic_event_t : public event_t
{
  double m_coefficient;
  T**    m_ptr;

  dynamic_event_t( sim_t* s ) : event_t( *s ), m_coefficient( 1.0 ), m_ptr( nullptr )
  { }

  const char* name() const override
  { return "Dynamic-Event-Base"; }

  static T* create( sim_t* s )
  { return new (*s) T( s ); }

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

    sim().print_debug( "{} adjust time by {}, remains= {}, new_remains={}",
                        name(), by_time.total_seconds(), remains().total_seconds(), new_remains.total_seconds() );

    // Otherwise, just clone this event and schedule it with the new time, bypassing the coefficient
    // adjustment
    create_clone() -> schedule( new_remains, false );

    event_t::cancel( this_ );
    return new_remains;
  }

  // Create a clone of this event
  virtual T* create_clone()
  {
    auto cloned = clone();
    if ( m_ptr )
    {
      cloned -> ptr( *m_ptr );
      *m_ptr = cloned;
    }
    cloned -> coefficient( m_coefficient );

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

    auto ratio = new_coefficient / m_coefficient;
    auto remains = this -> remains();
    auto new_duration = remains * ratio;

    sim().print_debug( "{} coefficient change, remains={} old_coeff={} new_coeff={} ratio={} new_remains={}",
      name(), remains.total_seconds(), m_coefficient, new_coefficient, ratio, new_duration.total_seconds() );

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
      cloned -> coefficient( new_coefficient )
             -> schedule( new_duration, false );

      // Cancel wants ref to a ptr, but we don't really care
      auto this_ = this;
      event_t::cancel( this_ );
      return cast( cloned );
    }
  }

  static T* cast( event_t* event )
  { return debug_cast<T*>( event ); }

  T* cast() const
  { return debug_cast<T*>( this ); }

  T* cast()
  { return debug_cast<T*>( this ); }

  T* ptr( T*& value )
  { m_ptr = &value; return cast(); }

  T* ptr() const
  { return m_ptr ? *m_ptr : nullptr; }

  T* coefficient( double value )
  { m_coefficient = value; return cast(); }

  double coefficient() const
  { return m_coefficient; }

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

  rune_event_t( sim_t* sim ) :
    super( sim ), m_rune( nullptr )
  { }

  const char* name() const override
  { return "Rune-Regen-Event"; }

  rune_event_t* clone() override
  { return create( &sim() ) -> rune( *m_rune ); }

  rune_t* rune() const
  { return m_rune; }

  rune_event_t* rune( rune_t& r )
  { m_rune = &r; return this; }

  void execute_event() override;
};

struct rune_t
{
  runes_t*   runes;     // Back reference to runes_t array so we can adjust rune state
  rune_state state;     // DEPLETED, REGENERATING, FULL
  rune_event_t* event;  // Regen event
  timespan_t regen_start; // Start time of the regeneration
  timespan_t regenerated; // Timestamp when rune regenerated to full

  rune_t() : runes( nullptr ), state( STATE_FULL ), event( nullptr ), regen_start( timespan_t::min() )
  { }

  rune_t( runes_t* r ) : runes( r ), state( STATE_FULL ), event( nullptr ), regen_start( timespan_t::min() )
  { }

  bool is_ready()    const     { return state == STATE_FULL    ; }
  bool is_regenerating() const { return state == STATE_REGENERATING; }
  bool is_depleted() const     { return state == STATE_DEPLETED; }

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
    event = nullptr;
    state = STATE_FULL;
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
  { range::for_each( slot, []( rune_t& r ) { r.update_coefficient(); } ); }

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

    for ( const auto& rune: slot )
    {
      char rune_letter;
      if ( rune.is_ready() ) {
        rune_letter = 'F';
      } else if ( rune.is_depleted() ) {
        rune_letter = 'd';
      } else {
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
        [ s ]( const unsigned& v, const rune_t& r ) { return v + ( r.state == s ); });
  }

  // Return the first rune in a specific state. If no rune in specific state found, return nullptr.
  rune_t* first_rune_in_state( rune_state s )
  {
    auto it = range::find_if( slot, [ s ]( const rune_t& rune ) { return rune.state == s; } );
    if ( it != slot.end() )
    {
      return &(*it);
    }

    return nullptr;
  }

  unsigned runes_regenerating() const
  { return runes_in_state( STATE_REGENERATING ); }

  unsigned runes_depleted() const
  { return runes_in_state( STATE_DEPLETED ); }

  unsigned runes_full() const
  { return runes_in_state( STATE_FULL ); }

  rune_t* first_depleted_rune()
  { return first_rune_in_state( STATE_DEPLETED ); }

  rune_t* first_regenerating_rune()
  { return first_rune_in_state( STATE_REGENERATING ); }

  rune_t* first_full_rune()
  { return first_rune_in_state( STATE_FULL ); }

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

struct death_knight_td_t : public actor_target_data_t {
  struct
  {
    // Blood
    propagate_const<dot_t*> blood_plague;
    // Frost
    propagate_const<dot_t*> frost_fever;
    // Unholy
    propagate_const<dot_t*> soul_reaper;
    propagate_const<dot_t*> virulent_plague;
    propagate_const<dot_t*> unholy_blight;
  } dot;

  struct
  {
    // Runeforges
    propagate_const<buff_t*> apocalypse_death; // Dummy debuff, healing reduction not implemented
    propagate_const<buff_t*> apocalypse_war;
    propagate_const<buff_t*> apocalypse_famine;
    buff_t* razorice;
	
    // General Talents
    propagate_const<buff_t*> brittle;

    // Blood
    propagate_const<buff_t*> mark_of_blood;
    propagate_const<buff_t*> tightening_grasp;
	
    // Frost
    propagate_const<buff_t*> piercing_chill;
    propagate_const<buff_t*> everfrost;
    propagate_const<buff_t*> lingering_chill;
	
    // Unholy
    propagate_const<buff_t*> festering_wound;
    propagate_const<buff_t*> rotten_touch;
    propagate_const<buff_t*> death_rot;

  } debuff;

  death_knight_td_t( player_t* target, death_knight_t* p );
};

using data_t = std::pair<std::string, simple_sample_data_with_min_max_t>;
using simple_data_t = std::pair<std::string, simple_sample_data_t>;

struct death_knight_t : public player_t {
public:
  // Stores the currently active death and decay ground event
  ground_aoe_event_t* active_dnd;

  // Expression warnings
  // for old dot.death_and_decay.x expressions
  bool deprecated_dnd_expression;
  // for runeforge.name expressions that call death knight runeforges instead of shadowlands legendary runeforges
  bool runeforge_expression_warning;

  // Counters
  unsigned int km_proc_attempts; // critical auto attacks since the last KM proc
  unsigned int festering_wounds_target_count; // cached value of the current number of enemies affected by FW
  unsigned int bone_shield_charges_consumed; // Counts how many bone shield charges have been consumed for T29 4pc blood

  stats_t* antimagic_shell;
  stats_t* antimagic_zone;

  // Buffs
  struct buffs_t {
    // Shared
    buff_t* antimagic_shell;
    buff_t* antimagic_zone;
    propagate_const<buff_t*> icebound_fortitude;
    propagate_const<buff_t*> rune_mastery;
    propagate_const<buff_t*> unholy_ground;
    propagate_const<buff_t*> icy_talons;
    propagate_const<buff_t*> empower_rune_weapon;
    propagate_const<buff_t*> abomination_limb;
    propagate_const<buff_t*> lichborne; // NYI

    // Runeforges
    propagate_const<buff_t*> rune_of_hysteria;
    propagate_const<buff_t*> stoneskin_gargoyle;
    propagate_const<buff_t*> unholy_strength; // runeforge of the fallen crusader

    // Blood
    absorb_buff_t* blood_shield;
    buff_t* bone_shield;
    propagate_const<buff_t*> coagulopathy;
    propagate_const<buff_t*> crimson_scourge;
    propagate_const<buff_t*> dancing_rune_weapon;
    propagate_const<buff_t*> heartrend;
    propagate_const<buff_t*> hemostasis;
    propagate_const<buff_t*> ossuary;
    propagate_const<buff_t*> perseverance_of_the_ebon_blade;
    propagate_const<buff_t*> rune_tap;
    propagate_const<buff_t*> sanguine_ground;
    propagate_const<buff_t*> tombstone;
    propagate_const<buff_t*> vampiric_blood;
    buff_t* vigorous_lifeblood_4pc;  // T29 4pc
    propagate_const<buff_t*> vampiric_strength; // T30 4pc str buff
    propagate_const<buff_t*> voracious;

    // Frost
    propagate_const<buff_t*> breath_of_sindragosa;
    propagate_const<buff_t*> cold_heart;
    propagate_const<buff_t*> gathering_storm;
    propagate_const<buff_t*> inexorable_assault;
    propagate_const<buff_t*> killing_machine;
    propagate_const<buff_t*> pillar_of_frost;
    propagate_const<buff_t*> pillar_of_frost_bonus; // Additional strength from runes spent
    propagate_const<buff_t*> remorseless_winter;
    propagate_const<buff_t*> rime;
    propagate_const<buff_t*> unleashed_frenzy;
    propagate_const<buff_t*> bonegrinder_crit;
    propagate_const<buff_t*> bonegrinder_frost;
    propagate_const<buff_t*> enduring_strength_builder;
    propagate_const<buff_t*> enduring_strength;
    propagate_const<buff_t*> frostwhelps_aid;
    buff_t* wrath_of_the_frostwyrm; // T30 4pc

    // Unholy
    propagate_const<buff_t*> dark_transformation;
    propagate_const<buff_t*> runic_corruption;
    propagate_const<buff_t*> sudden_doom;
    propagate_const<buff_t*> unholy_assault;
    propagate_const<buff_t*> unholy_pact;
    propagate_const<buff_t*> festermight;
    propagate_const<buff_t*> ghoulish_frenzy;
    propagate_const<buff_t*> plaguebringer; 
    propagate_const<buff_t*> ghoulish_infusion;
    propagate_const<buff_t*> commander_of_the_dead;
    propagate_const<buff_t*> defile_buff;
    buff_t* unholy_t30_2pc_stacking;
    buff_t* unholy_t30_2pc_mastery;
    buff_t* unholy_t30_4pc_mastery;

  } buffs;

  struct runeforge_t {
    bool rune_of_apocalypse;
    bool rune_of_hysteria;
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
  struct cooldowns_t {
    // Shared
    propagate_const<cooldown_t*> abomination_limb;
    propagate_const<cooldown_t*> death_and_decay_dynamic; // Shared cooldown object for death and decay, defile and death's due
    propagate_const<cooldown_t*> mind_freeze;

    // Blood
    cooldown_t* bone_shield_icd; // internal cooldown between bone shield stack consumption
    cooldown_t* blood_tap;
    cooldown_t* dancing_rune_weapon;
    propagate_const<cooldown_t*> vampiric_blood;
    // Frost
    propagate_const<cooldown_t*> icecap_icd; // internal cooldown that prevents several procs on the same dual-wield attack
    propagate_const<cooldown_t*> inexorable_assault_icd;  // internal cooldown to prevent multiple procs during aoe
    propagate_const<cooldown_t*> koltiras_favor_icd; // internal cooldown that prevents several procs on the same dual-wield attack
    propagate_const<cooldown_t*> frigid_executioner_icd; // internal cooldown that prevents several procs on the same dual-wield attack
    propagate_const<cooldown_t*> enduring_strength_icd; // internal cooldown that prevents several procs on the same dual-wield attacl
    propagate_const<cooldown_t*> pillar_of_frost;
    cooldown_t* frostwyrms_fury;
    
    // Unholy
    propagate_const<cooldown_t*> apocalypse;
    propagate_const<cooldown_t*> army_of_the_dead;
    propagate_const<cooldown_t*> dark_transformation;
    propagate_const<cooldown_t*> vile_contagion;
	
  } cooldown;

  // Active Spells
  struct active_spells_t {
    // Shared
    propagate_const<action_t*> runeforge_pestilence;
    propagate_const<action_t*> runeforge_razorice;
    propagate_const<action_t*> runeforge_sanguination;

    // Class Tree
    propagate_const<action_t*> blood_draw;

    // Blood
    propagate_const<action_t*> mark_of_blood_heal;
    action_t* shattering_bone;

    // Unholy
    propagate_const<action_t*> bursting_sores;
    propagate_const<action_t*> festering_wound;
    propagate_const<action_t*> virulent_eruption;
    propagate_const<action_t*> ruptured_viscera;

  } active_spells;

  // Gains
  struct gains_t {
    // Shared
    propagate_const<gain_t*> antimagic_shell; // RP from magic damage absorbed
    propagate_const<gain_t*> antimagic_zone; // RP from magic damage absorbed
    gain_t* rune; // Rune regeneration
    propagate_const<gain_t*> rune_of_hysteria;
    propagate_const<gain_t*> spirit_drain;
    propagate_const<gain_t*> coldthirst;
    propagate_const<gain_t*> start_of_combat_overflow;

    // Blood
    propagate_const<gain_t*> blood_tap;
    propagate_const<gain_t*> bryndaors_might;
    propagate_const<gain_t*> drw_heart_strike; // Blood Strike, Blizzard's hack to replicate HS rank 2 with DRW
    propagate_const<gain_t*> heartbreaker;
    propagate_const<gain_t*> tombstone;
    gain_t* vigorous_lifeblood_2pc; // T29 2pc

    // Frost
    propagate_const<gain_t*> breath_of_sindragosa;
    propagate_const<gain_t*> empower_rune_weapon;
    propagate_const<gain_t*> frost_fever; // RP generation per tick
    propagate_const<gain_t*> horn_of_winter;
    propagate_const<gain_t*> koltiras_favor;
    propagate_const<gain_t*> frigid_executioner; // Rune refund chance
    propagate_const<gain_t*> murderous_efficiency;
    propagate_const<gain_t*> obliteration;
    propagate_const<gain_t*> rage_of_the_frozen_champion;
    propagate_const<gain_t*> runic_attenuation;
    propagate_const<gain_t*> runic_empowerment;

    // Unholy
    propagate_const<gain_t*> apocalypse;
    propagate_const<gain_t*> festering_wound;
    propagate_const<gain_t*> replenishing_wounds;
    propagate_const<gain_t*> feasting_strikes;
  } gains;

  // Specialization
  struct specialization_t {
    // Class/spec auras
    const spell_data_t* death_knight;
    const spell_data_t* plate_specialization;
    const spell_data_t* riposte;
    const spell_data_t* blood_fortification;
    const spell_data_t* blood_death_knight;
    const spell_data_t* frost_death_knight;
    const spell_data_t* unholy_death_knight;

    // Shared
    const spell_data_t* frost_fever; // The RP energize spell is a spec ability in spelldata
    const spell_data_t* death_coil;
    const spell_data_t* death_grip;
    const spell_data_t* dark_command;
    const spell_data_t* death_and_decay;
    const spell_data_t* rune_strike;

    // Frost
    const spell_data_t* remorseless_winter;
    const spell_data_t* might_of_the_frozen_wastes;
    const spell_data_t* frostreaper;

    // Unholy
    const spell_data_t* dark_transformation_2;
    const spell_data_t* festering_wound;
    
  } spec;

  // Mastery
  struct mastery_t {
    const spell_data_t* blood_shield; // Blood
    const spell_data_t* frozen_heart; // Frost
    const spell_data_t* dreadblade; // Unholy
  } mastery;

  // Talents
  struct talents_t {

    // Shared Class Tree
    player_talent_t chains_of_ice;
    player_talent_t death_strike;
    player_talent_t raise_dead;
    // Row 2
    player_talent_t mind_freeze;
    player_talent_t antimagic_shell;
    player_talent_t cleaving_strikes;
    // Row 3
    player_talent_t blinding_sleet; // NYI
    player_talent_t coldthirst;
    player_talent_t permafrost; // NYI
    player_talent_t improved_death_strike;
    player_talent_t antimagic_barrier;
    player_talent_t march_of_darkness; // NYI
    player_talent_t sacrificial_pact;
    player_talent_t control_undead; // NYI
    player_talent_t enfeeble; // NYI
    // Row 4
    player_talent_t icebound_fortitude;
    player_talent_t blood_scent;
    player_talent_t veteran_of_the_third_war;
    player_talent_t suppression; // NYI
    player_talent_t brittle;
    // Row 5
    player_talent_t acclimation; // NYI
    player_talent_t merciless_strikes;
    player_talent_t antimagic_zone; // NYI
    player_talent_t might_of_thassarian;
    player_talent_t clenching_grasp; // NYI
    // Row 6
    player_talent_t proliferating_chill;
    player_talent_t asphyxiate; // NYI
    player_talent_t assimilation; // NYI
    player_talent_t death_pact; // NYI
    player_talent_t grip_of_the_dead; // NYI
    player_talent_t deaths_reach; // NYI
    player_talent_t unholy_endurance; // NYI
    player_talent_t gloom_ward; // NYI
    // Row 7
    player_talent_t runic_attenuation;
    player_talent_t wraith_walk; // NYI
    player_talent_t unholy_ground;
    player_talent_t insidious_chill; // NYI
    // Row 8
    player_talent_t blood_draw;
    player_talent_t will_of_the_necropolis; // NYI
    player_talent_t deaths_echo;
    // Row 9
    player_talent_t icy_talons;
    player_talent_t rune_mastery;
    player_talent_t unholy_bond;
    // Row 10
    player_talent_t empower_rune_weapon;
    player_talent_t abomination_limb;
    player_talent_t soul_reaper;

    // Blood
    struct {
      // Row 1
      player_talent_t heart_strike;
      // Row 2
      player_talent_t marrowrend;
      player_talent_t blood_boil;
      // Row 3
      player_talent_t vampiric_blood;
      player_talent_t crimson_scourge;
      // Row 4
      player_talent_t ossuary;
      player_talent_t improved_vampiric_blood;
      player_talent_t improved_heart_strike;
      player_talent_t deaths_caress;
      // Row 5
      player_talent_t rune_tap;
      player_talent_t heartbreaker;
      player_talent_t foul_bulwark;
      player_talent_t dancing_rune_weapon;
      player_talent_t hemostasis;
      player_talent_t perseverance_of_the_ebon_blade;
      player_talent_t relish_in_blood;
      // Row 6
      player_talent_t leeching_strike;
      player_talent_t improved_bone_shield;
      player_talent_t insatiable_blade;
      player_talent_t blooddrinker;
      player_talent_t consumption;
      player_talent_t rapid_decomposition;
      // Row 7
      player_talent_t blood_feast;
      player_talent_t blood_tap;
      player_talent_t reinforced_bones;
      player_talent_t everlasting_bond;
      player_talent_t voracious;
      player_talent_t bloodworms;
      player_talent_t coagulopathy;
      // Row 8
      player_talent_t mark_of_blood;
      player_talent_t tombstone;
      player_talent_t gorefiends_grasp;
      player_talent_t sanguine_ground;
      // Row 9
      player_talent_t shattering_bone;
      player_talent_t heartrend;
      player_talent_t tightening_grasp;
      player_talent_t iron_heart;
      player_talent_t red_thirst;
      // Row 10
      player_talent_t bonestorm;
      player_talent_t purgatory;
      player_talent_t bloodshot;
      player_talent_t umbilicus_eternus;
    } blood;

    // Frost
    struct {
      // Row 1
      player_talent_t frost_strike;
      // Row 2
      player_talent_t obliterate;
      player_talent_t howling_blast;
      // Row 3
      player_talent_t killing_machine;
      player_talent_t rime;
      // Row 4
      player_talent_t unleashed_frenzy;
      player_talent_t runic_command;
      player_talent_t improved_frost_strike;
      // Row 5
      player_talent_t improved_obliterate;
      player_talent_t glacial_advance;
      player_talent_t pillar_of_frost;
      player_talent_t improved_rime;
      player_talent_t frostscythe;
      // Row 6
      player_talent_t rage_of_the_frozen_champion;
      player_talent_t frigid_executioner;
      player_talent_t horn_of_winter;
      player_talent_t fatal_fixation;
      player_talent_t enduring_strength;
      player_talent_t frostwhelps_aid;
      player_talent_t cold_heart;
      player_talent_t biting_cold;
      player_talent_t chill_streak;
      // Row 7
      player_talent_t murderous_efficiency;
      player_talent_t inexorable_assault;
      player_talent_t icecap;
      player_talent_t empower_rune_weapon;
      player_talent_t gathering_storm;
      player_talent_t enduring_chill;
      player_talent_t piercing_chill;
      // Row 8
      player_talent_t bonegrinder;
      player_talent_t shattering_blade;
      player_talent_t avalanche;
      player_talent_t icebreaker;
      player_talent_t everfrost;
      // Row 9
      player_talent_t cold_blooded_rage;
      player_talent_t frostwyrms_fury;
      player_talent_t invigorating_freeze;
      // Row 10
      player_talent_t obliteration;
      player_talent_t absolute_zero;
      player_talent_t breath_of_sindragosa;
    } frost;

    // Unholy
    struct {
      // Row 1
      player_talent_t festering_strike;
      // Row 2
      player_talent_t scourge_strike;
      player_talent_t raise_dead;
      // Row 3
      player_talent_t outbreak;
      player_talent_t dark_transformation;
      // Row 4
      player_talent_t unholy_blight;
      player_talent_t improved_festering_strike; // Horrible naming convention, plz change blizz
      player_talent_t runic_mastery;
      player_talent_t infected_claws;
      // Row 5
      player_talent_t epidemic;
      player_talent_t replenishing_wounds;
      player_talent_t feasting_strikes;
      player_talent_t apocalypse;
      player_talent_t clawing_shadows;
      player_talent_t plaguebringer;
      player_talent_t sudden_doom;
      player_talent_t all_will_serve;
      // Row 6
      player_talent_t bursting_sores;
      player_talent_t ebon_fever;
      player_talent_t unholy_command;
      player_talent_t magus_of_the_dead;
      player_talent_t ruptured_viscera;
      player_talent_t improved_death_coil;
      player_talent_t rotten_touch;
      player_talent_t unholy_pact;
      player_talent_t defile;
      // Row 7
      player_talent_t vile_contagion;
      player_talent_t pestilence;
      player_talent_t eternal_agony;
      player_talent_t coil_of_devastation;
      player_talent_t harbinger_of_doom;
      player_talent_t reaping;
      // Row 8
      player_talent_t death_rot;
      player_talent_t army_of_the_dead;
      player_talent_t summon_gargoyle;
      // Row 9
      player_talent_t festermight;
      player_talent_t ghoulish_frenzy;
      player_talent_t army_of_the_damned;
      player_talent_t morbidity;
      player_talent_t unholy_aura;
      // Row 10
      player_talent_t unholy_assault;
      player_talent_t superstrain;
      player_talent_t commander_of_the_dead;
    } unholy;
  } talent;

  // Spells
  struct spells_t {
    // Shared
    const spell_data_t* brittle_debuff;
    const spell_data_t* dnd_buff; // obliterate aoe increase while in death's due (nf covenant ability)
    const spell_data_t* razorice_debuff;
    const spell_data_t* rune_mastery_buff;
    const spell_data_t* empower_rune_weapon_main; // Empower Rune Weapon has a unique ID for the spell itself, with each talent just modifying number of charges. 
    const spell_data_t* coldthirst_gain; // Coldthirst has a unique ID for the gain and cooldown reduction
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

    // Diseases (because they're not stored in spec data, unlike frost fever's rp gen...)
    const spell_data_t* blood_plague;
    const spell_data_t* frost_fever;
    const spell_data_t* virulent_plague;
    const spell_data_t* virulent_erruption;

    // Blood
    const spell_data_t* blood_shield;
    const spell_data_t* bone_shield;
    const spell_data_t* sanguine_ground;
    const spell_data_t* tightening_grasp_debuff;
    const spell_data_t* ossuary_buff;
    const spell_data_t* crimson_scourge_buff;
    const spell_data_t* heartrend_buff;
    const spell_data_t* preserverence_of_the_ebon_blade_buff;
    const spell_data_t* voracious_buff;
    const spell_data_t* blood_draw_damage;
    const spell_data_t* blood_draw_cooldown;
    const spell_data_t* bonestorm_heal;
    const spell_data_t* dancing_rune_weapon_buff;
    const spell_data_t* relish_in_blood_gains;
    const spell_data_t* leeching_strike_damage;
    const spell_data_t* shattering_bone_damage;

    // Frost
    const spell_data_t* runic_empowerment_gain;
    const spell_data_t* murderous_efficiency_gain;
    const spell_data_t* rage_of_the_frozen_champion; // RP generation spell
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
    const spell_data_t* lingering_chill;
    const spell_data_t* frost_t30_2pc; // TODO rename when blizz gives it a name
    const spell_data_t* frost_t30_4pc; // TODO rename when blizz gives it a name
    const spell_data_t* wrath_of_the_frostwyrm_damage;
    const spell_data_t* obliteration_gains;

    // Unholy
    const spell_data_t* runic_corruption; // buff
    const spell_data_t* runic_corruption_chance;
    const spell_data_t* festering_wound_debuff;
    const spell_data_t* rotten_touch_debuff;
    const spell_data_t* death_rot_debuff;
    const spell_data_t* coil_of_devastation_debuff;
    const spell_data_t* feasting_strikes_gain;
    const spell_data_t* ghoulish_frenzy_player;
    const spell_data_t* plaguebringer_buff;
    const spell_data_t* festermight_buff;
    const spell_data_t* ghoulish_infusion;
    const spell_data_t* unholy_blight_dot;
    const spell_data_t* commander_of_the_dead;
    const spell_data_t* defile_buff;
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
    // T30
    const spell_data_t* unholy_t30_2pc_stacking;
    const spell_data_t* unholy_t30_2pc_mastery;
    const spell_data_t* unholy_t30_4pc_mastery;
    const spell_data_t* unholy_t30_2pc_values;
    const spell_data_t* unholy_t30_4pc_values;

    // T29 Blood
    const spell_data_t* vigorous_lifeblood_4pc; // Damage and haste buff
    const spell_data_t* vigorous_lifeblood_energize; // Rune refund

    // T30 Blood
    const spell_data_t* vampiric_strength; // Str buff
  } spell;

  // Pet Abilities
  struct pet_spells_t {
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
    // All Will Serve skulker
    const spell_data_t* skulker_shot;
    // Army of the damned magus
    const spell_data_t* frostbolt;
    const spell_data_t* shadow_bolt;
    // Commander of the Dead Talent
    const spell_data_t* commander_of_the_dead;
    // Ruptured Viscera Talent
    const spell_data_t* ruptured_viscera;
    // Ghoulish Frenzy
    const spell_data_t* ghoulish_frenzy;
    // Vile Infusion
    const spell_data_t* vile_infusion;
    // DRW Spells
    const spell_data_t* drw_heart_strike;
    const spell_data_t* drw_heart_strike_rp_gen;
  } pet_spell;

  // RPPM
  struct rppm_t
  {
    real_ppm_t* bloodworms;
    real_ppm_t* runic_attenuation;
  } rppm;

  // Pets and Guardians
  struct pets_t
  {
    pets::dancing_rune_weapon_pet_t* dancing_rune_weapon_pet;
    pets::dancing_rune_weapon_pet_t* everlasting_bond_pet;
    pets::gargoyle_pet_t* gargoyle;
    pets::ghoul_pet_t* ghoul_pet;
    pet_t* risen_skulker;

    spawner::pet_spawner_t<pets::army_ghoul_pet_t, death_knight_t> army_ghouls;
    spawner::pet_spawner_t<pets::army_ghoul_pet_t, death_knight_t> apoc_ghouls;
    spawner::pet_spawner_t<pets::bloodworm_pet_t, death_knight_t> bloodworms;
    spawner::pet_spawner_t<pets::magus_pet_t, death_knight_t> army_magus;
    spawner::pet_spawner_t<pets::magus_pet_t, death_knight_t> apoc_magus;

    pets_t(death_knight_t* p) :
        army_ghouls("army_ghoul", p),
        apoc_ghouls("apoc_ghoul", p),
        bloodworms("bloodworm", p),
        army_magus("army_magus", p),
        apoc_magus("apoc_magus", p)

    {}
  } pets;

  // Procs
  struct procs_t
  {
    // Normal rune regeneration proc
    propagate_const<proc_t*> ready_rune;

    propagate_const<proc_t*> bloodworms;

    // Killing Machine spent on
    propagate_const<proc_t*> killing_machine_oblit;
    propagate_const<proc_t*> killing_machine_fsc;

    // Killing machine triggered by
    propagate_const<proc_t*> km_from_crit_aa;
    propagate_const<proc_t*> km_from_cold_blooded_rage; // Frost strike with talent
    propagate_const<proc_t*> km_from_obliteration_fs;   // Frost Strike during Obliteration
    propagate_const<proc_t*> km_from_obliteration_hb;   // Howling Blast during Obliteration
    propagate_const<proc_t*> km_from_obliteration_ga;   // Glacial Advance during Obliteration
    propagate_const<proc_t*> km_from_obliteration_sr;   // Soul Reaper during Obliteration
    propagate_const<proc_t*> km_from_t29_4pc;           // T29 Frost 4PC

    // Killing machine refreshed by
    propagate_const<proc_t*> km_from_crit_aa_wasted;
    propagate_const<proc_t*> km_from_cold_blooded_rage_wasted; // Frost strike with talent
    propagate_const<proc_t*> km_from_obliteration_fs_wasted;   // Frost Strike during Obliteration
    propagate_const<proc_t*> km_from_obliteration_hb_wasted;   // Howling Blast during Obliteration
    propagate_const<proc_t*> km_from_obliteration_ga_wasted;   // Glacial Advance during Obliteration
    propagate_const<proc_t*> km_from_obliteration_sr_wasted;   // Soul Reaper during Obliteration
    propagate_const<proc_t*> km_from_t29_4pc_wasted;           // T29 Frost 4PC

    // Runic corruption triggered by
    propagate_const<proc_t*> rp_runic_corruption; // from RP spent
    propagate_const<proc_t*> sr_runic_corruption; // from soul reaper
    propagate_const<proc_t*> al_runic_corruption; // from abomination limb
    propagate_const<proc_t*> fs_runic_corruption; // from feasting strikes

    // Festering Wound applied by
    propagate_const<proc_t*> fw_festering_strike;
    propagate_const<proc_t*> fw_infected_claws;
    propagate_const<proc_t*> fw_necroblast;
    propagate_const<proc_t*> fw_pestilence;
    propagate_const<proc_t*> fw_unholy_assault;
    propagate_const<proc_t*> fw_vile_contagion;
    propagate_const<proc_t*> fw_ruptured_viscera;
  } procs;

  // Death Knight Options
  struct options_t
  {
    bool disable_aotd = false;
    bool split_ghoul_regen = false;
    bool split_obliterate_schools = true;
    double ams_absorb_percent = 0;
    double amz_absorb_percent = 0;
    bool individual_pet_reporting = false;
  } options;

  // Runes
  runes_t _runes;

  death_knight_t( sim_t* sim, util::string_view name, race_e r ) :
    player_t( sim, DEATH_KNIGHT, name, r ),
    active_dnd( nullptr ),
    deprecated_dnd_expression( false ),
    runeforge_expression_warning( false ),
    km_proc_attempts( 0 ),
    festering_wounds_target_count( 0 ),
    bone_shield_charges_consumed( 0 ),
    antimagic_shell( nullptr ),
    antimagic_zone( nullptr ),
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
    _runes( this )
  {
    cooldown.abomination_limb         = get_cooldown( "abomination_limb_proc" );
    cooldown.apocalypse               = get_cooldown( "apocalypse" );
    cooldown.army_of_the_dead         = get_cooldown( "army_of_the_dead" );
    cooldown.blood_tap                = get_cooldown( "blood_tap" );
    cooldown.bone_shield_icd          = get_cooldown( "bone_shield_icd" );
    cooldown.dancing_rune_weapon      = get_cooldown( "dancing_rune_weapon" );
    cooldown.dark_transformation      = get_cooldown( "dark_transformation" );
    cooldown.death_and_decay_dynamic  = get_cooldown( "death_and_decay" ); // Default value, changed during action construction
    cooldown.icecap_icd               = get_cooldown( "icecap" );
    cooldown.inexorable_assault_icd   = get_cooldown( "inexorable_assault_icd" );
    cooldown.koltiras_favor_icd       = get_cooldown( "koltiras_favor_icd" );
    cooldown.frigid_executioner_icd   = get_cooldown( "frigid_executioner_icd" );
    cooldown.pillar_of_frost          = get_cooldown( "pillar_of_frost" );
    cooldown.vampiric_blood           = get_cooldown( "vampiric_blood" );
    cooldown.enduring_strength_icd    = get_cooldown( "enduring_strength" );
    cooldown.mind_freeze              = get_cooldown( "mind_freeze" );
    cooldown.frostwyrms_fury          = get_cooldown( "frostwyrms_fury_driver" );

    resource_regeneration = regen_type::DYNAMIC;
  }

  // Character Definition overrides
  void      init_spells() override;
  void      init_action_list() override;
  void      init_rng() override;
  void      init_base_stats() override;
  void      init_scaling() override;
  void      create_buffs() override;
  void      init_gains() override;
  void      init_procs() override;
  void      init_finished() override;
  double    composite_bonus_armor() const override;
  double    composite_attack_power_multiplier() const override;
  double    composite_melee_speed() const override;
  double    composite_melee_haste() const override;
  double    composite_spell_haste() const override;
  double    composite_attribute_multiplier( attribute_e attr ) const override;
  double    matching_gear_multiplier( attribute_e attr ) const override;
  double    composite_parry_rating() const override;
  double    composite_parry() const override;
  double    composite_dodge() const override;
  double    composite_leech() const override;
  double    composite_melee_expertise( const weapon_t* ) const override;
  double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double    composite_player_multiplier( school_e school ) const override;
  double    composite_player_pet_damage_multiplier( const action_state_t* /* state */, bool /* guardian */ ) const override;
  double    composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const override;
  double    composite_melee_crit_chance() const override;
  double    composite_spell_crit_chance() const override;
  double    composite_crit_avoidance() const override;
  double    composite_mastery_value() const override;
  void      combat_begin() override;
  void      activate() override;
  void      reset() override;
  void      arise() override;
  void      adjust_dynamic_cooldowns() override;
  void      assess_heal( school_e, result_amount_type, action_state_t* ) override;
  void      assess_damage( school_e, result_amount_type, action_state_t* ) override;
  void      assess_damage_imminent( school_e, result_amount_type, action_state_t* ) override;
  void      target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  void      do_damage( action_state_t* ) override;
  void      create_actions() override;
  action_t* create_action( util::string_view name, util::string_view options ) override;
  std::unique_ptr<expr_t>   create_expression( util::string_view name ) override;
  void      create_options() override;
  void      create_pets() override;
  resource_e primary_resource() const override { return RESOURCE_RUNIC_POWER; }
  role_e    primary_role() const override;
  stat_e    convert_hybrid_stat( stat_e s ) const override;
  void      invalidate_cache( cache_e ) override;
  double    resource_gain( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  double    resource_loss( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  void      copy_from( player_t* source ) override;
  void      merge( player_t& other ) override;
  void      datacollection_begin() override;
  void      datacollection_end() override;
  void      analyze( sim_t& sim ) override;
  void      apply_affecting_auras( action_t& action ) override;

  // Default consumables
  std::string default_flask() const override { return death_knight_apl::flask( this ); }
  std::string default_food() const override { return death_knight_apl::food( this ); }
  std::string default_potion() const override { return death_knight_apl::potion( this ); }
  std::string default_rune() const override { return death_knight_apl::rune( this ); }
  std::string default_temporary_enchant() const override { return death_knight_apl::temporary_enchant( this ); }
  
  // Create Profile options
  std::string create_profile( save_e ) override;

  // Death Knight specific methods
  // Rune related methods
  double    runes_per_second() const;
  double    rune_regen_coefficient() const;
  unsigned  replenish_rune( unsigned n, gain_t* gain = nullptr );
  // Shared
  bool      in_death_and_decay() const;
  // Blood
  void      bone_shield_handler( const action_state_t* ) const;
  // Frost
  void      trigger_killing_machine( double chance, proc_t* proc, proc_t* wasted_proc );
  void      consume_killing_machine( proc_t* proc );
  void      trigger_runic_empowerment( double rpcost );
  // Unholy
  void      trigger_festering_wound( const action_state_t* state, unsigned n_stacks = 1, proc_t* proc = nullptr );
  void      burst_festering_wound( player_t* target, unsigned n = 1 );
  void      trigger_runic_corruption( proc_t* proc, double rpcost, double override_chance = -1.0, bool death_trigger = false );
  // Start the repeated stacking of buffs, called at combat start
  void      start_cold_heart();
  void      start_inexorable_assault();
  // On-target-death triggers
  void      trigger_festering_wound_death( player_t* );
  void      trigger_soul_reaper_death( player_t* );
  void      trigger_virulent_plague_death( player_t* );

  // Runeforge expression handling for Death Knight Runeforges (not legendary)
  std::unique_ptr<expr_t> create_runeforge_expression( util::string_view runeforge_name, bool warning );

  target_specific_t<death_knight_td_t> target_data;

  const death_knight_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  death_knight_td_t* get_target_data( player_t* target ) const override
  {
    death_knight_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new death_knight_td_t( target, const_cast<death_knight_t*>( this ) );
    }
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

inline death_knight_td_t::death_knight_td_t( player_t* target, death_knight_t* p ) :
  actor_target_data_t( target, p )
{
  // Diseases
  dot.blood_plague         = target -> get_dot( "blood_plague", p );
  dot.frost_fever          = target -> get_dot( "frost_fever", p );
  dot.virulent_plague      = target -> get_dot( "virulent_plague", p );
  dot.unholy_blight        = target -> get_dot( "unholy_blight_dot", p );
  // Other dots
  dot.soul_reaper          = target -> get_dot( "soul_reaper", p );
  
  // General Talents
  debuff.brittle          = make_buff( *this, "brittle", p -> spell.brittle_debuff )
                            -> set_default_value_from_effect( 1 );

  // Blood
  debuff.mark_of_blood    = make_buff( *this, "mark_of_blood", p -> talent.blood.mark_of_blood )
                           -> set_cooldown( 0_ms );  // Handled by the action

  debuff.tightening_grasp = make_buff( *this, "tightening_grasp", p -> spell.tightening_grasp_debuff )
                              -> set_default_value( p -> spell.tightening_grasp_debuff -> effectN( 1 ).percent() );
  // Frost
  debuff.razorice         = make_buff( *this, "razorice", p -> spell.razorice_debuff )
                            -> set_default_value_from_effect( 1 )
                            -> set_period( 0_ms )
                            -> apply_affecting_aura( p -> talent.unholy_bond );

  debuff.piercing_chill   = make_buff( *this, "piercing_chill", p -> spell.piercing_chill_debuff )
                            -> set_default_value_from_effect( 1 );

  debuff.everfrost = make_buff( *this, "everfrost", p -> talent.frost.everfrost -> effectN( 1 ).trigger() )
                            -> set_default_value( p -> talent.frost.everfrost -> effectN( 1 ).percent() );

  debuff.lingering_chill = make_buff( *this, "lingering_chill", p -> spell.lingering_chill )
                            -> set_default_value( p -> spell.lingering_chill -> effectN( 1 ).percent() );

  // Unholy
  debuff.festering_wound  = make_buff( *this, "festering_wound", p -> spell.festering_wound_debuff )
                           -> set_cooldown( 0_ms )  // Handled by death_knight_t::trigger_festering_wound()
                           -> set_stack_change_callback( [ p ]( buff_t*, int old_, int new_ )
                           {
                             // Update the FW target count if needed
                             if ( old_ == 0 )
                               p -> festering_wounds_target_count++;
                             else if ( new_ == 0 )
                               p -> festering_wounds_target_count--;
                           } );
						  
  debuff.rotten_touch     = make_buff( *this, "rotten_touch", p -> spell.rotten_touch_debuff )
                           -> set_default_value_from_effect( 1 );
  
  debuff.death_rot        = make_buff( *this, "death_rot", p -> spell.death_rot_debuff )
                           -> set_default_value_from_effect( 1 );

  // Apocalypse Death Knight Runeforge Debuffs
  debuff.apocalypse_death  = make_buff( *this, "death", p -> spell.apocalypse_death_debuff )  // Effect not implemented
                            -> apply_affecting_aura( p -> talent.unholy_bond );
  debuff.apocalypse_famine = make_buff( *this, "famine", p -> spell.apocalypse_famine_debuff )
                            -> set_default_value_from_effect( 1 )
                            -> apply_affecting_aura( p -> talent.unholy_bond );
  debuff.apocalypse_war    = make_buff( *this, "war", p -> spell.apocalypse_war_debuff )
                            -> set_default_value_from_effect( 1 )
                            -> apply_affecting_aura( p -> talent.unholy_bond );


}

// ==========================================================================
// Death Knight Runes ( part 2 )
// ==========================================================================

inline void rune_event_t::execute_event()
{
  sim().print_debug( "{} regenerates a rune, start_time={}, regen_time={} current_coeff={}",
                     m_rune -> runes -> dk -> name(), m_rune -> regen_start.total_seconds(),
                     ( sim().current_time() - m_rune -> regen_start ).total_seconds(),
                     m_coefficient );

  m_rune -> fill_rune();
}

static void log_rune_status( const death_knight_t* p )
{
  if ( ! p -> sim -> debug ) return;
  std::string rune_string = p -> _runes.string_representation();
  p -> sim -> print_debug( "{} runes: {}", p -> name(), rune_string );
}

inline runes_t::runes_t( death_knight_t* p ) : dk( p ),
  cumulative_waste( dk -> name_str + "_Iteration_Rune_Waste", false ),
  rune_waste( dk -> name_str + "_Rune_Waste", false ),
  iteration_waste_sum( 0_ms )
{
  for ( auto& rune: slot )
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
  auto n_full_runes = runes_full();
  int n_wasting_runes = n_full_runes - MAX_REGENERATING_RUNES;
  int disable_waste = n_full_runes - runes <= MAX_REGENERATING_RUNES;

  while ( runes-- )
  {
    auto rune = first_full_rune();
    if ( n_wasting_runes > 0 )
    {
      // Waste time for a given rune is determined as the time the actor went past the maximum
      // regenerating runesa (waste_start), or if later in time, the time this specific rune
      // replenished (rune -> regenerated).
      auto wasted_time = dk -> sim -> current_time() - std::max( rune -> regenerated, waste_start );
      if ( wasted_time > 0_ms )
      {
        assert( wasted_time > 0_ms );
        iteration_waste_sum += wasted_time;
        rune_waste.add( wasted_time.total_seconds() );

        dk -> sim -> print_debug( "{} rune waste, n_full_runes={} rune_regened={} waste_started={} wasted_time={}",
          dk -> name(), n_full_runes, rune -> regenerated.total_seconds(), waste_start.total_seconds(), wasted_time.total_seconds() );

        n_wasting_runes--;
      }
    }
    rune -> consume();
  }

  log_rune_status( dk );

  // Full runes will be going below the maximum number of regenerating runes, so there's no longer
  // going to be any waste time.
  if ( disable_waste && waste_start >= 0_ms )
  {
    dk -> sim -> print_debug( "{} rune waste, waste ended, n_full_runes={}",
        dk -> name(), runes_full() );

    waste_start = timespan_t::min();
  }
}

inline void runes_t::regenerate_immediate( timespan_t seconds )
{
  if ( seconds <= 0_ms )
  {
    dk -> sim -> errorf( "%s warning, regenerating runes with an invalid immediate value (%.3f)",
      dk -> name(), seconds.total_seconds() );
    return;
  }

  dk -> sim -> print_debug( "{} regenerating runes with an immediate value of {}",
    dk -> name(), seconds.total_seconds() );
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
  });

  // Sort regenerating runes by ascending remaining time
  range::sort( regenerating_runes, []( const rune_t* l, const rune_t* r ) {
    timespan_t lv = l -> event -> remains();
    timespan_t rv = r -> event -> remains();
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
    while ( !regenerating_runes.empty() && regenerating_runes.front() -> is_ready() )
    {
      regenerating_runes.erase( regenerating_runes.begin() );
    }

    // Move any new regenerating runes from depleted runes to regenerating runes. They can be placed
    // at the back, since they will automatically have the longest regen time. This can occur if
    // there are depleted runes, and a regenerating rune fills up (causing a depleted rune to start
    // regenerating)
    while ( true )
    {
      auto it = range::find_if( depleted_runes, []( rune_t* r ) { return r -> is_regenerating(); } );
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
    auto first_left = regenerating_runes[ 0 ] -> event -> remains();

    // Clamp regenerating time units to the minimum of total regeneration time left, and the first
    // regenerating rune's time left
    timespan_t units_regened = std::min( seconds_left, first_left );

    // Regenerate all regenerating runes by units_regened
    for ( auto rune : regenerating_runes )
    {
      rune -> adjust_regen_event( -units_regened );
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
  if ( dk -> resources.current[ RESOURCE_RUNE ] >= as<double>( n_runes ) )
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
  });

  // Sort by ascending remaining time
  range::sort( regenerating_runes, []( const rune_t* l, const rune_t* r ) {
    timespan_t lv = l -> event -> remains();
    timespan_t rv = r -> event -> remains();
    // Use pointers as tiebreaker
    if ( lv == rv )
    {
      return l < r;
    }

    return lv < rv;
  } );

  // Number of unsatisfied runes
  int n_unsatisfied = n_runes - as<int>( dk -> resources.current[ RESOURCE_RUNE ] );

  // If we can satisfy the remaining unsatisfied runes with regenerating runes, return the N - 1th
  // remaining regeneration time
  if ( n_unsatisfied <= regenerating_runes.size() )
  {
    return regenerating_runes[ n_unsatisfied - 1 ] -> event -> remains();
  }

  // Which regenerating rune time should be picked when we have more unsatisfied runes than
  // currently regenerating ones.
  auto nth_regenerating_rune = n_unsatisfied - regenerating_runes.size();

  // Otherwise, the time is going to be the nth rune regen time plus the time it takes to regen a
  // depleted rune
  return regenerating_runes[ nth_regenerating_rune - 1 ] -> event -> remains() +
         timespan_t::from_seconds( 1 / dk -> runes_per_second() );
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

  auto regen_time_elapsed = runes -> dk -> sim -> current_time() - regen_start;
  auto total_rune_time = regen_time_elapsed + event -> remains();

  return 1.0 - event -> remains() / total_rune_time;
}

inline void rune_t::update_coefficient()
{
  if ( event )
  {
    event -> update_coefficient( runes -> dk -> rune_regen_coefficient() );
  }
}

inline void rune_t::consume()
{
  assert( state == STATE_FULL && event == nullptr );

  state = STATE_DEPLETED;

  // Immediately update the state of the next regenerating rune, since rune_t::regen_rune presumes
  // that the internal state of each invidual rune is always consistent with the rune regeneration
  // rules
  if ( runes -> runes_regenerating() < MAX_REGENERATING_RUNES )
  {
    runes -> first_depleted_rune() -> start_regenerating();
  }

  // Internal state consistency for current rune regeneration rules
  assert( runes -> runes_regenerating() <= MAX_REGENERATING_RUNES );
  assert( runes -> runes_depleted() == MAX_RUNES - runes -> runes_full() - runes -> runes_regenerating() );
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
    runes -> dk -> procs.ready_rune -> occur();
  }

  state = STATE_FULL;
  regenerated = runes -> dk -> sim -> current_time();

  // Update actor rune resources, so we can re-use a lot of the normal resource mechanisms that the
  // sim core offers
  runes -> dk -> resource_gain( RESOURCE_RUNE, 1, gain ? gain : runes -> dk -> gains.rune );

  // Immediately update the state of the next regenerating rune, since rune_t::regen_rune presumes
  // that the internal state of each invidual rune is always consistent with the rune regeneration
  // rules
  if ( runes -> runes_depleted() > 0 && runes -> runes_regenerating() < MAX_REGENERATING_RUNES )
  {
    runes -> first_depleted_rune() -> start_regenerating();
  }

  // Internal state consistency for current rune regeneration rules
  assert( runes -> runes_regenerating() <= MAX_REGENERATING_RUNES );
  assert( runes -> runes_depleted() == MAX_RUNES - runes -> runes_full() - runes -> runes_regenerating() );

  // If the actor goes past the maximum number of regenerating runes, mark the waste start
  if ( runes -> waste_start < 0_ms && runes -> runes_full() > MAX_REGENERATING_RUNES )
  {
    runes -> dk -> sim -> print_debug( "{} rune waste, waste started, n_full_runes={}",
        runes -> dk -> name(), runes -> runes_full() );
    runes -> waste_start = runes -> dk -> sim -> current_time();
  }
}

inline void rune_t::start_regenerating()
{
  assert( event == nullptr );
  state = STATE_REGENERATING;
  regen_start = runes -> dk -> sim -> current_time();

  // Create a new regen event with proper parameters
  event = rune_event_t::create( runes -> dk -> sim )
    -> rune( *this )
    -> ptr( event )
    -> coefficient( runes -> dk -> rune_regen_coefficient() )
    -> schedule( timespan_t::from_seconds( RUNE_REGEN_BASE ) );
}

inline void rune_t::adjust_regen_event( timespan_t adjustment )
{
  if ( ! event )
  {
    return;
  }

  auto new_remains = event -> remains() + adjustment;

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
      event = rune_event_t::create( runes -> dk -> sim )
        -> rune( *this )
        -> ptr( event )
        -> coefficient( event -> coefficient() )
        -> schedule( new_remains, false ); // Note, sheduled using new_remains and no coefficient

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
    event -> reschedule( new_remains );
  }
}

namespace pets {

// ==========================================================================
// Generic DK pet
// ==========================================================================

struct death_knight_pet_t : public pet_t
{
  bool use_auto_attack, precombat_spawn, affected_by_commander_of_the_dead, guardian;
  timespan_t precombat_spawn_adjust;
  util::string_view pet_name;
  action_t* proxy_action;

  death_knight_pet_t( death_knight_t* player, util::string_view name, bool guardian = true, bool auto_attack = true, bool dynamic = true ) :
    pet_t( player -> sim, player, name, guardian, dynamic ), use_auto_attack( auto_attack ),
    precombat_spawn( false ), precombat_spawn_adjust( 0_s ),
    affected_by_commander_of_the_dead( false ), guardian( guardian ), pet_name( name ), proxy_action( nullptr )
  {
    if ( auto_attack )
    {
      main_hand_weapon.type = WEAPON_BEAST;
    }

    if ( pet_name == "army_ghoul" )
    {
      proxy_action = dk()->find_action( "army_of_the_dead" );
    }
    if ( pet_name == "apoc_ghoul" )
    {
      proxy_action = dk()->find_action( "apocalypse" );
    }
    if ( pet_name == "ghoul" )
    {
      proxy_action = dk()->find_action( "raise_dead" );
    }
  }

  double composite_melee_speed() const override
  {
    if ( use_delayed_pet_stat_updates )
      return current_pet_stats.composite_melee_haste;
    else
      return owner->cache.attack_haste();
  }

  death_knight_t* dk() const
  { return debug_cast<death_knight_t*>( owner ); }

  virtual attack_t* create_auto_attack()
  { return nullptr; }

  void apply_affecting_auras( action_t& action ) override
  {
    player_t::apply_affecting_auras( action );

    // Hack to apply BDK aura to DRW abilities
    // 2021-01-29: The bdk aura is currently set to 0,
    // Keep an eye on this in case blizz changes the value as double dipping may happen (both ingame and here)
    dk() -> apply_affecting_auras( action );
  }

  double composite_player_multiplier( school_e s ) const override
  {
    double m = pet_t::composite_player_multiplier( s );

    if ( dk()->specialization() == DEATH_KNIGHT_UNHOLY && dk()->buffs.commander_of_the_dead->up() &&
         affected_by_commander_of_the_dead )
    {
      m *= 1.0 + dk()->pet_spell.commander_of_the_dead->effectN( 1 ).percent();
    }

    if ( dk()->mastery.dreadblade->ok() )
    {
      m *= 1.0 + dk()->cache.mastery_value();
    }

    if ( dk()->talent.unholy.unholy_aura.ok() )
    {
      if ( guardian )
        m *= 1.0 + dk()->talent.unholy.unholy_aura->effectN( 4 ).percent();
      else  // Pets
        m *= 1.0 + dk()->talent.unholy.unholy_aura->effectN( 3 ).percent();
    }
    return m;
  }

  // Standard Death Knight pet actions

  struct ghoul_auto_attack_t : public melee_attack_t
  {
    action_t* proxy_action;
    ghoul_auto_attack_t( death_knight_pet_t* p, action_t* a ) :
      melee_attack_t( "auto_attack", p ), proxy_action( a )
    {
      assert( p -> main_hand_weapon.type != WEAPON_NONE );
      p -> main_hand_attack = p -> create_auto_attack();
      school = SCHOOL_PHYSICAL;
      trigger_gcd = 0_ms;
      if( !p -> dk() -> options.individual_pet_reporting )
      {
        auto proxy                = proxy_action;
        auto it                   = range::find( proxy->child_action, data().id(), &action_t::id );
        if ( it != proxy->child_action.end() )
          stats = ( *it )->stats;
        else
          proxy->add_child( this );
      }
    }

    void execute() override
    { player -> main_hand_attack -> schedule_execute(); }

    bool ready() override
    {
      if ( player -> is_moving() ) return false;
      return ( player -> main_hand_attack -> execute_event == nullptr );
    }
  };

  struct auto_attack_t : public melee_attack_t
  {
    auto_attack_t( death_knight_pet_t* p ) :
      melee_attack_t( "auto_attack", p )
    {
      assert( p -> main_hand_weapon.type != WEAPON_NONE );
      p -> main_hand_attack = p -> create_auto_attack();
      trigger_gcd = 0_ms;
    }

    void execute() override
    { player -> main_hand_attack -> schedule_execute(); }

    bool ready() override
    {
      if ( player -> is_moving() ) return false;
      return ( player -> main_hand_attack -> execute_event == nullptr );
    }
  };

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if( pet_name == "ghoul" || pet_name == "army_ghoul" || pet_name == "apoc_ghoul" )
    {
      if ( name == "auto_attack" ) return new ghoul_auto_attack_t( this, proxy_action );
    }
    else
    {
      if ( name == "auto_attack" ) return new auto_attack_t( this );
    }

    return pet_t::create_action( name, options_str );
  }

  void init_action_list() override
  {
    action_priority_list_t* def = get_action_priority_list( "default" );
    if ( use_auto_attack )
      def -> add_action( "auto_attack" );

    pet_t::init_action_list();
  }
};

// ==========================================================================
// Base Death Knight Pet Action
// ==========================================================================

template <typename T_PET, typename T_ACTION>
struct pet_action_t : public T_ACTION
{
  pet_action_t( T_PET* p, util::string_view name, const spell_data_t* spell = spell_data_t::nil() ) :
    T_ACTION( name, p, spell )
  {
    this -> special = this -> may_crit = true;
  }

  T_PET* pet() const
  { return debug_cast<T_PET*>( this -> player ); }

  death_knight_t* dk() const
  { return debug_cast<death_knight_t*>( debug_cast<pet_t*>( this -> player ) -> owner ); }

  void init() override
  {
    T_ACTION::init();

    // Merge stats for pets sharing the same name
    if ( ! this -> player -> sim -> report_pets_separately )
    {
      auto it = range::find_if( dk() -> pet_list, [ this ]( pet_t* pet ) {
        return this -> player -> name_str == pet -> name_str;
      } );

      if ( it != dk() -> pet_list.end() && this -> player != *it )
      {
        this -> stats = ( *it ) -> get_stats( this -> name(), this );
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
  bool triggers_runeforge_apocalypse;

  pet_melee_attack_t( T_PET* p, util::string_view name, const spell_data_t* spell = spell_data_t::nil() ) :
    pet_action_t<T_PET, melee_attack_t>( p, name, spell ),
    triggers_runeforge_apocalypse( false )
  {
    if ( this -> school == SCHOOL_NONE )
    {
      this -> school = SCHOOL_PHYSICAL;
      this -> stats -> school = SCHOOL_PHYSICAL;
    }
  }

  void init() override
  {
    pet_action_t<T_PET, melee_attack_t>::init();

    // Default to a 1s gcd
    if ( ! this -> background && this -> trigger_gcd == 0_ms )
    {
      this -> trigger_gcd = 1_s;
    }
  }

  // Apocalypse debuffs only trigger on the main target
  void execute() override
  {
    pet_action_t<T_PET, melee_attack_t>::execute();

    if ( triggers_runeforge_apocalypse && this -> dk() -> runeforge.rune_of_apocalypse && this -> hit_any_target )
    {
      int n = static_cast<int>( this -> pet() -> rng().range( 0, runeforge_apocalypse::MAX ) );

      death_knight_td_t* td = this -> dk() -> get_target_data( this -> target );

      switch ( n )
      {
      case runeforge_apocalypse::DEATH:
        td -> debuff.apocalypse_death -> trigger();
        break;
      case runeforge_apocalypse::FAMINE:
        td -> debuff.apocalypse_famine -> trigger();
        break;
      case runeforge_apocalypse::PESTILENCE:
        this -> dk() -> active_spells.runeforge_pestilence -> execute_on_target( this -> target );
        break;
      case runeforge_apocalypse::WAR:
        td -> debuff.apocalypse_war -> trigger();
        break;
      }
    }
  }
};

// ==========================================================================
// Base Death Knight Pet Spell
// ==========================================================================

template <typename T_PET>
struct pet_spell_t : public pet_action_t<T_PET, spell_t>
{
  pet_spell_t( T_PET* p, util::string_view name, const spell_data_t* spell = spell_data_t::nil() ) :
    pet_action_t<T_PET, spell_t>( p, name, spell )
  {
    this -> tick_may_crit = true;
    this -> hasted_ticks  = false;
  }
};

// ==========================================================================
// Specialized Death Knight Pet Actions
// ==========================================================================

// Generic auto attack for meleeing pets
template <typename T>
struct auto_attack_melee_t : public pet_melee_attack_t<T>
{
  auto_attack_melee_t( T* p, util::string_view name = "auto_attack" ) :
    pet_melee_attack_t<T>( p, name )
  {
    this -> background = this -> repeating = true;
    this -> special = false;
    this -> weapon = &( p -> main_hand_weapon );
    this -> weapon_multiplier = 1.0;
    this -> base_execute_time = this -> weapon -> swing_time;
  }

  void execute() override
  {
    // If we're casting, we should clip a swing
    if ( this -> player -> executing )
      this -> schedule_execute();
    else
      pet_melee_attack_t<T>::execute();
  }
};

// ==========================================================================
// Generic Unholy energy ghoul (main pet and Army/Apoc ghouls)
// ==========================================================================

struct base_ghoul_pet_t : public death_knight_pet_t
{
  base_ghoul_pet_t( death_knight_t* owner, util::string_view name, bool guardian = false, bool dynamic = true ) :
    death_knight_pet_t( owner, name, guardian, true, dynamic )
  {
    main_hand_weapon.swing_time = 2.0_s;
  }

  attack_t* create_auto_attack() override
  { return new auto_attack_melee_t<base_ghoul_pet_t>( this ); }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    resources.base[ RESOURCE_ENERGY ] = 100;
    resources.base_regen_per_second[ RESOURCE_ENERGY ]  = 10;
  }

  void arise() override
  {
    death_knight_pet_t::arise();
    timespan_t duration = dk() -> pet_spell.pet_stun -> duration();
    if ( precombat_spawn_adjust > 0_s && precombat_spawn )
    {
      duration = duration - precombat_spawn_adjust;
      buffs.stunned -> trigger( duration );
      stun();
    }
    else if( !precombat_spawn )
    {
      buffs.stunned -> trigger( duration );
      stun();
    }
  }

  resource_e primary_resource() const override
  { return RESOURCE_ENERGY; }

  timespan_t available() const override
  {
    double energy = resources.current[ RESOURCE_ENERGY ];
    timespan_t time_to_next = timespan_t::from_seconds( ( 40 - energy ) / resource_regen_per_second( RESOURCE_ENERGY ) );

    // Cheapest Ability need 40 Energy
    if ( energy > 40 )
    {
      return 100_ms;
    }
    else
    {
      return std::max ( time_to_next, 100_ms );
    }
  }
};

// ===============================================================================
// Raise Dead ghoul (both temporary blood/frost ghouls and permanent unholy ghoul)
// ===============================================================================

struct ghoul_pet_t : public base_ghoul_pet_t
{
  cooldown_t* gnaw_cd; // shared cd between gnaw/monstrous_blow
  gain_t* dark_transformation_gain;
  buff_t* vile_infusion;
  buff_t* ghoulish_frenzy;

  // Generic Dark Transformation pet ability
  struct dt_melee_ability_t : public pet_melee_attack_t<ghoul_pet_t>
  {
    bool usable_in_dt;
    bool triggers_infected_claws;

    dt_melee_ability_t(ghoul_pet_t* p, util::string_view name, const spell_data_t* spell = spell_data_t::nil(),
        bool usable_in_dt = true) :
        pet_melee_attack_t(p, name, spell),
        usable_in_dt(usable_in_dt),
        triggers_infected_claws(false)
    { }

    void impact( action_state_t* state ) override
    {
      pet_melee_attack_t<ghoul_pet_t>::impact( state );

      if ( triggers_infected_claws && dk() -> talent.unholy.infected_claws.ok() &&
           rng().roll( dk() -> talent.unholy.infected_claws -> effectN( 1 ).percent() ) )
      {
        dk() -> trigger_festering_wound( state, 1, dk() -> procs.fw_infected_claws );
      }
    }

    bool ready() override
    {
      if ( usable_in_dt != ( dk() -> specialization()  == DEATH_KNIGHT_UNHOLY && dk() -> buffs.dark_transformation -> check() > 0 ) )
      {
        return false;
      }

      return pet_melee_attack_t<ghoul_pet_t>::ready();
    }
  };

  struct claw_t final : public dt_melee_ability_t
  {
    claw_t( ghoul_pet_t* p, util::string_view options_str ) :
      dt_melee_ability_t( p, "claw", p -> dk() -> pet_spell.ghoul_claw, false )
    {
      parse_options( options_str );
      triggers_infected_claws = triggers_runeforge_apocalypse = true;
      if( !p -> dk() -> options.individual_pet_reporting )
      {
        auto proxy = dk()->find_action("raise_dead");
        auto it = range::find(proxy->child_action, data().id(), &action_t::id);
        if (it != proxy->child_action.end())
          stats = (*it)->stats;
        else
          proxy->add_child(this);
      }
    }
  };

  struct sweeping_claws_t final : public dt_melee_ability_t
  {
    sweeping_claws_t( ghoul_pet_t* p, util::string_view options_str ) :
      dt_melee_ability_t( p, "sweeping_claws", p -> dk() -> pet_spell.sweeping_claws, true )
    {
      parse_options( options_str );
      aoe = -1;
      triggers_infected_claws = triggers_runeforge_apocalypse = true;
      if( !p -> dk() -> options.individual_pet_reporting )
      {
        auto proxy = dk()->find_action( "raise_dead" );
        auto it    = range::find( proxy->child_action, data().id(), &action_t::id );
        if ( it != proxy->child_action.end() )
          stats = ( *it )->stats;
        else
          proxy->add_child( this );
      }
    }
  };

  struct gnaw_t final : public dt_melee_ability_t
  {
    gnaw_t( ghoul_pet_t* p, util::string_view options_str ) :
      dt_melee_ability_t( p, "gnaw", p -> dk() -> pet_spell.gnaw, false )
    {
      parse_options( options_str );
      cooldown = p -> get_cooldown( "gnaw" );
      if( !p -> dk() -> options.individual_pet_reporting )
      {
        auto proxy = dk()->find_action( "raise_dead" );
        auto it    = range::find( proxy->child_action, data().id(), &action_t::id );
        if ( it != proxy->child_action.end() )
          stats = ( *it )->stats;
        else
          proxy->add_child( this );
      }
    }
  };

  struct monstrous_blow_t final : public dt_melee_ability_t
  {
    monstrous_blow_t( ghoul_pet_t* p, util::string_view options_str ):
      dt_melee_ability_t( p, "monstrous_blow", p -> dk() -> pet_spell.monstrous_blow, true )
    {
      parse_options( options_str );
      cooldown = p -> get_cooldown( "gnaw" );
      if( !p -> dk() -> options.individual_pet_reporting )
      {
        auto proxy = dk()->find_action( "raise_dead" );
        auto it    = range::find( proxy->child_action, data().id(), &action_t::id );
        if ( it != proxy->child_action.end() )
          stats = ( *it )->stats;
        else
          proxy->add_child( this );
      }
    }
  };

  struct ghoul_melee_t final : public auto_attack_melee_t<ghoul_pet_t>
  {
    ghoul_melee_t( ghoul_pet_t* p, util::string_view name = "auto_attack" ) :
      auto_attack_melee_t<ghoul_pet_t>( p, name )
    { }

    void impact( action_state_t* state ) override
    {
      auto_attack_melee_t::impact( state );

      if ( result_is_hit( state -> result ) )
      {
        if ( dk() -> sets -> has_set_bonus( DEATH_KNIGHT_UNHOLY, T29, B4 ) )
        {
          double chance = dk() -> sets -> set( DEATH_KNIGHT_UNHOLY, T29, B4 ) -> effectN( 1 ).percent();

          if ( dk() -> pets.ghoul_pet -> vile_infusion -> up() )
          {
            chance = dk() -> sets -> set( DEATH_KNIGHT_UNHOLY, T29, B4 ) -> effectN( 2 ).percent();
          }

          if ( dk() -> rng().roll( chance ) )
          {
            dk() -> buffs.ghoulish_infusion -> trigger();
          }
        }
      }
    }
  };

  ghoul_pet_t( death_knight_t* owner, bool guardian ) :
    base_ghoul_pet_t( owner, "ghoul" , guardian )
  {
    gnaw_cd = get_cooldown( "gnaw" );
    gnaw_cd -> duration = owner -> pet_spell.gnaw -> cooldown();
    if ( owner -> talent.unholy.raise_dead.ok() )
    {
      precombat_spawn = true;
      if( !owner -> talent.sacrificial_pact.ok() )
      {
        dynamic = false;
      }
    }
  }

  attack_t* create_auto_attack() override
  { return new ghoul_melee_t( this ); }


  double composite_player_multiplier( school_e school ) const override
  {
    double m = base_ghoul_pet_t::composite_player_multiplier( school );

    if( dk() -> specialization()  == DEATH_KNIGHT_UNHOLY )
    {
      m *= 1.0 + dk() -> buffs.dark_transformation -> value();

      m *= 1.0 + ( ghoulish_frenzy -> value() / 100 ) ;

      m *= 1.0 + vile_infusion -> value();
    }

    return m;
  }

  double composite_player_target_multiplier( player_t* target, school_e s ) const override
  {
    double m = base_ghoul_pet_t::composite_player_target_multiplier( target, s );

    // 2020-12-11: Seems to be increasing the player's damage as well as the main ghoul, but not other pets'
    // Does not use a whitelist, affects all damage sources
    if ( auto td = dk() -> find_target_data( target ) )
    {
      m *= 1.0 + td -> debuff.apocalypse_war -> check_value();
    }

    return m;
  }

  double composite_melee_speed() const override
  {
    double haste = base_ghoul_pet_t::composite_melee_speed();

    // The default value stores the %damage increase

    if ( ghoulish_frenzy -> up() )
      haste *= 1.0 / ( 1.0 + ghoulish_frenzy -> data().effectN( 2 ).percent() );

    if ( vile_infusion -> up() )
      haste *= 1.0 / ( 1.0 + vile_infusion -> data().effectN( 2 ).percent() );

    return haste;
  }

  void init_base_stats() override
  {
    base_ghoul_pet_t::init_base_stats();

    // Note: for some dumb reason, WCL has the ghoul's AP and SP swapped
    // Running a script ingame shows the correct values
    owner_coeff.ap_from_ap = 0.594;
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
    if( dk() -> talent.unholy.dark_transformation.ok() )
    {
      def -> add_action( "sweeping_claws" );
      def -> add_action( "claw,if=energy>70" );
      def -> add_action( "monstrous_blow" );
      def -> add_action( "Gnaw" );
    }
    else
    {
      def -> add_action( "claw,if=energy>70" );
      def -> add_action( "Gnaw" );
    }
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "claw"           ) return new           claw_t( this, options_str );
    if ( name == "gnaw"           ) return new           gnaw_t( this, options_str );
    if ( name == "sweeping_claws" ) return new sweeping_claws_t( this, options_str );
    if ( name == "monstrous_blow" ) return new monstrous_blow_t( this, options_str );

    return base_ghoul_pet_t::create_action( name, options_str );
  }

  void create_buffs() override
  {
    base_ghoul_pet_t::create_buffs();
	  
    ghoulish_frenzy = make_buff( this, "ghoulish_frenzy", dk() -> pet_spell.ghoulish_frenzy )
      -> set_default_value_from_effect( 1 )
      -> set_duration( 0_s );

    vile_infusion = make_buff( this, "vile_infusion", dk() -> pet_spell.vile_infusion )
      -> set_duration( dk() -> pet_spell.vile_infusion -> duration() )
      -> set_default_value_from_effect( 1 )
      -> set_cooldown( dk() -> sets -> set(DEATH_KNIGHT_UNHOLY, T29, B2 ) -> internal_cooldown() )
      -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
      -> add_invalidate( CACHE_ATTACK_SPEED );
  }
};

// ==========================================================================
// Army of the Dead and Apocalypse ghouls
// ==========================================================================

struct army_ghoul_pet_t : public base_ghoul_pet_t
{
  pet_spell_t<army_ghoul_pet_t>* ruptured_viscera;
  action_t* proxy_action;

  struct army_claw_t : public pet_melee_attack_t<army_ghoul_pet_t>
  {
    action_t* proxy_action;
    army_claw_t( army_ghoul_pet_t* p, action_t* a, util::string_view options_str ) :
      pet_melee_attack_t( p, "claw", p -> dk() -> pet_spell.army_claw ), proxy_action( a )
    {
      parse_options( options_str );
      if( !p -> dk() -> options.individual_pet_reporting )
      {
        auto proxy = proxy_action;
        auto it    = range::find( proxy->child_action, data().id(), &action_t::id );
        if ( it != proxy->child_action.end() )
          stats = ( *it )->stats;
        else
          proxy->add_child( this );
      }
    }
  };
  
  struct ruptured_viscera_t final : public pet_spell_t<army_ghoul_pet_t>
  {
    action_t* proxy_action;
    ruptured_viscera_t( army_ghoul_pet_t* p, action_t* a ) :
      pet_spell_t( p, "ruptured_viscera", p -> dk() -> pet_spell.ruptured_viscera ), proxy_action( a )
    {
      aoe = -1;
      background = true;
      if( !p -> dk() -> options.individual_pet_reporting )
      {
        auto proxy = proxy_action;
        auto it    = range::find( proxy->child_action, data().id(), &action_t::id );
        if ( it != proxy->child_action.end() )
          stats = ( *it )->stats;
        else
          proxy->add_child( this );
      }
    }

    void impact( action_state_t* s ) override
    {
      pet_spell_t::impact( s );
      if ( dk() -> rng().roll( dk() -> spell.ruptured_viscera_chance -> effectN( 1 ).percent() ) )
      {
        dk() -> trigger_festering_wound( s, 1, dk() -> procs.fw_ruptured_viscera );
      }
    }
  };

  army_ghoul_pet_t( death_knight_t* owner, util::string_view name = "army_ghoul" ) :
    base_ghoul_pet_t( owner, name, true ), proxy_action( nullptr )
  {
    affected_by_commander_of_the_dead = true;

    if( name == "army_ghoul" )
    {
      proxy_action = dk()->find_action( "army_of_the_dead" );
    }
    else
    {
      proxy_action = dk()->find_action( "apocalypse" );
    }
  }

  void init_base_stats() override
  {
    base_ghoul_pet_t::init_base_stats();

    // This three-decimal number was caused by a +6% hotfix slapped on the original 0.4 value
    owner_coeff.ap_from_ap = 0.4664;
  }

  void init_spells() override
  {
    base_ghoul_pet_t::init_spells();

    if ( dk()->talent.unholy.ruptured_viscera.ok() )
    {
      ruptured_viscera = new ruptured_viscera_t( this, proxy_action );
    }
  }

  void init_gains() override
  {
    base_ghoul_pet_t::init_gains();

    // Group Energy regen for the same pets together to reduce report clutter
    if ( ! dk() -> options.split_ghoul_regen && dk() -> find_pet( name_str ) )
    {
      this -> gains.resource_regen = dk() -> find_pet( name_str ) -> gains.resource_regen;
    }
  }

  void init_action_list() override
  {
    base_ghoul_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "claw" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "claw" ) return new army_claw_t( this, proxy_action, options_str );

    return base_ghoul_pet_t::create_action( name, options_str );
  }

  void dismiss( bool expired = false ) override
  {
    if ( !sim -> event_mgr.canceled && dk() -> talent.unholy.ruptured_viscera.ok() )
    {
      ruptured_viscera -> execute_on_target( target );
    }
	  
    pet_t::dismiss( expired );
  }
};

// ==========================================================================
// Gargoyle
// ==========================================================================
struct gargoyle_pet_t : public death_knight_pet_t
{
  buff_t* dark_empowerment;
  pet_spell_t<gargoyle_pet_t>* gargoyle_strike;
  action_t* proxy_action;

  gargoyle_pet_t( death_knight_t* owner, action_t* a ) :
    death_knight_pet_t( owner, "gargoyle", true, false ), dark_empowerment( nullptr ), gargoyle_strike( nullptr ), proxy_action( a )
  {
    resource_regeneration = regen_type::DISABLED;
    affected_by_commander_of_the_dead = true;
  }

  struct gargoyle_strike_t : public pet_spell_t<gargoyle_pet_t>
  {
    action_t* proxy_action;
    gargoyle_strike_t( gargoyle_pet_t* p, action_t* a ) :
      pet_spell_t( p, "gargoyle_strike", p -> dk() -> pet_spell.gargoyle_strike ), proxy_action( a )
    { 
      background = repeating = true;
      if( !p -> dk() -> options.individual_pet_reporting )
      {
        auto proxy = proxy_action;
        auto it    = range::find( proxy->child_action, data().id(), &action_t::id );
        if ( it != proxy->child_action.end() )
          stats = ( *it )->stats;
        else
          proxy->add_child( this );
      }
    }
  };

  void arise() override
  {
    death_knight_pet_t::arise();
    timespan_t duration = 2.8_s;
    buffs.stunned -> trigger( duration + rng().gauss( 200_ms, 0_ms ) );
    stun();
    reschedule_gargoyle();
  }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 1.0;
  }

  double composite_player_multiplier( school_e s ) const override
  {
    double m = death_knight_pet_t::composite_player_multiplier( s );

    m *= 1.0 + dark_empowerment -> stack_value();

    return m;
  }

  void create_buffs() override
  {
    death_knight_pet_t::create_buffs();

    dark_empowerment = make_buff( this, "dark_empowerment", dk() -> pet_spell.dark_empowerment );
  }

  void create_actions()
  {
    death_knight_pet_t::create_actions();
    gargoyle_strike = new gargoyle_strike_t( this, proxy_action );
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

  void schedule_ready( timespan_t /* delta_time */, bool /* waiting */ )
  {
    reschedule_gargoyle();
  }

  // Function that increases the gargoyle's dark empowerment buff value based on RP spent
  void increase_power ( double rp_spent )
  {
    if ( is_sleeping() )
    {
      return;
    }

    double increase = rp_spent * dark_empowerment -> data().effectN( 1 ).percent() / dk() -> talent.unholy.summon_gargoyle -> effectN( 4 ).base_value();

    if ( ! dark_empowerment -> check() )
    {
      dark_empowerment -> trigger( 1, increase );
    }
    else
    {
      sim -> print_debug( "{} increasing shadow_empowerment power by {}", name(), increase );
      dark_empowerment -> current_value += increase;
    }
  }
};

// ==========================================================================
// Risen Skulker (All Will Serve talent)
// ==========================================================================

struct risen_skulker_pet_t : public death_knight_pet_t
{
  pet_spell_t<risen_skulker_pet_t>* skulker_shot;
  risen_skulker_pet_t( death_knight_t* owner ) :
    death_knight_pet_t( owner, "risen_skulker", true, false, false ), skulker_shot( nullptr )
  {
    resource_regeneration = regen_type::DISABLED;
    main_hand_weapon.type = WEAPON_BEAST_RANGED;
    main_hand_weapon.swing_time = 2.7_s;
  }

  struct skulker_shot_t : public pet_spell_t<risen_skulker_pet_t>
  {
    skulker_shot_t( risen_skulker_pet_t* p ) :
      pet_spell_t<risen_skulker_pet_t>( p, "skulker_shot", p -> dk() -> pet_spell.skulker_shot )
    {
      weapon = &( p -> main_hand_weapon );
      background = true;
      // For some reason, Risen Skulker deals double damage to its main target, and normal damage to the other targets
      base_multiplier *= 2.0;
      aoe = -1;
      base_aoe_multiplier = 0.5;
      repeating = true;
      if( !p -> dk() -> options.individual_pet_reporting )
      {
        auto proxy = dk()->find_action( "raise_dead" );
        auto it    = range::find( proxy->child_action, data().id(), &action_t::id );
        if ( it != proxy->child_action.end() )
          stats = ( *it )->stats;
        else
          proxy->add_child( this );
      }
    }
  };

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 1.0;
  }

  void create_actions()
  {
    death_knight_pet_t::create_actions();
    skulker_shot = new skulker_shot_t( this );
  }

  void reschedule_skulker()
  {
    if ( executing || is_sleeping() || buffs.movement->check() || buffs.stunned->check() )
      return;

    else
    {
      skulker_shot->set_target( dk()->target );
      skulker_shot->schedule_execute();
    }
  }
  
  void arise() override
  {
    death_knight_pet_t::arise();
    reschedule_skulker();
  }

  void schedule_ready( timespan_t /* delta_time */, bool /* waiting */ )
  {
    reschedule_skulker();
  }
};

// ==========================================================================
// Dancing Rune Weapon
// ==========================================================================

struct dancing_rune_weapon_pet_t : public death_knight_pet_t
{
  target_specific_t<dot_t> blood_plague_dot;

  dot_t* get_blood_plague( player_t* target )
  {
    dot_t*& bp = blood_plague_dot[ target ];
    if ( ! bp )
    {
      bp = target -> get_dot( "blood_plague", this );
    }
    return bp;
  }

  template <typename T_ACTION>
  struct drw_action_t : public pet_action_t<dancing_rune_weapon_pet_t, T_ACTION>
  {
    struct affected_by_t
    {
      bool blood_plague;
    } affected_by;

    drw_action_t( dancing_rune_weapon_pet_t* p, util::string_view name, const spell_data_t* s ) :
      pet_action_t<dancing_rune_weapon_pet_t, T_ACTION>( p, name, s )
    {
      this -> background = true;
      this -> weapon = &( p -> main_hand_weapon );

      this -> affected_by.blood_plague = this -> data().affected_by( p -> dk() -> spell.blood_plague -> effectN( 4 ) );
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = pet_action_t<dancing_rune_weapon_pet_t, T_ACTION>::composite_target_multiplier( target );

      const death_knight_td_t* td = this -> dk() -> find_target_data( target );

      if ( td && this->affected_by.blood_plague && td -> dot.blood_plague -> is_ticking() )
      {
        m *= 1.0 + this -> dk() -> talent.blood.coagulopathy -> effectN( 1 ).percent();
      }

      return m;
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
    blood_plague_t( dancing_rune_weapon_pet_t* p ) :
      drw_action_t<spell_t>( p, "blood_plague", p -> dk() -> spell.blood_plague )
    {
      // DRW usually behaves the same regardless of talents, but BP ticks are affected by rapid decomposition
      this -> base_tick_time *= 1.0 + dk() -> talent.blood.rapid_decomposition -> effectN( 1 ).percent();
      snapshot_coagulopathy = 0.0;
    }

    double composite_ta_multiplier( const action_state_t* state ) const override
    {
      double m = drw_action_t::composite_ta_multiplier( state );

      // DRW snapshots the coag stacks when the weapon demises, so if DRW is down, use the snapshot value
      if ( dk() -> buffs.dancing_rune_weapon -> up() )
        m *= 1.0 + dk() -> buffs.coagulopathy -> stack_value();
      else
        m *= 1.0 + snapshot_coagulopathy;

      return m;
    }

    void tick( dot_t* dot ) override
    {
      drw_action_t::tick( dot );

      // Snapshot damage amp, as it's constant if drw demises
      if( dk() -> buffs.dancing_rune_weapon -> up() )
        snapshot_coagulopathy = dk() -> buffs.coagulopathy -> stack_value();
    }

    void execute() override
    {
      drw_action_t::execute();
      snapshot_coagulopathy = 0.0;
    }
  };

  struct blood_boil_t: public drw_action_t<spell_t>
  {
    blood_boil_t( dancing_rune_weapon_pet_t* p ) :
      drw_action_t<spell_t>( p, "blood_boil", p -> dk() -> talent.blood.blood_boil )
    {
      aoe = -1;
      cooldown -> duration = 0_ms;
      this -> impact_action = p -> ability.blood_plague;
      if ( dk() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T30, B2 ) )
      {
        this -> base_multiplier *= 1.0 + dk() -> sets -> set( DEATH_KNIGHT_BLOOD, T30, B2 ) -> effectN(1).percent();
      }
    }
  };

  struct consumption_t: public drw_action_t<melee_attack_t>
  {
    consumption_t( dancing_rune_weapon_pet_t* p ) :
      drw_action_t( p, "consumption", p -> dk() -> talent.blood.consumption )
    {
      aoe = -1;
      reduced_aoe_targets = data().effectN( 3 ).base_value();
    }
  };

  struct deaths_caress_t : public drw_action_t<spell_t>
  {
    int stack_gain;
    deaths_caress_t( dancing_rune_weapon_pet_t* p ) :
      drw_action_t( p, "deaths_caress", p -> dk() -> talent.blood.deaths_caress ),
      stack_gain( as<int>(data().effectN( 3 ).base_value()) )

    {
      this -> impact_action = p -> ability.blood_plague;
    }

    void impact( action_state_t* state ) override
    {
      drw_action_t::impact( state );

      dk() -> buffs.bone_shield -> trigger( stack_gain );
    }
  };

  struct death_strike_t : public drw_action_t<melee_attack_t>
  {
    death_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_action_t<melee_attack_t>( p, "death_strike", p -> dk() -> talent.death_strike )
    { }

    double action_multiplier() const override
    {
      double m = drw_action_t::action_multiplier();

      // DRW isn't usually affected by talents, and doesn't proc hemostasis, yet its death strike damage is increased by hemostasis
      // https://github.com/SimCMinMax/WoW-BugTracker/issues/241
      m *= 1.0 + dk() -> buffs.hemostasis -> stack_value();

      // DRW DS damage is buffed by player heartrend buff
      m *= 1.0 + dk() -> buffs.heartrend -> stack_value();

      return m;
    }
  };

  struct heart_strike_t : public drw_action_t<melee_attack_t>
  {
    double blood_strike_rp_generation;

    heart_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_action_t<melee_attack_t>( p, "heart_strike", p -> dk() -> pet_spell.drw_heart_strike ),
      // DRW is still using an old spell called "Blood Strike" for the 5 additional RP generation on Heart Strike
      blood_strike_rp_generation( dk() -> pet_spell.drw_heart_strike_rp_gen -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) )
    {
      this -> base_multiplier *= 1.0 + dk() -> talent.blood.improved_heart_strike -> effectN( 1 ).percent();
      if ( dk() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T30, B2 ) )
      {
        this -> base_multiplier *= 1.0 + dk() -> sets -> set( DEATH_KNIGHT_BLOOD, T30, B2 ) -> effectN( 1 ).percent();
      }
    }

    int n_targets() const override
    { return dk() -> in_death_and_decay() ? aoe + as<int>( dk() -> talent.cleaving_strikes -> effectN( 2 ).base_value() ) : aoe; }


    void execute() override
    {
      drw_action_t::execute();

      dk() -> resource_gain( RESOURCE_RUNIC_POWER, blood_strike_rp_generation, dk() -> gains.drw_heart_strike, this );
    }
  };

  struct marrowrend_t : public drw_action_t<melee_attack_t>
  {
    int stack_gain;
    marrowrend_t( dancing_rune_weapon_pet_t* p ) :
      drw_action_t<melee_attack_t>( p, "marrowrend", p -> dk() -> talent.blood.marrowrend ),
      stack_gain( as<int>(data().effectN( 3 ).base_value()) )
    { }

    void impact( action_state_t* state ) override
    {
      drw_action_t::impact( state );

      dk() -> buffs.bone_shield -> trigger( stack_gain );
    }
  };

  struct soul_reaper_execute_t : public drw_action_t<spell_t>
  {
      soul_reaper_execute_t(util::string_view name, dancing_rune_weapon_pet_t* p) :
          drw_action_t<spell_t>(p, name, p -> dk() -> spell.soul_reaper_execute )
    {
      background = true;
    }
  };

struct soul_reaper_t : public drw_action_t<melee_attack_t>
{
  soul_reaper_t( dancing_rune_weapon_pet_t* p ) :
    drw_action_t<melee_attack_t>( p, "soul_reaper", p -> dk() -> talent.soul_reaper ),
    soul_reaper_execute( get_action<soul_reaper_execute_t>( "soul_reaper_execute", p ) )
  {
    add_child( soul_reaper_execute );
    hasted_ticks = false;
    dot_behavior = DOT_EXTEND;
  }

  void tick( dot_t* dot ) override
  {
    // SR execute only fires if DRW is still alive
    if ( dk() -> buffs.dancing_rune_weapon -> up() &&
          dot -> target -> health_percentage() < data().effectN( 3 ).base_value() )
      soul_reaper_execute -> execute_on_target ( dot -> target );
  }
private:
    propagate_const<action_t*> soul_reaper_execute;
};

  struct abilities_t
  {
    drw_action_t<spell_t>*  blood_plague;
    drw_action_t<spell_t>*  blood_boil;
    drw_action_t<spell_t>*  deaths_caress;
    drw_action_t<melee_attack_t>* death_strike;
    drw_action_t<melee_attack_t>* heart_strike;
    drw_action_t<melee_attack_t>* marrowrend;
    drw_action_t<melee_attack_t>* soul_reaper;
    drw_action_t<melee_attack_t>* consumption;
  } ability;

  dancing_rune_weapon_pet_t( death_knight_t* owner, util::string_view drw_name ) :
    death_knight_pet_t( owner, drw_name, true, true ),
    blood_plague_dot( false ),
    ability()
  {
    // The pet wields the same weapon type as its owner for spells with weapon requirements
    main_hand_weapon.type       = owner -> main_hand_weapon.type;
    main_hand_weapon.swing_time = 3.5_s;

    owner_coeff.ap_from_ap = 1 / 3.0;
    resource_regeneration = regen_type::DISABLED;
  }


  void init_spells() override
  {
    death_knight_pet_t::init_spells();

    ability.blood_plague  = new blood_plague_t ( this );
    ability.blood_boil    = new blood_boil_t   ( this );
    ability.deaths_caress = new deaths_caress_t( this );
    ability.death_strike  = new death_strike_t ( this );
    ability.heart_strike  = new heart_strike_t ( this );
    ability.marrowrend    = new marrowrend_t   ( this );
    ability.soul_reaper   = new soul_reaper_t  ( this );
    ability.consumption   = new consumption_t  ( this );
  }

  void arise() override
  {
    death_knight_pet_t::arise();
    dk() -> buffs.dancing_rune_weapon -> trigger();
  }

  void demise() override
  {
    death_knight_pet_t::demise();
    dk() -> buffs.dancing_rune_weapon -> expire();
  }

  attack_t* create_auto_attack() override
  { return new auto_attack_melee_t<dancing_rune_weapon_pet_t>( this ); }
};

// ==========================================================================
// Bloodworms
// ==========================================================================

struct bloodworm_pet_t : public death_knight_pet_t
{
  bloodworm_pet_t( death_knight_t* owner ) :
    death_knight_pet_t( owner, "bloodworm", true, true )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = 1.4_s;

    owner_coeff.ap_from_ap = 0.25;
    resource_regeneration = regen_type::DISABLED;
  }

  attack_t* create_auto_attack() override
  { return new auto_attack_melee_t<bloodworm_pet_t>( this ); }
};

// ==========================================================================
// Magus of the Dead (Talent)
// ==========================================================================

struct magus_pet_t : public death_knight_pet_t
{
  action_t* proxy_action;
  struct magus_td_t : public actor_target_data_t
  {
    buff_t* frostbolt_debuff;

    magus_td_t( player_t* target, magus_pet_t* p ) :
      actor_target_data_t( target, p )
    {
      frostbolt_debuff = make_buff( *this, "frostbolt_magus_of_the_dead", p -> dk() -> pet_spell.frostbolt );
    }
  };

  target_specific_t<magus_td_t> target_data;

  const magus_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  magus_td_t* get_target_data( player_t* target ) const override
  {
    magus_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new magus_td_t( target, const_cast<magus_pet_t*>( this ) );
    }
    return td;
  }

  struct magus_spell_t : public pet_spell_t<magus_pet_t>
  {
    magus_spell_t( magus_pet_t* p, util::string_view name , const spell_data_t* spell, util::string_view options_str ) :
      pet_spell_t( p, name, spell )
    {
      parse_options( options_str );
    }

    // There's a 1 energy cost in spelldata but it might as well be ignored
    double cost() const override
    { return 0; }
  };

  struct frostbolt_magus_t final : public magus_spell_t
  {
    action_t* proxy_action;
    frostbolt_magus_t( magus_pet_t* p, action_t* a, util::string_view options_str ) :
      magus_spell_t( p, "frostbolt", p -> dk() -> pet_spell.frostbolt, options_str ), proxy_action( a )
    {
      // If the target is immune to slows, frostbolt seems to be used at most every 6 seconds
      cooldown -> duration = dk() -> pet_spell.frostbolt -> duration();
      if ( !p->dk()->options.individual_pet_reporting )
      {
        auto proxy = proxy_action;
        auto it    = range::find( proxy->child_action, data().id(), &action_t::id );
        if ( it != proxy->child_action.end() )
          stats = ( *it )->stats;
        else
          proxy->add_child( this );
      }
    }

    // Frostbolt applies a slowing debuff on non-boss targets
    // This is needed because Frostbolt won't ever target an enemy affected by the debuff
    void impact( action_state_t* state ) override
    {
      magus_spell_t::impact( state );

      if ( result_is_hit( state -> result ) && state -> target -> type == ENEMY_ADD )
      {
        pet() -> get_target_data( state -> target ) -> frostbolt_debuff -> trigger();
      }
    }
  };

  struct shadow_bolt_magus_t final : public magus_spell_t
  {
    action_t* proxy_action;
    shadow_bolt_magus_t( magus_pet_t* p, action_t* a, util::string_view options_str ) :
      magus_spell_t( p, "shadow_bolt", p -> dk() -> pet_spell.shadow_bolt, options_str ), proxy_action( a )
    {
      if ( !p->dk()->options.individual_pet_reporting )
      {
        auto proxy = proxy_action;
        auto it    = range::find( proxy->child_action, data().id(), &action_t::id );
        if ( it != proxy->child_action.end() )
          stats = ( *it )->stats;
        else
          proxy->add_child( this );
      }
    }
  };

  magus_pet_t( death_knight_t* owner, util::string_view name = "army_magus" ) :
    death_knight_pet_t( owner, name, true, false ), proxy_action( nullptr )
  {
    if( name == "army_magus" )
    {
      proxy_action = dk()->find_action( "army_of_the_dead" );
    }
    else
    {
      proxy_action = dk()->find_action( "apocalypse" );
    }

    resource_regeneration = regen_type::DISABLED;
    affected_by_commander_of_the_dead = true;
  }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 0.4;
    // Looks like Magus' AP coefficient is the same as the pet ghouls'
    // Including the +6% buff applied before magus was even a thing
    owner_coeff.ap_from_ap *= 1.06;
  }

  void init_action_list() override
  {
    death_knight_pet_t::init_action_list();

    // Default "auto-pilot" pet APL (if everything is left on auto-cast
    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "frostbolt" );
    def -> add_action( "shadow_bolt" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "frostbolt" ) return new frostbolt_magus_t( this, proxy_action, options_str );
    if ( name == "shadow_bolt" ) return new shadow_bolt_magus_t( this, proxy_action, options_str );

    return death_knight_pet_t::create_action( name, options_str );
  }
};

} // namespace pets

namespace { // UNNAMED NAMESPACE

// Template for common death knight action code. See priest_action_t.
template <class Base>
struct death_knight_action_t : public Base
{
  using action_base_t = Base;
  using base_t = death_knight_action_t<Base>;

  gain_t* gain;

  bool hasted_gcd;

  struct affected_by_t
  {
    // Masteries
    bool frozen_heart, frozen_heart_periodic;
    bool dreadblade, dreadblade_periodic;
    // Other whitelists
    bool razorice;
    bool brittle;
    bool death_rot;
    bool tightening_grasp;
    bool virulent_plague;
    bool frost_fever;
    bool blood_plague;
    bool unholy_blight;
    bool war;
    bool sanguine_ground, sanguine_ground_periodic;
    /*
    Pre-emptively writing these in, they are likely to be changed to whitelists too
    bool ghoulish_frenzy;
    bool bonegrinder;
    bool bloodshot;
    */
    // Tier 29
    bool ghoulish_infusion;
    bool vigorous_lifeblood_4pc;
    // Tier 30
    bool lingering_chill;
  } affected_by;

  death_knight_action_t( util::string_view n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    action_base_t( n, p, s ), gain( nullptr ),
    hasted_gcd( false ),
    affected_by()
  {
    this -> may_crit   = true;
    this -> may_glance = false;

    // Death Knights have unique snowflake mechanism for RP energize. Base actions indicate the
    // amount as a negative value resource cost in spell data, so abuse that.
    if ( this -> base_costs[ RESOURCE_RUNIC_POWER ] < 0 )
    {
      this -> energize_type = action_energize::ON_CAST;
      this -> energize_resource = RESOURCE_RUNIC_POWER;

      double rp_gain = std::fabs( this -> base_costs[ RESOURCE_RUNIC_POWER ] );

      this -> energize_amount += rp_gain;
      this -> base_costs[ RESOURCE_RUNIC_POWER ] = 0;
    }
    // Always rely on custom handling via replenish_rune() for Rune energize
    if ( this -> energize_resource == RESOURCE_RUNE )
    {
      this -> energize_type = action_energize::NONE;
      this -> energize_resource = RESOURCE_NONE;
    }

    // Inits to false if the spec doesn't have that mastery
    this -> affected_by.frozen_heart = this -> data().affected_by( p -> mastery.frozen_heart -> effectN( 1 ) );
    this -> affected_by.frozen_heart_periodic = this -> data().affected_by( p -> mastery.frozen_heart -> effectN( 2 ) );
    this -> affected_by.dreadblade = this -> data().affected_by( p -> mastery.dreadblade -> effectN( 1 ) );
    this -> affected_by.dreadblade_periodic = this -> data().affected_by( p -> mastery.dreadblade -> effectN( 2 ) );

    this -> affected_by.razorice = this ->  data().affected_by( p -> spell.razorice_debuff -> effectN( 1 ) );
    this -> affected_by.brittle = this -> data().affected_by( p -> spell.brittle_debuff -> effectN( 1 ) );
    this -> affected_by.tightening_grasp = this -> data().affected_by( p -> spell.tightening_grasp_debuff -> effectN( 1 ) );
    this -> affected_by.death_rot = this -> data().affected_by( p -> spell.death_rot_debuff -> effectN( 1 ) );
    this -> affected_by.ghoulish_infusion = this -> data().affected_by( p -> spell.ghoulish_infusion -> effectN( 1 ) );
    this -> affected_by.vigorous_lifeblood_4pc = this -> data().affected_by( p -> spell.vigorous_lifeblood_4pc -> effectN( 1 ) );
    this -> affected_by.lingering_chill = this ->  data().affected_by( p -> spell.lingering_chill -> effectN( 1 ) );
    this -> affected_by.virulent_plague = this -> data().affected_by( p -> spell.virulent_plague -> effectN( 3 ) );
    this -> affected_by.frost_fever = this -> data().affected_by( p -> spell.frost_fever -> effectN( 2 ) );
    this -> affected_by.blood_plague = this -> data().affected_by( p -> spell.blood_plague -> effectN( 4 ) );
    this -> affected_by.unholy_blight = this -> data().affected_by( p -> spell.unholy_blight_dot -> effectN( 2 ) );
    this -> affected_by.war = this -> data().affected_by( p -> spell.apocalypse_war_debuff -> effectN( 1 ) );
    this -> affected_by.sanguine_ground = this -> data().affected_by( p -> spell.sanguine_ground -> effectN( 1 ) );
    this -> affected_by.sanguine_ground_periodic = this -> data().affected_by( p -> spell.sanguine_ground -> effectN( 3 ) );
    /*
    this -> affected_by.ghoulish_frenzy = this -> data().affected_by( p -> spell.ghoulish_frenzy_player -> effectN() );
    this -> affected_by.bonegrinder = this -> data().affected_by( p -> spell.bonegrinder_frost_buff -> effectN() );
    this -> affected_by.bloodshot = this -> data().affected_by( p -> spell.blood_shield -> effectN() );
    */

    // TODO July 19 2022
    // Spelldata for Might of the frozen wastes is still all sorts of jank.
    // When using a 2H, might of the frozen wastes rank 1 effect#2 buffs the direct damage, but not td
    if ( p -> main_hand_weapon.group() == WEAPON_2H )
    {
      if ( this -> data().affected_by( p -> spec.might_of_the_frozen_wastes -> effectN( 2 ) ) )
      {
        this -> base_dd_multiplier *= 1.0 + ( p -> spec.might_of_the_frozen_wastes -> effectN( 2 ).percent() );
      }
    }
  }

  std::string full_name() const
  {
    std::string n = action_base_t::data().name_cstr();
    return n.empty() ? action_base_t::name_str : n;
  }

  death_knight_t* p() const
  { return debug_cast< death_knight_t* >( this -> player ); }

  const death_knight_td_t* find_td( player_t* t ) const
  { return p() -> find_target_data( t ); }

  death_knight_td_t* get_td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual double runic_power_generation_multiplier( const action_state_t* /* s */ ) const
  {
    return 1.0;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = Base::composite_da_multiplier( state );

    if ( this -> affected_by.frozen_heart || this -> affected_by.dreadblade )
    {
      m *= 1.0 + p() -> cache.mastery_value();
    }

    if ( p() -> specialization() == DEATH_KNIGHT_UNHOLY && this -> affected_by.ghoulish_infusion && p() -> sets -> has_set_bonus( DEATH_KNIGHT_UNHOLY, T29, B4 ) && p() -> buffs.ghoulish_infusion -> up() )
    {
      m *= 1.0 + p() -> buffs.ghoulish_infusion -> value();
    }

    if ( p() -> specialization() == DEATH_KNIGHT_BLOOD && this -> affected_by.vigorous_lifeblood_4pc && p() -> buffs.vigorous_lifeblood_4pc -> up() )
    {
      m *= 1.0 + p() -> buffs.vigorous_lifeblood_4pc -> value();
    }

    if( p() -> specialization() == DEATH_KNIGHT_BLOOD && this -> affected_by.sanguine_ground && p() -> buffs.sanguine_ground -> check() )
    {
      m *= 1.0 + p() -> buffs.sanguine_ground -> check_value();
    }

    return m;
  }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = Base::composite_ta_multiplier( state );

    if ( this -> affected_by.frozen_heart_periodic || this -> affected_by.dreadblade_periodic )
    {
      m *= 1.0 + p() -> cache.mastery_value();
    }

    if ( p() -> specialization() == DEATH_KNIGHT_UNHOLY && this -> affected_by.ghoulish_infusion && p() -> sets -> has_set_bonus( DEATH_KNIGHT_UNHOLY, T29, B4 ) && p() -> buffs.ghoulish_infusion -> up() )
    {
      m *= 1.0 + p() -> buffs.ghoulish_infusion -> value();
    }

    if ( p() -> specialization() == DEATH_KNIGHT_BLOOD && this -> affected_by.vigorous_lifeblood_4pc && p() -> buffs.vigorous_lifeblood_4pc -> up() )
    {
      m *= 1.0 + p() -> buffs.vigorous_lifeblood_4pc -> value();
    }

    if( p() -> specialization() == DEATH_KNIGHT_BLOOD && this -> affected_by.sanguine_ground_periodic && p() -> buffs.sanguine_ground -> check() )
    {
      m *= 1.0 + p() -> buffs.sanguine_ground -> check_value();
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = action_base_t::composite_target_multiplier( target );

    const death_knight_td_t* td = get_td( target );

    if ( td && this -> affected_by.razorice )
    {
      m *= 1.0 + td -> debuff.razorice -> check_stack_value();
    }

    if ( td && this -> affected_by.brittle )
    {
      m *= 1.0 + td -> debuff.brittle -> check_stack_value();
    }

    if ( td && this -> affected_by.tightening_grasp )
    {
      m *= 1.0 + td -> debuff.tightening_grasp -> check_stack_value();
    }

    if ( td && this -> affected_by.death_rot )
    {
      m *= 1.0 + td -> debuff.death_rot -> check_stack_value();
    }

    if ( td && this->affected_by.blood_plague && td -> dot.blood_plague -> is_ticking() )
    {
      m *= 1.0 + p() -> talent.blood.coagulopathy -> effectN( 1 ).percent();
    }

    if ( td && this->affected_by.virulent_plague && td->dot.virulent_plague->is_ticking() &&
         p()->talent.unholy.morbidity->ok() )
    {
      m *= 1.0 + p()->talent.unholy.morbidity->effectN( 1 ).percent();
    }

    if ( td && this->affected_by.frost_fever && td->dot.frost_fever->is_ticking() &&
         p()->talent.unholy.morbidity->ok() )
    {
      m *= 1.0 + p()->talent.unholy.morbidity->effectN( 1 ).percent();
    }

    if ( td && this->affected_by.blood_plague && td->dot.blood_plague->is_ticking() &&
         p()->talent.unholy.morbidity->ok() )
    {
      m *= 1.0 + p()->talent.unholy.morbidity->effectN( 1 ).percent();
    }

    if ( td && this->affected_by.unholy_blight && td->dot.unholy_blight->is_ticking() &&
         p()->talent.unholy.morbidity->ok() )
    {
      m *= 1.0 + p()->talent.unholy.morbidity->effectN( 1 ).percent();
    }

    if ( td && this->affected_by.war && td->debuff.apocalypse_war->check() )
    {
      m *= 1.0 + td -> debuff.apocalypse_war -> check_stack_value();
    }

    return m;
  }

  double composite_target_crit_damage_bonus_multiplier( player_t* target ) const override
  {
    double m = action_base_t::composite_target_crit_damage_bonus_multiplier( target );

    const death_knight_td_t* td = get_td( target );

    if ( td && this -> affected_by.lingering_chill && td -> debuff.lingering_chill -> check() )
    {
      m *= 1.0 + td -> debuff.lingering_chill -> check_stack_value();
    }

    return m;
  }


  double composite_energize_amount( const action_state_t* s ) const override
  {
    double amount = action_base_t::composite_energize_amount( s );

    if ( this -> energize_resource_() == RESOURCE_RUNIC_POWER )
    {
      amount *= this -> runic_power_generation_multiplier( s );
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

    if ( this -> base_costs[ RESOURCE_RUNE ] || this -> base_costs[ RESOURCE_RUNIC_POWER ] )
    {
      gain = this -> player -> get_gain( util::inverse_tokenize( this -> name_str ) );
    }

    // As these are only setting a boolean, it is safe for them to be called twice, so there is no harm in leaving these here
    if ( this -> data().affected_by( p() -> spec.death_knight -> effectN( 1 ) ) )
    {
      this -> cooldown -> hasted = true;
    }

    if ( this -> data().affected_by( p() -> spec.death_knight -> effectN( 2 ) ) )
    {
      this -> gcd_type = gcd_haste_type::ATTACK_HASTE;
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
      base_gcd *= this -> composite_haste();
    }

    if ( base_gcd < this -> min_gcd )
    {
      base_gcd = this -> min_gcd;
    }

    return base_gcd;
  }

  void execute() override
  {
    Base::execute();

    // For non tank DK's, we proc the ability on CD, attached to thier own executes, to simulate it
    if ( p() -> talent.blood_draw.ok() && p() -> specialization() != DEATH_KNIGHT_BLOOD && p() -> active_spells.blood_draw -> ready() && p() -> in_combat)
      p() -> active_spells.blood_draw -> execute();
  }

  void update_ready( timespan_t cd ) override
  {
    Base::update_ready( cd );
  }
};

// ==========================================================================
// Death Knight Attack
// ==========================================================================

struct death_knight_melee_attack_t : public death_knight_action_t<melee_attack_t>
{
  bool triggers_icecap;

  death_knight_melee_attack_t( util::string_view n, death_knight_t* p,
                               const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s ),
    triggers_icecap( false )
  {
    special    = true;
    may_crit   = true;
    may_glance = false;
  }

  void execute() override;
  void impact( action_state_t * state ) override;
};

// ==========================================================================
// Death Knight Spell
// ==========================================================================

struct death_knight_spell_t : public death_knight_action_t<spell_t>
{
  death_knight_spell_t( util::string_view n, death_knight_t* p,
                        const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  { }
};

struct death_knight_heal_t : public death_knight_action_t<heal_t>
{
  death_knight_heal_t( util::string_view n, death_knight_t* p,
                       const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  { }
};

// ==========================================================================
// Death Knight Diseases
// ==========================================================================
// Common diseases code

struct death_knight_disease_t : public death_knight_spell_t
{
  death_knight_disease_t( util::string_view n, death_knight_t* p, const spell_data_t* s ) :
    death_knight_spell_t( n, p, s )
  { }

  void tick( dot_t* d ) override
  {
    death_knight_spell_t::tick( d );

    if ( p()->talent.brittle.ok() &&
         rng().roll( p() -> talent.brittle -> proc_chance() ) )
    {
      get_td( d->target ) -> debuff.brittle -> trigger();
    }
  }

  timespan_t tick_time ( const action_state_t* s ) const override
  {
    auto base_tick_time = death_knight_spell_t::tick_time( s );

    if ( p() -> specialization()  == DEATH_KNIGHT_UNHOLY && p() -> buffs.plaguebringer -> up() )
    { 
      base_tick_time *= 1.0 + p() -> talent.unholy.plaguebringer->effectN( 1 ).percent();
    }

    return base_tick_time;
  }
};

// Blood Plague ============================================
struct blood_plague_heal_t final : public death_knight_heal_t
{
  blood_plague_heal_t( util::string_view name, death_knight_t* p ) :
    death_knight_heal_t( name, p, p -> spell.blood_plague )
  {
    background = true;
    callbacks = may_crit = may_miss = false;
    target = p;
    // Tick time, duration and healing amount handled by the damage
    attack_power_mod.direct = attack_power_mod.tick = 0;
    dot_duration = base_tick_time = 0_ms;
  }
};

struct blood_plague_t final : public death_knight_disease_t
{
  blood_plague_t( util::string_view name, death_knight_t* p, bool superstrain = false ) :
    death_knight_disease_t( name, p, p -> spell.blood_plague )
  {
    tick_may_crit = background = true;
    may_miss = may_crit = hasted_ticks = false;
    base_tick_time *= 1.0 + p -> talent.blood.rapid_decomposition -> effectN( 1 ).percent();
    heal = get_action<blood_plague_heal_t>( "blood_plague_heal", p );

    // The "reduced effectiveness" mentioned in the tooltip is handled server side
    // Value calculated from testing, may change without notice
    if ( superstrain )
    {
      base_multiplier *= 1 + (p -> talent.unholy.superstrain -> effectN( 2 ).percent());
      // It looks like the legendary modifier from superstrain is still being applied to blood plague, but not frost fever.
      if ( p -> bugs )
        base_multiplier *= 0.75;
    }
  }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_disease_t::composite_ta_multiplier( state );

    if( p() -> specialization() == DEATH_KNIGHT_BLOOD)
      m *= 1.0 + p() -> buffs.coagulopathy -> stack_value();

    return m;
  }

  void tick( dot_t* d ) override
  {
    death_knight_disease_t::tick( d );

    if ( d -> state -> result_amount > 0 )
    {
      // Healing is based off damage done, increased by Rapid Decomposition if talented
      heal -> base_dd_min = heal -> base_dd_max =
        d -> state -> result_amount * ( 1.0 + p() -> talent.blood.rapid_decomposition -> effectN( 3 ).percent() );
      heal -> execute();
    }
  }
private:
    propagate_const<action_t*> heal;
};

// Frost Fever =======================================================
struct frost_fever_t final : public death_knight_disease_t
{
  int rp_generation;

  frost_fever_t( util::string_view name, death_knight_t* p, bool superstrain = false ) :
    death_knight_disease_t( name, p, p -> spell.frost_fever ),
    rp_generation( ( as<int>( p -> spec.frost_fever -> effectN( 1 ).trigger()
                     -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) + ( p -> talent.unholy.superstrain -> effectN(3).base_value() / 10 ) ) ) ) 
  {
    ap_type = attack_power_type::WEAPON_BOTH;

    tick_may_crit = background = true;
    may_miss = may_crit = hasted_ticks = false;

    if ( p -> main_hand_weapon.group() == WEAPON_2H )
    {
      ap_type = attack_power_type::WEAPON_MAINHAND;
      // There's a 0.98 modifier hardcoded in the tooltip if a 2H weapon is equipped, probably server side magic
      base_multiplier *= 0.98;
    }

    // The "reduced effectiveness" mentioned in the tooltip is handled server side
    // Value calculated from testing, may change without notice
    if ( superstrain )
      base_multiplier *= 1.0 + ( p -> talent.unholy.superstrain -> effectN( 2 ).percent() );
  }

  void tick( dot_t* d ) override
  {
    death_knight_disease_t::tick( d );

    // TODO: Melekus, 2019-05-15: Frost fever proc chance and ICD have been removed from spelldata on PTR
    // Figure out what is up with the "30% proc chance, diminishing beyond the first target" from blue post.
    // https://us.forums.blizzard.com/en/wow/t/upcoming-ptr-class-changes-4-23/158332

    // 2020-05-05: It would seem that the proc chance is 0.30 * sqrt(FeverCount) / FeverCount
    unsigned ff_count = p() -> get_active_dots( internal_id );
    double chance = 0.30;
    if ( ( d -> state -> result == RESULT_CRIT ) && p() -> talent.frost.invigorating_freeze.ok() )
      chance += p() -> talent.frost.invigorating_freeze -> effectN( 1 ).percent();

    chance *= std::sqrt( ff_count ) / ff_count;

    if ( rng().roll( chance ) )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER, rp_generation, p() -> gains.frost_fever, this );
    }
  }
};

// Virulent Plague ====================================================
struct virulent_eruption_t final : public death_knight_disease_t
{
  virulent_eruption_t( death_knight_t* p ) :
    death_knight_disease_t( "virulent_eruption", p, p -> spell.virulent_erruption )
  {
    background = split_aoe_damage = true;
    aoe = -1;
  }
};

struct virulent_plague_t final : public death_knight_disease_t
{
  virulent_plague_t( util::string_view name, death_knight_t* p, bool superstrain = false ) :
    death_knight_disease_t( name, p, p -> spell.virulent_plague ),
      ff( get_action<frost_fever_t>( "frost_fever", p, true ) ),
      bp( get_action<blood_plague_t>( "blood_plague", p, true ) )
  {
    // Order of operation matters for dot_duration.  lingering plague gives you an extra 3 seconds, or 1 tick of the dot
    // Ebon fever does the same damage, in half the time, with tick rate doubled.  So you get the same number of ticks
    // Tested Oct 21 2020 in beta build 36294
    dot_duration *= 1.0 + p -> talent.unholy.ebon_fever -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> talent.unholy.ebon_fever -> effectN( 3 ).percent();
    base_tick_time *= 1.0 + p -> talent.unholy.ebon_fever -> effectN( 1 ).percent();
    tick_may_crit = background = true;
    may_miss = may_crit = hasted_ticks = false;
  }
  void impact( action_state_t* s ) override
  {
    death_knight_disease_t::impact( s );
    if ( p()->talent.unholy.superstrain.ok() )
    {
        ff->execute_on_target( s -> target );
        bp->execute_on_target( s -> target );
    }
  }
private:
    propagate_const<action_t*> ff;
    propagate_const<action_t*> bp;
};

// Unholy Blight DoT ====================================================
struct unholy_blight_dot_t final : public death_knight_disease_t
{
  unholy_blight_dot_t( util::string_view name, death_knight_t* p ) :
    death_knight_disease_t( name, p, p -> talent.unholy.unholy_blight -> effectN( 1 ).trigger() )
  {
    tick_may_crit = background = tick_on_application = true;
    may_miss = may_crit = hasted_ticks = false;
  }
};

struct unholy_blight_t final : public death_knight_disease_t
{
  unholy_blight_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_disease_t( "unholy_blight", p, p -> talent.unholy.unholy_blight ),
      dot( get_action<unholy_blight_dot_t>( "unholy_blight_dot", p ) ),
      vp( get_action<virulent_plague_t>( "virulent_plague", p )  )
  {
    may_crit = may_miss = may_dodge = may_parry = hasted_ticks = harmful = false;
    tick_zero = true;
    track_cd_waste = true;
    parse_options( options_str );
    radius = p -> spell.unholy_blight_dot -> effectN( 1 ).radius_max();
    aoe = -1;
    add_child( dot );
  }

  void tick( dot_t* d ) override
  {
    death_knight_disease_t::tick( d );
    dot -> execute_on_target( d -> state -> target );
    vp -> execute_on_target( d -> state -> target );
  }

private:
    propagate_const<action_t*> dot;
    propagate_const<action_t*> vp;
};

// ==========================================================================
// Triggers
// ==========================================================================

// ==========================================================================
// Death Knight Attack Methods
// ==========================================================================

// death_knight_melee_attack_t:execute() ====================================

void death_knight_melee_attack_t::execute()
{
  base_t::execute();
}

// death_knight_melee_attack_t::impact() ====================================

void death_knight_melee_attack_t::impact( action_state_t* state )
{
  base_t::impact( state );

  if ( state -> result_amount > 0 && callbacks && weapon &&
       ( ( p() -> runeforge.rune_of_razorice_mh && weapon -> slot == SLOT_MAIN_HAND ) ||
         ( p() -> runeforge.rune_of_razorice_oh && weapon -> slot == SLOT_OFF_HAND ) ) )
  {
    // Razorice is executed after the attack that triggers it
    p() -> active_spells.runeforge_razorice -> set_target( state -> target );
    p() -> active_spells.runeforge_razorice -> schedule_execute();
  }

  if ( triggers_icecap && p() -> talent.frost.icecap.ok() &&
       p() -> cooldown.icecap_icd -> is_ready() && state -> result == RESULT_CRIT )
  {
    p() -> cooldown.pillar_of_frost -> adjust( timespan_t::from_seconds(
      - p() -> talent.frost.icecap -> effectN( 1 ).base_value() / 10.0 ) );

    p() -> cooldown.icecap_icd -> start();
  }
}

// ==========================================================================
// Death Knight Secondary Abilities
// ==========================================================================

// Razorice Attack ==========================================================

struct razorice_attack_t final : public death_knight_melee_attack_t
{
  razorice_attack_t( death_knight_t* player, util::string_view name ) :
    death_knight_melee_attack_t( name, player, player -> spell.razorice_damage )
  {
    school      = SCHOOL_FROST;
    may_miss    = callbacks = false;
    background  = proc = true;

    // Note, razorice always attacks with the main hand weapon, regardless of which hand triggers it
    weapon = &( player -> main_hand_weapon );
    base_dd_multiplier *= 1.0 + player -> talent.unholy_bond -> effectN( 1 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );
    get_td( s -> target ) -> debuff.razorice -> trigger();
  }
};

// Inexorable Assault =======================================================

struct inexorable_assault_damage_t final : public death_knight_spell_t
{
  inexorable_assault_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> spell.inexorable_assault_damage )
  {
    background = true;
  }
};

// ==========================================================================
// Death Knight Attacks
// ==========================================================================

// Melee Attack =============================================================

struct melee_t : public death_knight_melee_attack_t
{
  int sync_weapons;
  bool first;

  melee_t( const char* name, death_knight_t* p, int sw ) :
    death_knight_melee_attack_t( name, p ), sync_weapons( sw ), first ( true )
  {
    school            = SCHOOL_PHYSICAL;
    may_glance        = true;
    background        = true;
    allow_class_ability_procs = true;
    not_a_proc        = true;
    repeating         = true;
    trigger_gcd       = 0_ms;
    special           = false;
    weapon_multiplier = 1.0;

    // Dual wielders have a -19% chance to hit on melee attacks
    if ( p -> dual_wield() )
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

    if ( first )
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, 0_ms ) : t / 2 ) : 0_ms;
    else
      return t;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = death_knight_melee_attack_t::composite_da_multiplier( s );

    if ( p() -> specialization() == DEATH_KNIGHT_BLOOD && p() -> buffs.vigorous_lifeblood_4pc -> up() )
    {
      m *= 1.0 + p() -> spell.vigorous_lifeblood_4pc -> effectN ( 5 ).percent();
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    const death_knight_td_t* td = get_td( target );
    if ( td && p() -> talent.blood.tightening_grasp.ok() )
    {
      m *= 1.0 + td -> debuff.tightening_grasp -> check_stack_value();
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

    if ( p() -> talent.runic_attenuation.ok() && p() -> rppm.runic_attenuation -> trigger()  )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER,
                            p() -> talent.runic_attenuation -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
                            p() -> gains.runic_attenuation, this );
    }

    if ( result_is_hit( s -> result ) )
    {
      if( p() -> specialization()  == DEATH_KNIGHT_UNHOLY )
      {
        p() -> buffs.sudden_doom -> trigger();
      }

      if ( p() -> talent.frost.killing_machine.ok() && s -> result == RESULT_CRIT )
      {
        p() -> trigger_killing_machine( 0, p() -> procs.km_from_crit_aa,
                                           p() -> procs.km_from_crit_aa_wasted );
      }

      // Crimson scourge doesn't proc if death and decay is ticking
      if ( get_td( s -> target ) -> dot.blood_plague -> is_ticking() && ! p() -> active_dnd )
      {
        if ( p() -> specialization() == DEATH_KNIGHT_BLOOD && p() -> buffs.crimson_scourge -> trigger() )
        {
          p() -> cooldown.death_and_decay_dynamic -> reset( true );
        }
      }

      if ( p() -> talent.blood.bloodworms.ok() )
      {
        trigger_bloodworm();
      }

    }
  }

  void trigger_bloodworm()
  {
    if ( ! p() -> rppm.bloodworms -> trigger() )
    {
      return;
    }
    p() -> procs.bloodworms -> occur();

    // TODO: check whether spelldata is wrong or tooltip is using the wrong spelldata
    // Spelldata used in the tooltip: 15s
    p() -> pets.bloodworms.spawn( p() -> talent.blood.bloodworms -> effectN( 2 ).trigger() -> effectN( 1 ).trigger() -> duration(), 1 );
    // Pet spawn spelldata: 16s
    // p() -> pets.bloodworms.spawn( p() -> talent.bloodworms -> effectN( 1 ).trigger() -> duration(), 1 );
  }
};

// Auto Attack ==============================================================

struct auto_attack_t final : public death_knight_melee_attack_t
{
  int sync_weapons;

  auto_attack_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_melee_attack_t( "auto_attack", p ), sync_weapons( 1 )
  {
    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    parse_options( options_str );
    ignore_false_positive = true;

    assert( p -> main_hand_weapon.type != WEAPON_NONE );

    p -> main_hand_attack = new melee_t( "auto_attack_mh", p, sync_weapons );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "auto_attack_oh", p, sync_weapons );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
      p -> off_hand_attack -> id = 1;
    }

    trigger_gcd = 0_ms;
  }

  void execute() override
  {
    player -> main_hand_attack -> schedule_execute();
    if ( player -> off_hand_attack )
    {
      player -> off_hand_attack -> schedule_execute();
    }
  }

  bool ready() override
  {
    if ( player -> is_moving() )
      return false;

    return( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// ==========================================================================
// Death Knight Abilities
// ==========================================================================

// Abomination Limb =========================================================

struct abomination_limb_damage_t final : public death_knight_spell_t
{
  int bone_shield_stack_gain;
  abomination_limb_damage_t( death_knight_t* p )
    : death_knight_spell_t( "abomination_limb_damage", p, p->talent.abomination_limb->effectN( 2 ).trigger() )
  {
    background = true;
    bone_shield_stack_gain = as<int>( p->talent.abomination_limb->effectN( 3 ).base_value() );
    aoe = -1;
    reduced_aoe_targets = p->talent.abomination_limb->effectN( 5 ).base_value();
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    // We proc this on cast, then every 6 seconds thereafter, on tick
    if ( p() -> cooldown.abomination_limb -> is_ready() )
    {
      switch ( p()->specialization() )
      {
        case DEATH_KNIGHT_BLOOD:
          p() -> buffs.bone_shield -> trigger( bone_shield_stack_gain );
          sim -> print_debug( "{} triggers bone shield via abominations_limb", player -> name() );
          break;
        case DEATH_KNIGHT_FROST:
          p() -> buffs.rime -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
          sim -> print_debug( "{} triggers rime via abominations_limb", player -> name() );
          break;
        case DEATH_KNIGHT_UNHOLY:
          p() -> trigger_runic_corruption( p()->procs.al_runic_corruption, 0, 1.0, false );
          sim -> print_debug( "{} triggers runic corruption via abominations_limb", player -> name() );
          break;
        default:
          break;
      }
      p() -> cooldown.abomination_limb -> start();
    }
  }
};

struct abomination_limb_buff_t final : public buff_t
{
  abomination_limb_damage_t* damage;  // (AOE) damage that ticks every second

  abomination_limb_buff_t( death_knight_t* p )
    : buff_t( p, "abomination_limb", p->talent.abomination_limb ), damage( new abomination_limb_damage_t( p ) )
  {
    cooldown->duration = 0_ms;  // Controlled by the action
    set_tick_callback( [ this ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ ) 
    { 
      damage -> execute(); 
    } );
    set_partial_tick( true );
  }
};

struct abomination_limb_t : public death_knight_spell_t
{
  abomination_limb_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "abomination_limb", p, p->talent.abomination_limb )
  {
    may_crit = may_miss = may_dodge = may_parry = false;

    parse_options( options_str );

    // Periodic behavior handled by the buff
    dot_duration = base_tick_time = 0_ms;

    track_cd_waste = true;

    if ( action_t* abomination_limb_damage = p -> find_action( "abomination_limb_damage" ) )
    {
      add_child( abomination_limb_damage );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    // Pull affect for this ability is NYI

    p() -> buffs.abomination_limb -> trigger();
  }
};

// Apocalypse ===============================================================

struct apocalypse_t final : public death_knight_melee_attack_t
{
  timespan_t summon_duration;
  int rune_generation;

  apocalypse_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_melee_attack_t( "apocalypse", p, p -> talent.unholy.apocalypse ),
    summon_duration( p -> spell.apocalypse_duration -> duration() ),
    rune_generation( as<int>( p -> spell.apocalypse_rune_gen -> effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );

    if ( p->talent.unholy.army_of_the_damned.ok() )
    {
      cooldown -> duration += p -> talent.unholy.army_of_the_damned->effectN( 3 ).time_value();
    }

    track_cd_waste = true;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );
    const death_knight_td_t* td = get_td( state -> target );
    assert( td && "apocalypse impacting without any target data" ); // td should should exist because the debuff is a condition of target_ready()
    auto n_wounds = std::min( as<int>( data().effectN( 2 ).base_value() ), td -> debuff.festering_wound -> check() );

    p() -> burst_festering_wound( state -> target, n_wounds );
    p() -> pets.apoc_ghouls.spawn( summon_duration, n_wounds );

    if ( p() -> talent.unholy.magus_of_the_dead.ok() )
    {
      p() -> pets.apoc_magus.spawn( summon_duration, 1 );
    }

    if ( p() -> talent.unholy.apocalypse.ok() )
    {
      p() -> replenish_rune( rune_generation, p() -> gains.apocalypse );
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    const death_knight_td_t* td = get_td( candidate_target );

    // In-game limitation: you can't use Apocalypse on a target that isn't affected by Festering Wounds
    if ( ! td || ! td -> debuff.festering_wound -> check() )
      return false;

    return death_knight_melee_attack_t::target_ready( candidate_target );
  }
};

// Army of the Dead =========================================================

struct army_of_the_dead_t final : public death_knight_spell_t
{
  double precombat_time;
  timespan_t summon_duration;

  struct summon_army_event_t : public event_t
  {
    int n_ghoul;
    timespan_t summon_interval;
    timespan_t summon_duration;
    death_knight_t* p;

    summon_army_event_t( death_knight_t* dk, int n, timespan_t interval, timespan_t duration ) :
      event_t( *dk, interval ),
      n_ghoul( n ),
      summon_interval( interval ),
      summon_duration( duration ),
      p( dk )
    { }

    void execute() override
    {
      p -> pets.army_ghouls.spawn( summon_duration, 1 );
      if ( ++n_ghoul < 8 )
      {
        make_event<summon_army_event_t>( sim(), p, n_ghoul, summon_interval, summon_duration );
      }
    }
  };

  army_of_the_dead_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "army_of_the_dead", p, p -> talent.unholy.army_of_the_dead ),
    precombat_time( 2.0 ),
    summon_duration( p -> talent.unholy.army_of_the_dead -> effectN( 1 ).trigger() -> duration() )
  {
    // disable_aotd=1 can be added to the profile to disable aotd usage, for example for specific dungeon simming

    if ( p -> options.disable_aotd )
      background = true;

    // If used during precombat, army is cast around X seconds before the fight begins
    // This is done to save rune regeneration time once the fight starts
    // Default duration is 6, and can be changed by the user

    add_option( opt_float( "precombat_time", precombat_time ) );
    parse_options( options_str );

    harmful = false;
    track_cd_waste = true;
  }

  void init_finished() override
  {
    death_knight_spell_t::init_finished();

    if ( is_precombat )
    {
      double MIN_TIME = player -> base_gcd.total_seconds(); // the player's base unhasted gcd: 1.5s
      double MAX_TIME = 10; // Using 10s as highest value because it's the time to recover the rune cost of AOTD at 0 haste

      // Ensure that we're using a positive value
      if ( precombat_time < 0 )
        precombat_time = -precombat_time;

      // Limit Army of the dead precast option between 10s before pull (because rune regeneration time is 10s at 0 haste) and on pull
      if ( precombat_time > MAX_TIME )
      {
        precombat_time = MAX_TIME;
        sim -> error( "{} tried to cast army of the dead more than {} seconds before combat begins, setting to {}",
                       player -> name(), MAX_TIME, MAX_TIME );
      }
      else if ( precombat_time < MIN_TIME )
      {
        precombat_time = MIN_TIME;
        sim -> error( "{} tried to cast army of the dead less than {} before combat begins, setting to {} (has to be >= base gcd)",
                       player -> name(), MIN_TIME, MIN_TIME );
      }
    }
    else precombat_time = 0;

  }

  // Army of the Dead should always cost resources
  double cost() const override
  { return base_costs[ RESOURCE_RUNE ]; }

  void execute() override
  {
    death_knight_spell_t::execute();

    int n_ghoul = 0;

    // There's a 0.5s interval between each ghoul's spawn
    timespan_t const summon_interval = p() -> talent.unholy.army_of_the_dead -> effectN( 1 ).period();

    if ( ! p() -> in_combat && precombat_time > 0 )
    {
      // The first pet spawns after the interval timer
      timespan_t duration_penalty = timespan_t::from_seconds( precombat_time ) - summon_interval;
      while ( duration_penalty >= 0_s && n_ghoul < 8 )
      {
        // Spawn with a duration penalty, and adjust the spawn/travel delay by the penalty
        auto pet = p() -> pets.army_ghouls.spawn( summon_duration - duration_penalty, 1 ).front();
        pet -> precombat_spawn_adjust = duration_penalty;
        pet -> precombat_spawn = true;
        // For each pet, reduce the duration penalty by the 0.5s interval
        duration_penalty -= summon_interval;
        n_ghoul++;
      }

      // Adjust the cooldown based on the precombat time
      p() -> cooldown.army_of_the_dead -> adjust( timespan_t::from_seconds( -precombat_time ), false );

      // Simulate RP decay and rune regeneration
      p() -> resource_loss( RESOURCE_RUNIC_POWER, RUNIC_POWER_DECAY_RATE * precombat_time, nullptr, nullptr );
      p() -> _runes.regenerate_immediate( timespan_t::from_seconds( precombat_time ) );

      sim -> print_debug( "{} used Army of the Dead in precombat with precombat time = {}, adjusting pets' duration and remaining cooldown.",
                          player -> name(), precombat_time );
    }

    // If precombat didn't summon every ghoul (due to interval between each spawn)
    // Or if the cast isn't during precombat
    // Summon the rest
    if ( n_ghoul < 8 )
      make_event<summon_army_event_t>( *sim, p(), n_ghoul, summon_interval, summon_duration );

    if ( p() -> talent.unholy.magus_of_the_dead.ok() )
      p() -> pets.army_magus.spawn( summon_duration - timespan_t::from_seconds( precombat_time ), 1 );

    if ( p() -> sets -> has_set_bonus ( DEATH_KNIGHT_UNHOLY, T30, B4 ) )
    {
      p() -> buffs.unholy_t30_2pc_stacking -> trigger( 20 );
    }
  }
};

// Blood Boil ===============================================================

struct blood_boil_t final : public death_knight_spell_t
{
  blood_boil_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "blood_boil", p, p -> talent.blood.blood_boil )
  {
    parse_options( options_str );

    aoe = -1;
    cooldown -> hasted = true;
    track_cd_waste = true;
    impact_action = get_action<blood_plague_t>( "blood_plague", p );
    
    if ( p -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T30, B2 ) )
    {
      base_multiplier *= 1.0 + p -> sets -> set( DEATH_KNIGHT_BLOOD, T30, B2 ) -> effectN(1).percent();
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> up() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.blood_boil -> execute_on_target( target );

      if ( p() -> talent.blood.everlasting_bond.ok() )
        p() -> pets.everlasting_bond_pet -> ability.blood_boil -> execute_on_target( target );
    }

    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T30, B2 ) )
    {
      if ( p() -> rng().roll( p() -> sets -> set( DEATH_KNIGHT_BLOOD, T30, B2 )->effectN( 2 ).percent() ) )
      {
        // If vamp str is up, we proc an extension, or if vamp str and vamp blood are down, we proc.  We do not proc if vamp str is down, but vamp blood is still up
        if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T30, B4 ) )
        {
          if ( p() -> buffs.vampiric_strength -> up() || ( !p() -> buffs.vampiric_blood -> up() && !p() -> buffs.vampiric_strength -> up() ) )
          {
            p() -> buffs.vampiric_strength -> extend_duration_or_trigger();
          }
        }

        p() -> buffs.vampiric_blood -> extend_duration_or_trigger( p() -> sets -> set( DEATH_KNIGHT_BLOOD, T30, B2 )->effectN( 3 ).time_value() );
      }
    }

    if( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T30, B4 ) && p() -> buffs.vampiric_strength -> up() )
    {
      p() -> buffs.vampiric_strength -> extend_duration( p(), p() -> sets -> set( DEATH_KNIGHT_BLOOD, T30, B4 )->effectN( 1 ).time_value() );
    }
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    p() -> buffs.hemostasis -> trigger();
  }

};

// Blood Tap ================================================================

struct blood_tap_t final : public death_knight_spell_t
{
  blood_tap_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "blood_tap", p, p -> talent.blood.blood_tap )
  {
    parse_options( options_str );
    track_cd_waste = true;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> replenish_rune( as<int>( p() -> talent.blood.blood_tap -> effectN( 1 ).resource( RESOURCE_RUNE ) ), p() -> gains.blood_tap );
  }
};

// Blood Draw =========================================================

struct blood_draw_t final : public death_knight_spell_t
{
  double health_threshold;
  blood_draw_t( util::string_view name, death_knight_t* p )
    : death_knight_spell_t( name, p, p -> spell.blood_draw_damage )
  {
    aoe        = -1;
    background = true;
    cooldown -> duration = p -> spell.blood_draw_cooldown -> duration();
    // Force set threshold to 100% health, to make the spell proc on cooldown
    health_threshold = 100;
    // TODO make health_threshold configurable
    // If role is set to tank, we can use the proper health % value
    if ( p -> primary_role() == ROLE_TANK )
      health_threshold = p -> talent.blood_draw -> effectN( 1 ).base_value();
  }

  bool ready() override
  {
    if ( p() -> health_percentage() > health_threshold )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Blooddrinker =============================================================

struct blooddrinker_heal_t final : public death_knight_heal_t
{
  blooddrinker_heal_t( util::string_view name, death_knight_t* p ) :
    death_knight_heal_t( name, p, p -> talent.blood.blooddrinker )
  {
    background = true;
    callbacks = may_crit = may_miss = false;
    base_costs[ RESOURCE_RUNE ] = 0;
    energize_type = action_energize::NONE;
    cooldown -> duration = 0_ms;
    target = p;
    attack_power_mod.direct = attack_power_mod.tick = 0;
    dot_duration = base_tick_time = 0_ms;
  }
};

// TODO: Implement the defensive stuff somehow
struct blooddrinker_t final : public death_knight_spell_t
{
  blooddrinker_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "blooddrinker", p, p -> talent.blood.blooddrinker ),
    heal( get_action<blooddrinker_heal_t>( "blooddrinker_heal", p ) )
  {
    parse_options( options_str );
    tick_may_crit = channeled = hasted_ticks = tick_zero = true;
    track_cd_waste = true;
    base_tick_time = 1.0_s;
  }

  void tick( dot_t* d ) override
  {
    death_knight_spell_t::tick( d );

    if ( d -> state -> result_amount > 0 )
    {
      heal -> base_dd_min = heal -> base_dd_max = d -> state -> result_amount;
      heal -> execute();
    }
  }
private:
    propagate_const<action_t*> heal;
};

// Bonestorm ================================================================

struct bonestorm_heal_t : public death_knight_heal_t
{
  bonestorm_heal_t( util::string_view name, death_knight_t* p ) :
    death_knight_heal_t( name, p, p -> spell.bonestorm_heal )
  {
    background = true;
    target = p;
  }
};

struct bonestorm_damage_t final : public death_knight_spell_t
{
  int heal_count;

  bonestorm_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> talent.blood.bonestorm -> effectN( 3 ).trigger() ),
    heal( get_action<bonestorm_heal_t>( "bonestorm_heal", p ) ), heal_count( 0 )
  {
    background = true;
    aoe = -1;
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

    if ( result_is_hit( state -> result ) )
    {
      // Healing is limited at 5 occurnces per tick, regardless of enemies hit
      if ( heal_count < p() -> talent.blood.bonestorm -> effectN( 4 ).base_value() )
      {
        heal -> execute();
        heal_count++;
      }
    }
  }
private:
    propagate_const<action_t*> heal;
};

struct bonestorm_t final : public death_knight_spell_t
{
  bonestorm_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "bonestorm", p, p -> talent.blood.bonestorm )
  {
    parse_options( options_str );
    hasted_ticks = false;
    tick_action = get_action<bonestorm_damage_t>( "bonestorm_damage", p );
    track_cd_waste = true;
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  { return base_tick_time * last_resource_cost / 10; }
};

// Breath of Sindragosa =====================================================

struct breath_of_sindragosa_tick_t final : public death_knight_spell_t
{
  breath_of_sindragosa_tick_t( util::string_view name, death_knight_t* p ):
    death_knight_spell_t( name, p, p -> talent.frost.breath_of_sindragosa -> effectN( 1 ).trigger() )
  {
    aoe = -1;
    background = true;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;

    ap_type = attack_power_type::WEAPON_BOTH;

    if ( p -> main_hand_weapon.group() == WEAPON_2H )
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
};

struct breath_of_sindragosa_buff_t : public buff_t
{
  double ticking_cost;
  const timespan_t tick_period;
  int rune_gen;

  breath_of_sindragosa_buff_t( death_knight_t* p ) :
    buff_t( p, "breath_of_sindragosa", p -> talent.frost.breath_of_sindragosa ),
    ticking_cost( 0.0 ), tick_period( p -> talent.frost.breath_of_sindragosa -> effectN( 1 ).period() ),
    rune_gen( as<int>( p -> spell.breath_of_sindragosa_rune_gen -> effectN( 1 ).base_value() ) )
  {
    tick_zero = true;
    cooldown -> duration = 0_ms; // Handled by the action

                                 // Extract the cost per tick from spelldata
    for ( size_t idx = 1; idx <= data().power_count(); idx++ )
    {
      const spellpower_data_t& power = data().powerN( idx );
      if ( power.aura_id() == 0 || player -> dbc->spec_by_spell( power.aura_id() ) == player -> specialization() )
      {
        ticking_cost = power.cost_per_tick();
      }
    }

    bos_damage = get_action<breath_of_sindragosa_tick_t>( "breath_of_sindragosa_tick", p );
    // Store the ticking cost in bos_damage too so resource_loss can fetch the base ability cost
    // cost() is overriden because the actual cost and resources spent are handled by the buff
    bos_damage -> base_costs[ RESOURCE_RUNIC_POWER ] =  ticking_cost;

    set_tick_callback( [ this ] ( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ )
    {
      death_knight_t* p = debug_cast< death_knight_t* >( this -> player );

      // TODO: Target the last enemy targeted by the player's foreground abilities
      // Currently use the player's target which is the first non invulnerable, active enemy found.
      player_t* bos_target = p -> target;

      // On cast execute damage for no cost, and generate runes
      if ( this -> current_tick == 0 )
      {
        bos_damage -> execute_on_target( bos_target );
        p -> replenish_rune( rune_gen, p -> gains.breath_of_sindragosa );
        return;
      }

      // If the player doesn't have enough RP to fuel this tick, BoS is cancelled and no RP is consumed
      // This can happen if the player uses another RP spender between two ticks and is left with < 15 RP
      if ( ! p -> resource_available( RESOURCE_RUNIC_POWER, this -> ticking_cost ) )
      {
        sim -> print_log( "Player {} doesn't have the {} Runic Power required for current tick. Breath of Sindragosa was cancelled.",
                          p -> name_str, this -> ticking_cost );

        // Separate the expiration event to happen immediately after tick processing
        make_event( *sim, 0_ms, [ this ]() { this -> expire(); } );
        return;
      }

      // Else, consume the resource and update the damage tick's resource stats
      p -> resource_loss( RESOURCE_RUNIC_POWER, this -> ticking_cost, nullptr, bos_damage );
      bos_damage -> stats -> consume_resource( RESOURCE_RUNIC_POWER, this -> ticking_cost );

      // If the player doesn't have enough RP to fuel the next tick, BoS is cancelled
      // after the RP consumption and before the damage event
      // This is the normal BoS expiration scenario
      if ( ! p -> resource_available( RESOURCE_RUNIC_POWER, this -> ticking_cost  ) )
      {
        sim -> print_log( "Player {} doesn't have the {} Runic Power required for next tick. Breath of Sindragosa was cancelled.",
                          p -> name_str, this -> ticking_cost );

        // Separate the expiration event to happen immediately after tick processing
        make_event( *sim, 0_ms, [ this ]() { this -> expire(); } );
        return;
      }

      // If there's enough resources for another tick, deal damage
      bos_damage -> execute_on_target( bos_target );

    } );
  }

  // Breath of Sindragosa always ticks every one second, not affected by haste
  timespan_t tick_time() const override
  {
    return tick_period;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    death_knight_t* p = debug_cast< death_knight_t* >( player );

    if ( ! p -> sim -> event_mgr.canceled )
    {
      // BoS generates 2 runes when it expires
      p -> replenish_rune( rune_gen, p -> gains.breath_of_sindragosa );
    }
  }
private:
    propagate_const<action_t*> bos_damage;
};

struct breath_of_sindragosa_t final : public death_knight_spell_t
{
  breath_of_sindragosa_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "breath_of_sindragosa", p, p -> talent.frost.breath_of_sindragosa )
  {
    parse_options( options_str );
    base_tick_time = 0_ms; // Handled by the buff
    add_child( get_action<breath_of_sindragosa_tick_t>( "breath_of_sindragosa_tick", p ) );
    track_cd_waste = true;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.breath_of_sindragosa -> trigger();
  }

  // Breath of Sindragosa can not be used if there isn't enough resources available for one tick
  bool ready() override
  {
    if ( ! p() -> resource_available( RESOURCE_RUNIC_POWER, this -> base_costs_per_tick[ RESOURCE_RUNIC_POWER ] ) )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Chains of Ice ============================================================

// Cold Heart damage
struct cold_heart_damage_t final  : public death_knight_spell_t
{
  cold_heart_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> spell.cold_heart_damage )
  {
    background = true;
  }

  double action_multiplier() const override
  {
    double m = death_knight_spell_t::action_multiplier();

    m *= p() -> buffs.cold_heart -> check();

    return m;
  }
};

struct chains_of_ice_t final : public death_knight_spell_t
{
  chains_of_ice_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "chains_of_ice", p, p -> talent.chains_of_ice )
  {
    parse_options( options_str );
    cold_heart = get_action<cold_heart_damage_t>( "cold_heart", p );
    if ( p -> talent.proliferating_chill.ok() )
    {
      aoe = p -> talent.chains_of_ice -> effectN( 1 ).chain_target() + as<int>( p -> talent.proliferating_chill -> effectN( 1 ).base_value() );
    }
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );
    if ( p() -> buffs.cold_heart -> check() > 0 )
      cold_heart -> execute_on_target( target );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.cold_heart -> check() > 0 )
      p() -> buffs.cold_heart -> expire();
  }
private:
    propagate_const<action_t*> cold_heart;
};

// Chill Streak =============================================================

struct chill_streak_damage_t final : public death_knight_spell_t
{
  int hit_count;

  chill_streak_damage_t( death_knight_t* p ) :
    death_knight_spell_t( "chill_streak_damage", p, p -> spell.chill_streak_damage )
  {
    background = proc = true;
  }
  
  double composite_target_multiplier( player_t* t ) const override
  {
  double m = death_knight_spell_t::composite_target_multiplier( t );

  if ( auto td = get_td( t ) )
    m *= 1.0 + td -> debuff.piercing_chill -> check_value();

  return m;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );
    hit_count++;

    if ( p()->talent.frost.enduring_chill.ok() &&
         rng().roll( p()->talent.frost.enduring_chill->effectN( 1 ).percent() ) )
    {
      hit_count--;
    }
	
    if ( ! state -> action -> result_is_hit( state -> result ) )
    {
      return;
    }

    get_td( state -> target ) -> debuff.piercing_chill -> trigger();

    for ( const auto target : sim -> target_non_sleeping_list )
    {
      if ( target != state -> target )
      {
        if ( hit_count < p() -> talent.frost.chill_streak -> effectN( 1 ).base_value() )
        {
          this -> set_target( target );
          this -> schedule_execute();
          return;
        }
      }
    }
  }
};

struct chill_streak_t final : public death_knight_spell_t
{
  chill_streak_damage_t* damage;

  chill_streak_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "chill_streak", p, p -> talent.frost.chill_streak ),
    damage( new chill_streak_damage_t( p ) )
  {
    parse_options( options_str );
    add_child( damage );
    impact_action = damage;
    aoe = 0;
    track_cd_waste = true;
  }

  void execute() override
  {
    damage -> hit_count = 0;
    death_knight_spell_t::execute();
  }
};

// Consumption ==============================================================

struct consumption_t final : public death_knight_melee_attack_t
{
  consumption_t( death_knight_t* p, util::string_view options_str )
    : death_knight_melee_attack_t( "consumption", p, p -> talent.blood.consumption )
  {
    // TODO: Healing from damage done

    parse_options( options_str );
    aoe = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();
    track_cd_waste = true;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> up() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.consumption -> execute_on_target( target );

      if ( p() -> talent.blood.everlasting_bond.ok() )
        p() -> pets.everlasting_bond_pet -> ability.consumption -> execute_on_target( target );
    }
  }
};

// Dancing Rune Weapon ======================================================
struct dancing_rune_weapon_t final : public death_knight_spell_t
{
  int bone_shield_stack_gain;
  dancing_rune_weapon_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "dancing_rune_weapon", p, p -> talent.blood.dancing_rune_weapon ),
    bone_shield_stack_gain( 0 )
  {
    may_miss = may_crit = may_dodge = may_parry = harmful = false;
    if ( p -> talent.blood.insatiable_blade.ok() )
      bone_shield_stack_gain += as<int>(p -> talent.blood.insatiable_blade -> effectN( 2 ).base_value());
    track_cd_waste = true;
    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    if ( p() -> talent.blood.insatiable_blade.ok() )
    {
      p() -> buffs.bone_shield -> trigger ( bone_shield_stack_gain );
    }

    // Only summon the rune weapons if the buff is down.
    if ( ! p() -> buffs.dancing_rune_weapon -> up() )
    {
      p() -> pets.dancing_rune_weapon_pet -> summon( p() -> talent.blood.dancing_rune_weapon -> duration() +
                                                                               p() -> talent.blood.everlasting_bond -> effectN( 2 ).time_value() );

      if ( p() -> talent.blood.everlasting_bond.ok() )
      {
        p() -> pets.everlasting_bond_pet -> summon( p() -> talent.blood.dancing_rune_weapon -> duration() +
                                                                              p() -> talent.blood.everlasting_bond -> effectN( 2 ).time_value() );
      }
    }
  }
};

// Dark Command =============================================================

struct dark_command_t final : public death_knight_spell_t
{
  dark_command_t( death_knight_t* p, util::string_view options_str ):
    death_knight_spell_t( "dark_command", p, p -> find_class_spell( "Dark Command" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    use_off_gcd = true;
  }

  void impact( action_state_t* s ) override
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    death_knight_spell_t::impact( s );
  }
};

// Dark Transformation and Unholy Pact ======================================

struct unholy_pact_damage_t final : public death_knight_spell_t
{
  unholy_pact_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> spell.unholy_pact_damage )
  {
    background = true;

    // TODO: the aoe is a line between the player and its pet, find a better way to translate that into # of targets hit
    // limited target threshold?
    // User inputted value?
    // 2018-12-30 hitting everything is good enough for now
    aoe = -1;
  }
};

// The ingame implementation is a mess with multiple unholy pact buffs on the player and pet
// As well as multiple unholy pact damage spells. The simc version is simplified
struct unholy_pact_buff_t final : public buff_t
{
  unholy_pact_buff_t( death_knight_t* p ) :
    buff_t( p, "unholy_pact",
      p -> talent.unholy.unholy_pact -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() ),
    damage( get_action<unholy_pact_damage_t>( "unholy_pact", p ) )
  {
    tick_zero = true;
    buff_period = 1.0_s;
    set_tick_behavior( buff_tick_behavior::CLIP );
    set_tick_callback( [ this ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ )
    {
      damage -> execute();
    } );
    // Unholy pact ticks twice on buff expiration, hence the following override
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/675
    set_stack_change_callback( [ this, p ] ( buff_t*, int, int new_ ) 
    {
      if ( !new_ )
      {
        damage -> execute();
        damage -> execute();
      }
    } );
  }
private:
    propagate_const<action_t*> damage;
};

struct dark_transformation_damage_t final : public death_knight_spell_t
{
  dark_transformation_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> spell.dark_transformation_damage )
  {
    background = dual = true;
    aoe = as<int>( data().effectN( 2 ).base_value() );
  }
};

// Even though the buff is tied to the pet ingame, it's simpler to add it to the player
struct dark_transformation_buff_t final : public buff_t
{
  dark_transformation_buff_t( death_knight_t* p ) :
    buff_t( p, "dark_transformation", p -> talent.unholy.dark_transformation )
  {
    set_default_value_from_effect( 1 );
    cooldown -> duration = 0_ms; // Handled by the player ability
    if( p -> talent.unholy.ghoulish_frenzy.ok() )
    {
      set_stack_change_callback( [ this, p ] ( buff_t*, int, int new_ ) 
      {
        if ( new_ )
        {
          p -> buffs.ghoulish_frenzy -> trigger();
          debug_cast<pets::ghoul_pet_t*>( p -> pets.ghoul_pet ) -> ghoulish_frenzy -> trigger();
        }
        else
        {
          p -> buffs.ghoulish_frenzy -> expire();
          debug_cast<pets::ghoul_pet_t*>( p -> pets.ghoul_pet ) -> ghoulish_frenzy -> expire();
        }
      } );
    }
  }
};

struct dark_transformation_t final : public death_knight_spell_t
{
  bool precombat_frenzy;

  dark_transformation_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "dark_transformation", p, p -> talent.unholy.dark_transformation ),
    precombat_frenzy( false )
  {
    add_option( opt_bool( "precombat_frenzy", precombat_frenzy ) );
    harmful = false;
    track_cd_waste = true;

    // Don't create and use the damage if the spell is used for precombat frenzy
    if ( ! precombat_frenzy )
    {
      execute_action = get_action<dark_transformation_damage_t>( "dark_transformation_damage", p );
      execute_action -> stats = stats;
    }

    if ( p -> talent.unholy.unholy_command.ok() )
    {
      cooldown->duration += p->talent.unholy.unholy_command->effectN( 1 ).time_value();
    }

    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.dark_transformation -> trigger();

    // Rank 2 still exists for unholy as a baseline
    if ( p() -> spec.dark_transformation_2 -> ok() )
    {
      p() -> pets.ghoul_pet -> resource_gain( RESOURCE_ENERGY, p() -> spec.dark_transformation_2 -> effectN( 1 ).base_value(),
                                                p() -> pets.ghoul_pet -> dark_transformation_gain, this );
    }

    if ( p() -> talent.unholy.unholy_pact.ok() )
    {
      p() -> buffs.unholy_pact -> trigger();
    }

    if ( p() -> talent.unholy.commander_of_the_dead.ok() )
    {
      p() -> buffs.commander_of_the_dead -> trigger();
    }

    if ( p() -> sets -> has_set_bonus ( DEATH_KNIGHT_UNHOLY, T30, B2 ) && p() -> buffs.unholy_t30_2pc_stacking -> check() )
    {
      p() -> buffs.unholy_t30_2pc_mastery -> trigger( p() -> buffs.unholy_t30_2pc_stacking -> check() );
      p() -> buffs.unholy_t30_2pc_stacking -> expire();
    }
  }

  bool ready() override
  {
    if ( ! p() -> pets.ghoul_pet || p() -> pets.ghoul_pet -> is_sleeping() )
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
  death_and_decay_damage_base_t( util::string_view name, death_knight_t* p, const spell_data_t* spell ) :
    death_knight_spell_t( name, p, spell )
  {
    aoe = -1;
    background = dual = true;
    tick_zero = true;
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );
    if ( p() -> talent.unholy.pestilence.ok() )
    {
      if ( rng().roll( p() -> talent.unholy.pestilence -> effectN( 1 ).percent() ) )
      {
        p() -> trigger_festering_wound( s, 1, p() -> procs.fw_pestilence );
      }
    }
  }
};

struct death_and_decay_damage_t final : public death_and_decay_damage_base_t
{
  death_and_decay_damage_t( util::string_view name, death_knight_t* p, const spell_data_t* s = nullptr ) :
    death_and_decay_damage_base_t( name, p, s == nullptr ? p -> spell.death_and_decay_damage : s )
  { }
};

struct defile_damage_t final : public death_and_decay_damage_base_t
{
  double active_defile_multiplier;
  const double defile_tick_multiplier;

  defile_damage_t( util::string_view name, death_knight_t* p ) :
    death_and_decay_damage_base_t( name, p, p -> spell.defile_damage ),
    active_defile_multiplier( 1.0 ),
    // Testing shows a 1.06 multiplicative damage increase for every tick of defile that hits an enemy
    // Can't seem to find it anywhere in defile's spelldata
    defile_tick_multiplier( 1.06 )
  {}

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

      p() -> buffs.defile_buff -> trigger();
    }
  }
};

// Relish in Blood healing and RP generation
struct relish_in_blood_t final : public death_knight_heal_t
{
  relish_in_blood_t( util::string_view name, death_knight_t* p ) :
    death_knight_heal_t( name, p, p -> spell.relish_in_blood_gains )
  {
    background = true;
    target = p;
  }

  double action_multiplier() const override
  {
    double m = death_knight_heal_t::action_multiplier();

    m *= p() -> buffs.bone_shield -> check();

    return m;
  }
};

// Main Death and Decay spells

struct death_and_decay_base_t : public death_knight_spell_t
{
  action_t* damage;

  death_and_decay_base_t( death_knight_t* p, util::string_view name, const spell_data_t* spell ) :
    death_knight_spell_t( name, p, spell ),
    damage( nullptr )
  {
    base_tick_time = dot_duration = 0_ms; // Handled by event
    ignore_false_positive = true; // TODO: Is this necessary?
    may_crit              = false;
    // Note, radius and ground_aoe flag needs to be set in base so spell_targets expression works
    ground_aoe            = true;
    radius                = data().effectN( 1 ).radius_max();
    track_cd_waste = true;
    // Set the player-stored death and decay cooldown to this action's cooldown
    p -> cooldown.death_and_decay_dynamic = cooldown;

    if ( p -> talent.blood.relish_in_blood.ok() )
      relish_in_blood = get_action<relish_in_blood_t>( "relish_in_blood", p );
  }

  void init_finished() override
  {
    death_knight_spell_t::init_finished();
    // Merge stats with the damage object
    damage -> stats = stats;
  }

  double cost() const override
  {
    if ( p() -> specialization() == DEATH_KNIGHT_BLOOD && p() -> buffs.crimson_scourge -> check() )
    {
      return 0;
    }

    return death_knight_spell_t::cost();
  }

  double runic_power_generation_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::runic_power_generation_multiplier( state );

    if ( p() -> specialization() == DEATH_KNIGHT_BLOOD && p() -> buffs.crimson_scourge -> check() )
    {
      m *= 1.0 + p() -> buffs.crimson_scourge -> data().effectN( 2 ).percent();
    }

    return m;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    // If bone shield isn't up, Relish in Blood doesn't heal or generate any RP
    if ( p() -> specialization() == DEATH_KNIGHT_BLOOD && p() -> buffs.crimson_scourge -> up() && p() -> talent.blood.relish_in_blood.ok() && p() -> buffs.bone_shield -> up() )
    {
      // The heal's energize data automatically handles RP generation
      relish_in_blood -> execute();
    }

    if( p() -> specialization() == DEATH_KNIGHT_BLOOD && p() -> buffs.crimson_scourge -> up() )
    {
      p() -> buffs.crimson_scourge -> decrement();
      if ( p() -> talent.blood.perseverance_of_the_ebon_blade.ok() )
        p() -> buffs.perseverance_of_the_ebon_blade -> trigger();
    }

    if ( p() -> talent.unholy_ground.ok() )
    {
      p() -> buffs.unholy_ground -> trigger();
      p() -> buffs.unholy_ground -> set_duration( data().duration() );
    }

    if ( p() -> specialization() == DEATH_KNIGHT_BLOOD && p() -> talent.blood.sanguine_ground.ok() )
    {
      p() -> buffs.sanguine_ground -> trigger();
      p() -> buffs.sanguine_ground -> set_duration( data().duration() );
    }

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
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
          case ground_aoe_params_t::EVENT_CREATED:
            p() -> active_dnd = event;
            break;
          case ground_aoe_params_t::EVENT_DESTRUCTED:
            p() -> active_dnd = nullptr;
            break;
          default:
            break;
        }
      } ), true /* Immediate pulse */ );
  }

private:
  timespan_t compute_tick_time() const
  {
    auto base = data().effectN( 3 ).period();
    base *= 1.0 + p() -> talent.blood.rapid_decomposition -> effectN( 1 ).percent();

    return base;
  }
  propagate_const<action_t*> relish_in_blood;
};

struct death_and_decay_t final : public death_and_decay_base_t
{
  death_and_decay_t( death_knight_t* p, util::string_view options_str ) :
    death_and_decay_base_t( p, "death_and_decay", p -> spec.death_and_decay )
  {
    damage = get_action<death_and_decay_damage_t>( "death_and_decay_damage", p );

    parse_options( options_str );

    if ( p->talent.deaths_echo.ok() )
      cooldown->charges += as<int>( p->talent.deaths_echo->effectN( 1 ).base_value() );
    // Disable when Defile or Death's Due are taken
    if ( p -> talent.unholy.defile.ok() )
      background = true;
  }
};

struct defile_t final : public death_and_decay_base_t
{
  defile_t( death_knight_t* p, util::string_view options_str ) :
    death_and_decay_base_t( p, "defile", p -> talent.unholy.defile )
  {
    damage = get_action<defile_damage_t>( "defile_damage", p );

    parse_options( options_str );
    if ( p->talent.deaths_echo.ok() )
      cooldown->charges += as<int>( p->talent.deaths_echo->effectN( 4 ).base_value() );
  }

  void execute() override
  {
    // Reset the damage component's increasing multiplier
    debug_cast<defile_damage_t*>( damage ) -> active_defile_multiplier = 1.0;

    death_and_decay_base_t::execute();
  }
};

// Death's Caress ===========================================================

struct deaths_caress_t final : public death_knight_spell_t
{
  deaths_caress_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "deaths_caress", p, p -> talent.blood.deaths_caress )
  {
    parse_options( options_str );
    impact_action = get_action<blood_plague_t>( "blood_plague", p );
    track_cd_waste = true;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.bone_shield -> trigger( as<int>(p() -> talent.blood.deaths_caress -> effectN( 3 ).base_value()) );

    if ( p() -> buffs.dancing_rune_weapon -> up() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.deaths_caress -> execute_on_target( target );

      if ( p() -> talent.blood.everlasting_bond.ok() )
        p() -> pets.everlasting_bond_pet -> ability.deaths_caress -> execute_on_target( target );
    }
  }
};

// Death Coil ===============================================================

struct coil_of_devastation_t final : public residual_action::residual_periodic_action_t<death_knight_spell_t>
{
  coil_of_devastation_t( util::string_view name, death_knight_t* p )
    : residual_action::residual_periodic_action_t<death_knight_spell_t>( name, p, p -> spell.coil_of_devastation_debuff )
  {
    background = dual = true;
    may_miss = may_crit = false;
  }
};

struct death_coil_damage_t final : public death_knight_spell_t
{
  death_coil_damage_t( util::string_view name, death_knight_t* p ) : death_knight_spell_t( name, p, p -> spell.death_coil_damage ), coil_of_devastation( nullptr )
  {
    background = dual = true;

    base_multiplier *= 1.0 + p -> talent.unholy.improved_death_coil -> effectN( 1 ).percent();

    if ( p -> sets -> has_set_bonus ( DEATH_KNIGHT_UNHOLY, T30, B2 ) )
    {
      base_multiplier *= 1.0 + p -> spell.unholy_t30_2pc_values -> effectN( 1 ).percent();
    }

    if ( p -> talent.unholy.coil_of_devastation.ok() )
    {
      coil_of_devastation = get_action<coil_of_devastation_t>( "coil_of_devastation", p );
    }
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( state );

    if ( p() -> talent.unholy.reaping.ok() && target -> health_percentage() < p() -> talent.unholy.reaping -> effectN( 2 ).base_value() )
    {
      m *= 1.0 + p() -> talent.unholy.reaping -> effectN( 1 ).percent();
    }

    if ( p() -> talent.unholy.harbinger_of_doom.ok() && p() -> buffs.sudden_doom -> check() )
    {
      m *= 1.0 + p() -> talent.unholy.harbinger_of_doom -> effectN( 3 ).percent() * p() -> buffs.sudden_doom -> stack();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( p() -> talent.unholy.coil_of_devastation.ok() && result_is_hit( state -> result ) )
    {
      residual_action::trigger( coil_of_devastation, state -> target,
                                state -> result_amount * p() -> talent.unholy.coil_of_devastation->effectN( 1 ).percent() );
    }
  }
private:
    propagate_const<action_t*> coil_of_devastation;
};

struct death_coil_t final : public death_knight_spell_t
{
  death_coil_t( death_knight_t* p, util::string_view options_str )
    : death_knight_spell_t( "death_coil", p, p -> spec.death_coil )
  {
    parse_options( options_str );

    impact_action = get_action<death_coil_damage_t>( "death_coil_damage", p );
    impact_action -> stats = stats;

    if ( p -> talent.unholy.coil_of_devastation.ok() )
      add_child( get_action<coil_of_devastation_t>( "coil_of_devastation", p ) );

    if ( p -> talent.unholy.improved_death_coil.ok() )
    {
      aoe = 1 + as<int>( p -> talent.unholy.improved_death_coil -> effectN( 2 ).base_value() );
    }
  }

  double cost() const override
  {
    if ( p() -> buffs.sudden_doom -> check() )
      return 0;

    double cost = death_knight_spell_t::cost();

    return cost;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    // Reduce the cooldown on Army of the Dead if Army of the Damned is talented

    p() -> cooldown.army_of_the_dead -> adjust( -timespan_t::from_seconds(
      p() -> talent.unholy.army_of_the_damned -> effectN( 2 ).base_value() / 10 ) );
	
	if ( p() -> buffs.dark_transformation -> up() && p() -> talent.unholy.eternal_agony.ok() )
    {
      p() -> buffs.dark_transformation -> extend_duration( p(),
        timespan_t::from_seconds( p() -> talent.unholy.eternal_agony -> effectN( 1 ).base_value() ) );
    }

    if ( p() -> sets -> has_set_bonus ( DEATH_KNIGHT_UNHOLY, T30, B2 ) )
    {
      p() -> buffs.unholy_t30_2pc_stacking -> trigger();

      if ( p() -> sets -> has_set_bonus ( DEATH_KNIGHT_UNHOLY, T30, B4 ) && p() -> buffs.sudden_doom -> up() )
      {
        p() -> buffs.unholy_t30_2pc_stacking -> trigger( as<int>(p() -> spell.unholy_t30_4pc_values -> effectN( 1 ).base_value()) );
        
      }
    }

    if( p() -> sets -> has_set_bonus ( DEATH_KNIGHT_UNHOLY, T30, B4 ) && p() -> buffs.sudden_doom -> check() )
    {
      p() -> buffs.unholy_t30_4pc_mastery -> trigger();
    }

    p() -> buffs.sudden_doom -> decrement();
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( p() -> talent.unholy.death_rot.ok() && result_is_hit( state -> result ) )
    {
      get_td( state -> target ) -> debuff.death_rot -> trigger();
      
      if ( p() -> buffs.sudden_doom -> check() )
      {
        get_td( state -> target ) -> debuff.death_rot -> trigger();
      }
    }

    if ( p() -> talent.unholy.rotten_touch.ok() && p() -> buffs.sudden_doom -> check() && 
         result_is_hit( state -> result ) )
    {
      get_td( state -> target ) -> debuff.rotten_touch -> trigger();
    }
  }

};

// Death Strike =============================================================

struct blood_shield_buff_t final : public absorb_buff_t
{
  blood_shield_buff_t( death_knight_t* player ) :
    absorb_buff_t( player, "blood_shield", player -> spell.blood_shield )
  {
    set_absorb_school( SCHOOL_PHYSICAL );
    set_absorb_source( player -> get_stats( "blood_shield" ) );
    modify_duration( player -> talent.blood.iron_heart ->effectN( 1 ).time_value() );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
};

struct blood_shield_t final : public absorb_t
{
  blood_shield_t( death_knight_t* p ) :
    absorb_t( "blood_shield", p, p -> spell.blood_shield )
  {
    may_miss = may_crit = callbacks = false;
    background = proc = true;
  }

  // Self only so we can do this in a simple way
  absorb_buff_t* create_buff( const action_state_t* ) override
  { return debug_cast<death_knight_t*>( player ) -> buffs.blood_shield; }

  void init() override
  {
    absorb_t::init();

    snapshot_flags = update_flags = 0;
  }
};

struct death_strike_heal_t final : public death_knight_heal_t
{
  blood_shield_t* blood_shield;
  timespan_t interval;
  double min_heal_multiplier;
  double max_heal_multiplier;

  death_strike_heal_t( util::string_view name, death_knight_t* p ) :
    death_knight_heal_t( name, p, p -> spell.death_strike_heal ),
    blood_shield( p -> specialization() == DEATH_KNIGHT_BLOOD ? new blood_shield_t( p ) : nullptr ),
    interval( timespan_t::from_seconds( p -> talent.death_strike -> effectN( 4 ).base_value() ) ),
    min_heal_multiplier( p -> talent.death_strike -> effectN( 3 ).percent() ),
    max_heal_multiplier( p -> talent.death_strike -> effectN( 2 ).percent() )
  {
    may_crit = callbacks = false;
    background = true;
    target     = p;

    // Melekus 2020-09-07: DS base healing with Voracious is a lot higher than expected
    // There's a hidden effect in Voracious' spelldata that increases death strike's effect#3 (base healing) by 50%
    if ( p -> talent.blood.voracious.ok() )
    {
      min_heal_multiplier *= 1.0 + p -> talent.blood.voracious -> effectN( 3 ).percent();
      max_heal_multiplier *= 1.0 + p -> talent.blood.voracious -> effectN( 2 ).percent();
    }
    if ( p -> talent.improved_death_strike.ok() )
    {
      // Blood only gets 10%, dps specs get 60%
      if ( p -> specialization() == DEATH_KNIGHT_BLOOD )
      {
        // Min and max pulls from the same effect for blood.
        min_heal_multiplier *= 1.0 + p -> talent.improved_death_strike -> effectN( 4 ).percent();
        max_heal_multiplier *= 1.0 + p -> talent.improved_death_strike -> effectN( 4 ).percent();
      }
      else
      {
        min_heal_multiplier *= 1.0 + p -> talent.improved_death_strike -> effectN( 2 ).percent();
        max_heal_multiplier *= 1.0 + p -> talent.improved_death_strike -> effectN( 1 ).percent();
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
    return std::max( player -> resources.max[ RESOURCE_HEALTH ] * min_heal_multiplier,
                              player -> compute_incoming_damage( interval ) * max_heal_multiplier );
  }

  double base_da_max( const action_state_t* ) const override
  {
    return std::max( player -> resources.max[ RESOURCE_HEALTH ] * min_heal_multiplier,
                              player -> compute_incoming_damage( interval ) * max_heal_multiplier );
  }

  double action_multiplier() const override
  {
    double m = death_knight_heal_t::action_multiplier();

    m *= 1.0 + p() -> buffs.hemostasis -> stack_value();

    return m;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_heal_t::impact( state );

    trigger_blood_shield( state );
  }

  void trigger_blood_shield( action_state_t* state )
  {
    if ( p() -> specialization() != DEATH_KNIGHT_BLOOD )
      return;

    double current_value = 0;
    if ( blood_shield -> target_specific[ state -> target ] )
      current_value = blood_shield -> target_specific[ state -> target ] -> current_value;

    double amount = state -> result_total;  // Total heal, including hemostasis

    // We first have to remove hemostasis from the heal, as blood shield does not include it
    amount /= 1.0 + p() -> buffs.hemostasis->stack_value();

    if ( p() -> mastery.blood_shield -> ok() )
      amount *= p() -> cache.mastery_value();

    amount *= 1.0 + p() -> talent.blood.iron_heart -> effectN( 2 ).percent();

    amount *= 1.0 + p() -> talent.gloom_ward -> effectN( 1 ).percent();

    // Blood Shield caps at max health
    if ( amount > player -> resources.max[ RESOURCE_HEALTH ] )
      amount = player -> resources.max[ RESOURCE_HEALTH ];

    sim -> print_debug( "{} Blood Shield buff trigger, old_value={} added_value={} new_value={}",
                     player -> name(), current_value,
                     state -> result_amount * p() -> cache.mastery_value(),
                     amount );

    blood_shield -> base_dd_min = blood_shield -> base_dd_max = amount;
    blood_shield -> execute();
  }
};

struct death_strike_offhand_t final : public death_knight_melee_attack_t
{
  death_strike_offhand_t( util::string_view name, death_knight_t* p ) :
    death_knight_melee_attack_t( name, p, p -> spell.death_strike_offhand )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
  }
};

struct death_strike_t final : public death_knight_melee_attack_t
{
  double improved_death_strike_reduction;

  death_strike_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_melee_attack_t( "death_strike", p, p -> talent.death_strike ),
    oh_attack( nullptr ),
    heal( get_action<death_strike_heal_t>( "death_strike_heal", p ) ),
    improved_death_strike_reduction( 0 )
  {
    parse_options( options_str );
    may_parry = false;

    weapon = &( p -> main_hand_weapon );

    if ( p -> dual_wield() )
    {
      oh_attack = get_action<death_strike_offhand_t>( "death_strike_offhand", p );
      add_child( oh_attack );
    }

    if ( p -> talent.improved_death_strike.ok() )
    {
      if ( p -> specialization() == DEATH_KNIGHT_BLOOD )
        improved_death_strike_reduction += p -> talent.improved_death_strike -> effectN( 5 ).resource( RESOURCE_RUNIC_POWER );
      else
        improved_death_strike_reduction += p -> talent.improved_death_strike -> effectN( 3 ).resource( RESOURCE_RUNIC_POWER );
    }
  }

  double action_multiplier() const override
  {
    double m = death_knight_melee_attack_t::action_multiplier();

    m *= 1.0 + p() -> buffs.hemostasis -> stack_value();

    m *= 1.0 + p() -> buffs.heartrend -> stack_value();

    // Death Strike is affected by bloodshot, even for the applying DS.  This is because in game, blood shield gets applied before DS damage is calculated.
    // So we apply the modifier here, but only if bloodshield is not up, to avoid double dip by the base multipler when blood shield is up and bloodshot is talented.
    if ( p() -> talent.blood.bloodshot.ok() && !p() -> buffs.blood_shield -> up() )
    {
      m *= 1.0 + p() -> talent.blood.bloodshot -> effectN( 1 ).percent();
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    // 2020-08-23: Seems to only affect main hand death strike, not OH hit, not DRW's spell
    // No spelldata either, just "increase damage based on target's missing health"
    // Update 2021-06-16 Changed from 1% to 1.25% damage increase
    // Testing shows a linear 1.25% damage increase for every 1% missing health, up to 100% damage increase
    if ( p() -> runeforge.rune_of_sanguination )
    {
      auto buff_amount = ( 1.0 - target -> health_percentage() / 100 ) * 1.25;
      buff_amount = std::min( buff_amount, 1.0 );  // Max 100% bonus damage
      m *= 1.0 + buff_amount;
    }

    return m;
  }

  double cost() const override
  {
    double c = death_knight_melee_attack_t::cost();

    c += p() -> buffs.ossuary -> value();

    c += improved_death_strike_reduction;

    return c;
  }

  void execute() override
  {
    p() -> buffs.voracious -> trigger();

    death_knight_melee_attack_t::execute();

    p() -> buffs.coagulopathy -> trigger();

    if ( oh_attack )
      oh_attack -> execute();

    if ( p() -> buffs.dancing_rune_weapon -> up() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.death_strike -> execute_on_target( target );

      if ( p() -> talent.blood.everlasting_bond.ok() )
        p() -> pets.everlasting_bond_pet -> ability.death_strike -> execute_on_target( target );

    }

    if ( hit_any_target )
    {
      heal -> execute();
    }

    p() -> buffs.hemostasis -> expire();
    p() -> buffs.heartrend -> expire();
  }
  private:
      propagate_const<action_t*> heal;
      propagate_const<action_t*> oh_attack;
};

// Empower Rune Weapon ======================================================
struct empower_rune_weapon_t final : public death_knight_spell_t
{
  empower_rune_weapon_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "empower_rune_weapon", p, p -> spell.empower_rune_weapon_main )
  {
    parse_options( options_str );

    harmful = false;
    track_cd_waste = true;

    // Buff handles the ticking, this one just triggers the buff
    dot_duration = base_tick_time = 0_ms;

    cooldown -> duration = p -> spell.empower_rune_weapon_main -> charge_cooldown();

    cooldown -> charges = as<int>(p -> spell.empower_rune_weapon_main -> charges() + p -> talent.empower_rune_weapon -> effectN( 1 ).base_value() + p -> talent.frost.empower_rune_weapon -> effectN( 1 ).base_value());
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.empower_rune_weapon -> trigger();
  }
};

// Epidemic =================================================================

struct epidemic_damage_main_t final : public death_knight_spell_t
{
  double soft_cap_multiplier;
  epidemic_damage_main_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> spell.epidemic_damage ),
    soft_cap_multiplier( 1.0 )
  {
    background = true;
    // Ignore spelldata for max targets for the main spell, as it is single target only
    aoe = 0;
    // this spell has both coefficients in it, and it seems like it is reading #2, the aoe portion, instead of #1
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double cam = death_knight_spell_t::composite_aoe_multiplier( state );

    cam *= soft_cap_multiplier;

    if( p() -> talent.unholy.harbinger_of_doom.ok() && p() -> buffs.sudden_doom -> check() )
    {
      cam *= 1.0 + p() -> talent.unholy.harbinger_of_doom -> effectN( 4 ).percent();
    }

    if( p() -> sets -> has_set_bonus ( DEATH_KNIGHT_UNHOLY, T30, B2 ) )
    {
      cam *= 1.0 + p() -> spell.unholy_t30_2pc_values -> effectN( 1 ).percent();
    }

    return cam;
  }
};

struct epidemic_damage_aoe_t final : public death_knight_spell_t
{
  double soft_cap_multiplier;
  epidemic_damage_aoe_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> spell.epidemic_damage ),
    soft_cap_multiplier( 1.0 )
  {
    background = true;
    // Main is one target, aoe is the other targets, so we take 1 off the max targets
    aoe = aoe - 1;

    attack_power_mod.direct = data().effectN( 2 ).ap_coeff();
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    death_knight_spell_t::available_targets( tl );

    auto it = range::find( tl, target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    return tl.size();
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double cam = death_knight_spell_t::composite_aoe_multiplier( state );

    cam *= soft_cap_multiplier;

    if( p() -> talent.unholy.harbinger_of_doom.ok() && p() -> buffs.sudden_doom -> check() )
    {
      cam *= 1.0 + p() -> talent.unholy.harbinger_of_doom -> effectN( 4 ).percent();
    }

    if( p() -> sets -> has_set_bonus ( DEATH_KNIGHT_UNHOLY, T30, B2 ) )
    {
      cam *= 1.0 + p() -> spell.unholy_t30_2pc_values -> effectN( 1 ).percent();
    }
    return cam;
  }
};

struct epidemic_t final : public death_knight_spell_t
{
  double custom_reduced_aoe_targets;  // Not in spelldata
  double soft_cap_multiplier;
  epidemic_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "epidemic", p, p -> talent.unholy.epidemic ),
    custom_reduced_aoe_targets( 8.0 ),
    soft_cap_multiplier( 1.0 )
  {
    parse_options( options_str );

    impact_action = get_action<epidemic_damage_main_t>( "epidemic_main", p );
    impact_action -> impact_action = get_action<epidemic_damage_aoe_t>( "epidemic_aoe", p );

    add_child( impact_action );
    add_child( impact_action -> impact_action );
  }

  double cost() const override
  {
    if ( p() -> buffs.sudden_doom -> check() )
      return 0;

    return death_knight_spell_t::cost();
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    death_knight_spell_t::available_targets( tl );

    // Remove enemies that are not affected by virulent plague
    tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ] ( player_t* t ) {
      return ! this -> get_td( t ) -> dot.virulent_plague -> is_ticking();
    } ), tl.end() );

    return tl.size();
  }

  void execute() override
  {
    // Reset target cache because of smart targetting
    target_cache.is_valid = false;

    death_knight_spell_t::execute();

    p() -> cooldown.army_of_the_dead -> adjust( -timespan_t::from_seconds(
      p() -> talent.unholy.army_of_the_damned -> effectN( 2 ).base_value() / 10 ) );

    if ( p() -> buffs.dark_transformation -> up() && p() -> talent.unholy.eternal_agony.ok() )
    {
      p() -> buffs.dark_transformation -> extend_duration( p(),
        timespan_t::from_seconds( p() -> talent.unholy.eternal_agony -> effectN( 1 ).base_value() ) );
    }

    if ( p() -> sets -> has_set_bonus ( DEATH_KNIGHT_UNHOLY, T30, B2 ) )
    {
      p() -> buffs.unholy_t30_2pc_stacking -> trigger();

      if ( p() -> buffs.sudden_doom -> up() )
      {
        p() -> buffs.unholy_t30_2pc_stacking -> trigger();
        
        if( p() -> sets -> has_set_bonus ( DEATH_KNIGHT_UNHOLY, T30, B4 ) && p() -> buffs.unholy_t30_2pc_mastery -> up() )
        {
          p() -> buffs.unholy_t30_4pc_mastery -> trigger();
        }
      }
    }

    p() -> buffs.sudden_doom -> decrement();
  }

  void impact( action_state_t* state ) override
  {
    // Set the multiplier for reduced aoe soft cap
    if ( state->n_targets > 0.0 && state->n_targets > custom_reduced_aoe_targets )
      soft_cap_multiplier = sqrt( custom_reduced_aoe_targets / std::min<int>( sim->max_aoe_enemies, state->n_targets) );
    else
      soft_cap_multiplier = 1.0;

    debug_cast<epidemic_damage_main_t*>( impact_action ) -> soft_cap_multiplier = soft_cap_multiplier;
    debug_cast<epidemic_damage_aoe_t*>( impact_action -> impact_action ) -> soft_cap_multiplier = soft_cap_multiplier;

    death_knight_spell_t::impact( state );

    if ( p() -> talent.unholy.death_rot.ok() && result_is_hit( state -> result ) )
    {
      get_td( state -> target ) -> debuff.death_rot -> trigger();
      
      if ( p() -> buffs.sudden_doom -> check() )
      {
        get_td( state -> target ) -> debuff.death_rot -> trigger();
      }
    }
  }
};

// Festering Strike and Wounds ===============================================

struct bursting_sores_t final : public death_knight_spell_t
{
  bursting_sores_t( death_knight_t* p ) :
    death_knight_spell_t( "bursting_sores", p, p -> spell.bursting_sores_damage )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();
  }

  // Bursting sores have a slight delay ingame, but nothing really significant
  // The travel time is overriden to zero to make it trigger properly on enemy death
  timespan_t travel_time() const override
  {
    return 0_ms;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
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

struct festering_wound_t final : public death_knight_spell_t
{
  festering_wound_t( death_knight_t* p ) :
    death_knight_spell_t( "festering_wound", p, p -> spell.festering_wound_damage )
  {
    background = true;

    base_multiplier *= 1.0 + p -> talent.unholy.bursting_sores -> effectN( 1 ).percent();
    base_multiplier *= 1.0 + p -> talent.unholy.improved_festering_strike -> effectN( 1 ).percent();
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> resource_gain( RESOURCE_RUNIC_POWER,
                        p() -> spec.festering_wound -> effectN( 1 ).trigger() -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER ),
                        p() -> gains.festering_wound, this );

    if ( p() -> talent.unholy.replenishing_wounds.ok() )
    {
      p()->resource_gain(
          RESOURCE_RUNIC_POWER,
          p()->talent.unholy.replenishing_wounds->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
          p()->gains.replenishing_wounds, this );
    }
  }
};

struct festering_strike_t final : public death_knight_melee_attack_t
{
  festering_strike_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_melee_attack_t( "festering_strike", p, p -> talent.unholy.festering_strike )
  {
    parse_options( options_str );

    base_multiplier *= 1.0 + p -> talent.unholy.improved_festering_strike -> effectN( 1 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    static const std::array<unsigned, 2> fw_proc_stacks = { { 2, 3 } };

    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      size_t n = rng().range( size_t(), fw_proc_stacks.size() );
      unsigned n_stacks = fw_proc_stacks[ n ];

      p() -> trigger_festering_wound( s, n_stacks, p() -> procs.fw_festering_strike );
    }

    if ( p() -> talent.unholy.feasting_strikes.ok() )
    {
      if ( rng().roll( p() -> talent.unholy.feasting_strikes -> effectN( 1 ).percent() ) )
      {
        p() -> replenish_rune( as<unsigned int>( p() -> spell.feasting_strikes_gain -> effectN( 1 ).base_value() ), p() -> gains.feasting_strikes );
        p() -> trigger_runic_corruption( p() -> procs.fs_runic_corruption, 0, 1.0, true );
      }
    }
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_melee_attack_t::composite_da_multiplier( state );

    if ( p() -> talent.unholy.reaping.ok() && target -> health_percentage() < p() -> talent.unholy.reaping -> effectN( 2 ).base_value() )
    {
      m *= 1.0 + p() -> talent.unholy.reaping -> effectN( 1 ).percent();
    }

    return m;
  }
};

// Frostscythe ==============================================================

struct frostscythe_t final : public death_knight_melee_attack_t
{
  frostscythe_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_melee_attack_t( "frostscythe", p, p -> talent.frost.frostscythe )
  {
    parse_options( options_str );

    inexorable_assault = get_action<inexorable_assault_damage_t>( "inexorable_assault", p );

    weapon = &( player -> main_hand_weapon );
    aoe = -1;
    reduced_aoe_targets = data().effectN( 5 ).base_value();
    triggers_icecap = true;
    // Crit multipier handled in death_knight_t::apply_affecting_aura()
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    if ( p() -> buffs.inexorable_assault -> up() && p() -> cooldown.inexorable_assault_icd -> is_ready() )
    {
      inexorable_assault -> set_target( target );
      inexorable_assault -> schedule_execute();
      p() -> buffs.inexorable_assault -> decrement();
      p() -> cooldown.inexorable_assault_icd -> start();
    }

    if ( p() -> talent.frost.enduring_strength.ok() && p() -> buffs.pillar_of_frost -> up() &&
          p() -> cooldown.enduring_strength_icd -> is_ready() && s -> result == RESULT_CRIT )
    {
      p() -> buffs.enduring_strength_builder -> trigger();
      p() -> cooldown.enduring_strength_icd -> start();
    }
  }

  double calculate_crit_damage_bonus( action_state_t* s ) const override
  {
    double m = death_knight_melee_attack_t::calculate_crit_damage_bonus( s );

    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T29, B2 ) )
    {
      m *= 1.0 + p() -> sets -> set( DEATH_KNIGHT_FROST, T29, B2 ) -> effectN(1).percent();
    }

    return m;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T29, B4 ) &&
          p() -> buffs.killing_machine -> up() )
    {
      p() -> consume_killing_machine( p() -> procs.killing_machine_fsc );
      p() -> trigger_killing_machine( p() -> sets -> set( DEATH_KNIGHT_FROST, T29, B4 ) -> effectN( 1 ).percent(),
                                      p() -> procs.km_from_t29_4pc, p() -> procs.km_from_t29_4pc_wasted );
    }
    else
    {
      p() -> consume_killing_machine( p() -> procs.killing_machine_fsc );
    }

    // Frostscythe procs rime at half the chance of Obliterate
    p() -> buffs.rime -> trigger( 1, buff_t::DEFAULT_VALUE(), p() -> buffs.rime->manual_chance / 2.0 );

  }

  double composite_crit_chance() const override
  {
    double cc = death_knight_melee_attack_t::composite_crit_chance();

    cc += p() -> buffs.killing_machine -> value();

    return cc;
  }
private:
    propagate_const<action_t*> inexorable_assault;
};

// Frostwyrm's Fury =========================================================
struct frostwyrms_fury_damage_t : public death_knight_spell_t
{
  frostwyrms_fury_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p,  p -> spell.frostwyrms_fury_damage )
  {
    aoe = -1;
    background = true;
    cooldown -> duration = 0_ms; // handled by the actions
  }
  
  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( s );
    
    if( p() -> sets -> has_set_bonus ( DEATH_KNIGHT_FROST, T30, B2 ) )
    {
      m *= 1.0 + p() -> buffs.wrath_of_the_frostwyrm -> check_stack_value();
    }

    return m;
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    p() -> buffs.wrath_of_the_frostwyrm -> expire();
  }

  void impact( action_state_t* s ) override
  {
    if ( p ()->sets->has_set_bonus ( DEATH_KNIGHT_FROST, T30, B4 ) )
    {
      get_td ( s->target )->debuff.lingering_chill->trigger ();
    }
    death_knight_spell_t::impact( s );    
  }
};

struct frostwyrms_fury_t final : public death_knight_spell_t
{
  frostwyrms_fury_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "frostwyrms_fury_driver", p, p -> talent.frost.frostwyrms_fury )
  {
    parse_options( options_str );
    execute_action = get_action<frostwyrms_fury_damage_t>( "frostwyrms_fury", p );
    track_cd_waste = true;

    if ( p -> talent.frost.absolute_zero -> ok() )
    {
      cooldown -> duration *= 1.0 + p -> talent.frost.absolute_zero->effectN( 1 ).percent();
    }
    // Stun is NYI
  }
};

// Frost Strike =============================================================

struct frost_strike_strike_t final : public death_knight_melee_attack_t
{
  bool sb;
  frost_strike_strike_t( death_knight_t* p, util::string_view n, weapon_t* w, const spell_data_t* s, bool shattering_blade ) :
    death_knight_melee_attack_t( n, p, s ), sb( shattering_blade )
  {
    background = special = true;
    weapon = w;
    triggers_icecap = true;
    base_multiplier *= 1.0 + p -> talent.frost.improved_frost_strike -> effectN( 1 ).percent();
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_melee_attack_t::composite_da_multiplier ( state );

    if ( sb )
    {
      m *= 1.0 + p() -> talent.frost.shattering_blade -> effectN( 1 ).percent();
    }

    return m;
  }
  
  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talent.frost.cold_blooded_rage.ok() && s -> result == RESULT_CRIT )
      {
        p() -> trigger_killing_machine( p() -> talent.frost.cold_blooded_rage -> effectN( 2 ).percent(), p() -> procs.km_from_cold_blooded_rage,
                                          p() -> procs.km_from_cold_blooded_rage_wasted );
      }
    }
  }
};

struct frost_strike_t final : public death_knight_melee_attack_t
{
  frost_strike_strike_t *mh, *oh, *mh_sb, *oh_sb;
  bool sb;
  frost_strike_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_melee_attack_t( "frost_strike", p, p -> talent.frost.frost_strike ),
    mh( nullptr ), oh( nullptr ), sb( false ), mh_sb( nullptr ), oh_sb( nullptr )
  {
    parse_options( options_str );
    may_crit = false;
    const spell_data_t* mh_data = p -> main_hand_weapon.group() == WEAPON_2H ?
      data().effectN( 4 ).trigger() : data().effectN( 2 ).trigger();

    dual = true;
    mh = new frost_strike_strike_t(p, "frost_strike", &(p->main_hand_weapon), mh_data, false  );
    add_child( mh );
    if( p -> talent.frost.shattering_blade.ok() )
    {
      mh_sb = new frost_strike_strike_t( p, "frost_strike_sb", &(p->main_hand_weapon), mh_data, true );
      add_child( mh_sb );
    }

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      oh = new frost_strike_strike_t( p, "frost_strike_offhand", &(p->off_hand_weapon), data().effectN(3).trigger(), false );
      add_child( oh );
      if( p -> talent.frost.shattering_blade.ok() )
      {
        oh_sb = new frost_strike_strike_t( p, "frost_strike_offhand_sb", &(p->off_hand_weapon), data().effectN(3).trigger(), true );
        add_child( oh_sb );
      }
    }
  }

  void execute() override
  {
    const auto td = get_td( target );

    if( p() -> talent.frost.shattering_blade.ok() && td -> debuff.razorice -> at_max_stacks() )
    {
      sb = true;
      td -> debuff.razorice -> expire();
    }

    death_knight_melee_attack_t::execute();

    if ( hit_any_target )
    {
      if( !sb )
      {
        mh -> execute_on_target( target );
        if ( oh )
        {
          oh -> execute_on_target( target );
        }
      }
      if( sb )
      {
        mh_sb -> execute_on_target( target );
        if ( oh_sb )
        {
          oh_sb -> execute_on_target( target );
        }
        sb = false;
      }
    }

    if ( p() -> buffs.pillar_of_frost -> up() && p() -> talent.frost.obliteration.ok() )
    {
      p() -> trigger_killing_machine( 1.0, p() -> procs.km_from_obliteration_fs,
                                           p() -> procs.km_from_obliteration_fs_wasted );

      // Obliteration's rune generation
      if ( rng().roll( p() -> talent.frost.obliteration -> effectN( 2 ).percent() ) )
      {
        p() -> replenish_rune( as<int>(p() -> spell.obliteration_gains -> effectN( 1 ).base_value()), p()->gains.obliteration);
      }
    }
  }
};

// Glacial Advance ==========================================================

struct glacial_advance_damage_t final : public death_knight_spell_t
{
  glacial_advance_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> spell.glacial_advance_damage )
  {
    aoe = -1; // TODO: Fancier targeting .. make it aoe for now
    background = true;
    ap_type = attack_power_type::WEAPON_BOTH;
    if ( p -> main_hand_weapon.group() == WEAPON_2H )
    {
      ap_type = attack_power_type::WEAPON_MAINHAND;
      // There's a 0.98 modifier hardcoded in the tooltip if a 2H weapon is equipped, probably server side magic
      base_multiplier *= 0.98;
    }
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    // Only applies the razorice debuff without the damage, regardless of runeforge equipped
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/663
    if ( p() -> bugs || p() -> runeforge.rune_of_razorice_mh || p() -> runeforge.rune_of_razorice_oh )
    {
      get_td( state -> target ) -> debuff.razorice -> trigger();
    }
  }
};

struct glacial_advance_t final : public death_knight_spell_t
{
  glacial_advance_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "glacial_advance", p, p -> talent.frost.glacial_advance )
  {
    parse_options( options_str );

    weapon = &( p -> main_hand_weapon );

    execute_action = get_action<glacial_advance_damage_t>( "glacial_advance_damage", p );
    add_child( execute_action );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.pillar_of_frost -> up() && p() -> talent.frost.obliteration.ok() )
    {
      p() -> trigger_killing_machine( 1.0, p() -> procs.km_from_obliteration_ga,
                                           p() -> procs.km_from_obliteration_ga_wasted );

      // Obliteration's rune generation
      if ( rng().roll( p() -> talent.frost.obliteration -> effectN( 2 ).percent() ) )
      {
        p() -> replenish_rune( as<int>(p() -> spell.obliteration_gains -> effectN( 1 ).base_value()), p() -> gains.obliteration );
      }
    }
  }
};

// Gorefiend's Grasp ========================================================

struct gorefiends_grasp_t final : public death_knight_spell_t
{
  gorefiends_grasp_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "gorefiends_grasp", p, p -> talent.blood.gorefiends_grasp )
  {
    parse_options( options_str );
    aoe = -1;
    track_cd_waste = true;
    cooldown->duration += p -> talent.blood.tightening_grasp -> effectN( 1 ).time_value();
  }

  void impact ( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( p() -> talent.blood.tightening_grasp.ok() )
    {
      auto td = get_td( state -> target );
      td -> debuff.tightening_grasp -> trigger();
    }
  }

};

// Heart Strike =============================================================

struct leeching_strike_t final : public death_knight_heal_t
{
  leeching_strike_t( util::string_view name, death_knight_t* p ) :
    death_knight_heal_t( name, p, p -> spell.leeching_strike_damage )
  {
    background = true;
    may_crit = callbacks = false;
    target = p;
    base_pct_heal = data().effectN( 1 ).percent();
  }
private:
    propagate_const<action_t*> damage;
};

struct heart_strike_t final : public death_knight_melee_attack_t
{
  double heartbreaker_rp_gen;

  heart_strike_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_melee_attack_t( "heart_strike", p, p -> talent.blood.heart_strike ),
    heartbreaker_rp_gen( p -> talent.blood.heartbreaker -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) )
  {
    parse_options( options_str );
    aoe = 2;
    weapon = &( p -> main_hand_weapon );
    base_multiplier *= 1.0 + p -> talent.blood.improved_heart_strike -> effectN( 1 ).percent();
    leeching_strike = get_action<leeching_strike_t>("leeching_strike", p);
    if ( p -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T30, B2 ) )
    {
      base_multiplier *= 1.0 + p -> sets -> set( DEATH_KNIGHT_BLOOD, T30, B2 ) -> effectN(1).percent();
    }
  }

  int n_targets() const override
  { return p() -> in_death_and_decay() ? aoe + as<int>( p() -> talent.cleaving_strikes -> effectN( 3 ).base_value() ) : aoe; }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( p() -> talent.blood.heartrend.ok() )
    {
      p() -> buffs.heartrend -> trigger();
    }

    if ( p() -> buffs.dancing_rune_weapon -> up() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.heart_strike -> execute_on_target( target );

      if ( p() -> talent.blood.everlasting_bond.ok() )
        p() -> pets.everlasting_bond_pet -> ability.heart_strike -> execute_on_target( target );
    }

    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T30, B2 ) )
    {
      if ( p() -> rng().roll( p() -> sets -> set( DEATH_KNIGHT_BLOOD, T30, B2 )->effectN( 2 ).percent() ) )
      {
        // If vamp str is up, we proc an extension, or if vamp str and vamp blood are down, we proc.  We do not proc if vamp str is down, but vamp blood is still up
        if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T30, B4 ) )
        {
          if ( p() -> buffs.vampiric_strength -> up() || ( !p() -> buffs.vampiric_blood -> up() && !p() -> buffs.vampiric_strength -> up() ) )
          {
            p() -> buffs.vampiric_strength -> extend_duration_or_trigger();
          }
        }

        p() -> buffs.vampiric_blood -> extend_duration_or_trigger( p() -> sets -> set( DEATH_KNIGHT_BLOOD, T30, B2 )->effectN( 3 ).time_value() );
      }
    }

    if( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T30, B4 ) && p() -> buffs.vampiric_strength -> up() )
    {
      p() -> buffs.vampiric_strength -> extend_duration( p(), p() -> sets -> set( DEATH_KNIGHT_BLOOD, T30, B4 )->effectN( 1 ).time_value() );
    }
  }

  void impact ( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    if ( p() -> talent.blood.heartbreaker.ok() && result_is_hit( state -> result ) )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER, heartbreaker_rp_gen, p() -> gains.heartbreaker, this );
    }

    if ( p() -> talent.blood.leeching_strike.ok() && get_td( state -> target ) -> dot.blood_plague -> is_ticking() )
    {
      leeching_strike -> execute();
    }
  }
private:
    propagate_const<action_t*> leeching_strike;
};

// Horn of Winter ===========================================================

struct horn_of_winter_t final : public death_knight_spell_t
{
  horn_of_winter_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "horn_of_winter", p, p -> talent.frost.horn_of_winter )
  {
    parse_options( options_str );
    harmful = false;
    track_cd_waste = true;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> resource_gain( RESOURCE_RUNIC_POWER, data().effectN( 2 ).resource( RESOURCE_RUNIC_POWER ),
        p() -> gains.horn_of_winter, this );

    p() -> replenish_rune( as<unsigned int>( data().effectN( 1 ).base_value() ), p() -> gains.horn_of_winter );
  }
};

// Howling Blast ============================================================

struct avalanche_t final : public death_knight_spell_t
{
  avalanche_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> spell.avalanche_damage )
  {
    aoe = -1;
    background = true;
  }

  void impact(action_state_t* state) override
  {
    death_knight_spell_t::impact( state );

    get_td( state -> target ) -> debuff.razorice -> trigger();
  }
};

struct howling_blast_t final : public death_knight_spell_t
{
  howling_blast_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "howling_blast", p, p -> talent.frost.howling_blast )
  {
    parse_options( options_str );

    aoe = -1;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;

    impact_action = get_action<frost_fever_t>( "frost_fever", p );

    ap_type = attack_power_type::WEAPON_BOTH;
    if ( p -> main_hand_weapon.group() == WEAPON_2H )
    {
      ap_type = attack_power_type::WEAPON_MAINHAND;
      // There's a 0.98 modifier hardcoded in the tooltip if a 2H weapon is equipped, probably server side magic
      base_multiplier *= 0.98;
    }

    if ( p -> talent.frost.avalanche.ok() )
    {
      avalanche = get_action<avalanche_t>( "avalanche", p );
      add_child( avalanche );
    }
  }

  double runic_power_generation_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::runic_power_generation_multiplier( state );

    if ( p() -> buffs.rime -> check() )
    {
      m *= 1.0 + p() -> buffs.rime -> data().effectN( 3 ).percent();
    }

    return m;
  }

  double action_multiplier() const override
  {
    double m = death_knight_spell_t::action_multiplier();

    if ( p() -> buffs.rime -> up() )
    {
      m *= 1.0 + p()->buffs.rime->data().effectN( 2 ).percent() + p() -> talent.frost.improved_rime -> effectN( 1 ).percent();
    }

    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T30, B2 ) )
    {
      m *= 1.0 + p() -> spell.frost_t30_2pc -> effectN( 1 ).percent();
    }

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = death_knight_spell_t::composite_target_multiplier( t );

    if ( p() -> talent.frost.icebreaker.ok() && this -> target == t )
    {
      m *= 1.0 + p() -> talent.frost.icebreaker->effectN( 1 ).percent();
    }

    return m;
  }

  double cost() const override
  {
    if ( p() -> buffs.rime -> check() )
    {
      return 0;
    }

    return death_knight_spell_t::cost();
  }

  void schedule_execute( action_state_t* state ) override
  {
    // If we have rime, and no runes left, and rime is expiring on the same ms timestamp as we would howling blast,
    // do not queue howling blast.  This avoids a crash/assert where the buff would get removed during the actions
    // executing, before howling blast goes off.  However, if we do have runes left, we will allow howling blast to
    // get fired off, to more accurately reflect what a player would do
    if ( p() -> buffs.rime -> check() && p() -> _runes.runes_full() == 0 && p() -> buffs.rime -> remains() == 0_ms )
    {
      sim->print_debug( "{} action={} attempted to schedule howling blast on the same tick rime expires {}",
        *player, *this, *target );

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

    // If Pillar of Frost is up, Rime procs still increases its value
    if ( p() -> buffs.pillar_of_frost -> up() && p() -> buffs.rime -> up() )
    {
      p() -> buffs.pillar_of_frost_bonus -> trigger( as<int>( base_costs[ RESOURCE_RUNE ] ) );
    }

    if ( p() -> buffs.pillar_of_frost -> up() && p() -> talent.frost.obliteration.ok() )
    {
      p() -> trigger_killing_machine( 1.0, p() -> procs.km_from_obliteration_hb,
                                           p() -> procs.km_from_obliteration_hb_wasted );

      // Obliteration's rune generation
      if ( rng().roll( p() -> talent.frost.obliteration -> effectN( 2 ).percent() ) )
      {
        p() -> replenish_rune( as<int>(p() -> spell.obliteration_gains -> effectN( 1 ).base_value()), p() -> gains.obliteration );
      }
    }

    if ( p() -> talent.frost.avalanche.ok() && p() -> buffs.rime -> up() )
      avalanche -> execute_on_target( target );
	
    if ( p() -> buffs.rime -> check() && p() -> talent.frost.rage_of_the_frozen_champion.ok() )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER,
                            p() -> spell.rage_of_the_frozen_champion -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
                            p() -> gains.rage_of_the_frozen_champion );
    }

    if ( p() -> buffs.rime -> check() &&  p() -> sets -> has_set_bonus ( DEATH_KNIGHT_FROST, T30, B2 ) )
    {
      p() -> buffs.wrath_of_the_frostwyrm -> trigger();
    }

    p() -> buffs.rime -> decrement();
  }
private:
    propagate_const<action_t*> avalanche;
};

// Marrowrend ===============================================================

struct marrowrend_t final : public death_knight_melee_attack_t
{
  marrowrend_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_melee_attack_t( "marrowrend", p, p -> talent.blood.marrowrend )
  {
    parse_options( options_str );
    weapon = &( p -> main_hand_weapon );
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> up() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.marrowrend -> execute_on_target( target );

      if ( p() -> talent.blood.everlasting_bond.ok() )
        p() -> pets.everlasting_bond_pet -> ability.marrowrend -> execute_on_target( target );
    }
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    p() -> buffs.bone_shield -> trigger( as<int>( data().effectN( 3 ).base_value() ) );
  }
};

// Mind Freeze ==============================================================

struct mind_freeze_t final : public death_knight_spell_t
{
  mind_freeze_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "mind_freeze", p, p -> find_class_spell( "Mind Freeze" ) )
  {
    parse_options( options_str );
    ignore_false_positive = is_interrupt = true;
    track_cd_waste = true;
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p()->talent.coldthirst.ok() )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER, 
          p() -> spell.coldthirst_gain -> effectN( 1 ).base_value() / 10,
          p() -> gains.coldthirst, this );

      p() -> cooldown.mind_freeze -> adjust( p() -> spell.coldthirst_gain -> effectN( 2 ).time_value() );

    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( ! candidate_target -> debuffs.casting || ! candidate_target -> debuffs.casting -> check() )
      return false;

    return death_knight_spell_t::target_ready( candidate_target );
  }
};

// Obliterate ===============================================================

struct obliterate_strike_t final : public death_knight_melee_attack_t
{
  int cleaving_strikes_targets;
  obliterate_strike_t( death_knight_t* p, util::string_view name,
                       weapon_t* w, const spell_data_t* s ) :
    death_knight_melee_attack_t( name, p, s )
  {
    background = special = true;
    may_miss = false;
    weapon = w;
    triggers_icecap = true;

    base_multiplier *= 1.0 + p -> talent.frost.frigid_executioner -> effectN ( 2 ).percent();
    base_multiplier *= 1.0 + p -> talent.frost.improved_obliterate -> effectN( 1 ).percent();
          
    if ( p -> spec.might_of_the_frozen_wastes -> ok() && p -> main_hand_weapon.group() == WEAPON_2H )
    {
      base_multiplier *= 1.0 + ( p -> spec.might_of_the_frozen_wastes -> effectN( 1 ).percent() );
    }


    // To support Cleaving strieks affecting Obliterate in Dragonflight:
    // - obliterate damage spells have gained a value of 1 in their chain target data
    // - the death and decay buff now has an effect that modifies obliterate's chain target with a value of 0
    // - cleaving strikes increases the aforementionned death and decay buff effect by 1
    cleaving_strikes_targets = data().effectN ( 1 ).chain_target() +
                                as<int>( p -> spell.dnd_buff -> effectN ( 4 ).base_value() ) +
                                as<int>( p -> talent.cleaving_strikes -> effectN( 2 ).base_value() );

    inexorable_assault = get_action<inexorable_assault_damage_t>( "inexorable_assault", p );
  }

  int n_targets() const override
  {
      if ( p() -> in_death_and_decay() )
      {
        if( p() -> talent.cleaving_strikes.ok() )
          return cleaving_strikes_targets;
      }

      return death_knight_melee_attack_t::n_targets();
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_melee_attack_t::composite_da_multiplier( state );
    // Obliterate does not list Frozen Heart in it's list of affecting spells.  So mastery does not get applied automatically.
    if ( p() -> spec.frostreaper->ok() && p() -> buffs.killing_machine -> up() )
    {
      m *= 1.0 + p() -> cache.mastery_value();
    }    

    return m;
  }

  double calculate_crit_damage_bonus( action_state_t* s ) const override
  {
    double m = death_knight_melee_attack_t::calculate_crit_damage_bonus( s );

    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T29, B2 ) )
    {
      m *= 1.0 + p() -> sets -> set(DEATH_KNIGHT_FROST, T29, B2 ) -> effectN( 1 ).percent();
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    const death_knight_td_t* td = get_td( target );
    // Obliterate does not list razorice in it's list of affecting spells, so debuff does not get applied automatically.
    if ( td && p() -> spec.frostreaper -> ok() && p() -> buffs.killing_machine -> up() )
    {
      m *= 1.0 + td -> debuff.razorice -> check_stack_value();
    }

    return m;
  }

  double composite_crit_chance() const override
  {
    double cc = death_knight_melee_attack_t::composite_crit_chance();

    cc += p() -> buffs.killing_machine -> value();

    return cc;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    if ( p()->talent.frost.enduring_strength.ok() && p()->buffs.pillar_of_frost->up() &&
         p()->cooldown.enduring_strength_icd->is_ready() && state->result == RESULT_CRIT )
    {
      p()->buffs.enduring_strength_builder->trigger();
      p()->cooldown.enduring_strength_icd->start();
    }

    if ( p() -> buffs.inexorable_assault -> up() && p() -> cooldown.inexorable_assault_icd -> is_ready() )
    {
      inexorable_assault -> set_target( target );
      inexorable_assault -> schedule_execute();
      p() -> buffs.inexorable_assault -> decrement();
      p() -> cooldown.inexorable_assault_icd -> start();
    }
  }

  void execute() override
  {
    if ( ! p() -> options.split_obliterate_schools && p() -> spec.frostreaper -> ok() && p() -> buffs.killing_machine -> up() )
    {
      school = SCHOOL_FROST;
    }

    death_knight_melee_attack_t::execute();

    // Improved Killing Machine - revert school after the hit
    if ( ! p() -> options.split_obliterate_schools ) school = SCHOOL_PHYSICAL;

  }
private:
    propagate_const<action_t*> inexorable_assault;
};

struct obliterate_t final : public death_knight_melee_attack_t
{
  obliterate_strike_t *mh, *oh, *km_mh, *km_oh;

  obliterate_t( death_knight_t* p, util::string_view options_str = {} ) :
    death_knight_melee_attack_t( "obliterate", p, p -> talent.frost.obliterate ),
    mh( nullptr ), oh( nullptr ), km_mh( nullptr ), km_oh( nullptr )
  {
    parse_options( options_str );
    dual = true;

    const spell_data_t* mh_data = p -> main_hand_weapon.group() == WEAPON_2H ? data().effectN( 4 ).trigger() : data().effectN( 2 ).trigger();

    mh = new obliterate_strike_t( p, "obliterate", &( p -> main_hand_weapon ), mh_data );
    add_child( mh );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      oh = new obliterate_strike_t( p, "obliterate_offhand", &( p -> off_hand_weapon ), data().effectN( 3 ).trigger() );
      add_child( oh );
    }
    if ( p -> options.split_obliterate_schools && p -> spec.frostreaper -> ok() )
    {
      km_mh = new obliterate_strike_t( p, "obliterate_km", &( p -> main_hand_weapon ), mh_data );
      km_mh -> school = SCHOOL_FROST;
      add_child( km_mh );
      if ( p -> off_hand_weapon.type != WEAPON_NONE )
      {
        km_oh = new obliterate_strike_t( p, "obliterate_offhand_km", &( p -> off_hand_weapon ), data().effectN( 3 ).trigger() );
        km_oh -> school = SCHOOL_FROST;
        add_child( km_oh );
      }
    }
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( hit_any_target )
    {
      if ( p() -> talent.frost.frigid_executioner.ok() && p() -> cooldown.frigid_executioner_icd->is_ready() )
      {
        if ( p() -> rng().roll(p() -> talent.frost.frigid_executioner->proc_chance()))
        {
          // # of runes to restore was stored in a secondary affect
          p() -> replenish_rune( as<unsigned int>( p() -> talent.frost.frigid_executioner->effectN( 1 ).trigger()->effectN( 1 ).base_value() ), p() -> gains.frigid_executioner );
          p() -> cooldown.frigid_executioner_icd -> start();
        }
      }
      if ( km_mh && p() -> buffs.killing_machine -> up() )
      {
        km_mh -> execute_on_target( target );
        if ( oh && km_oh )

          km_oh -> execute_on_target( target );
      }
      else
      {
        mh -> execute_on_target( target );
        if ( oh )
          oh -> execute_on_target( target );
      }

      p() -> buffs.rime -> trigger();
    }

    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T29, B4 ) &&
          p() -> buffs.killing_machine -> up() )
    {
      p() -> consume_killing_machine( p() -> procs.killing_machine_oblit );
      p() -> trigger_killing_machine( p() -> sets -> set( DEATH_KNIGHT_FROST, T29, B4 ) -> effectN( 1 ).percent(),
                                      p() -> procs.km_from_t29_4pc, p() -> procs.km_from_t29_4pc_wasted );
    }
    else
    {
      p() -> consume_killing_machine( p() -> procs.killing_machine_oblit );
    }
  }

  double cost() const override
  {
    double c = death_knight_melee_attack_t::cost();

    if ( c < 0 )
    {
      c = 0;
    }

    return c;
  }

  // Allow on-cast procs
  bool has_amount_result() const override
  { return true; }
};

// Outbreak ================================================================
struct outbreak_aoe_t final : public death_knight_spell_t
{
  outbreak_aoe_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p , p -> spell.outbreak_aoe ),
      vp( get_action<virulent_plague_t>( "virulent_plague", p ) )
  {
    impact_action = vp;
    aoe = -1;
    radius = data().effectN(1).radius_max();
    background = true;
  }
private:
    propagate_const<action_t*> vp;
};

struct outbreak_t final : public death_knight_spell_t
{
  outbreak_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "outbreak", p , p -> talent.unholy.outbreak ),
      outbreak_aoe( get_action<outbreak_aoe_t>( "outbreak_aoe", p ) )
  {
    parse_options( options_str );
    impact_action = outbreak_aoe;
  }
private:
    propagate_const<action_t*> outbreak_aoe;
};

// Rune of Apocalpyse - Pestilence ==========================================
struct runeforge_apocalypse_pestilence_t final : public death_knight_spell_t
{
  runeforge_apocalypse_pestilence_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> spell.apocalypse_pestilence_damage )
    {
      base_dd_multiplier *= 1.0 + p -> talent.unholy_bond -> effectN( 1 ).percent();
    }
};

// Pillar of Frost ==========================================================

struct frostwhelps_aid_t final : public death_knight_spell_t
{
  frostwhelps_aid_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p , p -> spell.frostwhelps_aid_damage )
  {
    aoe = -1;
    background = true;
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );
    p() -> buffs.frostwhelps_aid -> trigger();
  }
};

struct wrath_of_the_frostwyrm_damage_t : public death_knight_spell_t
{
  wrath_of_the_frostwyrm_damage_t( util::string_view name, death_knight_t* p) :
    death_knight_spell_t( name, p,  p -> spell.wrath_of_the_frostwyrm_damage )
  {
    aoe = -1;
    background = true;
    cooldown -> duration = 0_ms; // handled by the actions
    base_dd_multiplier = p -> spell.frost_t30_2pc -> effectN( 2 ).percent();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( s );
    
    if( p() -> sets -> has_set_bonus ( DEATH_KNIGHT_FROST, T30, B2 ) )
    {
      m *= 1.0 + p() -> buffs.wrath_of_the_frostwyrm -> check_stack_value();
    }

    return m;
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    p() -> buffs.wrath_of_the_frostwyrm -> expire();
  }

  void impact( action_state_t* s ) override
  {
    if ( p ()->sets->has_set_bonus ( DEATH_KNIGHT_FROST, T30, B4 ) )
    {
      get_td ( s->target )->debuff.lingering_chill->trigger ();
    }
    death_knight_spell_t::impact( s );
  }
};

struct pillar_of_frost_t final : public death_knight_spell_t
{
  pillar_of_frost_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "pillar_of_frost", p, p -> talent.frost.pillar_of_frost )
  {
    parse_options( options_str );

    harmful = false;
    track_cd_waste = true;

    if( p -> sets -> has_set_bonus ( DEATH_KNIGHT_FROST, T30, B2 ) )
    {
      t30_frostwyrm = get_action<wrath_of_the_frostwyrm_damage_t>( "frostwyrms_fury_t30", p );
      add_child( t30_frostwyrm );
    }

    if ( p -> talent.frost.frostwhelps_aid.ok() )
    {
      whelp = get_action<frostwhelps_aid_t>( "frostwhelps_aid", p );
      add_child( whelp );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    if ( p() -> talent.frost.frostwhelps_aid.ok() )
    {
      whelp -> execute();
    }
    if( p() -> sets -> has_set_bonus ( DEATH_KNIGHT_FROST, T30, B2 ) )
    {
      t30_frostwyrm->execute();
    }
    p() -> buffs.pillar_of_frost -> trigger();
  }
private:
    propagate_const<action_t*> whelp;
    propagate_const<action_t*> t30_frostwyrm;
};

// Vile Contagion =============================================================

struct vile_contagion_t final : public death_knight_spell_t
{
  vile_contagion_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "vile_contagion", p, p -> talent.unholy.vile_contagion )
  {
    parse_options( options_str );
    aoe = as<int>(p->talent.unholy.vile_contagion->effectN(1).base_value());
    track_cd_waste = true;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
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

    if ( result_is_hit( s -> result ) && get_td( target ) -> debuff.festering_wound->stack() > 0  )
    {
      unsigned n_stacks = get_td( target )->debuff.festering_wound->stack();

      p() -> trigger_festering_wound( s, n_stacks, p() -> procs.fw_vile_contagion );
    }
  }
};

// Raise Dead ===============================================================

struct raise_dead_t final : public death_knight_spell_t
{
  raise_dead_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "raise_dead", p, p -> talent.unholy.raise_dead.ok() ?
                          p -> talent.unholy.raise_dead : p -> talent.raise_dead )
  {
    parse_options( options_str );

    cooldown -> duration += p -> talent.unholy.all_will_serve -> effectN( 1 ).time_value();

    harmful = false;
    track_cd_waste = true;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    // If the action is done in precombat and the pet is permanent
    // Assume that the player used it long enough before pull that the cooldown is ready again
    if ( is_precombat && p() -> talent.unholy.raise_dead.ok() )
    {
      cooldown -> reset( false );
      p() -> pets.ghoul_pet -> precombat_spawn = true;
    }

    // Summon for the duration specified in spelldata if there's one (no data = permanent pet)
    p() -> pets.ghoul_pet -> summon( data().duration() );

    // Sacrificial Pact doesn't despawn risen skulker, so make sure it's not already up before spawning it
    if ( p() -> talent.unholy.all_will_serve.ok() && p() -> pets.risen_skulker -> is_sleeping() )
    {
      p() -> pets.risen_skulker -> summon( 0_ms );
    }
  }

  bool ready() override
  {
    if ( p() -> pets.ghoul_pet && ! p() -> pets.ghoul_pet -> is_sleeping() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Remorseless Winter =======================================================

struct remorseless_winter_damage_t final : public death_knight_spell_t
{
  double biting_cold_target_threshold;
  bool triggered_biting_cold;

  remorseless_winter_damage_t( death_knight_t* p ) :
    death_knight_spell_t( "remorseless_winter_damage", p, p -> spec.remorseless_winter -> effectN( 2 ).trigger() ),
    biting_cold_target_threshold( 0 ), triggered_biting_cold( false )
  {
    background = true;
    aoe = -1;

    ap_type = attack_power_type::WEAPON_BOTH;

    if ( p -> main_hand_weapon.group() == WEAPON_2H )
    {
      ap_type = attack_power_type::WEAPON_MAINHAND;
      // There's a 0.98 modifier hardcoded in the tooltip if a 2H weapon is equipped, probably server side magic
      base_multiplier *= 0.98;
    }

    if ( p -> talent.frost.biting_cold.ok() )
    {
      biting_cold_target_threshold = p -> talent.frost.biting_cold -> effectN ( 1 ).base_value();
      base_multiplier *= 1.0 + p -> talent.frost.biting_cold -> effectN( 2 ).percent();
    }
  }

  double action_multiplier() const override
  {
    double m = death_knight_spell_t::action_multiplier();

    m *= 1.0 + p() -> buffs.gathering_storm -> stack_value();

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = death_knight_spell_t::composite_target_multiplier( t );

    if ( auto td = get_td( t ) )
      m *= 1.0 + td -> debuff.everfrost -> check_stack_value();

    return m;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( state -> n_targets >= biting_cold_target_threshold && p() -> talent.frost.biting_cold.ok() && ! triggered_biting_cold )
    {
      p() -> buffs.rime -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
      triggered_biting_cold = true;
    }

    if ( p() -> talent.frost.everfrost.ok() )
      get_td( state -> target ) -> debuff.everfrost -> trigger();
  }
};

struct remorseless_winter_buff_t final : public buff_t
{
  remorseless_winter_damage_t* damage; // (AOE) damage that ticks every second

  remorseless_winter_buff_t( death_knight_t* p ) :
    buff_t( p, "remorseless_winter", p -> spec.remorseless_winter ),
    damage( new remorseless_winter_damage_t( p ) )
  {
    cooldown -> duration = 0_ms; // Controlled by the action
    set_refresh_behavior( buff_refresh_behavior::DURATION );
    set_tick_callback( [ this ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ )
    {
      damage -> execute();
    } );
    set_partial_tick( true );
    set_stack_change_callback( [ this, p ] ( buff_t*, int, int new_ ) 
    {
      if ( !new_ )
      {
        p -> buffs.gathering_storm -> expire();
      }
    } );
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    // TODO: check whether refreshing RW (with GS and high haste) lets you trigger biting cold again
    // Move to expire override if necessary
    debug_cast<remorseless_winter_damage_t*>( damage ) -> triggered_biting_cold = false;
    return buff_t::trigger( s, v, c, d );
  }
};

struct remorseless_winter_t final : public death_knight_spell_t
{
  remorseless_winter_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "remorseless_winter", p, p -> spec.remorseless_winter )
  {
    may_crit = may_miss = may_dodge = may_parry = false;

    parse_options( options_str );

    // Periodic behavior handled by the buff
    dot_duration = base_tick_time = 0_ms;
    track_cd_waste = true;

    if ( action_t* rw_damage = p -> find_action( "remorseless_winter_damage" ) )
    {
      add_child( rw_damage );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.remorseless_winter -> trigger();
  }
};

// Sacrificial Pact =========================================================

struct sacrificial_pact_damage_t final : public death_knight_spell_t
{
  sacrificial_pact_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> spell.sacrificial_pact_damage )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
    track_cd_waste = true;
  }
};

struct sacrificial_pact_t final : public death_knight_heal_t
{
  sacrificial_pact_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_heal_t( "sacrificial_pact", p, p -> talent.sacrificial_pact )
  {
    target = p;
    base_pct_heal = data().effectN( 1 ).percent();
    parse_options( options_str );
    damage = get_action<sacrificial_pact_damage_t>( "sacrificial_pact_damage", p );
  }

  void execute() override
  {
    death_knight_heal_t::execute();

    damage -> execute_on_target( player -> target );

    p() -> pets.ghoul_pet -> dismiss();
  }

  bool ready() override
  {
    if ( p() -> pets.ghoul_pet -> is_sleeping() )
      return false;

    return death_knight_heal_t::ready();
  }
private:
    propagate_const<action_t*> damage;
};

// Scourge Strike and Clawing Shadows =======================================

struct scourge_strike_base_t : public death_knight_melee_attack_t
{
  int dnd_cleave_targets; // For when in dnd how many targets we can cleave
  scourge_strike_base_t( util::string_view name, death_knight_t* p, const spell_data_t* spell ) :
    death_knight_melee_attack_t( name, p, spell ),
    dnd_cleave_targets( as<int>(p -> talent.unholy.scourge_strike -> effectN( 4 ).base_value()) )
  {
    weapon = &( player -> main_hand_weapon );
  }

  // The death and decay target cap is displayed both in scourge strike's effects
  // And in SS and CS' max_targets data entry. Using the latter
  int n_targets() const override
  {
    if ( p() -> talent.cleaving_strikes.ok() )
      return p() -> in_death_and_decay() ? dnd_cleave_targets : 0;
    return 0;
  }

  std::vector<player_t*>& target_list() const override // smart targeting to targets with wounds when cleaving SS
  {
    std::vector<player_t*>& current_targets = death_knight_melee_attack_t::target_list();
    // Don't bother ordering the list if all the valid targets will be hit
    if ( current_targets.size() <= as<size_t>( n_targets() ) )
      return current_targets;

    // first target, the action target, needs to be left in place
    std::sort( current_targets.begin() + 1, current_targets.end(),
      [this]( player_t* a, player_t* b) {
        return get_td( a ) -> debuff.festering_wound -> up() && ! get_td( b ) -> debuff.festering_wound -> up();
      } );

    return current_targets;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( t );

    if ( get_td( t ) -> debuff.rotten_touch->up() )
    {
      m *= 1.0 + p() -> spell.rotten_touch_debuff -> effectN( 1 ).percent();
    }

    return m;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_melee_attack_t::composite_da_multiplier( state );

    if ( p() -> talent.unholy.reaping.ok() && target -> health_percentage() < p() -> talent.unholy.reaping -> effectN( 2 ).base_value() )
    {
      m *= 1.0 + p() -> talent.unholy.reaping -> effectN( 1 ).percent();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    p() -> burst_festering_wound( state -> target, 1 );

    if ( p() -> talent.unholy.plaguebringer.ok() )
    {
      p()->buffs.plaguebringer->trigger();
    }
  }
};

struct clawing_shadows_t final : public scourge_strike_base_t
{
  clawing_shadows_t( death_knight_t* p, util::string_view options_str ) :
    scourge_strike_base_t( "clawing_shadows", p, p -> talent.unholy.clawing_shadows )
  {
    parse_options( options_str );
  }
};

struct scourge_strike_shadow_t final : public death_knight_melee_attack_t
{
  scourge_strike_shadow_t( util::string_view name, death_knight_t* p ) :
    death_knight_melee_attack_t( name, p, p -> talent.unholy.scourge_strike -> effectN( 3 ).trigger() )
  {
    may_miss = may_parry = may_dodge = false;
    background = proc = dual = true;
    weapon = &( player -> main_hand_weapon );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( t );

    if ( get_td( t ) -> debuff.rotten_touch -> up() )
    {
      m *= 1.0 + p() -> spell.rotten_touch_debuff -> effectN( 1 ).percent();
    }

    return m;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_melee_attack_t::composite_da_multiplier( state );

    if ( p() -> talent.unholy.reaping.ok() && target -> health_percentage() < p() -> talent.unholy.reaping -> effectN( 2 ).base_value() )
    {
      m *= 1.0 + p() -> talent.unholy.reaping -> effectN( 1 ).percent();
    }
    
    return m;
  }
};

struct scourge_strike_t final : public scourge_strike_base_t
{
  scourge_strike_t( death_knight_t* p, util::string_view options_str ) :
    scourge_strike_base_t( "scourge_strike", p, p -> talent.unholy.scourge_strike )
  {
    parse_options( options_str );

    impact_action = get_action<scourge_strike_shadow_t>( "scourge_strike_shadow", p );
    add_child( impact_action );

    // Disable when Clawing Shadows is talented
    if ( p -> talent.unholy.clawing_shadows.ok() )
      background = true;
  }
};

// Shattering Bone ==========================================================

struct shattering_bone_t final : public death_knight_spell_t
{
  double boneshield_charges_consumed;
  shattering_bone_t( death_knight_t* p ) :
    death_knight_spell_t( "shattering bone", p, p -> spell.shattering_bone_damage )
    {
      background = true;
      aoe = -1;
      base_multiplier *= 1.0 + p -> talent.blood.shattering_bone -> effectN( 2 ).percent();
      boneshield_charges_consumed = 1.0;
    }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( state );

    if ( p() -> in_death_and_decay() )
    {
      m *= p() -> talent.blood.shattering_bone -> effectN( 1 ).base_value();
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
};

// Soul Reaper ==============================================================

struct soul_reaper_execute_t : public death_knight_spell_t
{
  soul_reaper_execute_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> spell.soul_reaper_execute )
  {
    background = true;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( state );

    if ( p() -> talent.unholy.reaping.ok() && target -> health_percentage() < p() -> talent.unholy.reaping -> effectN( 2 ).base_value() )
    {
      m *= 1.0 + p() -> talent.unholy.reaping -> effectN( 1 ).percent();
    }

    return m;
  }
};

struct soul_reaper_t final : public death_knight_melee_attack_t
{
  soul_reaper_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_melee_attack_t( "soul_reaper", p, p -> talent.soul_reaper ),
    soul_reaper_execute( get_action<soul_reaper_execute_t>( "soul_reaper_execute", p ) )
  {
    parse_options( options_str );
    add_child( soul_reaper_execute );

    hasted_ticks = false;
    dot_behavior = DOT_EXTEND;
    track_cd_waste = true;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_melee_attack_t::composite_da_multiplier( state );

    if ( p() -> talent.unholy.reaping.ok() && target -> health_percentage() < p() -> talent.unholy.reaping -> effectN( 2 ).base_value() )
    {
      m *= 1.0 + p() -> talent.unholy.reaping -> effectN( 1 ).percent();
    }

    return m;
  }

  void tick( dot_t* dot ) override
  {
    if ( dot -> target -> health_percentage() < data().effectN( 3 ).base_value() )
      soul_reaper_execute -> execute_on_target ( dot -> target );
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();
    if ( p() -> specialization() == DEATH_KNIGHT_BLOOD && p() -> buffs.dancing_rune_weapon -> up() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.soul_reaper -> execute_on_target( target );

      if ( p() -> talent.blood.everlasting_bond.ok() )
        p() -> pets.everlasting_bond_pet -> ability.soul_reaper -> execute_on_target( target );
    }
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    if ( p() -> specialization() == DEATH_KNIGHT_FROST && p() -> buffs.pillar_of_frost -> up() && p() -> talent.frost.obliteration.ok() )
    {
      p() -> trigger_killing_machine( 1.0, p() -> procs.km_from_obliteration_sr,
                                           p() -> procs.km_from_obliteration_sr_wasted );

      // Obliteration's rune generation
      if ( rng().roll( p() -> talent.frost.obliteration -> effectN( 2 ).percent() ) )
      {
        p() -> replenish_rune( as<int>(p() -> spell.obliteration_gains -> effectN( 1 ).base_value()), p() -> gains.obliteration );
      }
    }
  }
private:
    propagate_const<action_t*> soul_reaper_execute;
};

// Summon Gargoyle ==========================================================

struct summon_gargoyle_t final : public death_knight_spell_t
{
  summon_gargoyle_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "summon_gargoyle", p, p -> talent.unholy.summon_gargoyle )
  {
    parse_options( options_str );
    harmful = false;
    track_cd_waste = true;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> pets.gargoyle -> summon( data().duration() );
  }
};

// Tombstone ================================================================
// Not with Defensive Abilities because of the reliable RP generation

struct tombstone_t final : public death_knight_spell_t
{
  tombstone_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "tombstone", p, p -> talent.blood.tombstone )
  {
    parse_options( options_str );

    harmful = may_crit = false;
    track_cd_waste = true;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    int charges = std::min( p() -> buffs.bone_shield -> check(), as<int>( data().effectN( 5 ).base_value() ) );

    double power = charges * data().effectN( 3 ).base_value();
    double shield = charges * data().effectN( 4 ).percent();

    p() -> resource_gain( RESOURCE_RUNIC_POWER, power, p() -> gains.tombstone, this );
    p() -> buffs.tombstone -> trigger( 1, shield * p() -> resources.max[ RESOURCE_HEALTH ] );
    p() -> buffs.bone_shield -> decrement( charges );
    p() -> cooldown.dancing_rune_weapon -> adjust( p() -> talent.blood.insatiable_blade -> effectN( 1 ).time_value() * charges );

    if ( p() -> talent.blood.blood_tap.ok() )
    {
      p() -> cooldown.blood_tap -> adjust( -1.0 * timespan_t::from_seconds( p() -> talent.blood.blood_tap -> effectN( 2 ).base_value() ) * charges );
    }

    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T29, B2 ) )
    {
      double chance = 0.20 * charges;  // 20% chance is not in spelldata for T29 2PC.  Tombstone gives 20% chance per boneshield stack consumed
      if ( rng().roll( chance ) )
      {
        p() -> replenish_rune( as<unsigned int>( p() -> spell.vigorous_lifeblood_energize -> effectN( 1 ).base_value() ), p() -> gains.vigorous_lifeblood_2pc );
      }
    }

    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T29, B4 ) )
    {
      // Perform this update in a loop, as we do allow overflow into the next stacks
      for ( int i = 0; i < charges; i++ )
      {
        p() -> bone_shield_charges_consumed++;
        if ( p() -> bone_shield_charges_consumed >= 10 )
        {
          p() -> bone_shield_charges_consumed = 0;
          p() -> buffs.vigorous_lifeblood_4pc -> trigger();
        }
      }
    }

    if ( charges > 0 && p() -> talent.blood.shattering_bone.ok() )
    {
      // Set the number of charges of BS consumed, as it's used as a multiplier in shattering bone
      debug_cast<shattering_bone_t*>(p() -> active_spells.shattering_bone) -> boneshield_charges_consumed = charges;
      p() -> active_spells.shattering_bone -> execute_on_target( target );
    }
  }
};

// Unholy Assault ============================================================

struct unholy_assault_t final : public death_knight_melee_attack_t
{
  unholy_assault_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_melee_attack_t( "unholy_assault", p, p -> talent.unholy.unholy_assault )
  {
    parse_options( options_str );
    track_cd_waste = true;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();
    p() -> buffs.unholy_assault -> trigger();
  }

  void impact( action_state_t* s) override
  {
    death_knight_melee_attack_t::impact( s );

    p()->trigger_festering_wound( s, as<int>( p() -> talent.unholy.unholy_assault -> effectN( 3 ).base_value() ),
                                  p() -> procs.fw_unholy_assault );
  }
};

// ==========================================================================
// Death Knight Defensive Abilities
// ==========================================================================

// Anti-magic Shell =========================================================

struct antimagic_shell_buff_t final : public buff_t
{
  double remaining_absorb;
  double damage;

  antimagic_shell_buff_t( death_knight_t* p ) :
    buff_t( p, "antimagic_shell", p -> talent.antimagic_shell ),
    remaining_absorb( 0.0 ), damage( 0 )
  {
    cooldown -> duration = 0_ms;

    if( p -> options.ams_absorb_percent > 0 )
    {
      set_period( 1_s );
      set_tick_time_behavior( buff_tick_time_behavior::HASTED );
      set_tick_callback( [ this, p ] ( buff_t*, int, timespan_t )
      {
        if ( p -> specialization() == DEATH_KNIGHT_UNHOLY || p -> specialization() == DEATH_KNIGHT_FROST )
        {
          double opt = p -> options.ams_absorb_percent;
          double ticks = buff_duration() / tick_time();
          double pct = opt / ticks;
          damage = ( p->resources.max[ RESOURCE_HEALTH ] * ( p -> talent.antimagic_shell -> effectN( 2 ).percent() ) * ( 1.0 + p -> talent.antimagic_barrier -> effectN( 3 ).percent() ) * ( 1.0 + p -> cache.heal_versatility() ) ) * pct;
          double absorbed = std::min( damage,
                                  debug_cast< antimagic_shell_buff_t* >( p -> buffs.antimagic_shell ) -> calc_absorb() );

          // AMS generates 0.833~ runic power per percentage max health absorbed.
          double rp_generated = absorbed / p->resources.max[RESOURCE_HEALTH] * 83.333;

          p -> resource_gain( RESOURCE_RUNIC_POWER, util::round( rp_generated ), p -> gains.antimagic_shell );
        }
      } );
    }
    

    // Assuming AMB's 30% increase is applied after Runic Barrier
    base_buff_duration *= 1.0 + p -> talent.antimagic_barrier -> effectN( 2 ).percent();
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    remaining_absorb = calc_absorb();

    buff_t::execute( stacks, value, duration );
  }

  void expire_override( int stacks, timespan_t duration ) override
  {
    buff_t::expire_override( stacks, duration );

    remaining_absorb = 0;
  }

  double calc_absorb()
  {
    death_knight_t* dk = debug_cast< death_knight_t* >( player );
    double max_absorb = dk -> resources.max[ RESOURCE_HEALTH ] * dk -> talent.antimagic_shell -> effectN( 2 ).percent();

    max_absorb *= 1.0 + dk -> talent.antimagic_barrier -> effectN( 2 ).percent();

    max_absorb *= 1.0 + dk -> cache.heal_versatility();

    return max_absorb;
  }

  double absorb_damage( double incoming_damage )
  {
    if ( incoming_damage >= remaining_absorb )
    {
      this -> expire();

      return remaining_absorb;
    }

    remaining_absorb -= incoming_damage;

    return incoming_damage;
  }
};

struct antimagic_shell_t final : public death_knight_spell_t
{
  double min_interval;
  double interval;
  double interval_stddev;
  double interval_stddev_opt;
  double damage;

  antimagic_shell_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "antimagic_shell", p, p -> talent.antimagic_shell ),
    min_interval( 60 ), interval( 60 ), interval_stddev( 0.05 ), interval_stddev_opt( 0 ), damage( p -> options.ams_absorb_percent )
  {
    cooldown -> duration += p -> talent.antimagic_barrier -> effectN( 1 ).time_value();
    harmful = may_crit = may_miss = false;
    base_dd_min = base_dd_max = 0;
    target = player;

    add_option( opt_float( "interval", interval ) );
    add_option( opt_float( "interval_stddev", interval_stddev_opt ) );
    add_option( opt_float( "damage", damage ) );
    parse_options( options_str );

    min_interval += p -> talent.antimagic_barrier -> effectN( 1 ).base_value() / 1000;
    interval += p -> talent.antimagic_barrier -> effectN( 1 ).base_value() / 1000;

    // Don't allow lower than AMS cd intervals
    if ( interval < min_interval )
    {
      sim -> errorf( "%s minimum interval for Anti-Magic Shell is %f seconds.", player -> name(), min_interval );
      interval = min_interval;
    }

    // Less than a second standard deviation is translated to a percent of
    // interval
    if ( interval_stddev_opt < 1 )
      interval_stddev = interval * interval_stddev_opt;
    // >= 1 seconds is used as a standard deviation normally
    else
      interval_stddev = interval_stddev_opt;

    // Setup an Absorb stats tracker for AMS if it's used "for reals"
    if ( damage == 0 )
    {
      stats -> type = STATS_ABSORB;
      if ( ! p -> antimagic_shell )
        p -> antimagic_shell = stats;
    }
  }

  void execute() override
  {
    if ( damage > 0 )
    {
     timespan_t new_cd = timespan_t::from_seconds( std::max( min_interval, rng().gauss( interval, interval_stddev ) ) );

     cooldown -> duration = new_cd;
    }

    death_knight_spell_t::execute();

    p() -> buffs.antimagic_shell -> trigger();
  }
};

// Anti-magic Zone =========================================================

struct antimagic_zone_buff_t final : public buff_t
{
  double remaining_absorb;
  double damage;

  antimagic_zone_buff_t( death_knight_t* p ) :
    buff_t( p, "antimagic_zone", p -> talent.antimagic_zone ),
    remaining_absorb( 0.0 ), damage( 0 )
  {
    cooldown -> duration = 0_ms;

    if( p -> options.amz_absorb_percent > 0 )
    {
      set_period( 1_s );
      set_tick_time_behavior( buff_tick_time_behavior::HASTED );
      set_tick_callback( [ this, p ] ( buff_t*, int, timespan_t )
      {
        if ( p -> talent.assimilation -> ok() && ( p -> specialization() == DEATH_KNIGHT_UNHOLY || p -> specialization() == DEATH_KNIGHT_FROST ) )
        {
          double opt = p -> options.amz_absorb_percent;
          double ticks = buff_duration() / tick_time();
          double pct = opt / ticks;
          double pct_of_max_hp = 1.5;
          damage = p->resources.max[ RESOURCE_HEALTH ] * pct_of_max_hp * ( 1.0 + p -> talent.assimilation -> effectN( 1 ).percent() ) * ( 1.0 + p -> cache.heal_versatility() ) * pct;
          double absorbed = std::min( damage,
                                  debug_cast< antimagic_zone_buff_t* >( p -> buffs.antimagic_zone ) -> calc_absorb() );

          // AMZ Generates 1% RP per 1% of the shield absorbed if Assimilation is talented.
          double absorb_pct = p -> resources.max[ RESOURCE_HEALTH ] * 1.5 * ( 1 + p -> talent.assimilation -> effectN( 1 ).percent() ) * ( 1 + p -> cache.heal_versatility() );
          // Assimilation can generate no more than 100 runic power 
          double rp_generated = std::min( p -> talent.assimilation -> effectN( 2 ).base_value() * 100, absorbed / absorb_pct * 100);

          p -> resource_gain( RESOURCE_RUNIC_POWER, util::round( rp_generated ), p -> gains.antimagic_zone );
        }
      } );
    }
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    remaining_absorb = calc_absorb();

    buff_t::execute( stacks, value, duration );
  }

  void expire_override( int stacks, timespan_t duration ) override
  {
    buff_t::expire_override( stacks, duration );

    remaining_absorb = 0;
  }

  double calc_absorb()
  {
    death_knight_t* dk = debug_cast< death_knight_t* >( player );
    // HP Value doesnt appear in spell data, instead stored in a variable in spell ID 51052, had coding the % hp value for now
    double max_absorb = dk -> resources.max[ RESOURCE_HEALTH ] * 1.5;

    max_absorb *= 1.0 + dk -> talent.assimilation -> effectN( 1 ).percent();

    max_absorb *= 1.0 + dk -> cache.heal_versatility();

    return max_absorb;
  }

  double absorb_damage( double incoming_damage )
  {
    if ( incoming_damage >= remaining_absorb )
    {
      this -> expire();

      return remaining_absorb;
    }

    remaining_absorb -= incoming_damage;

    return incoming_damage;
  }
};

struct antimagic_zone_t final : public death_knight_spell_t
{
  double min_interval;
  double interval;
  double interval_stddev;
  double interval_stddev_opt;
  double damage;

  antimagic_zone_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "antimagic_zone", p, p -> talent.antimagic_zone ),
    min_interval( 120 ), interval( 120 ), interval_stddev( 0.05 ), interval_stddev_opt( 0 ), damage( p -> options.amz_absorb_percent )
  {
    harmful = may_crit = may_miss = false;
    base_dd_min = base_dd_max = 0;
    target = player;

    add_option( opt_float( "interval", interval ) );
    add_option( opt_float( "interval_stddev", interval_stddev_opt ) );
    add_option( opt_float( "damage", damage ) );
    parse_options( options_str );

    // Don't allow lower than AMZ cd intervals
    if ( interval < min_interval )
    {
      sim -> errorf( "%s minimum interval for Anti-Magic Zone is %f seconds.", player -> name(), min_interval );
      interval = min_interval;
    }

    // Less than a second standard deviation is translated to a percent of
    // interval
    if ( interval_stddev_opt < 1 )
      interval_stddev = interval * interval_stddev_opt;
    // >= 1 seconds is used as a standard deviation normally
    else
      interval_stddev = interval_stddev_opt;

    // Setup an Absorb stats tracker for AMZ if it's used "for reals"
    if ( damage == 0 )
    {
      stats -> type = STATS_ABSORB;
      if ( ! p -> antimagic_zone )
        p -> antimagic_zone = stats;
    }
  }

  void execute() override
  {
    if ( damage > 0 )
    {
     timespan_t new_cd = timespan_t::from_seconds( std::max( min_interval, rng().gauss( interval, interval_stddev ) ) );

     cooldown -> duration = new_cd;
    }

    death_knight_spell_t::execute();

    p() -> buffs.antimagic_zone -> trigger();
  }
};

// Icebound Fortitude =======================================================

struct icebound_fortitude_t final : public death_knight_spell_t
{
  icebound_fortitude_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "icebound_fortitude", p, p -> talent.icebound_fortitude )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.icebound_fortitude -> trigger();
  }
};


// Mark of Blood ============================================================

struct mark_of_blood_heal_t final : public death_knight_heal_t
{
  mark_of_blood_heal_t( death_knight_t* p ) : // The data is removed and switched to the talent spell on PTR 8.3.0.32805
    death_knight_heal_t( "mark_of_blood_heal", p, p -> talent.blood.mark_of_blood )
  {
    may_crit = callbacks = false;
    background = dual = true;
    target = p;
    base_pct_heal = data().effectN( 1 ).percent();
  }
};

struct mark_of_blood_t final : public death_knight_spell_t
{
  mark_of_blood_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "mark_of_blood", p, p -> talent.blood.mark_of_blood )
  {
    parse_options( options_str );
    track_cd_waste = true;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    get_td( target ) -> debuff.mark_of_blood -> trigger();
  }
};

// Rune Tap =================================================================

struct rune_tap_t final : public death_knight_spell_t
{
  rune_tap_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "rune_tap", p, p -> talent.blood.rune_tap )
  {
    parse_options( options_str );
    use_off_gcd = true;
    track_cd_waste = true;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.rune_tap -> trigger();
  }
};

// Vampiric Blood ===========================================================
struct vampiric_blood_t final : public death_knight_spell_t
{
  vampiric_blood_t( death_knight_t* p, util::string_view options_str ) :
    death_knight_spell_t( "vampiric_blood", p, p -> talent.blood.vampiric_blood )
  {
    parse_options( options_str );

    harmful = false;
    base_dd_min = base_dd_max = 0;
    track_cd_waste = true;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.vampiric_blood -> trigger();

    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T30, B4 ) )
      p() -> buffs.vampiric_strength -> trigger();
  }
};

// Buffs ====================================================================

struct runic_corruption_buff_t final : public buff_t
{
  runic_corruption_buff_t( death_knight_t* p ) :
    buff_t( p, "runic_corruption", p -> spell.runic_corruption )
  {
    // Runic Corruption refreshes to remaining time + buff duration
    refresh_behavior = buff_refresh_behavior::EXTEND;
    set_affects_regen( true );
    set_stack_change_callback( [ p ]( buff_t*, int, int )
    {
      p -> _runes.update_coefficient();
    } );
  }

  // Runic Corruption duration is reduced by haste so it always regenerates
  // the equivalent of 0.9 of a rune ( 3 / 10 seconds on 3 regenerating runes )
  timespan_t buff_duration() const override
  {
    timespan_t initial_duration = buff_t::buff_duration();

    return initial_duration * player -> cache.attack_haste();
  }
};

} // UNNAMED NAMESPACE

// Runeforges ===============================================================

void runeforge::fallen_crusader( special_effect_t& effect )
{
  struct fallen_crusader_heal_t final : public death_knight_heal_t
  {
    fallen_crusader_heal_t( util::string_view name, death_knight_t* p, const spell_data_t* data ) :
      death_knight_heal_t( name, p, data )
    {
      background = true;
      target = player;
      callbacks = may_crit = false;
      base_pct_heal = data -> effectN( 2 ).percent();
      base_pct_heal *= 1.0 + p -> talent.unholy_bond -> effectN( 2 ).percent();
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

  death_knight_t* p = debug_cast<death_knight_t*>( effect.item -> player );

  // Create unholy strength heal if necessary, buff is always created for APL support
  p -> runeforge.rune_of_the_fallen_crusader = true;

  effect.custom_buff = p -> buffs.unholy_strength;
  effect.execute_action = get_action<fallen_crusader_heal_t>( "unholy_strength", p, effect.driver() -> effectN( 1 ).trigger() );

  new dbc_proc_callback_t( effect.item, effect );
}

void runeforge::razorice( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* p = debug_cast<death_knight_t*>( effect.item -> player );

  if ( ! p -> active_spells.runeforge_razorice )
    p -> active_spells.runeforge_razorice = new razorice_attack_t( p, "razorice" );

  // Store in which hand razorice is equipped, as it affects which abilities proc it
  if ( effect.item -> slot == SLOT_MAIN_HAND )
    p -> runeforge.rune_of_razorice_mh = true;
  else if ( effect.item -> slot == SLOT_OFF_HAND )
    p -> runeforge.rune_of_razorice_oh = true;
}

void runeforge::stoneskin_gargoyle( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* p = debug_cast<death_knight_t*>( effect.item -> player );

  p -> runeforge.rune_of_the_stoneskin_gargoyle = true;

  p -> buffs.stoneskin_gargoyle = make_buff( p, "stoneskin_gargoyle", effect.driver() )
    -> set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
    -> set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
    -> set_pct_buff_type( STAT_PCT_BUFF_STAMINA )
    // Stoneskin Gargoyle increases all primary stats, even the irrelevant ones
    -> set_pct_buff_type( STAT_PCT_BUFF_AGILITY )
    -> set_pct_buff_type( STAT_PCT_BUFF_INTELLECT )
    -> apply_affecting_aura( p -> talent.unholy_bond );

  // Change the player's base armor multiplier
  double am = effect.driver() -> effectN( 1 ).percent();
  am *= 1.0 + p -> talent.unholy_bond -> effectN( 2 ).percent();
  p -> base.armor_multiplier *= 1.0 + am;

  // This buff can only be applied on a 2H weapon, stacking mechanic is unknown territory

  // The buff isn't shown ingame, leave it visible in the sim for clarity
  // p -> quiet = true;
}

void runeforge::apocalypse( special_effect_t& effect )
{
  if ( effect.player -> type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* p = debug_cast<death_knight_t*>( effect.item -> player );
  // Nothing happens if the runeforge is applied on both weapons
  if ( p -> runeforge.rune_of_apocalypse )
    return;

  // Triggering the effects is handled in pet_melee_attack_t::impact()
  p -> runeforge.rune_of_apocalypse = true;
  // Even though a pet procs it, the damage from Pestilence belongs directly to the player in logs
  p -> active_spells.runeforge_pestilence = new runeforge_apocalypse_pestilence_t( "Pestilence", p );
}

void runeforge::hysteria( special_effect_t& effect )
{
  if ( effect.player -> type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* p = debug_cast<death_knight_t*>( effect.item -> player );

  if ( ! p -> runeforge.rune_of_hysteria )
  {
    p -> runeforge.rune_of_hysteria = true;
    p -> buffs.rune_of_hysteria = make_buff( p, "rune_of_hysteria", effect.driver() -> effectN( 1 ).trigger() )
      -> set_default_value_from_effect( 1 )
      -> apply_affecting_aura( p -> talent.unholy_bond );
  }

  // The RP cap increase stacks
  p->resources.base[ RESOURCE_RUNIC_POWER ] += effect.driver()->effectN( 2 ).resource( RESOURCE_RUNIC_POWER ) *
                                               ( 1.0 + p->talent.unholy_bond->effectN( 2 ).percent() );

  // The buff doesn't stack and doesn't have an increased effect
  // but the proc rate is increased and it has been observed to proc twice on the same damage event (2020-08-23)
  effect.custom_buff = p -> buffs.rune_of_hysteria;

  new dbc_proc_callback_t( effect.item, effect );
}

void runeforge::sanguination( special_effect_t& effect )
{
  if ( effect.player -> type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* p = debug_cast<death_knight_t*>( effect.item -> player );
  // This runeforge doesn't stack
  if ( p -> runeforge.rune_of_sanguination )
    return;

  struct sanguination_heal_t final : public death_knight_heal_t
  {
    double health_threshold;
    sanguination_heal_t( special_effect_t& effect ) :
      death_knight_heal_t( "rune_of_sanguination", debug_cast<death_knight_t*>( effect.player ),
                           effect.driver() -> effectN( 1 ).trigger() ),
      health_threshold( effect.driver() -> effectN( 1 ).base_value() )
    {
      background = true;
      tick_pct_heal = data().effectN( 1 ).percent();
      tick_pct_heal *= 1.0 + p() -> talent.unholy_bond -> effectN( 1 ).percent();
      // Sated-type debuff, for simplicity the debuff's duration is used as a simple cooldown in simc
      cooldown -> duration = p() -> spell.sanguination_cooldown -> duration();
    }

    bool ready() override
    {
      if ( p() -> health_percentage() > health_threshold )
        return false;

      return death_knight_heal_t::ready();
    }
  };

  p -> runeforge.rune_of_sanguination = true;

  p -> active_spells.runeforge_sanguination = new sanguination_heal_t( effect );
}

void runeforge::spellwarding( special_effect_t& effect )
{
  struct spellwarding_absorb_t final : public absorb_t
  {
    double health_percentage;
    spellwarding_absorb_t( util::string_view name, death_knight_t* p, const spell_data_t* data ) :
      absorb_t( name, p, data ),
      health_percentage( p -> spell.spellwarding_absorb -> effectN( 2 ).percent() )
      // The absorb amount is hardcoded in the effect tooltip, the only data is in the runeforging action spell
    {
      target = p;
      background = true;
      harmful = false;
    }

    void execute() override
    {
      base_dd_min = base_dd_max = health_percentage * player -> resources.max[ RESOURCE_HEALTH ];

      absorb_t::execute();
    }
  };

  if ( effect.player -> type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* p = debug_cast<death_knight_t*>( effect.item -> player );

  // Stacking the rune doubles the damage reduction, and seems to create a second proc
  p -> runeforge.rune_of_spellwarding += effect.driver() -> effectN( 2 ).percent();
  effect.execute_action = get_action<spellwarding_absorb_t>( "rune_of_spellwarding", p, effect.driver() -> effectN( 1 ).trigger() );

  new dbc_proc_callback_t( effect.item, effect );
}

// NYI
void runeforge::unending_thirst( special_effect_t& effect )
{
  if ( effect.player -> type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  // Placeholder for APL tracking purpose, effect NYI
  debug_cast<death_knight_t*>( effect.player ) -> runeforge.rune_of_unending_thirst = true;
}

// Resource Manipulation ====================================================

double death_knight_t::resource_gain( resource_e resource_type, double amount, gain_t* g, action_t* action )
{
  double actual_amount = player_t::resource_gain( resource_type, amount, g, action );

  if ( runeforge.rune_of_hysteria && resource_type == RESOURCE_RUNIC_POWER )
  {
    // Unholy Bond is implemented in the buff, so no need to boost it here.
    double bonus_rp = amount * buffs.rune_of_hysteria -> value();
    actual_amount += player_t::resource_gain( resource_type, bonus_rp, gains.rune_of_hysteria, action );
  }

  return actual_amount;
}

double death_knight_t::resource_loss( resource_e resource_type, double amount, gain_t* g, action_t* action )
{
  double actual_amount = player_t::resource_loss( resource_type, amount, g, action );

  if ( resource_type == RESOURCE_RUNE )
  {
    _runes.consume(as<int>(amount) );
    // Ensure rune state is consistent with the actor resource state for runes
    assert( _runes.runes_full() == resources.current[ RESOURCE_RUNE ] );
  }

  // Procs from runes spent
  if ( resource_type == RESOURCE_RUNE && action )
  {
    // Gathering Storm triggers a stack and extends RW duration by 0.5s
    // for each spell cast that normally consumes a rune (even if it ended up free)
    // But it doesn't count the original Relentless Winter cast
    if ( talent.frost.gathering_storm.ok() && buffs.remorseless_winter -> check() && action -> data().id() != spec.remorseless_winter -> id() )
    {
      unsigned consumed = static_cast<unsigned>( action -> base_costs[ RESOURCE_RUNE ] );
      buffs.gathering_storm -> trigger( consumed );
      timespan_t base_extension = timespan_t::from_seconds( talent.frost.gathering_storm -> effectN( 1 ).base_value() / 10.0 );
      buffs.remorseless_winter -> extend_duration( this, base_extension * consumed );
    }

    if ( talent.rune_mastery.ok() )
    {
      buffs.rune_mastery -> trigger();
    }

    // Effects that require the player to actually spend runes
    if ( actual_amount > 0 )
    {
      if ( specialization() == DEATH_KNIGHT_FROST && buffs.pillar_of_frost -> up() )
      {
        buffs.pillar_of_frost_bonus -> trigger( as<int>( actual_amount ) );
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

    // If an action is linked, fetch its base cost. Exclude Bonestorm from this otherwise it uses the base cost for
    // Insatiable Hunger instead of the actual rp spent
    if ( action && action->id != 194844 )
      base_rp_cost = action -> base_costs[ RESOURCE_RUNIC_POWER ];

    // 2020-12-16 - Melekus: Based on testing with both Frost Strike and Breath of Sindragosa during Hypothermic Presence,
    // RE is using the ability's base cost for its proc chance calculation, just like Runic Corruption
    trigger_runic_empowerment( base_rp_cost );
    trigger_runic_corruption( procs.rp_runic_corruption, base_rp_cost, false );

    if ( talent.unholy.summon_gargoyle.ok() && pets.gargoyle )
    {
      // 2019-02-12 Death Strike buffs gargoyle as if it had its full cost, even though it received a -10rp cost in 8.1
      // Assuming it's a bug for now, using base_costs instead of last_resource_cost won't affect free spells here
      // Free Death Coils are still handled in the action
      pets.gargoyle -> increase_power( base_rp_cost );
    }

    buffs.icy_talons -> trigger();

    if ( talent.frost.unleashed_frenzy.ok() )
    {
      buffs.unleashed_frenzy -> trigger();
    }

    // Effects that only trigger if resources were spent
    if ( actual_amount > 0 )
    {
      if ( talent.blood.red_thirst.ok() )
      {
        timespan_t sec = talent.blood.red_thirst -> effectN( 1 ).time_value() *
          actual_amount / talent.blood.red_thirst -> effectN( 2 ).base_value();
        cooldown.vampiric_blood -> adjust( -sec );
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
  add_option( opt_float( "deathknight.amz_absorb_percent", options.amz_absorb_percent, 0.0, 1.0 ) );
  add_option( opt_bool( "deathknight.individual_pet_reporting", options.individual_pet_reporting ) );
}

void death_knight_t::copy_from( player_t* source )
{
  player_t::copy_from( source );
  options = debug_cast<death_knight_t*>( source ) -> options;
}

void death_knight_t::merge( player_t& other )
{
  player_t::merge( other );

  death_knight_t& dk = dynamic_cast< death_knight_t& >( other );

  _runes.rune_waste.merge( dk._runes.rune_waste );
  _runes.cumulative_waste.merge( dk._runes.cumulative_waste );

}

std::string death_knight_t::create_profile( save_e type )
{
  std::string term;
  std::string profile_str = player_t::create_profile( type );
  term = "\n";
  if ( type & SAVE_PLAYER )
  {
    if( options.ams_absorb_percent > 0 )
    {
      profile_str += "deathknight.ams_absorb_percent=" + util::to_string( options.ams_absorb_percent ) + term;
    }

    if( options.amz_absorb_percent > 0 )
    {
      profile_str += "deathknight.amz_absorb_percent=" + util::to_string( options.amz_absorb_percent ) + term;
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
  if ( sim -> event_mgr.canceled )
  {
    return;
  }

  if ( ! talent.soul_reaper.ok() )
  {
    return;
  }

  if ( get_target_data( target ) -> dot.soul_reaper -> is_ticking() )
  {
    sim -> print_log( "Target {} died while affected by Soul Reaper, player {} gains Runic Corruption buff.",
                      target -> name(), name() );

    trigger_runic_corruption( procs.sr_runic_corruption, 0, 1.0, true );
    return;
  }
}

void death_knight_t::trigger_festering_wound_death( player_t* target )
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( sim -> event_mgr.canceled || ! spec.festering_wound -> ok() )
  {
    return;
  }

  const death_knight_td_t* td = get_target_data( target );
  if ( !td ) return;

  int n_wounds = td -> debuff.festering_wound -> check();

  // If the target wasn't affected by festering wound, return
  if ( n_wounds == 0 ) return;

  // Generate the RP you'd have gained if you popped the wounds manually
  resource_gain( RESOURCE_RUNIC_POWER,
                 spec.festering_wound -> effectN( 1 ).trigger() -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER ) * n_wounds,
                 gains.festering_wound );

  resource_gain( RESOURCE_RUNIC_POWER,
                 talent.unholy.replenishing_wounds->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) * n_wounds,
                 gains.replenishing_wounds );

  // Triggers a bursting sores explosion for each wound on the target
  if ( talent.unholy.bursting_sores.ok() && !active_spells.bursting_sores -> target_list().empty() )
  {
    for ( int i = 0; i < n_wounds; i++ )
      active_spells.bursting_sores -> execute_on_target( target );
  }

  if ( talent.unholy.festermight.ok() )
  {
    buffs.festermight->trigger( n_wounds );
  }

  if ( sets -> has_set_bonus( DEATH_KNIGHT_UNHOLY, T29, B2 ) )
  {
      pets.ghoul_pet -> vile_infusion -> trigger();
  }
}

void death_knight_t::trigger_virulent_plague_death( player_t* target )
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( sim -> event_mgr.canceled )
  {
    return;
  }

  if ( ! get_target_data( target ) -> dot.virulent_plague -> is_ticking() )
  {
    return;
  }

  // Schedule an execute for Virulent Eruption
  active_spells.virulent_eruption -> schedule_execute();
}

bool death_knight_t::in_death_and_decay() const
{
  if ( ! sim -> distance_targeting_enabled || ! active_dnd )
    return active_dnd != nullptr;

  return get_ground_aoe_distance( *active_dnd -> pulse_state ) <= active_dnd -> pulse_state -> action -> radius;
}

unsigned death_knight_t::replenish_rune( unsigned n, gain_t* gain )
{
  unsigned replenished = 0;

  while ( n-- )
  {
    rune_t* rune = _runes.first_depleted_rune();
    if ( ! rune )
    {
      rune = _runes.first_regenerating_rune();
    }

    if ( ! rune && gain )
    {
      gain -> add( RESOURCE_RUNE, 0, 1 );
    }
    else if ( rune )
    {
      rune -> fill_rune( gain );
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
  bool wasted = buffs.killing_machine -> at_max_stacks();
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
    if ( spec.might_of_the_frozen_wastes -> ok() && main_hand_weapon.group() == WEAPON_2H )
    {
      km_proc_chance = 1.0;
    }
    else
    {
      km_proc_chance = ++km_proc_attempts * 0.3;
    }

    if ( rng().roll( km_proc_chance ) )
    {
      triggered = true;
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

  buffs.killing_machine -> trigger();

  // If the proc is guaranteed, allow the player to instantly react to it
  if ( chance == 1.0 )
  {
    buffs.killing_machine -> predict();
  }

  // Use procs to track sources and wastes
  if ( !wasted )
  {
    proc -> occur();
  }
  else
  {
    wasted_proc -> occur();
  }
}

void death_knight_t::consume_killing_machine( proc_t* proc )
{
  if ( ! buffs.killing_machine -> up() )
  {
    return;
  }

  proc -> occur();

  buffs.killing_machine -> decrement();

  if ( rng().roll( talent.frost.murderous_efficiency -> effectN( 1 ).percent() ) )
  {
    replenish_rune( as<int>( spell.murderous_efficiency_gain -> effectN( 1 ).base_value() ), gains.murderous_efficiency );
  }

  if ( talent.frost.bonegrinder.ok() && !buffs.bonegrinder_frost -> up() )
  {
    buffs.bonegrinder_crit -> trigger();
    if ( buffs.bonegrinder_crit -> at_max_stacks() )
    {
      buffs.bonegrinder_frost -> trigger();
      buffs.bonegrinder_crit -> expire();
    }
  }
}

void death_knight_t::trigger_runic_empowerment( double rpcost )
{
  if ( ! spec.frost_death_knight -> ok() )
    return;

  double base_chance = spell.runic_empowerment_chance -> effectN( 1 ).percent() / 10.0;

  if ( ! rng().roll( base_chance * rpcost ) )
    return;

  int regenerated = replenish_rune( as<int>( spell.runic_empowerment_gain -> effectN( 1 ).resource( RESOURCE_RUNE ) ), gains.runic_empowerment );

  sim -> print_debug( "{} Runic Empowerment regenerated {} rune", name(), regenerated );
  log_rune_status( this );
}

void death_knight_t::trigger_runic_corruption( proc_t* proc, double rpcost, double override_chance, bool death_trigger )
{
  if ( ! spec.unholy_death_knight -> ok() && death_trigger == false )
      return;
  double proc_chance = 0.0;

  // Use the overriden chance if there's one and RP cost is 0
  proc_chance = (!rpcost && override_chance != -1.0) ? override_chance :
    // Else, use the general proc chance ( 1.6 per RP * RP / 100 as of patch 9.0.2 )
    spell.runic_corruption_chance->effectN(1).percent() * rpcost / 100.0;
  // Buff duration and refresh behavior handled in runic_corruption_buff_t
  if ( spec.unholy_death_knight -> ok() && buffs.runic_corruption -> trigger( 1, buff_t::DEFAULT_VALUE(), proc_chance ) && proc )
    proc -> occur();
}

void death_knight_t::trigger_festering_wound( const action_state_t* state, unsigned n, proc_t* proc )
{
  if ( ! state -> action -> result_is_hit( state -> result ) )
  {
    return;
  }

  get_target_data( state -> target ) -> debuff.festering_wound -> trigger( n );
  while ( n-- > 0 )
  {
    proc -> occur();
  }
}

void death_knight_t::burst_festering_wound( player_t* target, unsigned n )
{
  struct fs_burst_t : public event_t
  {
    unsigned n;
    player_t* target;
    death_knight_t* dk;

    fs_burst_t( death_knight_t* dk, player_t* target, unsigned n ) :
      event_t( *dk, 0_ms ), n( n ), target( target ), dk( dk )
    {
    }

    const char* name() const override
    { return "festering_wounds_burst"; }

    void execute() override
    {
      death_knight_td_t* td = dk -> get_target_data( target );

      unsigned n_executes = std::min( n, as<unsigned>( td -> debuff.festering_wound -> check() ) );
      for ( unsigned i = 0; i < n_executes; ++i )
      {
        dk -> active_spells.festering_wound -> execute_on_target( target );

        // Don't unnecessarily call bursting sores in single target scenarios
        if ( dk -> talent.unholy.bursting_sores.ok() &&
             !dk -> active_spells.bursting_sores -> target_list().empty() )
        {
          dk -> active_spells.bursting_sores -> execute_on_target( target );
        }
      }

      // Triggers once per target per player action:
      // Apocalypse is 10% * n wounds burst to proc
      // Scourge strike aoe is 1 - ( 0.9 ) ^ n targets to proc, or 10% for each target hit
      if ( dk-> talent.unholy.festermight.ok() )
      {
        dk->buffs.festermight->trigger( n_executes );
      }
      
      if ( dk -> sets -> has_set_bonus( DEATH_KNIGHT_UNHOLY, T29, B2 ) )
      {
        dk -> pets.ghoul_pet -> vile_infusion -> trigger();
      }

      td -> debuff.festering_wound -> decrement( n_executes ); 
    }
  };

  const death_knight_td_t* td = get_target_data( target );

  // Don't bother creating the event if n is 0, the target has no wounds, or is scheduled to demise
  if ( ! spec.festering_wound -> ok() || ! n || ! td || ! td -> debuff.festering_wound -> check() || target -> demise_event )
  {
    return;
  }

  make_event<fs_burst_t>( *sim, this, target, n );
}

// Launches the repeating event for the Inexorable Assault talent
void death_knight_t::start_inexorable_assault()
{
  if ( !talent.frost.inexorable_assault.ok() )
  {
    return;
  }

  buffs.inexorable_assault -> trigger( buffs.inexorable_assault -> max_stack() );

  // Inexorable assault keeps ticking out of combat and when it's at max stacks
  // We solve that by picking a random point at which the buff starts ticking
  timespan_t first = timespan_t::from_millis(
    rng().range( 0, as<int>( talent.frost.inexorable_assault -> effectN( 1 ).period().total_millis() ) ) );

  make_event( *sim, first, [ this ]() {
    buffs.inexorable_assault -> trigger();
    make_repeating_event( *sim, talent.frost.inexorable_assault -> effectN( 1 ).period(), [ this ]() {
      buffs.inexorable_assault -> trigger();
    } );
  } );
}

// Launches the repeting event for the cold heart talent
void death_knight_t::start_cold_heart()
{
  if ( !talent.frost.cold_heart.ok() )
  {
    return;
  }

  buffs.cold_heart -> trigger( buffs.cold_heart -> max_stack() );

  // Cold Heart keeps ticking out of combat and when it's at max stacks
  // We solve that by picking a random point at which the buff starts ticking
  timespan_t first = timespan_t::from_millis(
    rng().range( 0, as<int>( talent.frost.cold_heart -> effectN( 1 ).period().total_millis() ) ) );

  make_event( *sim, first, [ this ]() {
    buffs.cold_heart -> trigger();
    make_repeating_event( *sim, talent.frost.cold_heart -> effectN( 1 ).period(), [ this ]() {
      buffs.cold_heart -> trigger();
    } );
  } );
}

// ==========================================================================
// Death Knight Character Definition
// ==========================================================================

// death_knight_t::create_actions ===========================================

void death_knight_t::create_actions()
{
  // Class talents
  if ( talent.blood_draw.ok() )
    active_spells.blood_draw = new blood_draw_t( "blood_draw", this );

  // Blood
  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    if ( talent.blood.mark_of_blood.ok() )
    {
      active_spells.mark_of_blood_heal = new mark_of_blood_heal_t( this );
    }
    if ( talent.blood.shattering_bone.ok() )
    {
      active_spells.shattering_bone = new shattering_bone_t( this );
    }
  }

  // Unholy
  else if ( specialization() == DEATH_KNIGHT_UNHOLY )
  {
    if ( spec.festering_wound -> ok() )
    {
      active_spells.festering_wound = new festering_wound_t( this );
    }

    if ( talent.unholy.bursting_sores.ok() )
    {
      active_spells.bursting_sores = new bursting_sores_t( this );
    }
  }

  if ( talent.unholy.outbreak.ok() || talent.unholy.unholy_blight.ok() )
    active_spells.virulent_eruption = new virulent_eruption_t( this );

  player_t::create_actions();
}

// death_knight_t::create_action  ===========================================

action_t* death_knight_t::create_action( util::string_view name, util::string_view options_str )
{
  // General Actions
  if ( name == "abomination_limb"  ) return new abomination_limb_t  ( this, options_str );
  if ( name == "antimagic_shell"          ) return new antimagic_shell_t          ( this, options_str );
  if ( name == "antimagic_zone"           ) return new antimagic_zone_t           ( this, options_str );
  if ( name == "auto_attack"              ) return new auto_attack_t              ( this, options_str );
  if ( name == "chains_of_ice"            ) return new chains_of_ice_t            ( this, options_str );
  if ( name == "death_strike"             ) return new death_strike_t             ( this, options_str );
  if ( name == "icebound_fortitude"       ) return new icebound_fortitude_t       ( this, options_str );
  if ( name == "mind_freeze"              ) return new mind_freeze_t              ( this, options_str );
  if ( name == "raise_dead"               ) return new raise_dead_t               ( this, options_str );
  if ( name == "sacrificial_pact"         ) return new sacrificial_pact_t         ( this, options_str );

  // Blood Actions
  if ( name == "blood_boil"               ) return new blood_boil_t               ( this, options_str );
  if ( name == "blood_tap"                ) return new blood_tap_t                ( this, options_str );
  if ( name == "blooddrinker"             ) return new blooddrinker_t             ( this, options_str );
  if ( name == "bonestorm"                ) return new bonestorm_t                ( this, options_str );
  if ( name == "consumption"              ) return new consumption_t              ( this, options_str );
  if ( name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_t      ( this, options_str );
  if ( name == "dark_command"             ) return new dark_command_t             ( this, options_str );
  if ( name == "deaths_caress"            ) return new deaths_caress_t            ( this, options_str );
  if ( name == "gorefiends_grasp"         ) return new gorefiends_grasp_t         ( this, options_str );
  if ( name == "heart_strike"             ) return new heart_strike_t             ( this, options_str );
  if ( name == "mark_of_blood"            ) return new mark_of_blood_t            ( this, options_str );
  if ( name == "marrowrend"               ) return new marrowrend_t               ( this, options_str );
  if ( name == "rune_tap"                 ) return new rune_tap_t                 ( this, options_str );
  if ( name == "tombstone"                ) return new tombstone_t                ( this, options_str );
  if ( name == "vampiric_blood"           ) return new vampiric_blood_t           ( this, options_str );

  // Frost Actions
  if ( name == "breath_of_sindragosa"     ) return new breath_of_sindragosa_t     ( this, options_str );
  if ( name == "chill_streak"             ) return new chill_streak_t             ( this, options_str );
  if ( name == "empower_rune_weapon"      ) return new empower_rune_weapon_t      ( this, options_str );
  if ( name == "frost_strike"             ) return new frost_strike_t             ( this, options_str );
  if ( name == "frostscythe"              ) return new frostscythe_t              ( this, options_str );
  if ( name == "frostwyrms_fury"          ) return new frostwyrms_fury_t          ( this, options_str );
  if ( name == "glacial_advance"          ) return new glacial_advance_t          ( this, options_str );
  if ( name == "horn_of_winter"           ) return new horn_of_winter_t           ( this, options_str );
  if ( name == "howling_blast"            ) return new howling_blast_t            ( this, options_str );
  if ( name == "obliterate"               ) return new obliterate_t               ( this, options_str );
  if ( name == "pillar_of_frost"          ) return new pillar_of_frost_t          ( this, options_str );
  if ( name == "remorseless_winter"       ) return new remorseless_winter_t       ( this, options_str );

  // Unholy Actions
  if ( name == "army_of_the_dead"         ) return new army_of_the_dead_t         ( this, options_str );
  if ( name == "apocalypse"               ) return new apocalypse_t               ( this, options_str );
  if ( name == "clawing_shadows"          ) return new clawing_shadows_t          ( this, options_str );
  if ( name == "dark_transformation"      ) return new dark_transformation_t      ( this, options_str );
  if ( name == "death_and_decay"          ) return new death_and_decay_t          ( this, options_str );
  if ( name == "death_coil"               ) return new death_coil_t               ( this, options_str );
  if ( name == "defile"                   ) return new defile_t                   ( this, options_str );
  if ( name == "epidemic"                 ) return new epidemic_t                 ( this, options_str );
  if ( name == "festering_strike"         ) return new festering_strike_t         ( this, options_str );
  if ( name == "outbreak"                 ) return new outbreak_t                 ( this, options_str );
  if ( name == "scourge_strike"           ) return new scourge_strike_t           ( this, options_str );
  if ( name == "soul_reaper"              ) return new soul_reaper_t              ( this, options_str );
  if ( name == "summon_gargoyle"          ) return new summon_gargoyle_t          ( this, options_str );
  if ( name == "unholy_assault"           ) return new unholy_assault_t           ( this, options_str );
  if ( name == "unholy_blight"            ) return new unholy_blight_t            ( this, options_str );
  if ( name == "vile_contagion"           ) return new vile_contagion_t           ( this, options_str );

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
std::unique_ptr<expr_t> death_knight_t::create_runeforge_expression( util::string_view runeforge_name, bool warning = false )
{
  // Razorice, looks for the damage procs related to MH and OH
  if ( util::str_compare_ci( runeforge_name, "razorice" ) )
    return expr_t::create_constant( "razorice_runforge_expression", runeforge.rune_of_razorice_mh || runeforge.rune_of_razorice_oh );

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
    return expr_t::create_constant( "stoneskin_gargoyle_runforge_expression", runeforge.rune_of_the_stoneskin_gargoyle );

  // Apocalypse
  if ( util::str_compare_ci( runeforge_name, "apocalypse" ) )
    return expr_t::create_constant( "apocalypse_runforge_expression", runeforge.rune_of_apocalypse);

  // Hysteria
  if ( util::str_compare_ci( runeforge_name, "hysteria" ) )
    return expr_t::create_constant( "hysteria_runeforge_expression", runeforge.rune_of_hysteria  );

  // Sanguination
  if ( util::str_compare_ci( runeforge_name, "sanguination" ) )
    return expr_t::create_constant( "sanguination_runeforge_expression", runeforge.rune_of_sanguination );

  // Spellwarding
  if ( util::str_compare_ci( runeforge_name, "spellwarding" ) )
    return expr_t::create_constant( "spellwarding_runeforge_expression", runeforge.rune_of_spellwarding != 0 );

  // Unending Thirst, effect NYI
  if ( util::str_compare_ci( runeforge_name, "unending_thirst" ) )
    return expr_t::create_constant( "unending_thirst_runeforge_expression", runeforge.rune_of_unending_thirst );

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
      auto n = n_char - '0';
      if ( n <= 0 || as<size_t>( n ) > MAX_RUNES )
        throw std::invalid_argument( fmt::format( "Error in rune.time_to expression, please enter a valid amount of runes" ) );

      return make_fn_expr( "rune_time_to_x", [ this, n ]() {
        return _runes.time_to_regen( static_cast<unsigned>( n ) );
      } );
    }
  }

  // Check if IQD execute is disabled
  if ( util::str_compare_ci( splits[ 0 ], "death_knight" ) && splits.size() > 1 )
  {
    if ( util::str_compare_ci( splits[ 1 ], "disable_iqd_execute" ) && splits.size() == 2 )
      return expr_t::create_constant( " disable_iqd_execute_expression ", sim->shadowlands_opts.disable_iqd_execute);
  }

  // Expose AMZ Absorb Percent to the APL to prevent its use if disabled.
  if ( util::str_compare_ci( splits[ 0 ], "death_knight" ) && splits.size() > 1 )
  {
    if ( util::str_compare_ci( splits[ 1 ], "amz_absorb_percent" ) && splits.size() == 2 )
      return expr_t::create_constant( "amz_absorb_percent", options.amz_absorb_percent );
  }

  // Death Knight special expressions
  if ( util::str_compare_ci( splits[ 0 ], "death_knight" ) && splits.size() > 1 )
  {
    // Returns the value of the disable_aotd option
    if ( util::str_compare_ci( splits[ 1 ], "disable_aotd" ) && splits.size() == 2 )
      return expr_t::create_constant( "disable_aotd_expression", this -> options.disable_aotd );

    // Returns the number of targets currently affected by the festering wound debuff
    if ( util::str_compare_ci( splits[ 1 ], "fwounded_targets" ) && splits.size() == 2 )
      return make_fn_expr( "festering_wounds_target_count_expression", [ this ]() {
        return this -> festering_wounds_target_count;
      } );

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
  if ( ( util::str_compare_ci( splits[ 0 ], "defile" ) ||
         util::str_compare_ci( splits[ 0 ], "death_and_decay" ) ) && splits.size() == 2 )
  {
    // Returns true if there's an active dnd
    if ( util::str_compare_ci( splits[ 1 ], "ticking" ) ||
         util::str_compare_ci( splits[ 1 ], "up" ) )
    {
      return make_fn_expr( "dnd_ticking", [ this ]()
      { return active_dnd ? 1 : 0; } );
    }

    // Returns the remaining value on the active dnd, or 0 if there's no dnd
    if ( util::str_compare_ci( splits[ 1 ], "remains" ) )
    {
      return make_fn_expr( "dnd_remains", [ this ]()
      {
        return active_dnd ? active_dnd -> remaining_time().total_seconds() : 0;
      } );
    }

    // Returns true if there's an active dnd AND the player is inside it
    if ( util::str_compare_ci( splits[ 1 ], "active" ) )
    {
      return make_fn_expr( "dnd_ticking", [ this ]()
      { return in_death_and_decay() ? 1 : 0; } );
    }

    // Returns the remaining value on the active dnd if the player is inside it, or 0 otherwise
    if ( util::str_compare_ci( splits[ 1 ], "active_remains" ) )
    {
      return make_fn_expr( "dnd_remains", [ this ]() {
        return in_death_and_decay() ? active_dnd -> remaining_time().total_seconds() : 0;
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
  pets.ghoul_pet = new pets::ghoul_pet_t( this, !talent.unholy.raise_dead.ok() );

  if ( specialization() == DEATH_KNIGHT_UNHOLY )
  {
    // Initialized even if the talent isn't picked for APL purpose
    pets.gargoyle = new pets::gargoyle_pet_t( this, find_action( "summon_gargoyle" ) );

    if ( talent.unholy.all_will_serve.ok() )
    {
      pets.risen_skulker = new pets::risen_skulker_pet_t( this );
    }

    if ( find_action( "army_of_the_dead" ) )
    {
      pets.army_ghouls.set_creation_callback(
        [] ( death_knight_t* p ) { return new pets::army_ghoul_pet_t( p, "army_ghoul" ); } );

      if ( talent.unholy.magus_of_the_dead.ok() )
      {
        pets.army_magus.set_creation_callback(
          [] ( death_knight_t* p ) { return new pets::magus_pet_t( p, "army_magus" ); });
      }
    }

    if ( find_action( "apocalypse" ) )
    {
      pets.apoc_ghouls.set_creation_callback(
        [] ( death_knight_t* p ) { return new pets::army_ghoul_pet_t( p, "apoc_ghoul" ); } );

      if ( talent.unholy.magus_of_the_dead.ok() )
      {
        pets.apoc_magus.set_creation_callback(
          [] ( death_knight_t* p ) { return new pets::magus_pet_t( p, "apoc_magus" ); });
      }
    }
  }

  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    if ( find_action( "dancing_rune_weapon" ) )
    {
      pets.dancing_rune_weapon_pet = new pets::dancing_rune_weapon_pet_t( this, "dancing_rune_weapon" );

      if ( talent.blood.everlasting_bond.ok() )
        pets.everlasting_bond_pet = new pets::dancing_rune_weapon_pet_t( this, "everlasting_bond" );
    }

    if ( talent.blood.bloodworms.ok() )
    {
      pets.bloodworms.set_creation_callback(
        [] ( death_knight_t* p ) { return new pets::bloodworm_pet_t( p ); } );
    }
  }
}

// death_knight_t::composite_attack_haste() =================================

double death_knight_t::composite_melee_haste() const
{
  double haste = player_t::composite_melee_haste();

  if( specialization() == DEATH_KNIGHT_BLOOD && buffs.bone_shield -> up() && talent.blood.improved_bone_shield -> ok() )
  {
    haste *= 1.0 / ( 1.0 + talent.blood.improved_bone_shield -> effectN( 1 ).percent() );
  }

  return haste;
}

// death_knight_t::composite_spell_haste() ==================================

double death_knight_t::composite_spell_haste() const
{
  double haste = player_t::composite_spell_haste();

  if( specialization() == DEATH_KNIGHT_BLOOD && buffs.bone_shield -> up() && talent.blood.improved_bone_shield -> ok() )
  {
    haste *= 1.0 / ( 1.0 + talent.blood.improved_bone_shield -> effectN( 1 ).percent() );
  }

  return haste;
}

// death_knight_t::init_rng =================================================

void death_knight_t::init_rng()
{
  player_t::init_rng();

  rppm.bloodworms = get_rppm( "bloodworms", talent.blood.bloodworms );
  rppm.runic_attenuation = get_rppm( "runic_attenuation", talent.runic_attenuation );
}

// death_knight_t::init_base ================================================

void death_knight_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 5;

  player_t::init_base_stats();

  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility = 0.0;
  base.spell_power_per_intellect = 1.0;

  resources.base[ RESOURCE_RUNIC_POWER ] = 100;
  resources.base[ RESOURCE_RUNIC_POWER ] += spec.blood_death_knight -> effectN( 12 ).resource( RESOURCE_RUNIC_POWER );

  if ( talent.blood.ossuary.ok() )
    resources.base [ RESOURCE_RUNIC_POWER ] += talent.blood.ossuary -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER );

  if ( talent.frost.runic_command.ok() )
    resources.base [ RESOURCE_RUNIC_POWER ] += talent.frost.runic_command -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER );

  if ( talent.unholy.runic_mastery.ok() )
    resources.base [ RESOURCE_RUNIC_POWER ] += talent.unholy.runic_mastery -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER );


  resources.base[ RESOURCE_RUNE        ] = MAX_RUNES;

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.
}

// death_knight_t::init_spells ==============================================

void death_knight_t::init_spells()
{
  player_t::init_spells();

  // Specialization

  // Generic baselines
  spec.plate_specialization = find_specialization_spell( "Plate Specialization" );
  spec.death_knight         = find_spell( 137005 );  // "Death Knight" passive
  spec.death_coil           = find_class_spell( "Death Coil" );
  spec.death_grip           = find_class_spell( "Death Grip" );
  spec.dark_command         = find_class_spell( "Dark Command" );
  spec.death_and_decay      = find_class_spell( "Death and Decay" );
  spec.rune_strike          = find_class_spell( "Rune Strike" );

  // Blood Baselines
  spec.blood_death_knight       = find_specialization_spell( "Blood Death Knight" );
  spec.riposte                  = find_specialization_spell( "Riposte" );
  spec.blood_fortification      = find_specialization_spell( "Blood Fortification" );

  // Frost Baselines
  spec.frost_death_knight    = find_specialization_spell( "Frost Death Knight" );
  spec.remorseless_winter         = find_specialization_spell( "Remorseless Winter" );
  spec.might_of_the_frozen_wastes = find_specialization_spell( "Might of the Frozen Wastes" );
  spec.frostreaper                = find_specialization_spell( "Frostreaper" );

  // Unholy Baselines
  spec.unholy_death_knight = find_specialization_spell( "Unholy Death Knight" );
  spec.festering_wound     = find_spell( 197147 );
  spec.dark_transformation_2 = find_spell( 325554 );

  //////// Class Talent Tree
  // Row 1
  talent.chains_of_ice = find_talent_spell( talent_tree::CLASS, "Chains of Ice" );;
  talent.death_strike = find_talent_spell( talent_tree::CLASS, "Death Strike" );
  talent.raise_dead = find_talent_spell( talent_tree::CLASS, "Raise Dead" );
  // Row 2
  talent.mind_freeze = find_talent_spell( talent_tree::CLASS, "Mind Freeze" );
  talent.antimagic_shell = find_talent_spell( talent_tree::CLASS, "Anti-Magic Shell" );
  talent.cleaving_strikes = find_talent_spell( talent_tree::CLASS, "Cleaving Strikes" );
  // Row 3
  talent.blinding_sleet = find_talent_spell( talent_tree::CLASS, "Blinding Sleet" );
  talent.coldthirst = find_talent_spell( talent_tree::CLASS, "Coldthirst" );
  talent.permafrost = find_talent_spell( talent_tree::CLASS, "Permafrost" );
  talent.improved_death_strike = find_talent_spell( talent_tree::CLASS, "Improved Death Strike" );
  talent.antimagic_barrier = find_talent_spell( talent_tree::CLASS, "Anti-Magic Barrier" );
  talent.march_of_darkness = find_talent_spell( talent_tree::CLASS, "March of Darkness" );
  talent.sacrificial_pact = find_talent_spell( talent_tree::CLASS, "Sacrificial Pact" );
  talent.control_undead = find_talent_spell( talent_tree::CLASS, "Control Undead" );
  talent.enfeeble = find_talent_spell( talent_tree::CLASS, "Enfeeble" );
  // Row 4
  talent.icebound_fortitude = find_talent_spell( talent_tree::CLASS, "Icebound Fortitude" );
  talent.blood_scent = find_talent_spell( talent_tree::CLASS, "Blood Scent" );
  talent.veteran_of_the_third_war = find_talent_spell( talent_tree::CLASS, "Veteran of the Third War" );
  talent.suppression = find_talent_spell( talent_tree::CLASS, "Suppression" );
  talent.brittle = find_talent_spell( talent_tree::CLASS, "Brittle" );
  // Row 5
  talent.acclimation = find_talent_spell( talent_tree::CLASS, "Acclimation" );
  talent.merciless_strikes = find_talent_spell( talent_tree::CLASS, "Merciless Strikes" );
  talent.antimagic_zone = find_talent_spell( talent_tree::CLASS, "Anti-Magic Zone" );
  talent.might_of_thassarian = find_talent_spell( talent_tree::CLASS, "Might of Thassarian" );
  talent.clenching_grasp = find_talent_spell( talent_tree::CLASS, "Clenching Grasp" );
  // Row 6
  talent.proliferating_chill = find_talent_spell( talent_tree::CLASS, "Proliferating Chill" );
  talent.gloom_ward = find_talent_spell(talent_tree::CLASS, "Gloom Ward");
  talent.asphyxiate = find_talent_spell( talent_tree::CLASS, "Asphyxiate" );
  talent.assimilation = find_talent_spell( talent_tree::CLASS, "Assimilation" );
  talent.death_pact = find_talent_spell( talent_tree::CLASS, "Death Pact" );
  talent.grip_of_the_dead = find_talent_spell( talent_tree::CLASS, "Grip of the Dead" );
  talent.deaths_reach = find_talent_spell( talent_tree::CLASS, "Death's Reach" );
  // Row 7
  talent.runic_attenuation = find_talent_spell( talent_tree::CLASS, "Runic Attenuation" );
  talent.wraith_walk = find_talent_spell( talent_tree::CLASS, "Wraith Walk" );
  talent.unholy_ground = find_talent_spell( talent_tree::CLASS, "Unholy Ground" );
  // Row 8
  talent.insidious_chill = find_talent_spell( talent_tree::CLASS, "Insidious Chill" );
  talent.blood_draw = find_talent_spell( talent_tree::CLASS, "Blood Draw" );
  talent.will_of_the_necropolis = find_talent_spell( talent_tree::CLASS, "Will of the Necropolis" );
  talent.deaths_echo = find_talent_spell( talent_tree::CLASS, "Death's Echo" );
  // Row 9
  talent.icy_talons = find_talent_spell( talent_tree::CLASS, "Icy Talons" );
  talent.rune_mastery = find_talent_spell( talent_tree::CLASS, "Rune Mastery" );
  talent.unholy_bond = find_talent_spell( talent_tree::CLASS, "Unholy Bond" );
  // Row 10
  talent.empower_rune_weapon = find_talent_spell( talent_tree::CLASS, "Empower Rune Weapon" );
  talent.abomination_limb = find_talent_spell( talent_tree::CLASS, "Abomination Limb" );
  talent.soul_reaper = find_talent_spell( talent_tree::CLASS, "Soul Reaper" );

  //////// Blood
  // Row 1
  talent.blood.heart_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Heart Strike" );
  // Row 2
  talent.blood.marrowrend = find_talent_spell( talent_tree::SPECIALIZATION, "Marrowrend" );
  talent.blood.blood_boil = find_talent_spell( talent_tree::SPECIALIZATION, "Blood Boil" );
  // Row 3
  talent.blood.vampiric_blood = find_talent_spell( talent_tree::SPECIALIZATION, "Vampiric Blood" );
  talent.blood.crimson_scourge = find_talent_spell( talent_tree::SPECIALIZATION, "Crimson Scourge" );
  // Row 4
  talent.blood.ossuary = find_talent_spell( talent_tree::SPECIALIZATION, "Ossuary" );
  talent.blood.improved_vampiric_blood = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Vampiric Blood" );
  talent.blood.improved_heart_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Heart Strike" );
  talent.blood.deaths_caress = find_talent_spell( talent_tree::SPECIALIZATION, "Death's Caress" );
  // Row 5
  talent.blood.rune_tap = find_talent_spell( talent_tree::SPECIALIZATION, "Rune Tap" );
  talent.blood.heartbreaker = find_talent_spell( talent_tree::SPECIALIZATION, "Heartbreaker" );
  talent.blood.foul_bulwark = find_talent_spell( talent_tree::SPECIALIZATION, "Foul Bulwark" );
  talent.blood.dancing_rune_weapon = find_talent_spell( talent_tree::SPECIALIZATION, "Dancing Rune Weapon" );
  talent.blood.hemostasis = find_talent_spell( talent_tree::SPECIALIZATION, "Hemostasis" );
  talent.blood.perseverance_of_the_ebon_blade = find_talent_spell( talent_tree::SPECIALIZATION, "Perseverance of the Ebon Blade" );
  talent.blood.relish_in_blood = find_talent_spell( talent_tree::SPECIALIZATION, "Relish in Blood" );
  // Row 6
  talent.blood.leeching_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Leeching Strike" );
  talent.blood.improved_bone_shield = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Bone Shield" );
  talent.blood.insatiable_blade = find_talent_spell( talent_tree::SPECIALIZATION, "Insatiable Blade" );
  talent.blood.blooddrinker = find_talent_spell( talent_tree::SPECIALIZATION, "Blooddrinker" );
  talent.blood.consumption = find_talent_spell( talent_tree::SPECIALIZATION, "Consumption" );
  talent.blood.rapid_decomposition = find_talent_spell( talent_tree::SPECIALIZATION, "Rapid Decomposition" );
  // Row 7
  talent.blood.blood_feast = find_talent_spell( talent_tree::SPECIALIZATION, "Blood Feast" );
  talent.blood.blood_tap = find_talent_spell( talent_tree::SPECIALIZATION, "Blood Tap" );
  talent.blood.reinforced_bones = find_talent_spell( talent_tree::SPECIALIZATION, "Reinforced Bones" );
  talent.blood.everlasting_bond = find_talent_spell( talent_tree::SPECIALIZATION, "Everlasting Bond" );
  talent.blood.voracious = find_talent_spell( talent_tree::SPECIALIZATION, "Voracious" );
  talent.blood.bloodworms = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodworms" );
  talent.blood.coagulopathy = find_talent_spell( talent_tree::SPECIALIZATION, "Coagulopathy" );
  // Row 8
  talent.blood.mark_of_blood = find_talent_spell( talent_tree::SPECIALIZATION, "Mark of Blood" );
  talent.blood.tombstone = find_talent_spell( talent_tree::SPECIALIZATION, "Tombstone" );
  talent.blood.gorefiends_grasp = find_talent_spell( talent_tree::SPECIALIZATION, "Gorefiend's Grasp" );
  talent.blood.sanguine_ground = find_talent_spell( talent_tree::SPECIALIZATION, "Sanguine Ground" );
  // Row 9
  talent.blood.shattering_bone = find_talent_spell( talent_tree::SPECIALIZATION, "Shattering Bone" );
  talent.blood.heartrend = find_talent_spell( talent_tree::SPECIALIZATION, "Heartrend" );
  talent.blood.tightening_grasp = find_talent_spell( talent_tree::SPECIALIZATION, "Tightening Grasp" );
  talent.blood.iron_heart = find_talent_spell( talent_tree::SPECIALIZATION, "Iron Heart" );
  talent.blood.red_thirst = find_talent_spell( talent_tree::SPECIALIZATION, "Red Thirst" );
  // Row 10
  talent.blood.bonestorm = find_talent_spell( talent_tree::SPECIALIZATION, "Bonestorm" );
  talent.blood.purgatory = find_talent_spell( talent_tree::SPECIALIZATION, "Purgatory" );
  talent.blood.bloodshot = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodshot" );
  talent.blood.umbilicus_eternus = find_talent_spell( talent_tree::SPECIALIZATION, "Umbilicus Eternus" );

  //////// Frost
  // Row 1
  talent.frost.frost_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Frost Strike" );
  // Row 2
  talent.frost.obliterate = find_talent_spell( talent_tree::SPECIALIZATION, "Obliterate" );
  talent.frost.howling_blast = find_talent_spell( talent_tree::SPECIALIZATION, "Howling Blast" );
  // Row 3
  talent.frost.killing_machine = find_talent_spell( talent_tree::SPECIALIZATION, "Killing Machine" );
  talent.frost.rime = find_talent_spell( talent_tree::SPECIALIZATION, "Rime" );
  // Row 4
  talent.frost.unleashed_frenzy = find_talent_spell( talent_tree::SPECIALIZATION, "Unleashed Frenzy" );
  talent.frost.runic_command = find_talent_spell( talent_tree::SPECIALIZATION, "Runic Command" );
  talent.frost.improved_frost_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Frost Strike" );
  talent.frost.biting_cold = find_talent_spell( talent_tree::SPECIALIZATION, "Biting Cold" );
  // Row 5
  talent.frost.improved_obliterate = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Obliterate" );
  talent.frost.glacial_advance = find_talent_spell( talent_tree::SPECIALIZATION, "Glacial Advance" );
  talent.frost.pillar_of_frost = find_talent_spell( talent_tree::SPECIALIZATION, "Pillar of Frost" );
  talent.frost.improved_rime = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Rime" );
  talent.frost.frostscythe = find_talent_spell( talent_tree::SPECIALIZATION, "Frostscythe" );
  // Row 6
  talent.frost.rage_of_the_frozen_champion = find_talent_spell( talent_tree::SPECIALIZATION, "Rage of the Frozen Champion" );
  talent.frost.frigid_executioner = find_talent_spell( talent_tree::SPECIALIZATION, "Frigid Executioner" );
  talent.frost.horn_of_winter = find_talent_spell( talent_tree::SPECIALIZATION, "Horn of Winter" );
  talent.frost.fatal_fixation = find_talent_spell( talent_tree::SPECIALIZATION, "Fatal Fixation" );
  talent.frost.enduring_strength = find_talent_spell( talent_tree::SPECIALIZATION, "Enduring Strength" );
  talent.frost.frostwhelps_aid = find_talent_spell( talent_tree::SPECIALIZATION, "Frostwhelp's Aid" );
  talent.frost.cold_heart = find_talent_spell( talent_tree::SPECIALIZATION, "Cold Heart" );
  talent.frost.everfrost = find_talent_spell( talent_tree::SPECIALIZATION, "Everfrost" );
  talent.frost.chill_streak = find_talent_spell( talent_tree::SPECIALIZATION, "Chill Streak" );
  // Row 7
  talent.frost.murderous_efficiency = find_talent_spell( talent_tree::SPECIALIZATION, "Murderous Efficiency" );
  talent.frost.inexorable_assault = find_talent_spell( talent_tree::SPECIALIZATION, "Inexorable Assault" );
  talent.frost.icecap = find_talent_spell( talent_tree::SPECIALIZATION, "Icecap" );
  talent.frost.empower_rune_weapon = find_talent_spell( talent_tree::SPECIALIZATION, "Empower Rune Weapon" );
  talent.frost.gathering_storm = find_talent_spell( talent_tree::SPECIALIZATION, "Gathering Storm" );
  talent.frost.enduring_chill = find_talent_spell( talent_tree::SPECIALIZATION, "Enduring Chill" );
  talent.frost.piercing_chill = find_talent_spell( talent_tree::SPECIALIZATION, "Piercing Chill" );
  // Row 8
  talent.frost.bonegrinder = find_talent_spell( talent_tree::SPECIALIZATION, "Bonegrinder" );
  talent.frost.shattering_blade = find_talent_spell( talent_tree::SPECIALIZATION, "Shattering Blade" );
  talent.frost.avalanche = find_talent_spell( talent_tree::SPECIALIZATION, "Avalanche" );
  talent.frost.icebreaker = find_talent_spell( talent_tree::SPECIALIZATION, "Icebreaker" );
  // Row 9
  talent.frost.cold_blooded_rage = find_talent_spell( talent_tree::SPECIALIZATION, "Cold-Blooded Rage" );
  talent.frost.frostwyrms_fury = find_talent_spell( talent_tree::SPECIALIZATION, "Frostwyrm's Fury" );
  talent.frost.invigorating_freeze = find_talent_spell( talent_tree::SPECIALIZATION, "Invigorating Freeze" );
  // Row 10
  talent.frost.obliteration = find_talent_spell( talent_tree::SPECIALIZATION, "Obliteration" );
  talent.frost.absolute_zero = find_talent_spell( talent_tree::SPECIALIZATION, "Absolute Zero" );
  talent.frost.breath_of_sindragosa = find_talent_spell( talent_tree::SPECIALIZATION, "Breath of Sindragosa" );


  //////// Unholy
  // Row 1
  talent.unholy.festering_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Festering Strike");
  // Row 2
  talent.unholy.scourge_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Scourge Strike" );
  talent.unholy.raise_dead = find_talent_spell( talent_tree::SPECIALIZATION, "Raise Dead" );
  // Row 3
  talent.unholy.outbreak = find_talent_spell( talent_tree::SPECIALIZATION, "Outbreak" );
  talent.unholy.dark_transformation = find_talent_spell( talent_tree::SPECIALIZATION, "Dark Transformation" );
  // Row 4
  talent.unholy.unholy_blight = find_talent_spell( talent_tree::SPECIALIZATION, "Unholy Blight" );
  talent.unholy.improved_festering_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Festering Strike" ); // Cmon blizz, lets get creative with names!
  talent.unholy.runic_mastery = find_talent_spell( talent_tree::SPECIALIZATION, "Runic Mastery" );
  talent.unholy.infected_claws = find_talent_spell( talent_tree::SPECIALIZATION, "Infected Claws" );
  // Row 5
  talent.unholy.epidemic = find_talent_spell( talent_tree::SPECIALIZATION, "Epidemic" );
  talent.unholy.replenishing_wounds = find_talent_spell( talent_tree::SPECIALIZATION, "Replenishing Wounds" );
  talent.unholy.feasting_strikes = find_talent_spell( talent_tree::SPECIALIZATION, "Feasting Strikes" );
  talent.unholy.apocalypse = find_talent_spell( talent_tree::SPECIALIZATION, "Apocalypse" );
  talent.unholy.clawing_shadows = find_talent_spell( talent_tree::SPECIALIZATION, "Clawing Shadows" );
  talent.unholy.plaguebringer = find_talent_spell( talent_tree::SPECIALIZATION, "Plaguebringer" );
  talent.unholy.sudden_doom = find_talent_spell( talent_tree::SPECIALIZATION, "Sudden Doom" );
  talent.unholy.all_will_serve = find_talent_spell( talent_tree::SPECIALIZATION, "All Will Serve" );
  // Row 6
  talent.unholy.bursting_sores = find_talent_spell( talent_tree::SPECIALIZATION, "Bursting Sores" );
  talent.unholy.ebon_fever = find_talent_spell( talent_tree::SPECIALIZATION, "Ebon Fever" );
  talent.unholy.unholy_command = find_talent_spell( talent_tree::SPECIALIZATION, "Unholy Command" );
  talent.unholy.magus_of_the_dead = find_talent_spell( talent_tree::SPECIALIZATION, "Magus of the Dead" );
  talent.unholy.ruptured_viscera = find_talent_spell( talent_tree::SPECIALIZATION, "Ruptured Viscera" );
  talent.unholy.improved_death_coil = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Death Coil" );
  talent.unholy.rotten_touch = find_talent_spell( talent_tree::SPECIALIZATION, "Rotten Touch" );
  talent.unholy.unholy_pact = find_talent_spell( talent_tree::SPECIALIZATION, "Unholy Pact" );
  talent.unholy.defile = find_talent_spell( talent_tree::SPECIALIZATION, "Defile" );
  // Row 7
  talent.unholy.vile_contagion = find_talent_spell( talent_tree::SPECIALIZATION, "Vile Contagion" );
  talent.unholy.pestilence = find_talent_spell( talent_tree::SPECIALIZATION, "Pestilence" );
  talent.unholy.eternal_agony = find_talent_spell( talent_tree::SPECIALIZATION, "Eternal Agony" );
  talent.unholy.coil_of_devastation = find_talent_spell( talent_tree::SPECIALIZATION, "Coil of Devastation" );
  talent.unholy.harbinger_of_doom = find_talent_spell( talent_tree::SPECIALIZATION, "Harbinger of Doom" );
  talent.unholy.reaping = find_talent_spell( talent_tree::SPECIALIZATION, "Reaping" );
  // Row 8
  talent.unholy.death_rot = find_talent_spell( talent_tree::SPECIALIZATION, "Death Rot" );
  talent.unholy.army_of_the_dead = find_talent_spell( talent_tree::SPECIALIZATION, "Army of the Dead" );
  talent.unholy.summon_gargoyle = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Gargoyle" );
  // Row 9
  talent.unholy.festermight = find_talent_spell( talent_tree::SPECIALIZATION, "Festermight" );
  talent.unholy.ghoulish_frenzy = find_talent_spell( talent_tree::SPECIALIZATION, "Ghoulish Frenzy" );
  talent.unholy.army_of_the_damned = find_talent_spell( talent_tree::SPECIALIZATION, "Army of the Damned" );
  talent.unholy.morbidity = find_talent_spell( talent_tree::SPECIALIZATION, "Morbidity" ); // ITS MORBIN TIME
  talent.unholy.unholy_aura = find_talent_spell( talent_tree::SPECIALIZATION, "Unholy Aura" );
  // Row 10
  talent.unholy.unholy_assault = find_talent_spell( talent_tree::SPECIALIZATION, "Unholy Assault" );
  talent.unholy.superstrain = find_talent_spell( talent_tree::SPECIALIZATION, "Superstrain" );
  talent.unholy.commander_of_the_dead = find_talent_spell( talent_tree::SPECIALIZATION, "Commander of the Dead" );

  // Shared
  spec.frost_fever        = find_specialization_spell( "Frost Fever" ); // RP generation only

  mastery.blood_shield = find_mastery_spell( DEATH_KNIGHT_BLOOD );
  mastery.frozen_heart = find_mastery_spell( DEATH_KNIGHT_FROST );
  mastery.dreadblade   = find_mastery_spell( DEATH_KNIGHT_UNHOLY );

  // Generic spells
  // Shared
  spell.brittle_debuff               = find_spell( 374557 );
  spell.dnd_buff                     = find_spell( 188290 );
  spell.razorice_debuff              = find_spell( 51714 );
  spell.runic_empowerment_gain       = find_spell( 193486 );
  spell.rune_mastery_buff            = find_spell( 374585 );
  spell.empower_rune_weapon_main     = find_spell( 47568 );
  spell.coldthirst_gain              = find_spell( 378849 );
  spell.unholy_strength_buff         = find_spell( 53365 );
  spell.unholy_ground_buff           = find_spell( 374271 );
  spell.apocalypse_death_debuff      = find_spell( 327095 );
  spell.apocalypse_famine_debuff     = find_spell( 327092 );
  spell.apocalypse_war_debuff        = find_spell( 327096 );
  spell.apocalypse_pestilence_damage = find_spell( 327093 );
  spell.razorice_damage              = find_spell( 50401 );
  spell.death_and_decay_damage       = find_spell( 52212 );
  spell.death_coil_damage            = find_spell( 47632 );
  spell.death_strike_heal            = find_spell( 45470 );
  spell.sacrificial_pact_damage      = find_spell( 327611 );
  spell.soul_reaper_execute          = find_spell( 343295 );
  spell.sanguination_cooldown        = find_spell( 326809 );
  spell.spellwarding_absorb          = find_spell( 326855 );

  // Diseases
  spell.blood_plague       = find_spell( 55078 );
  spell.frost_fever        = find_spell( 55095 );
  spell.virulent_plague    = find_spell( 191587 );
  spell.virulent_erruption = find_spell( 191685 );

  // Blood
  spell.blood_shield                         = find_spell( 77535 );
  spell.bone_shield                          = find_spell( 195181 );
  spell.sanguine_ground                      = find_spell( 391459 );
  spell.tightening_grasp_debuff              = find_spell( 374776 );
  spell.ossuary_buff                         = find_spell( 219788 );
  spell.crimson_scourge_buff                 = find_spell( 81141 );
  spell.heartrend_buff                       = find_spell( 377656 );
  spell.preserverence_of_the_ebon_blade_buff = find_spell( 374748 );
  spell.voracious_buff                       = find_spell( 274009 );
  spell.blood_draw_damage                    = find_spell( 374606 );
  spell.blood_draw_cooldown                  = find_spell( 374609 );
  spell.bonestorm_heal                       = find_spell( 196545 );
  spell.dancing_rune_weapon_buff             = find_spell( 81256 );
  spell.relish_in_blood_gains                = find_spell( 317614 );
  spell.leeching_strike_damage               = find_spell( 377633 );
  spell.shattering_bone_damage               = find_spell( 377642 );
  // T29 Blood
  spell.vigorous_lifeblood_4pc               = find_spell( 394570 );
  spell.vigorous_lifeblood_energize          = find_spell( 394559 );
  // T30 Blood
  spell.vampiric_strength                    = find_spell( 408356 );

  // Frost
  spell.murderous_efficiency_gain     = find_spell( 207062 );
  spell.rage_of_the_frozen_champion   = find_spell( 341725 );
  spell.piercing_chill_debuff         = find_spell( 377359 );
  spell.runic_empowerment_chance      = find_spell( 81229 );
  spell.gathering_storm_buff          = find_spell( 211805 );
  spell.inexorable_assault_buff       = find_spell( 253595 );
  spell.bonegrinder_crit_buff         = find_spell( 377101 );
  spell.bonegrinder_frost_buff        = find_spell( 377103 );
  spell.enduring_strength_buff        = find_spell( 377195 );
  spell.frostwhelps_aid_buff          = find_spell( 377253 );
  spell.inexorable_assault_damage     = find_spell( 253597 );
  spell.breath_of_sindragosa_rune_gen = find_spell( 303753 );
  spell.cold_heart_damage             = find_spell( 281210 );
  spell.chill_streak_damage           = find_spell( 204167 );
  spell.death_strike_offhand          = find_spell( 66188 );
  spell.frostwyrms_fury_damage        = find_spell( 279303 );
  spell.glacial_advance_damage        = find_spell( 195975 );
  spell.avalanche_damage              = find_spell( 207150 );
  spell.frostwhelps_aid_damage        = find_spell( 377245 );
  spell.enduring_strength_cooldown    = find_spell( 377192 );
  spell.murderous_efficiency_gain     = find_spell( 207062 );
  spell.rage_of_the_frozen_champion   = find_spell( 341725 );
  spell.piercing_chill_debuff         = find_spell( 377359 );
  spell.runic_empowerment_chance      = find_spell( 81229 );
  spell.obliteration_gains            = find_spell( 281327 );
  // T30 Frost
  spell.frost_t30_2pc                 = find_spell( 405501 );
  spell.frost_t30_4pc                 = find_spell( 405502 );
  spell.lingering_chill               = find_spell( 410879 );
  spell.wrath_of_the_frostwyrm_damage = find_spell( 410790 );

  // Unholy
  spell.runic_corruption           = find_spell( 51460 );
  spell.runic_corruption_chance    = find_spell( 51462 );
  spell.festering_wound_debuff     = find_spell( 194310 );
  spell.rotten_touch_debuff        = find_spell( 390276 );
  spell.death_rot_debuff           = find_spell( 377540 );
  spell.coil_of_devastation_debuff = find_spell( 390271 );
  spell.feasting_strikes_gain      = find_spell( 390162 );
  spell.ghoulish_frenzy_player     = find_spell( 377587 );
  spell.plaguebringer_buff         = find_spell( 390178 );
  spell.festermight_buff           = find_spell( 377591 );
  spell.ghoulish_infusion          = find_spell( 394899 );
  spell.unholy_blight_dot          = find_spell( 115994 );
  spell.commander_of_the_dead      = find_spell( 390260 );
  spell.defile_buff                = find_spell( 218100 );
  spell.ruptured_viscera_chance    = find_spell( 390236 );
  spell.apocalypse_duration        = find_spell( 221180 );
  spell.apocalypse_rune_gen        = find_spell( 343758 );
  spell.unholy_pact_damage         = find_spell( 319236 );
  spell.dark_transformation_damage = find_spell( 344955 );
  spell.defile_damage              = find_spell( 156000 );
  spell.epidemic_damage            = find_spell( 212739 );
  spell.bursting_sores_damage      = find_spell( 207267 );
  spell.festering_wound_damage     = find_spell( 194311 );
  spell.outbreak_aoe               = find_spell( 196780 );
  // T30
  spell.unholy_t30_2pc_stacking    = find_spell( 408375 );
  spell.unholy_t30_2pc_mastery     = find_spell( 408376 );
  spell.unholy_t30_4pc_mastery     = find_spell( 408377 );
  spell.unholy_t30_2pc_values      = find_spell( 405503 );
  spell.unholy_t30_4pc_values      = find_spell( 405504 );

  // Pet abilities
  // Raise Dead abilities, used for both rank 1 and rank 2
  pet_spell.ghoul_claw              = find_spell( 91776 );
  pet_spell.sweeping_claws          = find_spell( 91778 );
  pet_spell.gnaw                    = find_spell( 91800 );
  pet_spell.monstrous_blow          = find_spell( 91797 );
  // Army of the dead
  pet_spell.army_claw               = find_spell( 199373 );
  // All Ghouls
  pet_spell.pet_stun                = find_spell( 47466 );
  // Gargoyle
  pet_spell.gargoyle_strike         = find_spell( 51963 );
  pet_spell.dark_empowerment        = find_spell( 211947 );
  // Risen Skulker (all will serve)
  pet_spell.skulker_shot            = find_spell( 212423 );
  // Magus of the dead (army of the damned)
  pet_spell.frostbolt               = find_spell( 317792 );
  pet_spell.shadow_bolt             = find_spell( 317791 );
  // Commander of the Dead Talent
  pet_spell.commander_of_the_dead   = find_spell( 390264 );
  // Ruptured Viscera Talent
  pet_spell.ruptured_viscera        = find_spell( 390220 );
  // Ghoulish Frenzy
  pet_spell.ghoulish_frenzy         = find_spell ( 377587 );
  // Vile Infusion
  pet_spell.vile_infusion           = find_spell ( 394863 );
  // DRW Spells
  pet_spell.drw_heart_strike        = find_spell( 228645 );
  pet_spell.drw_heart_strike_rp_gen = find_spell( 220890 );

  // Custom/Internal cooldowns default durations
  cooldown.bone_shield_icd -> duration = spell.bone_shield -> internal_cooldown();

  if ( talent.frost.icecap.ok() )
    cooldown.icecap_icd -> duration = talent.frost.icecap -> internal_cooldown();

  if ( talent.frost.enduring_strength.ok() )
	cooldown.enduring_strength_icd -> duration = spell.enduring_strength_cooldown -> internal_cooldown();

  if ( talent.frost.inexorable_assault.ok() )
    cooldown.inexorable_assault_icd -> duration = spell.inexorable_assault_buff -> internal_cooldown();  // Inexorable Assault buff spell id

  if ( talent.abomination_limb )
  {
    cooldown.abomination_limb -> duration = timespan_t::from_seconds( talent.abomination_limb -> effectN ( 4 ).base_value() );
  }

  if ( talent.frost.frigid_executioner )
    cooldown.frigid_executioner_icd -> duration = talent.frost.frigid_executioner -> internal_cooldown();

}

// death_knight_t::init_action_list =========================================

void death_knight_t::init_action_list()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( ! action_list_str.empty() )
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
    scaling -> enable( STAT_WEAPON_OFFHAND_DPS );

  if ( specialization() == DEATH_KNIGHT_BLOOD )
    scaling -> enable( STAT_BONUS_ARMOR );

  scaling -> disable( STAT_AGILITY );
}

// death_knight_t::init_buffs ===============================================
void death_knight_t::create_buffs()
{
  player_t::create_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  // Shared
  buffs.antimagic_shell = new antimagic_shell_buff_t( this );

  buffs.antimagic_zone = new antimagic_zone_buff_t( this );

  buffs.icebound_fortitude = make_buff( this, "icebound_fortitude", talent.icebound_fortitude )
        -> set_duration( talent.icebound_fortitude -> duration() )
        -> set_cooldown( 0_ms ); // Handled by the action

  buffs.rune_mastery = make_buff(this, "rune_mastery", spell.rune_mastery_buff)
      ->set_chance(0.15)  // This was found through testing 2022 July 21.  Not in spelldata.
      -> set_default_value(talent.rune_mastery->effectN(1).percent())
      ->set_pct_buff_type(STAT_PCT_BUFF_STRENGTH);

  buffs.unholy_strength = make_buff( this, "unholy_strength", spell.unholy_strength_buff )
        -> set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
        -> set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
        -> apply_affecting_aura( talent.unholy_bond );

  buffs.unholy_ground = make_buff( this, "unholy_ground", spell.unholy_ground_buff)
        -> set_default_value_from_effect( 1 )
        -> set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buffs.abomination_limb = new abomination_limb_buff_t( this );

  buffs.empower_rune_weapon = make_buff( this, "empower_rune_weapon", spell.empower_rune_weapon_main )
        -> set_tick_zero( true )
        -> set_cooldown( 0_ms )
        -> set_period( spell.empower_rune_weapon_main -> effectN( 1 ).period() )
        -> set_default_value_from_effect( 3 )
        -> set_pct_buff_type( STAT_PCT_BUFF_HASTE )
        -> set_refresh_behavior( buff_refresh_behavior::EXTEND )
        -> set_tick_behavior( buff_tick_behavior::REFRESH )
        -> set_tick_callback( [ this ] ( buff_t* b, int, timespan_t ) 
          {
            replenish_rune( as<unsigned int>( b -> data().effectN( 1 ).base_value() ),
                           gains.empower_rune_weapon );
            resource_gain( RESOURCE_RUNIC_POWER,
                          b -> data().effectN( 2 ).resource( RESOURCE_RUNIC_POWER ),
                          gains.empower_rune_weapon );
          } );

  buffs.icy_talons = make_buff( this, "icy_talons", talent.icy_talons -> effectN( 1 ).trigger() )
        -> add_invalidate( CACHE_ATTACK_SPEED )
        -> set_default_value( talent.icy_talons -> effectN( 1 ).percent() )
        -> set_cooldown( talent.icy_talons->internal_cooldown() )
        -> set_trigger_spell( talent.icy_talons );

  // Blood
  if ( this -> specialization() == DEATH_KNIGHT_BLOOD)
  {
  buffs.blood_shield = new blood_shield_buff_t( this );

  buffs.bone_shield = make_buff( this, "bone_shield", spell.bone_shield )
        -> add_invalidate( CACHE_HASTE )
        -> set_stack_change_callback( [ this ]( buff_t*, int old_stacks, int new_stacks )
          {
            if ( talent.blood.foul_bulwark.ok() ) // Change player's max health if FB is talented
            {
              double fb_health = talent.blood.foul_bulwark -> effectN( 1 ).percent();
              double health_change = ( 1.0 + new_stacks * fb_health ) / ( 1.0 + old_stacks * fb_health );

              resources.initial_multiplier[ RESOURCE_HEALTH ] *= health_change;

              recalculate_resource_max( RESOURCE_HEALTH );
            }

            // Trigger/cancel ossuary
            // Trigger is done regardless of current stacks to feed the 'refresh' data, but requires a stack gain
            if ( talent.blood.ossuary.ok() && new_stacks > old_stacks &&
                 new_stacks >= talent.blood.ossuary -> effectN( 1 ).base_value() )
              buffs.ossuary -> trigger();
            // Only expire if the buff is already up
            else if ( buffs.ossuary -> up() && new_stacks < talent.blood.ossuary -> effectN( 1 ).base_value() )
              buffs.ossuary -> expire();

            // If the buff starts or expires, invalidate relevant caches
            if ( ( ! old_stacks && new_stacks ) || ( old_stacks && ! new_stacks ) )
            {
              invalidate_cache( CACHE_BONUS_ARMOR );
            }
          } )
        // The internal cd in spelldata is for stack loss, handled in bone_shield_handler
        -> set_cooldown( 0_ms );

  buffs.ossuary = make_buff( this, "ossuary", spell.ossuary_buff )
        -> set_default_value_from_effect( 1, 0.1 );

  buffs.coagulopathy = make_buff( this, "coagulopathy", talent.blood.coagulopathy -> effectN( 2 ).trigger() )
        -> set_trigger_spell( talent.blood.coagulopathy )
        -> set_default_value_from_effect( 1 );

  buffs.crimson_scourge = make_buff( this, "crimson_scourge", spell.crimson_scourge_buff )
        -> set_trigger_spell( talent.blood.crimson_scourge );

  buffs.dancing_rune_weapon = make_buff( this, "dancing_rune_weapon", spell.dancing_rune_weapon_buff )
        -> set_cooldown( 0_ms )
        -> set_duration( 0_ms )
        -> set_default_value_from_effect_type( A_MOD_PARRY_PERCENT )
        -> add_invalidate( CACHE_PARRY );

  buffs.heartrend = make_buff( this, "heartrend", spell.heartrend_buff )
        -> set_default_value( talent.blood.heartrend -> effectN ( 1 ).percent() )
        -> set_chance( 0.10 );  // Sept 20 2022.  Chance was found via testing for 30 mins.  Other people have mentioned the same proc rate.  Not in spelldata.

  buffs.hemostasis = make_buff( this, "hemostasis", talent.blood.hemostasis -> effectN( 1 ).trigger() )
        -> set_trigger_spell( talent.blood.hemostasis )
        -> set_default_value_from_effect( 1 );

  buffs.perseverance_of_the_ebon_blade = make_buff( this, "perseverance_of_the_ebon_blade", spell.preserverence_of_the_ebon_blade_buff )
        -> set_default_value( talent.blood.perseverance_of_the_ebon_blade->effectN( 1 ).percent() )
        -> set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY );

  buffs.rune_tap = make_buff( this, "rune_tap", talent.blood.rune_tap )
        -> set_cooldown( 0_ms ); // Handled by the action

  buffs.sanguine_ground = make_buff( this, "sanguine_ground", spell.sanguine_ground )
        ->set_default_value_from_effect( 1 )
        ->set_schools_from_effect( 1 );

  buffs.tombstone = make_buff<absorb_buff_t>( this, "tombstone", talent.blood.tombstone )
        -> set_cooldown( 0_ms ); // Handled by the action

  buffs.vampiric_blood = make_buff( this, "vampiric_blood", talent.blood.vampiric_blood )
        -> set_cooldown( 0_ms )
        -> set_duration( talent.blood.vampiric_blood -> duration() + talent.blood.improved_vampiric_blood -> effectN( 3 ).time_value() )
        -> set_default_value_from_effect( 5 )
        -> set_stack_change_callback( [ this ] ( buff_t*, int, int new_ ) 
          {
            double old_health = resources.current[ RESOURCE_HEALTH ];
            double old_max_health = resources.max[ RESOURCE_HEALTH ];
            auto health_change = talent.blood.vampiric_blood -> effectN( 4 ).percent();

            if ( new_ )
            {
              resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + health_change;
              recalculate_resource_max( RESOURCE_HEALTH );
              resources.current[ RESOURCE_HEALTH ] *= 1.0 + health_change; // Update health after the maximum is increased

              sim -> print_debug( "{} gains Vampiric Blood: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                                  name(), health_change * 100.0,
                                  old_health, resources.current[ RESOURCE_HEALTH ],
                                  old_max_health, resources.max[ RESOURCE_HEALTH ] );
            }
            else
            {
              resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + health_change;
              resources.current[ RESOURCE_HEALTH ] /= 1.0 + health_change; // Update health before the maximum is reduced
              recalculate_resource_max( RESOURCE_HEALTH );

              sim -> print_debug( "{} loses Vampiric Blood: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                        name(), health_change * 100.0,
                        old_health, resources.current[ RESOURCE_HEALTH ],
                        old_max_health, resources.max[ RESOURCE_HEALTH ] );
            }
          } );

  buffs.vampiric_strength = make_buff( this, "vampiric_strength", spell.vampiric_strength )
        -> set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
        -> set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );

  buffs.vigorous_lifeblood_4pc = make_buff( this, "vigorous_lifeblood", spell.vigorous_lifeblood_4pc )
        -> set_default_value_from_effect_type( A_HASTE_ALL )
        -> set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buffs.voracious = make_buff( this, "voracious", spell.voracious_buff )
        -> set_trigger_spell( talent.blood.voracious )
        -> add_invalidate( CACHE_LEECH );
  }

  // Frost
  if( this -> specialization() == DEATH_KNIGHT_FROST )
  {
  buffs.breath_of_sindragosa = new breath_of_sindragosa_buff_t( this );

  buffs.cold_heart = make_buff( this, "cold_heart", talent.frost.cold_heart -> effectN( 1 ).trigger() );

  buffs.gathering_storm = make_buff( this, "gathering_storm", spell.gathering_storm_buff )
        -> set_trigger_spell( talent.frost.gathering_storm )
        -> set_default_value_from_effect( 1 );

  buffs.inexorable_assault = make_buff( this, "inexorable_assault", spell.inexorable_assault_buff );

  buffs.killing_machine = make_buff( this, "killing_machine", talent.frost.killing_machine -> effectN( 1 ).trigger() )
        -> set_chance( 1.0 )
        -> set_default_value_from_effect( 1 )
        -> set_max_stack( talent.frost.killing_machine.ok() ?
                   as<unsigned int>( talent.frost.killing_machine -> effectN( 1 ).trigger() -> max_stacks() + talent.frost.fatal_fixation -> effectN( 1 ).base_value() ) : 1 )
        -> set_stack_change_callback( [ this ] ( buff_t* buff_, int old_, int new_ )
            {
              // in 10.0.7 killing machine has a behavior where dropping from 2 -> 1 stacks will also refresh your buff
              // in 10.1.0 this behavior changed, KM now no longer refreshes, unsure if this is intended
              if (!bugs && new_ > 0 && buff_ -> check() && old_ > new_ )
              {
                buff_ -> refresh();
              }
            } );

  buffs.pillar_of_frost = make_buff( this, "pillar_of_frost", talent.frost.pillar_of_frost)
      ->set_cooldown( 0_ms )
      ->set_default_value( talent.frost.pillar_of_frost -> effectN( 1 ).percent() )
      ->add_invalidate( CACHE_STRENGTH )
      ->set_stack_change_callback( [ this ] ( buff_t*, int, int new_ ) 
          {
            // Right now it is impossible to refresh Pillar of Frost. If it is refreshed though, Enduring Strength builder, as well as Pillar's Bonus strength expire
	    // If enduring strength builder expires in a refresh, trigger the strength buff with the duration of the builder added as it was when pillar was refreshed.
            if ( !new_ || buffs.pillar_of_frost -> up() )
            {
                buffs.pillar_of_frost_bonus -> expire();
            }
            if ( !new_ && talent.frost.enduring_strength.ok() )
            {
              buffs.enduring_strength -> trigger();
              buffs.enduring_strength -> extend_duration( this, talent.frost.enduring_strength -> effectN( 2 ).time_value() * buffs.enduring_strength_builder -> stack() );
              buffs.enduring_strength_builder -> expire();
            }
          } );

  buffs.pillar_of_frost_bonus = make_buff( this, "pillar_of_frost_bonus" )
      ->set_max_stack( 99 )
      ->set_duration( talent.frost.pillar_of_frost -> duration() )
      ->set_default_value( talent.frost.pillar_of_frost -> effectN( 2 ).percent() )
      ->add_invalidate( CACHE_STRENGTH );

  buffs.remorseless_winter = new remorseless_winter_buff_t( this );

  buffs.rime = make_buff( this, "rime", talent.frost.rime -> effectN( 1 ).trigger() )
        -> set_trigger_spell( talent.frost.rime )
        -> set_chance( talent.frost.rime -> effectN( 2 ).percent() + talent.frost.rage_of_the_frozen_champion -> effectN( 1 ).percent() );
		
  buffs.bonegrinder_crit = make_buff( this, "bonegrinder_crit", spell.bonegrinder_crit_buff )
        -> set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
        -> set_pct_buff_type( STAT_PCT_BUFF_CRIT )
        -> set_cooldown( talent.frost.bonegrinder -> internal_cooldown() );
			  
  buffs.bonegrinder_frost = make_buff( this, "bonegrinder_frost", spell.bonegrinder_frost_buff )
        -> set_default_value( talent.frost.bonegrinder -> effectN( 1 ).percent() )
        -> set_schools_from_effect( 1 )
        -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
		
  buffs.enduring_strength_builder = make_buff( this, "enduring_strength_builder", talent.frost.enduring_strength -> effectN( 1 ).trigger() );
  
  buffs.enduring_strength = make_buff( this, "enduring_strength", spell.enduring_strength_buff )
        -> set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
        -> set_default_value( talent.frost.enduring_strength -> effectN( 3 ).percent() ); 
		
  buffs.frostwhelps_aid = make_buff( this, "frostwhelps_aid", spell.frostwhelps_aid_buff )
        -> set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
        -> set_default_value( talent.frost.frostwhelps_aid -> effectN( 3 ).base_value() );

  buffs.unleashed_frenzy = make_buff( this, "unleashed_frenzy", talent.frost.unleashed_frenzy->effectN( 1 ).trigger() )
      -> set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
      -> set_cooldown( talent.frost.unleashed_frenzy -> internal_cooldown() )
      -> set_default_value( talent.frost.unleashed_frenzy -> effectN( 1 ).percent() );

  buffs.wrath_of_the_frostwyrm = make_buff( this, "wrath_of_the_frostwyrm", find_spell ( 408368 ) )
      -> set_default_value( find_spell ( 408368 ) -> effectN( 1 ).percent() );

  }

  // Unholy
  if( this -> specialization() == DEATH_KNIGHT_UNHOLY )
  {
  buffs.dark_transformation = new dark_transformation_buff_t( this );

  buffs.runic_corruption = new runic_corruption_buff_t( this );

  buffs.sudden_doom = make_buff( this, "sudden_doom", talent.unholy.sudden_doom -> effectN( 1 ).trigger() )
          -> set_rppm( RPPM_ATTACK_SPEED, get_rppm( "sudden_doom", talent.unholy.sudden_doom ) -> get_frequency(), 1.0 + talent.unholy.harbinger_of_doom -> effectN( 2 ).percent())
          -> set_trigger_spell( talent.unholy.sudden_doom )
          -> set_max_stack( talent.unholy.sudden_doom.ok() ?
                   as<unsigned int>( talent.unholy.sudden_doom -> effectN( 1 ).trigger() -> max_stacks() + talent.unholy.harbinger_of_doom -> effectN( 1 ).base_value() ):
                   1 );

  buffs.unholy_assault = make_buff( this, "unholy_assault", talent.unholy.unholy_assault )
          -> set_default_value_from_effect( 1 )
          -> set_cooldown( 0_ms ) // Handled by the action
          -> set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buffs.unholy_pact = new unholy_pact_buff_t( this );

  buffs.ghoulish_frenzy = make_buff( this, "ghoulish_frenzy", spell.ghoulish_frenzy_player )
          -> add_invalidate( CACHE_ATTACK_SPEED )
          -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.plaguebringer = make_buff( this, "plaguebringer", spell.plaguebringer_buff )
          ->set_cooldown( talent.unholy.plaguebringer -> internal_cooldown() )
          ->set_default_value( talent.unholy.plaguebringer -> effectN( 1 ).percent() )
          ->set_max_stack( 1 );

  buffs.festermight = make_buff( this, "festermight", spell.festermight_buff )
          ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
          ->set_default_value( talent.unholy.festermight->effectN( 1 ).percent() )
          ->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buffs.ghoulish_infusion = make_buff( this, "ghoulish_infusion", spell.ghoulish_infusion )
          -> set_duration( spell.ghoulish_infusion -> duration() )
          -> set_default_value_from_effect( 2 )
          -> set_pct_buff_type( STAT_PCT_BUFF_HASTE )
          -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.commander_of_the_dead = make_buff( this, "commander_of_the_dead", spell.commander_of_the_dead );

  buffs.defile_buff = make_buff( this, "defile", spell.defile_buff )
          -> add_invalidate( CACHE_MASTERY )
          -> set_default_value( spell.defile_buff -> effectN( 1 ).percent() );

  buffs.unholy_t30_2pc_stacking = make_buff( this, "master_of_death", spell.unholy_t30_2pc_stacking )
          -> set_refresh_behavior( buff_refresh_behavior::DURATION );

  buffs.unholy_t30_4pc_mastery = make_buff( this, "doom_dealer", spell.unholy_t30_4pc_mastery )
          -> set_default_value( spell.unholy_t30_4pc_mastery -> effectN( 1 ).percent() )
          -> add_invalidate( CACHE_MASTERY );

  buffs.unholy_t30_2pc_mastery = make_buff( this, "death_dealer", spell.unholy_t30_2pc_mastery )
          -> set_default_value( spell.unholy_t30_2pc_stacking -> effectN( 1 ).percent() )
          -> set_max_stack( sets -> has_set_bonus ( DEATH_KNIGHT_UNHOLY, T30, B2 ) ? spell.unholy_t30_2pc_stacking -> max_stacks() : 1 )
          -> add_invalidate( CACHE_MASTERY );
  }
}

// death_knight_t::init_gains ===============================================

void death_knight_t::init_gains()
{
  player_t::init_gains();

  // Shared
  gains.antimagic_shell                  = get_gain( "Antimagic Shell" );
  gains.antimagic_zone                   = get_gain( "Antimagic Zone");
  gains.rune                             = get_gain( "Rune Regeneration" );
  gains.rune_of_hysteria                 = get_gain( "Rune of Hysteria" );
  gains.spirit_drain                     = get_gain( "Spirit Drain" );
  gains.start_of_combat_overflow         = get_gain( "Start of Combat Overflow" );
  gains.coldthirst                       = get_gain( "Coldthirst" );

  // Blood
  gains.blood_tap                        = get_gain( "Blood Tap" );
  gains.drw_heart_strike                 = get_gain( "Rune Weapon Heart Strike" );
  gains.heartbreaker                     = get_gain( "Heartbreaker" );
  gains.tombstone                        = get_gain( "Tombstone" );
  gains.bryndaors_might                  = get_gain( "Bryndaor's Might" );
  gains.vigorous_lifeblood_2pc           = get_gain( "Vigorous Lifeblood T29 2pc" );

  // Frost
  gains.breath_of_sindragosa             = get_gain( "Breath of Sindragosa" );
  gains.empower_rune_weapon              = get_gain( "Empower Rune Weapon" );
  gains.frost_fever                      = get_gain( "Frost Fever" );
  gains.horn_of_winter                   = get_gain( "Horn of Winter" );
  gains.murderous_efficiency             = get_gain( "Murderous Efficiency" );
  gains.obliteration                     = get_gain( "Obliteration" );
  gains.rage_of_the_frozen_champion      = get_gain( "Rage of the Frozen Champion" );
  gains.runic_attenuation                = get_gain( "Runic Attenuation" );
  gains.runic_empowerment                = get_gain( "Runic Empowerment" );
  gains.koltiras_favor                   = get_gain( "Koltira's Favor" );
  gains.frigid_executioner               = get_gain( "Frigid Executioner" );

  // Unholy
  gains.apocalypse                       = get_gain( "Apocalypse" );
  gains.festering_wound                  = get_gain( "Festering Wound" );
  gains.replenishing_wounds              = get_gain( "Replenishing Wounds" );
  gains.feasting_strikes                 = get_gain( "Feasting Strikes" );
}

// death_knight_t::init_procs ===============================================

void death_knight_t::init_procs()
{
  player_t::init_procs();

  procs.killing_machine_oblit = get_proc( "Killing Machine spent on Obliterate" );
  procs.killing_machine_fsc   = get_proc( "Killing Machine spent on Frostscythe" );

  procs.km_from_crit_aa           = get_proc( "Killing Machine: Critical auto attacks" );
  procs.km_from_cold_blooded_rage = get_proc( "Killing Machine: Cold-Blooded Rage");
  procs.km_from_obliteration_fs   = get_proc( "Killing Machine: Frost Strike" );
  procs.km_from_obliteration_hb   = get_proc( "Killing Machine: Howling Blast" );
  procs.km_from_obliteration_ga   = get_proc( "Killing Machine: Glacial Advance" );
  procs.km_from_obliteration_sr   = get_proc( "Killing Machine: Soul Reaper" );
  procs.km_from_t29_4pc           = get_proc( "Killing Machine: T29 4pc" );

  procs.km_from_crit_aa_wasted           = get_proc( "Killing Machine wasted: Critical auto attacks" );
  procs.km_from_cold_blooded_rage_wasted = get_proc( "Killing Machine wasted: Cold-Blooded Rage" );
  procs.km_from_obliteration_fs_wasted   = get_proc( "Killing Machine wasted: Frost Strike" );
  procs.km_from_obliteration_hb_wasted   = get_proc( "Killing Machine wasted: Howling Blast" );
  procs.km_from_obliteration_ga_wasted   = get_proc( "Killing Machine wasted: Glacial Advance" );
  procs.km_from_obliteration_sr_wasted   = get_proc( "Killing Machine wasted: Soul Reaper" );
  procs.km_from_t29_4pc_wasted           = get_proc( "Killing Machine wasted: T29 4pc" );

  procs.ready_rune            = get_proc( "Rune ready" );

  procs.rp_runic_corruption   = get_proc( "Runic Corruption from Runic Power Spent" );
  procs.sr_runic_corruption   = get_proc( "Runic Corruption from Soul Reaper" );
  procs.al_runic_corruption   = get_proc( "Runic Corruption from Abomination Limb" );
  procs.fs_runic_corruption   = get_proc( "Runic Corruption from Feasting Strikes" );

  procs.bloodworms            = get_proc( "Bloodworms" );

  procs.fw_festering_strike = get_proc( "Festering Wound from Festering Strike" );
  procs.fw_infected_claws   = get_proc( "Festering Wound from Infected Claws" );
  procs.fw_pestilence       = get_proc( "Festering Wound from Pestilence" );
  procs.fw_unholy_assault   = get_proc( "Festering Wound from Unholy Assault" );
  procs.fw_necroblast       = get_proc( "Festering Wound from Necroblast" );
  procs.fw_vile_contagion   = get_proc( "Festering Wound from Vile Contagion" );
  procs.fw_ruptured_viscera = get_proc( "Festering Wound from Ruptured Viscera" );

}

// death_knight_t::init_finished ============================================

void death_knight_t::init_finished()
{
  player_t::init_finished();

  if ( deprecated_dnd_expression )
  {
    sim -> errorf( "Player %s, Death and Decay and Defile expressions of the form "
                   "'dot.death_and_decay.X' have been deprecated. Use 'death_and_decay.ticking' "
                   "or death_and_decay.remains' instead.", name() );
  }

  if ( runeforge_expression_warning )
  {
    sim -> errorf( "Player %s, Death Knight runeforge expressions of the form "
                  "runeforge.name are to be used with Shadowlands Runeforge legendaries only. "
                  "Use death_knight.runeforge.name instead.", name() );
  }
}

// death_knight_t::activate =================================================

void death_knight_t::activate()
{
  player_t::activate();

  range::for_each( sim->actor_list, [ this ]( player_t* target ) {
    if ( !target->is_enemy() )
    {
      return;
    }

    // On target death triggers
    if ( talent.soul_reaper.ok() )
      target->register_on_demise_callback( this, [this]( player_t* t ) { trigger_soul_reaper_death( t ); } );

    if ( specialization() == DEATH_KNIGHT_UNHOLY )
    {

      if ( spec.festering_wound->ok() )
      {
        target->register_on_demise_callback( this, [this]( player_t* t ) { trigger_festering_wound_death( t ); } );
      }
    }

    if ( talent.unholy.outbreak.ok() || talent.unholy.unholy_blight.ok() )
    {
      target -> register_on_demise_callback( this, [ this ]( player_t* t ) { trigger_virulent_plague_death( t ); } );
    }
  } );
}

// death_knight_t::reset ====================================================

void death_knight_t::reset()
{
  player_t::reset();

  _runes.reset();
  active_dnd = nullptr;
  km_proc_attempts = 0;
  bone_shield_charges_consumed = 0;
}

// death_knight_t::assess_heal ==============================================

void death_knight_t::assess_heal( school_e school, result_amount_type t, action_state_t* s )
{
  if ( specialization() == DEATH_KNIGHT_BLOOD && buffs.vampiric_blood -> up() )
    s -> result_total *= 1.0 + buffs.vampiric_blood -> data().effectN( 1 ).percent() + talent.blood.improved_vampiric_blood -> effectN( 1 ).percent();

  if( talent.blood.sanguine_ground.ok() && in_death_and_decay() )
    s -> result_total *= 1.0 + spell.sanguine_ground -> effectN( 2 ).percent();

  player_t::assess_heal( school, t, s );
}

// death_knight_t::assess_damage ============================================

void death_knight_t::assess_damage( school_e school, result_amount_type type, action_state_t* s )
{
  player_t::assess_damage( school, type, s );

}

// death_knight_t::assess_damage_imminent ===================================

void death_knight_t::bone_shield_handler( const action_state_t* state ) const
{
  if ( ( specialization() == DEATH_KNIGHT_BLOOD && ! buffs.bone_shield -> check() || ! cooldown.bone_shield_icd -> up() || state -> action -> special ) )
  {
    return;
  }

  sim -> print_log( "{} took a successful auto attack and lost a bone shield charge", name() );
  if( specialization() == DEATH_KNIGHT_BLOOD )
  buffs.bone_shield -> decrement();
  cooldown.bone_shield_icd -> start();
  // Blood tap spelldata is a bit weird, it's not in milliseconds like other time values, and is positive even though it reduces a cooldown
  if ( talent.blood.blood_tap.ok() )
  {
    cooldown.blood_tap -> adjust( -1.0 * timespan_t::from_seconds( talent.blood.blood_tap -> effectN( 2 ).base_value() ) );
  }

  if ( talent.blood.shattering_bone.ok() )
    active_spells.shattering_bone -> execute_on_target( target );

  cooldown.dancing_rune_weapon -> adjust( talent.blood.insatiable_blade -> effectN( 1 ).time_value() );

  // T29 Blood tier set
  death_knight_t* dk = debug_cast<death_knight_t*>(state -> action -> target);
  if ( sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T29, B2 ) )
  {
    if ( rng().roll( 0.20 ) ) // 20% chance to proc, only mentioned in tooltip
    {
      dk -> replenish_rune( as<unsigned int>( spell.vigorous_lifeblood_energize -> effectN( 1 ).base_value() ), gains.vigorous_lifeblood_2pc );
    }
  }

  if ( sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T29, B4 ) )
  {
    dk -> bone_shield_charges_consumed++;
    if ( dk -> bone_shield_charges_consumed >= 10 )
    {
      dk -> bone_shield_charges_consumed = 0;
      buffs.vigorous_lifeblood_4pc -> trigger();
    }
  }
}

void death_knight_t::assess_damage_imminent( school_e school, result_amount_type, action_state_t* s )
{
  bone_shield_handler( s );

  if ( school != SCHOOL_PHYSICAL )
  {
    if ( buffs.antimagic_shell -> up() )
    {
      double damage_absorbed = debug_cast< antimagic_shell_buff_t* >( buffs.antimagic_shell ) -> absorb_damage( s -> result_amount );

      s -> result_amount -= damage_absorbed;
      s -> result_absorbed -= damage_absorbed;
      s -> self_absorb_amount += damage_absorbed;
      iteration_absorb_taken += damage_absorbed;

      if ( antimagic_shell )
        antimagic_shell -> add_result( damage_absorbed, damage_absorbed, result_amount_type::ABSORB, RESULT_HIT, BLOCK_RESULT_UNBLOCKED, this );

      // Generates 1 RP for every 1% max hp absorbed
      double rp_generated = damage_absorbed / resources.max[ RESOURCE_HEALTH ] * 100;

      resource_gain( RESOURCE_RUNIC_POWER, util::round( rp_generated ), gains.antimagic_shell, s -> action );
    }

    if ( buffs.antimagic_zone -> up() )
    {
      // AMZ only absorbs 20% of incoming magic damage
      double damage_absorbed = debug_cast< antimagic_zone_buff_t* >( buffs.antimagic_zone ) -> absorb_damage( s -> result_amount * 0.2 );

      s -> result_amount -= damage_absorbed;
      s -> result_absorbed -= damage_absorbed;
      s -> self_absorb_amount += damage_absorbed;
      iteration_absorb_taken += damage_absorbed;

      if ( antimagic_zone )
        antimagic_zone -> add_result( damage_absorbed, damage_absorbed, result_amount_type::ABSORB, RESULT_HIT, BLOCK_RESULT_UNBLOCKED, this );

      // Generates 1 RP for every 1% of the shield
      if (talent.assimilation)
      {
        double absorb_pct = resources.max[ RESOURCE_HEALTH ] * 1.5 * ( 1 + talent.assimilation -> effectN( 1 ).percent() ) * ( 1 + cache.heal_versatility() );
        // Assimilation can generate no more than 100 runic power 
        double rp_generated = std::min( talent.assimilation -> effectN( 2 ).base_value() * 100, damage_absorbed / absorb_pct * 100);
        
        resource_gain( RESOURCE_RUNIC_POWER, util::round( rp_generated ), gains.antimagic_zone, s -> action );
      }
    }
  }
}

// death_knight_t::do_damage ================================================

void death_knight_t::do_damage( action_state_t* state )
{
  player_t::do_damage( state );

  if ( state -> result_amount > 0 && talent.blood.mark_of_blood.ok() && ! state -> action -> special )
  {
    const death_knight_td_t* td = get_target_data( state -> action -> player );
    if ( td && td -> debuff.mark_of_blood -> check() )
      active_spells.mark_of_blood_heal -> execute();
  }

  if ( runeforge.rune_of_sanguination )
  {
    // Health threshold and internal cooldown are both handled in ready()
    if ( active_spells.runeforge_sanguination -> ready() )
      active_spells.runeforge_sanguination -> execute();
  }

  if ( talent.blood_draw.ok() && specialization() == DEATH_KNIGHT_BLOOD && active_spells.blood_draw -> ready() )
  {
    active_spells.blood_draw -> execute();
  }
}

// death_knight_t::target_mitigation ========================================

void death_knight_t::target_mitigation( school_e school, result_amount_type type, action_state_t* state )
{
  if ( specialization() == DEATH_KNIGHT_BLOOD )
    state -> result_amount *= 1.0 + spec.blood_fortification -> effectN( 2 ).percent();

  if ( specialization() == DEATH_KNIGHT_BLOOD && buffs.rune_tap -> up() )
    state -> result_amount *= 1.0 + buffs.rune_tap -> data().effectN( 1 ).percent();

  if ( buffs.icebound_fortitude -> up() )
    state -> result_amount *= 1.0 + buffs.icebound_fortitude -> data().effectN( 3 ).percent();

  const death_knight_td_t* td = get_target_data( state -> action -> player );
  if ( td && runeforge.rune_of_apocalypse )
    state -> result_amount *= 1.0 + td -> debuff.apocalypse_famine -> check_stack_value();

  if ( dbc::is_school( school, SCHOOL_MAGIC ) && runeforge.rune_of_spellwarding )
    state -> result_amount *= 1.0 + runeforge.rune_of_spellwarding;

  player_t::target_mitigation( school, type, state );
}

// death_knight_t::composite_bonus_armor =========================================

double death_knight_t::composite_bonus_armor() const
{
  double ba = player_t::composite_bonus_armor();

  if ( specialization() == DEATH_KNIGHT_BLOOD && buffs.bone_shield -> check() )
  {
    ba += spell.bone_shield -> effectN( 1 ).percent() * ( 1.0 + talent.blood.reinforced_bones -> effectN( 1 ).percent() ) * cache.strength();
  }

  return ba;
}

// death_knight_t::composite_attribute_multiplier ===========================

double death_knight_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STRENGTH )
  {
    if ( specialization() == DEATH_KNIGHT_FROST )
    m *= 1.0 + buffs.pillar_of_frost -> check_value() + buffs.pillar_of_frost_bonus -> check_stack_value();

    if ( talent.might_of_thassarian.ok() )
    {
      m *= 1.0 + talent.might_of_thassarian->effectN( 1 ).percent();
    }
  }

  else if ( attr == ATTR_STAMINA )
  {
    m *= 1.0 + talent.veteran_of_the_third_war -> effectN( 1 ).percent()
      + spec.blood_death_knight -> effectN( 13 ).percent();

    m *= 1.0 + spec.blood_fortification -> effectN( 1 ).percent();
  }

  return m;
}

// death_knight_t::matching_gear_multiplier =================================

double death_knight_t::matching_gear_multiplier( attribute_e attr ) const
{
  switch ( specialization() )
  {
    case DEATH_KNIGHT_FROST:
    case DEATH_KNIGHT_UNHOLY:
      if ( attr == ATTR_STRENGTH )
        return spec.plate_specialization -> effectN( 1 ).percent();
      break;
    case DEATH_KNIGHT_BLOOD:
      if ( attr == ATTR_STAMINA )
        return spec.plate_specialization -> effectN( 1 ).percent();
      break;
    default:
      break;
  }

  return 0.0;
}

// death_knight_t::composite_leech ========================================

double death_knight_t::composite_leech() const
{
  double m = player_t::composite_leech();

  if ( specialization() == DEATH_KNIGHT_BLOOD && buffs.voracious -> check() )
  {
    m += buffs.voracious -> data().effectN( 1 ).percent();
  }

  if ( specialization() == DEATH_KNIGHT_BLOOD && talent.blood_scent.ok() )
  {
    m += talent.blood_scent->effectN( 1 ).percent();
  }

  return m;
}

// death_knight_t::composite_melee_expertise ===============================

double death_knight_t::composite_melee_expertise( const weapon_t* ) const
{
  double expertise = player_t::composite_melee_expertise( nullptr );

  expertise += spec.blood_death_knight -> effectN( 10 ).percent();

  return expertise;
}

// warrior_t::composite_parry_rating() ========================================

double death_knight_t::composite_parry_rating() const
{
  double p = player_t::composite_parry_rating();

  // add Riposte
  if ( spec.riposte -> ok() )
    p += composite_melee_crit_rating();

  return p;
}

// death_knight_t::composite_parry ============================================

double death_knight_t::composite_parry() const
{
  double parry = player_t::composite_parry();

  if ( specialization() == DEATH_KNIGHT_BLOOD && buffs.dancing_rune_weapon -> check() )
    parry += buffs.dancing_rune_weapon -> check_value();

  return parry;
}

// death_knight_t::composite_dodge ============================================

double death_knight_t::composite_dodge() const
{
  double dodge = player_t::composite_dodge();

  return dodge;
}

// Player multipliers

double death_knight_t::composite_player_target_multiplier( player_t* target, school_e s ) const
{
  double m = player_t::composite_player_target_multiplier( target, s );

  const death_knight_td_t* td = get_target_data( target );

  return m;
}

double death_knight_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( specialization() == DEATH_KNIGHT_UNHOLY && buffs.ghoulish_frenzy -> check() )
  {
    m *= 1.0 + talent.unholy.ghoulish_frenzy -> effectN( 2 ).percent();
  }
  
  if ( specialization() == DEATH_KNIGHT_FROST && buffs.bonegrinder_frost->check() && dbc::is_school( school, SCHOOL_FROST ) )
  {
    m *= 1.0 + buffs.bonegrinder_frost->check_value();
  }

  if ( specialization() == DEATH_KNIGHT_BLOOD && talent.blood.bloodshot.ok() && buffs.blood_shield -> up() && dbc::is_school( school, SCHOOL_PHYSICAL ) )
  {
    m *= 1.0 + talent.blood.bloodshot -> effectN( 1 ).percent();
  }

  return m;
}

double death_knight_t::composite_player_pet_damage_multiplier( const action_state_t* state, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( state, guardian );

  if ( guardian )
  {
    m *= 1.0 + spec.blood_death_knight -> effectN( 16 ).percent();
    m *= 1.0 + spec.frost_death_knight -> effectN( 4 ).percent();
    m *= 1.0 + spec.unholy_death_knight -> effectN( 4 ).percent();
  }
  else
  {
    m *= 1.0 + spec.blood_death_knight -> effectN( 14 ).percent();
    m *= 1.0 + spec.frost_death_knight -> effectN( 3 ).percent();
    m *= 1.0 + spec.unholy_death_knight -> effectN( 3 ).percent();
  }

  if ( specialization() == DEATH_KNIGHT_BLOOD && buffs.vigorous_lifeblood_4pc -> check() )
  {
    m *= 1.0 + spell.vigorous_lifeblood_4pc -> effectN( 4 ).percent();
  }

  return m;
}

double death_knight_t::composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const
{
  double m = player_t::composite_player_target_pet_damage_multiplier( target, guardian );

  const death_knight_td_t* td = get_target_data( target );

  if ( td )
  {

    if ( td -> debuff.brittle -> check() )
    {
      m *= 1.0 + td -> debuff.brittle -> check_value();
    }

    if ( td -> debuff.tightening_grasp -> check() && !guardian )
    {
      m *= 1.0 + td -> debuff.tightening_grasp -> check_value();
    }

    if ( td && talent.unholy.morbidity.ok() )
    {
      m *= 1.0 + ( td->dot.virulent_plague->is_ticking() * talent.unholy.morbidity->effectN(1).percent() );
      m *= 1.0 + ( td->dot.frost_fever->is_ticking() * talent.unholy.morbidity->effectN(1).percent() );
      m *= 1.0 + ( td->dot.unholy_blight->is_ticking() * talent.unholy.morbidity->effectN(1).percent() );
      // Bugged as of 10/19/22 Morbidity is modifying effects 3, 4 and 5 of Blood Plague, does not include the guardian damage modifier, effect 6.
      if ( !guardian )
      {
        m *= 1.0 + ( td->dot.blood_plague->is_ticking() * talent.unholy.morbidity->effectN(1).percent() );
      }
    }
  }

  return m;
}

// death_knight_t::composite_attack_power_multiplier ==================================

double death_knight_t::composite_attack_power_multiplier() const
{
  double m = player_t::composite_attack_power_multiplier();

  if ( mastery.blood_shield -> ok() )
  {
    m *= 1.0 + mastery.blood_shield -> effectN( 2 ).mastery_value() * cache.mastery();
  }

  return m;
}

// death_knight_t::composite_attack_speed() =================================

double death_knight_t::composite_melee_speed() const
{
  double haste = player_t::composite_melee_speed();

  if ( buffs.icy_talons -> check() )
  {
    haste *= 1.0 / ( 1.0 + buffs.icy_talons -> check_stack_value() );
  }

  if ( specialization() == DEATH_KNIGHT_UNHOLY && buffs.ghoulish_frenzy -> check() )
  {
    haste *= 1.0 / ( 1.0 + talent.unholy.ghoulish_frenzy -> effectN( 1 ).percent() );
  }

  return haste;
}

double death_knight_t::composite_melee_crit_chance() const
{
  double c = player_t::composite_melee_crit_chance();

  c += talent.merciless_strikes->effectN( 1 ).percent();

  return c;
}

double death_knight_t::composite_spell_crit_chance() const
{
  double c = player_t::composite_spell_crit_chance();

  c += talent.merciless_strikes->effectN( 1 ).percent();

  return c;
}

// death_knight_t::composite_tank_crit ======================================

double death_knight_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  c += spec.blood_death_knight -> effectN( 8 ).percent();

  return c;
}

// death_knight_t::composite_mastery_value ===================================
// Additive, post spec mastery modifiers. 
double death_knight_t::composite_mastery_value() const
{
  double m = player_t::composite_mastery_value();

  if ( specialization() == DEATH_KNIGHT_UNHOLY )
  {
    m += buffs.unholy_t30_2pc_mastery -> stack_value();

    m += buffs.unholy_t30_4pc_mastery -> stack_value();

    m += buffs.defile_buff -> check_stack_value();
  }

  return m;
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
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_CRIT_CHANCE:
      if ( spec.riposte -> ok() )
        player_t::invalidate_cache( CACHE_PARRY );
      break;
    case CACHE_MASTERY:
      if ( specialization() == DEATH_KNIGHT_BLOOD )
        player_t::invalidate_cache( CACHE_ATTACK_POWER );
      break;
    case CACHE_STRENGTH:
      if ( spell.bone_shield -> ok() )
        player_t::invalidate_cache( CACHE_BONUS_ARMOR );
      break;
    default: break;
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
  default: return s;
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
  if ( specialization()  == DEATH_KNIGHT_UNHOLY && buffs.runic_corruption -> check() )
  {
    rps *= 1.0 + spell.runic_corruption -> effectN( 1 ).percent() + talent.unholy.runic_mastery -> effectN( 2 ).percent();
  }

  return rps;
}

inline double death_knight_t::rune_regen_coefficient() const
{
  auto coeff = cache.attack_haste();
  // Runic corruption doubles rune regeneration speed
  if ( specialization()  == DEATH_KNIGHT_UNHOLY && buffs.runic_corruption -> check() )
  {
    coeff /= 1.0 + spell.runic_corruption -> effectN( 1 ).percent() + talent.unholy.runic_mastery -> effectN( 2 ).percent();
  }

  return coeff;
}

// death_knight_t::arise ====================================================
void death_knight_t::arise()
{
  player_t::arise();
  if( runeforge.rune_of_the_stoneskin_gargoyle )
    buffs.stoneskin_gargoyle -> trigger();
  start_inexorable_assault();
  start_cold_heart();
}

void death_knight_t::adjust_dynamic_cooldowns()
{
  player_t::adjust_dynamic_cooldowns();

  _runes.update_coefficient();
}

void death_knight_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  // Spec and Class Auras
  action.apply_affecting_aura( spec.death_knight );
  action.apply_affecting_aura( spec.unholy_death_knight );
  action.apply_affecting_aura( spec.frost_death_knight );
  action.apply_affecting_aura( spec.blood_death_knight );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class death_knight_report_t : public player_report_extension_t
{
public:
  death_knight_report_t( death_knight_t& player ) : p( player )
  { }

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
    os.printf( "<td class=\"right\">%.3f / %.3f</td>", p._runes.rune_waste.mean(), p._runes.rune_waste.percentile( .5 ) );
    os.printf( "<td class=\"right\">%.3f</td>", p._runes.rune_waste.percentile( .95 ) );
    os.printf( "<td class=\"right\">%.3f</td>", p._runes.rune_waste.max() );
    os  << "</tr>\n";
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
    os.printf( "<td class=\"right\">%.3f / %.3f</td>", p._runes.cumulative_waste.mean(), p._runes.cumulative_waste.percentile( .5 ) );
    os.printf( "<td class=\"right\">%.3f</td>", p._runes.cumulative_waste.percentile( .95 ) );
    os.printf( "<td class=\"right\">%.3f</td>", p._runes.cumulative_waste.max() );
    os  << "</tr>\n";

    os << "</table>\n";

    os << "</div>\n";
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    if ( p._runes.cumulative_waste.percentile( .5 ) > 0 )
    {
      os << "<div class=\"player-section custom_section\">\n";
      html_rune_waste( os );
      os << "<div class=\"clear\"></div>\n";
      os << "</div>\n";
    }
  }
private:
  death_knight_t& p;
};

// DEATH_KNIGHT MODULE INTERFACE ============================================

struct death_knight_module_t : public module_t {
  death_knight_module_t() : module_t( DEATH_KNIGHT ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto  p = new death_knight_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new death_knight_report_t( *p ) );
    return p;
  }

  void static_init() const override
  {
    unique_gear::register_special_effect(  50401, runeforge::razorice );
    unique_gear::register_special_effect( 166441, runeforge::fallen_crusader );
    unique_gear::register_special_effect(  62157, runeforge::stoneskin_gargoyle );
    unique_gear::register_special_effect( 327087, runeforge::apocalypse );
    unique_gear::register_special_effect( 326913, runeforge::hysteria );
    unique_gear::register_special_effect( 326801, runeforge::sanguination );
    unique_gear::register_special_effect( 326864, runeforge::spellwarding );
    unique_gear::register_special_effect( 326982, runeforge::unending_thirst );
  }
  /*
  void register_hotfixes() const override
  {
      hotfix::register_spell( "Death Knight", "2023-4-7", "Gargoyle Strike Cast time increased to 2.5s", 51963, hotfix::HOTFIX_FLAG_LIVE )
      .field( "cast_time" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 2500 )
      .verification_value( 2000 );
      
      hotfix::register_effect( "Death Knight", "2023-4-7", "Gargoyle Strike buffed by 25%", 44400, hotfix::HOTFIX_FLAG_LIVE )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 0.36 )
      .verification_value( 0.28800 );
            
      hotfix::register_effect( "Death Knight", "2023-4-7", "Scourge Strike Physical buffed by 20%", 48019, hotfix::HOTFIX_FLAG_LIVE )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 0.484476 )
      .verification_value( 0.40373 );

      hotfix::register_effect( "Death Knight", "2023-4-7", "Scourge Strike Shadow buffed by 20%", 214692, hotfix::HOTFIX_FLAG_LIVE )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 0.26688 )
      .verification_value( 0.22240 );
    
      hotfix::register_effect( "Death Knight", "2023-4-7", "Clawing Shadows buffed by 20%", 324719, hotfix::HOTFIX_FLAG_LIVE )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 0.710568 )
      .verification_value( 0.59214 );

      hotfix::register_effect( "Death Knight", "2023-4-7", "Death Coil buffed by 10%", 39872, hotfix::HOTFIX_FLAG_LIVE )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 0.517341 )
      .verification_value( 0.47031 );

      hotfix::register_effect( "Death Knight", "2023-4-7", "Soul Reaper Initial buffed by 15%", 844983, hotfix::HOTFIX_FLAG_LIVE )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 0.4301 )
      .verification_value( 0.37400 );

      hotfix::register_effect( "Death Knight", "2023-4-7", "Soul Reaper Execute buffed by 15%", 844986, hotfix::HOTFIX_FLAG_LIVE )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 1.9734 )
      .verification_value( 1.71600 );
  }
  */
  void init( player_t* ) const override {}
  bool valid() const override { return true; }
  void combat_begin( sim_t* ) const override {}
  void combat_end( sim_t* ) const override {}
};

}// UNNAMED NAMESPACE

const module_t* module_t::death_knight() {
  static death_knight_module_t m;
  return &m;
}
