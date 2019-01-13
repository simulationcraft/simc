// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// TODO:
// All : 
// Check that all those azerite traits work as expected
// Use pet spawner for army of the dead ghouls, apoc ghouls, and bloodworms
// Unholy
// - Fix Unholy Blight reporting : currently the uptime contains both the dot uptime (24.2s every 45s)
//   and the driver uptime (6s every 45s)
// - Look into Summon Gargoyle spawn delay
// Blood
// 
// Frost
//

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

using namespace unique_gear;

struct death_knight_t;
struct runes_t;
struct rune_t;

namespace pets {
  struct death_knight_pet_t;
  struct dancing_rune_weapon_pet_t;
  struct bloodworm_pet_t;
  struct dt_pet_t;
  struct gargoyle_pet_t;
}

namespace runeforge {
  // Note, razorice uses a different method of initialization than the other runeforges
  void fallen_crusader( special_effect_t& );
  void stoneskin_gargoyle( special_effect_t& );
}

// ==========================================================================
// Death Knight Runes
// ==========================================================================

enum disease_type { DISEASE_NONE = 0, DISEASE_BLOOD_PLAGUE, DISEASE_FROST_FEVER, DISEASE_VIRULENT_PLAGUE = 4 };
enum rune_state { STATE_DEPLETED, STATE_REGENERATING, STATE_FULL };

const double RUNIC_POWER_REFUND = 0.9;
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

  virtual timespan_t adjust( const timespan_t& by_time )
  {
    auto this_ = this;
    // Execute early and cancel the event, if the adjustment would trigger the event
    if ( by_time >= remains() )
    {
      execute();
      event_t::cancel( this_ );
      return timespan_t::zero();
    }

    auto new_remains = remains() - by_time;
    if ( sim().debug )
    {
      sim().out_debug.printf( "%s adjust time by %.3f, remains=%.3f new_remains=%f",
        name(), by_time.total_seconds(), remains().total_seconds(), new_remains.total_seconds() );
    }

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
    if ( sim().debug )
    {
      sim().out_debug.printf( "%s coefficient change, remains=%.3f old_coeff=%f new_coeff=%f ratio=%f new_remains=%f",
        name(), remains.total_seconds(), m_coefficient, new_coefficient, ratio, new_duration.total_seconds() );
    }

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
  T* schedule( const timespan_t& duration, bool use_coeff = true )
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
  typedef dynamic_event_t<rune_event_t> super;

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
  void adjust_regen_event( const timespan_t& adjustment );

  void reset()
  {
    regen_start = timespan_t::min();
    regenerated = timespan_t::zero();
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

    waste_start = timespan_t::zero();
    if ( iteration_waste_sum > timespan_t::zero() )
    {
      cumulative_waste.add( iteration_waste_sum.total_seconds() );
    }
    iteration_waste_sum = timespan_t::zero();
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
  void regenerate_immediate( const timespan_t& seconds );

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
    dot_t* blood_plague;
    // Frost
    dot_t* breath_of_sindragosa;
    dot_t* frost_fever;
    // Unholy
    dot_t* outbreak;
    dot_t* soul_reaper;
    dot_t* virulent_plague;
  } dot;

  struct
  {
    // Blood
    buff_t* mark_of_blood;
    // Frost
    buff_t* razorice;
    // Unholy
    buff_t* festering_wound;

    // Azerite Traits
    buff_t* deep_cuts;
  } debuff;

  death_knight_td_t( player_t* target, death_knight_t* death_knight );
};

struct death_knight_t : public player_t {
public:
  // Active
  double runic_power_decay_rate;
  double antimagic_shell_remaining_absorb;
  std::vector<ground_aoe_event_t*> dnds;
  bool deprecated_dnd_expression;

  // Counters
  int t20_2pc_frost;
  int t20_4pc_frost; // Collect RP usage
  double eternal_rune_weapon_counter;
  bool triggered_frozen_tempest;

  stats_t* antimagic_shell;

  // Buffs
  struct buffs_t {
    // Shared
    buff_t* antimagic_shell;
    buff_t* icebound_fortitude;

    // Blood
    buff_t* bone_shield;
    buff_t* crimson_scourge;
    buff_t* dancing_rune_weapon;
    buff_t* hemostasis;
    buff_t* rune_tap;
    buff_t* vampiric_blood;
    buff_t* voracious;

    buff_t* t20_blood; // T20 buff
    buff_t* t21_4p_blood; // T21 4P buff

    absorb_buff_t* blood_shield;
    buff_t* tombstone;

    // Frost
    buff_t* cold_heart;
    buff_t* empower_rune_weapon;
    buff_t* frozen_pulse;
    buff_t* gathering_storm;
    buff_t* icy_talons;
    buff_t* inexorable_assault;
    buff_t* killing_machine;
    buff_t* pillar_of_frost;
    buff_t* pillar_of_frost_bonus;
    buff_t* remorseless_winter;
    buff_t* rime;
    
    buff_t* t20_4pc_frost;

    // Unholy
    buff_t* dark_transformation;
    buff_t* runic_corruption;
    buff_t* soul_reaper;
    buff_t* sudden_doom;
    buff_t* unholy_frenzy;

    buff_t* t20_2pc_unholy;

    // Azerite Traits
    buff_t* bloody_runeblade;
    buff_t* bones_of_the_damned;
    buff_t* eternal_rune_weapon;

    buff_t* frostwhelps_indignation;
    buff_t* icy_citadel_builder;
    buff_t* icy_citadel;

    buff_t* festermight;
    buff_t* helchains;
  } buffs;

  struct runeforge_t {
    buff_t* rune_of_the_fallen_crusader;
    buff_t* rune_of_the_stoneskin_gargoyle;
  } runeforge;

  // Cooldowns
  struct cooldowns_t {
    // Shared
    // Blood
    cooldown_t* bone_shield_icd;
    cooldown_t* dancing_rune_weapon;
    cooldown_t* death_and_decay;
    cooldown_t* rune_strike;
    cooldown_t* vampiric_blood;
    // Frost
    cooldown_t* empower_rune_weapon;
    cooldown_t* frost_fever_icd;
    cooldown_t* icecap_icd;
    cooldown_t* pillar_of_frost;
    // Unholy
    cooldown_t* apocalypse;
    cooldown_t* army_of_the_dead;
    cooldown_t* dark_transformation;
  } cooldown;

  // Active Spells
  struct active_spells_t {
    // Shared
    action_t* bone_spike_graveyard;
    action_t* razorice_mh;
    action_t* razorice_oh;

    // Blood
    spell_t* blood_plague;
    action_t* mark_of_blood;

    // Frost
    action_t* cold_heart;
    action_t* freezing_death; // T21 4P
    action_t* inexorable_assault;

    // Unholy
    action_t* bursting_sores;
    action_t* festering_wound;
    action_t* last_surprise; // Azerite
  } active_spells;

  // Gains
  struct gains_t {
    // Shared
    gain_t* antimagic_shell;
    gain_t* power_refund; // RP refund on miss
    gain_t* rune; // Rune regeneration
    gain_t* start_of_combat_overflow;

    // Blood
    gain_t* drw_heart_strike;
    gain_t* heartbreaker;
    gain_t* rune_strike;
    gain_t* tombstone;

    gain_t* t19_4pc_blood;
    gain_t* bloody_runeblade;

    // Frost
    gain_t* empower_rune_weapon;
    gain_t* frost_fever;
    gain_t* horn_of_winter;
    gain_t* murderous_efficiency;
    gain_t* obliteration;
    gain_t* runic_attenuation;
    gain_t* runic_empowerment;

    gain_t* t19_4pc_frost;

    // Unholy
    gain_t* festering_wound;
    gain_t* soul_reaper;
  } gains;

  // Specialization
  struct specialization_t {
    // Generic
    const spell_data_t* death_knight;
    const spell_data_t* plate_specialization;
    const spell_data_t* blood_death_knight;
    const spell_data_t* frost_death_knight;
    const spell_data_t* unholy_death_knight;
    const spell_data_t* death_and_decay;
    const spell_data_t* icebound_fortitude;
    const spell_data_t* death_strike_2;

    // Blood
    const spell_data_t* blood_boil;
    const spell_data_t* crimson_scourge;
    const spell_data_t* dancing_rune_weapon;
    const spell_data_t* deaths_caress;
    const spell_data_t* heart_strike;
    const spell_data_t* marrowrend;
    const spell_data_t* riposte;
    const spell_data_t* vampiric_blood;
    const spell_data_t* veteran_of_the_third_war;
    
    // Frost
    const spell_data_t* empower_rune_weapon;
    const spell_data_t* frost_fever;
    const spell_data_t* frost_strike;
    const spell_data_t* howling_blast;
    const spell_data_t* killing_machine;
    const spell_data_t* obliterate;
    const spell_data_t* pillar_of_frost;
    const spell_data_t* remorseless_winter;
    const spell_data_t* rime;
    const spell_data_t* runic_empowerment;
    
    // Unholy
    const spell_data_t* apocalypse;
    const spell_data_t* army_of_the_dead;
    const spell_data_t* dark_transformation;
    const spell_data_t* death_coil;
    const spell_data_t* festering_strike;
    const spell_data_t* festering_wound;
    const spell_data_t* outbreak;
    const spell_data_t* runic_corruption;
    const spell_data_t* scourge_strike;
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
    const spell_data_t* glacial_advance;
    const spell_data_t* frostwyrms_fury;
    
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
    const spell_data_t* defile;
    const spell_data_t* epidemic;

    const spell_data_t* army_of_the_damned;
    const spell_data_t* unholy_frenzy;
    const spell_data_t* summon_gargoyle;

    // Blood
    const spell_data_t* heartbreaker;
    const spell_data_t* blooddrinker;
    const spell_data_t* rune_strike;

    const spell_data_t* rapid_decomposition;
    const spell_data_t* hemostasis;
    const spell_data_t* consumption;
    
    const spell_data_t* foul_bulwark;
    const spell_data_t* ossuary;
    const spell_data_t* tombstone;

    const spell_data_t* will_of_the_necropolis; // NYI
    const spell_data_t* antimagic_barrier;
    const spell_data_t* rune_tap;

    const spell_data_t* voracious;
    const spell_data_t* bloodworms;
    const spell_data_t* mark_of_blood;

    const spell_data_t* purgatory; // NYI
    const spell_data_t* red_thirst; 
    const spell_data_t* bonestorm;
  } talent;

  // Spells
  struct spells_t {
    // Shared
    const spell_data_t* antimagic_shell;
    const spell_data_t* chains_of_ice;
    const spell_data_t* death_strike;
    const spell_data_t* dnd_buff;
    const spell_data_t* fallen_crusader;

    // Blood
    const spell_data_t* blood_plague;
    const spell_data_t* blood_shield;
    const spell_data_t* bone_shield;

    const spell_data_t* gravewarden; // T20
    const spell_data_t* rune_master; // T21 4P

    const spell_data_t* eternal_rune_weapon; // Azerite

    // Frost
    const spell_data_t* cold_heart_damage;
    const spell_data_t* frost_fever;
    const spell_data_t* inexorable_assault_damage;
    const spell_data_t* murderous_efficiency_gain;

    const spell_data_t* freezing_death; // T21 4P

    // Unholy
    const spell_data_t* death_coil_damage;
    const spell_data_t* bursting_sores;
    const spell_data_t* festering_wound_debuff;
    const spell_data_t* festering_wound_damage;
    const spell_data_t* runic_corruption;
    const spell_data_t* soul_reaper;
    const spell_data_t* virulent_eruption;
    const spell_data_t* virulent_plague;

    const spell_data_t* master_of_ghouls; // T20 2P
    const spell_data_t* coils_of_devastation; // T21 2P

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
    real_ppm_t* freezing_death;
    real_ppm_t* runic_attenuation;
    real_ppm_t* killing_machine;
  } rppm;

  // Pets and Guardians
  struct pets_t
  {
    std::array< pets::death_knight_pet_t*, 4 > apoc_ghoul;
    std::array< pets::death_knight_pet_t*, 8 > army_ghoul;
    std::array< pets::bloodworm_pet_t*, 6 > bloodworms;
    pets::dancing_rune_weapon_pet_t* dancing_rune_weapon_pet;
    pets::gargoyle_pet_t* gargoyle;
    pets::dt_pet_t* ghoul_pet;
    pet_t* risen_skulker;

    std::array< pets::death_knight_pet_t*, 2 > magus_pet;
    // Note: magus_pet[0] is summoned by apocalypse and [1] by army of the dead
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
    proc_t* km_from_killer_frost;

    proc_t* km_from_crit_aa_wasted;
    proc_t* km_from_obliteration_fs_wasted;
    proc_t* km_from_obliteration_hb_wasted;
    proc_t* km_from_obliteration_ga_wasted;
    proc_t* km_from_killer_frost_wasted;

    proc_t* pp_runic_corruption; // from pestilent pustules
    proc_t* rp_runic_corruption; // from RP spent
    proc_t* t19_2pc_unholy; // from t19 2P

    proc_t* fw_festering_strike;
    proc_t* fw_infected_claws;
    proc_t* fw_pestilence;
    proc_t* fw_unholy_frenzy;
    proc_t* fw_t19_4p;
  } procs;

  // Azerite Traits

  struct azerite_powers_t
  {
    // Shared
    azerite_power_t bone_spike_graveyard;
    azerite_power_t runic_barrier; // TODO : interaction with talents for overall duration also shield amount
    azerite_power_t cold_hearted; // NYI

    // Blood
    azerite_power_t bloody_runeblade;
    azerite_power_t bones_of_the_damned; // TODO : make sure the effectN( 2 ) is used for the % chance to proc. Check how it interacts with DRW
    azerite_power_t deep_cuts; // TODO : does it work with DRW ? both in application and damage amp
    azerite_power_t eternal_rune_weapon;
    azerite_power_t marrowblood; // TODO : how does it interact with blood shield ?
    
    // Frost
    azerite_power_t echoing_howl; // TODO : I have no idea how that actually works ingame
    azerite_power_t frostwhelps_indignation; // TODO: Find the damaging spell's ID
    azerite_power_t frozen_tempest; // TODO : check if the damage is increased per tick or per cast

    azerite_power_t icy_citadel;
    azerite_power_t killer_frost; // TODO : check if it procs KM from both swings as well
    azerite_power_t latent_chill; // TODO : check that the wording actually means what it means

    // Unholy
    azerite_power_t cankerous_wounds; // Is it a separate roll or does it affect the 50/50 roll between 2-3 wounds ?
    azerite_power_t festermight;
    azerite_power_t harrowing_decay; // TODO : How does it refresh on multiple DC casts in a row ?
    azerite_power_t helchains; // The ingame implementation is a mess with multiple helchains buffs on the player and multiple helchains damage spells. The simc version is simplified
    azerite_power_t last_surprise; 
    azerite_power_t magus_of_the_dead; // Trait is a buggy mess, the pet can melee in the middle of casting (not implemented because not reliable) and sometimes doesn't cast at all
  }azerite;

  // Runes
  runes_t _runes;

  death_knight_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DEATH_KNIGHT, name, r ),
    runic_power_decay_rate(),
    antimagic_shell_remaining_absorb( 0.0 ),
    deprecated_dnd_expression( false ),
    t20_2pc_frost( 0 ),
    t20_4pc_frost( 0 ),
    eternal_rune_weapon_counter( 0 ),
    triggered_frozen_tempest( false ),
    antimagic_shell( nullptr ),
    buffs( buffs_t() ),
    runeforge( runeforge_t() ),
    active_spells( active_spells_t() ),
    gains( gains_t() ),
    spec( specialization_t() ),
    mastery( mastery_t() ),
    talent( talents_t() ),
    spell( spells_t() ),
    pets( pets_t() ),
    procs( procs_t() ),
    _runes( this )
  {
    range::fill( pets.army_ghoul, nullptr );
    range::fill( pets.apoc_ghoul, nullptr );
    range::fill( pets.bloodworms, nullptr );
    
    cooldown.apocalypse          = get_cooldown( "apocalypse" );
    cooldown.army_of_the_dead    = get_cooldown( "army_of_the_dead" );
    cooldown.bone_shield_icd     = get_cooldown( "bone_shield_icd" );
    cooldown.dancing_rune_weapon = get_cooldown( "dancing_rune_weapon" );
    cooldown.dark_transformation = get_cooldown( "dark_transformation" );
    cooldown.death_and_decay     = get_cooldown( "death_and_decay" );
    cooldown.empower_rune_weapon = get_cooldown( "empower_rune_weapon" );
    cooldown.icecap_icd          = get_cooldown( "icecap" );
    cooldown.pillar_of_frost     = get_cooldown( "pillar_of_frost" );
    cooldown.rune_strike         = get_cooldown( "rune_strike" );
    cooldown.vampiric_blood      = get_cooldown( "vampiric_blood" );

    regen_type = REGEN_DYNAMIC;
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
  double    composite_base_armor_multiplier() const override;
  double    composite_armor_multiplier() const override;
  double    composite_bonus_armor() const override;
  double    composite_attack_power_multiplier() const override;
  double    composite_melee_speed() const override;
  double    composite_melee_haste() const override;
  double    composite_spell_haste() const override;
  double    composite_attribute_multiplier( attribute_e attr ) const override;
  double    matching_gear_multiplier( attribute_e attr ) const override;
  double    composite_parry_rating() const override;
  double    composite_parry() const override;
  double    composite_leech() const override;
  double    composite_melee_expertise( const weapon_t* ) const override;
  double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double    composite_player_pet_damage_multiplier( const action_state_t* /* state */ ) const override;
  double    composite_crit_avoidance() const override;
  void      combat_begin() override;
  void      reset() override;
  void      arise() override;
  void      adjust_dynamic_cooldowns() override;
  void      assess_heal( school_e, dmg_e, action_state_t* ) override;
  void      assess_damage_imminent( school_e, dmg_e, action_state_t* ) override;
  void      target_mitigation( school_e, dmg_e, action_state_t* ) override;
  void      do_damage( action_state_t* ) override;
  void      create_actions() override;
  action_t* create_action( const std::string& name, const std::string& options ) override;
  expr_t*   create_expression( const std::string& name ) override;
  void      create_pets() override;
  resource_e primary_resource() const override { return RESOURCE_RUNIC_POWER; }
  role_e    primary_role() const override;
  stat_e    convert_hybrid_stat( stat_e s ) const override;
  void      invalidate_cache( cache_e ) override;
  double    resource_loss( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  void      merge( player_t& other ) override;
  void      analyze( sim_t& sim ) override;
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;

  double    runes_per_second() const;
  double    rune_regen_coefficient() const;
  void      trigger_killing_machine( double chance, proc_t* proc, proc_t* wasted_proc );
  void      trigger_runic_empowerment( double rpcost );
  bool      trigger_runic_corruption( double rpcost, double override_chance = -1.0, proc_t* proc = nullptr );
  void      trigger_festering_wound( const action_state_t* state, unsigned n_stacks = 1, proc_t* proc = nullptr );
  void      burst_festering_wound( const action_state_t* state, unsigned n = 1 );
  void      default_apl_dps_precombat();
  void      default_apl_blood();
  void      default_apl_frost();
  void      default_apl_unholy();
  void      bone_shield_handler( const action_state_t* ) const;

  void      trigger_t20_2pc_frost( double consumed );
  void      trigger_t20_4pc_frost( double consumed );
  void      trigger_t20_4pc_unholy( double consumed );
  void      start_inexorable_assault();
  void      start_cold_heart();

  void      trigger_soul_reaper_death( player_t* );
  void      trigger_festering_wound_death( player_t* );
  void      trigger_virulent_plague_death( player_t* );

  // Actor is standing in their own Death and Decay or Defile
  bool      in_death_and_decay() const;
  expr_t*   create_death_and_decay_expression( const std::string& expr_str );

  unsigned  replenish_rune( unsigned n, gain_t* gain = nullptr );

  target_specific_t<death_knight_td_t> target_data;

  virtual death_knight_td_t* get_target_data( player_t* target ) const override
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
  if ( sim().debug )
  {
    sim().out_debug.printf( "%s regenerates a rune, start_time=%.3f, regen_time=%.3f current_coeff=%f",
      m_rune -> runes -> dk -> name(), m_rune -> regen_start.total_seconds(),
      ( sim().current_time() - m_rune -> regen_start ).total_seconds(),
      m_coefficient );
  }

  m_rune -> fill_rune();
}

inline death_knight_td_t::death_knight_td_t( player_t* target, death_knight_t* p ) :
  actor_target_data_t( target, p )
{
  dot.blood_plague         = target -> get_dot( "blood_plague",         p );
  dot.breath_of_sindragosa = target -> get_dot( "breath_of_sindragosa", p );
  dot.frost_fever          = target -> get_dot( "frost_fever",          p );
  dot.outbreak             = target -> get_dot( "outbreak",             p );
  dot.virulent_plague      = target -> get_dot( "virulent_plague",      p );
  dot.soul_reaper          = target -> get_dot( "soul_reaper",          p );

  debuff.mark_of_blood     = make_buff( *this, "mark_of_blood", p -> talent.mark_of_blood )
                           -> set_cooldown( timespan_t::zero() ); // Handled by the action
  debuff.razorice          = make_buff( *this, "razorice", p -> find_spell( 51714 ) )
                           -> set_period( timespan_t::zero() );
  debuff.festering_wound   = make_buff( *this, "festering_wound", p -> spell.festering_wound_debuff )
                           -> set_trigger_spell( p -> spec.festering_wound )
                           -> set_cooldown( timespan_t::zero() ); // Handled by trigger_festering_wound

  debuff.deep_cuts         = make_buff( *this, "deep_cuts", p -> find_spell( 272685 ) )
                           -> set_trigger_spell( p -> azerite.deep_cuts );

  // On target death triggers
  if ( p -> specialization() == DEATH_KNIGHT_UNHOLY )
  {
    if ( p -> talent.soul_reaper -> ok() )
    {
      target -> callbacks_on_demise.push_back( std::bind( &death_knight_t::trigger_soul_reaper_death, p, std::placeholders::_1 ) );
    }
    if ( p -> spec.festering_wound -> ok() )
    {
      target -> callbacks_on_demise.push_back( std::bind( &death_knight_t::trigger_festering_wound_death, p, std::placeholders::_1 ) );
    }
    if ( p -> spec.outbreak -> ok() ) 
    {
      target -> callbacks_on_demise.push_back( std::bind( &death_knight_t::trigger_virulent_plague_death, p, std::placeholders::_1 ) );
    }
  }
}

// ==========================================================================
// Local Utility Functions
// ==========================================================================
// RUNE UTILITY
// Log rune status ==========================================================

static void log_rune_status( const death_knight_t* p, bool debug = false ) {
  std::string rune_string = p -> _runes.string_representation();

  if ( ! debug )
    p -> sim -> out_log.printf( "%s runes: %s", p -> name(), rune_string.c_str() );
  else
    p -> sim -> out_debug.printf( "%s runes: %s", p -> name(), rune_string.c_str() );
}

inline runes_t::runes_t( death_knight_t* p ) : dk( p ),
  cumulative_waste( dk -> name_str + "_Iteration_Rune_Waste", false ),
  rune_waste( dk -> name_str + "_Rune_Waste", false ),
  iteration_waste_sum( timespan_t::zero() )
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
      if ( wasted_time > timespan_t::zero() )
      {
        assert( wasted_time > timespan_t::zero() );
        iteration_waste_sum += wasted_time;
        rune_waste.add( wasted_time.total_seconds() );

        if ( dk -> sim -> debug )
        {
          dk -> sim -> out_debug.printf( "%s rune waste, n_full_runes=%u rune_regened=%.3f waste_started=%.3f wasted_time=%.3f",
              dk -> name(), n_full_runes, rune -> regenerated.total_seconds(), waste_start.total_seconds(), wasted_time.total_seconds() );
        }
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
  if ( disable_waste && waste_start >= timespan_t::zero() )
  {
    if ( dk -> sim -> debug )
    {
      dk -> sim -> out_debug.printf( "%s rune waste, waste ended, n_full_runes=%u",
          dk -> name(), runes_full() );
    }

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

inline void runes_t::regenerate_immediate( const timespan_t& seconds )
{
  if ( seconds <= timespan_t::zero() )
  {
    dk -> sim -> errorf( "%s warning, regenerating runes with an invalid immediate value (%.3f)",
      dk -> name(), seconds.total_seconds() );
    return;
  }

  if ( dk -> sim -> debug )
  {
    dk -> sim -> out_debug.printf( "%s regenerating runes with an immediate value of %.3f",
        dk -> name(), seconds.total_seconds() );
    log_rune_status( dk );
  }

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
  while ( seconds_left > timespan_t::zero() )
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
    return timespan_t::zero();
  }

  if ( n_runes > MAX_RUNES )
  {
    return timespan_t::max();
  }

  // If we have the runes, no need to check anything.
  if ( dk -> resources.current[ RESOURCE_RUNE ] >= as<double>( n_runes ) )
  {
    return timespan_t::zero();
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
  if ( runes -> waste_start < timespan_t::zero() && runes -> runes_full() > MAX_REGENERATING_RUNES )
  {
    if ( runes -> dk -> sim -> debug )
    {
      runes -> dk -> sim -> out_debug.printf( "%s rune waste, waste started, n_full_runes=%u",
          runes -> dk -> name(), runes -> runes_full() );
    }
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

inline void rune_t::adjust_regen_event( const timespan_t& adjustment )
{
  if ( ! event )
  {
    return;
  }

  auto new_remains = event -> remains() + adjustment;

  // Reduce remaining rune regeneration time by adjustment seconds.
  if ( adjustment < timespan_t::zero() )
  {
    // Filled the rune through the adjustment
    if ( new_remains <= timespan_t::zero() )
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

      if ( adjustment < timespan_t::zero() )
      {
        regen_start += adjustment;
      }
    }
  }
  // Adjustment is positive, reschedule the regeneration event to the new remaining time.
  else if ( adjustment > timespan_t::zero() )
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
  bool use_auto_attack;

  death_knight_pet_t( death_knight_t* owner, const std::string& name, bool guardian = true, bool auto_attack = true ) :
    pet_t( owner -> sim, owner, name, guardian ), use_auto_attack( auto_attack )
  {
    if ( auto_attack )
    {
      main_hand_weapon.type = WEAPON_BEAST;
    }
  }

  death_knight_t* o() const
  { return debug_cast<death_knight_t*>( owner ); }

  void init_action_list() override
  {
    action_priority_list_t* def = get_action_priority_list( "default" );
    if ( use_auto_attack )
    {
      def -> add_action( "auto_attack" );
    }

    pet_t::init_action_list();
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override;

  // DK pets have their own base attack speed
  double composite_melee_speed() const override
  { return owner -> cache.attack_haste(); }

  virtual attack_t* create_auto_attack()
  { return nullptr; }
};

// ==========================================================================
// Base Death Knight Pet Action
// ==========================================================================

template <typename T_PET, typename T_ACTION>
struct pet_action_t : public T_ACTION
{
  typedef pet_action_t<T_PET, T_ACTION> super;

  pet_action_t( T_PET* pet, const std::string& name,
    const spell_data_t* spell = spell_data_t::nil(), const std::string& options = std::string() ) :
    T_ACTION( name, pet, spell )
  {
    this -> parse_options( options );

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
  typedef pet_melee_attack_t<T_PET> super;

  pet_melee_attack_t( T_PET* pet, const std::string& name,
    const spell_data_t* spell = spell_data_t::nil(), const std::string& options = std::string() ) :
    pet_action_t<T_PET, melee_attack_t>( pet, name, spell, options )
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

    if ( ! this -> background && this -> trigger_gcd == timespan_t::zero() )
    {
      this -> trigger_gcd = timespan_t::from_seconds( 1.5 );
    }
  }
};

// ==========================================================================
// Generalized Auto Attack Action
// ==========================================================================

struct auto_attack_t : public melee_attack_t
{
  auto_attack_t( death_knight_pet_t* player ) : melee_attack_t( "auto_attack", player )
  {
    assert( player -> main_hand_weapon.type != WEAPON_NONE );
    player -> main_hand_attack = player -> create_auto_attack();
    trigger_gcd = timespan_t::zero();
  }

  void execute() override
  { player -> main_hand_attack -> schedule_execute(); }

  bool ready() override
  {
    if ( player -> is_moving() ) return false;
    return ( player -> main_hand_attack -> execute_event == nullptr );
  }
};

// ==========================================================================
// Base Death Knight Pet Spell
// ==========================================================================

template <typename T_PET>
struct pet_spell_t : public pet_action_t<T_PET, spell_t>
{
  typedef pet_spell_t<T_PET> super;

  pet_spell_t( T_PET* pet, const std::string& name,
    const spell_data_t* spell = spell_data_t::nil(), const std::string& options = std::string() ) :
    pet_action_t<T_PET, spell_t>( pet, name, spell, options )
  {
    this -> tick_may_crit = true;
    this -> hasted_ticks  = false;
  }
};

// ==========================================================================
// Base Death Knight Pet Method Definitions
// ==========================================================================

action_t* death_knight_pet_t::create_action( const std::string& name,
                                       const std::string& options_str )
{
  if ( name == "auto_attack" ) return new auto_attack_t( this );

  return pet_t::create_action( name, options_str );
}

// ==========================================================================
// Specialized Death Knight Pet Actions
// ==========================================================================

// Generic Dark Transformation pet ability
template <typename T>
struct dt_melee_ability_t : public pet_melee_attack_t<T>
{
  typedef dt_melee_ability_t<T> super;

  bool usable_in_dt;
  bool triggers_infected_claws;

  dt_melee_ability_t( T* pet, const std::string& name,
      const spell_data_t* spell = spell_data_t::nil(), const std::string& options = std::string(),
      bool usable_in_dt = true ) :
    pet_melee_attack_t<T>( pet, name, spell, options ),
    usable_in_dt( usable_in_dt ),
    triggers_infected_claws( false )
  { }

  void trigger_infected_claws( const action_state_t* state ) const
  {
    if ( ! this -> p() -> o() -> talent.infected_claws -> ok() )
    {
      return;
    }

    if ( ! this -> rng().roll( this -> p() -> o() -> talent.infected_claws -> effectN( 1 ).percent() ) )
    {
      return;
    }

    this -> p() -> o() -> trigger_festering_wound( state, 1, this -> p() -> o() -> procs.fw_infected_claws );
  }

  void execute() override
  {
    pet_melee_attack_t<T>::execute();

    if ( triggers_infected_claws )
    {
      trigger_infected_claws( this -> execute_state );
    }
  }

  bool ready() override
  {
    bool dt_state = this -> p() -> o() -> buffs.dark_transformation -> check() > 0;

    if ( usable_in_dt != dt_state )
    {
      return false;
    }

    return pet_melee_attack_t<T>::ready();
  }
};

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
// Generic Unholy "ghoul" (Ghoul, Army ghouls)
// ==========================================================================

struct base_ghoul_pet_t : public death_knight_pet_t
{
  base_ghoul_pet_t( death_knight_t* owner, const std::string& name, bool guardian = false ) :
    death_knight_pet_t( owner, name, guardian, true )
  {
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
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
      return timespan_t::from_seconds( 0.1 );

    return std::max(
             timespan_t::from_seconds( ( 40 - energy ) / resource_regen_per_second( RESOURCE_ENERGY ) ),
             timespan_t::from_seconds( 0.1 )
           );
  }

  double resource_regen_per_second( resource_e r ) const override
  {
    double reg = pet_t::resource_regen_per_second( r );

    return reg;
  }
};

// ==========================================================================
// Unholy Dark Transformable pet
// ==========================================================================

// Dark Transformable pet auto attack
template <typename T>
struct dt_pet_auto_attack_t : public auto_attack_melee_t<T>
{
  dt_pet_auto_attack_t( T* player, const std::string& name = "main_hand" ) :
    auto_attack_melee_t<T>( player, name )
  { }
};

struct dt_pet_t : public base_ghoul_pet_t
{
  cooldown_t* gnaw_cd; // shared cd between gnaw/smash and their DT'd counterparts

  dt_pet_t( death_knight_t* owner, const std::string& name ) :
    base_ghoul_pet_t( owner, name, false )
  {
    gnaw_cd = get_cooldown( "gnaw" );
    gnaw_cd-> duration = owner -> spell.pet_gnaw -> cooldown();
  }

  attack_t* create_auto_attack() override
  { return new dt_pet_auto_attack_t< dt_pet_t >( this ); }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = base_ghoul_pet_t::composite_player_multiplier( school );

    if ( o() -> buffs.dark_transformation -> up() )
    {
      m *= 1.0 + o() -> buffs.dark_transformation -> data().effectN( 1 ).percent();
    }

    return m;
  }
};

// ==========================================================================
// Unholy basic ghoul pet
// ==========================================================================

struct ghoul_pet_t : public dt_pet_t
{
  struct claw_t : public dt_melee_ability_t<ghoul_pet_t>
  {
    claw_t( ghoul_pet_t* player, const std::string& options_str ) :
      super( player, "claw", player -> o() -> spell.pet_ghoul_claw, options_str, false )
    { triggers_infected_claws = true; }
  };

  struct sweeping_claws_t : public dt_melee_ability_t<ghoul_pet_t>
  {
    sweeping_claws_t( ghoul_pet_t* player, const std::string& options_str ) :
      super( player, "sweeping_claws", player -> o() -> spell.pet_sweeping_claws, options_str )
    {
      aoe = -1;
      triggers_infected_claws = true;
    }
  };

  struct gnaw_t : public dt_melee_ability_t<ghoul_pet_t>
  {
    gnaw_t( ghoul_pet_t* player, const std::string& options_str ) :
      super( player, "gnaw", player -> o() -> spell.pet_gnaw, options_str, false )
    {
      cooldown = player -> get_cooldown( "gnaw" );
    }
  };

  struct monstrous_blow_t : public dt_melee_ability_t<ghoul_pet_t>
  {
    monstrous_blow_t( ghoul_pet_t* player, const std::string& options_str ):
      super( player, "monstrous_blow", player -> o() -> spell.pet_monstrous_blow, options_str )
    {
      cooldown = player -> get_cooldown( "gnaw" );
    }
  };

  ghoul_pet_t( death_knight_t* owner ) : dt_pet_t( owner, "ghoul" )
  { }

  void init_base_stats() override
  {
    dt_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = .6;
  }

  void init_action_list() override
  {
    dt_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "Sweeping Claws" );
    def -> add_action( "Claw" );
    def -> add_action( "Monstrous Blow" );
    // def -> add_action( "Gnaw" ); Unused because a dps loss compared to waiting for DT and casting Monstrous Blow
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "claw"           ) return new           claw_t( this, options_str );
    if ( name == "gnaw"           ) return new           gnaw_t( this, options_str );
    if ( name == "sweeping_claws" ) return new sweeping_claws_t( this, options_str );
    if ( name == "monstrous_blow" ) return new monstrous_blow_t( this, options_str );

    return dt_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Army of the Dead Ghoul
// ==========================================================================

struct army_pet_t : public base_ghoul_pet_t
{
  pet_spell_t<army_pet_t>* last_surprise;

  struct army_claw_t : public pet_melee_attack_t<army_pet_t>
  {
    army_claw_t( army_pet_t* player, const std::string& options_str ) :
      super( player, "claw", player -> o() -> spell.pet_army_claw, options_str )
    { }
  };

  struct last_surprise_t : public pet_spell_t<army_pet_t>
  {
    last_surprise_t( army_pet_t* p ) :
      super( p, "last_surprise", p -> o() -> find_spell( 279606 ) )
    {
      aoe = -1;
      background = true;
      base_dd_min = base_dd_max = p -> o() -> azerite.last_surprise.value( 1 );
    }
  };

  army_pet_t( death_knight_t* owner, const std::string& name ) :
    base_ghoul_pet_t( owner, name, true )
  { }

  void init_base_stats() override
  {
    base_ghoul_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 0.4;
  }

  void init_action_list() override
  {
    base_ghoul_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "Claw" );
  }

  void init_spells() override
  {
    base_ghoul_pet_t::init_spells();
    
    if ( o() -> azerite.last_surprise.enabled() )
    {
      last_surprise = new last_surprise_t( this );
    }
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "claw" ) return new army_claw_t( this, options_str );

    return base_ghoul_pet_t::create_action( name, options_str );
  }

  void dismiss( bool expired ) override
  {
    if ( ! sim -> event_mgr.canceled && o() -> azerite.last_surprise.enabled() )
    {
      last_surprise -> set_target( target );
      last_surprise -> execute();
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

  struct travel_t : public action_t
  {
    bool executed;

    travel_t( player_t* player ) :
      action_t( ACTION_OTHER, "travel", player ),
      executed( false )
    {
      may_miss = false;
      dual = true;
    }

    result_e calculate_result( action_state_t* /* s */ ) const override
    { return RESULT_HIT; }

    block_result_e calculate_block_result( action_state_t* ) const override
    { return BLOCK_RESULT_UNBLOCKED; }

    void execute() override
    {
      action_t::execute();
      executed = true;
    }

    void cancel() override
    {
      action_t::cancel();
      executed = false;
    }

    // ~3 seconds seems to be the optimal initial delay
    // FIXME: Verify if behavior still exists on 5.3 PTR
    timespan_t execute_time() const override
    { return timespan_t::from_seconds( const_cast<travel_t*>( this ) -> rng().gauss( 2.9, 0.2 ) ); }

    bool ready() override
    { return ! executed; }
  };

  struct gargoyle_strike_t : public pet_spell_t<gargoyle_pet_t>
  {
    gargoyle_strike_t( gargoyle_pet_t* player, const std::string& options_str ) :
      super( player, "gargoyle_strike", player -> o() -> spell.pet_gargoyle_strike, options_str )
    { }
  };

  gargoyle_pet_t( death_knight_t* owner ) : 
    death_knight_pet_t( owner, "gargoyle", true, false ), dark_empowerment( nullptr )
  { regen_type = REGEN_DISABLED; }

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
    def -> add_action( "travel" );
    def -> add_action( "Gargoyle Strike" );
  }

  void create_buffs() override
  {
    death_knight_pet_t::create_buffs();

    dark_empowerment = make_buff( this, "dark_empowerment", o() -> spell.pet_dark_empowerment );
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "gargoyle_strike" ) return new gargoyle_strike_t( this, options_str );
    if ( name == "travel"          ) return new travel_t( this );

    return death_knight_pet_t::create_action( name, options_str );
  }

  void increase_power ( double rp_spent )
  {
    if ( is_sleeping() )
    {
      return;
    }

    double increase = rp_spent * dark_empowerment -> data().effectN( 1 ).percent() / 2.0;

    if ( ! dark_empowerment -> check() )
    {
      dark_empowerment -> trigger( 1, increase );
    }
    else
    {
      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s increasing shadow_empowerment power by %f", name(), increase );
      }
      dark_empowerment -> current_value += increase;
    }
  }
};

// ==========================================================================
// Risen Skulker (All Will Serve skelebro)
// ==========================================================================

struct risen_skulker_pet_t : public death_knight_pet_t
{
  struct skulker_shot_t : public pet_action_t<risen_skulker_pet_t, ranged_attack_t>
  {
    skulker_shot_t( risen_skulker_pet_t* player, const std::string& options_str ) :
      super( player, "skulker_shot", player -> o() -> spell.pet_skulker_shot, options_str )
    {
      weapon = &( player -> main_hand_weapon );

      // Risen Skulker deals twice the damage to its main target, and normal damage to the other targets
      base_multiplier *= 2.0;
      aoe = -1;
      base_aoe_multiplier = 0.5;
    }
  };

  risen_skulker_pet_t( death_knight_t* owner ) : death_knight_pet_t( owner, "risen_skulker", true, false )
  {
    regen_type = REGEN_DISABLED;
    main_hand_weapon.type = WEAPON_BEAST_RANGED;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.7 );
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

  action_t* create_action( const std::string& name, const std::string& options_str ) override
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
  struct drw_spell_t : public pet_spell_t<dancing_rune_weapon_pet_t>
  {
    drw_spell_t( dancing_rune_weapon_pet_t* p, const std::string& name, const spell_data_t* s ) :
      super( p, name, s )
    {
      background = true;
    }

    bool verify_actor_spec() const override
    {
      std::vector<specialization_e> spec_list;
      auto _s = p() -> o() -> specialization();

      if ( data().id() && p() -> o() -> dbc.ability_specialization( data().id(), spec_list ) &&
           range::find( spec_list, _s ) == spec_list.end() )
      {
        sim -> errorf( "Player %s attempting to execute action %s without the required spec.\n",
            player -> name(), name() );

        return false;
      }

      return true;
    }
  };

  struct drw_attack_t : public pet_melee_attack_t<dancing_rune_weapon_pet_t>
  {
    drw_attack_t( dancing_rune_weapon_pet_t* p, const std::string& name, const spell_data_t* s ) :
      super( p, name, s )
    {
      background = true;
      normalize_weapon_speed = false; // DRW weapon-based abilities use non-normalized speed
    }

    bool verify_actor_spec() const override
    {
      std::vector<specialization_e> spec_list;
      auto _s = p() -> o() -> specialization();

      if ( data().id() && p() -> o() -> dbc.ability_specialization( data().id(), spec_list ) &&
           range::find( spec_list, _s ) == spec_list.end() )
      {
        sim -> errorf( "Player %s attempting to execute action %s without the required spec.\n",
            player -> name(), name() );

        return false;
      }

      return true;
    }
  };

  struct blood_plague_t : public drw_spell_t
  {
    blood_plague_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( p, "blood_plague", p -> o() -> spell.blood_plague ) 
    {
      base_multiplier *= 1.0 + p -> o() -> spec.blood_death_knight -> effectN( 2 ).percent();

      // DRW usually behaves the same regardless of talents, but BP ticks are affected by rapid decomposition
      // https://github.com/SimCMinMax/WoW-BugTracker/issues/240
      if ( p -> o() -> bugs )
        base_tick_time *= 1.0 + p -> o() -> talent.rapid_decomposition -> effectN( 1 ).percent();
    }
  };

  struct blood_boil_t: public drw_spell_t
  {
    blood_boil_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( p, "blood_boil", p -> o() -> spec.blood_boil )
    {
      aoe = -1;
      cooldown -> duration = timespan_t::zero();
      cooldown -> charges = 0;

      base_multiplier *= 1.0 + p -> o() -> spec.blood_death_knight -> effectN( 1 ).percent();
    }

    void impact( action_state_t* s ) override
    {
      drw_spell_t::impact( s );

      if ( result_is_hit( s -> result ) )
      {
        p() -> ability.blood_plague -> set_target( s -> target );
        p() -> ability.blood_plague -> execute();
      }
    }
  };

  struct deaths_caress_t : public drw_spell_t
  {
    deaths_caress_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( p, "deaths_caress", p -> o() -> spec.deaths_caress )
    {
      base_multiplier *= 1.0 + p -> o() -> spec.blood_death_knight -> effectN( 1 ).percent();
    }

    void impact( action_state_t* s ) override
    {
      drw_spell_t::impact( s );

      if ( result_is_hit( s -> result ) )
      {
        p() -> ability.blood_plague -> set_target( s -> target );
        p() -> ability.blood_plague -> execute();
      }
    }
  };

  struct death_strike_t : public drw_attack_t
  {
    death_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_attack_t( p, "death_strike", p -> o() -> spell.death_strike )
    {
      weapon = &( p -> main_hand_weapon );
      base_multiplier *= 1.0 + p -> o() -> spec.blood_death_knight -> effectN( 1 ).percent();
      base_multiplier *= 1.0 + p -> o() -> spec.blood_death_knight -> effectN( 3 ).percent();
    }

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
    heart_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_attack_t( p, "heart_strike", p -> o() -> spec.heart_strike )
    {
      weapon = &( p -> main_hand_weapon );
      base_multiplier *= 1.0 + p -> o() -> spec.blood_death_knight -> effectN( 1 ).percent();
      aoe = 2;
    }

    int n_targets() const override
    { return p() -> o() -> in_death_and_decay() ? aoe + as<int>( p() -> o() -> spell.dnd_buff -> effectN( 3 ).base_value() ) : aoe; }
    
    void impact( action_state_t* s ) override
    {
      drw_attack_t::impact( s );
      p() -> o() -> resource_gain( RESOURCE_RUNIC_POWER, p() -> o() -> spec.heart_strike -> effectN( 3 ).base_value(), 
                                  p() -> o() -> gains.drw_heart_strike,
                                  nullptr );
    }
  };

  struct marrowrend_t : public drw_attack_t
  {
    marrowrend_t( dancing_rune_weapon_pet_t* p ) :
      drw_attack_t( p, "marrowrend", p -> o() -> spec.marrowrend )
    {
      weapon = &( p -> main_hand_weapon );
      base_multiplier *= 1.0 + p -> o() -> spec.blood_death_knight -> effectN( 1 ).percent();
    }

    void impact( action_state_t* state ) override
    {
      drw_attack_t::impact( state );

      int stack_gain = as<int>( data().effectN( 3 ).base_value() );

      p() -> o() -> buffs.bone_shield -> trigger( stack_gain );
    }
  };

  struct rune_strike_t : public drw_attack_t
  {
    rune_strike_t( dancing_rune_weapon_pet_t* p ) : 
      drw_attack_t( p, "rune_strike", p -> o() -> talent.rune_strike )
    {
      weapon = &( p -> main_hand_weapon );
      base_multiplier *= 1.0 + p -> o() -> spec.blood_death_knight -> effectN( 1 ).percent();

      cooldown -> duration = timespan_t::zero();
      cooldown -> charges = 0;
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
    drw_attack_t* rune_strike;
  } ability;

  dancing_rune_weapon_pet_t( death_knight_t* owner ) :
    death_knight_pet_t( owner, "dancing_rune_weapon", true, true )
  {
    // The pet wields the same weapon type as its owner for spells with weapon requirements
    main_hand_weapon.type       = owner -> main_hand_weapon.type;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 3.5 );

    owner_coeff.ap_from_ap = 1 / 3.0;
    regen_type = REGEN_DISABLED;

    memset( &ability, 0, sizeof( ability ) );
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
    ability.rune_strike   = new rune_strike_t  ( this );
    
    type = PLAYER_GUARDIAN; _spec = SPEC_NONE;
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
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.4 );

    owner_coeff.ap_from_ap = 0.25;
    regen_type = REGEN_DISABLED;
  }

  attack_t* create_auto_attack() override
  { return new auto_attack_melee_t<bloodworm_pet_t>( this ); }
};

// ==========================================================================
// Magus of the Dead (azerite)
// ==========================================================================

struct magus_pet_t : public death_knight_pet_t
{
  // The magus is coded weirdly: it will alternate between casting frostbolt and shadow bolt on a 2.4s interval
  // It receives an order to cast by its AI every 1.2s, much like the Risen Skulker used to
  // https://github.com/SimCMinMax/WoW-BugTracker/issues/385

  bool last_used_frostbolt;
  bool second_shadow_bolt;

  struct magus_spell_t : public pet_action_t<magus_pet_t, spell_t>
  {
    magus_pet_t* magus;
    bool is_frostbolt;

    magus_spell_t( magus_pet_t* player, const std::string& name , const spell_data_t* spell, const std::string& options_str ) :
      super( player, name, spell, options_str ),
      magus( player )
    {
      base_dd_min = base_dd_max = player -> o() -> azerite.magus_of_the_dead.value();
    }

    void execute() override
    {
      super::execute();

      magus -> last_used_frostbolt = this -> is_frostbolt;
    }

    // There's a 1 energy cost in spelldata but it might as well be ignored
    double cost() const override
    { return 0; }

    timespan_t gcd() const override
    {
      // Risen Skulker had a 2.4s between each cast start, making its cast time scaling with haste nearly irrelevant
      // This was fixed in 8.1 and seems to be back for Magus of the Dead
      // https://github.com/SimCMinMax/WoW-BugTracker/issues/385

      timespan_t interval = super::gcd();

      if ( p() -> o() -> bugs )
      {
        interval = execute_time() <= timespan_t::from_seconds( 1.2 ) ?
          timespan_t::from_seconds( 1.2 ) :
          timespan_t::from_seconds( 2.4 );
      }

      return interval;
    }

    bool ready() override
    {
      if ( magus -> last_used_frostbolt == this -> is_frostbolt )
      {
        return false;
      }
      return super::ready();
    }
  };

  struct frostbolt_magus_t : public magus_spell_t
  {
    frostbolt_magus_t( magus_pet_t* player, const std::string& options_str ) :
      magus_spell_t( player, "frostbolt", player -> o() -> find_spell( 288548 ), options_str )
    {
      is_frostbolt = true;
    }
  };

  struct shadow_bolt_magus_t : public magus_spell_t
  {
    shadow_bolt_magus_t( magus_pet_t* player, const std::string& options_str ) :
      magus_spell_t( player, "shadow_bolt", player -> o() -> find_spell( 288546 ), options_str )
    {
      is_frostbolt = false;
    }

    // Remove the execute override completely once the bug is fixed
    void execute() override
    {
      magus_spell_t::super::execute();

      // If a shadow bolt cast ends in less than 1.2s, the magus will cast it again, this isn't true for Frostbolt
      // https://github.com/SimCMinMax/WoW-BugTracker/issues/392
      if ( p() -> bugs && execute_time() <= timespan_t::from_seconds( 1.2 ) && ! magus -> second_shadow_bolt )
      {
        magus -> second_shadow_bolt = true;
      }
      else 
      {
        magus -> second_shadow_bolt = false;
        magus -> last_used_frostbolt = false;
      }
    }
  };

  magus_pet_t( death_knight_t* owner ) :
    death_knight_pet_t( owner, "magus_of_the_dead", true, false ),
    last_used_frostbolt( false ), second_shadow_bolt( false )
  {
  }

  void init_action_list() override
  {
    death_knight_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "frostbolt" );
    def -> add_action( "shadow_bolt" );
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "frostbolt" ) return new frostbolt_magus_t( this, options_str );
    if ( name == "shadow_bolt" ) return new shadow_bolt_magus_t( this, options_str );

    return death_knight_pet_t::create_action( name, options_str );
  }
};

} // namespace pets

namespace { // UNNAMED NAMESPACE

// Template for common death knight action code. See priest_action_t.
template <class Base>
struct death_knight_action_t : public Base
{
  typedef Base action_base_t;
  typedef death_knight_action_t base_t;

  gain_t* gain;

  bool hasted_gcd;
  weapon_e weapon_req;

  death_knight_action_t( const std::string& n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    action_base_t( n, p, s ), gain( nullptr ), hasted_gcd( false ), weapon_req( WEAPON_NONE )
  {
    this -> may_crit   = true;
    this -> may_glance = false;

    // Death Knights have unique snowflake mechanism for RP energize. Base actions indicate the
    // amount as a negative value resource cost in spell data, so abuse that.
    if ( this -> base_costs[ RESOURCE_RUNIC_POWER ] < 0 )
    {
      this -> energize_type = ENERGIZE_ON_CAST;
      this -> energize_resource = RESOURCE_RUNIC_POWER;

      double rp_gain = std::fabs( this -> base_costs[ RESOURCE_RUNIC_POWER ] );
      // Blood T19 2P : +10% RP gen from marrowrend/heart strike
      if ( this -> data().affected_by( p -> sets -> set ( DEATH_KNIGHT_BLOOD, T19, B2 ) -> effectN( 1 ) ) &&
        p -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T19, B2 ) )
      {
        rp_gain *= 1.0 + p -> sets -> set ( DEATH_KNIGHT_BLOOD, T19, B2 ) -> effectN ( 1 ).percent();
      }

      this -> energize_amount += rp_gain;
      this -> base_costs[ RESOURCE_RUNIC_POWER ] = 0;
    }

    // Spec Auras
    if ( this -> data().affected_by( p -> spec.unholy_death_knight -> effectN( 1 ) ) && p -> specialization() == DEATH_KNIGHT_UNHOLY )
    {
      this -> base_dd_multiplier *= 1.0 + p -> spec.unholy_death_knight -> effectN( 1 ).percent();
    }

    if ( this -> data().affected_by( p -> spec.unholy_death_knight -> effectN( 2 ) ) && p -> specialization() == DEATH_KNIGHT_UNHOLY )
    {
      this -> base_td_multiplier *= 1.0 + p -> spec.unholy_death_knight -> effectN( 2 ).percent();
    }

    if ( this -> data().affected_by( p -> spec.frost_death_knight -> effectN( 1 ) ) && p -> specialization() == DEATH_KNIGHT_FROST )
    {
      this -> base_dd_multiplier *= 1.0 + p -> spec.frost_death_knight -> effectN( 1 ).percent();
    }

    if ( this -> data().affected_by( p -> spec.frost_death_knight -> effectN( 2 ) ) && p -> specialization() == DEATH_KNIGHT_FROST )
    {
      this -> base_td_multiplier *= 1.0 + p -> spec.frost_death_knight -> effectN( 2 ).percent();
    }

    if ( this -> data().affected_by( p -> spec.blood_death_knight -> effectN( 1 ) ) && p -> specialization() == DEATH_KNIGHT_BLOOD )
    {
      this -> base_dd_multiplier *= 1.0 + p -> spec.blood_death_knight -> effectN( 1 ).percent();
    }

    if ( this -> data().affected_by( p -> spec.blood_death_knight -> effectN( 2 ) ) && p -> specialization() == DEATH_KNIGHT_BLOOD )
    {
      this -> base_td_multiplier *= 1.0 + p -> spec.blood_death_knight -> effectN( 2 ).percent();
    }
  }

  death_knight_t* p() const
  { return static_cast< death_knight_t* >( this -> player ); }

  death_knight_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual double runic_power_generation_multiplier( const action_state_t* /* s */ ) const
  {
    return 1.0;
  }

  virtual double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = Base::composite_da_multiplier( state );

    if ( ( this -> data().affected_by( p() -> mastery.frozen_heart -> effectN( 1 ) ) && p() -> mastery.frozen_heart -> ok() ) ||
         ( this -> data().affected_by( p() -> mastery.dreadblade   -> effectN( 1 ) ) && p() -> mastery.dreadblade   -> ok() ) )
    {
      m *= 1.0 + p() -> cache.mastery_value();
    }

    if ( this -> data().affected_by( p() -> buffs.t20_2pc_unholy -> data().effectN( 1 ) ) )
    {
      m *= 1.0 + p() -> buffs.t20_2pc_unholy -> stack_value();
    }

    return m;
  }

  virtual double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = Base::composite_ta_multiplier( state );

    if ( ( this -> data().affected_by( p() -> mastery.frozen_heart -> effectN( 2 ) ) && p() -> mastery.frozen_heart -> ok() ) ||
         ( this -> data().affected_by( p() -> mastery.dreadblade   -> effectN( 2 ) ) && p() -> mastery.dreadblade   -> ok() ) )
    {
      m *= 1.0 + p() -> cache.mastery_value();
    }

    if ( this -> data().affected_by( p() -> buffs.t20_2pc_unholy -> data().effectN( 2 ) ) )
    {
      m *= 1.0 + p ()-> buffs.t20_2pc_unholy -> stack_value();
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

  bool consume_cost_per_tick( const dot_t& dot ) override
  {
    auto ret = action_base_t::consume_cost_per_tick( dot );

    if ( this -> cost_per_tick( RESOURCE_RUNIC_POWER ) > 0 && this -> last_resource_cost > 0 )
    {
      p() -> buffs.icy_talons -> trigger();
      
      p() -> trigger_t20_2pc_frost( this -> last_resource_cost );

      p() -> trigger_runic_empowerment( this -> last_resource_cost );
    }

    return ret;
  }

  void consume_resource() override
  {
    action_base_t::consume_resource();

    if ( this -> base_costs[ RESOURCE_RUNE ] > 0 && p() -> talent.gathering_storm -> ok() )
    {
      // Gathering storm always benefits from the "base rune cost", even if something would adjust
      // it
      unsigned consumed = static_cast<unsigned>( this -> base_costs[ RESOURCE_RUNE ] );
      // Don't allow Relentless Winter itself to trigger Gathering Storm
      if ( p() -> buffs.remorseless_winter -> check() &&
           this -> data().id() != p() -> spec.remorseless_winter -> id() )
      {
        p() -> buffs.gathering_storm -> trigger( consumed );
        timespan_t base_extension = timespan_t::from_seconds( p() -> talent.gathering_storm -> effectN( 1 ).base_value() / 10.0 );
        p() -> buffs.remorseless_winter -> extend_duration( p(), base_extension * consumed );
      }
    }

    if ( this -> base_costs[ RESOURCE_RUNE ] > 0 && this -> last_resource_cost > 0 )
    {
      p() -> trigger_t20_4pc_frost( this -> last_resource_cost );
    }

    if ( this -> base_costs[ RESOURCE_RUNE ] > 0 && this -> last_resource_cost > 0 )
    {
      p() -> cooldown.rune_strike -> adjust( timespan_t::from_seconds( -1.0 * p() -> talent.rune_strike -> effectN( 3 ).base_value() ), false );
    }

    if ( this -> base_costs[ RESOURCE_RUNE] > 0 && this -> last_resource_cost > 0 && p() -> buffs.pillar_of_frost -> up() )
    {
      p() -> buffs.pillar_of_frost_bonus -> trigger( as<int>( this -> last_resource_cost ) );
    }

    if ( this -> base_costs[ RESOURCE_RUNE] > 0 && this -> last_resource_cost > 0 && p() -> buffs.dancing_rune_weapon -> up()
         && p() -> azerite.eternal_rune_weapon.enabled() && p() -> eternal_rune_weapon_counter <= p() -> azerite.eternal_rune_weapon.spell() -> effectN( 3 ).base_value() )
    {
      double duration_increase = p() -> azerite.eternal_rune_weapon.spell() -> effectN( 2 ).base_value() / 10 * this -> last_resource_cost;
      
      p() -> buffs.eternal_rune_weapon -> extend_duration( p(), timespan_t::from_seconds( duration_increase ) );
      p() -> buffs.dancing_rune_weapon -> extend_duration( p(), timespan_t::from_seconds( duration_increase ) );

      if ( p() -> pets.dancing_rune_weapon_pet && ! p() -> pets.dancing_rune_weapon_pet -> is_sleeping() )
      {
        timespan_t previous_expiration = p() -> pets.dancing_rune_weapon_pet -> expiration -> time;
        p() -> pets.dancing_rune_weapon_pet -> expiration -> reschedule_time = previous_expiration + timespan_t::from_seconds( duration_increase );
      }

      p() -> eternal_rune_weapon_counter += duration_increase;
    }

    if ( this -> base_costs[ RESOURCE_RUNIC_POWER ] > 0 && this -> last_resource_cost > 0 )
    {
      if ( p() -> spec.runic_empowerment -> ok() )
      {
        p() -> trigger_runic_empowerment( this -> last_resource_cost );
      }

      if ( p() -> spec.runic_corruption -> ok() )
      {
        p() -> trigger_runic_corruption( this -> last_resource_cost, -1.0, p() -> procs.rp_runic_corruption );
      }

      if ( p() -> talent.summon_gargoyle -> ok() && p() -> pets.gargoyle )
      {
        p() -> pets.gargoyle -> increase_power( this -> last_resource_cost );
      }
      
      if ( p() -> talent.red_thirst -> ok() )
      {
        timespan_t sec = timespan_t::from_seconds( p() -> talent.red_thirst -> effectN( 1 ).base_value() ) *
          this -> last_resource_cost / p() -> talent.red_thirst -> effectN( 2 ).base_value();
        p() -> cooldown.vampiric_blood -> adjust( -sec );
      }

      p() -> buffs.icy_talons -> trigger();

      p() -> trigger_t20_2pc_frost( this -> last_resource_cost );
    }
  }

  void init_finished() override
  {
    action_base_t::init_finished();

    if ( this -> base_costs[ RESOURCE_RUNE ] || this -> base_costs[ RESOURCE_RUNIC_POWER ] )
    {
      gain = this -> player -> get_gain( util::inverse_tokenize( this -> name_str ) );
    }

    if ( this -> data().affected_by( p() -> spec.death_knight -> effectN( 1 ) ) )
    {
      this -> cooldown -> hasted = true;
    }

    if ( this -> data().affected_by( p() -> spec.death_knight -> effectN( 2 ) ) )
    {
      this -> gcd_haste = HASTE_ATTACK;
    }
  }

  timespan_t gcd() const override
  {
    timespan_t base_gcd = action_base_t::gcd();
    if ( base_gcd == timespan_t::zero() )
    {
      return timespan_t::zero();
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

  expr_t* create_expression( const std::string& name_str ) override
  {
    auto dnd_expr = p() -> create_death_and_decay_expression( name_str );
    if ( dnd_expr )
    {
      return dnd_expr;
    }

    return action_base_t::create_expression( name_str );
  }
};

// ==========================================================================
// Death Knight Attack
// ==========================================================================

struct death_knight_melee_attack_t : public death_knight_action_t<melee_attack_t>
{
  death_knight_melee_attack_t( const std::string& n, death_knight_t* p,
                               const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    special    = true;
    may_crit   = true;
    may_glance = false;
  }

  void execute() override;
  void schedule_travel( action_state_t* state ) override;
  void impact( action_state_t* state ) override;
  bool ready() override;

  void consume_killing_machine( const action_state_t* state, proc_t* proc ) const;
  void trigger_icecap( const action_state_t* state ) const;
  void trigger_freezing_death( const action_state_t* state ) const;
  void trigger_razorice( const action_state_t* state ) const;
};

// ==========================================================================
// Death Knight Spell
// ==========================================================================

struct death_knight_spell_t : public death_knight_action_t<spell_t>
{
  death_knight_spell_t( const std::string& n, death_knight_t* p,
                        const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    _init_dk_spell();
  }

  void _init_dk_spell()
  {
    may_crit = true;
  }

  virtual void impact( action_state_t* state ) override;
  bool ready() override;

  void trigger_freezing_death( const action_state_t* state ) const;
};

struct death_knight_heal_t : public death_knight_action_t<heal_t>
{
  death_knight_heal_t( const std::string& n, death_knight_t* p,
                       const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  { }
};

// ==========================================================================
// Triggers
// ==========================================================================

// ==========================================================================
// Death Knight Attack Methods
// ==========================================================================

// death_knight_melee_attack_t::execute() ===================================

void death_knight_melee_attack_t::execute()
{
  base_t::execute();

  if ( hit_any_target && ! result_is_hit( execute_state -> result ) && last_resource_cost > 0 )
    p() -> resource_gain( RESOURCE_RUNIC_POWER, last_resource_cost * RUNIC_POWER_REFUND, p() -> gains.power_refund );
}

// death_knight_melee_attack_t::schedule_travel() ===========================

void death_knight_melee_attack_t::schedule_travel( action_state_t* state )
{
  base_t::schedule_travel( state );

  // Razorice has to be triggered in schedule_travel because simc core lacks support for per-target,
  // on execute callbacks (per target callbacks in core are handled on impact, i.e., after travel
  // time). Schedule_travel is guaranteed to be called per-target on execute, so we can hook into it
  // to deliver that behavior for Razorice as a special case.
  trigger_razorice( state );
}

// death_knight_melee_attack_t::impact() ====================================

void death_knight_melee_attack_t::impact( action_state_t* state )
{
  base_t::impact( state );

  trigger_freezing_death( state );
}

// death_knight_melee_attack_t::ready() =====================================

bool death_knight_melee_attack_t::ready()
{
  if ( ! base_t::ready() )
    return false;

  if ( weapon_req == WEAPON_NONE )
    return true;
  if ( weapon && weapon -> group() == weapon_req )
    return true;

  return false;
}

// death_knight_melee_attack_t::consume_killing_machine() ===================

void death_knight_melee_attack_t::consume_killing_machine( const action_state_t* state, proc_t* proc ) const
{
  if ( ! result_is_hit( state -> result ) )
  {
    return;
  }

  if ( ! p() -> buffs.killing_machine -> up() )
  {
    return;
  }

  proc -> occur();
 
  p() -> buffs.killing_machine -> decrement();

  if ( rng().roll( p() -> talent.murderous_efficiency -> effectN( 1 ).percent() ) )
  {
    p() -> replenish_rune( as<int>( p() -> spell.murderous_efficiency_gain -> effectN( 1 ).base_value() ), p() -> gains.murderous_efficiency );
  }
}

// death_knight_melee_attack_t::trigger_icecap() ============================

void death_knight_melee_attack_t::trigger_icecap( const action_state_t* state ) const
{
  if ( state -> result != RESULT_CRIT )
  {
    return;
  }

  if ( ! p() -> talent.icecap -> ok() )
  {
    return;
  }

  if ( p() -> cooldown.icecap_icd -> down() )
  {
    return;
  }

  p() -> cooldown.pillar_of_frost -> adjust( timespan_t::from_seconds(
    - p() -> talent.icecap -> effectN( 1 ).base_value() / 10.0 ) );

  p() -> cooldown.icecap_icd -> start( p() -> talent.icecap -> internal_cooldown() );
}

// death_knight_melee_attack_t::trigger_freezing_death ===================

void death_knight_melee_attack_t::trigger_freezing_death ( const action_state_t* state ) const
{
  if ( ! result_is_hit( state -> result ) )
  {
    return;
  }

  if ( school != SCHOOL_FROST )
  {
    return;
  }

  if ( ! p() -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T21, B4) )
  {
    return;
  }

  if ( ! p() -> rppm.freezing_death -> trigger() )
  {
    return;
  }
 
  // A single proc deals damage 3 times
  p() -> active_spells.freezing_death -> set_target( state -> target );
  p() -> active_spells.freezing_death -> execute();
  p() -> active_spells.freezing_death -> execute();
  p() -> active_spells.freezing_death -> execute();
}

// death_knight_melee_attack_t::trigger_razorice ===================================

void death_knight_melee_attack_t::trigger_razorice( const action_state_t* state ) const
{
  if ( state -> result_amount <= 0 || ! callbacks )
  {
    return;
  }

  // Weapon slot check, if the "melee attack" has no weapon defined, presume an implicit main hand
  // weapon
  action_t* razorice_attack = nullptr;

  if ( state -> action -> weapon )
  {
    if ( state -> action -> weapon -> slot == SLOT_MAIN_HAND && p() -> active_spells.razorice_mh )
    {
      razorice_attack = p() -> active_spells.razorice_mh;
    }
    else if ( state -> action -> weapon -> slot == SLOT_OFF_HAND && p() -> active_spells.razorice_oh )
    {
      razorice_attack = p() -> active_spells.razorice_oh;
    }
  }
  // No weapon defined in the action, presume an implicit main hand weapon, requiring that the
  // razorice runeforge is applied to the main hand weapon on the actor
  else
  {
    if ( p() -> active_spells.razorice_mh )
    {
      razorice_attack = p() -> active_spells.razorice_mh;
    }
  }

  if ( ! razorice_attack )
  {
    return;
  }

  razorice_attack -> set_target( state -> target );
  razorice_attack -> execute();

  td( state -> target ) -> debuff.razorice -> trigger();
}


// ==========================================================================
// Death Knight Spell Methods
// ==========================================================================

// death_knight_spell_t::impact() ===========================================

void death_knight_spell_t::impact( action_state_t* state )
{
  base_t::impact( state );
  trigger_freezing_death( state );
}

// death_knight_spell_t::ready() ============================================

bool death_knight_spell_t::ready()
{
  if ( ! base_t::ready() )
    return false;

  if ( weapon_req == WEAPON_NONE )
    return true;
  if ( weapon && weapon -> group() == weapon_req )
    return true;

  return false;
}

// death_knight_spell_t::trigger_freezing_death =============================

void death_knight_spell_t::trigger_freezing_death ( const action_state_t* state ) const
{
  if ( ! result_is_hit( state -> result ) )
  {
    return;
  }

  if ( school != SCHOOL_FROST )
  {
    return;
  }

  if ( ! p() -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T21, B4) )
  {
    return;
  }

  if ( ! p() -> rppm.freezing_death -> trigger() )
  {
    return;
  }

  // A single proc deals damage 3 times for some reason
  p() -> active_spells.freezing_death -> set_target( state -> target );
  p() -> active_spells.freezing_death -> execute();
  p() -> active_spells.freezing_death -> execute();
  p() -> active_spells.freezing_death -> execute();
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

    // Note, razorice always attacks with the main hand weapon, regardless of which hand it is used
    // in
    weapon = &( player -> main_hand_weapon );
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

// Freezing Death - Frost's T21 4P damage proc
struct freezing_death_t : public death_knight_melee_attack_t
{
  freezing_death_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "freezing_death", p, p -> spell.freezing_death )
  {
    background = true;
  }
};

struct inexorable_assault_damage_t : public death_knight_spell_t
{
  inexorable_assault_damage_t( death_knight_t* p ) :
    death_knight_spell_t( "inexorable_assault", p, p -> spell.inexorable_assault_damage )
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
  double bloody_runeblade_gen;

  melee_t( const char* name, death_knight_t* p, int sw ) :
    death_knight_melee_attack_t( name, p ), sync_weapons( sw ), first ( true ),
    frozen_pulse( p -> talent.frozen_pulse -> ok() ? new frozen_pulse_t( p ) : nullptr )
  {
    school            = SCHOOL_PHYSICAL;
    may_glance        = true;
    background        = true;
    repeating         = true;
    trigger_gcd       = timespan_t::zero();
    special           = false;
    weapon_multiplier = 1.0;

    bloody_runeblade_gen = p -> find_spell( 289348 ) -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER );

    // Dual wielders have a -19% chance to hit on melee attacks
    if ( p -> dual_wield() )
      base_hit -= 0.19;
  }

  void reset() override
  {
    death_knight_melee_attack_t::reset();

    first = true;
  }

  virtual double action_multiplier() const override
  {
    double am = death_knight_melee_attack_t::action_multiplier();

    // T20 2P bonus also a
    if ( p() -> buffs.t20_2pc_unholy -> up() )
    {
      am *= 1.0 + p() -> buffs.t20_2pc_unholy -> stack_value();
    }
    return am;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = death_knight_melee_attack_t::execute_time();

    if ( first )
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 ) : timespan_t::zero();
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

      if ( s -> result == RESULT_CRIT )
      {
        p() -> trigger_killing_machine( 0, p() -> procs.km_from_crit_aa,
                                           p() -> procs.km_from_crit_aa_wasted );
      }

      if ( weapon && p() -> buffs.frozen_pulse -> up() )
      {
        frozen_pulse -> set_target( s -> target );
        frozen_pulse -> schedule_execute();
      }

      if ( p() -> buffs.unholy_frenzy -> up() )
      {
        p() -> trigger_festering_wound( s, 1, p() -> procs.fw_unholy_frenzy );
      }

      // Crimson scourge doesn't proc if death and decay is ticking
      if ( td( s -> target ) -> dot.blood_plague -> is_ticking() && p() -> dnds.size() == 0 )
      {
        if ( p() -> buffs.crimson_scourge -> trigger() )
        {
          p() -> cooldown.death_and_decay -> reset( true );
          if ( p() -> azerite.bloody_runeblade.enabled() )
          {
            // TODO: check if it's always 5 RP or if it scales with number of traits
            p() -> resource_gain( RESOURCE_RUNIC_POWER, bloody_runeblade_gen, p() -> gains.bloody_runeblade );
            p() -> buffs.bloody_runeblade -> trigger();
          }
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

    for ( size_t i = 0 ; i < p() -> pets.bloodworms.size() ; i++ )
    {
      if ( ! p() -> pets.bloodworms[ i ] || p() -> pets.bloodworms[ i ] -> is_sleeping() )
      {
        p() -> pets.bloodworms[ i ] -> summon( timespan_t::from_seconds( p() -> talent.bloodworms -> effectN( 3 ).base_value() ) ); 
        return;
      }
    }
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

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    player -> main_hand_attack -> schedule_execute();
    if ( player -> off_hand_attack )
    {
      player -> off_hand_attack -> schedule_execute();
    }
  }

  virtual bool ready() override
  {
    if ( player -> is_moving() )
      return false;

    return( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// ==========================================================================
// Death Knight Abilities
// ==========================================================================

// Apocalypse ===============================================================

struct apocalypse_t : public death_knight_melee_attack_t
{
  timespan_t summon_duration;
  timespan_t magus_duration;

  apocalypse_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "apocalypse", p, p -> spec.apocalypse ),
    summon_duration( p -> find_spell( 221180 ) -> duration() ),
    magus_duration( p -> find_spell( 288544 ) -> duration() )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();
    auto n_wounds = std::min( as<int>( data().effectN( 2 ).base_value() ),
                              td( execute_state -> target ) -> debuff.festering_wound -> stack() );

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> burst_festering_wound( execute_state, n_wounds );

      for ( int i = 0; i < n_wounds; ++i )
      {
        p() -> pets.apoc_ghoul[ i ] -> summon( summon_duration );
        p() -> buffs.t20_2pc_unholy -> trigger();
      }
    }

    if ( p() -> azerite.magus_of_the_dead.enabled() )
    {
      p() -> pets.magus_pet[0] -> summon( magus_duration );
    }
  }

  virtual bool target_ready( player_t* candidate_target ) override
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
  int precombat_delay;
  timespan_t summon_duration;
  timespan_t magus_duration;

  struct summon_army_event_t : public event_t
  {
    int n_ghoul;
    timespan_t summon_interval;
    timespan_t summon_duration;
    death_knight_t* p;

    summon_army_event_t( death_knight_t* dk, int n, const timespan_t& interval, const timespan_t& duration ) :
      event_t( *dk, interval ),
      n_ghoul( n ),
      summon_interval( interval ),
      summon_duration( duration ),
      p( dk )
    { }

    void execute() override
    {
      p -> pets.army_ghoul[ n_ghoul ] -> summon( summon_duration );
      if ( ++n_ghoul < 8 )
      {
        make_event<summon_army_event_t>( sim(), p, n_ghoul, summon_interval, summon_duration );
      }
    }
  };

  army_of_the_dead_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "army_of_the_dead", p, p -> spec.army_of_the_dead ),
    precombat_delay( 6 ),
    summon_duration( p -> spec.army_of_the_dead -> effectN( 1 ).trigger() -> duration() ),
    magus_duration( p -> find_spell( 288544 ) -> duration() )
  {
    // If used during precombat, army is casted around X seconds before the fight begins
    // This is done to save rune regeneration time once the fight starts
    // Default duration is 6, and can be changed by the user

    add_option( opt_int( "delay", precombat_delay ) );
    parse_options( options_str );

    // Limit Army of the dead precast option between 10s before pull (because rune regeneration time is 10s at 0 haste) and on pull
    if ( precombat_delay > 10 )
    {
      precombat_delay = 10 ;
      sim -> out_debug.printf( "%s tried to precast army of the dead more than 10s before combat begins", p -> name() );
    }
    else if ( precombat_delay < 1 )
    { 
      precombat_delay = 1; 
      sim -> out_debug.printf( "%s tried to precast army of the dead too late (delay has to be >= 1s", p -> name() );
    }

    harmful = false;
  }

  // Army of the Dead should always cost resources
  double cost() const override
  { return base_costs[ RESOURCE_RUNE ]; }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.t20_2pc_unholy -> trigger();

    int n_ghoul = 0;

    // There's a 0.5s interval between each ghoul's spawn
    double const summon_interval = 0.5;

    timespan_t precombat_time = timespan_t::zero();

    if ( ! p() -> in_combat )
    {      
      precombat_time = timespan_t::from_seconds( precombat_delay );

      double duration_penalty = precombat_delay - ( n_ghoul + 1 ) * summon_interval;
      while ( duration_penalty >= 0 && n_ghoul < 8 )
      {
        p() -> pets.army_ghoul[ n_ghoul++ ] -> summon( summon_duration - timespan_t::from_seconds( duration_penalty ) );
        duration_penalty -= summon_interval;
      }
      
      p() -> buffs.t20_2pc_unholy -> extend_duration( p(), - precombat_time );
      p() -> cooldown.army_of_the_dead -> adjust( - precombat_time, false );

      // Simulate RP decay for X seconds
      p() -> resource_loss( RESOURCE_RUNIC_POWER, p() -> runic_power_decay_rate * precombat_delay, nullptr, nullptr );

      // Simulate rune regeneration for X seconds
      p() -> _runes.regenerate_immediate( precombat_time );

      // If every ghoul was summoned, return
      if ( n_ghoul == 8 ) return;
    }

    // If precombat didn't summon every ghoul (due to interval between each spawn)
    // Or if the cast isn't during precombat
    // Summon the rest
    make_event<summon_army_event_t>( *sim, p(), n_ghoul, timespan_t::from_seconds( summon_interval ), summon_duration );

    if ( p() -> azerite.magus_of_the_dead.enabled() )
    {
      p() -> pets.magus_pet[1] -> summon( magus_duration - precombat_time );
    }
  }

  virtual bool ready() override
  {
    if ( p() -> pets.army_ghoul[ 0 ] && ! p() -> pets.army_ghoul[ 0 ] -> is_sleeping() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Blood Boil and Blood Plague ==============================================

struct blood_plague_heal_t : public death_knight_heal_t
{
  blood_plague_heal_t( death_knight_t* p ) :
    death_knight_heal_t( "blood_plague_heal", p, p -> spell.blood_plague )
  {
    background = true;
    callbacks = may_crit = may_miss = false;
    target = p;
    attack_power_mod.direct = attack_power_mod.tick = 0;
    dot_duration = base_tick_time = timespan_t::zero();
  }
};

struct blood_plague_t : public death_knight_spell_t
{
  blood_plague_heal_t* heal;

  blood_plague_t( death_knight_t* p ) :
    death_knight_spell_t( "blood_plague", p, p -> spell.blood_plague ),
    heal( new blood_plague_heal_t( p ) )
  {
    tick_may_crit = background = true;
    may_miss = may_crit = hasted_ticks = false;

    base_tick_time *= 1.0 + p -> talent.rapid_decomposition -> effectN( 1 ).percent();
  }

  virtual double bonus_ta( const action_state_t* s ) const override
  {
    double ta = death_knight_spell_t::bonus_ta( s );

    if ( p() -> azerite.deep_cuts.enabled() &&  td( target ) -> debuff.deep_cuts -> up() )
    {
      ta += p() -> azerite.deep_cuts.value();
    }

    return ta;
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

struct blood_boil_t : public death_knight_spell_t
{
  blood_boil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blood_boil", p, p -> spec.blood_boil )
  {
    parse_options( options_str );

    aoe = -1;
    cooldown -> hasted = true;
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    
    if ( p() -> buffs.dancing_rune_weapon -> check() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.blood_boil -> set_target( execute_state -> target );
      p() -> pets.dancing_rune_weapon_pet -> ability.blood_boil -> execute();
    }
  }
  
  void impact( action_state_t* state ) override		
  {		
    death_knight_spell_t::impact( state );		
 		
    if ( result_is_hit( state -> result ) )		
    {		
      p() -> active_spells.blood_plague -> set_target( state -> target );
      p() -> active_spells.blood_plague -> execute();
    }
    
    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T20, B2 ) )
    {
      p() -> buffs.t20_blood -> trigger();
    }

    p() -> buffs.hemostasis -> trigger();
  }

};

// Blooddrinker =============================================================

struct blooddrinker_heal_t : public death_knight_heal_t
{
  blooddrinker_heal_t( death_knight_t* p ) :
    death_knight_heal_t( "blooddrinker_heal", p, p -> talent.blooddrinker )
  {
    background = true;
    callbacks = may_crit = may_miss = false;
    base_costs[ RESOURCE_RUNE ] = 0;
    energize_type = ENERGIZE_NONE;
    cooldown -> duration = timespan_t::zero();
    target = p;
    attack_power_mod.direct = attack_power_mod.tick = 0;
    dot_duration = base_tick_time = timespan_t::zero();
  }
};

// TODO: Implement the defensive stuff somehow
struct blooddrinker_t : public death_knight_spell_t
{
  blooddrinker_heal_t* heal;

  blooddrinker_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blooddrinker", p, p -> talent.blooddrinker ),
    heal( new blooddrinker_heal_t( p ) )
  {
    parse_options( options_str );
    tick_may_crit = channeled = hasted_ticks = tick_zero = true;
   
    base_tick_time = timespan_t::from_seconds( 1.0 );
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
  bonestorm_heal_t( death_knight_t* p ) :
    death_knight_heal_t( "bonestorm_heal", p, p -> find_spell( 196545 ) )
  {
    background = true;
    target = p;
  }
};

struct bonestorm_damage_t : public death_knight_spell_t
{
  bonestorm_heal_t* heal;
  int heal_count;

  bonestorm_damage_t( death_knight_t* p, bonestorm_heal_t* h ) :
    death_knight_spell_t( "bonestorm_damage", p, p -> talent.bonestorm -> effectN( 3 ).trigger() ),
    heal( h ), heal_count( 0 )
  {
    background = true;
    aoe = -1;
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
  bonestorm_heal_t* heal;

  bonestorm_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "bonestorm", p, p -> talent.bonestorm ),
    heal( new bonestorm_heal_t( p ) )
  {
    parse_options( options_str );
    hasted_ticks = false;
    tick_action = new bonestorm_damage_t( p, heal );
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  { return base_tick_time * last_resource_cost / 10; }
};

// Breath of Sindragosa =====================================================

struct breath_of_sindragosa_tick_t: public death_knight_spell_t
{
  action_t* parent;

  breath_of_sindragosa_tick_t( death_knight_t* p, action_t* parent ):
    death_knight_spell_t( "breath_of_sindragosa_tick", p, p -> talent.breath_of_sindragosa -> effectN( 1 ).trigger() ),
    parent( parent )
  {
    aoe = -1;
    background = true;

    base_aoe_multiplier = 0.3;
  }
};

struct breath_of_sindragosa_t : public death_knight_spell_t
{
  breath_of_sindragosa_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "breath_of_sindragosa", p, p -> talent.breath_of_sindragosa )
  {
    parse_options( options_str );

    may_miss = may_crit = hasted_ticks = false;
    tick_zero = dynamic_tick_action = use_off_gcd = callbacks = true;
        
    tick_action = new breath_of_sindragosa_tick_t( p, this );
    school = tick_action -> school;

    // Add the spec's aura damage modifier because it's not taken into account when applied to the ticking damage
    base_multiplier *= 1.0 + p -> spec.frost_death_knight -> effectN( 1 ).percent();

    for ( size_t idx = 1; idx <= data().power_count(); idx++ )
    {
      const spellpower_data_t& power = data().powerN( idx );
      if ( power.aura_id() == 0 || p -> dbc.spec_by_spell( power.aura_id() ) == p -> specialization() )
      {
        base_costs_per_tick[ power.resource() ] = power.cost_per_tick();
      }
    }
  }

  // Breath of Sindragosa very likely counts as direct damage, as it procs certain trinkets that are
  // flagged for "aoe harmful spell", but not "periodic".
  dmg_e amount_type( const action_state_t*, bool ) const override
  { return DMG_DIRECT; }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    return player -> sim -> expected_iteration_time * 2;
  }

  void cancel() override
  {
    death_knight_spell_t::cancel();
    if ( dot_t* d = get_dot( target ) )
      d -> cancel();
  }

  void execute() override
  {
    dot_t* d = get_dot( target );

    if ( d -> is_ticking() )
      d -> cancel();
    else
    {
      death_knight_spell_t::execute();
    }
  }

  void init() override
  {
    death_knight_spell_t::init();

    snapshot_flags |= STATE_AP | STATE_MUL_TA | STATE_TGT_MUL_TA | STATE_MUL_PERSISTENT;
    update_flags |= STATE_AP | STATE_MUL_TA | STATE_TGT_MUL_TA;
  }
};

// Chains of Ice ============================================================

// Cold Heart damage
struct cold_heart_damage_t : public death_knight_spell_t
{
  cold_heart_damage_t( death_knight_t* p ) :
    death_knight_spell_t( "cold_heart", p, p -> spell.cold_heart_damage )
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
  chains_of_ice_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "chains_of_ice", p, p -> spell.chains_of_ice )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.cold_heart -> check() > 0 )
    {
      p() -> active_spells.cold_heart -> set_target( execute_state -> target );
      p() -> active_spells.cold_heart -> execute();
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
    parse_options( options_str );
    aoe = -1;
  }
};

// Dancing Rune Weapon ======================================================

struct dancing_rune_weapon_buff_t : public buff_t
{
  dancing_rune_weapon_buff_t( death_knight_t* p ) :
    buff_t( p, "dancing_rune_weapon", p -> find_spell( 81256 ) )
  {
    set_cooldown( timespan_t::zero() );
    add_invalidate( CACHE_PARRY );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    death_knight_t* p = debug_cast< death_knight_t* >( player );

    p -> eternal_rune_weapon_counter = 0;

    // Triggers T21 4P
    if ( p -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T21, B4 ) )
    {
      p -> buffs.t21_4p_blood -> trigger();
    }
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

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.dancing_rune_weapon -> trigger();
    p() -> buffs.eternal_rune_weapon -> trigger();
    p() -> pets.dancing_rune_weapon_pet -> summon( timespan_t::from_seconds( p() -> spec.dancing_rune_weapon -> effectN( 4 ).base_value() ) );
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

  virtual void impact( action_state_t* s ) override
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    death_knight_spell_t::impact( s );
  }
};

// Dark Transformation ======================================================

struct helchains_damage_t : public death_knight_spell_t
{
  helchains_damage_t( death_knight_t* p ) :
    death_knight_spell_t( "helchains", p, p -> find_spell( 290814 ) )
  {
    background = true;

    base_dd_min = base_dd_max = p -> azerite.helchains.value( 1 );
    // TODO: the aoe is a line between the player and its pet, find a better way to translate that into # of targets hit
    // limited target threshold?
    // User inputted value?
    // 2018-12-30 hitting everything is good enough for now
    aoe = -1;
  }
};

struct helchains_buff_t : public buff_t
{
  helchains_damage_t* damage;

  helchains_buff_t( death_knight_t* p ) :
    buff_t( p, "helchains", 
            p -> azerite.helchains.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() ),
    damage( new helchains_damage_t( p ) )
  {
    tick_zero = true;
    buff_period = timespan_t::from_seconds( 1.0 );
    set_tick_behavior( buff_tick_behavior::CLIP );
    set_tick_callback( [ this ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ ) 
    {
      damage -> execute();
    } ); 
  }

  // Helchains ticks twice on buff application and buff expiration, hence the following overrides
  // https://github.com/SimCMinMax/WoW-BugTracker/issues/390
  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );
    damage -> execute();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    damage -> execute();
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

struct dark_transformation_buff_t : public buff_t
{
  dark_transformation_buff_t( death_knight_t* p ) :
    buff_t( p, "dark_transformation", p -> spec.dark_transformation )
  {
    set_cooldown( timespan_t::zero() );
  }

  void expire_override( int s, timespan_t t ) override
  {
    buff_t::expire_override( s, t );

    death_knight_t* p = debug_cast<death_knight_t*>( player );
  }
};

struct dark_transformation_t : public death_knight_spell_t
{
  dark_transformation_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "dark_transformation", p, p -> spec.dark_transformation )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.dark_transformation -> trigger();

    if ( p() -> azerite.helchains.enabled() )
    {
      p() -> buffs.helchains -> trigger();
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

// Damage spells

struct death_and_decay_damage_base_t : public death_knight_spell_t
{
  death_and_decay_damage_base_t( death_knight_spell_t* parent,
                                 const std::string& name, const spell_data_t* spell ) :
    death_knight_spell_t( name, parent -> p(), spell )
  {
    aoe              = -1;
    ground_aoe       = true;
    background       = true;
    dual             = true;
    // Combine results to parent
    stats            = parent -> stats;
  }

  void impact( action_state_t* s ) override
  {
    if ( s -> target -> debuffs.flying && s -> target -> debuffs.flying -> check() )
    {
      if ( sim -> debug )
      {
        sim -> out_debug.printf( "Ground effect %s can not hit flying target %s",
          name(), s -> target -> name() );
      }
    }
    else
    {
      death_knight_spell_t::impact( s );

      if ( p() -> talent.pestilence -> ok() && rng().roll( p() -> talent.pestilence -> effectN( 1 ).percent() ) )
      {
        p() -> trigger_festering_wound( s, 1, p() -> procs.fw_pestilence );
      }
    }
  }
};

struct death_and_decay_damage_t : public death_and_decay_damage_base_t
{
  death_and_decay_damage_t( death_knight_spell_t* parent ) :
    death_and_decay_damage_base_t( parent, "death_and_decay_damage", parent -> p() -> find_spell( 52212 ) )
  { }
};

struct defile_damage_t : public death_and_decay_damage_base_t
{
  defile_damage_t( death_knight_spell_t* parent ) :
    death_and_decay_damage_base_t( parent, "defile_damage", parent -> p() -> find_spell( 156000 ) )
  { }
};

// Bone Spike Graveyard azerite trait

struct bone_spike_graveyard_heal_t : public death_knight_heal_t
{
  bone_spike_graveyard_heal_t( death_knight_t* p ) :
    death_knight_heal_t( "bone_spike_graveyard_heal", p, p -> azerite.bone_spike_graveyard.spell() )
  {
    background = true;
    base_dd_min = base_dd_max = p -> azerite.bone_spike_graveyard.value( 2 );
    target = p;
  }
};

struct bone_spike_graveyard_t : public death_knight_spell_t
{
  bone_spike_graveyard_heal_t* bsg_heal;
  bone_spike_graveyard_t( death_knight_t* p ) :
    death_knight_spell_t( "bone_spike_graveyard", p, p -> azerite.bone_spike_graveyard.spell() ),
    bsg_heal( new bone_spike_graveyard_heal_t( p ) )
  {
    aoe = -1;
    background = true;
    base_dd_min = base_dd_max = p -> azerite.bone_spike_graveyard.value( 1 );
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    bsg_heal -> execute();
  }
};

// Main spell

struct death_and_decay_base_t : public death_knight_spell_t
{
  action_t* damage;

  death_and_decay_base_t( death_knight_t* p, const std::string& name, const spell_data_t* spell ) :
    death_knight_spell_t( name, p, spell ),
    damage( nullptr )
  {
    base_tick_time = dot_duration = timespan_t::zero(); // Handled by event
    ignore_false_positive = true;
    may_crit              = false;
    // Note, radius and ground_aoe flag needs to be set in base so spell_targets expression works
    ground_aoe            = true;
    radius                = data().effectN( 1 ).radius_max();

    // Blood has a lower base cd on DnD
    cooldown -> duration *= 1.0 + p -> spec.blood_death_knight -> effectN( 5 ).percent();
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
    death_knight_spell_t::execute();

    p() -> buffs.crimson_scourge -> decrement();

    if ( p() -> azerite.bone_spike_graveyard.enabled() )
    {
      p() -> active_spells.bone_spike_graveyard -> set_target( execute_state ->target );
      p() -> active_spells.bone_spike_graveyard -> execute();
    }

    make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
      .target( execute_state -> target )
      // Dnd is supposed to last 10s, but a total of 11 ticks (13 with rapid decomposition) are observed so we're adding a duration of 0.5s to make it work properly
      .duration( data().duration() + timespan_t::from_millis( 500 ) )
      .pulse_time( compute_tick_time() )
      .action( damage )
      // Keep track of on-going dnd events
      .state_callback( [ this ]( ground_aoe_params_t::state_type type, ground_aoe_event_t* event ) {
        switch ( type )
        {
          case ground_aoe_params_t::EVENT_CREATED:
            p() -> dnds.push_back( event );
            break;
          case ground_aoe_params_t::EVENT_DESTRUCTED:
          {
            auto it = range::find( p() -> dnds, event );
            assert( it != p() -> dnds.end() && "DnD event not found in vector" );
            if ( it != p() -> dnds.end() )
            {
              p() -> dnds.erase( it );
            }
            break;
          }
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
    damage = new death_and_decay_damage_t( this );

    parse_options( options_str );
  }

  void execute() override
  {
    death_and_decay_base_t::execute();
  }

  bool ready() override
  {
    if ( p() -> talent.defile -> ok() )
    {
      return false;
    }

    return death_and_decay_base_t::ready();
  }
};

struct defile_t : public death_and_decay_base_t
{
  defile_t( death_knight_t* p, const std::string& options_str ) :
    death_and_decay_base_t( p, "defile", p -> talent.defile )
  {
    damage = new defile_damage_t( this );

    parse_options( options_str );
  }
};

// Death's Caress ===========================================================

struct deaths_caress_t : public death_knight_spell_t
{
  deaths_caress_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "deaths_caress", p, p -> spec.deaths_caress )
  {
    parse_options( options_str );
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.deaths_caress -> set_target( execute_state -> target );
      p() -> pets.dancing_rune_weapon_pet -> ability.deaths_caress -> execute();
    }
  }

  void impact( action_state_t* state ) override		
  {		
    death_knight_spell_t::impact( state );		

    if ( result_is_hit( state -> result ) )		
    {		
      p() -> active_spells.blood_plague -> set_target( state -> target );
      p() -> active_spells.blood_plague -> execute();
    }
  }
};

// Death Coil ===============================================================

// Unholy T21 2P
struct coils_of_devastation_t 
  : public residual_action::residual_periodic_action_t<death_knight_spell_t>
{
  coils_of_devastation_t( death_knight_t* p ) :
    residual_action::residual_periodic_action_t<death_knight_spell_t>
    ( "coils_of_devastation" , p, p -> spell.coils_of_devastation )
  {
    background = true;
    may_miss = may_crit = false;
  } 
};

struct harrowing_decay_t : public death_knight_spell_t
{
  harrowing_decay_t( death_knight_t* p ) :
    death_knight_spell_t( "harrowing_decay", p, p -> azerite.harrowing_decay.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
  {
    background = true;
    base_td = p -> azerite.harrowing_decay.value( 1 );

    // TODO : assumptions only for now, need to test more
    hasted_ticks = tick_zero = false;
    tick_may_crit = true;
  }
};

// Unholy T21 4P, Procs some mechanics of Death Coil but not all of them
struct t21_death_coil_t : public death_knight_spell_t
{
  coils_of_devastation_t* coils_of_devastation;

  t21_death_coil_t( death_knight_t* p, coils_of_devastation_t* cod , const std::string& options_str ) :
    death_knight_spell_t( "death_coil T21", p, p -> spec.death_coil ),
    coils_of_devastation ( cod )
  {
    background = true;
    parse_options( options_str );

    attack_power_mod.direct = p -> spell.death_coil_damage -> effectN( 1 ).ap_coeff();

    // Wrong damage spell so generic application of damage aura and mastery doesn't work
    base_multiplier *= 1.0 + p -> spec.unholy_death_knight -> effectN( 1 ).percent();
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    // Wrong damage spell so generic application of damage aura and mastery doesn't work
    double m = death_knight_spell_t::composite_da_multiplier( state );

    m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  double cost() const override
  {
    return 0;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      // 2017-12-23 : looks like the bonus coil can also proc runic corruption
      p() -> trigger_runic_corruption( base_costs[ RESOURCE_RUNIC_POWER ], -1.0, p() -> procs.rp_runic_corruption );
    }

    // Reduces the cooldown Dark Transformation by 1s
    p() -> cooldown.dark_transformation -> adjust( - timespan_t::from_seconds(
        p() -> spec.death_coil -> effectN( 2 ).base_value() ) );

    // Reduce the cooldown on Apocalypse and Army of the Dead if Army of the Damned is talented
    p() -> cooldown.apocalypse -> adjust( -timespan_t::from_seconds( 
      p() -> talent.army_of_the_damned -> effectN( 1 ).base_value() / 10 ) );

    p() -> cooldown.army_of_the_dead -> adjust( -timespan_t::from_seconds( 
      p() -> talent.army_of_the_damned -> effectN( 2 ).base_value() / 10 ) );
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    // Can't happen ingame but if anyone wants to have fun combining T21 4P and T19 4P, might as well let them
    if ( rng().roll( player -> sets -> set( DEATH_KNIGHT_UNHOLY, T19, B4 ) -> effectN( 1 ).percent() ) )
    {
      p() -> trigger_festering_wound( state, 1, p() -> procs.fw_t19_4p );
    }

    // Coils of Devastation application
    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_UNHOLY, T21, B2 ) && result_is_hit( state -> result ) )
    {
      residual_action::trigger( coils_of_devastation, state -> target,
        state -> result_amount * p() -> sets -> set( DEATH_KNIGHT_UNHOLY, T21, B2 ) -> effectN( 1 ).percent() );
    }

    // 2017-12-23 : the bonus death coil can now proc off itself
    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_UNHOLY, T21, B4 ) && result_is_hit( state -> result ) )
    {
      if ( rng().roll( p() -> sets -> set( DEATH_KNIGHT_UNHOLY, T21, B4 ) -> proc_chance() ) ) 
      {
        this -> set_target( state -> target );
        this -> execute();
      }
    }
  }
};

struct death_coil_t : public death_knight_spell_t
{
  coils_of_devastation_t* coils_of_devastation;
  t21_death_coil_t* t21_death_coil;
  harrowing_decay_t* harrowing_decay;
  
  death_coil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "death_coil", p, p -> spec.death_coil ),
    coils_of_devastation( nullptr ),
    t21_death_coil( nullptr ),
    harrowing_decay( nullptr ) 
  {
    parse_options( options_str );

    attack_power_mod.direct = p -> spell.death_coil_damage -> effectN( 1 ).ap_coeff();
    
    // Wrong damage spell so generic application of damage aura and mastery doesn't work
    base_multiplier *= 1.0 + p -> spec.unholy_death_knight -> effectN( 1 ).percent();

    if ( p -> sets -> has_set_bonus( DEATH_KNIGHT_UNHOLY , T21, B2 ) )
    {
      coils_of_devastation = new coils_of_devastation_t( p );
      add_child( coils_of_devastation );
    }

    if ( p -> sets -> has_set_bonus( DEATH_KNIGHT_UNHOLY, T21, B4 ) )
    {
      t21_death_coil = new t21_death_coil_t( p, coils_of_devastation, options_str );
      add_child( t21_death_coil );
    }

    if ( p -> azerite.harrowing_decay.enabled() )
    {
      harrowing_decay = new harrowing_decay_t( p );
      add_child( harrowing_decay );
    }
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    // Wrong damage spell so generic application of damage aura and mastery doesn't work
    double m = death_knight_spell_t::composite_da_multiplier( state );

    m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  double cost() const override
  {
    if ( p() -> buffs.sudden_doom -> check() )
      return 0;

    return death_knight_spell_t::cost();
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    
    // Sudden Doomed Death Coils buff Gargoyle and trigger Runic Corruption
    if ( p() -> buffs.sudden_doom -> check() && p() -> pets.gargoyle )
    {
      p() -> pets.gargoyle -> increase_power( base_costs[ RESOURCE_RUNIC_POWER ] );
      p() -> trigger_runic_corruption( base_costs[ RESOURCE_RUNIC_POWER ], -1.0, p() -> procs.rp_runic_corruption  );
    }

    // Reduces the cooldown Dark Transformation by 1s

    p() -> cooldown.dark_transformation -> adjust( -timespan_t::from_seconds(
      p() -> spec.death_coil -> effectN( 2 ).base_value() ) );

    // Reduce the cooldown on Apocalypse and Army of the Dead if Army of the Damned is talented

    p() -> cooldown.apocalypse -> adjust( -timespan_t::from_seconds( 
      p() -> talent.army_of_the_damned -> effectN( 1 ).base_value() / 10 ) );

    p() -> cooldown.army_of_the_dead -> adjust( -timespan_t::from_seconds( 
      p() -> talent.army_of_the_damned -> effectN( 2 ).base_value() / 10 ) );
    
    p() -> buffs.sudden_doom -> decrement();
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );
    
    if ( rng().roll( player -> sets -> set( DEATH_KNIGHT_UNHOLY, T19, B4 ) -> effectN( 1 ).percent() ) )
    {
      p() -> trigger_festering_wound( state, 1, p() -> procs.fw_t19_4p );
    }
    
    // Coils of Devastation application
    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_UNHOLY, T21, B2 ) && result_is_hit( state -> result ) )
    {
      residual_action::trigger( coils_of_devastation, state -> target,
        state -> result_amount * p() -> sets -> set( DEATH_KNIGHT_UNHOLY, T21, B2 ) -> effectN( 1 ).percent() );
    }
    
    // T21 4P gives 20% chance to deal damage a second time
    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_UNHOLY, T21, B4 ) && result_is_hit( state -> result ) )
    {
      if ( rng().roll( p() -> sets -> set( DEATH_KNIGHT_UNHOLY, T21, B4 ) -> proc_chance() ) ) 
      {
        t21_death_coil -> set_target( state -> target );
        t21_death_coil -> execute();
      }
    }

    if ( p() -> azerite.harrowing_decay.enabled() )
    {
      harrowing_decay -> set_target( state -> target );
      harrowing_decay -> execute();
    }
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

  death_strike_heal_t( death_knight_t* p ) :
    death_knight_heal_t( "death_strike_heal", p, p -> find_spell( 45470 ) ),
    blood_shield( p -> specialization() == DEATH_KNIGHT_BLOOD ? new blood_shield_t( p ) : nullptr ),
    interval( timespan_t::from_seconds( p -> spell.death_strike -> effectN( 4 ).base_value() ) )
  {
    may_crit = callbacks = false;
    background = true;
    target     = p;
  }

  void init() override
  {
    death_knight_heal_t::init();

    snapshot_flags |= STATE_MUL_DA;
  }

  double base_da_min( const action_state_t* ) const override
  {
    return std::max( player -> resources.max[ RESOURCE_HEALTH ] * p() -> spell.death_strike -> effectN( 3 ).percent(),
                     player -> compute_incoming_damage( interval ) * p() -> spell.death_strike -> effectN( 2 ).percent() );
  }

  double base_da_max( const action_state_t* ) const override
  {
    return std::max( player -> resources.max[ RESOURCE_HEALTH ] * p() -> spell.death_strike -> effectN( 3 ).percent(),
                     player -> compute_incoming_damage( interval ) * p() -> spell.death_strike -> effectN( 2 ).percent() );
  }

  double action_multiplier() const override
  {
    double m = death_knight_heal_t::action_multiplier();

    m *= 1.0 + p() -> buffs.hemostasis -> stack_value();

    return m;
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double da = death_knight_heal_t::bonus_da( state );

    if ( p() -> azerite.marrowblood.enabled() && p() -> buffs.bone_shield -> up() )
    {
      da += p() -> azerite.marrowblood.value() * p() -> buffs.bone_shield -> stack();
    }

    return da;
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

    double amount = current_value;
    if ( p() -> mastery.blood_shield -> ok() )
      amount += state -> result_total * p() -> cache.mastery_value();

    if ( sim -> debug )
      sim -> out_debug.printf( "%s Blood Shield buff trigger, old_value=%f added_value=%f new_value=%f",
                     player -> name(), current_value,
                     state -> result_amount * p() -> cache.mastery_value(),
                     amount );

    blood_shield -> base_dd_min = blood_shield -> base_dd_max = amount;
    blood_shield -> execute();
  }
};

struct death_strike_offhand_t : public death_knight_melee_attack_t
{
  death_strike_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "death_strike_offhand", p, p -> find_spell( 66188 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
  }
};

struct death_strike_t : public death_knight_melee_attack_t
{
  death_strike_offhand_t* oh_attack;
  death_strike_heal_t* heal;
  double ossuary_cost_reduction;

  death_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "death_strike", p, p -> spell.death_strike ),
    oh_attack( nullptr ),
    heal( new death_strike_heal_t( p ) ),
    ossuary_cost_reduction( p -> find_spell( 219788 ) -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ) )
  {
    parse_options( options_str );
    may_parry = false;
    base_multiplier *= 1.0 + p -> spec.blood_death_knight -> effectN( 3 ).percent();
    
    weapon = &( p -> main_hand_weapon );

    if ( p -> dual_wield() )
    {
      oh_attack = new death_strike_offhand_t( p );
      add_child( oh_attack );
    }
  }

  double action_multiplier() const override
  {
    double m = death_knight_melee_attack_t::action_multiplier();

    m *= 1.0 + p() -> buffs.hemostasis -> stack_value();

    return m;
  }

  double cost() const override
  {
    double c = death_knight_melee_attack_t::cost();

    if ( p() -> talent.ossuary -> ok() &&
         p() -> buffs.bone_shield -> stack() >= p() -> talent.ossuary -> effectN( 1 ).base_value() )
    {
      c += ossuary_cost_reduction;
    }
    
    if ( p() -> buffs.t20_blood -> check() && p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T20, B4 ) )
    {
      c += p() -> spell.gravewarden -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER );
    }

    if ( p() -> spec.death_strike_2 -> ok() )
    {
      c += p() -> spec.death_strike_2 -> effectN( 3 ).resource( RESOURCE_RUNIC_POWER );
    }

    return c;
  }

  void execute() override
  {
    p() -> buffs.voracious -> trigger();

    death_knight_melee_attack_t::execute();

    if ( oh_attack )
      oh_attack -> execute();
    
    if ( p() -> buffs.dancing_rune_weapon -> check() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.death_strike -> set_target( execute_state -> target );
      p() -> pets.dancing_rune_weapon_pet -> ability.death_strike -> execute();
    }
    
    if ( result_is_hit( execute_state -> result ) )
    {
      heal -> execute();
      if ( rng().roll( p() -> sets -> set( DEATH_KNIGHT_BLOOD, T19, B4 ) -> effectN( 1 ).percent() ) )
      {
        p() -> replenish_rune( 1, p() -> gains.t19_4pc_blood );
      }
    }

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
    set_cooldown( timespan_t::zero() ); // Handled in the action
    set_period( p -> spec.empower_rune_weapon -> effectN( 1 ).period() );
    set_trigger_spell( p -> spec.empower_rune_weapon );
    set_default_value( p -> spec.empower_rune_weapon -> effectN( 3 ).percent() );
    add_invalidate( CACHE_HASTE );
    
    set_tick_callback( [ p ]( buff_t* b, int, const timespan_t& )
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
    // Handle energize in a custom way
    energize_type = ENERGIZE_NONE;

    // Buff handles the ticking, this one just triggers the buff
    dot_duration = base_tick_time = timespan_t::zero();
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
  epidemic_damage_main_t( death_knight_t* p, death_knight_spell_t* /* parent */ ) :
    death_knight_spell_t( "epidemic_main", p, p -> find_spell( 212739 ) )
  {
    background = true;
  }
};

struct epidemic_damage_aoe_t : public death_knight_spell_t
{
  epidemic_damage_aoe_t( death_knight_t* p ) :
    death_knight_spell_t( "epidemic_aoe", p, p -> find_spell( 215969 ) )
  {
    background = true;
    aoe = -1;
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
  epidemic_damage_main_t* main;
  epidemic_damage_aoe_t* aoe;

  epidemic_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "epidemic", p, p -> talent.epidemic ),
    main( new epidemic_damage_main_t( p, this ) ),
    aoe( new epidemic_damage_aoe_t( p ) )
  {
    parse_options( options_str );

    add_child( main );
    add_child( aoe );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    for ( const auto target : sim -> target_non_sleeping_list )
    {
      if ( td( target ) -> dot.virulent_plague -> is_ticking() )
      {
        main -> set_target( target );
        main -> execute();

        if ( sim -> target_non_sleeping_list.size() > 1 )
        {
          aoe -> set_target( target );
          aoe -> execute();
        }
      }
    }

    p() -> cooldown.apocalypse -> adjust( -timespan_t::from_seconds( 
      p() -> talent.army_of_the_damned -> effectN( 1 ).base_value() / 10 ) );

    p() -> cooldown.army_of_the_dead -> adjust( -timespan_t::from_seconds( 
      p() -> talent.army_of_the_damned -> effectN( 2 ).base_value() / 10 ) );
  }
};

// Festering Strike and Wounds ===============================================

struct bursting_sores_t : public death_knight_spell_t
{
  bursting_sores_t( death_knight_t* p ) :
    death_knight_spell_t( "bursting_sores", p, p -> spell.bursting_sores )
  {
    background = true;
    aoe = -1;
  }

  // Bursting sores have a slight delay ingame, but nothing really significant
  // The travel time is overriden to zero to make it trigger properly on enemy death
  timespan_t travel_time() const override
  {
    return timespan_t::zero();
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

    // Bug went unfixed for a while, may be intended ?
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/306
    if ( p -> bugs )
    {
      base_dd_min += data().effectN( 1 ).min( p );
      base_dd_max += data().effectN( 1 ).max( p );
    }
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
  double cankerous_wounds_chance;

  festering_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "festering_strike", p, p -> spec.festering_strike ),
    cankerous_wounds_chance( 0 )
  {
    parse_options( options_str );

    if ( p -> azerite.cankerous_wounds.enabled() )
    {
      cankerous_wounds_chance = p -> azerite.cankerous_wounds.spell() -> effectN( 2 ).base_value() / 100;
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = death_knight_melee_attack_t::bonus_da( s );

    if ( p() -> azerite.cankerous_wounds.enabled() )
    {
      da += p() -> azerite.cankerous_wounds.value( 1 );
    }

    return da;
  }

  void impact( action_state_t* s ) override
  {
    static const std::array<unsigned, 2> fw_proc_stacks = { { 2, 3 } };

    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      size_t n = rng().range( size_t(), fw_proc_stacks.size() );
      unsigned n_stacks = fw_proc_stacks[ n ];

      // Assuming cankerous wounds is a separate roll
      if ( p() -> azerite.cankerous_wounds.enabled() && rng().roll( cankerous_wounds_chance ) )
      {
        // Not in cankerous spelldata smh
        n_stacks = 3;
      }

      p() -> trigger_festering_wound( s, n_stacks, p() -> procs.fw_festering_strike );
    }
  }
};

// Frostscythe ==============================================================

struct frostscythe_t : public death_knight_melee_attack_t
{
  frostscythe_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "frostscythe", p, p -> talent.frostscythe )
  {
    parse_options( options_str );

    weapon = &( player -> main_hand_weapon );
    aoe = -1;

    weapon_req = WEAPON_1H;

    // T21 2P bonus : damage increase to Howling Blast, Frostscythe and Obliterate
    if ( p -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T21, B2 ) )
    {
      base_multiplier *= ( 1.0 + p -> sets -> set( DEATH_KNIGHT_FROST, T21, B2 ) -> effectN( 3 ).percent() );
    }
    
    crit_bonus_multiplier *= 1.0 + p -> spec.death_knight -> effectN( 5 ).percent();
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    consume_killing_machine( execute_state, p() -> procs.killing_machine_fsc );
    trigger_icecap( execute_state );

    if ( p() -> azerite.icy_citadel.enabled() && p() -> buffs.pillar_of_frost -> up() && execute_state -> result == RESULT_CRIT )
    {
      p() -> buffs.icy_citadel_builder -> trigger();
    }

    if ( p() -> buffs.inexorable_assault -> up() )
    {
      p() -> active_spells.inexorable_assault -> set_target( execute_state -> target );
      p() -> active_spells.inexorable_assault -> schedule_execute();
      p() -> buffs.inexorable_assault -> decrement();
    }

    // Frostscythe procs rime at half the chance of Obliterate
    p() -> buffs.rime -> trigger( 1, buff_t::DEFAULT_VALUE(),
            ( p() -> spec.rime -> effectN( 2 ).percent() + p() -> sets -> set( DEATH_KNIGHT_FROST, T19, B2 ) -> effectN( 1 ).percent() ) / 2.0 );
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
  frostwyrms_fury_damage_t( death_knight_t* p ) :
    death_knight_spell_t( "frostwyrms_fury", p, p -> find_spell( 279303 ) )
  {
    aoe = -1;
    background = true;
  }
};

struct frostwyrms_fury_t : public death_knight_spell_t
{
  frostwyrms_fury_damage_t* damage;

  frostwyrms_fury_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "frostwyrms_fury_driver", p, p -> talent.frostwyrms_fury ),
    damage( new frostwyrms_fury_damage_t( p ) )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    damage -> set_target( execute_state -> target );
    damage -> execute();
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
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = death_knight_melee_attack_t::bonus_da( s );

    if ( p() -> azerite.killer_frost.enabled() )
    {
      da += p() -> azerite.killer_frost.value( 1 );
    }

    int empty_runes = p() -> _runes.runes_depleted() + p() -> _runes.runes_regenerating();
    
    if ( p() -> azerite.latent_chill.enabled() &&
         empty_runes >= as<int>( p() -> azerite.latent_chill.spell() -> effectN( 2 ).base_value() ) )
    {
      double lc_damage = p() -> azerite.latent_chill.value( 1 );
      // Most traits affect OH and MH hits equally, this one doesn't give a damn.
      if ( weapon -> slot == SLOT_OFF_HAND )
      {
        lc_damage /= 2.0;
      }

      da += lc_damage;
    }

    return da;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    trigger_icecap( execute_state );
  }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    if ( p() -> azerite.killer_frost.enabled() && state -> result == RESULT_CRIT )
    {
      p() -> trigger_killing_machine( p() -> azerite.killer_frost.spell() -> effectN( 2 ).percent(),
                                      p() -> procs.km_from_killer_frost,
                                      p() -> procs.km_from_killer_frost_wasted );
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

    weapon_req = WEAPON_1H;

    mh = new frost_strike_strike_t( p, "frost_strike_mh", &( p -> main_hand_weapon ), data().effectN( 2 ).trigger() );
    add_child( mh );
    oh = new frost_strike_strike_t( p, "frost_strike_offhand", &( p -> off_hand_weapon ), data().effectN( 3 ).trigger() );
    add_child( oh );
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      mh -> set_target( execute_state -> target );
      mh -> execute();
      oh -> set_target( execute_state -> target );
      oh -> execute();
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
  glacial_advance_damage_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "glacial_advance", player, player -> find_spell( 195975 ) )
  {
    parse_options( options_str );
    aoe = -1; // TODO: Fancier targeting .. make it aoe for now
    background = true;
    ap_type = AP_WEAPON_BOTH;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    // Only applies the razorice debuff without the damage, and doesn't apply if no weapon is runeforged with razorice
    if ( p() -> active_spells.razorice_mh || p() -> active_spells.razorice_oh )
    {
      td( state -> target ) -> debuff.razorice -> trigger();
    }
  }
};

struct glacial_advance_t : public death_knight_spell_t
{
  glacial_advance_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "glacial_advance", player, player -> talent.glacial_advance )
  {
    parse_options( options_str );

    weapon = &( player -> main_hand_weapon );
    weapon_req = WEAPON_1H;

    execute_action = new glacial_advance_damage_t( player, options_str );
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
    aoe = 2;
    weapon = &( p -> main_hand_weapon );
  }

  int n_targets() const override
  { return p() -> in_death_and_decay() ? aoe + as<int>( p() -> spell.dnd_buff -> effectN( 3 ).base_value() ) : aoe; }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.heart_strike -> set_target( execute_state -> target );
      p() -> pets.dancing_rune_weapon_pet -> ability.heart_strike -> execute();
    }
  }

  void impact ( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    // Deep cuts is only applied to the primary target
    if ( p() -> azerite.deep_cuts.enabled() && state -> target == target )
    {
      td( state -> target ) -> debuff.deep_cuts -> trigger();
    }
    
    if ( p() -> talent.heartbreaker -> ok() && result_is_hit( state -> result ) )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER, heartbreaker_rp_gen, p() -> gains.heartbreaker, this );
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

    // Handle energize ourselves
    energize_type = ENERGIZE_NONE;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> resource_gain( RESOURCE_RUNIC_POWER, data().effectN( 2 ).resource( RESOURCE_RUNIC_POWER ),
        p() -> gains.horn_of_winter, this );

    p() -> replenish_rune( as<unsigned int>( data().effectN( 1 ).base_value() ), p() -> gains.horn_of_winter );
  }
};

// Howling Blast and Frost Fever ============================================

struct avalanche_t : public death_knight_spell_t
{
  avalanche_t( death_knight_t* p ) :
    death_knight_spell_t( "avalanche", p, p -> find_spell( 207150 ) )
  {
    aoe = -1;
    background = true;
  }
};

struct frost_fever_t : public death_knight_spell_t
{
  frost_fever_t( death_knight_t* p ) : 
    death_knight_spell_t( "frost_fever", p, p -> spell.frost_fever )
  {
    ap_type = AP_WEAPON_BOTH;

    p -> cooldown.frost_fever_icd = p -> get_cooldown( "frost_fever" );
    p -> cooldown.frost_fever_icd -> duration = p -> spec.frost_fever -> internal_cooldown();

    tick_may_crit = background = true;
    may_miss = may_crit = hasted_ticks = false;
  }

  void tick( dot_t* d ) override
  {
    death_knight_spell_t::tick( d );

    if ( p() -> cooldown.frost_fever_icd -> up() &&
         rng().roll( p() -> spec.frost_fever -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER,
                            p() -> spec.frost_fever -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
                            p() -> gains.frost_fever,
                            this );
      p() -> cooldown.frost_fever_icd -> start();
    }
  }
};


struct howling_blast_aoe_t : public death_knight_spell_t
{
  frost_fever_t* frost_fever;

  howling_blast_aoe_t( death_knight_t* p, const std::string& options_str, frost_fever_t* frf ) :
    death_knight_spell_t( "howling_blast_aoe", p, p -> find_spell( 237680 ) ),
    frost_fever( frf )
  {
    parse_options( options_str );
    ap_type = AP_WEAPON_BOTH;

    aoe = -1;
    background = true;

    // T21 2P bonus : damage increase to Howling Blast, Frostscythe and Obliterate
    if ( p -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T21, B2 ) )
    {
      base_multiplier *= 1.0 + p -> sets -> set( DEATH_KNIGHT_FROST, T21, B2 ) -> effectN( 1 ).percent();
    }
  }

  double action_multiplier() const override
  {
    double m = death_knight_spell_t::action_multiplier();

    if ( p() -> buffs.rime -> up() )
    {
      m *= 1.0 + p() -> buffs.rime -> data().effectN( 2 ).percent();
    }

    return m;
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

    if ( result_is_hit( s -> result ) )
    {
      frost_fever -> set_target( s -> target );
      frost_fever -> execute();
    }
  }
};

struct echoing_howl_t : public death_knight_spell_t
{
  echoing_howl_t( death_knight_t* p ) :
    death_knight_spell_t( "echoing howl", p, p -> find_spell( 275918 ) )
  {
    aoe = -1;
    base_dd_min = base_dd_max = p -> azerite.echoing_howl.value();
  }
};

struct howling_blast_t : public death_knight_spell_t
{
  frost_fever_t* frost_fever;
  howling_blast_aoe_t* aoe_damage;
  avalanche_t* avalanche;
  echoing_howl_t* echoing_howl;

  howling_blast_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "howling_blast", p, p -> spec.howling_blast ),
    frost_fever( new frost_fever_t( p ) ),
    aoe_damage( new howling_blast_aoe_t( p, options_str, frost_fever ) )
  {
    parse_options( options_str );

    aoe = 0;
    add_child( aoe_damage );
    add_child( frost_fever );
    ap_type = AP_WEAPON_BOTH;

    // T21 2P bonus : damage increase to Howling Blast, Frostscythe and Obliterate
    if ( p -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T21, B2 ) )
    {
      base_multiplier *= 1.0 + p -> sets -> set( DEATH_KNIGHT_FROST, T21, B2 ) -> effectN( 1 ).percent();
    }

    if ( p -> talent.avalanche -> ok() )
    {
      avalanche = new avalanche_t( p );
      add_child( avalanche );
    }
    
    if ( p -> azerite.echoing_howl.enabled() )
    {
      echoing_howl = new echoing_howl_t( p );
      add_child( echoing_howl );
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
      m *= 1.0 + p() -> buffs.rime -> data().effectN( 2 ).percent();
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

    if ( p() -> buffs.rime -> up() && p() -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T19, B4 ) )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER,
                            p() -> sets -> set( DEATH_KNIGHT_FROST, T19, B4 ) -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
                            p() -> gains.t19_4pc_frost,
                            this );
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
   
    // Don't trigger aoe when there's only one target
    if ( aoe_damage -> target_list().size() > 0 )
    {
      aoe_damage -> set_target( execute_state -> target );
      aoe_damage -> execute();
    }

    if ( p() -> talent.avalanche -> ok() && p() -> buffs.rime -> up() )
    {
      avalanche -> set_target( execute_state -> target );
      avalanche -> execute();
    }

    if ( p() -> azerite.echoing_howl.enabled() && p () -> buffs.rime -> up() )
    {
      echoing_howl -> set_target( execute_state -> target );
      echoing_howl -> execute();
    }

    p() -> buffs.rime -> decrement();
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      frost_fever -> set_target( s -> target );
      frost_fever -> execute();
    }
  }
};

// Marrowrend ===============================================================

struct marrowrend_t : public death_knight_melee_attack_t
{
  double bones_of_the_damned_proc_chance;

  marrowrend_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "marrowrend", p, p -> spec.marrowrend ),
    bones_of_the_damned_proc_chance( 0 )
  {
    parse_options( options_str );
    weapon = &( p -> main_hand_weapon );

    if ( p -> azerite.bones_of_the_damned.enabled() )
    {
      // Looks to be ~20% chance according to logs
      // Could be using the spelldata (currently = 5) to calculate the chance, hard-coding for now
      // bones_of_the_damned_proc_chance = 1.0 / ( p -> find_spell( 278484 ) -> effectN( 2 ).base_value() );
      bones_of_the_damned_proc_chance = 0.20;
    }
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();
    
    if ( p() -> buffs.dancing_rune_weapon -> check() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.marrowrend -> set_target( execute_state -> target );
      p() -> pets.dancing_rune_weapon_pet -> ability.marrowrend -> execute();
    }
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    int stack_gain = as<int>( data().effectN( 3 ).base_value() );

    if ( p() -> azerite.bones_of_the_damned.enabled() )
    {
      if ( rng().roll( bones_of_the_damned_proc_chance ) ) 
      {
        stack_gain += 1; // Not in spell data
      }
      p() -> buffs.bones_of_the_damned -> trigger();
    }

    p() -> buffs.bone_shield -> trigger( stack_gain );
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
  obliterate_strike_t( death_knight_t*     p,
                            const std::string&  name,
                            weapon_t*           w,
                            const spell_data_t* s ) :
    death_knight_melee_attack_t( name, p, s )
  {
    background = special = true;
    may_miss = false;
    weapon = w;
    
    // T21 2P bonus : damage increase to Howling Blast, Frostscythe and Obliterate
    if ( p -> sets -> has_set_bonus( DEATH_KNIGHT_FROST , T21, B2 ) )
    {
      base_multiplier *= ( 1.0 + p -> sets -> set( DEATH_KNIGHT_FROST, T21, B2 ) -> effectN( 2 ).percent() );
    }
  }

  double composite_crit_chance() const override
  {
    double cc = death_knight_melee_attack_t::composite_crit_chance();

    cc += p() -> buffs.killing_machine -> value();

    return cc;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    trigger_icecap( execute_state );

    if ( p() -> azerite.icy_citadel.enabled() && p() -> buffs.pillar_of_frost -> up() && execute_state -> result == RESULT_CRIT )
    {
      p() -> buffs.icy_citadel_builder -> trigger();
    }
  }
};

struct obliterate_t : public death_knight_melee_attack_t
{
  obliterate_strike_t* mh, *oh;

  obliterate_t( death_knight_t* p, const std::string& options_str = std::string() ) :
    death_knight_melee_attack_t( "obliterate", p, p -> spec.obliterate ),
    mh( nullptr ), oh( nullptr )
  {
    parse_options( options_str );

    weapon_req = WEAPON_1H;

    mh = new obliterate_strike_t( p, "obliterate_mh", &( p -> main_hand_weapon ), data().effectN( 2 ).trigger() );
    oh = new obliterate_strike_t( p, "obliterate_offhand", &( p -> off_hand_weapon ), data().effectN( 3 ).trigger() );

    add_child( mh );
    add_child( oh );
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      mh -> set_target( execute_state -> target );
      mh -> execute();

      oh -> set_target( execute_state -> target );
      oh -> execute();

      if ( p() -> buffs.inexorable_assault -> up() )
      {
        p() -> active_spells.inexorable_assault -> set_target( execute_state -> target );
        p() -> active_spells.inexorable_assault -> schedule_execute();
        p() -> buffs.inexorable_assault -> decrement();
      }

      p() -> buffs.rime -> trigger();
    }
    
    consume_killing_machine( execute_state, p() -> procs.killing_machine_oblit );
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

// Outbreak and Virulent Plague =============================================

struct virulent_eruption_t : public death_knight_spell_t
{
  virulent_eruption_t( death_knight_t* p ) :
    death_knight_spell_t( "virulent_eruption", p, p -> spell.virulent_eruption )
  {
    background = split_aoe_damage = true;
    aoe = -1;
  }
};

struct virulent_plague_t : public death_knight_spell_t
{
  virulent_eruption_t* eruption;

  virulent_plague_t( death_knight_t* p ) :
    death_knight_spell_t( "virulent_plague", p, p -> spell.virulent_plague ),
    eruption( new virulent_eruption_t( p ) )
  {
    base_tick_time *= 1.0 + p -> talent.ebon_fever -> effectN( 1 ).percent();
    dot_duration *= 1.0 + p -> talent.ebon_fever -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> talent.ebon_fever -> effectN( 3 ).percent();

    tick_may_crit = background = true;
    may_miss = may_crit = hasted_ticks = false;

    add_child( eruption );
  }

  void tick( dot_t* dot ) override
  {
    death_knight_spell_t::tick( dot );

    trigger_virulent_eruption( data().effectN( 2 ).percent(), dot -> target );
  }

  void trigger_virulent_eruption( double chance, player_t* t )
  {
    if ( rng().roll( chance ) )
    {
      eruption -> set_target( t );
      eruption -> execute();
    }
  }
};

struct outbreak_spreader_t : public death_knight_spell_t
{
  virulent_plague_t* vp;

  outbreak_spreader_t( death_knight_t* p, virulent_plague_t* plague ) :
    death_knight_spell_t( "outbreak_spreader", p ),
    vp( plague )
  {
    quiet = background = dual = true;
    callbacks = may_crit = false;
    aoe = -1;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      vp -> set_target( state -> target );
      vp -> execute();
    }
  }
};

struct outbreak_debuff_t : public death_knight_spell_t
{
  outbreak_spreader_t* spreader;
  const spell_data_t* outbreak_base;

  outbreak_debuff_t( death_knight_t* p, virulent_plague_t* vp ) :
    death_knight_spell_t( "outbreak_driver", p, dbc::find_spell( p, 196782 ) ),
    spreader( new outbreak_spreader_t( p, vp ) ),
    outbreak_base( p -> spec.outbreak )
  {
    quiet = background = tick_zero = dual = true;
    callbacks = hasted_ticks = false;

    tick_action = spreader;
  }

  // The debuff and the actual spell cast have different level requirements in spell data
  bool verify_actor_level() const override
  {
    return outbreak_base -> id() && outbreak_base -> is_level( player -> true_level ) &&
           outbreak_base -> level() <= MAX_LEVEL;
  }
};

struct outbreak_t : public death_knight_spell_t
{
  virulent_plague_t* vp;
  outbreak_debuff_t* debuff;

  outbreak_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "outbreak" ,p , p -> spec.outbreak ),
    vp( new virulent_plague_t( p ) ),
    debuff( new outbreak_debuff_t( p, vp ) )
  {
    parse_options( options_str );
    add_child( vp );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      debuff -> set_target( execute_state -> target );
      debuff -> schedule_execute();
    }
  }
};

// Pillar of Frost ==========================================================

struct frostwhelps_indignation_t : public death_knight_spell_t
{
  frostwhelps_indignation_t( death_knight_t* p ) :
    death_knight_spell_t( "frostwhelps_indignation", p ,
                          p -> azerite.frostwhelps_indignation.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
    // TODO: May not be the right spell ID because that one doesn't seem
    // to be affected by spec aura or mastery, blizzard oversight or different spell?
  {
    aoe = -1;
    background = true;

    base_dd_min = base_dd_max = p -> azerite.frostwhelps_indignation.value( 1 );
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );
    p() -> buffs.frostwhelps_indignation -> trigger();
  }
};

struct pillar_of_frost_bonus_buff_t : public buff_t
{
  pillar_of_frost_bonus_buff_t( death_knight_t* p ) :
    buff_t( p, "pillar_of_frost_bonus" )
  {
    set_max_stack( 100 );
    set_duration( p -> spec.pillar_of_frost -> duration() );
    set_default_value( p -> spec.pillar_of_frost -> effectN( 2 ).percent() );

    add_invalidate( CACHE_STRENGTH );
  }
};

struct pillar_of_frost_buff_t : public buff_t
{
  double icy_citadel_duration;

  pillar_of_frost_buff_t( death_knight_t* p ) :
    buff_t( p, "pillar_of_frost", p -> spec.pillar_of_frost )
  {
    set_cooldown( timespan_t::zero() );
    set_default_value( p -> spec.pillar_of_frost -> effectN( 1 ).percent() );
    add_invalidate( CACHE_STRENGTH );

    if ( p -> azerite.icy_citadel.enabled() )
    {
      icy_citadel_duration = p -> azerite.icy_citadel.spell() -> effectN( 2 ).base_value();
    }
  }

  void expire_override( int s, timespan_t t ) override
  {
    buff_t::expire_override( s, t );

    death_knight_t* p = debug_cast<death_knight_t*>( player );

    p -> buffs.pillar_of_frost_bonus -> expire();

    if ( p -> azerite.icy_citadel.enabled() )
    {
      p -> buffs.icy_citadel -> trigger();
      p -> buffs.icy_citadel -> extend_duration( p, timespan_t::from_millis( icy_citadel_duration * p -> buffs.icy_citadel_builder -> stack() ) );
      p -> buffs.icy_citadel_builder -> expire();
    }
  }
};

struct pillar_of_frost_t : public death_knight_spell_t
{
  frostwhelps_indignation_t* whelp;

  pillar_of_frost_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "pillar_of_frost", p, p -> spec.pillar_of_frost )
  {
    parse_options( options_str );

    if ( p -> azerite.frostwhelps_indignation.enabled() )
    {
      whelp = new frostwhelps_indignation_t( p );
    }

    harmful = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.pillar_of_frost -> trigger( 1,
                                             p() -> buffs.pillar_of_frost -> default_value +
                                             p() -> buffs.t20_4pc_frost -> stack_value() );
    p() -> buffs.t20_4pc_frost -> expire();

    if ( p() -> azerite.frostwhelps_indignation.enabled() )
    {
      whelp -> set_target( p() -> target );
      whelp -> execute();
    }
  }
};

// Raise Dead ===============================================================

struct raise_dead_t : public death_knight_spell_t
{
  raise_dead_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "raise_dead", p, p -> find_specialization_spell( "Raise Dead" ) )
  {
    parse_options( options_str );

    cooldown -> duration += p -> talent.all_will_serve -> effectN( 1 ).time_value();

    harmful = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> pets.ghoul_pet -> summon( timespan_t::zero() );
    if ( p() -> talent.all_will_serve -> ok() )
    {
      p() -> pets.risen_skulker -> summon( timespan_t::zero() );
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
  double frozen_tempest_target_threshold;

  remorseless_winter_damage_t( death_knight_t* p ) :
    death_knight_spell_t( "remorseless_winter_damage", p, p -> spec.remorseless_winter -> effectN( 2 ).trigger() ),
    frozen_tempest_target_threshold( 0 )
  {
    background = true;
    aoe = -1;

    if ( p -> azerite.frozen_tempest.enabled() )
    {
      frozen_tempest_target_threshold = p -> azerite.frozen_tempest.spell() -> effectN( 1 ).base_value();
    }
  }

  double action_multiplier() const override
  {
    double m = death_knight_spell_t::action_multiplier();

    m *= 1.0 + p() -> buffs.gathering_storm -> stack_value();

    return m;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = death_knight_spell_t::bonus_da( s );
    
    if ( p() -> azerite.frozen_tempest.ok() )
    {
      da += p() -> azerite.frozen_tempest.value( 2 );
    }

    return da;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( state -> n_targets >= frozen_tempest_target_threshold && p() -> azerite.frozen_tempest.enabled() && ! p() -> triggered_frozen_tempest )
    {
      p() -> buffs.rime -> trigger();
      p() -> triggered_frozen_tempest = true;
    }
  }
};

struct remorseless_winter_buff_t : public buff_t
{
  remorseless_winter_damage_t* damage; // (AOE) damage that ticks every second

  remorseless_winter_buff_t( death_knight_t* p ) :
    buff_t( p, "remorseless_winter", p -> spec.remorseless_winter ),
    damage( new remorseless_winter_damage_t( p ) )
  {
    set_cooldown( timespan_t::zero() ); // Controlled by the action
    set_refresh_behavior( buff_refresh_behavior::DURATION );
    set_tick_callback( [ this ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ ) 
    {
      damage -> execute();
    } ); 
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    debug_cast<death_knight_t*>( player ) -> buffs. gathering_storm -> expire();
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
    dot_duration = timespan_t::zero();
    base_tick_time = timespan_t::zero();

    ap_type = AP_WEAPON_BOTH;

    if ( action_t* rw_damage = p -> find_action( "remorseless_winter_damage" ) )
    {
      add_child( rw_damage );
    }
  }

  void execute() override
  {
    p() -> triggered_frozen_tempest = false;

    death_knight_spell_t::execute();

    p() -> buffs.remorseless_winter -> trigger();
  }
};

// Rune Strike ==============================================================

struct rune_strike_t : public death_knight_melee_attack_t
{
  rune_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "rune_strike", p, p -> talent.rune_strike )
  {
    parse_options( options_str );
    energize_type = ENERGIZE_NONE;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    p() -> replenish_rune( as<unsigned int>( data().effectN( 2 ).base_value() ), p() -> gains.rune_strike );

    // Bug ? Intended ? DRW copies Rune Strike now
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/323
    if ( p() -> buffs.dancing_rune_weapon -> check() && p() -> bugs )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.rune_strike -> set_target( execute_state -> target );
      p() -> pets.dancing_rune_weapon_pet -> ability.rune_strike -> execute();
    }
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

  int n_targets() const override
  { return p() -> in_death_and_decay() ? -1 : 0; }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      p() -> burst_festering_wound( state, 1 );
    }
  }
};

struct clawing_shadows_t : public scourge_strike_base_t
{
  clawing_shadows_t( death_knight_t* p, const std::string& options_str ) :
    scourge_strike_base_t( "clawing_shadows", p, p -> talent.clawing_shadows )
  {
    parse_options( options_str );
  }
};

struct scourge_strike_shadow_t : public death_knight_melee_attack_t
{
  const spell_data_t* scourge_base;

  scourge_strike_shadow_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "scourge_strike_shadow", p, p -> spec.scourge_strike -> effectN( 3 ).trigger() ),
    scourge_base( p -> spec.scourge_strike )
  {
    may_miss = may_parry = may_dodge = false;
    background = proc = dual = true;
    weapon = &( player -> main_hand_weapon );
  }

  // Fix the different level req in spell data between scourge strike and its shadow damage component
  bool verify_actor_level() const override
  {
    return scourge_base -> id() && scourge_base -> is_level( player -> true_level ) &&
      scourge_base -> level() <= MAX_LEVEL;
  }
};

struct scourge_strike_t : public scourge_strike_base_t
{
  scourge_strike_shadow_t* scourge_strike_shadow;

  scourge_strike_t( death_knight_t* p, const std::string& options_str ) :
    scourge_strike_base_t( "scourge_strike", p, p -> spec.scourge_strike ),
    scourge_strike_shadow( new scourge_strike_shadow_t( p ) )
  {
    parse_options( options_str );

    add_child( scourge_strike_shadow );
  }

  void impact( action_state_t* state ) override
  {
    scourge_strike_base_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      scourge_strike_shadow -> set_target( state -> target );
      scourge_strike_shadow -> execute();
    }
  }

  bool ready() override
  {
    if ( p() -> talent.clawing_shadows -> ok() )
    {
      return false;
    }

    return scourge_strike_base_t::ready();
  }
};

// Soul Reaper ==============================================================

struct soul_reaper_t : public death_knight_spell_t
{
  soul_reaper_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "soul_reaper", p, p ->  talent.soul_reaper )
  {
    parse_options( options_str );

    energize_type = ENERGIZE_NONE;

    tick_may_crit = tick_zero = true;
    may_miss = may_crit = hasted_ticks = false;       
  }
  
  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> replenish_rune( as<unsigned int>( p() -> talent.soul_reaper -> effectN( 2 ).base_value() ), p() -> gains.soul_reaper );
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

    p() -> pets.gargoyle -> summon( data().effectN( 3 ).trigger() -> duration() );
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
    if ( p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T21, B2 ) )
    {
      p() -> cooldown.dancing_rune_weapon -> adjust( 
        charges * timespan_t::from_millis( p() -> sets -> set( DEATH_KNIGHT_BLOOD, T21, B2) -> effectN( 1 ).base_value() ), false );
    }
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
    tick_action = new unholy_blight_dot_t( p );
  }
};

// Unholy Frenzy ============================================================

struct unholy_frenzy_t : public death_knight_spell_t
{
  unholy_frenzy_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "unholy_frenzy", p, p -> talent.unholy_frenzy )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    p() -> buffs.unholy_frenzy -> trigger();
  }
};

// ==========================================================================
// Death Knight Defensive Abilities
// ==========================================================================

// Anti-magic Shell =========================================================

struct antimagic_shell_buff_t : public buff_t
{
  double runic_barrier_duration;

  antimagic_shell_buff_t( death_knight_t* p ) :
    buff_t( p, "antimagic_shell", p -> spell.antimagic_shell )
  {
    set_cooldown( timespan_t::zero() );
    if ( p -> azerite.runic_barrier.enabled() )
    {
      runic_barrier_duration = p -> azerite.runic_barrier.spell() -> effectN( 2 ).base_value();
    }
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    death_knight_t* p = debug_cast< death_knight_t* >( player );
    double max_absorb = p -> resources.max[RESOURCE_HEALTH] * p -> spell.antimagic_shell -> effectN( 2 ).percent();
    
    max_absorb *= 1.0 + p -> talent.antimagic_barrier -> effectN( 2 ).percent();
    max_absorb *= 1.0 + p -> talent.spell_eater -> effectN( 1 ).percent();

    if ( p -> azerite.runic_barrier.enabled() )
    {
      max_absorb += p -> azerite.runic_barrier.value( 3 );
      duration = timespan_t::from_seconds( runic_barrier_duration );
    }

    max_absorb *= 1.0 + p -> cache.heal_versatility();
    p -> antimagic_shell_remaining_absorb = max_absorb;

    duration *= 1.0 + p -> talent.antimagic_barrier -> effectN( 2 ).percent();
    buff_t::execute( stacks, value, duration );
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
    death_knight_spell_t( "antimagic_shell", p, p -> spell.antimagic_shell ),
    min_interval( 60 ), interval( 60 ), interval_stddev( 0.05 ), interval_stddev_opt( 0 ), damage( 0 )
  {
    cooldown -> duration += timespan_t::from_millis( p -> talent.antimagic_barrier -> effectN( 1 ).base_value() );
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

    /*
    if ( damage > 0 )
      cooldown -> set_recharge_multiplier( 1.0 );
    */

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
      timespan_t new_cd = timespan_t::from_seconds( rng().gauss( interval, interval_stddev ) );
      if ( new_cd < timespan_t::from_seconds( min_interval ) )
        new_cd = timespan_t::from_seconds( min_interval );

      cooldown -> duration = new_cd;
    }

    death_knight_spell_t::execute();

    // If using the fake soaking, immediately grant the RP in one go
    if ( damage > 0 )
    {
      double max_absorb =  p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 2 ).percent();
      max_absorb *= 1.0 + p() -> talent.antimagic_barrier -> effectN( 2 ).percent();
      max_absorb *= 1.0 + p() -> talent.spell_eater -> effectN( 1 ).percent();

      if ( p() -> azerite.runic_barrier.enabled() )
      {
        max_absorb += p() -> azerite.runic_barrier.value( 3 );
      }

      max_absorb *= 1.0 + p() -> cache.heal_versatility();

      double absorbed = std::min( damage, max_absorb );

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
  mark_of_blood_heal_t( death_knight_t* p ) :
    death_knight_heal_t( "mark_of_blood", p, p -> find_spell( 206945 ) )
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

    td( execute_state -> target ) -> debuff.mark_of_blood -> trigger();
  }
};

// Rune Tap =================================================================

struct rune_tap_t : public death_knight_spell_t
{
  rune_tap_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "rune_tap", p, p -> talent.rune_tap )
  {
    parse_options( options_str );
    use_off_gcd = true;
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
  double delta;
  death_knight_t* p;

  vampiric_blood_buff_t( death_knight_t* player ) :
    buff_t( player, "vampiric_blood", player -> spec.vampiric_blood ),
    p( player )
  {
    set_cooldown( timespan_t::zero() );
    delta = data().effectN( 4 ).percent();
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    if ( delta != 0.0 )
    {
      double old_health = player -> resources.max[ RESOURCE_HEALTH ];
      player -> resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + delta;
      player -> recalculate_resource_max( RESOURCE_HEALTH );

      if ( this -> sim -> debug )
      {
        this -> sim -> out_debug.printf( "%s %s stacking health pct change %.2f%%, %.1f -> %.1f",
                                         this -> player -> name(), this -> name(), delta * 100.0, old_health,
                                         this -> player -> resources.max[ RESOURCE_HEALTH ] );
      }
    }
  }

  void expire_override( int s, timespan_t t ) override
  {
    buff_t::expire_override( s, t );

    death_knight_t* p = debug_cast<death_knight_t*>( player );

    if ( delta != 0.0 )
    {
      double old_health = p -> resources.max[ RESOURCE_HEALTH ];
      p -> resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + delta;
      p -> recalculate_resource_max( RESOURCE_HEALTH );

      if ( this -> sim -> debug )
      {
        this -> sim -> out_debug.printf( "%s %s stacking health pct change %.2f%%, %.1f -> %.1f",
                                         this -> player -> name(), this -> name(), delta * 100.0, old_health,
                                         this -> player -> resources.max[ RESOURCE_HEALTH ] );
      }
    }
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
    set_trigger_spell( p -> spec.runic_corruption );
    set_affects_regen( true );
    set_stack_change_callback( [ p ]( buff_t*, int, int )
    {
      p -> _runes.update_coefficient(); 
    } );
  }
};

} // UNNAMED NAMESPACE

void runeforge::fallen_crusader( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  // Fallen Crusader buff is shared between mh/oh
  buff_t* b = buff_t::find( effect.item -> player, "unholy_strength" );
  if ( ! b )
    return;

  action_t* heal = effect.item -> player -> find_action( "unholy_strength" );
  if ( ! heal )
  {
    struct fallen_crusader_heal_t : public death_knight_heal_t
    {
      fallen_crusader_heal_t( death_knight_t* dk, const spell_data_t* data ) :
        death_knight_heal_t( "unholy_strength", dk, data )
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

    heal = new fallen_crusader_heal_t( debug_cast< death_knight_t* >( effect.item -> player ), &b -> data() );
  }

  const death_knight_t* dk = debug_cast<const death_knight_t*>( effect.item -> player );

  effect.ppm_ = -1.0 * dk -> spell.fallen_crusader -> real_ppm();
  effect.custom_buff = b;
  effect.execute_action = heal;

  new dbc_proc_callback_t( effect.item, effect );
}

void runeforge::stoneskin_gargoyle( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  death_knight_t* p = debug_cast<death_knight_t*>( effect.item -> player );
  p -> runeforge.rune_of_the_stoneskin_gargoyle -> default_chance = 1.0;
}

double death_knight_t::resource_loss( resource_e resource_type, double amount, gain_t* g, action_t* a )
{
  double actual_amount = player_t::resource_loss( resource_type, amount, g, a );
  if ( resource_type == RESOURCE_RUNE )
  {
    _runes.consume(as<int>(amount) );
    // Ensure rune state is consistent with the actor resource state for runes
    assert( _runes.runes_full() == resources.current[ RESOURCE_RUNE ] );

    trigger_t20_4pc_unholy( amount );
  }

  return actual_amount;
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

void death_knight_t::trigger_t20_2pc_frost( double consumed )
{
  if ( ! sets -> has_set_bonus( DEATH_KNIGHT_FROST, T20, B2 ) )
  {
    return;
  }

  if ( ! buffs.pillar_of_frost -> up() )
  {
    return;
  }

  if ( consumed <= 0 )
  {
    return;
  }

  t20_2pc_frost += as<int>(consumed);

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s T20 2PC set bonus accumulates %.1f, total %d runic_power",
                             name(), consumed, t20_2pc_frost );
  }

  if ( t20_2pc_frost >= sets -> set( DEATH_KNIGHT_FROST, T20, B2 ) -> effectN( 1 ).base_value() )
  {
    buffs.pillar_of_frost -> extend_duration( this,
      timespan_t::from_seconds( sets -> set( DEATH_KNIGHT_FROST, T20, B2 ) -> effectN( 2 ).base_value() / 10.0 ) );
    
    t20_2pc_frost -= as<int>( sets -> set( DEATH_KNIGHT_FROST, T20, B2 ) -> effectN( 1 ).base_value() );
  }
}

void death_knight_t::trigger_t20_4pc_frost( double consumed )
{
  if ( ! sets -> has_set_bonus( DEATH_KNIGHT_FROST, T20, B4 ) )
  {
    return;
  }

  if ( consumed == 0 )
  {
    return;
  }

  t20_4pc_frost += as<int>(consumed);

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s T20 4PC set bonus accumulates %.1f, total %d runes",
                             name(), consumed, t20_4pc_frost );
  }

  if ( t20_4pc_frost >= sets -> set( DEATH_KNIGHT_FROST, T20, B4 ) -> effectN( 1 ).base_value() )
  {
    if ( ! buffs.t20_4pc_frost -> check() )
    {
      buffs.t20_4pc_frost -> trigger();
    }
    else
    {
      buffs.t20_4pc_frost -> current_value += buffs.t20_4pc_frost -> default_value;
    }
    t20_4pc_frost -= as<int>( sets -> set( DEATH_KNIGHT_FROST, T20, B4 ) -> effectN( 1 ).base_value() );
  }
}

void death_knight_t::trigger_t20_4pc_unholy( double consumed )
{
  if ( ! sets -> has_set_bonus( DEATH_KNIGHT_UNHOLY, T20, B4 ) )
  {
    return;
  }

  timespan_t cd_adjust = -timespan_t::from_seconds(
      sets -> set( DEATH_KNIGHT_UNHOLY, T20, B4 ) -> effectN( 1 ).base_value() / 10 );

  while ( consumed > 0 )
  {
    cooldown.army_of_the_dead -> adjust( cd_adjust );
    consumed--;
  }
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
    if ( sim -> log )
    {
      sim -> out_debug.printf( "Target died while affected by Soul Reaper debuff :  Death Knight %s gain the Soul Reaper buff.", name() );
    }

    buffs.soul_reaper -> trigger();
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
    trigger_runic_corruption( 0, talent.pestilent_pustules -> effectN( 1 ).percent() * n_wounds, procs.pp_runic_corruption );
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

  // Gains a festermight stack for each wound on the target
  if ( azerite.festermight.enabled() )
  {
    buffs.festermight -> trigger( n_wounds );
  }
}

void death_knight_t::trigger_virulent_plague_death( player_t* target )
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( sim -> event_mgr.canceled )
  {
    return;
  }

  if ( ! spec.outbreak -> ok() )
  {
    return;
  }

  death_knight_td_t* td = get_target_data( target );

  if ( ! td -> dot.virulent_plague -> is_ticking() )
  {
    return;
  }

  debug_cast<virulent_plague_t*>( td -> dot.virulent_plague -> current_action ) -> trigger_virulent_eruption( 1, target );
}

bool death_knight_t::in_death_and_decay() const
{
  if ( ! sim -> distance_targeting_enabled )
  {
    return dnds.size() > 0;
  }
  else
  {
    auto it = range::find_if( dnds, [ this ]( const ground_aoe_event_t* event ) {
      return get_ground_aoe_distance( *event -> pulse_state ) <= event -> pulse_state -> action -> radius;
    } );

    return it != dnds.end();
  }
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
  // If the given chance is 0, use the rppm effect
  if ( chance == 0 )
  {
    if ( rppm.killing_machine -> trigger() )
    {
      triggered = true;
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

void death_knight_t::trigger_runic_empowerment( double rpcost )
{
  double base_chance = spec.runic_empowerment -> effectN( 1 ).percent() / 10.0;

  if ( ! rng().roll( base_chance * rpcost ) )
    return;

  if ( sim -> debug )
  {
    log_rune_status( this );
  }

  if ( replenish_rune( 1, gains.runic_empowerment ) && sim -> debug )
  {
    sim -> out_debug.printf( "%s Runic Empowerment regenerated rune", name() );
    log_rune_status( this );
  }
}

bool death_knight_t::trigger_runic_corruption( double rpcost, double override_chance, proc_t* proc )
{
  double proc_chance = 0.0;
  // Check whether the proc is from a special effect, or from RP consumption
  if ( rpcost == 0 && override_chance != -1.0 )
  {
    proc_chance = override_chance;
  }
  else
  {
    proc_chance = spec.runic_corruption -> effectN( 1 ).percent() * rpcost / 100.0 ;
  }

  // If the roll fails, return
  if ( ! rng().roll( proc_chance ) )
    return false;

  // Runic Corruption duration is reduced by haste
  // It always regenerates 0.9 (3 times 3/10) of a rune
  timespan_t duration = timespan_t::from_seconds( 3.0 * cache.attack_haste() );
  // A refresh adds the full buff duration
  if ( buffs.runic_corruption -> check() == 0 )
    buffs.runic_corruption -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
  else
    buffs.runic_corruption -> extend_duration( this, duration );

  proc -> occur();

  return true;
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

void death_knight_t::burst_festering_wound( const action_state_t* state, unsigned n )
{
  struct fs_burst_t : public event_t
  {
    unsigned n;
    player_t* target;
    death_knight_t* dk;

    fs_burst_t( death_knight_t* dk, player_t* target, unsigned n ) :
      event_t( *dk, timespan_t::zero() ), n( n ), target( target ), dk( dk )
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

        // TODO: Is this per festering wound, or one try?
        if ( dk -> sets -> has_set_bonus( DEATH_KNIGHT_UNHOLY, T19, B2 ) )
        {
          dk -> trigger_runic_corruption( 0,
            dk -> sets -> set( DEATH_KNIGHT_UNHOLY, T19, B2 ) -> effectN( 1 ).percent(), 
            dk -> procs.t19_2pc_unholy );
        }
      }

      // Triggers once per target per player action:
      // Apocalypse is 10% * n wounds burst to proc
      // Scourge strike aoe is 1 - ( 0.9 ) ^ n targets to proc, or 10% for each target hit
      if ( dk -> talent.pestilent_pustules -> ok() )
      {
        dk -> trigger_runic_corruption( 0, dk -> talent.pestilent_pustules -> effectN( 1 ).percent() * n, dk -> procs.pp_runic_corruption );
      }

      td -> debuff.festering_wound -> decrement( n_executes );

      if ( dk -> azerite.festermight.enabled() )
      {
        dk -> buffs.festermight -> trigger( n_executes );
      }
    }
  };

  if ( ! spec.festering_wound -> ok() )
  {
    return;
  }

  if ( ! state -> action -> result_is_hit( state -> result ) )
  {
    return;
  }

  if ( ! get_target_data( state -> target ) -> debuff.festering_wound -> up() )
  {
    return;
  }

  make_event<fs_burst_t>( *sim, this, state -> target, n );
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
  // We solvee that by chosing a random number between 0 and the delay between each tick

  timespan_t first = timespan_t::from_seconds(
    rng().range( 0, talent.inexorable_assault -> effectN( 1 ).period().total_seconds() ) );

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
  // We solvee that by chosing a random number between 0 and the delay between each tick

  timespan_t first = timespan_t::from_seconds(
    rng().range( 0, talent.cold_heart -> effectN( 1 ).period().total_seconds() ) );
  
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
    if ( spec.blood_boil -> ok() || spec.deaths_caress -> ok() )
    {
      active_spells.blood_plague = new blood_plague_t( this );
    }

    if ( talent.mark_of_blood -> ok() )
    {
      active_spells.mark_of_blood = new mark_of_blood_heal_t( this );
    }
  }
  // Frost
  else if ( specialization() == DEATH_KNIGHT_FROST )
  {
    if ( talent.cold_heart -> ok() )
    {
      active_spells.cold_heart = new cold_heart_damage_t( this );
    }

    if ( spec.frost_death_knight -> ok() )
    {
      active_spells.freezing_death = new freezing_death_t( this );
    }

    if ( talent.inexorable_assault -> ok() )
    {
      active_spells.inexorable_assault = new inexorable_assault_damage_t( this );
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

  if ( azerite.bone_spike_graveyard.enabled() )
  {
    active_spells.bone_spike_graveyard = new bone_spike_graveyard_t( this );
  }

  player_t::create_actions();
}

// death_knight_t::create_action  ===========================================

action_t* death_knight_t::create_action( const std::string& name, const std::string& options_str )
{
  // General Actions
  if ( name == "antimagic_shell"          ) return new antimagic_shell_t          ( this, options_str );
  if ( name == "auto_attack"              ) return new auto_attack_t              ( this, options_str );
  if ( name == "chains_of_ice"            ) return new chains_of_ice_t            ( this, options_str );
  if ( name == "death_strike"             ) return new death_strike_t             ( this, options_str );
  if ( name == "icebound_fortitude"       ) return new icebound_fortitude_t       ( this, options_str );

  // Blood Actions
  if ( name == "blood_boil"               ) return new blood_boil_t               ( this, options_str );
  if ( name == "blooddrinker"             ) return new blooddrinker_t             ( this, options_str );
  if ( name == "bonestorm"                ) return new bonestorm_t                ( this, options_str );
  if ( name == "consumption"              ) return new consumption_t              ( this, options_str );
  if ( name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_t      ( this, options_str );
  if ( name == "dark_command"             ) return new dark_command_t             ( this, options_str );
  if ( name == "deaths_caress"            ) return new deaths_caress_t            ( this, options_str );
  if ( name == "heart_strike"             ) return new heart_strike_t             ( this, options_str );
  if ( name == "mark_of_blood"            ) return new mark_of_blood_t            ( this, options_str );
  if ( name == "marrowrend"               ) return new marrowrend_t               ( this, options_str );
  if ( name == "rune_strike"              ) return new rune_strike_t              ( this, options_str );
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
  if ( name == "mind_freeze"              ) return new mind_freeze_t              ( this, options_str );
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
  if ( name == "raise_dead"               ) return new raise_dead_t               ( this, options_str );
  if ( name == "scourge_strike"           ) return new scourge_strike_t           ( this, options_str );
  if ( name == "soul_reaper"              ) return new soul_reaper_t              ( this, options_str );
  if ( name == "summon_gargoyle"          ) return new summon_gargoyle_t          ( this, options_str );
  if ( name == "unholy_frenzy"            ) return new unholy_frenzy_t            ( this, options_str );
  if ( name == "unholy_blight"            ) return new unholy_blight_t            ( this, options_str );
    
  return player_t::create_action( name, options_str );
}

// death_knight_t::create_expression ========================================

expr_t* death_knight_t::create_death_and_decay_expression( const std::string& expr_str )
{
  auto expr = util::string_split( expr_str, "." );
  if ( expr.size() < 2 || ( expr.size() == 3 && ! util::str_compare_ci( expr[ 0 ], "dot" ) ) )
  {
    return nullptr;
  }

  auto spell_offset = expr.size() == 3u ? 1u : 0u;

  if ( ! util::str_compare_ci( expr[ spell_offset ], "death_and_decay" ) &&
       ! util::str_compare_ci( expr[ spell_offset ], "defile" ) )
  {
    return nullptr;
  }

  // "dot.death_and_decay|defile.X fallback warning"
  if ( expr.size() == 3 )
  {
    deprecated_dnd_expression = true;
  }

  if ( util::str_compare_ci( expr[ spell_offset + 1u ], "ticking" ) ||
       util::str_compare_ci( expr[ spell_offset + 1u ], "up" ) )
  {
    return make_fn_expr( "dnd_ticking", [ this ]() { return dnds.size() > 0 ? 1 : 0; } );
  }
  else if ( util::str_compare_ci( expr[ spell_offset + 1u ], "remains" ) )
  {
    return make_fn_expr( "dnd_remains", [ this ]() {
      return dnds.size() > 0
             ? dnds.back() -> remaining_time().total_seconds()
             : 0;
    } );
  }

  return nullptr;
}

expr_t* death_knight_t::create_expression( const std::string& name_str )
{
  auto splits = util::string_split( name_str, "." );

  if ( util::str_compare_ci( splits[ 0 ], "rune" ) && splits.size() > 1 )
  {
    if ( util::str_in_str_ci( splits[ 1 ], "time_to_" ) )
    {
      auto n_char = splits[ 1 ][ splits[ 1 ].size() - 1 ];
      auto n = n_char - '0';
      if ( n <= 0 || as<size_t>( n ) > MAX_RUNES )
      {
        sim -> errorf( "%s invalid expression '%s'.", name(), name_str.c_str() );
        return player_t::create_expression( name_str );
      }

      return make_fn_expr( "rune_time_to_x", [ this, n ]() {
        return _runes.time_to_regen( static_cast<unsigned>( n ) );
      } );
    }
  }

  auto dnd_expr = create_death_and_decay_expression( name_str );
  if ( dnd_expr )
  {
    return dnd_expr;
  }

  return player_t::create_expression( name_str );
}

// death_knight_t::create_pets ==============================================

void death_knight_t::create_pets()
{
  if ( specialization() == DEATH_KNIGHT_UNHOLY )
  {
    // Initialized even if the talent isn't picked for APL purpose
    pets.gargoyle = new pets::gargoyle_pet_t( this );

    if ( find_action( "raise_dead" ) )
    {
      pets.ghoul_pet = new pets::ghoul_pet_t( this );
      if ( talent.all_will_serve -> ok() )
      {
        pets.risen_skulker = new pets::risen_skulker_pet_t( this );
      }
    }

    if ( find_action( "army_of_the_dead" ) )
    {
      for ( int i = 0; i < 8; i++ )
      {
        pets.army_ghoul[ i ] = new pets::army_pet_t( this, "army_ghoul" );
      }
    }

    if ( find_action( "apocalypse" ) )
    {
      for ( int i = 0; i < 4; i++ )
      {
        pets.apoc_ghoul[ i ] = new pets::army_pet_t( this, "apoc_ghoul" );
      }
    }

    if ( azerite.magus_of_the_dead.enabled() )
    {
      pets.magus_pet[0] = new pets::magus_pet_t( this );
      pets.magus_pet[1] = new pets::magus_pet_t( this );
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
      for ( size_t i = 0; i < pets.bloodworms.size(); i++ )
      {
        pets.bloodworms[ i ] = new pets::bloodworm_pet_t( this );
      }
    }
  }
}

// death_knight_t::composite_attack_haste() =================================

double death_knight_t::composite_melee_haste() const
{
  double haste = player_t::composite_melee_haste();

  haste *= 1.0 / ( 1.0 + buffs.soul_reaper -> check_value() );

  haste *= 1.0 / ( 1.0 + buffs.unholy_frenzy -> check_value() );
	
  haste *= 1.0 / ( 1.0 + buffs.empower_rune_weapon -> check_value() );
  
  if ( buffs.bone_shield -> up() )
  {
    haste *= buffs.bone_shield -> value();
  }
  
  return haste;
}

// death_knight_t::composite_spell_haste() ==================================

double death_knight_t::composite_spell_haste() const
{
  double haste = player_t::composite_spell_haste();

  haste *= 1.0 / ( 1.0 + buffs.soul_reaper -> check_value() );

  haste *= 1.0 / ( 1.0 + buffs.unholy_frenzy -> check_value() );
	
  haste *= 1.0 / ( 1.0 + buffs.empower_rune_weapon -> check_value() );
  
  if ( buffs.bone_shield -> up() )
  {
    haste *= buffs.bone_shield -> value();
  }

  return haste;
}

// death_knight_t::init_rng =================================================

void death_knight_t::init_rng()
{
  player_t::init_rng();

  rppm.freezing_death = get_rppm( "freezing_death", sets -> set( DEATH_KNIGHT_FROST, T21, B4 ) );
  rppm.bloodworms = get_rppm( "bloodworms", talent.bloodworms );
  rppm.runic_attenuation = get_rppm( "runic_attenuation", talent.runic_attenuation );
  rppm.killing_machine = get_rppm( "killing_machine", spec.killing_machine );
}

// death_knight_t::init_base ================================================

void death_knight_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 5;

  player_t::init_base_stats();

  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility = 0.0;

  resources.base[ RESOURCE_RUNIC_POWER ] = 100;
  resources.base[ RESOURCE_RUNIC_POWER ] += spec.blood_death_knight -> effectN( 12 ).resource( RESOURCE_RUNIC_POWER );

  if ( talent.ossuary -> ok() )
    resources.base [ RESOURCE_RUNIC_POWER ] += ( talent.ossuary -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER ) );


  resources.base[ RESOURCE_RUNE        ] = MAX_RUNES;

  base_gcd = timespan_t::from_seconds( 1.0 );

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.

  base.dodge += 0.030;
}

// death_knight_t::init_spells ==============================================

void death_knight_t::init_spells()
{
  player_t::init_spells();

  // Specialization

  // Generic
  spec.plate_specialization = find_specialization_spell( "Plate Specialization" );
  spec.death_knight         = find_spell( 137005 ); // "Death Knight" passive
  // Shared
  spec.icebound_fortitude   = find_specialization_spell( "Icebound Fortitude" );
  spec.death_and_decay      = find_specialization_spell( "Death and Decay" );
  spec.death_strike_2       = find_specialization_spell( 278223 );

  // Blood
  spec.blood_death_knight       = find_specialization_spell( "Blood Death Knight" );
  spec.veteran_of_the_third_war = find_specialization_spell( "Veteran of the Third War" );
  spec.riposte                  = find_specialization_spell( "Riposte" );
  spec.blood_boil               = find_specialization_spell( "Blood Boil" );
  spec.crimson_scourge          = find_specialization_spell( "Crimson Scourge" );
  spec.dancing_rune_weapon      = find_specialization_spell( "Dancing Rune Weapon" );
  spec.deaths_caress            = find_specialization_spell( "Death's Caress" );
  spec.heart_strike             = find_specialization_spell( "Heart Strike" );
  spec.marrowrend               = find_specialization_spell( "Marrowrend" );
  spec.vampiric_blood           = find_specialization_spell( "Vampiric Blood" );

  // Frost
  spec.frost_death_knight  = find_specialization_spell( "Frost Death Knight" );
  spec.frost_fever         = find_specialization_spell( "Frost Fever" );
  spec.frost_strike        = find_specialization_spell( "Frost Strike" );
  spec.howling_blast       = find_specialization_spell( "Howling Blast" );
  spec.obliterate          = find_specialization_spell( "Obliterate" );
  spec.rime                = find_specialization_spell( "Rime" );
  spec.runic_empowerment   = find_specialization_spell( "Runic Empowerment" );
  spec.killing_machine     = find_specialization_spell( "Killing Machine" );
  spec.empower_rune_weapon = find_specialization_spell( "Empower Rune Weapon" );
  spec.pillar_of_frost     = find_specialization_spell( "Pillar of Frost" );
  spec.remorseless_winter  = find_specialization_spell( "Remorseless Winter" );

  // Unholy
  spec.unholy_death_knight = find_specialization_spell( "Unholy Death Knight" );
  spec.festering_strike    = find_specialization_spell( "Festering Strike" );
  spec.festering_wound     = find_specialization_spell( "Festering Wound" );
  spec.runic_corruption    = find_specialization_spell( "Runic Corruption" );
  spec.sudden_doom         = find_specialization_spell( "Sudden Doom" );
  spec.army_of_the_dead    = find_specialization_spell( "Army of the Dead" );
  spec.dark_transformation = find_specialization_spell( "Dark Transformation" );
  spec.death_coil          = find_specialization_spell( "Death Coil" );
  spec.outbreak            = find_specialization_spell( "Outbreak" );
  spec.scourge_strike      = find_specialization_spell( "Scourge Strike" );
  spec.apocalypse          = find_specialization_spell( "Apocalypse" );

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
  talent.glacial_advance      = find_talent_spell( "Glacial Advance" ); 
  talent.frostwyrms_fury      = find_talent_spell( "Frostwyrm's Fury" );

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
  talent.defile             = find_talent_spell( "Defile" );
  talent.epidemic           = find_talent_spell( "Epidemic" );

  talent.army_of_the_damned = find_talent_spell( "Army of the Damned" );
  talent.unholy_frenzy      = find_talent_spell( "Unholy Frenzy" );
  talent.summon_gargoyle    = find_talent_spell( "Summon Gargoyle" );
  
  // Blood Talents
  talent.heartbreaker           = find_talent_spell( "Heartbreaker" );
  talent.blooddrinker           = find_talent_spell( "Blooddrinker" );
  talent.rune_strike            = find_talent_spell( "Rune Strike"  );

  talent.rapid_decomposition    = find_talent_spell( "Rapid Decomposition" );
  talent.hemostasis             = find_talent_spell( "Hemostasis" );
  talent.consumption            = find_talent_spell( "Consumption" ); 

  talent.foul_bulwark           = find_talent_spell( "Foul Bulwark" );
  talent.ossuary                = find_talent_spell( "Ossuary" ); 
  talent.tombstone              = find_talent_spell( "Tombstone" );

  talent.will_of_the_necropolis = find_talent_spell( "Will of the Necropolis" ); // NYI
  talent.antimagic_barrier      = find_talent_spell( "Anti-Magic Barrier" ); 
  talent.rune_tap               = find_talent_spell( "Rune Tap" );
  
  talent.voracious              = find_talent_spell( "Voracious" ); 
  talent.bloodworms             = find_talent_spell( "Bloodworms" ); 
  talent.mark_of_blood          = find_talent_spell( "Mark of Blood" );

  talent.purgatory              = find_talent_spell( "Purgatory" ); // NYI
  talent.red_thirst             = find_talent_spell( "Red Thirst" );
  talent.bonestorm              = find_talent_spell( "Bonestorm" );

  // Generic spells
  // Shared
  spell.antimagic_shell = find_class_spell( "Anti-Magic Shell" );
  spell.chains_of_ice   = find_class_spell( "Chains of Ice" );
  spell.death_strike    = find_class_spell( "Death Strike" );
  spell.dnd_buff        = find_spell( 188290 );
  spell.fallen_crusader = find_spell( 166441 );
  
  // Blood
  spell.blood_shield        = find_spell( 77535 );
  spell.blood_plague        = find_spell( 55078 );
  spell.bone_shield         = find_spell( 195181 );
  
  spell.gravewarden         = find_spell( 242010 ); // T20
  spell.rune_master         = find_spell( 253381 ); // T21 4P

  // Frost
  spell.cold_heart_damage         = find_spell( 281210 );
  spell.frost_fever               = find_spell( 55095 );
  spell.inexorable_assault_damage = find_spell( 253597 );
  spell.murderous_efficiency_gain = find_spell( 207062 );

  spell.freezing_death            = find_spell( 253590 ); // T21 4P

  // Unholy
  spell.bursting_sores         = find_spell( 207267 );
  spell.death_coil_damage      = find_spell( 47632 );
  spell.festering_wound_debuff = find_spell( 194310 );
  spell.festering_wound_damage = find_spell( 194311 );
  spell.runic_corruption       = find_spell( 51460 );
  spell.soul_reaper            = find_spell( 215711 );
  spell.virulent_eruption      = find_spell( 191685 );
  spell.virulent_plague        = find_spell( 191587 );

  spell.master_of_ghouls       = find_spell( 246995 ); // T20 2P
  spell.coils_of_devastation   = find_spell( 253367 ); // T21 2P

  spell.pet_ghoul_claw         = find_spell( 91776 );
  spell.pet_sweeping_claws     = find_spell( 91778 );
  spell.pet_gnaw               = find_spell( 91800 );
  spell.pet_monstrous_blow     = find_spell( 91797 );
  spell.pet_army_claw          = find_spell( 199373 );
  spell.pet_gargoyle_strike    = find_spell( 51963 );
  spell.pet_dark_empowerment   = find_spell( 211947 );
  spell.pet_skulker_shot       = find_spell( 212423 );

  // Azerite Traits
  // Shared
  azerite.runic_barrier        = find_azerite_spell( "Runic Barrier" );
  azerite.bone_spike_graveyard = find_azerite_spell( "Bone Spike Graveyard" );
  azerite.cold_hearted         = find_azerite_spell( "Cold Hearted" ); // NYI

  // Blood
  azerite.deep_cuts                 = find_azerite_spell( "Deep Cuts" );
  azerite.marrowblood               = find_azerite_spell( "Marrowblood" );
  azerite.bones_of_the_damned       = find_azerite_spell( "Bones of the Damned" );
  azerite.eternal_rune_weapon       = find_azerite_spell( "Eternal Rune Weapon" );
  azerite.bloody_runeblade          = find_azerite_spell( "Bloody Runeblade" );
  
  // Frost
  azerite.frozen_tempest          = find_azerite_spell( "Frozen Tempest" );
  azerite.killer_frost            = find_azerite_spell( "Killer Frost" );
  azerite.icy_citadel             = find_azerite_spell( "Icy Citadel" );
  azerite.latent_chill            = find_azerite_spell( "Latent Chill" );
  azerite.echoing_howl            = find_azerite_spell( "Echoing Howl" );
  azerite.frostwhelps_indignation = find_azerite_spell( "Frostwhelp's Indignation" );

  // Unholy
  azerite.last_surprise          = find_azerite_spell( "Last Surprise" );
  azerite.festermight            = find_azerite_spell( "Festermight" );
  azerite.harrowing_decay        = find_azerite_spell( "Harrowing Decay" );
  azerite.cankerous_wounds       = find_azerite_spell( "Cankerous Wounds" );
  azerite.magus_of_the_dead      = find_azerite_spell( "Magus of the Dead" ); // TODO: NYI
  azerite.helchains              = find_azerite_spell( "Helchains" );
}

// death_knight_t::default_apl_dps_precombat ================================

void death_knight_t::default_apl_dps_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Flask
  precombat -> add_action( "flask" );

  // Food
  precombat -> add_action( "food" );

  precombat -> add_action( "augmentation" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Precombat potion
  precombat -> add_action( "potion" );
}

// death_knight_t::default_potion ===========================================

std::string death_knight_t::default_potion() const
{
  std::string frost_potion = ( true_level > 110 ) ? "battle_potion_of_strength" :
	                           ( true_level > 100 ) ? "prolonged_power" :
                             ( true_level >= 90 ) ? "draenic_strength" :
                             ( true_level >= 85 ) ? "mogu_power" :
                             ( true_level >= 80 ) ? "golemblood_potion" :
                             "disabled";

  std::string unholy_potion = ( true_level > 110 ) ? "battle_potion_of_strength" :
	                            ( true_level > 100 ) ? "prolonged_power" :
                              ( true_level >= 90 ) ? "draenic_strength" :
                              ( true_level >= 85 ) ? "mogu_power" :
                              ( true_level >= 80 ) ? "golemblood_potion" :
                              "disabled";

  std::string blood_potion =  ( true_level > 110 ) ? "battle_potion_of_strength" :
                              ( true_level > 100 ) ? "old_war" :
                              ( true_level >= 90 ) ? "draenic_strength" :
                              ( true_level >= 85 ) ? "mogu_power" :
                              ( true_level >= 80 ) ? "golemblood_potion" :
                              "disabled";

  switch ( specialization() )
  {
    case DEATH_KNIGHT_BLOOD: return blood_potion;
    case DEATH_KNIGHT_FROST: return frost_potion;
    default:                 return unholy_potion;
  }
}

// death_knight_t::default_food ========================================

std::string death_knight_t::default_food() const
{
  std::string frost_food = ( true_level > 110 ) ? "bountiful_captains_feast" :
                           ( true_level > 100 ) ? "lemon_herb_filet" :
                           ( true_level >= 90 ) ? "pickled_eel" :
                           ( true_level >= 85 ) ? "sea_mist_rice_noodles" :
                           ( true_level >= 80 ) ? "seafood_magnifique_feast" :
                           "disabled";

  std::string unholy_food = ( true_level > 110 ) ? "bountiful_captains_feast" :
                            ( true_level > 100 ) ? "nightborne_delicacy_platter" :
                            ( true_level >  90 ) ? "buttered_sturgeon" :
                            ( true_level >= 85 ) ? "sea_mist_rice_noodles" :
                            ( true_level >= 80 ) ? "seafood_magnifique_feast" :
                            "disabled";

  std::string blood_food =  ( true_level > 110 ) ? "swamp_fish_n_chips" :
	                          ( true_level > 100 ) ? "lavish_suramar_feast" :
                            ( true_level >  90 ) ? "pickled_eel" :
                            ( true_level >= 85 ) ? "sea_mist_rice_noodles" :
                            ( true_level >= 80 ) ? "seafood_magnifique_feast" :
                            "disabled";

  switch ( specialization() )
  {
    case DEATH_KNIGHT_BLOOD: return blood_food;
    case DEATH_KNIGHT_FROST: return frost_food;
    default:                 return unholy_food;
  }
}

// death_knight_t::default_flask ============================================

std::string death_knight_t::default_flask() const
{
  std::string flask_name = ( true_level >  110 ) ? "flask_of_the_undertow" : 
                           ( true_level >= 100 ) ? "countless_armies" :
                           ( true_level >= 90  ) ? "greater_draenic_strength_flask" :
                           ( true_level >= 85  ) ? "winters_bite" :
                           ( true_level >= 80  ) ? "titanic_strength" :
                           "disabled";

  // All specs use a strength flask as default
  return flask_name;
}

// death_knight_t::default_rune =============================================

std::string death_knight_t::default_rune() const
{
  return ( true_level >= 120 ) ? "battle_scarred" :
         ( true_level >= 110 ) ? "defiled" :
         ( true_level >= 100 ) ? "stout" :
         "disabled";
}

// death_knight_t::default_apl_blood ========================================

void death_knight_t::default_apl_blood()
{
  action_priority_list_t* def      = get_action_priority_list( "default" );
  action_priority_list_t* standard = get_action_priority_list( "standard" );

  // Setup precombat APL for DPS spec
  default_apl_dps_precombat();

  def -> add_action( "auto_attack" );
  // Interrupt
  // def -> add_action( this, "Mind Freeze" );

  // Racials
  def -> add_action( "blood_fury,if=cooldown.dancing_rune_weapon.ready&(!cooldown.blooddrinker.ready|!talent.blooddrinker.enabled)" );
  def -> add_action( "berserking" );

  // On-use items
  def -> add_action( "use_items,if=cooldown.dancing_rune_weapon.remains>90" );
  def -> add_action( "use_item,name=razdunks_big_red_button" );
  def -> add_action( "use_item,name=merekthas_fang" );

  // Cooldowns
  def -> add_action( "potion,if=buff.dancing_rune_weapon.up" );
  def -> add_action( this, "Dancing Rune Weapon", "if=!talent.blooddrinker.enabled|!cooldown.blooddrinker.ready" );
  def -> add_talent( this, "Tombstone", "if=buff.bone_shield.stack>=7" );
  def -> add_action( "call_action_list,name=standard" );

  // Single Target Rotation
  standard -> add_action( this, "Death Strike", "if=runic_power.deficit<=10" );
  standard -> add_talent( this, "Blooddrinker", "if=!buff.dancing_rune_weapon.up" );
  standard -> add_action( this, "Marrowrend", "if=(buff.bone_shield.remains<=rune.time_to_3|buff.bone_shield.remains<=(gcd+cooldown.blooddrinker.ready*talent.blooddrinker.enabled*2)|buff.bone_shield.stack<3)&runic_power.deficit>=20" );
  standard -> add_action( this, "Blood Boil", "if=charges_fractional>=1.8&(buff.hemostasis.stack<=(5-spell_targets.blood_boil)|spell_targets.blood_boil>2)" );
  standard -> add_action( this, "Marrowrend", "if=buff.bone_shield.stack<5&talent.ossuary.enabled&runic_power.deficit>=15" );
  standard -> add_talent( this, "Bonestorm", "if=runic_power>=100&!buff.dancing_rune_weapon.up" );
  standard -> add_action( this, "Death Strike", "if=runic_power.deficit<=(15+buff.dancing_rune_weapon.up*5+spell_targets.heart_strike*talent.heartbreaker.enabled*2)|target.time_to_die<10" );
  standard -> add_action( this, "Death and Decay", "if=spell_targets.death_and_decay>=3" );
  standard -> add_talent( this, "Rune Strike", "if=(charges_fractional>=1.8|buff.dancing_rune_weapon.up)&rune.time_to_3>=gcd" );
  standard -> add_action( this, "Heart Strike", "if=buff.dancing_rune_weapon.up|rune.time_to_4<gcd" );
  standard -> add_action( this, "Blood Boil", "if=buff.dancing_rune_weapon.up" );
  standard -> add_action( this, "Death and Decay", "if=buff.crimson_scourge.up|talent.rapid_decomposition.enabled|spell_targets.death_and_decay>=2" );
  standard -> add_talent( this, "Consumption" );
  standard -> add_action( this, "Blood Boil" );
  standard -> add_action( this, "Heart Strike", "if=rune.time_to_3<gcd|buff.bone_shield.stack>6" );
  standard -> add_talent( this, "Rune Strike" );
  standard -> add_action( "arcane_torrent,if=runic_power.deficit>20" );
}

// death_knight_t::default_apl_frost ========================================

void death_knight_t::default_apl_frost()
{
  action_priority_list_t* def          = get_action_priority_list( "default" );
  action_priority_list_t* cooldowns    = get_action_priority_list( "cooldowns" );
  action_priority_list_t* cold_heart   = get_action_priority_list( "cold_heart" );
  action_priority_list_t* standard     = get_action_priority_list( "standard" );
  action_priority_list_t* obliteration = get_action_priority_list( "obliteration" );
  action_priority_list_t* bos_pooling  = get_action_priority_list( "bos_pooling" );
  action_priority_list_t* bos_ticking  = get_action_priority_list( "bos_ticking" );
  action_priority_list_t* aoe          = get_action_priority_list( "aoe" );

  // Setup precombat APL for DPS spec
  default_apl_dps_precombat();

  def -> add_action( "auto_attack" );

  // Interrupt
  // def -> add_action( this, "Mind Freeze" );

  // Apply frost fever and maintain Icy Talons
  def -> add_action( this, "Howling Blast", "if=!dot.frost_fever.ticking&(!talent.breath_of_sindragosa.enabled|cooldown.breath_of_sindragosa.remains>15)", "Apply Frost Fever and maintain Icy Talons" );
  def -> add_talent( this, "Glacial Advance", "if=buff.icy_talons.remains<=gcd&buff.icy_talons.up&spell_targets.glacial_advance>=2&(!talent.breath_of_sindragosa.enabled|cooldown.breath_of_sindragosa.remains>15)" );
  def -> add_action( this, "Frost Strike", "if=buff.icy_talons.remains<=gcd&buff.icy_talons.up&(!talent.breath_of_sindragosa.enabled|cooldown.breath_of_sindragosa.remains>15)" );
  
  // Choose APL
  def -> add_action( "call_action_list,name=cooldowns" );
  def -> add_action( "run_action_list,name=bos_pooling,if=talent.breath_of_sindragosa.enabled&(cooldown.breath_of_sindragosa.remains<5|(cooldown.breath_of_sindragosa.remains<20&target.time_to_die<35))" );
  def -> add_action( "run_action_list,name=bos_ticking,if=dot.breath_of_sindragosa.ticking" );
  def -> add_action( "run_action_list,name=obliteration,if=buff.pillar_of_frost.up&talent.obliteration.enabled" );
  def -> add_action( "run_action_list,name=aoe,if=active_enemies>=2" );
  def -> add_action( "call_action_list,name=standard" );

  // On-use itemos
  cooldowns -> add_action( "use_items,if=(cooldown.pillar_of_frost.ready|cooldown.pillar_of_frost.remains>20)&(!talent.breath_of_sindragosa.enabled|cooldown.empower_rune_weapon.remains>95)" );
  cooldowns -> add_action( "use_item,name=razdunks_big_red_button" );
  cooldowns -> add_action( "use_item,name=merekthas_fang,if=!dot.breath_of_sindragosa.ticking&!buff.pillar_of_frost.up" );

  // In-combat potion
  cooldowns -> add_action( "potion,if=buff.pillar_of_frost.up&buff.empower_rune_weapon.up" );

  // Racials
  cooldowns -> add_action( "blood_fury,if=buff.pillar_of_frost.up&buff.empower_rune_weapon.up" );
  cooldowns -> add_action( "berserking,if=buff.pillar_of_frost.up" );

  // Pillar of Frost
  cooldowns -> add_action( this, "Pillar of Frost", "if=cooldown.empower_rune_weapon.remains", "Frost cooldowns" );
  cooldowns -> add_talent( this, "Breath of Sindragosa", "if=cooldown.empower_rune_weapon.remains&cooldown.pillar_of_frost.remains" );
  cooldowns -> add_action( this, "Empower Rune Weapon", "if=cooldown.pillar_of_frost.ready&!talent.breath_of_sindragosa.enabled&rune.time_to_5>gcd&runic_power.deficit>=10|target.time_to_die<20" );
  cooldowns -> add_action( this, "Empower Rune Weapon", "if=(cooldown.pillar_of_frost.ready|target.time_to_die<20)&talent.breath_of_sindragosa.enabled&rune>=3&runic_power>60" );

  // Cold Heart and Frostwyrm's Fury
  cooldowns -> add_action( "call_action_list,name=cold_heart,if=talent.cold_heart.enabled&((buff.cold_heart.stack>=10&debuff.razorice.stack=5)|target.time_to_die<=gcd)" );
  cooldowns -> add_talent( this, "Frostwyrm's Fury", "if=(buff.pillar_of_frost.remains<=gcd|(buff.pillar_of_frost.remains<8&buff.unholy_strength.remains<=gcd&buff.unholy_strength.up))&buff.pillar_of_frost.up&azerite.icy_citadel.rank<=2" );
  cooldowns -> add_talent( this, "Frostwyrm's fury", "if=(buff.icy_citadel.remains<=gcd|(buff.icy_citadel.remains<8&buff.unholy_strength.remains<=gcd&buff.unholy_strength.up))&buff.icy_citadel.up&azerite.icy_citadel.rank>2" );
  cooldowns -> add_talent( this, "Frostwyrm's Fury", "if=target.time_to_die<gcd|(target.time_to_die<cooldown.pillar_of_frost.remains&buff.unholy_strength.up)" );

  // Cold Heart conditionals
  cold_heart -> add_action( this, "Chains of Ice", "if=buff.cold_heart.stack>5&target.time_to_die<gcd", "Cold heart conditions" );
  cold_heart -> add_action( this, "Chains of Ice", "if=(buff.pillar_of_frost.remains<=gcd*(1+cooldown.frostwyrms_fury.ready)|buff.pillar_of_frost.remains<rune.time_to_3)&buff.pillar_of_frost.up&azerite.icy_citadel.rank<=2" );
  cold_heart -> add_action( this, "Chains of Ice", "if=buff.pillar_of_frost.remains<8&buff.unholy_strength.remains<gcd*(1+cooldown.frostwyrms_fury.ready)&buff.unholy_strength.remains&buff.pillar_of_frost.up&azerite.icy_citadel.rank<=2" );
  cold_heart -> add_action( this, "Chains of Ice", "if=(buff.icy_citadel.remains<=gcd*(1+cooldown.frostwyrms_fury.ready)|buff.icy_citadel.remains<rune.time_to_3)&buff.icy_citadel.up&azerite.icy_citadel.enabled&azerite.icy_citadel.rank>2" );
  cold_heart -> add_action( this, "Chains of Ice", "if=buff.icy_citadel.remains<8&buff.unholy_strength.remains<gcd*(1+cooldown.frostwyrms_fury.ready)&buff.unholy_strength.remains&buff.icy_citadel.up&!azerite.icy_citadel.enabled&azerite.icy_citadel.rank>2" );
  

  // "Breath of Sindragosa pooling rotation : starts 15s before the cd becomes available"
  bos_pooling -> add_action( this, "Howling Blast", "if=buff.rime.up", "Breath of Sindragosa pooling rotation : starts 20s before Pillar of Frost + BoS are available" );
  bos_pooling -> add_action( this, "Obliterate", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&rune.time_to_4<gcd&runic_power.deficit>=25&!talent.frostscythe.enabled" );
  bos_pooling -> add_action( this, "Obliterate", "if=rune.time_to_4<gcd&runic_power.deficit>=25" );
  bos_pooling -> add_talent( this, "Glacial Advance", "if=runic_power.deficit<20&cooldown.pillar_of_frost.remains>rune.time_to_4&spell_targets.glacial_advance>=2" );
  bos_pooling -> add_action( this, "Frost Strike", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&runic_power.deficit<20&cooldown.pillar_of_frost.remains>rune.time_to_4&!talent.frostscythe.enabled" );
  bos_pooling -> add_action( this, "Frost Strike", "if=runic_power.deficit<20&cooldown.pillar_of_frost.remains>rune.time_to_4" );
  bos_pooling -> add_talent( this, "Frostscythe", "if=buff.killing_machine.up&runic_power.deficit>(15+talent.runic_attenuation.enabled*3)&spell_targets.frostscythe>=2" );
  bos_pooling -> add_talent( this, "Frostscythe", "if=runic_power.deficit>=(35+talent.runic_attenuation.enabled*3)&spell_targets.frostscythe>=2" );
  bos_pooling -> add_action( this, "Obliterate", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&runic_power.deficit>=(35+talent.runic_attenuation.enabled*3)&!talent.frostscythe.enabled" );
  bos_pooling -> add_action( this, "Obliterate", "if=runic_power.deficit>=(35+talent.runic_attenuation.enabled*3)" );
  bos_pooling -> add_talent( this, "Glacial Advance", "if=cooldown.pillar_of_frost.remains>rune.time_to_4&runic_power.deficit<40&spell_targets.glacial_advance>=2" );
  bos_pooling -> add_action( this, "Frost Strike", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&cooldown.pillar_of_frost.remains>rune.time_to_4&runic_power.deficit<40&!talent.frostscythe.enabled" );
  bos_pooling -> add_action( this, "Frost Strike", "if=cooldown.pillar_of_frost.remains>rune.time_to_4&runic_power.deficit<40" );

  // Breath of Sindragosa uptime rotation
  bos_ticking -> add_action( this, "Obliterate", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&runic_power<=30&!talent.frostscythe.enabled" );
  bos_ticking -> add_action( this, "Obliterate", "if=runic_power<=30" );
  bos_ticking -> add_action( this, "Remorseless Winter", "if=talent.gathering_storm.enabled" );
  bos_ticking -> add_action( this, "Howling Blast", "if=buff.rime.up" );
  bos_ticking -> add_action( this, "Obliterate", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&rune.time_to_5<gcd|runic_power<=45&!talent.frostscythe.enabled" );
  bos_ticking -> add_action( this, "Obliterate", "if=rune.time_to_5<gcd|runic_power<=45" );
  bos_ticking -> add_talent( this, "Frostscythe", "if=buff.killing_machine.up&spell_targets.frostscythe>=2" );
  bos_ticking -> add_talent( this, "Horn of Winter", "if=runic_power.deficit>=30&rune.time_to_3>gcd" );
  bos_ticking -> add_action( this, "Remorseless Winter" );
  bos_ticking -> add_talent( this, "Frostscythe", "if=spell_targets.frostscythe>=2" );
  bos_ticking -> add_action( this, "Obliterate", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&runic_power.deficit>25|rune>3&!talent.frostscythe.enabled" );
  bos_ticking -> add_action( this, "Obliterate", "if=runic_power.deficit>25|rune>3" );
  bos_ticking -> add_action( "arcane_torrent,if=runic_power.deficit>20" );

  // Obliteration rotation
  obliteration -> add_action( this, "Remorseless Winter", "if=talent.gathering_storm.enabled", "Obliteration rotation" );
  obliteration -> add_action( this, "Obliterate", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&!talent.frostscythe.enabled&!buff.rime.up&spell_targets.howling_blast>=3" );
  obliteration -> add_action( this, "Obliterate", "if=!talent.frostscythe.enabled&!buff.rime.up&spell_targets.howling_blast>=3" );
  obliteration -> add_talent( this, "Frostscythe", "if=(buff.killing_machine.react|(buff.killing_machine.up&(prev_gcd.1.frost_strike|prev_gcd.1.howling_blast|prev_gcd.1.glacial_advance)))&spell_targets.frostscythe>=2" );
  obliteration -> add_action( this, "Obliterate", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&buff.killing_machine.react|(buff.killing_machine.up&(prev_gcd.1.frost_strike|prev_gcd.1.howling_blast|prev_gcd.1.glacial_advance))" );
  obliteration -> add_action( this, "Obliterate", "if=buff.killing_machine.react|(buff.killing_machine.up&(prev_gcd.1.frost_strike|prev_gcd.1.howling_blast|prev_gcd.1.glacial_advance))" );
  obliteration -> add_talent( this, "Glacial Advance", "if=(!buff.rime.up|runic_power.deficit<10|rune.time_to_2>gcd)&spell_targets.glacial_advance>=2" );
  obliteration -> add_action( this, "Howling Blast", "if=buff.rime.up&spell_targets.howling_blast>=2" );
  obliteration -> add_action( this, "Frost Strike", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&!buff.rime.up|runic_power.deficit<10|rune.time_to_2>gcd&!talent.frostscythe.enabled" );
  obliteration -> add_action( this, "Frost Strike", "if=!buff.rime.up|runic_power.deficit<10|rune.time_to_2>gcd" );
  obliteration -> add_action( this, "Howling Blast", "if=buff.rime.up" );
  obliteration -> add_talent( this, "Frostcythe", "if=spell_targets.frostscythe>=2" );
  obliteration -> add_action( this, "Obliterate", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&!talent.frostscythe.enabled" );
  obliteration -> add_action( this, "Obliterate" );

  // Standard rotation
  standard -> add_action( this, "Remorseless Winter", "", "Standard single-target rotation" );
  standard -> add_action( this, "Frost Strike", "if=cooldown.remorseless_winter.remains<=2*gcd&talent.gathering_storm.enabled" );
  standard -> add_action( this, "Howling Blast", "if=buff.rime.up" );
  standard -> add_action( this, "Obliterate", "if=!buff.frozen_pulse.up&talent.frozen_pulse.enabled" );
  standard -> add_action( this, "Frost Strike", "if=runic_power.deficit<(15+talent.runic_attenuation.enabled*3)" );
  standard -> add_talent( this, "Frostscythe", "if=buff.killing_machine.up&rune.time_to_4>=gcd" );
  standard -> add_action( this, "Obliterate", "if=runic_power.deficit>(25+talent.runic_attenuation.enabled*3)" );
  standard -> add_action( this, "Frost Strike" );
  standard -> add_talent( this, "Horn of Winter" );
  standard -> add_action( "arcane_torrent" );

  aoe -> add_action( this, "Remorseless Winter", "if=talent.gathering_storm.enabled|(azerite.frozen_tempest.rank&spell_targets.remorseless_winter>=3&!buff.rime.up)" );
  aoe -> add_talent( this, "Glacial Advance", "if=talent.frostscythe.enabled" );
  aoe -> add_action( this, "Frost Strike", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&cooldown.remorseless_winter.remains<=2*gcd&talent.gathering_storm.enabled&!talent.frostscythe.enabled" );
  aoe -> add_action( this, "Frost Strike", "if=cooldown.remorseless_winter.remains<=2*gcd&talent.gathering_storm.enabled" );
  aoe -> add_action( this, "Howling Blast", "if=buff.rime.up" );
  aoe -> add_talent( this, "Frostscythe", "if=buff.killing_machine.up" );
  aoe -> add_talent( this, "Glacial Advance", "if=runic_power.deficit<(15+talent.runic_attenuation.enabled*3)" );
  aoe -> add_action( this, "Frost Strike", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&runic_power.deficit<(15+talent.runic_attenuation.enabled*3)&!talent.frostscythe.enabled" );
  aoe -> add_action( this, "Frost Strike", "if=runic_power.deficit<(15+talent.runic_attenuation.enabled*3)" );
  aoe -> add_action( this, "Remorseless Winter" );
  aoe -> add_talent( this, "Frostscythe" );
  aoe -> add_action( this, "Obliterate", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&runic_power.deficit>(25+talent.runic_attenuation.enabled*3)&!talent.frostscythe.enabled" );
  aoe -> add_action( this, "Obliterate", "if=runic_power.deficit>(25+talent.runic_attenuation.enabled*3)" );
  aoe -> add_talent( this, "Glacial Advance" );
  aoe -> add_action( this, "Frost Strike", "target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&!talent.frostscythe.enabled" );
  aoe -> add_action( this, "Frost Strike" );
  aoe -> add_talent( this, "Horn of Winter" );
  aoe -> add_action( "arcane_torrent" );
}

void death_knight_t::default_apl_unholy()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default" );
  action_priority_list_t* generic   = get_action_priority_list( "generic" );
  action_priority_list_t* aoe       = get_action_priority_list( "aoe" );
  action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );

  // Setup precombat APL for DPS spec
  default_apl_dps_precombat();

  precombat -> add_action( this, "Raise Dead" );
  precombat -> add_action( this, "Army of the Dead", "delay=2" );

  def -> add_action( "auto_attack" );
  // Interrupt
  // def -> add_action( this, "Mind Freeze" );
  
  // Gargoyle pooling variable
  def -> add_action( "variable,name=pooling_for_gargoyle,value=cooldown.summon_gargoyle.remains<5&talent.summon_gargoyle.enabled" );

  // Ogcd cooldowns
  def -> add_action( "arcane_torrent,if=runic_power.deficit>65&(cooldown.summon_gargoyle.remains|!talent.summon_gargoyle.enabled)&rune.deficit>=5", "Racials, Items, and other ogcds" );
  def -> add_action( "blood_fury,if=pet.gargoyle.active|!talent.summon_gargoyle.enabled" );
  def -> add_action( "berserking,if=pet.gargoyle.active|!talent.summon_gargoyle.enabled" );
  def -> add_action( "use_items", "Custom trinkets usage" );
  def -> add_action( "use_item,name=bygone_bee_almanac,if=cooldown.summon_gargoyle.remains>60|!talent.summon_gargoyle.enabled" );
  def -> add_action( "use_item,name=jes_howler,if=pet.gargoyle.active|!talent.summon_gargoyle.enabled" );
  def -> add_action( "use_item,name=galecallers_beak,if=pet.gargoyle.active|!talent.summon_gargoyle.enabled" );
  def -> add_action( "potion,if=cooldown.army_of_the_dead.ready|pet.gargoyle.active|buff.unholy_frenzy.up" );
  // Maintain Virulent Plague
  def -> add_action( this, "Outbreak", "target_if=(dot.virulent_plague.tick_time_remains+tick_time<=dot.virulent_plague.remains)&dot.virulent_plague.remains<=gcd", "Maintain Virulent Plague" );
  // Action Lists
  def -> add_action( "call_action_list,name=cooldowns" );
  def -> add_action( "run_action_list,name=aoe,if=active_enemies>=2" );
  def -> add_action( "call_action_list,name=generic" );

  cooldowns -> add_action( this, "Army of the Dead" );
  cooldowns -> add_action( this, "Apocalypse", "if=debuff.festering_wound.stack>=4" );
  cooldowns -> add_action( this, "Dark Transformation", "if=!raid_event.adds.exists|raid_event.adds.in>15" );
  cooldowns -> add_talent( this, "Summon Gargoyle", "if=runic_power.deficit<14" );
  cooldowns -> add_talent( this, "Unholy Frenzy", "if=debuff.festering_wound.stack<4" );
  cooldowns -> add_talent( this, "Unholy Frenzy", "if=active_enemies>=2&((cooldown.death_and_decay.remains<=gcd&!talent.defile.enabled)|(cooldown.defile.remains<=gcd&talent.defile.enabled))" );
  cooldowns -> add_talent( this, "Soul Reaper", "target_if=target.time_to_die<8&target.time_to_die>4" );
  cooldowns -> add_talent( this, "Soul Reaper", "if=(!raid_event.adds.exists|raid_event.adds.in>20)&rune<=(1-buff.unholy_frenzy.up)" );
  cooldowns -> add_talent( this, "Unholy Blight" );

  generic -> add_action( this, "Death Coil", "if=buff.sudden_doom.react&!variable.pooling_for_gargoyle|pet.gargoyle.active" );
  generic -> add_action( this, "Death Coil", "if=runic_power.deficit<14&(cooldown.apocalypse.remains>5|debuff.festering_wound.stack>4)&!variable.pooling_for_gargoyle" );
  generic -> add_action( this, "Death and Decay", "if=talent.pestilence.enabled&cooldown.apocalypse.remains" );
  generic -> add_talent( this, "Defile", "if=cooldown.apocalypse.remains" );
  generic -> add_action( this, "Scourge Strike", "if=((debuff.festering_wound.up&cooldown.apocalypse.remains>5)|debuff.festering_wound.stack>4)&cooldown.army_of_the_dead.remains>5" );
  generic -> add_talent( this, "Clawing Shadows", "if=((debuff.festering_wound.up&cooldown.apocalypse.remains>5)|debuff.festering_wound.stack>4)&cooldown.army_of_the_dead.remains>5" );
  generic -> add_action( this, "Death Coil", "if=runic_power.deficit<20&!variable.pooling_for_gargoyle" );
  generic -> add_action( this, "Festering Strike", "if=((((debuff.festering_wound.stack<4&!buff.unholy_frenzy.up)|debuff.festering_wound.stack<3)&cooldown.apocalypse.remains<3)|debuff.festering_wound.stack<1)&cooldown.army_of_the_dead.remains>5" );
  generic -> add_action( this, "Death Coil", "if=!variable.pooling_for_gargoyle" );
 
  // Generic AOE actions to be done
  aoe -> add_action( this, "Death and Decay", "if=cooldown.apocalypse.remains", "AoE rotation" );
  aoe -> add_talent( this, "Defile" );
  aoe -> add_talent( this, "Epidemic", "if=death_and_decay.ticking&rune<2&!variable.pooling_for_gargoyle" );
  aoe -> add_action( this, "Death Coil", "if=death_and_decay.ticking&rune<2&!variable.pooling_for_gargoyle" );
  aoe -> add_action( this, "Scourge Strike", "if=death_and_decay.ticking&cooldown.apocalypse.remains" );
  aoe -> add_talent( this, "Clawing Shadows", "if=death_and_decay.ticking&cooldown.apocalypse.remains" );
  aoe -> add_talent( this, "Epidemic", "if=!variable.pooling_for_gargoyle" );
  aoe -> add_action( this, "Festering Strike", "target_if=debuff.festering_wound.stack<=1&cooldown.death_and_decay.remains" );
  aoe -> add_action( this, "Festering Strike", "if=talent.bursting_sores.enabled&spell_targets.bursting_sores>=2&debuff.festering_wound.stack<=1" );
  aoe -> add_action( this, "Death Coil", "if=buff.sudden_doom.react&rune.deficit>=4" );
  aoe -> add_action( this, "Death Coil", "if=buff.sudden_doom.react&!variable.pooling_for_gargoyle|pet.gargoyle.active" );
  aoe -> add_action( this, "Death Coil", "if=runic_power.deficit<14&(cooldown.apocalypse.remains>5|debuff.festering_wound.stack>4)&!variable.pooling_for_gargoyle" );
  aoe -> add_action( this, "Scourge Strike", "if=((debuff.festering_wound.up&cooldown.apocalypse.remains>5)|debuff.festering_wound.stack>4)&cooldown.army_of_the_dead.remains>5" );
  aoe -> add_talent( this, "Clawing Shadows", "if=((debuff.festering_wound.up&cooldown.apocalypse.remains>5)|debuff.festering_wound.stack>4)&cooldown.army_of_the_dead.remains>5" );
  aoe -> add_action( this, "Death Coil", "if=runic_power.deficit<20&!variable.pooling_for_gargoyle" );
  aoe -> add_action( this, "Festering Strike", "if=((((debuff.festering_wound.stack<4&!buff.unholy_frenzy.up)|debuff.festering_wound.stack<3)&cooldown.apocalypse.remains<3)|debuff.festering_wound.stack<1)&cooldown.army_of_the_dead.remains>5" );
  aoe -> add_action( this, "Death Coil", "if=!variable.pooling_for_gargoyle" );
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
      default_apl_unholy();
      break;
    case DEATH_KNIGHT_FROST:
      default_apl_frost();
      break;
    case DEATH_KNIGHT_BLOOD:
      default_apl_blood();
      break;
    default:
      default_apl_frost();
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
        -> set_cooldown( timespan_t::zero() );

  runeforge.rune_of_the_fallen_crusader = make_buff( this, "unholy_strength", spell.fallen_crusader -> effectN( 1 ).trigger() )
            -> add_invalidate( CACHE_STRENGTH );
  
  runeforge.rune_of_the_stoneskin_gargoyle = make_buff( this, "stoneskin_gargoyle", find_spell( 62157 ) )
            -> add_invalidate( CACHE_ARMOR )
            -> add_invalidate( CACHE_STAMINA )
            -> add_invalidate( CACHE_STRENGTH )
            -> set_chance( 0 );

  // Blood
  buffs.blood_shield = new blood_shield_buff_t( this );

  buffs.bone_shield = make_buff( this, "bone_shield", spell.bone_shield )
        -> set_default_value( 1.0 / ( 1.0 + spell.bone_shield -> effectN( 4 ).percent() ) )
        -> set_stack_change_callback( talent.foul_bulwark -> ok() ? [ this ]( buff_t*, int old_stacks, int new_stacks ) 
          {
            double old_buff = old_stacks * talent.foul_bulwark -> effectN( 1 ).percent();
            double new_buff = new_stacks * talent.foul_bulwark -> effectN( 1 ).percent();
            resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + old_buff;
            resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + new_buff;
            recalculate_resource_max( RESOURCE_HEALTH );
          } : buff_stack_change_callback_t() )
        // The internal cd in spelldata is for stack loss, handled in bone_shield_handler
        -> set_cooldown( timespan_t::zero() )
        -> set_max_stack( spell.bone_shield -> max_stacks() )
        -> add_invalidate( CACHE_BONUS_ARMOR )
        -> add_invalidate( CACHE_HASTE );

  buffs.crimson_scourge = make_buff( this, "crimson_scourge", find_spell( 81141 ) )
        -> set_trigger_spell( spec.crimson_scourge );

  buffs.dancing_rune_weapon = new dancing_rune_weapon_buff_t( this );

  buffs.hemostasis = make_buff( this, "hemostasis", talent.hemostasis -> effectN( 1 ).trigger() )
        -> set_trigger_spell( talent.hemostasis )
        -> set_default_value( talent.hemostasis -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );

  buffs.rune_tap = make_buff( this, "rune_tap", talent.rune_tap )
        -> set_cooldown( timespan_t::zero() );

  buffs.tombstone = make_buff<absorb_buff_t>( this, "tombstone", talent.tombstone )
        -> set_cooldown( timespan_t::zero() ); // Handled by the action

  buffs.vampiric_blood = new vampiric_blood_buff_t( this );

  buffs.voracious = make_buff( this, "voracious", find_spell( 274009 ) )
        -> set_trigger_spell( talent.voracious )
        -> add_invalidate( CACHE_LEECH );

  // Frost
  buffs.cold_heart = make_buff( this, "cold_heart", talent.cold_heart -> effectN( 1 ).trigger() )
        -> set_trigger_spell( talent.cold_heart );

  buffs.empower_rune_weapon = new empower_rune_weapon_buff_t( this );

  buffs.frozen_pulse = make_buff( this, "frozen_pulse", talent.frozen_pulse );

  buffs.gathering_storm = make_buff( this, "gathering_storm", find_spell( 211805 ) )
        -> set_trigger_spell( talent.gathering_storm )
        -> set_default_value( find_spell( 211805 ) -> effectN( 1 ).percent() );

  buffs.icy_talons = make_buff( this, "icy_talons", talent.icy_talons -> effectN( 1 ).trigger() )
        -> add_invalidate( CACHE_ATTACK_SPEED )
        -> set_default_value( talent.icy_talons -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
        -> set_trigger_spell( talent.icy_talons );

  buffs.inexorable_assault = make_buff( this, "inexorable_assault", find_spell( 253595 ) )
        -> set_trigger_spell( talent.inexorable_assault );

  buffs.killing_machine = make_buff( this, "killing_machine", spec.killing_machine -> effectN( 1 ).trigger() )
        -> set_trigger_spell( spec.killing_machine )
        -> set_chance( 1.0 )
        -> set_default_value( find_spell( 51124 ) -> effectN( 1 ).percent() );

  buffs.pillar_of_frost = new pillar_of_frost_buff_t( this );
  buffs.pillar_of_frost_bonus = new pillar_of_frost_bonus_buff_t( this );

  buffs.remorseless_winter = new remorseless_winter_buff_t( this );

  buffs.rime = make_buff( this, "rime", spec.rime -> effectN( 1 ).trigger() )
        -> set_trigger_spell( spec.rime )
        -> set_chance( spec.rime -> effectN( 2 ).percent() + sets -> set( DEATH_KNIGHT_FROST, T19, B2 ) -> effectN( 1 ).percent() );

  // Unholy
  buffs.dark_transformation = new dark_transformation_buff_t( this );
  
  buffs.runic_corruption = new runic_corruption_buff_t( this );
  
  buffs.soul_reaper = make_buff( this, "soul_reaper", spell.soul_reaper )
        -> set_default_value( spell.soul_reaper -> effectN( 1 ).percent() )
        -> add_invalidate( CACHE_HASTE );

  buffs.sudden_doom = make_buff( this, "sudden_doom", spec.sudden_doom -> effectN( 1 ).trigger() )
        -> set_rppm( RPPM_ATTACK_SPEED, get_rppm( "sudden_doom", spec.sudden_doom ) -> get_frequency(), 1.0 + talent.harbinger_of_doom -> effectN( 2 ).percent())
        -> set_trigger_spell( spec.sudden_doom )
        -> set_max_stack( specialization() == DEATH_KNIGHT_UNHOLY ? 
                   as<unsigned int>( spec.sudden_doom -> effectN( 1 ).trigger() -> max_stacks() + talent.harbinger_of_doom -> effectN( 1 ).base_value() ):
                   1 );

  buffs.unholy_frenzy = make_buff( this, "unholy_frenzy", talent.unholy_frenzy )
        -> set_default_value( talent.unholy_frenzy -> effectN( 1 ).percent() )
        -> add_invalidate( CACHE_HASTE );
  
  // Legion set bonnus
  buffs.t20_2pc_unholy = make_buff( this, "master_of_ghouls", spell.master_of_ghouls )
        -> set_trigger_spell( sets -> set( DEATH_KNIGHT_UNHOLY, T20, B2 ) )
        -> set_default_value( spell.master_of_ghouls -> effectN( 1 ).percent() )
        -> set_refresh_behavior( buff_refresh_behavior::EXTEND );

  buffs.t20_4pc_frost = make_buff( this, "icy_edge" )
        -> set_default_value( sets -> set( DEATH_KNIGHT_FROST, T20, B4 ) -> effectN( 2 ).percent() )
        -> set_chance( sets -> has_set_bonus( DEATH_KNIGHT_FROST, T20, B4 ) );
  
  buffs.t20_blood = make_buff<stat_buff_t>( this, "gravewarden", spell.gravewarden )
        -> add_stat( STAT_VERSATILITY_RATING, spell.gravewarden -> effectN( 1 ).base_value() );
  
  buffs.t21_4p_blood = make_buff( this, "rune_master", spell.rune_master )
        -> set_trigger_spell( spell.rune_master )
        -> set_affects_regen( true )
        -> set_stack_change_callback( [ this ]( buff_t*, int, int ){ this -> _runes.update_coefficient(); } );

  // Azerite Traits
  buffs.bones_of_the_damned = make_buff<stat_buff_t>( this, "bones_of_the_damned",
                                azerite.bones_of_the_damned.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
        -> add_stat( STAT_ARMOR, azerite.bones_of_the_damned.value( 1 ) );

  buffs.eternal_rune_weapon = make_buff<stat_buff_t>( this, "eternal_rune_weapon", 
                                azerite.eternal_rune_weapon.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
        -> add_stat( STAT_STRENGTH, azerite.eternal_rune_weapon.value() );
  
  buffs.icy_citadel = make_buff<stat_buff_t>( this, "icy_citadel", find_spell( 272723 ) )
        -> add_stat( STAT_STRENGTH, azerite.icy_citadel.value( 1 ) );

  buffs.icy_citadel_builder = make_buff( this, "icy_citadel_builder",
                                azerite.icy_citadel.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() );
  
  buffs.festermight = make_buff<stat_buff_t>( this, "festermight", find_spell( 274373 ) )
        -> add_stat( STAT_STRENGTH, azerite.festermight.value() )
        -> set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buffs.bloody_runeblade = make_buff<stat_buff_t>( this, "bloody_runeblade", find_spell( 289349 ) )
    -> add_stat( STAT_HASTE_RATING, azerite.bloody_runeblade.value( 2 ) );

  buffs.frostwhelps_indignation = make_buff<stat_buff_t>( this, "frostwhelps_indignation", find_spell( 287338 ) )
    -> add_stat( STAT_MASTERY_RATING, azerite.frostwhelps_indignation.value( 2 ) );

  buffs.helchains = new helchains_buff_t( this );
    
}

// death_knight_t::init_gains ===============================================

void death_knight_t::init_gains()
{
  player_t::init_gains();

  // Shared
  gains.antimagic_shell                  = get_gain( "Antimagic Shell" );
  gains.power_refund                     = get_gain( "power_refund" );
  gains.rune                             = get_gain( "Rune Regeneration" );
  gains.start_of_combat_overflow         = get_gain( "Start of Combat Overflow" );

  // Blood
  gains.drw_heart_strike                 = get_gain( "Rune Weapon Heart Strike" );
  gains.heartbreaker                     = get_gain( "Heartbreaker" );
  gains.rune_strike                      = get_gain( "Rune Strike" );
  gains.tombstone                        = get_gain( "Tombstone" );

  gains.t19_4pc_blood                    = get_gain( "Tier19 Blood 4PC" );
  gains.bloody_runeblade                 = get_gain( "Bloody Runeblade" );

  // Frost
  gains.empower_rune_weapon              = get_gain( "Empower Rune Weapon" );
  gains.frost_fever                      = get_gain( "Frost Fever" );
  gains.horn_of_winter                   = get_gain( "Horn of Winter" );
  gains.murderous_efficiency             = get_gain( "Murderous Efficiency" );
  gains.obliteration                     = get_gain( "Obliteration" );
  gains.runic_attenuation                = get_gain( "Runic Attenuation" );
  gains.runic_empowerment                = get_gain( "Runic Empowerment" );

  gains.t19_4pc_frost                    = get_gain( "Tier19 Frost 4PC" );

  // Unholy
  gains.festering_wound                  = get_gain( "Festering Wound" );
  gains.soul_reaper                      = get_gain( "Soul Reaper" );
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
  procs.km_from_killer_frost    = get_proc( "Killing Machine: Killer Frost" );

  procs.km_from_crit_aa_wasted         = get_proc( "Killing Machine wasted: Critical auto attacks" );
  procs.km_from_obliteration_fs_wasted = get_proc( "Killing Machine wasted: Frost Strike" );
  procs.km_from_obliteration_hb_wasted = get_proc( "Killing Machine wasted: Howling Blast" );
  procs.km_from_obliteration_ga_wasted = get_proc( "Killing Machine wasted: Glacial Advance" );
  procs.km_from_killer_frost_wasted    = get_proc( "Killing Machine wasted: Killer Frost" );

  procs.ready_rune            = get_proc( "Rune ready" );

  procs.rp_runic_corruption   = get_proc( "Runic Corruption from Runic Power Spent" );
  procs.pp_runic_corruption   = get_proc( "Runic Corruption from Pestilent Pustules" );

  procs.t19_2pc_unholy        = get_proc( "Runic Corruption from T19 2P" );

  procs.bloodworms            = get_proc( "Bloodworms" );

  procs.fw_festering_strike = get_proc( "Festering Wound from Festering Strike" );
  procs.fw_infected_claws   = get_proc( "Festering Wound from Infected Claws" );
  procs.fw_pestilence       = get_proc( "Festering Wound from Pestilence" );
  procs.fw_unholy_frenzy    = get_proc( "Festering Wound from Unholy Frenzy" );
  procs.fw_t19_4p           = get_proc( "Festering Wound from T19 4P" );
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
}

// death_knight_t::reset ====================================================

void death_knight_t::reset()
{
  player_t::reset();

  runic_power_decay_rate = 1; // 1 RP per second decay
  antimagic_shell_remaining_absorb = 0.0;
  _runes.reset();
  dnds.clear();
  t20_2pc_frost = 0;
  t20_4pc_frost = 0;
  eternal_rune_weapon_counter = 0;
}

// death_knight_t::assess_heal ==============================================

void death_knight_t::assess_heal( school_e school, dmg_e t, action_state_t* s )
{
  if ( buffs.vampiric_blood -> up() )
    s -> result_total *= 1.0 + buffs.vampiric_blood -> data().effectN( 1 ).percent();

  player_t::assess_heal( school, t, s );
}

// death_knight_t::assess_damage_imminent ===================================

void death_knight_t::bone_shield_handler( const action_state_t* state ) const
{
  if ( ! buffs.bone_shield -> up() || ! cooldown.bone_shield_icd -> up() || state -> action -> special )
  {
    return;
  }

  if ( sim -> log )
  {
    sim -> out_debug.printf( "%s took a successful auto attack and lost a stack on bone shield", name() );
  }

  buffs.bone_shield -> decrement();
  cooldown.bone_shield_icd -> start( spell.bone_shield -> internal_cooldown() );

  if ( sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T21, B2 ) )
  {
    cooldown.dancing_rune_weapon -> adjust( timespan_t::from_millis( sets -> set( DEATH_KNIGHT_BLOOD, T21, B2) -> effectN( 1 ).base_value() ), false );
  }
  
  if ( ! buffs.bone_shield -> up() && buffs.bones_of_the_damned -> up() )
  {
    buffs.bones_of_the_damned -> expire();
  }
}

void death_knight_t::assess_damage_imminent( school_e school, dmg_e, action_state_t* s )
{
  bone_shield_handler( s );

  if ( school != SCHOOL_PHYSICAL )
  {
    if ( buffs.antimagic_shell -> up() )
    {
      double damage_absorbed = s -> result_amount;

      if ( damage_absorbed > antimagic_shell_remaining_absorb )
      {
        damage_absorbed = antimagic_shell_remaining_absorb;
        buffs.antimagic_shell -> expire();
      }

      // Generates 1 RP for every 1% max hp absorbed
      double rp_generated = damage_absorbed / resources.max[RESOURCE_HEALTH] * 100;

      s -> result_amount -= damage_absorbed;
      s -> result_absorbed -= damage_absorbed;
      s -> self_absorb_amount += damage_absorbed;
      iteration_absorb_taken += damage_absorbed;

      if ( antimagic_shell )
        antimagic_shell -> add_result( damage_absorbed, damage_absorbed, ABSORB, RESULT_HIT, BLOCK_RESULT_UNBLOCKED, this );

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
    active_spells.mark_of_blood -> execute();
  }
}

// death_knight_t::target_mitigation ========================================

void death_knight_t::target_mitigation( school_e school, dmg_e type, action_state_t* state )
{
  if ( buffs.rune_tap -> up() )
    state -> result_amount *= 1.0 + buffs.rune_tap -> data().effectN( 1 ).percent();

  if ( buffs.icebound_fortitude -> up() )
    state -> result_amount *= 1.0 + buffs.icebound_fortitude -> data().effectN( 3 ).percent();

  player_t::target_mitigation( school, type, state );
}

// death_knight_t::composite_base_armor_multiplier ===============================

double death_knight_t::composite_base_armor_multiplier() const
{
  double bam = player_t::composite_base_armor_multiplier();
  return bam;
}

// death_knight_t::composite_armor_multiplier ===============================

double death_knight_t::composite_armor_multiplier() const
{
  double am = player_t::composite_armor_multiplier();

  if ( runeforge.rune_of_the_stoneskin_gargoyle -> check() )
    am *= 1.0 + runeforge.rune_of_the_stoneskin_gargoyle -> data().effectN( 1 ).percent();

  return am;
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
    if ( runeforge.rune_of_the_fallen_crusader -> up() )
    {
      m *= 1.0 + runeforge.rune_of_the_fallen_crusader -> data().effectN( 1 ).percent();
    }

    if ( runeforge.rune_of_the_stoneskin_gargoyle -> check() )
    {
      m *= 1.0 + runeforge.rune_of_the_stoneskin_gargoyle -> data().effectN( 2 ).percent();
    }

    m *= 1.0 + buffs.pillar_of_frost -> value() + buffs.pillar_of_frost_bonus -> stack_value();
  }

  else if ( attr == ATTR_STAMINA )
  {
    m *= 1.0 + spec.veteran_of_the_third_war -> effectN( 1 ).percent() + spec.blood_death_knight -> effectN( 13 ).percent();

    if ( runeforge.rune_of_the_stoneskin_gargoyle -> check() )
    {
      m *= 1.0 + runeforge.rune_of_the_stoneskin_gargoyle -> data().effectN( 2 ).percent();
    }
  }

  return m;
}

// death_knight_t::matching_gear_multiplier =================================

double death_knight_t::matching_gear_multiplier( attribute_e attr ) const
{
  int tree = specialization();

  if ( tree == DEATH_KNIGHT_UNHOLY || tree == DEATH_KNIGHT_FROST )
    if ( attr == ATTR_STRENGTH )
      return spec.plate_specialization -> effectN( 1 ).percent();

  if ( tree == DEATH_KNIGHT_BLOOD )
    if ( attr == ATTR_STAMINA )
      return spec.plate_specialization -> effectN( 1 ).percent();

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
    parry += buffs.dancing_rune_weapon -> data().effectN( 1 ).percent();
    
  return parry;
}

double death_knight_t::composite_player_pet_damage_multiplier( const action_state_t* state ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( state );

  auto school = state -> action -> get_school();
  if ( dbc::is_school( school, SCHOOL_SHADOW ) && mastery.dreadblade -> ok() )
  {
    m *= 1.0 + cache.mastery_value();
  }

  m *= 1.0 + spec.unholy_death_knight -> effectN( 3 ).percent();

  return m;
}

double death_knight_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );
  
  death_knight_td_t* td = get_target_data( target );

  if ( dbc::is_school( school, SCHOOL_FROST ) )
  {
    double debuff = td -> debuff.razorice -> data().effectN( 1 ).percent();
    m *= 1.0 + td -> debuff.razorice -> check() * debuff;
  }

  return m;
}

// death_knight_t::composite_attack_power_multiplier ==================================

double death_knight_t::composite_attack_power_multiplier() const
{
  double m = player_t::composite_attack_power_multiplier();

  m *= 1.0 + mastery.blood_shield -> effectN( 2 ).mastery_value() * composite_mastery();

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

  start_inexorable_assault();
  start_cold_heart();
 
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
  
  if ( buffs.t21_4p_blood -> check() )
  {
    rps *= 1.0 + spell.rune_master -> effectN( 1 ).percent();
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
  
  if ( buffs.t21_4p_blood -> check() )
  {
    coeff /= 1.0 + spell.rune_master -> effectN( 1 ).percent();
  }

  return coeff;
}

// death_knight_t::arise ====================================================

void death_knight_t::arise()
{
  player_t::arise();
  runeforge.rune_of_the_stoneskin_gargoyle -> trigger();
}

void death_knight_t::adjust_dynamic_cooldowns()
{
  player_t::adjust_dynamic_cooldowns();

  _runes.update_coefficient();
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

// We can infer razorice enablement from just one of the associated spells
struct razorice_attack_cb_t : public scoped_actor_callback_t<death_knight_t>
{
  razorice_attack_cb_t() : super( DEATH_KNIGHT )
  { }

  // Create the razorice actions based on handedness
  void manipulate( death_knight_t* p, const special_effect_t& e ) override
  {
    if ( e.item -> slot == SLOT_MAIN_HAND )
    {
      p -> active_spells.razorice_mh = new razorice_attack_t( p, e.name() );
    }
    else if ( e.item -> slot == SLOT_OFF_HAND )
    {
      p -> active_spells.razorice_oh = new razorice_attack_t( p, e.name() );
    }
  }
};

struct death_knight_module_t : public module_t {
  death_knight_module_t() : module_t( DEATH_KNIGHT ) {}

  player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new death_knight_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new death_knight_report_t( *p ) );
    return p;
  }

  void static_init() const override
  {
    unique_gear::register_special_effect(  50401, razorice_attack_cb_t() );
    unique_gear::register_special_effect( 166441, runeforge::fallen_crusader );
    unique_gear::register_special_effect(  62157, runeforge::stoneskin_gargoyle );
  }

  void register_hotfixes() const override
  {
  }

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
