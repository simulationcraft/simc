// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Mage
// ==========================================================================

// Forward declarations
struct mage_t;

namespace buffs {
  struct touch_of_the_magi_t;
  struct arcane_missiles_t;
}

namespace pets {
  namespace water_elemental {
    struct water_elemental_pet_t;
  }
}

struct state_switch_t
{
private:
  bool state;
  timespan_t last_enable;
  timespan_t last_disable;

public:
  state_switch_t()
  {
    reset();
  }

  bool enable( timespan_t now )
  {
    if ( last_enable == now )
    {
      return false;
    }
    state = true;
    last_enable = now;
    return true;
  }

  bool disable( timespan_t now )
  {
    if ( last_disable == now )
    {
      return false;
    }
    state = false;
    last_disable = now;
    return true;
  }

  bool on()
  {
    return state;
  }

  timespan_t duration( timespan_t now )
  {
    if ( !state )
    {
      return timespan_t::zero();
    }
    return now - last_enable;
  }

  void reset()
  {
    state        = false;
    last_enable  = timespan_t::min();
    last_disable = timespan_t::min();
  }
};

/// Icicle data, stored in an icicle container object. Contains a source stats object and the damage
struct icicle_data_t
{
  double damage;
  stats_t* stats;
};

/// Icicle container object, contains a timestamp and its corresponding icicle data!
struct icicle_tuple_t
{
  timespan_t timestamp;
  icicle_data_t data;
};

struct mage_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* blast_furnace;
    dot_t* conflagration_dot;
    dot_t* ignite;
    dot_t* living_bomb;
    dot_t* mark_of_aluneth;
    dot_t* nether_tempest;
  } dots;

  struct debuffs_t
  {
    buff_t* erosion;
    buff_t* slow;
    buffs::touch_of_the_magi_t* touch_of_the_magi;

    buff_t* frost_bomb;
    buff_t* water_jet; // Proxy Water Jet to compensate for expression system
    buff_t* winters_chill;
    buff_t* frozen;
  } debuffs;

  mage_td_t( player_t* target, mage_t* mage );
};

struct buff_stack_benefit_t
{
  const buff_t* buff;
  std::vector<benefit_t*> buff_stack_benefit;

  buff_stack_benefit_t( const buff_t* _buff,
                        const std::string& prefix ) :
    buff( _buff ),
    buff_stack_benefit()
  {
    for ( int i = 0; i <= buff -> max_stack(); i++ )
    {
      buff_stack_benefit.push_back( buff -> player ->
                                get_benefit( prefix + " " +
                                             buff -> data().name_cstr() + " " +
                                             util::to_string( i ) ) );
    }
  }

  void update()
  {
    for ( std::size_t i = 0; i < buff_stack_benefit.size(); ++i )
    {
      buff_stack_benefit[ i ] -> update( i == as<unsigned>( buff -> check() ) );
    }
  }
};

struct buff_source_benefit_t
{
  const buff_t* buff;
  std::vector<std::string> sources;
  std::vector<benefit_t*> buff_source_benefit;

  buff_source_benefit_t( const buff_t* _buff ) :
    buff( _buff )
  { }

  int get_source_id( const char* source )
  {
    std::string source_str = std::string( source );
    for ( size_t i = 0; i < sources.size(); i++ )
    {
      if ( sources[i] == source_str )
      {
        return as<int>( i );
      }
    }

    sources.push_back( source_str );

    std::string benefit_name =
      std::string( buff -> data().name_cstr() ) + " from " + source_str;
    benefit_t* source_benefit = buff -> player -> get_benefit( benefit_name );
    buff_source_benefit.push_back( source_benefit );

    return as<int>( sources.size() - 1 );
  }

  void update( int source_id, int stacks = 1 )
  {
    assert( source_id >= 0 && source_id < as<int>( sources.size() ) );

    for ( size_t i = 0; i < sources.size(); i++ )
    {
      for ( int j = 0; j < stacks; j++ )
      {
        buff_source_benefit[i] -> update( as<int>( i ) == source_id );
      }
    }
  }
};

struct mage_t : public player_t
{
public:
  // Icicles
  std::vector<icicle_tuple_t> icicles;
  action_t* icicle;
  event_t* icicle_event;

  // Ignite
  action_t* ignite;
  event_t* ignite_spread_event;

  // Active
  player_t* last_bomb_target;

  // State switches for rotation selection
  state_switch_t burn_phase;

  // Ground AoE tracking
  std::map<std::string, timespan_t> ground_aoe_expiration;
  ground_aoe_event_t* active_meteor_burn;

  // Miscellaneous
  double distance_from_rune;
  double global_cinder_count;
  timespan_t firestarter_time;
  int blessing_of_wisdom_count;

  // Cached actions
  struct actions_t
  {
    action_t* arcane_assault;
    action_t* frost_bomb_explosion;
    action_t* legendary_arcane_orb;
    action_t* legendary_meteor;
    action_t* legendary_comet_storm;
    action_t* touch_of_the_magi_explosion;
    action_t* unstable_magic_explosion;
  } action;

  // Benefits
  struct benefits_t
  {
    buff_stack_benefit_t* incanters_flow;

    struct arcane_charge_benefits_t
    {
      buff_stack_benefit_t* arcane_barrage;
      buff_stack_benefit_t* arcane_blast;
      buff_stack_benefit_t* arcane_explosion;
      buff_stack_benefit_t* arcane_missiles;
      buff_stack_benefit_t* nether_tempest;
    } arcane_charge;

    buff_source_benefit_t* arcane_missiles;

    buff_source_benefit_t* fingers_of_frost;
    buff_stack_benefit_t* ray_of_frost;
  } benefits;

  // Buffs
  struct buffs_t
  {
    // Arcane
    buff_t* arcane_charge;
    buff_t* arcane_familiar;
    buffs::arcane_missiles_t* arcane_missiles;
    buff_t* arcane_power;
    buff_t* chrono_shift;
    buff_t* crackling_energy; // T20 2pc Arcane
    buff_t* presence_of_mind;

    // Fire
    buff_t* combustion;
    buff_t* contained_infernal_core; // 7.2.5 legendary shoulder, tracking buff
    buff_t* critical_massive;        // T20 4pc Fire
    buff_t* enhanced_pyrotechnics;
    buff_t* erupting_infernal_core;  // 7.2.5 legendary shoulder, primed buff
    buff_t* frenetic_speed;
    buff_t* heating_up;
    buff_t* hot_streak;
    buff_t* ignition;                // T20 2pc Fire
    buff_t* pyretic_incantation;
    buff_t* scorched_earth;
    buff_t* streaking;               // T19 4pc Fire


    // Frost
    buff_t* brain_freeze;
    buff_t* fingers_of_frost;
    buff_t* frozen_mass;                       // T20 2pc Frost
    buff_t* icicles;                           // Buff to track icicles - doesn't always line up with icicle count though!
    buff_t* icy_veins;
    buff_t* rage_of_the_frost_wyrm;            // 7.2.5 legendary head, primed buff
    buff_t* shattered_fragments_of_sindragosa; // 7.2.5 legendary head, tracking buff


    // Talents
    buff_t* bone_chilling;
    buff_t* ice_floes;
    buff_t* incanters_flow;
    buff_t* ray_of_frost;
    buff_t* rune_of_power;

    // Artifact
    buff_t* chain_reaction;
    buff_t* chilled_to_the_core;
    buff_t* freezing_rain;
    buff_t* time_and_space;
    buff_t* warmth_of_the_phoenix;

    // Legendary
    buff_t* cord_of_infinity;
    buff_t* kaelthas_ultimate_ability;
    buff_t* lady_vashjs_grasp;
    buff_t* magtheridons_might;
    buff_t* rhonins_assaulting_armwraps;
    buff_t* shard_time_warp;
    buff_t* zannesu_journey;

    haste_buff_t* sephuzs_secret;

    // Miscellaneous Buffs
    buff_t* greater_blessing_of_widsom;
    buff_t* t19_oh_buff;

  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* combustion;
    cooldown_t* cone_of_cold;
    cooldown_t* evocation;
    cooldown_t* frost_nova;
    cooldown_t* frozen_orb;
    cooldown_t* icy_veins;
    cooldown_t* phoenixs_flames;
    cooldown_t* presence_of_mind;
    cooldown_t* ray_of_frost;
    cooldown_t* time_warp;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* aluneths_avarice;
    gain_t* greater_blessing_of_wisdom;
    gain_t* evocation;
    gain_t* mystic_kilt_of_the_rune_master;
  } gains;

  // Pets
  struct pets_t
  {
    pets::water_elemental::water_elemental_pet_t* water_elemental;

    std::vector<pet_t*> mirror_images;

    pets_t() : water_elemental( nullptr )
    {}
  } pets;

  // Procs
  struct procs_t
  {
    proc_t* heating_up_generated;         // Crits without HU/HS
    proc_t* heating_up_removed;           // Non-crits with HU >200ms after application
    proc_t* heating_up_ib_converted;      // IBs used on HU
    proc_t* hot_streak;                   // Total HS generated
    proc_t* hot_streak_spell;             // HU/HS spell impacts
    proc_t* hot_streak_spell_crit;        // HU/HS spell crits
    proc_t* hot_streak_spell_crit_wasted; // HU/HS spell crits with HS

    proc_t* ignite_applied;    // Direct ignite applications
    proc_t* ignite_spread;     // Spread events
    proc_t* ignite_new_spread; // Spread to new target
    proc_t* ignite_overwrite;  // Spread to target with existing ignite

    proc_t* controlled_burn; // Tracking Controlled Burn talent
  } procs;

  // Specializations
  struct specializations_t
  {
    // Arcane
    const spell_data_t* arcane_barrage_2;
    const spell_data_t* arcane_charge;
    const spell_data_t* arcane_mage;
    const spell_data_t* evocation_2;
    const spell_data_t* savant;

    // Fire
    const spell_data_t* critical_mass;
    const spell_data_t* critical_mass_2;
    const spell_data_t* fire_blast_2;
    const spell_data_t* fire_blast_3;
    const spell_data_t* fire_mage;
    const spell_data_t* ignite;

    // Frost
    const spell_data_t* brain_freeze;
    const spell_data_t* brain_freeze_2;
    const spell_data_t* blizzard_2;
    const spell_data_t* fingers_of_frost;
    const spell_data_t* frost_mage;
    const spell_data_t* icicles;
    const spell_data_t* icicles_driver;
    const spell_data_t* shatter;
    const spell_data_t* shatter_2;
  } spec;

  // State
  struct state_t
  {
    bool brain_freeze_active;
    bool fingers_of_frost_active;
    bool ignition_active;
  } state;

  // Talents
  struct talents_list_t
  {
    // Tier 15
    const spell_data_t* arcane_familiar;
    const spell_data_t* amplification;
    const spell_data_t* words_of_power;
    const spell_data_t* pyromaniac;
    const spell_data_t* conflagration;
    const spell_data_t* firestarter;
    const spell_data_t* ray_of_frost;
    const spell_data_t* lonely_winter;
    const spell_data_t* bone_chilling;

    // Tier 30
    const spell_data_t* shimmer;
    const spell_data_t* slipstream;
    const spell_data_t* blast_wave;
    const spell_data_t* ice_floes;
    const spell_data_t* mana_shield; // NYI
    const spell_data_t* blazing_soul; // NYI
    const spell_data_t* glacial_insulation; // NYI

    // Tier 45
    const spell_data_t* mirror_image;
    const spell_data_t* rune_of_power;
    const spell_data_t* incanters_flow;

    // Tier 60
    const spell_data_t* supernova;
    const spell_data_t* charged_up;
    const spell_data_t* resonance;
    const spell_data_t* alexstraszas_fury;
    const spell_data_t* flame_on;
    const spell_data_t* controlled_burn;
    const spell_data_t* ice_nova;
    const spell_data_t* frozen_touch;
    const spell_data_t* splitting_ice;

    // Tier 75
    const spell_data_t* chrono_shift;
    const spell_data_t* frenetic_speed;
    const spell_data_t* frigid_winds; // NYI
    const spell_data_t* ring_of_frost; // NYI
    const spell_data_t* ice_ward;

    // Tier 90
    const spell_data_t* nether_tempest;
    const spell_data_t* living_bomb;
    const spell_data_t* frost_bomb;
    const spell_data_t* unstable_magic;
    const spell_data_t* erosion;
    const spell_data_t* flame_patch;
    const spell_data_t* arctic_gale;

    // Tier 100
    const spell_data_t* overpowered;
    const spell_data_t* temporal_flux;
    const spell_data_t* arcane_orb;
    const spell_data_t* kindling;
    const spell_data_t* cinderstorm;
    const spell_data_t* meteor;
    const spell_data_t* thermal_void;
    const spell_data_t* glacial_spike;
    const spell_data_t* comet_storm;
  } talents;

  // Artifact
  struct artifact_spell_data_t
  {
    // Arcane
    artifact_power_t aegwynns_intensity;
    artifact_power_t aluneths_avarice;
    artifact_power_t time_and_space;
    artifact_power_t arcane_rebound;
    artifact_power_t ancient_power;
    artifact_power_t scorched_earth;
    artifact_power_t everywhere_at_once; //NYI
    artifact_power_t arcane_purification;
    artifact_power_t aegwynns_imperative;
    artifact_power_t aegwynns_ascendance;
    artifact_power_t aegwynns_wrath;
    artifact_power_t crackling_energy;
    artifact_power_t blasting_rod;
    artifact_power_t ethereal_sensitivity;
    artifact_power_t aegwynns_fury;
    artifact_power_t mana_shield; // NYI
    artifact_power_t mark_of_aluneth;
    artifact_power_t might_of_the_guardians;
    artifact_power_t rule_of_threes;
    artifact_power_t slooow_down; // NYI
    artifact_power_t torrential_barrage;
    artifact_power_t touch_of_the_magi;
    artifact_power_t intensity_of_the_tirisgarde;

    // Fire
    artifact_power_t aftershocks;
    artifact_power_t everburning_consumption;
    artifact_power_t blue_flame_special;
    artifact_power_t molten_skin; //NYI
    artifact_power_t phoenix_reborn;
    artifact_power_t great_balls_of_fire;
    artifact_power_t cauterizing_blink; //NYI
    artifact_power_t fire_at_will;
    artifact_power_t preignited;
    artifact_power_t warmth_of_the_phoenix;
    artifact_power_t strafing_run;
    artifact_power_t pyroclasmic_paranoia;
    artifact_power_t reignition_overdrive;
    artifact_power_t pyretic_incantation;
    artifact_power_t phoenixs_flames;
    artifact_power_t burning_gaze;
    artifact_power_t big_mouth;
    artifact_power_t blast_furnace;
    artifact_power_t wings_of_flame;
    artifact_power_t empowered_spellblade;
    artifact_power_t instability_of_the_tirisgarde;

    // Frost
    artifact_power_t ebonbolt;
    artifact_power_t jouster; // NYI
    artifact_power_t let_it_go;
    artifact_power_t frozen_veins;
    artifact_power_t the_storm_rages;
    artifact_power_t black_ice;
    artifact_power_t shield_of_alodi; //NYI
    artifact_power_t icy_caress;
    artifact_power_t ice_nine;
    artifact_power_t chain_reaction;
    artifact_power_t clarity_of_thought;
    artifact_power_t its_cold_outside;
    artifact_power_t shattering_bolts;
    artifact_power_t orbital_strike;
    artifact_power_t icy_hand;
    artifact_power_t ice_age;
    artifact_power_t chilled_to_the_core;
    artifact_power_t spellborne;
    artifact_power_t obsidian_lance;
    artifact_power_t freezing_rain;
    artifact_power_t glacial_eruption;
    artifact_power_t frigidity_of_the_tirisgarde;
  } artifact;

public:
  mage_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF );

  ~mage_t();

  // Character Definition
  virtual std::string get_special_use_items( const std::string& item = std::string(), bool specials = false );
  virtual void        init_spells() override;
  virtual void        init_base_stats() override;
  virtual void        create_buffs() override;
  virtual void        create_options() override;
  virtual void        init_gains() override;
  virtual void        init_procs() override;
  virtual void        init_benefits() override;
  virtual void        init_assessors() override;
  virtual void        invalidate_cache( cache_e c ) override;
  virtual void        init_resources( bool force ) override;
  virtual void        recalculate_resource_max( resource_e rt ) override;
  virtual void        reset() override;
  virtual expr_t*     create_expression( action_t*, const std::string& name ) override;
  virtual action_t*   create_action( const std::string& name, const std::string& options ) override;
  virtual bool        create_actions() override;
  virtual void        create_pets() override;
  virtual resource_e  primary_resource() const override { return RESOURCE_MANA; }
  virtual role_e      primary_role() const override { return ROLE_SPELL; }
  virtual stat_e      convert_hybrid_stat( stat_e s ) const override;
  virtual stat_e      primary_stat() const override { return STAT_INTELLECT; }
  virtual double      mana_regen_per_second() const override;
  virtual double      composite_player_multiplier( school_e school ) const override;
  virtual double      composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  virtual double      composite_player_pet_damage_multiplier( const action_state_t* ) const override;
  virtual double      composite_spell_crit_chance() const override;
  virtual double      composite_spell_crit_rating() const override;
  virtual double      composite_spell_haste() const override;
  virtual double      composite_mastery_rating() const override;
  virtual double      composite_attribute_multiplier( attribute_e ) const override;
  virtual double      matching_gear_multiplier( attribute_e attr ) const override;
  virtual void        update_movement( timespan_t duration ) override;
  virtual void        stun() override;
  virtual double      temporary_movement_modifier() const override;
  virtual double      passive_movement_modifier() const override;
  virtual void        arise() override;
  virtual std::string create_profile( save_e ) override;
  virtual void        copy_from( player_t* ) override;

  target_specific_t<mage_td_t> target_data;

  virtual mage_td_t* get_target_data( player_t* target ) const override
  {
    mage_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new mage_td_t( target, const_cast<mage_t*>(this) );
    }
    return td;
  }

  // Public mage functions:
  icicle_data_t get_icicle_object();
  void trigger_icicle( const action_state_t* trigger_state, bool chain = false, player_t* chain_target = nullptr );
  void trigger_touch_of_the_magi( buffs::touch_of_the_magi_t* touch_of_the_magi_buff );
  void trigger_t19_oh();

  bool apply_crowd_control( const action_state_t* state, spell_mechanic type );

  void              apl_precombat();
  void              apl_arcane();
  void              apl_fire();
  void              apl_frost();
  void              apl_default();
  virtual void      init_action_list() override;

  std::string       default_potion() const override;
  std::string       default_flask() const override;
  std::string       default_food() const override;
  std::string       default_rune() const override;
};

namespace pets
{
struct mage_pet_t : public pet_t
{
  mage_pet_t( sim_t* sim, mage_t* owner, std::string pet_name,
              bool guardian = false, bool dynamic = false )
    : pet_t( sim, owner, pet_name, guardian, dynamic )
  { }

  const mage_t* o() const
  {
    return static_cast<mage_t*>( owner );
  }

  mage_t* o()
  {
    return static_cast<mage_t*>( owner );
  }
};

struct mage_pet_spell_t : public spell_t
{
  mage_pet_spell_t( const std::string& n, mage_pet_t* p, const spell_data_t* s )
    : spell_t( n, p, s )
  { }

  mage_t* o()
  {
    return static_cast<mage_pet_t*>( player ) -> o();
  }

  const mage_t* o() const
  {
    return static_cast<mage_pet_t*>( player ) -> o();
  }
};

namespace water_elemental
{
// ==========================================================================
// Pet Water Elemental
// ==========================================================================
struct water_elemental_pet_t;

struct water_elemental_pet_td_t : public actor_target_data_t
{
  buff_t* water_jet;

public:
  water_elemental_pet_td_t( player_t* target, water_elemental_pet_t* welly );
};

struct water_elemental_pet_t : public mage_pet_t
{
  target_specific_t<water_elemental_pet_td_t> target_data;

  struct cooldowns_t
  {
    cooldown_t* wj_freeze; // Shared Freeze/Water Jet cooldown.
  } cooldown;

  water_elemental_pet_t( sim_t* sim, mage_t* owner )
    : mage_pet_t( sim, owner, "water_elemental" )
  {
    owner_coeff.sp_from_sp = 0.75;
    cooldown.wj_freeze = get_cooldown( "wj_freeze" );
    cooldown.wj_freeze -> duration = timespan_t::from_seconds( 25.0 );
  }

  void init_action_list() override
  {
    clear_action_priority_lists();
    auto default_list = get_action_priority_list( "default" );

    default_list -> add_action( this, find_pet_spell( "Water Jet" ), "Water Jet" );
    default_list -> add_action( this, find_pet_spell( "Waterbolt" ), "Waterbolt" );
    default_list -> add_action( this, find_pet_spell( "Freeze"    ), "Freeze"    );

    // Default
    use_default_action_list = true;

    mage_pet_t::init_action_list();
  }

  water_elemental_pet_td_t* td( player_t* t ) const
  {
    return get_target_data( t );
  }

  virtual water_elemental_pet_td_t* get_target_data(
      player_t* target ) const override
  {
    water_elemental_pet_td_t*& td = target_data[ target ];
    if ( !td )
      td = new water_elemental_pet_td_t(
          target, const_cast<water_elemental_pet_t*>( this ) );
    return td;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override;
};

water_elemental_pet_td_t::water_elemental_pet_td_t(
    player_t* target, water_elemental_pet_t* welly )
  : actor_target_data_t( target, welly )
{
  water_jet = buff_creator_t( *this, "water_jet", welly -> find_spell( 135029 ) )
                  .cd( timespan_t::zero() );
}

struct water_elemental_spell_t : public mage_pet_spell_t
{
  water_elemental_spell_t( const std::string& n, mage_pet_t* p, const spell_data_t* s )
    : mage_pet_spell_t( n, p, s )
  {
    base_multiplier *= 1.0 + o() -> spec.frost_mage -> effectN( 1 ).percent();
  }

  virtual double action_multiplier() const override
  {
    double am = mage_pet_spell_t::action_multiplier();

    if ( o() -> spec.icicles -> ok() )
    {
      am *= 1.0 + o() -> cache.mastery_value();
    }

    return am;
  }
};

struct waterbolt_t : public water_elemental_spell_t
{
  waterbolt_t( water_elemental_pet_t* p, const std::string& options_str )
    : water_elemental_spell_t( "waterbolt", p, p -> find_pet_spell( "Waterbolt" ) )
  {
    trigger_gcd = timespan_t::zero();
    parse_options( options_str );
    may_crit = true;
    base_multiplier *= 1.0 + o() -> artifact.its_cold_outside.data().effectN( 3 ).percent();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t cast_time = water_elemental_spell_t::execute_time();

    // For some reason welly seems to have a cap'd rate of cast of
    // 1.5/second. Instead of modeling this as a cooldown/GCD (like it is in game)
    // we model it as a capped cast time, with 1.5 being the lowest it can go.
    return std::max( cast_time, timespan_t::from_seconds( 1.5 ) );
  }
};

struct freeze_t : public water_elemental_spell_t
{
  int fof_source_id;

  freeze_t( water_elemental_pet_t* p, const std::string& options_str )
    : water_elemental_spell_t( "freeze", p, p -> find_pet_spell( "Freeze" ) )
  {
    parse_options( options_str );
    aoe                   = -1;
    may_crit              = true;
    ignore_false_positive = true;
    action_skill          = 1;

    cooldown = p -> cooldown.wj_freeze;
  }

  virtual bool init_finished() override
  {
    water_elemental_pet_t* p = static_cast<water_elemental_pet_t*>( player );
    fof_source_id = p -> o() -> benefits.fingers_of_frost
                             -> get_source_id( data().name_cstr() );

    return water_elemental_spell_t::init_finished();
  }

  virtual void impact( action_state_t* s ) override
  {
    water_elemental_spell_t::impact( s );

    bool success = o() -> apply_crowd_control( s, MECHANIC_ROOT );
    if ( success )
    {
      o() -> buffs.fingers_of_frost -> trigger();
      o() -> benefits.fingers_of_frost -> update( fof_source_id );
    }
  }
};

struct water_jet_t : public water_elemental_spell_t
{
  // queued water jet spell, auto cast water jet spell
  bool queued;
  bool autocast;

  water_jet_t( water_elemental_pet_t* p, const std::string& options_str )
    : water_elemental_spell_t( "water_jet", p, p -> find_pet_spell( "Water Jet" ) ),
      queued( false ),
      autocast( true )
  {
    parse_options( options_str );
    channeled = tick_may_crit = true;
    tick_zero         = true;
    cooldown = p -> cooldown.wj_freeze;
  }
  water_elemental_pet_td_t* td( player_t* t ) const
  {
    return static_cast<water_elemental_pet_t*>( player )
        ->get_target_data( t ? t : target );
  }

  void execute() override
  {
    // If this is a queued execute, disable queued status
    if ( !autocast && queued )
      queued = false;

    // Don't execute Water Jet if Water Elemental used Freeze
    // during the cast
    if ( cooldown -> up() )
    {
      water_elemental_spell_t::execute();
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    water_elemental_spell_t::impact( s );

    td( s -> target )
        -> water_jet -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0,
                              dot_duration * player -> composite_spell_speed() );

    // Trigger hidden proxy water jet for the mage, so
    // debuff.water_jet.<expression> works
    o() -> get_target_data( s->target )
        -> debuffs.water_jet -> trigger(
            1, buff_t::DEFAULT_VALUE(), 1.0,
            dot_duration * player -> composite_spell_speed() );
  }

  virtual void last_tick( dot_t* d ) override
  {
    water_elemental_spell_t::last_tick( d );
    td( d -> target ) -> water_jet -> expire();
  }

  bool ready() override
  {
    // Not ready, until the owner gives permission to cast
    if ( !autocast && !queued )
      return false;

    return water_elemental_spell_t::ready();
  }

  void reset() override
  {
    water_elemental_spell_t::reset();

    queued = false;
  }
};

action_t* water_elemental_pet_t::create_action( const std::string& name,
                                                const std::string& options_str )
{
  if ( name == "freeze" )
    return new freeze_t( this, options_str );
  if ( name == "waterbolt" )
    return new waterbolt_t( this, options_str );
  if ( name == "water_jet" )
    return new water_jet_t( this, options_str );

  return mage_pet_t::create_action( name, options_str );
}

}  // water_elemental

namespace mirror_image
{
// ==========================================================================
// Pet Mirror Image
// ==========================================================================

struct mirror_image_pet_t : public mage_pet_t
{
  buff_t* arcane_charge;

  mirror_image_pet_t( sim_t* sim, mage_t* owner )
    : mage_pet_t( sim, owner, "mirror_image", true ), arcane_charge( nullptr )
  {
    owner_coeff.sp_from_sp = 1.00;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override;

  virtual void init_action_list() override
  {
    switch ( o() -> specialization() )
    {
      case MAGE_FIRE:
        action_list_str = "fireball";
        break;
      case MAGE_ARCANE:
        action_list_str = "arcane_blast";
        break;
      case MAGE_FROST:
        action_list_str = "frostbolt";
        break;
      default:
        break;
    }

    mage_pet_t::init_action_list();
  }

  virtual void create_buffs() override
  {
    mage_pet_t::create_buffs();

    arcane_charge =
        buff_creator_t( this, "arcane_charge", o() -> spec.arcane_charge );
  }
};

struct mirror_image_spell_t : public mage_pet_spell_t
{
  mirror_image_spell_t( const std::string& n, mirror_image_pet_t* p,
                        const spell_data_t* s )
    : mage_pet_spell_t( n, p, s )
  {
    may_crit = true;
  }

  bool init_finished() override
  {
    if ( p() -> o() -> pets.mirror_images[ 0 ] )
    {
      stats = p() -> o() -> pets.mirror_images[ 0 ] -> get_stats( name_str );
    }

    return mage_pet_spell_t::init_finished();
  }

  mirror_image_pet_t* p() const
  {
    return static_cast<mirror_image_pet_t*>( player );
  }
};

struct arcane_blast_t : public mirror_image_spell_t
{
  arcane_blast_t( mirror_image_pet_t* p, const std::string& options_str )
    : mirror_image_spell_t( "arcane_blast", p,
                            p -> find_pet_spell( "Arcane Blast" ) )
  {
    parse_options( options_str );
    base_multiplier *= 1.0 + o() -> spec.arcane_mage -> effectN( 1 ).percent();
  }

  virtual void execute() override
  {
    mirror_image_spell_t::execute();

    p()->arcane_charge->trigger();
  }

  virtual double action_multiplier() const override
  {
    double am = mirror_image_spell_t::action_multiplier();

    // MI Arcane Charges are still hardcoded as 25% damage gains
    am *= 1.0 + p()->arcane_charge->check() * 0.25;

    return am;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double tm = mirror_image_spell_t::composite_target_multiplier( target );

    // Arcane Blast (88084) should work with Erosion, according to the spell data.
    // Does not work in game, as of build 24461, 2017-07-03.
    if ( ! o() -> bugs )
    {
      mage_td_t* tdata = o() -> get_target_data( target );
      tm *= 1.0 + tdata -> debuffs.erosion -> check_stack_value();
    }

    return tm;
  }
};

struct fireball_t : public mirror_image_spell_t
{
  fireball_t( mirror_image_pet_t* p, const std::string& options_str )
    : mirror_image_spell_t( "fireball", p, p -> find_pet_spell( "Fireball" ) )
  {
    parse_options( options_str );
    base_multiplier *= 1.0 + o() -> spec.fire_mage -> effectN( 1 ).percent();
  }
};

struct frostbolt_t : public mirror_image_spell_t
{
  frostbolt_t( mirror_image_pet_t* p, const std::string& options_str )
    : mirror_image_spell_t( "frostbolt", p, p -> find_pet_spell( "Frostbolt" ) )
  {
    parse_options( options_str );
    base_multiplier *= 1.0 + o() -> spec.frost_mage -> effectN( 1 ).percent();
  }
};

action_t* mirror_image_pet_t::create_action( const std::string& name,
                                             const std::string& options_str )
{
  if ( name == "arcane_blast" )
    return new arcane_blast_t( this, options_str );
  if ( name == "fireball" )
    return new fireball_t( this, options_str );
  if ( name == "frostbolt" )
    return new frostbolt_t( this, options_str );

  return mage_pet_t::create_action( name, options_str );
}

}  // mirror_image

}  // pets

// Cinderstorm impact helper event ============================================
namespace events {
struct cinder_impact_event_t : public event_t
{
  action_t* cinder;
  player_t* target;

  cinder_impact_event_t( actor_t& m, action_t* c, player_t* t,
                         timespan_t impact_time ) :
    event_t( m, impact_time ), cinder( c ), target( t )
  { }

  virtual const char* name() const override
  { return "cinder_impact_event"; }

  void execute() override
  {
    cinder -> set_target( target );
    cinder -> execute();
  }
};


}

namespace buffs {
// Arcane Missiles Buff =======================================================
struct arcane_missiles_t : public buff_t
{
  arcane_missiles_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "arcane_missiles", p -> find_spell( 79683 ) ) )
  {
    default_chance = p -> find_spell( 79684 ) -> effectN( 1 ).percent();
  }

  double proc_chance() const
  {
    double am_proc_chance = default_chance;

    mage_t* p = static_cast<mage_t*>( player );

    if ( p -> talents.words_of_power -> ok() )
    {
      double mult = p -> resources.pct( RESOURCE_MANA ) /
                    p -> talents.words_of_power -> effectN( 2 ).percent();
      am_proc_chance += mult * p -> talents.words_of_power -> effectN( 1 ).percent();
    }

    am_proc_chance += p -> artifact.ethereal_sensitivity.percent();
    am_proc_chance += p -> sets -> set( MAGE_ARCANE, T19, B2 ) -> effectN( 1 ).percent();

    return am_proc_chance;
  }

  bool trigger( int stacks, double value,
                double chance, timespan_t duration ) override
  {
    if ( chance < 0 )
    {
      chance = proc_chance();
    }
    return buff_t::trigger( stacks, value, chance, duration );
  }
};

struct erosion_t : public buff_t
{
  // Erosion debuff =============================================================

  struct erosion_event_t : public event_t
  {
    erosion_t* debuff;
    const spell_data_t* data;

    static timespan_t delta_time( const spell_data_t* data,
                                  bool player_triggered )
    {
      // Erosion debuff decays 3 seconds after direct application by a player,
      // followed by a 1 stack every second
      if ( player_triggered )
      {
        return data -> duration();
      }
      return data -> effectN( 1 ).period();
    }

    erosion_event_t( actor_t& m, erosion_t* _debuff, const spell_data_t* _data,
                     bool player_triggered = false )
      : event_t( m, delta_time( _data, player_triggered ) ),
        debuff( _debuff ),
        data( _data )
    { }

    const char* name() const override
    { return "erosion_decay_event"; }


    void execute() override
    {
      debuff -> decrement();

      // Always update the parent debuff's reference to the decay event, so that it
      // can be cancelled upon a new application of the debuff
      if ( debuff -> check() > 0 )
      {
        debuff->decay_event = make_event<erosion_event_t>(
            sim(), *( debuff->source ), debuff, data );
      }
      else
      {
        debuff -> decay_event = nullptr;
      }
    }
  };

  const spell_data_t* erosion_event_data;
  event_t* decay_event;

  erosion_t( mage_td_t* td ) :
    buff_t( buff_creator_t( *td, "erosion",
                            td -> source -> find_spell( 210134 ) ) ),
    erosion_event_data( td -> source -> find_spell( 210154 ) ),
    decay_event( nullptr )
  {
    set_default_value( data().effectN( 1 ).percent() );
  }

  bool trigger( int stacks, double value,
                double chance, timespan_t duration ) override
  {
    bool triggered = buff_t::trigger( stacks, value, chance, duration );

    if ( triggered )
    {
      if ( decay_event )
      {
        event_t::cancel( decay_event );
      }

      decay_event = make_event<erosion_event_t>( *sim, *source, this, erosion_event_data, true );
    }

    return triggered;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    event_t::cancel( decay_event );
  }

  void reset() override
  {
    event_t::cancel( decay_event );
    buff_t::reset();
  }
};


// Touch of the Magi debuff ===================================================

struct touch_of_the_magi_t : public buff_t
{
  mage_t* mage;
  double accumulated_damage;

  touch_of_the_magi_t( mage_td_t* td ) :
    buff_t( buff_creator_t( *td, "touch_of_the_magi",
                            td -> source -> find_spell( 210824 ) ) ),
    mage( static_cast<mage_t*>( td -> source ) ),
    accumulated_damage( 0.0 )
  { }

  virtual void reset() override
  {
    buff_t::reset();
    accumulated_damage = 0.0;
  }

  virtual void aura_loss() override
  {
    buff_t::aura_loss();

    mage -> trigger_touch_of_the_magi( this );

    accumulated_damage = 0.0;
  }

  double accumulate_damage( action_state_t* state )
  {
    if ( sim -> debug )
    {
      sim -> out_debug.printf(
        "%s's %s accumulates %f additional damage: %f -> %f",
        player -> name(), name(), state -> result_amount,
        accumulated_damage, accumulated_damage + state -> result_amount
      );
    }

    accumulated_damage += state -> result_amount;

    return accumulated_damage;
  }
};


// Custom buffs ===============================================================
struct incanters_flow_t : public buff_t
{
  incanters_flow_t( mage_t* p ) :
    buff_t( p, "incanters_flow", p -> find_spell( 116267 ) ) // Buff is a separate spell
  {
    set_duration( p -> sim -> max_time * 3 ); // Long enough duration to trip twice_expected_event
    set_period( p -> talents.incanters_flow -> effectN( 1 ).period() ); // Period is in the talent
    set_tick_behavior( BUFF_TICK_CLIP );
    set_default_value( data().effectN( 1 ).percent() );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  void bump( int stacks, double value ) override
  {
    int before_stack = current_stack;
    buff_t::bump( stacks, value );
    // Reverse direction if max stacks achieved before bump
    if ( before_stack == current_stack )
      reverse = true;
  }

  void decrement( int stacks, double value ) override
  {
    // This buff will never fade; reverse direction at 1 stack.
    // Buff uptime reporting _should_ work ok with this solution
    if ( current_stack > 1 )
      buff_t::decrement( stacks, value );
    else
      reverse = false;
  }
};


struct lady_vashjs_grasp_t: public buff_t
{
  int fof_source_id;
  mage_t* mage;

  lady_vashjs_grasp_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "lady_vashjs_grasp", p -> find_spell( 208147 ) ) ),
    fof_source_id( -1 ),
    mage( p )
  {
    set_tick_callback( [ this ] ( buff_t* /* buff */, int /* ticks */, const timespan_t& /* tick_time */ )
    {
      assert( fof_source_id != -1 );
      mage -> buffs.fingers_of_frost -> trigger();
      mage -> benefits.fingers_of_frost -> update( fof_source_id );
    } );
  }

  bool trigger( int stacks = 1, double value = DEFAULT_VALUE(),
                double chance = -1.0, timespan_t duration = timespan_t::min() ) override
  {
    bool success = buff_t::trigger( stacks, value, chance, duration );
    if ( success )
    {
      // Triggering LVG gives one stack of Fingers of Frost, regardless of the tick action.
      assert( fof_source_id != -1 );
      mage -> buffs.fingers_of_frost -> trigger();
      mage -> benefits.fingers_of_frost -> update( fof_source_id );
    }

    return success;
  }
};


struct ray_of_frost_buff_t : public buff_t
{
  timespan_t rof_cd;

  ray_of_frost_buff_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "ray_of_frost", p -> find_spell( 208141 ) ) )
  {
    set_default_value( data().effectN( 1 ).percent() );
    const spell_data_t* rof_data = p -> find_spell( 205021 );
    rof_cd = rof_data -> cooldown() - rof_data -> duration();
  }

  //TODO: This be calling expire_override instead
  virtual void aura_loss() override
  {
    buff_t::aura_loss();

    mage_t* p = static_cast<mage_t*>( player );
    p -> cooldowns.ray_of_frost -> start( rof_cd );

    if ( p -> channeling && p -> channeling -> id == 205021 ) // 205021 is the spell id for ray of frost action
    {
      p -> channeling -> interrupt_action();
    }
  }
};

} // buffs


namespace actions {
// ============================================================================
// Mage Spell
// ==========================================================================

struct mage_spell_state_t : public action_state_t
{
  mage_spell_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target )
  { }

  virtual bool frozen() const
  {
    const mage_t* p = debug_cast<const mage_t*>( action -> player );
    const mage_td_t* td = p -> get_target_data( target );

    return ( td -> debuffs.winters_chill -> check() )
        || ( td -> debuffs.frozen -> check() );
  }

  virtual double composite_crit_chance() const override;
};

struct mage_spell_t : public spell_t
{
  struct affected_by_t
  {
    bool arcane_mage;
    bool fire_mage;
    bool frost_mage;

    bool erosion;
    bool shatter;
  } affected_by;

  bool consumes_ice_floes;
  bool triggers_arcane_missiles;

  int am_trigger_source_id;
public:

  mage_spell_t( const std::string& n, mage_t* p,
                const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( n, p, s ),
    affected_by( affected_by_t() ),
    consumes_ice_floes( true ),
    triggers_arcane_missiles( true ),
    am_trigger_source_id( -1 )
  {
    may_crit      = true;
    tick_may_crit = true;
  }

  virtual void init() override
  {
    spell_t::init();

    if ( affected_by.arcane_mage )
    {
      base_multiplier *= 1.0 + p() -> spec.arcane_mage -> effectN( 1 ).percent();
    }
    if ( affected_by.fire_mage )
    {
      base_multiplier *= 1.0 + p() -> spec.fire_mage -> effectN( 1 ).percent();
    }
    if ( affected_by.frost_mage )
    {
      base_multiplier *= 1.0 + p() -> spec.frost_mage -> effectN( 1 ).percent();
    }

    if ( !harmful || background )
    {
      triggers_arcane_missiles = false;
    }
  }

  virtual bool init_finished() override
  {
    if ( p() -> specialization() == MAGE_ARCANE &&
         triggers_arcane_missiles &&
         data().ok() )
    {
      am_trigger_source_id = p() -> benefits.arcane_missiles
                                 -> get_source_id( data().name_cstr() );
    }

    return spell_t::init_finished();
  }

  mage_t* p()
  { return static_cast<mage_t*>( player ); }

  const mage_t* p() const
  { return static_cast<mage_t*>( player ); }

  mage_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual action_state_t* new_state() override
  {
    return new mage_spell_state_t( this, target );
  }

  virtual double cost() const override
  {
    double c = spell_t::cost();

    if ( p() -> buffs.arcane_power -> check() )
    {
      double arcane_power_reduction;
      arcane_power_reduction = p() -> buffs.arcane_power -> data().effectN( 2 ).percent()
                             + p() -> talents.overpowered -> effectN( 2 ).percent();

      c *= 1.0 + arcane_power_reduction;
    }

    return c;
  }

  virtual bool usable_moving() const override
  {
    buff_t* ice_floes = p() -> buffs.ice_floes;

    if ( ice_floes -> check() && data().affected_by( ice_floes -> data().effectN( 1 ) ) )
    {
      return true;
    }

    return spell_t::usable_moving();
  }

  // You can thank Frost Nova for why this isn't in arcane_mage_spell_t instead
  void trigger_am( int source_id, double chance = -1.0,
                   int stacks = 1 )
  {
    if ( static_cast<buff_t*>( p() -> buffs.arcane_missiles )
             -> trigger( stacks, buff_t::DEFAULT_VALUE(), chance ) )
    {
      if ( source_id < 0 )
      {
        p() -> sim -> out_debug.printf( "Action %s does not have valid AM source_id",
                                        name_str.c_str() );
      }
      else
      {
        p() -> benefits.arcane_missiles -> update( source_id, stacks );
      }
    }
  }

  virtual void execute() override
  {
    spell_t::execute();

    if ( background )
      return;

    if ( execute_time() > timespan_t::zero() && consumes_ice_floes && p() -> buffs.ice_floes -> up() )
    {
      p() -> buffs.ice_floes -> decrement();
    }

    if ( p() -> specialization() == MAGE_ARCANE && hit_any_target && triggers_arcane_missiles )
    {
      trigger_am( am_trigger_source_id );
    }

    if ( harmful && !background && p() -> talents.incanters_flow -> ok() )
    {
      p() -> benefits.incanters_flow -> update();
    }
  }

  void trigger_unstable_magic( action_state_t* state );

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double tm = spell_t::composite_target_multiplier( target );

    if ( affected_by.erosion )
    {
      tm *= 1.0 + td( target ) -> debuffs.erosion -> check_stack_value();
    }

    return tm;
  }

  // Helper methods for 7.2.5 fire shoulders and frost head.
  void trigger_legendary_effect( buff_t* tracking_buff, buff_t* primed_buff, action_t* action, player_t* target )
  {
    if ( tracking_buff -> check() == tracking_buff -> max_stack() - 2 )
    {
      tracking_buff -> expire();
      primed_buff -> trigger();
    }
    else if ( primed_buff -> check() == 0 )
    {
      tracking_buff -> trigger();
    }
    else
    {
      action -> set_target( target );
      action -> execute();

      primed_buff -> expire();
    }
  }
};

double mage_spell_state_t::composite_crit_chance() const
{
  double c = action_state_t::composite_crit_chance();
  const mage_spell_t* spell = debug_cast<const mage_spell_t*>( action );
  const mage_t* p = spell -> p();

  if ( spell -> affected_by.shatter && p -> spec.shatter -> ok() && frozen() )
  {
    // Multiplier is not in spell data, apparently.
    c *= 1.5;

    c += p -> spec.shatter -> effectN( 2 ).percent() + p -> spec.shatter_2 -> effectN( 1 ).percent();
  }

  return c;
}

typedef residual_action::residual_periodic_action_t< mage_spell_t > residual_action_t;


// ============================================================================
// Arcane Mage Spell
// ============================================================================

struct arcane_mage_spell_t : public mage_spell_t
{
  bool triggers_erosion;

  arcane_mage_spell_t( const std::string& n, mage_t* p,
                       const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s ),
    triggers_erosion( true )
  {
    affected_by.arcane_mage = true;
    affected_by.erosion = true;
  }

  void init() override
  {
    mage_spell_t::init();

    if ( !harmful )
    {
      triggers_erosion = false;
    }
  }

  double savant_damage_bonus() const
  {
    return p() -> spec.arcane_charge -> effectN( 1 ).percent() +
      p() -> composite_mastery() * p() -> spec.savant -> effectN( 2 ).mastery_value();
  }

  void trigger_arcane_charge( int stacks = 1 )
  {
    buff_t* ac = p() -> buffs.arcane_charge;

    if ( p() -> bugs )
    {
      // The damage bonus given by mastery seems to be snapshot at the moment
      // Arcane Charge is gained. As long as the stack number remains the same,
      // any future changes to mastery will have no effect.
      // As of build 24461, 2017-07-03.
      if ( ac -> check() < ac -> max_stack() )
      {
        ac -> trigger( stacks, savant_damage_bonus() );
      }
    }
    else
    {
      ac -> trigger( stacks );
    }
  }

  double arcane_charge_damage_bonus( bool amplification = false ) const
  {
    double per_ac_bonus =
      p() -> bugs ? p() -> buffs.arcane_charge -> check_value() : savant_damage_bonus();

    if ( p() -> talents.amplification -> ok() && amplification )
    {
      per_ac_bonus += p() -> talents.amplification -> effectN( 1 ).percent();
    }

    return 1.0 + p() -> buffs.arcane_charge -> check() * per_ac_bonus;
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( p() -> talents.erosion -> ok() && result_is_hit( s -> result ) && triggers_erosion )
    {
      td( s -> target ) -> debuffs.erosion -> trigger();
    }
  }
};


// ============================================================================
// Fire Mage Spell
// ============================================================================

struct ignite_spell_state_t : public mage_spell_state_t
{
  bool hot_streak;

  ignite_spell_state_t( action_t* action, player_t* target ) :
    mage_spell_state_t( action, target ),
    hot_streak( false )
  { }

  virtual void initialize() override
  {
    mage_spell_state_t::initialize();
    hot_streak = false;
  }

  virtual std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    mage_spell_state_t::debug_str( s ) << " hot_streak=" << hot_streak;
    return s;
  }

  virtual void copy_state( const action_state_t* s ) override
  {
    mage_spell_state_t::copy_state( s );
    const ignite_spell_state_t* is =
      debug_cast<const ignite_spell_state_t*>( s );
    hot_streak = is -> hot_streak;
  }
};

struct fire_mage_spell_t : public mage_spell_t
{
  bool triggers_pyretic_incantation;
  bool triggers_hot_streak;
  bool triggers_ignite;

  fire_mage_spell_t( const std::string& n, mage_t* p,
                     const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s ),
    triggers_pyretic_incantation( false ),
    triggers_hot_streak( false ),
    triggers_ignite( false )
  {
    affected_by.fire_mage = true;
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( triggers_ignite && p() -> ignite )
      {
        trigger_ignite( s );
      }

      if ( triggers_hot_streak )
      {
        handle_hot_streak( s );
      }

      if ( triggers_pyretic_incantation && p() -> artifact.pyretic_incantation.rank() )
      {
        if ( s -> result == RESULT_CRIT )
        {
          p() -> buffs.pyretic_incantation -> trigger();
        }
        else
        {
          p() -> buffs.pyretic_incantation -> expire();
        }
      }
    }
  }

  void handle_hot_streak( action_state_t* s )
  {
    mage_t* p = this -> p();

    p -> procs.hot_streak_spell -> occur();

    if ( s -> result == RESULT_CRIT )
    {
      p -> procs.hot_streak_spell_crit -> occur();

      // Crit with HS => wasted crit
      if ( p -> buffs.hot_streak -> check() )
      {
        p -> procs.hot_streak_spell_crit_wasted -> occur();
      }
      else
      {
        // Crit with HU => convert to HS
        if ( p -> buffs.heating_up -> up() )
        {
          p -> procs.hot_streak -> occur();
          // Check if HS was triggered by IB
          if ( s -> action -> data().id() == 108853 )
          {
            p -> procs.heating_up_ib_converted -> occur();
          }

          p -> buffs.heating_up -> expire();
          p -> buffs.hot_streak -> trigger();

          //TODO: Add proc tracking to this to track from talent or non-talent sources.
          if ( p -> sets -> has_set_bonus( MAGE_FIRE, T19, B4 ) &&
               rng().roll( p -> sets -> set( MAGE_FIRE, T19, B4) -> effectN( 1 ).percent() ) )
          {
            p -> buffs.streaking -> trigger();
          }
        }
        // Crit without HU => generate HU
        else
        {
          p -> procs.heating_up_generated -> occur();
          p -> buffs.heating_up -> trigger();

          // Controlled Burn HU -> HS conversion
          if ( p -> talents.controlled_burn -> ok() &&
               rng().roll ( p -> talents.controlled_burn
                              -> effectN( 1 ).percent() ) )
          {
            p -> procs.controlled_burn -> occur();
            p -> buffs.heating_up -> expire();
            p -> buffs.hot_streak -> trigger();
            if ( p -> sets -> has_set_bonus( MAGE_FIRE, T19, B4 ) &&
                  rng().roll( p -> sets -> set( MAGE_FIRE, T19, B4 ) -> effectN( 1 ).percent() ) )
            {
              p -> buffs.streaking -> trigger();
            }
          }
        }
      }
    }
    else // Non-crit
    {
      // Non-crit with HU => remove HU
      if ( p -> buffs.heating_up -> check() )
      {
        if ( p -> buffs.heating_up -> elapsed( sim -> current_time() ) >
             timespan_t::from_millis( 200 ) )
        {
          p -> procs.heating_up_removed -> occur();
          p -> buffs.heating_up -> expire();

          if ( sim -> debug )
          {
            sim -> out_log.printf( "Heating up removed by non-crit" );
          }
        }
        else
        {
          if ( sim -> debug )
          {
            sim -> out_log.printf(
              "Heating up removal ignored due to 200 ms protection" );
          }
        }
      }
    }
  }

  virtual double composite_ignite_multiplier( const action_state_t* /* s */ ) const
  { return 1.0; }

  void trigger_ignite( action_state_t* s )
  {
    double amount = s -> result_amount * p() -> cache.mastery_value();

    // TODO: Use client data from hot streak
    amount *= composite_ignite_multiplier( s );
    amount *= 1.0 + p() -> artifact.everburning_consumption.percent();

    bool ignite_exists = p() -> ignite -> get_dot( s -> target ) -> is_ticking();

    residual_action::trigger( p() -> ignite, s -> target, amount );

    if ( !ignite_exists )
    {
      p() -> procs.ignite_applied -> occur();
    }
  }

  bool firestarter_active( player_t* target ) const
  {
    if ( ! p() -> talents.firestarter -> ok() )
      return false;

    // Check for user-specified override.
    if ( p() -> firestarter_time > timespan_t::zero() )
    {
      return sim -> current_time() < p() -> firestarter_time;
    }
    else
    {
      return target -> health_percentage() > p() -> talents.firestarter -> effectN( 1 ).base_value();
    }
  }

  // Helper methods for Contained Infernal Core.
  void trigger_infernal_core( player_t* target )
  {
    // As of PTR build 24287, 2017-06-08, casting Fireball and instant Pyroblast
    // while at 28 stacks removes tracking buff, then triggers primed buff and
    // one stack of tracking buff (instead of triggering the Meteor). Next cast
    // then expires the primed buff, triggers the Meteor and triggers a second
    // stack of the tracking buff.
    // Conversely, doing the same when only the primed buff is active only
    // triggers Meteor (it should also trigger one stack of the tracking buff).
    // TODO: Check this
    trigger_legendary_effect( p() -> buffs.contained_infernal_core,
                              p() -> buffs.erupting_infernal_core,
                              p() -> action.legendary_meteor,
                              target );
  }
};


// ============================================================================
// Frost Mage Spell
// ============================================================================
//

// Custom Frost Mage spell state to help with Impact damage calc and
// Fingers of Frost snapshots.
struct frost_spell_state_t : public mage_spell_state_t
{
  bool impact_override;

  frost_spell_state_t( action_t* action, player_t* target ) :
    mage_spell_state_t( action, target ),
    impact_override( false )
  { }

  virtual void initialize() override
  {
    mage_spell_state_t::initialize();

    impact_override = false;
  }

  virtual std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    mage_spell_state_t::debug_str( s ) << " impact_override=" << impact_override;
    return s;
  }

  virtual void copy_state( const action_state_t* s ) override
  {
    mage_spell_state_t::copy_state( s );
    auto fss = debug_cast<const frost_spell_state_t*>( s );

    impact_override = fss -> impact_override;
  }

  virtual bool frozen() const override
  {
    // In game, FoF Ice Lances are implemented using a global flag which determines
    // whether to treat the targets as frozen or not. On IL execute, FoF is checked
    // and the flag set accordingly.
    //
    // This works fine under normal circumstances. However, once GCD drops below IL's
    // travel time, it's possible to:
    //
    //   a) cast FoF IL, cast non-FoF IL before the first one impacts
    //   b) cast non-FoF IL, cast FoF IL before the first one impacts
    //
    // In the a) case, neither Ice Lance gets the extra damage/Shatter bonus, in the
    // b) case, both Ice Lances do.

    return mage_spell_state_t::frozen() ||
        ( action -> data().id() == 30455 &&
          debug_cast<mage_t*>( action -> player ) -> state.fingers_of_frost_active );
  }
};

struct frost_mage_spell_t : public mage_spell_t
{
  bool chills;
  bool calculate_on_impact;
  int fof_source_id;

  frost_mage_spell_t( const std::string& n, mage_t* p,
                      const spell_data_t* s = spell_data_t::nil() )
    : mage_spell_t( n, p, s ),
      chills( false ),
      calculate_on_impact( false ),
      fof_source_id( -1 )
  {
    affected_by.frost_mage = true;
    affected_by.shatter = true;
  }

  struct brain_freeze_delay_event_t : public event_t
  {
    mage_t* mage;

    brain_freeze_delay_event_t( mage_t* p, timespan_t delay )
      : event_t( *p, delay ), mage( p )
    { }

    const char* name() const override
    {
      return "brain_freeze_delay";
    }

    void execute() override
    {
      if ( mage -> sets -> has_set_bonus( MAGE_FROST, T20, B4 ) )
      {
        mage -> cooldowns.frozen_orb
             -> adjust( -100 * mage -> sets -> set( MAGE_FROST, T20, B4 ) -> effectN( 1 ).time_value() );
      }

      mage -> buffs.brain_freeze -> trigger();
    }
  };

  void trigger_fof( int source_id, double chance, int stacks = 1 )
  {
    bool success = p() -> buffs.fingers_of_frost
                       -> trigger( stacks, buff_t::DEFAULT_VALUE(), chance );

    if ( success )
    {
      if ( source_id < 0 )
      {
        p() -> sim -> out_debug.printf( "Action %s does not have valid fof source_id",
                                        name_str.c_str() );
      }
      else
      {
        p() -> benefits.fingers_of_frost -> update( source_id, stacks );
      }
    }
  }

  void trigger_brain_freeze( double chance )
  {
    if ( rng().roll( chance ) )
    {
      if ( p() -> buffs.brain_freeze -> check() )
      {
        // Brain Freeze was already active, delay the new application
        make_event<brain_freeze_delay_event_t>( *sim, p(), timespan_t::from_seconds( 0.15 ) );
      }
      else
      {
        if ( p() -> sets -> has_set_bonus( MAGE_FROST, T20, B4 ) )
        {
          p() -> cooldowns.frozen_orb
              -> adjust( -100 * p() -> sets -> set( MAGE_FROST, T20, B4 ) -> effectN( 1 ).time_value() );
        }

        p() -> buffs.brain_freeze -> trigger();
      }
    }
  }

  virtual action_state_t* new_state() override
  {
    return new frost_spell_state_t( this, target );
  }

  static const frost_spell_state_t* cast_state( const action_state_t* st )
  {
    return debug_cast<const frost_spell_state_t*>( st );
  }

  static frost_spell_state_t* cast_state( action_state_t* st )
  {
    return debug_cast<frost_spell_state_t*>( st );
  }

  void trigger_icicle_gain( action_state_t* state, stats_t* stats )
  {
    if ( ! p() -> spec.icicles -> ok() )
      return;

    if ( ! result_is_hit( state -> result ) )
      return;

    double m = state -> target_da_multiplier;

    // Invulnerability event may make it so that there's no damage associated with the icicle
    // trigger. In that case, don't trigger any icicle gains.
    if ( m == 0 )
    {
      return;
    }

    double amount = state -> result_amount / m * p() -> cache.mastery_value();
    if ( p() -> artifact.black_ice.rank() && rng().roll( 0.2 ) )
    {
      amount *= 2;
    }
    if ( p() -> talents.splitting_ice -> ok() )
    {
      amount *= 1.0 + p() -> talents.splitting_ice -> effectN( 3 ).percent();
    }

    assert( as<int>( p() -> icicles.size() ) <=
            p() -> spec.icicles -> effectN( 2 ).base_value() );

    // Shoot one if capped
    if ( as<int>( p() -> icicles.size() ) ==
         p() -> spec.icicles -> effectN( 2 ).base_value() )
    {
      p() -> trigger_icicle( state );
    }

    icicle_tuple_t tuple{ p() -> sim -> current_time(),
                          icicle_data_t{ amount, stats } };
    p() -> icicles.push_back( tuple );

    if ( p() -> sim -> debug )
    {
      p() -> sim -> out_debug.printf( "%s icicle gain, damage=%f, total=%u",
                                      p() -> name(),
                                      amount,
                                      as<unsigned>( p() -> icicles.size() ) );
    }
  }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    if ( !calculate_on_impact || cast_state( s ) -> impact_override )
    {
      return mage_spell_t::calculate_direct_amount( s );
    }
    else
    {
      return s -> result_amount;
    }
  }

  virtual result_e calculate_result( action_state_t* s ) const override
  {
    if ( !calculate_on_impact || cast_state( s ) -> impact_override )
    {
      return mage_spell_t::calculate_result( s );
    }
    else
    {
      return s -> result;
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    if ( calculate_on_impact )
    {
      // Swap our flag to allow damage calculation again
      cast_state( s ) -> impact_override = true;

      // Re-call functions here, before the impact call to do the damage calculations as we impact.
      snapshot_state( s, amount_type( s ) );

      s -> result = calculate_result( s );
      s -> result_amount = calculate_direct_amount( s );
    }

    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) && chills && p() -> talents.bone_chilling -> ok() )
    {
      p() -> buffs.bone_chilling -> trigger();
    }
  }

  // Helper methods for Shattered Fragments of Sindragosa.
  void trigger_shattered_fragments( player_t* target )
  {
    trigger_legendary_effect( p() -> buffs.shattered_fragments_of_sindragosa,
                              p() -> buffs.rage_of_the_frost_wyrm,
                              p() -> action.legendary_comet_storm,
                              target );
  }
};

// Icicles ==================================================================

struct icicle_state_t : public mage_spell_state_t
{
  stats_t* source;

  icicle_state_t( action_t* action, player_t* target ) :
    mage_spell_state_t( action, target ), source( nullptr )
  { }

  void initialize() override
  { mage_spell_state_t::initialize(); source = nullptr; }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  { mage_spell_state_t::debug_str( s ) << " source=" << ( source ? source -> name_str : "unknown" ); return s; }

  void copy_state( const action_state_t* other ) override
  {
    mage_spell_state_t::copy_state( other );

    source = debug_cast<const icicle_state_t*>( other ) -> source;
  }
};

struct icicle_t : public frost_mage_spell_t
{
  icicle_t( mage_t* p ) :
    frost_mage_spell_t( "icicle", p, p -> find_spell( 148022 ) )
  {
    may_crit = false;
    proc = background = true;

    if ( p -> talents.splitting_ice -> ok() )
    {
      aoe = 1 + p -> talents.splitting_ice -> effectN( 1 ).base_value();
      base_aoe_multiplier *= p -> talents.splitting_ice
                               -> effectN( 2 ).percent();
    }
  }

  // To correctly record damage and execute information to the correct source
  // action, we set the stats object of the icicle cast to the source stats object,
  // carried from trigger_icicle() to here through the execute_event_t.
  void execute() override
  {
    const icicle_state_t* is = debug_cast<const icicle_state_t*>( pre_execute_state );
    assert( is -> source );
    stats = is -> source;

    frost_mage_spell_t::execute();
  }

  // And again, once the icicle impacts, we set the stats object here again
  // because multiple icicles can be executing, causing the state object to be
  // set to another object between the execution of this specific icicle, and
  // the impact.
  void impact( action_state_t* state ) override
  {
    const icicle_state_t* is = debug_cast<const icicle_state_t*>( state );
    assert( is -> source );
    stats = is -> source;

    frost_mage_spell_t::impact( state );
  }

  action_state_t* new_state() override
  { return new icicle_state_t( this, target ); }

  void init() override
  {
    frost_mage_spell_t::init();

    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_TGT_MUL_DA;
  }
};

// Presence of Mind Spell ===================================================

struct presence_of_mind_t : public arcane_mage_spell_t
{
  presence_of_mind_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "presence_of_mind", p,
                         p -> find_specialization_spell( "Presence of Mind" )  )
  {
    parse_options( options_str );
    harmful = false;
  }

  virtual bool ready() override
  {
    if ( p() -> buffs.presence_of_mind -> check() )
    {
      return false;
    }

    return arcane_mage_spell_t::ready();
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();

    p() -> buffs.presence_of_mind
        -> trigger( p() -> buffs.presence_of_mind -> max_stack() );

    if ( p() -> sets -> has_set_bonus( MAGE_ARCANE, T20, B2 ) )
    {
      trigger_arcane_charge( 4 );
      p() -> buffs.crackling_energy -> trigger();
    }
  }
};

// Conflagration Spell =====================================================
struct conflagration_dot_t : public fire_mage_spell_t
{
  conflagration_dot_t( mage_t* p ) :
    fire_mage_spell_t( "conflagration_dot", p, p -> find_spell( 226757 ) )
  {
    //TODO: Check callbacks
    hasted_ticks = false;
    tick_may_crit = may_crit = false;
    background = true;
  }
};

struct conflagration_t : public fire_mage_spell_t
{
  conflagration_t( mage_t* p ) :
    fire_mage_spell_t( "conflagration_explosion", p, p -> talents.conflagration )
  {
    parse_effect_data( p -> find_spell( 205345 ) -> effectN( 1 ) );
    callbacks = false;
    background = true;
    aoe = -1;
  }
};

// Ignite Spell ===================================================================

//Phoenix Reborn Spell
struct phoenix_reborn_t : public fire_mage_spell_t
{
  phoenix_reborn_t( mage_t* p ) :
    fire_mage_spell_t( "phoenix_reborn", p, p -> artifact.phoenix_reborn )
  {
    parse_effect_data( p -> find_spell( 215775 ) -> effectN( 1 ) );
    callbacks = false;
    background = true;
    internal_cooldown -> duration = p -> find_spell( 215773 ) -> internal_cooldown();
  }
  virtual void execute() override
  {
    if ( internal_cooldown -> down() )
      return;

    fire_mage_spell_t::execute();
    internal_cooldown -> start();
    p() -> cooldowns.phoenixs_flames -> adjust( -1000 * data().effectN( 1 ).time_value() );
  }

};

struct ignite_t : public residual_action_t
{
  conflagration_t* conflagration;
  phoenix_reborn_t* phoenix_reborn;

  ignite_t( mage_t* player ) :
    residual_action_t( "ignite", player, player -> find_spell( 12846 ) ),
    conflagration( nullptr ),
    phoenix_reborn( nullptr )
  {
    dot_duration = dbc::find_spell( player, 12654 ) -> duration();
    base_tick_time = dbc::find_spell( player, 12654 ) -> effectN( 1 ).period();
    school = SCHOOL_FIRE;

    //!! NOTE NOTE NOTE !! This is super dangerous and means we have to be extra careful with correctly
    // flagging thats that proc off events, to not proc off ignite if they shouldn't!
    callbacks = true;

    if ( player -> talents.conflagration -> ok() )
    {
      conflagration = new conflagration_t( player );
    }

    if ( player -> artifact.phoenix_reborn.rank() )
    {
      phoenix_reborn = new phoenix_reborn_t( player );
    }
  }

  void tick( dot_t* dot ) override
  {
    residual_action_t::tick( dot );

    if ( p() -> talents.conflagration -> ok() &&
         rng().roll( p() -> talents.conflagration -> effectN( 1 ).percent() ) )
    {
      conflagration -> set_target( dot -> target );
      conflagration -> execute();
    }

    if ( p() -> artifact.phoenix_reborn.rank() &&
         rng().roll( p() -> artifact.phoenix_reborn.data().proc_chance() ) )
    {
      phoenix_reborn -> set_target( dot -> target );
      phoenix_reborn -> execute();
    }
  }
};


// Aegwynn's Ascendance Spell =================================================
struct aegwynns_ascendance_t : public arcane_mage_spell_t
{
  aegwynns_ascendance_t( mage_t* p ) :
    arcane_mage_spell_t( "aegwynns_ascendance", p,
                         p -> find_spell( 187677 ) )
  {
    callbacks = false;
    aoe = -1;
    background = true;
    may_crit = false;

    affected_by.erosion = false;
  }

  virtual void init() override
  {
    arcane_mage_spell_t::init();
    // disable the snapshot_flags for all multipliers
    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_TGT_MUL_DA;
  }
};

// Arcane Barrage Spell =======================================================

// Arcane Rebound Spell
//TODO: Improve timing of impact of this vs Arcane Barrage if alpha timings go live
struct arcane_rebound_t : public arcane_mage_spell_t
{
  arcane_rebound_t( mage_t* p ) :
    arcane_mage_spell_t( "arcane_rebound", p, p -> find_spell( 210817 ) )
  {
    background = true;
    callbacks = false; // TODO: Is this true?
    aoe = -1;
  }

  virtual timespan_t travel_time() const override
  {
    // Hardcode no travel time to avoid parsed travel time in spelldata
    return timespan_t::zero();
  }

};
struct arcane_barrage_t : public arcane_mage_spell_t
{
  arcane_rebound_t* arcane_rebound;

  double mystic_kilt_of_the_rune_master_regen;
  double mantle_of_the_first_kirin_tor_chance;

  arcane_barrage_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_barrage", p, p -> find_specialization_spell( "Arcane Barrage" ) ),
    arcane_rebound( new arcane_rebound_t( p ) ),
    mystic_kilt_of_the_rune_master_regen( 0.0 ),
    mantle_of_the_first_kirin_tor_chance( 0.0 )
  {
    parse_options( options_str );
    base_aoe_multiplier *= data().effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> artifact.torrential_barrage.percent();
    cooldown -> hasted = true;
    add_child( arcane_rebound );
    if ( p -> action.legendary_arcane_orb )
    {
      add_child( p -> action.legendary_arcane_orb );
    }
  }

  virtual void execute() override
  {
    // Mantle of the First Kirin Tor has some really weird interactions. When ABar is cast, the number
    // of targets is decided first, then the roll for Arcane Orb happens. If it succeeds, Orb is cast
    // and the mage gains an Arcane Charge. This extra charge counts towards the bonus damage and also
    // towards Mystic Kilt of the Rune Master. After everything is done, Arcane Charges are reset.
    //
    // Hard to tell which part (if any) is a bug.
    // TODO: Check this.
    int charges = p() -> buffs.arcane_charge -> check();
    aoe = ( charges == 0 ) ? 0 : 1 + charges;

    if ( rng().roll( mantle_of_the_first_kirin_tor_chance * charges ) )
    {
      assert( p() -> action.legendary_arcane_orb );
      p() -> action.legendary_arcane_orb -> set_target( target );
      p() -> action.legendary_arcane_orb -> execute();

      // Update charges for Mystic Kilt of the Rune Master mana gain.
      charges = p() -> buffs.arcane_charge -> check();
    }

    p() -> benefits.arcane_charge.arcane_barrage -> update();

    if ( mystic_kilt_of_the_rune_master_regen > 0 && charges > 0 )
    {
      p() -> resource_gain(
        RESOURCE_MANA,
        charges * mystic_kilt_of_the_rune_master_regen * p() -> resources.max[ RESOURCE_MANA ],
        p() -> gains.mystic_kilt_of_the_rune_master );
    }

    arcane_mage_spell_t::execute();

    p() -> buffs.arcane_charge -> expire();
  }

  virtual void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( p() -> talents.chrono_shift -> ok() )
    {
      p() -> buffs.chrono_shift -> trigger();
    }

    if ( p() -> artifact.arcane_rebound.rank() && s -> n_targets > 2 && s -> chain_target == 0 )
    {
      arcane_rebound -> set_target( s -> target );
      arcane_rebound -> execute();
    }
  }

  virtual double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_damage_bonus();

    if ( p() -> talents.resonance -> ok() )
    {
      int targets = std::min( n_targets(), as<int>( target_list().size() ) );
      am *= 1.0 + p() -> talents.resonance -> effectN( 1 ).percent() * targets;
    }

    return am;
  }
};


// Arcane Blast Spell =======================================================

struct arcane_blast_t : public arcane_mage_spell_t
{
  struct touch_of_the_magi_t
  {
    mage_t* mage;
    const spell_data_t* data;
    cooldown_t* icd;

    touch_of_the_magi_t( mage_t* p ) :
      mage( p ),
      data( p -> find_spell( 210725 ) ),
      icd( p -> get_cooldown( "touch_of_the_magi_icd" ) )
    { }

    bool trigger_if_up( action_state_t* s )
    {
      if ( icd -> down() )
      {
        return false;
      }

      buff_t* touch = mage -> get_target_data( s -> target ) -> debuffs.touch_of_the_magi;
      bool triggered = touch -> trigger( 1, buff_t::DEFAULT_VALUE(),
                                         data -> proc_chance() );

      if ( triggered )
      {
        icd -> start( data -> internal_cooldown() );
      }

      return triggered;
    }
  } * touch_of_the_magi;

  arcane_blast_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_blast", p,
                         p -> find_specialization_spell( "Arcane Blast" ) )
  {
    parse_options( options_str );
    triggers_arcane_missiles = false; // Disable default AM proc logic.
    base_multiplier *= 1.0 + p -> artifact.blasting_rod.percent();

    if ( p -> artifact.touch_of_the_magi.rank() )
    {
      touch_of_the_magi = new touch_of_the_magi_t( p );
    }
    if ( p -> action.unstable_magic_explosion )
    {
      add_child( p -> action.unstable_magic_explosion );
    }
  }

  virtual bool init_finished() override
  {
    am_trigger_source_id = p() -> benefits.arcane_missiles
                               -> get_source_id( data().name_cstr() );

    return arcane_mage_spell_t::init_finished();
  }

  virtual double cost() const override
  {
    double c = arcane_mage_spell_t::cost();

    c *= 1.0 +  p() -> buffs.arcane_charge -> check() *
                p() -> spec.arcane_charge -> effectN( 5 ).percent();

    //TODO: Find a work-around to remove hardcoding
    if ( p() -> buffs.rhonins_assaulting_armwraps -> check() )
    {
      c = 0;
    }
    return c;
  }

  virtual void execute() override
  {
    p() -> benefits.arcane_charge.arcane_blast -> update();
    arcane_mage_spell_t::execute();

    p() -> buffs.rhonins_assaulting_armwraps -> expire();

    p() -> buffs.arcane_charge -> up();

    if ( hit_any_target )
    {
      trigger_am( am_trigger_source_id,
                  p() -> buffs.arcane_missiles -> proc_chance() * 2.0 );

      trigger_arcane_charge();
    }

    if ( p() -> buffs.presence_of_mind -> up() )
    {
      p() -> buffs.presence_of_mind -> decrement();
    }

    p() -> trigger_t19_oh();
  }

  virtual double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_damage_bonus();

    return am;
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buffs.presence_of_mind -> check() )
    {
      return timespan_t::zero();
    }

    timespan_t t = arcane_mage_spell_t::execute_time();

    if ( p() -> talents.temporal_flux -> ok() )
    {
      t *=  1.0 + p() -> buffs.arcane_charge -> stack() *
                  p() -> talents.temporal_flux -> effectN( 1 ).percent();
    }

    return t;
  }

  virtual void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_unstable_magic( s );

      if ( p() -> artifact.touch_of_the_magi.rank() )
      {
        touch_of_the_magi -> trigger_if_up( s );
      }
    }
  }

};

// Arcane Explosion Spell =====================================================

struct time_and_space_t : public arcane_mage_spell_t
{
  time_and_space_t( mage_t* p ) :
    arcane_mage_spell_t( "time_and_space", p, p -> find_spell( 240689 ) )
  {
    aoe = -1;
    background = true;

    // All other background actions trigger Erosion.
    // As of build 24461, 2017-07-03.
    if ( p -> bugs )
    {
      triggers_erosion = false;
    }

    base_multiplier *= 1.0 + p -> artifact.arcane_purification.percent();
    radius += p -> artifact.crackling_energy.data().effectN( 1 ).base_value();
  }

  virtual double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_damage_bonus();

    return am;
  }
};

struct arcane_explosion_t : public arcane_mage_spell_t
{
  time_and_space_t* time_and_space;

  arcane_explosion_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_explosion", p,
                         p -> find_specialization_spell( "Arcane Explosion" ) ),
    time_and_space( nullptr )
  {
    parse_options( options_str );
    aoe = -1;
    base_multiplier *= 1.0 + p -> artifact.arcane_purification.percent();
    radius += p -> artifact.crackling_energy.data().effectN( 1 ).base_value();

    if ( p -> artifact.time_and_space.rank() )
    {
      time_and_space = new time_and_space_t( p );
      add_child( time_and_space );
    }
  }

  virtual void execute() override
  {
    p() -> benefits.arcane_charge.arcane_explosion -> update();

    arcane_mage_spell_t::execute();

    p() -> buffs.arcane_charge -> up();

    if ( hit_any_target )
    {
      trigger_arcane_charge();
    }

    if ( p() -> artifact.time_and_space.rank() )
    {
      if ( p() -> buffs.time_and_space -> check() )
      {
        make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
          .pulse_time( timespan_t::from_seconds( 0.25 ) )
          .target( execute_state -> target )
          .n_pulses( 1 )
          .action( time_and_space ) );

        p() -> buffs.time_and_space -> trigger();
      }
      else
      {
        p() -> buffs.time_and_space -> trigger();
      }
    }
  }

  virtual double cost() const override
  {
    double c = arcane_mage_spell_t::cost();

    c *= 1.0 +  p() -> buffs.arcane_charge -> check() *
                p() -> spec.arcane_charge -> effectN( 5 ).percent();

    return c;
  }

  virtual double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_damage_bonus();

    return am;
  }
};


// Arcane Missiles Spell ====================================================

struct arcane_missiles_tick_t : public arcane_mage_spell_t
{
  arcane_missiles_tick_t( mage_t* p ) :
    arcane_mage_spell_t( "arcane_missiles_tick", p,
                         p -> find_specialization_spell( "Arcane Missiles" )
                           -> effectN( 2 ).trigger() )
  {
    background  = true;
  }

  void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    p() -> buffs.cord_of_infinity -> trigger();
  }
};

struct am_state_t : public mage_spell_state_t
{
  bool rule_of_threes;

  am_state_t( action_t* action, player_t* target ) :
    mage_spell_state_t( action, target ), rule_of_threes( false )
  { }

  void initialize() override
  { mage_spell_state_t::initialize(); rule_of_threes = false; }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    mage_spell_state_t::debug_str( s )
      << " rule_of_threes=" << rule_of_threes;
    return s;
  }

  void copy_state( const action_state_t* other ) override
  {
    mage_spell_state_t::copy_state( other );

    rule_of_threes = debug_cast<const am_state_t*>( other ) -> rule_of_threes;
  }
};

struct arcane_missiles_t : public arcane_mage_spell_t
{
  double rule_of_threes_ticks, rule_of_threes_ratio;

  arcane_missiles_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_missiles", p,
                         p -> find_specialization_spell( "Arcane Missiles" ) )
  {
    parse_options( options_str );
    may_miss = false;
    triggers_arcane_missiles = false;
    triggers_erosion = false;
    dot_duration      = data().duration();
    base_tick_time    = data().effectN( 2 ).period();
    channeled         = true;
    hasted_ticks      = false;
    dynamic_tick_action = true;
    tick_action = new arcane_missiles_tick_t( p );

    base_multiplier *= 1.0 + p -> artifact.aegwynns_fury.percent();
    base_crit += p -> artifact.aegwynns_intensity.percent();

    rule_of_threes_ticks = dot_duration / base_tick_time +
      p -> artifact.rule_of_threes.data().effectN( 2 ).base_value();
    rule_of_threes_ratio = ( dot_duration / base_tick_time ) / rule_of_threes_ticks;
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_damage_bonus( true );

    return am;
  }

  // Flag Arcane Missiles as direct damage for triggering effects
  dmg_e amount_type( const action_state_t* /* state */, bool /* periodic */ ) const override
  {
    return DMG_DIRECT;
  }

  action_state_t* new_state() override
  { return new am_state_t( this, target ); }

  // Roll (and snapshot) Rule of Threes here, it affects the whole AM channel.
  void snapshot_state( action_state_t* state, dmg_e rt ) override
  {
    arcane_mage_spell_t::snapshot_state( state, rt );

    if ( rng().roll( p() -> artifact.rule_of_threes.data().effectN( 1 ).percent() / 10.0 ) )
    {
      debug_cast<am_state_t*>( state ) -> rule_of_threes = true;
    }
  }

  // If Rule of Threes is used, return the channel duration in terms of number
  // of ticks, so we prevent weird issues with rounding on duration
  timespan_t composite_dot_duration( const action_state_t* state ) const override
  {
    auto s = debug_cast<const am_state_t*>( state );

    if ( s -> rule_of_threes )
    {
      return tick_time( state ) * rule_of_threes_ticks;
    }
    else
    {
      return arcane_mage_spell_t::composite_dot_duration( state );
    }
  }

  // Adjust tick time on Rule of Threes
  timespan_t tick_time( const action_state_t* state ) const override
  {
    auto s = debug_cast<const am_state_t*>( state );

    if ( s -> rule_of_threes )
    {
      return base_tick_time * rule_of_threes_ratio * state -> haste;
    }
    else
    {
      return arcane_mage_spell_t::tick_time( state );
    }
  }

  void execute() override
  {
    p() -> benefits.arcane_charge.arcane_missiles -> update();

    arcane_mage_spell_t::execute();

    p() -> buffs.rhonins_assaulting_armwraps -> trigger();

    if ( p() -> sets -> has_set_bonus( MAGE_ARCANE, T19, B4 ) )
    {
      p() -> cooldowns.evocation
          -> adjust( -1000 * p() -> sets -> set( MAGE_ARCANE, T19, B4 ) -> effectN( 1 ).time_value()  );
    }
    if ( p() -> sets -> has_set_bonus( MAGE_ARCANE, T20, B4 ) )
    {
      p() -> cooldowns.presence_of_mind
          -> adjust( -100 * p() -> sets -> set( MAGE_ARCANE, T20, B4 ) -> effectN( 1 ).time_value() );
    }

    p() -> buffs.arcane_missiles -> decrement();
  }

  void last_tick ( dot_t * d ) override
  {
    arcane_mage_spell_t::last_tick( d );

    trigger_arcane_charge();
  }

  bool ready() override
  {
    if ( ! p() -> buffs.arcane_missiles -> check() )
      return false;

    return arcane_mage_spell_t::ready();
  }

  bool usable_moving() const override
  {
    if ( p() -> talents.slipstream -> ok() )
      return true;

    return arcane_mage_spell_t::usable_moving();
  }
};

// Arcane Orb Spell ===========================================================

struct arcane_orb_bolt_t : public arcane_mage_spell_t
{
  arcane_orb_bolt_t( mage_t* p, bool legendary ) :
    arcane_mage_spell_t( legendary ? "legendary_arcane_orb_bolt" : "arcane_orb_bolt",
                         p, p -> find_spell( 153640 ) )
  {
    aoe = -1;
    background = true;
  }

  virtual bool init_finished() override
  {
    am_trigger_source_id = p() -> benefits.arcane_missiles
                               -> get_source_id( "Arcane Orb Impact" );

    return arcane_mage_spell_t::init_finished();
  }

  virtual void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_arcane_charge();
      trigger_am( am_trigger_source_id );
    }
  }
};

struct arcane_orb_t : public arcane_mage_spell_t
{
  arcane_orb_bolt_t* orb_bolt;

  arcane_orb_t( mage_t* p, const std::string& options_str, bool legendary = false ) :
    arcane_mage_spell_t( legendary ? "legendary_arcane_orb" : "arcane_orb", p,
                         p -> find_talent_spell( "Arcane Orb", SPEC_NONE, false, ! legendary ) ),
    orb_bolt( new arcane_orb_bolt_t( p, legendary ) )
  {
    parse_options( options_str );

    may_miss = false;
    may_crit = false;
    triggers_erosion = false;
    // Needs to be handled manually to account for the legendary shoulders.
    triggers_arcane_missiles = false;

    if ( legendary )
    {
      background = true;
      base_costs[ RESOURCE_MANA ] = 0;
    }

    add_child( orb_bolt );
  }

  virtual bool init_finished() override
  {
    if ( data().ok() )
    {
      am_trigger_source_id = p() -> benefits.arcane_missiles
                                 -> get_source_id( data().name_cstr() );
    }

    return arcane_mage_spell_t::init_finished();
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();

    trigger_am( am_trigger_source_id );
    trigger_arcane_charge();
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds(
      std::max( 0.1, ( player -> get_player_distance( *target ) - 10.0 ) / 16.0 ) );
  }

  virtual void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    orb_bolt -> execute();
  }
};

// Arcane Power Spell =======================================================

struct arcane_power_t : public arcane_mage_spell_t
{
  arcane_power_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_power", p,
                         p -> find_specialization_spell( "Arcane Power" ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();
    p() -> buffs.arcane_power -> trigger();
  }
};

// Blast Wave Spell ==========================================================

struct blast_wave_t : public fire_mage_spell_t
{
  blast_wave_t( mage_t* p, const std::string& options_str ) :
     fire_mage_spell_t( "blast_wave", p, p -> talents.blast_wave )
  {
    parse_options( options_str );
    triggers_pyretic_incantation = true;
    aoe = -1;
  }
};


// Blink Spell ==============================================================

struct blink_t : public mage_spell_t
{
  blink_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "blink", p, p -> find_class_spell( "Blink" ) )
  {
    parse_options( options_str );

    harmful = false;
    ignore_false_positive = true;
    base_teleport_distance = 20;

    movement_directionality = MOVEMENT_OMNI;

    if ( p -> talents.shimmer -> ok() )
    {
      background = true;
    }
  }
};


// Blizzard Spell ===========================================================

struct blizzard_shard_t : public frost_mage_spell_t
{
  blizzard_shard_t( mage_t* p ) :
    frost_mage_spell_t( "blizzard_shard", p, p -> find_spell( 190357 ) )
  {
    aoe = -1;
    background = true;
    ground_aoe = true;
    base_multiplier *= 1.0 + p -> talents.arctic_gale -> effectN( 1 ).percent();
    base_crit += p -> artifact.the_storm_rages.percent();
    chills = true;
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
        p() -> cooldowns.frozen_orb
            -> adjust( -10.0 * p() -> spec.blizzard_2
                                   -> effectN( 1 ).time_value() );
    }
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double cpm = frost_mage_spell_t::composite_persistent_multiplier( s );

    cpm *= 1.0 + p() -> buffs.zannesu_journey -> check_stack_value();

    return cpm;
  }
};

struct blizzard_t : public frost_mage_spell_t
{
  blizzard_shard_t* blizzard_shard;

  blizzard_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "blizzard", p, p -> find_specialization_spell( "Blizzard" ) ),
    blizzard_shard( new blizzard_shard_t( p ) )
  {
    parse_options( options_str );
    add_child( blizzard_shard );
    cooldown -> hasted = true;
    dot_duration = timespan_t::zero(); // This is just a driver for the ground effect.
    may_miss = false;
  }

  double false_positive_pct() const override
  {
    // Players are probably less likely to accidentally use blizzard than other spells.
    return ( frost_mage_spell_t::false_positive_pct() / 2 );
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buffs.freezing_rain -> check() )
    {
      return timespan_t::zero();
    }

    return frost_mage_spell_t::execute_time();
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    timespan_t ground_aoe_duration = data().duration() * player -> cache.spell_speed();
    p() -> ground_aoe_expiration[ name_str ]
      = sim -> current_time() + ground_aoe_duration;

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .target( execute_state -> target )
      .duration( ground_aoe_duration )
      .action( blizzard_shard )
      .hasted( ground_aoe_params_t::SPELL_SPEED ) );

    p() -> buffs.zannesu_journey -> expire();
  }
};

// Charged Up Spell =========================================================

struct charged_up_t : public arcane_mage_spell_t
{
  charged_up_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "charged_up", p, p -> talents.charged_up )
  {
    parse_options( options_str );
    harmful = false;
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();

    trigger_arcane_charge( 4 );
  }
};


// Cinderstorm Spell ==========================================================
// Cinderstorm travel mechanism:
// http://blue.mmo-champion.com/topic/409203-theorycrafting-questions/#post114
// "9.17 degrees" is assumed to be a rounded value of 0.16 radians.
// For distance k and deviation angle x, the arclength is k * x / sin(x).
// From testing, cinders have a variable velocity, averaging ~30 yards/second.

struct cinder_t : public fire_mage_spell_t
{
  cinder_t( mage_t* p ) :
    fire_mage_spell_t( "cinder", p, p -> find_spell( 198928 ) )
  {
    background = true;
    aoe = -1;
    triggers_ignite = true;
    //TODO: Revisit this once skullflower confirms intended behavior.
    triggers_pyretic_incantation = true;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = fire_mage_spell_t::composite_target_multiplier( target );

    if ( p() -> ignite -> get_dot( target ) -> is_ticking() )
    {
      m *= 1.0 + p() -> talents.cinderstorm -> effectN( 1 ).percent();
    }

    return m;
  }
};

struct cinderstorm_t : public fire_mage_spell_t
{
  cinder_t* cinder;
  int cinder_count;

  const double cinder_velocity_mean = 30.0; // Yards per second
  const double cinder_velocity_range = 6.0; // Yards per second
  const double cinder_converge_mean = 31.0; // Yards
  const double cinder_converge_range = 2.0; // Yards
  const double cinder_angle = 0.16; // Radians

  cinderstorm_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "cinderstorm", p, p -> talents.cinderstorm ),
    cinder( new cinder_t( p ) ),
    cinder_count( 6 )
  {
    add_option( opt_int( "cinders", cinder_count ) );
    parse_options( options_str );

    cooldown -> hasted = true;
    add_child( cinder );
  }

  virtual void execute() override
  {
    if ( p() -> global_cinder_count > 0 )
    {
      cinder_count = static_cast<int>( p() -> global_cinder_count );
    }
    fire_mage_spell_t::execute();

    double target_dist = player -> get_player_distance( *execute_state -> target );
    double cinder_converge_distance =
      rng().range( cinder_converge_mean - cinder_converge_range,
                   cinder_converge_mean + cinder_converge_range );

    // When cinder_count < 6, we assume "curviest" cinders are first to miss
    for ( int i = 1; i <= cinder_count; i++ )
    {
      // TODO: Optimize this code by caching theta and trig functions
      timespan_t travel_time;

      // Cinder deviation angle from "forward"
      double theta = cinder_angle * i;
      // Radius of arc drawn by cinder
      double radius = cinder_converge_distance / ( 2.0 * sin( theta ) );
      // Randomized cinder velocity
      double cinder_velocity =
        rng().range( cinder_velocity_mean - cinder_velocity_range,
                     cinder_velocity_mean + cinder_velocity_range);

      if ( target_dist > cinder_converge_distance )
      {
        // Time spent "curving around"
        timespan_t arc_time =
          timespan_t::from_seconds( radius * 2 * theta / cinder_velocity );
        // Time spent travelling straight at an angle, after curving
        timespan_t straight_time = timespan_t::from_seconds(
          // Residual distance beyond point of convergence
          ( target_dist - cinder_converge_distance ) /
          // Divided by magnitude of velocity in forward direction
          ( cinder_velocity * cos( theta ) )
        );
        // Travel time is equal to the sum of traversing arc and straight path
        travel_time = arc_time + straight_time;
      }
      else
      {
        // Use Cinderstorm's arc's symmetry to simplify calculations
        // First calculate the offset distance and angle from halfway
        double offset_dist = target_dist - ( cinder_converge_distance / 2.0 );
        double offset_angle = asin( offset_dist / radius );
        // Using this offset, we calculate the arc angle traced before impact,
        // which also gives us arc length
        double arc_angle = theta + offset_angle;
        double arc_dist = radius * arc_angle;
        // Divide by cinder velocity to obtain travel time
        travel_time = timespan_t::from_seconds( arc_dist / cinder_velocity );
      }

      make_event<events::cinder_impact_event_t>( *sim, *p(), cinder, target,
                                                  travel_time);
    }
  }
};

// Cold Snap Spell ============================================================

struct cold_snap_t : public frost_mage_spell_t
{
  cold_snap_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "cold_snap", p, p -> find_specialization_spell( "Cold Snap" ) )
  {
    parse_options( options_str );
    harmful = false;
  };

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    p() -> cooldowns.cone_of_cold -> reset( false );
    p() -> cooldowns.frost_nova -> reset( false );
  }
};


// Combustion Spell ===========================================================

struct combustion_t : public fire_mage_spell_t
{
  combustion_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "combustion", p, p -> find_specialization_spell( "Combustion" ) )
  {
    parse_options( options_str );
    dot_duration = timespan_t::zero();
    harmful = false;
  }

  virtual void execute() override
  {
    fire_mage_spell_t::execute();

    p() -> buffs.combustion -> trigger();
  }
};


// Comet Storm Spell =======================================================

struct comet_storm_projectile_t : public frost_mage_spell_t
{
  comet_storm_projectile_t( mage_t* p, bool legendary ) :
    frost_mage_spell_t( legendary ? "legendary_comet_storm_projectile" : "comet_storm_projectile", p,
                        p -> find_spell( 153596 ) )
  {
    aoe = -1;
    background = true;
  }
};

struct comet_storm_t : public frost_mage_spell_t
{
  comet_storm_projectile_t* projectile;

  comet_storm_t( mage_t* p, const std::string& options_str, bool legendary = false ) :
    frost_mage_spell_t( legendary ? "legendary_comet_storm" : "comet_storm",
                        p, p -> find_talent_spell( "Comet Storm", SPEC_NONE, false, ! legendary ) ),
    projectile( new comet_storm_projectile_t( p, legendary ) )
  {
    parse_options( options_str );
    may_miss = false;
    add_child( projectile );

    if ( legendary )
    {
      background = true;
      base_costs[ RESOURCE_MANA ] = 0;
    }
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.0 );
  }

  void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    timespan_t ground_aoe_duration = timespan_t::from_seconds( 1.2 );
    p() -> ground_aoe_expiration[ name_str ]
      = sim -> current_time() + ground_aoe_duration;

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .pulse_time( timespan_t::from_seconds( 0.2 ) )
      .target( s -> target )
      .duration( ground_aoe_duration )
      .action( projectile ), true );
  }
};


// Cone of Cold Spell =======================================================

struct cone_of_cold_t : public frost_mage_spell_t
{
  cone_of_cold_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "cone_of_cold", p,
                        p -> find_specialization_spell( "Cone of Cold" ) )
  {
    parse_options( options_str );
    aoe = -1;
    chills = true;
  }
};


// Counterspell Spell =======================================================

struct counterspell_t : public mage_spell_t
{
  counterspell_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "counterspell", p, p -> find_class_spell( "Counterspell" ) )
  {
    parse_options( options_str );
    may_miss = may_crit = false;
    ignore_false_positive = true;
    triggers_arcane_missiles = false;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();
    p() -> apply_crowd_control( execute_state, MECHANIC_INTERRUPT );
  }

  virtual bool ready() override
  {
    if ( ! target -> debuffs.casting || ! target -> debuffs.casting -> check() )
    {
      return false;
    }

    return mage_spell_t::ready();
  }
};

// Dragon's Breath Spell ====================================================
struct dragons_breath_t : public fire_mage_spell_t
{
  dragons_breath_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "dragons_breath", p,
                       p -> find_specialization_spell( "Dragon's Breath" ) )
  {
    parse_options( options_str );
    aoe = -1;
    triggers_pyretic_incantation = true;
    radius += p -> artifact.big_mouth.value();

    if ( p -> talents.alexstraszas_fury -> ok() )
    {
      base_crit = 1.0;
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( p() -> talents.alexstraszas_fury -> ok() && s -> chain_target == 0 )
    {
      handle_hot_streak( s );
    }

    p() -> apply_crowd_control( s, MECHANIC_DISORIENT );
  }
};

// Ebonbolt Spell ===========================================================
struct glacial_eruption_t : public frost_mage_spell_t
{
  glacial_eruption_t( mage_t* p ) :
    frost_mage_spell_t( "glacial_eruption", p, p -> find_spell( 242851 ) )
  {
    background = true;
    callbacks = false;
    aoe = -1;
  }
};

struct ebonbolt_t : public frost_mage_spell_t
{
  glacial_eruption_t* glacial_eruption;
  timespan_t glacial_eruption_delay;

  ebonbolt_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "ebonbolt", p, p -> artifact.ebonbolt ),
    glacial_eruption( new glacial_eruption_t( p ) )
  {
    // Ebonbolt has some weird 'callbacks' properties.
    // For example: Ebonbolt cast triggers Concordance, impact triggers
    // Mark of the Hidden Satyr but does not trigger Erratic Metronome and
    // Tarnished Sentinel Medallion.
    parse_options( options_str );
    parse_effect_data( p -> find_spell( 228599 ) -> effectN( 1 ) );
    if ( !p -> artifact.ebonbolt.rank() )
    {
      background = true;
    }
    calculate_on_impact = true;
    if ( p -> artifact.glacial_eruption.rank() )
    {
      glacial_eruption_delay = 1000 * p -> artifact.glacial_eruption.data().effectN( 1 ).time_value();
      add_child( glacial_eruption );
    }
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();
    trigger_brain_freeze( 1.0 );
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );
    if ( result_is_hit( s -> result ) && p() -> artifact.glacial_eruption.rank() )
    {
      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .pulse_time( glacial_eruption_delay )
        .target( s -> target )
        .n_pulses( 1 )
        .action( glacial_eruption ) );
    }
  }
};

// Evocation Spell ==========================================================

struct evocation_t : public arcane_mage_spell_t
{
  aegwynns_ascendance_t* aegwynns_ascendance;
  double mana_gained;

  evocation_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "evocation", p,
                         p -> find_specialization_spell( "Evocation" ) ),
    mana_gained( 0.0 )
  {
    parse_options( options_str );

    base_tick_time    = timespan_t::from_seconds( 2.0 );
    channeled         = true;
    dot_duration      = data().duration();
    harmful           = false;
    hasted_ticks      = false;
    tick_zero         = true;
    ignore_false_positive = true;

    cooldown -> duration *= 1.0 + p -> spec.evocation_2 -> effectN( 1 ).percent();

    if ( p -> artifact.aegwynns_ascendance.rank() )
    {
      aegwynns_ascendance = new aegwynns_ascendance_t( p );
    }
  }

  virtual void execute() override
  {
    mana_gained = 0.0;
    arcane_mage_spell_t::execute();
  }

  virtual void tick( dot_t* d ) override
  {
    arcane_mage_spell_t::tick( d );

    double mana_gain = p() -> resources.max[ RESOURCE_MANA ] *
                       data().effectN( 1 ).percent();

    mana_gained += p() -> resource_gain( RESOURCE_MANA, mana_gain,
                                         p() -> gains.evocation );
  }

  virtual void last_tick( dot_t* d ) override
  {
    arcane_mage_spell_t::last_tick( d );

    if ( p() -> artifact.aegwynns_ascendance.rank() )
    {
      double explosion_amount =
        mana_gained * p() -> artifact.aegwynns_ascendance.percent();

      aegwynns_ascendance -> set_target( d -> target );
      aegwynns_ascendance -> base_dd_adder = explosion_amount;
      aegwynns_ascendance -> execute();
    }
  }

  bool usable_moving() const override
  {
    if ( p() -> talents.slipstream -> ok() )
      return true;

    return arcane_mage_spell_t::usable_moving();
  }
};

// Fireball Spell ===========================================================

struct fireball_t : public fire_mage_spell_t
{
  conflagration_dot_t* conflagration_dot;

  fireball_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "fireball", p, p -> find_class_spell( "Fireball" ) ),
    conflagration_dot( new conflagration_dot_t( p ) )
  {
    parse_options( options_str );
    triggers_pyretic_incantation = true;
    triggers_hot_streak = true;
    triggers_ignite = true;
    base_multiplier *= 1.0 + p -> artifact.great_balls_of_fire.percent();
    base_execute_time *= 1.0 + p -> artifact.fire_at_will.percent();
    add_child( conflagration_dot );
    if ( p -> action.unstable_magic_explosion )
    {
      add_child( p -> action.unstable_magic_explosion );
    }
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t t = fire_mage_spell_t::travel_time();
    return std::min( timespan_t::from_seconds( 0.75 ), t );
  }

  virtual void execute() override
  {
    fire_mage_spell_t::execute();

    if ( p() -> sets -> has_set_bonus( MAGE_FIRE, T20, B2 ) )
    {
      p() -> buffs.ignition -> trigger();
    }

    p() -> trigger_t19_oh();
  }

  virtual void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );
    if ( result_is_hit( s -> result ) )
    {
      if ( s -> result == RESULT_CRIT )
      {
        p() -> buffs.enhanced_pyrotechnics -> expire();
      }
      else
      {
        p() -> buffs.enhanced_pyrotechnics -> trigger();
      }

      if ( p() -> talents.kindling -> ok() && s -> result == RESULT_CRIT )
      {
        p() -> cooldowns.combustion
            -> adjust( -1000 * p() -> talents.kindling
                                   -> effectN( 1 ).time_value() );
      }
      if ( p() -> talents.conflagration -> ok() )
      {
        conflagration_dot -> set_target ( s -> target );
        conflagration_dot -> execute();
      }

      trigger_unstable_magic( s );
      trigger_infernal_core( s -> target );
    }
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = fire_mage_spell_t::composite_target_crit_chance( target );

    if ( firestarter_active( target ) )
    {
      c = 1.0;
    }
    return c;
  }

  virtual double composite_crit_chance() const override
  {
    double c = fire_mage_spell_t::composite_crit_chance();

    c += p() -> buffs.enhanced_pyrotechnics -> check_stack_value();

    return c;
  }
};

// Flame Patch Spell ==========================================================

struct flame_patch_t : public fire_mage_spell_t
{
  flame_patch_t( mage_t* p ) :
    fire_mage_spell_t( "flame_patch", p, p -> talents.flame_patch )
  {
    parse_effect_data( p -> find_spell( 205472 ) -> effectN( 1 ) );
    aoe = -1;
    ground_aoe = background = true;
    school = SCHOOL_FIRE;
  }

  // Override damage type to avoid triggering Doom Nova
  dmg_e amount_type( const action_state_t* /* state */,
                     bool /* periodic */ ) const override
  {
    return DMG_OVER_TIME;
  }
};
// Flamestrike Spell ==========================================================

struct aftershocks_t : public fire_mage_spell_t
{
  aftershocks_t( mage_t* p ) :
    fire_mage_spell_t( "aftershocks", p, p -> find_spell( 194432 ) )
  {
    background = true;
    aoe = -1;
    triggers_ignite = true;

    base_multiplier *= 1.0 + p -> artifact.blue_flame_special.percent();
    // 2s according to the spell data.
    base_execute_time = timespan_t::zero();
  }

  virtual double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    // Not snapshot on Flamestrike execute, it seems.
    if ( p() -> buffs.critical_massive -> up() )
    {
      am *= 1.0 + p() -> buffs.critical_massive -> check_value();
    }

    return am;
  }

  virtual double composite_crit_chance() const override
  {
    double c = fire_mage_spell_t::composite_crit_chance();

    if ( p() -> state.ignition_active )
    {
      c += 1.0;
    }

    return c;
   }
};

struct flamestrike_t : public fire_mage_spell_t
{
  aftershocks_t* aftershocks;

  flame_patch_t* flame_patch;
  timespan_t flame_patch_duration;

  flamestrike_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "flamestrike", p,
                       p -> find_specialization_spell( "Flamestrike" ) ),
    flame_patch( new flame_patch_t( p ) ),
    flame_patch_duration( p -> find_spell( 205470 ) -> duration() )
  {
    parse_options( options_str );

    base_multiplier *= 1.0 + p -> artifact.blue_flame_special.percent();
    triggers_ignite = true;
    triggers_pyretic_incantation = true;
    aoe = -1;
    add_child( flame_patch );

    if ( p -> artifact.aftershocks.rank() )
    {
      aftershocks = new aftershocks_t( p );
      add_child( aftershocks );
    }
  }

  virtual action_state_t* new_state() override
  {
    return new ignite_spell_state_t( this, target );
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buffs.hot_streak -> check() )
    {
      return timespan_t::zero();
    }

    return fire_mage_spell_t::execute_time();
  }

  virtual void execute() override
  {
    fire_mage_spell_t::execute();
    p() -> buffs.hot_streak -> expire();

    // Ignition buff is removed shortly after Flamestrike/Pyroblast cast. In a situation
    // where you're hardcasting FS/PB followed by a Hot Streak FS/FB, both spells actually
    // benefit. As of build 24461, 2017-07-05.
    p() -> buffs.ignition -> expire( p() -> bugs ? timespan_t::from_millis( 15 ) : timespan_t::zero() );
    p() -> buffs.critical_massive -> expire();
  }

  virtual void impact( action_state_t* state ) override
  {
    fire_mage_spell_t::impact( state );

    if ( p() -> sets -> has_set_bonus( MAGE_FIRE, T20, B4 ) && state -> result == RESULT_CRIT )
    {
      p() -> buffs.critical_massive -> trigger();
    }

    if ( state -> chain_target == 0 && p() -> artifact.aftershocks.rank() )
    {
      // Ignition has a really weird interaction with Aftershocks. It looks like Flamestrike
      // sets some sort of global flag specifying whether Aftershocks benefits from Ignition or not.
      //
      // So, as an example, you cast Flamestrike with Ignition up (the flag is set to true) and then
      // follow up with another Flamestrike before first Aftershocks hit (the flag is set back to false).
      // None of the following Aftershocks get Ignition crit bonus.
      //
      // This should model that behavior correctly. Otherwise we might need custom snapshotting.
      // Last checked: build 24461, 2017-07-03.
      // TODO: Check if this is still true.
      p() -> state.ignition_active = p() -> buffs.ignition -> up();

      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .pulse_time( timespan_t::from_seconds( 0.75 ) )
        .target( state -> target )
        .n_pulses( 1 )
        .action( aftershocks ) );
    }
    if ( state -> chain_target == 0 && p() -> talents.flame_patch -> ok() )
    {
      p() -> ground_aoe_expiration[ flame_patch -> name_str ]
        = sim -> current_time() + flame_patch_duration;

      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .target( state -> target )
        .duration( flame_patch_duration )
        .action( flame_patch )
        .hasted( ground_aoe_params_t::SPELL_SPEED ) );
    }
  }

  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  {
    fire_mage_spell_t::snapshot_state( s, rt );

    ignite_spell_state_t* is = debug_cast<ignite_spell_state_t*>( s );
    is -> hot_streak = p() -> buffs.hot_streak -> check() != 0;
  }

  double composite_ignite_multiplier( const action_state_t* s ) const override
  {
   const ignite_spell_state_t* is = debug_cast<const ignite_spell_state_t*>( s );
   return is -> hot_streak ? 2.0 : 1.0;
  }

  virtual double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    if ( p() -> buffs.critical_massive -> up() )
    {
      am *= 1.0 + p() -> buffs.critical_massive -> check_value();
    }

    return am;
  }

  virtual double composite_crit_chance() const override
  {
    double c = fire_mage_spell_t::composite_crit_chance();

    if ( p() -> buffs.ignition -> up() )
    {
      c += 1.0;
    }

    return c;
   }
};

// Flurry Spell ===============================================================

struct flurry_bolt_t : public frost_mage_spell_t
{
  flurry_bolt_t( mage_t* p ) :
    frost_mage_spell_t( "flurry_bolt", p, p -> find_spell( 228354 ) )
  {
    background = true;
    chills = true;
    if ( p -> talents.lonely_winter -> ok() )
    {
      base_multiplier *= 1.0 + p -> talents.lonely_winter -> effectN( 1 ).percent()
                             + p -> artifact.its_cold_outside.data().effectN( 2 ).percent();
    }
    base_multiplier *= 1.0 + p -> artifact.ice_age.percent();
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( p() -> state.brain_freeze_active )
    {
      td( s -> target ) -> debuffs.winters_chill -> trigger();
    }
  }

  virtual double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    if ( p() -> state.brain_freeze_active )
    {
      am *= 1.0 + p() -> buffs.brain_freeze -> data().effectN( 2 ).percent();
    }

    return am;
  }
};

struct flurry_t : public frost_mage_spell_t
{
  flurry_bolt_t* flurry_bolt;

  flurry_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "flurry", p, p -> find_specialization_spell( "Flurry" ) ),
    flurry_bolt( new flurry_bolt_t( p ) )
  {
    parse_options( options_str );
    add_child( flurry_bolt );
  }

  virtual void init() override
  {
    frost_mage_spell_t::init();
    // Snapshot haste for bolt impact timing.
    snapshot_flags |= STATE_HASTE;
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buffs.brain_freeze -> check() )
    {
      return timespan_t::zero();
    }

    return frost_mage_spell_t::execute_time();
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    p() -> buffs.zannesu_journey -> trigger();
    p() -> state.brain_freeze_active = p() -> buffs.brain_freeze -> up();
    p() -> buffs.brain_freeze -> expire();
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    trigger_shattered_fragments( s -> target );

    // TODO: Remove hardcoded values once it exists in spell data for bolt impact timing.
    timespan_t pulse_time = timespan_t::from_seconds( 0.4 );

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .pulse_time( pulse_time * s -> haste )
      .target( s -> target )
      .n_pulses( data().effectN( 1 ).base_value() )
      .action( flurry_bolt ), true );
  }
};

// Frost Bomb Spell ===========================================================

struct frost_bomb_explosion_t : public frost_mage_spell_t
{
  frost_bomb_explosion_t( mage_t* p ) :
    frost_mage_spell_t( "frost_bomb_explosion", p, p -> find_spell( 113092 ) )
  {
    background = true;
    callbacks = false;
    radius = data().effectN( 2 ).radius_max();
    aoe = -1;
    parse_effect_data( data().effectN( 1 ) );
    base_aoe_multiplier *= data().effectN( 2 ).sp_coeff() / data().effectN( 1 ).sp_coeff();
  }
};

struct frost_bomb_t : public frost_mage_spell_t
{
  frost_bomb_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "frost_bomb", p, p -> talents.frost_bomb )
  {
    parse_options( options_str );
    // Frost Bomb no longer has ticking damage.
    dot_duration = timespan_t::zero();

    if ( p -> action.frost_bomb_explosion )
    {
      add_child( p -> action.frost_bomb_explosion );
    }
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    if ( hit_any_target )
    {
      if ( p() -> last_bomb_target != nullptr &&
           p() -> last_bomb_target != execute_state -> target )
      {
        td( p() -> last_bomb_target ) -> debuffs.frost_bomb -> expire();
      }
      p() -> last_bomb_target = execute_state -> target;
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs.frost_bomb -> trigger();
    }
  }
};

// Frostbolt Spell ==========================================================

struct frostbolt_t : public frost_mage_spell_t
{
  // Icicle stats variable to parent icicle damage to Frostbolt, instead of
  // clumping Icicle damage together in reports.
  stats_t* icicle;

  int water_jet_fof_source_id;

  frostbolt_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "frostbolt", p,
                        p -> find_specialization_spell( "Frostbolt" ) ),
    icicle( p -> get_stats( "icicle" ) )
  {
    parse_options( options_str );
    parse_effect_data( p -> find_spell( 228597 ) -> effectN( 1 ) );
    if ( p -> spec.icicles -> ok() )
    {
      stats -> add_child( icicle );
      icicle -> school = school;
      assert( p -> icicle );
      icicle -> action_list.push_back( p -> icicle );
    }
    if ( p -> action.unstable_magic_explosion )
    {
      add_child( p -> action.unstable_magic_explosion );
    }
    if ( p -> talents.lonely_winter -> ok() )
    {
      base_multiplier *= 1.0 + p -> talents.lonely_winter -> effectN( 1 ).percent() +
                               p -> artifact.its_cold_outside.data().effectN( 2 ).percent();
    }
    base_multiplier *= 1.0 + p -> artifact.icy_caress.percent();
    base_crit += p -> artifact.shattering_bolts.percent();
    chills = true;
    calculate_on_impact = true;
  }

  virtual bool init_finished() override
  {
    fof_source_id = p() -> benefits.fingers_of_frost
                        -> get_source_id( data().name_cstr() );
    water_jet_fof_source_id = p() -> benefits.fingers_of_frost
                                  -> get_source_id( "Water Jet" );

    return frost_mage_spell_t::init_finished();
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    p() -> buffs.icicles -> trigger();

    if ( hit_any_target )
    {
      double fof_proc_chance = p() -> spec.fingers_of_frost -> effectN( 1 ).percent();
      fof_proc_chance *= 1.0 + p() -> talents.frozen_touch -> effectN( 1 ).percent();
      trigger_fof( fof_source_id, fof_proc_chance );

      double bf_proc_chance = p() -> spec.brain_freeze -> effectN( 1 ).percent();
      bf_proc_chance += p() -> sets -> set( MAGE_FROST, T19, B2 ) -> effectN( 1 ).percent();
      bf_proc_chance += p() -> artifact.clarity_of_thought.percent();
      trigger_brain_freeze( bf_proc_chance );
    }

    p() -> trigger_t19_oh();
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_icicle_gain( s, icicle );

      if ( p() -> pets.water_elemental && !p() -> pets.water_elemental -> is_sleeping() )
      {
        auto we_td =
          p() -> pets.water_elemental
          -> get_target_data( s -> target );

        if ( we_td -> water_jet -> up() )
        {
          trigger_fof( water_jet_fof_source_id, 1.0 );
        }
      }

      //TODO: Fix hardcode once spelldata has value for proc rate.
      if ( p() -> artifact.ice_nine.rank() && rng().roll( 0.15 ) )
      {
        trigger_icicle_gain( s, icicle );
        p() -> buffs.icicles -> trigger();
      }
      if ( s -> result == RESULT_CRIT && p() -> artifact.frozen_veins.rank() )
      {
        p() -> cooldowns.icy_veins -> adjust( p() -> artifact.frozen_veins.time_value() );
      }
      if ( s -> result == RESULT_CRIT && p() -> artifact.chain_reaction.rank() )
      {
        p() -> buffs.chain_reaction -> trigger();
      }

      trigger_unstable_magic( s );
      trigger_shattered_fragments( s -> target );
    }
  }
};

// Frost Nova Spell ========================================================

struct frost_nova_t : public mage_spell_t
{
  frost_nova_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frost_nova", p, p -> find_class_spell( "Frost Nova" ) )
  {
    parse_options( options_str );

    affected_by.arcane_mage = true;
    affected_by.fire_mage = true;
    affected_by.frost_mage = true;

    affected_by.erosion = true;
    affected_by.shatter = true;

    cooldown -> charges += p -> talents.ice_ward -> effectN( 1 ).base_value();
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );
    p() -> apply_crowd_control( s, MECHANIC_ROOT );
  }
};

// Ice Time Super Frost Nova ================================================

struct ice_time_nova_t : public frost_mage_spell_t
{
  ice_time_nova_t( mage_t* p ) :
    frost_mage_spell_t( "ice_time_nova", p, p -> find_spell( 235235 ) )
  {
    background = true;
    aoe = -1;

    // According to the spell data.
    // As of build 24461, 2017-07-03.
    if ( p -> bugs )
    {
      affected_by.frost_mage = false;
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    p() -> apply_crowd_control( s, MECHANIC_ROOT );
  }
};

// Frozen Orb Spell =========================================================

struct frozen_orb_bolt_t : public frost_mage_spell_t
{
  frozen_orb_bolt_t( mage_t* p ) :
    frost_mage_spell_t( "frozen_orb_bolt", p, p -> find_spell( 84721 ) )
  {
    aoe = -1;
    background = true;

    if ( p -> talents.lonely_winter -> ok() )
    {
      base_multiplier *= 1.0 + p -> talents.lonely_winter -> effectN( 1 ).percent() +
                               p -> artifact.its_cold_outside.data().effectN( 2 ).percent();
    }
    crit_bonus_multiplier *= 1.0 + p -> artifact.orbital_strike.percent();
    chills = true;

    // As of build 24461, 2017-07-03.
    if ( p -> bugs )
    {
      affected_by.shatter = false;
    }
  }

  virtual bool init_finished() override
  {
    fof_source_id = p() -> benefits.fingers_of_frost
                        -> get_source_id( "Frozen Orb Tick" );

    return frost_mage_spell_t::init_finished();
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();
    if ( hit_any_target )
    {
      double fof_proc_chance = p() -> spec.fingers_of_frost -> effectN( 1 ).percent();
      fof_proc_chance += p() -> sets -> set( MAGE_FROST, T19, B4 ) -> effectN( 1 ).percent();
      fof_proc_chance *= 1.0 + p() -> talents.frozen_touch -> effectN( 1 ).percent();
      trigger_fof( fof_source_id, fof_proc_chance );
    }
  }
};

struct frozen_orb_t : public frost_mage_spell_t
{
  bool ice_time;
  timespan_t freezing_rain_base_duration;

  ice_time_nova_t* ice_time_nova;
  frozen_orb_bolt_t* frozen_orb_bolt;

  frozen_orb_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "frozen_orb", p,
                        p -> find_specialization_spell( "Frozen Orb" ) ),
    ice_time( false ),
    ice_time_nova( new ice_time_nova_t( p ) ),
    frozen_orb_bolt( new frozen_orb_bolt_t( p ) )
  {
    parse_options( options_str );
    add_child( frozen_orb_bolt );
    add_child( ice_time_nova );
    may_miss       = false;
    may_crit       = false;
    if ( p -> artifact.freezing_rain.rank() )
    {
      freezing_rain_base_duration = p -> buffs.freezing_rain -> data().duration();
    }
  }

  virtual bool init_finished() override
  {
    fof_source_id = p() -> benefits.fingers_of_frost
                        -> get_source_id( "Frozen Orb Initial Impact" );

    return frost_mage_spell_t::init_finished();
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    if ( p() -> sets -> has_set_bonus( MAGE_FROST, T20, B2 ) )
    {
      p() -> buffs.frozen_mass -> trigger();
    }
    if ( p() -> artifact.freezing_rain.rank() )
    {
      timespan_t freezing_rain_duration = freezing_rain_base_duration * p() -> cache.spell_speed();
      p() -> buffs.freezing_rain -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, freezing_rain_duration  );
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );
    player_t* t = s -> target;
    double x = t -> x_position;
    double y = t -> y_position;

    timespan_t ground_aoe_duration = timespan_t::from_seconds( 10.0 );
    p() -> ground_aoe_expiration[ name_str ]
      = sim -> current_time() + ground_aoe_duration;

    if ( result_is_hit( s -> result ) )
    {
      trigger_fof( fof_source_id, 1.0 );
      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .pulse_time( timespan_t::from_seconds( 0.5 ) )
        .target( t )
        .duration( ground_aoe_duration )
        .action( frozen_orb_bolt )
        .expiration_callback( [ this, t, x, y ] () {
          if ( ice_time )
          {
            action_state_t* state = ice_time_nova -> get_state();
            ice_time_nova -> snapshot_state( state, ice_time_nova -> amount_type( state ) );
            // Make sure Ice Time works correctly with distance targetting, e.g.
            // when the target moves out of Frozen Orb.
            state -> target     = t;
            state -> original_x = x;
            state -> original_y = y;

            ice_time_nova -> schedule_execute( state );
          }
        } ) );
    }
  }
};

// Glacial Spike Spell ==============================================================

struct glacial_spike_t : public frost_mage_spell_t
{
  double icicle_damage;

  glacial_spike_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "glacial_spike", p, p -> talents.glacial_spike ),
    icicle_damage( 0.0 )
  {
    parse_options( options_str );
    parse_effect_data( p -> find_spell( 228600 ) -> effectN( 1 ) );
    if ( p -> talents.splitting_ice -> ok() )
    {
      aoe = 1 + p -> talents.splitting_ice -> effectN( 1 ).base_value();
      base_aoe_multiplier *= p -> talents.splitting_ice
                               -> effectN( 2 ).percent();
    }
    calculate_on_impact = true;
  }

  virtual bool ready() override
  {
    if ( p() -> buffs.icicles -> check() < p() -> buffs.icicles -> max_stack() )
    {
      return false;
    }

    return frost_mage_spell_t::ready();
  }

  virtual double calculate_direct_amount( action_state_t* s ) const override
  {
    if ( cast_state( s ) -> impact_override )
    {
      double base_amount = mage_spell_t::calculate_direct_amount( s );
      double icicle_amount = icicle_damage;

      // Icicle portion is only affected by target-based damage multipliers.
      icicle_amount *= s -> target_da_multiplier;

      if ( s -> chain_target > 0 )
        icicle_amount *= base_aoe_multiplier;

      double amount = base_amount + icicle_amount;
      s -> result_raw = amount;

      if ( result_is_miss( s -> result ) )
      {
        s -> result_total = 0.0;
        return 0.0;
      }
      else
      {
        s -> result_total = amount;
        return amount;
      }
    }
    else
    {
      return s -> result_amount;
    }
  }

  virtual void execute() override
  {
    // Ideally, this would be passed to impact() in action_state_t, but since
    // it's pretty much impossible to execute another Glacial Spike before
    // the first one impacts, this should be fine.
    icicle_damage = 0.0;
    int icicle_count = as<int>( p() -> icicles.size() );

    for ( int i = 0; i < icicle_count; i++ )
    {
      icicle_data_t d = p() -> get_icicle_object();
      icicle_damage += d.damage;
    }

    if ( sim -> debug )
    {
      sim -> out_debug.printf( "Add %u icicles to glacial_spike for %f damage",
                               icicle_count, icicle_damage );
    }

    frost_mage_spell_t::execute();

    p() -> buffs.icicles -> expire();
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );
    p() -> apply_crowd_control( s, MECHANIC_ROOT );
  }
};


// Ice Floes Spell ============================================================

struct ice_floes_t : public mage_spell_t
{
  ice_floes_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "ice_floes", p, p -> talents.ice_floes )
  {
    parse_options( options_str );
    may_miss = may_crit = harmful = false;
    trigger_gcd = timespan_t::zero();

    internal_cooldown -> duration = data().internal_cooldown();
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    p() -> buffs.ice_floes -> trigger( 1 );
  }
};


// Ice Lance Spell ==========================================================

struct ice_lance_t : public frost_mage_spell_t
{
  ice_lance_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "ice_lance", p, p -> find_specialization_spell( "Ice Lance" ) )
  {
    parse_options( options_str );
    parse_effect_data( p -> find_spell( 228598 ) -> effectN( 1 ) );

    if ( p -> talents.lonely_winter -> ok() )
    {
      base_multiplier *= 1.0 + p -> talents.lonely_winter -> effectN( 1 ).percent() +
                               p -> artifact.its_cold_outside.data().effectN( 2 ).percent();
    }

    if ( p -> talents.splitting_ice -> ok() )
    {
      base_multiplier *= 1.0 + p -> talents.splitting_ice
                                 -> effectN( 3 ).percent();
      aoe = 1 + p -> talents.splitting_ice -> effectN( 1 ).base_value();
      base_aoe_multiplier *= p -> talents.splitting_ice
                               -> effectN( 2 ).percent();
    }
    crit_bonus_multiplier *= 1.0 + p -> artifact.let_it_go.percent();
    calculate_on_impact = true;
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    p() -> state.fingers_of_frost_active = p() -> buffs.fingers_of_frost -> up();
    p() -> buffs.magtheridons_might -> trigger();
    p() -> buffs.fingers_of_frost -> decrement();

    // Begin casting all Icicles at the target, beginning 0.25 seconds after the
    // Ice Lance cast with remaining Icicles launching at intervals of 0.4
    // seconds, the latter adjusted by haste. Casting continues until all
    // Icicles are gone, including new ones that accumulate while they're being
    // fired. If target dies, Icicles stop.
    if ( !p() -> talents.glacial_spike -> ok() )
    {
      p() -> trigger_icicle( execute_state, true, target );
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    auto fss = cast_state( s );
    if ( p() -> talents.thermal_void -> ok() &&
         p() -> buffs.icy_veins -> check() &&
         fss -> frozen() &&
         s -> chain_target == 0 )
    {
      timespan_t tv_extension = p() -> talents.thermal_void
                                    -> effectN( 1 ).time_value() * 1000;

      p() -> buffs.icy_veins -> extend_duration( p(), tv_extension );
    }
    if ( result_is_hit( s -> result ) && fss -> frozen() &&
         td( s -> target ) -> debuffs.frost_bomb -> check() )
    {
      assert( p() -> action.frost_bomb_explosion );
      p() -> action.frost_bomb_explosion -> set_target( s -> target );
      p() -> action.frost_bomb_explosion -> execute();
    }
  }

  virtual double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.chain_reaction -> check_stack_value();
    am *= 1.0 + p() -> buffs.magtheridons_might -> check_stack_value();

    return am;
  }

  virtual double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = frost_mage_spell_t::composite_da_multiplier( s );

    if ( cast_state( s ) -> frozen() )
    {
      m *= 3.0;
      m *= 1 + p() -> artifact.obsidian_lance.percent();
    }

    return m;
  }
};


// Ice Nova Spell ==========================================================

struct ice_nova_t : public frost_mage_spell_t
{
  ice_nova_t( mage_t* p, const std::string& options_str ) :
     frost_mage_spell_t( "ice_nova", p, p -> talents.ice_nova )
  {
    parse_options( options_str );

    aoe = -1;

    double in_mult = 1.0 + p -> talents.ice_nova -> effectN( 1 ).percent();
    base_multiplier *= in_mult;
    base_aoe_multiplier = 1.0 / in_mult;
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );
    p() -> apply_crowd_control( s, MECHANIC_ROOT );
  }
};


// Icy Veins Spell ==========================================================

struct icy_veins_t : public frost_mage_spell_t
{
  bool lady_vashjs_grasp;

  icy_veins_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "icy_veins", p, p -> find_specialization_spell( "Icy Veins" ) ),
    lady_vashjs_grasp( false )
  {
    parse_options( options_str );
    harmful = false;
  }

  bool init_finished() override
  {
    if ( lady_vashjs_grasp )
    {
      debug_cast<buffs::lady_vashjs_grasp_t*>( p() -> buffs.lady_vashjs_grasp ) -> fof_source_id =
        p() -> benefits.fingers_of_frost -> get_source_id( "Lady Vashj's Grasp" );
    }

    return frost_mage_spell_t::init_finished();
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    p() -> buffs.icy_veins -> trigger();

    if ( lady_vashjs_grasp )
    {
      // Refreshing infinite ticking buff doesn't quite work, remove
      // LVG manually and then trigger it again.
      p() -> buffs.lady_vashjs_grasp -> expire();
      p() -> buffs.lady_vashjs_grasp -> trigger();
    }
    if ( p() -> artifact.chilled_to_the_core.rank() )
    {
      p() -> buffs.chilled_to_the_core -> trigger();
    }
  }
};

// Fire Blast Spell ======================================================

struct blast_furnace_t : public fire_mage_spell_t
{
  blast_furnace_t( mage_t* p ) :
    fire_mage_spell_t( "blast_furnace", p, p -> find_spell( 194522 ) )
  {
    background = true;
    callbacks = false;
    hasted_ticks = false;
  }
};

struct fire_blast_t : public fire_mage_spell_t
{
  blast_furnace_t* blast_furnace;

  fire_blast_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "fire_blast", p,
                       p -> find_specialization_spell( "Fire Blast" ) ),
    blast_furnace( nullptr )
  {
    parse_options( options_str );
    base_multiplier *= 1.0 + p -> artifact.reignition_overdrive.percent();
    trigger_gcd = timespan_t::zero();

    cooldown -> charges = data().charges();
    cooldown -> charges += p -> spec.fire_blast_3 -> effectN( 1 ).base_value();
    cooldown -> charges += p -> talents.flame_on -> effectN( 1 ).base_value();

    cooldown -> duration = data().charge_cooldown();
    cooldown -> duration -= 1000 * p -> talents.flame_on -> effectN( 3 ).time_value();

    cooldown -> hasted = true;

    triggers_hot_streak = true;
    triggers_ignite = true;
    triggers_pyretic_incantation = true;

    if ( p -> artifact.blast_furnace.rank() )
    {
      blast_furnace = new blast_furnace_t( p );
      add_child( blast_furnace );
    }

    base_crit += p -> spec.fire_blast_2 -> effectN( 1 ).percent();
  }

  virtual void execute() override
  {
    fire_mage_spell_t::execute();

    // update_ready() assumes the ICD is affected by haste
    internal_cooldown -> start( data().cooldown() );
  }

  virtual void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( s -> result == RESULT_CRIT && p() -> talents.kindling -> ok() )
      {
        p() -> cooldowns.combustion
            -> adjust( -1000 * p() -> talents.kindling
                                   -> effectN( 1 ).time_value() );
      }

      if ( p() -> artifact.blast_furnace.rank() )
      {
        blast_furnace -> set_target( s -> target );
        blast_furnace -> execute();
      }
    }
  }
};


// Living Bomb Spell ========================================================

struct living_bomb_explosion_t;
struct living_bomb_t;

struct living_bomb_explosion_t : public fire_mage_spell_t
{
  living_bomb_t* child_lb;

  living_bomb_explosion_t( mage_t* p, living_bomb_t* parent_lb );
  virtual resource_e current_resource() const override;
  void impact( action_state_t* s ) override;
};

struct living_bomb_t : public fire_mage_spell_t
{
  bool casted;
  living_bomb_explosion_t* explosion;

  living_bomb_t( mage_t* p, const std::string& options_str, bool _casted );
  virtual timespan_t composite_dot_duration( const action_state_t* s )
    const override;
  virtual void last_tick( dot_t* d ) override;
  virtual void init() override;
};

living_bomb_explosion_t::
  living_bomb_explosion_t( mage_t* p, living_bomb_t* parent_lb ) :
    fire_mage_spell_t( "living_bomb_explosion", p, p -> find_spell( 44461 ) ),
    child_lb( nullptr )
{
  aoe = -1;
  radius = 10;
  background = true;
  if ( parent_lb -> casted )
  {
    child_lb = new living_bomb_t( p, std::string( "" ), false );
    child_lb -> background = true;
  }
}

resource_e living_bomb_explosion_t::current_resource() const
{ return RESOURCE_NONE; }

void living_bomb_explosion_t::impact( action_state_t* s )
{
  fire_mage_spell_t::impact( s );

  if ( child_lb && s -> chain_target > 0 )
  {
    if ( sim -> debug )
    {
      sim -> out_debug.printf(
        "%s %s on %s applies %s on %s",
        p() -> name(), name(), s -> action -> target -> name(),
        child_lb -> name(), s -> target -> name() );
    }

    child_lb -> set_target( s -> target );
    child_lb -> base_costs[ RESOURCE_MANA ] = 0;
    child_lb -> execute();
  }
}

living_bomb_t::living_bomb_t( mage_t* p, const std::string& options_str,
                              bool _casted = true ) :
  fire_mage_spell_t( "living_bomb", p, p -> talents.living_bomb ),
  casted( _casted ),
  explosion( new living_bomb_explosion_t( p, this ) )
{
  parse_options( options_str );
  // Why in Azeroth would they put DOT spell data in a separate spell??
  const spell_data_t* dot_data = p -> find_spell( 217694 );
  dot_duration = dot_data -> duration();
  for ( size_t i = 1; i <= dot_data -> effect_count(); i++ )
  {
    parse_effect_data( dot_data -> effectN( i ) );
  }

  cooldown -> hasted = true;
  hasted_ticks       = true;
  add_child( explosion );
}

timespan_t living_bomb_t::composite_dot_duration( const action_state_t* s ) const
{
  timespan_t duration = fire_mage_spell_t::composite_dot_duration( s );
  return duration * ( tick_time( s ) / base_tick_time );
}

void living_bomb_t::last_tick( dot_t* d )
{
  fire_mage_spell_t::last_tick( d );

  explosion -> set_target( d -> target );
  explosion -> execute();
}

void living_bomb_t::init()
{
  fire_mage_spell_t::init();

  update_flags &= ~STATE_HASTE;
}


// Mark of Aluneth Spell =============================================================

struct mark_of_aluneth_explosion_t : public arcane_mage_spell_t
{
  double mana_to_damage_pct;
  double aluneths_avarice_regen;
  double persistent_cord_multiplier;

  mark_of_aluneth_explosion_t( mage_t* p ) :
    arcane_mage_spell_t( "mark_of_aluneth_explosion", p, p -> find_spell( 211076 ) ),
    mana_to_damage_pct( p -> artifact.mark_of_aluneth.data().effectN( 1 ).percent() ),
    aluneths_avarice_regen( 0.0 ),
    persistent_cord_multiplier( 0.0 )
  {
    background = true;
    aoe = -1;

    // As of build 24461, 2017-07-03.
    if ( p -> bugs )
    {
      affected_by.arcane_mage = false;
      affected_by.erosion = false;
    }

    if ( p -> artifact.aluneths_avarice.rank() )
    {
      aluneths_avarice_regen = data().effectN( 2 ).percent();
    }
  }

  virtual void execute() override
  {
    base_dd_adder = p() -> resources.max[ RESOURCE_MANA ] * mana_to_damage_pct;

    arcane_mage_spell_t::execute();

    persistent_cord_multiplier = 0.0;
    if ( p() -> artifact.aluneths_avarice.rank() )
    {
      p() -> resource_gain( RESOURCE_MANA,
                            aluneths_avarice_regen * p() -> resources.max[ RESOURCE_MANA ],
                            p() -> gains.aluneths_avarice );
    }
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= 1.0 + persistent_cord_multiplier;

    return am;
  }
};

struct mark_of_aluneth_t : public arcane_mage_spell_t
{
  mark_of_aluneth_explosion_t* mark_explosion;

  mark_of_aluneth_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "mark_of_aluneth", p, p -> artifact.mark_of_aluneth ),
    mark_explosion( new mark_of_aluneth_explosion_t( p ) )
  {
    parse_options( options_str );
    school = SCHOOL_ARCANE;
    // Erosion needs to be triggered on tick, not on impact.
    triggers_erosion = false;
    spell_power_mod.tick = p -> find_spell( 211088 ) -> effectN( 1 ).sp_coeff();
    hasted_ticks = false;
    add_child( mark_explosion );
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();

    p() -> buffs.cord_of_infinity -> expire();
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = arcane_mage_spell_t::composite_persistent_multiplier( state );

    m *= 1.0 + p() -> buffs.cord_of_infinity -> check_stack_value();
    mark_explosion -> persistent_cord_multiplier = p() -> buffs.cord_of_infinity -> check_stack_value();

    return m;
  }

  void tick( dot_t* dot ) override
  {
    arcane_mage_spell_t::tick( dot );
    if ( p() -> talents.erosion -> ok() )
    {
      td( dot -> target ) -> debuffs.erosion -> trigger();
    }
  }

  void last_tick( dot_t* d ) override
  {
    arcane_mage_spell_t::last_tick( d );

    mark_explosion -> set_target( d -> target );
    mark_explosion -> execute();
  }
};

// Meteor Spell ===============================================================

// TODO: Have they fixed Meteor's implementation in Legion?
// Implementation details from Celestalon:
// http://blue.mmo-champion.com/topic/318876-warlords-of-draenor-theorycraft-discussion/#post301
// Meteor is split over a number of spell IDs, some of which don't seem to be
// used for anything useful:
// - Meteor (id=153561) is the talent spell, the driver
// - Meteor (id=153564) is the initial impact damage
// - Meteor Burn (id=155158) is the ground effect tick damage
// - Meteor Burn (id=175396) provides the tooltip's burn duration (8 seconds),
//   but doesn't match in game where we only see 7 ticks over 7 seconds.
// - Meteor (id=177345) contains the time between cast and impact
// None of these specify the 1 second falling duration given by Celestalon, so
// we're forced to hardcode it.

// We need to keep the current tick event stored somewhere so that we can cancel it
// (because Meteorn Burns do not overlap).
struct tracking_ground_aoe_event_t : public ground_aoe_event_t
{
  mage_t* mage;

  tracking_ground_aoe_event_t( mage_t* mage, const ground_aoe_params_t* param, action_state_t* ps, bool first_tick = false ):
    ground_aoe_event_t( mage, param, ps, first_tick ), mage( mage )
  { }

  tracking_ground_aoe_event_t( mage_t* mage, const ground_aoe_params_t& param, bool first_tick = false ) :
    ground_aoe_event_t( mage, param, first_tick ), mage( mage )
  { }

  void schedule_event() override
  {
    assert( ! mage -> active_meteor_burn );
    mage -> active_meteor_burn = make_event<tracking_ground_aoe_event_t>( sim(), mage, params, pulse_state );
  }

  void execute() override
  {
    assert( mage -> active_meteor_burn );
    mage -> active_meteor_burn = nullptr;
    ground_aoe_event_t::execute();
  }
};

struct meteor_burn_t : public fire_mage_spell_t
{
  meteor_burn_t( mage_t* p, int targets, bool legendary ) :
    fire_mage_spell_t( legendary ? "legendary_meteor_burn" : "meteor_burn",
                       p, p -> find_spell( 155158 ) )
  {
    background = true;
    aoe = targets;
    spell_power_mod.direct = data().effectN( 1 ).sp_coeff();
    spell_power_mod.tick = 0;
    dot_duration = timespan_t::zero();
    radius = p -> find_spell( 153564 ) -> effectN( 1 ).radius_max();
    ground_aoe = true;
  }

  // Override damage type because Meteor Burn is considered a DOT
  dmg_e amount_type( const action_state_t* /* state */,
                     bool /* periodic */ ) const override
  {
    return DMG_OVER_TIME;
  }
};

struct meteor_impact_t: public fire_mage_spell_t
{
  meteor_burn_t* meteor_burn;
  timespan_t meteor_burn_duration;
  timespan_t meteor_burn_pulse_time;

  meteor_impact_t( mage_t* p, meteor_burn_t* meteor_burn, int targets, bool legendary ):
    fire_mage_spell_t( legendary ? "legendary_meteor_imapct" : "meteor_impact",
                       p, p -> find_spell( 153564 ) ),
    meteor_burn( meteor_burn ),
    meteor_burn_duration( p -> find_spell( 175396 ) -> duration() )
  {
    background = true;
    aoe = targets;
    split_aoe_damage = true;
    //TODO: Revisit PI behavior once Skullflower confirms behavior.
    triggers_ignite = true;

    meteor_burn_pulse_time = meteor_burn -> data().effectN( 1 ).period();

    // It seems that the 8th tick happens only very rarely in game.
    // As of build 24461, 2017-07-03.
    if ( p -> bugs )
    {
      meteor_burn_duration -= meteor_burn_pulse_time;
    }
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.0 );
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    p() -> ground_aoe_expiration[ meteor_burn -> name_str ]
      = sim -> current_time() + meteor_burn_duration;

    event_t::cancel( p() -> active_meteor_burn );

    p() -> active_meteor_burn = make_event<tracking_ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .pulse_time( meteor_burn_pulse_time )
      .target( s -> target )
      .duration( meteor_burn_duration )
      .action( meteor_burn ) );
  }
};

struct meteor_t : public fire_mage_spell_t
{
  int targets;
  meteor_impact_t* meteor_impact;
  timespan_t meteor_delay;

  meteor_t( mage_t* p, const std::string& options_str, bool legendary = false ) :
    fire_mage_spell_t( legendary ? "legendary_meteor" : "meteor",
                       p, p -> find_talent_spell( "Meteor", SPEC_NONE, false, ! legendary ) ),
    targets( -1 ),
    meteor_delay( p -> find_spell( 177345 ) -> duration() )
  {
    add_option( opt_int( "targets", targets ) );
    parse_options( options_str );
    callbacks = false;

    meteor_burn_t* meteor_burn = new meteor_burn_t( p, targets, legendary );
    meteor_impact = new meteor_impact_t( p, meteor_burn, targets, legendary );

    add_child( meteor_impact );
    add_child( meteor_burn );

    if ( legendary )
    {
      background = true;
      base_costs[ RESOURCE_MANA ] = 0;
    }
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t impact_time = meteor_delay * p() -> composite_spell_haste();
    timespan_t meteor_spawn = impact_time - meteor_impact -> travel_time();
    meteor_spawn = std::max( timespan_t::zero(), meteor_spawn );

    return meteor_spawn;
  }

  void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );
    meteor_impact -> set_target( s -> target );
    meteor_impact -> execute();
  }
};

// Mirror Image Spell =========================================================

struct mirror_image_t : public mage_spell_t
{
  mirror_image_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "mirror_image", p, p -> find_talent_spell( "Mirror Image" ) )
  {
    parse_options( options_str );
    dot_duration = timespan_t::zero();
    harmful = false;
  }

  bool init_finished() override
  {
    std::vector<pet_t*> images = p() -> pets.mirror_images;

    for ( pet_t* image : images )
    {
      if ( !image )
      {
        continue;
      }

      stats -> add_child( image -> get_stats( "arcane_blast" ) );
      stats -> add_child( image -> get_stats( "fireball" ) );
      stats -> add_child( image -> get_stats( "frostbolt" ) );
    }

    return mage_spell_t::init_finished();
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( p() -> pets.mirror_images[ 0 ] )
    {
      for ( int i = 0; i < data().effectN( 2 ).base_value(); i++ )
      {
        p() -> pets.mirror_images[ i ] -> summon( data().duration() );
      }
    }
  }
};

// Nether Tempest AoE Spell ===================================================
struct nether_tempest_aoe_t: public arcane_mage_spell_t
{
  nether_tempest_aoe_t( mage_t* p ) :
    arcane_mage_spell_t( "nether_tempest_aoe", p, p -> find_spell( 114954 ) )
  {
    aoe = -1;
    background = true;
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.3 );
  }
};


// Nether Tempest Spell =======================================================
struct nether_tempest_t : public arcane_mage_spell_t
{
  nether_tempest_aoe_t* nether_tempest_aoe;

  nether_tempest_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "nether_tempest", p, p -> talents.nether_tempest ),
    nether_tempest_aoe( new nether_tempest_aoe_t( p ) )
  {
    parse_options( options_str );
    // Disable default AM proc logic due to early refresh proc behavior
    triggers_arcane_missiles = false;

    add_child( nether_tempest_aoe );
  }

  virtual bool init_finished() override
  {
    if ( data().ok() )
    {
      am_trigger_source_id = p() -> benefits.arcane_missiles
                                 -> get_source_id( data().name_cstr() );
    }

    return arcane_mage_spell_t::init_finished();
  }

  virtual void execute() override
  {
    p() -> benefits.arcane_charge.nether_tempest -> update();

    double am_proc_chance =  p() -> buffs.arcane_missiles -> proc_chance();
    timespan_t nt_remains = td( target ) -> dots.nether_tempest -> remains();

    if ( nt_remains > data().duration() * 0.3 )
    {
      double elapsed = std::min( 1.0, nt_remains / data().duration() );
      am_proc_chance *= 1.0 - elapsed;
    }

    arcane_mage_spell_t::execute();

    if ( hit_any_target )
    {
      if ( p() -> last_bomb_target != nullptr &&
           p() -> last_bomb_target != execute_state -> target )
      {
        td( p() -> last_bomb_target ) -> dots.nether_tempest -> cancel();
      }

      trigger_am( am_trigger_source_id, am_proc_chance );
      p() -> last_bomb_target = execute_state -> target;
    }
  }

  virtual void tick( dot_t* d ) override
  {
    arcane_mage_spell_t::tick( d );

    action_state_t* aoe_state = nether_tempest_aoe -> get_state( d -> state );
    aoe_state -> target = d -> target;

    nether_tempest_aoe -> schedule_execute( aoe_state );
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = arcane_mage_spell_t::composite_persistent_multiplier( state );

    m *= arcane_charge_damage_bonus();

    return m;
  }
};


// Phoenixs Flames Spell ======================================================

struct phoenixs_flames_splash_t : public fire_mage_spell_t
{
  int chain_number;
  double strafing_run_multiplier;

  phoenixs_flames_splash_t( mage_t* p ) :
    fire_mage_spell_t( "phoenixs_flames_splash", p, p -> find_spell( 224637 ) ),
    chain_number( 0 ),
    strafing_run_multiplier( p -> artifact.phoenixs_flames.data().effectN( 1 ).chain_multiplier() )
  {
    aoe = -1;
    background = true;
    triggers_ignite = true;
    // Phoenixs Flames always crits
    base_crit = 1.0;
  }

  virtual void impact( action_state_t* s ) override
  {
    // PF cleave does not impact main target
    if ( s -> chain_target == 0 )
    {
      return;
    }

    fire_mage_spell_t::impact( s );
  }

  virtual double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    // Phoenix's Flames splash deal 25% less damage compared to the
    // spell data/tooltip values. As of build 24461, 2017-07-03.
    am *= std::pow( strafing_run_multiplier, p() -> bugs ? chain_number + 1 : chain_number );

    return am;
  }
};

struct phoenixs_flames_t : public fire_mage_spell_t
{
  phoenixs_flames_splash_t* phoenixs_flames_splash;

  bool pyrotex_ignition_cloth;
  timespan_t pyrotex_ignition_cloth_reduction;

  phoenixs_flames_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "phoenixs_flames", p, p -> artifact.phoenixs_flames ),
    phoenixs_flames_splash( new phoenixs_flames_splash_t( p ) ),
    pyrotex_ignition_cloth( false ),
    pyrotex_ignition_cloth_reduction( timespan_t::zero() )
  {
    parse_options( options_str );
    // Phoenix's Flames always crits
    base_crit = 1.0;
    chain_multiplier = data().effectN( 1 ).chain_multiplier();

    // Strafing Run requires custom handling of Hot Streak
    triggers_hot_streak = false;
    triggers_ignite = true;
    triggers_pyretic_incantation = true;
    add_child( phoenixs_flames_splash );

    if ( p -> artifact.strafing_run.rank() )
    {
      aoe = 1 + p -> artifact.strafing_run.data().effectN( 1 ).base_value();
    }
  }

  virtual void execute() override
  {
    fire_mage_spell_t::execute();

    if ( pyrotex_ignition_cloth )
    {
      p() -> cooldowns.combustion
          -> adjust( -1000 * pyrotex_ignition_cloth_reduction );
    }

    if ( p() -> artifact.warmth_of_the_phoenix.rank() )
    {
      p() -> buffs.warmth_of_the_phoenix -> trigger();
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( s -> chain_target == 0 )
      {
        handle_hot_streak( s );
      }

      phoenixs_flames_splash -> chain_number = s -> chain_target;
      phoenixs_flames_splash -> set_target( s -> target );
      phoenixs_flames_splash -> execute();
    }
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t t = fire_mage_spell_t::travel_time();
    return std::min( t, timespan_t::from_seconds( 0.75 ) );
  }
};


// Pyroblast Spell ============================================================

struct pyroblast_t : public fire_mage_spell_t
{
  pyroblast_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "pyroblast", p, p -> find_specialization_spell( "Pyroblast" ) )
  {
    parse_options( options_str );

    triggers_ignite = true;
    triggers_hot_streak = true;
    triggers_pyretic_incantation = true;

    base_multiplier *= 1.0 + p -> artifact.pyroclasmic_paranoia.percent();
  }

  virtual double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    if ( p() -> buffs.kaelthas_ultimate_ability -> check() &&
         !p() -> buffs.hot_streak -> check() )
    {
      am *= 1.0 + p() -> buffs.kaelthas_ultimate_ability -> data().effectN( 1 ).percent();
    }

    if ( p() -> buffs.critical_massive -> up() )
    {
      am *= 1.0 + p() -> buffs.critical_massive -> check_value();
    }

    return am;
  }

  virtual action_state_t* new_state() override
  {
    return new ignite_spell_state_t( this, target );
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buffs.hot_streak -> check() )
    {
      return timespan_t::zero();
    }

    return fire_mage_spell_t::execute_time();
  }

  virtual void execute() override
  {
    p() -> buffs.hot_streak -> up();

    fire_mage_spell_t::execute();

    if ( p() -> buffs.kaelthas_ultimate_ability -> check() &&
        !p() -> buffs.hot_streak -> check() )
    {
      p() -> buffs.kaelthas_ultimate_ability -> expire();
    }
    if ( p() -> buffs.hot_streak -> check() )
    {
      p() -> buffs.kaelthas_ultimate_ability -> trigger();
    }

    // Ignition buff is removed shortly after Flamestrike/Pyroblast cast. In a situation
    // where you're hardcasting FS/PB followed by a Hot Streak FS/FB, both spells actually
    // benefit. As of build 24461, 2017-07-05.
    p() -> buffs.ignition -> expire( p() -> bugs ? timespan_t::from_millis( 15 ) : timespan_t::zero() );
    p() -> buffs.critical_massive -> expire();

    //TODO: Does this interact with T19 4pc?
    if ( p() -> talents.pyromaniac -> ok() &&
         rng().roll( p() -> talents.pyromaniac -> effectN( 1 ).percent() ) )
    {
      return;
    }
    else
    {
      p() -> buffs.hot_streak -> expire();
    }
  }

  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  {
    fire_mage_spell_t::snapshot_state( s, rt );

    ignite_spell_state_t* is = debug_cast<ignite_spell_state_t*>( s );
    is -> hot_streak = p() -> buffs.hot_streak -> check() != 0;
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t t = fire_mage_spell_t::travel_time();
    return std::min( t, timespan_t::from_seconds( 0.75 ) );
  }

  virtual void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( s -> result == RESULT_CRIT && p() -> talents.kindling -> ok() )
      {
        p() -> cooldowns.combustion
            -> adjust( -1000 * p() -> talents.kindling
                                   -> effectN( 1 ).time_value()  );
      }

      if ( p() -> sets -> has_set_bonus( MAGE_FIRE, T20, B4 ) && s -> result == RESULT_CRIT )
      {
        p() -> buffs.critical_massive -> trigger();
      }

      trigger_infernal_core( s -> target );
    }
  }

  virtual double composite_crit_chance() const override
  {
    double c = fire_mage_spell_t::composite_crit_chance();

    if ( p() -> buffs.ignition -> up() )
    {
      c += 1.0;
    }

    return c;
  }
   double composite_ignite_multiplier( const action_state_t* s ) const override
  {
    const ignite_spell_state_t* is = debug_cast<const ignite_spell_state_t*>( s );
    return is -> hot_streak ? 2.0 : 1.0;
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = fire_mage_spell_t::composite_target_crit_chance( target );

    if ( firestarter_active( target ) )
    {
      c = 1.0;
    }
    return c;
  }
};


// Ray of Frost Spell ===============================================================

struct ray_of_frost_t : public frost_mage_spell_t
{
  ray_of_frost_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "ray_of_frost", p, p -> talents.ray_of_frost )
  {
    parse_options( options_str );

    channeled         = true;
    hasted_ticks      = true;
  }

  void init() override
  {
    frost_mage_spell_t::init();
    update_flags |= STATE_HASTE; // Not snapshotted for this spell.
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    p() -> cooldowns.ray_of_frost -> reset( false );

    // Technically, the "castable duration" buff should be ID:208166
    // To keep things simple, we just apply a 0 stack of the damage buff 208141
    if ( !p() -> buffs.ray_of_frost -> check() )
    {
      p() -> buffs.ray_of_frost -> trigger( 0 );
    }
  }

  virtual timespan_t composite_dot_duration( const action_state_t* /* s */ )
    const override
  {
    return data().duration();
  }

  virtual void tick( dot_t* d ) override
  {
    p() -> benefits.ray_of_frost -> update();

    frost_mage_spell_t::tick( d );

    p() -> buffs.ray_of_frost -> bump( 1, p() -> buffs.ray_of_frost -> default_value );
  }

  virtual double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.ray_of_frost -> check_stack_value();

    return am;
  }
};


// Rune of Power ==============================================================

struct rune_of_power_t : public mage_spell_t
{
  rune_of_power_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "rune_of_power", p, p -> talents.rune_of_power )
  {
    parse_options( options_str );
    harmful = false;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    // Assume they're always in it
    p() -> distance_from_rune = 0;
    p() -> buffs.rune_of_power -> trigger();
  }
};


// Scorch Spell =============================================================

struct scorch_t : public fire_mage_spell_t
{
  bool koralons_burning_touch;
  double koralons_burning_touch_threshold;
  double koralons_burning_touch_multiplier;

  scorch_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "scorch", p,
                       p -> find_specialization_spell( "Scorch" ) ),
    koralons_burning_touch( false ),
    koralons_burning_touch_threshold( 0.0 ),
    koralons_burning_touch_multiplier( 0.0 )
  {
    parse_options( options_str );

    triggers_hot_streak = true;
    triggers_ignite = true;
    triggers_pyretic_incantation = true;
    consumes_ice_floes = false;
  }

  virtual double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    if ( koralons_burning_touch && ( target -> health_percentage() <= koralons_burning_touch_threshold ) )
    {
      am *= 1.0 + koralons_burning_touch_multiplier;
    }

    return am;
  }

  virtual double composite_crit_chance() const override
  {
    double c = fire_mage_spell_t::composite_crit_chance();

    if ( koralons_burning_touch && ( target -> health_percentage() <= koralons_burning_touch_threshold ) )
    {
      c = 1.0;
    }
    return c;
  }

  virtual void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( p() -> talents.frenetic_speed -> ok() )
    {
      p() -> buffs.frenetic_speed -> trigger();
    }

    if ( p() -> artifact.scorched_earth.rank() )
    {
      p() -> buffs.scorched_earth -> trigger();
    }
  }

  virtual bool usable_moving() const override
  { return true; }
};

// Shimmer Spell ============================================================

struct shimmer_t : public mage_spell_t
{
  shimmer_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "shimmer", p, p -> talents.shimmer )
  {
    parse_options( options_str );

    harmful = false;
    ignore_false_positive = true;
    base_teleport_distance = 20;

    movement_directionality = MOVEMENT_OMNI;
  }
};


// Slow Spell ===============================================================

struct slow_t : public arcane_mage_spell_t
{
  slow_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "slow", p, p -> find_specialization_spell( "Slow" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    triggers_arcane_missiles = false;
    triggers_erosion = false;
  }

  virtual void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs.slow -> trigger();
    }
  }
};

// Supernova Spell ==========================================================

struct supernova_t : public arcane_mage_spell_t
{
  int sn_aoe_am_source_id;

  supernova_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "supernova", p, p -> talents.supernova )
  {
    parse_options( options_str );

    aoe = -1;

    double sn_mult = 1.0 + p -> talents.supernova -> effectN( 1 ).percent();
    base_multiplier *= sn_mult;
    base_aoe_multiplier = 1.0 / sn_mult;
  }

  virtual bool init_finished() override
  {
    sn_aoe_am_source_id = p() -> benefits.arcane_missiles
                              -> get_source_id( "Supernova AOE" );

    return arcane_mage_spell_t::init_finished();
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();

    if ( hit_any_target && num_targets_hit > 1 )
    {
      // NOTE: Supernova AOE effect causes secondary trigger chance for AM
      // TODO: Verify this is still the case
      trigger_am( sn_aoe_am_source_id );
    }
  }
};

// Summon Water Elemental Spell ====================================================

struct summon_water_elemental_t : public frost_mage_spell_t
{
  summon_water_elemental_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "water_elemental", p, p -> find_specialization_spell( "Summon Water Elemental" ) )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;
    // TODO: Why is this not on GCD?
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    p() -> pets.water_elemental -> summon();
  }

  virtual bool ready() override
  {
    if ( !frost_mage_spell_t::ready() || p() -> talents.lonely_winter -> ok() )
      return false;

    // TODO: Check this
    return !p() -> pets.water_elemental ||
           p() -> pets.water_elemental -> is_sleeping();
  }
};

// Summon Arcane Familiar Spell ===============================================

struct arcane_assault_t : public arcane_mage_spell_t
{
  arcane_assault_t( mage_t* p )
    : arcane_mage_spell_t( "arcane_assault", p,  p -> find_spell( 225119 ) )
  {
    background = true;
  }
};

struct summon_arcane_familiar_t : public arcane_mage_spell_t
{
  summon_arcane_familiar_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "summon_arcane_familiar", p,
                         p -> talents.arcane_familiar )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();

    p() -> buffs.arcane_familiar -> trigger();
  }

  virtual bool ready() override
  {
    if ( p() -> buffs.arcane_familiar -> check() )
    {
      return false;
    }

    return arcane_mage_spell_t::ready();
  }
};


// Time Warp Spell ============================================================

struct time_warp_t : public mage_spell_t
{
  time_warp_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "time_warp", p, p -> find_class_spell( "Time Warp" ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if ( p -> buffs.exhaustion -> check() || p -> is_pet() )
        continue;

      p -> buffs.bloodlust -> trigger();
      p -> buffs.exhaustion -> trigger();
    }

    // If Shard of the Exodar is equipped, trigger bloodlust regardless.
    if ( p() -> player_t::buffs.bloodlust -> default_chance == 0.0 )
    {
      p() -> player_t::buffs.bloodlust -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
    }
  }

  virtual bool ready() override
  {
    // If we have shard of the exodar, we're controlling our own destiny. Overrides don't
    // apply to us.
    bool shard = p() -> player_t::buffs.bloodlust -> default_chance == 0.0;

    if ( !shard && sim -> overrides.bloodlust )
      return false;

    if ( !shard && player -> buffs.exhaustion -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Touch of the Magi ==========================================================

struct touch_of_the_magi_explosion_t : public arcane_mage_spell_t
{
  touch_of_the_magi_explosion_t( mage_t* p ) :
    arcane_mage_spell_t( "touch_of_the_magi", p, p -> find_spell( 210833 ) )
  {
    background = true;
    may_miss = may_crit = callbacks = false;
    aoe = -1;

    affected_by.erosion = false;
  }

  virtual void init() override
  {
    mage_spell_t::init();
    // disable the snapshot_flags for all multipliers
    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_TGT_MUL_DA;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = arcane_mage_spell_t::composite_target_multiplier( target );

    // It seems that TotM explosion only double dips on target based damage reductions
    // and not target based damage increases.
    m = std::min( m, 1.0 );

    return m;
  }

  virtual void execute() override
  {
    base_dd_adder *= p() -> artifact.touch_of_the_magi.data().effectN( 1 ).percent();

    mage_spell_t::execute();
  }
};


// ============================================================================
// Mage Custom Actions
// ============================================================================

// Arcane Mage "Burn" State Switch Action =====================================

struct start_burn_phase_t : public action_t
{
  start_burn_phase_t( mage_t* p, const std::string& options_str ):
    action_t( ACTION_USE, "start_burn_phase", p )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
    harmful = false;
    ignore_false_positive = true;
    action_skill = 1;
  }

  virtual void execute() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    bool success = p -> burn_phase.enable( sim -> current_time() );
    if ( !success )
    {
      sim -> errorf( "%s start_burn_phase infinite loop detected (no time passing between executes) at '%s'",
                     p -> name(), signature_str.c_str() );
      sim -> cancel_iteration();
      sim -> cancel();
      return;
    }
  }

  bool ready() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    if ( p -> burn_phase.on() )
    {
      return false;
    }

    return action_t::ready();
  }
};

struct stop_burn_phase_t : public action_t
{
  stop_burn_phase_t( mage_t* p, const std::string& options_str ):
     action_t( ACTION_USE, "stop_burn_phase", p )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
    harmful = false;
    ignore_false_positive = true;
    action_skill = 1;
  }

  virtual void execute() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    bool success = p -> burn_phase.disable( sim -> current_time() );
    if ( !success )
    {
      sim -> errorf( "%s stop_burn_phase infinite loop detected (no time passing between executes) at '%s'",
                     p -> name(), signature_str.c_str() );
      sim -> cancel_iteration();
      sim -> cancel();
      return;
    }
  }

  bool ready() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    if ( !p -> burn_phase.on() )
    {
      return false;
    }

    return action_t::ready();
  }
};


// Unstable Magic =============================================================

struct unstable_magic_explosion_t : public mage_spell_t
{
  unstable_magic_explosion_t( mage_t* p ) :
    mage_spell_t( "unstable_magic_explosion", p, p -> talents.unstable_magic )
  {
    may_miss = may_crit = false;
    callbacks = false;
    aoe = -1;
    background = true;

    switch ( p -> specialization() )
    {
      case MAGE_ARCANE:
        school = SCHOOL_ARCANE;
        break;
      case MAGE_FIRE:
        school = SCHOOL_FIRE;
        break;
      case MAGE_FROST:
        school = SCHOOL_FROST;
        break;
      default:
        // This shouldn't happen
        break;
    }
  }

  virtual void init() override
  {
    mage_spell_t::init();
    // disable the snapshot_flags for all multipliers
    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_TGT_MUL_DA;
  }

  virtual void execute() override
  {
    base_dd_adder *= data().effectN( 4 ).percent();

    mage_spell_t::execute();
  }
};

void mage_spell_t::trigger_unstable_magic( action_state_t* s )
{
  if ( ! p() -> talents.unstable_magic -> ok() )
    return;

  assert( p() -> action.unstable_magic_explosion );

  double um_proc_rate;
  switch ( p() -> specialization() )
  {
    case MAGE_ARCANE:
      um_proc_rate = p() -> action.unstable_magic_explosion
                         -> data().effectN( 1 ).percent();
      break;
    case MAGE_FROST:
      um_proc_rate = p() -> action.unstable_magic_explosion
                         -> data().effectN( 2 ).percent();
      break;
    case MAGE_FIRE:
      um_proc_rate = p() -> action.unstable_magic_explosion
                         -> data().effectN( 3 ).percent();
      break;
    default:
      um_proc_rate = 0.0;
      break;
  }

  if ( p() -> rng().roll( um_proc_rate ) )
  {
    p() -> action.unstable_magic_explosion -> set_target( s -> target );
    p() -> action.unstable_magic_explosion -> base_dd_adder = s -> result_amount;
    p() -> action.unstable_magic_explosion -> execute();
  }
}

// Proxy Freeze action ========================================================

struct freeze_t : public action_t
{
  pets::water_elemental::freeze_t* action;

  freeze_t( mage_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "freeze", p ), action( nullptr )
  {
    parse_options( options_str );

    may_miss = may_crit = callbacks = false;
    dual = true;
    trigger_gcd = timespan_t::zero();
    ignore_false_positive = true;
    action_skill = 1;
  }

  void reset() override
  {
    action_t::reset();

    mage_t* m = debug_cast<mage_t*>( player );

    if ( m -> pets.water_elemental && ! action )
    {
      action         = dynamic_cast<pets::water_elemental::freeze_t*   >( m -> pets.water_elemental -> find_action( "freeze"    ) );
      auto water_jet = dynamic_cast<pets::water_elemental::water_jet_t*>( m -> pets.water_elemental -> find_action( "water_jet" ) );
      if ( action && water_jet )
      {
        water_jet -> autocast = false;
      }
    }
  }

  void execute() override
  {
    assert( action );

    action -> set_target( target );
    action -> execute();
  }

  bool ready() override
  {
    mage_t* m = debug_cast<mage_t*>( player );
    if ( m -> talents.lonely_winter -> ok() )
    {
      return false;
    }

    if ( !action )
      return false;

    if ( !action -> ready() )
      return false;

    return action_t::ready();
  }
};

// Proxy cast Water Jet Action ================================================

struct water_jet_t : public action_t
{
  pets::water_elemental::water_jet_t* action;

  water_jet_t( mage_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "water_jet", p ), action( nullptr )
  {
    parse_options( options_str );

    may_miss = may_crit = callbacks = false;
    dual = true;
    trigger_gcd = timespan_t::zero();
    ignore_false_positive = true;
    action_skill = 1;
  }

  void reset() override
  {
    action_t::reset();

    mage_t* m = debug_cast<mage_t*>( player );

    if ( m -> pets.water_elemental && ! action )
    {
      action = dynamic_cast<pets::water_elemental::water_jet_t*>( m -> pets.water_elemental -> find_action( "water_jet" ) );
      if ( action )
      {
        action -> autocast = false;
      }
    }
  }

  void execute() override
  {
    assert( action );
    mage_t* m = debug_cast<mage_t*>( player );
    action -> queued = true;
    // Interrupt existing cast
    if ( m -> pets.water_elemental -> executing )
    {
      m -> pets.water_elemental -> executing -> interrupt_action();
    }

    // Cancel existing (potential) player-ready event ..
    if ( m -> pets.water_elemental -> readying )
    {
      event_t::cancel( m -> pets.water_elemental -> readying );
    }

    // and schedule a new one immediately.
    m -> pets.water_elemental -> schedule_ready();
  }

  bool ready() override
  {
    mage_t* m = debug_cast<mage_t*>( player );
    if ( m -> talents.lonely_winter -> ok() )
    {
      return false;
    }

    if ( !action )
      return false;

    // Ensure that the Water Elemental's water_jet is ready. Note that this
    // skips the water_jet_t::ready() call, and simply checks the "base" ready
    // properties of the spell (most importantly, the cooldown). If normal
    // ready() was called, this would always return false, as queued = false,
    // before this action executes.
    if ( ! action -> spell_t::ready() )
      return false;

    // Don't re-execute if water jet is already queued
    if ( action -> queued )
      return false;

    return action_t::ready();
  }
};
} // namespace actions


namespace events {
struct icicle_event_t : public event_t
{
  mage_t* mage;
  player_t* target;
  icicle_data_t state;

  icicle_event_t( mage_t& m, const icicle_data_t& s, player_t* t, bool first = false ) :
    event_t( m ), mage( &m ), target( t ), state( s )
  {
    double cast_time = first ? 0.25 : ( 0.4 * mage -> cache.spell_speed() );

    schedule( timespan_t::from_seconds( cast_time ) );
  }
  virtual const char* name() const override
  { return "icicle_event"; }

  void execute() override
  {
    // If the target of the icicle is dead, stop the chain
    if ( target -> is_sleeping() )
    {
      if ( mage -> sim -> debug )
        mage -> sim -> out_debug.printf( "%s icicle use on %s (sleeping target), stopping",
            mage -> name(), target -> name() );
      mage -> icicle_event = nullptr;
      return;
    }

    mage -> icicle -> set_target( target );
    auto new_s = debug_cast<actions::icicle_state_t*>( mage -> icicle -> get_state() );
    mage -> icicle -> snapshot_state( new_s, mage -> icicle -> amount_type( new_s ) );
    new_s -> source = state.stats;
    new_s -> target = target;

    mage -> icicle -> base_dd_adder = state.damage;

    // Immediately execute icicles so the correct damage is carried into the
    // travelling icicle object
    mage -> icicle -> pre_execute_state = new_s;
    mage -> icicle -> execute();

    mage -> buffs.icicles -> decrement();

    icicle_data_t new_state = mage -> get_icicle_object();
    if ( new_state.damage > 0 )
    {
      mage -> icicle_event = make_event<icicle_event_t>( sim(), *mage, new_state, target );
      if ( mage -> sim -> debug )
        mage -> sim -> out_debug.printf( "%s icicle use on %s (chained), damage=%f, total=%u",
                               mage -> name(), target -> name(), new_state.damage, as<unsigned>( mage -> icicles.size() ) );
    }
    else
      mage -> icicle_event = nullptr;
  }
};

struct ignite_spread_event_t : public event_t
{
  mage_t* mage;

  static double ignite_bank( dot_t* ignite )
  {
    if ( !ignite -> is_ticking() )
    {
      return 0.0;
    }

    auto ignite_state = debug_cast<residual_action::residual_periodic_state_t*>(
        ignite -> state );
    return ignite_state -> tick_amount * ignite -> ticks_left();
  }

  static bool ignite_compare ( dot_t* a, dot_t* b )
  {
    return ignite_bank( a ) > ignite_bank( b );
  }

  ignite_spread_event_t( mage_t& m, timespan_t delta_time ) :
    event_t( m, delta_time ), mage( &m )
  { }

  virtual const char* name() const override
  { return "ignite_spread_event"; }

  void execute() override
  {
    mage -> procs.ignite_spread -> occur();
    if ( mage -> sim -> log )
    {
      sim().out_log.printf( "%s ignite spread event occurs", mage -> name() );
    }

    std::vector< player_t* > tl = mage -> ignite -> target_list();

    if ( tl.size() == 1 )
    {
      return;
    }

    std::vector< dot_t* > active_ignites;
    std::vector< dot_t* > candidates;
    // Split ignite targets by whether ignite is ticking
    for ( size_t i = 0, actors = tl.size(); i < actors; i++ )
    {
      player_t* t = tl[ i ];

      dot_t* ignite = t -> get_dot( "ignite", mage );
      if ( ignite -> is_ticking() )
      {
        active_ignites.push_back( ignite );
      }
      else
      {
        candidates.push_back( ignite );
      }
    }

    // Sort active ignites by descending bank size
    std::sort( active_ignites.begin(), active_ignites.end(), ignite_compare );

    // Loop over active ignites:
    // - Pop smallest ignite for spreading
    // - Remove equal sized ignites from tail of spread candidate list
    // - Choose random target and execute spread
    // - Remove spread destination from candidate list
    // - Add spreaded ignite source to candidate list
    // This algorithm provides random selection of the spread target, while
    // guaranteeing that every source will have a larger ignite bank than the
    // destination. It also guarantees that each ignite will spread to a unique
    // target. This allows us to avoid N^2 spread validity checks.
    while ( active_ignites.size() > 0 )
    {
      dot_t* source = active_ignites.back();
      active_ignites.pop_back();
      double source_bank = ignite_bank(source);

      if ( !candidates.empty() )
      {
        // Skip candidates that have equal ignite bank size to the source
        int index = as<int>( candidates.size() ) - 1;
        while ( index >= 0 )
        {
          if ( ignite_bank( candidates[ index ] ) < source_bank )
          {
            break;
          }
          index--;
        }
        if ( index < 0 )
        {
          // No valid spread targets
          continue;
        }

        // TODO: Filter valid candidates by ignite spread range

        // Randomly select spread target from remaining candidates
        index = as<int>( floor( mage -> rng().real() * index ) );
        dot_t* destination = candidates[ index ];

        if ( destination -> is_ticking() )
        {
          // TODO: Use benefits to keep track of lost ignite banks
          destination -> cancel();
          mage -> procs.ignite_overwrite -> occur();
          if ( mage -> sim -> log )
          {
            sim().out_log.printf( "%s ignite spreads from %s to %s (overwrite)",
                                 mage -> name(), source -> target -> name(),
                                 destination -> target -> name() );
          }
        }
        else
        {
          mage -> procs.ignite_new_spread -> occur();
          if ( mage -> sim -> log )
          {
            sim().out_log.printf( "%s ignite spreads from %s to %s (new)",
                                 mage -> name(), source -> target -> name(),
                                 destination -> target -> name() );
          }
        }
        source -> copy( destination -> target, DOT_COPY_CLONE );

        // Remove spread destination from candidates
        candidates.erase( candidates.begin() + index );
      }

      // Add spread source to candidates
      candidates.push_back( source );
    }

    // Schedule next spread for 2 seconds later
    mage -> ignite_spread_event = make_event<events::ignite_spread_event_t>(
        sim(), *mage, timespan_t::from_seconds( 2.0 ) );
  }
};
} // namespace events

// ==========================================================================
// Mage Character Definition
// ==========================================================================

// mage_td_t ================================================================

mage_td_t::mage_td_t( player_t* target, mage_t* mage ) :
  actor_target_data_t( target, mage ),
  dots( dots_t() ),
  debuffs( debuffs_t() )
{
  dots.blast_furnace     = target -> get_dot( "blast_furnace",     mage );
  dots.conflagration_dot = target -> get_dot( "conflagration_dot", mage );
  dots.ignite            = target -> get_dot( "ignite",            mage );
  dots.living_bomb       = target -> get_dot( "living_bomb",       mage );
  dots.mark_of_aluneth   = target -> get_dot( "mark_of_aluneth",   mage );
  dots.nether_tempest    = target -> get_dot( "nether_tempest",    mage );

  debuffs.erosion     = new buffs::erosion_t( this );
  debuffs.slow        = buff_creator_t( *this, "slow",
                                        mage -> find_spell( 31589 ) );
  debuffs.touch_of_the_magi = new buffs::touch_of_the_magi_t( this );

  debuffs.frost_bomb  = buff_creator_t( *this, "frost_bomb",
                                        mage -> talents.frost_bomb );
  debuffs.frozen      = buff_creator_t( *this, "frozen" )
                          .duration( timespan_t::from_seconds( 0.5 ) );
  debuffs.water_jet   = buff_creator_t( *this, "water_jet",
                                        mage -> find_spell( 135029 ) )
                          .quiet( true )
                          .cd( timespan_t::zero() );
  debuffs.winters_chill = buff_creator_t( *this, "winters_chill",
                                        mage -> find_spell( 228358 ) );

}

mage_t::mage_t( sim_t* sim, const std::string& name, race_e r ) :
  player_t( sim, MAGE, name, r ),
  icicle( nullptr ),
  icicle_event( nullptr ),
  ignite( nullptr ),
  ignite_spread_event( nullptr ),
  last_bomb_target( nullptr ),
  active_meteor_burn( nullptr ),
  distance_from_rune( 0.0 ),
  global_cinder_count( 0.0 ),
  firestarter_time( timespan_t::zero() ),
  blessing_of_wisdom_count( 0 ),
  action( actions_t() ),
  benefits( benefits_t() ),
  buffs( buffs_t() ),
  cooldowns( cooldowns_t() ),
  gains( gains_t() ),
  pets( pets_t() ),
  procs( procs_t() ),
  spec( specializations_t() ),
  state( state_t() ),
  talents( talents_list_t() )
{
  // Cooldowns
  cooldowns.combustion       = get_cooldown( "combustion"       );
  cooldowns.cone_of_cold     = get_cooldown( "cone_of_cold"     );
  cooldowns.evocation        = get_cooldown( "evocation"        );
  cooldowns.frost_nova       = get_cooldown( "frost_nova"       );
  cooldowns.frozen_orb       = get_cooldown( "frozen_orb"       );
  cooldowns.icy_veins        = get_cooldown( "icy_veins"        );
  cooldowns.phoenixs_flames  = get_cooldown( "phoenixs_flames"  );
  cooldowns.presence_of_mind = get_cooldown( "presence_of_mind" );
  cooldowns.ray_of_frost     = get_cooldown( "ray_of_frost"     );
  cooldowns.time_warp        = get_cooldown( "time_warp"        );

  // Options
  regen_type = REGEN_DYNAMIC;
  regen_caches[ CACHE_MASTERY ] = true;

  talent_points.register_validity_fn( [ this ] ( const spell_data_t* spell )
  {
    // Soul of the Archmage
    if ( find_item( 151642 ) )
    {
      switch ( specialization() )
      {
        case MAGE_ARCANE:
          return spell -> id() == 234302; // Temporal Flux
        case MAGE_FIRE:
          return spell -> id() == 205029; // Flame On
        case MAGE_FROST:
          return spell -> id() == 205030; // Frozen Touch
        default:
          break;
      }
    }

    return false;
  } );
}


mage_t::~mage_t()
{
  delete benefits.incanters_flow;
  delete benefits.arcane_charge.arcane_barrage;
  delete benefits.arcane_charge.arcane_blast;
  delete benefits.arcane_charge.arcane_explosion;
  delete benefits.arcane_charge.arcane_missiles;
  delete benefits.arcane_charge.nether_tempest;
  delete benefits.arcane_missiles;
  delete benefits.fingers_of_frost;
  delete benefits.ray_of_frost;
}

/// Touch of the Magi explosion trigger
void mage_t::trigger_touch_of_the_magi( buffs::touch_of_the_magi_t* buff )
{
  assert( action.touch_of_the_magi_explosion );
  action.touch_of_the_magi_explosion -> set_target( buff -> player );
  action.touch_of_the_magi_explosion -> base_dd_adder = buff -> accumulated_damage;
  action.touch_of_the_magi_explosion -> execute();
}

void mage_t::trigger_t19_oh()
{
  if ( sets -> has_set_bonus( specialization(), T19OH, B8 ) )
  {
    buffs.t19_oh_buff -> trigger();
  }
}

bool mage_t::apply_crowd_control( const action_state_t* state, spell_mechanic type )
{
  if ( type == MECHANIC_INTERRUPT )
  {
    buffs.sephuzs_secret -> trigger();
    return true;
  }

  if ( action_t::result_is_hit( state -> result )
    && ( state -> target -> is_add() || state -> target -> level() < sim -> max_player_level + 3 ) )
  {
    buffs.sephuzs_secret -> trigger();
    if ( type == MECHANIC_ROOT )
    {
      get_target_data( state -> target ) -> debuffs.frozen -> trigger();
    }

    return true;
  }

  return false;
}

// mage_t::create_action ====================================================

action_t* mage_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  using namespace actions;

  // Arcane
  if ( name == "arcane_barrage"    ) return new             arcane_barrage_t( this, options_str );
  if ( name == "arcane_blast"      ) return new               arcane_blast_t( this, options_str );
  if ( name == "arcane_explosion"  ) return new           arcane_explosion_t( this, options_str );
  if ( name == "arcane_missiles"   ) return new            arcane_missiles_t( this, options_str );
  if ( name == "arcane_orb"        ) return new                 arcane_orb_t( this, options_str );
  if ( name == "arcane_power"      ) return new               arcane_power_t( this, options_str );
  if ( name == "charged_up"        ) return new                 charged_up_t( this, options_str );
  if ( name == "evocation"         ) return new                  evocation_t( this, options_str );
  if ( name == "nether_tempest"    ) return new             nether_tempest_t( this, options_str );
  if ( name == "presence_of_mind"  ) return new           presence_of_mind_t( this, options_str );
  if ( name == "slow"              ) return new                       slow_t( this, options_str );
  if ( name == "summon_arcane_familiar") return new summon_arcane_familiar_t( this, options_str );
  if ( name == "supernova"         ) return new                  supernova_t( this, options_str );

  if ( name == "start_burn_phase"  ) return new           start_burn_phase_t( this, options_str );
  if ( name == "stop_burn_phase"   ) return new            stop_burn_phase_t( this, options_str );

  // Fire
  if ( name == "blast_wave"        ) return new              blast_wave_t( this, options_str );
  if ( name == "cinderstorm"       ) return new             cinderstorm_t( this, options_str );
  if ( name == "combustion"        ) return new              combustion_t( this, options_str );
  if ( name == "dragons_breath"    ) return new          dragons_breath_t( this, options_str );
  if ( name == "fireball"          ) return new                fireball_t( this, options_str );
  if ( name == "flamestrike"       ) return new             flamestrike_t( this, options_str );
  if ( name == "fire_blast"        ) return new              fire_blast_t( this, options_str );
  if ( name == "living_bomb"       ) return new             living_bomb_t( this, options_str );
  if ( name == "meteor"            ) return new                  meteor_t( this, options_str );
  if ( name == "pyroblast"         ) return new               pyroblast_t( this, options_str );
  if ( name == "scorch"            ) return new                  scorch_t( this, options_str );

  // Frost
  if ( name == "blizzard"          ) return new                blizzard_t( this, options_str );
  if ( name == "cold_snap"         ) return new               cold_snap_t( this, options_str );
  if ( name == "comet_storm"       ) return new             comet_storm_t( this, options_str );
  if ( name == "cone_of_cold"      ) return new            cone_of_cold_t( this, options_str );
  if ( name == "flurry"            ) return new                  flurry_t( this, options_str );
  if ( name == "freeze"            ) return new                  freeze_t( this, options_str );
  if ( name == "frost_bomb"        ) return new              frost_bomb_t( this, options_str );
  if ( name == "frostbolt"         ) return new               frostbolt_t( this, options_str );
  if ( name == "frozen_orb"        ) return new              frozen_orb_t( this, options_str );
  if ( name == "glacial_spike"     ) return new           glacial_spike_t( this, options_str );
  if ( name == "ice_floes"         ) return new               ice_floes_t( this, options_str );
  if ( name == "ice_lance"         ) return new               ice_lance_t( this, options_str );
  if ( name == "ice_nova"          ) return new                ice_nova_t( this, options_str );
  if ( name == "icy_veins"         ) return new               icy_veins_t( this, options_str );
  if ( name == "ray_of_frost"      ) return new            ray_of_frost_t( this, options_str );
  if ( name == "water_elemental"   ) return new  summon_water_elemental_t( this, options_str );
  if ( name == "water_jet"         ) return new               water_jet_t( this, options_str );

  // Artifact Specific Spells
  // Arcane
  if ( name == "mark_of_aluneth"   ) return new          mark_of_aluneth_t( this, options_str );

  // Fire
  if ( name == "phoenixs_flames"   ) return new          phoenixs_flames_t( this, options_str );

  // Frost
  if ( name == "ebonbolt"          ) return new                 ebonbolt_t( this, options_str );

  // Shared spells
  if ( name == "blink"             )
  {
    if ( talents.shimmer -> ok() )
    {
      return new shimmer_t( this, options_str );
    }
    else
    {
      return new blink_t( this, options_str );
    }
  }
  if ( name == "counterspell"      ) return new            counterspell_t( this, options_str );
  if ( name == "frost_nova"        ) return new              frost_nova_t( this, options_str );
  if ( name == "time_warp"         ) return new               time_warp_t( this, options_str );

  // Shared talents
  if ( name == "mage_bomb"         )
  {
    if ( talents.frost_bomb -> ok() )
    {
      return new frost_bomb_t( this, options_str );
    }
    else if ( talents.living_bomb -> ok() )
    {
      return new living_bomb_t( this, options_str );
    }
    else if ( talents.nether_tempest -> ok() )
    {
      return new nether_tempest_t( this, options_str );
    }
  }
  if ( name == "mirror_image"      ) return new            mirror_image_t( this, options_str );
  if ( name == "rune_of_power"     ) return new           rune_of_power_t( this, options_str );
  if ( name == "shimmer"           ) return new                 shimmer_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// mage_t::create_actions =====================================================

bool mage_t::create_actions()
{
  using namespace actions;

  if ( spec.ignite -> ok() )
  {
    ignite = new ignite_t( this );
  }

  if ( spec.icicles -> ok() )
  {
    icicle = new icicle_t( this );
  }

  if ( talents.arcane_familiar -> ok() )
  {
    action.arcane_assault = new arcane_assault_t( this );
  }

  if ( talents.frost_bomb -> ok() )
  {
    action.frost_bomb_explosion = new frost_bomb_explosion_t( this );
  }

  if ( talents.unstable_magic -> ok() )
  {
    action.unstable_magic_explosion = new unstable_magic_explosion_t( this );
  }

  if ( artifact.touch_of_the_magi.rank() )
  {
    action.touch_of_the_magi_explosion = new touch_of_the_magi_explosion_t( this );
  }

  // Global actions for 7.2.5 legendaries.
  // TODO: Probably a better idea to construct these in the legendary callbacks?
  switch ( specialization() )
  {
    case MAGE_ARCANE:
      action.legendary_arcane_orb  = new arcane_orb_t ( this, "", true );
      break;
    case MAGE_FIRE:
      action.legendary_meteor      = new meteor_t     ( this, "", true );
      break;
    case MAGE_FROST:
      action.legendary_comet_storm = new comet_storm_t( this, "", true );
      break;
    default:
      break;
  }

  return player_t::create_actions();
}

// mage_t::create_options =====================================================
void mage_t::create_options()
{
  add_option( opt_float( "global_cinder_count", global_cinder_count ) );
  add_option( opt_timespan( "firestarter_time", firestarter_time ) );
  add_option( opt_int( "blessing_of_wisdom_count", blessing_of_wisdom_count ) );
  player_t::create_options();
}

// mage_t::create_profile ================================================

std::string mage_t::create_profile( save_e save_type )
{
  std::string profile = player_t::create_profile( save_type );

  if ( save_type == SAVE_ALL )
  {
    if ( firestarter_time > timespan_t::zero() )
    {
      profile += "firestarter_time=" + util::to_string( firestarter_time.total_seconds() ) + "\n";
    }
  }

  return profile;
}

// mage_t::copy_from =====================================================

void mage_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  mage_t* p = debug_cast<mage_t*>( source );

  global_cinder_count       = p -> global_cinder_count;
  firestarter_time          = p -> firestarter_time;
  blessing_of_wisdom_count  = p -> blessing_of_wisdom_count;
}

// mage_t::create_pets ========================================================

void mage_t::create_pets()
{
  if ( specialization() == MAGE_FROST && ! talents.lonely_winter -> ok() && find_action( "water_elemental" ) )
  {
    pets.water_elemental = new pets::water_elemental::water_elemental_pet_t( sim, this );
  }

  if ( talents.mirror_image -> ok() && find_action( "mirror_image" ) )
  {
    int image_num = talents.mirror_image -> effectN( 2 ).base_value();
    for ( int i = 0; i < image_num; i++ )
    {
      pets.mirror_images.push_back( new pets::mirror_image::mirror_image_pet_t( sim, this ) );
      if ( i > 0 )
      {
        pets.mirror_images[ i ] -> quiet = 1;
      }
    }
  }
}

// mage_t::init_spells ========================================================

void mage_t::init_spells()
{
  player_t::init_spells();

  // Talents
  // Tier 15
  talents.arcane_familiar    = find_talent_spell( "Arcane Familiar"    );
  talents.amplification      = find_talent_spell( "Amplification"      );
  talents.words_of_power     = find_talent_spell( "Words of Power"     );
  talents.pyromaniac         = find_talent_spell( "Pyromaniac"         );
  talents.conflagration      = find_talent_spell( "Conflagration"      );
  talents.firestarter        = find_talent_spell( "Firestarter"        );
  talents.ray_of_frost       = find_talent_spell( "Ray of Frost"       );
  talents.lonely_winter      = find_talent_spell( "Lonely Winter"      );
  talents.bone_chilling      = find_talent_spell( "Bone Chilling"      );
  // Tier 30
  talents.shimmer            = find_talent_spell( "Shimmer"            );
  talents.slipstream         = find_talent_spell( "Slipstream"         );
  talents.blast_wave         = find_talent_spell( "Blast Wave"         );
  talents.ice_floes          = find_talent_spell( "Ice Floes"          );
  talents.mana_shield        = find_talent_spell( "Mana Shield"        );
  talents.blazing_soul       = find_talent_spell( "Blazing Soul"       );
  talents.glacial_insulation = find_talent_spell( "Glacial Insulation" );
  // Tier 45
  talents.mirror_image       = find_talent_spell( "Mirror Image"       );
  talents.rune_of_power      = find_talent_spell( "Rune of Power"      );
  talents.incanters_flow     = find_talent_spell( "Incanter's Flow"    );
  // Tier 60
  talents.supernova          = find_talent_spell( "Supernova"          );
  talents.charged_up         = find_talent_spell( "Charged Up"         );
  talents.resonance          = find_talent_spell( "Resonance"          );
  talents.alexstraszas_fury  = find_talent_spell( "Alexstrasza's Fury" );
  talents.flame_on           = find_talent_spell( "Flame On"           );
  talents.controlled_burn    = find_talent_spell( "Controlled Burn"    );
  talents.ice_nova           = find_talent_spell( "Ice Nova"           );
  talents.frozen_touch       = find_talent_spell( "Frozen Touch"       );
  talents.splitting_ice      = find_talent_spell( "Splitting Ice"      );
  // Tier 75
  talents.chrono_shift       = find_talent_spell( "Chrono Shift"       );
  talents.frenetic_speed     = find_talent_spell( "Frenetic Speed"     );
  talents.frigid_winds       = find_talent_spell( "Frigid Winds"       );
  talents.ring_of_frost      = find_talent_spell( "Ring of Frost"      );
  talents.ice_ward           = find_talent_spell( "Ice Ward"           );
  // Tier 90
  talents.nether_tempest     = find_talent_spell( "Nether Tempest"     );
  talents.living_bomb        = find_talent_spell( "Living Bomb"        );
  talents.frost_bomb         = find_talent_spell( "Frost Bomb"         );
  talents.unstable_magic     = find_talent_spell( "Unstable Magic"     );
  talents.erosion            = find_talent_spell( "Erosion"            );
  talents.flame_patch        = find_talent_spell( "Flame Patch"        );
  talents.arctic_gale        = find_talent_spell( "Arctic Gale"        );
  // Tier 100
  talents.overpowered        = find_talent_spell( "Overpowered"        );
  talents.temporal_flux      = find_talent_spell( "Temporal Flux"      );
  talents.arcane_orb         = find_talent_spell( "Arcane Orb"         );
  talents.kindling           = find_talent_spell( "Kindling"           );
  talents.cinderstorm        = find_talent_spell( "Cinderstorm"        );
  talents.meteor             = find_talent_spell( "Meteor"             );
  talents.thermal_void       = find_talent_spell( "Thermal Void"       );
  talents.glacial_spike      = find_talent_spell( "Glacial Spike"      );
  talents.comet_storm        = find_talent_spell( "Comet Storm"        );

  //Artifact Spells
  //Arcane
  artifact.aegwynns_ascendance           = find_artifact_spell( "Aegwynn's Ascendance"          );
  artifact.aegwynns_fury                 = find_artifact_spell( "Aegwynn's Fury"                );
  artifact.aegwynns_imperative           = find_artifact_spell( "Aegwynn's Imperative"          );
  artifact.aegwynns_intensity            = find_artifact_spell( "Aegwynn's Intensity"           );
  artifact.aegwynns_wrath                = find_artifact_spell( "Aegwynn's Wrath"               );
  artifact.aluneths_avarice              = find_artifact_spell( "Aluneth's Avarice"             );
  artifact.arcane_purification           = find_artifact_spell( "Arcane Purification"           );
  artifact.arcane_rebound                = find_artifact_spell( "Arcane Rebound"                );
  artifact.blasting_rod                  = find_artifact_spell( "Blasting Rod"                  );
  artifact.crackling_energy              = find_artifact_spell( "Crackling Energy"              );
  artifact.intensity_of_the_tirisgarde   = find_artifact_spell( "Intensity of the Tirisgarde"   );
  artifact.mark_of_aluneth               = find_artifact_spell( "Mark of Aluneth"               );
  artifact.might_of_the_guardians        = find_artifact_spell( "Might of the Guardians"        );
  artifact.rule_of_threes                = find_artifact_spell( "Rule of Threes"                );
  artifact.torrential_barrage            = find_artifact_spell( "Torrential Barrage"            );
  artifact.everywhere_at_once            = find_artifact_spell( "Everywhere At Once"            );
  artifact.ethereal_sensitivity          = find_artifact_spell( "Ethereal Sensitivity"          );
  artifact.time_and_space                = find_artifact_spell( "Time and Space"                );
  artifact.touch_of_the_magi             = find_artifact_spell( "Touch of the Magi"             );
  artifact.ancient_power                 = find_artifact_spell( "Ancient Power"                 );
  //Fire
  artifact.aftershocks                   = find_artifact_spell( "Aftershocks"                   );
  artifact.scorched_earth                = find_artifact_spell( "Scorched Earth"                );
  artifact.big_mouth                     = find_artifact_spell( "Big Mouth"                     );
  artifact.blue_flame_special            = find_artifact_spell( "Blue Flame Special"            );
  artifact.everburning_consumption       = find_artifact_spell( "Everburning Consumption"       );
  artifact.instability_of_the_tirisgarde = find_artifact_spell( "Instability of the Tirisgarde" );
  artifact.molten_skin                   = find_artifact_spell( "Molten Skin"                   );
  artifact.phoenix_reborn                = find_artifact_spell( "Phoenix Reborn"                );
  artifact.phoenixs_flames               = find_artifact_spell( "Phoenix's Flames"              );
  artifact.great_balls_of_fire           = find_artifact_spell( "Great Balls of Fire"           );
  artifact.cauterizing_blink             = find_artifact_spell( "Cauterizing Blink"             );
  artifact.fire_at_will                  = find_artifact_spell( "Fire At Will"                  );
  artifact.preignited                    = find_artifact_spell( "Pre-Ignited"                   );
  artifact.pyroclasmic_paranoia          = find_artifact_spell( "Pyroclasmic Paranoia"          );
  artifact.pyretic_incantation           = find_artifact_spell( "Pyretic Incantation"           );
  artifact.reignition_overdrive          = find_artifact_spell( "Reignition Overdrive"          );
  artifact.strafing_run                  = find_artifact_spell( "Strafing Run"                  );
  artifact.burning_gaze                  = find_artifact_spell( "Burning Gaze"                  );
  artifact.blast_furnace                 = find_artifact_spell( "Blast Furnace"                 );
  artifact.warmth_of_the_phoenix         = find_artifact_spell( "Warmth of the Phoenix"         );
  artifact.wings_of_flame                = find_artifact_spell( "Wings of Flame"                );
  artifact.empowered_spellblade          = find_artifact_spell( "Empowered Spellblade"          );
  //Frost
  artifact.black_ice                     = find_artifact_spell( "Black Ice"                     );
  artifact.chain_reaction                = find_artifact_spell( "Chain Reaction"                );
  artifact.chilled_to_the_core           = find_artifact_spell( "Chilled To The Core"           );
  artifact.clarity_of_thought            = find_artifact_spell( "Clarity of Thought"            );
  artifact.ebonbolt                      = find_artifact_spell( "Ebonbolt"                      );
  artifact.freezing_rain                 = find_artifact_spell( "Freezing Rain"                 );
  artifact.frigidity_of_the_tirisgarde   = find_artifact_spell( "Frigidity of the Tirisgarde"   );
  artifact.frozen_veins                  = find_artifact_spell( "Frozen Veins"                  );
  artifact.glacial_eruption              = find_artifact_spell( "Glacial Eruption"              );
  artifact.ice_age                       = find_artifact_spell( "Ice Age"                       );
  artifact.ice_nine                      = find_artifact_spell( "Ice Nine"                      );
  artifact.icy_caress                    = find_artifact_spell( "Icy Caress"                    );
  artifact.icy_hand                      = find_artifact_spell( "Icy Hand"                      );
  artifact.its_cold_outside              = find_artifact_spell( "It's Cold Outside"             );
  artifact.jouster                       = find_artifact_spell( "Jouster"                       );
  artifact.let_it_go                     = find_artifact_spell( "Let It Go"                     );
  artifact.obsidian_lance                = find_artifact_spell( "Obsidian Lance"                );
  artifact.orbital_strike                = find_artifact_spell( "Orbital Strike"                );
  artifact.shield_of_alodi               = find_artifact_spell( "Shield of Alodi"               );
  artifact.shattering_bolts              = find_artifact_spell( "Shattering Bolts"              );
  artifact.spellborne                    = find_artifact_spell( "Spellborne"                    );
  artifact.the_storm_rages               = find_artifact_spell( "The Storm Rages"               );

  // Spec Spells
  spec.arcane_barrage_2      = find_specialization_spell( 231564 );
  spec.arcane_charge         = find_spell( 36032 );
  spec.arcane_mage           = find_specialization_spell( 137021 );
  spec.evocation_2           = find_specialization_spell( 231565 );

  spec.critical_mass         = find_specialization_spell( "Critical Mass"    );
  spec.critical_mass_2       = find_specialization_spell( 231630 );
  spec.fire_blast_2          = find_specialization_spell( 231568 );
  spec.fire_blast_3          = find_specialization_spell( 231567 );
  spec.fire_mage             = find_specialization_spell( 137019 );

  spec.brain_freeze          = find_specialization_spell( "Brain Freeze"     );
  spec.brain_freeze_2        = find_specialization_spell( 231584 );
  spec.blizzard_2            = find_specialization_spell( 236662 );
  spec.fingers_of_frost      = find_specialization_spell( "Fingers of Frost" );
  spec.frost_mage            = find_specialization_spell( 137020 );
  spec.shatter               = find_specialization_spell( "Shatter"          );
  spec.shatter_2             = find_specialization_spell( 231582 );


  // Mastery
  spec.savant                = find_mastery_spell( MAGE_ARCANE );
  spec.ignite                = find_mastery_spell( MAGE_FIRE );
  spec.icicles               = find_mastery_spell( MAGE_FROST );
  spec.icicles_driver        = find_spell( 148012 );
}

// mage_t::init_base ========================================================

void mage_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 30;

  player_t::init_base_stats();

  base.spell_power_per_intellect = 1.0;

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility = 0.0;

  base.mana_regen_per_second = resources.base[ RESOURCE_MANA ] * 0.015;
}

// mage_t::create_buffs =======================================================

void mage_t::create_buffs()
{
  player_t::create_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  // Arcane
  buffs.arcane_charge         = buff_creator_t( this, "arcane_charge", spec.arcane_charge );
  buffs.arcane_familiar       = buff_creator_t( this, "arcane_familiar", find_spell( 210126 ) )
                                  .default_value( find_spell( 210126 ) -> effectN( 1 ).percent() )
                                  .period( timespan_t::from_seconds( 3.0 ) )
                                  .tick_behavior( BUFF_TICK_CLIP )
                                  .tick_time_behavior( BUFF_TICK_TIME_HASTED )
                                  .tick_callback( [ this ] ( buff_t*, int, const timespan_t& )
                                    {
                                      assert( action.arcane_assault );
                                      action.arcane_assault -> set_target( target );
                                      action.arcane_assault -> execute();
                                    } )
                                  .stack_change_callback( [ this ] ( buff_t*, int, int )
                                    { recalculate_resource_max( RESOURCE_MANA ); } );
  buffs.arcane_missiles       = new buffs::arcane_missiles_t( this );
  buffs.arcane_power          = buff_creator_t( this, "arcane_power", find_spell( 12042 ) )
                                  .default_value( find_spell( 12042 ) -> effectN( 1 ).percent()
                                                + talents.overpowered -> effectN( 1 ).percent() )
                                  .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.arcane_power -> buff_duration += artifact.aegwynns_imperative.time_value();

  buffs.chrono_shift          = buff_creator_t( this, "chrono_shift", find_spell( 236298 ) )
                                  .default_value( find_spell( 236298 ) -> effectN( 1 ).percent() )
                                  .add_invalidate( CACHE_RUN_SPEED );
  buffs.crackling_energy      = buff_creator_t( this, "crackling_energy", find_spell( 246224 ) )
                                  .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                                  .default_value( find_spell( 246224 ) -> effectN( 1 ).percent() );
  buffs.presence_of_mind      = buff_creator_t( this, "presence_of_mind", find_spell( 205025 ) )
                                  .cd( timespan_t::zero() )
                                  .stack_change_callback( [ this ] ( buff_t*, int, int cur )
                                    { if ( cur == 0 ) cooldowns.presence_of_mind -> start(); } );

  // Fire
  buffs.combustion             = buff_creator_t( this, "combustion", find_spell( 190319 ) )
                                   .cd( timespan_t::zero() )
                                   .add_invalidate( CACHE_MASTERY )
                                   .add_invalidate( CACHE_CRIT_CHANCE )
                                   .default_value( find_spell( 190319 ) -> effectN( 1 ).percent() );
  buffs.combustion -> buff_duration += artifact.preignited.time_value();

  buffs.critical_massive       = buff_creator_t( this, "critical_massive", find_spell( 242251 ) )
                                   .default_value( find_spell( 242251 ) -> effectN( 1 ).percent() );
  buffs.enhanced_pyrotechnics  = buff_creator_t( this, "enhanced_pyrotechnics", find_spell( 157644 ) )
                                   .default_value( find_spell( 157644 ) -> effectN( 1 ).percent()
                                       + sets -> set( MAGE_FIRE, T19, B2 ) -> effectN( 1 ).percent() );
  buffs.erupting_infernal_core = buff_creator_t( this, "erupting_infernal_core", find_spell( 248147 ) );
  buffs.frenetic_speed         = buff_creator_t( this, "frenetic_speed", find_spell( 236060 ) )
                                   .default_value( find_spell( 236060 ) -> effectN( 1 ).percent() )
                                   .add_invalidate( CACHE_RUN_SPEED );
  buffs.ignition               = buff_creator_t( this, "ignition", find_spell( 246261 ) )
                                   .trigger_spell( sets -> set( MAGE_FIRE, T20, B2 ) );
  buffs.heating_up             = buff_creator_t( this, "heating_up",  find_spell( 48107 ) );
  buffs.hot_streak             = buff_creator_t( this, "hot_streak",  find_spell( 48108 ) );
  buffs.pyretic_incantation    = buff_creator_t( this, "pyretic_incantation", find_spell( 194329 ) )
                                   .default_value( find_spell( 194329 ) -> effectN( 1 ).percent() );
  buffs.streaking              = haste_buff_creator_t( this, "streaking", find_spell( 211399 ) )
                                   .default_value( find_spell( 211399 ) -> effectN( 1 ).percent() );
  buffs.scorched_earth         = buff_creator_t( this, "scorched_earth", find_spell( 227482 ) )
                                   .default_value( find_spell( 227482 ) -> effectN( 1 ).percent() )
                                   .add_invalidate( CACHE_RUN_SPEED );

  // Frost
  buffs.brain_freeze           = buff_creator_t( this, "brain_freeze", find_spell( 190446 ) );
  buffs.bone_chilling          = buff_creator_t( this, "bone_chilling", find_spell( 205766 ) )
                                   .default_value( talents.bone_chilling -> effectN( 1 ).percent() / 10 )
                                   .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.fingers_of_frost       = buff_creator_t( this, "fingers_of_frost", find_spell( 44544 ) )
                                   .max_stack( find_spell( 44544 ) -> max_stacks() +
                                               artifact.icy_hand.rank() );
  buffs.frozen_mass            = buff_creator_t( this, "frozen_mass", find_spell( 242253 ) )
                                   .default_value( find_spell( 242253 ) -> effectN( 1 ).percent() );
  buffs.rage_of_the_frost_wyrm = buff_creator_t( this, "rage_of_the_frost_wyrm", find_spell( 248177 ) );

  // Buff to track icicles. This does not, however, track the true amount of icicles present.
  // Instead, as it does in game, it tracks icicle buff stack count based on the number of *casts*
  // of icicle generating spells. icicles are generated on impact, so they are slightly de-synced.
  buffs.icicles                = buff_creator_t( this, "icicles", find_spell( 205473 ) );
  buffs.icy_veins              = haste_buff_creator_t( this, "icy_veins", find_spell( 12472 ) )
                                   .default_value( find_spell( 12472 ) -> effectN( 1 ).percent() )
                                   .cd( timespan_t::zero() )
                                   .stack_change_callback( [ this ] ( buff_t*, int, int cur )
                                     { if ( cur == 0 ) buffs.lady_vashjs_grasp -> expire(); } );
  buffs.icy_veins -> buff_duration += talents.thermal_void -> effectN( 2 ).time_value();

  buffs.ray_of_frost           = new buffs::ray_of_frost_buff_t( this );

  // Talents
  buffs.ice_floes              = buff_creator_t( this, "ice_floes", talents.ice_floes );
  buffs.incanters_flow         = new buffs::incanters_flow_t( this );
  buffs.rune_of_power          = buff_creator_t( this, "rune_of_power", find_spell( 116014 ) )
                                   .duration( find_spell( 116011 ) -> duration() )
                                   .default_value( find_spell( 116014 ) -> effectN( 1 ).percent() )
                                   .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Artifact
  buffs.chain_reaction        = buff_creator_t( this, "chain_reaction", find_spell( 195418 ) )
                                  .default_value( find_spell( 195418 ) -> effectN( 1 ).percent() );
  buffs.chilled_to_the_core   = buff_creator_t( this, "chilled_to_the_core", find_spell( 195446 ) )
                                  .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                                  .default_value( find_spell( 195446 ) -> effectN( 1 ).percent() );
  buffs.freezing_rain         = buff_creator_t( this, "freezing_rain", find_spell( 240555 ) );
  buffs.time_and_space        = buff_creator_t( this, "time_and_space", find_spell( 240692 ) );
  buffs.warmth_of_the_phoenix = stat_buff_creator_t( this, "warmth_of_the_phoenix", find_spell( 240671 ) )
                                  .add_stat( STAT_CRIT_RATING, find_spell( 240671 ) -> effectN( 1 ).base_value() )
                                  .chance( artifact.warmth_of_the_phoenix.data().proc_chance() );

  // Legendary
  buffs.lady_vashjs_grasp  = new buffs::lady_vashjs_grasp_t( this );

  //Misc

  // N active GBoWs are modeled by a single buff that gives N times as much mana.
  buffs.greater_blessing_of_widsom = make_buff( this, "greater_blessing_of_wisdom", find_spell( 203539 ) )
    -> set_tick_callback( [ this ]( buff_t*, int, const timespan_t& )
      { resource_gain( RESOURCE_MANA,
                       resources.max[ RESOURCE_MANA ] * 0.002 * blessing_of_wisdom_count,
                       gains.greater_blessing_of_wisdom ); } )
    -> set_period( find_spell( 203539 ) -> effectN( 2 ).period() )
    -> set_tick_behavior( BUFF_TICK_CLIP );
  buffs.t19_oh_buff = stat_buff_creator_t( this, "ancient_knowledge", find_spell( 221648 ) )
                        .trigger_spell( sets -> set( specialization(), T19OH, B8 ) );
}

// mage_t::init_gains =======================================================

void mage_t::init_gains()
{
  player_t::init_gains();

  gains.evocation                      = get_gain( "Evocation"                      );
  gains.mystic_kilt_of_the_rune_master = get_gain( "Mystic Kilt of the Rune Master" );
  gains.greater_blessing_of_wisdom     = get_gain( "Greater Blessing of Wisdom"     );
  gains.aluneths_avarice               = get_gain( "Aluneth's Avarice"              );
}

// mage_t::init_procs =======================================================

void mage_t::init_procs()
{
  player_t::init_procs();

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      break;
    case MAGE_FROST:
      break;
    case MAGE_FIRE:
      procs.heating_up_generated    = get_proc( "Heating Up generated" );
      procs.heating_up_removed      = get_proc( "Heating Up removed" );
      procs.heating_up_ib_converted = get_proc( "IB conversions of HU" );
      procs.hot_streak              = get_proc( "Total Hot Streak procs" );
      procs.hot_streak_spell        = get_proc( "Hot Streak spells used" );
      procs.hot_streak_spell_crit   = get_proc( "Hot Streak spell crits" );
      procs.hot_streak_spell_crit_wasted =
        get_proc( "Wasted Hot Streak spell crits" );

      procs.ignite_applied    = get_proc( "Direct Ignite applications" );
      procs.ignite_spread     = get_proc( "Ignites spread" );
      procs.ignite_new_spread = get_proc( "Ignites spread to new targets" );
      procs.ignite_overwrite  =
        get_proc( "Ignites spread to target with existing ignite" );
      procs.controlled_burn   = get_proc(" Controlled Burn HU -> HS Conversion ");
      break;
    default:
      // This shouldn't happen
      break;
  }
}

// mage_t::init_resources =====================================================
void mage_t::init_resources( bool force )
{
  player_t::init_resources( force );

  // This is the call needed to set max mana at the beginning of the sim.
  // If this is called without recalculating max mana afterwards, it will
  // overwrite the recalculating done earlier in reset() and cache_invalidate()
  // back to default max mana.
  if ( spec.savant -> ok() )
  {
    recalculate_resource_max( RESOURCE_MANA );
  }
}
// mage_t::init_uptimes =====================================================

void mage_t::init_benefits()
{
  player_t::init_benefits();

  if ( talents.incanters_flow -> ok() )
  {
    benefits.incanters_flow =
      new buff_stack_benefit_t( buffs.incanters_flow, "Incanter's Flow" );
  }

  if ( specialization() == MAGE_ARCANE )
  {
    benefits.arcane_charge.arcane_barrage =
      new buff_stack_benefit_t( buffs.arcane_charge, "Arcane Barrage" );
    benefits.arcane_charge.arcane_blast =
      new buff_stack_benefit_t( buffs.arcane_charge, "Arcane Blast" );
    benefits.arcane_charge.arcane_explosion =
      new buff_stack_benefit_t( buffs.arcane_charge, "Arcane Explosion" );
    benefits.arcane_charge.arcane_missiles =
      new buff_stack_benefit_t( buffs.arcane_charge, "Arcane Missiles" );
    if ( talents.nether_tempest -> ok() )
    {
      benefits.arcane_charge.nether_tempest =
        new buff_stack_benefit_t( buffs.arcane_charge, "Nether Tempest" );
    }

    benefits.arcane_missiles =
      new buff_source_benefit_t( buffs.arcane_missiles );
  }

  if ( specialization() == MAGE_FROST )
  {
    benefits.fingers_of_frost =
      new buff_source_benefit_t( buffs.fingers_of_frost );

    if ( talents.ray_of_frost -> ok() )
    {
      benefits.ray_of_frost =
        new buff_stack_benefit_t( buffs.ray_of_frost, "Ray of Frost" );
    }
  }
}

// mage_t::init_assessors =====================================================

void mage_t::init_assessors()
{
  player_t::init_assessors();

  if ( artifact.touch_of_the_magi.rank() )
  {
    auto assessor_fn = [ this ] ( dmg_e, action_state_t* state ) {
      buffs::touch_of_the_magi_t* buff =
        get_target_data( state -> target ) -> debuffs.touch_of_the_magi;

      if ( buff -> check() )
      {
        buff -> accumulate_damage( state );
      }

      return assessor::CONTINUE;
    };

    assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );

    for ( auto pet : pet_list )
    {
      pet -> assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );
    }
  }
}


// mage_t::init_actions =====================================================

void mage_t::init_action_list()
{
  if ( ! action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  apl_precombat();

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      apl_arcane();
      break;
    case MAGE_FROST:
      apl_frost();
      break;
    case MAGE_FIRE:
      apl_fire();
      break;
    default:
      apl_default(); // DEFAULT
      break;
  }

  // Default
  use_default_action_list = true;

  player_t::init_action_list();
}

// This method only handles 1 item per call in order to allow the user to add special conditons and placements
// to certain items.
std::string mage_t::get_special_use_items( const std::string& item_name, bool specials )
{
  std::string actions;
  std::string conditions;

  // If we're dealing with a special item, find its special conditional for the right spec.
  if ( specials )
  {
    if ( specialization() == MAGE_FIRE )
    {
      if ( item_name == "obelisk_of_the_void" )
      {
        conditions = "if=cooldown.combustion.remains>50";
      }
      if ( item_name == "horn_of_valor" )
      {
        conditions = "if=cooldown.combustion.remains>30";
      }
    }
  }

  for ( const auto& item : mage_t::player_t::items )
  {
    // This will skip Addon and Enchant-based on-use effects. Addons especially are important to
    // skip from the default APLs since they will interfere with the potion timer, which is almost
    // always preferred over an Addon.

    // Special or not, we need the name and slot
    if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) && item_name == item.name_str)
    {
      std::string action_string = "use_item,name=";
      action_string += item.name_str;

      // If special, we care about special conditions and placement. Else, we only care about placement in the APL.
      if ( specials )
      {
        action_string += ",";
        action_string += conditions;
      }
      actions = action_string;
    }
  }

  return actions;
}

// Pre-combat Action Priority List============================================

void mage_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  precombat -> add_action( "flask" );
  precombat -> add_action( "food" );
  precombat -> add_action( "augmentation" );

  // Water Elemental
  if ( specialization() == MAGE_FROST )
    precombat -> add_action( "water_elemental" );
  if ( specialization() == MAGE_ARCANE )
    precombat -> add_action( "summon_arcane_familiar" ) ;
  // Snapshot Stats
  precombat -> add_action( "snapshot_stats" );

  // Level 90 talents
  precombat -> add_talent( this, "Mirror Image" );

  precombat -> add_action( "potion" );

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      precombat -> add_action( this, "Mark of Aluneth", "if=set_bonus.tier20_2pc|talent.charged_up.enabled" );
      precombat -> add_action( this, "Arcane Blast", "if=!(set_bonus.tier20_2pc|talent.charged_up.enabled)" );
      break;
    case MAGE_FIRE:
      precombat -> add_action( this, "Pyroblast" );
      break;
    case MAGE_FROST:
      precombat -> add_action( this, "Frostbolt" );
      break;
    default:
      break;
  }
}

std::string mage_t::default_potion() const
{
  std::string lvl110_potion =
    ( specialization() == MAGE_ARCANE ) ? "deadly_grace" :
    ( specialization() == MAGE_FIRE )   ? "deadly_grace" :
                                          "prolonged_power";

  return ( true_level >= 100 ) ? lvl110_potion :
         ( true_level >=  90 ) ? "draenic_intellect" :
         ( true_level >=  85 ) ? "jade_serpent" :
         ( true_level >=  80 ) ? "volcanic" :
                                 "disabled";
}

std::string mage_t::default_flask() const
{
  return ( true_level >= 100 ) ? "whispered_pact" :
         ( true_level >=  90 ) ? "greater_draenic_intellect_flask" :
         ( true_level >=  85 ) ? "warm_sun" :
         ( true_level >=  80 ) ? "draconic_mind" :
                                 "disabled";
}

std::string mage_t::default_food() const
{
  std::string lvl100_food =
    ( specialization() == MAGE_ARCANE ) ? "sleeper_sushi" :
    ( specialization() == MAGE_FIRE )   ? "pickled_eel" :
                                          "salty_squid_roll";

  return ( true_level > 100 ) ? "lemon_herb_filet" :
         ( true_level >  90 ) ? lvl100_food :
         ( true_level >= 90 ) ? "mogu_fish_stew" :
         ( true_level >= 80 ) ? "seafood_magnifique_feast" :
                                "disabled";
}

std::string mage_t::default_rune() const
{
  return ( true_level >= 110 ) ? "defiled" :
         ( true_level >= 100 ) ? "focus" :
                                 "disabled";
}

// Arcane Mage Action List====================================================

void mage_t::apl_arcane()
{
  std::vector<std::string> racial_actions     = get_racial_actions();

  action_priority_list_t* default_list        = get_action_priority_list( "default"          );
  action_priority_list_t* variables           = get_action_priority_list( "variables"        );
  action_priority_list_t* build               = get_action_priority_list( "build"            );
  action_priority_list_t* conserve            = get_action_priority_list( "conserve"         );
  action_priority_list_t* miniburn_init       = get_action_priority_list( "miniburn_init"    );
  action_priority_list_t* burn                = get_action_priority_list( "burn"             );

  default_list -> add_action( this, "Counterspell", "if=target.debuff.casting.react" );
  default_list -> add_action( this, "Time Warp", "if=buff.bloodlust.down&(time=0|(buff.arcane_power.up&(buff.potion.up|!action.potion.usable))|target.time_to_die<=buff.bloodlust.duration)" );
  default_list -> add_action( "call_action_list,name=variables" );
  default_list -> add_action( "cancel_buff,name=presence_of_mind,if=active_enemies>1&set_bonus.tier20_2pc" );
  default_list -> add_action( mage_t::get_special_use_items( "horn_of_valor" ) );
  default_list -> add_action( mage_t::get_special_use_items( "obelisk_of_the_void" ) );
  default_list -> add_action( mage_t::get_special_use_items( "mrrgrias_favor" ) );
  default_list -> add_action( mage_t::get_special_use_items( "pharameres_forbidden_grimoire" ) );
  default_list -> add_action( mage_t::get_special_use_items( "kiljaedens_burning_wish" ) );
  default_list -> add_action( "call_action_list,name=build,if=buff.arcane_charge.stack<buff.arcane_charge.max_stack&!burn_phase&time>0" );
  default_list -> add_action( "call_action_list,name=burn,if=variable.time_until_burn=0|burn_phase" );
  default_list -> add_action( "call_action_list,name=conserve" );

  variables    -> add_action( "variable,name=arcane_missiles_procs,op=set,value=buff.arcane_missiles.react" );
  variables    -> add_action( "variable,name=time_until_burn,op=set,value=cooldown.arcane_power.remains" );
  variables    -> add_action( "variable,name=time_until_burn,op=max,value=cooldown.evocation.remains-variable.average_burn_length" );
  variables    -> add_action( "variable,name=time_until_burn,op=max,value=cooldown.presence_of_mind.remains,if=set_bonus.tier20_2pc" );
  variables    -> add_action( "variable,name=time_until_burn,op=max,value=action.rune_of_power.usable_in,if=talent.rune_of_power.enabled" );
  variables    -> add_action( "variable,name=time_until_burn,op=reset,if=target.time_to_die<variable.average_burn_length" );

  build -> add_talent( this, "Arcane Orb" );
  build -> add_talent( this, "Charged Up", "if=equipped.mystic_kilt_of_the_rune_master|(variable.arcane_missiles_procs=buff.arcane_missiles.max_stack&active_enemies<3)" );
  build -> add_action( this, "Arcane Missiles", "if=variable.arcane_missiles_procs=buff.arcane_missiles.max_stack&active_enemies<3" );
  build -> add_action( this, "Arcane Explosion", "if=active_enemies>1" );
  build -> add_action( this, "Arcane Blast" );

  burn  -> add_action( "variable,name=total_burns,op=add,value=1,if=!burn_phase" );
  burn  -> add_action( "start_burn_phase,if=!burn_phase" );
  burn  -> add_action( "stop_burn_phase,if=prev_gcd.1.evocation&cooldown.evocation.charges=0&burn_phase_duration>0" );
  burn  -> add_talent( this, "Nether Tempest", "if=refreshable|!ticking" );
  burn  -> add_action( this, "Mark of Aluneth" );
  burn  -> add_talent( this, "Mirror Image" );
  burn  -> add_talent( this, "Arcane Barrage", "if=set_bonus.tier20_2pc&cooldown.presence_of_mind.up&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  burn  -> add_talent( this, "Rune of Power", "if=mana.pct>30|(buff.arcane_power.up|cooldown.arcane_power.up)" );
  burn  -> add_action( this, "Arcane Power" );

  for( size_t i = 0; i < racial_actions.size(); i++ )
  {
    burn -> add_action( racial_actions[i] );
  }

  burn  -> add_action( "potion,if=buff.arcane_power.up&(buff.berserking.up|buff.blood_fury.up|!(race.troll|race.orc))" );
  burn  -> add_action( "use_items,if=buff.arcane_power.up|target.time_to_die<cooldown.arcane_power.remains" );
  burn  -> add_action( this, "Presence of Mind", "if=((mana.pct>30|buff.arcane_power.up)&set_bonus.tier20_2pc)|buff.rune_of_power.remains<=buff.presence_of_mind.max_stack*action.arcane_blast.execute_time|buff.arcane_power.remains<=buff.presence_of_mind.max_stack*action.arcane_blast.execute_time" );
  burn  -> add_talent( this, "Arcane Orb" );
  burn  -> add_action( this, "Arcane Barrage", "if=active_enemies>1&equipped.mantle_of_the_first_kirin_tor&buff.arcane_charge.stack=buff.arcane_charge.max_stack" );
  burn  -> add_action( this, "Arcane Missiles", "if=variable.arcane_missiles_procs=buff.arcane_missiles.max_stack&active_enemies<3" );
  burn  -> add_action( this, "Arcane Blast", "if=buff.presence_of_mind.up" );
  burn  -> add_talent( this, "Supernova" );
  burn  -> add_action( this, "Arcane Explosion", "if=active_enemies>1" );
  burn  -> add_action( this, "Arcane Missiles", "if=variable.arcane_missiles_procs" );
  burn  -> add_action( this, "Arcane Blast" );
  burn  -> add_action( "variable,name=average_burn_length,op=set,value=(variable.average_burn_length*variable.total_burns-variable.average_burn_length+burn_phase_duration)%variable.total_burns" );
  burn  -> add_action( this, "Evocation", "interrupt_if=ticks=2|mana.pct>=85,interrupt_immediate=1" );

  conserve -> add_talent( this, "Mirror Image", "if=variable.time_until_burn>recharge_time|variable.time_until_burn>target.time_to_die" );
  conserve -> add_action( this, "Mark of Aluneth" );
  conserve -> add_talent( this, "Rune of Power", "if=full_recharge_time<=execute_time|(prev_gcd.1.mark_of_aluneth&!set_bonus.tier20_4pc)" );
  conserve -> add_action( "swap_action_list,name=miniburn_init,if=set_bonus.tier20_4pc&cooldown.presence_of_mind.up&cooldown.arcane_power.remains>20&(action.rune_of_power.usable|!talent.rune_of_power.enabled)" );
  conserve -> add_action( this, "Arcane Missiles", "if=variable.arcane_missiles_procs=buff.arcane_missiles.max_stack&active_enemies<3" );
  conserve -> add_talent( this, "Supernova" );
  conserve -> add_talent( this, "Nether Tempest", "if=refreshable|!ticking" );
  conserve -> add_action( this, "Arcane Explosion", "if=active_enemies>1&(mana.pct>=70-(10*equipped.mystic_kilt_of_the_rune_master))" );
  conserve -> add_action( this, "Arcane Blast", "if=mana.pct>=90|buff.rhonins_assaulting_armwraps.up|(buff.rune_of_power.remains>=cast_time&equipped.mystic_kilt_of_the_rune_master)" );
  conserve -> add_action( this, "Arcane Missiles", "if=variable.arcane_missiles_procs" );
  conserve -> add_action( this, "Arcane Barrage" );
  conserve -> add_action( this, "Arcane Explosion", "if=active_enemies>1" );
  conserve -> add_action( this, "Arcane Blast" );

  miniburn_init -> add_talent( this, "Rune of Power" );
  miniburn_init -> add_action( this, "Arcane Barrage" );
  miniburn_init -> add_action( this, "Presence of Mind" );
  miniburn_init -> add_action( "swap_action_list,name=default" );
}

// Fire Mage Action List ===================================================================================================

void mage_t::apl_fire()
{
  std::vector<std::string> racial_actions     = get_racial_actions();


  action_priority_list_t* default_list        = get_action_priority_list( "default"           );
  action_priority_list_t* combustion_phase    = get_action_priority_list( "combustion_phase"  );
  action_priority_list_t* rop_phase           = get_action_priority_list( "rop_phase"         );
  action_priority_list_t* active_talents      = get_action_priority_list( "active_talents"    );
  action_priority_list_t* standard            = get_action_priority_list( "standard_rotation" );

  default_list -> add_action( this, "Counterspell", "if=target.debuff.casting.react" );
  default_list -> add_action( this, "Time Warp", "if=(time=0&buff.bloodlust.down)|(buff.bloodlust.down&equipped.132410&(cooldown.combustion.remains<1|target.time_to_die.remains<50))" );
  default_list -> add_talent( this, "Mirror Image", "if=buff.combustion.down" );
  default_list -> add_talent( this, "Rune of Power", "if=firestarter.active&action.rune_of_power.charges=2|cooldown.combustion.remains>40&buff.combustion.down&!talent.kindling.enabled|target.time_to_die.remains<11|talent.kindling.enabled&(charges_fractional>1.8|time<40)&cooldown.combustion.remains>40",
    "Standard Talent RoP Logic." );
  default_list -> add_talent( this, "Rune of Power", "if=(buff.kaelthas_ultimate_ability.react&(cooldown.combustion.remains>40|action.rune_of_power.charges>1))|(buff.erupting_infernal_core.up&(cooldown.combustion.remains>40|action.rune_of_power.charges>1))",
    "RoP use while using Legendary Items." );
  default_list -> add_action( mage_t::get_special_use_items( "horn_of_valor", true ) );
  default_list -> add_action( mage_t::get_special_use_items( "obelisk_of_the_void", true ) );
  default_list -> add_action( mage_t::get_special_use_items( "mrrgrias_favor" ) );
  default_list -> add_action( mage_t::get_special_use_items( "pharameres_forbidden_grimoire" ) );
  default_list -> add_action( mage_t::get_special_use_items( "kiljaedens_burning_wish" ) );

  default_list -> add_action( "call_action_list,name=combustion_phase,if=cooldown.combustion.remains<=action.rune_of_power.cast_time+(!talent.kindling.enabled*gcd)&(!talent.firestarter.enabled|!firestarter.active|active_enemies>=4|active_enemies>=2&talent.flame_patch.enabled)|buff.combustion.up" );
  default_list -> add_action( "call_action_list,name=rop_phase,if=buff.rune_of_power.up&buff.combustion.down" );
  default_list -> add_action( "call_action_list,name=standard_rotation" );

  combustion_phase -> add_talent( this, "Rune of Power", "if=buff.combustion.down" );
  combustion_phase -> add_action( "call_action_list,name=active_talents" );
  combustion_phase -> add_action( this, "Combustion" );
  combustion_phase -> add_action( "potion" );

  for( size_t i = 0; i < racial_actions.size(); i++ )
  {
    combustion_phase -> add_action( racial_actions[i] );
  }

  combustion_phase -> add_action( "use_items" );
  combustion_phase -> add_action( mage_t::get_special_use_items( "obelisk_of_the_void" ) );
  combustion_phase -> add_action( this, "Flamestrike", "if=(talent.flame_patch.enabled&active_enemies>2|active_enemies>4)&buff.hot_streak.up" );
  combustion_phase -> add_action( this, "Pyroblast", "if=buff.kaelthas_ultimate_ability.react&buff.combustion.remains>execute_time" );
  combustion_phase -> add_action( this, "Pyroblast", "if=buff.hot_streak.up" );
  combustion_phase -> add_action( this, "Fire Blast", "if=buff.heating_up.up" );
  combustion_phase -> add_action( this, "Phoenix's Flames" );
  combustion_phase -> add_action( this, "Scorch", "if=buff.combustion.remains>cast_time&target.health.pct<=30&equipped.132454" );
  combustion_phase -> add_action( this, "Fireball", "if=buff.combustion.remains>cast_time" );
  combustion_phase -> add_action( this, "Scorch", "if=buff.combustion.remains>cast_time" );
  combustion_phase -> add_action( this, "Dragon's Breath", "if=buff.hot_streak.down&action.fire_blast.charges<1&action.phoenixs_flames.charges<1" );
  combustion_phase -> add_action( this, "Scorch", "if=target.health.pct<=30&equipped.132454");

  rop_phase        -> add_talent( this, "Rune of Power" );
  rop_phase        -> add_action( this, "Flamestrike", "if=((talent.flame_patch.enabled&active_enemies>1)|(active_enemies>3))&buff.hot_streak.up" );
  rop_phase        -> add_action( this, "Pyroblast", "if=buff.hot_streak.up" );
  rop_phase        -> add_action( "call_action_list,name=active_talents" );
  rop_phase        -> add_action( this, "Pyroblast", "if=buff.kaelthas_ultimate_ability.react&execute_time<buff.kaelthas_ultimate_ability.remains" );
  rop_phase        -> add_action( this, "Fire Blast", "if=!prev_off_gcd.fire_blast&buff.heating_up.up&firestarter.active&charges_fractional>1.7" );
  rop_phase        -> add_action( this, "Phoenix's Flames", "if=!prev_gcd.1.phoenixs_flames&charges_fractional>2.7&firestarter.active" );
  rop_phase        -> add_action( this, "Fire Blast", "if=!prev_off_gcd.fire_blast&!firestarter.active" );
  rop_phase        -> add_action( this, "Phoenix's Flames", "if=!prev_gcd.1.phoenixs_flames" );
  rop_phase        -> add_action( this, "Scorch", "if=target.health.pct<=30&equipped.132454" );
  rop_phase        -> add_action( this, "Dragon's Breath", "if=active_enemies>2" );
  rop_phase        -> add_action( this, "Flamestrike", "if=(talent.flame_patch.enabled&active_enemies>2)|active_enemies>5" );
  rop_phase        -> add_action( this, "Fireball" );

  active_talents   -> add_talent( this, "Blast Wave", "if=(buff.combustion.down)|(buff.combustion.up&action.fire_blast.charges<1&action.phoenixs_flames.charges<1)" );
  active_talents   -> add_talent( this, "Meteor", "if=cooldown.combustion.remains>40|(cooldown.combustion.remains>target.time_to_die)|buff.rune_of_power.up|firestarter.active" );
  active_talents   -> add_talent( this, "Cinderstorm", "if=cooldown.combustion.remains<cast_time&(buff.rune_of_power.up|!talent.rune_on_power.enabled)|cooldown.combustion.remains>10*spell_haste&!buff.combustion.up" );
  active_talents   -> add_action( this, "Dragon's Breath", "if=equipped.132863|(talent.alexstraszas_fury.enabled&buff.hot_streak.down)" );
  active_talents   -> add_talent( this, "Living Bomb", "if=active_enemies>1&buff.combustion.down" );

  standard    -> add_action( this, "Flamestrike", "if=((talent.flame_patch.enabled&active_enemies>1)|active_enemies>3)&buff.hot_streak.up" );
  standard    -> add_action( this, "Pyroblast", "if=buff.hot_streak.up&buff.hot_streak.remains<action.fireball.execute_time" );
  standard    -> add_action( this, "Pyroblast", "if=buff.hot_streak.up&firestarter.active&!talent.rune_of_power.enabled" );
  standard    -> add_action( this, "Phoenix's Flames", "if=charges_fractional>2.7&active_enemies>2" );
  standard    -> add_action( this, "Pyroblast", "if=buff.hot_streak.up&!prev_gcd.1.pyroblast" );
  standard    -> add_action( this, "Pyroblast", "if=buff.hot_streak.react&target.health.pct<=30&equipped.132454" );
  standard    -> add_action( this, "Pyroblast", "if=buff.kaelthas_ultimate_ability.react&execute_time<buff.kaelthas_ultimate_ability.remains" );
  standard    -> add_action( "call_action_list,name=active_talents" );
  standard    -> add_action( this, "Fire Blast", "if=!talent.kindling.enabled&buff.heating_up.up&(!talent.rune_of_power.enabled|charges_fractional>1.4|cooldown.combustion.remains<40)&(3-charges_fractional)*(12*spell_haste)<cooldown.combustion.remains+3|target.time_to_die.remains<4" );
  standard    -> add_action( this, "Fire Blast", "if=talent.kindling.enabled&buff.heating_up.up&(!talent.rune_of_power.enabled|charges_fractional>1.5|cooldown.combustion.remains<40)&(3-charges_fractional)*(18*spell_haste)<cooldown.combustion.remains+3|target.time_to_die.remains<4" );
  standard    -> add_action( this, "Phoenix's Flames", "if=(buff.combustion.up|buff.rune_of_power.up|buff.incanters_flow.stack>3|talent.mirror_image.enabled)&artifact.phoenix_reborn.enabled&(4-charges_fractional)*13<cooldown.combustion.remains+5|target.time_to_die.remains<10" );
  standard    -> add_action( this, "Phoenix's Flames", "if=(buff.combustion.up|buff.rune_of_power.up)&(4-charges_fractional)*30<cooldown.combustion.remains+5" );
  standard    -> add_action( this, "Phoenix's Flames", "if=charges_fractional>2.5&cooldown.combustion.remains>23" );
  standard    -> add_action( this, "Flamestrike", "if=(talent.flame_patch.enabled&active_enemies>1)|active_enemies>5" );
  standard    -> add_action( this, "Scorch", "if=target.health.pct<=30&equipped.132454" );
  standard    -> add_action( this, "Fireball" );


}

// Frost Mage Action List ==============================================================================================================

void mage_t::apl_frost()
{
  std::vector<std::string> racial_actions = get_racial_actions();

  action_priority_list_t* default_list = get_action_priority_list( "default"           );
  action_priority_list_t* single       = get_action_priority_list( "single"            );
  action_priority_list_t* aoe          = get_action_priority_list( "aoe"               );
  action_priority_list_t* cooldowns    = get_action_priority_list( "cooldowns"         );
  action_priority_list_t* movement     = get_action_priority_list( "movement"          );
  action_priority_list_t* variables    = get_action_priority_list( "variables"         );

  default_list -> add_action( "call_action_list,name=variables" );
  default_list -> add_action( this, "Counterspell", "if=target.debuff.casting.react" );
  default_list -> add_action( this, "Ice Lance", "if=variable.fof_react=0&prev_gcd.1.flurry",
    "Free Ice Lance after Flurry. This action has rather high priority to ensure that we don't cast Rune of Power, Ray of Frost, "
    "etc. after Flurry and break up the combo. If FoF was already active, we do not lose anything by delaying the Ice Lance." );
  default_list -> add_action( this, "Time Warp",
    "if=buff.bloodlust.down&(buff.exhaustion.down|equipped.shard_of_the_exodar)&(time=0|cooldown.icy_veins.remains<1|target.time_to_die<50)",

    "Time Warp is used right at the start. If the actor has Shard of the Exodar, try to synchronize the second Time Warp with "
    "Icy Veins. If the target is about to die, use Time Warp regardless." );
  default_list -> add_action( mage_t::get_special_use_items( "horn_of_valor" ) );
  default_list -> add_action( mage_t::get_special_use_items( "obelisk_of_the_void" ) );
  default_list -> add_action( mage_t::get_special_use_items( "mrrgrias_favor" ) );
  default_list -> add_action( mage_t::get_special_use_items( "pharameres_forbidden_grimoire" ) );
  default_list -> add_action( mage_t::get_special_use_items( "kiljaedens_burning_wish" ) );
  default_list -> add_action( "call_action_list,name=movement" );
  default_list -> add_action( "call_action_list,name=cooldowns" );
  default_list -> add_action( "call_action_list,name=aoe,if=active_enemies>=4" );
  default_list -> add_action( "call_action_list,name=single" );

  single -> add_talent( this, "Ice Nova", "if=debuff.winters_chill.up",
    "In some circumstances, it is possible for both Ice Lance and Ice Nova to benefit from a single Winter's Chill." );
  single -> add_action( this, "Frozen Orb", "if=set_bonus.tier20_2pc",
    "With T20 2pc, Frozen Orb should be used as soon as it comes off CD." );
  single -> add_action( this, "Frostbolt", "if=prev_off_gcd.water_jet" );
  single -> add_action( "water_jet,if=prev_gcd.1.frostbolt&buff.fingers_of_frost.stack<(2+artifact.icy_hand.enabled)&buff.brain_freeze.react=0",
    "Basic Water Jet combo. Since Water Jet can only be used if the actor is not casting, we use it right after Frostbolt is executed. "
    "At the default distance, Frostbolt travels slightly over 1 s, giving Water Jet enough time to apply the DoT (Water Jet's cast time "
    "is 1 s, with haste scaling). The APL then forces another Frostbolt to guarantee getting both FoFs from the Water Jet. This works for "
    "most haste values (roughly from 0% to 160%). When changing the default distance, great care must be taken otherwise this action "
    "won't produce two FoFs." );
  single -> add_talent( this, "Ray of Frost", "if=buff.icy_veins.up|(cooldown.icy_veins.remains>action.ray_of_frost.cooldown&buff.rune_of_power.down)" );
  single -> add_action( this, "Flurry",
    "if=prev_gcd.1.ebonbolt|buff.brain_freeze.react&(!talent.glacial_spike.enabled&prev_gcd.1.frostbolt|talent.glacial_spike.enabled&"
    "(prev_gcd.1.glacial_spike|prev_gcd.1.frostbolt&(buff.icicles.stack<=3|cooldown.frozen_orb.remains<=10&set_bonus.tier20_2pc)))",

    "Winter's Chill from Flurry can apply to the spell cast right before (provided the travel time is long enough). This can be "
    "exploited to a great effect with Ebonbolt, Glacial Spike (which deal a lot of damage by themselves) and Frostbolt (as a "
    "guaranteed way to proc Frozen Veins and Chain Reaction). When using Glacial Spike, it is worth saving a Brain Freeze proc "
    "when Glacial Spike is right around the corner (i.e. with 4 or more Icicles). However, when the actor also has T20 2pc, "
    "Glacial Spike is delayed to fit into Frozen Mass, so we do not want to sit on a Brain Freeze proc for too long in that case." );
  single -> add_action( this, "Blizzard", "if=cast_time=0&active_enemies>1&variable.fof_react<3",
    "Freezing Rain Blizzard. While the normal Blizzard action is usually enough, right after Frozen Orb the actor will be "
    "getting a lot of FoFs, which might delay Blizzard to the point where we miss out on Freezing Rain. Therefore, if we are "
    "not at a risk of overcapping on FoF, use Blizzard before using Ice Lance." );
  single -> add_talent( this, "Frost Bomb", "if=debuff.frost_bomb.remains<action.ice_lance.travel_time&variable.fof_react>0" );
  single -> add_action( this, "Ice Lance", "if=variable.fof_react>0&cooldown.icy_veins.remains>10|variable.fof_react>2" );
  single -> add_action( this, "Ebonbolt" );
  single -> add_action( this, "Frozen Orb" );
  single -> add_talent( this, "Ice Nova" );
  single -> add_talent( this, "Comet Storm" );
  single -> add_action( this, "Blizzard",
    "if=active_enemies>2|active_enemies>1&!(talent.glacial_spike.enabled&talent.splitting_ice.enabled)|(buff.zannesu_journey.stack=5&"
    "buff.zannesu_journey.remains>cast_time)",

    "Against low number of targets, Blizzard is used as a filler. Use it only against 2 or more targets, 3 or more when using Glacial "
    "Spike and Splitting Ice. Zann'esu buffed Blizzard is used only at 5 stacks." );
  single -> add_action( this, "Frostbolt",
    "if=buff.frozen_mass.remains>execute_time+action.glacial_spike.execute_time+action.glacial_spike.travel_time&buff.brain_freeze.react=0&"
    "talent.glacial_spike.enabled",

    "While Frozen Mass is active, we want to fish for Brain Freeze for the next Glacial Spike. Stop when Frozen Mass is about to run out "
    "and we wouldn't be able to cast Glacial Spike in time." );
  single -> add_talent( this, "Glacial Spike", "if=cooldown.frozen_orb.remains>10|!set_bonus.tier20_2pc",
    "Glacial Spike is generally used as it is available, unless we have T20 2pc. In that case, Glacial Spike is delayed when "
    "Frozen Mass is happening soon (in less than 10 s)." );
  single -> add_action( this, "Frostbolt" );
  single -> add_action( this, "Blizzard", "if=cast_time=0",
    "While on the move, use instant Blizzard if available." );
  single -> add_action( this, "Ice Lance", "",
    "Otherwise just use Ice Lance to do at least some damage." );

  aoe -> add_action( this, "Frostbolt", "if=prev_off_gcd.water_jet" );
  aoe -> add_action( this, "Frozen Orb", "",
    "Make sure Frozen Orb is used before Blizzard if both are available. This is a small gain with Freezing Rain "
    "and on par without." );
  aoe -> add_action( this, "Blizzard" );
  aoe -> add_talent( this, "Comet Storm" );
  aoe -> add_talent( this, "Ice Nova" );
  aoe -> add_action( "water_jet,if=prev_gcd.1.frostbolt&buff.fingers_of_frost.stack<(2+artifact.icy_hand.enabled)&buff.brain_freeze.react=0" );
  aoe -> add_action( this, "Flurry", "if=prev_gcd.1.ebonbolt|(prev_gcd.1.glacial_spike|prev_gcd.1.frostbolt)&buff.brain_freeze.react" );
  aoe -> add_talent( this, "Frost Bomb", "if=debuff.frost_bomb.remains<action.ice_lance.travel_time&variable.fof_react>0" );
  aoe -> add_action( this, "Ice Lance", "if=variable.fof_react>0" );
  aoe -> add_action( this, "Ebonbolt" );
  aoe -> add_talent( this, "Glacial Spike" );
  aoe -> add_action( this, "Frostbolt" );
  aoe -> add_action( this, "Cone of Cold" );
  aoe -> add_action( this, "Ice Lance" );

  cooldowns -> add_talent( this, "Rune of Power",
    "if=cooldown.icy_veins.remains<cast_time|charges_fractional>1.9&cooldown.icy_veins.remains>10|buff.icy_veins.up|"
    "target.time_to_die.remains+5<charges_fractional*10",

    "Rune of Power is used when going into Icy Veins and while Icy Veins are up. Outside of Icy Veins, use Rune of Power "
    "when about to cap on charges or the target is about to die." );
  cooldowns -> add_action( "potion,if=cooldown.icy_veins.remains<1" );
  cooldowns -> add_action( this, "Icy Veins", "if=buff.icy_veins.down" );
  cooldowns -> add_talent( this, "Mirror Image" );
  cooldowns -> add_action( "use_items" );
  for( size_t i = 0; i < racial_actions.size(); i++ )
  {
    cooldowns -> add_action( racial_actions[i] );
  }

  movement -> add_action( this, "Blink", "if=movement.distance>10" );
  movement -> add_talent( this, "Ice Floes", "if=buff.ice_floes.down&movement.distance>0&variable.fof_react=0" );

  variables -> add_action( "variable,name=iv_start,value=time,if=prev_off_gcd.icy_veins",
    "Variable which tracks when Icy Veins were used. For use in time_until_fof variable." );
  variables -> add_action( "variable,name=time_until_fof,value=10-(time-variable.iv_start-floor((time-variable.iv_start)%10)*10)",
    "This variable tracks the remaining time until FoF proc from Lady Vashj's Grasp. Note that it doesn't check whether the actor "
    "actually has the legendary or that Icy Veins are currently active." );
  variables -> add_action( "variable,name=fof_react,value=buff.fingers_of_frost.react",
    "Replacement for buff.fingers_of_frost.react. Since some of the FoFs are not random and can be anticipated (Freeze, "
    "Lady Vashj's Grasp), we can bypass the .react check." );
  variables -> add_action( "variable,name=fof_react,value=buff.fingers_of_frost.stack,if=equipped.lady_vashjs_grasp&buff.icy_veins.up&"
    "variable.time_until_fof>9|prev_off_gcd.freeze|ground_aoe.frozen_orb.remains>9" );
}

// Default Action List ========================================================

void mage_t::apl_default()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );

  // TODO: What do mages below level 10 without specs actually use?
  default_list -> add_action( "Fireball" );
}


// mage_t::mana_regen_per_second ==============================================

double mage_t::mana_regen_per_second() const
{
  double mps = player_t::mana_regen_per_second();

  if ( spec.savant -> ok() )
  {
    mps *= 1.0 + composite_mastery() * spec.savant -> effectN( 1 ).mastery_value();
  }

  return mps;
}

// mage_t::invalidate_cache ===================================================

void mage_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_MASTERY:
      if ( spec.savant -> ok() )
      {
        recalculate_resource_max( RESOURCE_MANA );
      }
      break;
    case CACHE_SPELL_CRIT_CHANCE:
      // Combustion makes mastery dependent on spell crit chance rating. Thus
      // any spell_crit_chance invalidation (which should include any
      // spell_crit_rating changes) will also invalidate mastery.
      if ( specialization() == MAGE_FIRE )
      {
        invalidate_cache( CACHE_MASTERY );
      }
      break;
    default:
      break;
  }

}

// mage_t::recalculate_resource_max ===========================================

void mage_t::recalculate_resource_max( resource_e rt )
{
  if ( rt != RESOURCE_MANA )
  {
    return player_t::recalculate_resource_max( rt );
  }

  double current_mana = resources.current[ rt ],
         current_mana_max = resources.max[ rt ],
         mana_percent = current_mana / current_mana_max;

  player_t::recalculate_resource_max( rt );

  if ( spec.savant -> ok() )
  {
    resources.max[ rt ] *= 1.0 +
      composite_mastery() * ( spec.savant -> effectN( 1 ).mastery_value() );
    resources.current[ rt ] = resources.max[ rt ] * mana_percent;
    if ( sim -> debug )
    {
      sim -> out_debug.printf(
        "%s Savant adjusts mana from %.0f/%.0f to %.0f/%.0f",
        name(), current_mana, current_mana_max,
        resources.current[ rt ], resources.max[ rt ]);
    }

    current_mana = resources.current[ rt ];
    current_mana_max = resources.max[ rt ];
  }

  if ( talents.arcane_familiar -> ok() && buffs.arcane_familiar -> check() )
  {
    resources.max[ rt ] *= 1.0 +
      buffs.arcane_familiar -> check_value();
    resources.current[ rt ] = resources.max[ rt ] * mana_percent;
    if ( sim -> debug )
    {
      sim -> out_debug.printf(
          "%s Arcane Familiar adjusts mana from %.0f/%.0f to %.0f/%.0f",
          name(), current_mana, current_mana_max,
          resources.current[ rt ], resources.max[ rt ]);
    }
  }
}
// mage_t::composite_player_critical_damage_multiplier ===================

double mage_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_critical_damage_multiplier( s );

  if ( dbc::is_school( s -> action -> school, SCHOOL_FIRE ) )
  {
    m *= 1.0 + artifact.burning_gaze.percent();
  }

  if ( !dbc::is_school( s -> action -> get_school(), SCHOOL_PHYSICAL ) )
  {
    m *= 1.0 + buffs.pyretic_incantation -> check_stack_value();
  }

  if ( buffs.frozen_mass -> check() )
  {
    m *= 1.0 + buffs.frozen_mass -> check_value();
  }

  return m;
}

// mage_t::composite_player_pet_damage_multiplier ============================

double mage_t::composite_player_pet_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s );

  if ( buffs.rune_of_power -> check() )
  {
    m *= 1.0 + buffs.rune_of_power -> check_value();
  }

  if ( talents.incanters_flow -> ok() )
  {
    m *= 1.0 + buffs.incanters_flow -> check_stack_value();
  }

  m *= 1.0 + artifact.ancient_power.percent();
  m *= 1.0 + artifact.intensity_of_the_tirisgarde.data().effectN( 3 ).percent();

  m *= 1.0 + artifact.empowered_spellblade.percent();
  m *= 1.0 + artifact.instability_of_the_tirisgarde.data().effectN( 3 ).percent();

  m *= 1.0 + artifact.spellborne.percent();
  m *= 1.0 + artifact.frigidity_of_the_tirisgarde.data().effectN( 3 ).percent();

  return m;
}

// mage_t::composite_player_multiplier =======================================

double mage_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.rune_of_power -> check() )
  {
    m *= 1.0 + buffs.rune_of_power -> check_value();
  }

  if ( talents.incanters_flow -> ok() )
  {
    m *= 1.0 + buffs.incanters_flow -> check_stack_value();
  }

  // TODO: Check if AP interacts with multischool damage that includes physical.
  if ( buffs.arcane_power -> check() && !dbc::is_school( school, SCHOOL_PHYSICAL ) )
  {
    m *= 1.0 + buffs.arcane_power -> check_value();
  }

  if ( dbc::is_school( school, SCHOOL_ARCANE ) )
  {
    m *= 1.0 + artifact.might_of_the_guardians.percent();
  }

  if ( dbc::is_school( school, SCHOOL_ARCANE ) )
  {
    m *= 1.0 + artifact.ancient_power.percent();
  }

  m *= 1.0 + artifact.intensity_of_the_tirisgarde.data().effectN( 1 ).percent();

  if ( dbc::is_school( school, SCHOOL_FIRE ) )
  {
    m *= 1.0 + artifact.wings_of_flame.percent();
  }

  if ( dbc::is_school( school, SCHOOL_FIRE ) )
  {
    m *= 1.0 + artifact.empowered_spellblade.percent();
  }

  m *= 1.0 + artifact.instability_of_the_tirisgarde.data().effectN( 1 ).percent();

  if ( dbc::is_school( school, SCHOOL_FROST ) )
  {
    m *= 1.0 + buffs.bone_chilling -> check_stack_value();
  }

  if ( dbc::is_school( school, SCHOOL_FROST ) )
  {
    m *= 1.0 + artifact.spellborne.percent();
  }

  m *= 1.0 + artifact.frigidity_of_the_tirisgarde.data().effectN( 1 ).percent();


  if ( buffs.chilled_to_the_core -> check() && dbc::is_school( school, SCHOOL_FROST ) )
  {
    m *= 1.0 + buffs.chilled_to_the_core -> check_value();
  }

  if ( buffs.crackling_energy -> check() )
  {
    m *= 1.0 + buffs.crackling_energy -> check_value();
  }

  return m;
}

// mage_t::composite_mastery_rating =============================================

double mage_t::composite_mastery_rating() const
{
  double m = player_t::composite_mastery_rating();

  if ( buffs.combustion -> up() )
  {
    m += mage_t::composite_spell_crit_rating() * buffs.combustion -> data().effectN( 3 ).percent();
  }
 return m;
}

// mage_t::composite_spell_crit_rating ===============================================

double mage_t::composite_spell_crit_rating() const
{
  double cr = player_t::composite_spell_crit_rating();

  if ( spec.critical_mass -> ok() )
  {
    cr *= 1.0 + spec.critical_mass_2 -> effectN( 1 ).percent();
  }

  return cr;

}
// mage_t::composite_spell_crit_chance ===============================================

double mage_t::composite_spell_crit_chance() const
{
  double c = player_t::composite_spell_crit_chance();

  if ( buffs.combustion -> check() )
  {
    c += buffs.combustion -> check_value();
  }

  if ( spec.critical_mass -> ok() )
  {
    c += spec.critical_mass -> effectN( 1 ).percent();
  }

  c += artifact.aegwynns_wrath.percent();

  return c;
}

// mage_t::composite_spell_haste ==============================================

double mage_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.icy_veins -> check() )
  {
    h /= 1.0 + buffs.icy_veins -> check_value();
  }

  if ( buffs.sephuzs_secret -> default_chance != 0 )
  {
    h /= 1.0 + buffs.sephuzs_secret -> data().driver() -> effectN( 3 ).percent();
  }

  if ( buffs.sephuzs_secret -> check() )
  {
    h /= 1.0 + buffs.sephuzs_secret -> stack_value();
  }

  if ( buffs.streaking -> check() )
  {
    h /= 1.0 + buffs.streaking -> check_value();
  }

  return h;
}

double mage_t::composite_attribute_multiplier( attribute_e attribute ) const
{
  double m = player_t::composite_attribute_multiplier( attribute );
  switch ( attribute )
  {
    case ATTR_STAMINA:
      m *= 1.0 + artifact.frigidity_of_the_tirisgarde.data().effectN( 2 ).percent();
      m *= 1.0 + artifact.instability_of_the_tirisgarde.data().effectN( 2 ).percent();
      m *= 1.0 + artifact.intensity_of_the_tirisgarde.data().effectN( 2 ).percent();
      break;
    default:
      break;
  }
  return m;
}

// mage_t::matching_gear_multiplier =========================================

double mage_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return 0.05;

  return 0.0;
}

// mage_t::reset ============================================================

void mage_t::reset()
{
  player_t::reset();

  icicles.clear();
  event_t::cancel( icicle_event );
  event_t::cancel( ignite_spread_event );

  if ( spec.savant -> ok() )
  {
    recalculate_resource_max( RESOURCE_MANA );
  }

  last_bomb_target = nullptr;
  ground_aoe_expiration.clear();
  active_meteor_burn = nullptr;
  burn_phase.reset();
}

// mage_t::stun =============================================================

void mage_t::stun()
{
  // FIX ME: override this to handle Blink
  player_t::stun();
}

// mage_t::update_movement==================================================

void mage_t::update_movement( timespan_t duration )
{
  player_t::update_movement( duration );

  double yards = duration.total_seconds() * composite_movement_speed();
  distance_from_rune += yards;

  if ( buffs.rune_of_power -> check() )
  {
    if ( distance_from_rune > talents.rune_of_power -> effectN( 2 ).radius() )
    {
      buffs.rune_of_power -> expire();
      if ( sim -> debug ) sim -> out_debug.printf( "%s lost Rune of Power due to moving more than 8 yards away from it.", name() );
    }
  }
}

// mage_t::temporary_movement_modifier ==================================

double mage_t::temporary_movement_modifier() const
{
  double tmm = player_t::temporary_movement_modifier();

  if ( buffs.sephuzs_secret -> up() )
  {
    tmm = std::max( buffs.sephuzs_secret -> data().effectN( 1 ).percent(), tmm );
  }

  return tmm;
}

// mage_t::passive_movement_modifier ====================================

double mage_t::passive_movement_modifier() const
{
  double pmm = player_t::passive_movement_modifier();

  if ( buffs.sephuzs_secret -> default_chance != 0 )
  {
    pmm += buffs.sephuzs_secret -> data().driver() -> effectN( 2 ).percent();
  }

  if ( buffs.chrono_shift -> check() )
  {
    pmm += buffs.chrono_shift -> check_value();
  }

  if ( buffs.frenetic_speed -> check() )
  {
    pmm += buffs.frenetic_speed -> check_value();
  }

  if ( buffs.scorched_earth -> check() )
  {
    pmm += buffs.scorched_earth -> check_stack_value();
  }

  return pmm;
}

// mage_t::arise ============================================================

void mage_t::arise()
{
  player_t::arise();

  if ( talents.incanters_flow -> ok() )
    buffs.incanters_flow -> trigger();

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      break;
    case MAGE_FROST:
      break;
    case MAGE_FIRE:
      break;
    default:
      apl_default(); // DEFAULT
      break;
  }

  if ( blessing_of_wisdom_count > 0 )
  {
    buffs.greater_blessing_of_widsom -> trigger();
  }
  if ( spec.ignite -> ok()  )
  {
    timespan_t first_spread = timespan_t::from_seconds( rng().real() * 2.0 );
    ignite_spread_event =
        make_event<events::ignite_spread_event_t>( *sim, *this, first_spread );
  }
}

// mage_t::create_expression ================================================

expr_t* mage_t::create_expression( action_t* a, const std::string& name_str )
{
  struct mage_expr_t : public expr_t
  {
    mage_t& mage;
    mage_expr_t( const std::string& n, mage_t& m ) :
      expr_t( n ), mage( m ) { }
  };

  // Incanters flow direction
  // Evaluates to:  0.0 if IF talent not chosen or IF stack unchanged
  //                1.0 if next IF stack increases
  //               -1.0 if IF stack decreases
  if ( name_str == "incanters_flow_dir" )
  {
    struct incanters_flow_dir_expr_t : public mage_expr_t
    {
      incanters_flow_dir_expr_t( mage_t& m ) :
        mage_expr_t( "incanters_flow_dir", m )
      { }

      virtual double evaluate() override
      {
        if ( ! mage.talents.incanters_flow -> ok() )
          return 0.0;

        buff_t* flow = mage.buffs.incanters_flow;
        if ( flow -> reverse )
          return flow -> check() == 1 ? 0.0: -1.0;
        else
          return flow -> check() == 5 ? 0.0: 1.0;
      }
    };

    return new incanters_flow_dir_expr_t( *this );
  }

  // Arcane Burn Flag Expression ==============================================
  if ( name_str == "burn_phase" )
  {
    struct burn_phase_expr_t : public mage_expr_t
    {
      burn_phase_expr_t( mage_t& m ) :
        mage_expr_t( "burn_phase", m )
      { }

      virtual double evaluate() override
      {
        return mage.burn_phase.on();
      }
    };

    return new burn_phase_expr_t( *this );
  }

  if ( name_str == "burn_phase_duration" )
  {
    struct burn_phase_duration_expr_t : public mage_expr_t
    {
      burn_phase_duration_expr_t( mage_t& m ) :
        mage_expr_t( "burn_phase_duration", m )
      { }

      virtual double evaluate() override
      {
        return mage.burn_phase.duration( mage.sim -> current_time() )
                              .total_seconds();
      }
    };

    return new burn_phase_duration_expr_t( *this );
  }

  // Icicle Expressions =======================================================
  if ( util::str_compare_ci( name_str, "shooting_icicles" ) )
  {
    struct sicicles_expr_t : public mage_expr_t
    {
      sicicles_expr_t( mage_t& m ) : mage_expr_t( "shooting_icicles", m )
      { }

      double evaluate() override
      { return mage.icicle_event != nullptr; }
    };

    return new sicicles_expr_t( *this );
  }

  if ( util::str_compare_ci( name_str, "icicles" ) )
  {
    struct icicles_expr_t : public mage_expr_t
    {
      icicles_expr_t( mage_t& m ) : mage_expr_t( "icicles", m )
      { }

      double evaluate() override
      {
        if ( mage.icicles.empty() )
          return 0;
        else if ( mage.sim -> current_time() - mage.icicles[ 0 ].timestamp < mage.spec.icicles_driver -> duration() )
          return as<double>( mage.icicles.size() );
        else
        {
          size_t icicles = 0;
          for ( int i = as<int>( mage.icicles.size() - 1 ); i >= 0; i-- )
          {
            if ( mage.sim -> current_time() - mage.icicles[ i ].timestamp >= mage.spec.icicles_driver -> duration() )
              break;

            icicles++;
          }

          return as<double>( icicles );
        }
      }
    };

    return new icicles_expr_t( *this );
  }

  std::vector<std::string> splits = util::string_split( name_str, "." );

  // Firestarter expressions ==================================================
  if ( splits.size() == 2 && util::str_compare_ci( splits[0], "firestarter" ) )
  {
    enum expr_type_t
    {
      FIRESTARTER_ACTIVE,
      FIRESTARTER_REMAINS
    };

    struct firestarter_expr_t : public mage_expr_t
    {
      action_t* a;
      expr_type_t type;

      firestarter_expr_t( mage_t& m, const std::string& name, action_t* a, expr_type_t type ) :
        mage_expr_t( name, m ), a( a ), type( type )
      { }

      double evaluate() override
      {
        if ( ! mage.talents.firestarter -> ok() )
          return 0.0;

        timespan_t remains;

        if ( mage.firestarter_time > timespan_t::zero() )
        {
          remains = std::max( timespan_t::zero(), mage.firestarter_time - mage.sim -> current_time() );
        }
        else
        {
          remains = a -> target -> time_to_percent( mage.talents.firestarter -> effectN( 1 ).base_value() );
        }

        switch ( type )
        {
          case FIRESTARTER_ACTIVE:
            return static_cast<double>( remains > timespan_t::zero() );
          case FIRESTARTER_REMAINS:
            return remains.total_seconds();
          default:
            return 0.0;
        }
      }
    };

    if ( util::str_compare_ci( splits[1], "active" ) )
    {
      return new firestarter_expr_t( *this, name_str, a, FIRESTARTER_ACTIVE );
    }
    else if ( util::str_compare_ci( splits[1], "remains" ) )
    {
      return new firestarter_expr_t( *this, name_str, a, FIRESTARTER_REMAINS );
    }
    else
    {
      sim -> errorf( "Player %s firestarer expression: unknown operation '%s'", name(), splits[1].c_str() );
    }
  }

  // Ground AoE expressions ===================================================
  if ( splits.size() == 3 && util::str_compare_ci( splits[0], "ground_aoe" ) )
  {
    struct ground_aoe_expr_t : public mage_expr_t
    {
      std::string aoe_type;

      ground_aoe_expr_t( mage_t& m, const std::string& name_str, const std::string& aoe ) :
        mage_expr_t( name_str, m ),
        aoe_type( aoe )
      {
        util::tolower( aoe_type );
      }

      double evaluate() override
      {
        timespan_t expiration;

        auto it = mage.ground_aoe_expiration.find( aoe_type );
        if ( it != mage.ground_aoe_expiration.end() )
        {
          expiration = it -> second;
        }

        return std::max( 0.0, ( expiration - mage.sim -> current_time() ).total_seconds() );
      }
    };

    if ( util::str_compare_ci( splits[2], "remains" ) )
    {
      return new ground_aoe_expr_t( *this, name_str, splits[1] );
    }
    else
    {
      sim -> errorf( "Player %s ground_aoe expression: unknown operation '%s'", name(), splits[2].c_str() );
    }
  }

  return player_t::create_expression( a, name_str );
}

// mage_t::convert_hybrid_stat ==============================================

stat_e mage_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT:
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_STR_AGI:
    return STAT_NONE;
  case STAT_SPIRIT:
    return STAT_NONE;
  case STAT_BONUS_ARMOR:
    return STAT_NONE;
  default: return s;
  }
}

// mage_t::get_icicle_object ==================================================

icicle_data_t mage_t::get_icicle_object()
{
  if ( icicles.empty() )
  {
    return icicle_data_t{ 0.0, nullptr };
  }

  timespan_t threshold = spec.icicles_driver -> duration();

  // Find first icicle which did not time out
  auto idx =
      range::find_if( icicles, [ this, threshold ] ( const icicle_tuple_t& t ) {
        return sim -> current_time() - t.timestamp < threshold;
      } );

  // Remove all timed out icicles
  if ( idx != icicles.begin() )
  {
    icicles.erase( icicles.begin(), idx );
  }

  if ( !icicles.empty() )
  {
    icicle_data_t d = icicles.front().data;
    icicles.erase( icicles.begin() );
    return d;
  }

  return icicle_data_t{ 0.0, nullptr };
}

void mage_t::trigger_icicle( const action_state_t* trigger_state, bool chain, player_t* chain_target )
{
  if ( ! spec.icicles -> ok() )
    return;

  if ( icicles.empty() )
    return;

  player_t* icicle_target;
  if ( chain_target )
  {
    icicle_target = chain_target;
  }
  else
  {
    icicle_target = trigger_state -> target;
  }

  if ( chain && ! icicle_event )
  {
    icicle_data_t d = get_icicle_object();
    if ( d.damage == 0.0 )
      return;

    assert( icicle_target );
    icicle_event = make_event<events::icicle_event_t>( *sim, *this, d, icicle_target, true );

    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s icicle use on %s%s, damage=%f, total=%u",
                               name(), icicle_target -> name(),
                               chain ? " (chained)" : "", d.damage,
                               as<unsigned>( icicles.size() ) );
    }
  }
  else if ( ! chain )
  {
    icicle_data_t d = get_icicle_object();
    if ( d.damage == 0 )
      return;

    icicle -> set_target( icicle_target );
    auto new_state = debug_cast<actions::icicle_state_t*>( icicle -> get_state() );
    icicle -> snapshot_state( new_state, icicle -> amount_type( new_state ) );
    new_state -> target = icicle_target;
    new_state -> source = d.stats;

    icicle -> base_dd_adder = d.damage;

    // Immediately execute icicles so the correct damage is carried into the
    // travelling icicle object
    icicle -> pre_execute_state = new_state;
    icicle -> execute();

    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s icicle use on %s%s, damage=%f, total=%u",
                               name(), icicle_target -> name(),
                               chain ? " (chained)" : "", d.damage,
                               as<unsigned>( icicles.size() ) );
    }
  }
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class mage_report_t : public player_report_extension_t
{
public:
  mage_report_t( mage_t& player ) :
      p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void) p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
        << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
        << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  mage_t& p;
};
// Custom Gear ==============================================================
using namespace unique_gear;
using namespace actions;
// Legion Mage JC Neck

struct sorcerous_fireball_t : public spell_t
{
  sorcerous_fireball_t( const special_effect_t& effect ) :
    spell_t( "sorcerous_fireball", effect.player, effect.player -> find_spell( 222305 ) )
  {
    background = true;
    may_crit = true;
    hasted_ticks = false;
    base_dd_min = base_dd_max = data().effectN( 1 ).average( effect.item );
    base_td = data().effectN( 2 ).average( effect.item );
  }

  virtual double composite_crit_chance() const override
  { return 0.1; }

  virtual double composite_crit_chance_multiplier() const override
  { return 1.0; }
};

struct sorcerous_frostbolt_t : public spell_t
{
  sorcerous_frostbolt_t( const special_effect_t& effect ) :
    spell_t( "sorcerous_frostbolt", effect.player, effect.player -> find_spell( 222320 ) )
  {
    background = true;
    may_crit = true;
    base_dd_min = base_dd_max = data().effectN( 1 ).average( effect.item );
  }

  virtual double composite_crit_chance() const override
  { return 0.1; }

  virtual double composite_crit_chance_multiplier() const override
  { return 1.0; }
};

struct sorcerous_arcane_blast_t : public spell_t
{
  sorcerous_arcane_blast_t( const special_effect_t& effect ) :
    spell_t( "sorcerous_arcane_blast", effect.player, effect.player -> find_spell( 222321 ) )
  {
    background = true;
    may_crit = true;
    base_dd_min = base_dd_max = data().effectN( 1 ).average( effect.item );
  }

  virtual double composite_crit_chance() const override
  { return 0.1; }

  virtual double composite_crit_chance_multiplier() const override
  { return 1.0; }
};

struct sorcerous_shadowruby_pendant_driver_t : public spell_t
{
  std::array<spell_t*, 3> sorcerous_spells;
  sorcerous_shadowruby_pendant_driver_t( const special_effect_t& effect ) :
    spell_t( "wanton_sorcery", effect.player, effect.player -> find_spell( 222276 ) )
  {
    callbacks = harmful = false;
    background = quiet = true;
    sorcerous_spells[ 0 ] = new sorcerous_fireball_t( effect );
    sorcerous_spells[ 1 ] = new sorcerous_frostbolt_t( effect );
    sorcerous_spells[ 2 ] = new sorcerous_arcane_blast_t( effect );
  }
  virtual void execute() override
  {
    spell_t::execute();

    auto current_roll = static_cast<unsigned>( rng().range( 0, as<double>( sorcerous_spells.size() ) ) );
    auto spell = sorcerous_spells[ current_roll ];

    spell -> set_target( execute_state -> target );
    spell -> execute();
  }
};

static void sorcerous_shadowruby_pendant( special_effect_t& effect )
{
  effect.execute_action = new sorcerous_shadowruby_pendant_driver_t( effect );
}

// Mage Legendary Items
struct sephuzs_secret_t : public class_buff_cb_t<mage_t, haste_buff_t, haste_buff_creator_t>
{
  sephuzs_secret_t(): super( MAGE, "sephuzs_secret" )
  { }

  haste_buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return debug_cast<mage_t*>( e.player ) -> buffs.sephuzs_secret;
  }

  haste_buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.trigger() )
      .cd( e.player -> find_spell( 226262 ) -> duration() )
      .default_value( e.trigger() -> effectN( 2 ).percent() )
      .add_invalidate( CACHE_RUN_SPEED );
  }
};

struct shard_of_the_exodar_t : public scoped_actor_callback_t<mage_t>
{
  shard_of_the_exodar_t() : super( MAGE )
  { }

  void manipulate( mage_t* actor, const special_effect_t& /* e */ ) override
  {
    // Disable default Bloodlust and let us handle it in a custom way.
    actor -> cooldowns.time_warp -> charges = 2;
    actor -> player_t::buffs.bloodlust -> default_chance = 0.0;
    actor -> player_t::buffs.bloodlust -> cooldown -> duration = timespan_t::zero();
  }
};

// Arcane Legendary Items
struct mystic_kilt_of_the_rune_master_t : public scoped_action_callback_t<arcane_barrage_t>
{
  mystic_kilt_of_the_rune_master_t() : super( MAGE_ARCANE, "arcane_barrage" )
  { }

  void manipulate( arcane_barrage_t* action, const special_effect_t& e ) override
  { action -> mystic_kilt_of_the_rune_master_regen = e.driver() -> effectN( 1 ).percent(); }
};

struct rhonins_assaulting_armwraps_t : public class_buff_cb_t<mage_t, buff_t, buff_creator_t>
{
  rhonins_assaulting_armwraps_t() : super( MAGE_ARCANE, "rhonins_assaulting_armwraps" )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return debug_cast<mage_t*>( e.player ) -> buffs.rhonins_assaulting_armwraps;
  }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.trigger() )
      .chance( e.driver() -> effectN( 1 ).percent() );
  }
};

struct cord_of_infinity_t : public class_buff_cb_t<mage_t, buff_t, buff_creator_t>
{
  cord_of_infinity_t() : super( MAGE_ARCANE, "cord_of_infinity" )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return debug_cast<mage_t*>( e.player ) -> buffs.cord_of_infinity;
  }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.trigger() )
      .default_value( e.trigger() -> effectN( 1 ).percent() / 10.0 );
  }
};

struct gravity_spiral_t : public scoped_actor_callback_t<mage_t>
{
  gravity_spiral_t() : super( MAGE_ARCANE )
  { }

  void manipulate( mage_t* actor, const special_effect_t& e ) override
  {
    actor -> cooldowns.evocation -> charges += e.driver() -> effectN( 1 ).base_value();
  }
};

struct mantle_of_the_first_kirin_tor_t : public scoped_action_callback_t<arcane_barrage_t>
{
  mantle_of_the_first_kirin_tor_t() : super( MAGE_ARCANE, "arcane_barrage" )
  { }

  void manipulate( arcane_barrage_t* action, const special_effect_t& e ) override
  { action -> mantle_of_the_first_kirin_tor_chance = e.driver() -> effectN( 1 ).percent(); }
};

// Fire Legendary Items
struct koralons_burning_touch_t : public scoped_action_callback_t<scorch_t>
{
  koralons_burning_touch_t() : super( MAGE_FIRE, "scorch" )
  { }

  void manipulate( scorch_t* action, const special_effect_t& e ) override
  {
    action -> koralons_burning_touch = true;
    action -> koralons_burning_touch_threshold = e.driver() -> effectN( 1 ).base_value();
    action -> koralons_burning_touch_multiplier = e.driver() -> effectN( 2 ).percent();
  }
};

struct darcklis_dragonfire_diadem_t : public scoped_action_callback_t<dragons_breath_t>
{
  darcklis_dragonfire_diadem_t() : super( MAGE_FIRE, "dragons_breath" )
  { }

  void manipulate( dragons_breath_t* action, const special_effect_t& e ) override
  {
    action -> radius += e.driver() -> effectN( 1 ).base_value();
    action -> base_multiplier *= 1.0 + e.driver() -> effectN( 2 ).percent();
  }
};


struct marquee_bindings_of_the_sun_king_t : public class_buff_cb_t<mage_t, buff_t, buff_creator_t>
{
  marquee_bindings_of_the_sun_king_t() : super( MAGE_FIRE, "kaelthas_ultimate_ability" )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return debug_cast<mage_t*>( e.player ) -> buffs.kaelthas_ultimate_ability;
  }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.player -> find_spell( 209455 ) )
      .chance( e.driver() -> effectN( 1 ).percent() );
  }
};

struct pyrotex_ignition_cloth_t : public scoped_action_callback_t<phoenixs_flames_t>
{
  pyrotex_ignition_cloth_t() : super( MAGE_FIRE, "phoenixs_flames" )
  { }

  void manipulate( phoenixs_flames_t* action, const special_effect_t& e ) override
  {
    action -> pyrotex_ignition_cloth = true;
    action -> pyrotex_ignition_cloth_reduction = e.driver() -> effectN( 1 ).time_value();
  }
};

struct contained_infernal_core_t : public class_buff_cb_t<mage_t, buff_t, buff_creator_t>
{
  contained_infernal_core_t() : super( MAGE_FIRE, "contained_infernal_core" )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return debug_cast<mage_t*>( e.player ) -> buffs.contained_infernal_core;
  }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.player -> find_spell( 248146 ) );
  }
};

// Frost Legendary Items
struct magtheridons_banished_bracers_t : public class_buff_cb_t<mage_t, buff_t, buff_creator_t>
{
  magtheridons_banished_bracers_t() : super( MAGE_FROST, "magtheridons_might" )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return debug_cast<mage_t*>( e.player ) -> buffs.magtheridons_might;
  }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.trigger() )
      .default_value( e.trigger() -> effectN( 1 ).percent() );
  }
};

struct zannesu_journey_t : public class_buff_cb_t<mage_t, buff_t, buff_creator_t>
{
  zannesu_journey_t() : super( MAGE_FROST, "zannesu_journey" )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return debug_cast<mage_t*>( e.player ) -> buffs.zannesu_journey;
  }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.trigger() )
      .default_value( e.trigger() -> effectN( 1 ).percent() );
  }
};

struct lady_vashjs_grasp_t : public scoped_action_callback_t<icy_veins_t>
{
  lady_vashjs_grasp_t() : super( MAGE_FROST, "icy_veins" )
  { }

  void manipulate( icy_veins_t* action, const special_effect_t& /* e */ ) override
  {
    action -> lady_vashjs_grasp = true;
  }
};

struct ice_time_t : public scoped_action_callback_t<frozen_orb_t>
{
  ice_time_t() : super( MAGE_FROST, "frozen_orb" )
  { }

  void manipulate( frozen_orb_t* action,
                   const special_effect_t& /* e */ ) override
  {
    action -> ice_time = true;
  }
};

struct shattered_fragments_of_sindragosa_t : public class_buff_cb_t<mage_t, buff_t, buff_creator_t>
{
  shattered_fragments_of_sindragosa_t() : super( MAGE_FROST, "shattered_fragments_of_sindragosa" )
  { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return debug_cast<mage_t*>( e.player ) -> buffs.shattered_fragments_of_sindragosa;
  }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.player -> find_spell( 248176 ) );
  }
};
// MAGE MODULE INTERFACE ====================================================

struct mage_module_t : public module_t
{
public:
  mage_module_t() : module_t( MAGE ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new mage_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new mage_report_t( *p ) );
    return p;
  }

  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 209311, cord_of_infinity_t(),                  true );
    unique_gear::register_special_effect( 208099, koralons_burning_touch_t()                  );
    unique_gear::register_special_effect( 214403, magtheridons_banished_bracers_t(),     true );
    unique_gear::register_special_effect( 206397, zannesu_journey_t(),                   true );
    unique_gear::register_special_effect( 208146, lady_vashjs_grasp_t()                       );
    unique_gear::register_special_effect( 208080, rhonins_assaulting_armwraps_t(),       true );
    unique_gear::register_special_effect( 207547, darcklis_dragonfire_diadem_t()              );
    unique_gear::register_special_effect( 208051, sephuzs_secret_t(),                    true );
    unique_gear::register_special_effect( 207970, shard_of_the_exodar_t()                     );
    unique_gear::register_special_effect( 209450, marquee_bindings_of_the_sun_king_t(),  true );
    unique_gear::register_special_effect( 209280, mystic_kilt_of_the_rune_master_t()          );
    unique_gear::register_special_effect( 222276, sorcerous_shadowruby_pendant                );
    unique_gear::register_special_effect( 235940, pyrotex_ignition_cloth_t()                  );
    unique_gear::register_special_effect( 235227, ice_time_t()                                );
    unique_gear::register_special_effect( 235273, gravity_spiral_t()                          );
    unique_gear::register_special_effect( 248098, mantle_of_the_first_kirin_tor_t()           );
    unique_gear::register_special_effect( 248099, contained_infernal_core_t(),           true );
    unique_gear::register_special_effect( 248100, shattered_fragments_of_sindragosa_t(), true );
  }

  virtual void register_hotfixes() const override
  {
    hotfix::register_spell( "Mage", "2017-01-11", "Incorrect spell level for Frozen Orb Bolt.", 84721 )
      .field( "spell_level" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 57 )
      .verification_value( 81 );

    hotfix::register_spell( "Mage", "2017-03-20", "Manually set Frozen Orb's travel speed.", 84714 )
      .field( "prj_speed" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 20.0 )
      .verification_value( 0.0 );

    hotfix::register_spell( "Mage", "2017-06-21", "Ice Lance is slower than spell data suggests.", 30455 )
      .field( "prj_speed" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 38.0 )
      .verification_value( 50.0 );
  }

  virtual bool valid() const override { return true; }
  virtual void init        ( player_t* ) const override {}
  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end  ( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::mage()
{
  static mage_module_t m;
  return &m;
}
