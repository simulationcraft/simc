// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

using namespace residual_action;

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Mage
// ==========================================================================

struct mage_t;


enum mage_rotation_e { ROTATION_NONE = 0, ROTATION_DPS, ROTATION_DPM, ROTATION_MAX };
// Forcibly reset mage's current target, if it dies.
struct current_target_reset_cb_t
{
  mage_t* mage;

  current_target_reset_cb_t( player_t* m );

  void operator()(player_t*);
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

namespace pets {
struct water_elemental_pet_t;
}

// Icicle data, stored in an icicle container object. Contains a source stats object and the damage
typedef std::pair< double, stats_t* > icicle_data_t;
// Icicle container object, stored in a list to launch icicles at unsuspecting enemies!
typedef std::pair< timespan_t, icicle_data_t > icicle_tuple_t;

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
    buff_t* slow;

    buff_t* firestarter;

    buff_t* chilled,
          * frost_bomb,
          * water_jet; // Proxy Water Jet to compensate for expression system
  } debuffs;

  mage_td_t( player_t* target, mage_t* mage );
};

struct buff_stack_benefit_t
{
  const buff_t* buff;
  benefit_t** buff_stack_benefit;

  buff_stack_benefit_t( const buff_t* _buff,
                        const std::string& prefix ) :
    buff( _buff )
  {
    buff_stack_benefit = new benefit_t*[ buff -> max_stack() + 1 ];
    for ( int i = 0; i <= buff -> max_stack(); i++ )
    {
      buff_stack_benefit[ i ] = buff -> player ->
                                get_benefit( prefix + " " +
                                             buff -> data().name_cstr() + " " +
                                             util::to_string( i ) );
    }
  }

  virtual ~buff_stack_benefit_t()
  {
    delete[] buff_stack_benefit;
  }

  void update()
  {
    for ( int i = 0; i <= buff -> max_stack(); i++ )
    {
      buff_stack_benefit[ i ] -> update( i == buff -> check() );
    }
  }
};

struct buff_source_benefit_t;

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
  action_t* unstable_magic_explosion;
  player_t* last_bomb_target;

  // Artifact effects
  int scorched_earth_counter;

  // Tier 18 (WoD 6.2) trinket effects
  const special_effect_t* wild_arcanist; // Arcane
  const special_effect_t* pyrosurge;     // Fire
  const special_effect_t* shatterlance;  // Frost

  // State switches for rotation selection
  state_switch_t burn_phase;

  // Miscellaneous
  double cinder_count,
         distance_from_rune,
         incanters_flow_stack_mult,
         iv_haste,
         pet_multiplier;

  // Legendary Effects
  const special_effect_t* belovirs_final_stand,
                        * cord_of_infinity,
                        * darcklis_dragonfire_diadem,
                        * koralons_burning_touch,
                        * lady_vashjs_grasp,
                        * magtheridons_banished_bracers,
                        * marquee_bindings_of_the_sun_king,
                        * mythic_kilt_of_the_rune_master,
                        * norgannons_foresight,
                        * rhonins_assaulting_armwraps,
                        * shard_of_the_exodar,
                        * zannesu_journey;
                        
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
          * arcane_missiles,
          * arcane_power,
          * presence_of_mind,
          * arcane_affinity,       // T17 2pc Arcane
          * arcane_instability,    // T17 4pc Arcane
          * temporal_power,        // T18 4pc Arcane
          * quickening;

    stat_buff_t* mage_armor;

    // Fire
    buff_t* combustion,
          * enhanced_pyrotechnics,
          * heating_up,
          * hot_streak,
          * molten_armor,
          * pyretic_incantation,
          * pyromaniac,            // T17 4pc Fire
          * icarus_uprising;       // T18 4pc Fire

    // Frost
    buff_t* fingers_of_frost,
          * frost_armor,
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
              * inferno_blast,
              * phoenixs_flames,
              * presence_of_mind,
              * ray_of_frost;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* evocation;
  } gains;

  // Pets
  struct pets_t
  {
    pets::water_elemental_pet_t* water_elemental;

    pet_t** mirror_images;

    int temporal_hero_count = 10;
    pet_t** temporal_heroes;

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
           dpm_mana_gain;
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
    const spell_data_t* arcane_charge,
    // NOTE: arcane_charge_passive is the Arcane passive added in patch 5.2,
    //       called "Arcane Charge" (Spell ID: 114664).
    //       It contains Spellsteal's mana cost reduction, Evocation's mana
    //       gain reduction, and a deprecated Scorch damage reduction.
                      * arcane_charge_passive,
                      * arcane_familiar,
                      * mage_armor,
                      * savant;

    // Fire
    const spell_data_t* critical_mass,
                      * ignite,
                      * molten_armor;

    // Frost
    const spell_data_t* brain_freeze,
                      * fingers_of_frost,
                      * frost_armor,
                      * icicles,
                      * icicles_driver,
                      * shatter;
  } spec;

  // Talents
  struct talents_list_t
  {
    // Tier 15
    const spell_data_t* arcane_familiar,
                      * presence_of_mind,
                      * torrent,
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
                      * words_of_power,
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
                      * erosion, // NYI
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
                     touch_of_the_magi; // NYI

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
                     wings_of_flame;

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
                     chilled_to_the_core;
  } artifact;

public:
  mage_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, MAGE, name, r ),
    current_target( target ),
    icicle( nullptr ),
    icicle_event( nullptr ),
    ignite( nullptr ),
    ignite_spread_event( nullptr ),
    unstable_magic_explosion( nullptr ),
    last_bomb_target( nullptr ),
    scorched_earth_counter( 0 ),
    wild_arcanist( nullptr ),
    pyrosurge( nullptr ),
    shatterlance( nullptr ),
    belovirs_final_stand( nullptr ),
    cord_of_infinity( nullptr ),
    darcklis_dragonfire_diadem( nullptr ),
    koralons_burning_touch( nullptr ),
    lady_vashjs_grasp( nullptr ),
    magtheridons_banished_bracers( nullptr ),
    marquee_bindings_of_the_sun_king( nullptr ),
    mythic_kilt_of_the_rune_master( nullptr ),
    norgannons_foresight( nullptr ),
    rhonins_assaulting_armwraps( nullptr ),
    shard_of_the_exodar( nullptr ),
    zannesu_journey( nullptr ),
    cinder_count( 6.0 ),
    distance_from_rune( 0.0 ),
    incanters_flow_stack_mult( find_spell( 116267 ) -> effectN( 1 ).percent() ),
    iv_haste( 1.0 ),
    pet_multiplier( 1.0 ),
    benefits( benefits_t() ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
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
    cooldowns.inferno_blast    = get_cooldown( "inferno_blast"    );
    cooldowns.phoenixs_flames  = get_cooldown( "phoenixs_flames"  );
    cooldowns.presence_of_mind = get_cooldown( "presence_of_mind" );
    cooldowns.ray_of_frost     = get_cooldown( "ray_of_frost"     );

    // Options
    base.distance = 40;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_SPELL_HASTE ] = true;

    // Forcibly reset mage's current target, if it dies.
    struct current_target_reset_cb_t
    {
      mage_t* mage;

      current_target_reset_cb_t( mage_t* m ) : mage( m )
      { }

      void operator()(player_t*)
      {
        for ( size_t i = 0, end = mage -> sim -> target_non_sleeping_list.size(); i < end; ++i )
        {
          // If the mage's current target is still alive, bail out early.
          if ( mage -> current_target == mage -> sim -> target_non_sleeping_list[ i ] )
          {
            return;
          }
        }

        if ( mage -> sim -> debug )
        {
          mage -> sim -> out_debug.printf( "%s current target %s died. Resetting target to %s.",
              mage -> name(), mage -> current_target -> name(), mage -> target -> name() );
        }

        mage -> current_target = mage -> target;
      }
    };
  }

  // Character Definition
  virtual void      init_spells() override;
  virtual void      init_base_stats() override;
  virtual void      create_buffs() override;
  virtual void      create_options() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init_benefits() override;
  virtual void      init_stats() override;
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
  virtual double    composite_spell_crit() const override;
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

  // Public mage functions:
  icicle_data_t get_icicle_object();
  void trigger_icicle( const action_state_t* trigger_state, bool chain = false, player_t* chain_target = nullptr );

  void              apl_precombat();
  void              apl_arcane();
  void              apl_fire();
  void              apl_frost();
  void              apl_default();
  virtual void      init_action_list() override;

  virtual bool      has_t18_class_trinket() const override;
  std::string       get_potion_action();
};

struct buff_source_benefit_t
{
  const buff_t* buff;
  int trigger_count;
  std::vector<std::string> sources;
  std::vector<benefit_t*> buff_source_benefit;

  buff_source_benefit_t( const buff_t* _buff ) :
    buff( _buff ), trigger_count( 0 )
  { }

  void update( const std::string& source, int stacks = 1 )
  {
    mage_t* mage = static_cast<mage_t*>( buff -> player );

    // Update old sources
    int index = -1;
    for ( size_t i = 0; i < sources.size(); i++ )
    {
      bool source_matches = ( sources[i] == source );
      if ( source_matches )
      {
        index = as<int>( i );
      }

      for ( int j = 0; j < stacks; j++ )
      {
        buff_source_benefit[i] -> update( source_matches );
      }
    }

    if ( index == -1 )
    {
      // Add new source
      sources.push_back( source );

      std::string benefit_name = std::string( buff -> data().name_cstr() ) +
                                 " from " + source;
      benefit_t* source_benefit = mage -> get_benefit( benefit_name );
      for ( int i = 0; i < trigger_count; i++ )
      {
        source_benefit -> update( false );
      }
      for ( int i = 0; i < stacks; i++ )
      {
        source_benefit -> update( true );
      }

      buff_source_benefit.push_back( source_benefit );
    }

    trigger_count += stacks;
  }
};

inline current_target_reset_cb_t::current_target_reset_cb_t( player_t* m ):
  mage( debug_cast<mage_t*>( m ) )
{ }

inline void current_target_reset_cb_t::operator()(player_t*)
{
  for ( size_t i = 0, end = mage -> sim -> target_non_sleeping_list.size(); i < end; ++i )
  {
    // If the mage's current target is still alive, bail out early.
    if ( mage -> current_target == mage -> sim -> target_non_sleeping_list[ i ] )
    {
      return;
    }
  }

  if ( mage -> sim -> debug )
  {
    mage -> sim -> out_debug.printf( "%s current target %s died. Resetting target to %s.",
        mage -> name(), mage -> current_target -> name(), mage -> target -> name() );
  }

  mage -> current_target = mage -> target;
}

namespace pets {

struct mage_pet_spell_t : public spell_t
{
  mage_pet_spell_t( const std::string& n, pet_t* p, const spell_data_t* s ):
    spell_t( n, p, s )
  {}

  mage_t* o()
  {
    pet_t* pet = static_cast< pet_t* >( player );
    mage_t* mage = static_cast< mage_t* >( pet -> owner );
    return mage;
  }

  virtual void schedule_execute( action_state_t* execute_state ) override
  {
    target = o() -> current_target;

    spell_t::schedule_execute( execute_state );
  }
};

struct mage_pet_melee_attack_t : public melee_attack_t
{
  mage_pet_melee_attack_t( const std::string& n, pet_t* p ):
    melee_attack_t( n, p, spell_data_t::nil() )
  {}

  mage_t* o()
  {
    pet_t* pet = static_cast< pet_t* >( player );
    mage_t* mage = static_cast< mage_t* >( pet -> owner );
    return mage;
  }
  virtual void schedule_execute( action_state_t* execute_state ) override
  {
    target = o() -> current_target;

    melee_attack_t::schedule_execute( execute_state );
  }
};

//================================================================================
// Pet Arcane Familiar
//================================================================================
struct arcane_familiar_pet_t : public pet_t
{
  struct arcane_assault_t : public mage_pet_spell_t
  {
    arcane_assault_t( arcane_familiar_pet_t* p, const std::string& options_str ) :
      mage_pet_spell_t( "arcane_assault", p, p -> find_spell( 205235 ) )
    {
      parse_options( options_str );
      spell_power_mod.direct = p -> find_spell( 225119 ) -> effectN( 1 ).sp_coeff();
    }
  };
  void arise() override
  {
    mage_t* m = debug_cast<mage_t*>( owner );
    pet_t::arise();

    m -> recalculate_resource_max( RESOURCE_MANA );
  }

  void demise() override
  {
    mage_t* m = debug_cast<mage_t*>( owner );

    pet_t::demise();
    m -> recalculate_resource_max( RESOURCE_MANA );

  }
  arcane_familiar_pet_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "arcane_familiar", true )
  {
    owner_coeff.sp_from_sp = 1.00;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
  {
    if ( name == "arcane_assault" ) return new arcane_assault_t( this, options_str );
    return pet_t::create_action( name, options_str );
  }

  mage_t* o() const
  { return static_cast<mage_t*>( owner ); }

  virtual void init_action_list() override
  {
    action_list_str = "arcane_assault";

    pet_t::init_action_list();
  }
};

// ==========================================================================
// Pet Water Elemental
// ==========================================================================
struct water_elemental_pet_t;

struct water_elemental_pet_td_t: public actor_target_data_t
{
  buff_t* water_jet;
public:
  water_elemental_pet_td_t( player_t* target, water_elemental_pet_t* welly );
};
struct water_elemental_pet_t : public pet_t
{
  struct freeze_t : public mage_pet_spell_t
  {
    freeze_t( water_elemental_pet_t* p, const std::string& options_str ):
      mage_pet_spell_t( "freeze", p, p -> find_pet_spell( "Freeze" ) )
    {
      parse_options( options_str );
      aoe = -1;
      may_crit = true;
      ignore_false_positive = true;
      action_skill = 1;
    }

    virtual void impact( action_state_t* s ) override
    {
      spell_t::impact( s );

      water_elemental_pet_t* p = static_cast<water_elemental_pet_t*>( player );

      if ( result_is_hit( s -> result ) )
      {
        p -> o() -> buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
        p -> o() -> benefits.fingers_of_frost -> update( "Water Jet" );
      }
    }
  };

 struct water_jet_t : public mage_pet_spell_t
   {
     // queued water jet spell, auto cast water jet spell
     bool queued, autocast;

     water_jet_t( water_elemental_pet_t* p, const std::string& options_str ) :
       mage_pet_spell_t( "water_jet", p, p -> find_spell( 135029 ) ),
       queued( false ), autocast( true )
     {
       parse_options( options_str );
       channeled = tick_may_crit = true;

       if ( p -> o() -> sets.has_set_bonus( MAGE_FROST, T18, B4 ) )
       {
         dot_duration += p -> find_spell( 185971 ) -> effectN( 1 ).time_value();
       }
     }
     water_elemental_pet_td_t* td( player_t* t ) const
     { return p() -> get_target_data( t ? t : target ); }

    water_elemental_pet_t* p()
    { return static_cast<water_elemental_pet_t*>( player ); }

    const water_elemental_pet_t* p() const
    { return static_cast<water_elemental_pet_t*>( player ); }

    void execute() override
    {
      mage_pet_spell_t::execute();
      // If this is a queued execute, disable queued status
      if ( ! autocast && queued )
        queued = false;
    }

    virtual void impact( action_state_t* s ) override
    {
      mage_pet_spell_t::impact( s );

      td( s -> target ) -> water_jet
                        -> trigger(1, buff_t::DEFAULT_VALUE(), 1.0,
                                   dot_duration *
                                   p() -> composite_spell_haste());

      // Trigger hidden proxy water jet for the mage, so
      // debuff.water_jet.<expression> works
      p() -> o() -> get_target_data( s -> target ) -> debuffs.water_jet
          -> trigger(1, buff_t::DEFAULT_VALUE(), 1.0,
                     dot_duration * p() -> composite_spell_haste());
    }

    virtual double action_multiplier() const override
    {
      double am = mage_pet_spell_t::action_multiplier();

      if ( p() -> o() -> spec.icicles -> ok() )
      {
        am *= 1.0 + p() -> o() -> cache.mastery_value();
      }

      return am;
    }

    virtual void last_tick( dot_t* d ) override
    {
      mage_pet_spell_t::last_tick( d );
      td( d -> target ) -> water_jet -> expire();
    }

    bool ready() override
    {
      // Not ready, until the owner gives permission to cast
      if ( ! autocast && ! queued )
        return false;

      return mage_pet_spell_t::ready();
    }

    void reset() override
    {
      mage_pet_spell_t::reset();

      queued = false;
    }
  };

  water_elemental_pet_td_t* td( player_t* t ) const
  { return get_target_data( t ); }

  target_specific_t<water_elemental_pet_td_t> target_data;

  virtual water_elemental_pet_td_t* get_target_data( player_t* target ) const override
  {
    water_elemental_pet_td_t*& td = target_data[ target ];
    if ( ! td )
      td = new water_elemental_pet_td_t( target, const_cast<water_elemental_pet_t*>(this) );
    return td;
  }
  struct waterbolt_t: public mage_pet_spell_t
  {
    waterbolt_t( water_elemental_pet_t* p, const std::string& options_str ):
      mage_pet_spell_t( "waterbolt", p, p -> find_pet_spell( "Waterbolt" ) )
    {
      trigger_gcd = timespan_t::zero();
      parse_options( options_str );
      may_crit = true;
    }

    const water_elemental_pet_t* p() const
    { return static_cast<water_elemental_pet_t*>( player ); }

    virtual double action_multiplier() const override
    {
      double am = mage_pet_spell_t::action_multiplier();

      if ( p() -> o() -> spec.icicles -> ok() )
      {
        am *= 1.0 + p() -> o() -> cache.mastery_value();
      }

      if ( p() -> o() -> artifact.its_cold_outside.rank() )
      {
        am *= 1.0 + p() -> o() -> artifact.its_cold_outside.data().effectN( 3 ).percent();
      }
      return am;
    }

    virtual void impact( action_state_t* s ) override
    {
      water_elemental_pet_t* p = static_cast<water_elemental_pet_t*>( player );

      double fof_chance = p -> o() -> artifact.its_cold_outside.percent();


      spell_t::impact( s );

      if ( result_is_hit( s -> result ) && rng().roll( fof_chance ) )
      {
        p -> o() -> buffs.fingers_of_frost -> trigger();
        p -> o() -> benefits.fingers_of_frost -> update( "Waterbolt Proc", 1.0 );
      }
    }
  };

  water_elemental_pet_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "water_elemental" )
  {
    owner_coeff.sp_from_sp = 0.75;
    action_list_str  = "water_jet/waterbolt";
  }

  mage_t* o()
  { return static_cast<mage_t*>( owner ); }
  const mage_t* o() const
  { return static_cast<mage_t*>( owner ); }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
  {
    if ( name == "freeze"       ) return new              freeze_t( this, options_str );
    if ( name == "waterbolt"    ) return new           waterbolt_t( this, options_str );
    if ( name == "water_jet"    ) return new           water_jet_t( this, options_str );
    return pet_t::create_action( name, options_str );
  }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( o() -> buffs.rune_of_power -> check() )
    {
      m *= 1.0 + o() -> buffs.rune_of_power -> data().effectN( 3 ).percent();
    }

    if ( o() -> talents.incanters_flow -> ok() )
    {
      m *= 1.0 + o() -> buffs.incanters_flow -> current_stack *
                 o() -> incanters_flow_stack_mult;
    }

    m *= o() -> pet_multiplier;

    return m;
  }
};
water_elemental_pet_td_t::water_elemental_pet_td_t( player_t* target, water_elemental_pet_t* welly ) :
  actor_target_data_t( target, welly )
{
  water_jet = buff_creator_t( *this, "water_jet", welly -> find_spell( 135029 ) ).cd( timespan_t::zero() );
}

// ==========================================================================
// Pet Mirror Image
// ==========================================================================

struct mirror_image_pet_t : public pet_t
{
  struct mirror_image_spell_t : public mage_pet_spell_t
  {
    mirror_image_spell_t( const std::string& n, mirror_image_pet_t* p, const spell_data_t* s ):
      mage_pet_spell_t( n, p, s )
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
    { return static_cast<mirror_image_pet_t*>( player ); }
  };

  struct arcane_blast_t : public mirror_image_spell_t
  {
    arcane_blast_t( mirror_image_pet_t* p, const std::string& options_str ):
      mirror_image_spell_t( "arcane_blast", p, p -> find_pet_spell( "Arcane Blast" ) )
    {
      parse_options( options_str );
    }

    virtual void execute() override
    {
      mirror_image_spell_t::execute();

      p() -> arcane_charge -> trigger();
    }

    virtual double action_multiplier() const override
    {
      double am = mirror_image_spell_t::action_multiplier();

      // MI Arcane Charges are still hardcoded as 25% damage gains
      am *= 1.0 + p() -> arcane_charge -> check() * 0.25;

      return am;
    }
  };

  struct fireball_t : public mirror_image_spell_t
  {
    fireball_t( mirror_image_pet_t* p, const std::string& options_str ):
      mirror_image_spell_t( "fireball", p, p -> find_pet_spell( "Fireball" ) )
    {
      parse_options( options_str );
    }
  };

  struct frostbolt_t : public mirror_image_spell_t
  {
    frostbolt_t( mirror_image_pet_t* p, const std::string& options_str ):
      mirror_image_spell_t( "frostbolt", p, p -> find_pet_spell( "Frostbolt" ) )
    {
      parse_options( options_str );
    }
  };

  buff_t* arcane_charge;

  mirror_image_pet_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "mirror_image", true ),
    arcane_charge( nullptr )
  {
    owner_coeff.sp_from_sp = 1.00;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
  {
    if ( name == "arcane_blast" ) return new arcane_blast_t( this, options_str );
    if ( name == "fireball"     ) return new     fireball_t( this, options_str );
    if ( name == "frostbolt"    ) return new    frostbolt_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }

  mage_t* o() const
  { return static_cast<mage_t*>( owner ); }

  virtual void init_action_list() override
  {

      if ( o() -> specialization() == MAGE_FIRE )
      {
        action_list_str = "fireball";
      }
      else if ( o() -> specialization() == MAGE_ARCANE )
      {
        action_list_str = "arcane_blast";
      }
      else
      {
        action_list_str = "frostbolt";
      }

    pet_t::init_action_list();
  }

  virtual void create_buffs() override
  {
    pet_t::create_buffs();

    arcane_charge = buff_creator_t( this, "arcane_charge",
                                    o() -> spec.arcane_charge );
  }


  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    m *= o() -> pet_multiplier;

    return m;
  }
};


// ==========================================================================
// Pet Temporal Heroes (2T18)
// ==========================================================================

struct temporal_hero_t : public pet_t
{
  enum hero_e
  {
    ARTHAS,
    JAINA,
    SYLVANAS,
    TYRANDE
  };

  hero_e hero_type;
  static hero_e last_summoned;

  struct temporal_hero_melee_attack_t : public mage_pet_melee_attack_t
  {
    temporal_hero_melee_attack_t( pet_t* p ) :
      mage_pet_melee_attack_t( "melee", p )
    {
      may_crit = true;
      background = repeating = auto_attack = true;
      school = SCHOOL_PHYSICAL;
      special = false;
      weapon = &( p -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
    }

    bool init_finished() override
    {
      pet_t* p = debug_cast<pet_t*>( player );
      mage_t* m = debug_cast<mage_t*>( p -> owner );

      if ( m -> pets.temporal_heroes[0] )
      {
        stats = m -> pets.temporal_heroes[0] -> get_stats( "melee" );
      }

      return mage_pet_melee_attack_t::init_finished();
    }
  };

  struct temporal_hero_autoattack_t : public mage_pet_melee_attack_t
  {
    temporal_hero_autoattack_t( pet_t* p ) :
      mage_pet_melee_attack_t( "auto_attack", p )
    {
      p -> main_hand_attack = new temporal_hero_melee_attack_t( p );
      trigger_gcd = timespan_t::zero();
    }

    virtual void execute() override
    {
      player -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready() override
    {
      temporal_hero_t* hero = debug_cast<temporal_hero_t*>( player );
      if ( hero -> hero_type != ARTHAS )
      {
        return false;
      }

      return player -> main_hand_attack -> execute_event == nullptr;
    }
  };

  struct temporal_hero_frostbolt_t : public mage_pet_spell_t
  {
    temporal_hero_frostbolt_t( pet_t* p ) :
      mage_pet_spell_t( "frostbolt", p, p -> find_spell( 191764 ) )
    {
      base_dd_min = base_dd_max = 2750.0;
      may_crit = true;
    }

    virtual bool ready() override
    {
      temporal_hero_t* hero = debug_cast<temporal_hero_t*>( player );
      if ( hero -> hero_type != JAINA )
      {
        return false;
      }

      return mage_pet_spell_t::ready();
    }

    bool init_finished() override
    {
      pet_t* p = debug_cast<pet_t*>( player );
      mage_t* m = debug_cast<mage_t*>( p -> owner );

      if ( m -> pets.temporal_heroes[0] )
      {
        stats = m -> pets.temporal_heroes[0] -> get_stats( "frostbolt" );
      }

      return mage_pet_spell_t::init_finished();
    }
  };

  struct temporal_hero_shoot_t : public mage_pet_spell_t
  {
    temporal_hero_shoot_t( pet_t* p ) :
      mage_pet_spell_t( "shoot", p, p -> find_spell( 191799 ) )
    {
      school = SCHOOL_PHYSICAL;
      base_dd_min = base_dd_max = 3255.19;
      base_execute_time = p -> main_hand_weapon.swing_time;
      may_crit = true;
    }

    virtual bool ready() override
    {
      temporal_hero_t* hero = debug_cast<temporal_hero_t*>( player );
      if ( hero -> hero_type != SYLVANAS && hero -> hero_type != TYRANDE )
      {
        return false;
      }

      return player -> main_hand_attack -> execute_event == nullptr;
    }

    bool init_finished() override
    {
      pet_t* p = debug_cast<pet_t*>( player );
      mage_t* m = debug_cast<mage_t*>( p -> owner );

      if ( m -> pets.temporal_heroes[0] )
      {
        stats = m -> pets.temporal_heroes[0] -> get_stats( "shoot" );
      }

      return mage_pet_spell_t::init_finished();
    }
  };

  // Each Temporal Hero has base damage and hidden multipliers for damage done
  // Values are reverse engineered from 6.2 PTR Build 20157 testing
  double temporal_hero_multiplier;

  temporal_hero_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "temporal_hero", true, true ),
    hero_type( ARTHAS ), temporal_hero_multiplier( 1.0 )
  { }

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
                           const std::string& options_str ) override
  {
    if ( name == "auto_attack" ) return new temporal_hero_autoattack_t( this );
    if ( name == "frostbolt" )   return new  temporal_hero_frostbolt_t( this );
    if ( name == "shoot" )       return new      temporal_hero_shoot_t( this );

    return base_t::create_action( name, options_str );
  }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    // Temporal Hero benefits from Temporal Power applied by itself (1 stack).
    // Using owner's buff object, in order to avoid creating a separate buff_t
    // for each pet instance, and merging the buff statistics.
    if ( owner -> sets.has_set_bonus( MAGE_ARCANE, T18, B4 ) )
    {
      mage_t* mage = debug_cast<mage_t*>( owner );
      m *= 1.0 + mage -> buffs.temporal_power -> data().effectN( 1 ).percent();
    }

    m *= temporal_hero_multiplier;

    return m;
  }

  void arise() override
  {
    pet_t::arise();

    // Summoned heroes follow Jaina -> Arthas -> Sylvanas -> Tyrande order
    if ( last_summoned == JAINA )
    {
      hero_type = ARTHAS;
      last_summoned = hero_type;
      temporal_hero_multiplier = 0.1964;

      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s summons 2T18 temporal hero: Arthas",
                                 owner -> name() );
      }
    }
    else if ( last_summoned == TYRANDE )
    {
      hero_type = JAINA;
      last_summoned = hero_type;
      temporal_hero_multiplier = 0.6;

      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s summons 2T18 temporal hero: Jaina" ,
                                 owner -> name() );
      }
    }
    else if ( last_summoned == ARTHAS )
    {
      hero_type = SYLVANAS;
      last_summoned = hero_type;
      temporal_hero_multiplier = 0.5283;

      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s summons 2T18 temporal hero: Sylvanas",
                                 owner -> name() );
      }
    }
    else
    {
      hero_type = TYRANDE;
      last_summoned = hero_type;
      temporal_hero_multiplier = 0.5283;

      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s summons 2T18 temporal hero: Tyrande",
                                 owner -> name() );
      }
    }

    if ( owner -> sets.has_set_bonus( MAGE_ARCANE, T18, B4 ) )
    {
      mage_t* m = debug_cast<mage_t*>( owner );
      m -> buffs.temporal_power -> trigger();
    }

    owner -> invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  void demise() override
  {
    pet_t::demise();

    mage_t* m = debug_cast<mage_t*>( owner );
    m -> buffs.temporal_power -> decrement();

    owner -> invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  static void randomize_last_summoned( const mage_t* p )
  {
    double rand = p -> rng().real();
    if ( rand < 0.25 )
    {
      last_summoned = ARTHAS;
    }
    else if ( rand < 0.5 )
    {
      last_summoned = JAINA;
    }
    else if ( rand < 0.75 )
    {
      last_summoned = SYLVANAS;
    }
    else
    {
      last_summoned = TYRANDE;
    }
  }
};

temporal_hero_t::hero_e temporal_hero_t::last_summoned;

} // pets

// Arcane Missiles Buff =======================================================
struct arcane_missiles_buff_t : public buff_t
{
  arcane_missiles_buff_t( mage_t* p ) :
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

namespace actions {
// ==========================================================================
// Mage Spell
// ==========================================================================

struct mage_spell_t : public spell_t
{
  bool consumes_ice_floes,
       frozen,
       may_proc_missiles;

public:
  int dps_rotation,
      dpm_rotation;

  mage_spell_t( const std::string& n, mage_t* p,
                const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( n, p, s ),
    consumes_ice_floes( true ),
    frozen( false ),
    dps_rotation( 0 ),
    dpm_rotation( 0 )
  {
    may_proc_missiles = harmful && !background;
    may_crit      = true;
    tick_may_crit = true;
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

  virtual double composite_crit_multiplier() const override
  {
    double m = spell_t::composite_crit_multiplier();

    if ( frozen && p() -> spec.shatter -> ok() )
    {
      m *= 1.0 + p() -> spec.shatter -> effectN( 2 ).percent();
    }

    return m;
  }

  void snapshot_internal( action_state_t* s, uint32_t flags,
                          dmg_e rt ) override
  {
    spell_t::snapshot_internal( s, flags, rt );

    // Shatter's +50% crit bonus needs to be added after multipliers etc
    if ( ( flags & STATE_CRIT ) && frozen && p() -> spec.shatter -> ok() )
    {
      s -> crit += p() -> spec.shatter -> effectN( 2 ).percent();
    }
  }

  // You can thank Frost Nova for why this isn't in arcane_mage_spell_t instead
  void trigger_am( const std::string& source, double chance = -1.0,
                   int stacks = 1 )
  {
    if ( p() -> buffs.arcane_missiles
             -> trigger( stacks, buff_t::DEFAULT_VALUE(), chance ) )
    {
      p() -> benefits.arcane_missiles -> update( source, stacks );
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
         may_proc_missiles )
    {
      trigger_am( data().name_cstr() );
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
//

struct arcane_mage_spell_t : public mage_spell_t
{
  arcane_mage_spell_t( const std::string& n, mage_t* p,
                       const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s )
  {}

  double arcane_charge_damage_bonus() const
  {
    double per_ac_bonus = p() -> spec.arcane_charge -> effectN( 1 ).percent() +
                          ( p() -> cache.mastery_value() *
                           p() -> spec.savant -> effectN( 2 ).sp_coeff() );
    return 1.0 + p() -> buffs.arcane_charge -> check() * per_ac_bonus;

  };

  virtual double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

      if ( p() -> artifact.might_of_the_guardians && school == SCHOOL_ARCANE )
      {
         am *= 1.0 + p() -> artifact.might_of_the_guardians.percent();
      }

    return am;
  }

  virtual timespan_t gcd() const override
  {
    timespan_t t = mage_spell_t::gcd();
    return t;
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
  bool triggers_hot_streak,
       triggers_ignite;

  fire_mage_spell_t( const std::string& n, mage_t* p,
                     const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s ),
    triggers_hot_streak( false ),
    triggers_ignite( false )
  {
    base_multiplier *= 1.0 + p -> artifact.burning_gaze.percent();
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( triggers_ignite && result_is_hit( s -> result ) )
    {
      trigger_ignite( s );
    }

    if ( triggers_hot_streak && result_is_hit( s -> result ) )
    {
      handle_hot_streak( s );
    }

    if ( result_is_hit( s -> result ) && s -> result == RESULT_CRIT
                      && p() -> artifact.pyretic_incantation.rank()
                      && harmful == true
                      && background == false )
    {

      p() -> buffs.pyretic_incantation -> trigger();
    }
    else if ( result_is_hit( s -> result ) && s -> result != RESULT_CRIT
                      && p() -> artifact.pyretic_incantation.rank()
                      && harmful == true
                      && background == false )
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

  double total_crit_bonus() const override
  {
    // TODO: Only apply bonus to hardcast spells?
    double bonus = mage_spell_t::total_crit_bonus();
    if ( background == true )
    {
      return bonus;
    }

    if ( p() -> buffs.pyretic_incantation -> stack() > 0 )
    {
      bonus *= 1.0 + ( p() -> artifact.pyretic_incantation.percent() *
                       p() -> buffs.pyretic_incantation -> stack() );
    }
    return bonus;
  }
  void trigger_ignite( action_state_t* s )
  {
    mage_t* p = this -> p();

    double amount = s -> result_amount * p -> cache.mastery_value();

    ignite_spell_state_t* is = static_cast<ignite_spell_state_t*>( s );
    if ( is -> hot_streak )
    {
      // TODO: Use client data from hot streak
      amount *= 2.0;
    }

    amount *= 1.0 + p -> artifact.everburning_consumption.percent();

    bool ignite_exists = p -> ignite -> get_dot( s -> target ) -> is_ticking();

    trigger( p -> ignite, s -> target, amount );

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
    return am;
  }
};


// ============================================================================
// Frost Mage Spell
// ============================================================================
//

struct frost_mage_spell_t : public mage_spell_t
{
  bool chills;

  frost_mage_spell_t( const std::string& n, mage_t* p,
                      const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s ), chills( false )
  {}

  void trigger_fof( const std::string& source, double chance, int stacks = 1 )
  {
    bool success = p() -> buffs.fingers_of_frost
                       -> trigger( stacks, buff_t::DEFAULT_VALUE(), chance );

    if ( success )
    {
      p() -> benefits.fingers_of_frost -> update( source, stacks );
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

    icicle_tuple_t tuple = icicle_tuple_t( p() -> sim -> current_time(),
                                           icicle_data_t( amount, stats ) );
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
    double am = mage_spell_t::action_multiplier();

    // Divide effect percent by 10 to convert 5 into 0.5%, not into 5%.
    am *= 1.0 + ( p() -> buffs.bone_chilling -> current_stack * p() -> talents.bone_chilling -> effectN( 1 ).percent() / 10 );

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

  virtual double action_multiplier() const override
  {
    // Ignores all regular multipliers
    double am = 1.0;
    return am;
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
    may_proc_missiles = false;
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
    callbacks = false;
    background = true;
    base_costs[ RESOURCE_MANA ] = 0;
    trigger_gcd = timespan_t::zero();
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
  phoenix_reborn_t( mage_t* p ) :
    fire_mage_spell_t( "phoenix_reborn", p, p -> artifact.phoenix_reborn )
  {
    spell_power_mod.direct  = p -> find_spell( 215775 ) -> effectN( 1 ).sp_coeff();
    trigger_gcd =  timespan_t::zero();
    base_costs[ RESOURCE_MANA ] = 0;
    callbacks = false;
    background = true;
  }

  virtual void execute() override
  {
    fire_mage_spell_t::execute();
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
         rng().roll( p() -> artifact.phoenix_reborn.data().proc_chance() ) )
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

  arcane_barrage_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_barrage", p, p -> find_class_spell( "Arcane Barrage" ) ),
    arcane_rebound( new arcane_rebound_t( p, options_str ) )
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

    if ( p() -> talents.torrent -> ok() )
    {
      int targets = std::min( n_targets(), as<int>( target_list().size() ) );
      am *= 1.0 + p() -> talents.torrent -> effectN( 1 ).percent() * targets;
    }

    return am;
  }
};


// Arcane Blast Spell =======================================================

struct arcane_blast_t : public arcane_mage_spell_t
{
  double wild_arcanist_effect;

  arcane_blast_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_blast", p,
                         p -> find_class_spell( "Arcane Blast" ) ),
    wild_arcanist_effect( 0.0 )
  {
    parse_options( options_str );
    may_proc_missiles = false; // Disable default AM proc logic.
    base_multiplier *= 1.0 + p -> artifact.blasting_rod.percent();

    if ( p -> wild_arcanist )
    {
      const spell_data_t* data = p -> wild_arcanist -> driver();
      wild_arcanist_effect = std::fabs( data -> effectN( 1 ).average( p -> wild_arcanist -> item ) );
      wild_arcanist_effect /= 100.0;
    }
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

    return c;
  }

  virtual void execute() override
  {
    p() -> benefits.arcane_charge.arcane_blast -> update();

    arcane_mage_spell_t::execute();

    p() -> buffs.arcane_charge -> up();
    p() -> buffs.arcane_affinity -> up();

    if ( result_is_hit( execute_state -> result ) )
    {
      arcane_missiles_buff_t* am_buff =
        debug_cast<arcane_missiles_buff_t*>( p() -> buffs.arcane_missiles );
      trigger_am( "Arcane Blast", am_buff -> proc_chance() * 2.0 );

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

    if ( p() -> talents.quickening -> ok() )
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
    if ( p() -> talents.quickening -> ok() )
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
  }
};

struct arcane_missiles_t : public arcane_mage_spell_t
{
  timespan_t temporal_hero_duration;

  arcane_missiles_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_missiles", p,
                         p -> find_class_spell( "Arcane Missiles" ) ),
    temporal_hero_duration( timespan_t::zero() )
  {
    parse_options( options_str );
    may_miss = false;
    may_proc_missiles = false;
    dot_duration      = data().duration();
    base_tick_time    = data().effectN( 2 ).period();
    channeled         = true;
    hasted_ticks      = false;
    dynamic_tick_action = true;
    tick_action = new arcane_missiles_tick_t( p );
    may_miss = false;

    temporal_hero_duration = p -> find_spell( 188117 ) -> duration();

    base_multiplier *= 1.0 + p -> artifact.aegwynns_fury.percent();
  }

  virtual double action_multiplier() const override
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


  virtual void execute() override
  {
    p() -> benefits.arcane_charge.arcane_missiles -> update();

    // Reset base_tick_time to default before applying 4T17/Rule of Threes
    base_tick_time = data().effectN( 2 ).period();

    // 4T17 : Increase the number of missiles by reducing base_tick_time
    if ( p() -> buffs.arcane_instability -> up() )
    {
      base_tick_time *= 1 + p() -> buffs.arcane_instability
                                -> data().effectN( 1 ).percent() ;
      p() -> buffs.arcane_instability -> expire();
    }


    if ( p() -> artifact.rule_of_threes.rank() &&
         rng().roll( p() -> artifact.rule_of_threes
         .data().effectN( 1 ).percent() / 10 ) )
    {
      base_tick_time *=  1.0 - ( p() -> artifact.rule_of_threes.data().effectN( 1 ).percent() / 10 );
    }
    arcane_mage_spell_t::execute();

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
      for ( unsigned i = 0; p() -> pets.temporal_hero_count; i++ )
      {
        if ( p() -> pets.temporal_heroes[ i ] -> is_sleeping() )
        {
          p() -> pets.temporal_heroes[ i ] -> summon( temporal_hero_duration );
          break;
        }
      }
    }

    p() -> buffs.arcane_missiles -> decrement();

    if ( p() -> talents.quickening -> ok() )
    {
      p() -> buffs.quickening -> trigger();
    }
  }

  virtual void last_tick ( dot_t * d ) override
  {
    arcane_mage_spell_t::last_tick( d );

    p() -> buffs.arcane_charge -> trigger();

    p() -> buffs.arcane_instability -> trigger();
  }

  virtual bool ready() override
  {
    if ( ! p() -> buffs.arcane_missiles -> check() )
      return false;

    return arcane_mage_spell_t::ready();
  }
};


// Arcane Orb Spell =========================================================

struct arcane_orb_bolt_t : public arcane_mage_spell_t
{
  arcane_orb_bolt_t( mage_t* p ) :
    arcane_mage_spell_t( "arcane_orb_bolt", p, p -> find_spell( 153640 ) )
  {
    aoe = -1;
    background = true;
    dual = true;
    cooldown -> duration = timespan_t::zero(); // dbc has CD of 15 seconds
  }

  virtual void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> buffs.arcane_charge -> trigger();
      trigger_am( "Arcane Orb Impact" );

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
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();
    p() -> buffs.arcane_power -> trigger( 1, data().effectN( 1 ).percent() );
  }
};

// Blast Furance Spell =======================================================
// TODO: Assume this will be fixed and be pandmic extended like normal. If not,
//       fix to be the old DoT style
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
      trigger_fof( "Blizzard", fof_proc_chance );
    }
  }

  virtual double composite_crit() const override
  {
    double c = frost_mage_spell_t::composite_crit();
    if ( p() -> artifact.the_storm_rages.rank() )
    {
      c+= p() -> artifact.the_storm_rages.percent();
    }
    return c;
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

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t duration = frost_mage_spell_t::composite_dot_duration( s );
    return duration * ( tick_time( s ) / base_tick_time );
  }
};

// Charged Up Spell =========================================================

struct charged_up_t : public arcane_mage_spell_t
{
  charged_up_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "charged_up", p, p -> find_spell ("Charged Up" ) )
  {
    parse_options( options_str );
    may_proc_missiles = false;
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();

    p() -> buffs.arcane_charge -> trigger( 4.0 );
  }

};

// Cinderstorm Spell ========================================================
// NOTE: Due to the akward pathing of cinderstorm in game, Cinderstorm here is
//       just a driver for the "cinders" which actually deal damage. By altering
//       the loop inside cinderstorm_t execute() you can customize the number of
//       cinder impacts.
// TODO: Fix the extremly hack-ish way that the increased damage on ignite targets
//       is done.
struct cinders_t : public fire_mage_spell_t
{
  cinders_t( mage_t* p ) :
    fire_mage_spell_t( "cinders", p )
  {
    background = true;
    aoe = -1;
    triggers_ignite = true;
    spell_power_mod.direct = p -> find_spell( 198928 ) -> effectN( 1 ).sp_coeff();
    school = SCHOOL_FIRE;
  }
  virtual void execute() override
  {
    bool ignite_exists = p() -> ignite -> get_dot( p() -> target ) -> is_ticking();

    if ( ignite_exists )
    {
      base_dd_multiplier *= 1.0 + p() -> talents.cinderstorm -> effectN( 1 ).percent();
    }
    fire_mage_spell_t::execute();
    base_dd_multiplier = 1.0;
  }
};


struct cinderstorm_t : public fire_mage_spell_t
{
  cinders_t* cinder;

  cinderstorm_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "cinderstorm", p, p -> talents.cinderstorm ),
    cinder( new cinders_t( p ) )
  {
    parse_options( options_str );
    triggers_ignite = false;
    cooldown -> hasted = true;
  }
  virtual void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    cinder -> target = s -> target;

    for ( int i = 0; i < p() -> cinder_count; i++ )
    {
      cinder -> execute();
    }
  }
};
// Combustion Spell =========================================================

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
    split_aoe_damage = true;
    background = true;
    school = SCHOOL_FROST;
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.0 );
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

  void tick( dot_t* d ) override
  {
    frost_mage_spell_t::tick( d );
    projectile -> execute();
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
  dragons_breath_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "dragons_breath", p,
                       p -> find_class_spell( "Dragon's Breath" ) )
  {
    parse_options( options_str );
    aoe = -1;
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

    // Doesn't apply chill debuff but benefits from Bone Chilling somehow
    chills = true;
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();
    trigger_fof( "Ebonbolt", 1.0, 2 );
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
    may_proc_missiles = false;

    if ( p -> artifact.aegwynns_ascendance.rank() )
    {
      aegwynns_ascendance = new aegwynns_ascendance_t( p );
    }
  }

  virtual void execute()
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

    triggers_hot_streak = true;
    triggers_ignite = true;
    base_multiplier *= 1.0 + p -> artifact.great_balls_of_fire.percent();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = fire_mage_spell_t::execute_time();

    if ( p() -> artifact.fire_at_will.rank() )
    {
      t += p() -> artifact.fire_at_will.time_value();
    }

    return t;
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t t = fire_mage_spell_t::travel_time();
    return std::min( timespan_t::from_seconds( 0.75 ), t );
  }

  virtual void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );
    sim ->out_debug.printf("%f",p() -> cache.mastery_value());
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

  double composite_target_crit( player_t* target ) const override
  {
    double c = fire_mage_spell_t::composite_target_crit( target );

    // Fire PvP 4pc set bonus
    if ( td( target ) -> debuffs.firestarter -> check() )
    {
      c += td( target ) -> debuffs.firestarter
                        -> data().effectN( 1 ).percent();
    }

    return c;
  }

  virtual double composite_crit() const override
  {
    double c = fire_mage_spell_t::composite_crit();

      c += p() -> buffs.enhanced_pyrotechnics -> stack() *
           p() -> buffs.enhanced_pyrotechnics -> data().effectN( 1 ).percent();

      if( p() -> talents.fire_starter -> ok() && ( target -> health_percentage() >
          p() -> talents.fire_starter -> effectN( 1 ).base_value() ) )
        c = 1.0;
    return c;
  }

  virtual double composite_crit_multiplier() const override
  {
    double m = fire_mage_spell_t::composite_crit_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }


};

// Flame Patch Spell ==========================================================

struct flame_patch_t : public fire_mage_spell_t
{
  flame_patch_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "flame_patch", p, p -> talents.flame_patch )
  {
    parse_options( options_str );

    dot_duration =  p -> find_spell( 205470 ) -> duration();
    base_tick_time = timespan_t::from_seconds( 1.0 );//TODO: Hardcode this as it is not in the spell data.
    hasted_ticks=true;
    spell_power_mod.tick = p -> find_spell( 205472 ) -> effectN( 1 ).sp_coeff();
    aoe = -1;
  }
};
// Flamestrike Spell ==========================================================

struct aftershocks_t : public fire_mage_spell_t
{
  aftershocks_t( mage_t* p ) :
    fire_mage_spell_t( "aftershocks", p, p -> find_spell( 194432 ) )
  {
    background = true;
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
                       flame_patch( new flame_patch_t( p, options_str ) )
  {
    parse_options( options_str );

    base_multiplier *= 1.0 + p -> artifact.blue_flame_special.percent();
    triggers_ignite = true;
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

    if ( p() -> artifact.aftershocks.rank() )
    {
      aftershocks -> schedule_execute();
    }

    if ( p() -> talents.flame_patch -> ok() )
    {
      flame_patch -> execute();
    }
  }

  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  {
    fire_mage_spell_t::snapshot_state( s, rt );

    ignite_spell_state_t* is = static_cast<ignite_spell_state_t*>( s );
    is -> hot_streak = ( p() -> buffs.hot_streak -> check() != 0 );
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
      p() -> cooldowns.inferno_blast -> reset( false );
    }
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
  frostbolt_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "frostbolt", p,
                        p -> find_specialization_spell( "Frostbolt" ) ),
    icicle( p -> get_stats( "icicle" ) )
  {
    parse_options( options_str );
    stats -> add_child( icicle );
    icicle -> school = school;
    icicle -> action_list.push_back( p -> icicle );
    if ( p -> talents.lonely_winter -> ok() )
    {
      base_multiplier *= 1.0 + ( p -> talents.lonely_winter -> effectN( 1 ).percent() +
                               p -> artifact.its_cold_outside.data().effectN( 2 ).percent() );
    }
    base_multiplier *= 1.0 + p -> artifact.icy_caress.percent();
    chills = true;
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

    if ( result_is_hit( execute_state -> result ) )
    {
      double fof_proc_chance = p() -> spec.fingers_of_frost
                                   -> effectN( 1 ).percent();
      double bf_proc_chance = p() -> spec.brain_freeze
                                  -> effectN( 1 ).percent();

      if ( p() -> artifact.clarity_of_thought.rank() )
      {
        bf_proc_chance += p() -> artifact.clarity_of_thought.percent();
      }

      if( rng().roll( bf_proc_chance ) )
      {
        p() -> cooldowns.frozen_orb -> reset( false );
      }

      trigger_fof( "Frostbolt", fof_proc_chance );

      if ( p() -> shatterlance )
      {
        p() -> buffs.shatterlance -> trigger();
      }
    }
  }

  virtual void impact( action_state_t* s ) override
  {
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
        pets::water_elemental_pet_td_t* we_td =
          p() -> pets.water_elemental
          -> get_target_data( execute_state -> target );

        if ( we_td -> water_jet -> up() )
        {
          trigger_fof( "Water Jet", 1.0 );
        }
      }

      if ( p() -> artifact.ice_nine.rank() &&
           rng().roll( p() -> artifact.ice_nine.percent() ) )
      {
        trigger_icicle_gain( s, icicle );
      }
      if ( s -> result == RESULT_CRIT && p() -> artifact.frozen_veins.rank() )
      {
        p() -> cooldowns.icy_veins -> adjust( -1000 *
                                             p() -> artifact.frozen_veins.time_value() );
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

  virtual double composite_crit() const override
  {
    double c = frost_mage_spell_t::composite_crit();

    if ( p() -> artifact.shattering_bolts.rank() )
    {
      c+= p() -> artifact.shattering_bolts.percent();
    }
    return c;
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
    base_multiplier *= 1.0 + p -> artifact.orbital_strike.percent();
    chills = true;
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      double fof_proc_chance = p() -> spec.fingers_of_frost
                                   -> effectN( 2 ).percent();
      trigger_fof( "Frozen Orb Tick", fof_proc_chance );
    }
  }

  // Override damage type because Frozen Orb is considered a DOT
  dmg_e amount_type( const action_state_t* /* state */, bool /* periodic */ ) const override
  {
    return DMG_OVER_TIME;
  }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    frost_mage_spell_t::calculate_direct_amount( s );

    if ( result_is_hit( s -> result ) && s -> result == RESULT_CRIT )
    {
      s -> result_total *= 1.0 + p() -> artifact.ice_age.percent();
    }
    return s -> result_total;
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
    base_tick_time    = timespan_t::from_seconds( 1.0 );
    dot_duration      = data().duration();
    add_child( frozen_orb_bolt );
    may_miss       = false;
    may_crit       = false;
  }

  void tick( dot_t* d ) override
  {
    frost_mage_spell_t::tick( d );
    // "travel time" reduction of ticks based on distance from target - set on the side of less ticks lost.
    if ( d -> current_tick <= ( d -> num_ticks - util::round( ( ( player -> current.distance - 16.0 ) / 16.0 ), 0 ) ) )
    {
    frozen_orb_bolt -> target = d -> target;
    frozen_orb_bolt -> execute();
    }
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
      trigger_fof( "Frozen Orb Initial Impact", 1.0 );
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

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    trigger_fof( "Frozen Touch", 1.0, 2 );
  }
};


// Glacial Spike ==============================================================

struct glacial_spike_t : public frost_mage_spell_t
{
  glacial_spike_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "glacial_spike", p, p -> talents.glacial_spike )
  {
    parse_options( options_str );
  }

  virtual bool ready() override
  {
    if ( as<int>( p() -> icicles.size() ) <
         p() -> spec.icicles -> effectN( 2 ).base_value() )
    {
      return false;
    }

    return frost_mage_spell_t::ready();
  }

  virtual void execute() override
  {
    double icicle_damage_sum = 0;
    int icicle_count = as<int>( p() -> icicles.size() );
    assert( icicle_count == p() -> spec.icicles -> effectN( 2 ).base_value() );
    for ( size_t i = 0; i < p() -> icicles.size(); i++ )
    {
      icicle_data_t d = p() -> get_icicle_object();
      icicle_damage_sum += d.first;
    }
    if ( sim -> debug )
    {
      sim -> out_debug.printf("Add %u icicles to glacial_spike for %f damage",
                              icicle_count, icicle_damage_sum);
    }

    // Sum icicle damage
    base_dd_min = icicle_damage_sum;
    base_dd_max = icicle_damage_sum;

    frost_mage_spell_t::execute();
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

  double shatterlance_effect;

  ice_lance_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "ice_lance", p, p -> find_class_spell( "Ice Lance" ) ),
    shatterlance_effect( 0.0 )
  {
    parse_options( options_str );

    if ( p -> shatterlance )
    {
      const spell_data_t* data = p -> shatterlance -> driver();
      shatterlance_effect = data -> effectN( 1 ).average( p -> shatterlance -> item );
      shatterlance_effect /= 100.0;
    }

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
  }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    frost_mage_spell_t::calculate_direct_amount( s );

    if ( result_is_hit( s -> result ) && s -> result == RESULT_CRIT )
    {
      s -> result_total *= 1.0 + p() -> artifact.let_it_go.percent();
    }

    return s -> result_total;
  }
  virtual void execute() override
  {
    // Ice Lance treats the target as frozen with FoF up
    frozen = ( p() -> buffs.fingers_of_frost -> up() != 0 );

    p() -> buffs.shatterlance -> up();

    frost_mage_spell_t::execute();

    if ( p() -> talents.thermal_void -> ok() &&
         p() -> buffs.icy_veins -> check() )
    {
      timespan_t tv_extension = p() -> talents.thermal_void
                                    -> effectN( 1 ).time_value() * 1000;
      p() -> buffs.icy_veins -> extend_duration( p(), tv_extension);
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
    // Icicles are gone, including new ones that accumulate while they're being
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
    frost_mage_spell_t::impact( s );

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

    if ( frozen )
    {
      am *= 1.0 + data().effectN( 2 ).percent();
    }

    if ( p() -> buffs.fingers_of_frost -> check() )
    {
      am *= 1.0 + p() -> buffs.fingers_of_frost -> data().effectN( 2 ).percent();
    }

    if ( p() -> buffs.shatterlance -> check() )
    {
      am *= 1.0 + shatterlance_effect;
    }

    if ( p() -> buffs.chain_reaction -> up() )
    {
      am *= 1.0 + p() -> buffs.chain_reaction -> data().effectN( 1 ).percent();
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
    if ( p() -> artifact.chilled_to_the_core.rank() )
    {
    p() -> buffs.chilled_to_the_core -> trigger();
    }
  }
};

// Inferno Blast Spell ======================================================

struct inferno_blast_t : public fire_mage_spell_t
{
  double pyrosurge_chance;
  flamestrike_t* pyrosurge_flamestrike;
  cooldown_t* icd;
  blast_furance_t* blast_furnace;

  inferno_blast_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "inferno_blast", p,
                       p -> find_class_spell( "Inferno Blast" ) ),
    pyrosurge_chance( 0.0 ),
    blast_furnace( nullptr )
  {
    parse_options( options_str );

    base_multiplier *= 1.0 + p -> artifact.reignition_overdrive.percent();
    trigger_gcd = timespan_t::zero();
    cooldown -> charges = data().charges();
    cooldown -> duration = data().charge_cooldown();
    cooldown -> duration += p -> sets.set( MAGE_FIRE, T17, B2 ) -> effectN( 1 ).time_value();
    cooldown -> hasted = true;
    //TODO: What is this..?
    icd = p -> get_cooldown( "inferno_blast_icd" );

    triggers_hot_streak = true;
    triggers_ignite = true;

    // TODO: Is the spread range still 10 yards?
    radius = 10;

    if ( p -> pyrosurge )
    {
      const spell_data_t* data = p -> pyrosurge -> driver();
      pyrosurge_chance = data -> effectN( 1 ).average( p -> pyrosurge -> item );
      pyrosurge_chance /= 100.0;

      pyrosurge_flamestrike = new flamestrike_t( p, options_str );
      pyrosurge_flamestrike -> background = true;
      pyrosurge_flamestrike -> callbacks = false;
    }

    if ( p -> artifact.blast_furnace.rank() )
    {
      blast_furnace = new blast_furance_t( p, options_str );
    }
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
      if ( p() -> pyrosurge && p() -> rng().roll( pyrosurge_chance ) )
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

  // Inferno Blast always crits
  virtual double composite_crit() const override
  { return 1.0; }
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
        "living_bomb_explosion on %s applies living_bomb on %s",
        s -> action -> target -> name(),
        s -> target -> name() );
    }

    child_lb -> target = s -> target;
    child_lb -> execute();
  }
}

living_bomb_t::living_bomb_t( mage_t* p, const std::string& options_str,
                              bool _casted = true ) :
  fire_mage_spell_t( "living_bomb", p, p -> find_spell( 217694 ) ),
  casted( _casted ),
  explosion( new living_bomb_explosion_t( p, this ) )
{
  parse_options( options_str );

  snapshot_flags = STATE_HASTE;
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


// Mark of Aluneth Spell =============================================================
// TODO: Tick times are inconsistent in game. Until fixed, remove hasted ticks
//       and cap the DoT at 5 ticks, then an explosion.

struct mark_of_aluneth_explosion_t : public arcane_mage_spell_t
{
  mark_of_aluneth_explosion_t( mage_t* p ) :
    arcane_mage_spell_t( "mark_of_aluneth_explosion", p, p -> find_spell( 224968 ) )
  {
    background = true;
    school = SCHOOL_ARCANE;
  }
  virtual void init() override
  {
    arcane_mage_spell_t::init();
    // disable the snapshot_flags for all multipliers
    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_TGT_MUL_DA;
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
    may_proc_missiles = false;
    dot_duration = p -> find_spell( 210726 ) -> duration();
    base_tick_time = timespan_t::from_seconds( 1.2 ); // TODO: Hardcode until tick times are worked out
    spell_power_mod.tick = p -> find_spell( 211088 ) -> effectN( 1 ).sp_coeff();
    hasted_ticks = false;
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
    pet_t** images = p() -> pets.mirror_images;

    for ( int i = 0; i < data().effectN( 2 ).base_value(); i++ )
    {
      if ( !images[i] )
      {
        continue;
      }

      stats -> add_child( images[i] -> get_stats( "arcane_blast" ) );
      stats -> add_child( images[i] -> get_stats( "fire_blast" ) );
      stats -> add_child( images[i] -> get_stats( "fireball" ) );
      stats -> add_child( images[i] -> get_stats( "frostbolt" ) );
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
    add_child( nether_tempest_aoe );
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> last_bomb_target != nullptr &&
           p() -> last_bomb_target != execute_state -> target )
      {
        td( p() -> last_bomb_target ) -> dots.nether_tempest -> cancel();
      }
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

  virtual double composite_crit_multiplier() const override
  {
    double m = fire_mage_spell_t::composite_crit_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
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

  virtual double composite_crit_multiplier() const override
  {
    double m = fire_mage_spell_t::composite_crit_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t t = fire_mage_spell_t::travel_time();
    return std::min( t, timespan_t::from_seconds( 0.75 ) );
  }
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

  pyroblast_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "pyroblast", p, p -> find_class_spell( "Pyroblast" ) ),
    conjure_phoenix( nullptr )
  {
    parse_options( options_str );

    triggers_ignite = true;
    triggers_hot_streak = true;

    if ( p -> sets.has_set_bonus( MAGE_FIRE, T18, B2 ) )
    {
      conjure_phoenix = new conjure_phoenix_t( p );
      add_child( conjure_phoenix );
    }

    base_multiplier *= 1.0 + p -> artifact.pyroclasmic_paranoia.percent();
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

    ignite_spell_state_t* is = static_cast<ignite_spell_state_t*>( s );
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

      ignite_spell_state_t* is = static_cast<ignite_spell_state_t*>( s );
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

  virtual double composite_crit_multiplier() const override
  {
    double m = fire_mage_spell_t::composite_crit_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }

  virtual double composite_crit() const override
  {
    double c = fire_mage_spell_t::composite_crit();

    if ( p() -> buffs.pyromaniac -> check() )
    {
      c += 1.0;
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
  double scorch_multiplier;

  scorch_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "scorch", p,
                       p -> find_specialization_spell( "Scorch" ) ),
    scorch_multiplier( 0 )
  {
    parse_options( options_str );

    triggers_hot_streak = true;
    triggers_ignite = true;

    consumes_ice_floes = false;
  }

  double composite_crit_multiplier() const override
  {
    double m = fire_mage_spell_t::composite_crit_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
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
  supernova_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "supernova", p, p -> talents.supernova )
  {
    parse_options( options_str );

    aoe = -1;

    double sn_mult = 1.0 + p -> talents.supernova -> effectN( 1 ).percent();
    base_multiplier *= sn_mult;
    base_aoe_multiplier = 1.0 / sn_mult;
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) &&
         execute_state -> n_targets > 1 )
    {
      // NOTE: Supernova AOE effect causes secondary trigger chance for AM
      // TODO: Verify this is still the case
      trigger_am( "Supernova AOE" );
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
    may_proc_missiles = false;
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
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if ( p -> buffs.exhaustion -> check() || p -> is_pet() )
        continue;

      p -> buffs.bloodlust -> trigger(); // Bloodlust and Timewarp are the same
      p -> buffs.exhaustion -> trigger();
    }
  }

  virtual bool ready() override
  {
    if ( sim -> overrides.bloodlust )
      return false;

    if ( player -> buffs.exhaustion -> check() )
      return false;

    return mage_spell_t::ready();
  }
};



// ============================================================================
// Mage Custom Actions
// ============================================================================

// Choose Rotation ============================================================




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
  pets::water_elemental_pet_t::water_jet_t* action;

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
      action = debug_cast<pets::water_elemental_pet_t::water_jet_t*>( m -> pets.water_elemental -> find_action( "water_jet" ) );
      assert( action );
      action -> autocast = false;
    }
  }

  void execute() override
  {
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
    may_proc_missiles = false;
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
    may_proc_missiles = false;
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

struct erosion_event_t : public event_t
{
  mage_t* mage;
  player_t* target;
  erosion_event_t( mage_t& m, player_t* t ) :
    event_t( m ), mage( &m ), target( t )
  {}
};
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

    add_event( timespan_t::from_seconds( cast_time ) );
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
    new_s -> source = state.second;
    new_s -> target = target;

    mage -> icicle -> base_dd_min = mage -> icicle -> base_dd_max = state.first;

    // Immediately execute icicles so the correct damage is carried into the
    // travelling icicle object
    mage -> icicle -> pre_execute_state = new_s;
    mage -> icicle -> execute();

    icicle_data_t new_state = mage -> get_icicle_object();
    if ( new_state.first > 0 )
    {
      mage -> icicle_event = new ( sim() ) icicle_event_t( *mage, new_state, target );
      if ( mage -> sim -> debug )
        mage -> sim -> out_debug.printf( "%s icicle use on %s (chained), damage=%f, total=%u",
                               mage -> name(), target -> name(), new_state.first, as<unsigned>( mage -> icicles.size() ) );
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

    residual_periodic_state_t* ignite_state =
      debug_cast< residual_periodic_state_t* >( ignite -> state );
    return ignite_state -> tick_amount * ignite -> ticks_left();
  }

  static bool ignite_compare ( dot_t* a, dot_t* b )
  {
    return ignite_bank( a ) > ignite_bank( b );
  }

  ignite_spread_event_t( mage_t& m ) :
    event_t( m ), mage( &m )
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
    mage -> ignite_spread_event = new ( sim() ) events::ignite_spread_event_t( *mage );
    mage -> ignite_spread_event -> add_event( timespan_t::from_seconds( 2.0 ) );
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

  debuffs.frost_bomb  = buff_creator_t( *this, "frost_bomb",
                                        mage -> talents.frost_bomb );
  debuffs.chilled     = new chilled_t( this );
  debuffs.firestarter = buff_creator_t( *this, "firestarter" )
                          .chance( 1.0 )
                          .duration( timespan_t::from_seconds( 10.0 ) );
  debuffs.water_jet   = buff_creator_t( *this, "water_jet",
                                        mage -> find_spell( 135029 ) )
                          .quiet( true )
                          .cd( timespan_t::zero() );
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
  if ( name == "inferno_blast"     ) return new           inferno_blast_t( this, options_str );
  if ( name == "living_bomb"       ) return new             living_bomb_t( this, options_str );
  if ( name == "meteor"            ) return new                  meteor_t( this, options_str );
  if ( name == "pyroblast"         ) return new               pyroblast_t( this, options_str );
  if ( name == "scorch"            ) return new                  scorch_t( this, options_str );

  // Frost
  if ( name == "blizzard"          ) return new                blizzard_t( this, options_str );
  if ( name == "comet_storm"       ) return new             comet_storm_t( this, options_str );
  if ( name == "frost_bomb"        ) return new              frost_bomb_t( this, options_str );
  if ( name == "frostbolt"         ) return new               frostbolt_t( this, options_str );
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


// mage_t::create_pets ========================================================

void mage_t::create_pets()
{
  if ( specialization() == MAGE_FROST && find_action( "water_elemental" ) )
  {
    pets.water_elemental = new pets::water_elemental_pet_t( sim, this );
  }

  if ( talents.mirror_image -> ok() && find_action( "mirror_image" ) )
  {
    int image_num = talents.mirror_image -> effectN( 2 ).base_value();
    pets.mirror_images = new pet_t*[ image_num ];
    for ( int i = 0; i < image_num; i++ )
    {
      pets.mirror_images[ i ] = new pets::mirror_image_pet_t( sim, this );
      if ( i > 0 )
      {
        pets.mirror_images[ i ] -> quiet = 1;
      }
    }
  }

  if ( sets.has_set_bonus( MAGE_ARCANE, T18, B2 ) )
  {
    // There isn't really a cap on temporal heroes, but 10 sounds safe-ish
    pets.temporal_heroes = new pet_t*[ pets.temporal_hero_count ];
    for ( int i = 0; i < pets.temporal_hero_count; i++ )
    {
      pets.temporal_heroes[ i ] = new pets::temporal_hero_t( sim, this );
      pets::temporal_hero_t::randomize_last_summoned( this );
    }
  }

  if ( talents.arcane_familiar -> ok() &&
       find_action( "summon_arcane_familiar" ) )
  {
    pets.arcane_familiar = new pets::arcane_familiar_pet_t( sim, this );

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
  talents.torrent         = find_talent_spell( "Torrent"         );
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


  // Spec Spells
  spec.arcane_charge         = find_spell( 36032 );
  spec.arcane_charge_passive = find_spell( 114664 );
  if ( talents.arcane_familiar -> ok() )
  {
    spec.arcane_familiar    = find_spell( 210126 );
  }

  spec.critical_mass         = find_specialization_spell( "Critical Mass"    );

  spec.brain_freeze          = find_specialization_spell( "Brain Freeze"     );
  spec.fingers_of_frost      = find_spell( 112965 );
  spec.shatter               = find_specialization_spell( "Shatter"          );

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
  if ( talents.unstable_magic )
    unstable_magic_explosion = new actions::unstable_magic_explosion_t( this );
}

// mage_t::init_base ========================================================

void mage_t::init_base_stats()
{
  player_t::init_base_stats();

  base.spell_power_per_intellect = 1.0;

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility = 0.0;

  base.mana_regen_per_second = resources.base[ RESOURCE_MANA ] * 0.015;

  if ( race == RACE_ORC )
    pet_multiplier *= 1.0 + find_racial_spell( "Command" ) -> effectN( 1 ).percent();
}

// Custom buffs ===============================================================

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

struct ray_of_frost_buff_t : public buff_t
{
  timespan_t rof_cd;

  ray_of_frost_buff_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "ray_of_frost", p -> find_spell( 208141 ) ) )
  {
    const spell_data_t* rof_data = p -> find_spell( 205021 );
    rof_cd = rof_data -> cooldown() - rof_data -> duration();
  }

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
  buffs.arcane_missiles       = new arcane_missiles_buff_t( this );
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
                                  .add_invalidate( CACHE_SPELL_HASTE );

  // 4T18 Temporal Power buff has no duration and stacks multiplicatively
  buffs.temporal_power        = buff_creator_t( this, "temporal_power", find_spell( 190623 ) )
                                  .max_stack( 10 );

  // Fire
  buffs.combustion            = buff_creator_t( this, "combustion", find_spell( 190319 ) )
                                  .cd( timespan_t::zero() )
                                  .add_invalidate( CACHE_SPELL_CRIT )
                                  .add_invalidate( CACHE_MASTERY );
  buffs.enhanced_pyrotechnics = buff_creator_t( this, "enhanced_pyrotechnics", find_spell( 157644 ) );
  buffs.heating_up            = buff_creator_t( this, "heating_up",  find_spell( 48107 ) );
  buffs.hot_streak            = buff_creator_t( this, "hot_streak",  find_spell( 48108 ) );
  buffs.molten_armor          = buff_creator_t( this, "molten_armor", find_spell( 30482 ) )
                                  .add_invalidate( CACHE_SPELL_CRIT );
  buffs.icarus_uprising       = buff_creator_t( this, "icarus_uprising", find_spell( 186170 ) )
                                  .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                                  .add_invalidate( CACHE_SPELL_HASTE );
  buffs.pyretic_incantation   = buff_creator_t( this, "pyretic_incantation", find_spell( 194329 ) );
  buffs.pyromaniac            = buff_creator_t( this, "pyromaniac", sets.set( MAGE_FIRE, T17, B4 ) -> effectN( 1 ).trigger() )
                                  .trigger_spell( sets.set( MAGE_FIRE, T17, B4 ) );

  // Frost
  buffs.bone_chilling         = buff_creator_t( this, "bone_chilling", find_spell( 205766 ) );
  buffs.fingers_of_frost      = buff_creator_t( this, "fingers_of_frost", find_spell( 44544 ) )
                                  .max_stack( find_spell( 44544 ) -> max_stacks() +
                                              sets.set( MAGE_FROST, T18, B4 ) -> effectN( 2 ).base_value() +
                                              artifact.icy_hand.rank() );
  buffs.frost_armor           = buff_creator_t( this, "frost_armor", find_spell( 7302 ) )
                                  .add_invalidate( CACHE_SPELL_HASTE );
  buffs.icy_veins             = buff_creator_t( this, "icy_veins", find_spell( 12472 ) )
                                  .add_invalidate( CACHE_SPELL_HASTE );
  buffs.frost_t17_4pc         = buff_creator_t( this, "frost_t17_4pc", find_spell( 165470 ) )
                                  .duration( find_spell( 84714 ) -> duration() )
                                  .period( find_spell( 165470 ) -> effectN( 1 ).time_value() )
                                  .quiet( true )
                                  .tick_callback( [ this ]( buff_t*, int, const timespan_t& )
                                  {
                                    buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
                                    benefits.fingers_of_frost -> update( "4T17" );
                                    if ( sim -> debug )
                                    {
                                      sim -> out_debug.printf( "%s gains Fingers of Frost from 4T17", name() );
                                    }
                                  }
                                );
  buffs.ice_shard             = buff_creator_t( this, "ice_shard", find_spell( 166869 ) );
  buffs.ray_of_frost          = new ray_of_frost_buff_t( this );
  buffs.shatterlance          = buff_creator_t( this, "shatterlance")
                                  .duration( timespan_t::from_seconds( 0.9 ) )
                                  .cd( timespan_t::zero() )
                                  .chance( 1.0 );

  // Talents
  buffs.ice_floes             = buff_creator_t( this, "ice_floes", talents.ice_floes );
  buffs.incanters_flow        = new incanters_flow_t( this );
  buffs.rune_of_power         = buff_creator_t( this, "rune_of_power", find_spell( 116014 ) )
                                  .duration( find_spell( 116011 ) -> duration() )
                                  .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Artifact
  // Frost
  buffs.chain_reaction   = buff_creator_t( this, "chain_reaction", find_spell( 195418 ) );

  buffs.chilled_to_the_core = buff_creator_t( this, "chilled_to_the_core", find_spell( 195446 ) )
                                   .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
}

// mage_t::create_options ===================================================
void mage_t::create_options()
{
  add_option( opt_float( "cinderstorm_cinder_count", cinder_count ) );

  player_t::create_options();
}

// mage_t::init_gains =======================================================

void mage_t::init_gains()
{
  player_t::init_gains();

  gains.evocation              = get_gain( "evocation"              );
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
  sim -> target_non_sleeping_list.register_callback( current_target_reset_cb_t( this ) );
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

//Pre-combat Action Priority List============================================

void mage_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // TODO: Handle level 110 food/flasks
  if( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask_action = "flask,type=";

    if ( true_level <= 85 )
      flask_action += "draconic_mind" ;
    else if ( true_level <= 90 )
      flask_action += "warm_sun" ;
    else
      flask_action += "greater_draenic_intellect_flask" ;

    precombat -> add_action( flask_action );
  }
    // Food
  if ( sim -> allow_food && level() >= 80 )
  {
    std::string food_action = "food,type=";

    if ( level() <= 85 )
      food_action += "seafood_magnifique_feast" ;
    else if ( level() <= 90 )
      food_action += "mogu_fish_stew" ;
    else if ( specialization() == MAGE_ARCANE && sets.has_set_bonus( MAGE_ARCANE, T18, B4 ) )
      food_action += "buttered_sturgeon" ;
    else if ( specialization() == MAGE_ARCANE )
      food_action += "sleeper_sushi" ;
    else if ( specialization() == MAGE_FIRE )
      food_action += "pickled_eel" ;
    else
      food_action += "salty_squid_roll" ;

    precombat -> add_action( food_action );
  }

  // Arcane Brilliance
  precombat -> add_action( this, "Arcane Brilliance" );

  // Water Elemental
  if ( specialization() == MAGE_FROST )
    precombat -> add_action( "water_elemental" );

  // Snapshot Stats
  precombat -> add_action( "snapshot_stats" );

  // Level 90 talents
  precombat -> add_talent( this, "Rune of Power" );
  precombat -> add_talent( this, "Mirror Image" );

  //Potions
  if ( sim -> allow_potions && true_level >= 80 )
  {
    precombat -> add_action( get_potion_action() );
  }

  if ( specialization() == MAGE_ARCANE )
    precombat -> add_action( this, "Arcane Blast" );
  else if ( specialization() == MAGE_FIRE )
    precombat -> add_action( this, "Pyroblast" );
  else
  {
    precombat -> add_action( this, "Frostbolt", "if=!talent.frost_bomb.enabled" );
    precombat -> add_talent( this, "Frost Bomb" );
  }
}


// Util for using level appropriate potion
// TODO: Handle level 110 potions
std::string mage_t::get_potion_action()
{
  std::string potion_action = "potion,name=";

  if ( true_level <= 85 )
    potion_action += "volcanic" ;
  else if ( true_level <= 90 )
    potion_action += "jade_serpent" ;
  else
    potion_action += "draenic_intellect" ;

  return potion_action;
}


// Arcane Mage Action List====================================================

void mage_t::apl_arcane()
{
  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  action_priority_list_t* default_list        = get_action_priority_list( "default"          );
  /*
  action_priority_list_t* movement            = get_action_priority_list( "movement"         );
  action_priority_list_t* init_burn           = get_action_priority_list( "init_burn"        );
  action_priority_list_t* init_crystal        = get_action_priority_list( "init_crystal"     );
  action_priority_list_t* crystal_sequence    = get_action_priority_list( "crystal_sequence" );
  action_priority_list_t* cooldowns           = get_action_priority_list( "cooldowns"        );
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe"              );
  action_priority_list_t* burn                = get_action_priority_list( "burn"             );
  action_priority_list_t* conserve            = get_action_priority_list( "conserve"         );
  */

  default_list -> add_action( this, "Counterspell",
                              "if=target.debuff.casting.react" );
  default_list -> add_action( "stop_burn_phase,if=prev_gcd.evocation&burn_phase_duration>gcd.max" );
  default_list -> add_action( this, "Time Warp",
                              "if=target.health.pct<25|time>5" );
  default_list -> add_talent( this, "Rune of Power",
                              "if=buff.rune_of_power.remains<2*spell_haste" );
  default_list -> add_talent( this, "Mirror Image" );
  /*
  default_list -> add_action( "call_action_list,name=aoe,if=active_enemies>=5" );
  default_list -> add_action( "call_action_list,name=init_burn,if=!burn_phase" );
  default_list -> add_action( "call_action_list,name=burn,if=burn_phase" );
  default_list -> add_action( "call_action_list,name=conserve" );

  movement -> add_action( this, "Blink",
                          "if=movement.distance>10" );
  movement -> add_talent( this, "Blazing Speed",
                          "if=movement.remains>0" );
  movement -> add_talent( this, "Ice Floes",
                          "if=buff.ice_floes.down&(raid_event.movement.distance>0|raid_event.movement.in<2*spell_haste)" );


  init_burn -> add_action( "start_burn_phase,if=buff.arcane_charge.stack>=4&(legendary_ring.cooldown.remains<gcd.max|legendary_ring.cooldown.remains>target.time_to_die+15|!legendary_ring.has_cooldown)&(cooldown.prismatic_crystal.up|!talent.prismatic_crystal.enabled)&(cooldown.arcane_power.up|(glyph.arcane_power.enabled&cooldown.arcane_power.remains>60))&(cooldown.evocation.remains-2*buff.arcane_missiles.stack*spell_haste-gcd.max*talent.prismatic_crystal.enabled)*0.75*(1-0.1*(cooldown.arcane_power.remains<5))*(1-0.1*(talent.nether_tempest.enabled|talent.supernova.enabled))*(10%action.arcane_blast.execute_time)<mana.pct-20-2.5*active_enemies*(9-active_enemies)+(cooldown.evocation.remains*1.8%spell_haste)",
                           "Regular burn with evocation" );
  init_burn -> add_action( "start_burn_phase,if=target.time_to_die*0.75*(1-0.1*(talent.nether_tempest.enabled|talent.supernova.enabled))*(10%action.arcane_blast.execute_time)*1.1<mana.pct-10+(target.time_to_die*1.8%spell_haste)",
                           "End of fight burn" );


  init_crystal -> add_action( "call_action_list,name=conserve,if=buff.arcane_charge.stack<4|(buff.arcane_missiles.react&debuff.mark_of_doom.remains>2*spell_haste+(target.distance%20))",
                              "Conditions for initiating Prismatic Crystal" );
  init_crystal -> add_action( this, "Arcane Missiles",
                              "if=buff.arcane_missiles.react&t18_class_trinket" );
  init_crystal -> add_talent( this, "Prismatic Crystal" );


  crystal_sequence -> add_action( "call_action_list,name=cooldowns",
                                  "Actions while Prismatic Crystal is active" );
  crystal_sequence -> add_talent( this, "Nether Tempest",
                                  "if=buff.arcane_charge.stack=4&!ticking&pet.prismatic_crystal.remains>8" );
  crystal_sequence -> add_talent( this, "Supernova",
                                  "if=mana.pct<96" );
  crystal_sequence -> add_action( this, "Presence of Mind",
                                  "if=cooldown.cold_snap.up|pet.prismatic_crystal.remains<2*spell_haste" );
  crystal_sequence -> add_action( this, "Arcane Blast",
                                  "if=buff.arcane_charge.stack=4&mana.pct>93&pet.prismatic_crystal.remains>cast_time" );
  crystal_sequence -> add_action( this, "Arcane Missiles",
                                  "if=pet.prismatic_crystal.remains>2*spell_haste+(target.distance%20)" );
  crystal_sequence -> add_talent( this, "Supernova",
                                  "if=pet.prismatic_crystal.remains<2*spell_haste+(target.distance%20)" );
  crystal_sequence -> add_action( "choose_target,if=pet.prismatic_crystal.remains<action.arcane_blast.cast_time&buff.presence_of_mind.down" );
  crystal_sequence -> add_action( this, "Arcane Blast" );


  cooldowns -> add_action( this, "Arcane Power",
                           "",
                           "Consolidated damage cooldown abilities" );
  for( size_t i = 0; i < racial_actions.size(); i++ )
  {
    cooldowns -> add_action( racial_actions[i] );
  }
  cooldowns -> add_action( get_potion_action() + ",if=buff.arcane_power.up&(!talent.prismatic_crystal.enabled|pet.prismatic_crystal.active)" );
  for( size_t i = 0; i < item_actions.size(); i++ )
  {
    cooldowns -> add_action( item_actions[i] );
  }


  aoe -> add_action( "call_action_list,name=cooldowns",
                     "AoE sequence" );
  aoe -> add_talent( this, "Nether Tempest",
                     "cycle_targets=1,if=buff.arcane_charge.stack=4&(active_dot.nether_tempest=0|(ticking&remains<3.6))" );
  aoe -> add_talent( this, "Supernova" );
  aoe -> add_talent( this, "Arcane Orb",
                     "if=buff.arcane_charge.stack<4" );
  aoe -> add_action( this, "Arcane Explosion",
                     "if=prev_gcd.evocation",
                     "APL hack for evocation interrupt" );
  aoe -> add_action( this, "Evocation",
                     "interrupt_if=mana.pct>96,if=mana.pct<85-2.5*buff.arcane_charge.stack" );
  aoe -> add_action( this, "Arcane Missiles",
                     "if=set_bonus.tier17_4pc&active_enemies<10&buff.arcane_charge.stack=4&buff.arcane_instability.react" );
  aoe -> add_action( this, "Arcane Missiles",
                     "target_if=debuff.mark_of_doom.remains>2*spell_haste+(target.distance%20),if=buff.arcane_missiles.react" );
  aoe -> add_talent( this, "Nether Tempest",
                     "cycle_targets=1,if=talent.arcane_orb.enabled&buff.arcane_charge.stack=4&ticking&remains<cooldown.arcane_orb.remains" );
  aoe -> add_action( this, "Arcane Barrage",
                     "if=buff.arcane_charge.stack=4" );
  aoe -> add_action( this, "Cone of Cold",
                     "if=glyph.cone_of_cold.enabled" );
  aoe -> add_action( this, "Arcane Explosion" );


  burn -> add_action( "call_action_list,name=init_crystal,if=talent.prismatic_crystal.enabled&cooldown.prismatic_crystal.up",
                      "High mana usage, \"Burn\" sequence" );
  burn -> add_action( "call_action_list,name=crystal_sequence,if=talent.prismatic_crystal.enabled&pet.prismatic_crystal.active" );
  burn -> add_action( "call_action_list,name=cooldowns" );
  burn -> add_action( this, "Arcane Missiles",
                      "if=buff.arcane_missiles.react=3" );
  burn -> add_action( this, "Arcane Missiles",
                      "if=set_bonus.tier17_4pc&buff.arcane_instability.react&buff.arcane_instability.remains<action.arcane_blast.execute_time" );
  burn -> add_talent( this, "Supernova",
                      "if=target.time_to_die<8|charges=2" );
  burn -> add_talent( this, "Nether Tempest",
                      "cycle_targets=1,if=target!=pet.prismatic_crystal&buff.arcane_charge.stack=4&(active_dot.nether_tempest=0|(ticking&remains<3.6))" );
  burn -> add_talent( this, "Arcane Orb",
                      "if=buff.arcane_charge.stack<4" );
  burn -> add_action( this, "Arcane Barrage",
                      "if=talent.arcane_orb.enabled&active_enemies>=3&buff.arcane_charge.stack=4&(cooldown.arcane_orb.remains<gcd.max|prev_gcd.arcane_orb)" );
  burn -> add_action( this, "Presence of Mind",
                      "if=mana.pct>96&(!talent.prismatic_crystal.enabled|!cooldown.prismatic_crystal.up)" );
  burn -> add_action( this, "Arcane Blast",
                      "if=buff.arcane_charge.stack=4&mana.pct>93" );
  burn -> add_action( this, "Arcane Missiles",
                      "if=buff.arcane_charge.stack=4&(mana.pct>70|!cooldown.evocation.up|target.time_to_die<15)" );
  burn -> add_talent( this, "Supernova",
                      "if=mana.pct>70&mana.pct<96" );
  burn -> add_action( this, "Evocation",
                      "interrupt_if=mana.pct>100-10%spell_haste,if=target.time_to_die>10&mana.pct<30+2.5*active_enemies*(9-active_enemies)-(40*(t18_class_trinket&buff.arcane_power.up))" );
  burn -> add_action( this, "Presence of Mind",
                      "if=!talent.prismatic_crystal.enabled|!cooldown.prismatic_crystal.up" );
  burn -> add_action( this, "Arcane Blast" );
  burn -> add_action( this, "Evocation" );


  conserve -> add_action( "call_action_list,name=cooldowns,if=target.time_to_die<15",
                          "Low mana usage, \"Conserve\" sequence" );
  conserve -> add_action( this, "Arcane Missiles",
                          "if=buff.arcane_missiles.react=3|(talent.overpowered.enabled&buff.arcane_power.up&buff.arcane_power.remains<action.arcane_blast.execute_time)" );
  conserve -> add_action( this, "Arcane Missiles",
                          "if=set_bonus.tier17_4pc&buff.arcane_instability.react&buff.arcane_instability.remains<action.arcane_blast.execute_time" );
  conserve -> add_talent( this, "Nether Tempest",
                          "cycle_targets=1,if=target!=pet.prismatic_crystal&buff.arcane_charge.stack=4&(active_dot.nether_tempest=0|(ticking&remains<3.6))" );
  conserve -> add_talent( this, "Supernova",
                          "if=target.time_to_die<8|(charges=2&(buff.arcane_power.up|!cooldown.arcane_power.up|!legendary_ring.cooldown.up)&(!talent.prismatic_crystal.enabled|cooldown.prismatic_crystal.remains>8))" );
  conserve -> add_talent( this, "Arcane Orb",
                          "if=buff.arcane_charge.stack<2" );
  conserve -> add_action( this, "Presence of Mind",
                          "if=mana.pct>96&(!talent.prismatic_crystal.enabled|!cooldown.prismatic_crystal.up)" );
  conserve -> add_action( this, "Arcane Missiles",
                          "if=buff.arcane_missiles.react&debuff.mark_of_doom.remains>2*spell_haste+(target.distance%20)" );
  conserve -> add_action( this, "Arcane Blast",
                          "if=buff.arcane_charge.stack=4&mana.pct>93" );
  conserve -> add_action( this, "Arcane Barrage",
                          "if=talent.arcane_orb.enabled&active_enemies>=3&buff.arcane_charge.stack=4&(cooldown.arcane_orb.remains<gcd.max|prev_gcd.arcane_orb)" );
  conserve -> add_action( this, "Arcane Missiles",
                          "if=buff.arcane_charge.stack=4&(!talent.overpowered.enabled|cooldown.arcane_power.remains>10*spell_haste|legendary_ring.cooldown.remains>10*spell_haste)" );
  conserve -> add_talent( this, "Supernova",
                          "if=mana.pct<96&(buff.arcane_missiles.stack<2|buff.arcane_charge.stack=4)&(buff.arcane_power.up|(charges=1&(cooldown.arcane_power.remains>recharge_time|legendary_ring.cooldown.remains>recharge_time)))&(!talent.prismatic_crystal.enabled|current_target=pet.prismatic_crystal|(charges=1&cooldown.prismatic_crystal.remains>recharge_time+8))" );
  conserve -> add_talent( this, "Nether Tempest",
                          "cycle_targets=1,if=target!=pet.prismatic_crystal&buff.arcane_charge.stack=4&(active_dot.nether_tempest=0|(ticking&remains<(10-3*talent.arcane_orb.enabled)*spell_haste))" );
  conserve -> add_action( this, "Arcane Barrage",
                          "if=buff.arcane_charge.stack=4" );
  conserve -> add_action( this, "Presence of Mind",
                          "if=buff.arcane_charge.stack<2&mana.pct>93" );
  conserve -> add_action( this, "Arcane Blast" );
  conserve -> add_action( this, "Arcane Barrage" );
  */
}

// Fire Mage Action List ===================================================================================================

void mage_t::apl_fire()
{
  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  action_priority_list_t* default_list        = get_action_priority_list( "default"           );
  /*
  action_priority_list_t* movement            = get_action_priority_list( "movement"         );
  action_priority_list_t* crystal_sequence    = get_action_priority_list( "crystal_sequence"  );
  action_priority_list_t* init_combust        = get_action_priority_list( "init_combust"      );
  action_priority_list_t* combust_sequence    = get_action_priority_list( "combust_sequence"  );
  action_priority_list_t* active_talents      = get_action_priority_list( "active_talents"    );
  action_priority_list_t* living_bomb         = get_action_priority_list( "living_bomb"       );
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe"               );
  action_priority_list_t* single_target       = get_action_priority_list( "single_target"     );

  */
  default_list -> add_action( this, "Counterspell",
                              "if=target.debuff.casting.react" );
  default_list -> add_action( this, "Time Warp",
                              "if=target.health.pct<25|time>5" );
  default_list -> add_talent( this, "Rune of Power",
                              "if=buff.rune_of_power.remains<cast_time" );
  /*
  default_list -> add_action( "call_action_list,name=combust_sequence,if=pyro_chain" );
  default_list -> add_action( "call_action_list,name=crystal_sequence,if=talent.prismatic_crystal.enabled&pet.prismatic_crystal.active" );
  default_list -> add_action( "call_action_list,name=init_combust,if=!pyro_chain" );
  default_list -> add_talent( this, "Rune of Power",
                              "if=buff.rune_of_power.remains<action.fireball.execute_time+gcd.max&!(buff.heating_up.up&action.fireball.in_flight)",
                              "Utilize level 90 active talents while avoiding pyro munching" );
  default_list -> add_talent( this, "Mirror Image",
                              "if=!(buff.heating_up.up&action.fireball.in_flight)" );
  default_list -> add_action( "call_action_list,name=aoe,if=active_enemies>10" );
  default_list -> add_action( "call_action_list,name=single_target");

  movement -> add_action( this, "Blink",
                          "if=movement.distance>10" );
  movement -> add_talent( this, "Blazing Speed",
                          "if=movement.remains>0" );
  movement -> add_talent( this, "Ice Floes",
                          "if=buff.ice_floes.down&(raid_event.movement.distance>0|raid_event.movement.in<action.fireball.cast_time)" );


  // TODO: Add multi LB explosions on multitarget fights.
  crystal_sequence -> add_action( "choose_target,name=prismatic_crystal",
                                  "Action list while Prismatic Crystal is up" );
  crystal_sequence -> add_action( this, "Inferno Blast",
                                  "if=dot.combustion.ticking&active_dot.combustion<active_enemies+1",
                                  "Spread Combustion from PC" );
  crystal_sequence -> add_action( this, "Inferno Blast",
                                  "cycle_targets=1,if=dot.combustion.ticking&active_dot.combustion<active_enemies",
                                  "Spread Combustion on multitarget fights" );
  crystal_sequence -> add_talent( this, "Cold Snap",
                                  "if=!cooldown.dragons_breath.up" );
  crystal_sequence -> add_action( this, "Dragon's Breath",
                                  "if=glyph.dragons_breath.enabled" );
  crystal_sequence -> add_talent( this, "Blast Wave" );
  crystal_sequence -> add_action( this, "Pyroblast",
                                  "if=execute_time=gcd.max&pet.prismatic_crystal.remains<gcd.max+travel_time&pet.prismatic_crystal.remains>travel_time",
                                  "Use pyros before PC's expiration" );
  crystal_sequence -> add_action( "call_action_list,name=single_target" );


  init_combust -> add_action( "start_pyro_chain,if=talent.meteor.enabled&cooldown.meteor.up&(legendary_ring.cooldown.remains<gcd.max|legendary_ring.cooldown.remains>target.time_to_die+15|!legendary_ring.has_cooldown)&((cooldown.combustion.remains<gcd.max*3&buff.pyroblast.up&(buff.heating_up.up^action.fireball.in_flight))|(buff.pyromaniac.up&(cooldown.combustion.remains<ceil(buff.pyromaniac.remains%gcd.max)*gcd.max)))",
                              "Combustion sequence initialization\n"
                              "# This sequence lists the requirements for preparing a Combustion combo with each talent choice\n"
                              "# Meteor Combustion" );
  init_combust -> add_action( "start_pyro_chain,if=talent.prismatic_crystal.enabled&cooldown.prismatic_crystal.up&(legendary_ring.cooldown.remains<gcd.max|legendary_ring.cooldown.remains>target.time_to_die+15|!legendary_ring.has_cooldown)&((cooldown.combustion.remains<gcd.max*2&buff.pyroblast.up&(buff.heating_up.up^action.fireball.in_flight))|(buff.pyromaniac.up&(cooldown.combustion.remains<ceil(buff.pyromaniac.remains%gcd.max)*gcd.max)))",
                              "Prismatic Crystal Combustion" );
  init_combust -> add_action( "start_pyro_chain,if=talent.prismatic_crystal.enabled&!glyph.combustion.enabled&cooldown.prismatic_crystal.remains>20&((cooldown.combustion.remains<gcd.max*2&buff.pyroblast.up&buff.heating_up.up&action.fireball.in_flight)|(buff.pyromaniac.up&(cooldown.combustion.remains<ceil(buff.pyromaniac.remains%gcd.max)*gcd.max)))",
                              "Unglyphed Combustions between Prismatic Crystals" );
  init_combust -> add_action( "start_pyro_chain,if=!talent.prismatic_crystal.enabled&!talent.meteor.enabled&((cooldown.combustion.remains<gcd.max*4&buff.pyroblast.up&buff.heating_up.up&action.fireball.in_flight)|(buff.pyromaniac.up&cooldown.combustion.remains<ceil(buff.pyromaniac.remains%gcd.max)*(gcd.max+talent.kindling.enabled)))",
                              "Kindling or Level 90 Combustion" );

  combust_sequence -> add_talent( this, "Prismatic Crystal", "",
                                  "Combustion Sequence" );
  for( size_t i = 0; i < racial_actions.size(); i++ )
    combust_sequence -> add_action( racial_actions[i] );
  for( size_t i = 0; i < item_actions.size(); i++ )
    combust_sequence -> add_action( item_actions[i] );
  combust_sequence -> add_action( get_potion_action() );
  combust_sequence -> add_talent( this, "Meteor",
                                  "if=active_enemies<=2" );
  combust_sequence -> add_action( this, "Pyroblast",
                                  "if=set_bonus.tier17_4pc&buff.pyromaniac.up" );
  combust_sequence -> add_action( this, "Inferno Blast",
                                  "if=set_bonus.tier16_4pc_caster&(buff.pyroblast.up^buff.heating_up.up)" );
  combust_sequence -> add_action( this, "Fireball",
                                  "if=!dot.ignite.ticking&!in_flight" );
  combust_sequence -> add_action( this, "Pyroblast",
                                  "if=buff.pyroblast.up&dot.ignite.tick_dmg*(6-ceil(dot.ignite.remains-travel_time))<crit_damage*mastery_value" );
  combust_sequence -> add_action( this, "Inferno Blast",
                                  "if=talent.meteor.enabled&cooldown.meteor.duration-cooldown.meteor.remains<gcd.max*3",
                                  "Meteor Combustions can run out of Pyro procs before impact. Use IB to delay Combustion" );
  combust_sequence -> add_action( this, "Inferno Blast",
                                  "if=dot.ignite.tick_dmg*(6-dot.ignite.ticks_remain)<crit_damage*mastery_value" );
  combust_sequence -> add_action( this, "Combustion" );


  active_talents -> add_talent( this, "Meteor",
                                "if=active_enemies>=3|(glyph.combustion.enabled&(!talent.incanters_flow.enabled|buff.incanters_flow.stack+incanters_flow_dir>=4)&cooldown.meteor.duration-cooldown.combustion.remains<10)",
                                "Active talents usage" );
  active_talents -> add_action( "call_action_list,name=living_bomb,if=talent.living_bomb.enabled&(active_enemies>1|raid_event.adds.in<10)" );
  active_talents -> add_talent( this, "Cold Snap",
                                "if=glyph.dragons_breath.enabled&!talent.prismatic_crystal.enabled&!cooldown.dragons_breath.up" );
  active_talents -> add_action( this, "Dragon's Breath",
                                "if=glyph.dragons_breath.enabled&(!talent.prismatic_crystal.enabled|cooldown.prismatic_crystal.remains>8)" );
  active_talents -> add_talent( this, "Blast Wave",
                                "if=(!talent.incanters_flow.enabled|buff.incanters_flow.stack>=4)&(target.time_to_die<10|!talent.prismatic_crystal.enabled|(charges>=1&cooldown.prismatic_crystal.remains>recharge_time))" );


  living_bomb -> add_action( this, "Inferno Blast",
                             "cycle_targets=1,if=dot.living_bomb.ticking&active_enemies-active_dot.living_bomb>1",
                             "Living Bomb application" );
  living_bomb -> add_talent( this, "Living Bomb",
                             "if=target!=pet.prismatic_crystal&(((!talent.incanters_flow.enabled|incanters_flow_dir<0|buff.incanters_flow.stack=5)&remains<3.6)|((incanters_flow_dir>0|buff.incanters_flow.stack=1)&remains<gcd.max))&target.time_to_die>remains+12" );


  aoe -> add_action( this, "Inferno Blast",
                     "cycle_targets=1,if=(dot.combustion.ticking&active_dot.combustion<active_enemies)|(dot.pyroblast.ticking&active_dot.pyroblast<active_enemies)",
                     "AoE sequence" );
  aoe -> add_action( "call_action_list,name=active_talents" );
  aoe -> add_action( this, "Pyroblast",
                     "if=buff.pyroblast.react|buff.pyromaniac.react" );
  aoe -> add_talent( this, "Cold Snap",
                     "if=!cooldown.dragons_breath.up" );
  aoe -> add_action( this, "Dragon's Breath" );
  aoe -> add_action( this, "Flamestrike",
                     "if=mana.pct>10&remains<2.4"  );


  single_target -> add_action( this, "Inferno Blast",
                               "if=dot.combustion.ticking&active_dot.combustion<active_enemies",
                               "Single target sequence" );
  single_target -> add_action( this, "Pyroblast",
                               "if=buff.pyroblast.up&buff.pyroblast.remains<action.fireball.execute_time",
                               "Use Pyro procs before they run out" );
  single_target -> add_action( this, "Pyroblast",
                               "if=set_bonus.tier16_2pc_caster&buff.pyroblast.up&buff.potent_flames.up&buff.potent_flames.remains<gcd.max" );
  single_target -> add_action( this, "Pyroblast",
                               "if=set_bonus.tier17_4pc&buff.pyromaniac.react" );
  single_target -> add_action( this, "Pyroblast",
                               "if=buff.pyroblast.up&buff.heating_up.up&action.fireball.in_flight",
                               "Pyro camp during regular sequence; Do not use Pyro procs without HU and first using fireball" );
  single_target -> add_action( this, "Inferno Blast",
                               "if=buff.pyroblast.down&buff.heating_up.up&!(dot.living_bomb.remains>10&active_enemies>1)",
                               "Heating Up conversion to Pyroblast" );
  single_target -> add_action( "call_action_list,name=active_talents" );
  single_target -> add_action( this, "Inferno Blast",
                               "if=buff.pyroblast.up&buff.heating_up.down&!action.fireball.in_flight&!(dot.living_bomb.remains>10&active_enemies>1)",
                               "Adding Heating Up to Pyroblast" );
  single_target -> add_action( this, "Fireball" );
  single_target -> add_action( this, "Scorch", "moving=1" );
  */
}

// Frost Mage Action List ==============================================================================================================

void mage_t::apl_frost()
{
  std::vector<std::string> item_actions = get_item_actions();
  std::vector<std::string> racial_actions = get_racial_actions();

  action_priority_list_t* default_list      = get_action_priority_list( "default"          );
  /*

  action_priority_list_t* movement          = get_action_priority_list( "movement"         );
  action_priority_list_t* crystal_sequence  = get_action_priority_list( "crystal_sequence" );
  action_priority_list_t* cooldowns         = get_action_priority_list( "cooldowns"        );
  action_priority_list_t* init_water_jet    = get_action_priority_list( "init_water_jet"   );
  action_priority_list_t* water_jet         = get_action_priority_list( "water_jet"        );
  action_priority_list_t* aoe               = get_action_priority_list( "aoe"              );
  action_priority_list_t* single_target     = get_action_priority_list( "single_target"    );
  */

  default_list -> add_action( this, "Counterspell",
                              "if=target.debuff.casting.react" );
  default_list -> add_action( this, "Time Warp",
                              "if=target.health.pct<25|time>5" );
  default_list -> add_talent( this, "Mirror Image" );
  default_list -> add_talent( this, "Rune of Power",
                              "if=buff.rune_of_power.remains<cast_time" );
  default_list -> add_talent( this, "Rune of Power",
                              "if=(cooldown.icy_veins.remains<gcd.max&buff.rune_of_power.remains<20)|(cooldown.prismatic_crystal.remains<gcd.max&buff.rune_of_power.remains<10)" );
  /*
  default_list -> add_action( "call_action_list,name=cooldowns,if=target.time_to_die<24" );
  default_list -> add_action( "call_action_list,name=crystal_sequence,if=talent.prismatic_crystal.enabled&(cooldown.prismatic_crystal.remains<=gcd.max|pet.prismatic_crystal.active)" );
  default_list -> add_action( "call_action_list,name=water_jet,if=prev_off_gcd.water_jet|debuff.water_jet.remains>0" );
  default_list -> add_action( "water_jet,if=time<1&active_enemies<4&!(talent.ice_nova.enabled&talent.prismatic_crystal.enabled)",
                              "Water jet on pull for non PC talent combos" );
  default_list -> add_action( "call_action_list,name=aoe,if=active_enemies>=4" );
  default_list -> add_action( "call_action_list,name=single_target" );


  movement -> add_action( this, "Blink",
                          "if=movement.distance>10" );
  movement -> add_talent( this, "Blazing Speed",
                          "if=movement.remains>0" );
  movement -> add_talent( this, "Ice Floes",
                          "if=buff.ice_floes.down&(raid_event.movement.distance>0|raid_event.movement.in<action.frostbolt.cast_time)" );


  crystal_sequence -> add_talent( this, "Frost Bomb",
                                  "if=active_enemies=1&current_target!=pet.prismatic_crystal&remains<10",
                                  "Actions while Prismatic Crystal is active" );
  crystal_sequence -> add_action( this, "Frozen Orb",
                                  "target_if=max:target.time_to_die&target!=pet.prismatic_crystal" );
  crystal_sequence -> add_talent( this, "Prismatic Crystal" );
  crystal_sequence -> add_action( "call_action_list,name=cooldowns" );
  crystal_sequence -> add_talent( this, "Frost Bomb",
                                  "if=talent.prismatic_crystal.enabled&current_target=pet.prismatic_crystal&active_enemies>1&!ticking" );
  crystal_sequence -> add_action( this, "Ice Lance",
                                  "if=!t18_class_trinket&(buff.fingers_of_frost.react>=2+set_bonus.tier18_4pc*2|(buff.fingers_of_frost.react>set_bonus.tier18_4pc*2&active_dot.frozen_orb))" );
  crystal_sequence -> add_action( "water_jet,if=pet.prismatic_crystal.remains>(5+10*set_bonus.tier18_4pc)*spell_haste*0.8" );
  crystal_sequence -> add_talent( this, "Ice Nova",
                                  "if=charges=2|pet.prismatic_crystal.remains<4" );
  crystal_sequence -> add_action( this, "Ice Lance",
                                  "if=buff.fingers_of_frost.react&buff.shatterlance.up" );
  crystal_sequence -> add_action( this, "Frostfire Bolt",
                                  "if=buff.brain_freeze.react=2" );
  crystal_sequence -> add_action( this, "Frostbolt",
                                  "target_if=max:debuff.water_jet.remains,if=t18_class_trinket&buff.fingers_of_frost.react&!buff.shatterlance.up&pet.prismatic_crystal.remains>cast_time" );
  crystal_sequence -> add_action( this, "Ice Lance",
                                  "if=buff.fingers_of_frost.react" );
  crystal_sequence -> add_action( this, "Frostfire Bolt",
                                  "if=buff.brain_freeze.react" );
  crystal_sequence -> add_talent( this, "Ice Nova" );
  crystal_sequence -> add_action( this, "Blizzard",
                                 "interrupt_if=cooldown.frozen_orb.up|(talent.frost_bomb.enabled&buff.fingers_of_frost.react>=2+set_bonus.tier18_4pc),if=active_enemies>=5" );
  crystal_sequence -> add_action( "choose_target,if=pet.prismatic_crystal.remains<action.frostbolt.cast_time+action.frostbolt.travel_time" );
  crystal_sequence -> add_action( this, "Frostbolt" );


  cooldowns -> add_action( this, "Icy Veins",
                           "",
                           "Consolidated damage cooldown abilities" );

  for( size_t i = 0; i < racial_actions.size(); i++ )
    cooldowns -> add_action( racial_actions[i] );

  cooldowns -> add_action( get_potion_action() + ",if=buff.bloodlust.up|buff.icy_veins.up" );

  for( size_t i = 0; i < item_actions.size(); i++ )
    cooldowns -> add_action( item_actions[i] );


  init_water_jet -> add_talent( this, "Frost Bomb",
                                "if=remains<4*spell_haste*(1+set_bonus.tier18_4pc)+cast_time",
                                "Water Jet initialization" );
  init_water_jet -> add_action( "water_jet,if=prev_gcd.frostbolt|action.frostbolt.travel_time<spell_haste" );
  init_water_jet -> add_action( this, "Frostbolt" );


  water_jet -> add_action( this, "Frostbolt",
                           "if=prev_off_gcd.water_jet",
                           "Water Jet sequence" );
  water_jet -> add_action( this, "Ice Lance",
                           "if=set_bonus.tier18_4pc&buff.fingers_of_frost.react>2*set_bonus.tier18_4pc&buff.shatterlance.up" );
  water_jet -> add_action( this, "Frostfire Bolt",
                           "if=set_bonus.tier18_4pc&buff.brain_freeze.react=2" );
  water_jet -> add_action( this, "frostbolt",
                           "if=t18_class_trinket&debuff.water_jet.remains>cast_time+travel_time&buff.fingers_of_frost.react&!buff.shatterlance.up" );
  water_jet -> add_action( this, "Ice Lance",
                           "if=!t18_class_trinket&buff.fingers_of_frost.react>=2+2*set_bonus.tier18_4pc&action.frostbolt.in_flight" );
  water_jet -> add_action( this, "Frostbolt",
                           "if=!set_bonus.tier18_4pc&debuff.water_jet.remains>cast_time+travel_time" );


  aoe -> add_action( "call_action_list,name=cooldowns",
                     "AoE sequence" );
  aoe -> add_talent( this, "Frost Bomb",
                     "if=remains<action.ice_lance.travel_time&(cooldown.frozen_orb.remains<gcd.max|buff.fingers_of_frost.react>=2)" );
  aoe -> add_action( this, "Frozen Orb" );
  aoe -> add_action( this, "Ice Lance",
                     "if=talent.frost_bomb.enabled&buff.fingers_of_frost.react&debuff.frost_bomb.up" );
  aoe -> add_talent( this, "Comet Storm" );
  aoe -> add_talent( this, "Ice Nova" );
  aoe -> add_action( this, "Blizzard",
                     "interrupt_if=cooldown.frozen_orb.up|(talent.frost_bomb.enabled&buff.fingers_of_frost.react>=2)" );


  single_target -> add_action( "call_action_list,name=cooldowns,if=!talent.prismatic_crystal.enabled|cooldown.prismatic_crystal.remains>15",
                               "Single target sequence" );
  single_target -> add_action( this, "Ice Lance",
                               "if=buff.fingers_of_frost.react&(buff.fingers_of_frost.remains<action.frostbolt.execute_time|buff.fingers_of_frost.remains<buff.fingers_of_frost.react*gcd.max)",
                               "Safeguards against losing FoF and BF to buff expiry" );
  single_target -> add_action( this, "Frostfire Bolt",
                               "if=buff.brain_freeze.react&(buff.brain_freeze.remains<action.frostbolt.execute_time|buff.brain_freeze.remains<buff.brain_freeze.react*gcd.max)" );
  single_target -> add_talent( this, "Frost Bomb",
                               "if=!talent.prismatic_crystal.enabled&cooldown.frozen_orb.remains<gcd.max&debuff.frost_bomb.remains<10",
                               "Frozen Orb usage without Prismatic Crystal" );
  single_target -> add_action( this, "Frozen Orb",
                               "if=!talent.prismatic_crystal.enabled&buff.fingers_of_frost.stack<2&cooldown.icy_veins.remains>45-20*talent.thermal_void.enabled" );
  single_target -> add_action( this, "Ice Lance",
                               "if=buff.fingers_of_frost.react&buff.shatterlance.up",
                               "Single target routine; Rough summary: IN2 > FoF2 > CmS > IN > BF > FoF" );
  single_target -> add_talent( this, "Frost Bomb",
                               "if=remains<action.ice_lance.travel_time+t18_class_trinket*action.frostbolt.execute_time&(buff.fingers_of_frost.react>=(2+set_bonus.tier18_4pc*2)%(1+t18_class_trinket)|(buff.fingers_of_frost.react&(talent.thermal_void.enabled|buff.fingers_of_frost.remains<gcd.max*(buff.fingers_of_frost.react+1))))" );
  single_target -> add_talent( this, "Ice Nova",
                               "if=target.time_to_die<10|(charges=2&(!talent.prismatic_crystal.enabled|!cooldown.prismatic_crystal.up))" );
  single_target -> add_action( this, "Ice Lance",
                               "if=!t18_class_trinket&(buff.fingers_of_frost.react>=2+set_bonus.tier18_4pc*2|(buff.fingers_of_frost.react>1+set_bonus.tier18_4pc&active_dot.frozen_orb))" );
  single_target -> add_talent( this, "Comet Storm" );
  single_target -> add_talent( this, "Ice Nova",
                               "if=(!talent.prismatic_crystal.enabled|(charges=1&cooldown.prismatic_crystal.remains>recharge_time&(buff.incanters_flow.stack>3|!talent.ice_nova.enabled)))&(buff.icy_veins.up|(charges=1&cooldown.icy_veins.remains>recharge_time))" );
  single_target -> add_action( this, "Frostfire Bolt",
                               "if=buff.brain_freeze.react" );
  single_target -> add_action( "call_action_list,name=init_water_jet,if=pet.water_elemental.cooldown.water_jet.remains<=gcd.max*talent.frost_bomb.enabled&buff.fingers_of_frost.react<2+2*set_bonus.tier18_4pc&!active_dot.frozen_orb" );
  single_target -> add_action( this, "Frostbolt",
                               "if=t18_class_trinket&buff.fingers_of_frost.react&!buff.shatterlance.up" );
  single_target -> add_action( this, "Ice Lance",
                               "if=talent.frost_bomb.enabled&buff.fingers_of_frost.react&debuff.frost_bomb.remains>travel_time&(!talent.thermal_void.enabled|cooldown.icy_veins.remains>8)" );
  single_target -> add_action( this, "Frostbolt",
                               "if=set_bonus.tier17_2pc&buff.ice_shard.up&!(talent.thermal_void.enabled&buff.icy_veins.up&buff.icy_veins.remains<10)",
                               "Camp procs and spam Frostbolt while 4T17 buff is up" );
  single_target -> add_action( this, "Ice Lance",
                               "if=!talent.frost_bomb.enabled&buff.fingers_of_frost.react&(!talent.thermal_void.enabled|cooldown.icy_veins.remains>8)" );
  single_target -> add_action( this, "Frostbolt" );
  single_target -> add_action( this, "Ice Lance", "moving=1" );
  */

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
    mps *= 1.0 + cache.mastery_value() * spec.savant -> effectN( 1 ).sp_coeff();
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
      cache.mastery_value() * spec.savant -> effectN( 1 ).sp_coeff();
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

// mage_t::composite_player_multiplier =======================================

double mage_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  // TODO: which of these cache invalidations are actually needed?
  if ( specialization() == MAGE_ARCANE )
  {
    if ( buffs.arcane_power -> check() )
    {
      double v = buffs.arcane_power -> value();
      m *= 1.0 + v;
    }

    cache.player_mult_valid[ school ] = false;
  }

  // TODO: which of these cache invalidations are actually needed?
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

// mage_t::composite_spell_crit ===============================================

double mage_t::composite_spell_crit() const
{
  double c = player_t::composite_spell_crit();

  if ( buffs.molten_armor -> check() )
  {
    c += buffs.molten_armor -> data().effectN( 1 ).percent();
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

  if ( buffs.frost_armor -> check() )
  {
    h /= 1.0 + buffs.frost_armor -> data().effectN( 1 ).percent();
  }

  if ( buffs.icarus_uprising -> check() )
  {
    h /= 1.0 + buffs.icarus_uprising -> data().effectN( 1 ).percent();
  }

  // TODO: Double check scaling with hits.
  if ( buffs.quickening -> check() )
  {
    h /= 1.0 + buffs.quickening -> data().effectN( 1 ).percent() * buffs.quickening -> current_stack;
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
    pets::temporal_hero_t::randomize_last_summoned( this );
  }
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

  if ( spec.ignite -> ok()  )
  {
    ignite_spread_event = new ( *sim )events::ignite_spread_event_t( *this );
    timespan_t first_spread = timespan_t::from_seconds( rng().real() * 2.0 );
    ignite_spread_event -> add_event( first_spread );
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
      if ( a -> type != ACTION_CALL )
        return a;
      // Call_action_list action, don't execute anything, but rather recurse
      // into the called action list.
      else
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
        if ( mage.icicles.size() == 0 )
          return 0;
        else if ( mage.sim -> current_time() - mage.icicles[ 0 ].first < mage.spec.icicles_driver -> duration() )
          return static_cast<double>(mage.icicles.size());
        else
        {
          size_t icicles = 0;
          for ( int i = as<int>( mage.icicles.size() - 1 ); i >= 0; i-- )
          {
            if ( mage.sim -> current_time() - mage.icicles[ i ].first >= mage.spec.icicles_driver -> duration() )
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
  if ( icicles.size() == 0 )
    return icicle_data_t( (double) 0, (stats_t*) nullptr );

  timespan_t threshold = spec.icicles_driver -> duration();

  auto idx = icicles.begin(),
       end = icicles.end();
  for ( ; idx < end; ++idx )
  {
    if ( sim -> current_time() - ( *idx ).first < threshold )
      break;
  }

  // Set of icicles timed out
  if ( idx != icicles.begin() )
    icicles.erase( icicles.begin(), idx );

  if ( icicles.size() > 0 )
  {
    icicle_data_t d = icicles.front().second;
    icicles.erase( icicles.begin() );
    return d;
  }

  return icicle_data_t( (double) 0, (stats_t*) nullptr );
}

void mage_t::trigger_icicle( const action_state_t* trigger_state, bool chain, player_t* chain_target )
{
  if ( ! spec.icicles -> ok() )
    return;

  if ( icicles.size() == 0 )
    return;

  std::pair<double, stats_t*> d;

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
    d = get_icicle_object();
    if ( d.first == 0 )
      return;

    icicle_event = new ( *sim ) events::icicle_event_t( *this, d, icicle_target, true );

    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s icicle use on %s%s, damage=%f, total=%u",
                               name(), icicle_target -> name(),
                               chain ? " (chained)" : "", d.first,
                               as<unsigned>( icicles.size() ) );
    }
  }
  else if ( ! chain )
  {
    d = get_icicle_object();
    if ( d.first == 0 )
      return;

    icicle -> base_dd_min = icicle -> base_dd_max = d.first;

    actions::icicle_state_t* new_state = debug_cast<actions::icicle_state_t*>( icicle -> get_state() );
    new_state -> target = icicle_target;
    new_state -> source = d.second;

    // Immediately execute icicles so the correct damage is carried into the
    // travelling icicle object
    icicle -> pre_execute_state = new_state;
    icicle -> execute();

    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s icicle use on %s%s, damage=%f, total=%u",
                               name(), icicle_target -> name(),
                               chain ? " (chained)" : "", d.first,
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

// Generic Legendary Items
using namespace unique_gear;
using namespace actions;

struct koralons_burning_touch_t : public scoped_action_callback_t<scorch_t>
{
  koralons_burning_touch_t() : super( MAGE_FIRE, "scorch" )
  { }

  void manipulate( scorch_t* action, const special_effect_t& e ) override
  { action -> scorch_multiplier = e.driver() -> effectN( 1 ).percent(); }

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

static void pyrosurge( special_effect_t& effect )
{
  mage_t* p = debug_cast<mage_t*>( effect.player );
  do_trinket_init( p, MAGE_FIRE, p -> pyrosurge, effect );
}

static void shatterlance( special_effect_t& effect )
{
  mage_t* p = debug_cast<mage_t*>( effect.player );
  do_trinket_init( p, MAGE_FROST, p -> shatterlance, effect );
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
    unique_gear::register_special_effect( 184903, wild_arcanist );
    unique_gear::register_special_effect( 184904, pyrosurge     );
    unique_gear::register_special_effect( 184905, shatterlance  );
    unique_gear::register_special_effect( 208099, koralons_burning_touch_t() );
  }

  virtual void register_hotfixes() const override
  {
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
