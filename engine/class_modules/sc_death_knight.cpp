// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// TODO:
// Unholy
// - Does Festering Wound (generation|consumption) require a positive hit result?
// - Festering Strike Festering Wound generation probability distribution
// - Skelebro has an aoe spell (Arrow Spray), but the AI using it is very inconsistent
// - Portal to the Underworld damage is messed up (by ~3%).
// - Armies of the Damned is not implemented
// Blood
//

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

using namespace unique_gear;

struct death_knight_t;
struct runes_t;
struct rune_t;

namespace pets {
  struct death_knight_pet_t;
  struct valkyr_pet_t;
  struct dancing_rune_weapon_pet_t;
  struct dt_pet_t;
}

namespace runeforge {
  // TODO: set up rune forges
  void razorice_attack( special_effect_t& );
  void razorice_debuff( special_effect_t& );
  void fallen_crusader( special_effect_t& );
  void stoneskin_gargoyle( special_effect_t& );
}

enum {
  // http://us.battle.net/wow/en/forum/topic/20743504316?page=9#163
  CRYSTALLINE_SWORDS_MAX = 9
};

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
    dot_t* death_and_decay;
    dot_t* defile;
    dot_t* frost_fever;
    dot_t* outbreak;
    dot_t* remorseless_winter;
    dot_t* soul_reaper;
    dot_t* virulent_plague;
  } dot;

  struct
  {
    debuff_t* razorice;
    debuff_t* festering_wound;
    debuff_t* mark_of_blood;
    debuff_t* soul_reaper;
    debuff_t* blood_mirror;
    debuff_t* scourge_of_worlds;
    debuff_t* death; // Armies of the Damned ghoul proc
  } debuff;

  // Check if DnD or Defile are up for ScS/CS AOE
  bool in_aoe_radius() const
  {
    if ( ! source -> sim -> distance_targeting_enabled )
    {
      return dot.defile -> is_ticking() || dot.death_and_decay -> is_ticking();
    }
    else
    {
      if ( dot.defile -> is_ticking() )
      {
        return source -> get_ground_aoe_distance( *dot.defile -> state ) <=
               dot.defile -> current_action -> radius + target -> combat_reach;
      }
      else if ( dot.death_and_decay -> is_ticking() )
      {
        return source -> get_ground_aoe_distance( *dot.death_and_decay -> state ) <=
               dot.death_and_decay -> current_action -> radius + target -> combat_reach;
      }
      return false;
    }
  }

  death_knight_td_t( player_t* target, death_knight_t* death_knight );
};

struct death_knight_t : public player_t {
public:
  // Active
  double runic_power_decay_rate;
  double fallen_crusader, fallen_crusader_rppm;
  double aotd_proc_chance;
  double antimagic_shell_absorbed;

  // Counters
  int pestilent_pustules;
  int crystalline_swords;

  stats_t*  antimagic_shell;

  // Buffs
  struct buffs_t {
    buff_t* army_of_the_dead;
    buff_t* antimagic_shell;
    haste_buff_t* bone_shield;
    buff_t* crimson_scourge;
    buff_t* dancing_rune_weapon;
    buff_t* dark_transformation;
    buff_t* gathering_storm;
    buff_t* icebound_fortitude;
    buff_t* killing_machine;
    haste_buff_t* t18_4pc_frost_haste;
    buff_t* t18_4pc_frost_crit;
    haste_buff_t* t18_4pc_unholy;
    buff_t* obliteration;
    buff_t* pillar_of_frost;
    buff_t* rime;
    buff_t* runic_corruption;
    buff_t* sudden_doom;
    buff_t* vampiric_blood;
    buff_t* will_of_the_necropolis;

    absorb_buff_t* blood_shield;
    buff_t* rune_tap;
    stat_buff_t* riposte;
    buff_t* shadow_of_death;

    haste_buff_t* icy_talons;
    buff_t* blighted_rune_weapon;
    haste_buff_t* unholy_frenzy;
    buff_t* necrosis;
    stat_buff_t* defile;
    haste_buff_t* soul_reaper;
    buff_t* soulgorge;
    buff_t* antimagic_barrier;
    absorb_buff_t* tombstone;

    buff_t* frozen_pulse;

    stat_buff_t* t19oh_8pc;
    buff_t* skullflowers_haemostasis;
  } buffs;

  struct runeforge_t {
    buff_t* rune_of_the_fallen_crusader;
    buff_t* rune_of_the_stoneskin_gargoyle;
  } runeforge;

  // Cooldowns
  struct cooldowns_t {
    cooldown_t* antimagic_shell;
    cooldown_t* avalanche;
    cooldown_t* bone_shield_icd;
    cooldown_t* dark_transformation;
    cooldown_t* death_and_decay;
    cooldown_t* defile;
    cooldown_t* festering_wound;
    cooldown_t* frost_fever;
    cooldown_t* icecap;
    cooldown_t* pillar_of_frost;
    cooldown_t* vampiric_blood;
  } cooldown;

  // Active Spells
  struct active_spells_t {
    spell_t* blood_plague;
    spell_t* frost_fever;
    spell_t* necrosis;
    action_t* avalanche;
    action_t* festering_wound;
    action_t* virulent_plague;
    action_t* bursting_sores;
    action_t* mark_of_blood;
    action_t* crystalline_swords;
    action_t* necrobomb;
    action_t* pestilence; // Armies of the Damned
  } active_spells;

  // Gains
  struct gains_t {
    gain_t* antimagic_shell;
    gain_t* festering_wound;
    gain_t* frost_fever;
    gain_t* horn_of_winter;
    gain_t* hungering_rune_weapon;
    gain_t* murderous_efficiency;
    gain_t* power_refund;
    gain_t* rune;
    gain_t* runic_attenuation;
    gain_t* rc;
    gain_t* runic_empowerment;
    gain_t* empower_rune_weapon;
    gain_t* blood_tap;
    gain_t* pestilent_pustules;
    gain_t* tombstone;
    gain_t* overpowered;
    gain_t* t18_2pc_blood;
    gain_t* scourge_the_unbeliever;
    gain_t* draugr_girdle_everlasting_king;
    gain_t* uvanimor_the_unbeautiful;
    gain_t* koltiras_newfound_will;
    gain_t* t19_4pc_frost;
  } gains;

  // Specialization
  struct specialization_t {
    // Generic
    const spell_data_t* plate_specialization;
    const spell_data_t* death_knight;

    // Blood
    const spell_data_t* blood_death_knight;
    const spell_data_t* crimson_scourge;
    const spell_data_t* veteran_of_the_third_war;
    const spell_data_t* riposte;

    // Frost
    const spell_data_t* frost_fever;
    const spell_data_t* runic_empowerment;
    const spell_data_t* killing_machine;
    const spell_data_t* rime;

    // Unholy
    const spell_data_t* festering_wound;
    const spell_data_t* runic_corruption;
    const spell_data_t* deaths_advance;
    const spell_data_t* outbreak;
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

    // Tier 1
    const spell_data_t* shattering_strikes;
    const spell_data_t* icy_talons;
    const spell_data_t* murderous_efficiency;

    // Tier 2
    const spell_data_t* freezing_fog;
    const spell_data_t* frozen_pulse;
    const spell_data_t* horn_of_winter;

    // Tier 3
    const spell_data_t* icecap;
    const spell_data_t* hungering_rune_weapon;
    const spell_data_t* avalanche;

    // Tier 6
    const spell_data_t* frostscythe;
    const spell_data_t* runic_attenuation;
    const spell_data_t* gathering_storm;

    // Tier 7
    const spell_data_t* obliteration;
    const spell_data_t* breath_of_sindragosa;
    const spell_data_t* glacial_advance;

    // Unholy

    // Tier 1
    const spell_data_t* all_will_serve;
    const spell_data_t* bursting_sores;
    const spell_data_t* ebon_fever;

    // Tier 2
    const spell_data_t* epidemic;
    const spell_data_t* pestilent_pustules;
    const spell_data_t* blighted_rune_weapon;

    // Tier 3
    const spell_data_t* unholy_frenzy;
    const spell_data_t* castigator;
    const spell_data_t* clawing_shadows;

    // Tier 4
    const spell_data_t* sludge_belcher;

    // Tier 6
    const spell_data_t* shadow_infusion;
    const spell_data_t* necrosis;
    const spell_data_t* infected_claws;

    // Tier 7
    const spell_data_t* dark_arbiter;
    const spell_data_t* defile;
    const spell_data_t* soul_reaper;

    // Blood

    // Tier 1
    const spell_data_t* bloodworms; // Not Yet Implemented
    const spell_data_t* heartbreaker;
    const spell_data_t* blooddrinker;

    // Tier 2
    const spell_data_t* rapid_decomposition;
    const spell_data_t* soulgorge;
    const spell_data_t* spectral_deflection; // Not Yet Implemented

    // Tier 3
    const spell_data_t* ossuary;
    const spell_data_t* blood_tap;
    const spell_data_t* antimagic_barrier;

    // Tier 4
    const spell_data_t* mark_of_blood;
    const spell_data_t* red_thirst;
    const spell_data_t* tombstone;

    // Tier 6
    const spell_data_t* will_of_the_necropolis; // Not Yet Implemented
    const spell_data_t* rune_tap;
    const spell_data_t* foul_bulwark;

    // Tier 7
    const spell_data_t* bonestorm;
    const spell_data_t* blood_mirror;
    const spell_data_t* purgatory; // Not Yet Implemented
  } talent;

  struct artifact_t {
    // Frost
    artifact_power_t crystalline_swords;
    artifact_power_t blades_of_frost;
    artifact_power_t nothing_but_the_boots;
    artifact_power_t frozen_core;
    artifact_power_t frozen_skin;
    artifact_power_t dead_of_winter;
    artifact_power_t sindragosas_fury;
    artifact_power_t blast_radius;
    artifact_power_t chill_of_the_grave;
    artifact_power_t ice_in_your_veins;
    artifact_power_t overpowered;
    artifact_power_t frozen_soul;
    artifact_power_t cold_as_ice;
    artifact_power_t ambidexterity;
    artifact_power_t bad_to_the_bone;
    artifact_power_t hypothermia;
    artifact_power_t soulbiter;

    // Unholy
    artifact_power_t apocalypse;
    artifact_power_t feast_of_souls;
    artifact_power_t eternal_agony;
    artifact_power_t the_darkest_crusade;
    artifact_power_t portal_to_the_underworld;
    artifact_power_t runic_tattoos;
    artifact_power_t scourge_of_worlds;
    artifact_power_t rotten_touch;
    artifact_power_t plaguebearer;
    artifact_power_t armies_of_the_damned;
    artifact_power_t scourge_the_unbeliever;
    artifact_power_t the_shambler;
    artifact_power_t double_doom;
    artifact_power_t deadliest_coil;
    artifact_power_t fleshsearer;

  } artifact;

  // Spells
  struct spells_t {
    const spell_data_t* antimagic_shell;
    const spell_data_t* blood_rites;
    const spell_data_t* ossuary;
  } spell;

  // RPPM
  struct rppm_t
  {
    real_ppm_t* shambler;
  } rppm;

  // Pets and Guardians
  struct pets_t
  {
    std::array< pets::death_knight_pet_t*, 8 > army_ghoul;
    std::array< pets::death_knight_pet_t*, 8 > apocalypse_ghoul;
    pets::dancing_rune_weapon_pet_t* dancing_rune_weapon;
    pets::dt_pet_t* ghoul_pet; // Covers both Ghoul and Sludge Belcher
    pets::death_knight_pet_t* gargoyle;
    pets::valkyr_pet_t* dark_arbiter;
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
  } procs;

  // Legendaries
  struct legendary_t {
    // Frost
    const spell_data_t* koltiras_newfound_will;
    const spell_data_t* seal_of_necrofantasia;
    double toravons;

    // Unholy
    unsigned the_instructors_fourth_lesson;
    double draugr_girdle_everlasting_king;
    double uvanimor_the_unbeautiful;
    timespan_t death_march;

    legendary_t() :
      koltiras_newfound_will( spell_data_t::not_found() ),
      seal_of_necrofantasia( spell_data_t::not_found() ),
      toravons( 0 ), the_instructors_fourth_lesson( 0 ), draugr_girdle_everlasting_king( 0 ),
      uvanimor_the_unbeautiful( 0 ), death_march( timespan_t::zero() )
    { }

  } legendary;

  // Runes
  runes_t _runes;

  death_knight_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DEATH_KNIGHT, name, r ),
    runic_power_decay_rate(),
    fallen_crusader( 0 ),
    fallen_crusader_rppm( find_spell( 166441 ) -> real_ppm() ),
    aotd_proc_chance( 0.2 ),
    antimagic_shell_absorbed( 0.0 ),
    pestilent_pustules( 0 ),
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
    base.distance = 0;

    cooldown.antimagic_shell = get_cooldown( "antimagic_shell" );
    cooldown.avalanche       = get_cooldown( "avalanche" );
    cooldown.bone_shield_icd = get_cooldown( "bone_shield_icd" );
    cooldown.bone_shield_icd -> duration = timespan_t::from_seconds( 2.0 );
    cooldown.dark_transformation = get_cooldown( "dark_transformation" );
    cooldown.death_and_decay = get_cooldown( "death_and_decay" );
    cooldown.defile          = get_cooldown( "defile" );
    cooldown.festering_wound = get_cooldown( "festering_wound" );
    cooldown.icecap          = get_cooldown( "icecap" );
    cooldown.pillar_of_frost = get_cooldown( "pillar_of_frost" );
    cooldown.vampiric_blood = get_cooldown( "vampiric_blood" );

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
  void      init_resources( bool force ) override;
  void      init_absorb_priority() override;
  double    composite_armor_multiplier() const override;
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
  double    composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  double    composite_crit_avoidance() const override;
  double    passive_movement_modifier() const override;
  void      reset() override;
  void      arise() override;
  void      adjust_dynamic_cooldowns() override;
  void      assess_heal( school_e, dmg_e, action_state_t* ) override;
  void      assess_damage( school_e, dmg_e, action_state_t* ) override;
  void      assess_damage_imminent( school_e, dmg_e, action_state_t* ) override;
  void      target_mitigation( school_e, dmg_e, action_state_t* ) override;
  void      do_damage( action_state_t* ) override;
  action_t* create_action( const std::string& name, const std::string& options ) override;
  expr_t*   create_expression( action_t*, const std::string& name ) override;
  void      create_pets() override;
  void      create_options() override;
  resource_e primary_resource() const override { return RESOURCE_RUNIC_POWER; }
  role_e    primary_role() const override;
  stat_e    convert_hybrid_stat( stat_e s ) const override;
  void      invalidate_cache( cache_e ) override;
  double resource_loss( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;
  void      merge( player_t& other ) override;
  void      analyze( sim_t& sim ) override;

  double    runes_per_second() const;
  double    rune_regen_coefficient() const;
  void      trigger_runic_empowerment( double rpcost );
  bool      trigger_runic_corruption( double rpcost, double override_chance = -1.0 );
  void      trigger_festering_wound( const action_state_t* state, unsigned n_stacks = 1, bool bypass_icd = false );
  void      trigger_death_march( const action_state_t* state );
  void      apply_diseases( action_state_t* state, unsigned diseases );
  double    ready_runes_count( bool fractional ) const;
  double    runes_cooldown_min( ) const;
  double    runes_cooldown_max( ) const;
  double    runes_cooldown_time( const rune_t& r ) const;
  void      default_apl_dps_precombat( const std::string& food, const std::string& potion );
  void      default_apl_blood();
  void      default_apl_frost();
  void      default_apl_unholy();

  double    bone_shield_handler( const action_state_t* ) const;

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
  dot.blood_plague       = target -> get_dot( "blood_plague",       death_knight );
  dot.death_and_decay    = target -> get_dot( "death_and_decay",    death_knight );
  dot.defile             = target -> get_dot( "defile",             death_knight );
  dot.frost_fever        = target -> get_dot( "frost_fever",        death_knight );
  dot.outbreak           = target -> get_dot( "outbreak",           death_knight );
  dot.remorseless_winter = target -> get_dot( "remorseless_winter", death_knight );
  dot.soul_reaper        = target -> get_dot( "soul_reaper_dot",    death_knight );
  dot.virulent_plague    = target -> get_dot( "virulent_plague",    death_knight );

  debuff.razorice        = buff_creator_t( *this, "razorice", death_knight -> find_spell( 51714 ) )
                           .period( timespan_t::zero() );
  debuff.festering_wound = buff_creator_t( *this, "festering_wound" )
                           .spell( death_knight -> find_spell( 194310 ) )
                           .trigger_spell( death_knight -> spec.festering_wound )
                           .cd( timespan_t::zero() ); // Handled by trigger_festering_wound
  debuff.mark_of_blood   = buff_creator_t( *this, "mark_of_blood", death_knight -> talent.mark_of_blood )
                           .cd( timespan_t::zero() ); // Handled by the action
  debuff.soul_reaper     = buff_creator_t( *this, "soul_reaper", death_knight -> talent.soul_reaper )
                           .cd( timespan_t::zero() ); // Handled by the action
  debuff.blood_mirror = buff_creator_t( *this, "blood_mirror", death_knight -> talent.blood_mirror )
                        .cd( timespan_t::zero() ); // Handled by the action
  debuff.scourge_of_worlds = buff_creator_t( *this, "scourge_of_worlds" )
    .spell( death_knight -> artifact.scourge_of_worlds.data().effectN( 1 ).trigger() )
    .trigger_spell( death_knight -> artifact.scourge_of_worlds )
    .default_value( death_knight -> artifact.scourge_of_worlds.data().effectN( 1 ).trigger() -> effectN( 1 ).percent() );
  debuff.death = buff_creator_t( *this, "death", death_knight -> find_spell( 191730 ) )
    .trigger_spell( death_knight -> artifact.armies_of_the_damned )
    .default_value( death_knight -> find_spell( 191730 ) -> effectN( 1 ).percent() );
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

  // Otherwise, it's going to be the remaining regen time of the N - 1th rune, plus the regen time
  // of a depleted rune.
  return regenerating_runes[ n_unsatisfied - 1 ] -> event -> remains() +
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

  if ( runes -> dk -> sets.has_set_bonus( DEATH_KNIGHT_UNHOLY, T19OH, B8 ) ||
       runes -> dk -> sets.has_set_bonus( DEATH_KNIGHT_FROST, T19OH, B8 ) ||
       runes -> dk -> sets.has_set_bonus( DEATH_KNIGHT_BLOOD, T19OH, B8 ) )
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

    if ( dbc::is_school( school, SCHOOL_SHADOW ) && o() -> mastery.dreadblade -> ok() )
    {
      m *= 1.0 + o() -> cache.mastery_value();
    }

    m *= 1.0 + o() -> artifact.fleshsearer.percent();

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

    this -> p() -> o() -> trigger_festering_wound( state, 1, true ); // Bypass ICD
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
// Generic Unholy "ghoul" (Ghoul, Sludge Belcher, Army ghouls)
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
    base_energy_regen_per_second  = 10;
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
             timespan_t::from_seconds( ( 40 - energy ) / energy_regen_per_second() ),
             timespan_t::from_seconds( 0.1 )
           );
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

struct dt_pet_t : public base_ghoul_pet_t
{
  // Unholy T18 4pc buff
  buff_t* crazed_monstrosity;

  dt_pet_t( death_knight_t* owner, const std::string& name ) :
    base_ghoul_pet_t( owner, name, false ), crazed_monstrosity( nullptr )
  { }

  attack_t* create_auto_attack() override
  { return new dt_pet_auto_attack_t< dt_pet_t >( this ); }

  void create_buffs() override
  {
    base_ghoul_pet_t::create_buffs();

    crazed_monstrosity = buff_creator_t( this, "crazed_monstrosity", find_spell( 187970 ) )
                         .duration( find_spell( 187981 ) -> duration() ) // Grab duration from the player's spell
                         .chance( owner -> sets.has_set_bonus( DEATH_KNIGHT_UNHOLY, T18, B4 ) );
  }

  double composite_melee_speed() const override
  {
    double s = base_ghoul_pet_t::composite_melee_speed();

    if ( crazed_monstrosity -> up() )
    {
      s *= 1.0 / ( 1.0 + crazed_monstrosity -> data().effectN( 3 ).percent() );
    }

    return s;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = base_ghoul_pet_t::composite_player_multiplier( school );

    if ( crazed_monstrosity -> up() )
    {
      m *= 1.0 + crazed_monstrosity -> data().effectN( 2 ).percent();
    }

    if ( owner -> sets.has_set_bonus( DEATH_KNIGHT_UNHOLY, T18, B2 ) )
    {
      m *= 1.0 + owner -> sets.set( DEATH_KNIGHT_UNHOLY, T18, B2 ) -> effectN( 1 ).percent();
    }

    if ( o() -> buffs.dark_transformation -> up() )
    {
      double dtb = o() -> buffs.dark_transformation -> data().effectN( 1 ).percent();

      dtb += o() -> sets.set( DEATH_KNIGHT_UNHOLY, T17, B2 ) -> effectN( 2 ).percent();

      m *= 1.0 + dtb;
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
  };

  struct gnaw_t : public dt_melee_ability_t<ghoul_pet_t>
  {
    gnaw_t( ghoul_pet_t* player, const std::string& options_str ) :
      super( player, "gnaw", player -> find_spell( 91800 ), options_str, false )
    { }
  };

  struct monstrous_blow_t : public dt_melee_ability_t<ghoul_pet_t>
  {
    monstrous_blow_t( ghoul_pet_t* player, const std::string& options_str ):
      super( player, "monstrous_blow", player -> find_spell( 91797 ), options_str )
    {
      cooldown = player -> get_cooldown( "gnaw" ); // Shares CD with Gnaw
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
    def -> add_action( "Gnaw" );
    def -> add_action( "Monstrous Blow" );
    def -> add_action( "Sweeping Claws" );
    def -> add_action( "Claw" );
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
// Unholy Sludge Belcher
// ==========================================================================

struct sludge_belcher_pet_t : public dt_pet_t
{
  struct cleaver_t : public dt_melee_ability_t<sludge_belcher_pet_t>
  {
    cleaver_t( sludge_belcher_pet_t* player, const std::string& options_str ) :
      super( player, "cleaver", player -> find_spell( 212335 ), options_str, false )
    { triggers_infected_claws = true; }
  };

  struct smash_t : public dt_melee_ability_t<sludge_belcher_pet_t>
  {
    smash_t( sludge_belcher_pet_t* player, const std::string& options_str ):
      super( player, "smash", player -> find_spell( 212332 ), options_str, false )
    { }
  };

  struct vile_gas_t : public dt_melee_ability_t<sludge_belcher_pet_t>
  {
    vile_gas_t( sludge_belcher_pet_t* player, const std::string& options_str ) :
      super( player, "vile_gas", player -> find_spell( 212338 ), options_str )
    {
      aoe = -1;
      triggers_infected_claws = true;
    }
  };

  struct monstrous_blow_t : public dt_melee_ability_t<sludge_belcher_pet_t>
  {
    monstrous_blow_t( sludge_belcher_pet_t* player, const std::string& options_str ):
      super( player, "monstrous_blow", player -> find_spell( 91797 ), options_str )
    {
      cooldown = player -> get_cooldown( "smash" ); // Shares CD with Smash
    }
  };

  sludge_belcher_pet_t( death_knight_t* owner ) : dt_pet_t( owner, "sludge_belcher" )
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
    def -> add_action( "Smash" );
    def -> add_action( "Monstrous Blow" );
    def -> add_action( "Vile Gas" );
    def -> add_action( "Cleaver" );
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "cleaver"        ) return new        cleaver_t( this, options_str );
    if ( name == "vile_gas"       ) return new       vile_gas_t( this, options_str );
    if ( name == "smash"          ) return new          smash_t( this, options_str );
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

    // Triggers the currently selected Armies of the Damned proc for the pet
    void trigger_aotd_proc( const action_state_t* state )
    {
      if ( ! p() -> o() -> artifact.armies_of_the_damned.rank() )
      {
        return;
      }

      // 2016-08-23 Add a 20% default proc chance
      if ( ! rng().roll( p() -> o() -> aotd_proc_chance ) )
      {
        return;
      }

      // 4 different procs, but only 2 are relevant for the DPS
      switch ( static_cast<int>( rng().range( 0, 4 ) ) )
      {
        case 0:
          p() -> o() -> get_target_data( state -> target ) -> debuff.death -> trigger();
          break;
        case 1:
          p() -> o() -> active_spells.pestilence -> target = state -> target;
          p() -> o() -> active_spells.pestilence -> execute();
          break;
        default:
          break;
      }
    }

    void execute() override
    {
      super::execute();
      if ( result_is_hit( execute_state -> result ) )
      {
        trigger_aotd_proc( execute_state );
      }
    }
  };

  struct dragged_to_helheim_t : public pet_spell_t<army_pet_t>
  {
    dragged_to_helheim_t( army_pet_t* player ) :
      super( player, "dragged_to_helheim", player -> find_spell( 218321 ) )
    {
      background = true;
      aoe = -1;
    }

    // Uses owner's attack power .. probably. The damage in game is not very self-explanatory.
    // Technically this is wrong, but we have no action-specific composite attack power multiplier,
    // so just multiply it in here. The ghoul itself will never have an attack power multiplier so
    // there's no possibility of breakage.
    double composite_attack_power() const override
    { return p() -> o() -> cache.attack_power() * p() -> o() -> composite_attack_power_multiplier(); }
  };

  dragged_to_helheim_t* despawn_explosion;

  army_pet_t( death_knight_t* owner, const std::string& name ) :
    base_ghoul_pet_t( owner, name, true ), despawn_explosion( nullptr )
  { }

  void dismiss( bool expired ) override
  {
    if ( expired && despawn_explosion )
    {
      despawn_explosion -> execute();
    }

    pet_t::dismiss( expired );
  }

  void init_base_stats() override
  {
    base_ghoul_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 0.30;
  }

  void init_action_list() override
  {
    base_ghoul_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "Claw" );
  }

  bool create_actions() override
  {
    if ( o() -> artifact.portal_to_the_underworld.rank() )
    {
      despawn_explosion = new dragged_to_helheim_t( this );
    }

    return base_ghoul_pet_t::create_actions();
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

  gargoyle_pet_t( death_knight_t* owner ) : death_knight_pet_t( owner, "gargoyle", true, false )
  { regen_type = REGEN_DISABLED; }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    // As per Blizzard
    owner_coeff.sp_from_ap = 0.46625;
  }

  void init_action_list() override
  {
    death_knight_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "travel" );
    def -> add_action( "Gargoyle Strike" );
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "gargoyle_strike" ) return new gargoyle_strike_t( this, options_str );
    if ( name == "travel"          ) return new travel_t( this );

    return death_knight_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Dark Arbiter
// ==========================================================================

struct valkyr_pet_t : public death_knight_pet_t
{
  struct general_confusion_t : public action_t
  {
    bool executed;

    general_confusion_t( player_t* player ) : action_t( ACTION_OTHER, "general_confusion", player ),
      executed( false )
    {
      may_miss = false;
      dual = quiet = true;
      trigger_gcd = timespan_t::zero();
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

    // Seems to be spending roughly a second doing absolutely nothing after summoning
    timespan_t execute_time() const override
    { return timespan_t::from_seconds( rng().gauss( 1.0, 0.1 ) ); }

    bool ready() override
    { return ! executed; }
  };

  struct valkyr_strike_t : public pet_spell_t<valkyr_pet_t>
  {
    valkyr_strike_t( valkyr_pet_t* player, const std::string& options_str ) :
      super( player, "valkyr_strike", player -> find_spell( 198715 ), options_str )
    { }
  };

  buff_t* shadow_empowerment;

  valkyr_pet_t( death_knight_t* owner ) : death_knight_pet_t( owner, "valkyr_battlemaiden", true, false ), shadow_empowerment(nullptr)
  { regen_type = REGEN_DISABLED; }

  double composite_player_multiplier( school_e s ) const override
  {
    double m = death_knight_pet_t::composite_player_multiplier( s );

    m *= 1.0 + shadow_empowerment -> stack_value();

    return m;
  }

  void init_base_stats() override
  {
    death_knight_pet_t::init_base_stats();

    owner_coeff.ap_from_ap = 1/3.0;
  }

  void init_action_list() override
  {
    death_knight_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def -> add_action( "general_confusion" );
    def -> add_action( "Val'kyr Strike" );
  }

  void create_buffs() override
  {
    death_knight_pet_t::create_buffs();

    shadow_empowerment = buff_creator_t( this, "shadow_empowerment", find_spell( 211947 ) );
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "valkyr_strike"     ) return new     valkyr_strike_t( this, options_str );
    if ( name == "general_confusion" ) return new general_confusion_t( this );

    return death_knight_pet_t::create_action( name, options_str );
  }

  void increase_power( double rp_spent )
  {
    if ( is_sleeping() )
    {
      return;
    }

    double increase = rp_spent * shadow_empowerment -> data().effectN( 1 ).percent();

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
    owner_coeff.ap_from_ap = 2.0;
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
  };

  struct drw_attack_t : public pet_melee_attack_t<dancing_rune_weapon_pet_t>
  {
    drw_attack_t( dancing_rune_weapon_pet_t* p, const std::string& name, const spell_data_t* s ) :
      super( p, name, s )
    {
      background = true;
      normalize_weapon_speed = false; // DRW weapon-based abilities use non-normalized speed
    }
  };

  struct blood_plague_t : public drw_spell_t
  {
    blood_plague_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( p, "blood_plague", p -> owner -> find_spell( 55078 ) )  // Also check spell id 55078
    { }
  };

  struct blood_boil_t: public drw_spell_t
  {
    blood_boil_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( p, "blood_boil", p -> owner -> find_specialization_spell( "Blood Boil" ) )
    {
      aoe = -1;
      cooldown -> duration = timespan_t::zero();
      cooldown -> charges = 0;

      base_multiplier *= 1.0 + p -> o() -> spec.blood_death_knight -> effectN( 5 ).percent();
    }

    void impact( action_state_t* s ) override
    {
      drw_spell_t::impact( s );

      if ( result_is_hit( s -> result ) )
      {
        p() -> ability.blood_plague -> target = s -> target;
        p() -> ability.blood_plague -> execute();
      }
    }
  };

  struct deaths_caress_t : public drw_spell_t
  {
    deaths_caress_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( p, "deaths_caress", p -> owner -> find_specialization_spell( "Death's Caress" ) )
    { }

    void impact( action_state_t* s ) override
    {
      drw_spell_t::impact( s );

      if ( result_is_hit( s -> result ) )
      {
        p() -> ability.blood_plague -> target = s -> target;
        p() -> ability.blood_plague -> execute();
      }
    }
  };

  struct death_strike_t : public drw_attack_t
  {
    death_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_attack_t( p, "death_strike", p -> owner -> find_class_spell( "Death Strike" ) )
    {
      weapon = &( p -> main_hand_weapon );
      base_multiplier *= 1.0 + p -> o() -> spec.blood_death_knight -> effectN( 1 ).percent();
    }
  };

  struct heart_strike_t : public drw_attack_t
  {
    heart_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_attack_t( p, "heart_strike", p -> owner -> find_specialization_spell( "Heart Strike" ) )
    {
      weapon = &( p -> main_hand_weapon );
      aoe = 2;
    }
  };

  struct marrowrend_t : public drw_attack_t
  {
    marrowrend_t( dancing_rune_weapon_pet_t* p ) :
      drw_attack_t( p, "marrowrend", p -> owner -> find_specialization_spell( "Marrowrend" ) )
    {
      weapon = &( p -> main_hand_weapon );
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
      this -> energize_amount += std::fabs( this -> base_costs[ RESOURCE_RUNIC_POWER ] );
      this -> base_costs[ RESOURCE_RUNIC_POWER ] = 0;
    }
  }

  death_knight_t* p() const
  { return static_cast< death_knight_t* >( this -> player ); }

  death_knight_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual double runic_power_generation_multiplier( const action_state_t* s ) const
  {
    double m = 1.0;

    if ( p() -> talent.rapid_decomposition -> ok() && td( s -> target ) -> in_aoe_radius() )
    {
      m *= 1.0 + p() -> talent.rapid_decomposition -> effectN( 4 ).percent();
    }

    m *= 1.0 + p() -> artifact.runic_tattoos.data().effectN( 1 ).percent();

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

  void consume_resource() override
  {
    action_base_t::consume_resource();

    // TODO: Remorseless winter really needs to be targeted to the dk
    // TODO: How does Remorseless Winter tick with half-tick extensions?
    if ( this -> base_costs[ RESOURCE_RUNE ] > 0 &&
         p() -> talent.gathering_storm -> ok() )
    {
      unsigned consumed = static_cast<unsigned>( this -> base_costs[ RESOURCE_RUNE ] );
      if ( td( this -> target ) -> dot.remorseless_winter -> is_ticking() )
      {
        p() -> buffs.gathering_storm -> trigger( consumed );
        timespan_t base_extension = timespan_t::from_seconds( p() -> talent.gathering_storm -> effectN( 1 ).base_value() / 10.0 );
        td( this -> target ) -> dot.remorseless_winter -> extend_duration( base_extension * consumed );
      }
    }

    if ( this -> base_costs[ RESOURCE_RUNIC_POWER ] > 0 && this -> resource_consumed > 0 )
    {
      if ( p() -> talent.dark_arbiter -> ok() && p() -> pets.dark_arbiter )
      {
        p() -> pets.dark_arbiter -> increase_power( this -> resource_consumed );
      }

      if ( p() -> talent.red_thirst -> ok() )
      {
        timespan_t sec = timespan_t::from_seconds( p() -> talent.red_thirst -> effectN( 1 ).base_value() ) *
          this -> resource_consumed / p() -> talent.red_thirst -> effectN( 2 ).base_value();
        p() -> cooldown.vampiric_blood -> adjust( -sec );
      }
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

  virtual void burst_festering_wound( const action_state_t* state, unsigned n = 1 ) const
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
          dk -> active_spells.festering_wound -> target = target;
          dk -> active_spells.festering_wound -> execute();

          // Don't unnecessarily call bursting sores in single target scenarios
          if ( dk -> talent.bursting_sores -> ok() &&
               dk -> active_spells.bursting_sores -> target_list().size() > 0 )
          {
            dk -> active_spells.bursting_sores -> target = target;
            dk -> active_spells.bursting_sores -> execute();
          }

          if ( dk -> talent.pestilent_pustules -> ok() &&
               ++dk -> pestilent_pustules == dk -> talent.pestilent_pustules -> effectN( 1 ).base_value() )
          {
            dk -> replenish_rune( 1, dk -> gains.pestilent_pustules );
            dk -> pestilent_pustules = 0;
          }

          if ( dk -> talent.unholy_frenzy -> ok() )
          {
            dk -> buffs.unholy_frenzy -> trigger();
          }

          // TODO: Is this per festering wound, or one try?
          if ( dk -> sets.has_set_bonus( DEATH_KNIGHT_UNHOLY, T19, B2 ) )
          {
            if ( dk -> trigger_runic_corruption( 0,
                  dk -> sets.set( DEATH_KNIGHT_UNHOLY, T19, B2 ) -> effectN( 1 ).percent() ) )
            {
              dk -> procs.t19_2pc_unholy -> occur();
            }
          }
        }

        td -> debuff.festering_wound -> decrement( n_executes );
        if ( td -> debuff.soul_reaper -> up() )
        {
          dk -> buffs.soul_reaper -> trigger( n_executes );
        }
      }
    };

    if ( ! p() -> spec.festering_wound -> ok() )
    {
      return;
    }

    if ( this -> result_is_miss( state -> result ) )
    {
      return;
    }

    if ( ! td( state -> target ) -> debuff.festering_wound -> up() )
    {
      return;
    }

    make_event<fs_burst_t>( *this -> sim, p(), state -> target, n );
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

    if ( data().affected_by( p -> artifact.ambidexterity.data().effectN( 1 ) ) )
    {
      base_multiplier *= 1.0 + p -> artifact.ambidexterity.percent();
    }

    if ( data().affected_by( p -> artifact.nothing_but_the_boots.data().effectN( 1 ) ) )
    {
      crit_bonus_multiplier *= 1.0 + p -> artifact.nothing_but_the_boots.percent();
    }
  }

  virtual void   consume_resource() override;
  virtual void   execute() override;
  virtual void   impact( action_state_t* state ) override;
  virtual bool   ready() override;

  void consume_killing_machine( const action_state_t* state, proc_t* proc ) const;
  void trigger_icecap( const action_state_t* state ) const;
  void trigger_avalanche( const action_state_t* state ) const;
  void trigger_crystalline_swords( const action_state_t* state ) const;
  void trigger_necrobomb( const action_state_t* state ) const;
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

  if ( ! result_is_hit( execute_state -> result ) && resource_consumed > 0 )
    p() -> resource_gain( RESOURCE_RUNIC_POWER, resource_consumed * RUNIC_POWER_REFUND, p() -> gains.power_refund );
  trigger_crystalline_swords( execute_state );
}

// death_knight_melee_attack_t::impact() ====================================

void death_knight_melee_attack_t::impact( action_state_t* state )
{
  base_t::impact( state );

  trigger_avalanche( state );
  trigger_necrobomb( state );
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

  if ( ! p() -> sets.has_set_bonus( DEATH_KNIGHT_FROST, T18, B4 ) ||
       ( p() -> sets.has_set_bonus( DEATH_KNIGHT_FROST, T18, B4 ) &&
         ! p() -> rng().roll( player -> sets.set( DEATH_KNIGHT_FROST, T18, B4 ) -> effectN( 1 ).percent() ) ) )
  {
    killing_machine_consumed = p() -> buffs.killing_machine -> check() > 0;
    p() -> buffs.killing_machine -> decrement();
  }

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
          -p() -> talent.icecap -> effectN( 1 ).base_value() / 10.0 ) );

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

  p() -> active_spells.avalanche -> target = state -> target;
  p() -> active_spells.avalanche -> schedule_execute();

  p() -> cooldown.avalanche -> start( p() -> talent.avalanche -> internal_cooldown() );
}

// death_knight_melee_attack_t::trigger_crystalline_swords ==================

void death_knight_melee_attack_t::trigger_crystalline_swords( const action_state_t* state ) const
{
  if ( ! p() -> artifact.crystalline_swords.rank() )
  {
    return;
  }

  if ( ! weapon || ! callbacks )
  {
    return;
  }

  if ( ! result_is_hit( state -> result ) )
  {
    return;
  }

  if ( ! rng().roll( p() -> artifact.crystalline_swords.data().proc_chance() ) )
  {
    return;
  }

  if ( ++p() -> crystalline_swords < CRYSTALLINE_SWORDS_MAX )
  {
    return;
  }

  p() -> active_spells.crystalline_swords -> target = state -> target;
  // Two executes
  p() -> active_spells.crystalline_swords -> execute();
  p() -> active_spells.crystalline_swords -> execute();

  p() -> crystalline_swords = 0;
}

// death_knight_melee_attack_t::trigger_necrobomb ========================

void death_knight_melee_attack_t::trigger_necrobomb( const action_state_t* state ) const
{
  if ( ! result_is_hit( state -> result ) )
  {
    return;
  }

  if ( ! p() -> artifact.the_shambler.rank() )
  {
    return;
  }

  if ( ! p() -> rppm.shambler -> trigger() )
  {
    return;
  }

  if ( p() -> active_spells.necrobomb -> target != state -> target )
  {
    p() -> active_spells.necrobomb -> target_cache.is_valid = false;
  }

  p() -> active_spells.necrobomb -> target = state -> target;
  p() -> active_spells.necrobomb -> schedule_execute();
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
}

// ==========================================================================
// Death Knight Secondary Abilities
// ==========================================================================

// Crystalline Swords =======================================================

struct crystalline_swords_t : public death_knight_spell_t
{
  crystalline_swords_t( death_knight_t* player ) :
    death_knight_spell_t( "crystalline_swords", player, player -> find_spell( 205165 ) )
  {
    background = true;
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

// Wandering Plague =========================================================

struct wandering_plague_t : public death_knight_spell_t
{
  wandering_plague_t( death_knight_t* p, const item_t* i ) :
    death_knight_spell_t( "wandering_plague", p, p -> find_spell( 184899 ) )
  {
    background = split_aoe_damage = true;
    callbacks = may_miss = may_crit = false;
    item = i;

    aoe = -1;
  }

  void init() override
  {
    death_knight_spell_t::init();

    snapshot_flags = update_flags = 0;
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

// Necrobomb ===============================================================

struct necrobomb_t : public death_knight_spell_t
{
  necrobomb_t( death_knight_t* p ) :
    death_knight_spell_t( "necrobomb", p, p -> find_spell( 191758 ) )
  {
    background = true;
    aoe = -1;
  }

  // 2016-08-22 Necrobomb is not affected by Feast of Souls because reasons.
  double action_multiplier() const override
  {
    double m = death_knight_spell_t::action_multiplier();

    if ( p() -> artifact.feast_of_souls.rank() )
    {
      m /= 1.0 + p() -> artifact.feast_of_souls.percent();
    }

    return m;
  }

  void init() override
  {
    death_knight_spell_t::init();

    // TODO: 2016-07-30: Versatility does not affect necrobomb in game, check later
    snapshot_flags &= ~STATE_VERSATILITY;
  }
};

// Pestilence ===============================================================

struct pestilence_t : public death_knight_spell_t
{
  pestilence_t( death_knight_t* p ) :
    death_knight_spell_t( "pestilence", p, p -> find_spell( 191729 ) )
  {
    hasted_ticks = may_crit = tick_may_crit =false;
    background = true;
    dot_max_stack = data().max_stacks();
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
    school          = SCHOOL_PHYSICAL;
    may_glance      = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = timespan_t::zero();
    special         = false;

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
      p() -> resource_gain( RESOURCE_RUNIC_POWER,
          p() -> talent.runic_attenuation -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
          p() -> gains.runic_attenuation, this );
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
        frozen_pulse -> target = s -> target;
        frozen_pulse -> schedule_execute();
      }

      if ( p() -> buffs.blighted_rune_weapon -> up() )
      {
        p() -> trigger_festering_wound( s, 2 );
        p() -> buffs.blighted_rune_weapon -> decrement();
      }

      // TODO: Why doesn't Crimson Scourge proc while DnD is pulsing?
      if ( td( s -> target ) -> dot.blood_plague -> is_ticking() &&
           ! td( s -> target ) -> dot.death_and_decay -> is_ticking() )
      {
        p() -> buffs.crimson_scourge -> trigger();
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

// Apocalypse

struct apocalypse_t : public death_knight_melee_attack_t
{
  const spell_data_t* summon;
  timespan_t duration_penalty;

  apocalypse_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "apocalypse", p, p -> artifact.apocalypse ),
    summon( p -> find_spell( 221180 ) ), duration_penalty( timespan_t::from_seconds( 5 ) )
  {
    add_option( opt_timespan( "duration_penalty", duration_penalty ) );

    parse_options( options_str );

    weapon = &( p -> main_hand_weapon );
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();
    auto n_wounds = td( execute_state -> target ) -> debuff.festering_wound -> stack();

    if ( result_is_hit( execute_state -> result ) )
    {
      burst_festering_wound( execute_state, n_wounds );
    }

    for ( int i = 0; i < n_wounds; ++i )
    {
      timespan_t duration = summon -> duration() - duration_penalty;
      if ( duration <= timespan_t::zero() )
      {
        duration = timespan_t::from_seconds( 1 );
      }

      p() -> pets.apocalypse_ghoul[ i ] -> summon( duration );
    }
  }
};

// Army of the Dead =========================================================

struct army_of_the_dead_t : public death_knight_spell_t
{
  army_of_the_dead_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "army_of_the_dead", p, p -> find_specialization_spell( "Army of the Dead" ) )
  {
    parse_options( options_str );

    harmful = false;
  }

  virtual void schedule_execute( action_state_t* s ) override
  {
    death_knight_spell_t::schedule_execute( s );

    p() -> buffs.army_of_the_dead -> trigger( 1, p() -> cache.dodge() + p() -> cache.parry() );
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    if ( ! p() -> in_combat )
    {
      // Because of the new rune regen system in 4.0, it only makes
      // sense to cast ghouls 7-10s before a fight begins so you don't
      // waste rune regen and enter the fight depleted.  So, the time
      // you get for ghouls is 4-6 seconds less.
      // TODO: DBC
      for ( int i = 0; i < 8; i++ )
        p() -> pets.army_ghoul[ i ] -> summon( timespan_t::from_seconds( 35 ) );

      // Simulate rune regen for 5 seconds for the consumed runes. Ugly but works
      // Note that this presumes no other rune-using abilities are used
      // precombat
      //for ( size_t i = 0; i < MAX_RUNES; ++i )
      //  p() -> _runes.slot[ i ].regen_rune( timespan_t::from_seconds( 5.0 ) );

      //simulate RP decay for that 5 seconds
      p() -> resource_loss( RESOURCE_RUNIC_POWER, p() -> runic_power_decay_rate * 5, nullptr, nullptr );
    }
    else
    {
      // TODO: DBC
      for ( int i = 0; i < 8; i++ )
        p() -> pets.army_ghoul[ i ] -> summon( timespan_t::from_seconds( 40 ) );
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
  { }
};

// Frost Fever ==============================================================

struct hypothermia_t : public death_knight_spell_t
{
  hypothermia_t( death_knight_t* p ) :
    death_knight_spell_t( "hypothermia", p, p -> find_spell( 228322 ) )
  {
    background = true;
  }
};

struct frost_fever_t : public disease_t
{
  hypothermia_t* hypothermia;
  double rp_amount;

  frost_fever_t( death_knight_t* p ) : disease_t( p, "frost_fever", 55095 ),
    hypothermia( p -> artifact.hypothermia.rank() ? new hypothermia_t( p ) : nullptr )
  {
    base_multiplier *= 1.0 + p -> talent.freezing_fog -> effectN( 1 ).percent();

    p -> cooldown.frost_fever = p -> get_cooldown( "frost_fever" );
    p -> cooldown.frost_fever -> duration = p -> spec.frost_fever -> internal_cooldown();

    if ( hypothermia )
    {
      add_child( hypothermia );
    }
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

    if ( p() -> artifact.hypothermia.rank() &&
         rng().roll( p() -> artifact.hypothermia.data().proc_chance() ) )
    {
      hypothermia -> target = d -> target;
      hypothermia -> execute();
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
    base_multiplier *= 1.0 + p -> artifact.plaguebearer.percent();
    aoe = -1;
  }
};

struct virulent_plague_t : public disease_t
{
  virulent_plague_explosion_t* explosion;
  wandering_plague_t* wandering_plague;
  double wandering_plague_proc_chance;

  virulent_plague_t( death_knight_t* p ) :
    disease_t( p, "virulent_plague", 191587 ),
    explosion( new virulent_plague_explosion_t( p ) ),
    wandering_plague( nullptr ), wandering_plague_proc_chance( 0 )
  {
    base_tick_time *= 1.0 + p -> talent.ebon_fever -> effectN( 1 ).percent();
    dot_duration *= 1.0 + p -> talent.ebon_fever -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> artifact.plaguebearer.percent();

    add_child( explosion );
  }

  void tick( dot_t* dot ) override
  {
    disease_t::tick( dot );

    if ( rng().roll( data().effectN( 2 ).percent() ) )
    {
      if ( explosion -> target != dot -> target )
      {
        explosion -> target_cache.is_valid = false;
      }

      explosion -> target = dot -> target;
      explosion -> schedule_execute();
    }

    // Target cache does not need invalidation, since all the targets will take equal damage
    if ( dot -> state -> result_amount > 0 && rng().roll( wandering_plague_proc_chance ) )
    {
      wandering_plague -> base_dd_min = dot -> state -> result_amount;
      wandering_plague -> base_dd_max = dot -> state -> result_amount;
      wandering_plague -> target = dot -> target;
      wandering_plague -> execute();
    }
  }
};

// Blighted Rune Weapon =====================================================

struct blighted_rune_weapon_t : public death_knight_spell_t
{
  blighted_rune_weapon_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blighted_rune_weapon", p, p -> talent.blighted_rune_weapon )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.blighted_rune_weapon -> trigger( data().initial_stacks() );
  }
};

// Blood Boil ==============================================================

struct blood_boil_t : public death_knight_spell_t
{
  blood_boil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blood_boil", p, p -> find_specialization_spell( "Blood Boil" ) )
  {
    parse_options( options_str );

    aoe = -1;
    cooldown -> hasted = true;
    base_multiplier *= 1.0 + p -> spec.blood_death_knight -> effectN( 5 ).percent();
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
    {
      p() -> pets.dancing_rune_weapon -> ability.blood_boil -> target = execute_state -> target;
      p() -> pets.dancing_rune_weapon -> ability.blood_boil -> execute();
    }

    p() -> buffs.skullflowers_haemostasis -> trigger();
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( ! p() -> talent.soulgorge -> ok() && result_is_hit( state -> result ) )
    {
      p() -> apply_diseases( state, DISEASE_BLOOD_PLAGUE );
    }
  }
};

// Blood Mirror =============================================================

struct blood_mirror_damage_t : public death_knight_spell_t
{
  blood_mirror_damage_t( death_knight_t* p ) :
    death_knight_spell_t( "blood_mirror_damage", p, p -> find_spell( 221847 ) )
  {
    background = true;
    may_miss = may_crit = false;
    callbacks = false;
  }

  void init() override
  {
    death_knight_spell_t::init();
    snapshot_flags = update_flags = 0;
  }
};

struct blood_mirror_t : public death_knight_spell_t
{
  blood_mirror_damage_t* damage;

  blood_mirror_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blood_mirror", p, p -> talent.blood_mirror ),
    damage( new blood_mirror_damage_t( p ) )
  {
    parse_options( options_str );
    may_crit = may_miss = false;

    add_child( damage );
  }

  double mirror_handler( const action_state_t* state ) const
  {
    if ( ! td( state -> action -> player ) -> debuff.blood_mirror -> up() )
    {
      return 0;
    }

    double absorbed = 0;
    absorbed = state -> result_amount * data().effectN( 2 ).percent();

    damage -> base_dd_min = damage -> base_dd_max = absorbed;
    damage -> target = state -> action -> player;
    damage -> execute();

    return absorbed;
  }

  void init() override
  {
    death_knight_spell_t::init();

    if ( player -> instant_absorb_list.find( id ) != player -> instant_absorb_list.end() )
    {
      return;
    }

    p() -> instant_absorb_list[ id ] = new instant_absorb_t( player, &data(), "blood_mirror",
      std::bind( &blood_mirror_t::mirror_handler, this, std::placeholders::_1 ) );
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    td( state -> target ) -> debuff.blood_mirror -> trigger();
  }
};

// Blood Tap ================================================================

struct blood_tap_t : public death_knight_spell_t
{

  blood_tap_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blood_tap", p, p -> talent.blood_tap )

  {
    parse_options( options_str );
    harmful = false;
    energize_type = ENERGIZE_NONE;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> replenish_rune( data().effectN( 1 ).base_value(), p() -> gains.blood_tap );
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

    channeled = true;
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

  timespan_t tick_time( const action_state_t* ) const override
  { return base_tick_time; }
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
    death_knight_spell_t( "bonestorm", p, p -> find_spell( 196528 ) ),
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
  { return base_tick_time * resource_consumed / 10; }
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

    int exclusivity_check = 0;

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

    if ( exclusivity_check > 1 )
    {
      sim -> errorf( "Disabling Chains of Ice because multiple exclusive glyphs are affecting it." );
      background = true;
    }
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
      p() -> apply_diseases( state, DISEASE_FROST_FEVER );
  }
};

// Dancing Rune Weapon ======================================================

struct dancing_rune_weapon_t : public death_knight_spell_t
{
  dancing_rune_weapon_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "dancing_rune_weapon", p, p -> find_class_spell( "Dancing Rune Weapon" ) )
  {
    may_miss = may_crit = may_dodge = may_parry = harmful = false;

    parse_options( options_str );
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.dancing_rune_weapon -> trigger();
    p() -> pets.dancing_rune_weapon -> summon( data().duration() );
  }
};

// Dark Arbiter ============================================================

struct dark_arbiter_t : public death_knight_spell_t
{
  dark_arbiter_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "dark_arbiter", p, p -> talent.dark_arbiter )
  {
    parse_options( options_str );
    harmful = false;
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    p() -> pets.dark_arbiter -> summon( data().duration() );
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
    death_knight_spell_t( "dark_transformation", p, p -> find_specialization_spell( "Dark Transformation" ) )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.dark_transformation -> trigger();
    p() -> buffs.t18_4pc_unholy -> trigger();
    p() -> pets.ghoul_pet -> crazed_monstrosity -> trigger();

    trigger_taktheritrixs_command();
  }

  bool ready() override
  {
    if ( p() -> pets.ghoul_pet -> is_sleeping() )
    {
      return false;
    }

    return death_knight_spell_t::ready();
  }

  void trigger_taktheritrixs_command()
  {
    if ( p() -> pets.dark_arbiter && p() -> pets.dark_arbiter -> taktheritrix )
    {
      p() -> pets.dark_arbiter -> taktheritrix -> trigger();
    }

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

    for ( size_t i = 0; i < p() -> pets.apocalypse_ghoul.size(); i++ )
    {
      if ( p() -> pets.apocalypse_ghoul[ i ] &&
        p() -> pets.apocalypse_ghoul[ i ] -> taktheritrix )
      {
        p() -> pets.apocalypse_ghoul[ i ] -> taktheritrix -> trigger();
      }
    }
  }
};

// Death and Decay ==========================================================

struct death_and_decay_t : public death_knight_spell_t
{
  death_and_decay_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "death_and_decay", p, p -> find_specialization_spell( "Death and Decay" ) )
  {
    parse_options( options_str );

    aoe                   = -1;
    attack_power_mod.tick = p -> find_spell( 52212 ) -> effectN( 1 ).ap_coeff();
    base_tick_time        = timespan_t::from_seconds( 1.0 );
    dot_duration          = data().duration(); // 11 with tick_zero
    hasted_ticks          = false;
    tick_may_crit = tick_zero = ignore_false_positive = ground_aoe = true;

    base_tick_time *= 1.0 / ( 1.0 + p -> talent.rapid_decomposition -> effectN( 3 ).percent() );

    cooldown -> duration *= 1.0 + p -> spec.blood_death_knight -> effectN( 3 ).percent();
  }

  // Need to override dot duration to get full ticks
  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    auto n_ticks = static_cast<unsigned>( dot_duration / base_tick_time );

    return base_tick_time * n_ticks;
  }

  double cost() const override
  {
    if ( p() -> buffs.crimson_scourge -> check() )
    {
      return 0;
    }

    return death_knight_spell_t::cost();
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.crimson_scourge -> decrement();
  }

  void impact( action_state_t* s ) override
  {
    if ( s -> target -> debuffs.flying -> check() )
    {
      if ( sim -> debug ) sim -> out_debug.printf( "Ground effect %s can not hit flying target %s", name(), s -> target -> name() );
    }
    else
    {
      death_knight_spell_t::impact( s );
    }
  }

  bool ready() override
  {
    if ( p() -> talent.defile -> ok() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Death's Caress =========================================================

struct deaths_caress_t : public death_knight_spell_t
{
  deaths_caress_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "deaths_caress", p, p -> find_specialization_spell( "Death's Caress" ) )
  {
    parse_options( options_str );
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
      p() -> apply_diseases( execute_state, DISEASE_BLOOD_PLAGUE );

    if ( p() -> buffs.dancing_rune_weapon -> check() )
    {
      p() -> pets.dancing_rune_weapon -> ability.deaths_caress -> target = execute_state -> target;
      p() -> pets.dancing_rune_weapon -> ability.deaths_caress -> execute();
    }
  }
};

// Defile ==================================================================

struct defile_t : public death_knight_spell_t
{
  defile_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "defile", p, p -> talent.defile )
  {
    parse_options( options_str );

    aoe = -1;
    base_dd_min = base_dd_max = 0;
    school = p -> find_spell( 156000 ) -> get_school_type();
    attack_power_mod.tick = p -> find_spell( 156000 ) -> effectN( 1 ).ap_coeff();
    radius =  data().effectN( 1 ).radius();
    dot_duration = data().duration();
    tick_may_crit = tick_zero = true;
    hasted_ticks = false;
    ignore_false_positive = true;
    ground_aoe = true;
  }

  // Defile very likely counts as direct damage, as it procs certain trinkets that are flagged for
  // "aoe harmful spell", but not "periodic".
  dmg_e amount_type( const action_state_t*, bool ) const override
  { return DMG_DIRECT; }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_ta_multiplier( state );

    dot_t* dot = find_dot( state -> target );

    if ( dot )
    {
      m *= std::pow( 1.0 + data().effectN( 2 ).percent() / 100, std::max( dot -> current_tick - 1, 0 ) );
    }

    return m;
  }

  void impact( action_state_t* s ) override
  {
    if ( s -> target -> debuffs.flying -> check() )
    {
      if ( sim -> debug ) sim -> out_debug.printf( "Ground effect %s can not hit flying target %s", name(), s -> target -> name() );
    }
    else
    {
      death_knight_spell_t::impact( s );
    }
  }

  void tick( dot_t* dot ) override
  {
    death_knight_spell_t::tick( dot );

    p() -> buffs.defile -> trigger();
  }
};

// Death Coil ===============================================================

// TODO: Conveert to mimic blizzard spells
struct death_coil_t : public death_knight_spell_t
{
  const spell_data_t* unholy_vigor;

  death_coil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "death_coil", p, p -> find_specialization_spell( "Death Coil" ) ),
    unholy_vigor( p -> find_spell( 196263 ) )
  {
    parse_options( options_str );

    attack_power_mod.direct = p -> find_spell( 47632 ) -> effectN( 1 ).ap_coeff();
    base_multiplier *= 1.0 + p -> artifact.deadliest_coil.percent();
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

    // Sudden Doomed Death Coils will now generate power for Val'Kyr
    if ( p() -> buffs.sudden_doom -> check() && p() -> pets.dark_arbiter )
    {
      p() -> pets.dark_arbiter -> increase_power( base_costs[ RESOURCE_RUNIC_POWER ] );
    }

    p() -> buffs.sudden_doom -> decrement();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> trigger_runic_corruption( base_costs[ RESOURCE_RUNIC_POWER ] );

      td( execute_state -> target ) -> debuff.scourge_of_worlds -> trigger();
    }

    if ( p() -> talent.shadow_infusion -> ok() && ! p() -> buffs.dark_transformation -> up() )
    {
      p() -> cooldown.dark_transformation -> adjust( -timespan_t::from_seconds(
        p() -> talent.shadow_infusion -> effectN( 1 ).base_value() ) );
    }

    p() -> buffs.necrosis -> trigger();
    p() -> pets.ghoul_pet -> resource_gain( RESOURCE_ENERGY,
      unholy_vigor -> effectN( 1 ).resource( RESOURCE_ENERGY ), nullptr, this );

    p() -> trigger_death_march( execute_state );
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( rng().roll( player -> sets.set( DEATH_KNIGHT_UNHOLY, T19, B4 ) -> effectN( 1 ).percent() ) )
    {
      p() -> trigger_festering_wound( state, 1, true ); // TODO: Does this ignore ICD?
    }
  }
};

// Death Strike =============================================================

struct blood_shield_buff_t : public absorb_buff_t
{
  blood_shield_buff_t( death_knight_t* player ) :
    absorb_buff_t( absorb_buff_creator_t( player, "blood_shield", player -> find_spell( 77535 ) )
                   .school( SCHOOL_PHYSICAL )
                   .source( player -> get_stats( "blood_shield" ) ) )
  { }

  // Clamp shield value so that if T17 4PC is used, we have at least 5% of
  // current max health of absorb left, if Vampiric Blood is up
  void absorb_used( double ) override
  {
    death_knight_t* p = debug_cast<death_knight_t*>( player );
    if ( p -> sets.has_set_bonus( DEATH_KNIGHT_BLOOD, T17, B4 ) && p -> buffs.vampiric_blood -> up() )
    {
      double min_absorb = p -> resources.max[ RESOURCE_HEALTH ] *
                          p -> sets.set( DEATH_KNIGHT_BLOOD, T17, B4 ) -> effectN( 1 ).percent();

      if ( sim -> debug )
        sim -> out_debug.printf( "%s blood_shield absorb clamped to %f", player -> name(), min_absorb );

      if ( current_value < min_absorb )
        current_value = min_absorb;
    }
  }
};

struct death_strike_offhand_t : public death_knight_melee_attack_t
{
  death_strike_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "death_strike_offhand", p, p -> find_spell( 66188 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    base_multiplier  = 1.0 + p -> spec.veteran_of_the_third_war -> effectN( 7 ).percent();
  }
};

struct blood_shield_t : public absorb_t
{
  blood_shield_t( death_knight_t* p ) :
    absorb_t( "blood_shield", p, p -> find_spell( 77535 ) )
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
    ds_data( p -> find_class_spell( "Death Strike" ) ),
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
    return std::max( player -> resources.max[ RESOURCE_HEALTH ] * ds_data -> effectN( 5 ).percent(),
      player -> compute_incoming_damage( interval ) );
  }

  double base_da_max( const action_state_t* ) const override
  {
    return std::max( player -> resources.max[ RESOURCE_HEALTH ] * ds_data -> effectN( 5 ).percent(),
      player -> compute_incoming_damage( interval ) );
  }

  double action_multiplier() const override
  {
    double m = death_knight_heal_t::action_multiplier();

    if ( p() -> buffs.icebound_fortitude -> up() )
    {
      m *= 1.0 + p() -> artifact.ice_in_your_veins.percent();
    }

    m *= 1.0 + p() -> buffs.skullflowers_haemostasis -> stack_value();

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
    death_knight_melee_attack_t( "death_strike", p, p -> find_class_spell( "Death Strike" ) ),
    oh_attack( nullptr ), heal( new death_strike_heal_t( p ) )
  {
    parse_options( options_str );
    may_parry = false;
    base_multiplier *= 1.0 + p -> spec.blood_death_knight -> effectN( 1 ).percent();

    base_costs[ RESOURCE_RUNIC_POWER ] += p -> sets.set( DEATH_KNIGHT_BLOOD, T18, B4 ) -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER );

    weapon = &( p -> main_hand_weapon );
  }

  double action_multiplier() const override
  {
    double m = death_knight_melee_attack_t::action_multiplier();

    m *= 1.0 + p() -> buffs.skullflowers_haemostasis -> stack_value();

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

    return c;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( oh_attack )
      oh_attack -> execute();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
    {
      p() -> pets.dancing_rune_weapon -> ability.death_strike -> target = execute_state -> target;
      p() -> pets.dancing_rune_weapon -> ability.death_strike -> execute();
    }

    if ( result_is_hit( execute_state -> result ) )
    {
      heal -> execute();
    }

    p() -> trigger_death_march( execute_state );
    p() -> buffs.skullflowers_haemostasis -> expire();
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

struct empower_rune_weapon_t : public death_knight_spell_t
{
  empower_rune_weapon_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "empower_rune_weapon", p, p -> find_specialization_spell( "Empower Rune Weapon" ) )
  {
    parse_options( options_str );

    harmful = false;
    // Handle energize in a custom way
    energize_type = ENERGIZE_NONE;
  }

  void init() override
  {
    death_knight_spell_t::init();

    cooldown -> charges = data().charges() +
      p() -> legendary.seal_of_necrofantasia -> effectN( 1 ).base_value();
    cooldown -> duration = data().charge_cooldown() *
      ( 1.0 - p() -> legendary.seal_of_necrofantasia -> effectN( 2 ).percent() );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    double filled = 0, overflow = 0;
    for ( auto& rune: p() -> _runes.slot )
    {
      if ( rune.is_depleted() )
      {
        filled += 1;
      }
      else if ( rune.is_regenerating() )
      {
        auto fill_level = rune.fill_level();
        filled += 1.0 - fill_level;
        overflow += fill_level;
      }
      else
      {
        continue;
      }

      rune.fill_rune( p() -> gains.empower_rune_weapon );
    }
  }

  bool ready() override
  {
    if ( p() -> talent.hungering_rune_weapon -> ok() )
    {
      return false;
    }

    return death_knight_spell_t::ready();
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
        main -> target = target;
        main -> execute();

        if ( sim -> target_non_sleeping_list.size() > 1 )
        {
          aoe -> target_cache.is_valid = false;
          aoe -> target = target;
          aoe -> execute();
        }
      }
    }
  }
};

// Festering Strike =========================================================

struct festering_strike_t : public death_knight_melee_attack_t
{
  festering_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "festering_strike", p, p -> find_specialization_spell( "Festering Strike" ) )
  {
    parse_options( options_str );

    base_multiplier *= 1.0 + p -> artifact.rotten_touch.percent();
  }

  void impact( action_state_t* s ) override
  {
    static const std::array<unsigned, 4> fw_proc_stacks = { { 2, 3, 3, 4 } };

    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      unsigned n = static_cast<unsigned>( rng().range( 0, fw_proc_stacks.size() ) );
      unsigned n_stacks = fw_proc_stacks[ n ];
      if ( s -> result == RESULT_CRIT && p() -> talent.castigator -> ok() )
      {
        n_stacks += p() -> talent.castigator -> effectN( 1 ).base_value();
      }

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
  frostscythe_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "frostscythe", p, p -> talent.frostscythe )
  {
    parse_options( options_str );

    weapon = &( player -> main_hand_weapon );
    aoe = -1;

    // TODO: Check how this is exactly in game
    crit_bonus_multiplier *= 1.0 + p -> spec.death_knight -> effectN( 5 ).percent();
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    consume_killing_machine( execute_state, p() -> procs.fs_killing_machine );
    trigger_icecap( execute_state );
  }

  double composite_crit_chance() const override
  {
    double cc = death_knight_melee_attack_t::composite_crit_chance();

    cc += p() -> buffs.killing_machine -> value();

    return cc;
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
    range += p -> artifact.chill_of_the_grave.value();
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    if ( td( target ) -> debuff.razorice -> stack() == 5 ) // TODO: Hardcoded, sad face
    {
      m *= 1.0 + p() -> talent.shattering_strikes -> effectN( 1 ).percent();
    }

    return m;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    // TODO: Both hands, or just main hand?
    trigger_icecap( execute_state );

    // TODO: Both hands, or just main hand?
    if ( execute_state -> result == RESULT_CRIT )
    {
      p() -> buffs.t18_4pc_frost_crit -> trigger();
    }
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
    range += p -> artifact.chill_of_the_grave.value();

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
      mh -> target = execute_state -> target;
      mh -> execute();
      oh -> target = execute_state -> target;
      oh -> execute();

      p() -> trigger_runic_empowerment( resource_consumed );
    }

    death_knight_td_t* tdata = td( execute_state -> target );
    if ( p() -> talent.shattering_strikes -> ok() &&
         tdata -> debuff.razorice -> stack() == 5 ) // TODO: Hardcoded, sad face
    {
      tdata -> debuff.razorice -> expire();
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
};

struct glacial_advance_t : public death_knight_spell_t
{
  glacial_advance_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "glacial_advance", player, player -> talent.glacial_advance )
  {
    parse_options( options_str );
    school = SCHOOL_FROST; // Damage is frost so override this to make reports make more sense

    execute_action = new glacial_advance_damage_t( player );
    add_child( execute_action );
  }
};

// Heart Strike =============================================================

struct heart_strike_t : public death_knight_melee_attack_t
{
  heart_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "heart_strike", p, p -> find_specialization_spell( "Heart Strike" ) )
  {
    parse_options( options_str );

    energize_type = ENERGIZE_PER_HIT;
    energize_amount += p -> talent.heartbreaker -> effectN( 1 ).base_value();

    weapon = &( p -> main_hand_weapon );
  }

  int n_targets() const override
  { return td( target ) -> in_aoe_radius() ? 5 : 2; }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
    {
      p() -> pets.dancing_rune_weapon -> ability.heart_strike -> target = execute_state -> target;
      p() -> pets.dancing_rune_weapon -> ability.heart_strike -> execute();
    }

    if ( rng().roll( p() -> sets.set( DEATH_KNIGHT_BLOOD, T18, B2 ) -> effectN( 1 ).percent() ) )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER,
          p() -> sets.set( DEATH_KNIGHT_BLOOD, T18, B2 ) -> effectN( 2 ).base_value() / 10,
          p() -> gains.t18_2pc_blood, this );
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

    p() -> replenish_rune( data().effectN( 1 ).base_value(), p() -> gains.horn_of_winter );
  }
};

// Howling Blast ============================================================

struct howling_blast_t : public death_knight_spell_t
{
  howling_blast_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "howling_blast", p, p -> find_specialization_spell( "Howling Blast" ) )
  {
    parse_options( options_str );

    aoe                 = -1;
    base_aoe_multiplier = data().effectN( 1 ).percent();
    base_multiplier    *= 1.0 + p -> talent.freezing_fog -> effectN( 1 ).percent();
    base_multiplier    *= 1.0 + p -> artifact.blast_radius.percent();
  }

  double runic_power_generation_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::runic_power_generation_multiplier( state );

    if ( p() -> buffs.rime -> check() )
    {
      m *= 1.0 + ( p() -> buffs.rime -> data().effectN( 3 ).percent() +
                   p() -> sets.set( DEATH_KNIGHT_FROST, T19, B4 ) -> effectN( 1 ).percent() );
    }

    return m;
  }

  gain_t* energize_gain( const action_state_t* /* state */ ) const override
  {
    if ( p() -> buffs.rime -> check() && p() -> sets.has_set_bonus( DEATH_KNIGHT_FROST, T19, B4 ) )
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

    p() -> buffs.rime -> decrement();
  }

  void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      p() -> apply_diseases( s, DISEASE_FROST_FEVER );
  }
};

// Hungering Rune Weapon ======================================================

struct hungering_rune_weapon_t : public death_knight_spell_t
{
  hungering_rune_weapon_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "hungering_rune_weapon", p, p -> talent.hungering_rune_weapon )
  {
    parse_options( options_str );

    harmful = hasted_ticks = false;
    // Handle energize in a custom way
    energize_type = ENERGIZE_NONE;
    tick_zero = true;
    target = player;
    // Spell has two different periodicities in two effects, weird++. Pick the one that is indicated
    // by the tooltip.
    base_tick_time = data().effectN( 1 ).period();
  }

  void init() override
  {
    death_knight_spell_t::init();

    cooldown -> charges = data().charges() +
      p() -> legendary.seal_of_necrofantasia -> effectN( 1 ).base_value();
    cooldown -> duration = data().charge_cooldown() *
      ( 1.0 - p() -> legendary.seal_of_necrofantasia -> effectN( 2 ).percent() );
  }

  void tick( dot_t* d ) override
  {
    death_knight_spell_t::tick( d );

    p() -> replenish_rune( data().effectN( 1 ).base_value(), p() -> gains.hungering_rune_weapon );
    p() -> resource_gain( RESOURCE_RUNIC_POWER, data().effectN( 2 ).resource( RESOURCE_RUNIC_POWER ), p() -> gains.hungering_rune_weapon, this );
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

struct unholy_coil_t : public death_knight_heal_t
{
  unholy_coil_t( death_knight_t* p, const item_t* i ) :
    death_knight_heal_t( "unholy_coil", p, p -> find_spell( 184968 ) )
  {
    background = true;
    may_crit = callbacks = false;
    target = p;
    item = i;
  }

  void init() override
  {
    death_knight_heal_t::init();

    snapshot_flags = update_flags = 0;
  }
};

struct marrowrend_t : public death_knight_melee_attack_t
{
  unholy_coil_t* unholy_coil;
  double unholy_coil_coeff;

  marrowrend_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "marrowrend", p, p -> find_specialization_spell( "Marrowrend" ) ),
    unholy_coil( nullptr ), unholy_coil_coeff( 0 )
  {
    parse_options( options_str );

    weapon = &( p -> main_hand_weapon );
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
    {
      p() -> pets.dancing_rune_weapon -> ability.marrowrend -> target = execute_state -> target;
      p() -> pets.dancing_rune_weapon -> ability.marrowrend -> execute();
    }

    p() -> buffs.bone_shield -> trigger( data().effectN( 3 ).base_value() );

    if ( execute_state -> result_amount > 0 && unholy_coil )
    {
      unholy_coil -> base_dd_min = unholy_coil -> base_dd_max = execute_state -> result_amount * unholy_coil_coeff;
      unholy_coil -> execute();
    }
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

  bool ready() override
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Obliterate ===============================================================

struct frozen_obliteration_t : public death_knight_melee_attack_t
{
  double coeff;

  frozen_obliteration_t( death_knight_t* p, const std::string& name, double coeff ) :
    death_knight_melee_attack_t( name, p, p -> find_spell( 184982 ) ),
    coeff( coeff )
  {
    background = special = true;
    callbacks = false;
    may_crit = false;
  }

  void init() override
  {
    death_knight_melee_attack_t::init();

    snapshot_flags = update_flags = 0;
  }

  void proxy_execute( const action_state_t* source_state )
  {
    double m = coeff;
    target = source_state -> target;

    // Mastery has to be special cased here, since no other multipliers seem to work
    if ( p() -> mastery.frozen_heart -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    base_dd_min = base_dd_max = source_state -> result_amount * m;

    execute();
  }
};

struct obliterate_strike_t : public death_knight_melee_attack_t
{
  frozen_obliteration_t* fo;

  obliterate_strike_t( death_knight_t* p, const std::string& name, weapon_t* w, const spell_data_t* s ) :
    death_knight_melee_attack_t( name, p, s ),
    fo( nullptr )
  {
    background = special = true;
    may_miss = false;
    weapon = w;
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

    if ( result_is_hit( execute_state -> result ) && fo )
    {
      fo -> proxy_execute( execute_state );
    }

    trigger_icecap( execute_state );

    if ( execute_state -> result == RESULT_CRIT )
    {
      p() -> buffs.t18_4pc_frost_haste -> trigger();
    }
  }
};

struct obliterate_t : public death_knight_melee_attack_t
{
  obliterate_strike_t* mh, *oh;

  obliterate_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "obliterate", p, p -> find_class_spell( "Obliterate" ) ),
    mh( new obliterate_strike_t( p, "obliterate_mh", &( p -> main_hand_weapon ), data().effectN( 2 ).trigger() ) ),
    oh( new obliterate_strike_t( p, "obliterate_offhand", &( p -> off_hand_weapon ), data().effectN( 3 ).trigger() ) )
  {
    parse_options( options_str );

    add_child( mh );
    add_child( oh );
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      mh -> target = execute_state -> target;
      mh -> execute();

      oh -> target = execute_state -> target;
      oh -> execute();

      p() -> buffs.rime -> trigger();
    }

    if ( rng().roll( p() -> artifact.overpowered.data().proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_RUNIC_POWER, p() -> artifact.overpowered.value() / 10.0,
          p() -> gains.overpowered, this );
    }

    if ( rng().roll( p() -> legendary.koltiras_newfound_will -> proc_chance() ) )
    {
      p() -> replenish_rune( p() -> legendary.koltiras_newfound_will -> effectN( 1 ).trigger() -> effectN( 1 ).base_value(),
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

  outbreak_driver_t( death_knight_t* p ) :
    death_knight_spell_t( "outbreak_driver", p, p -> dbc.spell( 196782 ) ),
    spread( new outbreak_spreader_t( p ) )
  {
    quiet = background = tick_zero = dual = true;
    callbacks = hasted_ticks = false;
  }

  void tick( dot_t* dot ) override
  {
    if ( spread -> target != dot -> target )
    {
      spread -> target_cache.is_valid = false;
    }

    spread -> target = dot -> target;
    spread -> schedule_execute();
  }
};

struct outbreak_t : public death_knight_spell_t
{
  outbreak_driver_t* spread;

  outbreak_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "outbreak", p, p -> find_specialization_spell( "Outbreak" ) ),
    spread( new outbreak_driver_t( p ) )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      spread -> target = execute_state -> target;
      spread -> schedule_execute();
    }
  }
};

// Pillar of Frost ==========================================================

struct pillar_of_frost_t : public death_knight_spell_t
{
  pillar_of_frost_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "pillar_of_frost", p, p -> find_specialization_spell( "Pillar of Frost" ) )
  {
    parse_options( options_str );

    harmful = false;

    if ( p -> sets.has_set_bonus( DEATH_KNIGHT_FROST, T17, B2 ) )
    {
      energize_type = ENERGIZE_ON_CAST;
      energize_amount = p -> sets.set( DEATH_KNIGHT_FROST, T17, B2 ) -> effectN( 1 ).trigger() -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER );
      energize_resource = RESOURCE_RUNIC_POWER;
    }
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

struct frozen_soul_t : public death_knight_spell_t
{
  const std::vector<player_t*>& targets;

  frozen_soul_t( death_knight_t* p, std::vector<player_t*>& t ) :
    death_knight_spell_t( "frozen_soul", p, p -> find_spell( 204959 ) ),
    targets( t )
  {
    background = true;
    aoe = -1;
  }

  double action_multiplier() const override
  {
    double m = death_knight_spell_t::action_multiplier();

    m *= 1.0 + p() -> artifact.frozen_soul.data().effectN( 2 ).percent() * targets.size();

    return m;
  }
};

struct remorseless_winter_damage_t : public death_knight_spell_t
{
  std::vector<player_t*>& targets;

  remorseless_winter_damage_t( death_knight_t* p, std::vector<player_t*>& t ) :
    death_knight_spell_t( "remorseless_winter_damage", p, p -> find_spell( 196771 ) ),
    targets( t )
  {
    background = true;
    aoe = -1;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( p() -> artifact.frozen_soul.rank() &&
         range::find( targets, state -> target ) == targets.end() )
    {
      targets.push_back( state -> target );
    }
  }
};

// LEGION-TODO: Proper targeting
struct remorseless_winter_t : public death_knight_spell_t
{
  std::vector<player_t*> targets;
  frozen_soul_t* frozen_soul;

  remorseless_winter_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "remorseless_winter", p, p -> find_specialization_spell( "Remorseless Winter" ) ),
    frozen_soul( p -> artifact.frozen_soul.rank() ? new frozen_soul_t( p, targets ) : nullptr )
  {
    parse_options( options_str );

    hasted_ticks = false;
    base_multiplier *= 1.0 + p -> artifact.dead_of_winter.percent();
    tick_action = new remorseless_winter_damage_t( p, targets );
  }

  void last_tick( dot_t* d ) override
  {
    death_knight_spell_t::last_tick( d );

    if ( p() -> artifact.frozen_soul.rank() && targets.size() > 0 )
    {
      frozen_soul -> target = target;
      frozen_soul -> execute();
      targets.clear();
    }
  }

  void reset() override
  {
    death_knight_spell_t::reset();
    targets.clear();
  }
};

// Scourge Strike and Clawing Shadows =======================================

struct scourge_strike_base_t : public death_knight_melee_attack_t
{
  std::array<double, 6> instructors_chance;

  scourge_strike_base_t( const std::string& name, death_knight_t* p, const spell_data_t* spell ) :
    death_knight_melee_attack_t( name, p, spell )
  {
    weapon = &( player -> main_hand_weapon );

    instructors_chance = { { 0.20, 0.40, 0.20, 0.10, 0.05, 0.05 } };
  }

  int n_targets() const override
  { return td( target ) -> in_aoe_radius() ? -1 : 0; }

  double action_multiplier() const override
  {
    double m = death_knight_melee_attack_t::action_multiplier();

    if ( p() -> buffs.necrosis -> up() )
    {
      m *= 1.0 + p() -> buffs.necrosis -> stack_value();
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = death_knight_melee_attack_t::composite_target_multiplier( target );

    m *= 1.0 + td( target ) -> debuff.scourge_of_worlds -> stack_value();

    return m;
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    p() -> buffs.necrosis -> decrement();
  }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      int n_burst = 1;

      if ( p() -> talent.castigator -> ok() && state -> result == RESULT_CRIT )
      {
        n_burst += p() -> talent.castigator -> effectN( 2 ).base_value();
      }

      if ( p() -> legendary.the_instructors_fourth_lesson )
      {
        assert( instructors_chance.size() == p() -> legendary.the_instructors_fourth_lesson + 1 );

        double roll = rng().real();
        double sum = 0;

        for ( size_t i = 0; i < instructors_chance.size(); i++ )
        {
          sum += instructors_chance[ i ];

          if ( roll <= sum )
          {
            n_burst += ( int ) i;
            break;
          }
        }
      }

      if ( rng().roll( p() -> artifact.scourge_the_unbeliever.percent() ) )
      {
        p() -> replenish_rune( 1, p() -> gains.scourge_the_unbeliever );
      }

      burst_festering_wound( state, n_burst );
    }
  }
};

struct clawing_shadows_t : public scourge_strike_base_t
{
  clawing_shadows_t( death_knight_t* p, const std::string& options_str ) :
    scourge_strike_base_t( "clawing_shadows", p, p -> talent.clawing_shadows )
  {
    parse_options( options_str );

    // HOTFIX 2016-08-23: Clawing Shadows damage has been changed to 130% weapon damage (was 150% Attack Power).
    /*
    weapon_multiplier = 1.3;
    normalize_weapon_speed = true;
    */
  }
};

struct scourge_strike_t : public scourge_strike_base_t
{
  struct scourge_strike_shadow_t : public death_knight_melee_attack_t
  {
    scourge_strike_shadow_t( death_knight_t* p ) :
      death_knight_melee_attack_t( "scourge_strike_shadow", p, p -> find_spell( 70890 ) )
    {
      may_miss = may_parry = may_dodge = false;
      proc = background = true;
      weapon = &( player -> main_hand_weapon );
      dual = true;
      school = SCHOOL_SHADOW;
    }

    double action_multiplier() const override
    {
      double m = death_knight_melee_attack_t::action_multiplier();

      if ( p() -> buffs.necrosis -> up() )
      {
        m *= 1.0 + p() -> buffs.necrosis -> stack_value();
      }

      return m;
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = death_knight_melee_attack_t::composite_target_multiplier( target );

      m *= 1.0 + td( target ) -> debuff.scourge_of_worlds -> stack_value();

      return m;
    }

    void impact( action_state_t* state ) override
    {
      death_knight_melee_attack_t::impact( state );

      if ( state -> result == RESULT_CRIT && p() -> talent.castigator -> ok() )
      {
        burst_festering_wound( state, 1 );
      }
    }
  };

  scourge_strike_shadow_t* scourge_strike_shadow;

  scourge_strike_t( death_knight_t* p, const std::string& options_str ) :
    scourge_strike_base_t( "scourge_strike", p, p -> find_specialization_spell( "Scourge Strike" ) ),
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
      scourge_strike_shadow -> target = state -> target;
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

// Sindragosa's Fury ========================================================

// TODO: Fancy targeting
struct sindragosas_fury_t : public death_knight_spell_t
{
  sindragosas_fury_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "sindragosas_fury", p, p -> artifact.sindragosas_fury )
  {
    parse_options( options_str );

    aoe = -1;

    parse_effect_data( p -> find_spell( 190780 ) -> effectN( 1 ) );
  }
};

// Soul Reaper ==============================================================

struct soul_reaper_t : public death_knight_melee_attack_t
{
  soul_reaper_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "soul_reaper", p, p ->  talent.soul_reaper )
  {
    parse_options( options_str );

    weapon = &( p -> main_hand_weapon );
  }

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      td( execute_state -> target ) -> debuff.soul_reaper -> trigger();
    }
  }
};

// Soulgorge ================================================================

struct soulgorge_t : public death_knight_spell_t
{
  soulgorge_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "soulgorge", p, p -> talent.soulgorge )
  {
    parse_options( options_str );
    aoe = -1;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    death_knight_spell_t::available_targets( tl );

    auto it = tl.begin();
    while ( it != tl.end() )
    {
      if ( ! td( *it ) -> dot.blood_plague -> is_ticking() )
      {
        it = tl.erase( it );
      }
      else
      {
        it++;
      }
    }

    return tl.size();
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    double rune_regen_value = 0;
    for ( auto t : target_list() )
    {
      auto td_ = td( t );
      double expended = 1.0 - td_ -> dot.blood_plague -> remains() / td_ -> dot.blood_plague -> duration();

      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s soulgorges %s blood_plague: remains=%.3f duration=%.3f expended=%.f gain=%.f%%",
          player -> name(), t -> name(), td_ -> dot.blood_plague -> remains().total_seconds(),
          td_ -> dot.blood_plague -> duration().total_seconds(),
          expended,
          100.0 * data().effectN( 2 ).percent() * expended );
      }

      td_ -> dot.blood_plague -> cancel();

      rune_regen_value += data().effectN( 2 ).percent() * expended;
    }

    if ( rune_regen_value > 0 )
    {
      p() -> buffs.soulgorge -> expire();
      p() -> buffs.soulgorge -> trigger( static_cast<int>( rune_regen_value * 100 ), rune_regen_value );
    }
  }
};

// Summon Gargoyle ==========================================================

struct summon_gargoyle_t : public death_knight_spell_t
{
  summon_gargoyle_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "summon_gargoyle", p, p -> find_class_spell( "Summon Gargoyle" ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    timespan_t duration = data().effectN( 3 ).trigger() -> duration();

    p() -> pets.gargoyle -> summon( duration );
  }

  bool ready() override
  {
    if ( p() -> talent.dark_arbiter -> ok() )
    {
      return false;
    }

    return death_knight_spell_t::ready();
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

    double power = p() -> buffs.bone_shield -> stack() * data().effectN( 3 ).base_value();
    double shield = p() -> buffs.bone_shield -> check() * data().effectN( 4 ).percent();

    p() -> resource_gain( RESOURCE_RUNIC_POWER, power, p() -> gains.tombstone, this );
    p() -> buffs.tombstone -> trigger( 1, shield * p() -> resources.max[ RESOURCE_HEALTH ] );
    p() -> buffs.bone_shield -> expire();
  }
};

// Breath of Sindragosa =====================================================

struct breath_of_sindragosa_tick_t: public death_knight_spell_t
{
  action_t* parent;

  breath_of_sindragosa_tick_t( death_knight_t* p, action_t* parent ):
    death_knight_spell_t( "breath_of_sindragosa_tick", p, p -> find_spell( 155166 ) ),
    parent( parent )
  {
    aoe = -1;
    background = true;
  }

  void impact( action_state_t* s ) override
  {
    if ( s -> target == target )
      death_knight_spell_t::impact( s );
    else
    {
      double damage = s -> result_amount;
      damage /= execute_state -> n_targets;
      s -> result_amount = damage;
      death_knight_spell_t::impact( s );
    }
  }
};

struct breath_of_sindragosa_t : public death_knight_spell_t
{
  breath_of_sindragosa_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "breath_of_sindragosa", p, p -> find_talent_spell( "Breath of Sindragosa" ) )
  {
    parse_options( options_str );

    may_miss = may_crit = hasted_ticks = callbacks = false;
    tick_zero = dynamic_tick_action = true;

    tick_action = new breath_of_sindragosa_tick_t( p, this );
    school = tick_action -> school;

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

    p() -> trigger_runic_empowerment( resource_consumed );

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

    snapshot_flags |= STATE_MUL_TA | STATE_TGT_MUL_TA | STATE_MUL_PERSISTENT;
    update_flags |= STATE_MUL_TA | STATE_TGT_MUL_TA;
  }
};

// Anti-magic Shell =========================================================

struct antimagic_shell_buff_t : public buff_t
{
  death_knight_t* dk;

  antimagic_shell_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "antimagic_shell", p -> find_class_spell( "Anti-Magic Shell" ) )
                              .cd( timespan_t::zero() ) ),
      dk( p )
  { }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    death_knight_t* p = debug_cast< death_knight_t* >( player );
    p -> antimagic_shell_absorbed = 0.0;
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
    death_knight_spell_t( "antimagic_shell", p, p -> find_class_spell( "Anti-Magic Shell" ) ),
    interval( 60 ), interval_stddev( 0.05 ), interval_stddev_opt( 0 ), damage( 0 )
  {
    cooldown = p -> cooldown.antimagic_shell;
    harmful = may_crit = may_miss = false;
    base_dd_min = base_dd_max = 0;
    target = player;

    add_option( opt_float( "interval", interval ) );
    add_option( opt_float( "interval_stddev", interval_stddev_opt ) );
    add_option( opt_float( "damage", damage ) );
    parse_options( options_str );

    // Allow as low as 15 second intervals, due to new glyph
    if ( interval < 15.0 )
    {
      sim -> errorf( "%s minimum interval for Anti-Magic Shell is 15 seconds.", player -> name() );
      interval = 15.0;
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

    if ( p() -> talent.antimagic_barrier -> ok() )
    {
      p() -> buffs.antimagic_barrier -> trigger();
    }

    // If using the fake soaking, immediately grant the RP in one go
    if ( damage > 0 )
    {
      double absorbed = std::min( damage * data().effectN( 1 ).percent(),
                                  p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 2 ).percent() );

      double generated = absorbed / p() -> resources.max[ RESOURCE_HEALTH ];

      // AMS generates 2 runic power per percentage max health absorbed.
      p() -> resource_gain( RESOURCE_RUNIC_POWER, util::round( generated * 100.0 * 2.0 ), p() -> gains.antimagic_shell, this );
    }
    else
      p() -> buffs.antimagic_shell -> trigger();
  }
};

// Vampiric Blood ===========================================================

struct vampiric_blood_t : public death_knight_spell_t
{
  vampiric_blood_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "vampiric_blood", p, p -> find_specialization_spell( "Vampiric Blood" ) )
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
    death_knight_spell_t( "icebound_fortitude", p, p -> find_class_spell( "Icebound Fortitude" ) )
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

// Death Pact

// TODO-WOD: Healing absorb kludge

struct death_pact_t : public death_knight_heal_t
{
  death_pact_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_heal_t( "death_pact", p, p -> find_talent_spell( "Death Pact" ) )
  {
    may_crit = false;
    base_pct_heal = data().effectN( 1 ).percent();

    parse_options( options_str );
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
    bp_expr = a -> target -> get_dot( "blood_plague", p ) -> create_expression( p -> active_spells.blood_plague, expression, true );
    ff_expr = a -> target -> get_dot( "frost_fever", p ) -> create_expression( p -> active_spells.frost_fever, expression, true );

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
    buff_t( buff_creator_t( p, "runic_corruption", p -> find_spell( 51460 ) )
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
    super( buff_creator_t( p, "vampiric_blood", p -> find_specialization_spell( "Vampiric Blood" ) ).cd( timespan_t::zero() ) )
  {
    delta = data().effectN( 2 ).percent();
  }
};

struct antimagic_barrier_buff_t : public health_pct_increase_buff_t<buff_t, buff_creator_t>
{
  antimagic_barrier_buff_t( death_knight_t* p ) :
    super( buff_creator_t( p, "antimagic_barrier", p -> find_spell( 205725 ) ) )
  {
    delta = dk() -> talent.antimagic_barrier -> effectN( 1 ).percent();
  }
};

} // UNNAMED NAMESPACE

// Runeforges ==============================================================

void runeforge::razorice_attack( special_effect_t& effect )
{
  struct razorice_attack_t : public death_knight_melee_attack_t
  {
    razorice_attack_t( death_knight_t* player, const std::string& name ) :
      death_knight_melee_attack_t( name, player, player -> find_spell( 50401 ) )
    {
      school      = SCHOOL_FROST;
      may_miss    = callbacks = false;
      background  = proc = true;
      base_multiplier *= 1.0 + player -> artifact.bad_to_the_bone.percent();

      weapon = &( player -> main_hand_weapon );
    }

    // No double dipping to Frost Vulnerability
    double composite_target_multiplier( player_t* t ) const override
    {
      double m = death_knight_melee_attack_t::composite_target_multiplier( t );

      m /= 1.0 + td( t ) -> debuff.razorice -> check() *
            td( t ) -> debuff.razorice -> data().effectN( 1 ).percent();

      return m;
    }
  };

  effect.proc_flags_ = PF_MELEE | PF_MELEE_ABILITY;
  effect.proc_flags2_ = PF2_ALL_HIT;
  effect.execute_action = new razorice_attack_t( debug_cast<death_knight_t*>( effect.item -> player ), effect.name() );
  effect.proc_chance_ = 1.0;
  new dbc_proc_callback_t( effect.item, effect );
}

void runeforge::razorice_debuff( special_effect_t& effect )
{
  struct razorice_callback_t : public dbc_proc_callback_t
  {
    razorice_callback_t( const special_effect_t& effect ) :
     dbc_proc_callback_t( effect.item, effect )
    { }

    void execute( action_t* a, action_state_t* state ) override
    {
      debug_cast< death_knight_t* >( a -> player ) -> get_target_data( state -> target ) -> debuff.razorice -> trigger();
      if ( a -> sim -> current_time() < timespan_t::from_seconds( 0.01 ) )
        debug_cast< death_knight_t* >( a -> player ) -> get_target_data( state -> target ) -> debuff.razorice -> constant = false;
    }
  };

  effect.proc_flags_ = PF_MELEE | PF_MELEE_ABILITY;
  effect.proc_flags2_ = PF2_ALL_HIT;

  new razorice_callback_t( effect );
}

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
        base_pct_heal *= 1.0 + dk -> artifact.the_darkest_crusade.percent();
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
    _runes.consume( amount );
    // Ensure rune state is consistent with the actor resource state for runes
    assert( _runes.runes_full() == resources.current[ RESOURCE_RUNE ] );
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
    : ( spec.runic_corruption -> effectN( 2 ).percent() * rpcost / 100.0 );

  if ( ! rng().roll( actual_chance ) )
    return false;

  timespan_t duration = timespan_t::from_seconds( 3.0 * cache.attack_haste() );
  if ( buffs.runic_corruption -> check() == 0 )
    buffs.runic_corruption -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
  else
    buffs.runic_corruption -> extend_duration( this, duration );

  return true;
}

void death_knight_t::trigger_festering_wound( const action_state_t* state, unsigned n, bool bypass_icd )
{
  if ( ! bypass_icd && cooldown.festering_wound -> down() )
  {
    return;
  }

  auto td = get_target_data( state -> target );

  td -> debuff.festering_wound -> trigger( n );

  cooldown.festering_wound -> start( spec.festering_wound -> internal_cooldown() );
}

void death_knight_t::trigger_death_march( const action_state_t* /* state */ )
{
  if ( legendary.death_march == timespan_t::zero() )
  {
    return;
  }

  cooldown.defile -> adjust( legendary.death_march );
  cooldown.death_and_decay -> adjust( legendary.death_march );
}

// ==========================================================================
// Death Knight Character Definition
// ==========================================================================

// death_knight_t::create_action  ===========================================

action_t* death_knight_t::create_action( const std::string& name, const std::string& options_str )
{
  // General Actions
  if ( name == "antimagic_shell"          ) return new antimagic_shell_t          ( this, options_str );
  if ( name == "auto_attack"              ) return new auto_attack_t              ( this, options_str );
  if ( name == "blood_boil"               ) return new blood_boil_t               ( this, options_str );
  if ( name == "chains_of_ice"            ) return new chains_of_ice_t            ( this, options_str );
  if ( name == "icebound_fortitude"       ) return new icebound_fortitude_t       ( this, options_str );
  if ( name == "soul_reaper"              ) return new soul_reaper_t              ( this, options_str );

  // Blood Actions
  if ( name == "blood_tap"                ) return new blood_tap_t                ( this, options_str );
  if ( name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_t      ( this, options_str );
  if ( name == "dark_command"             ) return new dark_command_t             ( this, options_str );
  if ( name == "deaths_caress"            ) return new deaths_caress_t            ( this, options_str );
  if ( name == "heart_strike"             ) return new heart_strike_t             ( this, options_str );
  if ( name == "marrowrend"               ) return new marrowrend_t               ( this, options_str );
  if ( name == "vampiric_blood"           ) return new vampiric_blood_t           ( this, options_str );

  // Frost Actions
  if ( name == "empower_rune_weapon"      ) return new empower_rune_weapon_t      ( this, options_str );
  if ( name == "frost_strike"             ) return new frost_strike_t             ( this, options_str );
  if ( name == "howling_blast"            ) return new howling_blast_t            ( this, options_str );
  if ( name == "mind_freeze"              ) return new mind_freeze_t              ( this, options_str );
  if ( name == "obliterate"               ) return new obliterate_t               ( this, options_str );
  if ( name == "pillar_of_frost"          ) return new pillar_of_frost_t          ( this, options_str );
  if ( name == "remorseless_winter"       ) return new remorseless_winter_t       ( this, options_str );
  if ( name == "horn_of_winter"           ) return new horn_of_winter_t           ( this, options_str );
  if ( name == "frostscythe"              ) return new frostscythe_t              ( this, options_str );
  if ( name == "hungering_rune_weapon"    ) return new hungering_rune_weapon_t    ( this, options_str );
  if ( name == "obliteration"             ) return new obliteration_t             ( this, options_str );
  if ( name == "glacial_advance"          ) return new glacial_advance_t          ( this, options_str );

  // Unholy Actions
  if ( name == "apocalypse"               ) return new apocalypse_t               ( this, options_str );
  if ( name == "army_of_the_dead"         ) return new army_of_the_dead_t         ( this, options_str );
  if ( name == "dark_transformation"      ) return new dark_transformation_t      ( this, options_str );
  if ( name == "death_and_decay"          ) return new death_and_decay_t          ( this, options_str );
  if ( name == "death_coil"               ) return new death_coil_t               ( this, options_str );
  if ( name == "death_strike"             ) return new death_strike_t             ( this, options_str );
  if ( name == "festering_strike"         ) return new festering_strike_t         ( this, options_str );
  if ( name == "outbreak"                 ) return new outbreak_t                 ( this, options_str );
  if ( name == "raise_dead"               ) return new raise_dead_t               ( this, options_str );
  if ( name == "scourge_strike"           ) return new scourge_strike_t           ( this, options_str );
  if ( name == "summon_gargoyle"          ) return new summon_gargoyle_t          ( this, options_str );

  // Talents
  if ( name == "blighted_rune_weapon"     ) return new blighted_rune_weapon_t     ( this, options_str );
  if ( name == "blood_mirror"             ) return new blood_mirror_t             ( this, options_str );
  if ( name == "blooddrinker"             ) return new blooddrinker_t             ( this, options_str );
  if ( name == "bonestorm"                ) return new bonestorm_t                ( this, options_str );
  if ( name == "breath_of_sindragosa"     ) return new breath_of_sindragosa_t     ( this, options_str );
  if ( name == "clawing_shadows"          ) return new clawing_shadows_t          ( this, options_str );
  if ( name == "dark_arbiter"             ) return new dark_arbiter_t             ( this, options_str );
  if ( name == "death_pact"               ) return new death_pact_t               ( this, options_str );
  if ( name == "defile"                   ) return new defile_t                   ( this, options_str );
  if ( name == "epidemic"                 ) return new epidemic_t                 ( this, options_str );
  if ( name == "mark_of_blood"            ) return new mark_of_blood_t            ( this, options_str );
  if ( name == "rune_tap"                 ) return new rune_tap_t                 ( this, options_str );
  if ( name == "soulgorge"                ) return new soulgorge_t                ( this, options_str );
  if ( name == "tombstone"                ) return new tombstone_t                ( this, options_str );

  // Artifact
  if ( name == "sindragosas_fury"         ) return new sindragosas_fury_t         ( this, options_str );

  return player_t::create_action( name, options_str );
}

// death_knight_t::create_expression ========================================

expr_t* death_knight_t::create_expression( action_t* a, const std::string& name_str ) {
  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( util::str_compare_ci( splits[ 0 ], "rune" ) && splits.size() > 1 )
  {
    if ( util::str_in_str_ci( splits[ 1 ], "time_to_" ) )
    {
      auto n_char = splits[ 1 ][ splits[ 1 ].size() - 1 ];
      auto n = n_char - '0';
      if ( n <= 0 || as<size_t>( n ) > MAX_RUNES )
      {
        sim -> errorf( "%s invalid expression '%s' for %s", name(), name_str.c_str(), a -> signature_str.c_str() );
        return player_t::create_expression( a, name_str );
      }

      return make_fn_expr( "rune_time_to_x", [ this, n ]() {
        return _runes.time_to_regen( static_cast<unsigned>( n ) );
      } );
    }
  }

  return player_t::create_expression( a, name_str );
}

// death_knight_t::create_pets ==============================================

void death_knight_t::create_pets()
{
  if ( specialization() == DEATH_KNIGHT_UNHOLY )
  {
    if ( find_action( "summon_gargoyle" ) && ! talent.dark_arbiter -> ok() )
    {
      pets.gargoyle = new pets::gargoyle_pet_t( this );
    }

    if ( find_action( "dark_arbiter" ) && talent.dark_arbiter -> ok() )
    {
      pets.dark_arbiter = new pets::valkyr_pet_t( this );
    }

    if ( find_action( "raise_dead" ) )
    {
      if ( talent.sludge_belcher -> ok() )
      {
        pets.ghoul_pet = new pets::sludge_belcher_pet_t( this );
      }
      else
      {
        pets.ghoul_pet = new pets::ghoul_pet_t( this );
      }

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

    if ( artifact.apocalypse.rank() && find_action( "apocalypse" ) )
    {
      for ( auto i = 0; i < 8; i++ )
      {
        pets.apocalypse_ghoul[ i ] = new pets::army_pet_t( this, "apocalypse_ghoul" );
      }
    }
  }

  if ( find_action( "dancing_rune_weapon" ) && specialization() == DEATH_KNIGHT_BLOOD )
  {
    pets.dancing_rune_weapon = new pets::dancing_rune_weapon_pet_t( this );
  }
}

// death_knight_t::composite_attack_haste() =================================

double death_knight_t::composite_melee_haste() const
{
  double haste = player_t::composite_melee_haste();

  //haste *= 1.0 / ( 1.0 + buffs.unholy_presence -> value() );

  haste *= 1.0 / ( 1.0 + spec.veteran_of_the_third_war -> effectN( 6 ).percent() );

  haste *= 1.0 / ( 1.0 + buffs.soul_reaper -> stack_value() );

  if ( buffs.bone_shield -> up() )
  {
    haste *= buffs.bone_shield -> value();
  }

  if ( buffs.t18_4pc_frost_haste -> up() )
  {
    haste *= buffs.t18_4pc_frost_haste -> check_value();
  }

  return haste;
}

// death_knight_t::composite_spell_haste() ==================================

double death_knight_t::composite_spell_haste() const
{
  double haste = player_t::composite_spell_haste();

  haste *= 1.0 / ( 1.0 + spec.veteran_of_the_third_war -> effectN( 6 ).percent() );

  haste *= 1.0 / ( 1.0 + buffs.soul_reaper -> stack_value() );

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

  rppm.shambler = get_rppm( "shambler", artifact.the_shambler );
}

// death_knight_t::init_base ================================================

void death_knight_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility = 0.0;

  resources.base[ RESOURCE_RUNIC_POWER ] = 100;
  resources.base[ RESOURCE_RUNIC_POWER ] += spec.veteran_of_the_third_war -> effectN( 10 ).resource( RESOURCE_RUNIC_POWER );
  resources.base[ RESOURCE_RUNIC_POWER ] += artifact.runic_tattoos.data().effectN( 2 ).base_value() / 10.0;
  resources.base[ RESOURCE_RUNE        ] = MAX_RUNES;

  base_gcd = timespan_t::from_seconds( 1.0 );

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.

  base.dodge += 0.030 + spec.veteran_of_the_third_war -> effectN( 2 ).percent();

}

// death_knight_t::init_spells ==============================================

void death_knight_t::init_spells()
{
  player_t::init_spells();

  // Specialization

  // Generic
  spec.plate_specialization       = find_specialization_spell( "Plate Specialization" );
  spec.death_knight               = find_spell( 137005 ); // "Death Knight" passive

  // Blood
  spec.blood_death_knight         = find_specialization_spell( "Blood Death Knight" );
  spec.crimson_scourge            = find_specialization_spell( "Crimson Scourge" );
  spec.veteran_of_the_third_war   = find_specialization_spell( "Veteran of the Third War" );
  spec.riposte                    = find_specialization_spell( "Riposte" );

  // Frost
  spec.frost_fever                = find_specialization_spell( "Frost Fever" );
  spec.runic_empowerment          = find_specialization_spell( "Runic Empowerment" );
  spec.rime                       = find_specialization_spell( "Rime" );
  spec.killing_machine            = find_specialization_spell( "Killing Machine" );

  // Unholy
  spec.festering_wound            = find_specialization_spell( "Festering Wound" );
  spec.deaths_advance             = find_specialization_spell( "Death's Advance" );
  spec.outbreak                   = find_specialization_spell( "Outbreak" );
  spec.runic_corruption           = find_specialization_spell( "Runic Corruption" );
  spec.sudden_doom                = find_specialization_spell( "Sudden Doom" );

  mastery.blood_shield            = find_mastery_spell( DEATH_KNIGHT_BLOOD );
  mastery.frozen_heart            = find_mastery_spell( DEATH_KNIGHT_FROST );
  mastery.dreadblade              = find_mastery_spell( DEATH_KNIGHT_UNHOLY );

  // Frost Talents
  // Tier 1
  talent.shattering_strikes    = find_talent_spell( "Shattering Strikes" );
  talent.icy_talons            = find_talent_spell( "Icy Talons" );
  talent.murderous_efficiency  = find_talent_spell( "Murderous Efficiency" );
  // Tier 2
  talent.freezing_fog          = find_talent_spell( "Freezing Fog" );
  talent.frozen_pulse          = find_talent_spell( "Frozen Pulse" );
  talent.horn_of_winter        = find_talent_spell( "Horn of Winter" );
  // Tier 3
  talent.icecap                = find_talent_spell( "Icecap" );
  talent.hungering_rune_weapon = find_talent_spell( "Hungering Rune Weapon" );
  talent.avalanche             = find_talent_spell( "Avalanche" );
  // Tier 6
  talent.frostscythe           = find_talent_spell( "Frostscythe" );
  talent.runic_attenuation     = find_talent_spell( "Runic Attenuation" );
  talent.gathering_storm       = find_talent_spell( "Gathering Storm" );
  // Tier 7
  talent.obliteration          = find_talent_spell( "Obliteration" );
  talent.breath_of_sindragosa  = find_talent_spell( "Breath of Sindragosa" );
  talent.glacial_advance       = find_talent_spell( "Glacial Advance" );

  // Unholy Talents
  // Tier 1
  talent.all_will_serve        = find_talent_spell( "All Will Serve" );
  talent.bursting_sores        = find_talent_spell( "Bursting Sores" );
  talent.ebon_fever            = find_talent_spell( "Ebon Fever" );

  // Tier 2
  talent.epidemic              = find_talent_spell( "Epidemic" );
  talent.pestilent_pustules    = find_talent_spell( "Pestilent Pustules" );
  talent.blighted_rune_weapon  = find_talent_spell( "Blighted Rune Weapon" );

  // Tier 3
  talent.unholy_frenzy         = find_talent_spell( "Unholy Frenzy" );
  talent.castigator            = find_talent_spell( "Castigator" );
  talent.clawing_shadows       = find_talent_spell( "Clawing Shadows" );

  // Tier 4
  talent.sludge_belcher        = find_talent_spell( "Sludge Belcher" );

  // Tier 6
  talent.shadow_infusion       = find_talent_spell( "Shadow Infusion" );
  talent.necrosis              = find_talent_spell( "Necrosis" );
  talent.infected_claws        = find_talent_spell( "Infected Claws" );

  // Tier 7
  talent.dark_arbiter          = find_talent_spell( "Dark Arbiter" );
  talent.defile                = find_talent_spell( "Defile" );
  talent.soul_reaper           = find_talent_spell( "Soul Reaper" );

  // Blood Talents
  // Tier 1
  talent.bloodworms            = find_talent_spell( "Bloodworms" );
  talent.heartbreaker          = find_talent_spell( "Heartbreaker" );
  talent.blooddrinker          = find_talent_spell( "Blooddrinker" );

  // Tier 2
  talent.rapid_decomposition   = find_talent_spell( "Rapid Decomposition" );
  talent.soulgorge             = find_talent_spell( "Soulgorge" );
  talent.spectral_deflection   = find_talent_spell( "Spectral Deflection" );

  // Tier 3
  talent.ossuary               = find_talent_spell( "Ossuary" );
  talent.blood_tap             = find_talent_spell( "Blood Tap" );
  talent.antimagic_barrier     = find_talent_spell( "Anti-Magic Barrier" );

  // Tier 4
  talent.mark_of_blood         = find_talent_spell( "Mark of Blood" );
  talent.red_thirst            = find_talent_spell( "Red Thirst" );
  talent.tombstone             = find_talent_spell( "Tombstone" );

  // Tier 6
  talent.will_of_the_necropolis= find_talent_spell( "Will of the Necropolis" );
  talent.rune_tap              = find_talent_spell( "Rune Tap" );
  talent.foul_bulwark          = find_talent_spell( "Foul Bulwark" );

  // Tier 7
  talent.bonestorm             = find_talent_spell( "Bonestorm" );
  talent.blood_mirror          = find_talent_spell( "Blood Mirror" );
  talent.purgatory             = find_talent_spell( "Purgatory" );

  // Artifacts

  // Frost
  artifact.crystalline_swords  = find_artifact_spell( "Crystalline Swords" );
  artifact.blades_of_frost     = find_artifact_spell( "Blades of Frost" );
  artifact.nothing_but_the_boots = find_artifact_spell( "Nothing but the Boots" );
  artifact.frozen_core         = find_artifact_spell( "Frozen Core" );
  artifact.frozen_skin         = find_artifact_spell( "Frozen Skin" );
  artifact.dead_of_winter      = find_artifact_spell( "Dead of Winter" );
  artifact.sindragosas_fury    = find_artifact_spell( "Sindragosa's Fury" );
  artifact.blast_radius        = find_artifact_spell( "Blast Radius" );
  artifact.chill_of_the_grave  = find_artifact_spell( "Chill of the Grave" );
  artifact.ice_in_your_veins   = find_artifact_spell( "Ice in Your Veins" );
  artifact.overpowered         = find_artifact_spell( "Over-Powered" );
  artifact.frozen_soul         = find_artifact_spell( "Frozen Soul" );
  artifact.cold_as_ice         = find_artifact_spell( "Cold as Ice" );
  artifact.ambidexterity       = find_artifact_spell( "Ambidexterity" );
  artifact.bad_to_the_bone     = find_artifact_spell( "Bad to the Bone" );
  artifact.hypothermia         = find_artifact_spell( "Hypothermia" );
  artifact.soulbiter           = find_artifact_spell( "Soulbiter" );

  artifact.apocalypse          = find_artifact_spell( "Apocalypse" );
  artifact.feast_of_souls      = find_artifact_spell( "Feast of Souls" );
  artifact.eternal_agony       = find_artifact_spell( "Eternal Agony" );
  artifact.the_darkest_crusade = find_artifact_spell( "The Darkest Crusade" );
  artifact.portal_to_the_underworld = find_artifact_spell( "Portal to the Underworld" );
  artifact.runic_tattoos       = find_artifact_spell( "Runic Tattoos" );
  artifact.scourge_of_worlds   = find_artifact_spell( "Scourge of Worlds" );
  artifact.rotten_touch        = find_artifact_spell( "Rotten Touch" );
  artifact.plaguebearer        = find_artifact_spell( "Plaguebearer" );
  artifact.armies_of_the_damned = find_artifact_spell( "Armies of the Damned" );
  artifact.scourge_the_unbeliever = find_artifact_spell( "Scourge the Unbeliever" );
  artifact.the_shambler        = find_artifact_spell( "The Shambler" );
  artifact.double_doom         = find_artifact_spell( "Double Doom" );
  artifact.deadliest_coil      = find_artifact_spell( "Deadliest Coil" );
  artifact.fleshsearer         = find_artifact_spell( "Fleshsearer" );
  // Generic spells
  spell.antimagic_shell        = find_class_spell( "Anti-Magic Shell" );
  spell.blood_rites            = find_spell( 163948 );
  spell.ossuary                = find_spell( 219788 );

  // Active Spells
  fallen_crusader += find_spell( 53365 ) -> effectN( 1 ).percent();
  fallen_crusader *= 1.0 + artifact.the_darkest_crusade.percent();

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

  if ( artifact.crystalline_swords.rank() )
  {
    active_spells.crystalline_swords = new crystalline_swords_t( this );
  }

  if ( artifact.the_shambler.rank() )
  {
    active_spells.necrobomb = new necrobomb_t( this );
  }

  if ( artifact.armies_of_the_damned.rank() )
  {
    active_spells.pestilence = new pestilence_t( this );
  }
}

// death_knight_t::default_apl_dps_precombat ================================

void death_knight_t::default_apl_dps_precombat( const std::string& food_name, const std::string& potion_name )
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  std::string flask_name = ( true_level >  100 ) ? "countless_armies" :
                           ( true_level >= 90  ) ? "greater_draenic_strength_flask" :
                           ( true_level >= 85  ) ? "winters_bite" :
                           ( true_level >= 80  ) ? "titanic_strength" :
                           "";

  // Flask
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    precombat -> add_action( "flask,name=" + flask_name );
  }

  // Food
  if ( sim -> allow_food && true_level >= 80 )
  {
    precombat -> add_action( "food,name=" + food_name );
  }

  if ( true_level >= 110 )
  {
    precombat -> add_action( "augmentation,name=defiled" );
  }

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Precombat potion
  if ( sim -> allow_potions && true_level >= 80 )
  {
    precombat -> add_action( "potion,name=" + potion_name );
  }
}

// death_knight_t::default_apl_blood ========================================

void death_knight_t::default_apl_blood()
{
    // TODO: mrdmnd - implement
}

// death_knight_t::default_apl_frost ========================================

void death_knight_t::default_apl_frost()
{
  action_priority_list_t* def       = get_action_priority_list( "default"   );
  action_priority_list_t* bos       = get_action_priority_list( "bos"       );
  action_priority_list_t* generic   = get_action_priority_list( "generic"   );
  action_priority_list_t* core      = get_action_priority_list( "core"      );
  action_priority_list_t* shatter   = get_action_priority_list( "shatter"   );

  std::string food_name = ( true_level > 100 ) ? "fishbrul_special" :
                          ( true_level >  90 ) ? "pickled_eel" :
                          ( true_level >= 85 ) ? "sea_mist_rice_noodles" :
                          ( true_level >= 80 ) ? "seafood_magnifique_feast" :
                          "";
  std::string potion_name = ( true_level > 100 ) ? "old_war" :
                            ( true_level >= 90 ) ? "draenic_strength" :
                            ( true_level >= 85 ) ? "mogu_power" :
                            ( true_level >= 80 ) ? "golemblood_potion" :
                            "";

  // Setup precombat APL for DPS spec
  default_apl_dps_precombat( food_name, potion_name );

  def -> add_action( "auto_attack" );
  def -> add_action( this, "Pillar of Frost" );

  // Interrupt
  def -> add_action( this, "Mind Freeze" );

  // Racials
  def -> add_action( "arcane_torrent,if=runic_power.deficit>20" );
  def -> add_action( "blood_fury,if=!talent.breath_of_sindragosa.enabled|dot.breath_of_sindragosa.ticking" );
  def -> add_action( "berserking,if=buff.pillar_of_frost.up" );

  // On-use items
  for ( const auto& item : items )
  {
    if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      def -> add_action( "use_item,slot=" + std::string( item.slot_name() ) );
    }
  }

  // In-combat potion
  if ( sim -> allow_potions && true_level >= 80 )
  {
    def -> add_action( "potion,name=" + potion_name );
  }

  // Cooldowns
  def -> add_action( this, "Sindragosa's Fury", "if=buff.pillar_of_frost.up" );
  def -> add_talent( this, "Obliteration" );
  def -> add_talent( this, "Breath of Sindragosa", "if=runic_power>=50" );

  // Choose APL
  def -> add_action( "run_action_list,name=bos,if=dot.breath_of_sindragosa.ticking" );
  def -> add_action( "call_action_list,name=shatter,if=talent.shattering_strikes.enabled" );
  def -> add_action( "call_action_list,name=generic,if=!talent.shattering_strikes.enabled" );

  // Core rotation
  core -> add_action( this, "Frost Strike", "if=buff.obliteration.up&!buff.killing_machine.react" );
  core -> add_action( this, "Remorseless Winter", "if=(spell_targets.remorseless_winter>=2|talent.gathering_storm.enabled)&!(talent.frostscythe.enabled&buff.killing_machine.react&spell_targets.frostscythe>=2)" );
  core -> add_talent( this, "Frostscythe", "if=(buff.killing_machine.react&spell_targets.frostscythe>=2)" );
  core -> add_talent( this, "Glacial Advance", "if=spell_targets.glacial_advance>=2" );
  core -> add_talent( this, "Frostscythe", "if=spell_targets.frostscythe>=3" );
  core -> add_action( this, "Obliterate", "if=buff.killing_machine.react" );
  core -> add_action( this, "Obliterate" );
  core -> add_talent( this, "Glacial Advance" );
  core -> add_action( this, "Remorseless Winter", "if=talent.frozen_pulse.enabled" );

  // Generic single target rotation

  // Refresh Icy talons if it's about to expire
  generic -> add_action( this, "Frost Strike", "if=buff.icy_talons.remains<1.5&talent.icy_talons.enabled" );

  // Howling blast disease upkeep and rimeing
  generic -> add_action( this, "Howling Blast", "target_if=!dot.frost_fever.ticking" );
  generic -> add_action( this, "Howling Blast", "if=buff.rime.react" );

  // Prevent RP waste
  generic -> add_action( this, "Frost Strike", "if=runic_power>=80" );

  // Do core rotation
  generic -> add_action( "call_action_list,name=core" );

  // Continue the generic one with Horn of Winter stuff
  generic -> add_talent( this, "Horn of Winter", "if=talent.breath_of_sindragosa.enabled&cooldown.breath_of_sindragosa.remains>15" );
  generic -> add_talent( this, "Horn of Winter", "if=!talent.breath_of_sindragosa.enabled" );

  // If nothing else to do, do Frost Strike
  generic -> add_action( this, "Frost Strike", "if=talent.breath_of_sindragosa.enabled&cooldown.breath_of_sindragosa.remains>15" );
  generic -> add_action( this, "Frost Strike", "if=!talent.breath_of_sindragosa.enabled" );

  // Misc actions, Breath of Sindragosa version
  generic -> add_action( this, "Empower Rune Weapon", "if=talent.breath_of_sindragosa.enabled&cooldown.breath_of_sindragosa.remains>15" );
  generic -> add_talent( this, "Hungering Rune Weapon", "if=talent.breath_of_sindragosa.enabled&cooldown.breath_of_sindragosa.remains>15" );

  // Misc actions, Bossless version
  generic -> add_action( this, "Empower Rune Weapon", "if=!talent.breath_of_sindragosa.enabled" );
  generic -> add_talent( this, "Hungering Rune Weapon", "if=!talent.breath_of_sindragosa.enabled" );

  // Shattering Strikes single target rotation

  // Frost Strike on 5 Razorice
  shatter -> add_action( this, "Frost Strike", "if=debuff.razorice.stack=5" );

  // Howling blast disease upkeep and rimeing
  shatter -> add_action( this, "Howling Blast", "target_if=!dot.frost_fever.ticking" );
  shatter -> add_action( this, "Howling Blast", "if=buff.rime.react" );

  // Prevent RP waste
  shatter -> add_action( this, "Frost Strike", "if=runic_power>=80" );

  // Do core rotation
  shatter -> add_action( "call_action_list,name=core" );

  // Continue the generic one with Horn of Winter stuff
  shatter -> add_talent( this, "Horn of Winter", "if=talent.breath_of_sindragosa.enabled&cooldown.breath_of_sindragosa.remains>15" );
  shatter -> add_talent( this, "Horn of Winter", "if=!talent.breath_of_sindragosa.enabled" );

  // If nothing else to do, do Frost Strike
  shatter -> add_action( this, "Frost Strike", "if=talent.breath_of_sindragosa.enabled&cooldown.breath_of_sindragosa.remains>15" );
  shatter -> add_action( this, "Frost Strike", "if=!talent.breath_of_sindragosa.enabled" );

  // Misc actions, Breath of Sindragosa version
  shatter -> add_action( this, "Empower Rune Weapon", "if=talent.breath_of_sindragosa.enabled&cooldown.breath_of_sindragosa.remains>15" );
  shatter -> add_talent( this, "Hungering Rune Weapon", "if=talent.breath_of_sindragosa.enabled&cooldown.breath_of_sindragosa.remains>15" );

  // Misc actions, Bossless version
  shatter -> add_action( this, "Empower Rune Weapon", "if=!talent.breath_of_sindragosa.enabled" );
  shatter -> add_talent( this, "Hungering Rune Weapon", "if=!talent.breath_of_sindragosa.enabled" );

  // Breath of Sindragosa rotation

  // Do keep up Frost Fevers, even in BoS
  bos -> add_action( this, "Howling Blast", "target_if=!dot.frost_fever.ticking" );

  // Do core rotation
  bos -> add_action( "call_action_list,name=core" );

  bos -> add_talent( this, "Horn of Winter" );
  bos -> add_action( this, "Empower Rune Weapon", "if=runic_power<=70" );
  bos -> add_talent( this, "Hungering Rune Weapon" );

  // Low priority howling blasts, if they are free
  bos -> add_action( this, "Howling Blast", "if=buff.rime.react" );
}

void death_knight_t::default_apl_unholy()
{
  action_priority_list_t* precombat = get_action_priority_list("precombat");
  action_priority_list_t* def = get_action_priority_list("default");
  action_priority_list_t* valkyr = get_action_priority_list("valkyr");
  action_priority_list_t* generic = get_action_priority_list("generic");
  action_priority_list_t* aoe = get_action_priority_list("aoe");
  action_priority_list_t* standard = get_action_priority_list("standard");
  action_priority_list_t* castigator = get_action_priority_list("castigator");
  action_priority_list_t* instructors = get_action_priority_list("instructors");

  std::string food_name = (true_level > 100) ? "the_hungry_magister" :
    (true_level >  90) ? "buttered_sturgeon" :
    (true_level >= 85) ? "sea_mist_rice_noodles" :
    (true_level >= 80) ? "seafood_magnifique_feast" :
    "";
  std::string potion_name = (true_level > 100) ? "old_war" :
    (true_level >= 90) ? "draenic_strength" :
    (true_level >= 85) ? "mogu_power" :
    (true_level >= 80) ? "golemblood_potion" :
    "";

  // Setup precombat APL for DPS spec
  default_apl_dps_precombat(food_name, potion_name);

  precombat->add_action(this, "Raise Dead");
  precombat->add_action(this, "Army of the Dead");

  def->add_action("auto_attack");
  def->add_action(this, "Mind Freeze");

  // Racials
  def->add_action("arcane_torrent,if=runic_power.deficit>20");
  def->add_action("blood_fury");
  def->add_action("berserking");

  // On-use items
  for (const auto& item : items)
  {
    if (item.has_special_effect(SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE))
    {
      def->add_action("use_item,slot=" + std::string(item.slot_name()));
    }
  }

  // In-combat potion
  if (sim->allow_potions && true_level >= 80)
  {
    def->add_action("potion,name=" + potion_name + ",if=buff.unholy_strength.react");
  }

  // Generic things that should be always done
  def->add_action(this, "Outbreak", "target_if=!dot.virulent_plague.ticking");
  def->add_action(this, "Dark Transformation", "if=equipped.137075&cooldown.dark_arbiter.remains>165");
  def->add_action(this, "Dark Transformation", "if=equipped.137075&!talent.shadow_infusion.enabled&cooldown.dark_arbiter.remains>55");
  def->add_action(this, "Dark Transformation", "if=equipped.137075&talent.shadow_infusion.enabled&cooldown.dark_arbiter.remains>35");
  def->add_action(this, "Dark Transformation", "if=equipped.137075&target.time_to_die<cooldown.dark_arbiter.remains-8");
  def->add_action(this, "Dark Transformation", "if=equipped.137075&cooldown.summon_gargoyle.remains>160");
  def->add_action(this, "Dark Transformation", "if=equipped.137075&!talent.shadow_infusion.enabled&cooldown.summon_gargoyle.remains>55");
  def->add_action(this, "Dark Transformation", "if=equipped.137075&talent.shadow_infusion.enabled&cooldown.summon_gargoyle.remains>35");
  def->add_action(this, "Dark Transformation", "if=equipped.137075&target.time_to_die<cooldown.summon_gargoyle.remains-8");
  def->add_action(this, "Dark Transformation", "if=!equipped.137075&rune<=3");
  def->add_talent(this, "Blighted Rune Weapon", "if=rune<=3");

  // Pick an APL to run
  def->add_action("run_action_list,name=valkyr,if=talent.dark_arbiter.enabled&pet.valkyr_battlemaiden.active");
  def->add_action("call_action_list,name=generic");

  // Default generic target APL

  generic->add_talent(this, "Dark Arbiter", "if=!equipped.137075&runic_power.deficit<30");
  generic->add_talent(this, "Dark Arbiter", "if=equipped.137075&runic_power.deficit<30&cooldown.dark_transformation.remains<2");
  generic->add_action(this, "Summon Gargoyle", "if=!equipped.137075,if=rune<=3");
  generic->add_action(this, "Summon Gargoyle", "if=equipped.137075&cooldown.dark_transformation.remains<10&rune<=3");

  // Apocalypso
  generic->add_talent(this, "Soul Reaper", "if=debuff.festering_wound.stack>=7&cooldown.apocalypse.remains<2");
  generic->add_action(this, "Apocalypse", "if=debuff.festering_wound.stack>=7");

  // Death coilage
  generic->add_action(this, "Death Coil", "if=runic_power.deficit<30");
  generic->add_action(this, "Death Coil", "if=!talent.dark_arbiter.enabled&buff.sudden_doom.up&!buff.necrosis.up&rune<=3");
  generic->add_action(this, "Death Coil", "if=talent.dark_arbiter.enabled&buff.sudden_doom.up&cooldown.dark_arbiter.remains>5&rune<=3");

  // FW stacking
  generic->add_action(this, "Festering Strike", "if=debuff.festering_wound.stack<7&cooldown.apocalypse.remains<5");

  //Wait if apocalypse not casted
  generic->add_action("wait,sec=cooldown.apocalypse.remains,if=cooldown.apocalypse.remains<=1&cooldown.apocalypse.remains");

  // Soul reapering
  generic->add_talent(this, "Soul Reaper", "if=debuff.festering_wound.stack>=3");
  generic->add_action(this, "Festering Strike", "if=debuff.soul_reaper.up&!debuff.festering_wound.up");
  generic->add_action(this, "Scourge Strike", "if=debuff.soul_reaper.up&debuff.festering_wound.stack>=1");
  generic->add_talent(this, "Clawing Shadows", "if=debuff.soul_reaper.up&debuff.festering_wound.stack>=1");

  // Misc things
  generic->add_talent(this, "Defile");
  generic->add_action("call_action_list,name=aoe,if=active_enemies>=2");
  generic->add_action("call_action_list,name=instructors,if=equipped.132448");
  generic->add_action("call_action_list,name=standard,if=!talent.castigator.enabled&!equipped.132448");
  generic->add_action("call_action_list,name=castigator,if=talent.castigator.enabled&!equipped.132448");


  // Standard single target base rotation
  standard->add_action(this, "Festering Strike", "if=debuff.festering_wound.stack<=4&runic_power.deficit>23");
  standard->add_action(this, "Death Coil", "if=!buff.necrosis.up&talent.necrosis.enabled&rune<=3");
  standard->add_action(this, "Scourge Strike", "if=buff.necrosis.react&debuff.festering_wound.stack>=1&runic_power.deficit>15");
  standard->add_talent(this, "Clawing Shadows", "if=buff.necrosis.react&debuff.festering_wound.stack>=1&runic_power.deficit>15");
  standard->add_action(this, "Scourge Strike", "if=buff.unholy_strength.react&debuff.festering_wound.stack>=1&runic_power.deficit>15");
  standard->add_talent(this, "Clawing Shadows", "if=buff.unholy_strength.react&debuff.festering_wound.stack>=1&runic_power.deficit>15");
  standard->add_action(this, "Scourge Strike", "if=rune>=2&debuff.festering_wound.stack>=1&runic_power.deficit>15");
  standard->add_talent(this, "Clawing Shadows", "if=rune>=2&debuff.festering_wound.stack>=1&runic_power.deficit>15");
  standard->add_action(this, "Death Coil", "if=talent.shadow_infusion.enabled&talent.dark_arbiter.enabled&!buff.dark_transformation.up&cooldown.dark_arbiter.remains>15");
  standard->add_action(this, "Death Coil", "if=talent.shadow_infusion.enabled&!talent.dark_arbiter.enabled&!buff.dark_transformation.up");
  standard->add_action(this, "Death Coil", "if=talent.dark_arbiter.enabled&cooldown.dark_arbiter.remains>15");
  standard->add_action(this, "Death Coil", "if=!talent.shadow_infusion.enabled&!talent.dark_arbiter.enabled");

  // Standard single target castigator rotation
  castigator->add_action(this, "Festering Strike", "if=debuff.festering_wound.stack<=4&runic_power.deficit>23");
  castigator->add_action(this, "Death Coil", "if=!buff.necrosis.up&talent.necrosis.enabled&rune<=3");
  castigator->add_action(this, "Scourge Strike", "if=buff.necrosis.react&debuff.festering_wound.stack>=3&runic_power.deficit>23");
  castigator->add_action(this, "Scourge Strike", "if=buff.unholy_strength.react&debuff.festering_wound.stack>=3&runic_power.deficit>23");
  castigator->add_action(this, "Scourge Strike", "if=rune>=2&debuff.festering_wound.stack>=3&runic_power.deficit>23");
  castigator->add_action(this, "Death Coil", "if=talent.shadow_infusion.enabled&talent.dark_arbiter.enabled&!buff.dark_transformation.up&cooldown.dark_arbiter.remains>15");
  castigator->add_action(this, "Death Coil", "if=talent.shadow_infusion.enabled&!talent.dark_arbiter.enabled&!buff.dark_transformation.up");
  castigator->add_action(this, "Death Coil", "if=talent.dark_arbiter.enabled&cooldown.dark_arbiter.remains>15");
  castigator->add_action(this, "Death Coil", "if=!talent.shadow_infusion.enabled&!talent.dark_arbiter.enabled");

  // Standard single target instructors rotation
  instructors->add_action(this, "Festering Strike", "if=debuff.festering_wound.stack<=4&runic_power.deficit>23");
  instructors->add_action(this, "Death Coil", "if=!buff.necrosis.up&talent.necrosis.enabled&rune<=3");
  instructors->add_action(this, "Scourge Strike", "if=buff.necrosis.react&debuff.festering_wound.stack>=5&runic_power.deficit>29");
  instructors->add_talent(this, "Clawing Shadows", "if=buff.necrosis.react&debuff.festering_wound.stack>=5&runic_power.deficit>29");
  instructors->add_action(this, "Scourge Strike", "if=buff.unholy_strength.react&debuff.festering_wound.stack>=5&runic_power.deficit>29");
  instructors->add_talent(this, "Clawing Shadows", "if=buff.unholy_strength.react&debuff.festering_wound.stack>=5&runic_power.deficit>29");
  instructors->add_action(this, "Scourge Strike", "if=rune>=2&debuff.festering_wound.stack>=5&runic_power.deficit>29");
  instructors->add_talent(this, "Clawing Shadows", "if=rune>=2&debuff.festering_wound.stack>=5&runic_power.deficit>29");
  instructors->add_action(this, "Death Coil", "if=talent.shadow_infusion.enabled&talent.dark_arbiter.enabled&!buff.dark_transformation.up&cooldown.dark_arbiter.remains>15");
  instructors->add_action(this, "Death Coil", "if=talent.shadow_infusion.enabled&!talent.dark_arbiter.enabled&!buff.dark_transformation.up");
  instructors->add_action(this, "Death Coil", "if=talent.dark_arbiter.enabled&cooldown.dark_arbiter.remains>15");
  instructors->add_action(this, "Death Coil", "if=!talent.shadow_infusion.enabled&!talent.dark_arbiter.enabled");

  // Generic AOE actions to be done
  aoe->add_action(this, "Death and Decay", "if=spell_targets.death_and_decay>=2");
  aoe->add_talent(this, "Epidemic", "if=spell_targets.epidemic>4");
  aoe->add_action(this, "Scourge Strike", "if=spell_targets.scourge_strike>=2&(dot.death_and_decay.ticking|dot.defile.ticking)");
  aoe->add_talent(this, "Clawing Shadows", "if=spell_targets.clawing_shadows>=2&(dot.death_and_decay.ticking|dot.defile.ticking)");
  aoe->add_talent(this, "Epidemic", "if=spell_targets.epidemic>2");

  // Valkyr APL uses many a runic power
  valkyr->add_action(this, "Death Coil");

  // Apocalypso
  valkyr->add_action(this, "Apocalypse", "if=debuff.festering_wound.stack=8");

  // FW stacking
  valkyr->add_action(this, "Festering Strike", "if=debuff.festering_wound.stack<8&cooldown.apocalypse.remains<5");

  // Misc AOE things
  valkyr->add_action("call_action_list,name=aoe,if=active_enemies>=2");

  // Single target base rotation when Valkyr is around
  valkyr->add_action(this, "Festering Strike", "if=debuff.festering_wound.stack<=3");
  valkyr->add_action(this, "Scourge Strike", "if=debuff.festering_wound.up");
  valkyr->add_talent(this, "Clawing Shadows", "if=debuff.festering_wound.up");
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
    scales_with[ STAT_WEAPON_OFFHAND_DPS   ] = true;

  if ( specialization() == DEATH_KNIGHT_BLOOD )
    scales_with[ STAT_BONUS_ARMOR ] = true;

  scales_with[ STAT_AGILITY ] = false;
}

// death_knight_t::init_buffs ===============================================
void death_knight_t::create_buffs()
{
  player_t::create_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buffs.army_of_the_dead    = buff_creator_t( this, "army_of_the_dead", find_class_spell( "Army of the Dead" ) )
                              .cd( timespan_t::zero() );
  buffs.blood_shield        = new blood_shield_buff_t( this );
  buffs.rune_tap            = buff_creator_t( this, "rune_tap", talent.rune_tap )
                              .cd( timespan_t::zero() );

  buffs.antimagic_shell     = new antimagic_shell_buff_t( this );

  buffs.bone_shield         = haste_buff_creator_t( this, "bone_shield", find_spell( 195181 ) )
                              .default_value( 1.0 / ( 1.0 + find_spell( 195181 ) -> effectN( 4 ).percent() ) )
                              .stack_change_callback( talent.foul_bulwark -> ok() ? [ this ]( buff_t*, int old_stacks, int new_stacks ) {
                                double old = old_stacks * talent.foul_bulwark -> effectN( 1 ).percent();
                                double new_ = new_stacks * talent.foul_bulwark -> effectN( 1 ).percent();
                                resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + old;
                                resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + new_;
                                recalculate_resource_max( RESOURCE_HEALTH );
                              } : buff_stack_change_callback_t() );
  buffs.crimson_scourge     = buff_creator_t( this, "crimson_scourge" )
                              .spell( find_spell( 81141 ) )
                              .trigger_spell( spec.crimson_scourge );
  buffs.dancing_rune_weapon = buff_creator_t( this, "dancing_rune_weapon", find_spell( 81256 ) )
                              .cd( timespan_t::zero() )
                              .add_invalidate( CACHE_PARRY );
  buffs.dark_transformation = buff_creator_t( this, "dark_transformation", find_class_spell( "Dark Transformation" ) )
    .duration( find_specialization_spell( "Dark Transformation" ) -> duration() + artifact.eternal_agony.time_value() )
    .cd( timespan_t::zero() ); // Handled by the action

  buffs.gathering_storm     = buff_creator_t( this, "gathering_storm", find_spell( 211805 ) )
                              .trigger_spell( talent.gathering_storm )
                              .default_value( find_spell( 211805 ) -> effectN( 1 ).percent() );
  buffs.icebound_fortitude  = buff_creator_t( this, "icebound_fortitude", find_class_spell( "Icebound Fortitude" ) )
                              .duration( find_class_spell( "Icebound Fortitude" ) -> duration() )
                              .cd( timespan_t::zero() );
  buffs.icy_talons          = haste_buff_creator_t( this, "icy_talons", find_spell( 194879 ) )
                              .add_invalidate( CACHE_ATTACK_SPEED )
                              .default_value( find_spell( 194879 ) -> effectN( 1 ).percent() )
                              .trigger_spell( talent.icy_talons );
  buffs.killing_machine     = buff_creator_t( this, "killing_machine", spec.killing_machine -> effectN( 1 ).trigger() )
                              .trigger_spell( spec.killing_machine )
                              .default_value( find_spell( 51124 ) -> effectN( 1 ).percent() );
  buffs.obliteration        = buff_creator_t( this, "obliteration", talent.obliteration )
                              .cd( timespan_t::zero() ); // Handled by action
  buffs.pillar_of_frost = buff_creator_t(this, "pillar_of_frost", find_class_spell("Pillar of Frost"))
                              .cd(timespan_t::zero())
                              .default_value(find_class_spell("Pillar of Frost")->effectN(1).percent())
                              .add_invalidate(CACHE_STRENGTH)
                              .add_invalidate(artifact.frozen_core.rank() ? CACHE_PLAYER_DAMAGE_MULTIPLIER : CACHE_NONE)
                              .add_invalidate(legendary.toravons ? CACHE_PLAYER_DAMAGE_MULTIPLIER : CACHE_NONE);
  buffs.rime                = buff_creator_t( this, "rime", spec.rime -> effectN( 1 ).trigger() )
                              .trigger_spell( spec.rime )
                              .chance( spec.rime -> proc_chance() + sets.set( DEATH_KNIGHT_FROST, T19, B2 ) -> effectN( 1 ).percent() );
  buffs.riposte             = stat_buff_creator_t( this, "riposte", spec.riposte -> effectN( 1 ).trigger() )
                              .cd( spec.riposte -> internal_cooldown() )
                              .chance( spec.riposte -> proc_chance() )
                              .add_stat( STAT_CRIT_RATING, 0 );
  //buffs.runic_corruption    = buff_creator_t( this, "runic_corruption", find_spell( 51460 ) )
  //                            .chance( talent.runic_corruption -> proc_chance() );
  buffs.runic_corruption    = new runic_corruption_buff_t( this );
  buffs.sudden_doom         = buff_creator_t( this, "sudden_doom" )
                              .spell( spec.sudden_doom -> effectN( 1 ).trigger() )
                              .rppm_mod( 1.0 + artifact.double_doom.data().effectN( 2 ).percent() )
                              .rppm_scale( RPPM_ATTACK_SPEED ) // 2016-08-08: Hotfixed, not in spell data
                              .max_stack( specialization() == DEATH_KNIGHT_UNHOLY
                                  ? ( spec.sudden_doom -> effectN( 1 ).trigger() -> initial_stacks() +
                                      artifact.double_doom.data().effectN( 1 ).base_value() )
                                  : 1 )
                              .trigger_spell( spec.sudden_doom );

  buffs.vampiric_blood      = new vampiric_blood_buff_t( this );
  buffs.will_of_the_necropolis = buff_creator_t( this, "will_of_the_necropolis", find_spell( 157335 ) )
                                 .cd( find_spell( 157335 ) -> duration() );

  runeforge.rune_of_the_fallen_crusader = buff_creator_t( this, "unholy_strength", find_spell( 53365 ) )
                                          .add_invalidate( CACHE_STRENGTH );

  runeforge.rune_of_the_stoneskin_gargoyle = buff_creator_t( this, "stoneskin_gargoyle", find_spell( 62157 ) )
                                             .add_invalidate( CACHE_ARMOR )
                                             .add_invalidate( CACHE_STAMINA )
                                             .chance( 0 );

  buffs.blighted_rune_weapon = buff_creator_t( this, "blighted_rune_weapon", talent.blighted_rune_weapon )
                               .cd( timespan_t::zero() ); // Handled by action
  buffs.unholy_frenzy = haste_buff_creator_t( this, "unholy_frenzy", find_spell( 207290 ) )
    .add_invalidate( CACHE_ATTACK_SPEED )
    .trigger_spell( talent.unholy_frenzy )
    .default_value( 1.0 / ( 1.0 + find_spell( 207290 ) -> effectN( 1 ).percent() ) )
    .refresh_behavior( BUFF_REFRESH_CUSTOM )
    // Unholy Frenzy duration is hard capped at 10 seconds
    .refresh_duration_callback( []( const buff_t* b, const timespan_t& duration ) {
      timespan_t total_duration = b -> remains() + duration;
      if ( total_duration > timespan_t::from_seconds( 10 ) )
      {
        total_duration = timespan_t::from_seconds( 10 );
      }
      return total_duration;
    } );
  buffs.necrosis = buff_creator_t( this, "necrosis", find_spell( 216974 ) )
    .default_value( find_spell( 216974 ) -> effectN( 1 ).percent() )
    .trigger_spell( talent.necrosis );

  buffs.defile = stat_buff_creator_t( this, "defile", find_spell( 218100 ) )
    .trigger_spell( talent.defile );

  buffs.soul_reaper = haste_buff_creator_t( this, "soul_reaper_haste", talent.soul_reaper -> effectN( 2 ).trigger() )
    .default_value( talent.soul_reaper -> effectN( 2 ).trigger() -> effectN( 1 ).percent() )
    .trigger_spell( talent.soul_reaper );
  buffs.t18_4pc_frost_haste = haste_buff_creator_t( this, "obliteration_t18", find_spell( 187893 ) )
    .add_invalidate( CACHE_ATTACK_HASTE )
    .default_value( 1.0 / ( 1.0 + find_spell( 187893 ) -> effectN( 1 ).percent() ) )
    .trigger_spell( sets.set( DEATH_KNIGHT_FROST, T18, B2 ) );
  buffs.t18_4pc_frost_crit = buff_creator_t( this, "frozen_wake", find_spell( 187894 ) )
    .default_value( find_spell( 187894 ) -> effectN( 1 ).percent() )
    .trigger_spell( sets.set( DEATH_KNIGHT_FROST, T18, B2 ) );
  buffs.t18_4pc_unholy = haste_buff_creator_t( this, "crazed_monstrosity", find_spell( 187981 ) )
    .chance( sets.has_set_bonus( DEATH_KNIGHT_UNHOLY, T18, B4 ) )
    .add_invalidate( CACHE_ATTACK_SPEED )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.soulgorge = buff_creator_t( this, "soulgorge", find_spell( 213003 ) )
    .affects_regen( true );
  buffs.antimagic_barrier = new antimagic_barrier_buff_t( this );
  buffs.tombstone = absorb_buff_creator_t( this, "tombstone", talent.tombstone )
    .cd( timespan_t::zero() ); // Handled by the action
  buffs.t19oh_8pc = stat_buff_creator_t( this, "deathlords_might" )
    .spell( sets.set( specialization(), T19OH, B8 ) -> effectN( 1 ).trigger() )
    .trigger_spell( sets.set( specialization(), T19OH, B8 ) );

  buffs.frozen_pulse = buff_creator_t( this, "frozen_pulse", talent.frozen_pulse );
}

// death_knight_t::init_gains ===============================================

void death_knight_t::init_gains()
{
  player_t::init_gains();

  gains.antimagic_shell                  = get_gain( "antimagic_shell"            );
  gains.horn_of_winter                   = get_gain( "Horn of Winter"             );
  gains.hungering_rune_weapon            = get_gain( "Hungering Rune Weapon"      );
  gains.frost_fever                      = get_gain( "Frost Fever"                );
  gains.festering_wound                  = get_gain( "Festering Wound"            );
  gains.murderous_efficiency             = get_gain( "Murderous Efficiency"       );
  gains.power_refund                     = get_gain( "power_refund"               );
  gains.rune                             = get_gain( "Rune Regeneration"          );
  gains.runic_empowerment                = get_gain( "Runic Empowerment"          );
  gains.empower_rune_weapon              = get_gain( "Empower Rune Weapon"        );
  gains.blood_tap                        = get_gain( "blood_tap"                  );
  gains.rc                               = get_gain( "runic_corruption_all"       );
  gains.runic_attenuation                = get_gain( "Runic Attenuation"          );
  // gains.blood_tap_blood                  = get_gain( "blood_tap_blood"            );
  //gains.blood_tap_blood          -> type = ( resource_e ) RESOURCE_RUNE_BLOOD   ;
  gains.pestilent_pustules               = get_gain( "Pestilent Pustules"         );
  gains.tombstone                        = get_gain( "Tombstone"                  );
  gains.overpowered                      = get_gain( "Over-Powered"               );
  gains.t18_2pc_blood                    = get_gain( "Tier18 Blood 2PC"           );
  gains.scourge_the_unbeliever           = get_gain( "Scourge the Unbeliever"     );
  gains.draugr_girdle_everlasting_king   = get_gain( "Draugr, Girdle of the Everlasting King" );
  gains.uvanimor_the_unbeautiful         = get_gain( "Uvanimor, the Unbeautiful"  );
  gains.koltiras_newfound_will           = get_gain( "Koltira's Newfound Will"    );
  gains.t19_4pc_frost                    = get_gain( "Tier19 Frost 4PC"           );
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

  procs.t19_2pc_unholy           = get_proc( "Tier19 Unholy 2PC" );
}

// death_knight_t::init_resources ===========================================

void death_knight_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_RUNIC_POWER ] = 0;
  resources.current[ RESOURCE_RUNE        ] = resources.max[ RESOURCE_RUNE ];
}

// death_knight_t::init_absorb_priority =====================================

double death_knight_t::bone_shield_handler( const action_state_t* state ) const
{
  if ( ! buffs.bone_shield -> up() )
  {
    return 0;
  }

  if ( ! cooldown.bone_shield_icd -> up() )
  {
    return 0;
  }

  double absorbed = 0;
  double absorb_pct = buffs.bone_shield -> data().effectN( 5 ).percent();
  int n_stacks = 1;

  if ( talent.spectral_deflection -> ok() && state -> result_raw >
       resources.max[ RESOURCE_HEALTH ] * talent.spectral_deflection -> effectN( 1 ).percent() &&
       buffs.bone_shield -> check() > 1 )
  {
    absorb_pct *= 2;
    n_stacks *= 2;
  }

  absorbed = absorb_pct * state -> result_amount;

  buffs.bone_shield -> decrement( n_stacks );
  cooldown.bone_shield_icd -> start();

  return absorbed;
}

void death_knight_t::init_absorb_priority()
{
  player_t::init_absorb_priority();

  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    instant_absorb_list[ 195181 ] = new instant_absorb_t( this, find_spell( 195181 ), "bone_shield",
      std::bind( &death_knight_t::bone_shield_handler, this, std::placeholders::_1 ) );

    // TODO: What is the absorb ordering for blood dks?
    absorb_priority.push_back( 206977 ); // Blood Mirror
    absorb_priority.push_back( 195181 ); // Bone Shield (NYI)
    //absorb_priority.push_back( 77535  ); // Blood Shield
    //absorb_priority.push_back( 219809 ); // Tombstone
  }
}

// death_knight_t::reset ====================================================

void death_knight_t::reset() {
  player_t::reset();
  runic_power_decay_rate = 1; // 1 RP per second decay
  antimagic_shell_absorbed = 0.0;
  pestilent_pustules = 0;
  crystalline_swords = 0;
  _runes.reset();
}

// death_knight_t::assess_heal ==============================================

void death_knight_t::assess_heal( school_e school, dmg_e t, action_state_t* s )
{
  if ( buffs.vampiric_blood -> up() )
    s -> result_total *= 1.0 + buffs.vampiric_blood -> data().effectN( 1 ).percent();

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
  //if ( buffs.blood_presence -> check() )
  //  state -> result_amount *= 1.0 + buffs.blood_presence -> data().effectN( 6 ).percent();

  if ( buffs.rune_tap -> up() )
    state -> result_amount *= 1.0 + buffs.rune_tap -> data().effectN( 1 ).percent();

  if ( buffs.icebound_fortitude -> up() )
    state -> result_amount *= 1.0 + buffs.icebound_fortitude -> data().effectN( 3 ).percent();

  if ( buffs.army_of_the_dead -> check() )
    state -> result_amount *= 1.0 - buffs.army_of_the_dead -> value();

  if ( talent.defile -> ok() )
  {
    death_knight_td_t* tdata = get_target_data( state -> action -> player );
    if ( tdata -> dot.defile -> is_ticking() )
      state -> result_amount *= 1.0 - talent.defile -> effectN( 4 ).percent();
  }

  player_t::target_mitigation( school, type, state );
}

// death_knight_t::composite_armor_multiplier ===============================

double death_knight_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  a *= 1.0 + spec.veteran_of_the_third_war -> effectN( 3 ).percent();

  if ( runeforge.rune_of_the_stoneskin_gargoyle -> check() )
    a *= 1.0 + runeforge.rune_of_the_stoneskin_gargoyle -> data().effectN( 1 ).percent();

  a *= 1.0 + artifact.frozen_skin.percent();

  return a;
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
    m *= 1.0 + spec.veteran_of_the_third_war -> effectN( 5 ).percent();

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
  double leech = player_t::composite_leech();

  // TODO: Additive or multiplicative?
/*  if ( buffs.lichborne -> up() )
  {
    leech += buffs.lichborne -> data().effectN( 1 ).percent();
  }*/

  return leech;
}

// death_knight_t::composite_melee_expertise ===============================

double death_knight_t::composite_melee_expertise( const weapon_t* ) const
{
  double expertise = player_t::composite_melee_expertise( nullptr );

  expertise += spec.veteran_of_the_third_war -> effectN( 7 ).percent();

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

  if ( dbc::is_school( school, SCHOOL_SHADOW ) )
  {
    if ( mastery.dreadblade -> ok() )
    {
      m *= 1.0 + cache.mastery_value();
    }

    m *= 1.0 + artifact.feast_of_souls.percent();
  }

  if ( dbc::is_school( school, SCHOOL_FROST ) )
  {
    if ( mastery.frozen_heart -> ok() )
      m *= 1.0 + cache.mastery_value();

    if ( buffs.pillar_of_frost -> up() )
    {
      m *= 1.0 + artifact.frozen_core.percent();
      m *= 1.0 + legendary.toravons;
    }

    m *= 1.0 + artifact.cold_as_ice.percent();
  }

  m *= 1.0 + artifact.soulbiter.percent();
  m *= 1.0 + artifact.fleshsearer.percent();

  if ( buffs.t18_4pc_unholy -> up() )
  {
    m *= 1.0 + buffs.t18_4pc_unholy -> data().effectN( 2 ).percent();
  }



  return m;
}

double death_knight_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  death_knight_td_t* td = get_target_data( target );
  m *= 1.0 + td -> debuff.death -> stack_value();

  if ( dbc::is_school( school, SCHOOL_FROST ) )
  {
    double debuff = td -> debuff.razorice -> data().effectN( 1 ).percent();
    m *= 1.0 + td -> debuff.razorice -> check() * debuff;
  }

  return m;
}

// death_knight_t::composite_player_critical_damage_multiplier ===================

double death_knight_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_critical_damage_multiplier( s );

  m *= 1.0 + buffs.t18_4pc_frost_crit -> stack_value();

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

  m *= 1.0 + mastery.blood_shield -> effectN( 3 ).mastery_value() * composite_mastery();

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

  if ( buffs.unholy_frenzy -> up() )
  {
    haste *= buffs.unholy_frenzy -> check_value();
  }

  haste *= 1.0 / ( 1.0 + artifact.blades_of_frost.percent() );

  if ( buffs.t18_4pc_unholy -> up() )
  {
    haste *= 1.0 / ( 1.0 + buffs.t18_4pc_unholy -> data().effectN( 1 ).percent() );
  }

  return haste;
}

// death_knight_t::composite_tank_crit ======================================

double death_knight_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  c += spec.veteran_of_the_third_war -> effectN( 2 ).percent();

  return c;
}

// death_knight_t::passive_movement_modifier====================================

double death_knight_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( spec.deaths_advance -> ok() )
    ms += spec.deaths_advance -> effectN( 1 ).percent();

  /*
  if ( buffs.unholy_presence -> up() )
    ms += buffs.unholy_presence -> data().effectN( 2 ).percent();

  */
  return ms;
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
      player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      if ( specialization() == DEATH_KNIGHT_BLOOD )
        player_t::invalidate_cache( CACHE_ATTACK_POWER );
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

// death_knight_t::create_options ================================================

void death_knight_t::create_options()
{
  player_t::create_options();

  add_option( opt_float( "fallen_crusader_str", fallen_crusader ) );
  add_option( opt_float( "fallen_crusader_rppm", fallen_crusader_rppm ) );
  add_option( opt_float( "aotd_proc_chance", aotd_proc_chance ) );

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
    rps *= 2.0;
  }

  if ( buffs.soulgorge -> check() )
  {
    // Note, don't use stack_value() here, since the correct (fractional) value is triggered in the
    // buff when soulgorge consumes blood plagues.
    rps *= 1.0 + buffs.soulgorge -> check_value();
  }

  return rps;
}

inline double death_knight_t::rune_regen_coefficient() const
{
  auto coeff = cache.attack_haste();
  // Runic corruption doubles rune regeneration speed
  if ( buffs.runic_corruption -> check() )
  {
    coeff *= .5;
  }

  if ( buffs.soulgorge -> check() )
  {
    // Note, don't use stack_value() here, since the correct (fractional) value is triggered in the
    // buff when soulgorge consumes blood plagues.
    coeff *= 1.0 / ( 1.0 + buffs.soulgorge -> check_value() );
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
    active_spells.blood_plague -> target = state -> target;
    active_spells.blood_plague -> execute();
  }

  if ( diseases & DISEASE_FROST_FEVER )
  {
    active_spells.frost_fever -> target = state -> target;
    active_spells.frost_fever -> execute();
  }

  if ( diseases & DISEASE_VIRULENT_PLAGUE )
  {
    active_spells.virulent_plague -> target = state -> target;
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

// death_knight_t::runes_cooldown_min =======================================

double death_knight_t::runes_cooldown_min( ) const
{
  double min_time = std::numeric_limits<double>::max();

  for ( size_t rune_idx = 0; rune_idx < MAX_RUNES; ++rune_idx )
  {
    const rune_t& r = _runes.slot[ rune_idx ];

    if ( r.is_ready() ) {
      return 0.0;
    }

    double time = runes_cooldown_time( r );
    if ( time < min_time )
    {
      min_time = time;
    }
  }

  return min_time;
}

// death_knight_t::runes_cooldown_max =======================================

double death_knight_t::runes_cooldown_max( ) const
{
  double max_time = 0;

  for ( size_t rune_idx = 0; rune_idx < MAX_RUNES; ++rune_idx )
  {
    const rune_t& r = _runes.slot[ rune_idx ];

    double time = runes_cooldown_time( r );
    if ( time > max_time )
    {
      max_time = time;
    }
  }

  return max_time;
}

// death_knight_t::runes_cooldown_time ======================================

double death_knight_t::runes_cooldown_time( const rune_t& rune ) const
{
  return rune.is_ready() ? 0.0 : ( 1.0 - rune.fill_level() ) / runes_per_second();
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
    os.format( "<td class=\"right\">%.3f</td>", p._runes.rune_waste.min() );
    os.format( "<td class=\"right\">%.3f</td>", p._runes.rune_waste.percentile( .05 ) );
    os.format( "<td class=\"right\">%.3f / %.3f</td>", p._runes.rune_waste.mean(), p._runes.rune_waste.percentile( .5 ) );
    os.format( "<td class=\"right\">%.3f</td>", p._runes.rune_waste.percentile( .95 ) );
    os.format( "<td class=\"right\">%.3f</td>", p._runes.rune_waste.max() );
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

    os.format( "<td class=\"right\">%.3f</td>", p._runes.cumulative_waste.min() );
    os.format( "<td class=\"right\">%.3f</td>", p._runes.cumulative_waste.percentile( .05 ) );
    os.format( "<td class=\"right\">%.3f / %.3f</td>", p._runes.cumulative_waste.mean(), p._runes.cumulative_waste.percentile( .5 ) );
    os.format( "<td class=\"right\">%.3f</td>", p._runes.cumulative_waste.percentile( .95 ) );
    os.format( "<td class=\"right\">%.3f</td>", p._runes.cumulative_waste.max() );
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

struct reapers_harvest_frost_t : public scoped_action_callback_t<obliterate_strike_t>
{
  reapers_harvest_frost_t( const std::string& n ) : super( DEATH_KNIGHT, n )
  { }

  void manipulate( obliterate_strike_t* action, const special_effect_t& e ) override
  {
    action_t* parent = action -> player -> find_action( "obliterate" );
    double coeff = e.driver() -> effectN( 1 ).average( e.item ) / 100.0;
    std::string n = "frozen_obliteration";
    if ( action -> weapon == &( action -> player -> off_hand_weapon ) )
    {
      n += "_oh";
    }

    action -> fo = new frozen_obliteration_t( action -> p(), n, coeff );
    if ( parent )
    {
      parent -> add_child( action -> fo );
    }
  }
};

struct reapers_harvest_unholy_t : public scoped_action_callback_t<virulent_plague_t>
{
  reapers_harvest_unholy_t() : super( DEATH_KNIGHT, "virulent_plague" )
  { }

  void manipulate( virulent_plague_t* action, const special_effect_t& e ) override
  {
    double chance = e.driver() -> effectN( 1 ).average( e.item ) / 100.0;

    action -> wandering_plague = new wandering_plague_t( action -> p(), e.item );
    action -> wandering_plague_proc_chance = chance;
    action -> add_child( action -> wandering_plague );
  }
};

struct reapers_harvest_blood_t : public scoped_action_callback_t<marrowrend_t>
{
  reapers_harvest_blood_t() : super( DEATH_KNIGHT, "marrowrend" )
  { }

  void manipulate( marrowrend_t* action, const special_effect_t& e ) override
  {
    double coeff = e.driver() -> effectN( 1 ).average( e.item ) / 100.0;

    action -> base_multiplier *= 1.0 + e.driver() -> effectN( 2 ).average( e.item ) / 100.0;
    action -> unholy_coil_coeff = coeff;
    action -> unholy_coil = new unholy_coil_t( action -> p(), e.item );
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
    create_buff( p -> pets.dark_arbiter );

    for ( size_t i = 0; i < p -> pets.army_ghoul.size(); i++ )
    {
      create_buff( p -> pets.army_ghoul[ i ] );
    }

    for ( size_t i = 0; i < p -> pets.apocalypse_ghoul.size(); i++ )
    {
      create_buff( p -> pets.apocalypse_ghoul[ i ] );
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
  { p -> legendary.the_instructors_fourth_lesson = e.driver() -> effectN( 1 ).base_value(); }
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

struct consorts_cold_core_t : public scoped_action_callback_t<sindragosas_fury_t>
{
  consorts_cold_core_t() : super( DEATH_KNIGHT, "sindragosas_fury" )
  { }

  void manipulate( sindragosas_fury_t* action, const special_effect_t& e ) override
  {
    auto m = 1.0 + e.driver() -> effectN( 1 ).percent();
    action -> cooldown -> duration = action -> data().cooldown() * m;
  }
};

struct death_march_t : public scoped_actor_callback_t<death_knight_t>
{
  death_march_t() : super( DEATH_KNIGHT )
  { }

  // Make adjustment time negative here, spell data value is positive
  void manipulate( death_knight_t* p, const special_effect_t& e ) override
  { p -> legendary.death_march = timespan_t::from_seconds( -e.driver() -> effectN( 1 ).base_value() / 10 ); }
};

struct skullflowers_haemostasis_t : public class_buff_cb_t<buff_t>
{
  skullflowers_haemostasis_t() : super( DEATH_KNIGHT, "haemostasis" )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<death_knight_t*>( e.player ) -> buffs.skullflowers_haemostasis; }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
           .spell( e.trigger() )
           .default_value( e.trigger() -> effectN( 1 ).percent() )
           // Grab 1 second ICD from the driver
           .cd( e.driver() -> internal_cooldown() );
  }
};

struct seal_of_necrofantasia_t : public scoped_actor_callback_t<death_knight_t>
{
  seal_of_necrofantasia_t() : super( DEATH_KNIGHT )
  { }

  void manipulate( death_knight_t* p, const special_effect_t& e ) override
  { p -> legendary.seal_of_necrofantasia = e.driver(); }
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
    unique_gear::register_special_effect(  50401, runeforge::razorice_attack );
    unique_gear::register_special_effect(  51714, runeforge::razorice_debuff );
    unique_gear::register_special_effect( 166441, runeforge::fallen_crusader );
    unique_gear::register_special_effect(  62157, runeforge::stoneskin_gargoyle );
    unique_gear::register_special_effect( 184898, reapers_harvest_frost_t( "obliterate_mh" ) );
    unique_gear::register_special_effect( 184898, reapers_harvest_frost_t( "obliterate_offhand" ) );
    unique_gear::register_special_effect( 184983, reapers_harvest_unholy_t() );
    unique_gear::register_special_effect( 184897, reapers_harvest_blood_t()  );
    unique_gear::register_special_effect( 215068, taktheritrixs_shoulderpads_t() );
    unique_gear::register_special_effect( 205658, toravons_bindings_t() );
    unique_gear::register_special_effect( 208713, the_instructors_fourth_lesson_t() );
    unique_gear::register_special_effect( 208161, draugr_girdle_everlasting_king_t() );
    unique_gear::register_special_effect( 208786, uvanimor_the_unbeautiful_t() );
    unique_gear::register_special_effect( 208782, koltiras_newfound_will_t() );
    unique_gear::register_special_effect( 212216, seal_of_necrofantasia_t() );
    // 7.1.5
    unique_gear::register_special_effect( 235605, consorts_cold_core_t() );
    unique_gear::register_special_effect( 235556, death_march_t() );
    unique_gear::register_special_effect( 235558, skullflowers_haemostasis_t(), true );
  }

  void register_hotfixes() const override
  {
    hotfix::register_effect( "Death Knight", "2016-11-18", "Runic Empowerment base proc chance is intended to be 1.5% per Runic Power spent.", 68676, hotfix::HOTFIX_FLAG_LIVE )
      .field( "base_value" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 15 )
      .verification_value( 1 );
    /*
    hotfix::register_effect( "Death Knight", "2016-08-23", "Clawing Shadows damage has been changed to 130% weapon damage (was 150% Attack Power).", 324719 )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 0 )
      .verification_value( 1.5 );

    hotfix::register_effect( "Death Knight", "2016-08-23", "Blood Boil's damage has been reduced by 39%.", 43101 )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1 - 0.39 )
      .verification_value( 3.75 );

    hotfix::register_effect( "Death Knight", "2016-09-23", "Remorseless Winter damage increased by 50%.", 288891 )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.5 )
      .verification_value( 0.24 );

    hotfix::register_effect( "Death Knight", "2016-09-23", "Obliterate damage increased by 19%.", 331345 )
      .field( "base_value" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.19 )
      .verification_value( 320 );

    hotfix::register_effect( "Death Knight", "2016-09-23", "Obliterate damage increased by 19%.-offhand", 60373 )
      .field( "base_value" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.19 )
      .verification_value( 320 );

    hotfix::register_effect( "Death Knight", "2016-09-23", "Frost Strike damage increased by 12%.", 331348 )
      .field( "base_value" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.12 )
      .verification_value( 250 );

    hotfix::register_effect( "Death Knight", "2016-09-23", "Frost Strike damage increased by 12%.-offhand", 60369 )
      .field( "base_value" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.12 )
      .verification_value( 250 );

    hotfix::register_effect( "Death Knight", "2016-09-23", "Howling Blast damage increased by 10%.", 41296 )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.10 )
      .verification_value( 0.5 );

    hotfix::register_effect( "Death Knight", "2016-09-23", "Frostscythe (Talent) damage increased by 13%.", 306492 )
      .field( "base_value" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.13 )
      .verification_value( 120 );

    hotfix::register_effect( "Death Knight", "2016-09-23", "Frozen Pulse (Talent) damage increased by 11%.", 287339 )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.11 )
      .verification_value( 0.72 );

    hotfix::register_effect( "Death Knight", "2016-09-23", "Breath of Sindragosa (Talent) damage increased by 17%.", 215545 )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.17 )
      .verification_value( 1.5 );
      */
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
