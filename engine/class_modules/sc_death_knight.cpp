// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// TODO:
// Unholy
// - Skelebro has an aoe spell (Arrow Spray), but the AI using it is very inconsistent
// - Army of the dead ghouls should spawn once every 0.5s for 4s rather than all at once
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
  timespan_t time_to_regen( unsigned n_runes ) const;
};

// ==========================================================================
// Death Knight
// ==========================================================================

struct death_knight_td_t : public actor_target_data_t {
  struct
  {
    dot_t* blood_plague;
    dot_t* breath_of_sindragosa;
    dot_t* frost_fever;
    dot_t* outbreak;
    dot_t* soul_reaper;
    dot_t* virulent_plague;
    dot_t* unholy_blight;
  } dot;

  struct
  {
    buff_t* razorice;
    buff_t* festering_wound;
    buff_t* mark_of_blood;
    buff_t* perseverance_of_the_ebon_martyr;
  } debuff;

  death_knight_td_t( player_t* target, death_knight_t* death_knight );
};

struct death_knight_t : public player_t {
public:
  // Active
  double runic_power_decay_rate;
  double fallen_crusader, fallen_crusader_rppm;
  double antimagic_shell_absorbed;
  std::vector<ground_aoe_event_t*> dnds;
  bool deprecated_dnd_expression;

  // Counters
  int t20_2pc_frost;
  int t20_4pc_frost; // Collect RP usage

  stats_t*  antimagic_shell;

  // Buffs
  struct buffs_t {
    buff_t* antimagic_shell;
    haste_buff_t* bone_shield;
    buff_t* crimson_scourge;
    buff_t* dancing_rune_weapon;
    buff_t* dark_transformation;
    buff_t* death_and_decay;
    buff_t* gathering_storm;
    buff_t* icebound_fortitude;
    buff_t* killing_machine;
    buff_t* obliteration;
    buff_t* pillar_of_frost;
    buff_t* rime;
    buff_t* runic_corruption;
    buff_t* sudden_doom;
    buff_t* vampiric_blood;
    buff_t* will_of_the_necropolis;
    buff_t* remorseless_winter;
    buff_t* empower_rune_weapon;
    haste_buff_t* empower_rune_weapon_haste;
    buff_t* t20_2pc_unholy;
    buff_t* t20_4pc_frost;
    buff_t* t20_blood;
    buff_t* t21_4p_blood;
    buff_t* hemostasis;
    buff_t* voracious;

    absorb_buff_t* blood_shield;
    buff_t* rune_tap;
    stat_buff_t* riposte;

    haste_buff_t* icy_talons;
    haste_buff_t* unholy_frenzy;
    haste_buff_t* soul_reaper;
    absorb_buff_t* tombstone;

    buff_t* frozen_pulse;

    stat_buff_t* t19oh_8pc;
    buff_t* skullflowers_haemostasis;
    haste_buff_t* sephuzs_secret;
    buff_t* cold_heart;
    buff_t* toravons;
  } buffs;

  struct runeforge_t {
    buff_t* rune_of_the_fallen_crusader;
    buff_t* rune_of_the_stoneskin_gargoyle;
  } runeforge;

  // Cooldowns
  struct cooldowns_t {
    cooldown_t* antimagic_shell;
    cooldown_t* army_of_the_dead;
    cooldown_t* apocalypse;
    cooldown_t* avalanche;
    cooldown_t* bone_shield_icd;
    cooldown_t* dancing_rune_weapon;
    cooldown_t* dark_transformation;
    cooldown_t* death_and_decay;
    cooldown_t* defile;
    cooldown_t* empower_rune_weapon;
    cooldown_t* frost_fever;
    cooldown_t* icecap;
    cooldown_t* pillar_of_frost;
    cooldown_t* rune_strike;
    cooldown_t* vampiric_blood;
  } cooldown;

  // Active Spells
  struct active_spells_t {
    spell_t* blood_plague;
    spell_t* frost_fever;
    action_t* avalanche;
    action_t* festering_wound;
    action_t* virulent_plague;
    action_t* bursting_sores;
    action_t* mark_of_blood;
    action_t* t20_2pc_unholy;
    action_t* cold_heart;
    action_t* freezing_death;
    action_t* t20_blood;
    action_t* t21_4pc_blood;

    action_t* razorice_mh;
    action_t* razorice_oh;
  } active_spells;

  // Gains
  struct gains_t {
    gain_t* antimagic_shell;
    gain_t* festering_wound;
    gain_t* frost_fever;
    gain_t* horn_of_winter;
    gain_t* murderous_efficiency;
    gain_t* power_refund;
    gain_t* rune;
    gain_t* runic_attenuation;
    gain_t* rc; // Runic Corruption
    gain_t* runic_empowerment;
    gain_t* empower_rune_weapon;
    gain_t* tombstone;
    gain_t* t19_4pc_blood;
    gain_t* draugr_girdle_everlasting_king;
    gain_t* uvanimor_the_unbeautiful;
    gain_t* koltiras_newfound_will;
    gain_t* t19_4pc_frost;
    gain_t* start_of_combat_overflow;
    gain_t* shackles_of_bryndaor;
    gain_t* heartbreaker;
    gain_t* drw_heart_strike;
    gain_t* rune_strike;
    gain_t* soul_reaper;
  } gains;

  // Specialization
  struct specialization_t {
    // Generic
    const spell_data_t* plate_specialization;
    const spell_data_t* death_knight;
    const spell_data_t* unholy_death_knight;
    const spell_data_t* frost_death_knight;
    const spell_data_t* blood_death_knight;
    const spell_data_t* icebound_fortitude;

    // Blood
    const spell_data_t* veteran_of_the_third_war;
    const spell_data_t* riposte;
    const spell_data_t* crimson_scourge;
    const spell_data_t* blood_boil;
    const spell_data_t* dancing_rune_weapon;
    const spell_data_t* deaths_caress;
    const spell_data_t* heart_strike;
    const spell_data_t* marrowrend;
    const spell_data_t* vampiric_blood;

    // Frost
    const spell_data_t* frost_fever;
    const spell_data_t* killing_machine;
    const spell_data_t* runic_empowerment;
    const spell_data_t* rime;
    const spell_data_t* empower_rune_weapon;
    const spell_data_t* pillar_of_frost;
    const spell_data_t* remorseless_winter;

    // Unholy
    const spell_data_t* festering_wound;
    const spell_data_t* runic_corruption;
    const spell_data_t* sudden_doom;
    const spell_data_t* army_of_the_dead;
    const spell_data_t* dark_transformation;
    const spell_data_t* death_coil;
    const spell_data_t* outbreak;
    const spell_data_t* scourge_strike;
    const spell_data_t* apocalypse;

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

    // Tier 1
    const spell_data_t* icy_talons;
    const spell_data_t* runic_attenuation;

    // Tier 2
    const spell_data_t* murderous_efficiency;
    const spell_data_t* horn_of_winter;

    // Tier 3
    const spell_data_t* icecap;
    const spell_data_t* glacial_advance;
    const spell_data_t* avalanche;
    
    // Tier 4 
    const spell_data_t* inexorable_assault; // Not yet implemented
    const spell_data_t* volatile_shielding;

    // Tier 6
    const spell_data_t* frostscythe;
    const spell_data_t* frozen_pulse;
    const spell_data_t* gathering_storm;
    
    // Tier 7
    const spell_data_t* obliteration;
    const spell_data_t* breath_of_sindragosa;
    const spell_data_t* frostwyrms_fury;

    // Unholy

    // Tier 1
    const spell_data_t* infected_claws;
    const spell_data_t* all_will_serve;
    const spell_data_t* clawing_shadows;

    // Tier 2
    const spell_data_t* pestilent_pustules;
    const spell_data_t* harbinger_of_doom;
    const spell_data_t* soul_reaper;

    // Tier 3
    const spell_data_t* bursting_sores;
    const spell_data_t* ebon_fever;
    const spell_data_t* unholy_blight;


    // Tier 6
    const spell_data_t* pestilence;
    const spell_data_t* defile;
    const spell_data_t* epidemic;

    // Tier 7
    const spell_data_t* army_of_the_damned;
    const spell_data_t* unholy_frenzy;
    const spell_data_t* summon_gargoyle;

    // Blood

    // Tier 1
    const spell_data_t* heartbreaker;
    const spell_data_t* blooddrinker;
    const spell_data_t* rune_strike;

    // Tier 2
    const spell_data_t* rapid_decomposition;
    const spell_data_t* hemostasis;
    const spell_data_t* consumption;
    
    // Tier 3
    const spell_data_t* will_of_the_necropolis; // NYI
    const spell_data_t* antimagic_barrier;
    const spell_data_t* rune_tap;

    // Tier 4 Utility tier, NYI
    const spell_data_t* tightening_grasp;
    const spell_data_t* grip_of_the_dead;
    const spell_data_t* march_of_the_damned;

    // Tier 5
    const spell_data_t* foul_bulwark;
    const spell_data_t* ossuary;
    const spell_data_t* tombstone;

    // Tier 6
    const spell_data_t* voracious;
    const spell_data_t* bloodworms;
    const spell_data_t* mark_of_blood; // NYI

    // Tier 7
    const spell_data_t* purgatory; // NYI
    const spell_data_t* red_thirst; 
    const spell_data_t* bonestorm; 
  } talent;

  // Spells
  struct spells_t {
    const spell_data_t* antimagic_shell;
    const spell_data_t* death_strike;

    const spell_data_t* blood_shield;
    const spell_data_t* bone_shield;
    const spell_data_t* dancing_rune_weapon;
    const spell_data_t* gravewarden;
    const spell_data_t* ossuary;
    const spell_data_t* rune_master;

    const spell_data_t* death_coil_damage;
    const spell_data_t* runic_corruption;

  } spell;

  // RPPM
  struct rppm_t
  {
    real_ppm_t* freezing_death;
    real_ppm_t* bloodworms;
    real_ppm_t* runic_attenuation;
  } rppm;

  // Pets and Guardians
  struct pets_t
  {
    std::array< pets::death_knight_pet_t*, 8 > army_ghoul;
    std::array< pets::death_knight_pet_t*, 4 > apoc_ghoul;
    std::array< pets::bloodworm_pet_t*, 6 > bloodworms;
    pets::dancing_rune_weapon_pet_t* dancing_rune_weapon_pet;
    pets::dt_pet_t* ghoul_pet;
    pets::gargoyle_pet_t* gargoyle;
    pet_t* risen_skulker;
  } pets;

  // Procs
  struct procs_t
  {
    proc_t* runic_empowerment;
    proc_t* runic_empowerment_wasted;
    proc_t* oblit_killing_machine;
    proc_t* fs_killing_machine;
    proc_t* ready_rune;
    proc_t* km_natural_expiration;
    proc_t* t19_2pc_unholy;
    proc_t* bloodworms;
    proc_t* pp_runic_corruption;
    proc_t* rp_runic_corruption;
  } procs;

  // Legendaries
  struct legendary_t {
    // Frost
    const spell_data_t* koltiras_newfound_will;
    const spell_data_t* seal_of_necrofantasia;
    const spell_data_t* perseverance_of_the_ebon_martyr;
    double toravons;

    // Blood
    const spell_data_t* lanathels_lament;
    const spell_data_t* soulflayers_corruption;
    const spell_data_t* shackles_of_bryndaor;

    // Unholy
    unsigned the_instructors_fourth_lesson;
    double draugr_girdle_everlasting_king;
    double uvanimor_the_unbeautiful;
    const spell_data_t* death_march;


    // Shared
    const spell_data_t* sephuzs_secret;

    legendary_t() :
      koltiras_newfound_will( spell_data_t::not_found() ),
      seal_of_necrofantasia( spell_data_t::not_found() ),
      perseverance_of_the_ebon_martyr( spell_data_t::not_found() ),
      toravons( 0 ),
      lanathels_lament( spell_data_t::not_found() ),
      soulflayers_corruption( spell_data_t::not_found() ),
      shackles_of_bryndaor( spell_data_t::not_found() ),
      the_instructors_fourth_lesson( 0 ),
      draugr_girdle_everlasting_king( 0 ),
      uvanimor_the_unbeautiful( 0 ),
      death_march( spell_data_t::not_found() ),
      sephuzs_secret( nullptr )
    { }

  } legendary;

  // Runes
  runes_t _runes;

  death_knight_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DEATH_KNIGHT, name, r ),
    runic_power_decay_rate(),
    fallen_crusader( 0 ),
    fallen_crusader_rppm( find_spell( 166441 ) -> real_ppm() ),
    antimagic_shell_absorbed( 0.0 ),
    deprecated_dnd_expression( false ),
    t20_2pc_frost( 0 ),
    t20_4pc_frost( 0 ),
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
    legendary( legendary_t() ),
    _runes( this )
  {
    range::fill( pets.army_ghoul, nullptr );
    range::fill( pets.apoc_ghoul, nullptr );
    range::fill( pets.bloodworms, nullptr );
    
    cooldown.antimagic_shell = get_cooldown( "antimagic_shell" );
    cooldown.army_of_the_dead = get_cooldown( "army_of_the_dead" );
    cooldown.apocalypse = get_cooldown( "apocalypse" );
    cooldown.avalanche = get_cooldown( "avalanche" );
    cooldown.bone_shield_icd = get_cooldown( "bone_shield_icd" );
    cooldown.bone_shield_icd -> duration = timespan_t::from_seconds( 2.5 );
    cooldown.dancing_rune_weapon = get_cooldown( "dancing_rune_weapon" );
    cooldown.dark_transformation = get_cooldown( "dark_transformation" );
    cooldown.death_and_decay = get_cooldown( "death_and_decay" );
    cooldown.defile = get_cooldown( "defile" );
    cooldown.empower_rune_weapon = get_cooldown( "empower_rune_weapon" );
    cooldown.icecap = get_cooldown( "icecap" );
    cooldown.pillar_of_frost = get_cooldown( "pillar_of_frost" );
    cooldown.rune_strike = get_cooldown( "rune_strike" );
    cooldown.vampiric_blood = get_cooldown( "vampiric_blood" );

    talent_points.register_validity_fn( [ this ] ( const spell_data_t* spell )
    {
      // Soul of the Deathlord
      if ( find_item( 151640 ) )
      {
        switch ( specialization() )
        {
          case DEATH_KNIGHT_BLOOD:
            return spell -> id() == 206974; // Foul Bulwark
          case DEATH_KNIGHT_FROST:
            return spell -> id() == 194912; // Gathering Storm
          case DEATH_KNIGHT_UNHOLY:
            return spell -> id() == 207264; // Bursting Sores
         default:
            break;
        }
      }
      return false;
    } );

    regen_type = REGEN_DYNAMIC;
  }

  // Character Definition
  void      init_spells() override;
  void      init_action_list() override;
  bool      init_actions() override;
  void      init_rng() override;
  void      init_base_stats() override;
  void      init_scaling() override;
  void      create_buffs() override;
  void      init_gains() override;
  void      init_procs() override;
  void      init_absorb_priority() override;
  bool      init_finished() override;
  double    composite_armor_multiplier() const override;
  double    composite_bonus_armor() const override;
  double    composite_melee_attack_power() const override;
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
  double    composite_player_multiplier( school_e school ) const override;
  double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double    composite_player_dh_multiplier( school_e school ) const override;
  double    composite_player_th_multiplier( school_e school ) const override;
  double    composite_player_pet_damage_multiplier( const action_state_t* /* state */ ) const override;
  double    composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  double    composite_crit_avoidance() const override;
  double    passive_movement_modifier() const override;
  void      combat_begin() override;
  void      reset() override;
  void      arise() override;
  void      adjust_dynamic_cooldowns() override;
  void      assess_heal( school_e, dmg_e, action_state_t* ) override;
  void      assess_damage( school_e, dmg_e, action_state_t* ) override;
  void      assess_damage_imminent( school_e, dmg_e, action_state_t* ) override;
  void      target_mitigation( school_e, dmg_e, action_state_t* ) override;
  void      do_damage( action_state_t* ) override;
  bool      create_actions() override;
  action_t* create_action( const std::string& name, const std::string& options ) override;
  expr_t*   create_expression( const std::string& name ) override;
  void      create_pets() override;
  void      create_options() override;
  resource_e primary_resource() const override { return RESOURCE_RUNIC_POWER; }
  role_e    primary_role() const override;
  stat_e    primary_stat() const override;
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
  void      trigger_runic_empowerment( double rpcost );
  bool      trigger_runic_corruption( double rpcost, double override_chance = -1.0 );
  void      trigger_festering_wound( const action_state_t* state, unsigned n_stacks = 1 );
  void      burst_festering_wound( const action_state_t* state, unsigned n = 1 );
  void      trigger_death_march( const action_state_t* state );
  void      apply_diseases( action_state_t* state, unsigned diseases );
  double    ready_runes_count( bool fractional ) const;
  void      default_apl_dps_precombat();
  void      default_apl_blood();
  void      default_apl_frost();
  void      default_apl_unholy();
  void      copy_from( player_t* ) override;
  double    bone_shield_handler( const action_state_t* ) const;

  void      trigger_t20_2pc_frost( double consumed );
  void      trigger_t20_4pc_frost( double consumed );
  void      trigger_t20_4pc_unholy( double consumed );

  void      trigger_soul_reaper_death( player_t* );

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
      td = new death_knight_td_t( target, const_cast<death_knight_t*>(this) );
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

inline death_knight_td_t::death_knight_td_t( player_t* target, death_knight_t* death_knight ) :
  actor_target_data_t( target, death_knight )
{
  dot.blood_plague         = target -> get_dot( "blood_plague",         death_knight );
  dot.breath_of_sindragosa = target -> get_dot( "breath_of_sindragosa", death_knight );
  dot.frost_fever          = target -> get_dot( "frost_fever",          death_knight );
  dot.outbreak             = target -> get_dot( "outbreak",             death_knight );
  dot.virulent_plague      = target -> get_dot( "virulent_plague",      death_knight );
  dot.soul_reaper          = target -> get_dot( "soul_reaper",          death_knight );
  dot.unholy_blight        = target -> get_dot( "unholy_blight",        death_knight );

  debuff.razorice          = make_buff( *this, "razorice", death_knight -> find_spell( 51714 ) )
                           -> set_period( timespan_t::zero() );
  debuff.festering_wound   = buff_creator_t( *this, "festering_wound", death_knight -> find_spell( 194310 ) )
                           .trigger_spell( death_knight -> spec.festering_wound )
                           .cd( timespan_t::zero() ); // Handled by trigger_festering_wound
  debuff.mark_of_blood     = buff_creator_t( *this, "mark_of_blood", death_knight -> talent.mark_of_blood )
                           .cd( timespan_t::zero() ); // Handled by the action
  debuff.perseverance_of_the_ebon_martyr = buff_creator_t( *this, "perseverance_of_the_ebon_martyr", death_knight -> find_spell( 216059 ) )
    .chance( death_knight -> legendary.perseverance_of_the_ebon_martyr -> ok() )
    .default_value( death_knight -> find_spell( 216059 ) -> effectN( 1 ).percent() )
    .duration( timespan_t::from_seconds( 5 ) ); //In game testing shows it's around 5 seconds.

  if ( death_knight -> specialization() == DEATH_KNIGHT_UNHOLY && death_knight -> talent.soul_reaper -> ok() )
  {
    target -> callbacks_on_demise.push_back( std::bind( &death_knight_t::trigger_soul_reaper_death, death_knight, std::placeholders::_1 ) );
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
    return l -> event -> remains() < r -> event -> remains();
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

timespan_t runes_t::time_to_regen( unsigned n_runes ) const
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
  std::vector<const rune_t*> regenerating_runes;
  range::for_each( slot, [ &regenerating_runes ]( const rune_t& r ) {
    if ( r.is_regenerating() )
    {
      regenerating_runes.push_back( &( r ) );
    }
  });

  // Sort by ascending remaining time
  range::sort( regenerating_runes, []( const rune_t* l, const rune_t* r ) {
    return l -> event -> remains() < r -> event -> remains();
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

  if ( runes -> dk -> sets -> has_set_bonus( DEATH_KNIGHT_UNHOLY, T19OH, B8 ) ||
       runes -> dk -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T19OH, B8 ) ||
       runes -> dk -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T19OH, B8 ) )
  {
    runes -> dk -> buffs.t19oh_8pc -> trigger();
  }

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
  buff_t* taktheritrix;

  death_knight_pet_t( death_knight_t* owner, const std::string& name, bool guardian = true, bool auto_attack = true ) :
    pet_t( owner -> sim, owner, name, guardian ), use_auto_attack( auto_attack ),
    taktheritrix( nullptr )
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

  double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( taktheritrix )
    {
      m *= 1.0 + taktheritrix -> check_value();
    }

    return m;
  }

  // DK pets do not inherit owner attack speed apparently
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
    if ( target -> is_sleeping() ) return false;
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

  bool usable_in_dt,
       triggers_infected_claws;

  dt_melee_ability_t( T* pet, const std::string& name,
      const spell_data_t* spell = spell_data_t::nil(), const std::string& options = std::string(),
      bool usable_in_dt = true ) :
    pet_melee_attack_t<T>( pet, name, spell, options ),
    usable_in_dt( usable_in_dt ), triggers_infected_claws( false )
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

    this -> p() -> o() -> trigger_festering_wound( state, 1 );
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

  void impact( action_state_t* s ) override
  {
    pet_melee_attack_t<T>::impact( s );

    if ( this -> result_is_hit( s -> result ) )
    {
      if ( ! this -> rng().roll( this -> p() -> o() -> legendary.uvanimor_the_unbeautiful ) )
        return;

      if ( this -> sim -> debug )
      {
        log_rune_status( this -> p() -> o() );
      }

      if ( this -> p() -> o() -> replenish_rune( 1, this -> p() -> o() -> gains.uvanimor_the_unbeautiful ) &&
           this -> sim -> debug )
      {
        this -> sim -> out_debug.printf( "%s uvanimor_the_unbeautiful regenerated rune",
            this -> p() -> o() -> name() );
        log_rune_status( this -> p() -> o() );
      }
    }
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

    if ( r == RESOURCE_ENERGY )
    {
      // Army of the dead and pet ghoul energy regen double dips with haste
      // https://github.com/SimCMinMax/WoW-BugTracker/issues/108
      if ( o() -> bugs )
        reg *= ( 1.0 / cache.attack_haste() );
    }

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

  void impact( action_state_t* s ) override
  {
    auto_attack_melee_t<T>::impact( s );

    /*
    if ( this -> result_is_hit( s -> result ) )
    {
      if ( ! this -> rng().roll( this -> p() -> o() -> legendary.uvanimor_the_unbeautiful ) )
        return;

      if ( this -> sim -> debug )
      {
        log_rune_status( this -> p() -> o() );
      }

      if ( this -> p() -> o() -> replenish_rune( 1, this -> p() -> o() -> gains.uvanimor_the_unbeautiful ) &&
           this -> sim -> debug )
      {
        this -> sim -> out_debug.printf( "%s uvanimor_the_unbeautiful regenerated rune",
            this -> p() -> o() -> name() );
        log_rune_status( this -> p() -> o() );
      }
    }
    */
  }
};

struct dt_pet_t : public base_ghoul_pet_t
{
  struct cooldowns_t
  {
    cooldown_t* gnaw_smash; // shared cd between gnaw/smash and their DT'd counterparts
  } cooldown;

  dt_pet_t( death_knight_t* owner, const std::string& name ) :
    base_ghoul_pet_t( owner, name, false )
  {
    cooldown.gnaw_smash = get_cooldown( "gnaw_smash" );
    cooldown.gnaw_smash -> duration = find_spell( 91800 ) -> cooldown();
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
      super( player, "claw", player -> find_spell( 91776 ), options_str, false )
    { triggers_infected_claws = true; }

    double action_multiplier() const override
    {
      double m = super::action_multiplier();

      if ( p() -> o() -> mastery.dreadblade -> ok() )
      {
        m *= 1.0 + p() -> o() -> cache.mastery_value();
      }

      return m;
    }
  };

  struct sweeping_claws_t : public dt_melee_ability_t<ghoul_pet_t>
  {
    sweeping_claws_t( ghoul_pet_t* player, const std::string& options_str ) :
      super( player, "sweeping_claws", player -> find_spell( 91778 ), options_str )
    {
      aoe = -1; // TODO: Nearby enemies == all now?
      triggers_infected_claws = true;
    }
  };

  struct gnaw_t : public dt_melee_ability_t<ghoul_pet_t>
  {
    gnaw_t( ghoul_pet_t* player, const std::string& options_str ) :
      super( player, "gnaw", player -> find_spell( 91800 ), options_str, false )
    {
      cooldown = player -> get_cooldown( "gnaw_smash" );
    }
  };

  struct monstrous_blow_t : public dt_melee_ability_t<ghoul_pet_t>
  {
    monstrous_blow_t( ghoul_pet_t* player, const std::string& options_str ):
      super( player, "monstrous_blow", player -> find_spell( 91797 ), options_str )
    {
      cooldown = player -> get_cooldown( "gnaw_smash" );
    }
  };

  ghoul_pet_t( death_knight_t* owner ) : dt_pet_t( owner, "ghoul" )
  { }

  void init_base_stats() override
  {
    dt_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = .8;
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
  struct army_claw_t : public pet_melee_attack_t<army_pet_t>
  {
    army_claw_t( army_pet_t* player, const std::string& options_str ) :
      super( player, "claw", player -> find_spell( 91776 ), options_str )
    { }
  };

  army_pet_t( death_knight_t* owner, const std::string& name ) :
    base_ghoul_pet_t( owner, name, true )
  { }

  void init_base_stats() override
  {
    base_ghoul_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 0.30;
    // 2017-01-10: Army of the Dead and apoc ghouls damage has been increased.
    // FIXME: Exact number TBDiscovered, 33% sounds fine
    owner_coeff.ap_from_ap *= 1.33;
  }

  void init_action_list() override
  {
    base_ghoul_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "Claw" );
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
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
  buff_t* shadow_empowerment;

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
      super( player, "gargoyle_strike", player -> find_spell( 51963 ), options_str )
    { }
  };

  gargoyle_pet_t( death_knight_t* owner ) : 
    death_knight_pet_t( owner, "gargoyle", true, false ), shadow_empowerment( nullptr )
  { regen_type = REGEN_DISABLED; }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 1;
  }

  double composite_player_multiplier( school_e s ) const override
  {
    double m = death_knight_pet_t::composite_player_multiplier( s );

    m *= 1.0 + shadow_empowerment -> stack_value();

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

    shadow_empowerment = buff_creator_t( this, "shadow_empowerment", find_spell( 211947 ) );
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

    double increase = rp_spent * shadow_empowerment -> data().effectN( 1 ).percent() / 2;

    if ( ! shadow_empowerment -> check() )
    {
      shadow_empowerment -> trigger( 1, increase );
    }
    else
    {
      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s increasing shadow_empowerment power by %f", name(), increase );
      }
      shadow_empowerment -> current_value += increase;
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
      super( player, "skulker_shot", player -> find_spell( 212423 ), options_str )
    {
      weapon = &( player -> main_hand_weapon );

      // 2016-08-06 Hotfixed to do aoe splash damage, splash multiplier currently not in spell data
      aoe = -1;
      base_aoe_multiplier = 0.5;

      // Approximate the Skulker bro's lag as 400ms mean with 50ms standard deviation. This roughly
      // matches in game behavior around 2016-07-22.
      // 2016-08-06 AI Lag is now gone
      //ability_lag = timespan_t::from_millis( 400 );
      //ability_lag_stddev = timespan_t::from_millis( 50 );
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
    owner_coeff.ap_from_ap = 2.0 * 1.15;
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
      drw_spell_t( p, "blood_plague", p -> o() -> find_spell( 55078 ) ) 
    {
      base_multiplier *= 1.0 + p -> o() -> spec.blood_death_knight -> effectN( 2 ).percent();
      base_multiplier *= 1.0 + p -> o() -> legendary.soulflayers_corruption -> effectN( 1 ).percent();

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

    void execute() override
    {
      drw_spell_t::execute();

      if ( result_is_hit( execute_state -> result ) )
        p() -> o() -> buffs.skullflowers_haemostasis -> trigger();
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
      aoe = 2;
      base_multiplier *= 1.0 + p -> o() -> spec.blood_death_knight -> effectN( 1 ).percent();
    }

    int n_targets() const override
    {
      return p() -> o() -> in_death_and_decay() ? 5 : 2;
    }
    
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
    death_knight_pet_t( owner, "dancing_rune_weapon", true, true )
  {
    main_hand_weapon.type       = WEAPON_BEAST_2H;
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
    death_knight_pet_t( owner, "bloodworm", true, true)
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.4 );

    owner_coeff.ap_from_ap = 0.3; // TODO : once properly implemented and armor is fixed, figure out the actual value. Close enough for now
    regen_type = REGEN_DISABLED;
  }

  attack_t* create_auto_attack() override
  { return new auto_attack_melee_t<bloodworm_pet_t>( this ); }
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

  death_knight_action_t( const std::string& n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    action_base_t( n, p, s ), gain( nullptr ), hasted_gcd( false )
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
      // T19 2P BDK
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

    p() -> trigger_t20_2pc_frost( this -> last_resource_cost );

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
      p() -> buffs.pillar_of_frost -> current_value += last_resource_cost / 100;
      p() -> invalidate_cache( CACHE_STRENGTH );
    }

    if ( this -> base_costs[ RESOURCE_RUNIC_POWER ] > 0 && this -> last_resource_cost > 0 )
    {
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

      p() -> trigger_t20_2pc_frost( this -> last_resource_cost );
    }
  }

  bool init_finished() override
  {
    bool ret = action_base_t::init_finished();

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

    return ret;
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

  void consume_resource() override;
  void execute() override;
  void schedule_travel( action_state_t* state ) override;
  void impact( action_state_t* state ) override;
  bool ready() override;

  void consume_killing_machine( const action_state_t* state, proc_t* proc ) const;
  void trigger_icecap( const action_state_t* state ) const;
  void trigger_avalanche( const action_state_t* state ) const;
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

  virtual void   consume_resource() override;
  virtual void   execute() override;
  virtual void   impact( action_state_t* state ) override;

  virtual double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = base_t::composite_da_multiplier( state );

    return m;
  }

  void trigger_freezing_death( const action_state_t* state ) const;

};

struct death_knight_heal_t : public death_knight_action_t<heal_t>
{
  death_knight_heal_t( const std::string& n, death_knight_t* p,
                       const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
  }
};

// ==========================================================================
// Triggers
// ==========================================================================

// ==========================================================================
// Death Knight Attack Methods
// ==========================================================================

// death_knight_melee_attack_t::consume_resource() ==========================

void death_knight_melee_attack_t::consume_resource()
{
  base_t::consume_resource();
}

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

  trigger_avalanche( state );
  trigger_freezing_death( state );
}

// death_knight_melee_attack_t::ready() =====================================

bool death_knight_melee_attack_t::ready()
{
  return base_t::ready();
}

// death_knight_melee_attack_t::consume_killing_machine() ===================

void death_knight_melee_attack_t::consume_killing_machine( const action_state_t* state, proc_t* proc ) const
{
  if ( ! result_is_hit( state -> result ) )
  {
    return;
  }

  bool killing_machine_consumed = false;
  if ( p() -> buffs.killing_machine -> check() )
  {
    proc -> occur();
  }

  killing_machine_consumed = p() -> buffs.killing_machine -> check() > 0;
  p() -> buffs.killing_machine -> decrement();

  if ( killing_machine_consumed &&
       rng().roll( p() -> talent.murderous_efficiency -> effectN( 1 ).percent() ) )
  {
    // TODO: Spell data the number of runes
    p() -> replenish_rune( 1, p() -> gains.murderous_efficiency );
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

  if ( p() -> cooldown.icecap -> down() )
  {
    return;
  }

  p() -> cooldown.pillar_of_frost -> adjust( timespan_t::from_seconds(
    - p() -> talent.icecap -> effectN( 1 ).base_value() / 10.0 ) );

  p() -> cooldown.icecap -> start( p() -> talent.icecap -> internal_cooldown() );
}

// death_knight_melee_attack_t::trigger_avalanche() ========================

void death_knight_melee_attack_t::trigger_avalanche( const action_state_t* state ) const
{
  if ( state -> result != RESULT_CRIT || proc || ! callbacks )
  {
    return;
  }

  if ( ! p() -> talent.avalanche -> ok() )
  {
    return;
  }

  if ( ! p() -> buffs.pillar_of_frost -> up() )
  {
    return;
  }

  if ( p() -> cooldown.avalanche -> down() )
  {
    return;
  }

  p() -> active_spells.avalanche -> set_target( state -> target );
  p() -> active_spells.avalanche -> schedule_execute();

  p() -> cooldown.avalanche -> start( p() -> talent.avalanche -> internal_cooldown() );
}

// death_knight_melee_attack_t::trigger_freezing_death ===================

void death_knight_melee_attack_t::trigger_freezing_death (const action_state_t* state ) const
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

// death_knight_spell_t::trigger_razorice ===================================

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


// death_knight_spell_t::consume_resource() =================================

void death_knight_spell_t::consume_resource()
{
  base_t::consume_resource();
}

// death_knight_spell_t::execute() ==========================================

void death_knight_spell_t::execute()
{
  base_t::execute();
}

// death_knight_spell_t::impact() ===========================================

void death_knight_spell_t::impact( action_state_t* state )
{
  base_t::impact( state );
  trigger_freezing_death( state );
}

// death_knight_spell_t::trigger_freezing_death ===================

void death_knight_spell_t::trigger_freezing_death (const action_state_t* state ) const
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

// Avalanche ===============================================================

struct avalanche_t : public death_knight_spell_t
{
  avalanche_t( death_knight_t* player ) :
    death_knight_spell_t( "avalanche", player, player -> talent.avalanche -> effectN( 1 ).trigger() )
  {
    aoe = -1;
    background = true;
  }
};

// Festering Wound ==========================================================

struct festering_wound_t : public death_knight_spell_t
{
  const spell_data_t* energize;
  festering_wound_t( death_knight_t* p ) :
    death_knight_spell_t( "festering_wound", p, p -> find_spell( 194311 ) ),
    energize( p -> find_spell( 195757 ) )
  {
    background = true;

    base_multiplier *= 1.0 + p -> talent.bursting_sores -> effectN( 1 ).percent();
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    player -> resource_gain( RESOURCE_RUNIC_POWER,
                             energize -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER ),
                             p() -> gains.festering_wound, this );
  }
};

// Bursting Sores ==========================================================

struct bursting_sores_t : public death_knight_spell_t
{
  bursting_sores_t( death_knight_t* p ) :
    death_knight_spell_t( "bursting_sores", p, p -> find_spell( 207267 ) )
  {
    background = true;
    aoe = -1;
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

// Cold Heart damage spell
struct cold_heart_damage_t : public death_knight_spell_t
{
  cold_heart_damage_t( death_knight_t* p ) :
    death_knight_spell_t( "cold_heart", p, p -> find_spell( 248397 ) )
  {
    background = true;
  }

  double action_multiplier() const override
  {
    double m = death_knight_spell_t::action_multiplier();

    m *= 1.0 + ( p() -> buffs.cold_heart -> stack() - 1 );

    return m;
  }
};

// Freezing Death -- Frost's T21 4P damage proc
struct freezing_death_t : public death_knight_melee_attack_t
{
  freezing_death_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "freezing_death", p, p -> find_spell( 253590 ) )
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
    trigger_gcd       = timespan_t::zero();
    special           = false;
    weapon_multiplier = 1.0;

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

    if ( p() -> talent.runic_attenuation -> ok() )
    {
      if ( p() -> rppm.runic_attenuation -> trigger() )
      {
        p() -> resource_gain( RESOURCE_RUNIC_POWER,
                              p() -> talent.runic_attenuation -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
                              p() -> gains.runic_attenuation, this );
      }
    }

    if ( result_is_hit( s -> result ) )
    {
      p() -> buffs.sudden_doom -> trigger();

      if ( s -> result == RESULT_CRIT )
      {
        p() -> buffs.killing_machine -> trigger();
      }

      if ( weapon && p() -> buffs.frozen_pulse -> up() )
      {
        frozen_pulse -> set_target( s -> target );
        frozen_pulse -> schedule_execute();
      }

      if ( p() -> buffs.unholy_frenzy -> up() )
      {
        p() -> trigger_festering_wound( s, 1 );
      }

      // TODO: Why doesn't Crimson Scourge proc while DnD is pulsing?
      if ( td( s -> target ) -> dot.blood_plague -> is_ticking() && p() -> dnds.size() == 0 )
      {
        if ( p() -> buffs.crimson_scourge -> trigger() )
        {
          p() -> cooldown.death_and_decay -> reset( true );
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
    if ( target -> is_sleeping() )
    {
      return false;
    }

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
  const spell_data_t* summon;

  apocalypse_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "apocalypse", p, p -> spec.apocalypse ),
    summon( p -> find_spell( 221180 ) )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();
    auto n_wounds = std::min( (int) data().effectN( 2 ).base_value(),
                              td( execute_state -> target ) -> debuff.festering_wound -> stack() );

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> burst_festering_wound( execute_state, n_wounds );

      for ( int i = 0; i < n_wounds; ++i )
      {
        p() -> pets.apoc_ghoul[ i ] -> summon( summon -> duration() );
        p() -> buffs.t20_2pc_unholy -> trigger();
      }
    }
  }

  virtual bool ready() override
  {
    death_knight_td_t* td = p() -> get_target_data( target );
    if ( ! td -> debuff.festering_wound -> check() )
      return false;

    return death_knight_melee_attack_t::ready();
  }
};

// Army of the Dead =========================================================

struct army_of_the_dead_t : public death_knight_spell_t
{
  army_of_the_dead_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "army_of_the_dead", p, p -> spec.army_of_the_dead )
  {
    parse_options( options_str );

    harmful = false;
  }

  void schedule_execute( action_state_t* s ) override
  {
    death_knight_spell_t::schedule_execute( s );
  }

  // Army of the Dead should always cost resources
  double cost() const override
  { return base_costs[ RESOURCE_RUNE ]; }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( ! p() -> in_combat )
    {
      double precombat_army = 6.0;
      timespan_t precombat_time = timespan_t::from_seconds( precombat_army );
      timespan_t army_duration = p() -> spec.army_of_the_dead -> effectN( 1 ).trigger() -> duration();

      // If used during precombat, army is casted around 6s before the fight begins
      // so you don'twaste rune regen and enter the fight depleted.
      // The time you get for ghouls is 4-6 seconds less.
      for ( int i = 0; i < 8; i++ )
      {
        p() -> pets.army_ghoul[ i ] -> summon( army_duration - precombat_time );
        p() -> buffs.t20_2pc_unholy -> trigger();
      }

      p() -> buffs.t20_2pc_unholy -> extend_duration( p(), - precombat_time );
      p() -> cooldown.army_of_the_dead -> adjust( - precombat_time, false );

      //simulate RP decay for that 6 seconds
      p() -> resource_loss( RESOURCE_RUNIC_POWER, p() -> runic_power_decay_rate * precombat_army, nullptr, nullptr );

      // Simulate rune regeneration for 6 seconds
      p() -> _runes.regenerate_immediate( precombat_time );
    }
    else
    {
      for ( int i = 0; i < 8; i++ )
      {
        p() -> pets.army_ghoul[ i ] -> summon( p() -> spec.army_of_the_dead -> effectN( 1 ).trigger() -> duration() );
        p() -> buffs.t20_2pc_unholy -> trigger();
      }
    }
  }

  virtual bool ready() override
  {
    if ( p() -> pets.army_ghoul[ 0 ] && ! p() -> pets.army_ghoul[ 0 ] -> is_sleeping() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Diseases =================================================================

struct disease_t : public death_knight_spell_t
{
  disease_t( death_knight_t* p, const std::string& name, unsigned spell_id ) :
    death_knight_spell_t( name, p, p -> find_spell( spell_id ) )
  {
    tick_may_crit    = true;
    background       = true;
    may_miss         = false;
    may_crit         = false;
    hasted_ticks     = false;
  }
};

// Blood Plague =============================================================

struct blood_plague_t : public disease_t
{
  blood_plague_t( death_knight_t* p ) :
    disease_t( p, "blood_plague", 55078 )
  {
    base_tick_time *= 1.0 + p -> talent.rapid_decomposition -> effectN( 1 ).percent();
  }

  double action_multiplier() const override
  {
    double m = disease_t::action_multiplier();

    m *= 1.0 + p() -> legendary.soulflayers_corruption -> effectN( 1 ).percent();

    return m;
  }
};

// Frost Fever ==============================================================

struct frost_fever_t : public disease_t
{
  double rp_amount;

  frost_fever_t( death_knight_t* p ) : disease_t( p, "frost_fever", 55095 )
  {
    ap_type = AP_WEAPON_BOTH;

    p -> cooldown.frost_fever = p -> get_cooldown( "frost_fever" );
    p -> cooldown.frost_fever -> duration = p -> spec.frost_fever -> internal_cooldown();
  }

  void tick( dot_t* d ) override
  {
    // Roll for energize.
    energize_amount = rng().roll( p() -> spec.frost_fever -> proc_chance() ) ?
      rp_amount : 0;

    disease_t::tick( d );

    if ( p() -> cooldown.frost_fever -> up() &&
         rng().roll( p() -> spec.frost_fever -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER,
          p() -> spec.frost_fever -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
          p() -> gains.frost_fever,
          this );
      p() -> cooldown.frost_fever -> start();
    }
  }
};

// Virulent Plague ==========================================================

struct virulent_plague_explosion_t : public death_knight_spell_t
{
  virulent_plague_explosion_t( death_knight_t* p ) :
    death_knight_spell_t( "virulent_eruption", p, p -> find_spell( 191685 ) )
  {
    background = split_aoe_damage = true;
    aoe = -1;
  }
};

struct virulent_plague_t : public disease_t
{
  virulent_plague_explosion_t* explosion;

  virulent_plague_t( death_knight_t* p ) :
    disease_t( p, "virulent_plague", 191587 ),
    explosion( new virulent_plague_explosion_t( p ) )
  {
    base_tick_time *= 1.0 + p -> talent.ebon_fever -> effectN( 1 ).percent();
    dot_duration *= 1.0 + p -> talent.ebon_fever -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> talent.ebon_fever -> effectN( 3 ).percent();

    add_child( explosion );
  }

  void tick( dot_t* dot ) override
  {
    disease_t::tick( dot );

    if ( rng().roll( data().effectN( 2 ).percent() ) )
    {
      explosion -> set_target( dot -> target );
      explosion -> schedule_execute();
    }
  }
};

// Blood Boil ==============================================================

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

    if ( result_is_hit( execute_state -> result ) )
      p() -> buffs.skullflowers_haemostasis -> trigger();
  }
  
  void impact( action_state_t* state ) override		
  {		
    death_knight_spell_t::impact( state );		
 		
    if ( result_is_hit( state -> result ) )		
    {		
      p() -> apply_diseases( state, DISEASE_BLOOD_PLAGUE );		
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

struct bonestorm_aoe_t : public death_knight_spell_t
{
  bonestorm_heal_t* heal;

  bonestorm_aoe_t( death_knight_t* p, bonestorm_heal_t* heal ) :
    death_knight_spell_t( "bonestorm_damage", p, p -> talent.bonestorm -> effectN( 3 ).trigger() ),
    heal( heal )
  {
    background = true;
    aoe = -1;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      heal -> execute();
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
    tick_action = new bonestorm_aoe_t( p, heal );
    add_child( heal );
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  { return base_tick_time * last_resource_cost / 10; }
};

// Chains of Ice ============================================================

struct chains_of_ice_t : public death_knight_spell_t
{
  const spell_data_t* pvp_bonus;

  chains_of_ice_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "chains_of_ice", p, p -> find_class_spell( "Chains of Ice" ) ),
    pvp_bonus( p -> find_spell( 62459 ) )
  {
    parse_options( options_str );

    for ( size_t i = 0, end = sizeof_array( p -> items[ SLOT_HANDS ].parsed.data.id_spell ); i < end; i++ )
    {
      if ( p -> items[ SLOT_HANDS ].parsed.data.id_spell[ i ] == static_cast<int>( pvp_bonus -> id() ) )
      {
        energize_type     = ENERGIZE_ON_HIT;
        energize_resource = RESOURCE_RUNIC_POWER;
        energize_amount   = pvp_bonus -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER );
        break;
      }
    }
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

// Dancing Rune Weapon ======================================================

//T21 4P buff
struct rune_master_buff_t : public buff_t
{
  rune_master_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "rune_master", p -> spell.rune_master )
      .trigger_spell( p -> spell.rune_master ).affects_regen( true )
      .stack_change_callback( [ p ]( buff_t*, int, int ) { p -> _runes.update_coefficient(); } ) )
  { }
};


// DRW buff
struct dancing_rune_weapon_buff_t : public buff_t
{
  dancing_rune_weapon_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "dancing_rune_weapon", p -> spell.dancing_rune_weapon )
      .duration( p -> spell.dancing_rune_weapon -> duration() )
      .cd( timespan_t::zero() )
      .add_invalidate( CACHE_PARRY ) )
  { }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    death_knight_t* p = debug_cast< death_knight_t* >( player );

    // Triggers Rune Master buff if T21 4P is equipped
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
    p() -> pets.dancing_rune_weapon_pet -> summon( timespan_t::from_seconds( p() -> spec.dancing_rune_weapon -> effectN( 4 ).base_value() ) );
  }
};

// Dark Command =======================================================================

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
   
    trigger_taktheritrixs_command();
  }

  bool ready() override
  {
    if ( ! p() -> pets.ghoul_pet || p() -> pets.ghoul_pet -> is_sleeping() )
    {
      return false;
    }

    return death_knight_spell_t::ready();
  }

  void trigger_taktheritrixs_command()
  {
    if ( p() -> pets.gargoyle && p() -> pets.gargoyle -> taktheritrix )
    {
      p() -> pets.gargoyle -> taktheritrix -> trigger();
    }

    for ( size_t i = 0; i < p() -> pets.army_ghoul.size(); i++ )
    {
      if ( p() -> pets.army_ghoul[ i ] && p() -> pets.army_ghoul[ i ] -> taktheritrix )
      {
        p() -> pets.army_ghoul[ i ] -> taktheritrix -> trigger();
      }
    }

    for ( size_t i = 0; i < p() -> pets.apoc_ghoul.size(); i++ )
    {
      if ( p() -> pets.apoc_ghoul[ i ] &&
           p() -> pets.apoc_ghoul[ i ] -> taktheritrix )
      {
        p() -> pets.apoc_ghoul[ i ] -> taktheritrix -> trigger();
      }
    }
  }
};

// Death and Decay and Defile ==============================================

struct death_and_decay_damage_base_t : public death_knight_spell_t
{
  action_t* parent;

  death_and_decay_damage_base_t( death_knight_spell_t* parent_,
                                 const std::string& name, const spell_data_t* spell ) :
    death_knight_spell_t( name, parent_ -> p(), spell ),
    parent( parent_ )
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
        p() -> trigger_festering_wound( s, 1 );
      }
    }
  }
};

struct death_and_decay_damage_t : public death_and_decay_damage_base_t
{
  death_and_decay_damage_t( death_knight_spell_t* parent ) :
    death_and_decay_damage_base_t( parent, "death_and_decay_damage", parent -> player -> find_spell( 52212 ) )
  { }
};

struct defile_damage_t : public death_and_decay_damage_base_t
{
  defile_damage_t( death_knight_spell_t* parent ) :
    death_and_decay_damage_base_t( parent, "defile_damage", parent -> player -> find_spell( 156000 ) )
  { }
};

struct death_and_decay_base_t : public death_knight_spell_t
{
  action_t* damage;

  death_and_decay_base_t( death_knight_t* p, const std::string& name, const spell_data_t* spell ) :
    death_knight_spell_t( name, p, spell ), damage( nullptr )
  {
    base_tick_time        = timespan_t::zero();
    dot_duration          = timespan_t::zero();
    ignore_false_positive = true;
    may_crit              = false;
    // Note, radius and ground_aoe flag needs to be set in base so spell_targets expression works
    ground_aoe            = true;
    radius                = data().effectN( 1 ).radius_max();

    // Blood has a lower cd on DnD
    cooldown -> duration += cooldown -> duration * p -> spec.blood_death_knight -> effectN( 5 ).percent();
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
    death_and_decay_base_t( p, "death_and_decay", p -> find_specialization_spell( "Death and Decay" ) )
  {
    damage = new death_and_decay_damage_t( this );

    parse_options( options_str );
  }

  void execute() override
  {
    death_and_decay_base_t::execute();

    p() -> buffs.death_and_decay -> trigger();
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

// Death's Caress =========================================================

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

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      p() -> apply_diseases( execute_state, DISEASE_BLOOD_PLAGUE );
  }
};

// Death Coil ===============================================================

// Unholy T21 2P
struct coils_of_devastation_t 
  : public residual_action::residual_periodic_action_t<death_knight_spell_t>
{
  coils_of_devastation_t( death_knight_t* p ) :
    residual_action::residual_periodic_action_t<death_knight_spell_t>
    ( "coils_of_devastation" , p, p -> find_spell( 253367 ) )
  {
    background = true;
    may_miss = may_crit = false;
  } 
};

// Unholy T21 4P
// Procs some mechanics of Death Coil but not all of them
// Replicates Dark infusion, T21 2P, Death March and now (2017-12-23) T21 4P (can proc off itself) and Runic Corruption
// Doesn't replicate DA Empowerment
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
      p() -> trigger_runic_corruption( base_costs[ RESOURCE_RUNIC_POWER ] );
    }

    // Reduces the cooldown Dark Transformation by 1s, +3s if Dark Infusion is talented

    p() -> cooldown.dark_transformation -> adjust( - timespan_t::from_seconds(
        p() -> spec.death_coil -> effectN( 2 ).base_value() ) );

    p() -> trigger_death_march( execute_state );
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    // Can't happen ingame but if anyone wants to have fun combining T21 4P and T19 4P, might as well let them
    if ( rng().roll( player -> sets -> set( DEATH_KNIGHT_UNHOLY, T19, B4 ) -> effectN( 1 ).percent() ) )
    {
      p() -> trigger_festering_wound( state, 1 );
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

// TODO: Conveert to mimic blizzard spells
struct death_coil_t : public death_knight_spell_t
{
  coils_of_devastation_t* coils_of_devastation;
  t21_death_coil_t* t21_death_coil;
  
  death_coil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "death_coil", p, p -> spec.death_coil ),
    coils_of_devastation( nullptr ),
    t21_death_coil( nullptr )
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
    
    
    // Sudden Doomed Death Coils buff Gargoyle
    if ( p() -> buffs.sudden_doom -> check() && p() -> pets.gargoyle )
    {
      p() -> pets.gargoyle -> increase_power( base_costs[ RESOURCE_RUNIC_POWER ] );
    }

    

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> trigger_runic_corruption( base_costs[ RESOURCE_RUNIC_POWER ] );
    }

    // Reduces the cooldown Dark Transformation by 1s, +3s if Dark Infusion is talented

    p() -> cooldown.dark_transformation -> adjust( -timespan_t::from_seconds(
      p() -> spec.death_coil -> effectN( 2 ).base_value() ) );

    p() -> cooldown.apocalypse -> adjust( -timespan_t::from_seconds( 
      p() -> talent.army_of_the_damned -> effectN( 1 ).base_value() / 10 ) );

    p() -> cooldown.army_of_the_dead -> adjust( -timespan_t::from_seconds( 
      p() -> talent.army_of_the_damned -> effectN( 2 ).base_value() / 10 ) );

    p() -> trigger_death_march( execute_state );
    
    p() -> buffs.sudden_doom -> decrement();
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );
    
    if ( rng().roll( player -> sets -> set( DEATH_KNIGHT_UNHOLY, T19, B4 ) -> effectN( 1 ).percent() ) )
    {
      p() -> trigger_festering_wound( state, 1 );
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

struct death_strike_offhand_t : public death_knight_melee_attack_t
{
  death_strike_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "death_strike_offhand", p, p -> find_spell( 66188 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
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
  const spell_data_t* ds_data;
  blood_shield_t* blood_shield;
  timespan_t interval;

  death_strike_heal_t( death_knight_t* p ) :
    death_knight_heal_t( "death_strike_heal", p, p -> find_spell( 45470 ) ),
    ds_data( p -> spell.death_strike ),
    blood_shield( p -> specialization() == DEATH_KNIGHT_BLOOD ? new blood_shield_t( p ) : nullptr ),
    interval( timespan_t::from_seconds( ds_data -> effectN( 4 ).base_value() ) )
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
    return std::max( player -> resources.max[ RESOURCE_HEALTH ] * ds_data -> effectN( 3 ).percent(),
      player -> compute_incoming_damage( interval ) * ds_data -> effectN( 2 ).percent() );
  }

  double base_da_max( const action_state_t* ) const override
  {
    return std::max( player -> resources.max[ RESOURCE_HEALTH ] * ds_data -> effectN( 3 ).percent(),
      player -> compute_incoming_damage( interval ) * ds_data -> effectN( 2 ).percent() );
  }

  double action_multiplier() const override
  {
    double m = death_knight_heal_t::action_multiplier();

    m *= 1.0 + p() -> buffs.skullflowers_haemostasis -> stack_value();
    m *= 1.0 + p() -> buffs.hemostasis -> stack_value();

    return m;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_heal_t::impact( state );

    // RP refund from shackles of bryndaor
    // DS heals for a minimum of 10% of your max hp and is modified by vers
    // Shackles require the heal from DS to be above 10% of your max hp, before overheal.
    // That means any point in versa automatically validates the trait
    if ( state -> result_total > player -> resources.max[ RESOURCE_HEALTH ] * p() -> legendary.shackles_of_bryndaor -> effectN( 2 ).percent() )
    {
      // Last resource cost doesn't return anything so we have to get the cost of Death Strike
      double c = p() -> find_action( "death_strike" ) -> cost();
          
      // T20 4P doesn't actually reduce shackles of bryndaor's rp refund, even though it reduces the cost so we're adding it to the base cost
      if ( p() -> buffs.t20_blood -> check() && p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T20, B4 ) )
      {
        c -= p() -> spell.gravewarden -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER );
      }

      p() -> resource_gain( RESOURCE_RUNIC_POWER, p() -> legendary.shackles_of_bryndaor -> effectN( 1 ).percent() * c,
      p() -> gains.shackles_of_bryndaor, this );
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

    if ( sim -> debug )
      sim -> out_debug.printf( "%s Blood Shield buff trigger, old_value=%f added_value=%f new_value=%f",
                     player -> name(), current_value,
                     state -> result_amount * p() -> cache.mastery_value(),
                     amount );

    blood_shield -> base_dd_min = blood_shield -> base_dd_max = amount;
    blood_shield -> execute();
  }
};

struct death_strike_t : public death_knight_melee_attack_t
{
  death_strike_offhand_t* oh_attack;
  death_strike_heal_t* heal;

  death_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "death_strike", p, p -> spell.death_strike ),
    oh_attack( nullptr ), heal( new death_strike_heal_t( p ) )
  {
    parse_options( options_str );
    may_parry = false;
    base_multiplier *= 1.0 + p -> spec.blood_death_knight -> effectN( 3 ).percent();
    
    weapon = &( p -> main_hand_weapon );
  }

  double action_multiplier() const override
  {
    double m = death_knight_melee_attack_t::action_multiplier();

    m *= 1.0 + p() -> buffs.skullflowers_haemostasis -> stack_value();
    m *= 1.0 + p() -> buffs.hemostasis -> stack_value();

    return m;
  }

  double cost() const override
  {
    double c = death_knight_melee_attack_t::cost();

    if ( p() -> talent.ossuary -> ok() &&
         p() -> buffs.bone_shield -> stack() >= p() -> talent.ossuary -> effectN( 1 ).base_value() )
    {
      c += p() -> spell.ossuary -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER );
    }
    
    if ( p() -> buffs.t20_blood -> check() && p() -> sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T20, B4 ) )
    {
      c += p() -> spell.gravewarden -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER );
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

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> spec.runic_empowerment )
      {
        p() -> trigger_runic_empowerment( base_costs[ RESOURCE_RUNIC_POWER ] );
      }
      else if ( p() -> spec.runic_corruption )
      {
        p() -> trigger_runic_corruption( base_costs[ RESOURCE_RUNIC_POWER ] );
      }
    }

    p() -> buffs.icy_talons -> trigger();
    p() -> trigger_death_march( execute_state );
    p() -> buffs.skullflowers_haemostasis -> expire();
    p() -> buffs.hemostasis -> expire();
  }

  bool ready() override
  {
    if ( ! melee_attack_t::ready() )
      return false;

    //return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
    //TODO:mrdmnd
    return true;
  }
};

// Empower Rune Weapon ======================================================

struct empower_rune_weapon_buff_t : public buff_t
{
  empower_rune_weapon_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "empower_rune_weapon", p -> spec.empower_rune_weapon )
            .cd( timespan_t::zero() ) // Handled in the action
            .period( p -> spec.empower_rune_weapon -> effectN( 1 ).period() )
            .tick_callback( [ this, p ]( buff_t* b, int, const timespan_t& ) {
    p -> replenish_rune( as<unsigned int>( b -> data().effectN( 1 ).base_value() ),
                         p -> gains.empower_rune_weapon );

    p -> resource_gain( RESOURCE_RUNIC_POWER,
                        b -> data().effectN( 2 ).resource( RESOURCE_RUNIC_POWER ),
                        p -> gains.empower_rune_weapon );
    } ) )
  {
    tick_zero = true;
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

  void init() override
  {
    death_knight_spell_t::init();

    cooldown -> charges = data().charges() +
      as<int>( p() -> legendary.seal_of_necrofantasia -> effectN( 1 ).base_value() );
    cooldown -> duration = data().charge_cooldown() *
      ( 1.0 + p() -> legendary.seal_of_necrofantasia -> effectN( 2 ).percent() );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.empower_rune_weapon -> trigger();
    p() -> buffs.empower_rune_weapon_haste -> trigger();
  }
};

// Epidemic =================================================================

struct epidemic_damage_main_t : public death_knight_spell_t
{
  epidemic_damage_main_t( death_knight_t* p ) :
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
    main( new epidemic_damage_main_t( p ) ),
    aoe( new epidemic_damage_aoe_t( p ) )
  {
    parse_options( options_str );

    add_child( main );
    add_child( aoe );

    // spec.death_knight -> effectN( 4 )
    cooldown -> hasted = true;
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

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> trigger_runic_corruption( base_costs[ RESOURCE_RUNIC_POWER ] );
    }

    p() -> cooldown.apocalypse -> adjust( -timespan_t::from_seconds( 
      p() -> talent.army_of_the_damned -> effectN( 1 ).base_value() / 10 ) );

    p() -> cooldown.army_of_the_dead -> adjust( -timespan_t::from_seconds( 
      p() -> talent.army_of_the_damned -> effectN( 2 ).base_value() / 10 ) );
  }
};

// Festering Strike =========================================================

struct festering_strike_t : public death_knight_melee_attack_t
{
  festering_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "festering_strike", p, p -> find_specialization_spell( "Festering Strike" ) )
  {
    parse_options( options_str );
  }

  void impact( action_state_t* s ) override
  {
    static const std::array<unsigned, 2> fw_proc_stacks = { { 2, 3 } };

    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      size_t n = rng().range( size_t(), fw_proc_stacks.size() );
      unsigned n_stacks = fw_proc_stacks[ n ];

      p() -> trigger_festering_wound( s, n_stacks );

      trigger_draugr_girdle_everlasting_king();
    }
  }

  void trigger_draugr_girdle_everlasting_king()
  {
    if (!rng().roll(p()->legendary.draugr_girdle_everlasting_king))
      return;

    if (sim->debug)
    {
      log_rune_status(p());
    }

    if (p() -> replenish_rune(1, p()->gains.draugr_girdle_everlasting_king) && sim->debug)
    {
      sim->out_debug.printf("draugr_girdle_of_the_everlasting_king regenerated rune");
      log_rune_status(p());
    }
  }
};

// Frostscythe ==============================================================

struct frostscythe_t : public death_knight_melee_attack_t
{
  double rime_proc_chance;

  frostscythe_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "frostscythe", p, p -> talent.frostscythe ),
    rime_proc_chance( ( p -> spec.rime -> proc_chance() +
                        p -> sets -> set( DEATH_KNIGHT_FROST, T19, B2 ) -> effectN( 1 ).percent() ) / 2.0 )
  {
    parse_options( options_str );

    weapon = &( player -> main_hand_weapon );
    aoe = -1;

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

    consume_killing_machine( execute_state, p() -> procs.fs_killing_machine );
    trigger_icecap( execute_state );

    // Frostscythe procs rime at half the chance of Obliterate
    p() -> buffs.rime -> trigger( 1, buff_t::DEFAULT_VALUE(), rime_proc_chance );
  }

  double composite_crit_chance() const override
  {
    double cc = death_knight_melee_attack_t::composite_crit_chance();

    cc += p() -> buffs.killing_machine -> value();

    return cc;
  }
};

// Frostwyrm's Fury ========================================================

// TODO: Fancy targeting
struct frostwyrms_fury_t : public death_knight_spell_t
{
  frostwyrms_fury_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "frostwyrms_fury", p, p -> talent.frostwyrms_fury )
  {
    parse_options( options_str );

    aoe = -1;

    parse_effect_data( p -> find_spell( 279303 ) -> effectN( 1 ) );
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

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    return m;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    trigger_icecap( execute_state );
  }
};

struct frost_strike_t : public death_knight_melee_attack_t
{
  frost_strike_strike_t *mh, *oh;

  frost_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "frost_strike", p, p -> find_specialization_spell( "Frost Strike" ) ),
    mh( nullptr ), oh( nullptr )
  {
    parse_options( options_str );
    may_crit = false;

    mh = new frost_strike_strike_t( p, "frost_strike_mh", &( p -> main_hand_weapon ), data().effectN( 2 ).trigger() );
    add_child( mh );
    oh = new frost_strike_strike_t( p, "frost_strike_offhand", &( p -> off_hand_weapon ), data().effectN( 3 ).trigger() );
    add_child( oh );
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    death_knight_td_t* tdata = td( execute_state -> target );
      
    if ( result_is_hit( execute_state -> result ) )
    {
      mh -> set_target( execute_state -> target );
      mh -> execute();
      oh -> set_target( execute_state -> target );
      oh -> execute();

      p() -> trigger_runic_empowerment( last_resource_cost );
    }

    p() -> buffs.icy_talons -> trigger();

    // Note note, killing machine is a RPPM thing, but we need to trigger it unconditionally when
    // obliterate is up, so just bypas "trigger" and directly execute the buff, while making sure
    // correct bookkeeping information is kept. Ugly but will work for now.
    if ( p() -> buffs.obliteration -> up() )
    {
      //p() -> buffs.killing_machine -> trigger_attempts++;
      p() -> buffs.killing_machine -> execute();
    }
  }
};

// Glacial Advance =========================================================

// TODO: Fancier targeting .. make it aoe for now
struct glacial_advance_damage_t : public death_knight_spell_t
{
  glacial_advance_damage_t( death_knight_t* player ) :
    death_knight_spell_t( "glacial_advance_damage", player, player -> find_spell( 195975 ) )
  {
    aoe = -1;
    background = true;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );
    td( state -> target ) -> debuff.razorice -> trigger();
  }
};

struct glacial_advance_t : public death_knight_spell_t
{
  glacial_advance_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "glacial_advance", player, player -> talent.glacial_advance )
  {
    parse_options( options_str );
    school = SCHOOL_FROST; // Damage is frost so override this to make reports make more sense
    ap_type = AP_WEAPON_BOTH;

    execute_action = new glacial_advance_damage_t( player );
    add_child( execute_action );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.icy_talons -> trigger();
  }
};

// Heart Strike =============================================================

struct heart_strike_t : public death_knight_melee_attack_t
{
  heart_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "heart_strike", p, p -> spec.heart_strike )
  {
    parse_options( options_str );

    weapon = &( p -> main_hand_weapon );
  }

  int n_targets() const override
  { return p() -> in_death_and_decay() ? 5 : 2; }

  void impact ( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );
    
    if ( p() -> talent.heartbreaker -> ok() && result_is_hit( state -> result ) )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER, p() -> find_spell( 210738 ) -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
      p() -> gains.heartbreaker, this );
    }
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();
    
    if ( p() -> buffs.dancing_rune_weapon -> check() )
    {
      p() -> pets.dancing_rune_weapon_pet -> ability.heart_strike -> set_target( execute_state -> target );
      p() -> pets.dancing_rune_weapon_pet -> ability.heart_strike -> execute();
    }
  }
};

// Consumption

struct consumption_t : public death_knight_melee_attack_t
{
  consumption_t( death_knight_t* p, const std::string& options_str )
    : death_knight_melee_attack_t( "consumption", p, p -> talent.consumption )
    {
      parse_options( options_str );
      aoe = -1;

      base_multiplier *= 1.0 + p -> spec.blood_death_knight -> effectN( 3 ).percent();
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

// Howling Blast ============================================================

struct howling_blast_aoe_t : public death_knight_spell_t
{
  howling_blast_aoe_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "howling_blast_aoe", p, p -> find_spell( 237680 ) )
  {
    parse_options( options_str );

    aoe = -1;
    background = true;

    // T21 2P bonus : damage increase to Howling Blast, Frostscythe and Obliterate
    if ( p -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T21, B2 ) )
    {
      base_multiplier *= ( 1.0 + p -> sets -> set( DEATH_KNIGHT_FROST, T21, B2 ) -> effectN( 1 ).percent() );
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

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_spell_t::composite_target_multiplier( target );

    m *= 1.0 + td( target ) -> debuff.perseverance_of_the_ebon_martyr -> check_value();

    return m;
  }
};

struct howling_blast_t : public death_knight_spell_t
{
  howling_blast_aoe_t* aoe_damage;

  howling_blast_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "howling_blast", p, p -> find_specialization_spell( "Howling Blast" ) ),
    aoe_damage( new howling_blast_aoe_t( p, options_str ) )
  {
    parse_options( options_str );

    aoe = 1;
    add_child( aoe_damage );
    ap_type = AP_WEAPON_BOTH;

    // T21 2P bonus : damage increase to Howling Blast, Frostscythe and Obliterate
    if ( p -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T21, B2 ) )
    {
      base_multiplier *= ( 1.0 + p -> sets -> set( DEATH_KNIGHT_FROST, T21, B2 ) -> effectN( 1 ).percent() );
    }
  }

  double runic_power_generation_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::runic_power_generation_multiplier( state );

    if ( p() -> buffs.rime -> check() )
    {
      m *= 1.0 + ( p() -> buffs.rime -> data().effectN( 3 ).percent() +
                   p() -> sets -> set( DEATH_KNIGHT_FROST, T19, B4 ) -> effectN( 1 ).percent() );
    }

    return m;
  }
  
  // T19 4P bonus : RP gain when using Rime proc
  gain_t* energize_gain( const action_state_t* /* state */ ) const override
  {
    if ( p() -> buffs.rime -> check() && p() -> sets -> has_set_bonus( DEATH_KNIGHT_FROST, T19, B4 ) )
    {
      return p() -> gains.t19_4pc_frost;
    }

    return gain;
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

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_spell_t::composite_target_multiplier( target );

    m *= 1.0 + td( target ) -> debuff.perseverance_of_the_ebon_martyr -> check_value();

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

    if ( p() -> buffs.obliteration -> up() )
    {
      p() -> buffs.killing_machine -> execute();
    }
   
    aoe_damage -> set_target( execute_state -> target );
    aoe_damage -> execute();

    p() -> buffs.rime -> decrement();
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      p() -> apply_diseases( s, DISEASE_FROST_FEVER );
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


// Marrowrend ===============================================================


struct marrowrend_t : public death_knight_melee_attack_t
{

  marrowrend_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "marrowrend", p, p -> spec.marrowrend )
  {
    parse_options( options_str );
    weapon = &( p -> main_hand_weapon );
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

  void execute() override
  {
    death_knight_spell_t::execute();
    if ( p() -> legendary.sephuzs_secret != nullptr )
    {
      p() -> buffs.sephuzs_secret -> trigger();
    }
  }

  bool ready() override
  {
    if ( ! target -> debuffs.casting || ! target -> debuffs.casting -> check() )
      return false;

    return death_knight_spell_t::ready();
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

  double action_multiplier() const override
  {
    double m = death_knight_melee_attack_t::action_multiplier();
    
    m *= 1.0 + p() -> legendary.koltiras_newfound_will -> effectN( 2 ).percent();
    
    return m;
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
  }
};

struct obliterate_t : public death_knight_melee_attack_t
{
  obliterate_strike_t* mh, *oh;

  obliterate_t( death_knight_t* p, const std::string& options_str = std::string() ) :
    death_knight_melee_attack_t( "obliterate", p, p -> find_specialization_spell( "Obliterate" ) ),
    mh( nullptr ), oh( nullptr )
  {
    parse_options( options_str );

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

      p() -> buffs.rime -> trigger();
    }

    if ( rng().roll( p() -> legendary.koltiras_newfound_will -> proc_chance() ) )
    {
      p() -> replenish_rune( as<unsigned int>( p() -> legendary.koltiras_newfound_will -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() ),
          p() -> gains.koltiras_newfound_will );
    }
    
    consume_killing_machine( execute_state, p() -> procs.oblit_killing_machine );
  }

  double cost() const override
  {
    double c = death_knight_melee_attack_t::cost();

    if ( p() -> buffs.obliteration -> check() )
    {
      c += p() -> buffs.obliteration -> data().effectN( 1 ).base_value();
    }

    if ( c < 0 )
    {
      c = 0;
    }

    return c;
  }
};

// Obliteration =============================================================

struct obliteration_t : public death_knight_spell_t
{
  obliteration_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "obliteration", p, p -> talent.obliteration )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.obliteration -> trigger();
  }
};

// Outbreak =================================================================

struct outbreak_spreader_t : public death_knight_spell_t
{
  outbreak_spreader_t( death_knight_t* p ) :
    death_knight_spell_t( "outbreak_spreader", p )
  {
    quiet = background = dual = true;
    callbacks = may_crit = false;
    aoe = -1;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    p() -> apply_diseases( state, DISEASE_VIRULENT_PLAGUE );
  }
};

struct outbreak_driver_t : public death_knight_spell_t
{
  outbreak_spreader_t* spread;
  const spell_data_t* outbreak_base;

  outbreak_driver_t( death_knight_t* p ) :
    death_knight_spell_t( "outbreak_driver", p, p -> dbc.spell( 196782 ) ),
    spread( new outbreak_spreader_t( p ) ),
    outbreak_base( p -> spec.outbreak )
  {
    quiet = background = tick_zero = dual = true;
    callbacks = hasted_ticks = false;
  }

  bool verify_actor_level() const override
  {
    return outbreak_base -> id() && outbreak_base -> is_level( player -> true_level ) &&
           outbreak_base -> level() <= MAX_LEVEL;
  }

  void tick( dot_t* dot ) override
  {
    spread -> set_target( dot -> target );
    spread -> schedule_execute();
  }
};

struct outbreak_t : public death_knight_spell_t
{
  outbreak_driver_t* spread;

  outbreak_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "outbreak" ,p , p -> spec.outbreak ),
    spread( new outbreak_driver_t( p ) )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      spread -> set_target( execute_state -> target );
      spread -> schedule_execute();
    }
  }
};

// Pillar of Frost ==========================================================

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

    p() -> buffs.pillar_of_frost -> trigger( 1,
                                             p() -> buffs.pillar_of_frost -> default_value +
                                             p() -> buffs.t20_4pc_frost -> stack_value() );
    p() -> buffs.t20_4pc_frost -> expire();
    if ( p() -> legendary.toravons ) 
      p() -> buffs.toravons -> trigger();
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
  remorseless_winter_damage_t( death_knight_t* p ) :
    death_knight_spell_t( "remorseless_winter_damage", p, p -> find_spell( 196771 ) )
  {
    background = true;
    aoe = -1;
  }

  double action_multiplier() const override
  {
    double m = death_knight_spell_t::action_multiplier();

    m *= 1.0 + p() -> buffs.gathering_storm -> stack_value();

    return m;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    td( state -> target ) -> debuff.perseverance_of_the_ebon_martyr -> trigger();
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
    death_knight_spell_t::execute();

    p() -> buffs.remorseless_winter -> trigger();
  }
};

// Rune Strike 

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
  }
};

// Scourge Strike and Clawing Shadows =======================================

struct scourge_strike_base_t : public death_knight_melee_attack_t
{
  std::array<double, 3> instructors_chance;

  scourge_strike_base_t( const std::string& name, death_knight_t* p, const spell_data_t* spell ) :
    death_knight_melee_attack_t( name, p, spell )
  {
    weapon = &( player -> main_hand_weapon );

    //instructors_chance = { { 0.20, 0.40, 0.20, 0.10, 0.05, 0.05 } };
    //
    // Updated 2017-07-18 based on empirical testing
    instructors_chance = { { .635, .1825, .1825 } };
    if ( p -> legendary.the_instructors_fourth_lesson > 0 &&
         instructors_chance.size() != p -> legendary.the_instructors_fourth_lesson + 1 )
    {
      sim -> errorf( "Player %s Instructor's Fourth Lesson probability distribution mismatch",
        p -> name() );
    }
  }

  int n_targets() const override
  { return p() -> in_death_and_decay() ? -1 : 0; }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      int n_burst = 1;

      if ( p() -> legendary.the_instructors_fourth_lesson )
      {
        double roll = rng().real();
        double sum = 0;

        for ( size_t i = 0; i < instructors_chance.size(); i++ )
        {
          sum += instructors_chance[ i ];

          if ( roll <= sum )
          {
            n_burst += as<int>( i );
            break;
          }
        }
      }

      p() -> burst_festering_wound( state, n_burst );
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

struct scourge_strike_t : public scourge_strike_base_t
{
  struct scourge_strike_shadow_t : public death_knight_melee_attack_t
  {
    const spell_data_t* scourge_base;

    scourge_strike_shadow_t( death_knight_t* p ) :
      death_knight_melee_attack_t( "scourge_strike_shadow", p, p -> spec.scourge_strike -> effectN( 3 ).trigger() ),
      scourge_base( p -> spec.scourge_strike )
    {
      may_miss = may_parry = may_dodge = false;
      proc = background = true;
      weapon = &( player -> main_hand_weapon );
      dual = true;
      school = SCHOOL_SHADOW;
    }

    bool verify_actor_level() const override
    {
      return scourge_base -> id() && scourge_base -> is_level( player -> true_level ) &&
             scourge_base -> level() <= MAX_LEVEL;
    }
  };

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

    int charges = p() -> buffs.bone_shield -> stack();
    // Tomnstone doesn't consume more than 5 bone shield charges
    if ( charges > 5 )
      charges = 5;

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
    death_knight_spell_t( "unholy_blight", p, p -> talent.unholy_blight -> effectN( 1 ).trigger() )
  {
    background = true;
    hasted_ticks = false;
  }
};

struct unholy_blight_t : public death_knight_spell_t
{
  unholy_blight_dot_t* ub_dot;

  unholy_blight_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "unholy_blight_spreader", p, p -> talent.unholy_blight ),
    ub_dot( new unholy_blight_dot_t( p ) )
  {
    may_crit = may_miss = may_dodge = may_parry = false;
    hasted_ticks = false;
    tick_zero = true;
    parse_options( options_str );

    aoe = -1;
  }

  void tick( dot_t* dot ) override
  {
    death_knight_spell_t::tick( dot );

    ub_dot -> set_target( dot -> target );
    ub_dot -> schedule_execute();
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
  }

  void impact( action_state_t* s ) override
  {
    if ( s -> target == target )
    {
      death_knight_spell_t::impact( s );
      p() -> buffs.icy_talons -> trigger();
    }

    else
    {
      double damage = s -> result_amount;
      // Damage nerfed on secondary targets on BfA alpha
      damage /= ( execute_state -> n_targets * 5/3 );
      s -> result_amount = s -> result_total = damage;
      death_knight_spell_t::impact( s );
    }
  }
};

struct breath_of_sindragosa_t : public death_knight_spell_t
{
  breath_of_sindragosa_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "breath_of_sindragosa", p, p -> talent.breath_of_sindragosa )
  {
    parse_options( options_str );

    may_miss = may_crit = hasted_ticks = callbacks = false;
    tick_zero = dynamic_tick_action = true;

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

  bool consume_cost_per_tick( const dot_t& dot ) override
  {
    bool ret = death_knight_spell_t::consume_cost_per_tick( dot );

    p() -> trigger_runic_empowerment( last_resource_cost );

    return ret;
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

// Anti-magic Shell =========================================================

struct antimagic_shell_buff_t : public buff_t
{
  antimagic_shell_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "antimagic_shell", p -> spell.antimagic_shell )
                              .cd( timespan_t::zero() ) )
  { }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    death_knight_t* p = debug_cast< death_knight_t* >( player );
    p -> antimagic_shell_absorbed = 0.0;
    duration *= 1.0 + p -> talent.antimagic_barrier -> effectN( 2 ).percent();
    buff_t::execute( stacks, value, duration );
  }
};

struct antimagic_shell_t : public death_knight_spell_t
{
  double interval;
  double interval_stddev;
  double interval_stddev_opt;
  double damage;

  antimagic_shell_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "antimagic_shell", p, p -> spell.antimagic_shell ),
    interval( 60 ), interval_stddev( 0.05 ), interval_stddev_opt( 0 ), damage( 0 )
  {
    cooldown = p -> cooldown.antimagic_shell;
    cooldown -> duration += timespan_t::from_millis( p -> talent.antimagic_barrier -> effectN( 1 ).base_value() );
    harmful = may_crit = may_miss = false;
    base_dd_min = base_dd_max = 0;
    target = player;

    add_option( opt_float( "interval", interval ) );
    add_option( opt_float( "interval_stddev", interval_stddev_opt ) );
    add_option( opt_float( "damage", damage ) );
    parse_options( options_str );

    // Allow as low as 30 second intervals
    if ( interval < 30.0 )
    {
      sim -> errorf( "%s minimum interval for Anti-Magic Shell is 30 seconds.", player -> name() );
      interval = 30.0;
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
      if ( new_cd < timespan_t::from_seconds( 15.0 ) )
        new_cd = timespan_t::from_seconds( 15.0 );

      cooldown -> duration = new_cd;
    }

    death_knight_spell_t::execute();

    // If using the fake soaking, immediately grant the RP in one go
    if ( damage > 0 )
    {
      double absorbed = std::min( damage * data().effectN( 1 ).percent(),
                                  p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 2 ).percent() );

      double generated = absorbed / p() -> resources.max[ RESOURCE_HEALTH ];
      
      // Volatile shielding increases AMS' RP generation
      if ( p() -> talent.volatile_shielding -> ok() )
        generated *= 1.0 + p() -> talent.volatile_shielding -> effectN( 2 ).percent();

      // AMS generates 2 runic power per percentage max health absorbed.
      p() -> resource_gain( RESOURCE_RUNIC_POWER, util::round( generated * 100.0 * 2.0 ), p() -> gains.antimagic_shell, this );
    }
    else
      p() -> buffs.antimagic_shell -> trigger();
  }
};

// That legendary crap ring ===========================================================

struct sephuzs_secret_buff_t: public haste_buff_t
{
  cooldown_t* icd;
  sephuzs_secret_buff_t( death_knight_t* p ):
    haste_buff_t( p, "sephuzs_secret", p -> find_spell( 208052 ) )
  {
    set_default_value( p -> find_spell( 208052 ) -> effectN( 2 ).percent() );
    add_invalidate( CACHE_HASTE );
    icd = p -> get_cooldown( "sephuzs_secret_cooldown" );
    icd  -> duration = p -> find_spell( 226262 ) -> duration();
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    if ( icd -> down() )
      return;
    buff_t::execute( stacks, value, duration );
    icd -> start();
  }
};

// Vampiric Blood ===========================================================

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

// Rune Tap

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

// Expressions

struct disease_expr_t : public expr_t
{
  enum type_e { TYPE_NONE, TYPE_MIN, TYPE_MAX };

  type_e  type;
  expr_t* bp_expr;
  expr_t* ff_expr;
  expr_t* np_expr;

  double default_value;

  disease_expr_t( const action_t* a, const std::string& expression, type_e t ) :
    expr_t( "disease_expr" ), type( t ), bp_expr( nullptr ), ff_expr( nullptr ), np_expr( nullptr ),
    default_value( 0 )
  {
    death_knight_t* p = debug_cast< death_knight_t* >( a -> player );
    bp_expr = dot_t::create_expression( nullptr, p -> active_spells.blood_plague, expression, true );
    ff_expr = dot_t::create_expression( nullptr, p -> active_spells.frost_fever, expression, true );

    if ( type == TYPE_MIN )
      default_value = std::numeric_limits<double>::max();
    else if ( type == TYPE_MAX )
      default_value = std::numeric_limits<double>::min();
  }

  double evaluate() override
  {
    double ret = default_value;

    if ( bp_expr )
    {
      double val = bp_expr -> eval();
      if ( type == TYPE_NONE && val != 0 )
        return val;
      else if ( type == TYPE_MIN && val < ret )
        ret = val;
      else if ( type == TYPE_MAX && val > ret )
        ret = val;
    }

    if ( ff_expr )
    {
      double val = ff_expr -> eval();
      if ( type == TYPE_NONE && val != 0 )
        return val;
      else if ( type == TYPE_MIN && val < ret )
        ret = val;
      else if ( type == TYPE_MAX && val > ret )
        ret = val;
    }

    if ( np_expr )
    {
      double val = np_expr -> eval();
      if ( type == TYPE_NONE && val != 0 )
        return val;
      else if ( type == TYPE_MIN && val < ret )
        ret = val;
      else if ( type == TYPE_MAX && val > ret )
        ret = val;
    }

    if ( ret == default_value )
      ret = 0;

    return ret;
  }

  ~disease_expr_t()
  {
    delete bp_expr;
    delete ff_expr;
    delete np_expr;
  }
};

// Buffs ====================================================================

struct runic_corruption_buff_t : public buff_t
{
  runic_corruption_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "runic_corruption", p -> spell.runic_corruption )
            .trigger_spell( p -> spec.runic_corruption ).affects_regen( true )
            .stack_change_callback( [ p ]( buff_t*, int, int ) { p -> _runes.update_coefficient(); } ) )
  { }
};

// Generic health increase buff
template <typename T_BUFF, typename T_CREATOR>
struct health_pct_increase_buff_t : public T_BUFF
{
  double delta;

  typedef health_pct_increase_buff_t<T_BUFF, T_CREATOR> super;

  health_pct_increase_buff_t( const T_CREATOR& creator ) : T_BUFF( creator ), delta( 1.0 )
  { }

  death_knight_t* dk() const
  { return debug_cast<death_knight_t*>( this -> player ); }

  void execute( int stacks = 1, double value = T_BUFF::DEFAULT_VALUE(), timespan_t duration = timespan_t::min() ) override
  {
    T_BUFF::execute( stacks, value, duration );

    if ( delta != 0.0 )
    {
      double old_health = this -> player -> resources.max[ RESOURCE_HEALTH ];
      this -> player -> resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + delta;
      this -> player -> recalculate_resource_max( RESOURCE_HEALTH );

      if ( this -> sim -> debug )
      {
        this -> sim -> out_debug.printf( "%s %s stacking health pct change %.2f%%, %.1f -> %.1f",
            this -> player -> name(), this -> name(), delta * 100.0, old_health,
            this -> player -> resources.max[ RESOURCE_HEALTH ] );
      }
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    T_BUFF::expire_override( expiration_stacks, remaining_duration );

    if ( delta != 0.0 )
    {
      double old_health = this -> player -> resources.max[ RESOURCE_HEALTH ];
      this -> player -> resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + delta;
      this -> player -> recalculate_resource_max( RESOURCE_HEALTH );

      if ( this -> sim -> debug )
      {
        this -> sim -> out_debug.printf( "%s %s stacking health pct change %.2f%%, %.1f -> %.1f",
            this -> player -> name(), this -> name(), delta * 100.0, old_health,
            this -> player -> resources.max[ RESOURCE_HEALTH ] );
      }
    }
  }
};

struct vampiric_blood_buff_t : public health_pct_increase_buff_t<buff_t, buff_creator_t>
{
  vampiric_blood_buff_t( death_knight_t* p ) :
    super( buff_creator_t( p, "vampiric_blood", p -> spec.vampiric_blood ).cd( timespan_t::zero() ) )
  { }
};

// Remorseless Winter

struct remorseless_winter_buff_t : public buff_t
{
  buff_t*                      gathering_storm; // Gathering Storm buff, needs expiration on trigger
  remorseless_winter_damage_t* damage; // (AOE) damage that ticks every second

  remorseless_winter_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "remorseless_winter", p -> spec.remorseless_winter )
      .cd( timespan_t::zero() ) // Controlled by the action
      .refresh_behavior( buff_refresh_behavior::DURATION )
      .tick_callback( [ this ]( buff_t* /* buff */, int /* total_ticks */, timespan_t /* tick_time */ ) {
        damage -> execute();
      } ) ),
    gathering_storm( buff_t::find( p, "gathering_storm" ) ),
    damage( new remorseless_winter_damage_t( p ) )
  { }

  void execute( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() ) override
  {
    buff_t::execute( stacks, value, duration );

    // Refresh existing Gathering Storm duration (adds a stack too?)
    if ( gathering_storm && gathering_storm -> check() )
    {
      gathering_storm -> trigger();
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( gathering_storm )
    {
      gathering_storm -> expire();
    }
  }
};

} // UNNAMED NAMESPACE

void runeforge::fallen_crusader( special_effect_t& effect )
{
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

  effect.ppm_ = -1.0 * dk -> fallen_crusader_rppm;
  effect.custom_buff = b;
  effect.execute_action = heal;

  new dbc_proc_callback_t( effect.item, effect );
}

void runeforge::stoneskin_gargoyle( special_effect_t& effect )
{
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
    buffs.pillar_of_frost -> extend_duration( this, timespan_t::from_seconds(
      sets -> set( DEATH_KNIGHT_FROST, T20, B2 ) -> effectN( 2 ).base_value() / 10.0 ) );
    buffs.toravons -> extend_duration( this, timespan_t::from_seconds(
      sets -> set( DEATH_KNIGHT_FROST, T20, B2 ) -> effectN( 2 ).base_value() / 10.0 ) );
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

bool death_knight_t::trigger_runic_corruption( double rpcost, double override_chance )
{
  double actual_chance = override_chance != -1.0
    ? override_chance
    : ( spec.runic_corruption -> effectN( 1 ).percent() * rpcost / 100.0 );

  if ( ! rng().roll( actual_chance ) )
    return false;


  if ( override_chance == -1.0 )
  {
    procs.rp_runic_corruption -> occur();
  }

  timespan_t duration = timespan_t::from_seconds( 3.0 * cache.attack_haste() );
  if ( buffs.runic_corruption -> check() == 0 )
    buffs.runic_corruption -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
  else
    buffs.runic_corruption -> extend_duration( this, duration );

  return true;
}

void death_knight_t::trigger_festering_wound( const action_state_t* state, unsigned n )
{
  if ( ! state -> action -> result_is_hit( state -> result ) )
  {
    return;
  }

  auto td = get_target_data( state -> target );

  td -> debuff.festering_wound -> trigger( n );
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
          if ( dk -> trigger_runic_corruption( 0,
                dk -> sets -> set( DEATH_KNIGHT_UNHOLY, T19, B2 ) -> effectN( 1 ).percent() ) )
          {
            dk -> procs.t19_2pc_unholy -> occur();
          }
        }
      }

      // Triggers once for all the festering wound burst event on the same target
      // Apocalypse is 10% * n wounds burst to proc
      // Scourge strike in DnD is 1 - ( 0.9 ) ^ n targets to proc
      if ( dk -> talent.pestilent_pustules -> ok() )
      {
        if ( dk -> trigger_runic_corruption( 0, dk -> talent.pestilent_pustules -> effectN( 1 ).percent() * n ) )
        {
          dk -> procs.pp_runic_corruption -> occur();
        }
      }

      td -> debuff.festering_wound -> decrement( n_executes );
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

void death_knight_t::trigger_death_march( const action_state_t* /* state */ )
{
  if ( ! legendary.death_march -> ok() )
  {
    return;
  }

  timespan_t adjust = -timespan_t::from_seconds( legendary.death_march -> effectN( 1 ).base_value() / 10.0 );

  cooldown.defile -> adjust( adjust );
  cooldown.death_and_decay -> adjust( adjust );
}

// ==========================================================================
// Death Knight Character Definition
// ==========================================================================

// death_knight_t::create_actions ===========================================

bool death_knight_t::create_actions()
{
  if ( talent.avalanche -> ok() )
  {
    active_spells.avalanche = new avalanche_t( this );
  }

  if ( spec.festering_wound -> ok() )
  {
    active_spells.festering_wound = new festering_wound_t( this );
  }

  if ( spec.outbreak -> ok() )
  {
    active_spells.virulent_plague = new virulent_plague_t( this );
  }

  if ( talent.bursting_sores -> ok() )
  {
    active_spells.bursting_sores = new bursting_sores_t( this );
  }

  if ( talent.mark_of_blood -> ok() )
  {
    active_spells.mark_of_blood = new mark_of_blood_heal_t( this );
  }

  if ( spec.frost_death_knight -> ok() )
  {
    active_spells.freezing_death = new freezing_death_t( this );
  }

  return player_t::create_actions();
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
  if ( name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_t      ( this, options_str );
  if ( name == "dark_command"             ) return new dark_command_t             ( this, options_str );
  if ( name == "deaths_caress"            ) return new deaths_caress_t            ( this, options_str );
  if ( name == "heart_strike"             ) return new heart_strike_t             ( this, options_str );
  if ( name == "marrowrend"               ) return new marrowrend_t               ( this, options_str );
  if ( name == "vampiric_blood"           ) return new vampiric_blood_t           ( this, options_str );
  
  // Talents
  if ( name == "blooddrinker"             ) return new blooddrinker_t             ( this, options_str );
  if ( name == "bonestorm"                ) return new bonestorm_t                ( this, options_str );
  if ( name == "consumption"              ) return new consumption_t              ( this, options_str );
  if ( name == "mark_of_blood"            ) return new mark_of_blood_t            ( this, options_str );
  if ( name == "rune_strike"              ) return new rune_strike_t              ( this, options_str );
  if ( name == "rune_tap"                 ) return new rune_tap_t                 ( this, options_str );
  if ( name == "tombstone"                ) return new tombstone_t                ( this, options_str );

  // Frost Actions
  if ( name == "empower_rune_weapon"      ) return new empower_rune_weapon_t      ( this, options_str );
  if ( name == "frost_strike"             ) return new frost_strike_t             ( this, options_str );
  if ( name == "howling_blast"            ) return new howling_blast_t            ( this, options_str );
  if ( name == "mind_freeze"              ) return new mind_freeze_t              ( this, options_str );
  if ( name == "obliterate"               ) return new obliterate_t               ( this, options_str );
  if ( name == "pillar_of_frost"          ) return new pillar_of_frost_t          ( this, options_str );
  if ( name == "remorseless_winter"       ) return new remorseless_winter_t       ( this, options_str );
  
  // Talents
  if ( name == "breath_of_sindragosa"     ) return new breath_of_sindragosa_t     ( this, options_str );
  if ( name == "frostscythe"              ) return new frostscythe_t              ( this, options_str );
  if ( name == "glacial_advance"          ) return new glacial_advance_t          ( this, options_str );
  if ( name == "horn_of_winter"           ) return new horn_of_winter_t           ( this, options_str );
  if ( name == "obliteration"             ) return new obliteration_t             ( this, options_str );
  if ( name == "frostwyrms_fury"          ) return new frostwyrms_fury_t          ( this, options_str );

  // Unholy Actions
  if ( name == "army_of_the_dead"         ) return new army_of_the_dead_t         ( this, options_str );
  if ( name == "apocalypse"               ) return new apocalypse_t               ( this, options_str );
  if ( name == "dark_transformation"      ) return new dark_transformation_t      ( this, options_str );
  if ( name == "death_and_decay"          ) return new death_and_decay_t          ( this, options_str );
  if ( name == "death_coil"               ) return new death_coil_t               ( this, options_str );
  if ( name == "festering_strike"         ) return new festering_strike_t         ( this, options_str );
  if ( name == "outbreak"                 ) return new outbreak_t                 ( this, options_str );
  if ( name == "raise_dead"               ) return new raise_dead_t               ( this, options_str );
  if ( name == "scourge_strike"           ) return new scourge_strike_t           ( this, options_str );

  // Talents
  if ( name == "clawing_shadows"          ) return new clawing_shadows_t          ( this, options_str );
  if ( name == "defile"                   ) return new defile_t                   ( this, options_str );
  if ( name == "epidemic"                 ) return new epidemic_t                 ( this, options_str );
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
    if ( find_action( "summon_gargoyle" ) && talent.summon_gargoyle -> ok() )
    {
      pets.gargoyle = new pets::gargoyle_pet_t( this );
    }

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
  }
  
  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    if ( find_action( "dancing_rune_weapon" ) ) 
    {
      pets.dancing_rune_weapon_pet = new pets::dancing_rune_weapon_pet_t( this );
    }

    if ( talent.bloodworms -> ok() )
    {
      for ( auto i = 0; i < 6; i++ )
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

  haste *= 1.0 / ( 1.0 + buffs.sephuzs_secret -> check_value() );

  haste *= 1.0 / ( 1.0 + buffs.soul_reaper -> check_value() );

  haste *= 1.0 / ( 1.0 + buffs.unholy_frenzy -> check_value() );
	
  haste *= 1.0 / ( 1.0 + buffs.empower_rune_weapon_haste -> check_value() );
  
  if ( buffs.bone_shield -> up() )
  {
    haste *= buffs.bone_shield -> value();
  }
  
  if ( legendary.sephuzs_secret )
  {
    haste *= 1.0 / ( 1.0 + legendary.sephuzs_secret -> effectN( 3 ).percent() );
  }
  
  return haste;
}

// death_knight_t::composite_spell_haste() ==================================

double death_knight_t::composite_spell_haste() const
{
  double haste = player_t::composite_spell_haste();

  haste *= 1.0 / ( 1.0 + buffs.sephuzs_secret -> check_value() );

  haste *= 1.0 / ( 1.0 + buffs.soul_reaper -> check_value() );

  haste *= 1.0 / ( 1.0 + buffs.unholy_frenzy -> check_value() );
	
  haste *= 1.0 / ( 1.0 + buffs.empower_rune_weapon_haste -> check_value() );
  
  if ( buffs.bone_shield -> up() )
  {
    haste *= buffs.bone_shield -> value();
  }

  if ( legendary.sephuzs_secret )
  {
    haste *= 1.0 / ( 1.0 + legendary.sephuzs_secret -> effectN( 3 ).percent() );
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
  resources.base[ RESOURCE_RUNIC_POWER ] += ( spec.veteran_of_the_third_war -> effectN( 8 ).resource( RESOURCE_RUNIC_POWER ) );
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
  spec.plate_specialization       = find_specialization_spell( "Plate Specialization" );
  spec.death_knight               = find_spell( 137005 ); // "Death Knight" passive
  // Shared
  spec.icebound_fortitude         = find_specialization_spell( "Icebound Fortitude" );

  // Blood
  spec.blood_death_knight         = find_specialization_spell( "Blood Death Knight" );
  spec.veteran_of_the_third_war   = find_specialization_spell( "Veteran of the Third War" );
  spec.riposte                    = find_specialization_spell( "Riposte" );
  spec.blood_boil                 = find_specialization_spell( "Blood Boil" );
  spec.crimson_scourge            = find_specialization_spell( "Crimson Scourge" );
  spec.dancing_rune_weapon        = find_specialization_spell( "Dancing Rune Weapon" );
  spec.deaths_caress              = find_specialization_spell( "Death's Caress" );
  spec.heart_strike               = find_specialization_spell( "Heart Strike" );
  spec.marrowrend                 = find_specialization_spell( "Marrowrend" );
  spec.vampiric_blood             = find_specialization_spell( "Vampiric Blood" );

  // Frost
  spec.frost_death_knight         = find_specialization_spell( "Frost Death Knight" );
  spec.frost_fever                = find_specialization_spell( "Frost Fever" );
  spec.rime                       = find_specialization_spell( "Rime" );
  spec.runic_empowerment          = find_specialization_spell( "Runic Empowerment" );
  spec.killing_machine            = find_specialization_spell( "Killing Machine" );
  spec.empower_rune_weapon        = find_specialization_spell( "Empower Rune Weapon" );
  spec.pillar_of_frost            = find_specialization_spell( "Pillar of Frost" );
  spec.remorseless_winter         = find_specialization_spell( "Remorseless Winter" );

  // Unholy
  spec.unholy_death_knight        = find_specialization_spell( "Unholy Death Knight" );
  spec.festering_wound            = find_specialization_spell( "Festering Wound" );
  spec.runic_corruption           = find_specialization_spell( "Runic Corruption" );
  spec.sudden_doom                = find_specialization_spell( "Sudden Doom" );
  spec.army_of_the_dead           = find_specialization_spell( "Army of the Dead" );
  spec.dark_transformation        = find_specialization_spell( "Dark Transformation" );
  spec.death_coil                 = find_specialization_spell( "Death Coil" );
  spec.outbreak                   = find_specialization_spell( "Outbreak" );
  spec.scourge_strike             = find_specialization_spell( "Scourge Strike" );
  spec.apocalypse                 = find_specialization_spell( "Apocalypse" );

  mastery.blood_shield            = find_mastery_spell( DEATH_KNIGHT_BLOOD );
  mastery.frozen_heart            = find_mastery_spell( DEATH_KNIGHT_FROST );
  mastery.dreadblade              = find_mastery_spell( DEATH_KNIGHT_UNHOLY );

  // Frost Talents
  // Tier 1
  talent.icy_talons            = find_talent_spell( "Icy Talons" );
  talent.runic_attenuation     = find_talent_spell( "Runic Attenuation" );
  // Tier 2
  talent.murderous_efficiency  = find_talent_spell( "Murderous Efficiency" );
  talent.horn_of_winter        = find_talent_spell( "Horn of Winter" );
  // Tier 3
  talent.icecap                = find_talent_spell( "Icecap" );
  talent.glacial_advance       = find_talent_spell( "Glacial Advance" ); 
  talent.avalanche             = find_talent_spell( "Avalanche" );
  
  // Tier 4  
  talent.inexorable_assault    = find_talent_spell( "Inexorable Assault" ); // TODO ? NYI
  talent.volatile_shielding    = find_talent_spell( "Volatile Shielding" );
    
  // Tier 6
  talent.frostscythe           = find_talent_spell( "Frostscythe" );
  talent.frozen_pulse          = find_talent_spell( "Frozen Pulse" );
  talent.gathering_storm       = find_talent_spell( "Gathering Storm" );
  
  // Tier 7
  talent.obliteration          = find_talent_spell( "Obliteration" );
  talent.breath_of_sindragosa  = find_talent_spell( "Breath of Sindragosa" );
  talent.frostwyrms_fury       = find_talent_spell( "Frostwyrm's Fury" );
	

  // Unholy Talents
  // Tier 1
  talent.infected_claws        = find_talent_spell( "Infected Claws" );
  talent.all_will_serve        = find_talent_spell( "All Will Serve" );
  talent.clawing_shadows       = find_talent_spell( "Clawing Shadows" );

  // Tier 2
  talent.pestilent_pustules    = find_talent_spell( "Pestilent Pustules" );
  talent.harbinger_of_doom     = find_talent_spell( "Harbinger of Doom" );
  talent.soul_reaper           = find_talent_spell( "Soul Reaper" );

  // Tier 3
  talent.bursting_sores        = find_talent_spell( "Bursting Sores" );
  talent.ebon_fever            = find_talent_spell( "Ebon Fever" );
  talent.unholy_blight         = find_talent_spell( "Unholy Blight" );

  // Tier 6
  talent.pestilence            = find_talent_spell( "Pestilence" );
  talent.defile                = find_talent_spell( "Defile" );
  talent.epidemic              = find_talent_spell( "Epidemic" );

  // Tier 7
  talent.army_of_the_damned    = find_talent_spell( "Army of the Damned" );
  talent.unholy_frenzy         = find_talent_spell( "Unholy Frenzy" );
  talent.summon_gargoyle       = find_talent_spell( "Summon Gargoyle" );


  // Blood Talents
  // Tier 1

  talent.heartbreaker           = find_talent_spell( "Heartbreaker" );
  talent.blooddrinker           = find_talent_spell( "Blooddrinker" );
  talent.rune_strike            = find_talent_spell( "Rune Strike"  );

  // Tier 2
  talent.rapid_decomposition    = find_talent_spell( "Rapid Decomposition" );
  talent.hemostasis             = find_talent_spell( "Hemostasis"          );
  talent.consumption            = find_talent_spell( "Consumption"         ); 

  // Tier 3
  talent.will_of_the_necropolis = find_talent_spell( "Will of the Necropolis" ); // NYI
  talent.antimagic_barrier      = find_talent_spell( "Anti-Magic Barrier"     ); 
  talent.rune_tap               = find_talent_spell( "Rune Tap"               );

  // Tier 4 Utility - NYI
  talent.tightening_grasp       = find_talent_spell( "Tightening Gasp"     );
  talent.grip_of_the_dead       = find_talent_spell( "Grip of the Dead"    );
  talent.march_of_the_damned    = find_talent_spell( "March of the Damned" );

  // Tier 5
  talent.foul_bulwark          = find_talent_spell( "Foul Bulwark" );
  talent.ossuary               = find_talent_spell( "Ossuary"      ); 
  talent.tombstone             = find_talent_spell( "Tombstone"    );

  // Tier 6
  talent.voracious             = find_talent_spell( "Voracious"     ); 
  talent.bloodworms            = find_talent_spell( "Bloodworms"    ); 
  talent.mark_of_blood         = find_talent_spell( "Mark of Blood" );

  // Tier 7
  talent.purgatory             = find_talent_spell( "Purgatory"  ); // NYI
  talent.red_thirst            = find_talent_spell( "Red Thirst" );
  talent.bonestorm             = find_talent_spell( "Bonestorm"  );

  // Generic spells
  // Shared
  spell.antimagic_shell        = find_class_spell( "Anti-Magic Shell" );
  spell.death_strike           = find_class_spell( "Death Strike" );
  
  // Blood
  spell.blood_shield           = find_spell( 77535 );
  spell.bone_shield            = find_spell( 195181 );
  spell.dancing_rune_weapon    = find_spell( 81256 );
  spell.gravewarden            = find_spell( 242010 );
  spell.ossuary                = find_spell( 219788 );
  spell.rune_master            = find_spell( 253381 );

  // Frost
  
  // Unholy
  spell.death_coil_damage      = find_spell( 47632 );
  spell.runic_corruption       = find_spell( 51460 );

  // Active Spells
  fallen_crusader += find_spell( 53365 ) -> effectN( 1 ).percent();
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

// death_knight_t::default_apl_blood ========================================

void death_knight_t::default_apl_blood()
{
  action_priority_list_t* def        = get_action_priority_list( "default" );
  action_priority_list_t* standard   = get_action_priority_list( "standard" );
  // action_priority_list_t* cooldowns  = get_action_priority_list( "cooldowns" ); // Not used atm

  // Setup precombat APL for DPS spec
  default_apl_dps_precombat();

  def -> add_action( "auto_attack" );
  def -> add_action( this, "Mind Freeze" );

  // Racials
  def -> add_action( "arcane_torrent,if=runic_power.deficit>20" );
  def -> add_action( "blood_fury" );
  def -> add_action( "berserking,if=buff.dancing_rune_weapon.up" );

  // On-use items
  def -> add_action( "use_items" );

  // Cooldowns
  def -> add_action( "potion,if=buff.dancing_rune_weapon.up" );
  def -> add_action( this, "Dancing Rune Weapon", "if=(!talent.blooddrinker.enabled|!cooldown.blooddrinker.ready)" );
  def -> add_talent( this, "Tombstone", "if=buff.bone_shield.stack>=7" );
  def -> add_action( "call_action_list,name=standard" );

  // Single Target Rotation
  standard -> add_action( this, "Death Strike", "if=runic_power.deficit<10" );
  standard -> add_talent( this, "Blooddrinker", "if=!buff.dancing_rune_weapon.up" );
  standard -> add_action( this, "Marrowrend", "if=buff.bone_shield.remains<=gcd*2" );
  standard -> add_action( this, "Blood Boil", "if=charges_fractional>=1.8&buff.haemostasis.stack<5&(buff.haemostasis.stack<3|!buff.dancing_rune_weapon.up)" );
  standard -> add_action( this, "Marrowrend", "if=(buff.bone_shield.stack<5&talent.ossuary.enabled)|buff.bone_shield.remains<gcd*3" );
  standard -> add_talent( this, "Bonestorm", "if=runic_power>=100&spell_targets.bonestorm>=3" );
  standard -> add_action( this, "Death Strike", "if=buff.blood_shield.up|(runic_power.deficit<15&(runic_power.deficit<25|!buff.dancing_rune_weapon.up))" );
  standard -> add_action( this, "Heart Strike", "if=buff.dancing_rune_weapon.up" );
  standard -> add_action( this, "Death and Decay", "if=buff.crimson_scourge.up" );
  standard -> add_action( this, "Death and Decay" );
  standard -> add_action( this, "Heart Strike", "if=rune.time_to_3<gcd|buff.bone_shield.stack>6" );
}

// death_knight_t::default_potion ===========================================

std::string death_knight_t::default_potion() const
{
  std::string frost_potion = ( true_level > 100 ) ? "prolonged_power" :
                             ( true_level >= 90 ) ? "draenic_strength" :
                             ( true_level >= 85 ) ? "mogu_power" :
                             ( true_level >= 80 ) ? "golemblood_potion" :
                             "disabled";

  std::string unholy_potion = ( true_level > 100 ) ? "prolonged_power" :
                              ( true_level >= 90 ) ? "draenic_strength" :
                              ( true_level >= 85 ) ? "mogu_power" :
                              ( true_level >= 80 ) ? "golemblood_potion" :
                              "disabled";

  std::string blood_potion = ( true_level > 100 ) ? "old_war" :
                              ( true_level >= 90 ) ? "draenic_strength" :
                              ( true_level >= 85 ) ? "mogu_power" :
                              ( true_level >= 80 ) ? "golemblood_potion" :
                              "disabled";

  switch ( specialization() )
  {
    case DEATH_KNIGHT_FROST: return frost_potion;
    case DEATH_KNIGHT_BLOOD: return blood_potion;
    default:                 return unholy_potion;
  }
}

// death_knight_t::default_food ========================================

std::string death_knight_t::default_food() const
{
  std::string frost_food = ( true_level > 100 ) ? "lemon_herb_filet" :
                           ( true_level >  90 ) ? "pickled_eel" :
                           ( true_level >= 85 ) ? "sea_mist_rice_noodles" :
                           ( true_level >= 80 ) ? "seafood_magnifique_feast" :
                           "disabled";

  std::string unholy_food = ( true_level > 100 ) ? "nightborne_delicacy_platter" :
                            ( true_level >  90 ) ? "buttered_sturgeon" :
                            ( true_level >= 85 ) ? "sea_mist_rice_noodles" :
                            ( true_level >= 80 ) ? "seafood_magnifique_feast" :
                            "disabled";

  std::string blood_food =  ( true_level > 100 ) ? "lavish_suramar_feast" :
                            ( true_level >  90 ) ? "pickled_eel" :
                            ( true_level >= 85 ) ? "sea_mist_rice_noodles" :
                            ( true_level >= 80 ) ? "seafood_magnifique_feast" :
                            "disabled";

  switch ( specialization() )
  {
    case DEATH_KNIGHT_FROST: return frost_food;
    case DEATH_KNIGHT_BLOOD: return blood_food;
    default:                 return unholy_food;
  }
}

// death_knight_t::default_flask ============================================

std::string death_knight_t::default_flask() const
{
  std::string flask_name = ( true_level >  100 ) ? "countless_armies" :
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
  return ( true_level >= 110 ) ? "defiled" :
         ( true_level >= 100 ) ? "stout" :
         "disabled";
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


  // Setup precombat APL for DPS spec
  default_apl_dps_precombat();

  def -> add_action( "auto_attack" );

  // Interrupt
  def -> add_action( this, "Mind Freeze" );

  // Choose APL
  def -> add_action( "call_action_list,name=cooldowns" );
  def -> add_action( "run_action_list,name=bos_pooling,if=talent.breath_of_sindragosa.enabled&cooldown.breath_of_sindragosa.remains<15" );
  def -> add_action( "run_action_list,name=bos_ticking,if=dot.breath_of_sindragosa.ticking" );
  def -> add_action( "run_action_list,name=obliteration,if=buff.obliteration.up" );
  def -> add_action( "call_action_list,name=standard" );

  // "Breath of Sindragosa pooling rotation : starts 15s before the cd becomes available"
  bos_pooling -> add_action( this, "Remorseless Winter", "if=talent.gathering_storm.enabled", "Breath of Sindragosa pooling rotation : starts 15s before the cd becomes available" );
  bos_pooling -> add_action( this, "Howling Blast", "if=buff.rime.up&rune.time_to_4<(gcd*2)" );
  bos_pooling -> add_action( this, "Obliterate", "if=rune.time_to_6<gcd&!talent.gathering_storm.enabled" );
  bos_pooling -> add_action( this, "Obliterate", "if=rune.time_to_4<gcd&(cooldown.breath_of_sindragosa.remains|runic_power.deficit>=30)" );
  bos_pooling -> add_action( this, "Frost Strike", "if=runic_power.deficit<5&set_bonus.tier19_4pc&cooldown.breath_of_sindragosa.remains" );
  bos_pooling -> add_action( this, "Remorseless Winter", "if=buff.rime.up&equipped.perseverance_of_the_ebon_martyr" );
  bos_pooling -> add_action( this, "Howling Blast", "if=buff.rime.up&(buff.remorseless_winter.up|cooldown.remorseless_winter.remains>gcd|(!equipped.perseverance_of_the_ebon_martyr&!talent.gathering_storm.enabled))" );
  bos_pooling -> add_action( this, "Obliterate", "if=!buff.rime.up&!(talent.gathering_storm.enabled&!(cooldown.remorseless_winter.remains>(gcd*2)|rune>4))&rune>3" );
  bos_pooling -> add_action( this, "Frost Strike", "if=runic_power.deficit<30" );
  bos_pooling -> add_talent( this, "Frostscythe", "if=buff.killing_machine.react&(!equipped.koltiras_newfound_will|spell_targets.frostscythe>=2)" );
  bos_pooling -> add_talent( this, "Glacial Advance", "if=spell_targets.glacial_advance>=2" );
  bos_pooling -> add_action( this, "Remorseless Winter", "if=spell_targets.remorseless_winter>=2" );
  bos_pooling -> add_talent( this, "Frostscythe", "if=spell_targets.frostscythe>=3" );
  bos_pooling -> add_action( this, "Frost Strike", "if=(cooldown.remorseless_winter.remains<(gcd*2)|buff.gathering_storm.stack=10)&cooldown.breath_of_sindragosa.remains>rune.time_to_4&talent.gathering_storm.enabled" );
  bos_pooling -> add_action( this, "Obliterate", "if=!buff.rime.up&(!talent.gathering_storm.enabled|cooldown.remorseless_winter.remains>gcd)" );
  bos_pooling -> add_action( this, "Frost Strike", "if=cooldown.breath_of_sindragosa.remains>rune.time_to_4" );

  // Breath of Sindragosa uptime rotation
  bos_ticking -> add_action( this, "Remorseless Winter", "if=runic_power>=30&((buff.rime.up&equipped.perseverance_of_the_ebon_martyr)|(talent.gathering_storm.enabled&(buff.remorseless_winter.remains<=gcd|!buff.remorseless_winter.remains)))", "Breath of Sindragosa uptime rotation" );
  bos_ticking -> add_action( this, "Howling Blast", "if=((runic_power>=20&set_bonus.tier19_4pc)|runic_power>=30)&buff.rime.up" );
  bos_ticking -> add_action( this, "Frost Strike", "if=set_bonus.tier20_2pc&runic_power.deficit<=15&rune<=3&buff.pillar_of_frost.up" );
  bos_ticking -> add_action( this, "Obliterate", "if=runic_power<=45|rune.time_to_5<gcd" );
  bos_ticking -> add_talent( this, "Horn of Winter", "if=runic_power.deficit>=30&rune.time_to_3>gcd" );
  bos_ticking -> add_talent( this, "Frostscythe", "if=buff.killing_machine.react&(!equipped.koltiras_newfound_will|talent.gathering_storm.enabled|spell_targets.frostscythe>=2)" );
  bos_ticking -> add_talent( this, "Glacial Advance", "if=spell_targets.glacial_advance>=2" );
  bos_ticking -> add_action( this, "Remorseless Winter", "if=spell_targets.remorseless_winter>=2" );
  bos_ticking -> add_action( this, "Obliterate", "if=runic_power.deficit>25|rune>3" );
  bos_ticking -> add_action( this, "Empower Rune Weapon", "if=runic_power<30&rune.time_to_2>gcd" );

  // Racials
  cooldowns -> add_action( "arcane_torrent,if=runic_power.deficit>=20&!talent.breath_of_sindragosa.enabled" );
  cooldowns -> add_action( "arcane_torrent,if=dot.breath_of_sindragosa.ticking&runic_power.deficit>=50&rune<2" );
  cooldowns -> add_action( "blood_fury,if=buff.pillar_of_frost.up" );
  cooldowns -> add_action( "berserking,if=buff.pillar_of_frost.up" );

  // On-use itemos
  cooldowns -> add_action( "use_items" );
  cooldowns -> add_action( "use_item,name=ring_of_collapsing_futures,"
                           "if=(buff.temptation.stack=0&target.time_to_die>60)|target.time_to_die<60" );
  cooldowns -> add_action( "use_item,name=horn_of_valor,"
                           "if=buff.pillar_of_frost.up&(!talent.breath_of_sindragosa.enabled|!cooldown.breath_of_sindragosa.remains)" );
  cooldowns -> add_action( "use_item,name=draught_of_souls,"
                           "if=rune.time_to_5<3&(!dot.breath_of_sindragosa.ticking|runic_power>60)" );
  cooldowns -> add_action( "use_item,name=feloiled_infernal_machine,"
                           "if=!talent.obliteration.enabled|buff.obliteration.up" );

  // In-combat potion
  cooldowns -> add_action( "potion,if=buff.pillar_of_frost.up&(dot.breath_of_sindragosa.ticking|buff.obliteration.up)" );

  // Pillar of Frost
  cooldowns -> add_action( this, "Pillar of Frost", "if=talent.obliteration.enabled&(cooldown.obliteration.remains>20|cooldown.obliteration.remains<10|!talent.icecap.enabled)", "Pillar of frost conditions" );
  cooldowns -> add_action( this, "Pillar of Frost", "if=talent.breath_of_sindragosa.enabled&cooldown.breath_of_sindragosa.ready&runic_power>50" );
  cooldowns -> add_action( this, "Pillar of Frost", "if=talent.breath_of_sindragosa.enabled&cooldown.breath_of_sindragosa.remains>40" );

  // Tier 100 cooldowns + Cold Heart
  cooldowns -> add_talent( this, "Breath of Sindragosa", "if=buff.pillar_of_frost.up" );
  cooldowns -> add_action( "call_action_list,name=cold_heart,if=equipped.cold_heart&((buff.cold_heart.stack>=10&!buff.obliteration.up&debuff.razorice.stack=5)|target.time_to_die<=gcd)" );
  cooldowns -> add_talent( this, "Obliteration", "if=rune>=1&runic_power>=20&(!talent.frozen_pulse.enabled|rune<2|buff.pillar_of_frost.remains<=12)&(!talent.gathering_storm.enabled|!cooldown.remorseless_winter.ready)&(buff.pillar_of_frost.up|!talent.icecap.enabled)" );

  // Cold Heart conditionals
  cold_heart -> add_action( this, "Chains of Ice", "if=buff.cold_heart.stack=20&buff.unholy_strength.react&cooldown.pillar_of_frost.remains>6", "Cold heart conditions" );
  cold_heart -> add_action( this, "Chains of Ice", "if=buff.cold_heart.stack>=16&(cooldown.obliteration.ready&talent.obliteration.enabled)&buff.pillar_of_frost.up" );
  cold_heart -> add_action( this, "Chains of Ice", "if=buff.pillar_of_frost.up&buff.pillar_of_frost.remains<gcd&(buff.cold_heart.stack>=11|(buff.cold_heart.stack>=10&set_bonus.tier20_4pc))" );
  cold_heart -> add_action( this, "Chains of Ice", "if=buff.cold_heart.stack>=17&buff.unholy_strength.react&buff.unholy_strength.remains<gcd&cooldown.pillar_of_frost.remains>6" );
  cold_heart -> add_action( this, "Chains of Ice", "if=buff.cold_heart.stack>=4&target.time_to_die<=gcd" );

  // Obliteration rotation
  obliteration -> add_action( this, "Remorseless Winter", "if=talent.gathering_storm.enabled", "Obliteration rotation" );
  obliteration -> add_talent( this, "Frostscythe", "if=(buff.killing_machine.up&(buff.killing_machine.react|prev_gcd.1.frost_strike|prev_gcd.1.howling_blast))&spell_targets.frostscythe>1" );
  obliteration -> add_action( this, "Obliterate", "if=(buff.killing_machine.up&(buff.killing_machine.react|prev_gcd.1.frost_strike|prev_gcd.1.howling_blast))|(spell_targets.howling_blast>=3&!buff.rime.up&!talent.frostscythe.enabled)" );
  obliteration -> add_action( this, "Howling Blast", "if=buff.rime.up&spell_targets.howling_blast>1" );
  obliteration -> add_action( this, "Frost Strike", "if=!buff.rime.up|rune.time_to_1>=gcd|runic_power.deficit<20" );
  obliteration -> add_action( this, "Howling Blast", "if=buff.rime.up" );
  obliteration -> add_action( this, "Obliterate" );

  // Standard rotation
  standard -> add_action( this, "Frost Strike", "if=talent.icy_talons.enabled&buff.icy_talons.remains<=gcd", "Standard rotation" );
  standard -> add_action( this, "Remorseless Winter", "if=(buff.rime.up&equipped.perseverance_of_the_ebon_martyr)|talent.gathering_storm.enabled" );
  standard -> add_action( this, "Obliterate", "if=(equipped.koltiras_newfound_will&talent.frozen_pulse.enabled&set_bonus.tier19_2pc=1)|rune.time_to_4<gcd" );
  standard -> add_action( this, "Frost Strike", "if=runic_power.deficit<10" );
  standard -> add_action( this, "Howling Blast", "if=buff.rime.up" );
  standard -> add_action( this, "Obliterate", "if=(equipped.koltiras_newfound_will&talent.frozen_pulse.enabled&set_bonus.tier19_2pc=1)|rune.time_to_5<gcd" );
  standard -> add_action( this, "Frost Strike", "if=runic_power.deficit<10" );
  standard -> add_talent( this, "Frostscythe", "if=buff.killing_machine.react&(!equipped.koltiras_newfound_will|spell_targets.frostscythe>=2)" );
  standard -> add_action( this, "Obliterate", "if=buff.killing_machine.react" );
  standard -> add_action( this, "Frost Strike", "if=runic_power.deficit<20" );
  standard -> add_action( this, "Remorseless Winter", "if=spell_targets.remorseless_winter>=2" );
  standard -> add_talent( this, "Glacial Advance", "if=spell_targets.glacial_advance>=2" );
  standard -> add_talent( this, "Frostscythe", "if=spell_targets.frostscythe>=3" );
  standard -> add_action( this, "Obliterate", "if=!talent.gathering_storm.enabled|cooldown.remorseless_winter.remains>(gcd*2)" );
  standard -> add_talent( this, "Horn of Winter", "if=rune.time_to_2>gcd|!talent.frozen_pulse.enabled" );
  standard -> add_action( this, "Frost Strike", "if=!(runic_power<50&talent.obliteration.enabled&cooldown.obliteration.remains<=gcd)" );
  standard -> add_action( this, "Obliterate", "if=!talent.gathering_storm.enabled|talent.icy_talons.enabled" );
  standard -> add_action( this, "Empower Rune Weapon", "if=!talent.breath_of_sindragosa.enabled|target.time_to_die<cooldown.breath_of_sindragosa.remains" );
}

void death_knight_t::default_apl_unholy()
{
  action_priority_list_t* precombat  = get_action_priority_list( "precombat"  );
  action_priority_list_t* def        = get_action_priority_list( "default"    );
  action_priority_list_t* generic    = get_action_priority_list( "generic"    );
  action_priority_list_t* aoe        = get_action_priority_list( "aoe"        );
  action_priority_list_t* cooldowns  = get_action_priority_list( "cooldowns"  );
  action_priority_list_t* cold_heart = get_action_priority_list( "cold_heart" );

  // Setup precombat APL for DPS spec
  default_apl_dps_precombat();

  precombat -> add_action( this, "Raise Dead" );
  precombat -> add_action( this, "Army of the Dead" );

  def -> add_action( "auto_attack" );
  def -> add_action( this, "Mind Freeze" );
  
  // Ogcd cooldowns
  def -> add_action( "arcane_torrent,if=runic_power.deficit>20", "Racials, Items, and other ogcds" );
  def -> add_action( "blood_fury" );
  def -> add_action( "berserking" );
  def -> add_action( "use_items" );
  def -> add_action( "use_item,name=ring_of_collapsing_futures,"
                  "if=(buff.temptation.stack=0&target.time_to_die>60)|target.time_to_die<60" );
  def -> add_action( "potion,if=buff.unholy_strength.react" );
  // Maintain Virulent Plague
  def -> add_action( this, "Outbreak", "target_if=(dot.virulent_plague.tick_time_remains+tick_time<=dot.virulent_plague.remains)&dot.virulent_plague.remains<=gcd", "Maintain Virulent Plague" );
  // Action Lists
  def -> add_action( "call_action_list,name=cooldowns" );
  def -> add_action( "call_action_list,name=generic" );

  cooldowns -> add_action( "call_action_list,name=cold_heart,if=equipped.cold_heart&buff.cold_heart.stack>10", "Cold heart and other on-gcd cooldowns" );
  cooldowns -> add_action( this, "Army of the Dead" );
  cooldowns -> add_action( this, "Summon Gargoyle", "if=(!equipped.137075|cooldown.dark_transformation.remains<10)&rune.time_to_4>=gcd" );
  cooldowns -> add_talent( this, "Soul Reaper", "if=debuff.festering_wound.stack>=3&rune>=3" );
  cooldowns -> add_action( this, "Dark Transformation", "if=rune.time_to_4>=gcd" );

  // Cold Heart
  cold_heart -> add_action( this, "Chains of ice", "if=buff.unholy_strength.remains<gcd&buff.unholy_strength.react&buff.cold_heart.stack>16", "Cold Heart legendary" );
  cold_heart -> add_action( this, "Chains of ice", "if=buff.master_of_ghouls.remains<gcd&buff.master_of_ghouls.up&buff.cold_heart.stack>17" );
  cold_heart -> add_action( this, "Chains of ice", "if=buff.cold_heart.stack=20&buff.unholy_strength.react" );

  generic -> add_action( this, "Death Coil", "if=runic_power.deficit<22" );
  generic -> add_talent( this, "Defile" );
  generic -> add_action( "call_action_list,name=aoe,if=active_enemies>=2", "Switch to aoe" );
  // Wounds management
  generic -> add_action( this, "Festering Strike", "if=debuff.festering_wound.stack<=2|rune.time_to_4<=gcd", "Wounds management" );
  generic -> add_action( this, "Scourge Strike", "if=(buff.unholy_strength.react|rune>=2)&debuff.festering_wound.stack>=1&debuff.festering_wound.stack>=3|(cooldown.army_of_the_dead.remains>5|rune.time_to_4<=gcd)" );
  generic -> add_talent( this, "Clawing Shadows", "if=(buff.unholy_strength.react|rune>=2)&debuff.festering_wound.stack>=1&debuff.festering_wound.stack>=3|(cooldown.army_of_the_dead.remains>5|rune.time_to_4<=gcd)" );
 
  // Generic AOE actions to be done
  aoe -> add_action( this, "Death and Decay", "if=spell_targets.death_and_decay>=2", "AoE rotation" );
  aoe -> add_talent( this, "Epidemic", "if=spell_targets.epidemic>4" );
  aoe -> add_action( this, "Scourge Strike", "if=spell_targets.scourge_strike>=2&(death_and_decay.ticking|defile.ticking)" );
  aoe -> add_talent( this, "Clawing Shadows", "if=spell_targets.clawing_shadows>=2&(death_and_decay.ticking|defile.ticking)" );
  aoe -> add_talent( this, "Epidemic", "if=spell_targets.epidemic>2" );
}

// death_knight_t::init_actions =============================================

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

bool death_knight_t::init_actions()
{
  active_spells.blood_plague = new blood_plague_t( this );
  active_spells.frost_fever = new frost_fever_t( this );

  return player_t::init_actions();
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

  buffs.blood_shield        = new blood_shield_buff_t( this );
  buffs.rune_tap            = buff_creator_t( this, "rune_tap", talent.rune_tap )
                              .cd( timespan_t::zero() );

  buffs.antimagic_shell     = new antimagic_shell_buff_t( this );


  buffs.sephuzs_secret = new sephuzs_secret_buff_t( this );

  buffs.bone_shield   = make_buff<haste_buff_t>( this, "bone_shield", spell.bone_shield );
  buffs.bone_shield -> set_default_value( 1.0 / ( 1.0 + spell.bone_shield -> effectN( 4 ).percent() ) )
                              -> set_stack_change_callback( talent.foul_bulwark -> ok() ? [ this ]( buff_t*, int old_stacks, int new_stacks ) {
                                double old_buff = old_stacks * talent.foul_bulwark -> effectN( 1 ).percent();
                                double new_buff = new_stacks * talent.foul_bulwark -> effectN( 1 ).percent();
                                resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + old_buff;
                                resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + new_buff;
                                recalculate_resource_max( RESOURCE_HEALTH );
                              } : buff_stack_change_callback_t() )
                              -> set_max_stack( spell.bone_shield -> max_stacks() )
                              -> add_invalidate( CACHE_BONUS_ARMOR );
  buffs.crimson_scourge     = buff_creator_t( this, "crimson_scourge", find_spell( 81141 ) )
                              .trigger_spell( spec.crimson_scourge );
  buffs.dancing_rune_weapon = new dancing_rune_weapon_buff_t( this );
  buffs.dark_transformation = buff_creator_t( this, "dark_transformation", spec.dark_transformation )
    .duration( spec.dark_transformation -> duration() )
    .cd( timespan_t::zero() ); // Handled by the action

  buffs.death_and_decay     = buff_creator_t( this, "death_and_decay", find_spell( 188290 ) )
                              .period( timespan_t::from_seconds( 1.0 ) )
                              .duration( find_spell( 188290 ) -> duration() );
  buffs.gathering_storm     = buff_creator_t( this, "gathering_storm", find_spell( 211805 ) )
                              .trigger_spell( talent.gathering_storm )
                              .default_value( find_spell( 211805 ) -> effectN( 1 ).percent() );
  buffs.hemostasis          = buff_creator_t( this, "hemostasis", talent.hemostasis -> effectN( 1 ).trigger() )
                              .trigger_spell( talent.hemostasis )
                              .default_value( talent.hemostasis -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );
  buffs.icebound_fortitude  = buff_creator_t( this, "icebound_fortitude", spec.icebound_fortitude )
                              .duration( spec.icebound_fortitude -> duration() )
                              .cd( timespan_t::zero() );
  buffs.icy_talons          = make_buff<haste_buff_t>( this, "icy_talons", talent.icy_talons -> effectN( 1 ).trigger() ) ;
  buffs.icy_talons->add_invalidate( CACHE_ATTACK_SPEED )
      ->set_default_value( find_spell( 194879 ) -> effectN( 1 ).percent() )
      ->set_trigger_spell( talent.icy_talons );
  buffs.killing_machine     = buff_creator_t( this, "killing_machine", spec.killing_machine -> effectN( 1 ).trigger() )
                              .trigger_spell( spec.killing_machine )
                              .default_value( find_spell( 51124 ) -> effectN( 1 ).percent() );
  buffs.obliteration        = buff_creator_t( this, "obliteration", talent.obliteration )
                              .cd( timespan_t::zero() ); // Handled by action
  buffs.pillar_of_frost     = buff_creator_t( this, "pillar_of_frost", spec.pillar_of_frost )
    .cd( timespan_t::zero() )
    .default_value( spec.pillar_of_frost -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_STRENGTH );
  buffs.toravons            = buff_creator_t( this, "toravons_whiteout_bindings", find_spell( 205658 ) )
                              .default_value( find_spell( 205658 ) -> effectN( 1 ).percent() )
                              .duration( spec.pillar_of_frost -> duration() )
                              .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.rime                = buff_creator_t( this, "rime", spec.rime -> effectN( 1 ).trigger() )
                              .trigger_spell( spec.rime )
                              .chance( spec.rime -> effectN( 2 ).percent() + sets -> set( DEATH_KNIGHT_FROST, T19, B2 ) -> effectN( 1 ).percent() );
  buffs.riposte             = make_buff<stat_buff_t>( this, "riposte", spec.riposte -> effectN( 1 ).trigger() )
                              ->add_stat( STAT_CRIT_RATING, 0 );
  buffs.riposte ->set_cooldown( spec.riposte -> internal_cooldown() )
      -> set_chance( spec.riposte -> proc_chance() );
  buffs.runic_corruption    = new runic_corruption_buff_t( this );
  buffs.sudden_doom         = buff_creator_t( this, "sudden_doom", spec.sudden_doom -> effectN( 1 ).trigger() )
                              .rppm_scale( RPPM_ATTACK_SPEED ) // 2016-08-08: Hotfixed, not in spell data
                              .rppm_mod( 1.0 + talent.harbinger_of_doom -> effectN( 2 ).percent() )
                              .trigger_spell( spec.sudden_doom )
                              .max_stack( specialization() == DEATH_KNIGHT_UNHOLY ? as<unsigned int>(
                                spec.sudden_doom -> effectN( 1 ).trigger() -> max_stacks() + talent.harbinger_of_doom -> effectN( 1 ).base_value() ):
                                1 );
  buffs.vampiric_blood      = new vampiric_blood_buff_t( this );
  buffs.voracious           = buff_creator_t( this, "voracious", find_spell( 274009 ) )
                              .trigger_spell( talent.voracious )
                              .add_invalidate( CACHE_LEECH );
  buffs.will_of_the_necropolis = buff_creator_t( this, "will_of_the_necropolis", find_spell( 157335 ) )
                                 .cd( find_spell( 157335 ) -> duration() );

  runeforge.rune_of_the_fallen_crusader = buff_creator_t( this, "unholy_strength", find_spell( 53365 ) )
                                          .add_invalidate( CACHE_STRENGTH );
  runeforge.rune_of_the_stoneskin_gargoyle = buff_creator_t( this, "stoneskin_gargoyle", find_spell( 62157 ) )
                                             .add_invalidate( CACHE_ARMOR )
                                             .add_invalidate( CACHE_STAMINA )
                                             .chance( 0 );

  buffs.unholy_frenzy = make_buff<haste_buff_t>( this, "unholy_frenzy", talent.unholy_frenzy );
  buffs.unholy_frenzy -> set_default_value( talent.unholy_frenzy -> effectN( 1 ).percent() );
  buffs.soul_reaper = make_buff<haste_buff_t>( this, "soul_reaper", find_spell( 215711 ) );
  buffs.soul_reaper -> set_default_value( find_spell( 215711 ) -> effectN( 1 ).percent() );
  buffs.tombstone = make_buff<absorb_buff_t>( this, "tombstone", talent.tombstone );
  buffs.tombstone -> set_cooldown( timespan_t::zero() ); // Handled by the action
  buffs.t19oh_8pc = make_buff<stat_buff_t>( this, "deathlords_might", sets -> set( specialization(), T19OH, B8 ) -> effectN( 1 ).trigger() );
  buffs.t19oh_8pc -> set_trigger_spell( sets -> set( specialization(), T19OH, B8 ) );

  buffs.frozen_pulse = buff_creator_t( this, "frozen_pulse", talent.frozen_pulse );

  // Must be created after Gathering Storms buff (above) to get correct linkage
  buffs.remorseless_winter = new remorseless_winter_buff_t( this );

  buffs.empower_rune_weapon = new empower_rune_weapon_buff_t( this );
  buffs.empower_rune_weapon_haste = make_buff<haste_buff_t>( this, "empower_rune_weapon_haste", spec.empower_rune_weapon );
  buffs.empower_rune_weapon_haste->set_default_value( spec.empower_rune_weapon -> effectN( 3 ).percent() )
    ->set_trigger_spell( spec.empower_rune_weapon );

  buffs.t20_2pc_unholy = buff_creator_t( this, "master_of_ghouls", find_spell( 246995 ) )
    .trigger_spell( sets -> set( DEATH_KNIGHT_UNHOLY, T20, B2 ) )
    .default_value( find_spell( 246995 ) -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .refresh_behavior( buff_refresh_behavior::EXTEND );
  buffs.t20_4pc_frost = buff_creator_t( this, "icy edge" )
    .default_value( sets -> set( DEATH_KNIGHT_FROST, T20, B4 ) -> effectN( 2 ).base_value() )
    .chance( sets -> has_set_bonus( DEATH_KNIGHT_FROST, T20, B4 ) );
  buffs.t20_blood = make_buff<stat_buff_t>( this, "gravewarden", spell.gravewarden )
    ->add_stat( STAT_VERSATILITY_RATING, spell.gravewarden -> effectN( 1 ).base_value() );
  buffs.t21_4p_blood = new rune_master_buff_t( this );
    
}

// death_knight_t::init_gains ===============================================

void death_knight_t::init_gains()
{
  player_t::init_gains();

  gains.antimagic_shell                  = get_gain( "Antimagic Shell"            );
  gains.horn_of_winter                   = get_gain( "Horn of Winter"             );
  gains.frost_fever                      = get_gain( "Frost Fever"                );
  gains.festering_wound                  = get_gain( "Festering Wound"            );
  gains.murderous_efficiency             = get_gain( "Murderous Efficiency"       );
  gains.power_refund                     = get_gain( "power_refund"               );
  gains.rune                             = get_gain( "Rune Regeneration"          );
  gains.runic_empowerment                = get_gain( "Runic Empowerment"          );
  gains.empower_rune_weapon              = get_gain( "Empower Rune Weapon"        );
  gains.rc                               = get_gain( "runic_corruption_all"       );
  gains.runic_attenuation                = get_gain( "Runic Attenuation"          );;
  gains.tombstone                        = get_gain( "Tombstone"                  );
  gains.t19_4pc_blood                    = get_gain( "Tier19 Blood 4PC"           );
  gains.draugr_girdle_everlasting_king   = get_gain( "Draugr, Girdle of the Everlasting King" );
  gains.uvanimor_the_unbeautiful         = get_gain( "Uvanimor, the Unbeautiful"  );
  gains.koltiras_newfound_will           = get_gain( "Koltira's Newfound Will"    );
  gains.t19_4pc_frost                    = get_gain( "Tier19 Frost 4PC"           );
  gains.start_of_combat_overflow         = get_gain( "Start of Combat Overflow"   );
  gains.shackles_of_bryndaor             = get_gain( "Shackles of Bryndaor"       );
  gains.heartbreaker                     = get_gain( "Heartbreaker"               );
  gains.drw_heart_strike                 = get_gain( "Rune Weapon Heart Strike"   );
  gains.rune_strike                      = get_gain( "Rune Strike"                );
  gains.soul_reaper                      = get_gain( "Soul Reaper"                );
}

// death_knight_t::init_procs ===============================================

void death_knight_t::init_procs()
{
  player_t::init_procs();

  procs.runic_empowerment        = get_proc( "Runic Empowerment"            );
  procs.runic_empowerment_wasted = get_proc( "Wasted Runic Empowerment"     );
  procs.oblit_killing_machine    = get_proc( "Killing Machine: Obliterate"  );
  procs.fs_killing_machine       = get_proc( "Killing Machine: Frostscythe" );

  procs.ready_rune               = get_proc( "Rune ready" );

  procs.t19_2pc_unholy           = get_proc( "Runic Corruption : T19 2P "             );
  procs.rp_runic_corruption      = get_proc( "Runic Corruption : Runic Power Spent"   );
  procs.pp_runic_corruption      = get_proc( "Runic Corruption : Pestilent Pustules " );

  procs.bloodworms               = get_proc( "Bloodworms" );
}

// death_knight_t::init_absorb_priority =====================================

double death_knight_t::bone_shield_handler( const action_state_t* state ) const
{
  if ( ! buffs.bone_shield -> up() )
  {
    return 0;
  }
  
  // Bone shield only lose stacks on auto-attack damage. The only distinguishing feature of auto attacks is that
  // our enemies call them "melee_main_hand" and "melee_off_hand", so we need to check for "hand" in name_str
  if ( util::str_in_str_ci( state -> action -> name_str, "_hand" ) && cooldown.bone_shield_icd -> up() )
  {
    buffs.bone_shield -> decrement();
    
    if ( sets -> has_set_bonus( DEATH_KNIGHT_BLOOD, T21, B2 ) )
    {
      cooldown.dancing_rune_weapon -> adjust( timespan_t::from_millis( sets -> set( DEATH_KNIGHT_BLOOD, T21, B2) -> effectN( 1 ).base_value() ), false );
    }
    cooldown.bone_shield_icd -> start();
  }

  double absorbed = 0;
  double absorb_pct = 0; // TODO turn absorb into armor

  // Legendary pants increase the absorbed amount by 2%
  absorbed = absorb_pct * state -> result_amount;
  
  return absorbed;
}

void death_knight_t::init_absorb_priority()
{
  player_t::init_absorb_priority();
  
  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    instant_absorb_list.insert( std::make_pair<unsigned, instant_absorb_t>(
        195181, instant_absorb_t( this, spell.bone_shield, "bone_shield",
                                  std::bind( &death_knight_t::bone_shield_handler, this, std::placeholders::_1 ) ) ) );

    // TODO: What is the absorb ordering for blood dks?
    absorb_priority.push_back( 195181 ); // Bone Shield (NYI)
    //absorb_priority.push_back( 77535  ); // Blood Shield
    //absorb_priority.push_back( 219809 ); // Tombstone
  }
}

// death_knight_t::init_finished ============================================

bool death_knight_t::init_finished()
{
  auto ret = player_t::init_finished();

  if ( deprecated_dnd_expression )
  {
    sim -> errorf( "Player %s, Death and Decay and Defile expressions of the form "
                   "'dot.death_and_decay.X' have been deprecated. Use 'death_and_decay.ticking' "
                   "or death_and_decay.remains' instead.", name() );
  }

  return ret;
}

// death_knight_t::reset ====================================================

void death_knight_t::reset()
{
  player_t::reset();

  runic_power_decay_rate = 1; // 1 RP per second decay
  antimagic_shell_absorbed = 0.0;
  _runes.reset();
  dnds.clear();
  t20_2pc_frost = 0;
  t20_4pc_frost = 0;
}

// death_knight_t::assess_heal ==============================================

void death_knight_t::assess_heal( school_e school, dmg_e t, action_state_t* s )
{
  if ( buffs.vampiric_blood -> up() )
    s -> result_total *= 1.0 + buffs.vampiric_blood -> data().effectN( 4 ).percent();

  player_t::assess_heal( school, t, s );
}

// death_knight_t::assess_damage_imminent ===================================

void death_knight_t::assess_damage_imminent( school_e school, dmg_e, action_state_t* s )
{
  if ( school != SCHOOL_PHYSICAL )
  {
    if ( buffs.antimagic_shell -> up() )
    {
      double absorbed = s -> result_amount * spell.antimagic_shell -> effectN( 1 ).percent();
      antimagic_shell_absorbed += absorbed;

      double max_hp_absorb = resources.max[RESOURCE_HEALTH] * 0.4;

      max_hp_absorb *= 1.0 + talent.volatile_shielding -> effectN( 1 ).percent();
      max_hp_absorb *= 1.0 + talent.antimagic_barrier -> effectN( 3 ).percent();

      if ( antimagic_shell_absorbed > max_hp_absorb )
      {
        absorbed = antimagic_shell_absorbed - max_hp_absorb;
        antimagic_shell_absorbed = -1.0; // Set to -1.0 so expire_override knows that we don't need to reduce cooldown from regenerative magic.
        buffs.antimagic_shell -> expire();
      }

      double generated = absorbed / resources.max[RESOURCE_HEALTH];

      s -> result_amount -= absorbed;
      s -> result_absorbed -= absorbed;
      s -> self_absorb_amount += absorbed;
      iteration_absorb_taken += absorbed;

      //gains.antimagic_shell -> add( RESOURCE_HEALTH, absorbed );

      if ( antimagic_shell )
        antimagic_shell -> add_result( absorbed, absorbed, ABSORB, RESULT_HIT, BLOCK_RESULT_UNBLOCKED, this );

      resource_gain( RESOURCE_RUNIC_POWER, util::round( generated * 100.0 ), gains.antimagic_shell, s -> action );
    }
  }
}

// death_knight_t::assess_damage ============================================

void death_knight_t::assess_damage( school_e     school,
                                    dmg_e        dtype,
                                    action_state_t* s )
{
  player_t::assess_damage( school, dtype, s );
  if ( s -> result == RESULT_DODGE || s -> result == RESULT_PARRY )
  {
    buffs.riposte -> stats[ 0 ].amount = ( current.stats.dodge_rating + current.stats.parry_rating ) * spec.riposte -> effectN( 1 ).percent();
    buffs.riposte -> trigger();
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

// death_knight_t::composite_armor_multiplier ===============================

double death_knight_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  a *= 1.0 + spec.veteran_of_the_third_war -> effectN( 2 ).percent();

  if ( runeforge.rune_of_the_stoneskin_gargoyle -> check() )
    a *= 1.0 + runeforge.rune_of_the_stoneskin_gargoyle -> data().effectN( 1 ).percent();

  return a;
}

// death_knight_t::composite_bonus_armor =========================================

double death_knight_t::composite_bonus_armor() const
{
  double ba = player_t::composite_bonus_armor();

  if ( buffs.bone_shield -> check() )
  {
    ba += spell.bone_shield -> effectN( 1 ).percent() * cache.attack_power();
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
      m *= 1.0 + fallen_crusader;
    }
    m *= 1.0 + buffs.pillar_of_frost -> value();
  }

  else if ( attr == ATTR_STAMINA )
  {
    m *= 1.0 + spec.veteran_of_the_third_war -> effectN( 4 ).percent();
    if ( runeforge.rune_of_the_stoneskin_gargoyle -> check() )
      m *= 1.0 + runeforge.rune_of_the_stoneskin_gargoyle -> data().effectN( 2 ).percent();
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

  m += buffs.voracious -> data().effectN( 1 ).percent();

  return m;
}

// death_knight_t::composite_melee_expertise ===============================

double death_knight_t::composite_melee_expertise( const weapon_t* ) const
{
  double expertise = player_t::composite_melee_expertise( nullptr );

  expertise += spec.veteran_of_the_third_war -> effectN( 5 ).percent();

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

// death_knight_t::composite_dodge ============================================

double death_knight_t::composite_dodge() const
{
  double dodge = player_t::composite_dodge();

  return dodge;
}

// death_knight_t::composite_player_multiplier ==============================

double death_knight_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( dbc::is_school( school, SCHOOL_FROST ) && buffs.toravons -> up() )
  {
    m *= 1.0 + buffs.toravons -> default_value;
  }

  return m;
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

  // Note, done here so the multiplier won't cache. Otherwise things would get hairy in real
  // distance-based calculations where the actor moves out of their own DnD/Defile. Tiny performance
  // hit.
  if ( legendary.lanathels_lament -> ok() && in_death_and_decay() )
  {
    m *= 1.0 + legendary.lanathels_lament -> effectN( 1 ).percent();
  }

  return m;
}

// death_knight_t::composite_player_dh/th_multiplier =============================

double death_knight_t::composite_player_dh_multiplier( school_e school ) const
{
  auto m = player_t::composite_player_dh_multiplier( school );

  // Note, done here so the multiplier won't cache. Otherwise things would get hairy in real
  // distance-based calculations where the actor moves out of their own DnD/Defile. Tiny performance
  // hit.
  if ( legendary.lanathels_lament -> ok() && in_death_and_decay() )
  {
    m *= 1.0 + legendary.lanathels_lament -> effectN( 2 ).percent();
  }

  return m;
}

double death_knight_t::composite_player_th_multiplier( school_e school ) const
{
  auto m = player_t::composite_player_th_multiplier( school );

  // Note, done here so the multiplier won't cache. Otherwise things would get hairy in real
  // distance-based calculations where the actor moves out of their own DnD/Defile. Tiny performance
  // hit.
  if ( legendary.lanathels_lament -> ok() && in_death_and_decay() )
  {
    m *= 1.0 + legendary.lanathels_lament -> effectN( 2 ).percent();
  }

  return m;
}

// death_knight_t::composite_player_critical_damage_multiplier ===================

double death_knight_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_critical_damage_multiplier( s );

  return m;
}

// death_knight_t::composite_melee_attack_power ==================================

double death_knight_t::composite_melee_attack_power() const
{
  double ap = player_t::composite_melee_attack_power();

  return ap;
}

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

  c += spec.veteran_of_the_third_war -> effectN( 1 ).percent();

  return c;
}

// death_knight_t::passive_movement_modifier====================================

double death_knight_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( legendary.sephuzs_secret )
  {
    ms += legendary.sephuzs_secret -> effectN( 2 ).percent();
  }

  return ms;
}

// death_knight_t::combat_begin =============================================

void death_knight_t::combat_begin()
{
  player_t::combat_begin();

  buffs.cold_heart -> trigger( buffs.cold_heart -> max_stack() );

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
    case CACHE_ATTACK_POWER:
      if ( specialization() == DEATH_KNIGHT_BLOOD )
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

// death_knight_t::primary_stat ==================================================

stat_e death_knight_t::primary_stat() const
{
  switch ( specialization() )
  {
    case DEATH_KNIGHT_BLOOD: return STAT_STAMINA;
    default:                 return STAT_STRENGTH;
  }
}

// death_knight_t::create_options ================================================

void death_knight_t::create_options()
{
  player_t::create_options();

  add_option( opt_float( "fallen_crusader_str", fallen_crusader ) );
  add_option( opt_float( "fallen_crusader_rppm", fallen_crusader_rppm ) );
}

// death_knight_t::copy_from =====================================================

void death_knight_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  death_knight_t* p = debug_cast<death_knight_t*>( source );

  fallen_crusader = p -> fallen_crusader;
  fallen_crusader_rppm = p -> fallen_crusader_rppm;
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

// death_knight_t::trigger_runic_empowerment ================================

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

void death_knight_t::apply_diseases( action_state_t* state, unsigned diseases )
{
  if ( diseases & DISEASE_BLOOD_PLAGUE )
  {
    active_spells.blood_plague -> set_target( state -> target );
    active_spells.blood_plague -> execute();
  }

  if ( diseases & DISEASE_FROST_FEVER )
  {
    active_spells.frost_fever -> set_target( state -> target );
    active_spells.frost_fever -> execute();
  }

  if ( diseases & DISEASE_VIRULENT_PLAGUE )
  {
    active_spells.virulent_plague -> set_target( state -> target );
    active_spells.virulent_plague -> execute();
  }
}

// death_knight_t rune inspections ==========================================

// death_knight_t::ready_runes_count ========================================

// how many runes of type rt are ready?
double death_knight_t::ready_runes_count( bool fractional ) const
{
  // If fractional, then a rune array [0.1, 0.1, 0.3, 0.4, 0.1, 0.2] would return
  // 0.1+0.1+0.3+0.4+0.1+0.2 = 1.2 (total runes)
  // This could be used to estimate when you're in a "high resource" state versus a low resource state.

  // If fractional is false, then calling this method on that rune array would return zero, because
  // there are no runes of value 1.0
  double result = 0;
  for ( size_t rune_idx = 0; rune_idx < MAX_RUNES; ++rune_idx )
  {
    const rune_t& r = _runes.slot[ rune_idx ];
    if ( fractional || r.is_ready() )
    {
      result += r.fill_level();
    }
  }

  return result;
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

struct toravons_bindings_t : public scoped_actor_callback_t<death_knight_t>
{
  toravons_bindings_t() : super( DEATH_KNIGHT )
  { }

  void manipulate( death_knight_t* p, const special_effect_t& e ) override
  { p -> legendary.toravons = e.driver() -> effectN( 1 ).percent(); }
};

struct taktheritrixs_shoulderpads_t : public scoped_actor_callback_t<death_knight_t>
{
  taktheritrixs_shoulderpads_t() : super( DEATH_KNIGHT )
  { }

  void manipulate( death_knight_t* p, const special_effect_t& /* e */ ) override
  {
    create_buff( p -> pets.gargoyle );

    for ( size_t i = 0; i < p -> pets.army_ghoul.size(); i++ )
    {
      create_buff( p -> pets.army_ghoul[ i ] );
    }

    for ( size_t i = 0; i < p -> pets.apoc_ghoul.size(); i++ )
    {
      create_buff( p -> pets.apoc_ghoul[ i ] );
    }
  }

  void create_buff( pets::death_knight_pet_t* pet )
  {
    if ( ! pet )
      return;

    auto spell = pet -> find_spell( 215069 );

    pet -> taktheritrix = buff_creator_t( pet, "taktheritrixs_command", spell )
      .default_value( spell -> effectN( 1 ).percent() )
      .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
};

struct the_instructors_fourth_lesson_t : public scoped_actor_callback_t<death_knight_t>
{
  the_instructors_fourth_lesson_t() : super( DEATH_KNIGHT_UNHOLY )
  { }

  void manipulate( death_knight_t* p, const special_effect_t& e ) override
  { p -> legendary.the_instructors_fourth_lesson = as<unsigned int>( e.driver() -> effectN( 1 ).base_value() ); }
};

struct draugr_girdle_everlasting_king_t : public scoped_actor_callback_t<death_knight_t>
{
  draugr_girdle_everlasting_king_t() : super( DEATH_KNIGHT )
  { }

  void manipulate( death_knight_t* p, const special_effect_t& e ) override
  { p -> legendary.draugr_girdle_everlasting_king = e.driver() -> proc_chance(); }
};

struct uvanimor_the_unbeautiful_t : public scoped_actor_callback_t<death_knight_t>
{
  uvanimor_the_unbeautiful_t() : super( DEATH_KNIGHT )
  { }

  void manipulate( death_knight_t* p, const special_effect_t& e ) override
  { p -> legendary.uvanimor_the_unbeautiful = e.driver() -> effectN( 1 ).percent(); }
};

struct koltiras_newfound_will_t : public scoped_actor_callback_t<death_knight_t>
{
  koltiras_newfound_will_t() : super( DEATH_KNIGHT )
  { }

  void manipulate( death_knight_t* p, const special_effect_t& e ) override
  { p -> legendary.koltiras_newfound_will = e.driver(); }
};

struct consorts_cold_core_t : public scoped_action_callback_t<frostwyrms_fury_t>
{
  consorts_cold_core_t() : super( DEATH_KNIGHT, "frostwyrms_fury" )
  { }

  void manipulate( frostwyrms_fury_t* action, const special_effect_t& e ) override
  {
    auto m = 1.0 + e.driver() -> effectN( 1 ).percent();
    action -> cooldown -> duration = action -> data().cooldown() * m;
  }
};

struct death_march_t : public scoped_actor_callback_t<death_knight_t>
{
  death_march_t() : super( DEATH_KNIGHT )
  { }

  void manipulate( death_knight_t* p, const special_effect_t& e ) override
  { p -> legendary.death_march = e.driver(); }
};

template <typename T>
struct death_march_passive_t : public scoped_action_callback_t<T>
{
  death_march_passive_t( const std::string& n ) : scoped_action_callback_t<T>( DEATH_KNIGHT, n )
  { }

  void manipulate( T* a, const special_effect_t& e ) override
  { a -> base_multiplier *= 1.0 + e.driver() -> effectN( 2 ).percent(); }
};

struct sephuzs_secret_t: public unique_gear::scoped_actor_callback_t<death_knight_t>
{
  sephuzs_secret_t(): super( DEATH_KNIGHT )
  {}

  void manipulate( death_knight_t* dk, const special_effect_t& e ) override
  {
    dk -> legendary.sephuzs_secret = e.driver();
  }
};

struct skullflowers_haemostasis_t : public class_buff_cb_t<buff_t>
{
  skullflowers_haemostasis_t() : super( DEATH_KNIGHT, "haemostasis" )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<death_knight_t*>( e.player ) -> buffs.skullflowers_haemostasis; }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.trigger() )
      ->set_default_value( e.trigger() -> effectN( 1 ).percent() )
           // Grab 1 second ICD from the driver
           // ICD isn't observed in game anymore and counters the application through DRW weapons
           // .cd( e.driver() -> internal_cooldown() )
		;
  }
};

struct seal_of_necrofantasia_t : public scoped_actor_callback_t<death_knight_t>
{
  seal_of_necrofantasia_t() : super( DEATH_KNIGHT )
  { }

  void manipulate( death_knight_t* p, const special_effect_t& e ) override
  { p -> legendary.seal_of_necrofantasia = e.driver(); }
};

struct perseverance_of_the_ebon_martyr_t : public scoped_actor_callback_t<death_knight_t>
{
  perseverance_of_the_ebon_martyr_t() : super( DEATH_KNIGHT )
  { }

  void manipulate( death_knight_t* p, const special_effect_t& e ) override
  { p -> legendary.perseverance_of_the_ebon_martyr = e.driver(); }
};

struct cold_heart_t : public scoped_actor_callback_t<death_knight_t>
{
  struct cold_heart_tick_t : public event_t
  {
    timespan_t delay;
    buff_t* buff;

    cold_heart_tick_t( buff_t* b, const timespan_t& d ) :
      event_t( *b -> sim, d ), delay( d ), buff( b )
    { }

    void execute() override
    {
      buff -> trigger();
      make_event<cold_heart_tick_t>( sim(), buff, delay );
    }
  };

  cold_heart_t() : super( DEATH_KNIGHT )
  { }

  void manipulate( death_knight_t* p, const special_effect_t& e ) override
  {
    p -> callbacks_on_arise.push_back( [ p, &e ]() {
      make_event<cold_heart_tick_t>( *p -> sim, p -> buffs.cold_heart, e.driver() -> effectN( 1 ).period() );
    } );

    p -> active_spells.cold_heart = new cold_heart_damage_t( p );
  }
};

struct cold_heart_buff_t: public class_buff_cb_t<buff_t>
{
  cold_heart_buff_t() : super( DEATH_KNIGHT, "cold_heart" )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<death_knight_t*>( e.player ) -> buffs.cold_heart; }

  buff_t* creator( const special_effect_t& e ) const override
  { return make_buff( e.player, buff_name, e.player -> find_spell( 235599 ) ); }
};

struct lanathels_lament_t : public scoped_actor_callback_t<death_knight_t>
{
  lanathels_lament_t() : super( DEATH_KNIGHT )
  { }

  void manipulate( death_knight_t* p, const special_effect_t& ) override
  { p -> legendary.lanathels_lament = p -> find_spell( 212975 ); }
};

struct soulflayers_corruption_t : public scoped_actor_callback_t<death_knight_t>
{
  soulflayers_corruption_t() : super( DEATH_KNIGHT )
  { }

  void manipulate( death_knight_t* p, const special_effect_t& e ) override
  { p -> legendary.soulflayers_corruption = e.driver(); }
};

struct shackles_of_bryndaor_t : public scoped_actor_callback_t<death_knight_t>
{
  shackles_of_bryndaor_t() : super( DEATH_KNIGHT )
  { }

  void manipulate( death_knight_t* p, const special_effect_t& e ) override
  { p -> legendary.shackles_of_bryndaor = e.driver(); }
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
    unique_gear::register_special_effect( 215068, taktheritrixs_shoulderpads_t() );
    unique_gear::register_special_effect( 205658, toravons_bindings_t() );
    unique_gear::register_special_effect( 208713, the_instructors_fourth_lesson_t() );
    unique_gear::register_special_effect( 208161, draugr_girdle_everlasting_king_t() );
    unique_gear::register_special_effect( 208786, uvanimor_the_unbeautiful_t() );
    unique_gear::register_special_effect( 208782, koltiras_newfound_will_t() );
    unique_gear::register_special_effect( 212216, seal_of_necrofantasia_t() );
    unique_gear::register_special_effect( 216059, perseverance_of_the_ebon_martyr_t() );
    unique_gear::register_special_effect( 212974, lanathels_lament_t() );
    unique_gear::register_special_effect( 209228, shackles_of_bryndaor_t() );
    // 7.1.5
    unique_gear::register_special_effect( 235605, consorts_cold_core_t() );
    unique_gear::register_special_effect( 235556, death_march_t() );
    unique_gear::register_special_effect( 235556, death_march_passive_t<death_knight_spell_t>( "death_coil" ) );
    unique_gear::register_special_effect( 235556, death_march_passive_t<death_knight_melee_attack_t>( "death_strike" ) );
    unique_gear::register_special_effect( 235556, death_march_passive_t<death_knight_melee_attack_t>( "death_strike_offhand" ) );
    unique_gear::register_special_effect( 235558, skullflowers_haemostasis_t(), true );
    unique_gear::register_special_effect( 208051, sephuzs_secret_t() );
    // 7.2.5
    unique_gear::register_special_effect( 235592, cold_heart_t() );
    unique_gear::register_special_effect( 235592, cold_heart_buff_t(), true );
    unique_gear::register_special_effect( 248066, soulflayers_corruption_t() );
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
