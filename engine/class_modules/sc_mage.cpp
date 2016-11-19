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


enum mage_rotation_e { ROTATION_NONE = 0, ROTATION_DPS, ROTATION_DPM, ROTATION_MAX };

enum class temporal_hero_e
{
  INVALID,
  ARTHAS,
  JAINA,
  SYLVANAS,
  TYRANDE
};

struct state_switch_t
{
private:
  bool state;
  timespan_t last_enable,
             last_disable;

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
    dot_t* flamestrike;
    dot_t* frost_bomb;
    dot_t* ignite;
    dot_t* living_bomb;
    dot_t* nether_tempest;
    dot_t* frozen_orb;
  } dots;

  struct debuffs_t
  {
    buff_t* erosion,
          * slow;
    buffs::touch_of_the_magi_t* touch_of_the_magi;

    buff_t* firestarter;

    buff_t* chilled,
          * frost_bomb,
          * water_jet, // Proxy Water Jet to compensate for expression system
          * winters_chill;
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
      buff_stack_benefit[ i ] -> update( i == as<unsigned>(buff -> check()) );
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
  // Current target
  player_t* current_target;

  // Icicles
  std::vector<icicle_tuple_t> icicles;
  action_t* icicle;
  event_t* icicle_event;

  // Ignite
  action_t* ignite;
  event_t* ignite_spread_event;

  // Active
  action_t* touch_of_the_magi_explosion;
  action_t* unstable_magic_explosion;
  player_t* last_bomb_target;

  // Artifact effects
  int scorched_earth_counter;

  // Tier 18 (WoD 6.2) trinket effects
  const special_effect_t* wild_arcanist; // Arcane
  const special_effect_t* pyrosurge;     // Fire
  const special_effect_t* shatterlance;  // Frost

  temporal_hero_e last_summoned;

  // State switches for rotation selection
  state_switch_t burn_phase;

  // Miscellaneous
  double distance_from_rune,
         global_cinder_count,
         incanters_flow_stack_mult,
         iv_haste;

  // Benefits
  struct benefits_t
  {
    buff_stack_benefit_t* incanters_flow;

    struct arcane_charge_benefits_t
    {
      buff_stack_benefit_t* arcane_barrage,
                          * arcane_blast,
                          * arcane_explosion,
                          * arcane_missiles,
                          * nether_tempest;
    } arcane_charge;
    buff_source_benefit_t* arcane_missiles;

    buff_source_benefit_t* fingers_of_frost;
    buff_stack_benefit_t* ray_of_frost;
  } benefits;

  // Buffs
  struct buffs_t
  {
    // Arcane
    buff_t* arcane_charge,
          * arcane_power,
          * presence_of_mind,
          * arcane_affinity,       // T17 2pc Arcane
          * arcane_instability,    // T17 4pc Arcane
          * temporal_power,        // T18 4pc Arcane
          * quickening;
    buffs::arcane_missiles_t* arcane_missiles;

    stat_buff_t* mage_armor;

    // Fire
    buff_t* combustion,
          * enhanced_pyrotechnics,
          * heating_up,
          * hot_streak,
          * molten_armor,
          * pyretic_incantation,
          * pyromaniac,            // T17 4pc Fire
          * icarus_uprising,       // T18 4pc Fire
          * streaking;             // T19 4pc Fire

    // Frost
    buff_t* brain_freeze,
          * fingers_of_frost,
          * frost_armor,
          * icicles,               // Buff to track icicles - doesn't always line up with icicle count though!
          * icy_veins,
          * ice_shard,             // T17 2pc Frost
          * frost_t17_4pc,         // T17 4pc Frost
          * shatterlance;          // T18 (WoD 6.2) Frost Trinket


    // Talents
    buff_t* bone_chilling,
          * ice_floes,
          * incanters_flow,
          * ray_of_frost,
          * rune_of_power;

    // Artifact
    // Frost
    buff_t* chain_reaction,
          * chilled_to_the_core;

    // Legendary
    buff_t* cord_of_infinity,
          * kaelthas_ultimate_ability,
          * lady_vashjs_grasp,
          * magtheridons_might,
          * rhonins_assaulting_armwraps,
          * sephuzs_secret,
          * shard_time_warp,
          * zannesu_journey;

  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* combustion,
              * cone_of_cold,
              * dragons_breath,
              * evocation,
              * frozen_orb,
              * icy_veins,
              * fire_blast,
              * phoenixs_flames,
              * presence_of_mind,
              * ray_of_frost;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* evocation,
          * mystic_kilt_of_the_rune_master;
  } gains;

  // Legendary
  struct legendary_t
  {
    bool zannesu_journey;
    bool lady_vashjs_grasp;
    bool sephuzs_secret;
    bool shard_of_the_exodar;
    bool shatterlance;
    bool cord_of_infinity;
    double shatterlance_effect;
    double zannesu_journey_multiplier;
  } legendary;
  // Pets
  struct pets_t
  {
    pets::water_elemental::water_elemental_pet_t* water_elemental;

    std::vector<pet_t*> mirror_images;

    unsigned temporal_hero_count = 10;
    std::vector<pet_t*> temporal_heroes;

    pet_t* arcane_familiar;
  } pets;

  // Procs
  struct procs_t
  {
    proc_t* heating_up_generated, // Crits without HU/HS
          * heating_up_removed, // Non-crits with HU >200ms after application
          * heating_up_ib_converted, // IBs used on HU
          * hot_streak, // Total HS generated
          * hot_streak_spell, // HU/HS spell impacts
          * hot_streak_spell_crit, // HU/HS spell crits
          * hot_streak_spell_crit_wasted; // HU/HS spell crits with HS

    proc_t* ignite_applied, // Direct ignite applications
          * ignite_spread, // Spread events
          * ignite_new_spread, // Spread to new target
          * ignite_overwrite; // Spread to target with existing ignite

    proc_t* controlled_burn; // Tracking Controlled Burn talent
  } procs;

  // Rotation ( DPS vs DPM )
  struct rotation_t
  {
    mage_rotation_e current;
    double mana_gain,
           dps_mana_loss,
           dpm_mana_loss;
    timespan_t dps_time,
               dpm_time,
               last_time;

    void reset() { memset( this, 0, sizeof( *this ) ); current = ROTATION_DPS; }
    rotation_t() { reset(); }
  } rotation;

  // Specializations
  struct specializations_t
  {
    // Arcane
    const spell_data_t* arcane_barrage_2,
                      * arcane_charge,
    // NOTE: arcane_charge_passive is the Arcane passive added in patch 5.2,
    //       called "Arcane Charge" (Spell ID: 114664).
    //       It contains Spellsteal's mana cost reduction, Evocation's mana
    //       gain reduction, and a deprecated Scorch damage reduction.
                      * arcane_charge_passive,
                      * arcane_familiar,
                      * evocation_2,
                      * mage_armor,
                      * savant;

    // Fire
    const spell_data_t* critical_mass,
                      * base_crit_bonus,    //as of 8/25/2016 Fire has a +5% base crit increase.
                      * fire_blast_2,
                      * fire_blast_3,
                      * ignite,
                      * molten_armor,
                      * molten_armor_2;

    // Frost
    const spell_data_t* brain_freeze,
                      * brain_freeze_2,
                      * fingers_of_frost,
                      * frost_armor,
                      * icicles,
                      * icicles_driver,
                      * shatter,
                      * shatter_2;
  } spec;

  // Talents
  struct talents_list_t
  {
    // Tier 15
    const spell_data_t* arcane_familiar,
                      * presence_of_mind,
                      * pyromaniac,
                      * conflagration,
                      * fire_starter,
                      * ray_of_frost,
                      * lonely_winter,
                      * bone_chilling;

    // Tier 30
    const spell_data_t* shimmer, // NYI
                      * cauterize, // NYI
                      * ice_block; // NYI

    // Tier 45
    const spell_data_t* mirror_image,
                      * rune_of_power,
                      * incanters_flow;

    // Tier 60
    const spell_data_t* supernova,
                      * charged_up,
                      * words_of_power, // TODO: Move this to 15
                      * resonance,
                      * blast_wave,
                      * flame_on,
                      * controlled_burn,
                      * ice_nova,
                      * frozen_touch,
                      * splitting_ice;

    // Tier 75
    const spell_data_t* ice_floes,
                      * ring_of_frost, // NYI
                      * ice_ward; // NYI

    // Tier 90
    const spell_data_t* nether_tempest,
                      * living_bomb,
                      * frost_bomb,
                      * unstable_magic,
                      * erosion,
                      * flame_patch,
                      * arctic_gale;

    // Tier 100
    const spell_data_t* overpowered,
                      * quickening,
                      * arcane_orb,
                      * kindling,
                      * cinderstorm,
                      * meteor,
                      * thermal_void,
                      * glacial_spike,
                      * comet_storm;
  } talents;

  // Artifact
  struct artifact_spell_data_t
  {
    // Arcane
    artifact_power_t arcane_rebound,
                     ancient_power,
                     everywhere_at_once, //NYI
                     arcane_purification,
                     aegwynns_imperative,
                     aegwynns_ascendance,
                     aegwynns_wrath,
                     crackling_energy,
                     blasting_rod,
                     ethereal_sensitivity,
                     aegwynns_fury,
                     mana_shield, // NYI
                     mark_of_aluneth,
                     might_of_the_guardians,
                     rule_of_threes,
                     slooow_down, // NYI
                     torrential_barrage,
                     touch_of_the_magi;

    // Fire
    artifact_power_t aftershocks,
                     scorched_earth,
                     everburning_consumption,
                     blue_flame_special,
                     molten_skin, //NYI
                     phoenix_reborn,
                     great_balls_of_fire,
                     cauterizing_blink, //NYI
                     fire_at_will,
                     pyroclasmic_paranoia,
                     reignition_overdrive,
                     pyretic_incantation,
                     phoenixs_flames,
                     burning_gaze,
                     big_mouth, //NYI
                     blast_furnace,
                     wings_of_flame,
                     empowered_spellblade;

    // Frost
    artifact_power_t ebonbolt,
                     jouster, // NYI
                     let_it_go,
                     frozen_veins,
                     the_storm_rages,
                     black_ice,
                     shield_of_alodi, //NYI
                     icy_caress,
                     ice_nine,
                     chain_reaction,
                     clarity_of_thought,
                     its_cold_outside,
                     shattering_bolts,
                     orbital_strike,
                     icy_hand,
                     ice_age,
                     chilled_to_the_core,
                     spellborne;
  } artifact;

public:
  mage_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF );

  ~mage_t();

  // Character Definition
  virtual           std::string get_special_use_items( const std::string& item = std::string(), bool specials = false );
  virtual           std::vector<std::string> get_non_speical_item_actions();
  virtual void      init_spells() override;
  virtual void      init_base_stats() override;
  virtual void      create_buffs() override;
  virtual void      create_options() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init_benefits() override;
  virtual void      init_stats() override;
  virtual void      init_assessors() override;
  virtual void      invalidate_cache( cache_e c ) override;
  void init_resources( bool force ) override;
  virtual void      recalculate_resource_max( resource_e rt ) override;
  virtual void      reset() override;
  virtual expr_t*   create_expression( action_t*, const std::string& name ) override;
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual action_t* create_proc_action( const std::string& name, const special_effect_t& effect ) override;
  virtual void      create_pets() override;
  virtual resource_e primary_resource() const override { return RESOURCE_MANA; }
  virtual role_e    primary_role() const override { return ROLE_SPELL; }
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual double    mana_regen_per_second() const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  virtual double    composite_spell_crit_chance() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_mastery_rating() const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual void      update_movement( timespan_t duration ) override;
  virtual void      stun() override;
  virtual double    temporary_movement_modifier() const override;
  virtual void      arise() override;
  virtual action_t* select_action( const action_priority_list_t& ) override;

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

  // Resource gain tracking
  virtual double resource_gain( resource_e, double amount,
                                gain_t* = 0, action_t* = 0 ) override;
  virtual double resource_loss( resource_e, double amount,
                                gain_t* = 0, action_t* = 0 ) override;

  // Public mage functions:
  icicle_data_t get_icicle_object();
  void trigger_icicle( const action_state_t* trigger_state, bool chain = false, player_t* chain_target = nullptr );

  void trigger_touch_of_the_magi( buffs::touch_of_the_magi_t* touch_of_the_magi_buff );

  void              apl_precombat();
  void              apl_arcane();
  void              apl_fire();
  void              apl_frost();
  void              apl_default();
  virtual void      init_action_list() override;

  virtual bool      has_t18_class_trinket() const override;
  std::string       get_food_action();
  std::string       get_potion_action();
};

namespace pets
{
struct mage_pet_t : public pet_t
{
  mage_pet_t( sim_t* sim, mage_t* owner, std::string pet_name,
              bool guardian = false, bool dynamic = false )
    : pet_t( sim, owner, pet_name, guardian, dynamic )
  {
  }
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
  {
  }

  mage_t* o()
  {
    return static_cast<mage_pet_t*>( player )->o();
  }

  const mage_t* o() const
  {
    return static_cast<mage_pet_t*>( player )->o();
  }

  virtual void schedule_execute( action_state_t* execute_state ) override
  {
    target = o()->current_target;

    spell_t::schedule_execute( execute_state );
  }
};

struct mage_pet_melee_attack_t : public melee_attack_t
{
  mage_pet_melee_attack_t( const std::string& n, mage_pet_t* p )
    : melee_attack_t( n, p, spell_data_t::nil() )
  {
  }

  mage_t* o()
  {
    return static_cast<mage_pet_t*>( player )->o();
  }

  const mage_t* o() const
  {
    return static_cast<mage_pet_t*>( player )->o();
  }

  virtual void schedule_execute( action_state_t* execute_state ) override
  {
    target = o()->current_target;

    melee_attack_t::schedule_execute( execute_state );
  }
};

namespace arcane_familiar
{
//================================================================================
// Pet Arcane Familiar
//================================================================================
struct arcane_familiar_pet_t : public mage_pet_t
{
  arcane_familiar_pet_t( sim_t* sim, mage_t* owner )
    : mage_pet_t( sim, owner, "arcane_familiar", true )
  {
    owner_coeff.sp_from_sp = 1.00;
  }

  void arise() override
  {
    mage_pet_t::arise();

    owner->recalculate_resource_max( RESOURCE_MANA );
  }

  void demise() override
  {
    mage_pet_t::demise();

    owner->recalculate_resource_max( RESOURCE_MANA );
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override;

  virtual void init_action_list() override
  {
    action_list_str = "arcane_assault";

    mage_pet_t::init_action_list();
  }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = mage_pet_t::composite_player_multiplier( school );

    if ( o()->buffs.rune_of_power->check() )
    {
      m *= 1.0 + o()->buffs.rune_of_power->data().effectN( 3 ).percent();
    }

    if ( o()->talents.incanters_flow->ok() )
    {
      m *= 1.0 +
           o()->buffs.incanters_flow->current_stack *
               o()->incanters_flow_stack_mult;
    }
    return m;
  }
};

struct arcane_assault_t : public mage_pet_spell_t
{
  arcane_assault_t( arcane_familiar_pet_t* p, const std::string& options_str )
    : mage_pet_spell_t( "arcane_assault", p, p->find_spell( 205235 ) )
  {
    parse_options( options_str );
    spell_power_mod.direct = p->find_spell( 225119 )->effectN( 1 ).sp_coeff();
    cooldown -> duration = timespan_t::from_seconds( 3.0 );
    cooldown -> hasted = true;
    may_crit = true;
  }
};

action_t* arcane_familiar_pet_t::create_action( const std::string& name,
                                                const std::string& options_str )
{
  if ( name == "arcane_assault" )
    return new arcane_assault_t( this, options_str );

  return mage_pet_t::create_action( name, options_str );
}

}  // arcane familiar

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

  water_elemental_pet_t( sim_t* sim, mage_t* owner )
    : mage_pet_t( sim, owner, "water_elemental" )
  {
    owner_coeff.sp_from_sp = 0.75;
  }

  void init_action_list() override
  {
    clear_action_priority_lists();
    auto default_list = get_action_priority_list( "default" );

    default_list->add_action( this, find_pet_spell( "Water Jet" ), "Water Jet" );
    default_list->add_action( this, find_pet_spell( "Waterbolt" ), "Waterbolt" );

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

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = mage_pet_t::composite_player_multiplier( school );

    if ( o()->buffs.rune_of_power->check() )
    {
      m *= 1.0 + o()->buffs.rune_of_power->data().effectN( 3 ).percent();
    }

    if ( o()->talents.incanters_flow->ok() )
    {
      m *= 1.0 +
           o()->buffs.incanters_flow->current_stack *
               o()->incanters_flow_stack_mult;
    }

    return m;
  }
};

water_elemental_pet_td_t::water_elemental_pet_td_t(
    player_t* target, water_elemental_pet_t* welly )
  : actor_target_data_t( target, welly )
{
  water_jet = buff_creator_t( *this, "water_jet", welly->find_spell( 135029 ) )
                  .cd( timespan_t::zero() );
}

struct waterbolt_t : public mage_pet_spell_t
{
  int fof_source_id;

  waterbolt_t( water_elemental_pet_t* p, const std::string& options_str )
    : mage_pet_spell_t( "waterbolt", p, p->find_pet_spell( "Waterbolt" ) )
  {
    trigger_gcd = timespan_t::zero();
    parse_options( options_str );
    may_crit = true;
  }

  virtual bool init_finished() override
  {
    fof_source_id = o() -> benefits.fingers_of_frost
                        -> get_source_id( data().name_cstr() );

    return mage_pet_spell_t::init_finished();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t cast_time = mage_pet_spell_t::execute_time();

    // For some reason welly seems to have a cap'd rate of cast of
    // 1.5/second. Instead of modeling this as a cooldown/GCD (like it is in game)
    // we model it as a capped cast time, with 1.5 being the lowest it can go.
    return std::max(cast_time, timespan_t::from_seconds( 1.5 ) );
  }

  virtual double action_multiplier() const override
  {
    double am = mage_pet_spell_t::action_multiplier();

    if ( o()->spec.icicles->ok() )
    {
      am *= 1.0 + o()->cache.mastery_value();
    }

    if ( o()->artifact.its_cold_outside.rank() )
    {
      am *= 1.0 + o()->artifact.its_cold_outside.data().effectN( 3 ).percent();
    }
    return am;
  }

  virtual void impact( action_state_t* s ) override
  {
    double fof_chance = o() -> artifact.its_cold_outside.percent();

    spell_t::impact( s );
    if ( result_is_hit( s->result ) && rng().roll( fof_chance ) )
    {
      o() -> buffs.fingers_of_frost -> trigger();
      o() -> benefits.fingers_of_frost -> update( fof_source_id );
    }
  }
};

struct freeze_t : public mage_pet_spell_t
{
  int fof_source_id;

  freeze_t( water_elemental_pet_t* p, const std::string& options_str )
    : mage_pet_spell_t( "freeze", p, p->find_pet_spell( "Freeze" ) )
  {
    parse_options( options_str );
    aoe                   = -1;
    may_crit              = true;
    ignore_false_positive = true;
    action_skill          = 1;
  }

  virtual bool init_finished() override
  {
    water_elemental_pet_t* p = static_cast<water_elemental_pet_t*>( player );
    fof_source_id = p -> o() -> benefits.fingers_of_frost
                             -> get_source_id( data().name_cstr() );

    return mage_pet_spell_t::init_finished();
  }

  virtual void impact( action_state_t* s ) override
  {
    spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      o() -> buffs.fingers_of_frost->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
      o() -> benefits.fingers_of_frost->update( fof_source_id );
    }
  }
};

struct water_jet_t : public mage_pet_spell_t
{
  // queued water jet spell, auto cast water jet spell
  bool queued, autocast;

  water_jet_t( water_elemental_pet_t* p, const std::string& options_str )
    : mage_pet_spell_t( "water_jet", p, p->find_spell( 135029 ) ),
      queued( false ),
      autocast( true )
  {
    parse_options( options_str );
    channeled = tick_may_crit = true;

    if ( p->o()->sets.has_set_bonus( MAGE_FROST, T18, B4 ) )
    {
      dot_duration += p->find_spell( 185971 )->effectN( 1 ).time_value();
    }
  }
  water_elemental_pet_td_t* td( player_t* t ) const
  {
    return static_cast<water_elemental_pet_t*>( player )
        ->get_target_data( t ? t : target );
  }

  void execute() override
  {
    mage_pet_spell_t::execute();
    // If this is a queued execute, disable queued status
    if ( !autocast && queued )
      queued = false;
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_pet_spell_t::impact( s );

    td( s->target )
        ->water_jet->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0,
                              dot_duration * player->composite_spell_haste() );

    // Trigger hidden proxy water jet for the mage, so
    // debuff.water_jet.<expression> works
    o()->get_target_data( s->target )
        ->debuffs.water_jet->trigger(
            1, buff_t::DEFAULT_VALUE(), 1.0,
            dot_duration * player->composite_spell_haste() );
  }

  virtual double action_multiplier() const override
  {
    double am = mage_pet_spell_t::action_multiplier();

    if ( o()->spec.icicles->ok() )
    {
      am *= 1.0 + o()->cache.mastery_value();
    }

    return am;
  }

  virtual void last_tick( dot_t* d ) override
  {
    mage_pet_spell_t::last_tick( d );
    td( d->target )->water_jet->expire();
  }

  bool ready() override
  {
    // Not ready, until the owner gives permission to cast
    if ( !autocast && !queued )
      return false;

    return mage_pet_spell_t::ready();
  }

  void reset() override
  {
    mage_pet_spell_t::reset();

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
    if ( o()->specialization() == MAGE_FIRE )
    {
      action_list_str = "fireball";
    }
    else if ( o()->specialization() == MAGE_ARCANE )
    {
      action_list_str = "arcane_blast";
    }
    else
    {
      action_list_str = "frostbolt";
    }

    mage_pet_t::init_action_list();
  }

  virtual void create_buffs() override
  {
    mage_pet_t::create_buffs();

    arcane_charge =
        buff_creator_t( this, "arcane_charge", o()->spec.arcane_charge );
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
    if ( p()->o()->pets.mirror_images[ 0 ] )
    {
      stats = p()->o()->pets.mirror_images[ 0 ]->get_stats( name_str );
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
                            p->find_pet_spell( "Arcane Blast" ) )
  {
    parse_options( options_str );
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
};

struct fireball_t : public mirror_image_spell_t
{
  fireball_t( mirror_image_pet_t* p, const std::string& options_str )
    : mirror_image_spell_t( "fireball", p, p->find_pet_spell( "Fireball" ) )
  {
    parse_options( options_str );
  }
};

struct frostbolt_t : public mirror_image_spell_t
{
  frostbolt_t( mirror_image_pet_t* p, const std::string& options_str )
    : mirror_image_spell_t( "frostbolt", p, p->find_pet_spell( "Frostbolt" ) )
  {
    parse_options( options_str );
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

namespace temporal_hero
{
// ==========================================================================
// Pet Temporal Heroes (2T18)
// ==========================================================================

struct temporal_hero_t : public mage_pet_t
{
  temporal_hero_e hero_type;

  // Each Temporal Hero has base damage and hidden multipliers for damage done
  // Values are reverse engineered from 6.2 PTR Build 20157 testing
  double temporal_hero_multiplier;

  temporal_hero_t( sim_t* sim, mage_t* owner )
    : mage_pet_t( sim, owner, "temporal_hero", true, true ),
      hero_type( temporal_hero_e::ARTHAS ),
      temporal_hero_multiplier( 1.0 )
  {
  }

  void init_base_stats() override
  {
    owner_coeff.ap_from_sp = 11.408;
    owner_coeff.sp_from_sp = 5.0;

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 7262.57;
    main_hand_weapon.max_dmg    = 7262.57;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    pet_t::init_base_stats();
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack/frostbolt/shoot";

    use_default_action_list = true;

    pet_t::init_action_list();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override;

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    // Temporal Hero benefits from Temporal Power applied by itself (1 stack).
    // Using owner's buff object, in order to avoid creating a separate buff_t
    // for each pet instance, and merging the buff statistics.
    if ( owner->sets.has_set_bonus( MAGE_ARCANE, T18, B4 ) )
    {
      m *= 1.0 + o()->buffs.temporal_power->data().effectN( 1 ).percent();
    }

    m *= temporal_hero_multiplier;

    return m;
  }

  void arise() override
  {
    pet_t::arise();
    assert( o()->last_summoned != temporal_hero_e::INVALID );

    // Summoned heroes follow Jaina -> Arthas -> Sylvanas -> Tyrande order
    if ( o()->last_summoned == temporal_hero_e::JAINA )
    {
      hero_type = temporal_hero_e::ARTHAS;
      o()->last_summoned       = hero_type;
      temporal_hero_multiplier = 0.1964;

      if ( sim->debug )
      {
        sim->out_debug.printf( "%s summons 2T18 temporal hero: Arthas",
                               owner->name() );
      }
    }
    else if ( o()->last_summoned == temporal_hero_e::TYRANDE )
    {
      hero_type = temporal_hero_e::JAINA;
      o()->last_summoned       = hero_type;
      temporal_hero_multiplier = 0.6;

      if ( sim->debug )
      {
        sim->out_debug.printf( "%s summons 2T18 temporal hero: Jaina",
                               owner->name() );
      }
    }
    else if ( o()->last_summoned == temporal_hero_e::ARTHAS )
    {
      hero_type = temporal_hero_e::SYLVANAS;
      o()->last_summoned       = hero_type;
      temporal_hero_multiplier = 0.5283;

      if ( sim->debug )
      {
        sim->out_debug.printf( "%s summons 2T18 temporal hero: Sylvanas",
                               owner->name() );
      }
    }
    else
    {
      hero_type = temporal_hero_e::TYRANDE;
      o()->last_summoned       = hero_type;
      temporal_hero_multiplier = 0.5283;

      if ( sim->debug )
      {
        sim->out_debug.printf( "%s summons 2T18 temporal hero: Tyrande",
                               owner->name() );
      }
    }

    if ( owner->sets.has_set_bonus( MAGE_ARCANE, T18, B4 ) )
    {
      o()->buffs.temporal_power->trigger();
    }

    owner->invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  void demise() override
  {
    pet_t::demise();

    o()->buffs.temporal_power->decrement();

    owner->invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
};

struct temporal_hero_melee_attack_t : public mage_pet_melee_attack_t
{
  temporal_hero_melee_attack_t( temporal_hero_t* p )
    : mage_pet_melee_attack_t( "melee", p )
  {
    may_crit   = true;
    background = repeating = true;
    school            = SCHOOL_PHYSICAL;
    special           = false;
    weapon            = &( p->main_hand_weapon );
    base_execute_time = weapon->swing_time;
  }

  bool init_finished() override
  {
    pet_t* p  = debug_cast<pet_t*>( player );
    mage_t* m = debug_cast<mage_t*>( p->owner );

    if ( m->pets.temporal_heroes[ 0 ] )
    {
      stats = m->pets.temporal_heroes[ 0 ]->get_stats( "melee" );
    }

    return mage_pet_melee_attack_t::init_finished();
  }
};

struct temporal_hero_autoattack_t : public mage_pet_melee_attack_t
{
  temporal_hero_autoattack_t( temporal_hero_t* p )
    : mage_pet_melee_attack_t( "auto_attack", p )
  {
    p->main_hand_attack = new temporal_hero_melee_attack_t( p );
    trigger_gcd         = timespan_t::zero();
  }

  virtual void execute() override
  {
    player->main_hand_attack->schedule_execute();
  }

  virtual bool ready() override
  {
    temporal_hero_t* hero = static_cast<temporal_hero_t*>( player );
    if ( hero->hero_type != temporal_hero_e::ARTHAS )
    {
      return false;
    }

    return player->main_hand_attack->execute_event == nullptr;
  }
};

struct temporal_hero_frostbolt_t : public mage_pet_spell_t
{
  temporal_hero_frostbolt_t( temporal_hero_t* p )
    : mage_pet_spell_t( "frostbolt", p, p->find_spell( 191764 ) )
  {
    base_dd_min = base_dd_max = 2750.0;
    may_crit = true;
  }

  virtual bool ready() override
  {
    temporal_hero_t* hero = static_cast<temporal_hero_t*>( player );
    if ( hero->hero_type != temporal_hero_e::JAINA )
    {
      return false;
    }

    return mage_pet_spell_t::ready();
  }

  bool init_finished() override
  {
    temporal_hero_t* p = static_cast<temporal_hero_t*>( player );
    mage_t* m          = p->o();

    if ( m->pets.temporal_heroes[ 0 ] )
    {
      stats = m->pets.temporal_heroes[ 0 ]->get_stats( "frostbolt" );
    }

    return mage_pet_spell_t::init_finished();
  }
};

struct temporal_hero_shoot_t : public mage_pet_spell_t
{
  temporal_hero_shoot_t( temporal_hero_t* p )
    : mage_pet_spell_t( "shoot", p, p->find_spell( 191799 ) )
  {
    school      = SCHOOL_PHYSICAL;
    base_dd_min = base_dd_max = 3255.19;
    base_execute_time = p->main_hand_weapon.swing_time;
    may_crit          = true;
  }

  virtual bool ready() override
  {
    temporal_hero_t* hero = static_cast<temporal_hero_t*>( player );
    if ( hero->hero_type != temporal_hero_e::SYLVANAS &&
         hero->hero_type != temporal_hero_e::TYRANDE )
    {
      return false;
    }

    return player->main_hand_attack->execute_event == nullptr;
  }

  bool init_finished() override
  {
    temporal_hero_t* p = static_cast<temporal_hero_t*>( player );
    mage_t* m          = p->o();

    if ( m->pets.temporal_heroes[ 0 ] )
    {
      stats = m->pets.temporal_heroes[ 0 ]->get_stats( "shoot" );
    }

    return mage_pet_spell_t::init_finished();
  }
};

action_t* temporal_hero_t::create_action( const std::string& name,
                                          const std::string& options_str )
{
  if ( name == "auto_attack" )
    return new temporal_hero_autoattack_t( this );
  if ( name == "frostbolt" )
    return new temporal_hero_frostbolt_t( this );
  if ( name == "shoot" )
    return new temporal_hero_shoot_t( this );

  return base_t::create_action( name, options_str );
}

void randomize_last_summoned( mage_t* p )
{
  double rand = p->rng().real();
  if ( rand < 0.25 )
  {
    p->last_summoned = temporal_hero_e::ARTHAS;
  }
  else if ( rand < 0.5 )
  {
    p->last_summoned = temporal_hero_e::JAINA;
  }
  else if ( rand < 0.75 )
  {
    p->last_summoned = temporal_hero_e::SYLVANAS;
  }
  else
  {
    p->last_summoned = temporal_hero_e::TYRANDE;
  }
}

}  // temporal_hero

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
  {
  }

  virtual const char* name() const override
  { return "cinder_impact_event"; }

  void execute() override
  {
    cinder -> target = target;
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

    if ( p -> artifact.ethereal_sensitivity.rank() )
    {
      am_proc_chance += p -> artifact.ethereal_sensitivity.percent();
    }

    if ( p -> sets.has_set_bonus( MAGE_ARCANE, T19, B2 ) )
    {
      am_proc_chance += p -> sets.set( MAGE_ARCANE, T19, B2 ) -> effectN( 1 ).percent();
    }
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


// Chilled debuff =============================================================

struct chilled_t : public buff_t
{
  chilled_t( mage_td_t* td ) :
    buff_t( buff_creator_t( *td, "chilled",
                            td -> source -> find_spell( 205708 ) ) )
  {}

  bool trigger( int stacks, double value,
                double chance, timespan_t duration ) override
  {
    //sim -> out_debug.printf("SOURCE OF chill IS %s", source -> name() );
    mage_t* p = debug_cast<mage_t*>( source );

    if ( p -> talents.bone_chilling -> ok() )
    {
      p -> buffs.bone_chilling -> trigger();
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
        return data->duration();
      }
      return data->effectN( 1 ).period();
    }

    erosion_event_t( actor_t& m, erosion_t* _debuff, const spell_data_t* _data,
                     bool player_triggered = false )
      : event_t( m, delta_time( _data, player_triggered ) ),
        debuff( _debuff ),
        data( _data )
    {
    }

    const char* name() const override
    { return "erosion_decay_event"; }


    void execute() override
    {
      debuff -> decrement();

      // Always update the parent debuff's reference to the decay event, so that it
      // can be cancelled upon a new application of the debuff
      if ( debuff->check() > 0 )
      {
        debuff->decay_event = make_event<erosion_event_t>(
            sim(), *( debuff->source ), debuff, data );
      }
      else
      {
        debuff->decay_event = nullptr;
      }
    }
  };

  const spell_data_t* erosion_event_data;
  event_t* decay_event;

  erosion_t( mage_td_t* td ) :
    buff_t( buff_creator_t( *td, "erosion",
                            td -> source -> find_spell( 210134 ) ) ),
    erosion_event_data( td -> source -> find_spell( 210154) ),
    decay_event( nullptr )
  {}

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
struct icy_veins_buff_t : public buff_t
{
  mage_t* p;
  icy_veins_buff_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "icy_veins", p -> find_spell( 12472 ) )
            .add_invalidate( CACHE_SPELL_HASTE ) ),p( p )

  {}

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
    if ( p -> legendary.lady_vashjs_grasp )
    {
      p -> buffs.lady_vashjs_grasp -> expire();
    }
  }
};

struct sephuzs_secret_buff_t : public buff_t
{
  cooldown_t* icd;
  sephuzs_secret_buff_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "sephuzs_secret", p -> find_spell( 208052 ) )
                            .default_value( p -> find_spell( 208502 ) -> effectN( 2 ).percent() )
                            .add_invalidate( CACHE_SPELL_HASTE ) )
  {
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

struct incanters_flow_t : public buff_t
{
  incanters_flow_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "incanters_flow", p -> find_spell( 116267 ) ) // Buff is a separate spell
            .duration( p -> sim -> max_time * 3 ) // Long enough duration to trip twice_expected_event
            .period( p -> talents.incanters_flow -> effectN( 1 ).period() ) ) // Period is in the talent
  { }

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

  lady_vashjs_grasp_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "lady_vashjs_grasp",
                            p -> find_spell( 208147 ) )
              .tick_callback( [ p ]( buff_t* buff, int, const timespan_t& )
                {
                  p -> buffs.fingers_of_frost -> trigger();
                  lady_vashjs_grasp_t * lvg =
                    debug_cast<lady_vashjs_grasp_t *>( buff );
                  p -> benefits.fingers_of_frost
                    -> update( lvg -> fof_source_id );
                }
              )
          )
  {}
};


struct ray_of_frost_buff_t : public buff_t
{
  timespan_t rof_cd;

  ray_of_frost_buff_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "ray_of_frost", p -> find_spell( 208141 ) ) )
  {
    const spell_data_t* rof_data = p -> find_spell( 205021 );
    rof_cd = rof_data -> cooldown() - rof_data -> duration();
  }

  //TODO: This be calling expire_override instead
  virtual void aura_loss() override
  {
    buff_t::aura_loss();

    mage_t* p = static_cast<mage_t*>( player );
    p -> cooldowns.ray_of_frost -> start( rof_cd );

    if ( p -> channeling && p -> channeling -> name_str == "ray_of_frost" )
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

struct mage_spell_t : public spell_t
{
  bool consumes_ice_floes,
       frozen,
       triggers_arcane_missiles;

  int am_trigger_source_id;
public:
  int dps_rotation,
      dpm_rotation;

  mage_spell_t( const std::string& n, mage_t* p,
                const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( n, p, s ),
    consumes_ice_floes( true ),
    frozen( false ),
    am_trigger_source_id( -1 ),
    dps_rotation( 0 ),
    dpm_rotation( 0 )
  {
    triggers_arcane_missiles = harmful && !background;
    may_crit      = true;
    tick_may_crit = true;
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

  virtual double cost() const override
  {
    double c = spell_t::cost();

    if ( p() -> buffs.arcane_power -> check() )
    {
      c *= 1.0 + p() -> buffs.arcane_power -> data().effectN( 2 ).percent();
    }

    return c;
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    // If there is no state to schedule, make one and put the actor's current
    // target into it. This guarantees that:
    // 1) action_execute_event_t::execute() does not execute on dead targets, if the target died during cast
    // 2) We do not modify action's variables this early in the game
    //
    // If this is a tick action, there's going to be a state object passed to
    // it, that will have the correct target anyhow, since the parent will have
    // had it's target adjusted during its execution.
    if ( state == nullptr )
    {
      state = get_state();

      // If cycle_targets or target_if option is used, we need to target the spell to the (found)
      // target of the action, as it was selected during the action_t::ready() call.
      if ( cycle_targets == 1 || target_if_mode != TARGET_IF_NONE )
        state -> target = target;
      // Put the actor's current target into the state object always.
      else
        state -> target = p() -> current_target;
    }

    spell_t::schedule_execute( state );
  }

  virtual bool usable_moving() const override
  {
    if ( p() -> buffs.ice_floes -> check() )
    {
      return true;
    }

    return spell_t::usable_moving();
  }

  virtual double composite_crit_chance_multiplier() const override
  {
    double m = spell_t::composite_crit_chance_multiplier();

    if ( frozen && p() -> spec.shatter -> ok() )
    {
      m *= 1.0 + ( p() -> spec.shatter -> effectN( 2 ).percent() + p() -> spec.shatter_2 -> effectN( 1 ).percent() );
    }

    return m;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double tm = spell_t::composite_target_multiplier( target );
    mage_td_t* tdata = td( target );

    if ( school == SCHOOL_ARCANE )
    {
      tm *= 1.0 + tdata -> debuffs.erosion -> check() *
                  tdata -> debuffs.erosion -> data().effectN( 1 ).percent();
    }

    return tm;
  }

  void snapshot_internal( action_state_t* s, uint32_t flags,
                          dmg_e rt ) override
  {
    spell_t::snapshot_internal( s, flags, rt );

    // Shatter's +50% crit bonus needs to be added after multipliers etc
    if ( ( flags & STATE_CRIT ) && frozen && p() -> spec.shatter -> ok() )
    {
      s -> crit_chance += ( p() -> spec.shatter -> effectN( 2 ).percent() + p() -> spec.shatter_2 -> effectN( 1 ).percent() );
    }
  }

  // You can thank Frost Nova for why this isn't in arcane_mage_spell_t instead
  void trigger_am( int source_id, double chance = -1.0,
                   int stacks = 1 )
  {
    if ( static_cast<buff_t*>(p() -> buffs.arcane_missiles)
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
    player_t* original_target = nullptr;
    // Mage spells will always have a pre_execute_state defined, because of
    // schedule_execute() trickery.
    //
    // Adjust the target of this action to always match what the
    // pre_execute_state targets. Note that execute() will never be called if
    // the actor's current target (at the time of cast beginning) has demised
    // before the cast finishes.
    if ( pre_execute_state )
    {
      // Adjust target if necessary
      if ( target != pre_execute_state -> target )
      {
        original_target = target;
        target = pre_execute_state -> target;
      }

      // Massive hack to describe a situation where schedule_execute()
      // forcefully made a pre-execute state to pass the current target to
      // execute. In this case we release the pre_execute_state, because we
      // want the action to snapshot it's own stats on "cast finish". We have,
      // however changed the target of the action to the one specified whe nthe
      // casting begun (in schedule_execute()).
      if ( pre_execute_state -> result_type == RESULT_TYPE_NONE )
      {
        action_state_t::release( pre_execute_state );
        pre_execute_state = nullptr;
      }
    }

    spell_t::execute();

    // Restore original target if necessary
    if ( original_target )
      target = original_target;

    if ( background )
      return;

    if ( execute_time() > timespan_t::zero() && consumes_ice_floes && p() -> buffs.ice_floes -> up() )
    {
      p() -> buffs.ice_floes -> decrement();
    }

    if ( p() -> specialization() == MAGE_ARCANE &&
         result_is_hit( execute_state -> result ) &&
         triggers_arcane_missiles )
    {
      trigger_am( am_trigger_source_id );
    }

    if ( harmful && !background && p() -> talents.incanters_flow -> ok() )
    {
      p() -> benefits.incanters_flow -> update();
    }
  }

  void trigger_unstable_magic( action_state_t* state );
};

typedef residual_action::residual_periodic_action_t< mage_spell_t > residual_action_t;


// ============================================================================
// Arcane Mage Spell
// ============================================================================

struct arcane_mage_spell_t : public mage_spell_t
{
  arcane_mage_spell_t( const std::string& n, mage_t* p,
                       const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s )
  {}

  double arcane_charge_damage_bonus() const
  {
    double per_ac_bonus = p() -> spec.arcane_charge -> effectN( 1 ).percent() +
                          ( p() -> composite_mastery() *
                           p() -> spec.savant -> effectN( 2 ).mastery_value() );
    return 1.0 + p() -> buffs.arcane_charge -> check() * per_ac_bonus;

  }

  virtual double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

      if ( p() -> artifact.might_of_the_guardians && school == SCHOOL_ARCANE )
      {
         am *= 1.0 + p() -> artifact.might_of_the_guardians.percent();
      }

      if ( p() -> artifact.ancient_power && school == SCHOOL_ARCANE )
      {
        am *= 1.0 + p() -> artifact.ancient_power.percent();
      }
    return am;
  }

  virtual timespan_t gcd() const override
  {
    timespan_t t = mage_spell_t::gcd();
    return t;
  }
  virtual void execute() override
  {
    mage_spell_t::execute();
    if( ( p() -> resources.current[ RESOURCE_MANA ] / p() -> resources.max[ RESOURCE_MANA ] ) <= p() -> buffs.cord_of_infinity -> default_value &&
         p() -> legendary.cord_of_infinity == true )
    {
      p() -> buffs.cord_of_infinity -> trigger();
    }
  }
  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( p() -> talents.erosion -> ok() &&  result_is_hit( s -> result ) && harmful == true
      && s -> action -> id != 224968 )
    {
      td( s -> target ) -> debuffs.erosion -> trigger();
    }
  }
};


// ============================================================================
// Fire Mage Spell
// ============================================================================

struct ignite_spell_state_t : action_state_t
{
  bool hot_streak;

  ignite_spell_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ),
    hot_streak( false )
  { }

  virtual void initialize() override
  {
    action_state_t:: initialize();
    hot_streak = false;
  }

  virtual std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s ) << " hot_streak=" << hot_streak;
    return s;
  }

  virtual void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );
    const ignite_spell_state_t* is =
      debug_cast<const ignite_spell_state_t*>( s );
    hot_streak = is -> hot_streak;
  }
};

struct fire_mage_spell_t : public mage_spell_t
{
  bool triggers_pyretic_incantation,
       triggers_hot_streak,
       triggers_ignite;

  fire_mage_spell_t( const std::string& n, mage_t* p,
                     const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s ),
    triggers_pyretic_incantation( false ),
    triggers_hot_streak( false ),
    triggers_ignite( false )
  {
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( triggers_ignite && result_is_hit( s -> result ) && p() -> ignite )
    {
      trigger_ignite( s );
    }

    if ( triggers_hot_streak && result_is_hit( s -> result ) )
    {
      handle_hot_streak( s );
    }

    if ( triggers_pyretic_incantation
                      && p() -> artifact.pyretic_incantation.rank()
                      && result_is_hit( s -> result ) && s -> result == RESULT_CRIT )
    {
      p() -> buffs.pyretic_incantation -> trigger();
    }
    else if ( triggers_pyretic_incantation
                      && p() -> artifact.pyretic_incantation.rank()
                      && result_is_hit( s -> result ) && s -> result != RESULT_CRIT )
    {
      p() -> buffs.pyretic_incantation -> expire();
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
          if ( s -> action -> s_data -> _id == 108853 )
          {
            p -> procs.heating_up_ib_converted -> occur();
          }

          p -> buffs.heating_up -> expire();
          p -> buffs.hot_streak -> trigger();
          p -> buffs.pyromaniac -> trigger();

          //TODO: Add proc tracking to this to track from talent or non-talent sources.
          if ( p -> sets.has_set_bonus( MAGE_FIRE, T19, B4 ) &&
               rng().roll( p -> sets.set( MAGE_FIRE, T19, B4) -> effectN( 1 ).percent() ) )
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
            if ( p -> sets.has_set_bonus( MAGE_FIRE, T19, B4 ) &&
                  rng().roll( p -> sets.set( MAGE_FIRE, T19, B4 ) -> effectN( 1 ).percent() ) )
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
    mage_t* p = this -> p();

    double amount = s -> result_amount * p -> cache.mastery_value();

    // TODO: Use client data from hot streak
    amount *= composite_ignite_multiplier( s );


    amount *= 1.0 + p -> artifact.everburning_consumption.percent();

    bool ignite_exists = p -> ignite -> get_dot( s -> target ) -> is_ticking();

    residual_action::trigger( p -> ignite, s -> target, amount );

    if ( !ignite_exists )
    {
      p -> procs.ignite_applied -> occur();
    }
  }

  virtual double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

      if ( p() -> artifact.wings_of_flame && school == SCHOOL_FIRE )
      {
         am *= 1.0 + p() -> artifact.wings_of_flame.percent();
      }

      if ( p() -> artifact.empowered_spellblade && school == SCHOOL_FIRE )
      {
        am *= 1.0 + p() -> artifact.empowered_spellblade.percent();
      }
    return am;
  }
};


// ============================================================================
// Frost Mage Spell
// ============================================================================
//

// Custom Frost Mage spell state to help with Impact damage calc and
// Fingers of Frost snapshots.
struct frost_spell_state_t : action_state_t
{
  bool impact_override;
  frost_spell_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ),
    impact_override( false )
  { }

  virtual void initialize() override
  {
    action_state_t:: initialize();
    impact_override = false;
  }

  virtual void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );
    const frost_spell_state_t* fss =
      debug_cast<const frost_spell_state_t*>( s );
    impact_override = fss -> impact_override;
  }
};

struct frost_mage_spell_t : public mage_spell_t
{
  bool chills;
  int fof_source_id;

  frost_mage_spell_t( const std::string& n, mage_t* p,
                      const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s ), chills( false ), fof_source_id( -1 )
  {}

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

  void trigger_icicle_gain( action_state_t* state, stats_t* stats )
  {
    if ( ! p() -> spec.icicles -> ok() )
      return;

    if ( ! result_is_hit( state -> result ) )
      return;

    // Icicles do not double dip on target based multipliers
    double m = state -> target_da_multiplier * state -> versatility;
    double amount = state -> result_amount / m * p() -> cache.mastery_value();
    if ( p() -> artifact.black_ice.rank() && rng().roll( 0.2 ) )
    {
      amount *= 2;
    }

    assert( as<int>( p() -> icicles.size() ) <=
            p() -> spec.icicles -> effectN( 2 ).base_value() );

    // Shoot one if capped
    if ( as<int>( p() -> icicles.size() ) ==
         p() -> spec.icicles -> effectN( 2 ).base_value() )
    {
      p() -> trigger_icicle( state );
    }

    icicle_tuple_t tuple{p()->sim->current_time(),
                         icicle_data_t{ amount, stats }};
    p() -> icicles.push_back( tuple );

    if ( p() -> sim -> debug )
    {
      p() -> sim -> out_debug.printf( "%s icicle gain, damage=%f, total=%u",
                                      p() -> name(),
                                      amount,
                                      as<unsigned>( p() -> icicles.size() ) );
    }
  }

  virtual double action_multiplier() const override
  {
    // NOTE!!: As of Legion, Icicles benefit from anything inside frost_mage_spell_t::action_multiplier()! Always check
    // for icicle interaction if something is added here, and adjust icicle_t::composite_da_multiplier() accordingly.
    double am = mage_spell_t::action_multiplier();

    // Divide effect percent by 10 to convert 5 into 0.5%, not into 5%.
    am *= 1.0 + ( p() -> buffs.bone_chilling -> current_stack * p() -> talents.bone_chilling -> effectN( 1 ).percent() / 10 );

    if ( p() -> artifact.spellborne && school == SCHOOL_FROST )
    {
      am *= 1.0 + p() -> artifact.spellborne.percent();
    }
    return am;
  }

  virtual void impact( action_state_t* state ) override
  {
    mage_spell_t::impact( state );

    if ( result_is_hit( state -> result ) && chills )
    {
      td( state -> target ) -> debuffs.chilled -> trigger();
    }
  }

  void handle_frozen( action_state_t* state )
  {
    // Handle Frozen with this, but let Ice Lance take care of itself since it needs
    // to snapshot FoF/Frozen state on execute.

    if ( td ( state -> target ) -> debuffs.winters_chill -> up() )
    {
      frozen = true;
    }
    else if ( !td ( state -> target ) -> debuffs.winters_chill -> up() &&
               state -> action -> s_data -> _id != 30455 )
    {
      frozen = false;
    }
  }
};

// Icicles ==================================================================

struct icicle_state_t : public action_state_t
{
  stats_t* source;

  icicle_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ), source( nullptr )
  { }

  void initialize() override
  { action_state_t::initialize(); source = nullptr; }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  { action_state_t::debug_str( s ) << " source=" << ( source ? source -> name_str : "unknown" ); return s; }

  void copy_state( const action_state_t* other ) override
  {
    action_state_t::copy_state( other );

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
      base_multiplier *= 1.0 + p -> talents.splitting_ice
                                 -> effectN( 3 ).percent();
      aoe = 1 + p -> talents.splitting_ice -> effectN( 1 ).base_value();
      base_aoe_multiplier *= p -> talents.splitting_ice
                               -> effectN( 2 ).percent();
    }
  }

  // To correctly record damage and execute information to the correct source
  // action (FB or FFB), we set the stats object of the icicle cast to the
  // source stats object, carried from trigger_icicle() to here through the
  // execute_event_t.
  void execute() override
  {
    const icicle_state_t* is = debug_cast<const icicle_state_t*>( pre_execute_state );
    assert( is -> source );
    stats = is -> source;

    frost_mage_spell_t::execute();
  }

  // Due to the mage targeting system, the pre_execute_state in is emptied by
  // the mage_spell_t::execute() call (before getting to action_t::execute()),
  // thus we need to "re-set" the stats object into the state object that is
  // used for the next leg of the execution path (execute() using travel event
  // to impact()). This is done in schedule_travel().
  void schedule_travel( action_state_t* state ) override
  {
    icicle_state_t* is = debug_cast<icicle_state_t*>( state );
    is -> source = stats;

    frost_mage_spell_t::schedule_travel( state );
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

    snapshot_flags &= ~( STATE_SP | STATE_CRIT | STATE_TGT_CRIT );
  }

  virtual double composite_da_multiplier( const action_state_t* ) const override
  {
    // Override this to remove composite_player_multiplier benefits (RoP/IF/CttC)
    // Also remove composute_player_dd_multipliers. Only return action and action_da multipliers.
    double am = action_multiplier();
    // Icicles shouldn't be double dipping on Bone Chilling.
    if ( p() -> buffs.bone_chilling -> up() )
    {
      am /= 1.0 + ( p() -> buffs.bone_chilling -> current_stack * p() -> talents.bone_chilling -> effectN( 1 ).percent() / 10 );

    }
    return am * action_da_multiplier();
  }
};

// Presence of Mind Spell ===================================================

struct presence_of_mind_t : public arcane_mage_spell_t
{
  presence_of_mind_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "presence_of_mind", p,
                         p -> find_talent_spell( "Presence of Mind" )  )
  {
    parse_options( options_str );
    harmful = false;
    triggers_arcane_missiles = false;
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
  }
};

// Conflagration Spell =====================================================
struct conflagration_dot_t : public fire_mage_spell_t
{
  conflagration_dot_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "conflagration_dot", p, p -> find_spell( 226757 ) )
  {
    parse_options( options_str );
    //TODO: Check callbacks
    hasted_ticks = false;
    tick_may_crit = may_crit = false;
    background = true;
    base_costs[ RESOURCE_MANA ] = 0;
    trigger_gcd = timespan_t::zero();
  }
  void init() override
  {
    fire_mage_spell_t::init();
    snapshot_flags &= ~STATE_HASTE;
    update_flags &= ~STATE_HASTE;
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
    base_costs[ RESOURCE_MANA ] = 0;
    trigger_gcd = timespan_t::zero();
  }
};

// Ignite Spell ===================================================================

//Phoenix Reborn Spell
struct phoenix_reborn_t : public fire_mage_spell_t
{
  cooldown_t* icd;
  phoenix_reborn_t( mage_t* p ) :
    fire_mage_spell_t( "phoenix_reborn", p, p -> artifact.phoenix_reborn )
  {
    spell_power_mod.direct  = p -> find_spell( 215775 ) -> effectN( 1 ).sp_coeff();
    trigger_gcd =  timespan_t::zero();
    base_costs[ RESOURCE_MANA ] = 0;
    callbacks = false;
    background = true;
    icd = p -> get_cooldown( "phoenix_reborn_icd" );
    icd -> duration = p -> find_spell( 215773 ) -> internal_cooldown();
  }
  virtual void execute() override
  {
    fire_mage_spell_t::execute();
    icd -> start();
    p() -> cooldowns.phoenixs_flames -> adjust( -1000 * p() -> artifact.phoenix_reborn.data()
                                                 .effectN( 1 ).time_value() );
  }

};

struct ignite_t : public residual_action_t
{
  conflagration_t* conflagration;
  phoenix_reborn_t* phoenix_reborn;

  ignite_t( mage_t* player ) :
    residual_action_t( "ignite", player, player -> find_spell( 12846 ) ),
    conflagration( nullptr ),phoenix_reborn( nullptr )
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
      conflagration -> target = dot -> target;
      conflagration -> execute();
    }

    if ( p() -> artifact.phoenix_reborn.rank() &&
         rng().roll( p() -> artifact.phoenix_reborn.data().proc_chance() )
         && phoenix_reborn -> icd -> up() )
    {
      phoenix_reborn -> target = dot -> target;
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
    school = SCHOOL_ARCANE;
    aoe = -1;
    trigger_gcd = timespan_t::zero();
    background = true;
    may_crit = false;
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
  arcane_rebound_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_rebound", p, p -> find_spell( 210817 ) )
  {
    parse_options( options_str );
    background = true;
    callbacks = false; // TODO: Is this true?
    aoe = -1;
    triggers_arcane_missiles = false;
  }

  virtual timespan_t travel_time() const override
  {
    // Hardcode no travel time to avoid parsed travel time in spelldata
    return timespan_t::from_seconds( 0.0 );
  }

};
struct arcane_barrage_t : public arcane_mage_spell_t
{
  arcane_rebound_t* arcane_rebound;
  double mystic_kilt_of_the_rune_master_regen;
  arcane_barrage_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_barrage", p, p -> find_class_spell( "Arcane Barrage" ) ),
    arcane_rebound( new arcane_rebound_t( p, options_str ) ),
    mystic_kilt_of_the_rune_master_regen( 0.0 )
  {
    parse_options( options_str );

    base_aoe_multiplier *= data().effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> artifact.torrential_barrage.percent();
    cooldown -> hasted = true;
  }

  virtual void execute() override
  {
    int charges = p() -> buffs.arcane_charge -> check();
    aoe = ( charges == 0 ) ? 0 : 1 + charges;

    p() -> benefits.arcane_charge.arcane_barrage -> update();

    if ( mystic_kilt_of_the_rune_master_regen > 0 &&
         p() -> buffs.arcane_charge -> check() )
    {
      p() -> resource_gain( RESOURCE_MANA, ( p() -> buffs.arcane_charge -> check() * mystic_kilt_of_the_rune_master_regen * p() -> resources.max[ RESOURCE_MANA ] ), p() -> gains.mystic_kilt_of_the_rune_master );
    }

    arcane_mage_spell_t::execute();

    p() -> buffs.arcane_charge -> expire();
    p() -> buffs.quickening -> expire();

  }

  virtual void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( p() -> artifact.arcane_rebound.rank() && ( s -> n_targets > 2 ) && ( s -> chain_target == 0 ) )
    {
      arcane_rebound -> target = s -> target;
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
  double wild_arcanist_effect;
  struct touch_of_the_magi_t
  {
    mage_t* mage;
    const spell_data_t* data;
    cooldown_t* icd;

    touch_of_the_magi_t( mage_t* p ) :
      mage( p ),
      data( p -> find_spell( 210725 ) ),
      icd( p -> get_cooldown( "touch_of_the_magi_icd" ) )
    {}

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
                         p -> find_class_spell( "Arcane Blast" ) ),
    wild_arcanist_effect( 0.0 )
  {
    parse_options( options_str );
    triggers_arcane_missiles = false; // Disable default AM proc logic.
    base_multiplier *= 1.0 + p -> artifact.blasting_rod.percent();

    if ( p -> wild_arcanist )
    {
      const spell_data_t* data = p -> wild_arcanist -> driver();
      wild_arcanist_effect = std::fabs( data -> effectN( 1 ).average( p -> wild_arcanist -> item ) );
      wild_arcanist_effect /= 100.0;
    }

    if ( p -> artifact.touch_of_the_magi.rank() )
    {
      touch_of_the_magi = new touch_of_the_magi_t( p );
    }
  }

  virtual bool init_finished() override
  {
    am_trigger_source_id = p() -> benefits.arcane_missiles
                               -> get_source_id( "Arcane Blast" );

    return arcane_mage_spell_t::init_finished();
  }

  virtual double cost() const override
  {
    double c = arcane_mage_spell_t::cost();

    c *= 1.0 +  p() -> buffs.arcane_charge -> check() *
                p() -> spec.arcane_charge -> effectN( 2 ).percent();

    if ( p() -> buffs.arcane_affinity -> check() )
    {
      c *= 1.0 + p() -> buffs.arcane_affinity -> data().effectN( 1 ).percent();
    }
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
    if ( p() -> buffs.rhonins_assaulting_armwraps -> check() )
    {
      p() -> buffs.rhonins_assaulting_armwraps -> expire();
    }

    p() -> buffs.arcane_charge -> up();
    p() -> buffs.arcane_affinity -> up();

    if ( result_is_hit( execute_state -> result ) )
    {
      trigger_am( am_trigger_source_id,
                  p() -> buffs.arcane_missiles -> proc_chance() * 2.0 );

      p() -> buffs.arcane_charge -> trigger();
      p() -> buffs.arcane_instability -> trigger();
    }

    if ( p() -> buffs.presence_of_mind -> up() )
    {
      p() -> buffs.presence_of_mind -> decrement();
      if ( p() -> buffs.presence_of_mind -> stack() == 0 )
      {
        timespan_t cd = p() -> buffs.presence_of_mind -> data().duration();
        p() -> cooldowns.presence_of_mind -> start( cd );
      }
    }

    if ( p() -> talents.quickening -> ok() &&
         p() -> buffs.quickening -> check() < p() -> buffs.quickening -> max_stack() )
    {
      p() -> buffs.quickening -> trigger();
    }
  }

  virtual double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_damage_bonus();

    if ( p() -> wild_arcanist && p() -> buffs.arcane_power -> check() )
    {
      am *= 1.0 + wild_arcanist_effect;
    }

    return am;
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buffs.presence_of_mind -> check() )
    {
      return timespan_t::zero();
    }

    timespan_t t = arcane_mage_spell_t::execute_time();

    if ( p() -> buffs.arcane_affinity -> check() )
    {
      t *= 1.0 + p() -> buffs.arcane_affinity -> data().effectN( 1 ).percent();
    }

    if ( p() -> wild_arcanist && p() -> buffs.arcane_power -> check() )
    {
      t *= 1.0 - wild_arcanist_effect;
    }

    return t;
  }

  virtual timespan_t gcd() const override
  {
    timespan_t t = arcane_mage_spell_t::gcd();

    if ( p() -> wild_arcanist && p() -> buffs.arcane_power -> check() )
    {
      // Hidden GCD cap on ToSW
      t = std::max( timespan_t::from_seconds( 1.2 ), t );
      t *= 1.0 - wild_arcanist_effect;
    }

    return t;
  }

  virtual void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talents.unstable_magic -> ok() )
      {
        trigger_unstable_magic( s );
      }

      if ( p() -> artifact.touch_of_the_magi.rank() &&
           touch_of_the_magi -> icd -> up() )
      {
        touch_of_the_magi -> trigger_if_up( s );
      }
    }
  }

};


// Arcane Explosion Spell =====================================================

struct arcane_explosion_t : public arcane_mage_spell_t
{
  arcane_explosion_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_explosion", p,
                         p -> find_class_spell( "Arcane Explosion" ) )
  {
    parse_options( options_str );
    aoe = -1;
    base_multiplier *= 1.0 + p -> artifact.arcane_purification.percent();
    radius += p -> artifact.crackling_energy.data().effectN( 1 ).base_value();
  }

  virtual void execute() override
  {
    p() -> benefits.arcane_charge.arcane_blast -> update();

    arcane_mage_spell_t::execute();

    p() -> buffs.arcane_charge -> up();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> buffs.arcane_charge -> trigger();

      p() -> buffs.arcane_instability -> trigger();
    }
    if ( p() -> talents.quickening -> ok() &&
         p() -> buffs.quickening -> check() < p() -> buffs.quickening -> max_stack() )
    {
      p() -> buffs.quickening -> trigger();
    }
  }

  virtual double cost() const override
  {
    double c = arcane_mage_spell_t::cost();

    c *= 1.0 +  p() -> buffs.arcane_charge -> check() *
                p() -> spec.arcane_charge -> effectN( 2 ).percent();

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
                         p -> find_class_spell( "Arcane Missiles" )
                           -> effectN( 2 ).trigger() )
  {
    background  = true;
    dot_duration = timespan_t::zero();

    triggers_arcane_missiles = false;
  }
};

struct am_state_t : public action_state_t
{
  bool rule_of_threes, arcane_instability;

  am_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ), rule_of_threes( false ), arcane_instability( false )
  { }

  void initialize() override
  { action_state_t::initialize(); rule_of_threes = false; arcane_instability = false; }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s )
      << " rule_of_threes=" << rule_of_threes
      << " arcane_instability=" << arcane_instability;
    return s;
  }

  void copy_state( const action_state_t* other ) override
  {
    action_state_t::copy_state( other );

    rule_of_threes = debug_cast<const am_state_t*>( other ) -> rule_of_threes;
    arcane_instability = debug_cast<const am_state_t*>( other ) -> arcane_instability;
  }
};

struct arcane_missiles_t : public arcane_mage_spell_t
{
  timespan_t temporal_hero_duration;
  double rhonins_assaulting_armwraps_proc_rate;
  double rule_of_threes_ticks, rule_of_threes_ratio;
  double arcane_instability_ticks, arcane_instability_ratio;
  arcane_missiles_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_missiles", p,
                         p -> find_class_spell( "Arcane Missiles" ) ),
    temporal_hero_duration( timespan_t::zero() ),
    rhonins_assaulting_armwraps_proc_rate( 0.0 )
  {
    parse_options( options_str );
    may_miss = false;
    triggers_arcane_missiles = false;
    dot_duration      = data().duration();
    base_tick_time    = data().effectN( 2 ).period();
    channeled         = true;
    hasted_ticks      = false;
    dynamic_tick_action = true;
    tick_action = new arcane_missiles_tick_t( p );
    may_miss = false;

    temporal_hero_duration = p -> find_spell( 188117 ) -> duration();

    base_multiplier *= 1.0 + p -> artifact.aegwynns_fury.percent();

    rule_of_threes_ticks = dot_duration / base_tick_time +
      p -> artifact.rule_of_threes.data().effectN( 2 ).base_value();
    rule_of_threes_ratio = ( dot_duration / base_tick_time ) / rule_of_threes_ticks;

    arcane_instability_ratio = 1.0 + p -> buffs.arcane_instability -> data().effectN( 1 ).percent();
    arcane_instability_ticks = dot_duration / base_tick_time / arcane_instability_ratio;
  }

  double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_damage_bonus();

    return am;
  }

  // Flag Arcane Missiles as direct damage for triggering effects
  dmg_e amount_type( const action_state_t* /* state */, bool /* periodic */ ) const override
  {
    return DMG_DIRECT;
  }

  action_state_t* new_state() override
  { return new am_state_t( this, target ); }

  // Roll (and snapshot) Rule of Threes and/or Arcane instability buffs here, they affect the whole
  // AM channel.
  void snapshot_state( action_state_t* state, dmg_e rt ) override
  {
    arcane_mage_spell_t::snapshot_state( state, rt );

    if ( rng().roll( p() -> artifact.rule_of_threes.data().effectN( 1 ).percent() / 10.0 ) )
    {
      debug_cast<am_state_t*>( state ) -> rule_of_threes = true;
    }

    if ( p() -> buffs.arcane_instability -> up() )
    {
      debug_cast<am_state_t*>( state ) -> arcane_instability = true;
    }
  }

  // If Rule of Threes or Arcane Instability is used, return the channel duration in terms of number
  // of ticks, so we prevent weird issues with rounding on duration
  timespan_t composite_dot_duration( const action_state_t* state ) const override
  {
    auto s = debug_cast<const am_state_t*>( state );

    if ( s -> rule_of_threes )
    {
      return tick_time( state ) * rule_of_threes_ticks;
    }
    else if ( s -> arcane_instability )
    {
      return tick_time( state ) * arcane_instability_ticks;
    }
    else
    {
      return arcane_mage_spell_t::composite_dot_duration( state );
    }
  }

  // Adjust tick time on Rule of Threes and Arcane Instability
  timespan_t tick_time( const action_state_t* state ) const override
  {
    auto s = debug_cast<const am_state_t*>( state );

    if ( s -> rule_of_threes )
    {
      return base_tick_time * rule_of_threes_ratio * state -> haste;
    }
    else if ( s -> arcane_instability )
    {
      return base_tick_time * arcane_instability_ratio * state -> haste;
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

    if ( rhonins_assaulting_armwraps_proc_rate > 0 && rng().roll( rhonins_assaulting_armwraps_proc_rate ) )
    {
      p() -> buffs.rhonins_assaulting_armwraps -> trigger();
    }

    p() -> buffs.arcane_instability -> expire();

    if ( p() -> buffs.arcane_power -> check() &&
         p() -> talents.overpowered -> ok() )
    {
      timespan_t extension =
        timespan_t::from_seconds( p() -> talents.overpowered
                                      -> effectN( 1 ).base_value() );

      p() -> buffs.arcane_power
          -> extend_duration( p(), extension );
    }

    if ( p() -> sets.has_set_bonus( MAGE_ARCANE, T18, B2 ) &&
         rng().roll( p() -> sets.set( MAGE_ARCANE, T18, B2 )
                         -> proc_chance() ) )
    {
      for ( pet_t* temporal_hero : p() -> pets.temporal_heroes )
      {
        if ( temporal_hero -> is_sleeping() )
        {
          temporal_hero -> summon( temporal_hero_duration );
          break;
        }
      }
    }

    p() -> buffs.arcane_missiles -> decrement();

    if ( p() -> talents.quickening -> ok() &&
         p() -> buffs.quickening -> check() < p() -> buffs.quickening -> max_stack() )
    {
      p() -> buffs.quickening -> trigger();
    }

    if ( p() -> sets.has_set_bonus( MAGE_ARCANE, T19, B4 ) )
    {
      p() -> cooldowns.evocation
          -> adjust( -1000 * p() -> sets.set( MAGE_ARCANE, T19, B4 ) -> effectN( 1 ).time_value()  );
    }
  }

  void tick ( dot_t* d ) override
  {
    arcane_mage_spell_t::tick( d );
  }
  void last_tick ( dot_t * d ) override
  {
    arcane_mage_spell_t::last_tick( d );

    p() -> buffs.arcane_charge -> trigger();

    p() -> buffs.arcane_instability -> trigger();
  }

  bool ready() override
  {
    if ( ! p() -> buffs.arcane_missiles -> check() )
      return false;

    return arcane_mage_spell_t::ready();
  }
};


// Arcane Orb Spell =========================================================

struct arcane_orb_bolt_t : public arcane_mage_spell_t
{
  int ao_impact_am_source_id;

  arcane_orb_bolt_t( mage_t* p ) :
    arcane_mage_spell_t( "arcane_orb_bolt", p, p -> find_spell( 153640 ) )
  {
    aoe = -1;
    background = true;
    dual = true;
    cooldown -> duration = timespan_t::zero(); // dbc has CD of 15 seconds

  }

  virtual bool init_finished() override
  {
    ao_impact_am_source_id = p() -> benefits.arcane_missiles
                                 -> get_source_id( "Arcane Orb Impact" );

    return arcane_mage_spell_t::init_finished();
  }

  virtual void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> buffs.arcane_charge -> trigger();
      trigger_am( ao_impact_am_source_id );

      p() -> buffs.arcane_instability -> trigger();
    }
  }
};

struct arcane_orb_t : public arcane_mage_spell_t
{
  arcane_orb_bolt_t* orb_bolt;

  arcane_orb_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_orb", p,
                         p -> find_talent_spell( "Arcane Orb" ) ),
    orb_bolt( new arcane_orb_bolt_t( p ) )
  {
    parse_options( options_str );

    may_miss       = false;
    may_crit       = false;
    add_child( orb_bolt );
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();
    p() -> buffs.arcane_charge -> trigger();

    p() -> buffs.arcane_instability -> trigger();
  }


  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( ( player -> current.distance - 10.0 ) /
                                     16.0 );
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
                         p -> find_class_spell( "Arcane Power" ) )
  {
    parse_options( options_str );
    harmful = false;
    triggers_arcane_missiles = false;
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();
    p() -> buffs.arcane_power -> trigger( 1, data().effectN( 1 ).percent() );
  }
};

// Blast Furance Spell =======================================================
struct blast_furance_t : public fire_mage_spell_t
{
  blast_furance_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "blast_furance", p, p -> find_spell( 194522 ) )
  {
    parse_options( options_str );
    background = true;
    callbacks = false;
    hasted_ticks = false;
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

    double bw_mult = 1.0 + p -> talents.blast_wave -> effectN( 1 ).percent();
    base_multiplier *= bw_mult;
    base_aoe_multiplier = 1.0 / bw_mult;
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
    spell_power_mod.direct *= 1.0 + p -> talents.arctic_gale -> effectN( 1 ).percent();
    chills = true;
  }

  virtual bool init_finished() override
  {
    fof_source_id = p() -> benefits.fingers_of_frost
                        -> get_source_id( data().name_cstr() );

    return frost_mage_spell_t::init_finished();
  }

  // Override damage type because Blizzard is considered a DOT
  dmg_e amount_type( const action_state_t* /* state */, bool /* periodic */ ) const override
  {
    return DMG_OVER_TIME;
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      double fof_proc_chance = p() -> spec.fingers_of_frost
                                   -> effectN( 2 ).percent();
      trigger_fof( fof_source_id , fof_proc_chance );
    }
  }
  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  { return frost_mage_spell_t::snapshot_state( s, rt ); }

  virtual double calculate_direct_amount( action_state_t* s ) const override
  {
    frost_mage_spell_t::calculate_direct_amount( s );

    //TODO: This should *probably* by an action_multiplier?
    if ( p() -> legendary.zannesu_journey )
    {
      s -> result_total *= 1.0 + p() -> legendary.zannesu_journey_multiplier;
    }
    return s -> result_total;
  }
};

struct blizzard_t : public frost_mage_spell_t
{
  blizzard_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "blizzard", p, p -> find_spell( 190356 ) )
  {
    parse_options( options_str );
    may_miss     = false;
    ignore_false_positive = true;
    snapshot_flags = STATE_HASTE;
    dot_duration = data().duration();
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks = true;
    cooldown -> hasted = true;

    tick_action = new blizzard_shard_t( p );
  }
  virtual double composite_crit_chance() const override
  {
    double c = frost_mage_spell_t::composite_crit_chance();
    if ( p() -> artifact.the_storm_rages.rank() )
    {
      c+= p() -> artifact.the_storm_rages.percent();
    }
    return c;
  }
  virtual void execute() override
  {
    // "Snapshot" multiplier before we execute blizzard
    if ( p() -> legendary.zannesu_journey && p() -> buffs.zannesu_journey -> check() )
    {
      p() -> legendary.zannesu_journey_multiplier = p() -> buffs.zannesu_journey -> data().effectN( 1 ).percent() * p() -> buffs.zannesu_journey -> check();
    }

    frost_mage_spell_t::execute();

    p() -> buffs.zannesu_journey -> expire();

  }
  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t duration = frost_mage_spell_t::composite_dot_duration( s );
    return duration * ( tick_time( s ) / base_tick_time );
  }

  virtual void tick( dot_t* d ) override
  {
    frost_mage_spell_t::tick( d );
    handle_frozen( d -> state );
  }
  virtual void last_tick( dot_t* d ) override
  {
    frost_mage_spell_t::last_tick( d );

  }
};

// Charged Up Spell =========================================================

struct charged_up_t : public arcane_mage_spell_t
{
  charged_up_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "charged_up", p, p -> find_spell ("Charged Up" ) )
  {
    parse_options( options_str );
    triggers_arcane_missiles = false;
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();

    p() -> buffs.arcane_charge -> trigger( 4.0 );
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
      cinder_count = p() -> global_cinder_count;
    }
    fire_mage_spell_t::execute();

    double target_dist = player -> current.distance;
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
        rng().range( cinder_velocity_mean - cinder_converge_range,
                     cinder_converge_mean + cinder_converge_range );

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


// Combustion Spell ===========================================================

struct combustion_t : public fire_mage_spell_t
{
  combustion_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "combustion", p, p -> find_class_spell( "Combustion" ) )
  {
    //TODO: Re-enable once spelldata parses correctly, and stops building the old DoT.
    parse_options( options_str );
    //TODO: Should this have callbacks?
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
  comet_storm_projectile_t( mage_t* p) :
    frost_mage_spell_t( "comet_storm_projectile", p,
                        p -> find_spell( 153596 ) )
  {
    aoe = -1;
    background = true;
    school = SCHOOL_FROST;
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.0 );
  }

  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  { return frost_mage_spell_t::snapshot_state( s, rt ); }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

    if ( fss -> impact_override == true )
    {
      return frost_mage_spell_t::calculate_direct_amount( s );
    }
    else
    {
      return s -> result_amount;
    }
  }


  virtual result_e calculate_result( action_state_t* s ) const override
  {
     frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

     if ( fss -> impact_override == true )
     {
       return frost_mage_spell_t::calculate_result( s );
     }
     else
     {
       return s -> result;
     }
  }

  virtual void impact( action_state_t* s ) override
  {
    // Swap our flag to allow damage calculation again
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );
    fss -> impact_override = true;

    // Re-call functions here, before the impact call to do the damage calculations as we impact.
    handle_frozen( s );
    snapshot_state( s, amount_type ( s ) );

    s -> result = calculate_result( s );
    s -> result_amount = calculate_direct_amount( s );

    frost_mage_spell_t::impact( s );
  }
  virtual action_state_t* new_state() override
  {
    return new frost_spell_state_t( this, target );
  }
};

struct comet_storm_t : public frost_mage_spell_t
{
  comet_storm_projectile_t* projectile;

  comet_storm_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "comet_storm", p, p -> talents.comet_storm ),
    projectile( new comet_storm_projectile_t( p ) )
  {
    parse_options( options_str );

    may_miss = false;

    base_tick_time    = timespan_t::from_seconds( 0.2 );
    dot_duration      = timespan_t::from_seconds( 1.2 );
    hasted_ticks      = false;

    dynamic_tick_action = true;
    add_child( projectile );
  }


  virtual void execute() override
  {
    frost_mage_spell_t::execute();
    projectile -> execute();
  }

  virtual void impact( action_state_t* s ) override
  {
    // Swap our flag to allow damage calculation again
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );
    fss -> impact_override = true;

    // Re-call functions here, before the impact call to do the damage calculations as we impact.
    snapshot_state( s, amount_type ( s ) );
    s -> result = calculate_result( s );
    s -> result_amount = calculate_direct_amount( s );

    frost_mage_spell_t::impact( s );

  }
  void tick( dot_t* d ) override
  {
    frost_mage_spell_t::tick( d );
    projectile -> execute();
  }
  virtual action_state_t* new_state() override
  {
    return new frost_spell_state_t( this, target );
  }
};


// Cone of Cold Spell =======================================================

struct cone_of_cold_t : public frost_mage_spell_t
{
  cone_of_cold_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "cone_of_cold", p,
                        p -> find_class_spell( "Cone of Cold" ) )
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

  virtual bool ready() override
  {
    if ( ! target -> debuffs.casting -> check() )
    {
      return false;
    }

    return mage_spell_t::ready();
  }
};

// Dragon's Breath Spell ====================================================

struct dragons_breath_t : public fire_mage_spell_t
{
  double  darcklis_dragonfire_diadem_multiplier;
  dragons_breath_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "dragons_breath", p,
                       p -> find_class_spell( "Dragon's Breath" ) ),
                       darcklis_dragonfire_diadem_multiplier( 0.0 )
  {
    parse_options( options_str );
    aoe = -1;
    triggers_pyretic_incantation = true;
  }

  virtual void execute() override
  {
    fire_mage_spell_t::execute();
    if ( p() -> legendary.sephuzs_secret )
    {
      p() -> buffs.sephuzs_secret -> trigger();
    }
  }
  virtual double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    if ( darcklis_dragonfire_diadem_multiplier > 0 )
    {
      am *= 1.0 + darcklis_dragonfire_diadem_multiplier;
    }
    return am;
  }

};

// Ebonbolt Spell ===========================================================
struct ebonbolt_t : public frost_mage_spell_t
{
  ebonbolt_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "ebonbolt", p, p -> artifact.ebonbolt )
  {
    parse_options( options_str );
    if ( !p -> artifact.ebonbolt.rank() )
    {
      background=true;
    }

    spell_power_mod.direct = p -> find_spell( 228599 ) -> effectN( 1 ).sp_coeff();
    // Doesn't apply chill debuff but benefits from Bone Chilling somehow
  }

  virtual bool init_finished() override
  {
    fof_source_id = p() -> benefits.fingers_of_frost
                        -> get_source_id( data().name_cstr() );

    return frost_mage_spell_t::init_finished();
  }

  virtual action_state_t* new_state() override
  {
    return new frost_spell_state_t( this, target );
  }

  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  { return frost_mage_spell_t::snapshot_state( s, rt ); }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

    if ( fss -> impact_override == true )
    {
      return frost_mage_spell_t::calculate_direct_amount( s );
    }
    else
    {
      return s -> result_amount;
    }
  }


  virtual result_e calculate_result( action_state_t* s ) const override
  {
     frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

     if ( fss -> impact_override == true )
     {
       return frost_mage_spell_t::calculate_result( s );
     }
     else
     {
       return s -> result;
     }
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();
    trigger_fof( fof_source_id, 1.0, 2 );
  }

  virtual void impact( action_state_t* s ) override
  {
    // Swap our flag to allow damage calculation again
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );
    fss -> impact_override = true;

    // Re-call functions here, before the impact call to do the damage calculations as we impact.
    snapshot_state( s, amount_type ( s ) );
    s -> result = calculate_result( s );
    s -> result_amount = calculate_direct_amount( s );

    frost_mage_spell_t::impact( s );

  }
};
// Evocation Spell ==========================================================

struct evocation_t : public arcane_mage_spell_t
{
  aegwynns_ascendance_t* aegwynns_ascendance;
  double mana_gained;

  evocation_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "evocation", p,
                         p -> find_class_spell( "Evocation" ) ),
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
    triggers_arcane_missiles = false;

    cooldown = p -> cooldowns.evocation;
    cooldown -> duration += p -> spec.evocation_2 -> effectN( 1 ).time_value();

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

    if ( p() -> sets.has_set_bonus( MAGE_ARCANE, T17, B2 ) )
    {
      p() -> buffs.arcane_affinity -> trigger();
    }

    if ( p() -> artifact.aegwynns_ascendance.rank() )
    {
      double explosion_amount = mana_gained *
                                p() -> artifact.aegwynns_ascendance.percent();
      aegwynns_ascendance -> target = d -> target;
      aegwynns_ascendance -> base_dd_max = explosion_amount;
      aegwynns_ascendance -> base_dd_min = explosion_amount;
      aegwynns_ascendance -> execute();
    }
  }
};

// Fireball Spell ===========================================================

struct fireball_t : public fire_mage_spell_t
{
  conflagration_dot_t* conflagration_dot;

  fireball_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "fireball", p, p -> find_class_spell( "Fireball" ) ),
    conflagration_dot( new conflagration_dot_t( p, options_str ) )
  {
    parse_options( options_str );
    triggers_pyretic_incantation = true;
    triggers_hot_streak = true;
    triggers_ignite = true;
    base_multiplier *= 1.0 + p -> artifact.great_balls_of_fire.percent();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t cast_time = fire_mage_spell_t::execute_time();

    if ( p() -> artifact.fire_at_will.rank() )
    {
      cast_time *= 1.0 + p() -> artifact.fire_at_will.percent();
    }

    return cast_time;
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t t = fire_mage_spell_t::travel_time();
    return std::min( timespan_t::from_seconds( 0.75 ), t );
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
        conflagration_dot -> target = s -> target;
        conflagration_dot -> execute();
      }
    }

    if ( result_is_hit( s -> result) )
    {
      if ( p() -> talents.unstable_magic -> ok() )
      {
        trigger_unstable_magic( s );
      }
    }
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = fire_mage_spell_t::composite_target_crit_chance( target );

    // Fire PvP 4pc set bonus
    if ( td( target ) -> debuffs.firestarter -> check() )
    {
      c += td( target ) -> debuffs.firestarter
                        -> data().effectN( 1 ).percent();
    }
    if( p() -> talents.fire_starter -> ok() && ( target -> health_percentage() >
        p() -> talents.fire_starter -> effectN( 1 ).base_value() ) )
    {
      c = 1.0;
    }
    return c;
  }

  virtual double composite_crit_chance() const override
  {
    double c = fire_mage_spell_t::composite_crit_chance();

      c += p() -> buffs.enhanced_pyrotechnics -> stack() *
           p() -> buffs.enhanced_pyrotechnics -> data().effectN( 1 ).percent();

      if ( p() -> sets.has_set_bonus( MAGE_FIRE, T19, B2 ) )
      {
        c += p() -> buffs.enhanced_pyrotechnics -> stack() *
             p() -> sets.set( MAGE_FIRE, T19, B2 ) -> effectN( 1 ).percent();
      }
    return c;
  }


  virtual double composite_crit_chance_multiplier() const override
  {
    double m = fire_mage_spell_t::composite_crit_chance_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }


};

// Flame Patch Spell ==========================================================

struct flame_patch_t : public fire_mage_spell_t
{
  flame_patch_t( mage_t* p ) :
    fire_mage_spell_t( "flame_patch", p, p -> talents.flame_patch )
  {
    hasted_ticks=true;
    spell_power_mod.direct = p -> find_spell( 205472 ) -> effectN( 1 ).sp_coeff();
    aoe = -1;
    background = true;
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

//TODO: This needs to have an execute time of 0.75s, not 2s as spelldata suggests.
struct aftershocks_t : public fire_mage_spell_t
{
  aftershocks_t( mage_t* p ) :
    fire_mage_spell_t( "aftershocks", p, p -> find_spell( 194432 ) )
  {
    background = true;
    aoe = -1;
    triggers_ignite = true;
  }
};

struct flamestrike_t : public fire_mage_spell_t
{
  aftershocks_t* aftershocks;
  flame_patch_t* flame_patch;

  flamestrike_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "flamestrike", p,
                       p -> find_specialization_spell( "Flamestrike" ) ),
                       flame_patch( new flame_patch_t( p ) )
  {
    parse_options( options_str );

    base_multiplier *= 1.0 + p -> artifact.blue_flame_special.percent();
    triggers_ignite = true;
    triggers_pyretic_incantation = true;
    aoe = -1;

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
  }

  virtual void impact( action_state_t* state ) override
  {
    fire_mage_spell_t::impact( state );

    if ( state -> chain_target == 0 && p() -> artifact.aftershocks.rank() )
    {
      aftershocks -> schedule_execute();
    }
    if ( state -> chain_target == 0 && p() -> talents.flame_patch -> ok() )
    {
      // DurationID: 205470. 8s
      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .pulse_time( timespan_t::from_seconds( 1.0 ) )
        .target( execute_state -> target )
        .duration( timespan_t::from_seconds( 8.0 ) )
        .action( flame_patch )
        .hasted( ground_aoe_params_t::SPELL_HASTE ), true );
    }
  }

  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  {
    fire_mage_spell_t::snapshot_state( s, rt );

    ignite_spell_state_t* is = debug_cast<ignite_spell_state_t*>( s );
    is -> hot_streak = ( p() -> buffs.hot_streak -> check() != 0 );
  }

   double composite_ignite_multiplier( const action_state_t* s ) const override
  {
   const ignite_spell_state_t* is = debug_cast<const ignite_spell_state_t*>( s );

    if ( is -> hot_streak )
    {
      return 2.0;
    }

    return 1.0;
  }
};

// Pyrosurge Flamestrike Spell ==========================================================

struct pyrosurge_flamestrike_t : public fire_mage_spell_t
{
  aftershocks_t* aftershocks;
  flame_patch_t* flame_patch;

  pyrosurge_flamestrike_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "pyrosurge_flamestrike", p,
                       p -> find_specialization_spell( "Flamestrike" ) ),
                       flame_patch( new flame_patch_t( p ) )
  {
    parse_options( options_str );
    base_costs[ RESOURCE_MANA ] = 0;
    base_multiplier *= 1.0 + p -> artifact.blue_flame_special.percent();
    background = true;
    triggers_ignite = true;
    triggers_hot_streak = false;
    triggers_pyretic_incantation = true;
    aoe = -1;

    if ( p -> artifact.aftershocks.rank() )
    {
      aftershocks = new aftershocks_t( p );
    }
  }

  virtual action_state_t* new_state() override
  {
    return new ignite_spell_state_t( this, target );
  }

  virtual void impact( action_state_t* state ) override
  {
    fire_mage_spell_t::impact( state );

    if ( state -> chain_target == 0 && p() -> artifact.aftershocks.rank() )
    {
      aftershocks -> schedule_execute();
    }

    if ( state -> chain_target == 0 && p() -> talents.flame_patch -> ok() )
    {
      // DurationID: 205470. 8s
      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .pulse_time( timespan_t::from_seconds( 1.0 ) )
        .target( execute_state -> target )
        .duration( timespan_t::from_seconds( 8.0 ) )
        .action( flame_patch )
        .hasted( ground_aoe_params_t::SPELL_HASTE ), true );
    }
  }
};
// Flame On Spell =============================================================

struct flame_on_t : public fire_mage_spell_t
{
  flame_on_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "flame_on", p, p -> talents.flame_on )
  {
    parse_options( options_str );
  }

  virtual void execute() override
  {
    fire_mage_spell_t::execute();

    // TODO: Change reset() to accept # of charges as parameter?
    for ( int i = data().effectN( 1 ).base_value(); i > 0; i-- )
    {
      p() -> cooldowns.fire_blast -> reset( false );
    }
  }
};

// Flurry Spell ===============================================================

struct flurry_bolt_t : public frost_mage_spell_t
{
  bool brain_freeze_buffed = false;
  flurry_bolt_t( mage_t* p ) :
    frost_mage_spell_t( "flurry_bolt", p, p -> find_spell( 228354 ) ),
    brain_freeze_buffed( false )
  {
    chills = true;
    if ( p -> talents.lonely_winter -> ok() )
    {
      base_multiplier *= 1.0 + p -> talents.lonely_winter -> effectN( 1 ).percent();
    }
  }
  virtual action_state_t* new_state() override
  {
    return new frost_spell_state_t( this, target );
  }

  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  { return frost_mage_spell_t::snapshot_state( s, rt ); }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

    if ( fss -> impact_override == true )
    {
      return frost_mage_spell_t::calculate_direct_amount( s );
    }
    else
    {
      return s -> result_amount;
    }
  }


  virtual result_e calculate_result( action_state_t* s ) const override
  {
     frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

     if ( fss -> impact_override == true )
     {
       return frost_mage_spell_t::calculate_result( s );
     }
     else
     {
       return s -> result;
     }
  }

  virtual void impact( action_state_t* s ) override
  {
    // Swap our flag to allow damage calculation again
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );
    fss -> impact_override = true;

    // Re-call functions here, before the impact call to do the damage calculations as we impact.
    handle_frozen( s );
    snapshot_state( s, amount_type ( s ) );

    s -> result = calculate_result( s );
    s -> result_amount = calculate_direct_amount( s );

    frost_mage_spell_t::impact( s );

    td( s -> target ) -> debuffs.winters_chill -> trigger();
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

  }
  virtual double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    if ( p() -> artifact.ice_age.rank() )
    {
      am *= 1.0 + p() -> artifact.ice_age.percent();
    }

    return am;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = frost_mage_spell_t::composite_persistent_multiplier( state );

    if( brain_freeze_buffed == true )
    {
      m *= 1.0 + p() -> buffs.brain_freeze -> data().effectN( 2 ).percent();
    }

    return m;
  }

};
struct flurry_t : public frost_mage_spell_t
{
  flurry_bolt_t* flurry_bolt;
  flurry_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "flurry", p, p -> find_spell( 44614 ) ),
    flurry_bolt( new flurry_bolt_t( p ) )
  {
    parse_options( options_str );
    hasted_ticks = false;

    //TODO: Remove hardcoded values once it exists in spell data for bolt impact timing.
    dot_duration = timespan_t::from_seconds( 0.03 );
    base_tick_time = timespan_t::from_seconds( 0.01 );

  }

  virtual timespan_t travel_time() const override
  {
    // Approximate travel time from in game data.
    // TODO: Improve approximation
    return timespan_t::from_seconds( ( player -> current.distance / 38 ) );
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

    if ( p() -> legendary.zannesu_journey == true )
    {
      p() -> buffs.zannesu_journey -> trigger();
    }
    if ( p() -> buffs.brain_freeze  -> check() )
    {
      flurry_bolt -> brain_freeze_buffed = true;
    }
    else
    {
      flurry_bolt -> brain_freeze_buffed = false;
    }
    p() -> buffs.brain_freeze -> expire();
  }

  void tick( dot_t* d ) override
  {
    frost_mage_spell_t::tick( d );
    flurry_bolt -> target = d -> target;
    flurry_bolt -> execute();
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
  virtual action_state_t* new_state() override
  {
    return new frost_spell_state_t( this, target );
  }

  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  { return frost_mage_spell_t::snapshot_state( s, rt ); }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

    if ( fss -> impact_override == true )
    {
      return frost_mage_spell_t::calculate_direct_amount( s );
    }
    else
    {
      return s -> result_amount;
    }
  }


  virtual result_e calculate_result( action_state_t* s ) const override
  {
     frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

     if ( fss -> impact_override == true )
     {
       return frost_mage_spell_t::calculate_result( s );
     }
     else
     {
       return s -> result;
     }
  }

  virtual void impact( action_state_t* s ) override
  {
    // Swap our flag to allow damage calculation again
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );
    fss -> impact_override = true;

    // Re-call functions here, before the impact call to do the damage calculations as we impact.
    handle_frozen( s );
    snapshot_state( s, amount_type ( s ) );

    s -> result = calculate_result( s );
    s -> result_amount = calculate_direct_amount( s );

    frost_mage_spell_t::impact( s );
  }

  virtual resource_e current_resource() const override
  { return RESOURCE_NONE; }

  virtual timespan_t travel_time() const override
  { return timespan_t::zero(); }
};

struct frost_bomb_t : public frost_mage_spell_t
{
  frost_bomb_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "frost_bomb", p, p -> talents.frost_bomb )
  {
    parse_options( options_str );
  }

  virtual void execute() override
  {

    frost_mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> last_bomb_target != nullptr &&
           p() -> last_bomb_target != execute_state -> target )
      {
        td( p() -> last_bomb_target ) -> dots.frost_bomb -> cancel();
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
  // clumping FB/FFB icicle damage together in reports.
  stats_t* icicle;

  int water_jet_fof_source_id;

  frostbolt_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "frostbolt", p,
                        p -> find_specialization_spell( "Frostbolt" ) ),
    icicle( p -> get_stats( "icicle" ) )
  {
    parse_options( options_str );
    spell_power_mod.direct = p -> find_spell( 228597 ) -> effectN( 1 ).sp_coeff();
    if ( p -> spec.icicles -> ok() )
    {
      stats -> add_child( icicle );
      icicle -> school = school;
      assert( p -> icicle );
      icicle -> action_list.push_back( p -> icicle );
    }
    if ( p -> talents.lonely_winter -> ok() )
    {
      base_multiplier *= 1.0 + ( p -> talents.lonely_winter -> effectN( 1 ).percent() +
                               p -> artifact.its_cold_outside.data().effectN( 2 ).percent() );
    }
    base_multiplier *= 1.0 + p -> artifact.icy_caress.percent();
    chills = true;
  }

  virtual bool init_finished() override
  {
    fof_source_id = p() -> benefits.fingers_of_frost
                        -> get_source_id( data().name_cstr() );
    water_jet_fof_source_id = p() -> benefits.fingers_of_frost
                                  -> get_source_id( "Water Jet" );

    return frost_mage_spell_t::init_finished();
  }

  virtual action_state_t* new_state() override
  {
    return new frost_spell_state_t( this, target );
  }

  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  { return frost_mage_spell_t::snapshot_state( s, rt ); }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

    if ( fss -> impact_override == true )
    {
      return frost_mage_spell_t::calculate_direct_amount( s );
    }
    else
    {
      return s -> result_amount;
    }
  }

  virtual result_e calculate_result( action_state_t* s ) const override
  {
     frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

     if ( fss -> impact_override == true )
     {
       return frost_mage_spell_t::calculate_result( s );
     }
     else
     {
       return s -> result;
     }

  }

  virtual timespan_t execute_time() const override
  {
    timespan_t cast = frost_mage_spell_t::execute_time();

    // 2T17 bonus
    if ( p() -> buffs.ice_shard -> check() )
    {
      cast *= 1.0 + ( p() -> buffs.ice_shard -> stack() *
                      p() -> buffs.ice_shard
                          -> data().effectN( 1 ).percent() );
    }

    return cast;
  }

  virtual void execute() override
  {
    p() -> buffs.ice_shard -> up();

    frost_mage_spell_t::execute();

    p() -> buffs.icicles -> trigger();

    if ( result_is_hit( execute_state -> result ) )
    {
      double fof_proc_chance = p() -> spec.fingers_of_frost
                                   -> effectN( 1 ).percent();
      double bf_proc_chance = p() -> spec.brain_freeze
                                  -> effectN( 1 ).percent();

      if ( p() -> sets.has_set_bonus( MAGE_FROST, T19, B2 ) )
      {
        bf_proc_chance += p() -> sets.set( MAGE_FROST, T19, B2 )
                              -> effectN( 1 ).percent();
      }
      if ( p() -> artifact.clarity_of_thought.rank() )
      {
        bf_proc_chance += p() -> artifact.clarity_of_thought.percent();
      }

      if( rng().roll( bf_proc_chance ) )
      {
        p() -> buffs.brain_freeze -> trigger();
      }

      trigger_fof( fof_source_id, fof_proc_chance );

      if ( p() -> legendary.shatterlance == true )
      {
        p() -> buffs.shatterlance -> trigger();
      }
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    // Swap our flag to allow damage calculation again
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );
    fss -> impact_override = true;

    // Re-call functions here, before the impact call to do the damage calculations as we impact.
    handle_frozen( s );
    snapshot_state( s, amount_type ( s ) );

    s -> result = calculate_result( s );
    s -> result_amount = calculate_direct_amount( s );

    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talents.unstable_magic -> ok() )
      {
        trigger_unstable_magic( s );
      }

      trigger_icicle_gain( s, icicle );

      if (  p() -> pets.water_elemental && !p() -> pets.water_elemental -> is_sleeping() )
      {
        auto we_td =
          p() -> pets.water_elemental
          -> get_target_data( execute_state -> target );

        if ( we_td -> water_jet -> up() )
        {
          trigger_fof( water_jet_fof_source_id, 1.0 );
        }
      }

      //TODO: Fix hardcode once spelldata has value for proc rate.
      if ( p() -> artifact.ice_nine.rank() &&
           rng().roll( 0.2 ) )
      {
        trigger_icicle_gain( s, icicle );
        p() -> buffs.icicles -> trigger();
      }
      if ( s -> result == RESULT_CRIT && p() -> artifact.frozen_veins.rank() )
      {
        p() -> cooldowns.icy_veins -> adjust( p() -> artifact.frozen_veins.time_value() );
        p() -> buffs.icy_veins -> cooldown -> adjust( p() -> artifact.frozen_veins.time_value() );
      }

      if ( s -> result == RESULT_CRIT && p() -> artifact.chain_reaction.rank() )
      {
        p() -> buffs.chain_reaction -> trigger();
      }
    }
  }

  virtual double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    if ( p() -> buffs.ice_shard -> check() )
    {
      am *= 1.0 + ( p() -> buffs.ice_shard -> stack() * p() -> buffs.ice_shard -> data().effectN( 2 ).percent() );
    }
    return am;
  }

  virtual double composite_crit_chance() const override
  {
    double c = frost_mage_spell_t::composite_crit_chance();

    if ( p() -> artifact.shattering_bolts.rank() )
    {
      c+= p() -> artifact.shattering_bolts.percent();
    }
    return c;
  }
};

// Frost Nova Spell ========================================================

struct frost_nova_t : public mage_spell_t
{
  frost_nova_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frost_nova", p, p -> find_class_spell( "Frost Nova" ) )
  {
    parse_options( options_str );
    triggers_arcane_missiles = true;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( p() -> legendary.sephuzs_secret )
    {
      p() -> buffs.sephuzs_secret -> trigger();
    }
  }
};
// Frozen Orb Spell =========================================================

struct frozen_orb_bolt_t : public frost_mage_spell_t
{
  frozen_orb_bolt_t( mage_t* p ) :
    frost_mage_spell_t( "frozen_orb_bolt", p,
                        p -> find_class_spell( "Frozen Orb" ) -> ok() ?
                          p -> find_spell( 84721 ) :
                          spell_data_t::not_found() )
  {
    aoe = -1;
    background = true;
    dual = true;
    cooldown -> duration = timespan_t::zero(); // dbc has CD of 6 seconds

    //TODO: Is this actually how these modifiers work?
    if ( p -> talents.lonely_winter -> ok() )
    {
      base_multiplier *= 1.0 + ( p -> talents.lonely_winter -> effectN( 1 ).percent() +
                               p -> artifact.its_cold_outside.data().effectN( 2 ).percent() );
    }
    crit_bonus_multiplier *= 1.0 + p -> artifact.orbital_strike.percent();
    chills = true;
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
    if ( result_is_hit( execute_state -> result ) )
    {
      double fof_proc_chance = p() -> spec.fingers_of_frost
                                   -> effectN( 1 ).percent();

      if ( p() -> sets.has_set_bonus( MAGE_FROST, T19, B4 ) )
      {
        fof_proc_chance += p() -> sets.set( MAGE_FROST, T19, B4 ) -> effectN( 1 ).percent();
      }
      trigger_fof( fof_source_id, fof_proc_chance );
    }
  }

  // Override damage type because Frozen Orb is considered a DOT
  dmg_e amount_type( const action_state_t* /* state */, bool /* periodic */ ) const override
  {
    return DMG_OVER_TIME;
  }

};

struct frozen_orb_t : public frost_mage_spell_t
{
  //TODO: Redo how frozen_orb_bolt is set up to take base_multipler from parent.
  frozen_orb_bolt_t* frozen_orb_bolt;
  frozen_orb_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "frozen_orb", p,
                        p -> find_class_spell( "Frozen Orb" ) ),
    frozen_orb_bolt( new frozen_orb_bolt_t( p ) )
  {
    parse_options( options_str );
    hasted_ticks = false;
    base_tick_time    = timespan_t::from_seconds( 0.5 );
    dot_duration      = timespan_t::from_seconds( 10.0 );
    add_child( frozen_orb_bolt );
    may_miss       = false;
    may_crit       = false;
    travel_speed = 20;
  }

  virtual bool init_finished() override
  {
    fof_source_id = p() -> benefits.fingers_of_frost
                        -> get_source_id( "Frozen Orb Initial Impact" );

    return frost_mage_spell_t::init_finished();
  }

  void tick( dot_t* d ) override
  {
    frost_mage_spell_t::tick( d );
    // "travel time" reduction of ticks based on distance from target - set on the side of less ticks lost.
    //TODO: Update/Check this for legion - does it still lose ticks on travel?
    frozen_orb_bolt -> target = d -> target;
    frozen_orb_bolt -> execute();
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    if ( p() -> sets.has_set_bonus( MAGE_FROST, T17, B4 ) )
    {
      p() -> buffs.frost_t17_4pc -> trigger();
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_fof( fof_source_id, 1.0 );
    }
  }
};


// Frozen Touch Spell =========================================================

struct frozen_touch_t : public frost_mage_spell_t
{
  frozen_touch_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t("frozen_touch", p, p -> talents.frozen_touch )
  {
    parse_options( options_str );
  }

  virtual bool init_finished() override
  {
    fof_source_id = p() -> benefits.fingers_of_frost
                        -> get_source_id( data().name_cstr() );

    return frost_mage_spell_t::init_finished();
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    trigger_fof( fof_source_id, 1.0, 2 );
  }
};


// Glacial Spike Spell ==============================================================

struct glacial_spike_t : public frost_mage_spell_t
{
  double stored_icicle_value;
  glacial_spike_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "glacial_spike", p, p -> talents.glacial_spike ),
    stored_icicle_value( 0 )
  {
    parse_options( options_str );
    spell_power_mod.direct = p -> find_spell( 228600 ) -> effectN( 1 ).sp_coeff();
    //FIXME: Figure out a better way to do this than a fake cooldown
    cooldown -> duration = timespan_t::from_seconds( 1.5 );
    if ( p -> talents.splitting_ice -> ok() )
    {
      base_multiplier *= 1.0 + p -> talents.splitting_ice
                                 -> effectN( 3 ).percent();
      aoe = 1 + p -> talents.splitting_ice -> effectN( 1 ).base_value();
      base_aoe_multiplier *= p -> talents.splitting_ice
                               -> effectN( 2 ).percent();
    }
  }

  virtual bool ready() override
  {
    if ( p() -> buffs.icicles -> current_stack < p() -> buffs.icicles -> max_stack() )
    {
      return false;
    }
    return frost_mage_spell_t::ready();

  }

  virtual void update_ready( timespan_t cd ) override
  {
    frost_mage_spell_t::update_ready( cd );
  }
  virtual action_state_t* new_state() override
  {
    return new frost_spell_state_t( this, target );
  }

  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  { return frost_mage_spell_t::snapshot_state( s, rt ); }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

    if ( fss -> impact_override == true )
    {
      return frost_mage_spell_t::calculate_direct_amount( s );
    }
    else
    {
      return s -> result_amount;
    }
  }


  virtual result_e calculate_result( action_state_t* s ) const override
  {
     frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

     if ( fss -> impact_override == true )
     {
       return frost_mage_spell_t::calculate_result( s );
     }
     else
     {
       return s -> result;
     }
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );
    double icicle_damage_sum = 0;
    int icicle_count = as<int>( p() -> icicles.size() );
    assert( icicle_count == p() -> spec.icicles -> effectN( 2 ).base_value() );
    for ( int i = 0; i < icicle_count; i++ )
    {
      icicle_data_t d = p() -> get_icicle_object();
      icicle_damage_sum += d.damage;
    }

    if ( sim -> debug )
    {
      sim -> out_debug.printf("Add %u icicles to glacial_spike for %f damage",
                              icicle_count, icicle_damage_sum);
    }
    // Note: This needs to be manually added to icicle values for splitting ice, as normally it's a base multiplier.
    if ( p() -> talents.splitting_ice -> ok() )
    {
      icicle_damage_sum *= 1.0 + p() -> talents.splitting_ice -> effectN( 3 ).percent();
    }

    // If we're dealing with the first target when using splitting ice, store the total icicle value.
    if ( s -> n_targets > 1 && s -> chain_target == 0 )
    {
      stored_icicle_value = icicle_damage_sum;
    }

    // If we're dealing with the non-primary target, no icicles exist.
    // So grab the istored icicle value
    if ( s -> chain_target != 0 )
    {
      icicle_damage_sum = stored_icicle_value;
    }
    base_dd_min = icicle_damage_sum;
    base_dd_max = icicle_damage_sum;

    // Swap our flag to allow damage calculation again
    fss -> impact_override = true;

    // Re-call functions here, before the impact call to do the damage calculations as we impact.
    handle_frozen( s );
    snapshot_state( s, amount_type ( s ) );

    s -> result = calculate_result( s );
    s -> result_amount = calculate_direct_amount( s );
    frost_mage_spell_t::impact( s );
    p() -> buffs.icicles -> expire();
    if ( p() -> legendary.sephuzs_secret )
    {
      p() -> buffs.sephuzs_secret -> trigger();
    }
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
  int frozen_orb_action_id;
  frost_bomb_explosion_t* frost_bomb_explosion;

  double magtheridons_banished_bracers_multiplier;
  bool magtheridons_bracers;

  ice_lance_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "ice_lance", p, p -> find_class_spell( "Ice Lance" ) ),
    frozen_orb_action_id( 0 ),
    magtheridons_banished_bracers_multiplier( 0.0 ),
    magtheridons_bracers( false )
  {
    parse_options( options_str );
    spell_power_mod.direct = p -> find_spell( 228598 ) -> effectN( 1 ).sp_coeff();


    if ( p -> talents.frost_bomb -> ok() )
    {
      frost_bomb_explosion = new frost_bomb_explosion_t( p );
    }

    if ( p -> talents.lonely_winter -> ok() )
    {
      base_multiplier *= 1.0 + ( p -> talents.lonely_winter -> effectN( 1 ).percent() +
                               p -> artifact.its_cold_outside.data().effectN( 2 ).percent() );
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
  }

  virtual action_state_t* new_state() override
  {
    return new frost_spell_state_t( this, target );
  }



  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  { return frost_mage_spell_t::snapshot_state( s, rt ); }
  virtual double calculate_direct_amount( action_state_t* s ) const override
  {
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

    if ( fss -> impact_override == true )
    {
      return frost_mage_spell_t::calculate_direct_amount( s );
    }
    else
      return s -> result_amount;
  }

  virtual result_e calculate_result( action_state_t* s ) const override
  {
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

    if ( fss -> impact_override == true )
    {
      return frost_mage_spell_t::calculate_result( s );
    }
    else
      return s -> result;
  }
  virtual void execute() override
  {
    // Ice Lance treats the target as frozen with FoF up, this is snapshot on execute
    frozen = ( p() -> buffs.fingers_of_frost -> up() != 0 );

    p() -> buffs.shatterlance -> up();

    frost_mage_spell_t::execute();

    //TODO: This is technically not correct - the buff should be step-wise decreased; but it has
    // no real effect on gameplay.
    if ( !p() -> talents.glacial_spike -> ok() )
    {
      p() -> buffs.icicles -> expire();
    }

    if ( magtheridons_bracers == true )
    {
      p() -> buffs.magtheridons_might -> trigger();
      magtheridons_banished_bracers_multiplier = p() -> buffs.magtheridons_might -> data().effectN( 1 ).percent();
    }


    if ( p() -> sets.has_set_bonus( MAGE_FROST, T17, B2 ) &&
         frozen_orb_action_id >= 0 &&
         p() -> get_active_dots( frozen_orb_action_id ) >= 1)
    {
      p() -> buffs.ice_shard -> trigger();
    }

    // Begin casting all Icicles at the target, beginning 0.25 seconds after the
    // Ice Lance cast with remaining Icicles launching at intervals of 0.75
    // seconds, both values adjusted by haste. Casting continues until all
    // Icicles are gone, including new ones that accumulate Iwhile they're being
    // fired. If target dies, Icicles stop. If Ice Lance is cast again, the
    // current sequence is interrupted and a new one begins.
    if ( !p() -> talents.glacial_spike -> ok() )
    {
      p() -> trigger_icicle( execute_state, true, target );
    }

    p() -> buffs.fingers_of_frost -> decrement();
  }

  virtual void impact( action_state_t* s ) override
  {
    // Swap our flag to allow damage calculation again
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );
    fss -> impact_override = true;

    // Re-call functions here, before the impact call to do the damage calculations as we impact.
    handle_frozen( s );
    snapshot_state( s, amount_type ( s ) );

    s -> result = calculate_result( s );
    s -> result_amount = calculate_direct_amount( s );


    frost_mage_spell_t::impact( s );

    if ( p() -> talents.thermal_void -> ok() &&
         p() -> buffs.icy_veins -> check() &&
         frozen &&
         s -> chain_target == 0 )
    {
      timespan_t tv_extension = p() -> talents.thermal_void
                                    -> effectN( 1 ).time_value() * 1000;

      p() -> buffs.icy_veins -> extend_duration( p(), tv_extension );
    }
    if ( result_is_hit( s -> result ) && frozen &&
         td( s -> target ) -> debuffs.frost_bomb -> check() )
    {
      frost_bomb_explosion -> target = s -> target;
      frost_bomb_explosion -> execute();
    }
  }

  virtual void init() override
  {
    frost_mage_spell_t::init();

    frozen_orb_action_id = p() -> find_action_id( "frozen_orb" );

  }

  virtual double action_multiplier() const override
  {

    double am = frost_mage_spell_t::action_multiplier();

    //TODO: Fix hardcoding of this value
    if ( frozen )
    {
      am *= 3.0;
    }

    if ( p() -> buffs.shatterlance -> check() )
    {
      am *= 1.0 + p() -> legendary.shatterlance_effect;
    }

    if ( p() -> buffs.chain_reaction -> up() )
    {
      am *= 1.0 + ( p() -> buffs.chain_reaction -> data().effectN( 1 ).percent() * p() -> buffs.chain_reaction -> check() );
    }

    if ( p() -> buffs.magtheridons_might -> up() )
    {
      am *= 1.0 + ( magtheridons_banished_bracers_multiplier * p() -> buffs.magtheridons_might -> check() );
    }

    return am;
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
  virtual action_state_t* new_state() override
  {
    return new frost_spell_state_t( this, target );
  }

  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  { return frost_mage_spell_t::snapshot_state( s, rt ); }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

    if ( fss -> impact_override == true )
    {
      return frost_mage_spell_t::calculate_direct_amount( s );
    }
    else
    {
      return s -> result_amount;
    }
  }


  virtual result_e calculate_result( action_state_t* s ) const override
  {
     frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );

     if ( fss -> impact_override == true )
     {
       return frost_mage_spell_t::calculate_result( s );
     }
     else
     {
       return s -> result;
     }
  }

  virtual void impact( action_state_t* s ) override
  {
    // Swap our flag to allow damage calculation again
    frost_spell_state_t* fss = debug_cast<frost_spell_state_t*>( s );
    fss -> impact_override = true;

    // Re-call functions here, before the impact call to do the damage calculations as we impact.
    handle_frozen( s );
    snapshot_state( s, amount_type ( s ) );
    s -> result = calculate_result( s );
    s -> result_amount = calculate_direct_amount( s );

    frost_mage_spell_t::impact( s );
    if ( p() -> legendary.sephuzs_secret )
    {
      p() -> buffs.sephuzs_secret -> trigger();
    }
  }
};


// Icy Veins Spell ==========================================================

struct icy_veins_t : public frost_mage_spell_t
{
  icy_veins_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "icy_veins", p, p -> find_class_spell( "Icy Veins" ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    p() -> buffs.icy_veins -> trigger();
    if ( p() -> legendary.lady_vashjs_grasp )
    {
      p() -> buffs.lady_vashjs_grasp -> trigger();
      // Trigger 1 stack of FoF when IV is triggered with LVG legendary,
      // This is independant of the tick action gains.
      p() -> buffs.fingers_of_frost -> trigger();
    }
    if ( p() -> artifact.chilled_to_the_core.rank() )
    {
    p() -> buffs.chilled_to_the_core -> trigger();
    }
  }
};

// Fire Blast Spell ======================================================

struct fire_blast_t : public fire_mage_spell_t
{
  double pyrosurge_chance;
  pyrosurge_flamestrike_t* pyrosurge_flamestrike;
  cooldown_t* icd;
  blast_furance_t* blast_furnace;
  double fire_blast_crit_chance;

  fire_blast_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "fire_blast", p,
                       p -> find_class_spell( "Fire Blast" ) ),
    pyrosurge_chance( 0.0 ),
    pyrosurge_flamestrike( new pyrosurge_flamestrike_t( p, options_str ) ),
    blast_furnace( nullptr ), fire_blast_crit_chance( 0 )
  {
    parse_options( options_str );

    base_multiplier *= 1.0 + p -> artifact.reignition_overdrive.percent();
    trigger_gcd = timespan_t::zero();
    cooldown -> charges = data().charges();
    cooldown -> charges += p -> spec.fire_blast_3 -> effectN( 1 ).base_value();
    cooldown -> duration = data().charge_cooldown();
    cooldown -> duration += p -> sets.set( MAGE_FIRE, T17, B2 ) -> effectN( 1 ).time_value();
    cooldown -> hasted = true;
    // Fire Blast has a small ICD to prevent it from being double casted
    icd = p -> get_cooldown( "fire_blast_icd" );

    triggers_hot_streak = true;
    triggers_ignite = true;
    triggers_pyretic_incantation = true;

    // TODO: Is the spread range still 10 yards?
    radius = 10;

     pyrosurge_flamestrike -> background = true;
     pyrosurge_flamestrike -> callbacks = false;
     pyrosurge_flamestrike -> triggers_hot_streak = false;

    if ( p -> artifact.blast_furnace.rank() )
    {
      blast_furnace = new blast_furance_t( p, options_str );
    }

    fire_blast_crit_chance = p -> spec.fire_blast_2 -> effectN( 1 ).percent();
  }

  virtual bool ready() override
  {
    if ( icd -> down() )
    {
      return false;
    }

    return fire_mage_spell_t::ready();
  }

  virtual void execute() override
  {
    fire_mage_spell_t::execute();

    icd -> start( data().cooldown() );
  }

  virtual void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {

      if ( p() -> rng().roll( pyrosurge_chance ) )
      {

        pyrosurge_flamestrike -> target = s -> target;
        pyrosurge_flamestrike -> execute();
      }

      if ( s -> result == RESULT_CRIT && p() -> talents.kindling -> ok() )
      {
        p() -> cooldowns.combustion
            -> adjust( -1000 * p() -> talents.kindling
                                   -> effectN( 1 ).time_value() );
      }

      if ( p() -> artifact.blast_furnace.rank() )
      {
        blast_furnace -> target = s -> target;
        blast_furnace -> execute();
      }
    }
  }

  // Fire Blast always crits
  virtual double composite_crit_chance() const override
  { return fire_blast_crit_chance; }
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

    child_lb -> target = s -> target;
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
}

timespan_t living_bomb_t::composite_dot_duration( const action_state_t* s )
  const
{
  timespan_t duration = fire_mage_spell_t::composite_dot_duration( s );
  return duration * ( tick_time( s ) / base_tick_time );
}

void living_bomb_t::last_tick( dot_t* d )
{
  fire_mage_spell_t::last_tick( d );

  explosion -> target = d -> target;
  explosion -> execute();
}

void living_bomb_t::init()
{
  fire_mage_spell_t::init();

  update_flags &= ~STATE_HASTE;
}


// Mark of Aluneth Spell =============================================================
// TODO: Tick times are inconsistent in game. Until fixed, remove hasted ticks
//       and cap the DoT at 5 ticks, then an explosion.

struct mark_of_aluneth_explosion_t : public arcane_mage_spell_t
{
  mark_of_aluneth_explosion_t( mage_t* p ) :
    arcane_mage_spell_t( "mark_of_aluneth_explosion", p, p -> find_spell( 210726 ) )
  {
    background = true;
    school = SCHOOL_ARCANE;
    // Override the nonsense data from their duplicated MoA spelldata.
    dot_duration = timespan_t::zero();
    base_costs[ RESOURCE_MANA ] = 0;
    aoe = -1;
    trigger_gcd = timespan_t::zero();
    triggers_arcane_missiles = false;
  }

  virtual void execute() override
  {
    base_dd_max = p() -> resources.max[ RESOURCE_MANA ] * data().effectN( 1 ).percent();
    base_dd_min = p() -> resources.max[ RESOURCE_MANA ] * data().effectN( 1 ).percent();

    arcane_mage_spell_t::execute();
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
    triggers_arcane_missiles = false;
    spell_power_mod.tick = p -> find_spell( 211088 ) -> effectN( 1 ).sp_coeff();
    hasted_ticks = false;
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

    mark_explosion -> target = d -> target;
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
// TODO: Why is meteor burn not a DoT here?
struct meteor_burn_t : public fire_mage_spell_t
{
  meteor_burn_t( mage_t* p, int targets ) :
    fire_mage_spell_t( "meteor_burn", p, p -> find_spell( 155158 ) )
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
  meteor_impact_t( mage_t* p, int targets ):
    fire_mage_spell_t( "meteor_impact", p, p -> find_spell( 153564 ) )
  {
    background = true;
    aoe = targets;
    split_aoe_damage = true;
    spell_power_mod.direct = data().effectN( 1 ).sp_coeff();
    ground_aoe = true;
    //TODO: Revisit PI behavior once Skullflower confirms behavior.
    triggers_ignite = true;
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.0 );
  }
};

struct meteor_t : public fire_mage_spell_t
{
  int targets;
  meteor_impact_t* meteor_impact;
  meteor_burn_t* meteor_burn;
  timespan_t meteor_delay;
  timespan_t actual_tick_time;
  meteor_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "meteor", p, p -> find_talent_spell( "Meteor") ),
    targets( -1 ),
    meteor_impact( new meteor_impact_t( p, targets ) ),
    meteor_burn( new meteor_burn_t( p, targets ) ),
    meteor_delay( p -> find_spell( 177345 ) -> duration() )
  {
    add_option( opt_int( "targets", targets ) );
    parse_options( options_str );
    hasted_ticks = false;
    callbacks = false;
    add_child( meteor_impact );
    add_child( meteor_burn );
    dot_duration = p -> find_spell( 175396 ) -> duration() -
                   p -> find_spell( 153564 ) -> duration();
    actual_tick_time = p -> find_spell( 155158 ) -> effectN( 1 ).period();
    school = SCHOOL_FIRE;
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t impact_time = meteor_delay * p() ->  composite_spell_haste();
    timespan_t meteor_spawn = impact_time - timespan_t::from_seconds( 1.0 );
    meteor_spawn = std::max( timespan_t::zero(), meteor_spawn );

    return meteor_spawn;
  }

  void impact( action_state_t* s ) override
  {
    // Yep. Don't hate. Need to make the dot tick 1 second after impact.
    base_tick_time = timespan_t::from_seconds( 2 );

    fire_mage_spell_t::impact( s );
    meteor_impact -> target = s -> target;
    meteor_impact -> execute();
    base_tick_time = actual_tick_time;
  }

  void tick( dot_t* d ) override
  {
    fire_mage_spell_t::tick( d );
    meteor_burn -> target = d -> target;
    meteor_burn -> execute();
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
      stats -> add_child( image -> get_stats( "fire_blast" ) );
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

  virtual resource_e current_resource() const override
  { return RESOURCE_NONE; }

  virtual void execute() override
  {
    p() -> benefits.arcane_charge.nether_tempest -> update();

    arcane_mage_spell_t::execute();
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
    am_trigger_source_id = p() -> benefits.arcane_missiles
                               -> get_source_id( data().name_cstr() );

    return arcane_mage_spell_t::init_finished();
  }

  virtual void execute() override
  {
    double am_proc_chance =  p() -> buffs.arcane_missiles -> proc_chance();
    timespan_t nt_remains = td( target ) -> dots.nether_tempest -> remains();

    if ( nt_remains > data().duration() * 0.3 )
    {
      double elapsed = std::min( 1.0, nt_remains / data().duration() );
      am_proc_chance *= 1.0 - elapsed;
    }

    arcane_mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
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
  phoenixs_flames_splash_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "phoenixs_flames_splash", p, p -> find_spell( 224637 ) )
  {
    parse_options( options_str );
    aoe = -1;
    background = true;

    triggers_ignite = true;
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
  // Phoenixs Flames always crits
  virtual double composite_crit_chance() const override
  { return 1.0; }
};

struct phoenixs_flames_t : public fire_mage_spell_t
{
  phoenixs_flames_splash_t* phoenixs_flames_splash;

  phoenixs_flames_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "phoenixs_flames", p, p -> artifact.phoenixs_flames ),
    phoenixs_flames_splash( new phoenixs_flames_splash_t( p, options_str ) )
  {
    parse_options( options_str );

    triggers_hot_streak = true;
    triggers_ignite = true;
    triggers_pyretic_incantation = true;
  }

  virtual void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      phoenixs_flames_splash -> target = s -> target;
      phoenixs_flames_splash -> execute();
    }
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t t = fire_mage_spell_t::travel_time();
    return std::min( t, timespan_t::from_seconds( 0.75 ) );
  }

  // Phoenixs Flames always crits
  virtual double composite_crit_chance() const override
  { return 1.0; }
};


// Pyroblast Spell ============================================================

//Mage T18 2pc Fire Set Bonus
struct conjure_phoenix_t : public fire_mage_spell_t
{
  conjure_phoenix_t( mage_t* p ) :
    fire_mage_spell_t( "conjure_phoenix", p, p -> find_spell( 186181 ) )
  {
    background = true;
    callbacks = false;
  }

  virtual void execute() override
  {
    fire_mage_spell_t::execute();

    if ( p() -> sets.has_set_bonus( MAGE_FIRE, T18, B4 ) )
    {
      p() -> buffs.icarus_uprising -> trigger();
    }
  }
};

struct pyroblast_t : public fire_mage_spell_t
{
  conjure_phoenix_t* conjure_phoenix;
  double marquee_bindings_of_the_sun_king_proc_chance;

  pyroblast_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "pyroblast", p, p -> find_class_spell( "Pyroblast" ) ),
    conjure_phoenix( nullptr ),
    marquee_bindings_of_the_sun_king_proc_chance( 0.0 )
  {
    parse_options( options_str );

    triggers_ignite = true;
    triggers_hot_streak = true;
    triggers_pyretic_incantation = true;

    if ( p -> sets.has_set_bonus( MAGE_FIRE, T18, B2 ) )
    {
      conjure_phoenix = new conjure_phoenix_t( p );
      add_child( conjure_phoenix );
    }

    base_multiplier *= 1.0 + p -> artifact.pyroclasmic_paranoia.percent();
  }

  virtual double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    if ( p() -> buffs.kaelthas_ultimate_ability -> check() &&
         !p() -> buffs.hot_streak -> check() &&
         marquee_bindings_of_the_sun_king_proc_chance > 0 )
    {
      am *= 1.0 + p() -> buffs.kaelthas_ultimate_ability -> data().effectN( 1 ).percent();
    }
    return am;
  }
  virtual action_state_t* new_state() override
  {
    return new ignite_spell_state_t( this, target );
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buffs.hot_streak -> check() ||
         p() -> buffs.pyromaniac -> check() )
    {
      return timespan_t::zero();
    }

    return fire_mage_spell_t::execute_time();
  }

  virtual void execute() override
  {
    p() -> buffs.hot_streak -> up();
    p() -> buffs.pyromaniac -> up();

    fire_mage_spell_t::execute();

    if ( p() -> buffs.kaelthas_ultimate_ability -> check() &&
        !p() -> buffs.hot_streak -> check() )
    {
      p() -> buffs.kaelthas_ultimate_ability -> expire();
    }
    if ( marquee_bindings_of_the_sun_king_proc_chance > 0 &&
         p() -> buffs.hot_streak -> check() &&
         rng().roll( marquee_bindings_of_the_sun_king_proc_chance ) )
    {
      p() -> buffs.kaelthas_ultimate_ability -> trigger();
    }
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
    is -> hot_streak = ( p() -> buffs.hot_streak -> check() != 0 );
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

      ignite_spell_state_t* is = debug_cast<ignite_spell_state_t*>( s );
      if ( p() -> sets.has_set_bonus( MAGE_FIRE, PVP, B4 ) &&
           is -> hot_streak )
      {
        td( s -> target ) -> debuffs.firestarter -> trigger();
      }

      if ( p() -> sets.has_set_bonus( MAGE_FIRE, T18, B2 ) &&
           rng().roll( p() -> sets.set( MAGE_FIRE, T18, B2 )
                           -> proc_chance() ) )
      {
         conjure_phoenix -> schedule_execute();
      }
    }
  }

  virtual double composite_crit_chance_multiplier() const override
  {
    double m = fire_mage_spell_t::composite_crit_chance_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }

  virtual double composite_crit_chance() const override
  {
    double c = fire_mage_spell_t::composite_crit_chance();

    if ( p() -> buffs.pyromaniac -> check() )
    {
      c += 1.0;
    }

    return c;
  }
   double composite_ignite_multiplier( const action_state_t* s ) const override
  {
    const ignite_spell_state_t* is = debug_cast<const ignite_spell_state_t*>( s );

    if ( is -> hot_streak )
    {
      return 2.0;
    }

    return 1.0;
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

    p() -> buffs.ray_of_frost -> bump();
  }

  virtual double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.ray_of_frost -> check() *
                p() -> buffs.ray_of_frost -> data().effectN( 1 ).percent();

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
    triggers_arcane_missiles = false;
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
  double koralons_burning_touch_multiplier;

  scorch_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "scorch", p,
                       p -> find_specialization_spell( "Scorch" ) ),
    koralons_burning_touch_multiplier( 0.0 )
  {
    parse_options( options_str );

    triggers_hot_streak = true;
    triggers_ignite = true;
    triggers_pyretic_incantation = true;

    consumes_ice_floes = false;
  }

  double composite_crit_chance_multiplier() const override
  {
    double m = fire_mage_spell_t::composite_crit_chance_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }
  double composite_target_multiplier( player_t* target ) const override
  {
    double ctm = fire_mage_spell_t::composite_target_multiplier( target );

    if ( ( koralons_burning_touch_multiplier > 0.0 ) && ( target -> health_percentage() <= 35 ) )
    {
      ctm *= 1.0 + koralons_burning_touch_multiplier;
    }

    return ctm;
  }
  virtual void execute() override
  {
    fire_mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) &&
         p() -> artifact.scorched_earth.rank() )
    {
      // Can't use last_gcd_action because it misses IB/Counterspell/Combustion
      // XXX: Fix this to allow item actions such as trinkets or potions
      if ( p() -> last_foreground_action &&
           p() -> last_foreground_action -> data().id() == data().id() )
      {
        p() -> scorched_earth_counter++;
      }
      else
      {
        p() -> scorched_earth_counter = 1;
      }

      if ( p() -> scorched_earth_counter ==
           p() -> artifact.scorched_earth.data().effectN( 1 ).base_value() )
      {
        if ( sim -> debug )
        {
          sim -> out_debug.printf(
              "%s generates hot_streak from scorched_earth",
              player -> name()
           );
        }

        p() -> scorched_earth_counter = 0;
        p() -> buffs.hot_streak -> trigger();
      }
    }
  }

  virtual bool usable_moving() const override
  { return true; }
};


// Slow Spell ===============================================================

struct slow_t : public arcane_mage_spell_t
{
  slow_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "slow", p, p -> find_class_spell( "Slow" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
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

    if ( result_is_hit( execute_state -> result ) &&
         execute_state -> n_targets > 1 )
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
    frost_mage_spell_t( "water_elemental", p, p -> find_class_spell( "Summon Water Elemental" ) )
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

struct summon_arcane_familiar_t : public arcane_mage_spell_t
{
  summon_arcane_familiar_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "summon_arcane_familiar", p,
                         p -> talents.arcane_familiar )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;
    triggers_arcane_missiles = false;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();

    p() -> pets.arcane_familiar -> summon();
  }

  virtual bool ready() override
  {
    if ( !arcane_mage_spell_t::ready() )
    {
      return false;
    }

    return !p() -> pets.arcane_familiar ||
           p() -> pets.arcane_familiar -> is_sleeping();
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
    triggers_arcane_missiles = false;
  }
  virtual void init() override
  {
    mage_spell_t::init();
    // To let us model the legendary ring, it effectivly gives us a 2 charge lust system.

    if ( p() -> legendary.shard_of_the_exodar )
    {
      cooldown -> charges = 2;
      p() -> player_t::buffs.bloodlust -> cooldown -> duration = timespan_t::zero();
    }
  }
  virtual void execute() override
  {
    mage_spell_t::execute();

    // Let us use this to bloodlust ourselves if we have the legendary and have disabled the standard sim lust.
    if ( p() -> legendary.shard_of_the_exodar )
    {
      p() -> player_t::buffs.bloodlust -> default_chance = 1.0;
    }

    // If we have no exhaustion, we're lusting for the raid and everyone gets it.
    if ( !player -> buffs.exhaustion -> check() )
    {
      for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
      {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if ( p -> buffs.exhaustion -> check() || p -> is_pet() )
        continue;

      p -> buffs.bloodlust -> trigger(); // Bloodlust and Timewarp are the same
      p -> buffs.exhaustion -> trigger();
      }
    }

    // If we have the legendary and exhaustion is up, we're lusting for ourselves.
    if ( p() -> legendary.shard_of_the_exodar && player -> buffs.exhaustion -> check() )
    {
      p() -> player_t::buffs.bloodlust -> trigger();
    }
  }

  virtual bool ready() override
  {
    // If we have shard of the exodar, we're controlling our own destiny. Overrides don't
    // apply to us.
    if ( sim -> overrides.bloodlust && !p() -> legendary.shard_of_the_exodar )
      return false;

    if ( player -> buffs.exhaustion -> check() && !p() -> legendary.shard_of_the_exodar )
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
    ignore_false_positive = true;
    trigger_gcd = timespan_t::zero();
    may_miss = may_crit = callbacks = false;

    triggers_arcane_missiles = false;

    aoe = -1;
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
    base_dd_max *= p() -> artifact.touch_of_the_magi.data().effectN( 1 ).percent();
    base_dd_min *= p() -> artifact.touch_of_the_magi.data().effectN( 1 ).percent();

    mage_spell_t::execute();
  }
};


// ============================================================================
// Mage Custom Actions
// ============================================================================

// Choose Rotation ============================================================
/*
struct choose_rotation_t : public action_t
{
  double evocation_target_mana_percentage;
  double ab_cost;
  int force_dps;
  int force_dpm;
  timespan_t final_burn_offset;
  double oom_offset;

  choose_rotation_t( mage_t* p, const std::string& options_str ) :
    action_t( ACTION_USE, "choose_rotation", p )
  {
    cooldown -> duration = timespan_t::from_seconds( 10 );
    evocation_target_mana_percentage = 35;
    force_dps = 0;
    force_dpm = 0;

    final_burn_offset = timespan_t::from_seconds( 20 );
    oom_offset = 0;
    //TODO: Double check this. Scale with AC state.
    double ab_cost = p -> find_class_spell( "Arcane Blast" ) -> cost( POWER_MANA ) * p -> resources.base[ RESOURCE_MANA ];
    add_option( opt_timespan( "cooldown", ( cooldown -> duration ) ) );
    add_option( opt_float( "evocation_pct", evocation_target_mana_percentage ) );
    add_option( opt_int( "force_dps", force_dps ) );
    add_option( opt_int( "force_dpm", force_dpm ) );
    add_option( opt_timespan( "final_burn_offset", ( final_burn_offset ) ) );
    add_option( opt_float( "oom_offset", oom_offset ) );

    parse_options( options_str );

    if ( cooldown -> duration < timespan_t::from_seconds( 1.0 ) )
    {
      sim -> errorf( "Player %s: choose_rotation cannot have cooldown -> duration less than 1.0sec", p -> name() );
      cooldown -> duration = timespan_t::from_seconds( 1.0 );
    }

    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = debug_cast<mage_t*>( player );

    if ( force_dps || force_dpm )
    {
      if ( p -> rotation.current == ROTATION_DPS )
      {
        p -> rotation.dps_time += ( sim -> current_time - p -> rotation.last_time );
      }
      else if ( p -> rotation.current == ROTATION_DPM )
      {
        p -> rotation.dpm_time += ( sim -> current_time - p -> rotation.last_time );
      }
      p -> rotation.last_time = sim -> current_time;

      if ( sim -> log )
      {
        sim -> out_log.printf( "%f burn mps, %f time to die", ( p -> rotation.dps_mana_loss / p -> rotation.dps_time.total_seconds() ) - ( p -> rotation.mana_gain / sim -> current_time.total_seconds() ), sim -> target -> time_to_percent( 0.0 ).total_seconds() );
      }

      if ( force_dps )
      {
        if ( sim -> log ) sim -> out_log.printf( "%s switches to DPS spell rotation", p -> name() );
        p -> rotation.current = ROTATION_DPS;
      }
      if ( force_dpm )
      {
        if ( sim -> log ) sim -> out_log.printf( "%s switches to DPM spell rotation", p -> name() );
        p -> rotation.current = ROTATION_DPM;
      }

      update_ready();

      return;
    }

    if ( sim -> log ) sim -> out_log.printf( "%s Considers Spell Rotation", p -> name() );

    // The purpose of this action is to automatically determine when to start dps rotation.
    // We aim to either reach 0 mana at end of fight or evocation_target_mana_percentage at next evocation.
    // If mana gem has charges we assume it will be used during the next dps rotation burn.
    // The dps rotation should correspond only to the actual burn, not any corrections due to overshooting.

    // It is important to smooth out the regen rate by averaging out the returns from Evocation and Mana Gems.
    // In order for this to work, the resource_gain() method must filter out these sources when
    // tracking "rotation.mana_gain".

    double regen_rate = p -> rotation.mana_gain / sim -> current_time.total_seconds();

    timespan_t ttd = sim -> target -> time_to_percent( 0.0 );
    timespan_t tte = p -> cooldowns.evocation -> remains();

    if ( p -> rotation.current == ROTATION_DPS )
    {
      p -> rotation.dps_time += ( sim -> current_time - p -> rotation.last_time );

      // We should only drop out of dps rotation if we break target mana treshold.
      // In that situation we enter a mps rotation waiting for evocation to come off cooldown or for fight to end.
      // The action list should take into account that it might actually need to do some burn in such a case.

      if ( tte < ttd )
      {
        // We're going until target percentage
        if ( p -> resources.current[ RESOURCE_MANA ] / p -> resources.max[ RESOURCE_MANA ] <= evocation_target_mana_percentage / 100.0 )
        {
          if ( sim -> log ) sim -> out_log.printf( "%s switches to DPM spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPM;
        }
      }
      else
      {
        // We're going until OOM, stop when we can no longer cast full stack AB (approximately, 4 stack with AP can be 6177)
        if ( p -> resources.current[ RESOURCE_MANA ] < ab_cost*4 )
        {
          if ( sim -> log ) sim -> out_log.printf( "%s switches to DPM spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPM;
        }
      }
    }
    else if ( p -> rotation.current == ROTATION_DPM )
    {
      p -> rotation.dpm_time += ( sim -> current_time - p -> rotation.last_time );

      // Calculate consumption rate of dps rotation and determine if we should start burning.

      double consumption_rate = ( p -> rotation.dps_mana_loss / p -> rotation.dps_time.total_seconds() ) - regen_rate;
      double available_mana = p -> resources.current[ RESOURCE_MANA ];

      // If this will be the last evocation then figure out how much of it we can actually burn before end and adjust appropriately.

      timespan_t evo_cooldown = timespan_t::from_seconds( 240.0 );

      timespan_t target_time;
      double target_pct;

      if ( ttd < tte + evo_cooldown )
      {
        if ( ttd < tte + final_burn_offset )
        {
          // No more evocations, aim for OOM
          target_time = ttd;
          target_pct = oom_offset;
        }
        else
        {
          // If we aim for normal evo percentage we'll get the most out of burn/mana adept synergy.
          target_time = tte;
          target_pct = evocation_target_mana_percentage / 100.0;
        }
      }
      else
      {
        // We'll cast more then one evocation, we're aiming for a normal evo target burn.
        target_time = tte;
        target_pct = evocation_target_mana_percentage / 100.0;
      }

      if ( consumption_rate > 0 )
      {
        // Compute time to get to desired percentage.
        timespan_t expected_time = timespan_t::from_seconds( ( available_mana - target_pct * p -> resources.max[ RESOURCE_MANA ] ) / consumption_rate );

        if ( expected_time >= target_time )
        {
          if ( sim -> log ) sim -> out_log.printf( "%s switches to DPS spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPS;
        }
      }
      else
      {
        // If dps rotation is regen, then obviously use it all the time.

        if ( sim -> log ) sim -> out_log.printf( "%s switches to DPS spell rotation", p -> name() );

        p -> rotation.current = ROTATION_DPS;
      }
    }
    p -> rotation.last_time = sim -> current_time;

    update_ready();
  }

  virtual bool ready()
  {
    // This delierately avoids calling the supreclass ready method;
    // not all the checks there are relevnt since this isn't a spell
    if ( cooldown -> down() )
      return false;

    if ( sim -> current_time < cooldown -> duration )
      return false;

    if ( if_expr && ! if_expr -> success() )
      return false;

    return true;
  }
};

*/
// Choose Target Action =======================================================

struct choose_target_t : public action_t
{
  bool check_selected;
  player_t* selected_target;

  // Infinite loop protection
  timespan_t last_execute;

  std::string target_name;

  choose_target_t( mage_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "choose_target", p ),
    check_selected( false ), selected_target( nullptr ),
    last_execute( timespan_t::min() )
  {
    add_option( opt_string( "name", target_name ) );
    add_option( opt_bool( "check_selected", check_selected ) );
    parse_options( options_str );

    radius = range = -1.0;
    trigger_gcd = timespan_t::zero();

    harmful = may_miss = may_crit = callbacks = false;
    ignore_false_positive = true;
    action_skill = 1;
  }

  bool init_finished() override
  {
    if ( ! target_name.empty() && ! util::str_compare_ci( target_name, "default" ) )
    {
      selected_target = player -> actor_by_name_str( target_name );
    }
    else
      selected_target = player -> target;

    return action_t::init_finished();
  }

  result_e calculate_result( action_state_t* ) const override
  { return RESULT_HIT; }

  block_result_e calculate_block_result( action_state_t* ) const override
  { return BLOCK_RESULT_UNBLOCKED; }

  void execute() override
  {
    action_t::execute();

    // Don't do anything if selected target is sleeping
    if ( ! selected_target || selected_target -> is_sleeping() )
    {
      return;
    }

    mage_t* p = debug_cast<mage_t*>( player );
    assert( ! target_if_expr || ( selected_target == select_target_if_target() ) );

    if ( sim -> current_time() == last_execute )
    {
      sim -> errorf( "%s choose_target infinite loop detected (due to no time passing between executes) at '%s'",
        p -> name(), signature_str.c_str() );
      sim -> cancel_iteration();
      sim -> cancel();
      return;
    }

    last_execute = sim -> current_time();

    if ( sim -> debug )
      sim -> out_debug.printf( "%s swapping target from %s to %s", player -> name(), p -> current_target -> name(), selected_target -> name() );

    p -> current_target = selected_target;

    // Invalidate target caches
    for ( size_t i = 0, end = p -> action_list.size(); i < end; i++ )
      p -> action_list[i] -> target_cache.is_valid = false;
    }

  bool ready() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    if ( target_if_mode != TARGET_IF_NONE )
    {
      selected_target = select_target_if_target();
      if ( selected_target == nullptr )
      {
        return false;
      }
    }

    // Safeguard stupidly against breaking the sim.
    if ( selected_target -> is_sleeping() )
    {
      // Reset target to default actor target if we're still targeting a dead selected target
      if ( p -> current_target == selected_target )
        p -> current_target = p -> target;

      return false;
    }

    if ( p -> current_target == selected_target )
      return false;

    player_t* original_target = nullptr;
    if ( check_selected )
    {
      if ( target != selected_target )
        original_target = target;

      target = selected_target;
    }
    else
      target = p -> current_target;

    bool rd = action_t::ready();

    if ( original_target )
      target = original_target;

    return rd;
  }

  void reset() override
  {
    action_t::reset();
    last_execute = timespan_t::min();
  }
};


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
    may_miss = may_dodge = may_parry = may_crit = may_block = false;
    callbacks = false;
    aoe = -1;
    base_costs[ RESOURCE_MANA ] = 0;
    trigger_gcd = timespan_t::zero();
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
    base_dd_max *= data().effectN( 4 ).percent();
    base_dd_min *= data().effectN( 4 ).percent();

    mage_spell_t::execute();
  }
};

void mage_spell_t::trigger_unstable_magic( action_state_t* s )
{
  double um_proc_rate;
  switch ( p() -> specialization() )
  {
    case MAGE_ARCANE:
      um_proc_rate = p() -> unstable_magic_explosion
                       -> data().effectN( 1 ).percent();
      break;
    case MAGE_FROST:
      um_proc_rate = p() -> unstable_magic_explosion
                       -> data().effectN( 2 ).percent();
      break;
    case MAGE_FIRE:
      um_proc_rate = p() -> unstable_magic_explosion
                       -> data().effectN( 3 ).percent();
      break;
    default:
      um_proc_rate = p() -> unstable_magic_explosion
                       -> data().effectN( 3 ).percent();
      break;
  }

  if ( p() -> rng().roll( um_proc_rate ) )
  {
    p() -> unstable_magic_explosion -> target = s -> target;
    p() -> unstable_magic_explosion -> base_dd_max = s -> result_amount;
    p() -> unstable_magic_explosion -> base_dd_min = s -> result_amount;
    p() -> unstable_magic_explosion -> execute();
  }
}


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

    if ( ! action )
    {
      mage_t* m = debug_cast<mage_t*>( player );
      action = debug_cast<pets::water_elemental::water_jet_t*>( m -> pets.water_elemental -> find_action( "water_jet" ) );
      if ( action )
      {
        action->autocast = false;
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
    if ( action -> queued == true )
      return false;

    return action_t::ready();
  }
};

// =====================================================================================
// Mage Specific Spell Overrides
// =====================================================================================


// Override T18 trinket procs so they are affected by mage multipliers
struct darklight_ray_t : public mage_spell_t
{
  darklight_ray_t( mage_t* p, const special_effect_t& effect ) :
    mage_spell_t( "darklight_ray", p, p -> find_spell( 183950 ) )
  {
    background = may_crit = true;
    callbacks = false;

    // Mage specific control
    triggers_arcane_missiles = false;
    consumes_ice_floes = false;

    base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( effect.item );

    aoe = -1;
  }
};

struct doom_nova_t : public mage_spell_t
{
  doom_nova_t( mage_t* p, const special_effect_t& effect ) :
    mage_spell_t( "doom_nova", p, p -> find_spell( 184075 ) )
  {
    background = may_crit = true;
    callbacks = false;

    // Mage specific control
    triggers_arcane_missiles = false;
    consumes_ice_floes = false;

    base_dd_min = base_dd_max = data().effectN( 1 ).average( effect.item );

    aoe = -1;
  }
};

// Override Nithramus ring explosion to impact Prismatic Crystal

struct nithramus_t : public mage_spell_t
{
  double damage_coeff;
  nithramus_t( mage_t* p, const special_effect_t& effect ) :
    mage_spell_t( "nithramus", p, p -> find_spell( 187611 ) )
  {
    damage_coeff = data().effectN( 1 ).average( effect.item ) / 10000.0;

    background = split_aoe_damage = true;
    may_crit = callbacks = false;
    trigger_gcd = timespan_t::zero();
    aoe = -1;
    radius = 20;
    range = -1;
    travel_speed = 0.0;
  }

  void init() override
  {
    mage_spell_t::init();

    snapshot_flags = STATE_MUL_DA;
    update_flags = 0;
  }

  double composite_da_multiplier( const action_state_t* ) const override
  {
    return damage_coeff;
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
    double cast_time = first ? 0.25 : 0.75;
    cast_time *= mage -> cache.spell_speed();

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

    actions::icicle_state_t* new_s = debug_cast<actions::icicle_state_t*>( mage -> icicle -> get_state() );
    new_s -> source = state.stats;
    new_s -> target = target;

    mage -> icicle -> base_dd_min = mage -> icicle -> base_dd_max = state.damage;

    // Immediately execute icicles so the correct damage is carried into the
    // travelling icicle object
    mage -> icicle -> pre_execute_state = new_s;
    mage -> icicle -> execute();

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
    if ( !ignite->is_ticking() )
    {
      return 0.0;
    }

    auto ignite_state = debug_cast<residual_action::residual_periodic_state_t*>(
        ignite->state );
    return ignite_state->tick_amount * ignite->ticks_left();
  }

  static bool ignite_compare ( dot_t* a, dot_t* b )
  {
    return ignite_bank( a ) > ignite_bank( b );
  }

  ignite_spread_event_t( mage_t& m, timespan_t delta_time ) :
    event_t( m, delta_time ), mage( &m )
  {
  }

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
        index = floor( mage -> rng().real() * index );
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
    mage->ignite_spread_event = make_event<events::ignite_spread_event_t>(
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
  dots.flamestrike    = target -> get_dot( "flamestrike",    mage );
  dots.frost_bomb     = target -> get_dot( "frost_bomb",     mage );
  dots.ignite         = target -> get_dot( "ignite",         mage );
  dots.living_bomb    = target -> get_dot( "living_bomb",    mage );
  dots.nether_tempest = target -> get_dot( "nether_tempest", mage );
  dots.frozen_orb     = target -> get_dot( "frozen_orb",     mage );

  debuffs.erosion     = new buffs::erosion_t( this );
  debuffs.slow        = buff_creator_t( *this, "slow",
                                        mage -> find_spell( 31589 ) );
  debuffs.touch_of_the_magi = new buffs::touch_of_the_magi_t( this );

  debuffs.firestarter = buff_creator_t( *this, "firestarter" )
                          .chance( 1.0 )
                          .duration( timespan_t::from_seconds( 10.0 ) );

  debuffs.chilled     = new buffs::chilled_t( this );
  debuffs.frost_bomb  = buff_creator_t( *this, "frost_bomb",
                                        mage -> talents.frost_bomb );
  debuffs.water_jet   = buff_creator_t( *this, "water_jet",
                                        mage -> find_spell( 135029 ) )
                          .quiet( true )
                          .cd( timespan_t::zero() );
  //TODO: Find spelldata for this!
  debuffs.winters_chill = buff_creator_t( *this, "winters_chill" )
                          .duration( timespan_t::from_seconds( 1.0 ) );
}

mage_t::mage_t( sim_t* sim, const std::string& name, race_e r ) :
  player_t( sim, MAGE, name, r ),
  current_target( target ),
  icicle( nullptr ),
  icicle_event( nullptr ),
  ignite( nullptr ),
  ignite_spread_event( nullptr ),
  touch_of_the_magi_explosion( nullptr ),
  unstable_magic_explosion( nullptr ),
  last_bomb_target( nullptr ),
  scorched_earth_counter( 0 ),
  wild_arcanist( nullptr ),
  pyrosurge( nullptr ),
  shatterlance( nullptr ),
  last_summoned( temporal_hero_e::INVALID ),
  distance_from_rune( 0.0 ),
  global_cinder_count( 0 ),
  incanters_flow_stack_mult( find_spell( 116267 ) -> effectN( 1 ).percent() ),
  iv_haste( 1.0 ),
  benefits( benefits_t() ),
  buffs( buffs_t() ),
  cooldowns( cooldowns_t() ),
  gains( gains_t() ),
  legendary( legendary_t() ),
  pets( pets_t() ),
  procs( procs_t() ),
  rotation( rotation_t() ),
  spec( specializations_t() ),
  talents( talents_list_t() )
{
  // Cooldowns
  cooldowns.combustion       = get_cooldown( "combustion"       );
  cooldowns.cone_of_cold     = get_cooldown( "cone_of_cold"     );
  cooldowns.dragons_breath   = get_cooldown( "dragons_breath"   );
  cooldowns.evocation        = get_cooldown( "evocation"        );
  cooldowns.frozen_orb       = get_cooldown( "frozen_orb"       );
  cooldowns.icy_veins        = get_cooldown( "icy_veins"        );
  cooldowns.fire_blast       = get_cooldown( "fire_blast"    );
  cooldowns.phoenixs_flames  = get_cooldown( "phoenixs_flames"  );
  cooldowns.presence_of_mind = get_cooldown( "presence_of_mind" );
  cooldowns.ray_of_frost     = get_cooldown( "ray_of_frost"     );

  // Options
  base.distance = 30;
  regen_type = REGEN_DYNAMIC;
  regen_caches[ CACHE_MASTERY ] = true;

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

  touch_of_the_magi_explosion -> target = buff -> player;
  touch_of_the_magi_explosion -> base_dd_max = buff -> accumulated_damage;
  touch_of_the_magi_explosion -> base_dd_min = buff -> accumulated_damage;
  touch_of_the_magi_explosion -> execute();
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
  if ( name == "charged_up"        ) return new                charged_up_t( this, options_str  );
  if ( name == "evocation"         ) return new                  evocation_t( this, options_str );
  if ( name == "mark_of_aluneth"   ) return new            mark_of_aluneth_t( this, options_str );
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
  if ( name == "flame_on"          ) return new                flame_on_t( this, options_str );
  if ( name == "fire_blast"     ) return new                 fire_blast_t( this, options_str );
  if ( name == "living_bomb"       ) return new             living_bomb_t( this, options_str );
  if ( name == "meteor"            ) return new                  meteor_t( this, options_str );
  if ( name == "pyroblast"         ) return new               pyroblast_t( this, options_str );
  if ( name == "scorch"            ) return new                  scorch_t( this, options_str );

  // Frost
  if ( name == "blizzard"          ) return new                blizzard_t( this, options_str );
  if ( name == "comet_storm"       ) return new             comet_storm_t( this, options_str );
  if ( name == "flurry"            ) return new                  flurry_t( this, options_str );
  if ( name == "frost_bomb"        ) return new              frost_bomb_t( this, options_str );
  if ( name == "frostbolt"         ) return new               frostbolt_t( this, options_str );
  if ( name == "frost_nova"        ) return new              frost_nova_t( this, options_str );
  if ( name == "frozen_orb"        ) return new              frozen_orb_t( this, options_str );
  if ( name == "frozen_touch"      ) return new            frozen_touch_t( this, options_str );
  if ( name == "glacial_spike"     ) return new           glacial_spike_t( this, options_str );
  if ( name == "ice_lance"         ) return new               ice_lance_t( this, options_str );
  if ( name == "ice_nova"          ) return new                ice_nova_t( this, options_str );
  if ( name == "icy_veins"         ) return new               icy_veins_t( this, options_str );
  if ( name == "ray_of_frost"      ) return new            ray_of_frost_t( this, options_str );
  if ( name == "water_elemental"   ) return new  summon_water_elemental_t( this, options_str );
  if ( name == "water_jet"         ) return new               water_jet_t( this, options_str );

  // Artifact Specific Spells
  // Fire
  if ( name == "phoenix_reborn"    ) return new           phoenix_reborn_t( this );
  if ( name == "phoenixs_flames"   ) return new          phoenixs_flames_t( this, options_str );

  // Frost
  if ( name == "ebonbolt"          ) return new                 ebonbolt_t( this, options_str );

  // Shared spells
  if ( name == "blink"             ) return new                   blink_t( this, options_str );
  if ( name == "cone_of_cold"      ) return new            cone_of_cold_t( this, options_str );
  if ( name == "counterspell"      ) return new            counterspell_t( this, options_str );
  if ( name == "time_warp"         ) return new               time_warp_t( this, options_str );

  if ( name == "choose_target"     ) return new           choose_target_t( this, options_str );

  // Shared talents
  if ( name == "ice_floes"         ) return new               ice_floes_t( this, options_str );
  // TODO: Implement all T45 talents and enable freezing_grasp
  // if ( name == "freezing_grasp"    )
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

  return player_t::create_action( name, options_str );
}

// mage_t::create_proc_action =================================================

action_t* mage_t::create_proc_action( const std::string& name, const special_effect_t& effect )
{
  if ( util::str_compare_ci( name, "darklight_ray" ) ) return new actions::darklight_ray_t( this, effect );
  if ( util::str_compare_ci( name, "doom_nova" ) )     return new     actions::doom_nova_t( this, effect );
  if ( util::str_compare_ci( name, "nithramus" ) )     return new     actions::nithramus_t( this, effect );

  return nullptr;
}

// mage_t::create_options =====================================================
void mage_t::create_options()
{
  add_option( opt_float( "global_cinder_count", global_cinder_count ) );

  player_t::create_options();
}
// mage_t::create_pets ========================================================

void mage_t::create_pets()
{
  if ( specialization() == MAGE_FROST && find_action( "water_elemental" ) )
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

  if ( sets.has_set_bonus( MAGE_ARCANE, T18, B2 ) )
  {
    // There isn't really a cap on temporal heroes, but 10 sounds safe-ish
    for ( unsigned i = 0; i < pets.temporal_hero_count; i++ )
    {
      pets.temporal_heroes.push_back( new pets::temporal_hero::temporal_hero_t( sim, this ) );
    }
    pets::temporal_hero::randomize_last_summoned( this );
  }

  if ( talents.arcane_familiar -> ok() &&
       find_action( "summon_arcane_familiar" ) )
  {
    pets.arcane_familiar = new pets::arcane_familiar::arcane_familiar_pet_t( sim, this );

  }
}

// mage_t::init_spells ========================================================

void mage_t::init_spells()
{
  player_t::init_spells();

  // Talents
  // Tier 15
  talents.arcane_familiar = find_talent_spell( "Arcane Familiar" );
  talents.presence_of_mind= find_talent_spell( "Presence of Mind");
  talents.resonance     = find_talent_spell( "Resonance"         );
  talents.pyromaniac      = find_talent_spell( "Pyromaniac"      );
  talents.conflagration   = find_talent_spell( "Conflagration"   );
  talents.fire_starter    = find_talent_spell( "Firestarter"    );
  talents.ray_of_frost    = find_talent_spell( "Ray of Frost"    );
  talents.lonely_winter   = find_talent_spell( "Lonely Winter"   );
  talents.bone_chilling   = find_talent_spell( "Bone Chilling"   );
  // Tier 30
  talents.shimmer         = find_talent_spell( "Shimmer"         );
  talents.cauterize       = find_talent_spell( "Cauterize"       );
  talents.ice_block       = find_talent_spell( "Ice Block"       );
  // Tier 45
  talents.mirror_image    = find_talent_spell( "Mirror Image"    );
  talents.rune_of_power   = find_talent_spell( "Rune of Power"   );
  talents.incanters_flow  = find_talent_spell( "Incanter's Flow" );
  // Tier 60
  talents.supernova       = find_talent_spell( "Supernova"       );
  talents.charged_up      = find_talent_spell( "Charged Up"      );
  talents.words_of_power  = find_talent_spell( "Words of Power"  );
  talents.blast_wave      = find_talent_spell( "Blast Wave"      );
  talents.flame_on        = find_talent_spell( "Flame On"        );
  talents.controlled_burn = find_talent_spell( "Controlled Burn" );
  talents.ice_nova        = find_talent_spell( "Ice Nova"        );
  talents.frozen_touch    = find_talent_spell( "Frozen Touch"    );
  talents.splitting_ice   = find_talent_spell( "Splitting Ice"   );
  // Tier 75
  talents.ice_floes       = find_talent_spell( "Ice Floes"       );
  talents.ring_of_frost   = find_talent_spell( "Ring of Frost"   );
  talents.ice_ward        = find_talent_spell( "Ice Ward"        );
  // Tier 90
  talents.nether_tempest  = find_talent_spell( "Nether Tempest"  );
  talents.living_bomb     = find_talent_spell( "Living Bomb"     );
  talents.frost_bomb      = find_talent_spell( "Frost Bomb"      );
  talents.unstable_magic  = find_talent_spell( "Unstable Magic"  );
  talents.erosion         = find_talent_spell( "Erosion"         );
  talents.flame_patch     = find_talent_spell( "Flame Patch"     );
  talents.arctic_gale     = find_talent_spell( "Arctic Gale"     );
  // Tier 100
  talents.overpowered     = find_talent_spell( "Overpowered"     );
  talents.quickening      = find_talent_spell( "Quickening"      );
  talents.arcane_orb      = find_talent_spell( "Arcane Orb"      );
  talents.kindling        = find_talent_spell( "Kindling"        );
  talents.cinderstorm     = find_talent_spell( "Cinderstorm"     );
  talents.meteor          = find_talent_spell( "Meteor"          );
  talents.thermal_void    = find_talent_spell( "Thermal Void"    );
  talents.glacial_spike   = find_talent_spell( "Glacial Spike"   );
  talents.comet_storm     = find_talent_spell( "Comet Storm"     );

  //Artifact Spells
  //Arcane
  artifact.aegwynns_ascendance     = find_artifact_spell( "Aegwynn's Ascendance"   );
  artifact.aegwynns_fury           = find_artifact_spell( "Aegwynn's Fury"         );
  artifact.aegwynns_imperative     = find_artifact_spell( "Aegwynn's Imperative"   );
  artifact.aegwynns_wrath          = find_artifact_spell( "Aegwynn's Wrath"        );
  artifact.arcane_purification     = find_artifact_spell( "Arcane Purification"    );
  artifact.arcane_rebound          = find_artifact_spell( "Arcane Rebound"         );
  artifact.blasting_rod            = find_artifact_spell( "Blasting Rod"           );
  artifact.crackling_energy        = find_artifact_spell( "Crackling Energy"       );
  artifact.mark_of_aluneth         = find_artifact_spell( "Mark of Aluneth"        );
  artifact.might_of_the_guardians  = find_artifact_spell( "Might of the Guardians" );
  artifact.rule_of_threes          = find_artifact_spell( "Rule of Threes"         );
  artifact.torrential_barrage      = find_artifact_spell( "Torrential Barrage"     );
  artifact.everywhere_at_once      = find_artifact_spell( "Everywhere At Once"     );
  artifact.ethereal_sensitivity    = find_artifact_spell( "Ethereal Sensitivity"   );
  artifact.touch_of_the_magi       = find_artifact_spell( "Touch of the Magi"      );
  artifact.ancient_power           = find_artifact_spell( "Ancient Power"          );
  //Fire
  artifact.aftershocks             = find_artifact_spell( "Aftershocks"            );
  artifact.scorched_earth          = find_artifact_spell( "Scorched Earth"         );
  artifact.big_mouth               = find_artifact_spell( "Big Mouth"              );
  artifact.blue_flame_special      = find_artifact_spell( "Blue Flame Special"     );
  artifact.everburning_consumption = find_artifact_spell( "Everburning Consumption");
  artifact.molten_skin             = find_artifact_spell( "Molten Skin"            );
  artifact.phoenix_reborn          = find_artifact_spell( "Phoenix Reborn"         );
  artifact.phoenixs_flames         = find_artifact_spell( "Phoenix's Flames"       );
  artifact.great_balls_of_fire     = find_artifact_spell( "Great Balls of Fire"    );
  artifact.cauterizing_blink       = find_artifact_spell( "Cauterizing Blink"      );
  artifact.fire_at_will            = find_artifact_spell( "Fire At Will"           );
  artifact.pyroclasmic_paranoia    = find_artifact_spell( "Pyroclasmic Paranoia"   );
  artifact.reignition_overdrive    = find_artifact_spell( "Reignition Overdrive"   );
  artifact.pyretic_incantation     = find_artifact_spell( "Pyretic Incantation"    );
  artifact.burning_gaze            = find_artifact_spell( "Burning Gaze"           );
  artifact.blast_furnace           = find_artifact_spell( "Blast Furnace"          );
  artifact.wings_of_flame          = find_artifact_spell( "Wings of Flame"         );
  artifact.empowered_spellblade    = find_artifact_spell( "Empowered Spellblade"   );
  //Frost
  artifact.ebonbolt                = find_artifact_spell( "Ebonbolt"               );
  artifact.jouster                 = find_artifact_spell( "Jouster"                );
  artifact.let_it_go               = find_artifact_spell( "Let It Go"              );
  artifact.frozen_veins            = find_artifact_spell( "Frozen Veins"           );
  artifact.the_storm_rages         = find_artifact_spell( "The Storm Rages"        );
  artifact.black_ice               = find_artifact_spell( "Black Ice"              );
  artifact.shield_of_alodi         = find_artifact_spell( "Shield of Alodi"        );
  artifact.clarity_of_thought      = find_artifact_spell( "Clarity of Thought"     );
  artifact.icy_caress              = find_artifact_spell( "Icy Caress"             );
  artifact.chain_reaction          = find_artifact_spell( "Chain Reaction"         );
  artifact.orbital_strike          = find_artifact_spell( "Orbital Strike"         );
  artifact.its_cold_outside        = find_artifact_spell( "It's Cold Outside"      );
  artifact.icy_hand                = find_artifact_spell( "Icy Hand"               );
  artifact.ice_age                 = find_artifact_spell( "Ice Age"                );
  artifact.chilled_to_the_core     = find_artifact_spell( "Chilled To The Core"    );
  artifact.shattering_bolts        = find_artifact_spell( "Shattering Bolts"       );
  artifact.ice_nine                = find_artifact_spell( "Ice Nine"               );
  artifact.spellborne              = find_artifact_spell( "Spellborne"             );


  // Spec Spells
  spec.arcane_barrage_2      = find_specialization_spell( 231564 );
  spec.arcane_charge         = find_spell( 36032 );
  spec.arcane_charge_passive = find_spell( 114664 );
  if ( talents.arcane_familiar -> ok() )
  {
    spec.arcane_familiar    = find_spell( 210126 );
  }
  spec.evocation_2           = find_specialization_spell( 231565 );

  spec.critical_mass         = find_specialization_spell( "Critical Mass"    );
  spec.base_crit_bonus       = find_spell( 137019 );
  spec.brain_freeze          = find_specialization_spell( "Brain Freeze"     );
  spec.brain_freeze_2        = find_specialization_spell( 231584 );
  spec.molten_armor_2        = find_specialization_spell( 231630 );
  spec.fingers_of_frost      = find_spell( 112965 );
  spec.shatter               = find_specialization_spell( "Shatter"          );
  spec.shatter_2             = find_specialization_spell( 231582 );

  spec.fire_blast_2          = find_specialization_spell( 231568 );
  spec.fire_blast_3          = find_specialization_spell( 231567 );

  // Mastery
  spec.savant                = find_mastery_spell( MAGE_ARCANE );
  spec.ignite                = find_mastery_spell( MAGE_FIRE );
  spec.icicles               = find_mastery_spell( MAGE_FROST );
  spec.icicles_driver        = find_spell( 148012 );

  // Active spells
  if ( spec.ignite -> ok()  )
    ignite = new actions::ignite_t( this );
  if ( spec.icicles -> ok() )
    icicle = new actions::icicle_t( this );
  if ( talents.unstable_magic -> ok() )
    unstable_magic_explosion = new actions::unstable_magic_explosion_t( this );

  if ( artifact.touch_of_the_magi.rank() )
  {
    touch_of_the_magi_explosion =
      new actions::touch_of_the_magi_explosion_t( this );
  }
}

// mage_t::init_base ========================================================

void mage_t::init_base_stats()
{
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
  buffs.arcane_affinity       = buff_creator_t( this, "arcane_affinity", find_spell( 166871 ) )
                                  .trigger_spell( sets.set( MAGE_ARCANE, T17, B2 ) );
  buffs.arcane_charge         = buff_creator_t( this, "arcane_charge", spec.arcane_charge );
  buffs.arcane_instability    = buff_creator_t( this, "arcane_instability", find_spell( 166872 ) )
                                  .trigger_spell( sets.set( MAGE_ARCANE, T17, B4 ) );
  buffs.arcane_missiles       = new buffs::arcane_missiles_t( this );
  buffs.arcane_power          = buff_creator_t( this, "arcane_power", find_spell( 12042 ) )
                                  .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  if ( artifact.aegwynns_imperative.rank() )
    buffs.arcane_power -> buff_duration += artifact.aegwynns_imperative.time_value();
  buffs.mage_armor            = stat_buff_creator_t( this, "mage_armor", find_spell( 6117 ) );
  buffs.presence_of_mind      = buff_creator_t( this, "presence_of_mind", find_spell( 205025 ) )
                                  .activated( true )
                                  .cd( timespan_t::zero() )
                                  .duration( timespan_t::zero() );
  buffs.quickening            = buff_creator_t( this, "quickening", find_spell( 198924 ) )
                                  .add_invalidate( CACHE_SPELL_HASTE )
                                  .max_stack( 50 );

  // 4T18 Temporal Power buff has no duration and stacks multiplicatively
  buffs.temporal_power        = buff_creator_t( this, "temporal_power", find_spell( 190623 ) )
                                  .max_stack( 10 );

  // Fire
  buffs.combustion            = buff_creator_t( this, "combustion", find_spell( 190319 ) )
                                  .cd( timespan_t::zero() )
                                  .add_invalidate( CACHE_SPELL_CRIT_CHANCE )
                                  .add_invalidate( CACHE_MASTERY );
  buffs.enhanced_pyrotechnics = buff_creator_t( this, "enhanced_pyrotechnics", find_spell( 157644 ) );
  buffs.heating_up            = buff_creator_t( this, "heating_up",  find_spell( 48107 ) );
  buffs.hot_streak            = buff_creator_t( this, "hot_streak",  find_spell( 48108 ) );
  buffs.molten_armor = buff_creator_t( this, "molten_armor", find_spell( 30482 ) )
                                  .add_invalidate( CACHE_SPELL_CRIT_CHANCE );
  buffs.icarus_uprising       = buff_creator_t( this, "icarus_uprising", find_spell( 186170 ) )
                                  .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                                  .add_invalidate( CACHE_SPELL_HASTE );
  buffs.pyretic_incantation   = buff_creator_t( this, "pyretic_incantation", find_spell( 194329 ) );
  buffs.pyromaniac            = buff_creator_t( this, "pyromaniac", sets.set( MAGE_FIRE, T17, B4 ) -> effectN( 1 ).trigger() )
                                  .trigger_spell( sets.set( MAGE_FIRE, T17, B4 ) );
  buffs.streaking             = buff_creator_t( this, "streaking", find_spell( 211399 ) )
                                  .add_invalidate( CACHE_SPELL_HASTE );

  // Frost
  //TODO: Remove hardcoded duration once spelldata contains the value
  buffs.brain_freeze          = buff_creator_t( this, "brain_freeze", find_spell( 190446 ) )
                                  .duration( timespan_t::from_seconds( 15.0 ) );
  buffs.bone_chilling         = buff_creator_t( this, "bone_chilling", find_spell( 205766 ) );
  buffs.fingers_of_frost      = buff_creator_t( this, "fingers_of_frost", find_spell( 44544 ) )
                                  .max_stack( find_spell( 44544 ) -> max_stacks() +
                                              sets.set( MAGE_FROST, T18, B4 ) -> effectN( 2 ).base_value() +
                                              artifact.icy_hand.rank() );
  buffs.frost_armor           = buff_creator_t( this, "frost_armor", find_spell( 7302 ) )
                                  .add_invalidate( CACHE_SPELL_HASTE );

  // Buff to track icicles. This does not, however, track the true amount of icicles present.
  // Instead, as it does in game, it tracks icicle buff stack count based on the number of *casts*
  // of icicle generating spells. icicles are generated on impact, so they are slightly de-synced.
  buffs.icicles               = buff_creator_t( this, "icicles", find_spell( 148012 ) ).max_stack( 5.0 );

  buffs.icy_veins             = new buffs::icy_veins_buff_t( this );
  buffs.frost_t17_4pc         = buff_creator_t( this, "frost_t17_4pc", find_spell( 165470 ) )
                                  .duration( find_spell( 84714 ) -> duration() )
                                  .period( find_spell( 165470 ) -> effectN( 1 ).time_value() )
                                  .quiet( true )
                                  .tick_callback( [ this ]( buff_t*, int, const timespan_t& )
                                  {
                                    buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );

                                    // Remove benefit tracking to save work
                                    // benefits.fingers_of_frost -> update( "4T17" );
                                  }
                                );
  buffs.ice_shard             = buff_creator_t( this, "ice_shard", find_spell( 166869 ) );
  buffs.ray_of_frost          = new buffs::ray_of_frost_buff_t( this );
  buffs.shatterlance          = buff_creator_t( this, "shatterlance")
                                  .duration( timespan_t::from_seconds( 0.9 ) )
                                  .cd( timespan_t::zero() )
                                  .chance( 1.0 );

  // Talents
  buffs.ice_floes             = buff_creator_t( this, "ice_floes", talents.ice_floes );
  buffs.incanters_flow        = new buffs::incanters_flow_t( this );
  buffs.rune_of_power         = buff_creator_t( this, "rune_of_power", find_spell( 116014 ) )
                                  .duration( find_spell( 116011 ) -> duration() )
                                  .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Artifact
  // Frost
  buffs.chain_reaction   = buff_creator_t( this, "chain_reaction", find_spell( 195418 ) );

  buffs.chilled_to_the_core = buff_creator_t( this, "chilled_to_the_core", find_spell( 195446 ) )
                                   .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Legendary
  buffs.cord_of_infinity   = buff_creator_t( this, "cord_of_infinity", find_spell( 209316 ) )
                                             .default_value( find_spell( 209311 ) -> effectN( 1 ).percent() );
  buffs.magtheridons_might = buff_creator_t( this, "magtheridons_might", find_spell( 214404 ) );
  buffs.zannesu_journey    = buff_creator_t( this, "zannesu_journey", find_spell( 226852 ) );
  buffs.lady_vashjs_grasp  = new buffs::lady_vashjs_grasp_t( this );
  buffs.rhonins_assaulting_armwraps = buff_creator_t( this, "rhonins_assaulting_armwraps", find_spell( 208081 ) );

  buffs.sephuzs_secret     = new buffs::sephuzs_secret_buff_t( this );

  buffs.kaelthas_ultimate_ability = buff_creator_t( this, "kaelthas_ultimate_ability", find_spell( 209455 ) );
}

// mage_t::init_gains =======================================================

void mage_t::init_gains()
{
  player_t::init_gains();

  gains.evocation              = get_gain( "evocation"              );
  gains.mystic_kilt_of_the_rune_master = get_gain( "Mystic Kilt of the Rune Master" );
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

// mage_t::init_stats =========================================================

void mage_t::init_stats()
{
  player_t::init_stats();

  // Cache Icy Veins haste multiplier for performance reasons
  double haste = buffs.icy_veins -> data().effectN( 1 ).percent();
  iv_haste = 1.0 / ( 1.0 + haste );

  // Register target reset callback here (anywhere later on than in
  // constructor) so older GCCs are happy
  // Forcibly reset mage's current target, if it dies.
  sim->target_non_sleeping_list.register_callback( [this]( player_t* ) {

    // If the mage's current target is still alive, bail out early.
    if ( range::find( sim->target_non_sleeping_list, current_target ) !=
         sim->target_non_sleeping_list.end() )
    {
      return;
    }

    if ( sim->debug )
    {
      sim->out_debug.printf(
          "%s current target %s died. Resetting target to %s.", name(),
          current_target->name(), target->name() );
    }

    current_target = target;
  } );
}


// mage_t::init_assessors =====================================================

void mage_t::init_assessors()
{
  player_t::init_assessors();

  if ( artifact.touch_of_the_magi.rank() )
  {
    mage_t* mage = this;
    assessor_out_damage.add(
      assessor::TARGET_DAMAGE - 1,
      [ mage ]( dmg_e, action_state_t* state ) {
        buffs::touch_of_the_magi_t* buff = mage -> get_target_data( state -> target )
                            -> debuffs.touch_of_the_magi;

        if ( buff -> check() )
        {
          buff -> accumulate_damage( state );
        }

        return assessor::CONTINUE;
      }
    );
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

// mage_t::has_t18_class_trinket ==============================================

bool mage_t::has_t18_class_trinket() const
{
  if ( specialization() == MAGE_ARCANE )
  {
    return wild_arcanist != nullptr;
  }
  else if ( specialization() == MAGE_FIRE )
  {
    return pyrosurge != nullptr;
  }
  else if ( specialization() == MAGE_FROST )
  {
    return shatterlance != nullptr;
  }

  return false;
}

// This method only handles 1 item per call in order to allow the user to add special conditons and placements
// to certain items.
std::string mage_t::get_special_use_items( const std::string& item_name, bool specials )
{
  std::string actions;
  std::string conditions;

  // If we're dealing with a special item, find its special conditional.
  if ( specials )
  {
    if ( item_name == "obelisk_of_the_void" )
    {
      conditions = "if=buff.rune_of_power.up&cooldown.combustion.remains>50";
    }
    if ( item_name == "horn_of_valor" )
    {
      conditions = "if=cooldown.combustion.remains>30";
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
      std::string action_string = "use_item,slot=";
      action_string += item.slot_name();

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

// Because we care about both the ability to control special conditions AND position of our on use items,
// we must use our own get_item_actions which knows to ignore all "special" items and let them be handled by get_special_use_items()
std::vector<std::string> mage_t::get_non_speical_item_actions()
{
  std::vector<std::string> actions;
  bool special = false;
  std::vector<std::string> specials;

  // very ugly construction of our list of special items
  specials.push_back( "obelisk_of_the_void" );
  specials.push_back( "horn_of_valor" );

  for ( const auto& item : items )
  {
    // Check our list of specials to see if we're dealing with one
    for ( size_t i = 0; i < specials.size(); i++ )
    {
      if ( item.name_str == specials[i] )
        special = true;
    }

    // This will skip Addon and Enchant-based on-use effects. Addons especially are important to
    // skip from the default APLs since they will interfere with the potion timer, which is almost
    // always preferred over an Addon. Don't do this for specials.
    if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) && special == false )
    {
      std::string action_string = "use_item,slot=";
      action_string += item.slot_name();
      actions.push_back( action_string );
    }
    // We're moving onto a new item, reset special flag.
    special = false;
  }

  return actions;
}

//Pre-combat Action Priority List============================================

void mage_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  if( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask_action = "flask,type=";

    if ( true_level <= 85 )
    {
      flask_action += "draconic_mind" ;
    }
    else if ( true_level <= 90 )
    {
      flask_action += "warm_sun" ;
    }
    else if ( true_level <= 100 )
    {
      flask_action += "greater_draenic_intellect_flask" ;
    }
    else
    {
      flask_action += "flask_of_the_whispered_pact";
    }

    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && true_level >= 80 )
  {
    precombat -> add_action( get_food_action() );
  }

  if ( true_level > 100 )
    precombat -> add_action( "augmentation,type=defiled" );

  // Water Elemental
  if ( specialization() == MAGE_FROST )
    precombat -> add_action( "water_elemental" );
  if ( specialization() == MAGE_ARCANE )
    precombat -> add_action( "summon_arcane_familiar" ) ;
  // Snapshot Stats
  precombat -> add_action( "snapshot_stats" );

  // Level 90 talents
  precombat -> add_talent( this, "Mirror Image" );

  //Potions
  if ( sim -> allow_potions && level() >= 80 )
  {
    precombat -> add_action( get_potion_action() );
  }

  if ( specialization() == MAGE_ARCANE )
    precombat -> add_action( this, "Arcane Blast" );
  else if ( specialization() == MAGE_FIRE )
    precombat -> add_action( this, "Pyroblast" );
  else if ( specialization() == MAGE_FROST )
    precombat -> add_action( this, "Frostbolt" );
}

std::string mage_t::get_food_action()
{
  std::string food_action = "food,type=";

  if ( true_level <= 85 )
  {
    food_action += "seafood_magnifique_feast" ;
  }
  else if ( true_level <= 90 )
  {
    food_action += "mogu_fish_stew" ;
  }
  else if ( true_level <= 100 )
  {
    if ( specialization() == MAGE_ARCANE )
    {
      if ( sets.has_set_bonus( MAGE_ARCANE, T18, B4 ) )
      {
        food_action += "buttered_sturgeon" ;
      }
      else
      {
        food_action += "sleeper_sushi" ;
      }
    }
    else if ( specialization() == MAGE_FIRE )
    {
      food_action += "pickled_eel" ;
    }
    else
    {
      food_action += "salty_squid_roll" ;
    }
  }
  else
  {
    if ( specialization() == MAGE_ARCANE )
    {
      food_action += "the_hungry_magister" ;
    }
    else if ( specialization() == MAGE_FIRE )
    {
      food_action += "the_hungry_magister" ;
    }
    else
    {
      food_action += "azshari_salad" ;
    }
  }

  return food_action;
}


std::string mage_t::get_potion_action()
{
  std::string potion_action = "potion,name=";

  if ( true_level <= 85 )
  {
    potion_action += "volcanic";
  }
  else if ( true_level <= 90 )
  {
    potion_action += "jade_serpent";
  }
  else if ( true_level <= 100 )
  {
    potion_action += "draenic_intellect";
  }
  else
  {
    potion_action += "deadly_grace";
  }

  return potion_action;
}


// Arcane Mage Action List====================================================

void mage_t::apl_arcane()
{
  std::vector<std::string> item_actions       = get_non_speical_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  action_priority_list_t* default_list        = get_action_priority_list( "default"          );
  action_priority_list_t* conserve            = get_action_priority_list( "conserve"         );
  action_priority_list_t* rop_phase           = get_action_priority_list( "rop_phase"        );
  action_priority_list_t* build               = get_action_priority_list( "build"            );
  action_priority_list_t* cooldowns           = get_action_priority_list( "cooldowns"        );
  action_priority_list_t* burn                = get_action_priority_list( "burn"             );
  action_priority_list_t* init_burn           = get_action_priority_list( "init_burn"        );

  default_list -> add_action( this, "Counterspell",
                              "if=target.debuff.casting.react" );
  default_list -> add_action( this, "Time Warp", "if=(time=0&buff.bloodlust.down)|(buff.bloodlust.down&equipped.132410)" );
  default_list -> add_talent( this, "Mirror Image", "if=buff.arcane_power.down" );
  default_list -> add_action( "stop_burn_phase,if=prev_gcd.evocation&burn_phase_duration>gcd.max" );
  default_list -> add_action( this, "Mark of Aluneth", "if=cooldown.arcane_power.remains>20" );
  default_list -> add_action( "call_action_list,name=build,if=buff.arcane_charge.stack<4" );
  default_list -> add_action( "call_action_list,name=init_burn,if=buff.arcane_power.down&buff.arcane_charge.stack=4&(cooldown.mark_of_aluneth.remains=0|cooldown.mark_of_aluneth.remains>20)&(!talent.rune_of_power.enabled|(cooldown.arcane_power.remains<=action.rune_of_power.cast_time|action.rune_of_power.recharge_time<cooldown.arcane_power.remains))|target.time_to_die<45" );
  default_list -> add_action( "call_action_list,name=burn,if=burn_phase" );
  default_list -> add_action( "call_action_list,name=rop_phase,if=buff.rune_of_power.up&!burn_phase" );
  default_list -> add_action( "call_action_list,name=conserve" );

  conserve     -> add_action( this, "Arcane Missiles", "if=buff.arcane_missiles.react=3" );

  conserve     -> add_action( this, "Arcane Explosion", "if=buff.quickening.remains<action.arcane_blast.cast_time&talent.quickening.enabled" );
  conserve     -> add_action( this, "Arcane Blast", "if=mana.pct>99" );
  conserve     -> add_talent( this, "Nether Tempest", "if=(refreshable|!ticking)" );
  conserve     -> add_action( this, "Arcane Blast", "if=buff.rhonins_assaulting_armwraps.up&equipped.132413" );
  conserve     -> add_action( this, "Arcane Missiles" );
  conserve     -> add_talent( this, "Supernova", "if=mana.pct<100" );
  conserve     -> add_action( this, "Frost Nova", "if=equipped.132452" );
  conserve     -> add_action( this, "Arcane Explosion", "if=mana.pct>=82&equipped.132451&active_enemies>1" );
  conserve     -> add_action( this, "Arcane Blast", "if=mana.pct>=82&equipped.132451" );
  conserve     -> add_action( this, "Arcane Barrage", "if=mana.pct<100&cooldown.arcane_power.remains>5" );
  conserve     -> add_action( this, "Arcane Explosion", "if=active_enemies>1" );
  conserve     -> add_action( this, "Arcane Blast" );

  rop_phase    -> add_action( this, "Arcane Missiles", "if=buff.arcane_missiles.react=3" );
  rop_phase    -> add_action( this, "Arcane Explosion", "if=buff.quickening.remains<action.arcane_blast.cast_time&talent.quickening.enabled" );
  rop_phase    -> add_talent( this, "Nether Tempest", "if=dot.nether_tempest.remains<=2|!ticking" );
  rop_phase    -> add_action( this, "Arcane Missiles", "if=buff.arcane_charge.stack=4" );
  rop_phase    -> add_talent( this, "Super Nova", "if=mana.pct<100" );
  rop_phase    -> add_action( this, "Arcane Explosion", "if=active_enemies>1" );
  rop_phase    -> add_action( this, "Arcane Blast", "if=mana.pct>45" );
  rop_phase    -> add_action( this, "Arcane Barrage" );

  build        -> add_talent( this, "Charged Up", "if=buff.arcane_charge.stack<=1" );
  build        -> add_action( this, "Arcane Missiles", "if=buff.arcane_missiles.react=3" );
  build        -> add_talent( this, "Arcane Orb" );
  build        -> add_action( this, "Arcane Explosion", "if=active_enemies>1" );
  build        -> add_action( this, "Arcane Blast" );

  cooldowns    -> add_talent( this, "Rune of Power", "if=mana.pct>45&buff.arcane_power.down" );
  cooldowns    -> add_action( this, "Arcane Power" );
  for( size_t i = 0; i < racial_actions.size(); i++ )
  {
    cooldowns -> add_action( racial_actions[i] );
  }
  for( size_t i = 0; i < item_actions.size(); i++ )
  {
    cooldowns -> add_action( item_actions[i] );
  }
  if ( race == RACE_TROLL || race == RACE_ORC )
  {
    cooldowns -> add_action( "potion,name=deadly_grace,if=buff.arcane_power.up&(buff.berserking.up|buff.blood_fury.up)" );
  }
  else
  {
    cooldowns -> add_action( "potion,name=deadly_grace,if=buff.arcane_power.up" );
  }

  init_burn -> add_action( this, "Mark of Aluneth" );
  init_burn -> add_action( this, "Frost Nova", "if=equipped.132452" );
  init_burn -> add_talent( this, "Nether Tempest", "if=dot.nether_tempest.remains<10&(prev_gcd.mark_of_aluneth|(talent.rune_of_power.enabled&cooldown.rune_of_power.remains<gcd.max))" );
  init_burn -> add_talent( this, "Rune of Power" );
  init_burn -> add_action( "start_burn_phase,if=((cooldown.evocation.remains-(2*burn_phase_duration))%2<burn_phase_duration)|cooldown.arcane_power.remains=0|target.time_to_die<55" );

  burn      -> add_action( "call_action_list,name=cooldowns" );
  burn      -> add_action( this, "Arcane Missiles", "if=buff.arcane_missiles.react=3" );
  burn      -> add_action( this, "Arcane Explosion", "if=buff.quickening.remains<action.arcane_blast.cast_time&talent.quickening.enabled" );
  burn      -> add_talent( this, "Presence of Mind", "if=buff.arcane_power.remains>2*gcd" );
  burn      -> add_talent( this, "Nether Tempest", "if=dot.nether_tempest.remains<=2|!ticking" );
  burn      -> add_action( this, "Arcane Blast", "if=active_enemies<=1&mana.pct%10*execute_time>target.time_to_die" );
  burn      -> add_action( this, "Arcane Explosion", "if=active_enemies>1&mana.pct%10*execute_time>target.time_to_die" );
  burn      -> add_action( this, "Arcane Missiles", "if=buff.arcane_missiles.react>1" );
  burn      -> add_action( this, "Arcane Explosion", "if=active_enemies>1&buff.arcane_power.remains>cast_time" );
  burn      -> add_action( this, "Arcane Blast", "if=buff.presence_of_mind.up|buff.arcane_power.remains>cast_time" );
  burn      -> add_talent( this, "Supernova", "if=mana.pct<100" );
  burn      -> add_action( this, "Arcane Missiles", "if=mana.pct>10&(talent.overpowered.enabled|buff.arcane_power.down)" );
  burn      -> add_action( this, "Arcane Explosion", "if=active_enemies>1" );
  burn      -> add_action( this, "Arcane Blast" );
  burn      -> add_action( this, "Evocation", "interrupt_if=mana.pct>99" );


  /*
  TODO: Arcane APL needs love :<
  */
}

// Fire Mage Action List ===================================================================================================

void mage_t::apl_fire()
{
  std::vector<std::string> non_special_item_actions       = mage_t::get_non_speical_item_actions();
  std::vector<std::string> racial_actions                 = get_racial_actions();


  action_priority_list_t* default_list        = get_action_priority_list( "default"           );
  action_priority_list_t* combustion_phase    = get_action_priority_list( "combustion_phase"  );
  action_priority_list_t* rop_phase           = get_action_priority_list( "rop_phase"         );
  action_priority_list_t* active_talents      = get_action_priority_list( "active_talents"    );
  action_priority_list_t* single_target       = get_action_priority_list( "single_target"     );

  default_list -> add_action( this, "Counterspell", "if=target.debuff.casting.react" );
  default_list -> add_action( this, "Time Warp", "if=(time=0&buff.bloodlust.down)|(buff.bloodlust.down&equipped.132410)" );
  default_list -> add_talent( this, "Mirror Image", "if=buff.combustion.down" );
  default_list -> add_talent( this, "Rune of Power", "if=cooldown.combustion.remains>40&buff.combustion.down&(cooldown.flame_on.remains<5|cooldown.flame_on.remains>30)&!talent.kindling.enabled|target.time_to_die.remains<11|talent.kindling.enabled&(charges_fractional>1.8|time<40)&cooldown.combustion.remains>40" );
  default_list -> add_action( mage_t::get_special_use_items( "horn_of_valor", true ) );
  default_list -> add_action( mage_t::get_special_use_items( "obelisk_of_the_void", true ) );
  default_list -> add_action( mage_t::get_special_use_items( "mrrgrias_favor", false ) );

  default_list -> add_action( "call_action_list,name=combustion_phase,if=cooldown.combustion.remains<=action.rune_of_power.cast_time+(!talent.kindling.enabled*gcd)|buff.combustion.up" );
  default_list -> add_action( "call_action_list,name=rop_phase,if=buff.rune_of_power.up&buff.combustion.down" );
  default_list -> add_action( "call_action_list,name=single_target" );

  combustion_phase -> add_talent( this, "Rune of Power", "if=buff.combustion.down" );
  combustion_phase -> add_action( "call_action_list,name=active_talents" );
  combustion_phase -> add_action( this, "Combustion" );
  combustion_phase -> add_action( get_potion_action() );

  for( size_t i = 0; i < racial_actions.size(); i++ )
  {
    combustion_phase -> add_action( racial_actions[i] );
  }

  for( size_t i = 0; i < non_special_item_actions.size(); i++ )
  {
    combustion_phase -> add_action( non_special_item_actions[i] );
  }
  combustion_phase -> add_action( mage_t::get_special_use_items( "obelisk_of_the_void", false ) );
  combustion_phase -> add_action( this, "Pyroblast", "if=buff.kaelthas_ultimate_ability.react&buff.combustion.remains>execute_time" );
  combustion_phase -> add_action( this, "Pyroblast", "if=buff.hot_streak.up" );
  combustion_phase -> add_action( this, "Fire Blast", "if=buff.heating_up.up" );
  combustion_phase -> add_action( this, "Phoenix's Flames" );
  combustion_phase -> add_action( this, "Scorch", "if=buff.combustion.remains>cast_time" );
  combustion_phase -> add_talent( this, "Dragon's Breath", "if=buff.hot_streak.down&action.fire_blast.charges<1&action.phoenixs_flames.charges<1" );
  combustion_phase -> add_action( this, "Scorch", "if=target.health.pct<=25&equipped.132454");

  rop_phase        -> add_talent( this, "Rune of Power" );
  rop_phase        -> add_action( this, "Pyroblast", "if=buff.hot_streak.up" );
  rop_phase        -> add_action( "call_action_list,name=active_talents" );
  rop_phase        -> add_action( this, "Pyroblast", "if=buff.kaelthas_ultimate_ability.react" );
  rop_phase        -> add_action( this, "Fire Blast", "if=!prev_off_gcd.fire_blast" );
  rop_phase        -> add_action( this, "Phoenix's Flames", "if=!prev_gcd.phoenixs_flames" );
  rop_phase        -> add_action( this, "Scorch", "if=target.health.pct<=25&equipped.132454" );
  rop_phase        -> add_action( this, "Fireball" );

  active_talents   -> add_talent( this, "Flame On", "if=action.fire_blast.charges=0&(cooldown.combustion.remains>40+(talent.kindling.enabled*25)|target.time_to_die.remains<cooldown.combustion.remains)" );
  active_talents   -> add_talent( this, "Blast Wave", "if=(buff.combustion.down)|(buff.combustion.up&action.fire_blast.charges<1&action.phoenixs_flames.charges<1)" );
  active_talents   -> add_talent( this, "Meteor", "if=cooldown.combustion.remains>30|(cooldown.combustion.remains>target.time_to_die)|buff.rune_of_power.up" );
  active_talents   -> add_talent( this, "Cinderstorm", "if=cooldown.combustion.remains<cast_time&(buff.rune_of_power.up|!talent.rune_on_power.enabled)|cooldown.combustion.remains>10*spell_haste&!buff.combustion.up" );
  active_talents   -> add_action( this, "Dragon's Breath", "if=equipped.132863" );
  active_talents   -> add_talent( this, "Living Bomb", "if=active_enemies>1&buff.combustion.down" );

  single_target    -> add_action( this, "Pyroblast", "if=buff.hot_streak.up&buff.hot_streak.remains<action.fireball.execute_time" );
  single_target    -> add_action( this, "Phoenix's Flames", "if=charges_fractional>2.7&active_enemies>2" );
  single_target    -> add_action( this, "Flamestrike", "if=talent.flame_patch.enabled&active_enemies>2&buff.hot_streak.react" );
  single_target    -> add_action( this, "Pyroblast", "if=buff.hot_streak.up&!prev_gcd.pyroblast" );
  single_target    -> add_action( this, "Pyroblast", "if=buff.hot_streak.react&target.health.pct<=25&equipped.132454" );
  single_target    -> add_action( this, "Pyroblast", "if=buff.kaelthas_ultimate_ability.react" );
  single_target    -> add_action( "call_action_list,name=active_talents" );
  single_target    -> add_action( this, "Fire Blast", "if=!talent.kindling.enabled&buff.heating_up.up&(!talent.rune_of_power.enabled|charges_fractional>1.4|cooldown.combustion.remains<40)&(3-charges_fractional)*(12*spell_haste)<cooldown.combustion.remains+3|target.time_to_die.remains<4" );
  single_target    -> add_action( this, "Fire Blast", "if=talent.kindling.enabled&buff.heating_up.up&(!talent.rune_of_power.enabled|charges_fractional>1.5|cooldown.combustion.remains<40)&(3-charges_fractional)*(18*spell_haste)<cooldown.combustion.remains+3|target.time_to_die.remains<4" );
  single_target    -> add_action( this, "Phoenix's Flames", "if=(buff.combustion.up|buff.rune_of_power.up|buff.incanters_flow.stack>3|talent.mirror_image.enabled)&artifact.phoenix_reborn.enabled&(4-charges_fractional)*13<cooldown.combustion.remains+5|target.time_to_die.remains<10" );
  single_target    -> add_action( this, "Phoenix's Flames", "if=(buff.combustion.up|buff.rune_of_power.up)&(4-charges_fractional)*30<cooldown.combustion.remains+5" );
  single_target    -> add_action( this, "Scorch", "if=target.health.pct<=25&equipped.132454" );
  single_target    -> add_action( this, "Fireball" );


}

// Frost Mage Action List ==============================================================================================================

void mage_t::apl_frost()
{
  std::vector<std::string> item_actions = get_item_actions();
  std::vector<std::string> racial_actions = get_racial_actions();

  action_priority_list_t* default_list      = get_action_priority_list( "default"           );
  action_priority_list_t* cooldowns         = get_action_priority_list( "cooldowns"         );

  default_list -> add_action( this, "Counterspell", "if=target.debuff.casting.react" );
  default_list -> add_action( this, "Ice Lance", "if=buff.fingers_of_frost.react=0&prev_gcd.flurry" );
  default_list -> add_action( this, "Time Warp", "if=(time=0&buff.bloodlust.down)|(buff.bloodlust.down&equipped.132410)" );
  default_list -> add_action( "call_action_list,name=cooldowns" );
  default_list -> add_talent( this, "Ice Nova", "if=debuff.winters_chill.up" );
  default_list -> add_action( this, "Frostbolt", "if=prev_off_gcd.water_jet" );
  default_list -> add_action( "water_jet,if=prev_gcd.frostbolt&buff.fingers_of_frost.stack<(2+artifact.icy_hand.enabled)&buff.brain_freeze.react=0" );
  default_list -> add_talent( this, "Ray of Frost", "if=buff.icy_veins.up|(cooldown.icy_veins.remains>action.ray_of_frost.cooldown&buff.rune_of_power.down)" );
  default_list -> add_action( this, "Flurry", "if=buff.brain_freeze.react&buff.fingers_of_frost.react=0&prev_gcd.frostbolt" );
  default_list -> add_talent( this, "Frozen Touch", "if=buff.fingers_of_frost.stack<=(0+artifact.icy_hand.enabled)&((cooldown.icy_veins.remains>30&talent.thermal_void.enabled)|!talent.thermal_void.enabled)" );
  default_list -> add_talent( this, "Frost Bomb", "if=debuff.frost_bomb.remains<action.ice_lance.travel_time&buff.fingers_of_frost.react>0" );
  default_list -> add_action( this, "Ice Lance", "if=buff.fingers_of_frost.react>0&cooldown.icy_veins.remains>10|buff.fingers_of_frost.react>2" );
  default_list -> add_action( this, "Frozen Orb" );
  default_list -> add_talent( this, "Ice Nova" );
  default_list -> add_talent( this, "Comet Storm" );
  default_list -> add_action( this, "Blizzard", "if=talent.arctic_gale.enabled|active_enemies>1|((buff.zannesu_journey.stack>4|buff.zannesu_journey.remains<cast_time+1)&equipped.133970)" );
  default_list -> add_action( this, "Ebonbolt", "if=buff.fingers_of_frost.stack<=(0+artifact.icy_hand.enabled)" );
  default_list -> add_talent( this, "Glacial Spike" );
  default_list -> add_action( this, "Frostbolt" );

  cooldowns    -> add_talent( this, "Rune of Power", "if=cooldown.icy_veins.remains<cast_time|charges_fractional>1.9&cooldown.icy_veins.remains>10|buff.icy_veins.up|target.time_to_die.remains+5<charges_fractional*10" );
  cooldowns -> add_action( "potion,name=potion_of_prolonged_power,if=cooldown.icy_veins.remains<1" );
  cooldowns    -> add_action( this, "Icy Veins", "if=buff.icy_veins.down" );
  cooldowns    -> add_talent( this, "Mirror Image" );
  for( size_t i = 0; i < item_actions.size(); i++ )
  {
    cooldowns -> add_action( item_actions[i] );
  }
  for( size_t i = 0; i < racial_actions.size(); i++ )
  {
    cooldowns -> add_action( racial_actions[i] );
  }
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
    mps *= 1.0 + composite_mastery() * ( spec.savant -> effectN( 1 ).mastery_value() );
  }

  // This, technically, is not correct. The buff itself, ticking every 1s, should
  // be granting mana regen. However, the total regan gained should be
  // similar doing it here, or doing it in the buff tick.
  // TODO: Move to buff ticks.
  if ( buffs.cord_of_infinity -> up() )
  {
    mps *= 1.0 + buffs.cord_of_infinity -> data().effectN( 1 ).percent();
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
      else if ( spec.icicles -> ok() )
      {
        pets.water_elemental -> invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
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

  if ( pets.arcane_familiar && !pets.arcane_familiar -> is_sleeping() )
  {
    resources.max[ rt ] *= 1.0 +
      spec.arcane_familiar -> effectN( 1 ).percent();
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

  if ( artifact.burning_gaze.rank() && s -> action -> school == SCHOOL_FIRE )
  {
    m *= 1.0 + artifact.burning_gaze.percent();
  }

  if ( ( !dbc::is_school( s -> action -> get_school(), SCHOOL_PHYSICAL ) ) &&
       buffs.pyretic_incantation -> check() > 0 )
  {
    m *= 1.0 + ( buffs.pyretic_incantation -> data().effectN( 1 ).percent() *
                 buffs.pyretic_incantation -> stack() );
  }

  return m;
}
// mage_t::composite_player_multiplier =======================================

double mage_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( specialization() == MAGE_ARCANE )
  {
    // Spell data says magic damage only.
    if ( buffs.arcane_power -> check() && dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK )
    {
      double v = buffs.arcane_power -> value();
      m *= 1.0 + v;
    }
  }

  if ( talents.rune_of_power -> ok() )
  {
    if ( buffs.rune_of_power -> check() )
    {
      m *= 1.0 + buffs.rune_of_power -> data().effectN( 1 ).percent();
    }
  }
  else if ( talents.incanters_flow -> ok() )
  {
    m *= 1.0 + buffs.incanters_flow -> stack() * incanters_flow_stack_mult;

    cache.player_mult_valid[ school ] = false;
  }

  if ( buffs.icarus_uprising -> check() )
  {
    m *= 1.0 + buffs.icarus_uprising -> data().effectN( 2 ).percent();
  }

  if ( buffs.temporal_power -> check() &&
       sets.has_set_bonus( MAGE_ARCANE, T18, B4 ) )
  {
    m *= std::pow( 1.0 + buffs.temporal_power -> data().effectN( 1 ).percent(),
                   buffs.temporal_power -> check() );
  }

  if ( buffs.chilled_to_the_core -> check() )
  {
    m *= 1.0 + buffs.chilled_to_the_core -> data().effectN( 1 ).percent();
  }

  return m;
}

// mage_t::composite_mastery_rating =============================================

double mage_t::composite_mastery_rating() const
{
  double m = player_t::composite_mastery_rating();

  if ( buffs.combustion -> up() )
  {
    m += mage_t::composite_spell_crit_rating();
  }
 return m;
}

// mage_t::composite_spell_crit_chance ===============================================

double mage_t::composite_spell_crit_chance() const
{
  double c = player_t::composite_spell_crit_chance();

  if ( buffs.molten_armor -> check() )
  {
    c += ( buffs.molten_armor -> data().effectN( 1 ).percent() + spec.molten_armor_2 -> effectN( 1 ).percent() );
  }

  if ( specialization() == MAGE_FIRE )
  {
    c += spec.base_crit_bonus -> effectN( 4 ).percent();
  }
  if ( buffs.combustion -> check() )
  {
    c += buffs.combustion -> data().effectN( 1 ).percent();
  }

  if ( artifact.aegwynns_wrath.rank() )
  {
    c += artifact.aegwynns_wrath.percent();
  }
  return c;
}

// mage_t::composite_spell_haste ==============================================

double mage_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.icy_veins -> check() )
  {
    h *= iv_haste;
  }

  if ( buffs.sephuzs_secret -> check() )
  {
    h *= 1.0 / ( 1.0 + buffs.sephuzs_secret -> default_value );
  }

  if ( buffs.frost_armor -> check() )
  {
    h /= 1.0 + buffs.frost_armor -> data().effectN( 1 ).percent();
  }

  if ( buffs.icarus_uprising -> check() )
  {
    h /= 1.0 + buffs.icarus_uprising -> data().effectN( 1 ).percent();
  }

  if ( buffs.streaking -> check() )
  {
    h /= 1.0 + buffs.streaking -> data().effectN( 1 ).percent();
  }
  // TODO: Double check scaling with hits.
  if ( buffs.quickening -> check() )
  {
    h /= 1.0 + buffs.quickening -> data().effectN( 1 ).percent() * buffs.quickening -> check();
  }

  return h;
}

// mage_t::matching_gear_multiplier =========================================

// TODO: Is this still around?
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

  current_target = target;

  icicles.clear();
  event_t::cancel( icicle_event );
  event_t::cancel( ignite_spread_event );

  if ( spec.savant -> ok() )
  {
    recalculate_resource_max( RESOURCE_MANA );
  }

  scorched_earth_counter = 0;
  last_bomb_target = nullptr;
  burn_phase.reset();

  if ( sets.has_set_bonus( MAGE_ARCANE, T18, B2 ) )
  {
    pets::temporal_hero::randomize_last_summoned( this );
  }
}



// mage_t::resource_gain ====================================================

double mage_t::resource_gain( resource_e resource,
                              double    amount,
                              gain_t*   source,
                              action_t* action )
{
  double actual_amount = player_t::resource_gain( resource, amount, source, action );

  if ( resource == RESOURCE_MANA )
  {
    if ( source != gains.evocation )
    {
      rotation.mana_gain += actual_amount;
    }
  }

  return actual_amount;
}

// mage_t::resource_loss ====================================================

double mage_t::resource_loss( resource_e resource,
                              double    amount,
                              gain_t*   source,
                              action_t* action )
{
  double actual_amount = player_t::resource_loss( resource, amount, source, action );

  if ( resource == RESOURCE_MANA )
  {
    if ( rotation.current == ROTATION_DPS )
    {
      rotation.dps_mana_loss += actual_amount;
    }
    else if ( rotation.current == ROTATION_DPM )
    {
      rotation.dpm_mana_loss += actual_amount;
    }
  }

  return actual_amount;
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
  double temporary = player_t::temporary_movement_modifier();

  // TODO: New movement buffs?

  return temporary;
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
      buffs.mage_armor -> trigger();
      break;
    case MAGE_FROST:
      buffs.frost_armor -> trigger();
      break;
    case MAGE_FIRE:
      buffs.molten_armor -> trigger();
      break;
    default:
      apl_default(); // DEFAULT
      break;
  }

  if ( legendary.lady_vashjs_grasp )
  {
    buffs::lady_vashjs_grasp_t* lvg =
      debug_cast<buffs::lady_vashjs_grasp_t*>( buffs.lady_vashjs_grasp );
    lvg -> fof_source_id =
      benefits.fingers_of_frost -> get_source_id( "Lady Vashj's Grasp" );
  }

  if ( spec.ignite -> ok()  )
  {
    timespan_t first_spread = timespan_t::from_seconds( rng().real() * 2.0 );
    ignite_spread_event =
        make_event<events::ignite_spread_event_t>( *sim, *this, first_spread );
  }
}

// Copypasta, execept for target selection. This is a massive kludge. Buyer
// beware!

action_t* mage_t::select_action( const action_priority_list_t& list )
{
  player_t* action_target = nullptr;

  // Mark this action list as visited with the APL internal id
  visited_apls_ |= list.internal_id_mask;

  // Cached copy for recursion, we'll need it if we come back from a
  // call_action_list tree, with nothing to show for it.
  uint64_t _visited = visited_apls_;

  for (auto a : list.foreground_action_list)
  {
    visited_apls_ = _visited;

    if ( a -> background ) continue;

    if ( a -> wait_on_ready == 1 )
      break;

    // Change the target of the action before ready call ...
    if ( a -> target != current_target )
    {
      action_target = a -> target;
      a -> target = current_target;
    }

    if ( a -> ready() )
    {
      // Ready variables execute, and processing conitnues
      if ( a -> type == ACTION_VARIABLE )
      {
        a -> execute();
        continue;
      }
      // Call_action_list action, don't execute anything, but rather recurse
      // into the called action list.
      else if ( a -> type == ACTION_CALL )
      {
        call_action_list_t* call = static_cast<call_action_list_t*>( a );
        // Restore original target before recursing into the called action list
        if ( action_target )
        {
          a -> target = action_target;
          action_target = nullptr;
        }

        // If the called APLs bitflag (based on internal id) is up, we're in an
        // infinite loop, and need to cancel the sim
        if ( visited_apls_ & call -> alist -> internal_id_mask )
        {
          sim -> errorf( "%s action list in infinite loop", name() );
          sim -> cancel_iteration();
          sim -> cancel();
          return nullptr;
        }

        // We get an action from the call, return it
        if ( action_t* real_a = select_action( *call -> alist ) )
        {
          if ( real_a -> action_list )
            real_a -> action_list -> used = true;
          return real_a;
        }
      }
      else
      {
        return a;
      }
    }
    // Action not ready, restore target for extra safety
    else if ( action_target )
    {
      a -> target = action_target;
      action_target = nullptr;
    }
  }

  return nullptr;
}
// mage_t::create_expression ================================================

expr_t* mage_t::create_expression( action_t* a, const std::string& name_str )
{
  struct mage_expr_t : public expr_t
  {
    mage_t& mage;
    mage_expr_t( const std::string& n, mage_t& m ) :
      expr_t( n ), mage( m ) {}
  };

  // Current target expression support
  // "current_target" returns the actor id of the mage's current target
  // "current_target.<some other expression>" evaluates <some other expression> on the current
  // target of the mage
  if ( util::str_in_str_ci( name_str, "current_target" ) )
  {
    std::string::size_type offset = name_str.find( '.' );
    if ( offset != std::string::npos )
    {
      struct current_target_wrapper_expr_t : public target_wrapper_expr_t
      {
        mage_t& mage;

        current_target_wrapper_expr_t( action_t& action, const std::string& suffix_expr_str ) :
          target_wrapper_expr_t( action, "current_target_wrapper_expr_t", suffix_expr_str ),
          mage( static_cast<mage_t&>( *action.player ) )
        { }

        player_t* target() const override
        { assert( mage.current_target ); return mage.current_target; }
      };

      return new current_target_wrapper_expr_t( *a, name_str.substr( offset + 1 ) );
    }
    else
    {
      struct current_target_expr_t : public mage_expr_t
      {
        current_target_expr_t( const std::string& n, mage_t& m ) :
          mage_expr_t( n, m )
        { }

        double evaluate() override
        {
          assert( mage.current_target );
          return static_cast<double>( mage.current_target -> actor_index );
        }
      };

      return new current_target_expr_t( name_str, *this );
    }
  }

  // Default target expression support
  // "default_target" returns the actor id of the mage's default target (typically the first enemy
  // in the sim, e.g. fluffy_pillow)
  // "default_target.<some other expression>" evaluates <some other expression> on the default
  // target of the mage
  if ( util::str_compare_ci( name_str, "default_target" ) )
  {
    std::string::size_type offset = name_str.find( '.' );
    if ( offset != std::string::npos )
    {
      return target -> create_expression( a, name_str.substr( offset + 1 ) );
    }
    else
    {
      return make_ref_expr( name_str, target -> actor_index );
    }
  }

  // Incanters flow direction
  // Evaluates to:  0.0 if IF talent not chosen or IF stack unchanged
  //                1.0 if next IF stack increases
  //               -1.0 if IF stack decreases
  if ( name_str == "incanters_flow_dir" )
  {
    struct incanters_flow_dir_expr_t : public mage_expr_t
    {
      mage_t * mage;

      incanters_flow_dir_expr_t( mage_t& m ) :
        mage_expr_t( "incanters_flow_dir", m ), mage( &m )
      {}

      virtual double evaluate() override
      {
        if ( !mage -> talents.incanters_flow -> ok() )
          return 0.0;

        buff_t*flow = mage -> buffs.incanters_flow;
        if ( flow -> reverse )
          return flow -> current_stack == 1 ? 0.0: -1.0;
        else
          return flow -> current_stack == 5 ? 0.0: 1.0;
      }
    };

    return new incanters_flow_dir_expr_t( * this );
  }

  // Arcane Burn Flag Expression ==============================================
  if ( name_str == "burn_phase" )
  {
    struct burn_phase_expr_t : public mage_expr_t
    {
      burn_phase_expr_t( mage_t& m ) :
        mage_expr_t( "burn_phase", m )
      {}
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
      {}
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
          return static_cast<double>(mage.icicles.size());
        else
        {
          size_t icicles = 0;
          for ( int i = as<int>( mage.icicles.size() - 1 ); i >= 0; i-- )
          {
            if ( mage.sim -> current_time() - mage.icicles[ i ].timestamp >= mage.spec.icicles_driver -> duration() )
              break;

            icicles++;
          }

          return static_cast<double>(icicles);
        }
      }
    };

    return new icicles_expr_t( *this );
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
  // This is all a guess at how the hybrid primaries will work, since they
  // don't actually appear on cloth gear yet. TODO: confirm behavior
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
      range::find_if( icicles, [this, threshold]( const icicle_tuple_t& t ) {
        return ( sim->current_time() - t.timestamp ) < threshold;
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

    icicle -> base_dd_min = icicle -> base_dd_max = d.damage;

    actions::icicle_state_t* new_state = debug_cast<actions::icicle_state_t*>( icicle -> get_state() );
    new_state -> target = icicle_target;
    new_state -> source = d.stats;

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

//TODO: Whitelist these spell effects so we don't have to hardcode their damage values.
struct sorcerous_fireball_t : public spell_t
{
  sorcerous_fireball_t( mage_t* p ) :
    spell_t( "sorcerous_fireball", p )
  {
    background = true;
    may_crit = true;
    base_dd_min = base_dd_max = 246600;
    dot_duration = timespan_t::from_seconds( 5.0 );
    base_tick_time = timespan_t::from_seconds( 1.0 );
    base_td = 36990;
  }
  virtual double composite_crit_chance() const override
  { return 0.1; }
  virtual double composite_crit_chance_multiplier() const override
  { return 1.0; }
};

struct sorcerous_frostbolt_t : public spell_t
{
  sorcerous_frostbolt_t( mage_t* p ) :
    spell_t( "sorcerous_frostbolt", p )
  {
  background = true;
  may_crit = true;

  base_dd_min = base_dd_max = 406890;
  }
  virtual double composite_crit_chance() const override
  { return 0.1; }
  virtual double composite_crit_chance_multiplier() const override
  { return 1.0; }
};

struct sorcerous_arcane_blast_t : public spell_t
{
  sorcerous_arcane_blast_t( mage_t* p ) :
    spell_t( "sorcerous_arcane_blast", p )
  {
    background = true;
    may_crit = true;
    base_dd_min = base_dd_max = 431550;
  }
  virtual double composite_crit_chance() const override
  { return 0.1; }
  virtual double composite_crit_chance_multiplier() const override
  { return 1.0; }
};

struct sorcerous_shadowruby_pendant_driver_t : public spell_t
{
  unsigned current_roll;
  std::array<spell_t*, 3> sorcerous_spells;
  sorcerous_shadowruby_pendant_driver_t( const special_effect_t& effect ) :
    spell_t( "wanton_sorcery", effect.player, effect.player -> find_spell( 222276 ) )
  {
    callbacks = harmful = false;
    background = quiet = true;
    mage_t* p = debug_cast<mage_t*>( effect.player );
    sorcerous_spells[ 0 ] = new sorcerous_fireball_t( p );
    sorcerous_spells[ 1 ] = new sorcerous_frostbolt_t( p );
    sorcerous_spells[ 2 ] = new sorcerous_arcane_blast_t( p );
  }
  virtual void impact( action_state_t* s ) override
  {
    spell_t::impact( s );
    current_roll = static_cast<unsigned>( rng().range( 0, sorcerous_spells.size() ) );
    sorcerous_spells[ current_roll ] -> execute();
  }
};

static void sorcerous_shadowruby_pendant( special_effect_t& effect )
{
  effect.execute_action = new sorcerous_shadowruby_pendant_driver_t( effect );
}
// Mage Legendary Items
struct sephuzs_secret_t : public scoped_actor_callback_t<mage_t>
{
  sephuzs_secret_t(): super( MAGE )
  { }

  void manipulate( mage_t* actor, const special_effect_t& /* e */ ) override
  { actor -> legendary.sephuzs_secret = true; }
};
struct shard_of_the_exodar_t : public scoped_actor_callback_t<mage_t>
{
  shard_of_the_exodar_t() : super( MAGE )
  { }

  void manipulate( mage_t* actor, const special_effect_t& /* e */ ) override
  { actor -> legendary.shard_of_the_exodar = true;
    actor -> player_t::buffs.bloodlust -> default_chance = 0; }
};

// Arcane Legendary Items
struct mystic_kilt_of_the_rune_master_t : public scoped_action_callback_t<arcane_barrage_t>
{
  mystic_kilt_of_the_rune_master_t() : super( MAGE_ARCANE, "arcane_barrage" )
  { }

  void manipulate( arcane_barrage_t* action, const special_effect_t& e ) override
  { action -> mystic_kilt_of_the_rune_master_regen = e.driver() -> effectN( 1 ).percent(); }
};
struct rhonins_assaulting_armwraps_t : public scoped_action_callback_t<arcane_missiles_t>
{
  rhonins_assaulting_armwraps_t() : super( MAGE_ARCANE, "arcane_missiles" )
  { }

  void manipulate( arcane_missiles_t* action, const special_effect_t&  e ) override
  { action -> rhonins_assaulting_armwraps_proc_rate = e.driver() -> effectN( 1 ).percent(); }
};

struct cord_of_infinity_t : public scoped_actor_callback_t<mage_t>
{
  cord_of_infinity_t(): super( MAGE )
  { }

  void manipulate( mage_t* actor, const special_effect_t& /* e */) override
  { actor -> legendary.cord_of_infinity = true; }
};
// Fire Legendary Items
struct koralons_burning_touch_t : public scoped_action_callback_t<scorch_t>
{
  koralons_burning_touch_t() : super( MAGE_FIRE, "scorch" )
  { }

  void manipulate( scorch_t* action, const special_effect_t& e ) override
  { action -> koralons_burning_touch_multiplier = e.driver() -> effectN( 1 ).percent(); }

};

struct darcklis_dragonfire_diadem_t : public scoped_action_callback_t<dragons_breath_t>
{
  darcklis_dragonfire_diadem_t() : super( MAGE_FIRE, "dragons_breath" )
  { }

  void manipulate( dragons_breath_t* action, const special_effect_t& e ) override
  { action -> darcklis_dragonfire_diadem_multiplier = e.driver() -> effectN( 2 ).percent(); }
};

struct marquee_bindings_of_the_sun_king_t : public scoped_action_callback_t<pyroblast_t>
{
  marquee_bindings_of_the_sun_king_t() : super( MAGE_FIRE, "pyroblast" )
  { }

  void manipulate( pyroblast_t* action, const special_effect_t& e ) override
  { action ->  marquee_bindings_of_the_sun_king_proc_chance = e.driver() -> effectN( 1 ).percent(); }
};

// Frost Legendary Items
struct magtheridons_banished_bracers_t : public scoped_action_callback_t<ice_lance_t>
{
  magtheridons_banished_bracers_t() : super( MAGE_FROST, "ice_lance" )
  { }

  void manipulate( ice_lance_t* action, const special_effect_t& /* e */ ) override
  { action -> magtheridons_bracers = true; }
};

struct zannesu_journey_t : public scoped_actor_callback_t<mage_t>
{
  zannesu_journey_t() : super( MAGE_FROST )
  { }

  void manipulate( mage_t* actor, const special_effect_t& /* e */ ) override
  { actor -> legendary.zannesu_journey = true; }
};

struct lady_vashjs_grasp_t : public scoped_actor_callback_t<mage_t>
{
  lady_vashjs_grasp_t() : super( MAGE_FROST )
  { }

  void manipulate( mage_t* actor, const special_effect_t& /* e */ ) override
  { actor -> legendary.lady_vashjs_grasp = true; }
};

// 6.2 Class Trinkets

struct pyrosurge_t : public scoped_action_callback_t<fire_blast_t>
{
  pyrosurge_t() : super( MAGE_FIRE, "fire_blast" )
  { }

  void manipulate( fire_blast_t* action, const special_effect_t& e ) override
  { action -> pyrosurge_chance = ( e.driver() -> effectN( 1 ).average( e.item ) ) / 100; }
};

struct shatterlance_t : public scoped_actor_callback_t<mage_t>
{
  shatterlance_t() : super( MAGE_FROST )
  { }

  void manipulate( mage_t* actor, const special_effect_t& e ) override
  { actor -> legendary.shatterlance_effect = ( e.driver() -> effectN( 1 ).average( e.item ) ) / 100;
    actor -> legendary.shatterlance = true; }
};
// MAGE MODULE INTERFACE ====================================================

static void do_trinket_init( mage_t*                  p,
                             specialization_e         spec,
                             const special_effect_t*& ptr,
                             const special_effect_t&  effect )
{
  if ( !p -> find_spell( effect.spell_id ) -> ok() ||
       p -> specialization() != spec )
  {
    return;
  }

  ptr = &( effect );
}

static void wild_arcanist( special_effect_t& effect )
{
  mage_t* p = debug_cast<mage_t*>( effect.player );
  do_trinket_init( p, MAGE_ARCANE, p -> wild_arcanist, effect );
}

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
    unique_gear::register_special_effect( 184903, wild_arcanist                          );
    unique_gear::register_special_effect( 184904, pyrosurge_t()                          );
    unique_gear::register_special_effect( 184905, shatterlance_t()                       );
    unique_gear::register_special_effect( 209311, cord_of_infinity_t()                   );
    unique_gear::register_special_effect( 208099, koralons_burning_touch_t()             );
    unique_gear::register_special_effect( 214403, magtheridons_banished_bracers_t()      );
    unique_gear::register_special_effect( 206397, zannesu_journey_t()                    );
    unique_gear::register_special_effect( 208146, lady_vashjs_grasp_t()                  );
    unique_gear::register_special_effect( 208080, rhonins_assaulting_armwraps_t()        );
    unique_gear::register_special_effect( 207547, darcklis_dragonfire_diadem_t()         );
    unique_gear::register_special_effect( 208051, sephuzs_secret_t()                     );
    unique_gear::register_special_effect( 207970, shard_of_the_exodar_t()                );
    unique_gear::register_special_effect( 209450, marquee_bindings_of_the_sun_king_t()   );
    unique_gear::register_special_effect( 209280, mystic_kilt_of_the_rune_master_t()     );
    unique_gear::register_special_effect( 222276, sorcerous_shadowruby_pendant           );
  }

  virtual void register_hotfixes() const override
  { }

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
