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
// - Revisit Eradicating Blow, Deaths Due, Koltiras to verify 1H vs 2H behavior, especially with DD cleave
// - Test Frost's damage with atypical weapon setups (single 1H/OH, etc.)
//     on abilities with the 2H penalty or combined AP type
// - Figure out what to do with Obliterate/Frost Strike strikes, reporting, etc.
// TOCHECK: accurate spawn/travel delay timers for all dynamic pets
// - DRW, Magus of the dead, Bloodworms have no delay at all
// - Army ghoul, Apoc ghoul, Raise dead ghoul have a different delay (currently estimated at 4.5s with a 0.1 stddev)
//   TODO: even more variable travel time for apoc/army based on distance from boss on spawn?
// - Gargoyle and Reanimated Shambler have their own delays too

#include "simulationcraft.hpp"
#include "player/pet_spawner.hpp"
#include "class_modules/apl/apl_death_knight.hpp"

namespace { // UNNAMED NAMESPACE

using namespace unique_gear;

struct death_knight_t;
struct runes_t;
struct rune_t;

namespace pets {
  struct army_ghoul_pet_t;
  struct bloodworm_pet_t;
  struct dancing_rune_weapon_pet_t;
  struct gargoyle_pet_t;
  struct ghoul_pet_t;
  struct magus_pet_t;
  struct reanimated_shambler_pet_t;
}

namespace runeforge {
  // Note, razorice uses a different method of initialization than the other runeforges
  void apocalypse( special_effect_t& );
  void fallen_crusader( special_effect_t& );
  void hysteria( special_effect_t& );
  void razorice( special_effect_t& );
  void sanguination( special_effect_t& );
  void spellwarding( special_effect_t& );
  void stoneskin_gargoyle( special_effect_t& );
  void unending_thirst( special_effect_t& ); // Effect only procs on killing blows, NYI
  // Legendary runeforges, blame blizzard for using the same names
  void reanimated_shambler( special_effect_t& );
}

enum runeforge_apocalypse { DEATH, FAMINE, PESTILENCE, WAR, MAX };

// ==========================================================================
// Death Knight Runes
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
    auto remains = this -> remains(), new_duration = remains * ratio;

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
  { return static_cast<T*>( event ); }

  T* cast() const
  { return static_cast<T*>( this ); }

  T* cast()
  { return static_cast<T*>( this ); }

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

  rune_event_t( sim_t* s );

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
  rune_t* consume();
  // Fill this rune and adjust internal rune state
  rune_t* fill_rune( gain_t* gain = nullptr );
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

// ==========================================================================
// Death Knight
// ==========================================================================

struct death_knight_td_t : public actor_target_data_t {
  struct
  {
    // Shared
    dot_t* shackle_the_unworthy;
    // Blood
    dot_t* blood_plague;
    // Frost
    dot_t* frost_fever;
    // Unholy
    dot_t* soul_reaper;
    dot_t* virulent_plague;
  } dot;

  struct
  {
    // Shared
    buff_t* abomination_limb; // Target-specific icd
    // Runeforges
    buff_t* apocalypse_death; // Dummy debuff, healing reduction not implemented
    buff_t* apocalypse_war;
    buff_t* apocalypse_famine;
    buff_t* razorice;

    // Blood
    buff_t* mark_of_blood;
    // Unholy
    buff_t* festering_wound;
    buff_t* unholy_blight;

    // Soulbinds
    buff_t* debilitating_malady;
    buff_t* everfrost;
  } debuff;

  death_knight_td_t( player_t* target, death_knight_t* death_knight );
};

struct death_knight_t : public player_t {
public:
  // Active
  ground_aoe_event_t* active_dnd;

  // Expression warnings
  bool deprecated_dnd_expression;
  bool runeforge_expression_warning;

  // Counters
  unsigned int km_proc_attempts;
  unsigned int festering_wounds_target_count;

  stats_t* antimagic_shell;

  // Buffs
  struct buffs_t {
    // Shared
    buff_t* antimagic_shell;
    buff_t* icebound_fortitude;
    // Runeforges
    buff_t* rune_of_hysteria;
    buff_t* stoneskin_gargoyle;
    buff_t* unholy_strength;

    // Blood
    absorb_buff_t* blood_shield;
    buff_t* bone_shield;
    buff_t* crimson_scourge;
    buff_t* dancing_rune_weapon;
    buff_t* hemostasis;
    buff_t* ossuary;
    buff_t* rune_tap;
    buff_t* tombstone;
    buff_t* vampiric_blood;
    buff_t* voracious;

    // Frost
    buff_t* breath_of_sindragosa;
    buff_t* cold_heart;
    buff_t* empower_rune_weapon;
    buff_t* frozen_pulse;
    buff_t* gathering_storm;
    buff_t* hypothermic_presence;
    buff_t* icy_talons;
    buff_t* inexorable_assault;
    buff_t* killing_machine;
    buff_t* pillar_of_frost;
    buff_t* pillar_of_frost_bonus; // Additional strength from runes spent
    buff_t* remorseless_winter;
    buff_t* rime;

    // Unholy
    buff_t* dark_transformation;
    buff_t* runic_corruption;
    buff_t* sudden_doom;
    buff_t* unholy_assault;
    buff_t* unholy_pact;
    buff_t* unholy_blight;

    // Conduits
    buff_t* eradicating_blow;
    buff_t* meat_shield;
    buff_t* unleashed_frenzy;

    // Legendaries
    buff_t* crimson_rune_weapon;
    buff_t* death_turf; // Phearomones buff
    buff_t* frenzied_monstrosity;

    // Covenants
    buff_t* abomination_limb; // Necrolord
    buff_t* deaths_due; // Night Fae
    buff_t* swarming_mist; // Venthyr
  } buffs;

  struct runeforge_t {
    bool rune_of_apocalypse;
    bool rune_of_hysteria;
    bool rune_of_razorice;
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
    cooldown_t* abomination_limb;
    cooldown_t* death_and_decay_dynamic; // DnD/Defile/Death's due cooldown object
    cooldown_t* shackle_the_unworthy_icd;

    // Blood
    cooldown_t* bone_shield_icd;
    cooldown_t* blood_tap;
    cooldown_t* dancing_rune_weapon;
    cooldown_t* vampiric_blood;
    // Frost
    cooldown_t* empower_rune_weapon;
    cooldown_t* icecap_icd;
    cooldown_t* koltiras_favor_icd;
    cooldown_t* pillar_of_frost;
    // Unholy
    cooldown_t* apocalypse;
    cooldown_t* army_of_the_dead;
    cooldown_t* dark_transformation;
  } cooldown;

  // Active Spells
  struct active_spells_t {
    // Shared
    action_t* razorice_mh;
    action_t* razorice_oh;
    action_t* runeforge_pestilence;
    action_t* rune_of_sanguination;

    // Blood
    action_t* mark_of_blood_heal;

    // Unholy
    action_t* bursting_sores;
    action_t* festering_wound;
    action_t* virulent_eruption;
  } active_spells;

  // Gains
  struct gains_t {
    // Shared
    gain_t* antimagic_shell;
    gain_t* rune; // Rune regeneration
    gain_t* start_of_combat_overflow;
    gain_t* rune_of_hysteria;

    // Blood
    gain_t* blood_tap;
    gain_t* drw_heart_strike;
    gain_t* heartbreaker;
    gain_t* tombstone;

    gain_t* bryndaors_might;

    // Frost
    gain_t* breath_of_sindragosa;
    gain_t* empower_rune_weapon;
    gain_t* frost_fever;
    gain_t* horn_of_winter;
    gain_t* koltiras_favor;
    gain_t* murderous_efficiency;
    gain_t* obliteration;
    gain_t* rage_of_the_frozen_champion;
    gain_t* runic_attenuation;
    gain_t* runic_empowerment;

    // Unholy
    gain_t* apocalypse;
    gain_t* festering_wound;

    // Shadowlands / Covenants
    gain_t* swarming_mist;
  } gains;

  // Specialization
  struct specialization_t {
    // Generic
    const spell_data_t* death_knight;
    const spell_data_t* plate_specialization;
    const spell_data_t* blood_death_knight;
    const spell_data_t* frost_death_knight;
    const spell_data_t* unholy_death_knight;

    // Shared
    const spell_data_t* antimagic_shell;
    const spell_data_t* chains_of_ice;
    const spell_data_t* death_and_decay;
    const spell_data_t* death_and_decay_2;
    const spell_data_t* death_strike;
    const spell_data_t* death_strike_2;
    const spell_data_t* frost_fever; // RP generation spell
    const spell_data_t* icebound_fortitude;
    const spell_data_t* raise_dead;
    const spell_data_t* sacrificial_pact;
    const spell_data_t* veteran_of_the_third_war;

    // Blood
    const spell_data_t* blood_boil;
    const spell_data_t* blood_boil_2;
    const spell_data_t* crimson_scourge;
    const spell_data_t* dancing_rune_weapon;
    const spell_data_t* deaths_caress;
    const spell_data_t* heart_strike;
    const spell_data_t* heart_strike_2;
    const spell_data_t* heart_strike_3;
    const spell_data_t* marrowrend;
    const spell_data_t* marrowrend_2;
    const spell_data_t* ossuary;
    const spell_data_t* riposte;
    const spell_data_t* rune_tap;
    const spell_data_t* rune_tap_2;
    const spell_data_t* vampiric_blood;
    const spell_data_t* vampiric_blood_2;
    const spell_data_t* veteran_of_the_third_war_2;

    // Frost
    const spell_data_t* empower_rune_weapon;
    const spell_data_t* empower_rune_weapon_2;
    const spell_data_t* frost_strike;
    const spell_data_t* frost_strike_2;
    const spell_data_t* frostwyrms_fury;
    const spell_data_t* howling_blast;
    const spell_data_t* killing_machine;
    const spell_data_t* killing_machine_2;
    const spell_data_t* might_of_the_frozen_wastes;
    const spell_data_t* might_of_the_frozen_wastes_2;
    const spell_data_t* obliterate;
    const spell_data_t* obliterate_2;
    const spell_data_t* pillar_of_frost;
    const spell_data_t* pillar_of_frost_2;
    const spell_data_t* remorseless_winter;
    const spell_data_t* remorseless_winter_2;
    const spell_data_t* rime;
    const spell_data_t* rime_2;
    const spell_data_t* runic_empowerment;

    // Unholy
    const spell_data_t* apocalypse;
    const spell_data_t* apocalypse_2;
    const spell_data_t* apocalypse_3;
    const spell_data_t* army_of_the_dead;
    const spell_data_t* dark_transformation;
    const spell_data_t* dark_transformation_2;
    const spell_data_t* death_coil;
    const spell_data_t* death_coil_2;
    const spell_data_t* epidemic;
    const spell_data_t* festering_strike;
    const spell_data_t* festering_strike_2;
    const spell_data_t* festering_wound;
    const spell_data_t* outbreak;
    const spell_data_t* raise_dead_2;
    const spell_data_t* runic_corruption;
    const spell_data_t* scourge_strike;
    const spell_data_t* scourge_strike_2;
    const spell_data_t* sudden_doom;
  } spec;

  // Mastery
  struct mastery_t {
    const spell_data_t* blood_shield;
    const spell_data_t* frozen_heart;
    const spell_data_t* dreadblade;
  } mastery;

  // Talents
  struct talents_t {

    // Frost
    const spell_data_t* inexorable_assault;
    const spell_data_t* icy_talons;
    const spell_data_t* cold_heart;

    const spell_data_t* runic_attenuation;
    const spell_data_t* murderous_efficiency;
    const spell_data_t* horn_of_winter;

    const spell_data_t* avalanche;
    const spell_data_t* frozen_pulse;
    const spell_data_t* frostscythe;

    const spell_data_t* gathering_storm;
    const spell_data_t* hypothermic_presence;
    const spell_data_t* glacial_advance;

    const spell_data_t* icecap;
    const spell_data_t* obliteration;
    const spell_data_t* breath_of_sindragosa;

    // Unholy
    const spell_data_t* infected_claws;
    const spell_data_t* all_will_serve;
    const spell_data_t* clawing_shadows;

    const spell_data_t* bursting_sores;
    const spell_data_t* ebon_fever;
    const spell_data_t* unholy_blight;

    const spell_data_t* pestilent_pustules;
    const spell_data_t* harbinger_of_doom;
    const spell_data_t* soul_reaper;

    const spell_data_t* spell_eater;

    const spell_data_t* pestilence;
    const spell_data_t* unholy_pact;
    const spell_data_t* defile;

    const spell_data_t* army_of_the_damned;
    const spell_data_t* summon_gargoyle;
    const spell_data_t* unholy_assault;

    // Blood
    const spell_data_t* heartbreaker;
    const spell_data_t* blooddrinker;
    const spell_data_t* tombstone;

    const spell_data_t* rapid_decomposition;
    const spell_data_t* hemostasis;
    const spell_data_t* consumption;

    const spell_data_t* foul_bulwark;
    const spell_data_t* relish_in_blood;
    const spell_data_t* blood_tap;

    const spell_data_t* will_of_the_necropolis; // NYI
    const spell_data_t* antimagic_barrier;
    const spell_data_t* mark_of_blood;

    const spell_data_t* voracious;
    const spell_data_t* death_pact; // NYI
    const spell_data_t* bloodworms;

    const spell_data_t* purgatory; // NYI
    const spell_data_t* red_thirst;
    const spell_data_t* bonestorm;
  } talent;

  // Spells
  struct spells_t {
    // Shared
    const spell_data_t* dnd_buff;
    const spell_data_t* razorice_debuff;
    const spell_data_t* deaths_due;

    // Diseases
    const spell_data_t* blood_plague;
    const spell_data_t* frost_fever;
    const spell_data_t* virulent_plague;

    // Blood
    const spell_data_t* blood_shield;
    const spell_data_t* bone_shield;

    // Frost
    const spell_data_t* murderous_efficiency_gain;
    const spell_data_t* rage_of_the_frozen_champion;

    // Unholy
    const spell_data_t* bursting_sores;
    const spell_data_t* festering_wound_debuff;
    const spell_data_t* festering_wound_damage;
    const spell_data_t* runic_corruption;
    const spell_data_t* soul_reaper_execute;
    const spell_data_t* virulent_eruption;

    // Unholy pets abilities
    const spell_data_t* pet_ghoul_claw;
    const spell_data_t* pet_sweeping_claws;
    const spell_data_t* pet_gnaw;
    const spell_data_t* pet_monstrous_blow;
    const spell_data_t* pet_army_claw;
    const spell_data_t* pet_gargoyle_strike;
    const spell_data_t* pet_dark_empowerment;
    const spell_data_t* pet_skulker_shot;
  } spell;

  // RPPM
  struct rppm_t
  {
    real_ppm_t* bloodworms;
    real_ppm_t* runic_attenuation;
    real_ppm_t* reanimated_shambler;
  } rppm;

  // Pets and Guardians
  struct pets_t
  {
    pets::dancing_rune_weapon_pet_t* dancing_rune_weapon_pet;
    pets::gargoyle_pet_t* gargoyle;
    pets::ghoul_pet_t* ghoul_pet;
    pet_t* risen_skulker;

    spawner::pet_spawner_t<pets::army_ghoul_pet_t, death_knight_t> army_ghouls;
    spawner::pet_spawner_t<pets::army_ghoul_pet_t, death_knight_t> apoc_ghouls;
    spawner::pet_spawner_t<pets::bloodworm_pet_t, death_knight_t> bloodworms;
    spawner::pet_spawner_t<pets::magus_pet_t, death_knight_t> magus_of_the_dead;
    spawner::pet_spawner_t<pets::reanimated_shambler_pet_t, death_knight_t> reanimated_shambler;

    pets_t( death_knight_t* p ) :
      army_ghouls( "army_ghoul", p ),
      apoc_ghouls( "apoc_ghoul", p ),
      bloodworms( "bloodworm", p ),
      magus_of_the_dead( "magus_of_the_dead", p ),
      reanimated_shambler( "reanimated_shambler", p )
    {}
  } pets;

  // Procs
  struct procs_t
  {
    proc_t* ready_rune;

    proc_t* bloodworms;

    proc_t* killing_machine_oblit;
    proc_t* killing_machine_fsc;

    proc_t* km_from_crit_aa;
    proc_t* km_from_obliteration_fs;
    proc_t* km_from_obliteration_hb;
    proc_t* km_from_obliteration_ga;

    proc_t* km_from_crit_aa_wasted;
    proc_t* km_from_obliteration_fs_wasted;
    proc_t* km_from_obliteration_hb_wasted;
    proc_t* km_from_obliteration_ga_wasted;

    proc_t* pp_runic_corruption; // from pestilent pustules
    proc_t* rp_runic_corruption; // from RP spent
    proc_t* sr_runic_corruption; // from soul reaper
    proc_t* al_runic_corruption; // from abomination limb

    proc_t* fw_festering_strike;
    proc_t* fw_infected_claws;
    proc_t* fw_pestilence;
    proc_t* fw_unholy_assault;
    proc_t* fw_necroblast;

    proc_t* reanimated_shambler;

  } procs;

  struct soulbind_conduits_t
  { // Commented out = NYI           // ID
    // Shared - Covenant ability conduits
    conduit_data_t brutal_grasp; // Necrolord, 127
    conduit_data_t impenetrable_gloom; // Venthyr, 126
    conduit_data_t proliferation; // Kyrian, 128
    conduit_data_t withering_ground; // Night Fae, 250

    // Shared - other throughput
    // conduit_data_t spirit_drain; Finesse trait, 70

    // Blood
    conduit_data_t debilitating_malady; // 123
    conduit_data_t meat_shield; // Endurance trait, 121
    conduit_data_t withering_plague; // 80

    // Frost
    conduit_data_t accelerated_cold; // 79
    conduit_data_t everfrost;        // 91
    conduit_data_t eradicating_blow; // 83
    conduit_data_t unleashed_frenzy; // 122

    // Unholy
    conduit_data_t convocation_of_the_dead; // 124
    conduit_data_t embrace_death; // 89
    conduit_data_t eternal_hunger; // 65
    conduit_data_t lingering_plague; // 125

    // Defensive - Endurance/Finesse
    // conduit_data_t blood_bond; // Blood only, 86
    // conduit_data_t chilled_resilience; // 68
    // conduit_data_t fleeting_wind; // 99
    // conduit_data_t hardened_bones; // 88
    // conduit_data_t insatiable_appetite; // 108
    // conduit_data_t reinforced_shell; // 74
    // conduit_data_t unending_grip; // 106
  } conduits;

  struct covenant_t
  {
    const spell_data_t* abomination_limb; // Necrolord
    const spell_data_t* deaths_due; // Night Fae
    const spell_data_t* shackle_the_unworthy; // Kyrian
    const spell_data_t* swarming_mist; // Venthyr
  } covenant;

  struct legendary_t
  { // Commented out = NYI                        // bonus ID
    // Shared
    item_runeforge_t phearomones; // 6954
    item_runeforge_t superstrain; // 6953

    // Blood
    item_runeforge_t bryndaors_might; // 6940
    item_runeforge_t crimson_rune_weapon; // 6941

    // Frost                                      // bonus_id
    item_runeforge_t absolute_zero;               // 6946
    item_runeforge_t biting_cold;                 // 6945
    item_runeforge_t koltiras_favor;              // 6944
    item_runeforge_t rage_of_the_frozen_champion; // 7160

    // Unholy
    item_runeforge_t deadliest_coil; // 6952
    item_runeforge_t deaths_certainty; // 6951
    item_runeforge_t frenzied_monstrosity; // 6950
    item_runeforge_t reanimated_shambler; // 6949

    // Defensive/Utility
    // item_runeforge_t deaths_embrace; // 6947
    // item_runeforge_t grip_of_the_everlasting; // 6948
    item_runeforge_t gorefiends_domination; // 6943
    // item_runeforge_t vampiric_aura; // 6942
  } legendary;

  // Death Knight Options
  struct options_t
  {
    bool disable_aotd = false;
    bool split_ghoul_regen = false;
    bool split_obliterate_schools = true;
  } options;

  // Runes
  runes_t _runes;

  death_knight_t( sim_t* sim, util::string_view name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DEATH_KNIGHT, name, r ),
    active_dnd( nullptr ),
    deprecated_dnd_expression( false ),
    runeforge_expression_warning( false ),
    km_proc_attempts( 0 ),
    festering_wounds_target_count( 0 ),
    antimagic_shell( nullptr ),
    buffs(),
    runeforge(),
    active_spells(),
    gains(),
    spec(),
    mastery(),
    talent(),
    spell(),
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
    cooldown.empower_rune_weapon      = get_cooldown( "empower_rune_weapon" );
    cooldown.icecap_icd               = get_cooldown( "icecap" );
    cooldown.koltiras_favor_icd       = get_cooldown( "koltiras_favor_icd" );
    cooldown.pillar_of_frost          = get_cooldown( "pillar_of_frost" );
    cooldown.shackle_the_unworthy_icd = get_cooldown( "shackle_the_unworthy_icd" );
    cooldown.vampiric_blood           = get_cooldown( "vampiric_blood" );

    resource_regeneration = regen_type::DYNAMIC;
  }

  // Character Definition
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
  double    composite_player_pet_damage_multiplier( const action_state_t* /* state */ ) const override;
  double    composite_crit_avoidance() const override;
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
  action_t* create_action( util::string_view name, const std::string& options ) override;
  std::unique_ptr<expr_t>   create_expression( util::string_view name ) override;
  void      create_options() override;
  void      create_pets() override;
  resource_e primary_resource() const override { return RESOURCE_RUNIC_POWER; }
  role_e    primary_role() const override;
  stat_e    convert_hybrid_stat( stat_e s ) const override;
  void      invalidate_cache( cache_e ) override;
  double    resource_gain( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  double    resource_loss( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  void      copy_from( player_t* other ) override;
  void      merge( player_t& other ) override;
  void      analyze( sim_t& sim ) override;
  void      apply_affecting_auras( action_t& action ) override;

  std::string default_flask() const override { return death_knight_apl::flask( this ); }
  std::string default_food() const override { return death_knight_apl::food( this ); }
  std::string default_potion() const override { return death_knight_apl::potion( this ); }
  std::string default_rune() const override { return death_knight_apl::rune( this ); }
  std::string default_temporary_enchant() const override { return death_knight_apl::temporary_enchant( this ); }

  double    runes_per_second() const;
  double    rune_regen_coefficient() const;
  void      trigger_killing_machine( double chance, proc_t* proc, proc_t* wasted_proc );
  void      consume_killing_machine( proc_t* proc );
  void      trigger_runic_empowerment( double rpcost );
  void      trigger_runic_corruption( proc_t* proc, double rpcost, double override_chance = -1.0 );
  void      trigger_festering_wound( const action_state_t* state, unsigned n_stacks = 1, proc_t* proc = nullptr );
  void      burst_festering_wound( player_t* target, unsigned n = 1 );
  void      bone_shield_handler( const action_state_t* ) const;

  void      start_inexorable_assault();
  void      start_cold_heart();

  void      trigger_soul_reaper_death( player_t* );
  void      trigger_festering_wound_death( player_t* );
  void      trigger_virulent_plague_death( player_t* );

  // Actor is standing in their own Death and Decay or Defile
  bool      in_death_and_decay() const;
  std::unique_ptr<expr_t>   create_runeforge_expression( util::string_view expr_str, bool warning );

  unsigned  replenish_rune( unsigned n, gain_t* gain = nullptr );

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
};

inline rune_event_t::rune_event_t( sim_t* sim ) :
  super( sim ), m_rune( nullptr )
{ }

inline void rune_event_t::execute_event()
{
  sim().print_debug( "{} regenerates a rune, start_time={}, regen_time={} current_coeff={}",
    m_rune -> runes -> dk -> name(), m_rune -> regen_start.total_seconds(),
    ( sim().current_time() - m_rune -> regen_start ).total_seconds(),
    m_coefficient );

  m_rune -> fill_rune();
}

inline death_knight_td_t::death_knight_td_t( player_t* target, death_knight_t* p ) :
  actor_target_data_t( target, p )
{
  dot.blood_plague         = target -> get_dot( "blood_plague",         p );
  dot.frost_fever          = target -> get_dot( "frost_fever",          p );
  dot.virulent_plague      = target -> get_dot( "virulent_plague",      p );
  dot.soul_reaper          = target -> get_dot( "soul_reaper",          p );
  dot.shackle_the_unworthy = target -> get_dot( "shackle_the_unworthy", p );

  debuff.mark_of_blood     = make_buff( *this, "mark_of_blood", p -> talent.mark_of_blood )
                           -> set_cooldown( 0_ms ); // Handled by the action
  debuff.razorice          = make_buff( *this, "razorice", p -> spell.razorice_debuff )
                           -> set_default_value_from_effect( 1 )
                           -> set_period( 0_ms );
  debuff.festering_wound   = make_buff( *this, "festering_wound", p -> spell.festering_wound_debuff )
                           -> set_cooldown( 0_ms ) // Handled by death_knight_t::trigger_festering_wound()
                           -> set_stack_change_callback( [ p ]( buff_t*, int old_, int new_ )
                           {
                             // Update the FW target count if needed
                             if ( old_ == 0 )
                               p -> festering_wounds_target_count++;
                             else if ( new_ == 0 )
                               p -> festering_wounds_target_count--;
                           } );

  debuff.apocalypse_death  = make_buff( *this, "death", p -> find_spell( 327095 ) ); // Effect not implemented
  debuff.apocalypse_famine = make_buff( *this, "famine", p -> find_spell( 327092 ) )
                           -> set_default_value_from_effect( 1 );
  debuff.apocalypse_war    = make_buff( *this, "war", p -> find_spell( 327096 ) )
                           -> set_default_value_from_effect( 1 );

  debuff.debilitating_malady = make_buff( *this, "debilitating_malady", p -> find_spell( 338523 ) )
                           -> set_default_value( p -> conduits.debilitating_malady.percent() );

  debuff.everfrost         = make_buff( *this, "everfrost", p -> find_spell( 337989 ) )
                           -> set_default_value( p -> conduits.everfrost.percent() );

  debuff.unholy_blight     = make_buff( *this, "unholy_blight_debuff", p -> talent.unholy_blight -> effectN( 1 ).trigger() )
                           -> set_default_value_from_effect( 2 );

}

// ==========================================================================
// Local Utility Functions
// ==========================================================================
// RUNE UTILITY
// Log rune status ==========================================================

static void log_rune_status( const death_knight_t* p, bool debug = false ) {
  std::string rune_string = p -> _runes.string_representation();
  (void) debug;
  p -> sim -> print_debug( "{} runes: {}", p -> name(), rune_string.c_str() );
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
  auto disable_waste = n_full_runes - runes <= MAX_REGENERATING_RUNES;

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

  if ( dk -> sim -> debug )
  {
    log_rune_status( dk );
  }

  // Full runes will be going below the maximum number of regenerating runes, so there's no longer
  // going to be any waste time.
  if ( disable_waste && waste_start >= 0_ms )
  {
    dk -> sim -> print_debug( "{} rune waste, waste ended, n_full_runes={}",
        dk -> name(), runes_full() );

    waste_start = timespan_t::min();
  }

  if ( dk -> talent.frozen_pulse -> ok() )
  {
    auto full_runes = static_cast<int>( dk -> resources.current[ RESOURCE_RUNE ] );
    if ( ! dk -> buffs.frozen_pulse -> check() &&
         full_runes < dk -> talent.frozen_pulse -> effectN( 2 ).base_value() )
    {
      dk -> buffs.frozen_pulse -> trigger();
    }
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
  std::vector<rune_t*> regenerating_runes, depleted_runes;
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
    timespan_t lv = l -> event -> remains(), rv = r -> event -> remains();
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
    while ( regenerating_runes.size() > 0 && regenerating_runes.front() -> is_ready() )
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
    if ( regenerating_runes.size() == 0 )
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

  if ( dk -> sim -> debug )
  {
    log_rune_status( dk );
  }
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
    timespan_t lv = l -> event -> remains(), rv = r -> event -> remains();
    // Use pointers as tiebreaker
    if ( lv == rv )
    {
      return l < r;
    }

    return lv < rv;
  } );

  // Number of unsatisfied runes
  unsigned n_unsatisfied = n_runes - as<unsigned>( dk -> resources.current[ RESOURCE_RUNE ] );

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

inline rune_t* rune_t::consume()
{
  rune_t* new_regenerating_rune = nullptr;

  assert( state == STATE_FULL && event == nullptr );

  state = STATE_DEPLETED;

  // Immediately update the state of the next regenerating rune, since rune_t::regen_rune presumes
  // that the internal state of each invidual rune is always consistent with the rune regeneration
  // rules
  if ( runes -> runes_regenerating() < MAX_REGENERATING_RUNES )
  {
    new_regenerating_rune = runes -> first_depleted_rune();
    new_regenerating_rune -> start_regenerating();
  }

  // Internal state consistency for current rune regeneration rules
  assert( runes -> runes_regenerating() <= MAX_REGENERATING_RUNES );
  assert( runes -> runes_depleted() == MAX_RUNES - runes -> runes_full() - runes -> runes_regenerating() );

  return new_regenerating_rune;
}

inline rune_t* rune_t::fill_rune( gain_t* gain )
{
  rune_t* new_regenerating_rune = nullptr;
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
    auto new_regenerating_rune = runes -> first_depleted_rune();
    new_regenerating_rune -> start_regenerating();
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

  if ( runes -> dk -> talent.frozen_pulse -> ok() )
  {
    auto full_runes = static_cast<int>( runes -> dk -> resources.current[ RESOURCE_RUNE ] );
    if ( runes -> dk -> buffs.frozen_pulse -> check() &&
         full_runes >= runes -> dk -> talent.frozen_pulse -> effectN( 2 ).base_value() )
    {
      runes -> dk -> buffs.frozen_pulse -> expire();
    }
  }

  return new_regenerating_rune;
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
  bool use_auto_attack, precombat_spawn;
  double spawn_travel_duration, spawn_travel_stddev, precombat_spawn_adjust;

  death_knight_pet_t( death_knight_t* owner, const std::string& name, bool guardian = true, bool auto_attack = true, bool dynamic = true ) :
    pet_t( owner -> sim, owner, name, guardian, dynamic ), use_auto_attack( auto_attack ), precombat_spawn( false ),
    spawn_travel_duration( 0 ), spawn_travel_stddev( 0 ),  precombat_spawn_adjust( 0 )
  {
    if ( auto_attack )
    {
      main_hand_weapon.type = WEAPON_BEAST;
    }
  }

  death_knight_t* o() const
  { return debug_cast<death_knight_t*>( owner ); }

  // DK pets have their own base attack speed
  double composite_melee_speed() const override
  { return owner -> cache.attack_haste(); }

  virtual attack_t* create_auto_attack()
  { return nullptr; }

  void apply_affecting_auras( action_t& action ) override
  {
    player_t::apply_affecting_auras( action );

    o()->apply_affecting_auras(action);
  }

  double composite_player_target_multiplier( player_t* target, school_e school ) const override
  {
    double m = pet_t::composite_player_target_multiplier( target, school );

    const death_knight_td_t* td = o() -> get_target_data( target );

    if ( td -> debuff.unholy_blight -> check() )
    {
      m *= 1.0 + td -> debuff.unholy_blight -> stack_value();
    }

    return m;
  }

  double composite_melee_haste() const override
  {
    double haste = pet_t::composite_melee_haste();

    // pets are currently doubledipping and getting their own haste buff on top of scaling with the owner's haste - 11/14/20
    if (o() -> legendary.phearomones -> ok() && o() -> bugs)
    {
      haste *= 1.0 / (1.0 + o() -> buffs.death_turf -> check_value());
    }

    return haste;
  }

  double composite_spell_haste() const override
  {
    double haste = pet_t::composite_spell_haste();

    // pets are currently doubledipping and getting their own haste buff on top of scaling with the owner's haste - 11/14/20
    if (o() -> legendary.phearomones -> ok() && o() -> bugs)
    {
      haste *= 1.0 / (1.0 + o() -> buffs.death_turf -> check_value());
    }

    return haste;
  }

  // Standard Death Knight pet actions

  struct auto_attack_t : public melee_attack_t
  {
    auto_attack_t( death_knight_pet_t* player ) : melee_attack_t( "auto_attack", player )
    {
      assert( player -> main_hand_weapon.type != WEAPON_NONE );
      player -> main_hand_attack = player -> create_auto_attack();
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

  struct spawn_travel_t : public action_t
  {
    bool executed;

    spawn_travel_t( death_knight_pet_t* p ) : action_t( ACTION_OTHER, "spawn_travel", p ),
      executed( false )
    {
      may_miss = false;
      quiet    = true;
    }

    result_e calculate_result( action_state_t* /* s */ ) const override
    { return RESULT_HIT; }

    block_result_e calculate_block_result( action_state_t* ) const override
    { return BLOCK_RESULT_UNBLOCKED; }

    void execute() override
    {
      action_t::execute();
      executed = true;
      debug_cast<death_knight_pet_t*>( player ) -> precombat_spawn = false;
    }

    void cancel() override
    {
      action_t::cancel();
      executed = false;
    }

    timespan_t execute_time() const override
    {
      death_knight_pet_t* p = debug_cast<death_knight_pet_t*>( player );
      double mean_duration = p -> spawn_travel_duration;

      if ( p -> precombat_spawn )
        mean_duration -= p -> precombat_spawn_adjust;
      // Don't bother gaussing null delays
      if ( mean_duration <= 0 )
        return 0_ms;

      return timespan_t::from_seconds( rng().gauss( mean_duration, p -> spawn_travel_stddev ) );
    }

    bool ready() override
    {
      return !executed;
    }
  };

  action_t* create_action( util::string_view name, const std::string& options_str )
  {
    if ( name == "auto_attack" ) return new auto_attack_t( this );
    if ( name == "spawn_travel" ) return new spawn_travel_t( this );

    return pet_t::create_action( name, options_str );
  }

  void init_action_list() override
  {
    action_priority_list_t* def = get_action_priority_list( "default" );
    if ( spawn_travel_duration )
      def -> add_action( "spawn_travel" );
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
  typedef pet_action_t<T_PET, T_ACTION> super;

  pet_action_t( T_PET* pet, util::string_view name, const spell_data_t* spell = spell_data_t::nil()) :
    T_ACTION( name, pet, spell )
  {
    this -> special = true;
    this -> may_crit = true;
  }

  T_PET* p() const
  { return debug_cast<T_PET*>( this -> player ); }

  void init() override
  {
    T_ACTION::init();

    if ( ! this -> player -> sim -> report_pets_separately )
    {
      auto it = range::find_if( p() -> o() -> pet_list, [ this ]( pet_t* pet ) {
        return this -> player -> name_str == pet -> name_str;
      } );

      if ( it != p() -> o() -> pet_list.end() && this -> player != *it )
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
  using super = pet_melee_attack_t<T_PET>;

  bool triggers_runeforge_apocalypse;

  pet_melee_attack_t( T_PET* pet, util::string_view name,
    const spell_data_t* spell = spell_data_t::nil() ) :
    pet_action_t<T_PET, melee_attack_t>( pet, name, spell ),
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

    if ( ! this -> background && this -> trigger_gcd == 0_ms )
    {
      this -> trigger_gcd = 1.5_s;
    }
  }

  void impact( action_state_t* state ) override
  {
    pet_action_t<T_PET, melee_attack_t>::impact( state );

    if ( triggers_runeforge_apocalypse && this -> p() -> o() -> runeforge.rune_of_apocalypse )
    {
      // Only triggers on main target for aoe spells
      if ( this -> target == state -> target )
      {
        int n = static_cast<int>( this -> p() -> rng().range( 0, runeforge_apocalypse::MAX ) );

        death_knight_td_t* td = this -> p() -> o() -> get_target_data( state -> target );

        switch ( n )
        {
        case runeforge_apocalypse::DEATH:
          td -> debuff.apocalypse_death -> trigger();
          break;
        case runeforge_apocalypse::FAMINE:
          td -> debuff.apocalypse_famine -> trigger();
          break;
        case runeforge_apocalypse::PESTILENCE:
          this -> p() -> o() -> active_spells.runeforge_pestilence -> set_target( state -> target );
          this -> p() -> o() -> active_spells.runeforge_pestilence -> execute();
          break;
        case runeforge_apocalypse::WAR:
          td -> debuff.apocalypse_war -> trigger();
          break;
        }
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
  using super = pet_spell_t<T_PET>;

  pet_spell_t( T_PET* pet, util::string_view name,
    const spell_data_t* spell = spell_data_t::nil() ) :
    pet_action_t<T_PET, spell_t>( pet, name, spell )
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
  auto_attack_melee_t( T* player, const std::string& name = "main_hand" ) :
    pet_melee_attack_t<T>( player, name )
  {
    this -> background = this -> repeating = true;
    this -> special = false;
    this -> weapon = &( player -> main_hand_weapon );
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
  base_ghoul_pet_t( death_knight_t* owner, const std::string& name, bool guardian = false, bool dynamic = true ) :
    death_knight_pet_t( owner, name, guardian, true, dynamic )
  {
    main_hand_weapon.swing_time = 2.0_s;

    // Army ghouls, apoc ghouls and raise dead ghoul all share the same spawn/travel animation lock
    spawn_travel_duration = 4.5;
    spawn_travel_stddev = 0.1;
  }

  attack_t* create_auto_attack() override
  { return new auto_attack_melee_t<base_ghoul_pet_t>( this ); }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    resources.base[ RESOURCE_ENERGY ] = 100;
    resources.base_regen_per_second[ RESOURCE_ENERGY ]  = 10;
  }

  resource_e primary_resource() const override
  { return RESOURCE_ENERGY; }

  timespan_t available() const override
  {
    double energy = resources.current[ RESOURCE_ENERGY ];

    // Cheapest Ability need 40 Energy
    if ( energy > 40 )
      return 0.1_s;

    return std::max(
             timespan_t::from_seconds( ( 40 - energy ) / resource_regen_per_second( RESOURCE_ENERGY ) ),
             0.1_s
           );
  }
};

// ===============================================================================
// Raise Dead ghoul (both temporary blood/frost ghouls and permanent unholy ghoul)
// ===============================================================================

struct ghoul_pet_t : public base_ghoul_pet_t
{
  cooldown_t* gnaw_cd; // shared cd between gnaw/monstrous_blow
  gain_t* dark_transformation_gain;
  buff_t* frenzied_monstrosity;

  // Ghoul Pet Abilities ======================================================

  // Generic Dark Transformation pet ability
  struct dt_melee_ability_t : public pet_melee_attack_t<ghoul_pet_t>
  {
    bool usable_in_dt;
    bool triggers_infected_claws;

    dt_melee_ability_t( ghoul_pet_t* pet, util::string_view name,
                        const spell_data_t* spell = spell_data_t::nil(),
                        bool usable_in_dt = true ) :
      pet_melee_attack_t( pet, name, spell ),
      usable_in_dt( usable_in_dt ),
      triggers_infected_claws( false )
    { }

    void impact( action_state_t* state ) override
    {
      pet_melee_attack_t::impact( state );

      if ( triggers_infected_claws &&
           this -> p() -> o() -> talent.infected_claws -> ok() &&
           this -> rng().roll( this -> p() -> o() -> talent.infected_claws -> effectN( 1 ).percent() ) )
      {
        this -> p() -> o() -> trigger_festering_wound( state, 1, this -> p() -> o() -> procs.fw_infected_claws );
      }
    }

    bool ready() override
    {
      if ( usable_in_dt != ( p() -> o() -> buffs.dark_transformation -> check() > 0 ) )
      {
        return false;
      }

      return pet_melee_attack_t::ready();
    }
  };

  struct claw_t : public dt_melee_ability_t
  {
    claw_t( ghoul_pet_t* player, util::string_view options_str ) :
      dt_melee_ability_t( player, "claw", player -> o() -> spell.pet_ghoul_claw, false )
    {
      parse_options( options_str );
      triggers_infected_claws = triggers_runeforge_apocalypse = true;
    }
  };

  struct sweeping_claws_t : public dt_melee_ability_t
  {
    sweeping_claws_t( ghoul_pet_t* player, util::string_view options_str ) :
      dt_melee_ability_t( player, "sweeping_claws", player -> o() -> spell.pet_sweeping_claws )
    {
      parse_options( options_str );
      aoe = -1;
      triggers_infected_claws = triggers_runeforge_apocalypse = true;
    }
  };

  struct gnaw_t : public dt_melee_ability_t
  {
    gnaw_t( ghoul_pet_t* player, util::string_view options_str ) :
      dt_melee_ability_t( player, "gnaw", player -> o() -> spell.pet_gnaw, false )
    {
      parse_options( options_str );
      cooldown = player -> get_cooldown( "gnaw" );
    }
  };

  struct monstrous_blow_t : public dt_melee_ability_t
  {
    monstrous_blow_t( ghoul_pet_t* player, util::string_view options_str ):
      dt_melee_ability_t( player, "monstrous_blow", player -> o() -> spell.pet_monstrous_blow )
    {
      parse_options( options_str );
      cooldown = player -> get_cooldown( "gnaw" );
    }
  };

  ghoul_pet_t( death_knight_t* owner ) :
    base_ghoul_pet_t( owner, "ghoul" , false, false )
  {
    gnaw_cd = get_cooldown( "gnaw" );
    gnaw_cd -> duration = owner -> spell.pet_gnaw -> cooldown();

    // With a permanent pet, make sure that the precombat spawn ignores the spawn/travel delay
    if ( owner -> spec.raise_dead_2 )
      precombat_spawn_adjust = spawn_travel_duration;
  }

  attack_t* create_auto_attack() override
  { return new auto_attack_melee_t< ghoul_pet_t >( this, "main_hand" ); }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = base_ghoul_pet_t::composite_player_multiplier( school );

    m *= 1.0 + o() -> buffs.dark_transformation -> value();

    if ( frenzied_monstrosity -> up() )
      m *= 1.0 + frenzied_monstrosity -> data().effectN( 1 ).percent();

    return m;
  }

  double composite_player_target_multiplier( player_t* target, school_e s ) const override
  {
    double m = base_ghoul_pet_t::composite_player_target_multiplier( target, s );

    death_knight_td_t* td = o() -> get_target_data( target );

    // 2020-12-11: Seems to be increasing the player's damage as well as the main ghoul, but not other pets'
    // Does not use a whitelist, affects all damage sources
    if ( o() -> runeforge.rune_of_apocalypse )
    {
      m *= 1.0 + td -> debuff.apocalypse_war -> stack_value();
    }

    return m;
  }

  double composite_melee_speed() const override
  {
    double haste = base_ghoul_pet_t::composite_melee_speed();

    if ( frenzied_monstrosity->up() )
      haste *= 1.0 / ( 1.0 + frenzied_monstrosity -> data().effectN( 2 ).percent() );

    return haste;
  }

  void init_base_stats() override
  {
    base_ghoul_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = .6;
  }

  void init_gains() override
  {
    base_ghoul_pet_t::init_gains();

    dark_transformation_gain = get_gain( "Dark Transformation" );
  }

  void init_action_list() override
  {
    base_ghoul_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "sweeping_claws" );
    def -> add_action( "claw,if=energy>70" );
    def -> add_action( "monstrous_blow" );
    // Using Gnaw without DT is a dps loss compared to waiting for DT and casting Monstrous Blow
    if ( ! o() -> spec.dark_transformation -> ok() )
      def -> add_action( "Gnaw" );
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
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

    // TODO: change spellID to 334895 once data is regenerated
    frenzied_monstrosity = make_buff( this, "frenzied_monstrosity", find_spell ( 334896 ) )
      -> set_duration( 0_s ); // Buff is handled by DT buff
  }
};

// ==========================================================================
// Army of the Dead and Apocalypse ghouls
// ==========================================================================

struct army_ghoul_pet_t : public base_ghoul_pet_t
{
  struct army_claw_t : public pet_melee_attack_t<army_ghoul_pet_t>
  {
    army_claw_t( army_ghoul_pet_t* player, util::string_view options_str ) :
      super( player, "claw", player -> o() -> spell.pet_army_claw )
    {
      parse_options( options_str );
    }
  };

  army_ghoul_pet_t( death_knight_t* owner, const std::string& name = "army_ghoul" ) :
    base_ghoul_pet_t( owner, name, true )
  { }

  void init_base_stats() override
  {
    base_ghoul_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 0.4;

    // The AP inheritance factor was increased by 6% in patch 8.2
    // Notes mention "damage increased by 6%"
    owner_coeff.ap_from_ap *= 1.06;
  }

  void init_action_list() override
  {
    base_ghoul_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "Claw" );
  }

  void init_gains() override
  {
    base_ghoul_pet_t::init_gains();

    // Group Energy regen for the same pets together to reduce clutter in reporting
    if ( ! o() -> options.split_ghoul_regen && o() -> find_pet( name_str ) )
    {
      this -> gains.resource_regen = o() -> find_pet( name_str ) -> gains.resource_regen;
    }
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "claw" ) return new army_claw_t( this, options_str );

    return base_ghoul_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Gargoyle
// ==========================================================================

struct gargoyle_pet_t : public death_knight_pet_t
{
  buff_t* dark_empowerment;

  struct gargoyle_strike_t : public pet_spell_t<gargoyle_pet_t>
  {
    gargoyle_strike_t( gargoyle_pet_t* player, util::string_view options_str ) :
      super( player, "gargoyle_strike", player -> o() -> spell.pet_gargoyle_strike )
    {
      parse_options( options_str );
    }
  };

  gargoyle_pet_t( death_knight_t* owner ) :
    death_knight_pet_t( owner, "gargoyle", true, false ), dark_empowerment( nullptr )
  {
    resource_regeneration = regen_type::DISABLED;

    spawn_travel_duration = 2.9;
    spawn_travel_stddev = 0.2;
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

  void init_action_list() override
  {
    death_knight_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "Gargoyle Strike" );
  }

  void create_buffs() override
  {
    death_knight_pet_t::create_buffs();

    dark_empowerment = make_buff( this, "dark_empowerment", o() -> spell.pet_dark_empowerment );
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "gargoyle_strike" ) return new gargoyle_strike_t( this, options_str );

    return death_knight_pet_t::create_action( name, options_str );
  }

  void increase_power ( double rp_spent )
  {
    if ( is_sleeping() )
    {
      return;
    }

    double increase = rp_spent * dark_empowerment -> data().effectN( 1 ).percent() / o() -> talent.summon_gargoyle -> effectN( 4 ).base_value();

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
  struct skulker_shot_t : public pet_action_t<risen_skulker_pet_t, ranged_attack_t>
  {
    skulker_shot_t( risen_skulker_pet_t* player, util::string_view options_str ) :
      super( player, "skulker_shot", player -> o() -> spell.pet_skulker_shot )
    {
      parse_options( options_str );
      weapon = &( player -> main_hand_weapon );

      // Risen Skulker deals twice the damage to its main target, and normal damage to the other targets
      base_multiplier *= 2.0;
      aoe = -1;
      base_aoe_multiplier = 0.5;
    }
  };

  risen_skulker_pet_t( death_knight_t* owner ) :
    death_knight_pet_t( owner, "risen_skulker", true, false, false )
  {
    resource_regeneration = regen_type::DISABLED;
    main_hand_weapon.type = WEAPON_BEAST_RANGED;
    main_hand_weapon.swing_time = 2.7_s;
  }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    // As per Blizzard
    // 2016-08-06 Changes to pet, AP ineritance to 200% (guesstimate, based on Skulker Shot data)
    // 2017-01-10 All Will Serve damage increased by 15%.
    owner_coeff.ap_from_ap = 1.0;
  }

  void init_action_list() override
  {
    death_knight_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "Skulker Shot" );
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "skulker_shot" ) return new skulker_shot_t( this, options_str );

    return death_knight_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Dancing Rune Weapon
// ==========================================================================

struct dancing_rune_weapon_pet_t : public death_knight_pet_t
{
  struct drw_td_t : public actor_target_data_t
  {
    dot_t* blood_plague;

    drw_td_t( player_t* target, dancing_rune_weapon_pet_t* p ) :
      actor_target_data_t( target, p )
    {
      blood_plague = target -> get_dot( "blood_plague", p );
    }
  };

  target_specific_t<drw_td_t> target_data;

  const drw_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  drw_td_t* get_target_data( player_t* target ) const override
  {
    drw_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new drw_td_t( target, const_cast<dancing_rune_weapon_pet_t*>( this ) );
    }
    return td;
  }

  struct drw_spell_t : public pet_spell_t<dancing_rune_weapon_pet_t>
  {
    drw_spell_t( dancing_rune_weapon_pet_t* p, const std::string& name, const spell_data_t* s ) :
      super( p, name, s )
    {
      background = true;
    }

    // Override verify actor spec, the pet's abilities are blood's abilities and require blood spec in spelldata
    // The pet can only be spawned in blood spec and all the method does is set the ability to background=true
    // Which doesn't stop us from using DRW abilities anyway (they're called directly with -> execute())
    bool verify_actor_spec() const override
    {
      return true;
    }
  };

  struct drw_attack_t : public pet_melee_attack_t<dancing_rune_weapon_pet_t>
  {
    drw_attack_t( dancing_rune_weapon_pet_t* p, util::string_view name, const spell_data_t* s ) :
      super( p, name, s )
    {
      background = true;
      weapon = &( p -> main_hand_weapon );
    }

    // Override verify actor spec, the pet's abilities are blood's abilities and require blood spec in spelldata
    // The pet can only be spawned in blood spec and all the method does is set the ability to background=true
    // Which doesn't stop us from using DRW abilities anyway (they're called directly with -> execute())
    bool verify_actor_spec() const override
    {
      return true;
    }
  };

  struct blood_plague_t : public drw_spell_t
  {
    blood_plague_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( p, "blood_plague", p -> o() -> spell.blood_plague )
    {
      // DRW usually behaves the same regardless of talents, but BP ticks are affected by rapid decomposition
      base_tick_time *= 1.0 + p -> o() -> talent.rapid_decomposition -> effectN( 1 ).percent();
    }
  };

  struct blood_boil_t: public drw_spell_t
  {
    blood_boil_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( p, "blood_boil", p -> o() -> spec.blood_boil )
    {
      aoe = -1;
      cooldown -> duration = 0_ms;
      impact_action = p -> ability.blood_plague;
    }
  };

  struct deaths_caress_t : public drw_spell_t
  {
    deaths_caress_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( p, "deaths_caress", p -> o() -> spec.deaths_caress )
    {
      impact_action = p -> ability.blood_plague;
    }
  };

  struct death_strike_t : public drw_attack_t
  {
    death_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_attack_t( p, "death_strike", p -> o() -> spec.death_strike )
    { }

    double action_multiplier() const override
    {
      double m = drw_attack_t::action_multiplier();

      // DRW isn't usually affected by talents, and doesn't proc hemostasis, yet its death strike damage are increased by hemostasis
      // https://github.com/SimCMinMax/WoW-BugTracker/issues/241
      m *= 1.0 + p() -> o() -> buffs.hemostasis -> stack_value();

      return m;
    }
  };

  struct heart_strike_t : public drw_attack_t
  {
    double blood_strike_rp_generation;

    heart_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_attack_t( p, "heart_strike", p -> o() -> spec.heart_strike ),
      blood_strike_rp_generation( p -> find_spell( 220890 ) -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) )
    {
      base_multiplier *= 1.0 + p -> o() -> spec.heart_strike_3 -> effectN( 1 ).percent();

      // DRW is still using an old spell called "Blood Strike" for the 5 additional RP generation on Heart Strike
      blood_strike_rp_generation = p -> find_spell( 220890 ) -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER );
    }

    int n_targets() const override
    { return p() -> o() -> in_death_and_decay() ? aoe + as<int>( p() -> o() -> spec.death_and_decay_2 -> effectN( 1 ).base_value() ) : aoe; }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = drw_attack_t::composite_target_multiplier( t );

      if ( p() -> o() -> conduits.withering_plague -> ok() && p() -> get_target_data( t ) -> blood_plague -> is_ticking() )
      {
        m *= 1.0 + p() -> o() -> conduits.withering_plague.percent();
      }

      return m;
    }

    void execute( ) override
    {
      drw_attack_t::execute();

      if ( p() -> o() -> legendary.gorefiends_domination.ok() )
      {
        p() -> o() -> cooldown.vampiric_blood -> adjust( -timespan_t::from_seconds( p() -> o() -> legendary.gorefiends_domination -> effectN( 1 ).base_value() ) );
      }

      p() -> o() -> resource_gain( RESOURCE_RUNIC_POWER, blood_strike_rp_generation, p() -> o() -> gains.drw_heart_strike, this );
    }
  };

  struct marrowrend_t : public drw_attack_t
  {
    marrowrend_t( dancing_rune_weapon_pet_t* p ) :
      drw_attack_t( p, "marrowrend", p -> o() -> spec.marrowrend )
    { }

    void impact( action_state_t* state ) override
    {
      drw_attack_t::impact( state );

      int stack_gain = as<int>( data().effectN( 3 ).base_value() );

      p() -> o() -> buffs.bone_shield -> trigger( stack_gain );
    }
  };

  struct abilities_t
  {
    drw_spell_t*  blood_plague;
    drw_spell_t*  blood_boil;
    drw_spell_t*  deaths_caress;

    drw_attack_t* death_strike;
    drw_attack_t* heart_strike;
    drw_attack_t* marrowrend;
  } ability;

  dancing_rune_weapon_pet_t( death_knight_t* owner ) :
    death_knight_pet_t( owner, "dancing_rune_weapon", true, true ),
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

    // Kludge of the century to get pointless initialization warnings to
    // go away.
    type = DEATH_KNIGHT; _spec = DEATH_KNIGHT_BLOOD;

    ability.blood_plague  = new blood_plague_t ( this );
    ability.blood_boil    = new blood_boil_t   ( this );
    ability.deaths_caress = new deaths_caress_t( this );
    ability.death_strike  = new death_strike_t ( this );
    ability.heart_strike  = new heart_strike_t ( this );
    ability.marrowrend    = new marrowrend_t   ( this );

    type = PLAYER_GUARDIAN; _spec = SPEC_NONE;
  }

  void arise() override
  {
    death_knight_pet_t::arise();
    o() -> buffs.dancing_rune_weapon -> trigger();
  }

  void demise() override
  {
    death_knight_pet_t::demise();
    o() -> buffs.dancing_rune_weapon -> expire();
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
// Magus of the Dead (Army of the Damned)
// ==========================================================================

struct magus_pet_t : public death_knight_pet_t
{
  struct magus_td_t : public actor_target_data_t
  {
    buff_t* frostbolt_debuff;

    magus_td_t( player_t* target, magus_pet_t* p ) :
      actor_target_data_t( target, p )
    {
      frostbolt_debuff = make_buff( *this, "frostbolt_magus_of_the_dead", p -> owner -> find_spell( 288548 ) );
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

  struct magus_spell_t : public pet_action_t<magus_pet_t, spell_t>
  {
    magus_spell_t( magus_pet_t* player, util::string_view name , const spell_data_t* spell, util::string_view options_str ) :
      super( player, name, spell )
    {
      parse_options( options_str );
    }

    // There's a 1 energy cost in spelldata but it might as well be ignored
    double cost() const override
    { return 0; }
  };

  struct frostbolt_magus_t : public magus_spell_t
  {
    frostbolt_magus_t( magus_pet_t* player, const std::string& options_str ) :
      magus_spell_t( player, "frostbolt", player -> o() -> find_spell( 317792 ), options_str )
    {
      // If the target is immune to slows, frostbolt seems to be used at most every 3 seconds
      cooldown -> duration = 3_s;
    }

    // Frostbolt applies a slowing debuff on non-boss targets
    // This is needed because Frostbolt won't ever target an enemy affected by the debuff
    void impact( action_state_t* state ) override
    {
      magus_spell_t::impact( state );

      magus_td_t* td = p() -> get_target_data( state -> target );

      if ( result_is_hit( state -> result )
            && ( state -> target -> is_add() || state -> target -> level() < sim -> max_player_level + 3 ) )
      {
        td -> frostbolt_debuff -> trigger();
      }
    }

    bool target_ready( player_t* candidate_target ) override
    {
      magus_td_t* td = p() -> get_target_data( candidate_target );

      // Frostbolt can't target an enemy already affected by its slowing debuff
      if ( td -> frostbolt_debuff -> check() )
        return false;

      return magus_spell_t::target_ready( candidate_target );
    }
  };

  struct shadow_bolt_magus_t : public magus_spell_t
  {
    shadow_bolt_magus_t( magus_pet_t* player, const std::string& options_str ) :
      magus_spell_t( player, "shadow_bolt", player -> o() -> find_spell( 317791 ), options_str )
    { }
  };

  magus_pet_t( death_knight_t* owner ) :
    death_knight_pet_t( owner, "magus_of_the_dead", true, false )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = 1.4_s;
    resource_regeneration = regen_type::DISABLED;
  }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 0.4;
    // Looks like Magus' AP coefficient is the same as the pet ghouls'
    // Including the +6% buff applied before magus was even a thing
    owner_coeff.ap_from_ap *= 1.06;
  }

  // Magus of the dead Action Priority List:
  // Frostbolt has a 3s cooldown that doesn't seeem to be in spelldata, and applies a 4s snare on non-boss enemies
  // Frostbolt is used on cooldown and if the target isn't slowed by the debuff, and shadow bolt is used the rest of the time
  void init_action_list() override
  {
    death_knight_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "frostbolt" ); // Cooldown and debuff are handled in the action
    def -> add_action( "shadow_bolt" );
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "frostbolt" ) return new frostbolt_magus_t( this, options_str );
    if ( name == "shadow_bolt" ) return new shadow_bolt_magus_t( this, options_str );

    return death_knight_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Reanimated Shambler (legendary)
// ==========================================================================

struct reanimated_shambler_pet_t : public death_knight_pet_t
{
  struct necroblast_t : public pet_action_t<reanimated_shambler_pet_t, melee_attack_t>
  {
    necroblast_t( reanimated_shambler_pet_t* player ) :
      pet_action_t( player, "necroblast", player -> find_spell( 334851 ) )
    { }

    void execute() override
    {
      pet_action_t::execute();

      p()->dismiss();
    }

    void impact( action_state_t* state ) override
    {
      pet_action_t::impact( state );
      p() -> o() -> trigger_festering_wound( state, 1, p() -> o() -> procs.fw_necroblast );
    }

  };

  reanimated_shambler_pet_t( death_knight_t* owner ) :
    death_knight_pet_t( owner, "reanimated_shambler", true, false )
  {
    resource_regeneration = regen_type::DISABLED;

    spawn_travel_duration = 4.597;
    spawn_travel_stddev = 0.3399;
  }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 1.0;
  }

  void init_action_list() override
  {
    death_knight_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "necroblast" );
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override
  {
    if ( name == "necroblast" )
      return new necroblast_t( this );

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

  bool triggers_shackle_the_unworthy;

  struct affected_by_t
  {
    // Masteries
    bool frozen_heart, dreadblade;
    // Runeforge
    bool razorice;
  } affected_by;

  death_knight_action_t( util::string_view n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    action_base_t( n, p, s ), gain( nullptr ),
    hasted_gcd( false ),
    triggers_shackle_the_unworthy( false ),
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
    this -> affected_by.dreadblade = this -> data().affected_by( p -> mastery.dreadblade -> effectN( 1 ) );

    this -> affected_by.razorice = this ->  data().affected_by( p -> spell.razorice_debuff -> effectN( 1 ) );
  }

  death_knight_t* p() const
  { return static_cast< death_knight_t* >( this -> player ); }

  death_knight_td_t* td( player_t* t ) const
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

    return m;
  }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = Base::composite_ta_multiplier( state );

    if ( this -> affected_by.frozen_heart || this -> affected_by.dreadblade )
    {
      m *= 1.0 + p() -> cache.mastery_value();
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = action_base_t::composite_target_multiplier( target );

    death_knight_td_t* td = p() -> get_target_data( target );

    if ( this -> affected_by.razorice )
    {
      m *= 1.0 + td -> debuff.razorice -> check_stack_value();
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

  double cost() const override
  {
    double c = action_base_t::cost();

    // Hypothermic Presence reduces the cost of runic power spenders while it's up
    // TODO: Tested with Frost Strike, does it work with BoS correctly?
    if ( this -> current_resource() == RESOURCE_RUNIC_POWER )
    {
      c *= 1.0 + p() -> buffs.hypothermic_presence -> value();
    }
     return c;
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
    action_base_t::execute();
    // If we spend a rune, we have a chance to spread the dot
    dot_t* source_dot = p() -> get_target_data( action_t::target ) -> dot.shackle_the_unworthy;
    if ( p() -> covenant.shackle_the_unworthy -> ok() && this->triggers_shackle_the_unworthy &&
         source_dot -> is_ticking() && p() -> cooldown.shackle_the_unworthy_icd -> is_ready() &&
        p() -> rng().roll( p() -> covenant.shackle_the_unworthy -> effectN( 5 ).percent() ) )
    {
      for ( auto destination : action_t::target_list() )
      {
        death_knight_td_t* destination_td = p() -> get_target_data( destination );
        if ( action_t::target == destination || destination_td -> dot.shackle_the_unworthy -> is_ticking() )
        {
          continue;
        }

        action_t::sim -> print_log("{} spreads shackle the unworthy with {} from {} to {} (remains={} )",
                *action_t::player, *this, *action_t::target, *destination, source_dot->remains() );

        source_dot->copy(destination, DOT_COPY_CLONE);
        p() -> cooldown.shackle_the_unworthy_icd -> start( p() -> covenant.shackle_the_unworthy -> internal_cooldown() );
        // after we successfully spread to one target, return.
        return;
      }
    }
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

// Forward declarations because superstrain zzz
struct blood_plague_t;
struct frost_fever_t;
struct virulent_plague_t;

// Common diseases code

struct death_knight_disease_t : public death_knight_spell_t
{
  std::vector<action_t*> superstrain_diseases;

  death_knight_disease_t( util::string_view n, death_knight_t* p, const spell_data_t* s ) :
    death_knight_spell_t( n, p, s ),
    superstrain_diseases()
  { }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );

    for ( auto disease : superstrain_diseases )
    {
      disease -> set_target( s -> target );
      disease -> execute();
    }
  }
};

// Blood Plague ============================================

struct blood_plague_heal_t : public death_knight_heal_t
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

struct blood_plague_t : public death_knight_disease_t
{
  action_t* heal;

  blood_plague_t( util::string_view name, death_knight_t* p, bool superstrain = false ) :
    death_knight_disease_t( name, p, p -> spell.blood_plague )
  {
    tick_may_crit = background = true;
    may_miss = may_crit = hasted_ticks = false;

    heal = get_action<blood_plague_heal_t>( "blood_plague_heal", p );

    base_tick_time *= 1.0 + p -> talent.rapid_decomposition -> effectN( 1 ).percent();

    // The "reduced effectiveness" mentionned in the tooltip is handled server side
    // Value calculated from testing, may change without notice
    if ( superstrain )
      base_multiplier *= .75;
    // Create superstrain-triggered spells if needed
    else if ( p -> legendary.superstrain -> ok() )
    {
      superstrain_diseases.push_back( get_action<frost_fever_t>(
        "frost_fever_superstrain", p, true ) );
      superstrain_diseases.push_back( get_action<virulent_plague_t>(
        "virulent_plague_superstrain", p, true ) );
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = death_knight_spell_t::composite_target_multiplier( t );

    m *= 1.0 + p() -> get_target_data( t ) -> debuff.debilitating_malady -> check_stack_value();

    return m;
  }

  void tick( dot_t* d ) override
  {
    death_knight_spell_t::tick( d );

    if ( d -> state -> result_amount > 0 )
    {
      // Healing is based off damage done, increased by Rapid Decomposition if talented
      heal -> base_dd_min = heal -> base_dd_max =
        d -> state -> result_amount * ( 1.0 + p() -> talent.rapid_decomposition -> effectN( 3 ).percent() );
      heal -> execute();
    }
  }
};

// Frost Fever =======================================================

struct frost_fever_t : public death_knight_disease_t
{
  int rp_generation;

  frost_fever_t( util::string_view name, death_knight_t* p, bool superstrain = false ) :
    death_knight_disease_t( name, p, p -> spell.frost_fever ),
    rp_generation( ( as<int>( p -> spec.frost_fever -> effectN( 1 ).trigger()
                     -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) ) ) )
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

    // The "reduced effectiveness" mentionned in the tooltip is handled server side
    // Value calculated from testing, may change without notice
    if ( superstrain )
      base_multiplier *= .375;
    // Create superstrain-triggered spells if needed
    else if ( p -> legendary.superstrain -> ok() )
    {
      superstrain_diseases.push_back( get_action<blood_plague_t>(
        "blood_plague_superstrain", p, true ) );
      superstrain_diseases.push_back( get_action<virulent_plague_t>(
        "virulent_plague_superstrain", p, true ) );
    }
  }

  void tick( dot_t* d ) override
  {
    death_knight_spell_t::tick( d );

    // TODO: Melekus, 2019-05-15: Frost fever proc chance and ICD have been removed from spelldata on PTR
    // Figure out what is up with the "30% proc chance, diminishing beyond the first target" from blue post.
    // https://us.forums.blizzard.com/en/wow/t/upcoming-ptr-class-changes-4-23/158332

    // 2020-05-05: It would seem that the proc chance is 0.30 * sqrt(FeverCount) / FeverCount
    unsigned ff_count = p() -> get_active_dots( internal_id );
    double chance = 0.30 * std::sqrt( ff_count ) / ff_count;

    if ( rng().roll( chance ) )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER, rp_generation, p() -> gains.frost_fever, this );
    }
  }
};

// Virulent Plague ====================================================

struct virulent_plague_t : public death_knight_disease_t
{
  virulent_plague_t( util::string_view name, death_knight_t* p, bool superstrain = false ) :
    death_knight_disease_t( name, p, p -> spell.virulent_plague )
  {
    // TODO: Does VP also apply in aoe with superstrain? Keep at one target for now
    aoe = superstrain ? 0 : -1;

    base_tick_time *= 1.0 + p -> talent.ebon_fever -> effectN( 1 ).percent();

    // Order of operation matters for dot_duration.  lingering plague gives you an extra 3 seconds, or 1 tick of the dot
    // Ebon fever does the same damage, in half the time, with tick rate doubled.  So you get the same number of ticks
    // Tested Oct 21 2020 in beta build 36294
    dot_duration += p -> conduits.lingering_plague -> effectN( 2 ).time_value();
    dot_duration *= 1.0 + p -> talent.ebon_fever -> effectN( 2 ).percent();

    base_multiplier *= 1.0 + p -> conduits.lingering_plague.percent();
    base_multiplier *= 1.0 + p -> talent.ebon_fever -> effectN( 3 ).percent();

    tick_may_crit = background = true;
    may_miss = may_crit = hasted_ticks = false;

    // The "reduced effectiveness" mentionned in the tooltip is handled server side
    // Value calculated from testing, may change without notice
    if ( superstrain )
      base_multiplier *= .375;
    // Create superstrain-triggered spells if needed
    else if ( p -> legendary.superstrain -> ok() )
    {
      superstrain_diseases.push_back( get_action<blood_plague_t>(
        "blood_plague_superstrain", p, true ) );
      superstrain_diseases.push_back( get_action<frost_fever_t>(
        "frost_fever_superstrain", p, true ) );
    }
  }
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

  if ( triggers_icecap && p() -> talent.icecap -> ok() && hit_any_target &&
       p() -> cooldown.icecap_icd -> is_ready() && execute_state -> result == RESULT_CRIT )
  {
    p() -> cooldown.pillar_of_frost -> adjust( timespan_t::from_seconds(
      - p() -> talent.icecap -> effectN( 1 ).base_value() / 10.0 ) );

    p() -> cooldown.icecap_icd -> start( p() -> talent.icecap -> internal_cooldown() );
  }
}

// death_knight_melee_attack_t::impact() ====================================

void death_knight_melee_attack_t::impact( action_state_t* state )
{
  base_t::impact( state );

  if ( state -> result_amount > 0 && callbacks && p() -> runeforge.rune_of_razorice )
  {
    // Use the action's weapon slot, or default to main hand
    auto razorice_attack = state -> action -> weapon && state -> action -> weapon -> slot == SLOT_OFF_HAND ?
      p() -> active_spells.razorice_oh :
      p() -> active_spells.razorice_mh;

    if ( razorice_attack )
    {
      // Razorice is executed after the attack that triggers it
      razorice_attack -> set_target( state -> target );
      razorice_attack -> schedule_execute();
    }
  }
}

// ==========================================================================
// Death Knight Secondary Abilities
// ==========================================================================

// Razorice Attack ==========================================================

struct razorice_attack_t : public death_knight_melee_attack_t
{
  razorice_attack_t( death_knight_t* player, const std::string& name ) :
    death_knight_melee_attack_t( name, player, player -> find_spell( 50401 ) )
  {
    school      = SCHOOL_FROST;
    may_miss    = callbacks = false;
    background  = proc = true;

    // Note, razorice always attacks with the main hand weapon, regardless of which hand triggers it
    weapon = &( player -> main_hand_weapon );
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );
    td( s -> target ) -> debuff.razorice -> trigger();
  }
};

// Frozen Pulse =============================================================

struct frozen_pulse_t : public death_knight_spell_t
{
  frozen_pulse_t( death_knight_t* player ) :
    death_knight_spell_t( "frozen_pulse", player, player -> talent.frozen_pulse -> effectN( 1 ).trigger() )
  {
    aoe = -1;
    background = true;
  }
};

struct inexorable_assault_damage_t : public death_knight_spell_t
{
  inexorable_assault_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> find_spell( 253597 ) )
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
  action_t* frozen_pulse;

  melee_t( const char* name, death_knight_t* p, int sw ) :
    death_knight_melee_attack_t( name, p ), sync_weapons( sw ), first ( true ),
    frozen_pulse( p -> talent.frozen_pulse -> ok() ? new frozen_pulse_t( p ) : nullptr )
  {
    school            = SCHOOL_PHYSICAL;
    may_glance        = true;
    background        = true;
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

  void execute() override
  {
    if ( first )
      first = false;

    death_knight_melee_attack_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    if ( p() -> talent.runic_attenuation -> ok() && p() -> rppm.runic_attenuation -> trigger()  )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER,
                            p() -> talent.runic_attenuation -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
                            p() -> gains.runic_attenuation, this );
    }

    if ( result_is_hit( s -> result ) )
    {
      p() -> buffs.sudden_doom -> trigger();

      if ( p() -> spec.killing_machine -> ok() && s -> result == RESULT_CRIT )
      {
        p() -> trigger_killing_machine( 0, p() -> procs.km_from_crit_aa,
                                           p() -> procs.km_from_crit_aa_wasted );
      }

      if ( weapon && p() -> buffs.frozen_pulse -> up() )
      {
        frozen_pulse -> set_target( s -> target );
        frozen_pulse -> schedule_execute();
      }

      // Crimson scourge doesn't proc if death and decay is ticking
      if ( td( s -> target ) -> dot.blood_plague -> is_ticking() && ! p() -> active_dnd )
      {
        if ( p() -> buffs.crimson_scourge -> trigger() )
        {
          p() -> cooldown.death_and_decay_dynamic -> reset( true );
        }
      }

      if ( p() -> talent.bloodworms -> ok() )
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
    p() -> pets.bloodworms.spawn( p() -> talent.bloodworms -> effectN( 2 ).trigger() -> effectN( 1 ).trigger() -> duration(), 1 );
    // Pet spawn spelldata: 16s
    // p() -> pets.bloodworms.spawn( p() -> talent.bloodworms -> effectN( 1 ).trigger() -> duration(), 1 );
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public death_knight_melee_attack_t
{
  int sync_weapons;

  auto_attack_t( death_knight_t* p, const std::string& options_str ) :
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

struct abomination_limb_damage_t : public death_knight_spell_t
{
  int bone_shield_stack_gain;
  abomination_limb_damage_t( death_knight_t* p ) :
    death_knight_spell_t( "abomination_limb_damage", p, p -> covenant.abomination_limb -> effectN( 2 ).trigger() )
  {
    background = true;
    base_multiplier *= 1.0 + p -> conduits.brutal_grasp.percent();
    bone_shield_stack_gain = as<int>(p -> covenant.abomination_limb -> effectN( 3 ).base_value());
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    // We proc this on cast, then every 6 seconds thereafter, on tick
    if ( p() ->cooldown.abomination_limb -> is_ready() )
    {
      switch ( p() ->specialization() )
      {
        case DEATH_KNIGHT_BLOOD:
          p() -> buffs.bone_shield -> trigger( bone_shield_stack_gain );
          break;
        case DEATH_KNIGHT_FROST:
          p() -> buffs.rime -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
          break;
        case DEATH_KNIGHT_UNHOLY:
          p() -> trigger_runic_corruption( p() -> procs.al_runic_corruption, 0, 1.0 );
          break;
        default:
          break;
      }
      p() -> cooldown.abomination_limb -> start( timespan_t::from_seconds(p() -> covenant.abomination_limb -> effectN ( 4 ).base_value() ) );
    }
  }
};

struct abomination_limb_buff_t : public buff_t
{
  abomination_limb_damage_t* damage; // (AOE) damage that ticks every second

  abomination_limb_buff_t( death_knight_t* p ) :
    buff_t( p, "abomination_limb", p -> covenant.abomination_limb ),
    damage( new abomination_limb_damage_t( p ) )
  {
    cooldown -> duration = 0_ms; // Controlled by the action
    set_tick_callback( [ this ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ )
    {
      damage -> execute();
    } );
    set_partial_tick( true );
  }
};

struct abomination_limb_t : public death_knight_spell_t
{
  abomination_limb_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "abomination_limb", p, p -> covenant.abomination_limb )
  {
    may_crit = may_miss = may_dodge = may_parry = false;

    parse_options( options_str );

    // Periodic behavior handled by the buff
    dot_duration = base_tick_time = 0_ms;

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

struct apocalypse_t : public death_knight_melee_attack_t
{
  timespan_t summon_duration;
  timespan_t magus_duration;
  int rune_generation;

  apocalypse_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "apocalypse", p, p -> spec.apocalypse ),
    summon_duration( p -> find_spell( 221180 ) -> duration() ),
    magus_duration( p -> find_spell( 317776 ) -> duration() ),
    rune_generation( as<int>( p -> find_spell( 343758 ) -> effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );

    cooldown -> duration += p -> spec.apocalypse_2 -> effectN( 1 ).time_value();
    base_multiplier *= 1.0 + p -> spec.apocalypse_2 -> effectN( 2 ).percent();
  }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );
    auto n_wounds = std::min( as<int>( data().effectN( 2 ).base_value() ),
                              td( state -> target ) -> debuff.festering_wound -> stack() );

    p() -> burst_festering_wound( state -> target, n_wounds );
    p() -> pets.apoc_ghouls.spawn( summon_duration, n_wounds );

    if ( p() -> talent.army_of_the_damned -> ok() )
    {
      p() -> pets.magus_of_the_dead.spawn( magus_duration, 1 );
    }

    if ( p() -> spec.apocalypse_3 -> ok() )
    {
      p() -> replenish_rune( rune_generation, p() -> gains.apocalypse );
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    death_knight_td_t* td = p() -> get_target_data( candidate_target );

    // In-game limitation: you can't use Apocalypse on a target that isn't affected by Festering Wounds
    if ( ! td -> debuff.festering_wound -> check() )
      return false;

    return death_knight_melee_attack_t::target_ready( candidate_target );
  }
};

// Army of the Dead =========================================================

struct army_of_the_dead_t : public death_knight_spell_t
{
  double precombat_time;
  timespan_t summon_duration;
  timespan_t magus_duration;

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

  army_of_the_dead_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "army_of_the_dead", p, p -> spec.army_of_the_dead ),
    precombat_time( 6.0 ),
    summon_duration( p -> spec.army_of_the_dead -> effectN( 1 ).trigger() -> duration() ),
    magus_duration( p -> find_spell( 317776 ) -> duration() )
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
    double const summon_interval = 0.5;

    if ( ! p() -> in_combat && precombat_time > 0 )
    {
      double duration_penalty = precombat_time - summon_interval;
      while ( duration_penalty >= 0 && n_ghoul < 8 )
      {
        auto pet = p() -> pets.army_ghouls.spawn( summon_duration - timespan_t::from_seconds( duration_penalty ), 1 ).front();
        pet -> precombat_spawn_adjust = duration_penalty;
        pet -> precombat_spawn = true;
        duration_penalty -= summon_interval;
        n_ghoul++;
      }

      p() -> cooldown.army_of_the_dead -> adjust( timespan_t::from_seconds( -precombat_time ), false );

      // Simulate RP decay for X seconds
      p() -> resource_loss( RESOURCE_RUNIC_POWER, RUNIC_POWER_DECAY_RATE * precombat_time, nullptr, nullptr );

      // Simulate rune regeneration for X seconds
      p() -> _runes.regenerate_immediate( timespan_t::from_seconds( precombat_time ) );

      sim -> print_debug( "{} used Army of the Dead in precombat with precombat time = {}, adjusting pets' duration and remaining cooldown.",
                          player -> name(), precombat_time );
    }

    // If precombat didn't summon every ghoul (due to interval between each spawn)
    // Or if the cast isn't during precombat
    // Summon the rest
    if ( n_ghoul < 8 )
      make_event<summon_army_event_t>( *sim, p(), n_ghoul, timespan_t::from_seconds( summon_interval ), summon_duration );

    if ( p() -> talent.army_of_the_damned -> ok() )
    {
      // Bug? Magus of the dead is summoned for the same duration as Army of the Dead even though Army of the Damned's tooltip states 15s.
      timespan_t actual_magus_duration = p() -> bugs ? summon_duration : magus_duration;

      p() -> pets.magus_of_the_dead.spawn( actual_magus_duration - timespan_t::from_seconds( precombat_time ), 1 );
    }
  }
};

// Blood Boil ===============================================================

struct blood_boil_t : public death_knight_spell_t
{
  blood_boil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blood_boil", p, p -> spec.blood_boil )
  {
    parse_options( options_str );

    aoe = -1;
    cooldown -> hasted = true;
    cooldown -> charges += as<int>( p -> spec.blood_boil_2 -> effectN( 1 ).base_value() );
    impact_action = get_action<blood_plague_t>( "blood_plague", p );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> up() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.blood_boil -> set_target( target );
      p() -> pets.dancing_rune_weapon_pet -> ability.blood_boil -> execute();
    }
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( p() -> conduits.debilitating_malady.ok() )
      td( state -> target ) -> debuff.debilitating_malady -> trigger();

    p() -> buffs.hemostasis -> trigger();
  }

};

// Blood Tap ================================================================

struct blood_tap_t : public death_knight_spell_t
{
  blood_tap_t( death_knight_t* p, const std::string options_str ) :
    death_knight_spell_t( "blood_tap", p, p -> talent.blood_tap )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> replenish_rune( as<int>( p() -> talent.blood_tap -> effectN( 1 ).resource( RESOURCE_RUNE ) ), p() -> gains.blood_tap );
  }
};

// Blooddrinker =============================================================

struct blooddrinker_heal_t : public death_knight_heal_t
{
  blooddrinker_heal_t( util::string_view name, death_knight_t* p ) :
    death_knight_heal_t( name, p, p -> talent.blooddrinker )
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
struct blooddrinker_t : public death_knight_spell_t
{
  action_t* heal;

  blooddrinker_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blooddrinker", p, p -> talent.blooddrinker ),
    heal( get_action<blooddrinker_heal_t>( "blooddrinker_heal", p ) )
  {
    parse_options( options_str );
    tick_may_crit = channeled = hasted_ticks = tick_zero = true;

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
};

// Bonestorm ================================================================

struct bonestorm_heal_t : public death_knight_heal_t
{
  bonestorm_heal_t( util::string_view name, death_knight_t* p ) :
    death_knight_heal_t( name, p, p -> find_spell( 196545 ) )
  {
    background = true;
    target = p;
  }
};

struct bonestorm_damage_t : public death_knight_spell_t
{
  action_t* heal;
  int heal_count;

  bonestorm_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> talent.bonestorm -> effectN( 3 ).trigger() ),
    heal( get_action<bonestorm_heal_t>( "bonestorm_heal", p ) ), heal_count( 0 )
  {
    background = true;
    aoe = as<int>( data().effectN( 2 ).base_value() );
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
      if ( heal_count < p() -> talent.bonestorm -> effectN( 4 ).base_value() )
      {
        heal -> execute();
        heal_count++;
      }
    }
  }
};

struct bonestorm_t : public death_knight_spell_t
{
  bonestorm_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "bonestorm", p, p -> talent.bonestorm )
  {
    parse_options( options_str );
    hasted_ticks = false;
    tick_action = get_action<bonestorm_damage_t>( "bonestorm_damage", p );
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  { return base_tick_time * last_resource_cost / 10; }
};

// Breath of Sindragosa =====================================================

struct breath_of_sindragosa_tick_t: public death_knight_spell_t
{
  breath_of_sindragosa_tick_t( util::string_view name, death_knight_t* p ):
    death_knight_spell_t( name, p, p -> talent.breath_of_sindragosa -> effectN( 1 ).trigger() )
  {
    aoe = -1;
    background = true;
    reduced_aoe_damage = true;

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
  action_t* bos_damage;

  breath_of_sindragosa_buff_t( death_knight_t* p ) :
    buff_t( p, "breath_of_sindragosa", p -> talent.breath_of_sindragosa ),
    tick_period( p -> talent.breath_of_sindragosa -> effectN( 1 ).period() ),
    rune_gen( as<int>( p -> find_spell( 303753 ) -> effectN( 1 ).base_value() ) )
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

      // On cast, generate two runes and execute damage for no cost
      if ( this -> current_tick == 0 )
      {
        p -> replenish_rune( rune_gen, p -> gains.breath_of_sindragosa );
        bos_damage -> set_target( bos_target );
        bos_damage -> execute();
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
      bos_damage -> set_target( bos_target );
      bos_damage -> execute();
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
};

struct breath_of_sindragosa_t : public death_knight_spell_t
{
  breath_of_sindragosa_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "breath_of_sindragosa", p, p -> talent.breath_of_sindragosa )
  {
    parse_options( options_str );
    base_tick_time = 0_ms; // Handled by the buff
    add_child( get_action<breath_of_sindragosa_tick_t>( "breath_of_sindragosa_tick", p ) );
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
struct cold_heart_damage_t : public death_knight_spell_t
{
  cold_heart_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> find_spell( 281210 ) )
  {
    background = true;
  }

  double action_multiplier() const override
  {
    double m = death_knight_spell_t::action_multiplier();

    m *= p() -> buffs.cold_heart -> stack();

    return m;
  }
};

struct chains_of_ice_t : public death_knight_spell_t
{
  action_t* cold_heart;

  chains_of_ice_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "chains_of_ice", p, p -> spec.chains_of_ice )
  {
    parse_options( options_str );
    cold_heart = get_action<cold_heart_damage_t>( "cold_heart", p );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.cold_heart -> check() > 0 )
    {
      cold_heart -> set_target( target );
      cold_heart -> execute();
      p() -> buffs.cold_heart -> expire();
    }
  }
};

// Consumption ==============================================================

struct consumption_t : public death_knight_melee_attack_t
{
  consumption_t( death_knight_t* p, const std::string& options_str )
    : death_knight_melee_attack_t( "consumption", p, p -> talent.consumption )
  {
    // TODO: Healing from damage done

    parse_options( options_str );
    aoe = as<int>( data().effectN( 3 ).base_value() );
  }
};

// Dancing Rune Weapon ======================================================

struct dancing_rune_weapon_buff_t : public buff_t
{
  dancing_rune_weapon_buff_t( death_knight_t* p ) :
    buff_t( p, "dancing_rune_weapon", p -> find_spell( 81256 ) )
  {
    cooldown -> duration = 0_ms; // Handled by the ability
    base_buff_duration = 0_ms; // Uptime handled by the pet
    set_default_value_from_effect_type( A_MOD_PARRY_PERCENT );
    add_invalidate( CACHE_PARRY );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    death_knight_t* p = debug_cast< death_knight_t* >( player );

    if ( p -> legendary.crimson_rune_weapon -> ok() )
      p -> buffs.crimson_rune_weapon -> trigger();
  }
};

struct dancing_rune_weapon_t : public death_knight_spell_t
{
  dancing_rune_weapon_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "dancing_rune_weapon", p, p -> spec.dancing_rune_weapon )
  {
    may_miss = may_crit = may_dodge = may_parry = harmful = false;

    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> pets.dancing_rune_weapon_pet -> summon( timespan_t::from_seconds( p() -> spec.dancing_rune_weapon -> effectN( 4 ).base_value() ) +
                                                                             p() -> conduits.meat_shield -> effectN( 2 ).time_value() );
  }
};

// Dark Command =============================================================

struct dark_command_t: public death_knight_spell_t
{
  dark_command_t( death_knight_t* p, const std::string& options_str ):
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

struct unholy_pact_damage_t : public death_knight_spell_t
{
  unholy_pact_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> find_spell( 319236 ) )
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
struct unholy_pact_buff_t : public buff_t
{
  action_t* damage;

  unholy_pact_buff_t( death_knight_t* p ) :
    buff_t( p, "unholy_pact",
      p -> talent.unholy_pact -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() ),
    damage( get_action<unholy_pact_damage_t>( "unholy_pact", p ) )
  {
    set_default_value( data().effectN( 4 ).trigger() -> effectN( 1 ).percent() );
    add_invalidate( CACHE_STRENGTH );
    tick_zero = true;
    buff_period = 1.0_s;
    set_tick_behavior( buff_tick_behavior::CLIP );
    set_tick_callback( [ this ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ )
    {
      damage -> execute();
    } );
  }

  // Unholy pact ticks twice on buff expiration, hence the following override
  // https://github.com/SimCMinMax/WoW-BugTracker/issues/675
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    damage -> schedule_execute();
    damage -> schedule_execute();
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

struct dark_transformation_damage_t : public death_knight_spell_t
{
  dark_transformation_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> find_spell( 344955 ) )
  {
    background = dual = true;
    aoe = as<int>( data().effectN( 2 ).base_value() );
  }
};

// Even though the buff is tied to the pet ingame, it's simpler to add it to the player
struct dark_transformation_buff_t : public buff_t
{
  dark_transformation_buff_t( death_knight_t* p ) :
    buff_t( p, "dark_transformation", p -> spec.dark_transformation )
  {
    base_buff_duration += p -> conduits.eternal_hunger -> effectN( 2 ).time_value();
    set_default_value_from_effect( 1 );
    cooldown -> duration = 0_ms; // Handled by the player ability
  }

  // Unlike the player buff, Frenzied Monstrosity follows Dark Trasnformation uptime on the pet
  // even with effects that increase the buff's duration
  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    death_knight_t* p = debug_cast<death_knight_t*>( player );
    if ( p -> legendary.frenzied_monstrosity -> ok() )
      debug_cast<pets::ghoul_pet_t*>( p -> pets.ghoul_pet ) -> frenzied_monstrosity -> trigger();

    return buff_t::trigger( s, v, c, d );
  }

  void expire_override( int, timespan_t ) override
  {
    debug_cast<pets::ghoul_pet_t*>( debug_cast<death_knight_t*>( player )
        -> pets.ghoul_pet ) -> frenzied_monstrosity -> expire();
  }
};

struct dark_transformation_t : public death_knight_spell_t
{
  bool precombat_frenzy;

  dark_transformation_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "dark_transformation", p, p -> spec.dark_transformation ),
    precombat_frenzy( false )
  {
    add_option( opt_bool( "precombat_frenzy", precombat_frenzy ) );
    harmful = false;

    // Don't create and use the damage if the spell is used for precombat frenzy
    if ( ! precombat_frenzy )
    {
      execute_action = get_action<dark_transformation_damage_t>( "dark_transformation_damage", p );
      execute_action -> stats = stats;
    }

    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    // If used during precombat and the precombat frenzy option is selected, trigger all the related buffs with a 2s penalty
    // As well as Frenzied Monstrosity regardless of legendary to model using DT with legendary equipped, then switching to another one before combat starts
    // NOTE: THIS IS A HACK
    if ( precombat_frenzy && is_precombat )
    {
      p() -> buffs.dark_transformation -> trigger(
        p() -> buffs.dark_transformation -> buff_duration() - 2_s );

      if ( p() -> talent.unholy_pact -> ok() )
      {
        p() -> buffs.unholy_pact -> trigger( p() -> buffs.unholy_pact -> buff_duration() - 2_s );
      }

      p() -> buffs.frenzied_monstrosity -> trigger( p() -> buffs.frenzied_monstrosity -> buff_duration() - 2_s );
      p() -> pets.ghoul_pet -> frenzied_monstrosity -> trigger();

      p() -> cooldown.dark_transformation -> adjust( -2_s, false );
    }

    else
    {
      p() -> buffs.dark_transformation -> trigger();

      if ( p() -> spec.dark_transformation_2 -> ok() )
      {
        p() -> pets.ghoul_pet -> resource_gain( RESOURCE_ENERGY, p() -> spec.dark_transformation_2 -> effectN( 1 ).base_value(),
                                                p() -> pets.ghoul_pet -> dark_transformation_gain, this );
      }

      if ( p() -> talent.unholy_pact -> ok() )
      {
        p() -> buffs.unholy_pact -> trigger();
      }

      // Unlike the pet's buff, the player's frenzied monstrosity buff has a flat duration that doesn't necessarily follow DT's
      if ( p() -> legendary.frenzied_monstrosity.ok() )
      {
        p() -> buffs.frenzied_monstrosity -> trigger();
      }
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
  }
};

struct death_and_decay_damage_t : public death_and_decay_damage_base_t
{
  // Values found from testing
  const int PESTILENCE_CAP_PER_TICK = 2;
  const int PESTILENCE_CAP_PER_CAST = 10;

  int pestilence_procs_per_tick;
  int pestilence_procs_per_cast;

  death_and_decay_damage_t( util::string_view name, death_knight_t* p, const spell_data_t* s = nullptr ) :
    death_and_decay_damage_base_t( name, p, s == nullptr ? p -> find_spell( 52212 ) : s ),
    pestilence_procs_per_tick( 0 ),
    pestilence_procs_per_cast( 0 )
  { }

  void execute() override
  {
    pestilence_procs_per_tick = 0;

    death_and_decay_damage_base_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    death_and_decay_damage_base_t::impact( s );

    if ( p() -> talent.pestilence -> ok() &&
         pestilence_procs_per_tick < PESTILENCE_CAP_PER_TICK &&
         pestilence_procs_per_cast < PESTILENCE_CAP_PER_CAST )
    {
      if ( rng().roll( p() -> talent.pestilence -> effectN( 1 ).percent() ) )
      {
        p() -> trigger_festering_wound( s, 1, p() -> procs.fw_pestilence );
        pestilence_procs_per_tick++;
        pestilence_procs_per_cast++;
      }
    }
  }
};

struct defile_damage_t : public death_and_decay_damage_base_t
{
  double active_defile_multiplier;
  const double defile_tick_multiplier;

  defile_damage_t( util::string_view name, death_knight_t* p ) :
    death_and_decay_damage_base_t( name, p, p -> find_spell( 156000 ) ),
    active_defile_multiplier( 1.0 ),
    // Testing shows a 1.06 multiplicative damage increase for every tick of defile that hits an enemy
    // Can't seem to find it anywhere in defile's spelldata
    defile_tick_multiplier( 1.06 )
  {
    base_multiplier *= 1.0 + p -> conduits.withering_ground.percent();
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
};

struct deaths_due_damage_t : public death_and_decay_damage_t
// Death's due is typed after dnd_damage rather than dnd_damage_base
// Because it shares the pestilence interaction and is replaced by defile too
{
  deaths_due_damage_t( util::string_view name, death_knight_t* p ) :
    death_and_decay_damage_t( name, p, p -> find_spell( 341340 ) )
  {
    base_multiplier *= 1.0 + p -> conduits.withering_ground.percent();
  }
};

// Relish in Blood healing and RP generation
struct relish_in_blood_t : public death_knight_heal_t
{
  relish_in_blood_t( util::string_view name, death_knight_t* p ) :
    death_knight_heal_t( name, p, p -> find_spell( 317614 ) )
  {
    background = true;
    target = p;
  }

  double action_multiplier() const override
  {
    double m = death_knight_heal_t::action_multiplier();

    m *= p() -> buffs.bone_shield -> stack();

    return m;
  }
};

// Main Death and Decay spells

struct death_and_decay_base_t : public death_knight_spell_t
{
  action_t* damage;
  action_t* relish_in_blood;

  death_and_decay_base_t( death_knight_t* p, const std::string& name, const spell_data_t* spell ) :
    death_knight_spell_t( name, p, spell ),
    damage( nullptr )
  {
    base_tick_time = dot_duration = 0_ms; // Handled by event
    ignore_false_positive = true; // TODO: Is this necessary?
    may_crit              = false;
    // Note, radius and ground_aoe flag needs to be set in base so spell_targets expression works
    ground_aoe            = true;
    radius                = data().effectN( 1 ).radius_max();

    // Set the player-stored death and decay cooldown to this action's cooldown
    p -> cooldown.death_and_decay_dynamic = cooldown;

    if ( p -> talent.relish_in_blood -> ok() )
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
    if ( p() -> buffs.crimson_scourge -> check() )
    {
      return 0;
    }

    return death_knight_spell_t::cost();
  }

  double runic_power_generation_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::runic_power_generation_multiplier( state );

    if ( p() -> buffs.crimson_scourge -> check() )
    {
      m *= 1.0 + p() -> buffs.crimson_scourge -> data().effectN( 2 ).percent();
    }

    return m;
  }

  void execute() override
  {
    // If a death and decay/defile is already active, cancel it
    if ( p() -> active_dnd )
    {
      event_t::cancel( p() -> active_dnd );
      p() -> active_dnd = nullptr;
    }

    death_knight_spell_t::execute();

    // If bone shield isn't up, Relish in Blood doesn't heal or generate any RP
    if ( p() -> buffs.crimson_scourge -> up() && p() -> talent.relish_in_blood -> ok() && p() -> buffs.bone_shield -> up() )
    {
      // The heal's energize data automatically handles RP generation
      relish_in_blood -> execute();
    }

    p() -> buffs.crimson_scourge -> decrement();

    if ( p() -> legendary.phearomones -> ok() )
    {
      p() -> buffs.death_turf -> trigger();
      p() -> buffs.death_turf -> set_duration(data().duration() + 500_ms);
    }

    make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
      .target( target )
      // Dnd is supposed to last 10s, but a total of 11 ticks (13 with rapid decomposition) are observed so we're adding a duration of 0.5s to make it work properly
      .duration( data().duration() + 500_ms )
      .pulse_time( compute_tick_time() )
      .action( damage )
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
    base *= 1.0 + p() -> talent.rapid_decomposition -> effectN( 1 ).percent();

    return base;
  }
};

struct death_and_decay_t : public death_and_decay_base_t
{
  death_and_decay_t( death_knight_t* p, const std::string& options_str ) :
    death_and_decay_base_t( p, "death_and_decay", p -> spec.death_and_decay )
  {
    damage = get_action<death_and_decay_damage_t>( "death_and_decay_damage", p );

    parse_options( options_str );

    // Disable when Defile or Death's Due are taken
    if ( p -> talent.defile -> ok() || p -> covenant.deaths_due -> ok() )
      background = true;
  }

  void execute() override
  {
    // Reset death and decay damage's pestilence proc per cast counter
    debug_cast<death_and_decay_damage_t*>( damage ) -> pestilence_procs_per_cast = 0;

    death_and_decay_base_t::execute();
  }
};

struct defile_t : public death_and_decay_base_t
{
  defile_t( death_knight_t* p, const std::string& options_str ) :
    death_and_decay_base_t( p, "defile", p -> talent.defile )
  {
    damage = get_action<defile_damage_t>( "defile_damage", p );

    parse_options( options_str );
  }

  void execute() override
  {
    // Reset the damage component's increasing multiplier
    static_cast<defile_damage_t*>( damage ) -> active_defile_multiplier = 1.0;

    death_and_decay_base_t::execute();
  }
};

struct deaths_due_t : public death_and_decay_base_t
{
  deaths_due_t( death_knight_t* p, const std::string& options_str ) :
    death_and_decay_base_t( p, "deaths_due", p -> covenant.deaths_due )
  {
    damage = get_action<deaths_due_damage_t>( "deaths_due_damage", p );

    parse_options( options_str );

    // Disable when Defile is taken
    if ( p -> talent.defile -> ok() )
      background = true;
  }

  void execute() override
  {
    debug_cast<deaths_due_damage_t*>( damage ) -> pestilence_procs_per_cast = 0;

    death_and_decay_base_t::execute();
  }
};

// Death's Caress ===========================================================

struct deaths_caress_t : public death_knight_spell_t
{
  deaths_caress_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "deaths_caress", p, p -> spec.deaths_caress )
  {
    parse_options( options_str );
    triggers_shackle_the_unworthy = true;
    impact_action = get_action<blood_plague_t>( "blood_plague", p );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> up() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.deaths_caress -> set_target( target );
      p() -> pets.dancing_rune_weapon_pet -> ability.deaths_caress -> execute();
    }
  }
};

// Death Coil ===============================================================

struct death_coil_damage_t : public death_knight_spell_t
{
  death_coil_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> find_spell( 47632 ) )
  {
    background = dual = true;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_da_multiplier( state );
    if ( p() -> buffs.sudden_doom -> check() )
    {
      m *= 1.0 + p() -> conduits.embrace_death.percent();
    }

    m *= 1.0 + p() -> legendary.deaths_certainty->effectN( 2 ).percent();

    return m;
  }
};

struct death_coil_t : public death_knight_spell_t
{
  death_coil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "death_coil", p, p -> spec.death_coil )
  {
    parse_options( options_str );

    execute_action = get_action<death_coil_damage_t>( "death_coil_damage", p );
    execute_action -> stats = stats;
  }

  double cost() const override
  {
    if ( p() -> buffs.sudden_doom -> check() )
      return 0;

    double cost = death_knight_spell_t::cost();

    if ( p() -> legendary.deadliest_coil.ok() )
    {
      cost += (p() -> legendary.deadliest_coil->effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) );
    }

    return cost;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    // Rank 2 Reduces the cooldown Dark Transformation by 1s
    if ( p() -> spec.death_coil_2 -> ok() )
      p() -> cooldown.dark_transformation -> adjust( -1.0 * p() -> spec.death_coil -> effectN( 2 ).time_value() );

    // Reduce the cooldown on Apocalypse and Army of the Dead if Army of the Damned is talented
    p() -> cooldown.apocalypse -> adjust( -timespan_t::from_seconds(
      p() -> talent.army_of_the_damned -> effectN( 1 ).base_value() / 10 ) );

    p() -> cooldown.army_of_the_dead -> adjust( -timespan_t::from_seconds(
      p() -> talent.army_of_the_damned -> effectN( 2 ).base_value() / 10 ) );

    p() -> cooldown.death_and_decay_dynamic -> adjust( -timespan_t::from_seconds(
      p() -> legendary.deaths_certainty -> effectN( 1 ).base_value() / 10 ) );

    if ( p() -> buffs.dark_transformation -> up() && p() -> legendary.deadliest_coil.ok() )
    {
      p() -> buffs.dark_transformation -> extend_duration( p(),
        timespan_t::from_seconds( p() -> legendary.deadliest_coil -> effectN( 2 ).base_value() ) );
    }

    p() -> buffs.sudden_doom -> decrement();
  }
};

// Death Strike =============================================================

struct blood_shield_buff_t : public absorb_buff_t
{
  blood_shield_buff_t( death_knight_t* player ) :
    absorb_buff_t( player, "blood_shield", player -> spell.blood_shield )
  {
    set_absorb_school( SCHOOL_PHYSICAL );
    set_absorb_source( player -> get_stats( "blood_shield" ) );
  }
};

struct blood_shield_t : public absorb_t
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

struct death_strike_heal_t : public death_knight_heal_t
{
  blood_shield_t* blood_shield;
  timespan_t interval;
  double minimum_healing;
  double last_death_strike_cost;

  death_strike_heal_t( util::string_view name, death_knight_t* p ) :
    death_knight_heal_t( name, p, p -> find_spell( 45470 ) ),
    blood_shield( p -> specialization() == DEATH_KNIGHT_BLOOD ? new blood_shield_t( p ) : nullptr ),
    interval( timespan_t::from_seconds( p -> spec.death_strike -> effectN( 4 ).base_value() ) ),
    minimum_healing( p -> spec.death_strike -> effectN( 3 ).percent() ),
    last_death_strike_cost( 0 )
  {
    may_crit = callbacks = false;
    background = true;
    target     = p;

    // Melekus 2020-09-07: DS base healing with Voracious is a lot higher than expected
    // There's a hidden effect in Voracious' spelldata that increases death strike's effect#3 (base healing) by 50%
    // Rounded down as a percent of player's max hp (7 * 1.5 = 10.5 -> 10), this matches the healing observed ingame.
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/627
    if ( p -> bugs && p -> talent.voracious -> ok() )
    {
      minimum_healing = std::floor( (minimum_healing * (1.0 + p -> talent.voracious -> effectN( 3 ).percent())) * 100 ) / 100;
    }

    // TODO: Implement Death Strike rank 2 healing increase for dps specs
  }

  void init() override
  {
    death_knight_heal_t::init();

    snapshot_flags |= STATE_MUL_DA;
  }

  double base_da_min( const action_state_t* ) const override
  {
    return std::max( player -> resources.max[ RESOURCE_HEALTH ] * minimum_healing,
                              player -> compute_incoming_damage( interval ) * p() -> spec.death_strike -> effectN( 2 ).percent() );
  }

  double base_da_max( const action_state_t* ) const override
  {
    return std::max( player -> resources.max[ RESOURCE_HEALTH ] * minimum_healing,
                              player -> compute_incoming_damage( interval ) * p() -> spec.death_strike -> effectN( 2 ).percent() );
  }

  double action_multiplier() const override
  {
    double m = death_knight_heal_t::action_multiplier();

    m *= 1.0 + p() -> buffs.hemostasis -> stack_value();

    // see Voracious bug in constructor
    if ( ! p() -> bugs )
      m *= 1.0 + p() -> talent.voracious -> effectN( 2 ).percent();

    return m;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_heal_t::impact( state );

    if ( state -> result_total > player -> resources.max[ RESOURCE_HEALTH ] * p() -> legendary.bryndaors_might -> effectN( 2 ).percent() )
    {
      // deathstrike cost gets set from the parent as a call before heal -> execute()
      p() -> resource_gain( RESOURCE_RUNIC_POWER, p() -> legendary.bryndaors_might -> effectN( 1 ).percent() * last_death_strike_cost,
      p() -> gains.bryndaors_might, this );
    }

    trigger_blood_shield( state );
  }

  void trigger_blood_shield( action_state_t* state )
  {
    if ( p() -> specialization() != DEATH_KNIGHT_BLOOD )
      return;

    double current_value = 0;
    if ( blood_shield -> target_specific[ state -> target ] )
      current_value = blood_shield -> target_specific[ state -> target ] -> current_value;

    double amount = current_value;
    if ( p() -> mastery.blood_shield -> ok() )
      amount += state -> result_total * p() -> cache.mastery_value();

    sim -> print_debug( "{} Blood Shield buff trigger, old_value={} added_value={} new_value={}",
                     player -> name(), current_value,
                     state -> result_amount * p() -> cache.mastery_value(),
                     amount );

    blood_shield -> base_dd_min = blood_shield -> base_dd_max = amount;
    blood_shield -> execute();
  }
};

struct death_strike_offhand_t : public death_knight_melee_attack_t
{
  death_strike_offhand_t( util::string_view name, death_knight_t* p ) :
    death_knight_melee_attack_t( name, p, p -> find_spell( 66188 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
  }
};

struct death_strike_t : public death_knight_melee_attack_t
{
  action_t* oh_attack;
  action_t* heal;

  death_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "death_strike", p, p -> spec.death_strike ),
    oh_attack( nullptr ),
    heal( get_action<death_strike_heal_t>( "death_strike_heal", p ) )
  {
    parse_options( options_str );
    may_parry = false;

    weapon = &( p -> main_hand_weapon );

    if ( p -> dual_wield() )
    {
      oh_attack = get_action<death_strike_offhand_t>( "death_strike_offhand", p );
      add_child( oh_attack );
    }

    // 2019-03-01: Since 8.1.5, Death Strike has a rank 2 reducing RP costs for dps specs
    // RE/RC/Gargoyle behave as if Death Strike costed 35RP, unlike other cost reduction mechanics
    // We model that by directly changing the base RP cost from spelldata, rather than manipulating cost()
    base_costs[ RESOURCE_RUNIC_POWER ] += p -> spec.death_strike_2 -> effectN( 3 ).resource( RESOURCE_RUNIC_POWER );
  }

  double action_multiplier() const override
  {
    double m = death_knight_melee_attack_t::action_multiplier();

    m *= 1.0 + p() -> buffs.hemostasis -> stack_value();

    m *= 1.0 + p() -> legendary.deaths_certainty->effectN( 2 ).percent();

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    // 2020-08-23: Seems to only affect main hand death strike, not OH hit, not DRW's spell
    // No spelldata either, just "increase damage based on target's missing health"
    // Testing shows a linear 1% damage increase for every 1% missing health
    if ( p() -> runeforge.rune_of_sanguination )
    {
      m *= 1.0 + 1.0 - target -> resources.pct( RESOURCE_HEALTH );
    }

    return m;
  }

  double cost() const override
  {
    double c = death_knight_melee_attack_t::cost();

    c += p() -> buffs.ossuary -> value();

    return c;
  }

  void execute() override
  {
    p() -> buffs.voracious -> trigger();

    // Store the cost for Bryndaor legendary effect before resources are consumed
    debug_cast<death_strike_heal_t*>( heal ) -> last_death_strike_cost = cost();

    death_knight_melee_attack_t::execute();

    if ( oh_attack )
      oh_attack -> execute();

    if ( p() -> buffs.dancing_rune_weapon -> up() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.death_strike -> set_target( target );
      p() -> pets.dancing_rune_weapon_pet -> ability.death_strike -> execute();
    }

    if ( hit_any_target )
    {
      heal -> execute();
    }

    p() -> cooldown.death_and_decay_dynamic -> adjust( -timespan_t::from_seconds(
      p() -> legendary.deaths_certainty -> effectN( 1 ).base_value() / 10 ) );

    p() -> buffs.hemostasis -> expire();
  }
};

// Empower Rune Weapon ======================================================

struct empower_rune_weapon_buff_t : public buff_t
{
  empower_rune_weapon_buff_t( death_knight_t* p ) :

    buff_t( p, "empower_rune_weapon", p -> spec.empower_rune_weapon )
  {
    tick_zero = true;
    cooldown -> duration = 0_ms; // Handled in the action
    set_period( p -> spec.empower_rune_weapon -> effectN( 1 ).period() );
    set_default_value( p -> spec.empower_rune_weapon -> effectN( 3 ).percent() + p -> conduits.accelerated_cold.percent());
    add_invalidate( CACHE_HASTE );
    set_refresh_behavior( buff_refresh_behavior::EXTEND);
    set_tick_behavior( buff_tick_behavior::REFRESH );

    set_tick_callback( [ p ]( buff_t* b, int, timespan_t )
    {
      p -> replenish_rune( as<unsigned int>( b -> data().effectN( 1 ).base_value() ),
                           p -> gains.empower_rune_weapon );
      p -> resource_gain( RESOURCE_RUNIC_POWER,
                          b -> data().effectN( 2 ).resource( RESOURCE_RUNIC_POWER ),
                          p -> gains.empower_rune_weapon );
    } );
  }
};

struct empower_rune_weapon_t : public death_knight_spell_t
{
  empower_rune_weapon_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "empower_rune_weapon", p, p -> spec.empower_rune_weapon )
  {
    parse_options( options_str );

    harmful = false;

    // Buff handles the ticking, this one just triggers the buff
    dot_duration = base_tick_time = 0_ms;

    cooldown -> duration += p -> spec.empower_rune_weapon_2->effectN( 1 ).time_value();
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double m = death_knight_spell_t::recharge_multiplier( cd );

    if ( p() -> conduits.accelerated_cold->ok() )
    {
      m *= 1.0 + p()->conduits.accelerated_cold->effectN( 2 ).percent();
    }

    return m;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.empower_rune_weapon -> trigger();
  }
};

// Epidemic =================================================================

struct epidemic_damage_main_t : public death_knight_spell_t
{
  epidemic_damage_main_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> find_spell( 212739 ) )
  {
    background = true;
    // Ignore spelldata for max targets for the main spell, as it is single target only
    aoe = 0;
    // this spell has both coefficients in it, and it seems like it is reading #2, the aoe portion, instead of #1
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
  }
};

struct epidemic_damage_aoe_t : public death_knight_spell_t
{
  epidemic_damage_aoe_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> find_spell( 215969 ) )
  {
    background = true;
    // Main is one target, aoe is the other targets, so we take 1 off the max targets
    aoe = aoe - 1;
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
};

struct epidemic_t : public death_knight_spell_t
{
  epidemic_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "epidemic", p, p -> spec.epidemic )
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
    tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ] ( player_t* t ) { return ! this -> td( t ) -> dot.virulent_plague -> is_ticking(); } ), tl.end() );

    return tl.size();
  }

  void execute() override
  {
    // Reset target cache because of smart targetting
    target_cache.is_valid = false;

    death_knight_spell_t::execute();

    p() -> cooldown.apocalypse -> adjust( -timespan_t::from_seconds(
      p() -> talent.army_of_the_damned -> effectN( 1 ).base_value() / 10 ) );

    p() -> cooldown.army_of_the_dead -> adjust( -timespan_t::from_seconds(
      p() -> talent.army_of_the_damned -> effectN( 2 ).base_value() / 10 ) );

    p() -> buffs.sudden_doom -> decrement();
  }
};

// Festering Strike and Wounds ===============================================

struct bursting_sores_t : public death_knight_spell_t
{
  bursting_sores_t( death_knight_t* p ) :
    death_knight_spell_t( "bursting_sores", p, p -> spell.bursting_sores )
  {
    background = true;
    // Value is 9, -1 is hardcoded in tooltip. Probably because it counts the initial target of the wound burst
    aoe = as<int> ( data().effectN( 3 ).base_value() - 1 );
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

struct festering_wound_t : public death_knight_spell_t
{
  festering_wound_t( death_knight_t* p ) :
    death_knight_spell_t( "festering_wound", p, p -> spell.festering_wound_damage )
  {
    background = true;

    base_multiplier *= 1.0 + p -> talent.bursting_sores -> effectN( 1 ).percent();

    // "Festering Strike - Upgrade - Rank 2 - Increases damage done by 20%" does not actually increase FS damage by 20%
    // Instead it increases Festering Wound damage by 20% according to spelldata and ingame testing.
    // 2020-12-25 - Melekus: gonna consider this a bug for now.
    if ( p -> bugs )
      base_multiplier *= 1.0 + p -> spec.festering_strike_2 -> effectN( 1 ).percent();
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> resource_gain( RESOURCE_RUNIC_POWER,
                          p() -> spec.festering_wound -> effectN( 1 ).trigger() -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER ),
                          p() -> gains.festering_wound, this );
  }
};

struct festering_strike_t : public death_knight_melee_attack_t
{
  festering_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "festering_strike", p, p -> spec.festering_strike )
  {
    parse_options( options_str );
    triggers_shackle_the_unworthy = true;
    // "Festering Strike - Upgrade - Rank 2 - Increases damage done by 20%" does not actually increase FS damage by 20%
    // Instead it increases Festering Wound damage by 20% according to spelldata and ingame testing.
    // 2020-12-25 - Melekus: gonna consider this a bug for now.
    if ( ! p -> bugs )
      base_multiplier *= 1.0 + p -> spec.festering_strike_2 -> effectN( 1 ).percent();
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
  }
};

// Frostscythe ==============================================================

struct frostscythe_t : public death_knight_melee_attack_t
{
  action_t* inexorable_assault;

  frostscythe_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "frostscythe", p, p -> talent.frostscythe )
  {
    parse_options( options_str );

    inexorable_assault = get_action<inexorable_assault_damage_t>( "inexorable_assault", p );

    weapon = &( player -> main_hand_weapon );
    aoe = as<int>( data().effectN( 5 ).base_value() );
    triggers_shackle_the_unworthy = triggers_icecap = true;
    // Crit multipier handled in death_knight_t::apply_affecting_aura()
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    p() -> consume_killing_machine( p() -> procs.killing_machine_fsc );

    if ( p() -> buffs.inexorable_assault -> up() )
    {
      inexorable_assault -> set_target( target );
      inexorable_assault -> schedule_execute();
      p() -> buffs.inexorable_assault -> decrement();
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
};

// Frostwyrm's Fury =========================================================

struct frostwyrms_fury_damage_t : public death_knight_spell_t
{
  frostwyrms_fury_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p,  p -> find_spell( 279303 ) )
  {
    aoe = -1;
    background = true;
  }
};

struct frostwyrms_fury_t : public death_knight_spell_t
{
  frostwyrms_fury_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "frostwyrms_fury_driver", p, p -> spec.frostwyrms_fury )
  {
    parse_options( options_str );
    execute_action = get_action<frostwyrms_fury_damage_t>( "frostwyrms_fury", p );

    if ( p -> legendary.absolute_zero -> ok() )
    {
      cooldown -> duration *= 1.0 + p -> legendary.absolute_zero->effectN( 1 ).percent();
    }
    // Stun is NYI
  }
};

// Frost Strike =============================================================

struct frost_strike_strike_t : public death_knight_melee_attack_t
{
  frost_strike_strike_t( death_knight_t* p, const std::string& n, weapon_t* w, const spell_data_t* s ) :
    death_knight_melee_attack_t( n, p, s )
  {
    background = special = true;
    weapon = w;
    base_multiplier *= 1.0 + p -> spec.frost_strike_2 -> effectN( 1 ).percent();
    triggers_icecap = true;
  }

  double action_multiplier() const override
  {
    double m = death_knight_melee_attack_t::action_multiplier();

    if ( p() -> buffs.eradicating_blow -> check() )
    {
      m *= 1.0 + ( p() -> buffs.eradicating_blow -> stack_value() );
    }

    return m;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( p() -> conduits.unleashed_frenzy->ok() )
    {
      p() -> buffs.unleashed_frenzy->trigger();
    }

  }
};

struct frost_strike_t : public death_knight_melee_attack_t
{
  frost_strike_strike_t *mh, *oh;

  frost_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "frost_strike", p, p -> spec.frost_strike ),
    mh( nullptr ), oh( nullptr )
  {
    parse_options( options_str );
    may_crit = false;
    const spell_data_t* mh_data = p -> main_hand_weapon.group() == WEAPON_2H ?
      data().effectN( 4 ).trigger() : data().effectN( 2 ).trigger();

    dual = true;
    mh = new frost_strike_strike_t( p, "frost_strike", &( p -> main_hand_weapon ), mh_data );
    add_child( mh );
    execute_action = mh;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      oh = new frost_strike_strike_t( p, "frost_strike_offhand", &( p -> off_hand_weapon ), data().effectN( 3 ).trigger() );
      add_child( oh );
    }
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( hit_any_target )
    {
      if ( oh )
      {
        oh -> set_target( target );
        oh -> execute();
      }
    }

    if ( p() -> buffs.eradicating_blow -> up() )
    {
      p() -> buffs.eradicating_blow -> expire();
    }

    if ( p() -> buffs.pillar_of_frost -> up() && p() -> talent.obliteration -> ok() )
    {
      p() -> trigger_killing_machine( 1.0, p() -> procs.km_from_obliteration_fs,
                                           p() -> procs.km_from_obliteration_fs_wasted );

      // Obliteration's rune generation
      if ( rng().roll( p() -> talent.obliteration -> effectN( 2 ).percent() ) )
      {
        // WTB spelldata for the rune gain
        p() -> replenish_rune( 1, p() -> gains.obliteration );
      }
    }
  }
};

// Glacial Advance ==========================================================

struct glacial_advance_damage_t : public death_knight_spell_t
{
  glacial_advance_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> find_spell( 195975 ) )
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

    // Only applies the razorice debuff without the damage, regardless of runeforge equipped (bug?)
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/663
    if ( p() -> bugs || p() -> runeforge.rune_of_razorice )
    {
      td( state -> target ) -> debuff.razorice -> trigger();
    }
  }
};

struct glacial_advance_t : public death_knight_spell_t
{
  glacial_advance_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "glacial_advance", p, p -> talent.glacial_advance )
  {
    parse_options( options_str );

    weapon = &( p -> main_hand_weapon );

    execute_action = get_action<glacial_advance_damage_t>( "glacial_advance_damage", p );
    add_child( execute_action );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.pillar_of_frost -> up() && p() -> talent.obliteration -> ok() )
    {
      p() -> trigger_killing_machine( 1.0, p() -> procs.km_from_obliteration_ga,
                                           p() -> procs.km_from_obliteration_ga_wasted );

      // Obliteration's rune generation
      if ( rng().roll( p() -> talent.obliteration -> effectN( 2 ).percent() ) )
      {
        // WTB spelldata for the rune gain
        p() -> replenish_rune( 1, p() -> gains.obliteration );
      }
    }
  }
};

// Heart Strike =============================================================

struct heart_strike_t : public death_knight_melee_attack_t
{
  double heartbreaker_rp_gen;

  heart_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "heart_strike", p, p -> spec.heart_strike ),
    heartbreaker_rp_gen( p -> find_spell( 210738 ) -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) )
  {
    parse_options( options_str );
    triggers_shackle_the_unworthy = true;
    aoe = 2;
    weapon = &( p -> main_hand_weapon );
    energize_amount += p -> spec.heart_strike_2 -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER );
    base_multiplier *= 1.0 + p -> spec.heart_strike_3 -> effectN( 1 ).percent();
  }

  int n_targets() const override
  { return p() -> in_death_and_decay() ? aoe + as<int>( p() -> spec.death_and_decay_2 -> effectN( 1 ).base_value() ) : aoe; }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( t );

    if ( p() -> conduits.withering_plague -> ok() && p() -> get_target_data( t ) -> dot.blood_plague -> is_ticking() )
    {
      m *= 1.0 + p() -> conduits.withering_plague.percent();
    }

    return m;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> up() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.heart_strike -> set_target( target );
      p() -> pets.dancing_rune_weapon_pet -> ability.heart_strike -> execute();
    }

    if ( p() -> legendary.gorefiends_domination.ok() )
    {
      p() -> cooldown.vampiric_blood -> adjust( -timespan_t::from_seconds( p() -> legendary.gorefiends_domination -> effectN( 1 ).base_value() ) );
    }
  }

  void impact ( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    if ( p() -> talent.heartbreaker -> ok() && result_is_hit( state -> result ) )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER, heartbreaker_rp_gen, p() -> gains.heartbreaker, this );
    }

    if ( p() -> covenant.deaths_due -> ok() && p() -> in_death_and_decay() )
    {
      p() -> buffs.deaths_due->trigger();
    }
  }
};

// Horn of Winter ===========================================================

struct horn_of_winter_t : public death_knight_spell_t
{
  horn_of_winter_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "horn_of_winter", p, p -> talent.horn_of_winter )
  {
    parse_options( options_str );
    harmful = false;
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

struct avalanche_t : public death_knight_spell_t
{
  avalanche_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> find_spell( 207150 ) )
  {
    aoe = -1;
    background = true;
  }
};

struct howling_blast_t : public death_knight_spell_t
{
  action_t* avalanche;

  howling_blast_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "howling_blast", p, p -> spec.howling_blast )
  {
    parse_options( options_str );
    triggers_shackle_the_unworthy = true;

    aoe = -1;
    reduced_aoe_damage = true;

    impact_action = get_action<frost_fever_t>( "frost_fever", p );

    ap_type = attack_power_type::WEAPON_BOTH;
    if ( p -> main_hand_weapon.group() == WEAPON_2H )
    {
      ap_type = attack_power_type::WEAPON_MAINHAND;
      // There's a 0.98 modifier hardcoded in the tooltip if a 2H weapon is equipped, probably server side magic
      base_multiplier *= 0.98;
    }

    if ( p -> talent.avalanche -> ok() )
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
      m *= 1.0 + p()->buffs.rime->data().effectN( 2 ).percent() + p() -> spec.rime_2 -> effectN( 1 ).percent();
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

  void execute() override
  {
    death_knight_spell_t::execute();

    // If Pillar of Frost is up, Rime procs still increases its value
    if ( p() -> buffs.pillar_of_frost -> up() && p() -> buffs.rime -> up() )
    {
      p() -> buffs.pillar_of_frost_bonus -> trigger( as<int>( base_costs[ RESOURCE_RUNE ] ) );
    }

    if ( p() -> buffs.pillar_of_frost -> up() && p() -> talent.obliteration -> ok() )
    {
      p() -> trigger_killing_machine( 1.0, p() -> procs.km_from_obliteration_hb,
                                           p() -> procs.km_from_obliteration_hb_wasted );

      // Obliteration's rune generation
      if ( rng().roll( p() -> talent.obliteration -> effectN( 2 ).percent() ) )
      {
        // WTB spelldata for the rune gain
        p() -> replenish_rune( 1, p() -> gains.obliteration );
      }
    }

    if ( p() -> talent.avalanche -> ok() && p() -> buffs.rime -> up() )
    {
      avalanche -> set_target( target );
      avalanche -> execute();
    }

    if ( p() -> buffs.rime -> check() && p() -> legendary.rage_of_the_frozen_champion.ok() )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER,
                            p() -> spell.rage_of_the_frozen_champion -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
                            p() -> gains.rage_of_the_frozen_champion );
    }

    p() -> buffs.rime -> decrement();
  }
};

// Hypothermic Presence =====================================================

struct hypothermic_presence_t : public death_knight_spell_t
{
  hypothermic_presence_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "hypothermic_presence", p, p -> talent.hypothermic_presence )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    p() -> buffs.hypothermic_presence -> trigger();
  }
};

// Marrowrend ===============================================================

struct marrowrend_t : public death_knight_melee_attack_t
{
  marrowrend_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "marrowrend", p, p -> spec.marrowrend )
  {
    parse_options( options_str );
    weapon = &( p -> main_hand_weapon );
    triggers_shackle_the_unworthy = true;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> up() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.marrowrend -> set_target(  target );
      p() -> pets.dancing_rune_weapon_pet -> ability.marrowrend -> execute();
    }
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    p() -> buffs.bone_shield -> trigger( as<int>( data().effectN( 3 ).base_value() ) );
  }
};

// Mind Freeze ==============================================================

struct mind_freeze_t : public death_knight_spell_t
{
  mind_freeze_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "mind_freeze", p, p -> find_class_spell( "Mind Freeze" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( ! candidate_target -> debuffs.casting || ! candidate_target -> debuffs.casting -> check() )
      return false;

    return death_knight_spell_t::target_ready( candidate_target );
  }
};

// Obliterate ===============================================================

struct obliterate_strike_t : public death_knight_melee_attack_t
{
  int deaths_due_cleave_targets;
  obliterate_strike_t( death_knight_t* p, const std::string& name,
                       weapon_t* w, const spell_data_t* s ) :
    death_knight_melee_attack_t( name, p, s )
  {
    background = special = true;
    may_miss = false;
    weapon = w;
    triggers_icecap = true;

    // To support Death's Due affecting Obliterate in shadowlands:
    // - obliterate damage spells have gained a value of 1 in their chain target data
    // - the death and decay buff now has an effect that modifies obliterate's chain target with a value of 0
    // - death's due increases the aforementionned death and decay buff effect by 1
    deaths_due_cleave_targets = data().effectN ( 1 ).chain_target() +
                                as<int>( p -> spell.dnd_buff -> effectN ( 4 ).base_value() ) +
                                as<int>( p -> spell.deaths_due -> effectN( 2 ).base_value() );

    base_multiplier *= 1.0 + p -> spec.obliterate_2 -> effectN( 1 ).percent();
    // Rank 2 validates the 2H bonus that is contained in rank 1 data
    if ( p -> spec.might_of_the_frozen_wastes_2 -> ok() && p -> main_hand_weapon.group() == WEAPON_2H )
    {
      base_multiplier *= 1.0 + p -> spec.might_of_the_frozen_wastes -> effectN( 1 ).percent();
    }
    if ( p -> legendary.koltiras_favor.ok() )
    {
      base_multiplier *= 1.0 + p -> legendary.koltiras_favor -> effectN ( 2 ).percent();
    }
  }

  int n_targets() const override
  {
      if ( p() -> covenant.deaths_due -> ok() && p() -> in_death_and_decay() )
        return deaths_due_cleave_targets;
      return death_knight_melee_attack_t::n_targets();
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_melee_attack_t::composite_da_multiplier( state );
    // Obliterate does not list Frozen Heart in it's list of affecting spells.  So mastery does not get applied automatically.
    if ( p() -> spec.killing_machine_2 -> ok() && p() -> buffs.killing_machine -> up() )
    {
      m *= 1.0 + p() -> cache.mastery_value();
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    death_knight_td_t* td = p() -> get_target_data( target );
    // Obliterate does not list razorice in it's list of affecting spells, so debuff does not get applied automatically.
    if ( p() -> spec.killing_machine_2 -> ok() && p() -> buffs.killing_machine -> up() )
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
    if ( p() -> covenant.deaths_due && p() -> in_death_and_decay() )
    {
      p() -> buffs.deaths_due->trigger();
    }
  }

  void execute() override
  {
    if ( ! p() -> options.split_obliterate_schools &&
         p() -> spec.killing_machine_2 -> ok() && p() -> buffs.killing_machine -> up() )
    {
      school = SCHOOL_FROST;
    }

    death_knight_melee_attack_t::execute();

    if ( p() -> conduits.eradicating_blow->ok() )
    {
      p() -> buffs.eradicating_blow -> trigger();
    }

    if ( p() -> legendary.koltiras_favor.ok() && p() -> cooldown.koltiras_favor_icd->is_ready() )
    {
      if ( p() -> rng().roll(p() -> legendary.koltiras_favor->proc_chance()))
      {
        // # of runes to restore was stored in a secondary affect
        p() -> replenish_rune( as<unsigned int>( p() -> legendary.koltiras_favor->effectN( 1 ).trigger()->effectN( 1 ).base_value() ), p() -> gains.koltiras_favor );
        p() -> cooldown.koltiras_favor_icd -> start( p() -> legendary.koltiras_favor -> internal_cooldown() );
      }
    }

    // KM Rank 2 - revert school after the hit
    if ( ! p() -> options.split_obliterate_schools ) school = SCHOOL_PHYSICAL;
  }
};

struct obliterate_t : public death_knight_melee_attack_t
{
  obliterate_strike_t *mh, *oh, *km_mh, *km_oh;
  action_t* inexorable_assault;

  obliterate_t( death_knight_t* p, const std::string& options_str = std::string() ) :
    death_knight_melee_attack_t( "obliterate", p, p -> spec.obliterate ),
    mh( nullptr ), oh( nullptr ), km_mh( nullptr ), km_oh( nullptr )
  {
    parse_options( options_str );
    dual = true;
    triggers_shackle_the_unworthy = true;

    inexorable_assault = get_action<inexorable_assault_damage_t>( "inexorable_assault", p );

    const spell_data_t* mh_data = p -> main_hand_weapon.group() == WEAPON_2H ? data().effectN( 4 ).trigger() : data().effectN( 2 ).trigger();

    mh = new obliterate_strike_t( p, "obliterate", &( p -> main_hand_weapon ), mh_data );
    add_child( mh );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      oh = new obliterate_strike_t( p, "obliterate_offhand", &( p -> off_hand_weapon ), data().effectN( 3 ).trigger() );
      add_child( oh );
    }
    if ( p -> options.split_obliterate_schools && p -> spec.killing_machine_2 -> ok() )
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
      if ( km_mh && p() -> buffs.killing_machine -> up() )
      {
        km_mh -> set_target( target );
        km_mh -> execute();
      }
      else
      {
        mh -> set_target( target );
        mh -> execute();
      }

      if ( oh )
      {
        if ( km_oh && p() -> buffs.killing_machine -> up() )
        {
          km_oh -> set_target( target );
          km_oh -> execute();
        }
        else
        {
          oh -> set_target( target );
          oh -> execute();
        }
      }

      if ( p() -> buffs.inexorable_assault -> up() )
      {
        inexorable_assault -> set_target( target );
        inexorable_assault -> schedule_execute();
        p() -> buffs.inexorable_assault -> decrement();
      }

      p() -> buffs.rime -> trigger();
    }

    p() -> consume_killing_machine( p() -> procs.killing_machine_oblit );
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
};

// Outbreak and Virulent Eruption ===========================================

struct virulent_eruption_t : public death_knight_spell_t
{
  virulent_eruption_t( death_knight_t* p ) :
    death_knight_spell_t( "virulent_eruption", p, p -> spell.virulent_eruption )
  {
    background = split_aoe_damage = true;
    aoe = -1;
  }
};

struct outbreak_t : public death_knight_spell_t
{
  outbreak_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "outbreak" ,p , p -> spec.outbreak )
  {
    parse_options( options_str );
    triggers_shackle_the_unworthy = true;
    impact_action = get_action<virulent_plague_t>( "virulent_plague", p );
  }
};

// Pillar of Frost ==========================================================

// Ingame it seems to only change Pillar of frost's strength bonus
// The simc implementation creates a dummy buff to better track the strength increase through stack count
struct pillar_of_frost_bonus_buff_t : public buff_t
{
  pillar_of_frost_bonus_buff_t( death_knight_t* p ) :
    buff_t( p, "pillar_of_frost_bonus" )
  {
    set_max_stack( 99 );
    set_duration( p -> spec.pillar_of_frost -> duration() );
    set_default_value( p -> spec.pillar_of_frost -> effectN( 2 ).percent() );

    add_invalidate( CACHE_STRENGTH );
  }
};

struct pillar_of_frost_buff_t : public buff_t
{
  pillar_of_frost_buff_t( death_knight_t* p ) :
    buff_t( p, "pillar_of_frost", p -> spec.pillar_of_frost )
  {
    cooldown -> duration = 0_ms;
    set_default_value( p -> spec.pillar_of_frost -> effectN( 1 ).percent() +
                       p -> spec.pillar_of_frost_2 -> effectN( 1 ).percent() );
    add_invalidate( CACHE_STRENGTH );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    // Refreshing Pillar of Frost resets the ramping strength bonus and triggers Icy Citadel
    death_knight_t* p = debug_cast<death_knight_t*>( player );
    if ( p -> buffs.pillar_of_frost -> up() )
    {
      p -> buffs.pillar_of_frost_bonus -> expire();
    }

    return buff_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int, timespan_t ) override
  {
    death_knight_t* p = debug_cast<death_knight_t*>( player );

    p -> buffs.pillar_of_frost_bonus -> expire();
  }
};

struct pillar_of_frost_t : public death_knight_spell_t
{
  pillar_of_frost_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "pillar_of_frost", p, p -> spec.pillar_of_frost )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.pillar_of_frost -> trigger();
  }
};

// Raise Dead ===============================================================

struct raise_dead_t : public death_knight_spell_t
{
  raise_dead_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "raise_dead", p, p -> spec.raise_dead_2 -> ok() ?
                          p -> spec.raise_dead_2 : p -> spec.raise_dead )
  {
    parse_options( options_str );

    cooldown -> duration += p -> talent.all_will_serve -> effectN( 1 ).time_value();

    harmful = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    // If the action is done in precombat and the pet is permanent
    // Assume that the player used it long enough before pull that the cooldown is ready again
    if ( is_precombat && p() -> spec.raise_dead_2 )
    {
      cooldown -> reset( false );
      p() -> pets.ghoul_pet -> precombat_spawn = true;
    }

    // Summon for the duration specified in spelldata if there's one (no data = permanent pet)
    p() -> pets.ghoul_pet -> summon( data().duration() );

    // Sacrificial Pact doesn't despawn risen skulker, so make sure it's not already up before spawning it
    if ( p() -> talent.all_will_serve -> ok() && p() -> pets.risen_skulker -> is_sleeping() )
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

struct remorseless_winter_damage_t : public death_knight_spell_t
{
  double biting_cold_target_threshold;
  bool triggered_biting_cold;

  remorseless_winter_damage_t( death_knight_t* p ) :
    death_knight_spell_t( "remorseless_winter_damage", p, p -> spec.remorseless_winter -> effectN( 2 ).trigger() ),
    biting_cold_target_threshold( 0 ), triggered_biting_cold( false )
  {
    background = true;
    aoe = -1;
    base_multiplier *= 1.0 + p -> spec.remorseless_winter_2 -> effectN( 1 ).percent();

    ap_type = attack_power_type::WEAPON_BOTH;

    if ( p -> main_hand_weapon.group() == WEAPON_2H )
    {
      ap_type = attack_power_type::WEAPON_MAINHAND;
      // There's a 0.98 modifier hardcoded in the tooltip if a 2H weapon is equipped, probably server side magic
      base_multiplier *= 0.98;
    }

    if ( p -> legendary.biting_cold.ok() )
    {
      biting_cold_target_threshold = p -> legendary.biting_cold->effectN ( 1 ).base_value();
      base_multiplier *= 1.0 + p -> legendary.biting_cold->effectN( 2 ).percent();
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

    m *= 1.0 + p() -> get_target_data( t ) -> debuff.everfrost -> stack_value();

    return m;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( state -> n_targets >= biting_cold_target_threshold && p() -> legendary.biting_cold.ok() && ! triggered_biting_cold )
    {
      p() -> buffs.rime -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
      triggered_biting_cold = true;
    }

    if ( p() -> conduits.everfrost.ok() )
      td( state -> target ) -> debuff.everfrost -> trigger();
  }
};

struct remorseless_winter_buff_t : public buff_t
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
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    // TODO: check whether refreshing RW (with GS and high haste) lets you trigger biting cold again
    // Move to expire override if necessary
    debug_cast<remorseless_winter_damage_t*>( damage ) -> triggered_biting_cold = false;
    return buff_t::trigger( s, v, c, d );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    debug_cast<death_knight_t*>( player ) -> buffs.gathering_storm -> expire();
  }
};

struct remorseless_winter_t : public death_knight_spell_t
{
  remorseless_winter_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "remorseless_winter", p, p -> spec.remorseless_winter )
  {
    may_crit = may_miss = may_dodge = may_parry = false;

    parse_options( options_str );

    // Periodic behavior handled by the buff
    dot_duration = base_tick_time = 0_ms;

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

struct sacrificial_pact_damage_t : public death_knight_spell_t
{
  sacrificial_pact_damage_t( util::string_view name, death_knight_t* p ) :
    death_knight_spell_t( name, p, p -> find_spell( 327611 ) )
  {
    background = true;
    aoe = as<int>( data().effectN( 2 ).base_value() );
  }
};

struct sacrificial_pact_t : public death_knight_heal_t
{
  action_t* damage;
  sacrificial_pact_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_heal_t( "sacrificial_pact", p, p -> spec.sacrificial_pact )
  {
    target = p;
    base_pct_heal = data().effectN( 1 ).percent();
    parse_options( options_str );
    damage = get_action<sacrificial_pact_damage_t>( "sacrificial_pact_damage", p );
  }

  void execute() override
  {
    death_knight_heal_t::execute();

    damage -> set_target( player -> target );
    damage -> execute();

    p() -> pets.ghoul_pet -> dismiss();
  }

  bool ready() override
  {
    if ( p() -> pets.ghoul_pet -> is_sleeping() )
      return false;

    return death_knight_heal_t::ready();
  }
};

// Scourge Strike and Clawing Shadows =======================================

struct scourge_strike_base_t : public death_knight_melee_attack_t
{
  scourge_strike_base_t( const std::string& name, death_knight_t* p, const spell_data_t* spell ) :
    death_knight_melee_attack_t( name, p, spell )
  {
    weapon = &( player -> main_hand_weapon );
  }

  // The death and decay target cap is displayed both in scourge strike's effects
  // And in SS and CS' max_targets data entry. Using the latter
  int n_targets() const override
  { return p() -> in_death_and_decay() ? data().max_targets() : 0; }

  std::vector<player_t*>& target_list() const override // smart targeting to targets with wounds when cleaving SS
  {
    std::vector<player_t*>& current_targets = death_knight_melee_attack_t::target_list();
    if (current_targets.size() < 2 || !p() -> in_death_and_decay()) {
      return current_targets;
    }

    // first target, the action target, needs to be left in place
    std::sort( current_targets.begin() + 1, current_targets.end(),
      [this]( player_t* a, player_t* b) {
        int a_stacks = td( a ) -> debuff.festering_wound -> up() ? 1 : 0;
        int b_stacks = td( b ) -> debuff.festering_wound -> up() ? 1 : 0;
        return a_stacks > b_stacks;
      } );

    return current_targets;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    p() -> burst_festering_wound( state -> target, 1 );

    if ( p() -> covenant.deaths_due -> ok() && p() -> in_death_and_decay() )
    {
      p() -> buffs.deaths_due->trigger();
    }
  }
};

struct clawing_shadows_t : public scourge_strike_base_t
{
  clawing_shadows_t( death_knight_t* p, const std::string& options_str ) :
    scourge_strike_base_t( "clawing_shadows", p, p -> talent.clawing_shadows )
  {
    parse_options( options_str );
    triggers_shackle_the_unworthy = true;
    base_multiplier *= 1.0 + p -> spec.scourge_strike_2 -> effectN( 1 ).percent();
  }
};

struct scourge_strike_shadow_t : public death_knight_melee_attack_t
{
  scourge_strike_shadow_t( util::string_view name, death_knight_t* p ) :
    death_knight_melee_attack_t( name, p, p -> spec.scourge_strike -> effectN( 3 ).trigger() )
  {
    may_miss = may_parry = may_dodge = false;
    background = proc = dual = true;
    weapon = &( player -> main_hand_weapon );
    base_multiplier *= 1.0 + p -> spec.scourge_strike_2 -> effectN( 1 ).percent();
  }
};

struct scourge_strike_t : public scourge_strike_base_t
{
  scourge_strike_t( death_knight_t* p, const std::string& options_str ) :
    scourge_strike_base_t( "scourge_strike", p, p -> spec.scourge_strike )
  {
    parse_options( options_str );
    triggers_shackle_the_unworthy = true;
    base_multiplier *= 1.0 + p -> spec.scourge_strike_2 -> effectN( 1 ).percent();

    impact_action = get_action<scourge_strike_shadow_t>( "scourge_strike_shadow", p );
    add_child( impact_action );

    // Disable when Clawing Shadows is talented
    if ( p -> talent.clawing_shadows -> ok() )
      background = true;
  }
};

// Shackle the Unworthy =====================================================
// Damage reduction debuff is NYI

struct shackle_the_unworthy_t : public death_knight_spell_t
{
  shackle_the_unworthy_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "shackle_the_unworthy", p, p -> covenant.shackle_the_unworthy )
  {
    parse_options( options_str );
    base_multiplier *= 1.0 + p -> conduits.proliferation.percent();
    dot_duration += p -> conduits.proliferation -> effectN( 2 ).time_value();
    base_tick_time = p-> covenant.shackle_the_unworthy -> effectN( 1 ).period();
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
};

struct soul_reaper_t : public death_knight_melee_attack_t
{
  action_t* soul_reaper_execute;

  soul_reaper_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "soul_reaper", p, p ->  talent.soul_reaper ),
    soul_reaper_execute( get_action<soul_reaper_execute_t>( "soul_reaper_execute", p ) )
  {
    parse_options( options_str );
    add_child( soul_reaper_execute );

    triggers_shackle_the_unworthy = true;
    hasted_ticks = false;
  }

  void last_tick( dot_t* dot ) override
  {
    if ( dot -> target -> health_percentage() < data().effectN( 3 ).base_value() )
    {
      soul_reaper_execute -> set_target ( dot -> target );
      soul_reaper_execute -> execute();
    }
  }
};

// Summon Gargoyle ==========================================================

struct summon_gargoyle_t : public death_knight_spell_t
{
  summon_gargoyle_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "summon_gargoyle", p, p -> talent.summon_gargoyle )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> pets.gargoyle -> summon( data().duration() );
  }
};

// Swarming Mist ============================================================

struct swarming_mist_damage_t : public death_knight_spell_t
{
  int swarming_mist_energize_target_cap;
  int swarming_mist_energize_tick;
  int swarming_mist_energize_amount;

  swarming_mist_damage_t( death_knight_t* p ) :
    death_knight_spell_t( "swarming_mist_damage", p, p -> covenant.swarming_mist -> effectN( 1 ).trigger() ),
    swarming_mist_energize_target_cap( as<int>( p -> covenant.swarming_mist -> effectN( 3 ).base_value() ) ),
    swarming_mist_energize_tick( 0 )
  {
    background = true;
    aoe = -1;
    base_multiplier *= 1.0 + p -> conduits.impenetrable_gloom.percent();
    swarming_mist_energize_amount = as<int>( p -> covenant.swarming_mist->ok() ? p -> find_spell( 312546 ) -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) : 0 );
  }

  void execute() override
  {
    swarming_mist_energize_tick = 0;
    death_knight_spell_t::execute();
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double cam = death_knight_spell_t::composite_aoe_multiplier( state );

    if ( state->n_targets > p() -> covenant.swarming_mist ->effectN( 5 ).base_value() )
        // When we cross over 5 targets, sqrt on all targets kicks in
        cam *= std::sqrt( 5.0 / state->n_targets );

    return cam;
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );
    if ( swarming_mist_energize_tick < swarming_mist_energize_target_cap )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER,
                 swarming_mist_energize_amount,
                 p() -> gains.swarming_mist );
      swarming_mist_energize_tick++;
    }
  }
};

struct swarming_mist_buff_t : public buff_t
{
  swarming_mist_damage_t* damage; // (AOE) damage that ticks every second

  swarming_mist_buff_t( death_knight_t* p ) :
    buff_t( p, "swarming_mist", p -> covenant.swarming_mist ),
    damage( new swarming_mist_damage_t( p ) )
  {
    cooldown -> duration = 0_ms; // Controlled by the action
    set_tick_callback( [ this ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ )
    {
      damage -> execute();
    } );
    set_partial_tick( true );

    add_invalidate( CACHE_DODGE );
  }
};

struct swarming_mist_t : public death_knight_spell_t
{
  swarming_mist_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "swarming_mist", p, p -> covenant.swarming_mist )
  {
    may_crit = may_miss = may_dodge = may_parry = false;

    parse_options( options_str );

    // Periodic behavior handled by the buff
    dot_duration = base_tick_time = 0_ms;

    if ( action_t* swarming_mist_damage = p -> find_action( "swarming_mist_damage" ) )
    {
      add_child( swarming_mist_damage );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.swarming_mist -> trigger();
  }
};

// Tombstone ================================================================
// Not with Defensive Abilities because of the reliable RP generation

struct tombstone_t : public death_knight_spell_t
{
  tombstone_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "tombstone", p, p -> talent.tombstone )
  {
    parse_options( options_str );

    harmful = may_crit = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    int charges = std::min( p() -> buffs.bone_shield -> stack(), as<int>( data().effectN( 5 ).base_value() ) );

    double power = charges * data().effectN( 3 ).base_value();
    double shield = charges * data().effectN( 4 ).percent();

    p() -> resource_gain( RESOURCE_RUNIC_POWER, power, p() -> gains.tombstone, this );
    p() -> buffs.tombstone -> trigger( 1, shield * p() -> resources.max[ RESOURCE_HEALTH ] );
    p() -> buffs.bone_shield -> decrement( charges );
  }
};

// Unholy Blight ============================================================

struct unholy_blight_dot_t : public death_knight_spell_t
{
  unholy_blight_dot_t( death_knight_t* p ) :
    death_knight_spell_t( "unholy_blight_dot", p, p -> talent.unholy_blight -> effectN( 1 ).trigger() )
  {
    tick_may_crit = background = true;
    may_miss = may_crit = hasted_ticks = false;
    aoe = -1;
    impact_action = get_action<virulent_plague_t>( "virulent_plague", p );
  }

  timespan_t calculate_dot_refresh_duration( const dot_t*, timespan_t triggered_duration ) const override
  {
    // No longer pandemics
    return triggered_duration;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    td( state->target ) -> debuff.unholy_blight -> trigger();
  }
};

struct unholy_blight_buff_t : public buff_t
{
  unholy_blight_dot_t* dot;

  unholy_blight_buff_t( death_knight_t* p ) :
    buff_t( p, "unholy_blight", p -> talent.unholy_blight ),
    dot( new unholy_blight_dot_t( p ) )
  {
    cooldown -> duration = 0_ms;
    set_tick_callback( [ this ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ )
    {
      dot -> execute();
    } );
  }
};

struct unholy_blight_t : public death_knight_spell_t
{
  unholy_blight_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "unholy_blight", p, p -> talent.unholy_blight )
  {
    may_crit = may_miss = may_dodge = may_parry = hasted_ticks = false;
    tick_zero = true;
    parse_options( options_str );

    aoe = -1;

    if ( action_t* dot_damage = p -> find_action( "unholy_blight_dot" ) )
    {
      add_child( dot_damage );
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.unholy_blight -> trigger();
  }
};

// Unholy Assault ============================================================

struct unholy_assault_t : public death_knight_melee_attack_t
{
  unholy_assault_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "unholy_assault", p, p -> talent.unholy_assault )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();
    p() -> buffs.unholy_assault -> trigger();
  }

  void impact( action_state_t* s) override
  {
    death_knight_melee_attack_t::impact( s );

    p()->trigger_festering_wound( s, as<int>( p() -> talent.unholy_assault -> effectN( 3 ).base_value() ),
                                  p() -> procs.fw_unholy_assault );
  }
};

// ==========================================================================
// Death Knight Defensive Abilities
// ==========================================================================

// Anti-magic Shell =========================================================

struct antimagic_shell_buff_t : public buff_t
{
  double remaining_absorb;

  antimagic_shell_buff_t( death_knight_t* p ) :
    buff_t( p, "antimagic_shell", p -> spec.antimagic_shell ),
    remaining_absorb( 0.0 )
  {
    cooldown -> duration = 0_ms;

    base_buff_duration += p -> talent.spell_eater -> effectN( 2 ).time_value();

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
    double max_absorb = dk -> resources.max[ RESOURCE_HEALTH ] * dk -> spec.antimagic_shell -> effectN( 2 ).percent();

    max_absorb *= 1.0 + dk -> talent.antimagic_barrier -> effectN( 2 ).percent();
    max_absorb *= 1.0 + dk -> talent.spell_eater -> effectN( 1 ).percent();

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

struct antimagic_shell_t : public death_knight_spell_t
{
  double min_interval;
  double interval;
  double interval_stddev;
  double interval_stddev_opt;
  double damage;

  antimagic_shell_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "antimagic_shell", p, p -> spec.antimagic_shell ),
    min_interval( 60 ), interval( 60 ), interval_stddev( 0.05 ), interval_stddev_opt( 0 ), damage( 0 )
  {
    cooldown -> duration += p -> talent.antimagic_barrier -> effectN( 1 ).time_value();
    harmful = may_crit = may_miss = false;
    base_dd_min = base_dd_max = 0;
    target = player;

    add_option( opt_float( "interval", interval ) );
    add_option( opt_float( "interval_stddev", interval_stddev_opt ) );
    add_option( opt_float( "damage", damage ) );
    parse_options( options_str );

    min_interval += p -> talent.antimagic_barrier -> effectN( 1 ).base_value();

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

    // If using the fake soaking, immediately grant the RP in one go
    if ( damage > 0 )
    {
      double absorbed = std::min( damage,
                                  debug_cast< antimagic_shell_buff_t* >( p() -> buffs.antimagic_shell ) -> calc_absorb() );

      // AMS generates 1 runic power per percentage max health absorbed.
      double rp_generated = absorbed / p() -> resources.max[ RESOURCE_HEALTH ] * 100;

      p() -> resource_gain( RESOURCE_RUNIC_POWER, util::round( rp_generated ), p() -> gains.antimagic_shell, this );
    }
    else
      p() -> buffs.antimagic_shell -> trigger();
  }
};

// Icebound Fortitude =======================================================

struct icebound_fortitude_t : public death_knight_spell_t
{
  icebound_fortitude_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "icebound_fortitude", p, p -> spec.icebound_fortitude )
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

struct mark_of_blood_heal_t : public death_knight_heal_t
{
  mark_of_blood_heal_t( death_knight_t* p ) : // The data is removed and switched to the talent spell on PTR 8.3.0.32805
    death_knight_heal_t( "mark_of_blood_heal", p, p -> talent.mark_of_blood )
  {
    may_crit = callbacks = false;
    background = dual = true;
    target = p;
    base_pct_heal = data().effectN( 1 ).percent();
  }
};

struct mark_of_blood_t : public death_knight_spell_t
{
  mark_of_blood_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "mark_of_blood", p, p -> talent.mark_of_blood )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    td( target ) -> debuff.mark_of_blood -> trigger();
  }
};

// Rune Tap =================================================================

struct rune_tap_t : public death_knight_spell_t
{
  rune_tap_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "rune_tap", p, p -> spec.rune_tap )
  {
    parse_options( options_str );
    use_off_gcd = true;
    cooldown -> charges += as<int>( p -> spec.rune_tap_2 -> effectN( 1 ).base_value() );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.rune_tap -> trigger();
  }
};

// Vampiric Blood ===========================================================

struct vampiric_blood_buff_t : public buff_t
{
  double health_change;

  vampiric_blood_buff_t( death_knight_t* player ) :
    buff_t( player, "vampiric_blood", player -> spec.vampiric_blood ),
    health_change( data().effectN( 4 ).percent() )
  {
    // Cooldown handled by the action
    cooldown -> duration = 0_ms;
    base_buff_duration += player -> spec.vampiric_blood_2 -> effectN( 3 ).time_value();
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    buff_t::start( stacks, value, duration );

    double old_health = player -> resources.current[ RESOURCE_HEALTH ];
    double old_max_health = player -> resources.max[ RESOURCE_HEALTH ];

    player -> resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + health_change;
    player -> recalculate_resource_max( RESOURCE_HEALTH );
    player -> resources.current[ RESOURCE_HEALTH ] *= 1.0 + health_change; // Update health after the maximum is increased

    sim -> print_debug( "{} gains Vampiric Blood: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                                  player -> name(), health_change * 100.0,
                                  old_health, player -> resources.current[ RESOURCE_HEALTH ],
                                  old_max_health, player -> resources.max[ RESOURCE_HEALTH ] );
  }

  void expire_override( int, timespan_t ) override
  {
    double old_health = player -> resources.current[ RESOURCE_HEALTH ];
    double old_max_health = player -> resources.max[ RESOURCE_HEALTH ];

    player -> resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + health_change;
    player -> resources.current[ RESOURCE_HEALTH ] /= 1.0 + health_change; // Update health before the maximum is reduced
    player -> recalculate_resource_max( RESOURCE_HEALTH );

    sim -> print_debug( "{} loses Vampiric Blood: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                        player -> name(), health_change * 100.0,
                        old_health, player -> resources.current[ RESOURCE_HEALTH ],
                        old_max_health, player -> resources.max[ RESOURCE_HEALTH ] );
  }
};

struct vampiric_blood_t : public death_knight_spell_t
{
  vampiric_blood_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "vampiric_blood", p, p -> spec.vampiric_blood )
  {
    parse_options( options_str );

    harmful = false;
    base_dd_min = base_dd_max = 0;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.vampiric_blood -> trigger();
  }
};

// Buffs ====================================================================

struct runic_corruption_buff_t : public buff_t
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
  struct fallen_crusader_heal_t : public death_knight_heal_t
  {
    fallen_crusader_heal_t( util::string_view name, death_knight_t* p, const spell_data_t* data ) :
      death_knight_heal_t( name, p, data )
    {
      background = true;
      target = player;
      callbacks = may_crit = false;
      base_pct_heal = data -> effectN( 2 ).percent();
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

  p -> runeforge.rune_of_razorice = true;

  // Create the appropriate razorice attack depending on where the runeforge is applied
  if ( effect.item -> slot == SLOT_MAIN_HAND )
  {
    p -> active_spells.razorice_mh = new razorice_attack_t( p, "razorice" );
  }
  else if ( effect.item -> slot == SLOT_OFF_HAND )
  {
    p -> active_spells.razorice_oh = new razorice_attack_t( p, "razorice_offhand" );
  }
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
    -> set_pct_buff_type( STAT_PCT_BUFF_INTELLECT );

  // Change the player's base armor multiplier
  p -> base.armor_multiplier *= 1.0 + effect.driver() -> effectN( 1 ).percent();

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
  p -> active_spells.runeforge_pestilence = new death_knight_spell_t( "Pestilence", p, p -> find_spell( 327093 ) );
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
      -> set_default_value_from_effect( 1 );
  }

  // The RP cap increase stacks
  p -> resources.base[ RESOURCE_RUNIC_POWER ] += effect.driver() -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER );

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

  struct sanguination_heal_t : public death_knight_heal_t
  {
    double health_threshold;
    sanguination_heal_t( special_effect_t& effect ) :
      death_knight_heal_t( "rune_of_sanguination", static_cast<death_knight_t*>( effect.player ),
                           effect.driver() -> effectN( 1 ).trigger() ),
      health_threshold( effect.driver() -> effectN( 1 ).base_value() )
    {
      background = true;
      tick_pct_heal = data().effectN( 1 ).percent();
      // Sated-type debuff, for simplicity the debuff's duration is used as a simple cooldown in simc
      cooldown -> duration = effect.player -> find_spell( 326809 ) -> duration();
    }

    bool ready() override
    {
      if ( p() -> health_percentage() > health_threshold )
        return false;

      return death_knight_heal_t::ready();
    }
  };

  p -> runeforge.rune_of_sanguination = true;

  p -> active_spells.rune_of_sanguination = new sanguination_heal_t( effect );
}

void runeforge::spellwarding( special_effect_t& effect )
{
  struct spellwarding_absorb_t : public absorb_t
  {
    double health_percentage;
    spellwarding_absorb_t( util::string_view name, death_knight_t* p, const spell_data_t* data ) :
      absorb_t( name, p, data ),
      health_percentage( p -> find_spell( 326855 ) -> effectN( 2 ).percent() )
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
  static_cast<death_knight_t*>( effect.player ) -> runeforge.rune_of_unending_thirst = true;
}

// Legendary runeforge Reanimated Shambler
void runeforge::reanimated_shambler( special_effect_t& effect )
{
  if ( effect.player -> type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  struct trigger_reanimated_shambler_t : death_knight_spell_t
  {
    trigger_reanimated_shambler_t( death_knight_t* p ) :
      death_knight_spell_t( "reanimated_shambler_trigger", p )
    {
      quiet = background = true;
      callbacks = false;
    }

    void execute() override
    {
      p() -> procs.reanimated_shambler -> occur();

      p() -> pets.reanimated_shambler.spawn( 1 );
    }
  };

  effect.execute_action = new trigger_reanimated_shambler_t( static_cast<death_knight_t*>( effect.player ) );

  new dbc_proc_callback_t( effect.item, effect );
}


// Resource Manipulation ====================================================

double death_knight_t::resource_gain( resource_e resource_type, double amount, gain_t* g, action_t* action )
{
  double actual_amount = player_t::resource_gain( resource_type, amount, g, action );

  if ( runeforge.rune_of_hysteria && resource_type == RESOURCE_RUNIC_POWER )
  {
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
    if ( talent.gathering_storm -> ok() && buffs.remorseless_winter -> check() &&
         action -> data().id() != spec.remorseless_winter -> id() )
    {
      unsigned consumed = static_cast<unsigned>( action -> base_costs[ RESOURCE_RUNE ] );
      buffs.gathering_storm -> trigger( consumed );
      timespan_t base_extension = timespan_t::from_seconds( talent.gathering_storm -> effectN( 1 ).base_value() / 10.0 );
      buffs.remorseless_winter -> extend_duration( this, base_extension * consumed );
    }

    // Effects that require the player to actually spend runes
    if ( actual_amount > 0 )
    {
      if ( buffs.pillar_of_frost -> up() )
      {
        buffs.pillar_of_frost_bonus -> trigger( as<int>( actual_amount ) );
      }
    }
  }

  // Procs from RP spent
  if ( resource_type == RESOURCE_RUNIC_POWER )
  {
    // 2019-02-12: Runic Empowerment, Runic Corruption, Red Thirst and gargoyle's shadow empowerment
    // seem to be using the base cost of the ability rather than the last resource cost
    // Bug? Intended?
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/396
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/397

    // Some abilities use the actual RP spent by the ability, others use the base RP cost
    double base_rp_cost = actual_amount;

    // If an action is linked, fetch its base cost
    if ( action )
      base_rp_cost = action -> base_costs[ RESOURCE_RUNIC_POWER ];

    // 2020-12-16 - Melekus: Based on testing with both Frost Strike and Breath of Sindragosa during Hypothermic Presence,
    // RE is using the ability's base cost for its proc chance calculation, just like Runic Corruption
    trigger_runic_empowerment( base_rp_cost );
    trigger_runic_corruption( procs.rp_runic_corruption, base_rp_cost );

    if ( talent.summon_gargoyle -> ok() && pets.gargoyle )
    {
      // 2019-02-12 Death Strike buffs gargoyle as if it had its full cost, even though it received a -10rp cost in 8.1
      // Assuming it's a bug for now, using base_costs instead of last_resource_cost won't affect free spells here
      // Free Death Coils are still handled in the action
      pets.gargoyle -> increase_power( base_rp_cost );
    }

    buffs.icy_talons -> trigger();

    // Effects that only trigger if resources were spent
    if ( actual_amount > 0 )
    {
      if ( talent.red_thirst -> ok() )
      {
        timespan_t sec = timespan_t::from_seconds( talent.red_thirst -> effectN( 1 ).base_value() / 100 ) *
          actual_amount / talent.red_thirst -> effectN( 2 ).base_value();
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

  add_option( opt_bool( "disable_aotd", options.disable_aotd ) );
  add_option( opt_bool( "split_ghoul_regen", options.split_ghoul_regen ) );
  add_option( opt_bool( "split_obliterate_schools", options.split_obliterate_schools ) );
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

  if ( ! talent.soul_reaper -> ok() )
  {
    return;
  }

  death_knight_td_t* td = get_target_data( target );

  if ( td -> dot.soul_reaper -> is_ticking() )
  {
    sim -> print_log( "Target {} died while affected by Soul Reaper, player {} gains Runic Corruption buff.",
                      target -> name(), name() );

    trigger_runic_corruption( procs.sr_runic_corruption, 0, 1.0 );
  }
}

void death_knight_t::trigger_festering_wound_death( player_t* target )
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( sim -> event_mgr.canceled )
  {
    return;
  }

  if ( ! spec.festering_wound -> ok() )
  {
    return;
  }

  death_knight_td_t* td = get_target_data( target );
  int n_wounds = td -> debuff.festering_wound -> check();

  // If the target wasn't affected by festering wound, return
  if ( n_wounds == 0 ) return;

  // Generate the RP you'd have gained if you popped the wounds manually
  resource_gain( RESOURCE_RUNIC_POWER,
                 spec.festering_wound -> effectN( 1 ).trigger() -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER ) * n_wounds,
                 gains.festering_wound );

  if ( talent.pestilent_pustules -> ok() )
  {
    trigger_runic_corruption( procs.pp_runic_corruption, 0, talent.pestilent_pustules -> effectN( 1 ).percent() * n_wounds );
  }

  // Triggers a bursting sores explosion for each wound on the target
  if ( talent.bursting_sores -> ok() && active_spells.bursting_sores -> target_list().size() > 0 )
  {
    for ( int i = 0; i < n_wounds; i++ )
    {
      active_spells.bursting_sores -> set_target( target );
      active_spells.bursting_sores -> execute();
    }
  }
}

void death_knight_t::trigger_virulent_plague_death( player_t* target )
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( sim -> event_mgr.canceled )
  {
    return;
  }

  death_knight_td_t* td = get_target_data( target );

  if ( ! td -> dot.virulent_plague -> is_ticking() )
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
  bool wasted = buffs.killing_machine -> up();
  // If the given chance is 0, use the auto attack proc mechanic (new system, or rppm if the option is selected)
  // Melekus, 2020-06-03: It appears that Killing Machine now procs from auto attacks following a custom system
  // Every critical auto attack has a 30% * number of missed proc attempts to trigger Killing Machine
  // Originally found by Bicepspump, made public on 2020-05-17
  // This may have been added to the game on patch 8.2, when rppm data from Killing Machine was removed from the game
  if ( chance == 0 )
  {
    // If we are using a 1H, we use 0.3 per attempt, with 2H it looks to be 0.7 through testing
    double km_proc_chance = 0.3;
    if ( spec.might_of_the_frozen_wastes_2 -> ok() && main_hand_weapon.group() == WEAPON_2H )
    {
      km_proc_chance = 0.7;
    }

    if ( rng().roll( ++km_proc_attempts * km_proc_chance ) )
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

  if ( rng().roll( talent.murderous_efficiency -> effectN( 1 ).percent() ) )
  {
    replenish_rune( as<int>( spell.murderous_efficiency_gain -> effectN( 1 ).base_value() ), gains.murderous_efficiency );
  }
}

void death_knight_t::trigger_runic_empowerment( double rpcost )
{
  if ( ! spec.runic_empowerment -> ok() )
    return;

  double base_chance = spec.runic_empowerment -> effectN( 1 ).percent() / 10.0;

  if ( ! rng().roll( base_chance * rpcost ) )
    return;

  if ( sim -> debug )
  {
    log_rune_status( this );
  }

  if ( replenish_rune( 1, gains.runic_empowerment ) && sim -> debug )
  {
    sim -> print_debug( "{} Runic Empowerment regenerated rune", name() );
    log_rune_status( this );
  }
}

void death_knight_t::trigger_runic_corruption( proc_t* proc, double rpcost, double override_chance )
{
  if ( ! spec.runic_corruption -> ok() )
    return;

  double proc_chance = 0.0;
  // Use the overriden chance if there's one and RP cost is 0
  proc_chance = ( !rpcost && override_chance != -1.0 ) ? override_chance :
    // Else, use the general proc chance ( 1.6 per RP * RP / 100 as of patch 9.0.2 )
    spec.runic_corruption -> effectN( 1 ).percent() * rpcost / 100.0;

  // Buff duration and refresh behavior handled in runic_corruption_buff_t
  if ( buffs.runic_corruption -> trigger( 1, buff_t::DEFAULT_VALUE(), proc_chance ) && proc )
    proc -> occur();
}

void death_knight_t::trigger_festering_wound( const action_state_t* state, unsigned n, proc_t* proc )
{
  if ( ! state -> action -> result_is_hit( state -> result ) )
  {
    return;
  }

  auto td = get_target_data( state -> target );

  td -> debuff.festering_wound -> trigger( n );
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
        dk -> active_spells.festering_wound -> set_target( target );
        dk -> active_spells.festering_wound -> execute();

        // Don't unnecessarily call bursting sores in single target scenarios
        if ( dk -> talent.bursting_sores -> ok() &&
             dk -> active_spells.bursting_sores -> target_list().size() > 0 )
        {
          dk -> active_spells.bursting_sores -> set_target( target );
          dk -> active_spells.bursting_sores -> execute();
        }
        if ( dk -> conduits.convocation_of_the_dead.ok() )
        {
          dk -> cooldown.apocalypse -> adjust( -timespan_t::from_seconds(
            dk -> conduits.convocation_of_the_dead.value() / 10 ) );
        }
      }

      // Triggers once per target per player action:
      // Apocalypse is 10% * n wounds burst to proc
      // Scourge strike aoe is 1 - ( 0.9 ) ^ n targets to proc, or 10% for each target hit
      if ( dk -> talent.pestilent_pustules -> ok() )
      {
        dk -> trigger_runic_corruption( dk -> procs.pp_runic_corruption, 0, dk -> talent.pestilent_pustules -> effectN( 1 ).percent() * n );
      }

      td -> debuff.festering_wound -> decrement( n_executes );
    }
  };

  if ( ! spec.festering_wound -> ok() )
  {
    return;
  }

  if ( ! get_target_data( target ) -> debuff.festering_wound -> up() )
  {
    return;
  }

  make_event<fs_burst_t>( *sim, this, target, n );
}

// Launches the repeating event for the Inexorable Assault talent
void death_knight_t::start_inexorable_assault()
{
  if ( !talent.inexorable_assault -> ok() )
  {
    return;
  }

  buffs.inexorable_assault -> trigger( buffs.inexorable_assault -> max_stack() );

  // Inexorable assault keeps ticking out of combat and when it's at max stacks
  // We solve that by picking a random point at which the buff starts ticking
  timespan_t first = timespan_t::from_millis(
    rng().range( 0, as<int>( talent.inexorable_assault -> effectN( 1 ).period().total_millis() ) ) );

  make_event( *sim, first, [ this ]() {
    buffs.inexorable_assault -> trigger();
    make_repeating_event( *sim, talent.inexorable_assault -> effectN( 1 ).period(), [ this ]() {
      buffs.inexorable_assault -> trigger();
    } );
  } );
}

// Launches the repeting event for the cold heart talent
void death_knight_t::start_cold_heart()
{
  if ( !talent.cold_heart -> ok() )
  {
    return;
  }

  buffs.cold_heart -> trigger( buffs.cold_heart -> max_stack() );

  // Cold Heart keeps ticking out of combat and when it's at max stacks
  // We solve that by picking a random point at which the buff starts ticking
  timespan_t first = timespan_t::from_millis(
    rng().range( 0, as<int>( talent.cold_heart -> effectN( 1 ).period().total_millis() ) ) );

  make_event( *sim, first, [ this ]() {
    buffs.cold_heart -> trigger();
    make_repeating_event( *sim, talent.cold_heart -> effectN( 1 ).period(), [ this ]() {
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
  // Blood
  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    if ( talent.mark_of_blood -> ok() )
    {
      active_spells.mark_of_blood_heal = new mark_of_blood_heal_t( this );
    }
  }

  // Unholy
  else if ( specialization() == DEATH_KNIGHT_UNHOLY )
  {
    if ( spec.festering_wound -> ok() )
    {
      active_spells.festering_wound = new festering_wound_t( this );
    }

    if ( talent.bursting_sores -> ok() )
    {
      active_spells.bursting_sores = new bursting_sores_t( this );
    }
  }

  if ( spec.outbreak -> ok() || talent.unholy_blight -> ok() || legendary.superstrain -> ok() )
    active_spells.virulent_eruption = new virulent_eruption_t( this );


  player_t::create_actions();
}

// death_knight_t::create_action  ===========================================

action_t* death_knight_t::create_action( util::string_view name, const std::string& options_str )
{
  // General Actions
  if ( name == "abomination_limb"         ) return new abomination_limb_t         ( this, options_str );
  if ( name == "antimagic_shell"          ) return new antimagic_shell_t          ( this, options_str );
  if ( name == "auto_attack"              ) return new auto_attack_t              ( this, options_str );
  if ( name == "chains_of_ice"            ) return new chains_of_ice_t            ( this, options_str );
  if ( name == "death_strike"             ) return new death_strike_t             ( this, options_str );
  if ( name == "deaths_due"               ) return new deaths_due_t               ( this, options_str );
  if ( name == "icebound_fortitude"       ) return new icebound_fortitude_t       ( this, options_str );
  if ( name == "mind_freeze"              ) return new mind_freeze_t              ( this, options_str );
  if ( name == "raise_dead"               ) return new raise_dead_t               ( this, options_str );
  if ( name == "sacrificial_pact"         ) return new sacrificial_pact_t         ( this, options_str );
  if ( name == "shackle_the_unworthy"     ) return new shackle_the_unworthy_t     ( this, options_str );

  // Blood Actions
  if ( name == "blood_boil"               ) return new blood_boil_t               ( this, options_str );
  if ( name == "blood_tap"                ) return new blood_tap_t                ( this, options_str );
  if ( name == "blooddrinker"             ) return new blooddrinker_t             ( this, options_str );
  if ( name == "bonestorm"                ) return new bonestorm_t                ( this, options_str );
  if ( name == "consumption"              ) return new consumption_t              ( this, options_str );
  if ( name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_t      ( this, options_str );
  if ( name == "dark_command"             ) return new dark_command_t             ( this, options_str );
  if ( name == "deaths_caress"            ) return new deaths_caress_t            ( this, options_str );
  if ( name == "heart_strike"             ) return new heart_strike_t             ( this, options_str );
  if ( name == "mark_of_blood"            ) return new mark_of_blood_t            ( this, options_str );
  if ( name == "marrowrend"               ) return new marrowrend_t               ( this, options_str );
  if ( name == "rune_tap"                 ) return new rune_tap_t                 ( this, options_str );
  if ( name == "tombstone"                ) return new tombstone_t                ( this, options_str );
  if ( name == "vampiric_blood"           ) return new vampiric_blood_t           ( this, options_str );

  // Frost Actions
  if ( name == "breath_of_sindragosa"     ) return new breath_of_sindragosa_t     ( this, options_str );
  if ( name == "empower_rune_weapon"      ) return new empower_rune_weapon_t      ( this, options_str );
  if ( name == "frost_strike"             ) return new frost_strike_t             ( this, options_str );
  if ( name == "frostscythe"              ) return new frostscythe_t              ( this, options_str );
  if ( name == "frostwyrms_fury"          ) return new frostwyrms_fury_t          ( this, options_str );
  if ( name == "glacial_advance"          ) return new glacial_advance_t          ( this, options_str );
  if ( name == "horn_of_winter"           ) return new horn_of_winter_t           ( this, options_str );
  if ( name == "howling_blast"            ) return new howling_blast_t            ( this, options_str );
  if ( name == "hypothermic_presence"     ) return new hypothermic_presence_t     ( this, options_str );
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

  // Covenant Actions
  if ( name == "swarming_mist"            ) return new swarming_mist_t            ( this, options_str );

  // Dynamic actions
  // any_dnd and dnd_any return defile if talented, or death and decay otherwise
  if ( name == "any_dnd" || name == "dnd_any" )
  {
    if ( talent.defile -> ok() )
    {
      return create_action( "defile", options_str );
    }
    else if ( covenant.deaths_due -> ok() )
    {
      return create_action( "deaths_due", options_str );
    }
    return create_action( "death_and_decay", options_str );
  }

  // wound_spender will return clawing shadows if talented, scourge strike if it's not
  if ( name == "wound_spender" )
    return create_action( talent.clawing_shadows -> ok() ? "clawing_shadows" : "scourge_strike", options_str );

  return player_t::create_action( name, options_str );
}

// death_knight_t::create_expression ========================================

// Equipped death knight runeforge expressions
std::unique_ptr<expr_t> death_knight_t::create_runeforge_expression( util::string_view name, bool warning = false )
{
  // Razorice, looks for the damage procs related to MH and OH
  if ( util::str_compare_ci( name, "razorice" ) )
    return make_fn_expr( "razorice_runforge_expression", [ this ]() {
      return runeforge.rune_of_razorice;
    } );

  // Razorice MH and OH expressions (this can matter for razorice application)
  if ( util::str_compare_ci( name, "razorice_mh" ) )
    return make_fn_expr( "razorice_mh_runforge_expression", [ this ]() {
      return active_spells.razorice_mh != nullptr;
    } );
  if ( util::str_compare_ci( name, "razorice_oh" ) )
    return make_fn_expr( "razorice_oh_runforge_expression", [ this ]() {
      return active_spells.razorice_oh != nullptr;
    } );

  // Fallen Crusader, looks for the unholy strength healing action
  if ( util::str_compare_ci( name, "fallen_crusader" ) )
    return make_fn_expr( "fallen_crusader_runforge_expression", [ this ]() {
      return runeforge.rune_of_the_fallen_crusader;
    } );

  // Stoneskin Gargoyle
  if ( util::str_compare_ci( name, "stoneskin_gargoyle" ) )
    return make_fn_expr( "stoneskin_gargoyle_runforge_expression", [ this ]() {
      return runeforge.rune_of_the_stoneskin_gargoyle;
    } );

  // Apocalypse
  if ( util::str_compare_ci( name, "apocalypse" ) )
    return make_fn_expr( "apocalypse_runforge_expression", [ this ]() {
      return runeforge.rune_of_apocalypse;
    } );

  // Hysteria
  if ( util::str_compare_ci( name, "hysteria" ) )
    return make_fn_expr( "hysteria_runeforge_expression", [ this ]() {
      return runeforge.rune_of_hysteria;
    } );

  // Sanguination
  if ( util::str_compare_ci( name, "sanguination" ) )
    return make_fn_expr( "sanguination_runeforge_expression", [ this ]() {
      return runeforge.rune_of_sanguination;
    } );

  // Spellwarding
  if ( util::str_compare_ci( name, "spellwarding" ) )
    return make_fn_expr( "spellwarding_runeforge_expression", [ this ]() {
      return runeforge.rune_of_spellwarding != 0;
    } );

  // Unending Thirst, effect NYI
  if ( util::str_compare_ci( name, "unending_thirst" ) )
    return make_fn_expr( "unending_thirst_runeforge_expression", [ this ]() {
      return runeforge.rune_of_unending_thirst;
    } );

  // Only throw an error with death_knight.runeforge expressions
  // runeforge.x already spits out a warning for relevant runeforges and has to send a runeforge legendary if needed
  if ( !warning )
    throw std::invalid_argument( fmt::format( "Unknown Death Knight runeforge name '{}'", name ) );
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

  // Death Knight special expressions
  if ( util::str_compare_ci( splits[ 0 ], "death_knight" ) && splits.size() > 1 )
  {
    // Returns the value of the disable_aotd option
    if ( util::str_compare_ci( splits[ 1 ], "disable_aotd" ) && splits.size() == 2 )
      return make_fn_expr( "disable_aotd_expression", [ this ]() {
        return this -> options.disable_aotd;
      } );

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
  pets.ghoul_pet = new pets::ghoul_pet_t( this );

  if ( specialization() == DEATH_KNIGHT_UNHOLY )
  {
    // Initialized even if the talent isn't picked for APL purpose
    pets.gargoyle = new pets::gargoyle_pet_t( this );

    if ( talent.all_will_serve -> ok() )
    {
      pets.risen_skulker = new pets::risen_skulker_pet_t( this );
    }

    if ( find_action( "army_of_the_dead" ) )
    {
      pets.army_ghouls.set_creation_callback(
        [] ( death_knight_t* p ) { return new pets::army_ghoul_pet_t( p, "army_ghoul" ); } );
    }

    if ( find_action( "apocalypse" ) )
    {
      pets.apoc_ghouls.set_creation_callback(
        [] ( death_knight_t* p ) { return new pets::army_ghoul_pet_t( p, "apoc_ghoul" ); } );

    }

    if ( talent.army_of_the_damned -> ok() )
    {
      pets.magus_of_the_dead.set_creation_callback(
        [] ( death_knight_t* p ) { return new pets::magus_pet_t( p ); } );
    }

    if ( legendary.reanimated_shambler.ok() )
    {
      pets.reanimated_shambler.set_creation_callback(
          [] ( death_knight_t* p ) { return new pets::reanimated_shambler_pet_t( p ); } );
    }
  }

  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    if ( find_action( "dancing_rune_weapon" ) )
    {
      pets.dancing_rune_weapon_pet = new pets::dancing_rune_weapon_pet_t( this );
    }

    if ( talent.bloodworms -> ok() )
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

  haste *= 1.0 / ( 1.0 + buffs.unholy_assault -> check_value() );

  haste *= 1.0 / ( 1.0 + buffs.empower_rune_weapon -> check_value() );

  if ( buffs.bone_shield -> up() && spec.marrowrend_2 -> ok() )
  {
    haste *= buffs.bone_shield -> value();
  }

  return haste;
}

// death_knight_t::composite_spell_haste() ==================================

double death_knight_t::composite_spell_haste() const
{
  double haste = player_t::composite_spell_haste();

  haste *= 1.0 / ( 1.0 + buffs.unholy_assault -> check_value() );

  haste *= 1.0 / ( 1.0 + buffs.empower_rune_weapon -> check_value() );

  if ( buffs.bone_shield -> up() && spec.marrowrend_2 -> ok() )
  {
    haste *= buffs.bone_shield -> value();
  }

  return haste;
}

// death_knight_t::init_rng =================================================

void death_knight_t::init_rng()
{
  player_t::init_rng();

  rppm.bloodworms = get_rppm( "bloodworms", talent.bloodworms );
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

  if ( spec.ossuary -> ok() )
    resources.base [ RESOURCE_RUNIC_POWER ] += spec.ossuary -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER );


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

  // Generic
  spec.plate_specialization = find_specialization_spell( "Plate Specialization" );
  spec.death_knight         = find_spell( 137005 );  // "Death Knight" passive
  // Veteran of the Third and Fourth War are identical, Third's data is used for the generic effect
  spec.veteran_of_the_third_war = find_class_spell( "Veteran of the Third War" );

  // Shared
  spec.antimagic_shell    = find_class_spell( "Anti-Magic Shell" );
  spec.chains_of_ice      = find_class_spell( "Chains of Ice" );
  spec.death_and_decay    = find_class_spell( "Death and Decay" );
  spec.death_and_decay_2  = find_specialization_spell( "Death and Decay", "Rank 2" );
  spec.death_coil         = find_class_spell( "Death Coil" );
  spec.death_strike       = find_class_spell( "Death Strike" );
  spec.death_strike_2     = find_specialization_spell( "Death Strike", "Rank 2" );
  spec.frost_fever        = find_specialization_spell( "Frost Fever" ); // RP generation only
  spec.icebound_fortitude = find_class_spell( "Icebound Fortitude" );
  spec.raise_dead         = find_class_spell( "Raise Dead" );
  spec.sacrificial_pact   = find_class_spell( "Sacrificial Pact" );

  // Blood
  spec.blood_death_knight       = find_specialization_spell( "Blood Death Knight" );
  spec.riposte                  = find_specialization_spell( "Riposte" );
  spec.blood_boil               = find_specialization_spell( "Blood Boil" );
  spec.blood_boil_2             = find_specialization_spell( "Blood Boil", "Rank 2" );
  spec.crimson_scourge          = find_specialization_spell( "Crimson Scourge" );
  spec.dancing_rune_weapon      = find_specialization_spell( "Dancing Rune Weapon" );
  spec.deaths_caress            = find_specialization_spell( "Death's Caress" );
  spec.heart_strike             = find_specialization_spell( "Heart Strike" );
  spec.heart_strike_2           = find_specialization_spell( "Heart Strike", "Rank 2" );
  spec.heart_strike_3           = find_specialization_spell( "Heart Strike", "Rank 3" );
  spec.marrowrend               = find_specialization_spell( "Marrowrend" );
  spec.marrowrend_2             = find_specialization_spell( "Marrowrend", "Rank 2" );
  spec.ossuary                  = find_specialization_spell( "Ossuary" );
  spec.rune_tap                 = find_specialization_spell( "Rune Tap" );
  spec.rune_tap_2               = find_specialization_spell( "Rune Tap", "Rank 2" );
  spec.vampiric_blood           = find_specialization_spell( "Vampiric Blood" );
  spec.vampiric_blood_2         = find_specialization_spell( "Vampiric Blood", "Rank 2" );
  spec.veteran_of_the_third_war_2 = find_specialization_spell( "Veteran of the Third War", "Rank 2" );

  // Frost
  spec.frost_death_knight    = find_specialization_spell( "Frost Death Knight" );
  spec.frost_strike          = find_specialization_spell( "Frost Strike" );
  spec.frost_strike_2        = find_specialization_spell( "Frost Strike", "Rank 2" );
  spec.frostwyrms_fury       = find_specialization_spell( "Frostwyrm's Fury" );
  spec.howling_blast         = find_specialization_spell( "Howling Blast" );
  spec.obliterate            = find_specialization_spell( "Obliterate" );
  spec.obliterate_2          = find_specialization_spell( "Obliterate", "Rank 2" );
  spec.might_of_the_frozen_wastes = find_specialization_spell( "Might of the Frozen Wastes" );
  spec.might_of_the_frozen_wastes_2 = find_specialization_spell( "Might of the Frozen Wastes", "Rank 2" );
  spec.rime                  = find_specialization_spell( "Rime" );
  spec.rime_2                = find_specialization_spell( "Rime", "Rank 2" );
  spec.runic_empowerment     = find_specialization_spell( "Runic Empowerment" );
  spec.killing_machine       = find_specialization_spell( "Killing Machine" );
  spec.killing_machine_2     = find_specialization_spell( "Killing Machine", "Rank 2" );
  spec.empower_rune_weapon   = find_specialization_spell( "Empower Rune Weapon" );
  spec.empower_rune_weapon_2 = find_specialization_spell( "Empower Rune Weapon", "Rank 2" );
  spec.pillar_of_frost       = find_specialization_spell( "Pillar of Frost" );
  spec.pillar_of_frost_2     = find_specialization_spell( "Pillar of Frost", "Rank 2" );
  spec.remorseless_winter    = find_specialization_spell( "Remorseless Winter" );
  spec.remorseless_winter_2  = find_specialization_spell( "Remorseless Winter", "Rank 2" );

  // Unholy
  spec.death_coil_2        = find_specialization_spell( "Death Coil", "Rank 2" );
  spec.unholy_death_knight = find_specialization_spell( "Unholy Death Knight" );
  spec.festering_strike    = find_specialization_spell( "Festering Strike" );
  spec.festering_strike_2  = find_specialization_spell( "Festering Strike", "Rank 2" );
  spec.festering_wound     = find_specialization_spell( "Festering Wound" );
  spec.runic_corruption    = find_specialization_spell( "Runic Corruption" );
  spec.sudden_doom         = find_specialization_spell( "Sudden Doom" );
  spec.army_of_the_dead    = find_specialization_spell( "Army of the Dead" );
  spec.dark_transformation = find_specialization_spell( "Dark Transformation" );
  spec.dark_transformation_2 = find_specialization_spell( "Dark Transformation", "Rank 2" );
  spec.epidemic            = find_specialization_spell( "Epidemic" );
  spec.outbreak            = find_specialization_spell( "Outbreak" );
  spec.scourge_strike      = find_specialization_spell( "Scourge Strike" );
  spec.scourge_strike_2    = find_specialization_spell( "Scourge Strike", "Rank 2" );
  spec.apocalypse          = find_specialization_spell( "Apocalypse" );
  spec.apocalypse_2        = find_specialization_spell( "Apocalypse", "Rank 2" );
  spec.apocalypse_3        = find_specialization_spell( "Apocalypse", "Rank 3" );
  spec.raise_dead_2        = find_specialization_spell( "Raise Dead", "Rank 2" );

  mastery.blood_shield = find_mastery_spell( DEATH_KNIGHT_BLOOD );
  mastery.frozen_heart = find_mastery_spell( DEATH_KNIGHT_FROST );
  mastery.dreadblade   = find_mastery_spell( DEATH_KNIGHT_UNHOLY );

  // Frost Talents
  talent.inexorable_assault   = find_talent_spell( "Inexorable Assault" );
  talent.icy_talons           = find_talent_spell( "Icy Talons" );
  talent.cold_heart           = find_talent_spell( "Cold Heart" );

  talent.runic_attenuation    = find_talent_spell( "Runic Attenuation" );
  talent.murderous_efficiency = find_talent_spell( "Murderous Efficiency" );
  talent.horn_of_winter       = find_talent_spell( "Horn of Winter" );

  talent.avalanche            = find_talent_spell( "Avalanche" );
  talent.frozen_pulse         = find_talent_spell( "Frozen Pulse" );
  talent.frostscythe          = find_talent_spell( "Frostscythe" );

  talent.gathering_storm      = find_talent_spell( "Gathering Storm" );
  talent.hypothermic_presence = find_talent_spell( "Hypothermic Presence" );
  talent.glacial_advance      = find_talent_spell( "Glacial Advance" );

  talent.icecap               = find_talent_spell( "Icecap" );
  talent.obliteration         = find_talent_spell( "Obliteration" );
  talent.breath_of_sindragosa = find_talent_spell( "Breath of Sindragosa" );

  // Unholy Talents
  talent.infected_claws     = find_talent_spell( "Infected Claws" );
  talent.all_will_serve     = find_talent_spell( "All Will Serve" );
  talent.clawing_shadows    = find_talent_spell( "Clawing Shadows" );

  talent.bursting_sores     = find_talent_spell( "Bursting Sores" );
  talent.ebon_fever         = find_talent_spell( "Ebon Fever" );
  talent.unholy_blight      = find_talent_spell( "Unholy Blight" );

  talent.pestilent_pustules = find_talent_spell( "Pestilent Pustules" );
  talent.harbinger_of_doom  = find_talent_spell( "Harbinger of Doom" );
  talent.soul_reaper        = find_talent_spell( "Soul Reaper" );

  talent.spell_eater        = find_talent_spell( "Spell Eater" );

  talent.pestilence         = find_talent_spell( "Pestilence" );
  talent.unholy_pact        = find_talent_spell( "Unholy Pact" );
  talent.defile             = find_talent_spell( "Defile" );

  talent.army_of_the_damned = find_talent_spell( "Army of the Damned" );
  talent.summon_gargoyle    = find_talent_spell( "Summon Gargoyle" );
  talent.unholy_assault     = find_talent_spell( "Unholy Assault" );

  // Blood Talents
  talent.heartbreaker           = find_talent_spell( "Heartbreaker" );
  talent.blooddrinker           = find_talent_spell( "Blooddrinker" );
  talent.tombstone              = find_talent_spell( "Tombstone" );

  talent.rapid_decomposition    = find_talent_spell( "Rapid Decomposition" );
  talent.hemostasis             = find_talent_spell( "Hemostasis" );
  talent.consumption            = find_talent_spell( "Consumption" );

  talent.foul_bulwark           = find_talent_spell( "Foul Bulwark" );
  talent.relish_in_blood        = find_talent_spell( "Relish in Blood" );
  talent.blood_tap              = find_talent_spell( "Blood Tap" );

  talent.will_of_the_necropolis = find_talent_spell( "Will of the Necropolis" ); // NYI
  talent.antimagic_barrier      = find_talent_spell( "Anti-Magic Barrier" );
  talent.mark_of_blood          = find_talent_spell( "Mark of Blood" );

  talent.voracious              = find_talent_spell( "Voracious" );
  talent.death_pact             = find_talent_spell( "Death Pact" ); // NYI
  talent.bloodworms             = find_talent_spell( "Bloodworms" );

  talent.purgatory              = find_talent_spell( "Purgatory" ); // NYI
  talent.red_thirst             = find_talent_spell( "Red Thirst" );
  talent.bonestorm              = find_talent_spell( "Bonestorm" );

  // Generic spells
  // Shared
  spell.dnd_buff        = find_spell( 188290 );
  spell.razorice_debuff = find_spell( 51714 );
  spell.deaths_due      = find_spell( 315442 );

  // Raise Dead abilities, used for both rank 1 and rank 2
  spell.pet_ghoul_claw         = find_spell( 91776 );
  spell.pet_gnaw               = find_spell( 91800 );

  // DIseases
  spell.blood_plague    = find_spell( 55078 );
  spell.frost_fever     = find_spell( 55095 );
  spell.virulent_plague = find_spell( 191587 );

  // Blood
  spell.blood_shield        = find_spell( 77535 );
  spell.bone_shield         = find_spell( 195181 );

  // Frost
  spell.murderous_efficiency_gain = find_spell( 207062 );
  spell.rage_of_the_frozen_champion = find_spell( 341725 );

  // Unholy
  spell.bursting_sores         = find_spell( 207267 );
  spell.festering_wound_debuff = find_spell( 194310 );
  spell.festering_wound_damage = find_spell( 194311 );
  spell.runic_corruption       = find_spell( 51460 );
  spell.soul_reaper_execute    = find_spell( 343295 );
  spell.virulent_eruption      = find_spell( 191685 );

  // DT ghoul abilities
  spell.pet_sweeping_claws     = find_spell( 91778 );
  spell.pet_monstrous_blow     = find_spell( 91797 );
  // Other pets
  spell.pet_army_claw          = find_spell( 199373 );
  spell.pet_gargoyle_strike    = find_spell( 51963 );
  spell.pet_dark_empowerment   = find_spell( 211947 );
  spell.pet_skulker_shot       = find_spell( 212423 );

  // Shadowlands specific - Commented out = NYI

  // Covenants
  // Death's due damage debuff is NYI
  covenant.deaths_due = find_covenant_spell( "Death's Due" );
  // Damage debuff is not implemented yet for shackle_the_unworthy
  covenant.shackle_the_unworthy = find_covenant_spell( "Shackle the Unworthy" );
  covenant.swarming_mist = find_covenant_spell( "Swarming Mist" );

  // Conduits

  // Shared - Covenant ability conduits
  conduits.brutal_grasp = find_conduit_spell( "Brutal Grasp" );
  conduits.impenetrable_gloom = find_conduit_spell( "Impenetrable Gloom" );
  conduits.proliferation = find_conduit_spell( "Proliferation" );
  conduits.withering_ground = find_conduit_spell( "Withering Ground" );

  // Shared - other throughput
  // conduits.spirit_drain = find_conduit_spell( "Spirit Drain" );

  // Blood
  conduits.debilitating_malady = find_conduit_spell( "Debilitating Malady" );
  conduits.meat_shield = find_conduit_spell( "Meat Shield" );
  conduits.withering_plague = find_conduit_spell( "Withering Plague" );

  // Frost
  conduits.accelerated_cold      = find_conduit_spell( "Accelerated Cold" );
  conduits.everfrost             = find_conduit_spell( "Everfrost" );
  conduits.eradicating_blow      = find_conduit_spell( "Eradicating Blow" );
  conduits.unleashed_frenzy      = find_conduit_spell( "Unleashed Frenzy" );

  // Unholy
  conduits.convocation_of_the_dead = find_conduit_spell( "Convocation of the Dead" );
  conduits.embrace_death = find_conduit_spell( "Embrace Death" );
  conduits.eternal_hunger = find_conduit_spell( "Eternal Hunger" );
  conduits.lingering_plague = find_conduit_spell( "Lingering Plague" );

  // Defensive - Endurance/Finesse
  // conduits.blood_bond = find_conduit_spell( "Blood Bond" );
  // conduits.chilled_resilience = find_conduit_spell( "Chilled Resilience" );
  // conduits.fleeting_wind = find_conduit_spell( "Fleeting Wind" );
  // conduits.hardened_bones = find_conduit_spell( "Hardened Bones" );
  // conduits.insatiable_appetite = find_conduit_spell( "Insatiable Appetite" );
  // conduits.reinforced_shell = find_conduit_spell( "Reinforced Shell" );
  // conduits.unending_grip = find_conduit_spell( "Unending Grip" );

  // Legendary Items

  // Shared
  legendary.phearomones = find_runeforge_legendary( "Phearomones" );
  legendary.superstrain = find_runeforge_legendary( "Superstrain" );

  // Blood
  legendary.bryndaors_might = find_runeforge_legendary( "Bryndaor's Might" );
  legendary.crimson_rune_weapon = find_runeforge_legendary( "Crimson Rune Weapon" );

  // Frost
  legendary.absolute_zero               = find_runeforge_legendary( "Absolute Zero" );
  legendary.biting_cold                 = find_runeforge_legendary( "Biting Cold" );
  legendary.koltiras_favor              = find_runeforge_legendary( "Koltira's Favor" );
  legendary.rage_of_the_frozen_champion = find_runeforge_legendary( "Rage of the Frozen Champion" );

  // Unholy
  legendary.deadliest_coil = find_runeforge_legendary( "Deadliest Coil" );
  legendary.deaths_certainty = find_runeforge_legendary( "Death's Certainty" );
  legendary.frenzied_monstrosity = find_runeforge_legendary( "Frenzied Monstrosity" );
  legendary.reanimated_shambler = find_runeforge_legendary( "Reanimated Shambler" );

  // Defensive/Utility
  // legendary.deaths_embrace = find_runeforge_legendary( "Death's Embrace" );
  // legendary.grip_of_the_everlasting = find_runeforge_legendary( "Grip of the Everlasting" );
  legendary.gorefiends_domination = find_runeforge_legendary( "Gorefiend's Domination" );
  // legendary.vampiric_aura = find_runeforge_legendary( "Vampiric Aura" );

  // Covenants
  covenant.abomination_limb = find_covenant_spell( "Abomination Limb" );

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

  buffs.icebound_fortitude = make_buff( this, "icebound_fortitude", spec.icebound_fortitude )
        -> set_duration( spec.icebound_fortitude -> duration() )
        -> set_cooldown( 0_ms ); // Handled by the action

  buffs.unholy_strength = make_buff( this, "unholy_strength", find_spell( 53365 ) )
        -> set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
        -> set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );

  // Blood
  buffs.blood_shield = new blood_shield_buff_t( this );

  buffs.bone_shield = make_buff( this, "bone_shield", spell.bone_shield )
        -> set_default_value( 1.0 / ( 1.0 + spec.marrowrend_2 -> effectN( 1 ).percent() ) ) // Haste buff
        -> set_stack_change_callback( [ this ]( buff_t*, int old_stacks, int new_stacks )
          {
            if ( talent.foul_bulwark -> ok() ) // Change player's max health if FB is talented
            {
              double fb_health = talent.foul_bulwark -> effectN( 1 ).percent();
              double health_change = ( 1.0 + new_stacks * fb_health ) / ( 1.0 + old_stacks * fb_health );

              resources.initial_multiplier[ RESOURCE_HEALTH ] *= health_change;

              recalculate_resource_max( RESOURCE_HEALTH );
            }

            // Trigger/cancel ossuary
            // Trigger is done regardless of current stacks to feed the 'refresh' data, but requires a stack gain
            if ( spec.ossuary -> ok() && new_stacks > old_stacks &&
                 new_stacks >= spec.ossuary -> effectN( 1 ).base_value() )
              buffs.ossuary -> trigger();
            // Only expire if the buff is already up
            else if ( buffs.ossuary -> up() && new_stacks < spec.ossuary -> effectN( 1 ).base_value() )
              buffs.ossuary -> expire();

            // If the buff starts or expires, invalidate relevant caches
            if ( ( ! old_stacks && new_stacks ) || ( old_stacks && ! new_stacks ) )
            {
              invalidate_cache( CACHE_BONUS_ARMOR );
              if ( spec.marrowrend_2 -> ok() )
                invalidate_cache( CACHE_HASTE );
            }
          } )
        // The internal cd in spelldata is for stack loss, handled in bone_shield_handler
        -> set_cooldown( 0_ms );

  buffs.ossuary = make_buff( this, "ossuary", find_spell( 219788 ) )
        -> set_default_value_from_effect( 1, 0.1 );

  buffs.crimson_scourge = make_buff( this, "crimson_scourge", find_spell( 81141 ) )
    -> set_trigger_spell( spec.crimson_scourge );

  buffs.dancing_rune_weapon = new dancing_rune_weapon_buff_t( this );

  buffs.hemostasis = make_buff( this, "hemostasis", talent.hemostasis -> effectN( 1 ).trigger() )
        -> set_trigger_spell( talent.hemostasis )
        -> set_default_value_from_effect( 1 );

  buffs.rune_tap = make_buff( this, "rune_tap", spec.rune_tap )
        -> set_cooldown( 0_ms ); // Handled by the action

  buffs.tombstone = make_buff<absorb_buff_t>( this, "tombstone", talent.tombstone )
        -> set_cooldown( 0_ms ); // Handled by the action

  buffs.vampiric_blood = new vampiric_blood_buff_t( this );

  buffs.voracious = make_buff( this, "voracious", find_spell( 274009 ) )
        -> set_trigger_spell( talent.voracious )
        -> add_invalidate( CACHE_LEECH );

  // Frost
  buffs.breath_of_sindragosa = new breath_of_sindragosa_buff_t( this );

  buffs.cold_heart = make_buff( this, "cold_heart", talent.cold_heart -> effectN( 1 ).trigger() );

  buffs.empower_rune_weapon = new empower_rune_weapon_buff_t( this );

  buffs.frozen_pulse = make_buff( this, "frozen_pulse", talent.frozen_pulse );

  buffs.gathering_storm = make_buff( this, "gathering_storm", find_spell( 211805 ) )
        -> set_trigger_spell( talent.gathering_storm )
        -> set_default_value_from_effect( 1 );

  buffs.hypothermic_presence = make_buff( this, "hypothermic_presence", talent.hypothermic_presence )
        -> set_default_value_from_effect( 1 );

  buffs.icy_talons = make_buff( this, "icy_talons", talent.icy_talons -> effectN( 1 ).trigger() )
        -> add_invalidate( CACHE_ATTACK_SPEED )
        -> set_default_value_from_effect( 1 )
        -> set_cooldown( talent.icy_talons->internal_cooldown() )
        -> set_trigger_spell( talent.icy_talons );

  buffs.inexorable_assault = make_buff( this, "inexorable_assault", find_spell( 253595 ) );

  buffs.killing_machine = make_buff( this, "killing_machine", spec.killing_machine -> effectN( 1 ).trigger() )
        -> set_chance( 1.0 )
        -> set_default_value_from_effect( 1 );

  buffs.pillar_of_frost = new pillar_of_frost_buff_t( this );
  buffs.pillar_of_frost_bonus = new pillar_of_frost_bonus_buff_t( this );

  buffs.remorseless_winter = new remorseless_winter_buff_t( this );

  buffs.rime = make_buff( this, "rime", spec.rime -> effectN( 1 ).trigger() )
        -> set_trigger_spell( spec.rime )
        -> set_chance( spec.rime -> effectN( 2 ).percent() + legendary.rage_of_the_frozen_champion ->effectN( 1 ).percent() );

  // Unholy
  buffs.dark_transformation = new dark_transformation_buff_t( this );

  buffs.runic_corruption = new runic_corruption_buff_t( this );

  buffs.sudden_doom = make_buff( this, "sudden_doom", spec.sudden_doom -> effectN( 1 ).trigger() )
        -> set_rppm( RPPM_ATTACK_SPEED, get_rppm( "sudden_doom", spec.sudden_doom ) -> get_frequency(), 1.0 + talent.harbinger_of_doom -> effectN( 2 ).percent())
        -> set_trigger_spell( spec.sudden_doom )
        -> set_max_stack( spec.sudden_doom -> ok() ?
                   as<unsigned int>( spec.sudden_doom -> effectN( 1 ).trigger() -> max_stacks() + talent.harbinger_of_doom -> effectN( 1 ).base_value() ):
                   1 );

  buffs.unholy_assault = make_buff( this, "unholy_assault", talent.unholy_assault )
        -> set_default_value_from_effect( 1 )
        -> set_cooldown( 0_ms ) // Handled by the action
        -> add_invalidate( CACHE_HASTE );

  buffs.unholy_pact = new unholy_pact_buff_t( this );

  buffs.unholy_blight = new unholy_blight_buff_t( this );

  // Conduits
  buffs.eradicating_blow = make_buff( this, "eradicating_blow", find_spell( 337936 ) )
        -> set_default_value( conduits.eradicating_blow.percent() )
        -> set_trigger_spell( conduits.eradicating_blow )
        -> set_cooldown( conduits.eradicating_blow -> internal_cooldown() );

  buffs.meat_shield = make_buff( this, "meat_shield", find_spell( 338438 ) )
        -> set_default_value( conduits.meat_shield.percent() )
        -> set_trigger_spell( conduits.meat_shield )
        -> set_stack_change_callback( [ this ]( buff_t*, int old_stacks, int new_stacks )
          {
            double ms_health = conduits.meat_shield.percent();
            double health_change = ( 1.0 + new_stacks * ms_health ) / ( 1.0 + old_stacks * ms_health );

            resources.initial_multiplier[ RESOURCE_HEALTH ] *= health_change;

            recalculate_resource_max( RESOURCE_HEALTH );
          } );

  buffs.unleashed_frenzy = make_buff( this, "unleashed_frenzy", conduits.unleashed_frenzy->effectN( 1 ).trigger() )
      -> add_invalidate( CACHE_STRENGTH )
      -> set_default_value( conduits.unleashed_frenzy.percent() );

  // Legendaries

  buffs.crimson_rune_weapon = make_buff( this, "crimson_rune_weapon", find_spell( 334526 ) )
      -> set_default_value_from_effect( 1 )
      -> set_affects_regen( true )
      -> set_stack_change_callback( [ this ]( buff_t*, int, int )
           { _runes.update_coefficient(); } );
  buffs.frenzied_monstrosity = make_buff( this, "frenzied_monstrosity", find_spell ( 334896 ) )
    -> add_invalidate( CACHE_ATTACK_SPEED )
    -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Covenants
  buffs.deaths_due = make_buff( this, "deaths_due", find_spell( 324165 ) )
      -> set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
      -> set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );

  buffs.death_turf = make_buff( this, "death_turf", find_spell ( 335180) )
    -> set_default_value_from_effect( 1 )
    -> set_pct_buff_type( STAT_PCT_BUFF_HASTE );
  // According to tooltip data and ingame testing, the buff's value is halved for blood
  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    buffs.death_turf -> default_value /= 2;
  }

  // Covenants
  buffs.abomination_limb = new abomination_limb_buff_t( this );
  buffs.swarming_mist = new swarming_mist_buff_t( this );
}

// death_knight_t::init_gains ===============================================

void death_knight_t::init_gains()
{
  player_t::init_gains();

  // Shared
  gains.antimagic_shell                  = get_gain( "Antimagic Shell" );
  gains.rune                             = get_gain( "Rune Regeneration" );
  gains.start_of_combat_overflow         = get_gain( "Start of Combat Overflow" );
  gains.rune_of_hysteria                 = get_gain( "Rune of Hysteria" );

  // Blood
  gains.blood_tap                        = get_gain( "Blood Tap" );
  gains.drw_heart_strike                 = get_gain( "Rune Weapon Heart Strike" );
  gains.heartbreaker                     = get_gain( "Heartbreaker" );
  gains.tombstone                        = get_gain( "Tombstone" );

  gains.bryndaors_might                  = get_gain( "Bryndaor's Might" );

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

  // Unholy
  gains.apocalypse                       = get_gain( "Apocalypse" );
  gains.festering_wound                  = get_gain( "Festering Wound" );

  // Shadowlands / Covenants
  gains.swarming_mist                    = get_gain( "Swarming Mist" );
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

  procs.km_from_crit_aa_wasted         = get_proc( "Killing Machine wasted: Critical auto attacks" );
  procs.km_from_obliteration_fs_wasted = get_proc( "Killing Machine wasted: Frost Strike" );
  procs.km_from_obliteration_hb_wasted = get_proc( "Killing Machine wasted: Howling Blast" );
  procs.km_from_obliteration_ga_wasted = get_proc( "Killing Machine wasted: Glacial Advance" );

  procs.ready_rune            = get_proc( "Rune ready" );

  procs.rp_runic_corruption   = get_proc( "Runic Corruption from Runic Power Spent" );
  procs.pp_runic_corruption   = get_proc( "Runic Corruption from Pestilent Pustules" );
  procs.sr_runic_corruption   = get_proc( "Runic Corruption from Soul Reaper" );
  procs.al_runic_corruption   = get_proc( "Runic Corruption from Abomination Limb" );

  procs.bloodworms            = get_proc( "Bloodworms" );

  procs.fw_festering_strike = get_proc( "Festering Wound from Festering Strike" );
  procs.fw_infected_claws   = get_proc( "Festering Wound from Infected Claws" );
  procs.fw_pestilence       = get_proc( "Festering Wound from Pestilence" );
  procs.fw_unholy_assault   = get_proc( "Festering Wound from Unholy Assault" );
  procs.fw_necroblast       = get_proc( "Festering Wound from Necroblast" );

  procs.reanimated_shambler = get_proc( "Reanimated Shambler" );
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
    if ( specialization() == DEATH_KNIGHT_UNHOLY )
    {
      if ( talent.soul_reaper->ok() )
      {
        target->register_on_demise_callback( this, [this]( player_t* t ) { trigger_soul_reaper_death( t ); } );
      }

      if ( spec.festering_wound->ok() )
      {
        target->register_on_demise_callback( this, [this]( player_t* t ) { trigger_festering_wound_death( t ); } );
      }
    }

    if ( spec.outbreak -> ok() || talent.unholy_blight -> ok() || legendary.superstrain -> ok() )
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
}

// death_knight_t::assess_heal ==============================================

void death_knight_t::assess_heal( school_e school, result_amount_type t, action_state_t* s )
{
  if ( buffs.vampiric_blood -> up() )
    s -> result_total *= 1.0 + buffs.vampiric_blood -> data().effectN( 1 ).percent() + spec.vampiric_blood_2 -> effectN( 1 ).percent();

  player_t::assess_heal( school, t, s );
}

// death_knight_t::assess_damage ============================================

void death_knight_t::assess_damage( school_e school, result_amount_type type, action_state_t* s )
{
  player_t::assess_damage( school, type, s );

  if ( s-> result == RESULT_PARRY && buffs.dancing_rune_weapon -> up() )
  {
    buffs.meat_shield -> trigger();
  }
}

// death_knight_t::assess_damage_imminent ===================================

void death_knight_t::bone_shield_handler( const action_state_t* state ) const
{
  if ( ! buffs.bone_shield -> up() || ! cooldown.bone_shield_icd -> up() || state -> action -> special )
  {
    return;
  }

  sim -> print_log( "{} took a successful auto attack and lost a bone shield charge", name() );

  buffs.bone_shield -> decrement();
  cooldown.bone_shield_icd -> start( spell.bone_shield -> internal_cooldown() );
  // Blood tap spelldata is a bit weird, it's not in milliseconds like other time values, and is positive even though it reduces a cooldown
  if ( talent.blood_tap -> ok() )
  {
    cooldown.blood_tap -> adjust( -1.0 * timespan_t::from_seconds( talent.blood_tap -> effectN( 2 ).base_value() ) );
  }

  cooldown.dancing_rune_weapon -> adjust( legendary.crimson_rune_weapon -> effectN( 1 ).time_value() );
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
  }
}

// death_knight_t::do_damage ================================================

void death_knight_t::do_damage( action_state_t* state )
{
  player_t::do_damage( state );

  if ( state -> result_amount > 0 && talent.mark_of_blood -> ok() && ! state -> action -> special &&
       get_target_data( state -> action -> player ) -> debuff.mark_of_blood -> up() )
  {
    active_spells.mark_of_blood_heal -> execute();
  }

  if ( runeforge.rune_of_sanguination )
  {
    // Health threshold and internal cooldown are both handled in ready()
    if ( active_spells.rune_of_sanguination -> ready() )
      active_spells.rune_of_sanguination -> execute();
  }
}

// death_knight_t::target_mitigation ========================================

void death_knight_t::target_mitigation( school_e school, result_amount_type type, action_state_t* state )
{
  if ( buffs.rune_tap -> up() )
    state -> result_amount *= 1.0 + buffs.rune_tap -> data().effectN( 1 ).percent();

  if ( buffs.icebound_fortitude -> up() )
    state -> result_amount *= 1.0 + buffs.icebound_fortitude -> data().effectN( 3 ).percent();

  death_knight_td_t* td = get_target_data( state -> action -> player );
  if ( runeforge.rune_of_apocalypse )
    state -> result_amount *= 1.0 + td -> debuff.apocalypse_famine -> stack_value();

  if ( dbc::is_school( school, SCHOOL_MAGIC ) && runeforge.rune_of_spellwarding )
    state -> result_amount *= 1.0 + runeforge.rune_of_spellwarding;

  player_t::target_mitigation( school, type, state );
}

// death_knight_t::composite_bonus_armor =========================================

double death_knight_t::composite_bonus_armor() const
{
  double ba = player_t::composite_bonus_armor();

  if ( buffs.bone_shield -> check() )
  {
    ba += spell.bone_shield -> effectN( 1 ).percent() * cache.strength();
  }

  return ba;
}

// death_knight_t::composite_attribute_multiplier ===========================

double death_knight_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STRENGTH )
  {
    m *= 1.0 + buffs.pillar_of_frost -> value() + buffs.pillar_of_frost_bonus -> stack_value();

    m *= 1.0 + buffs.unleashed_frenzy -> stack_value();

    m *= 1.0 + buffs.unholy_pact -> value();
  }

  else if ( attr == ATTR_STAMINA )
  {
    m *= 1.0 + spec.veteran_of_the_third_war -> effectN( 1 ).percent()
      + spec.veteran_of_the_third_war_2 -> effectN( 1 ).percent()
      + spec.blood_death_knight -> effectN( 13 ).percent();
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

  if ( buffs.voracious -> up() )
  {
    m += buffs.voracious -> data().effectN( 1 ).percent();
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

  if ( buffs.dancing_rune_weapon -> up() )
    parry += buffs.dancing_rune_weapon -> value();

  return parry;
}

// death_knight_t::composite_dodge ============================================

double death_knight_t::composite_dodge() const
{
  double dodge = player_t::composite_dodge();

  if ( buffs.swarming_mist -> up() )
  {
    dodge += buffs.swarming_mist -> data().effectN( 2 ).percent();

    if ( conduits.impenetrable_gloom.ok() )
      dodge += conduits.impenetrable_gloom -> effectN( 2 ).percent();
  }

  return dodge;
}

// Player multipliers

double death_knight_t::composite_player_target_multiplier( player_t* target, school_e s ) const
{
  double m = player_t::composite_player_target_multiplier( target, s );

  death_knight_td_t* td = get_target_data( target );

  // 2020-12-11: Seems to be increasing the player's damage as well as the main ghoul, but not other pets'
  // Does not use a whitelist, affects all damage sources
  if ( runeforge.rune_of_apocalypse )
  {
    m *= 1.0 + td -> debuff.apocalypse_war -> stack_value();
  }

  return m;
}

double death_knight_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.frenzied_monstrosity->up() )
  {
    m *= 1.0 + buffs.frenzied_monstrosity->data().effectN( 2 ).percent();
  }

  return m;
}

double death_knight_t::composite_player_pet_damage_multiplier( const action_state_t* state ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( state );

  if ( mastery.dreadblade -> ok() )
  {
    m *= 1.0 + cache.mastery_value();
  }

  if ( conduits.eternal_hunger.ok() )
  {
    m *= 1.0 + conduits.eternal_hunger.percent();
  }

  m *= 1.0 + spec.blood_death_knight -> effectN( 14 ).percent();
  m *= 1.0 + spec.frost_death_knight -> effectN( 3 ).percent();
  m *= 1.0 + spec.unholy_death_knight -> effectN( 3 ).percent();

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

  if ( buffs.icy_talons -> up() )
  {
    haste *= 1.0 / ( 1.0 + buffs.icy_talons -> check_stack_value() );
  }

  if ( buffs.frenzied_monstrosity -> up() )
  {
    haste *= 1.0 / ( 1.0 + buffs.frenzied_monstrosity -> data().effectN( 1 ).percent() );
  }

  return haste;
}

// death_knight_t::composite_tank_crit ======================================

double death_knight_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  c += spec.blood_death_knight -> effectN( 8 ).percent();

  return c;
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
  if ( buffs.runic_corruption -> check() )
  {
    rps *= 1.0 + spell.runic_corruption -> effectN( 1 ).percent();
  }

  if ( buffs.crimson_rune_weapon -> check() )
  {
    rps *= 1.0 + buffs.crimson_rune_weapon -> check_value();
  }

  return rps;
}

inline double death_knight_t::rune_regen_coefficient() const
{
  auto coeff = cache.attack_haste();
  // Runic corruption doubles rune regeneration speed
  if ( buffs.runic_corruption -> check() )
  {
    coeff /= 1.0 + spell.runic_corruption -> effectN( 1 ).percent();
  }

  if ( buffs.crimson_rune_weapon -> check() )
  {
    coeff /= 1.0 + buffs.crimson_rune_weapon -> check_value();
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
    unique_gear::register_special_effect( 334836, runeforge::reanimated_shambler );
  }

  /*
  void register_hotfixes() const override
  {

  }*/

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
