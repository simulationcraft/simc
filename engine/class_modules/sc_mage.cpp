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

/// Icicle container object, contains a timestamp and its corresponding icicle data!
struct icicle_tuple_t
{
  timespan_t timestamp;
  action_t*  icicle_action;
};

struct mage_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* nether_tempest;
  } dots;

  struct debuffs_t
  {
    buff_t* frozen;
    buff_t* winters_chill;
    buff_t* touch_of_the_magi;

    // Azerite
    buff_t* packed_ice;
    buff_t* preheat;
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

struct cooldown_reduction_data_t
{
  const cooldown_t* cd;

  luxurious_sample_data_t* effective;
  luxurious_sample_data_t* wasted;

  cooldown_reduction_data_t( const cooldown_t* cooldown, const std::string& name ) :
    cd( cooldown )
  {
    player_t* p = cd -> player;

    effective = p -> get_sample_data( name + " effective cooldown reduction" );
    wasted    = p -> get_sample_data( name + " wasted cooldown reduction" );
  }

  void add( timespan_t reduction )
  {
    assert( effective );
    assert( wasted );

    timespan_t remaining = timespan_t::zero();

    if ( cd -> charges > 1 )
    {
      if ( cd -> recharge_event )
      {
        remaining = cd -> current_charge_remains() +
          ( cd -> charges - cd -> current_charge - 1 ) * cd -> duration;
      }
    }
    else
    {
      remaining = cd -> remains();
    }

    double reduction_sec = -reduction.total_seconds();
    double remaining_sec = remaining.total_seconds();

    double effective_sec = std::min( reduction_sec, remaining_sec );
    effective -> add( effective_sec );

    double wasted_sec = reduction_sec - effective_sec;
    wasted -> add( wasted_sec );
  }
};

struct cooldown_waste_data_t : private noncopyable
{
  const cooldown_t* cd;
  double buffer;

  extended_sample_data_t normal;
  extended_sample_data_t cumulative;

  cooldown_waste_data_t( const cooldown_t* cooldown, bool simple = true ) :
    cd( cooldown ),
    buffer( 0.0 ),
    normal( cd -> name_str + " cooldown waste", simple ),
    cumulative( cd -> name_str + " cooldown cumulative waste", simple )
  { }

  bool may_add( timespan_t cd_override = timespan_t::min() ) const
  {
    return ( cd -> duration > timespan_t::zero() || cd_override > timespan_t::zero() )
        && ( ( cd -> charges == 1 && cd -> up() ) || ( cd -> charges >= 2 && cd -> current_charge == cd -> charges ) );
  }

  void add( timespan_t cd_override = timespan_t::min(), timespan_t time_to_execute = timespan_t::zero() )
  {
    if ( may_add( cd_override ) )
    {
      double wasted = ( cd -> sim.current_time() - cd -> last_charged ).total_seconds();
      if ( cd -> charges == 1 )
      {
        // Waste caused by execute time is unavoidable for single charge spells,
        // don't count it.
        wasted -= time_to_execute.total_seconds();
      }
      normal.add( wasted );
      buffer += wasted;
    }
  }

  bool active() const
  {
    return normal.count() > 0 && cumulative.sum() > 0;
  }

  void merge( const cooldown_waste_data_t& other )
  {
    normal.merge( other.normal );
    cumulative.merge( other.cumulative );
  }

  void analyze()
  {
    normal.analyze();
    cumulative.analyze();
  }

  void datacollection_begin()
  {
    buffer = 0.0;
  }

  void datacollection_end()
  {
    if ( may_add() )
      buffer += ( cd -> sim.current_time() - cd -> last_charged ).total_seconds();

    cumulative.add( buffer );
    buffer = 0.0;
  }
};

struct proc_source_t : private noncopyable
{
  const std::string name_str;
  auto_dispose<std::vector<proc_t*> > procs;

  proc_source_t( sim_t& sim, const std::string& name, size_t count ) :
    name_str( name )
  {
    for ( size_t i = 0; i < count; i++ )
    {
      procs.push_back( new proc_t( sim, name_str + " " + util::to_string( i ) ) );
    }
  }

  void occur( size_t index )
  {
    assert( index < procs.size() );
    procs[ index ] -> occur();
  }

  const proc_t& get( size_t index ) const
  {
    assert( index < procs.size() );
    return *procs[ index ];
  }

  bool active() const
  {
    return range::find_if( procs, [] ( proc_t* p ) { return p -> count.sum() > 0.0; } ) != procs.end();
  }

  void reset()
  {
    range::for_each( procs, std::mem_fn( &proc_t::reset ) );
  }

  void merge( const proc_source_t& other )
  {
    for ( size_t i = 0; i < procs.size(); i++ )
    {
      procs[ i ] -> merge( *other.procs[ i ] );
    }
  }

  void datacollection_begin()
  {
    range::for_each( procs, std::mem_fn( &proc_t::datacollection_begin ) );
  }

  void datacollection_end()
  {
    range::for_each( procs, std::mem_fn( &proc_t::datacollection_end ) );
  }
};

struct mage_t : public player_t
{
public:
  // Icicles
  std::vector<icicle_tuple_t> icicles;
  event_t* icicle_event;

  struct icicles_t
  {
    action_t* frostbolt;
    action_t* flurry;
  } icicle;

  // Ignite
  action_t* ignite;
  event_t* ignite_spread_event;

  // Time Anomaly
  event_t* time_anomaly_tick_event;

  // Active
  player_t* last_bomb_target;
  player_t* last_frostbolt_target;

  // State switches for rotation selection
  state_switch_t burn_phase;

  // Ground AoE tracking
  std::map<std::string, timespan_t> ground_aoe_expiration;

  // Miscellaneous
  double distance_from_rune;
  timespan_t firestarter_time;
  int blessing_of_wisdom_count;
  bool allow_shimmer_lance;

  // Data collection
  auto_dispose<std::vector<cooldown_waste_data_t*> > cooldown_waste_data_list;
  auto_dispose<std::vector<proc_source_t*> > proc_source_list;

  // Cached actions
  struct actions_t
  {
    action_t* arcane_assault;
    action_t* conflagration_flare_up;
    action_t* glacial_assault;
    action_t* touch_of_the_magi_explosion;
    action_t* legendary_arcane_orb;
    action_t* legendary_meteor;
    action_t* legendary_comet_storm;
  } action;

  // Benefits
  struct benefits_t
  {
    struct arcane_charge_benefits_t
    {
      buff_stack_benefit_t* arcane_barrage;
      buff_stack_benefit_t* arcane_blast;
      buff_stack_benefit_t* nether_tempest;
    } arcane_charge;

    buff_stack_benefit_t* magtheridons_might;
    buff_stack_benefit_t* zannesu_journey;
  } benefits;

  // Buffs
  struct buffs_t
  {
    // Arcane
    buff_t* arcane_charge;
    buff_t* arcane_power;
    buff_t* clearcasting;
    buff_t* clearcasting_channel; // Hidden buff which governs tick and channel time
    buff_t* evocation;
    buff_t* presence_of_mind;

    buff_t* arcane_familiar;
    buff_t* chrono_shift;
    buff_t* rule_of_threes;

    buff_t* crackling_energy; // T20 2pc Arcane
    buff_t* expanding_mind;   // T21 2pc Arcane
    buff_t* quick_thinker;    // T21 4pc Arcane

    buff_t* rhonins_assaulting_armwraps;


    // Fire
    buff_t* combustion;
    buff_t* enhanced_pyrotechnics;
    buff_t* heating_up;
    buff_t* hot_streak;

    buff_t* frenetic_speed;
    buff_t* pyroclasm;

    buff_t* streaking;                 // T19 4pc Fire
    buff_t* ignition;                  // T20 2pc Fire
    buff_t* critical_massive;          // T20 4pc Fire
    buff_t* inferno;                   // T21 4pc Fire

    buff_t* contained_infernal_core;   // 7.2.5 legendary shoulder, tracking buff
    buff_t* erupting_infernal_core;    // 7.2.5 legendary shoulder, primed buff
    buff_t* kaelthas_ultimate_ability;


    // Frost
    buff_t* brain_freeze;
    buff_t* fingers_of_frost;
    buff_t* icicles;                           // Buff to track icicles - doesn't always line up with icicle count though!
    buff_t* icy_veins;

    buff_t* bone_chilling;
    buff_t* chain_reaction;
    buff_t* freezing_rain;
    buff_t* ice_floes;
    buff_t* ray_of_frost;

    buff_t* frozen_mass;                       // T20 2pc Frost
    buff_t* arctic_blast;                      // T21 4pc Frost

    buff_t* lady_vashjs_grasp;
    buff_t* magtheridons_might;
    buff_t* rage_of_the_frost_wyrm;            // 7.2.5 legendary head, primed buff
    buff_t* shattered_fragments_of_sindragosa; // 7.2.5 legendary head, tracking buff
    buff_t* zannesu_journey;


    // Shared
    buff_t* incanters_flow;
    buff_t* rune_of_power;

    buff_t* sephuzs_secret;

    // Azerite
    buff_t* arcane_pummeling;
    buff_t* brain_storm;

    buff_t* blaster_master;
    buff_t* firemind;
    buff_t* flames_of_alacrity;

    buff_t* frigid_grasp;
    buff_t* tunnel_of_ice;
    buff_t* winters_reach;

    // Miscellaneous Buffs
    buff_t* greater_blessing_of_widsom;
    buff_t* t19_oh_buff;
    buff_t* shimmer;

  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* combustion;
    cooldown_t* cone_of_cold;
    cooldown_t* evocation;
    cooldown_t* frost_nova;
    cooldown_t* frozen_orb;
    cooldown_t* presence_of_mind;
    cooldown_t* time_warp;
  } cooldowns;

  // Gains
  struct gains_t
  {
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
    { }
  } pets;

  // Procs
  struct procs_t
  {
    proc_t* heating_up_generated;         // Crits without HU/HS
    proc_t* heating_up_removed;           // Non-crits with HU >200ms after application
    proc_t* heating_up_ib_converted;      // IBs used on HU
    proc_t* hot_streak;                   // Total HS generated
    proc_t* hot_streak_pyromaniac;        // Total HS from Pyromaniac
    proc_t* hot_streak_spell;             // HU/HS spell impacts
    proc_t* hot_streak_spell_crit;        // HU/HS spell crits
    proc_t* hot_streak_spell_crit_wasted; // HU/HS spell crits with HS

    proc_t* ignite_applied;    // Direct ignite applications
    proc_t* ignite_spread;     // Spread events
    proc_t* ignite_new_spread; // Spread to new target
    proc_t* ignite_overwrite;  // Spread to target with existing ignite


    proc_t* brain_freeze_flurry;
    proc_t* fingers_of_frost_wasted;
  } procs;

  struct shuffled_rngs_t
  {
    shuffled_rng_t* time_anomaly;
  } shuffled_rng;

  // Sample data
  struct sample_data_t
  {
    cooldown_reduction_data_t* blizzard;
    cooldown_reduction_data_t* t20_4pc;

    extended_sample_data_t* icy_veins_duration;

    extended_sample_data_t* burn_duration_history;
    extended_sample_data_t* burn_initial_mana;
  } sample_data;

  // Specializations
  struct specializations_t
  {
    // Arcane
    const spell_data_t* arcane_barrage_2;
    const spell_data_t* arcane_charge;
    const spell_data_t* arcane_mage;
    const spell_data_t* clearcasting;
    const spell_data_t* evocation_2;
    const spell_data_t* savant;

    // Fire
    const spell_data_t* critical_mass;
    const spell_data_t* critical_mass_2;
    const spell_data_t* enhanced_pyrotechnics;
    const spell_data_t* fire_blast_2;
    const spell_data_t* fire_blast_3;
    const spell_data_t* fire_mage;
    const spell_data_t* hot_streak;
    const spell_data_t* ignite;

    // Frost
    const spell_data_t* brain_freeze;
    const spell_data_t* brain_freeze_2;
    const spell_data_t* blizzard_2;
    const spell_data_t* fingers_of_frost;
    const spell_data_t* frost_mage;
    const spell_data_t* icicles;
    const spell_data_t* shatter;
    const spell_data_t* shatter_2;
  } spec;

  // State
  struct state_t
  {
    bool brain_freeze_active;
    bool winters_reach_active;
    bool fingers_of_frost_active;

    int flurry_bolt_count;
  } state;

  // Talents
  struct talents_list_t
  {
    // Tier 15
    const spell_data_t* amplification;
    const spell_data_t* rule_of_threes;
    const spell_data_t* arcane_familiar;
    const spell_data_t* firestarter;
    const spell_data_t* pyromaniac;
    const spell_data_t* searing_touch;
    const spell_data_t* bone_chilling;
    const spell_data_t* lonely_winter;
    const spell_data_t* ice_nova;

    // Tier 30
    const spell_data_t* shimmer;
    const spell_data_t* mana_shield; // NYI
    const spell_data_t* slipstream;
    const spell_data_t* blazing_soul; // NYI
    const spell_data_t* blast_wave;
    const spell_data_t* glacial_insulation; // NYI
    const spell_data_t* ice_floes;

    // Tier 45
    const spell_data_t* incanters_flow;
    const spell_data_t* mirror_image;
    const spell_data_t* rune_of_power;

    // Tier 60
    const spell_data_t* resonance;
    const spell_data_t* charged_up;
    const spell_data_t* supernova;
    const spell_data_t* flame_on;
    const spell_data_t* alexstraszas_fury;
    const spell_data_t* phoenix_flames;
    const spell_data_t* frozen_touch;
    const spell_data_t* chain_reaction;
    const spell_data_t* ebonbolt;

    // Tier 75
    const spell_data_t* ice_ward;
    const spell_data_t* ring_of_frost; // NYI
    const spell_data_t* chrono_shift;
    const spell_data_t* frenetic_speed;
    const spell_data_t* frigid_winds; // NYI

    // Tier 90
    const spell_data_t* reverberate;
    const spell_data_t* touch_of_the_magi;
    const spell_data_t* nether_tempest;
    const spell_data_t* flame_patch;
    const spell_data_t* conflagration;
    const spell_data_t* living_bomb;
    const spell_data_t* freezing_rain;
    const spell_data_t* splitting_ice;
    const spell_data_t* comet_storm;

    // Tier 100
    const spell_data_t* overpowered;
    const spell_data_t* time_anomaly;
    const spell_data_t* arcane_orb;
    const spell_data_t* kindling;
    const spell_data_t* pyroclasm;
    const spell_data_t* meteor;
    const spell_data_t* thermal_void;
    const spell_data_t* ray_of_frost;
    const spell_data_t* glacial_spike;
  } talents;

  // Azerite Powers
  struct azerite_powers_t
  {
    // Arcane
    azerite_power_t anomalous_impact;
    azerite_power_t arcane_pressure;
    azerite_power_t arcane_pummeling;
    azerite_power_t brain_storm;
    azerite_power_t explosive_echo;
    azerite_power_t galvanizing_spark;

    // Fire
    azerite_power_t blaster_master;
    azerite_power_t duplicative_incineration;
    azerite_power_t firemind;
    azerite_power_t flames_of_alacrity;
    azerite_power_t preheat;
    azerite_power_t trailing_embers;

    // Frost
    azerite_power_t frigid_grasp;
    azerite_power_t glacial_assault;
    azerite_power_t packed_ice;
    azerite_power_t tunnel_of_ice;
    azerite_power_t whiteout;
    azerite_power_t winters_reach;
  } azerite;

  struct uptimes_t {
    uptime_t* burn_phase;
    uptime_t* conserve_phase;
  } uptime;

public:
  mage_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF );

  ~mage_t();

  // Character Definition
  virtual std::string get_special_use_items( const std::string& item = std::string(), bool specials = false );
  virtual void        init_spells() override;
  virtual void        init_base_stats() override;
  virtual void        create_buffs() override;
  virtual void        create_options() override;
  virtual void        init_assessors() override;
  virtual void        init_gains() override;
  virtual void        init_procs() override;
  virtual void        init_benefits() override;
  virtual void        init_uptimes() override;
  virtual void        init_rng() override;
  virtual void        invalidate_cache( cache_e c ) override;
  virtual void        init_resources( bool force ) override;
  virtual void        recalculate_resource_max( resource_e rt ) override;
  virtual void        reset() override;
  virtual expr_t*     create_expression( const std::string& name ) override;
  virtual expr_t*     create_action_expression( action_t&, const std::string& name ) override;
  virtual action_t*   create_action( const std::string& name, const std::string& options ) override;
  virtual void        create_actions() override;
  virtual void        create_pets() override;
  virtual resource_e  primary_resource() const override { return RESOURCE_MANA; }
  virtual role_e      primary_role() const override { return ROLE_SPELL; }
  virtual stat_e      convert_hybrid_stat( stat_e s ) const override;
  virtual stat_e      primary_stat() const override { return STAT_INTELLECT; }
  virtual double      resource_regen_per_second( resource_e ) const override;
  virtual double      composite_player_multiplier( school_e school ) const override;
  virtual double      composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  virtual double      composite_player_pet_damage_multiplier( const action_state_t* ) const override;
  virtual double      composite_spell_crit_chance() const override;
  virtual double      composite_spell_crit_rating() const override;
  virtual double      composite_spell_haste() const override;
  virtual double      composite_mastery_rating() const override;
  virtual double      matching_gear_multiplier( attribute_e attr ) const override;
  virtual void        update_movement( timespan_t duration ) override;
  virtual void        stun() override;
  virtual double      temporary_movement_modifier() const override;
  virtual double      passive_movement_modifier() const override;
  virtual void        arise() override;
  virtual void        combat_begin() override;
  virtual void        combat_end() override;
  virtual std::string create_profile( save_e ) override;
  virtual void        copy_from( player_t* ) override;
  virtual void        merge( player_t& ) override;
  virtual void        analyze( sim_t& ) override;
  virtual void        datacollection_begin() override;
  virtual void        datacollection_end() override;
  virtual void        regen( timespan_t ) override;

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

  cooldown_waste_data_t* get_cooldown_waste_data( cooldown_t* cd )
  {
    for ( auto cdw : cooldown_waste_data_list )
    {
      if ( cdw -> cd -> name_str == cd -> name_str )
        return cdw;
    }

    auto cdw = new cooldown_waste_data_t( cd );
    cooldown_waste_data_list.push_back( cdw );
    return cdw;
  }

  proc_source_t* get_proc_source( const std::string& name, size_t count )
  {
    for ( auto ps : proc_source_list )
    {
      if ( ps -> name_str == name )
      {
        return ps;
      }
    }

    auto ps = new proc_source_t( *sim, name, count );
    proc_source_list.push_back( ps );
    return ps;
  }

  // Public mage functions:
  action_t* get_icicle();
  void      trigger_icicle( const action_state_t* trigger_state, bool chain = false, player_t* chain_target = nullptr );
  void      trigger_evocation( timespan_t duration_override = timespan_t::min(), bool hasted = true );
  void      trigger_arcane_charge( int stacks = 1 );
  bool      apply_crowd_control( const action_state_t* state, spell_mechanic type );

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
  {
    may_crit = tick_may_crit = true;
    weapon_multiplier = 0.0;
  }

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
struct water_elemental_pet_t : public mage_pet_t
{
  water_elemental_pet_t( sim_t* sim, mage_t* owner )
    : mage_pet_t( sim, owner, "water_elemental" )
  {
    owner_coeff.sp_from_sp = 0.75;
  }

  virtual void init_action_list() override
  {
    action_list_str = "waterbolt";
    mage_pet_t::init_action_list();
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override;
};

struct waterbolt_t : public mage_pet_spell_t
{
  waterbolt_t( water_elemental_pet_t* p, const std::string& options_str )
    : mage_pet_spell_t( "waterbolt", p, p -> find_pet_spell( "Waterbolt" ) )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t cast_time = mage_pet_spell_t::execute_time();

    // Waterbolt has 1 s GCD, here we model it as min cast time.
    return std::max( cast_time, timespan_t::from_seconds( 1.0 ) );
  }
};

struct freeze_t : public mage_pet_spell_t
{
  freeze_t( water_elemental_pet_t* p ) :
    mage_pet_spell_t( "freeze", p, p -> find_pet_spell( "Freeze" ) )
  {
    background = true;
    aoe = -1;
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_pet_spell_t::impact( s );
    o() -> apply_crowd_control( s, MECHANIC_ROOT );
  }
};

action_t* water_elemental_pet_t::create_action( const std::string& name,
                                                const std::string& options_str )
{
  if ( name == "waterbolt" )
    return new waterbolt_t( this, options_str );

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
    owner_coeff.sp_from_sp = 0.60;
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

    // MI Arcane Charge is hardcoded as 25% damage increase.
    arcane_charge = make_buff( this, "arcane_charge", o() -> spec.arcane_charge )
                      -> set_default_value( 0.25 );
  }
};

struct mirror_image_spell_t : public mage_pet_spell_t
{
  mirror_image_spell_t( const std::string& n, mirror_image_pet_t* p,
                        const spell_data_t* s )
    : mage_pet_spell_t( n, p, s )
  { }

  virtual void init_finished() override
  {
    if ( o() -> pets.mirror_images[ 0 ] )
    {
      stats = o() -> pets.mirror_images[ 0 ] -> get_stats( name_str );
    }

    mage_pet_spell_t::init_finished();
  }

  mirror_image_pet_t* p() const
  {
    return static_cast<mirror_image_pet_t*>( player );
  }
};

struct arcane_blast_t : public mirror_image_spell_t
{
  arcane_blast_t( mirror_image_pet_t* p, const std::string& options_str )
    : mirror_image_spell_t( "arcane_blast", p, p -> find_pet_spell( "Arcane Blast" ) )
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

    am *= 1.0 + p() -> arcane_charge -> check_stack_value();

    return am;
  }
};

struct fireball_t : public mirror_image_spell_t
{
  fireball_t( mirror_image_pet_t* p, const std::string& options_str )
    : mirror_image_spell_t( "fireball", p, p -> find_pet_spell( "Fireball" ) )
  {
    parse_options( options_str );
  }
};

struct frostbolt_t : public mirror_image_spell_t
{
  frostbolt_t( mirror_image_pet_t* p, const std::string& options_str )
    : mirror_image_spell_t( "frostbolt", p, p -> find_pet_spell( "Frostbolt" ) )
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

}  // pets

namespace buffs {

// Touch of the Magi debuff ===================================================

struct touch_of_the_magi_t : public buff_t
{
  double accumulated_damage;

  touch_of_the_magi_t( mage_td_t* td ) :
    buff_t( *td, "touch_of_the_magi", td -> source -> find_spell( 210824 ) ),
    accumulated_damage( 0.0 )
  {
    const spell_data_t* data = source -> find_spell( 210725 );

    set_chance( data -> proc_chance() );
    set_cooldown( data -> internal_cooldown() );
  }

  virtual void reset() override
  {
    buff_t::reset();
    accumulated_damage = 0.0;
  }

  virtual void expire_override( int stacks, timespan_t duration ) override
  {
    buff_t::expire_override( stacks, duration );

    auto explosion = debug_cast<mage_t*>( source ) -> action.touch_of_the_magi_explosion;
    assert( explosion );

    explosion -> set_target( player );
    explosion -> base_dd_min = explosion -> base_dd_max = accumulated_damage;
    explosion -> execute();

    accumulated_damage = 0.0;
  }

  void accumulate_damage( const action_state_t* state )
  {
    if ( sim -> debug )
    {
      sim -> out_debug.printf(
        "%s's %s accumulates %f additional damage: %f -> %f",
        player -> name(), name(), state -> result_total,
        accumulated_damage, accumulated_damage + state -> result_total
      );
    }

    accumulated_damage += state -> result_total;
  }
};
// Custom buffs ===============================================================
struct brain_freeze_buff_t : public buff_t
{
  brain_freeze_buff_t( mage_t* p ) :
    buff_t( p, "brain_freeze", p -> find_spell( 190446 ) )
  { }

  virtual void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    auto mage = debug_cast<mage_t*>( player );
    if ( mage -> sets -> has_set_bonus( MAGE_FROST, T20, B4 ) )
    {
      timespan_t cd_reduction = -100 * mage -> sets -> set( MAGE_FROST, T20, B4 ) -> effectN( 1 ).time_value();
      mage -> sample_data.t20_4pc -> add( cd_reduction );
      mage -> cooldowns.frozen_orb -> adjust( cd_reduction );
    }
  }

  virtual void refresh( int stacks, double value, timespan_t duration ) override
  {
    buff_t::refresh( stacks, value, duration );

    // The T21 4pc buff seems to be triggered on refresh as well as expire.
    // As of build 25881, 2018-01-22.
    debug_cast<mage_t*>( player ) -> buffs.arctic_blast -> trigger();
  }

  virtual void expire_override( int stacks, timespan_t duration ) override
  {
    buff_t::expire_override( stacks, duration );

    debug_cast<mage_t*>( player ) -> buffs.arctic_blast -> trigger();
  }
};

struct incanters_flow_t : public buff_t
{
  incanters_flow_t( mage_t* p ) :
    buff_t( p, "incanters_flow", p -> find_spell( 116267 ) ) // Buff is a separate spell
  {
    set_duration( p -> sim -> max_time * 3 ); // Long enough duration to trip twice_expected_event
    set_period( p -> talents.incanters_flow -> effectN( 1 ).period() ); // Period is in the talent
    set_default_value( data().effectN( 1 ).percent() );
  }

  virtual void bump( int stacks, double value ) override
  {
    if ( check() == max_stack() )
      reverse = true;
    else
      buff_t::bump( stacks, value );
  }

  virtual void decrement( int stacks, double value ) override
  {
    if ( check() == 1 )
      reverse = false;
    else
      buff_t::decrement( stacks, value );
  }
};

struct icy_veins_buff_t : public buff_t
{
  icy_veins_buff_t( mage_t* p ) :
    buff_t( p, "icy_veins", p -> find_spell( 12472 ) )
  {
    set_default_value( data().effectN( 1 ).percent() );
    set_cooldown( timespan_t::zero() );
    add_invalidate( CACHE_SPELL_HASTE );
    buff_duration += p -> talents.thermal_void -> effectN( 2 ).time_value();
  }

  virtual void expire_override( int stacks, timespan_t duration ) override
  {
    buff_t::expire_override( stacks, duration );

    auto mage = debug_cast<mage_t*>( player );
    mage -> buffs.lady_vashjs_grasp -> expire();
    if ( mage -> talents.thermal_void -> ok() && duration == timespan_t::zero() )
    {
      mage -> sample_data.icy_veins_duration -> add( elapsed( sim -> current_time() ).total_seconds() );
    }
  }
};

struct lady_vashjs_grasp_t : public buff_t
{
  proc_t* proc_fof;

  lady_vashjs_grasp_t( mage_t* p ) :
    buff_t( p, "lady_vashjs_grasp", p -> find_spell( 208147 ) )
  {
    // Disable by default.
    set_chance( 0.0 );
    tick_zero = true;
    set_tick_callback( [ this, p ] ( buff_t* /* buff */, int /* ticks */, const timespan_t& /* tick_time */ )
    {
      p -> buffs.fingers_of_frost -> trigger();
      p -> buffs.fingers_of_frost -> predict();
      proc_fof -> occur();
    } );
  }
};

} // buffs


namespace actions {
// ============================================================================
// Mage Spell
// ==========================================================================

struct mage_spell_state_t : public action_state_t
{
  // Simple bitfield for tracking sources of the Frozen effect.
  unsigned frozen;

  mage_spell_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ),
    frozen( 0u )
  { }

  virtual void initialize() override
  {
    action_state_t::initialize();
    frozen = 0u;
  }

  virtual std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s ) << " frozen=" << ( frozen != 0u );
    return s;
  }

  virtual void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );
    auto mss = debug_cast<const mage_spell_state_t*>( s );

    frozen = mss -> frozen;
  }

  virtual double composite_crit_chance() const override
  {
    double c = action_state_t::composite_crit_chance();

    if ( frozen )
    {
      auto p = debug_cast<const mage_t*>( action -> player );

      // Multiplier is not in spell data, apparently.
      c *= 1.5;
      c += p -> spec.shatter -> effectN( 2 ).percent() + p -> spec.shatter_2 -> effectN( 1 ).percent();
    }

    return c;
  }
};

struct mage_spell_t : public spell_t
{
  struct affected_by_t
  {
    // Permanent damage increase.
    bool arcane_mage;
    bool fire_mage;
    bool frost_mage;

    // Temporary damage increase.
    bool arcane_power;
    bool bone_chilling;
    bool crackling_energy;
    bool incanters_flow;
    bool rune_of_power;

    // Misc
    bool combustion;
    bool ice_floes;
    bool shatter;

    affected_by_t() :
      arcane_mage( true ),
      fire_mage( true ),
      frost_mage( true ),
      arcane_power( true ),
      bone_chilling( true ),
      crackling_energy( true ),
      incanters_flow( true ),
      rune_of_power( true ),
      combustion( true ),
      ice_floes( false ),
      shatter( false )
    { }
  } affected_by;

  static const snapshot_state_e STATE_FROZEN = STATE_TGT_USER_1;

  enum frozen_type_e
  {
    FROZEN_WINTERS_CHILL = 0,
    FROZEN_ROOT,
    FROZEN_FINGERS_OF_FROST,
    FROZEN_MAX
  };

  enum frozen_flag_e
  {
    FF_WINTERS_CHILL    = 1 << FROZEN_WINTERS_CHILL,
    FF_ROOT             = 1 << FROZEN_ROOT,
    FF_FINGERS_OF_FROST = 1 << FROZEN_FINGERS_OF_FROST
  };

  bool track_cd_waste;
  cooldown_waste_data_t* cd_waste;
public:

  mage_spell_t( const std::string& n, mage_t* p,
                const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( n, p, s ),
    affected_by(),
    track_cd_waste( false ),
    cd_waste( nullptr )
  {
    may_crit = tick_may_crit = true;
    weapon_multiplier = 0.0;
    affected_by.ice_floes = data().affected_by( p -> talents.ice_floes -> effectN( 1 ) );
    track_cd_waste = data().cooldown() > timespan_t::zero() || data().charge_cooldown() > timespan_t::zero();
  }

  mage_t* p()
  { return static_cast<mage_t*>( player ); }

  const mage_t* p() const
  { return static_cast<mage_t*>( player ); }

  mage_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual action_state_t* new_state() override
  { return new mage_spell_state_t( this, target ); }

  virtual void init() override
  {
    if ( initialized )
      return;

    spell_t::init();

    if ( affected_by.arcane_mage )
      base_multiplier *= 1.0 + p() -> spec.arcane_mage -> effectN( 1 ).percent();

    if ( affected_by.fire_mage )
      base_multiplier *= 1.0 + p() -> spec.fire_mage -> effectN( 1 ).percent();

    if ( affected_by.frost_mage )
      base_multiplier *= 1.0 + p() -> spec.frost_mage -> effectN( 1 ).percent();

    if ( harmful && affected_by.shatter && p() -> spec.shatter -> ok() )
    {
      snapshot_flags |= STATE_FROZEN;
      update_flags   |= STATE_FROZEN;
    }
  }

  virtual void init_finished() override
  {
    if ( track_cd_waste && sim -> report_details != 0 )
    {
      cd_waste = p() -> get_cooldown_waste_data( cooldown );
    }

    spell_t::init_finished();
  }

  virtual double action_multiplier() const override
  {
    double m = spell_t::action_multiplier();

    if ( affected_by.arcane_power )
      m *= 1.0 + p() -> buffs.arcane_power -> check_value();

    if ( affected_by.bone_chilling )
      m *= 1.0 + p() -> buffs.bone_chilling -> check_stack_value();

    if ( affected_by.crackling_energy )
      m *= 1.0 + p() -> buffs.crackling_energy -> check_value();

    if ( affected_by.incanters_flow )
      m *= 1.0 + p() -> buffs.incanters_flow -> check_stack_value();

    if ( affected_by.rune_of_power )
      m *= 1.0 + p() -> buffs.rune_of_power -> check_value();

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = spell_t::composite_crit_chance();

    if ( affected_by.combustion )
      c += p() -> buffs.combustion -> check_value();

    return c;
  }

  virtual unsigned frozen( const action_state_t* s ) const
  {
    const mage_td_t* td = p() -> target_data[ s -> target ];

    if ( ! td )
      return 0u;

    unsigned source = 0u;

    if ( td -> debuffs.winters_chill -> check() )
      source |= FF_WINTERS_CHILL;

    if ( td -> debuffs.frozen -> check() )
      source |= FF_ROOT;

    return source;
  }

  virtual void snapshot_internal( action_state_t* s, unsigned flags, dmg_e rt ) override
  {
    if ( flags & STATE_FROZEN )
    {
      debug_cast<mage_spell_state_t*>( s ) -> frozen = frozen( s );
    }

    spell_t::snapshot_internal( s, flags, rt );
  }

  virtual double cost() const override
  {
    double c = spell_t::cost();

    if ( p() -> buffs.arcane_power -> check() )
    {
      c *= 1.0 + p() -> buffs.arcane_power -> data().effectN( 2 ).percent()
               + p() -> talents.overpowered -> effectN( 2 ).percent();
    }

    return c;
  }

  struct buff_delay_event_t : public event_t
  {
    buff_t* buff;

    buff_delay_event_t( buff_t* b, timespan_t delay )
      : event_t( *b -> player, delay ), buff( b )
    { }

    virtual const char* name() const override
    {
      return "buff_delay";
    }

    virtual void execute() override
    {
      buff -> trigger();
    }
  };

  virtual void consume_resource() override
  {
    spell_t::consume_resource();

    if ( ! harmful || current_resource() != RESOURCE_MANA || ! p() -> spec.clearcasting -> ok() )
    {
      return;
    }

    // Mana spending required for 1% chance.
    double mana_step = p() -> spec.clearcasting -> cost( POWER_MANA ) * p() -> resources.base[ RESOURCE_MANA ];
    mana_step /= p() -> spec.clearcasting -> effectN( 1 ).percent();

    double cc_proc_chance = 0.01 * last_resource_cost / mana_step;

    if ( rng().roll( cc_proc_chance ) )
    {
      if ( p() -> buffs.clearcasting -> check() )
      {
        make_event<buff_delay_event_t>( *sim, p() -> buffs.clearcasting, timespan_t::from_seconds( 0.15 ) );
      }
      else
      {
        p() -> buffs.clearcasting -> trigger();
      }
    }
  }

  virtual void update_ready( timespan_t cd ) override
  {
    if ( cd_waste )
      cd_waste -> add( cd, time_to_execute );

    spell_t::update_ready( cd );
  }

  virtual bool usable_moving() const override
  {
    if ( p() -> buffs.ice_floes -> check() && affected_by.ice_floes )
    {
      return true;
    }

    return spell_t::usable_moving();
  }

  virtual void execute() override
  {
    spell_t::execute();

    if ( background )
      return;

    if ( affected_by.ice_floes
      && p() -> talents.ice_floes -> ok()
      && time_to_execute > timespan_t::zero()
      && p() -> buffs.ice_floes -> up() )
    {
      p() -> buffs.ice_floes -> decrement();
    }
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

      // It looks like the debuff expiration is slightly delayed in game, allowing two spells
      // impacting at the same time to trigger multiple Meteors or Comet Storms.
      // As of build 25881, 2018-01-22.
      primed_buff -> expire( p() -> bugs ? timespan_t::from_millis( 30 ) : timespan_t::zero() );
    }
  }
};

typedef residual_action::residual_periodic_action_t<mage_spell_t> residual_action_t;


// ============================================================================
// Arcane Mage Spell
// ============================================================================

struct arcane_mage_spell_t : public mage_spell_t
{

  arcane_mage_spell_t( const std::string& n, mage_t* p,
                       const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s )
  { }

  double arcane_charge_damage_bonus( bool arcane_barrage = false ) const
  {
    double per_ac_bonus = 0.0;

    if ( arcane_barrage )
    {
      per_ac_bonus = p() -> spec.arcane_charge -> effectN( 2 ).percent()
                   + p() -> cache.mastery() * p() -> spec.savant -> effectN( 3 ).mastery_value();
    }
    else
    {
      per_ac_bonus = p() -> spec.arcane_charge -> effectN( 1 ).percent()
                   + p() -> cache.mastery() * p() -> spec.savant -> effectN( 2 ).mastery_value();
    }

    return 1.0 + p() -> buffs.arcane_charge -> check() * per_ac_bonus;
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
    hot_streak = debug_cast<const ignite_spell_state_t*>( s ) -> hot_streak;
  }
};

struct fire_mage_spell_t : public mage_spell_t
{
  bool triggers_hot_streak;
  bool triggers_ignite;
  bool triggers_kindling;

  fire_mage_spell_t( const std::string& n, mage_t* p,
                     const spell_data_t* s = spell_data_t::nil() ) :
    mage_spell_t( n, p, s ),
    triggers_hot_streak( false ),
    triggers_ignite( false ),
    triggers_kindling( false )
  { }

  // Use only after schedule_execute, which sets time_to_execute.
  bool benefits_from_hot_streak( bool benefit_tracking = false ) const
  {
    // In-game, only instant cast Pyroblast and Flamestrike benefit from (and
    // consume) Hot Streak.
    return ( benefit_tracking ? p() -> buffs.hot_streak -> up() : p() -> buffs.hot_streak -> check() )
        && time_to_execute == timespan_t::zero();
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( triggers_ignite )
      {
        trigger_ignite( s );
      }

      if ( triggers_hot_streak )
      {
        handle_hot_streak( s );
      }

      if ( triggers_kindling && p() -> talents.kindling -> ok() && s -> result == RESULT_CRIT )
      {
        p() -> cooldowns.combustion -> adjust( -1000 * p() -> talents.kindling -> effectN( 1 ).time_value() );
      }
    }
  }

  void handle_hot_streak( action_state_t* s )
  {
    mage_t* p = this -> p();

    if ( ! p -> spec.hot_streak -> ok() )
      return;

    p -> procs.hot_streak_spell -> occur();

    bool guaranteed = s -> composite_crit_chance() >= 1.0;

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

          bool hu_react = p -> buffs.heating_up -> stack_react() > 0;
          p -> buffs.heating_up -> expire();
          p -> buffs.hot_streak -> trigger();
          if ( guaranteed && hu_react )
            p -> buffs.hot_streak -> predict();

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
          p -> buffs.heating_up -> trigger(
            1, buff_t::DEFAULT_VALUE(), -1.0,
            p -> buffs.heating_up -> buff_duration * p -> cache.spell_speed() );
          if ( guaranteed )
          {
            p -> buffs.heating_up -> predict();
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

  void trigger_ignite( action_state_t* state )
  {
    if ( ! p() -> spec.ignite -> ok() )
      return;

    if ( ! result_is_hit( state -> result ) )
      return;

    double m = state -> target_da_multiplier;
    if ( m <= 0.0 )
      return;

    double amount = state -> result_total / m * p() -> cache.mastery_value();
    if ( amount <= 0.0 )
      return;

    amount *= composite_ignite_multiplier( state );

    bool ignite_exists = p() -> ignite -> get_dot( state -> target ) -> is_ticking();

    residual_action::trigger( p() -> ignite, state -> target, amount );

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
    trigger_legendary_effect( p() -> buffs.contained_infernal_core,
                              p() -> buffs.erupting_infernal_core,
                              p() -> action.legendary_meteor,
                              target );
  }
};


// ============================================================================
// Frost Mage Spell
// ============================================================================

// Some Frost spells snapshot on impact (rather than execute). This is handled via
// the calculate_on_impact flag.
//
// When set to true:
//   * All snapshot flags are moved from snapshot_flags to impact_flags.
//   * calculate_result and calculate_direct_amount don't do any calculations.
//   * On spell impact:
//     - State is snapshot via frost_mage_spell_t::impact_state.
//     - Result is calculated via frost_mage_spell_t::calculate_impact_result.
//     - Amount is calculated via frost_mage_spell_t::calculate_impact_direct_amount.
//
// The previous functions are virtual and can be overridden when needed.
struct frost_mage_spell_t : public mage_spell_t
{
  bool chills;
  bool calculate_on_impact;

  proc_t* proc_fof;

  bool track_shatter;
  proc_source_t* shatter_source;

  unsigned impact_flags;

  frost_mage_spell_t( const std::string& n, mage_t* p,
                      const spell_data_t* s = spell_data_t::nil() )
    : mage_spell_t( n, p, s ),
      chills( false ),
      calculate_on_impact( false ),
      proc_fof( nullptr ),
      track_shatter( false ),
      shatter_source( nullptr ),
      impact_flags( 0u )
  {
    affected_by.shatter = true;
  }

  virtual void init() override
  {
    if ( initialized )
      return;

    mage_spell_t::init();

    if ( calculate_on_impact )
    {
      std::swap( snapshot_flags, impact_flags );
    }
  }

  void init_finished() override
  {
    if ( track_shatter && sim -> report_details != 0 )
    {
      shatter_source = p() -> get_proc_source( "Shatter/" + name_str, FROZEN_MAX );
    }

    mage_spell_t::init_finished();
  }

  void trigger_fof( double chance, int stacks = 1, proc_t* source = nullptr )
  {
    if ( ! source )
      source = proc_fof;

    bool success = p() -> buffs.fingers_of_frost -> trigger( stacks, buff_t::DEFAULT_VALUE(), chance );
    if ( success )
    {
      if ( ! source )
      {
        assert( false );
        return;
      }

      for ( int i = 0; i < stacks; i++ )
        source -> occur();

      if ( chance >= 1.0 )
        p() -> buffs.fingers_of_frost -> predict();
    }
  }

  void trigger_brain_freeze( double chance )
  {
    if ( rng().roll( chance ) )
    {
      if ( p() -> buffs.brain_freeze -> check() )
      {
        // Brain Freeze was already active, delay the new application
        make_event<buff_delay_event_t>( *sim, p() -> buffs.brain_freeze, timespan_t::from_seconds( 0.15 ) );
      }
      else
      {
        p() -> buffs.brain_freeze -> trigger();
      }
    }
  }

  double icicle_sp_coefficient() const
  {
    return p() -> cache.mastery() * p() -> spec.icicles -> effectN( 3 ).sp_coeff();
  }

  void trigger_icicle_gain( action_state_t* state, action_t* icicle_action )
  {
    if ( ! p() -> spec.icicles -> ok() )
      return;

    p() -> buffs.icicles -> trigger();

    // Shoot one if capped
    if ( as<int>( p() -> icicles.size() ) == p() -> spec.icicles -> effectN( 2 ).base_value() )
    {
      p() -> trigger_icicle( state );
    }

    icicle_tuple_t tuple{ p() -> sim -> current_time(), icicle_action };
    p() -> icicles.push_back( tuple );
  }

  virtual void impact_state( action_state_t* s, dmg_e rt )
  { snapshot_internal( s, impact_flags, rt ); }

  virtual double calculate_direct_amount( action_state_t* s ) const override
  {
    if ( ! calculate_on_impact )
    {
      return mage_spell_t::calculate_direct_amount( s );
    }
    else
    {
      // Don't do any extra work, this result won't be used.
      return 0.0;
    }
  }

  virtual double calculate_impact_direct_amount( action_state_t* s ) const
  { return mage_spell_t::calculate_direct_amount( s ); }

  virtual result_e calculate_result( action_state_t* s ) const override
  {
    if ( ! calculate_on_impact )
    {
      return mage_spell_t::calculate_result( s );
    }
    else
    {
      // Don't do any extra work, this result won't be used.
      return RESULT_NONE;
    }
  }

  virtual result_e calculate_impact_result( action_state_t* s ) const
  { return mage_spell_t::calculate_result( s ); }

  void record_shatter_source( const action_state_t* s, proc_source_t* source = nullptr )
  {
    unsigned frozen = debug_cast<const mage_spell_state_t*>( s ) -> frozen;

    if ( ! frozen )
      return;

    if ( ! source )
      source = shatter_source;

    assert( source );

    if ( frozen & FF_WINTERS_CHILL )
      source -> occur( FROZEN_WINTERS_CHILL );
    else if ( frozen & ~FF_FINGERS_OF_FROST )
      source -> occur( FROZEN_ROOT );
    else
      source -> occur( FROZEN_FINGERS_OF_FROST );
  }

  virtual void impact( action_state_t* s ) override
  {
    if ( calculate_on_impact )
    {
      // Re-call functions here, before the impact call to do the damage calculations as we impact.
      impact_state( s, amount_type( s ) );

      s -> result = calculate_impact_result( s );
      s -> result_amount = calculate_impact_direct_amount( s );
    }

    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) && shatter_source )
    {
      record_shatter_source( s );
    }

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

struct icicle_t : public frost_mage_spell_t
{
  icicle_t( mage_t* p, const std::string& trigger_spell ) :
    frost_mage_spell_t( trigger_spell + "_icicle", p, p -> find_spell( 148022 ) )
  {
    proc = background = true;

    base_dd_min = base_dd_max = 1;

    if ( p -> talents.splitting_ice -> ok() )
    {
      aoe = as<int>( 1 + p -> talents.splitting_ice -> effectN( 1 ).base_value() );
      base_multiplier *= 1.0 + p -> talents.splitting_ice -> effectN( 3 ).percent();
      base_aoe_multiplier *= p -> talents.splitting_ice -> effectN( 2 ).percent();
    }
  }

  virtual double spell_direct_power_coefficient( const action_state_t* ) const override
  { return spell_power_mod.direct + icicle_sp_coefficient(); }
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
      p() -> trigger_arcane_charge( 4 );
      p() -> buffs.crackling_energy -> trigger();
    }
  }
};

// Ignite Spell ===================================================================

struct ignite_t : public residual_action_t
{
  ignite_t( mage_t* p ) :
    residual_action_t( "ignite", p, p -> find_spell( 12846 ) )
  {
    dot_duration = p -> find_spell( 12654 ) -> duration();
    base_tick_time = p -> find_spell( 12654 ) -> effectN( 1 ).period();
    school = SCHOOL_FIRE;

    //!! NOTE NOTE NOTE !! This is super dangerous and means we have to be extra careful with correctly
    // flagging thats that proc off events, to not proc off ignite if they shouldn't!
    callbacks = true;
  }

  virtual void init() override
  {
    residual_action_t::init();

    snapshot_flags |= STATE_TGT_MUL_TA;
    update_flags |= STATE_TGT_MUL_TA;
  }

  virtual void tick( dot_t* dot ) override
  {
    residual_action_t::tick( dot );

    if ( p() -> talents.conflagration -> ok()
      && rng().roll( p() -> talents.conflagration -> effectN( 1 ).percent() ) )
    {
      p() -> action.conflagration_flare_up -> set_target( dot -> target );
      p() -> action.conflagration_flare_up -> execute();
    }
  }
};

// Arcane Barrage Spell =======================================================

struct arcane_barrage_t : public arcane_mage_spell_t
{
  double mystic_kilt_of_the_rune_master_regen;
  double mantle_of_the_first_kirin_tor_chance;

  arcane_barrage_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_barrage", p, p -> find_specialization_spell( "Arcane Barrage" ) ),
    mystic_kilt_of_the_rune_master_regen( 0.0 ),
    mantle_of_the_first_kirin_tor_chance( 0.0 )
  {
    parse_options( options_str );
    cooldown -> hasted = true;
    base_aoe_multiplier *= data().effectN( 2 ).percent();
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

    if ( p() -> spec.arcane_barrage_2 -> ok() )
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


    if ( p() -> sets -> has_set_bonus( MAGE_ARCANE, T21, B2 ) )
    {
      p() -> buffs.expanding_mind
          -> trigger( 1, charges * p() -> sets -> set ( MAGE_ARCANE, T21, B2 ) -> effectN( 1 ).percent() );
    }

    p() -> buffs.arcane_charge -> expire();
  }

  virtual void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( p() -> talents.chrono_shift -> ok() )
    {
      p() -> buffs.chrono_shift -> trigger();
    }
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    double da = arcane_mage_spell_t::bonus_da( s );

    if ( p() -> azerite.arcane_pressure.enabled()
      && s -> target -> health_percentage() < p() -> azerite.arcane_pressure.spell_ref().effectN( 2 ).base_value() )
    {
      da += p() -> azerite.arcane_pressure.value() * p() -> buffs.arcane_charge -> check();
    }

    return da;
  }

  virtual double action_multiplier() const override
  {
    double am = arcane_mage_spell_t::action_multiplier();

    am *= arcane_charge_damage_bonus( true );

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
  arcane_blast_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_blast", p, p -> find_specialization_spell( "Arcane Blast" ) )
  {
    parse_options( options_str );
    base_dd_adder += p -> azerite.galvanizing_spark.value( 2 );
  }

  virtual double cost() const override
  {
    double c = arcane_mage_spell_t::cost();

    c *= 1.0 + p() -> buffs.arcane_charge -> check() *
               p() -> spec.arcane_charge -> effectN( 5 ).percent();


    c *= 1.0 + p() -> buffs.rule_of_threes -> check_value();

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

    p() -> buffs.rhonins_assaulting_armwraps -> decrement();

    if ( last_resource_cost == 0 )
    {
      p() -> buffs.rule_of_threes -> decrement();
    }

    p() -> buffs.arcane_charge -> up();

    if ( hit_any_target )
    {
      p() -> trigger_arcane_charge();

      //TODO: Benefit tracking
      if ( p() -> azerite.galvanizing_spark.enabled() &&
           rng().roll( p() -> azerite.galvanizing_spark.spell_ref().effectN( 1 ).percent() ) )
      {
        p() -> trigger_arcane_charge();
      }
    }

    if ( p() -> buffs.presence_of_mind -> up() )
    {
      p() -> buffs.presence_of_mind -> decrement();
    }

    p() -> buffs.t19_oh_buff -> trigger();
    p() -> buffs.quick_thinker -> trigger();

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

    t *=  1.0 + p() -> buffs.arcane_charge -> check() *
                p() -> spec.arcane_charge -> effectN( 4 ).percent();

    return t;
  }

  virtual void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) && p() -> talents.touch_of_the_magi -> ok() )
    {
      td( s -> target ) -> debuffs.touch_of_the_magi -> trigger();
    }
  }

};

// Arcane Explosion Spell =====================================================

struct arcane_explosion_t : public arcane_mage_spell_t
{
  arcane_explosion_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_explosion", p,
                         p -> find_specialization_spell( "Arcane Explosion" ) )
  {
    parse_options( options_str );
    aoe = -1;

    if ( p -> azerite.explosive_echo.enabled() )
    {
      base_dd_adder += p -> azerite.explosive_echo.value( 2 );
    }
  }

  virtual double cost() const override
  {
    double c = arcane_mage_spell_t::cost();

    c *= 1.0 + p() -> buffs.clearcasting -> check_value();

    return c;
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();

    if ( last_resource_cost == 0 )
    {
      p() -> buffs.clearcasting -> decrement();
    }

    if ( hit_any_target )
    {
      p() -> trigger_arcane_charge();
    }

    if ( p() -> talents.reverberate -> ok()
      && num_targets_hit >= as<int>( p() -> talents.reverberate -> effectN( 2 ).base_value() )
      && rng().roll( p() -> talents.reverberate -> effectN( 1 ).percent() ) )
    {
      p() -> trigger_arcane_charge();
    }
    p() -> buffs.quick_thinker -> trigger();
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    double da = arcane_mage_spell_t::bonus_da( s );

    if ( p() -> azerite.explosive_echo.enabled()
      && target_list().size() >= as<size_t>( p() -> azerite.explosive_echo.spell_ref().effectN( 1 ).base_value() )
      && rng().roll ( p() -> azerite.explosive_echo.spell_ref().effectN( 3 ).percent() ) )
    {
      da += p() -> azerite.explosive_echo.value( 4 );
    }

    return da;
  }

};

// Arcane Intellect Spell ===================================================

struct arcane_intellect_t : public mage_spell_t
{
  arcane_intellect_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_intellect", p, p -> find_class_spell( "Arcane Intellect" ) )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;

    background = sim -> overrides.arcane_intellect != 0;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( ! sim -> overrides.arcane_intellect )
      sim -> auras.arcane_intellect -> trigger();
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

    base_multiplier *= 1.0 + p -> sets -> set( MAGE_ARCANE, T19, B2 ) -> effectN( 1 ).percent();
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();

    p() -> buffs.arcane_pummeling -> trigger();
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    double da = arcane_mage_spell_t::bonus_da( s );

    da += p() -> azerite.anomalous_impact.value() * p() -> buffs.arcane_charge -> check();
    da += p() -> buffs.arcane_pummeling -> check_stack_value();

    return da;
  }
};

struct am_state_t : public mage_spell_state_t
{
  double tick_time_multiplier;

  am_state_t( action_t* action, player_t* target ) :
    mage_spell_state_t( action, target ),
    tick_time_multiplier( 1.0 )
  { }

  virtual void initialize() override
  {
    mage_spell_state_t::initialize();
    tick_time_multiplier = 1.0;
  }

  virtual std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    mage_spell_state_t::debug_str( s )
      << " tick_time_multiplier=" << tick_time_multiplier;
    return s;
  }

  virtual void copy_state( const action_state_t* other ) override
  {
    mage_spell_state_t::copy_state( other );

    tick_time_multiplier = debug_cast<const am_state_t*>( other ) -> tick_time_multiplier;
  }
};

struct arcane_missiles_t : public arcane_mage_spell_t
{
  double cc_duration_reduction;
  double cc_tick_time_reduction;

  arcane_missiles_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "arcane_missiles", p, p -> find_specialization_spell( "Arcane Missiles" ) )
  {
    parse_options( options_str );
    may_miss = false;
    dot_duration = data().duration();
    base_tick_time = data().effectN( 2 ).period();
    tick_zero = channeled = true;
    tick_action = new arcane_missiles_tick_t( p );

    auto cc_data = p -> buffs.clearcasting_channel -> data();
    cc_duration_reduction  = cc_data.effectN( 1 ).percent();
    cc_tick_time_reduction = cc_data.effectN( 2 ).percent() + p -> talents.amplification -> effectN( 1 ).percent();
  }

  void handle_clearcasting( bool is_active )
  {
    if ( is_active )
    {
      p() -> buffs.clearcasting_channel -> trigger();
    }
    else
    {
      p() -> buffs.clearcasting_channel -> expire();
    }
  }

  // Flag Arcane Missiles as direct damage for triggering effects
  virtual dmg_e amount_type( const action_state_t* /* state */, bool /* periodic */ ) const override
  {
    return DMG_DIRECT;
  }

  virtual action_state_t* new_state() override
  { return new am_state_t( this, target ); }

  // We need to snapshot any tick time reduction effect here so that it correctly affects the whole
  // channel.
  virtual void snapshot_state( action_state_t* state, dmg_e rt ) override
  {
    arcane_mage_spell_t::snapshot_state( state, rt );

    if ( p() -> buffs.clearcasting_channel -> check() )
    {
      debug_cast<am_state_t*>( state ) -> tick_time_multiplier = 1.0 + cc_tick_time_reduction;
    }
  }

  virtual timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    // AM channel duration is a bit fuzzy, it will go above or below the standard 2 s
    // to make sure it has the correct number of ticks.

    timespan_t full_duration = dot_duration * s -> haste;

    if ( p() -> buffs.clearcasting_channel -> check() )
    {
      full_duration *= 1.0 + cc_duration_reduction;
    }

    timespan_t tick_duration = tick_time( s );

    double ticks = std::round( full_duration / tick_duration );

    return ticks * tick_duration;
  }

  virtual timespan_t tick_time( const action_state_t* s ) const override
  {
    timespan_t t = arcane_mage_spell_t::tick_time( s );

    t *= debug_cast<const am_state_t*>( s ) -> tick_time_multiplier;

    return t;
  }

  virtual double last_tick_factor( const dot_t*, const timespan_t&, const timespan_t& ) const override
  {
    // AM always does full damage, even on "partial" ticks.
    return 1.0;
  }

  timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
  {
    // 30% refresh rule apparently works with AM as well!
    return triggered_duration + std::min( triggered_duration * 0.3, dot -> remains() );
  }

  virtual double cost() const override
  {
    double c = arcane_mage_spell_t::cost();

    c *= 1.0 + p() -> buffs.rule_of_threes -> check_value();
    c *= 1.0 + p() -> buffs.clearcasting -> check_value();

    return c;
  }

  virtual void execute() override
  {
    p() -> buffs.arcane_pummeling -> expire();

    // In game, the channel refresh happens before the hidden Clearcasting buff is updated.
    bool cc_active = p() -> buffs.clearcasting -> check() != 0;
    bool cc_delay = p() -> bugs && get_dot( target ) -> is_ticking();

    if ( ! cc_delay )
    {
      handle_clearcasting( cc_active );
    }

    arcane_mage_spell_t::execute();

    p() -> buffs.rhonins_assaulting_armwraps -> trigger();

    if ( last_resource_cost == 0 )
    {
      if ( p() -> buffs.clearcasting -> check() )
      {
        p() -> buffs.clearcasting -> decrement();
      }
      else
      {
        p() -> buffs.rule_of_threes -> decrement();
      }
    }

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

    p() -> buffs.quick_thinker -> trigger();

    if ( cc_delay )
    {
      handle_clearcasting( cc_active );
    }
  }

  virtual bool usable_moving() const override
  {
    if ( p() -> talents.slipstream -> ok() && p() -> buffs.clearcasting -> check() )
      return true;

    return arcane_mage_spell_t::usable_moving();
  }

  virtual void last_tick( dot_t* d ) override
  {
    arcane_mage_spell_t::last_tick( d );

    p() -> buffs.clearcasting_channel -> expire();
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

  virtual void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> trigger_arcane_charge();
      p() -> buffs.quick_thinker -> trigger();
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
    may_miss = may_crit = false;

    if ( legendary )
    {
      background = true;
      base_costs[ RESOURCE_MANA ] = 0;
      cooldown -> duration = timespan_t::zero();
    }

    add_child( orb_bolt );
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();
    p() -> trigger_arcane_charge();
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds(
      std::max( 0.1, ( player -> get_player_distance( *target ) - 10.0 ) / 16.0 ) );
  }

  virtual void impact( action_state_t* s ) override
  {
    arcane_mage_spell_t::impact( s );

    orb_bolt -> set_target( s -> target );
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
    base_teleport_distance = data().effectN( 1 ).radius_max();

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
    chills = true;
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    if ( hit_any_target )
    {
      timespan_t base_cd_reduction = -10.0 * p() -> spec.blizzard_2 -> effectN( 1 ).time_value();
      timespan_t total_cd_reduction = num_targets_hit * base_cd_reduction;
      p() -> sample_data.blizzard -> add( total_cd_reduction );
      p() -> cooldowns.frozen_orb -> adjust( total_cd_reduction );
    }
  }

  virtual double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.freezing_rain -> check_value();

    return am;
  }

  virtual double composite_persistent_multiplier( const action_state_t* s ) const override
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
    may_miss = may_crit = affected_by.shatter = false;
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buffs.freezing_rain -> check() )
    {
      return timespan_t::zero();
    }

    return frost_mage_spell_t::execute_time();
  }

  virtual double false_positive_pct() const override
  {
    // Players are probably less likely to accidentally use blizzard than other spells.
    return ( frost_mage_spell_t::false_positive_pct() / 2 );
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    if ( p() -> buffs.zannesu_journey -> default_chance != 0.0 )
    {
      p() -> benefits.zannesu_journey -> update();
    }

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

    p() -> trigger_arcane_charge( 4 );

    for ( int i = 0; i < 4; i++ )
      if ( p() -> buffs.quick_thinker -> trigger() )
        break;
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

    if ( p() -> sets -> has_set_bonus( MAGE_FIRE, T21, B4) )
    {
      p() -> buffs.inferno -> trigger();
    }
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
    may_miss = may_crit = affected_by.shatter = false;
    add_child( projectile );

    if ( legendary )
    {
      background = true;
      base_costs[ RESOURCE_MANA ] = 0;
      cooldown -> duration = timespan_t::zero();
    }
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.0 );
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    timespan_t ground_aoe_duration = timespan_t::from_seconds( 1.4 );
    p() -> ground_aoe_expiration[ name_str ]
      = sim -> current_time() + ground_aoe_duration;

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .pulse_time( timespan_t::from_seconds( 0.2 ) )
      .target( s -> target )
      .duration( ground_aoe_duration )
      .action( projectile ) );
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


// Conflagration Spell =====================================================

struct conflagration_t : public fire_mage_spell_t
{
  conflagration_t( mage_t* p ) :
    fire_mage_spell_t( "conflagration", p, p -> find_spell( 226757 ) )
  {
    background = true;
  }
};

struct conflagration_flare_up_t : public fire_mage_spell_t
{
  conflagration_flare_up_t( mage_t* p ) :
    fire_mage_spell_t( "conflagration_flare_up", p, p -> find_spell( 205345 ) )
  {
    callbacks = false;
    background = true;
    aoe = -1;
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

// Evocation Spell ==========================================================

struct evocation_t : public arcane_mage_spell_t
{
  evocation_t( mage_t* p, const std::string& options_str ) :
    arcane_mage_spell_t( "evocation", p,
                         p -> find_specialization_spell( "Evocation" ) )
  {
    parse_options( options_str );

    base_tick_time = timespan_t::from_seconds( 1.0 );
    dot_duration = data().duration();
    channeled = ignore_false_positive = tick_zero = true;
    harmful = false;

    cooldown -> duration *= 1.0 + p -> spec.evocation_2 -> effectN( 1 ).percent();
  }

  virtual void execute() override
  {
    arcane_mage_spell_t::execute();

    p() -> trigger_evocation();
  }

  virtual void tick( dot_t* d ) override
  {
    arcane_mage_spell_t::tick( d );

    if ( p() -> azerite.brain_storm.enabled() )
    {
      p() -> buffs.brain_storm -> trigger();
    }
  }

  virtual void last_tick( dot_t* d ) override
  {
    arcane_mage_spell_t::last_tick( d );

    p() -> buffs.evocation -> expire();
  }

  virtual bool usable_moving() const override
  {
    if ( p() -> talents.slipstream -> ok() )
      return true;

    return arcane_mage_spell_t::usable_moving();
  }
};

// Ebonbolt Spell ===========================================================

struct ebonbolt_t : public frost_mage_spell_t
{
  ebonbolt_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "ebonbolt", p, p -> talents.ebonbolt )
  {
    parse_options( options_str );
    parse_effect_data( p -> find_spell( 257538 ) -> effectN( 1 ) );

    if ( p -> talents.splitting_ice -> ok() )
    {
      aoe = as<int>( 1 + p -> talents.splitting_ice -> effectN( 1 ).base_value() );
      base_aoe_multiplier *= p -> talents.splitting_ice -> effectN( 2 ).percent();
    }

    calculate_on_impact = true;
    track_shatter = true;
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();
    trigger_brain_freeze( 1.0 );
  }
};

// Fireball Spell ===========================================================

struct fireball_t : public fire_mage_spell_t
{
  conflagration_t* conflagration;

  fireball_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "fireball", p, p -> find_class_spell( "Fireball" ) ),
    conflagration( nullptr )
  {
    parse_options( options_str );
    triggers_hot_streak = true;
    triggers_ignite = true;
    triggers_kindling = true;

    if ( p -> talents.conflagration -> ok() )
    {
      conflagration = new conflagration_t( p );
      add_child( conflagration );
    }

    base_dd_adder += p -> azerite.duplicative_incineration.value( 2 );
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

    p() -> buffs.t19_oh_buff -> trigger();

    if ( rng().roll( p() -> azerite.duplicative_incineration.spell_ref().effectN( 1 ).percent() ) )
    {
      execute();
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );
    if ( result_is_hit( s -> result ) )
    {
      if ( s -> result == RESULT_CRIT )
      {
        p() -> buffs.enhanced_pyrotechnics -> expire();
        p() -> buffs.flames_of_alacrity -> expire();
      }
      else
      {
        p() -> buffs.enhanced_pyrotechnics -> trigger();
        p() -> buffs.flames_of_alacrity -> trigger();
      }

      if ( conflagration )
      {
        conflagration -> set_target( s -> target );
        conflagration -> execute();
      }

      trigger_infernal_core( s -> target );
    }
  }

  virtual double composite_target_crit_chance( player_t* target ) const override
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
  virtual dmg_e amount_type( const action_state_t* /* state */,
                             bool /* periodic */ ) const override
  {
    return DMG_OVER_TIME;
  }
};

// Flamestrike Spell ==========================================================

struct flamestrike_t : public fire_mage_spell_t
{
  flame_patch_t* flame_patch;
  timespan_t flame_patch_duration;

  flamestrike_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "flamestrike", p,
                       p -> find_specialization_spell( "Flamestrike" ) ),
    flame_patch( new flame_patch_t( p ) ),
    flame_patch_duration( p -> find_spell( 205470 ) -> duration() )
  {
    parse_options( options_str );

    triggers_ignite = true;
    aoe = -1;
    add_child( flame_patch );
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
    bool hot_streak = benefits_from_hot_streak( true );

    fire_mage_spell_t::execute();

    // Ignition/Critical Massive buffs are removed shortly after Flamestrike/Pyroblast cast.
    // In a situation where you're hardcasting FS/PB followed by a Hot Streak FS/FB, both
    // spells actually benefit. As of build 25881, 2018-01-22.
    p() -> buffs.ignition -> expire( p() -> bugs ? timespan_t::from_millis( 15 ) : timespan_t::zero() );
    p() -> buffs.critical_massive -> expire( p() -> bugs ? timespan_t::from_millis( 15 ) : timespan_t::zero() );

    if ( hot_streak )
    {
      p() -> buffs.hot_streak -> expire();

      p() -> buffs.kaelthas_ultimate_ability -> trigger();
      p() -> buffs.pyroclasm -> trigger();
      p() -> buffs.firemind -> trigger();

      if ( p() -> talents.pyromaniac -> ok()
        && rng().roll( p() -> talents.pyromaniac -> effectN( 1 ).percent() ) )
      {
        p() -> procs.hot_streak -> occur();
        p() -> procs.hot_streak_pyromaniac -> occur();
        p() -> buffs.hot_streak -> trigger();
      }
    }
  }

  virtual void impact( action_state_t* state ) override
  {
    fire_mage_spell_t::impact( state );

    if ( p() -> sets -> has_set_bonus( MAGE_FIRE, T20, B4 ) && state -> result == RESULT_CRIT )
    {
      p() -> buffs.critical_massive -> trigger();
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
    debug_cast<ignite_spell_state_t*>( s ) -> hot_streak = benefits_from_hot_streak();
  }

  virtual double composite_ignite_multiplier( const action_state_t* s ) const override
  {
    return debug_cast<const ignite_spell_state_t*>( s ) -> hot_streak ? 2.0 : 1.0;
  }

  virtual double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.critical_massive -> value();

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

struct glacial_assault_t : public frost_mage_spell_t
{
  glacial_assault_t( mage_t* p ) :
    frost_mage_spell_t( "glacial_assault", p, p -> find_spell( 279856 ) )
  {
    // TODO: Is this affected by shatter?
    background = true;
    aoe = -1;

    base_dd_min = base_dd_max = p -> azerite.glacial_assault.value();
  }
};

struct flurry_bolt_t : public frost_mage_spell_t
{
  double glacial_assault_chance;

  flurry_bolt_t( mage_t* p ) :
    frost_mage_spell_t( "flurry_bolt", p, p -> find_spell( 228354 ) ),
    glacial_assault_chance( 0.0 )
  {
    background = true;
    chills = true;
    base_multiplier *= 1.0 + p -> talents.lonely_winter -> effectN( 1 ).percent();

    if ( p -> azerite.glacial_assault.enabled() )
    {
      glacial_assault_chance = p -> azerite.glacial_assault.spell_ref().effectN( 1 ).trigger() -> proc_chance();
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    p() -> state.flurry_bolt_count++;

    if ( p() -> state.brain_freeze_active )
    {
      td( s -> target ) -> debuffs.winters_chill -> trigger();
    }

    if ( rng().roll( glacial_assault_chance ) )
    {
      // TODO: Double check the delay.
      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .pulse_time( timespan_t::from_seconds( 1.0 ) )
        .target( s -> target )
        .n_pulses( 1 )
        .action( p() -> action.glacial_assault ) );
    }
  }

  virtual double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    if ( p() -> state.brain_freeze_active )
    {
      am *= 1.0 + p() -> buffs.brain_freeze -> data().effectN( 2 ).percent();
    }

    // In-game testing shows that 6 successive Flurry bolt impacts (with no cast
    // in between to reset the counter) results in the following bonus from T20 2pc:
    //
    //   1st   2nd   3rd   4th   5th   6th
    //   +0%  +25%  +50%  +25%  +25%  +25%
    int adjusted_bolt_count = p() -> state.flurry_bolt_count;
    if ( adjusted_bolt_count > 2 )
      adjusted_bolt_count = 1;

    am *= 1.0 + adjusted_bolt_count
              * p() -> sets -> set( MAGE_FROST, T21, B2 ) -> effectN( 1 ).percent();

    return am;
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    double da = frost_mage_spell_t::bonus_da( s );

    if ( p() -> state.winters_reach_active )
    {
      // Buff most likely won't be active here, need to grab its default_value
      // instead of using value/stack_value.
      da += p() -> buffs.winters_reach -> default_value;
    }

    return da;
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
    may_miss = may_crit = affected_by.shatter = false;
    add_child( flurry_bolt );
    if ( p -> spec.icicles -> ok() )
    {
      add_child( p -> icicle.flurry );
    }
    if ( p -> action.glacial_assault )
    {
      add_child( p -> action.glacial_assault );
    }
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

    trigger_icicle_gain( execute_state, p() -> icicle.flurry );

    bool brain_freeze = p() -> buffs.brain_freeze -> up();
    p() -> state.brain_freeze_active = brain_freeze;
    p() -> buffs.brain_freeze -> decrement();
    p() -> state.flurry_bolt_count = 0;
    p() -> buffs.zannesu_journey -> trigger();

    p() -> state.winters_reach_active = ! brain_freeze && p() -> buffs.winters_reach -> up();

    if ( brain_freeze )
    {
      p() -> buffs.winters_reach -> trigger();
      p() -> procs.brain_freeze_flurry -> occur();
    }
    else
    {
      p() -> buffs.winters_reach -> decrement();
    }
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
      .n_pulses( as<int>( data().effectN( 1 ).base_value() ) )
      .action( flurry_bolt ), true );
  }
};

// Frostbolt Spell ==========================================================

struct frostbolt_t : public frost_mage_spell_t
{
  frostbolt_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "frostbolt", p, p -> find_specialization_spell( "Frostbolt" ) )
  {
    parse_options( options_str );
    parse_effect_data( p -> find_spell( 228597 ) -> effectN( 1 ) );
    if ( p -> spec.icicles -> ok() )
    {
      add_child( p -> icicle.frostbolt );
    }

    base_multiplier *= 1.0 + p -> talents.lonely_winter -> effectN( 1 ).percent();
    chills = true;
    calculate_on_impact = true;
    track_shatter = true;
  }

  void init_finished() override
  {
    proc_fof = p() -> get_proc( std::string( "Fingers of Frost from " ) + data().name_cstr() );
    frost_mage_spell_t::init_finished();
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    trigger_icicle_gain( execute_state, p() -> icicle.frostbolt );

    double fof_proc_chance = p() -> spec.fingers_of_frost -> effectN( 1 ).percent();
    fof_proc_chance *= 1.0 + p() -> talents.frozen_touch -> effectN( 1 ).percent();
    trigger_fof( fof_proc_chance );

    double bf_proc_chance = p() -> spec.brain_freeze -> effectN( 1 ).percent();
    bf_proc_chance += p() -> sets -> set( MAGE_FROST, T19, B2 ) -> effectN( 1 ).percent();
    bf_proc_chance *= 1.0 + p() -> talents.frozen_touch -> effectN( 1 ).percent();
    // TODO: Double check if it interacts this way
    trigger_brain_freeze( bf_proc_chance );

    p() -> buffs.t19_oh_buff -> trigger();

    if ( execute_state -> target != p() -> last_frostbolt_target )
    {
      p() -> buffs.tunnel_of_ice -> expire();
    }
    p() -> last_frostbolt_target = execute_state -> target;
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    p() -> buffs.tunnel_of_ice -> trigger();
    trigger_shattered_fragments( s -> target );
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    double da = frost_mage_spell_t::bonus_da( s );

    da += p() -> buffs.tunnel_of_ice -> check_stack_value();

    return da;
  }
};

// Frost Nova Spell ========================================================

struct frost_nova_t : public mage_spell_t
{
  frost_nova_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frost_nova", p, p -> find_class_spell( "Frost Nova" ) )
  {
    parse_options( options_str );
    aoe = -1;

    affected_by.shatter = true;

    cooldown -> charges += as<int>( p -> talents.ice_ward -> effectN( 1 ).base_value() );
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
    chills = true;
  }

  void init_finished() override
  {
    proc_fof = p() -> get_proc( "Fingers of Frost from Frozen Orb Tick" );
    frost_mage_spell_t::init_finished();
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();
    if ( hit_any_target )
    {
      double fof_proc_chance = p() -> spec.fingers_of_frost -> effectN( 2 ).percent();
      fof_proc_chance += p() -> sets -> set( MAGE_FROST, T19, B4 ) -> effectN( 1 ).percent();
      trigger_fof( fof_proc_chance );
    }
  }

  virtual double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> cache.mastery() * p() -> spec.icicles -> effectN( 4 ).mastery_value();

    return am;
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( p() -> azerite.packed_ice.enabled() )
    {
      td( s -> target ) -> debuffs.packed_ice -> trigger();
    }
  }
};

struct frozen_orb_t : public frost_mage_spell_t
{
  bool ice_time;

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
    may_miss = may_crit = affected_by.shatter = false;
  }

  void init_finished() override
  {
    proc_fof = p() -> get_proc( "Fingers of Frost from Frozen Orb Initial Impact" );
    frost_mage_spell_t::init_finished();
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t t = frost_mage_spell_t::travel_time();

    // Frozen Orb activates after about 0.5 s, even in melee range.
    t = std::max( t, timespan_t::from_seconds( 0.5 ) );

    return t;
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    if ( p() -> sets -> has_set_bonus( MAGE_FROST, T20, B2 ) )
    {
      p() -> buffs.frozen_mass -> trigger();
    }
    if ( p() -> talents.freezing_rain -> ok() )
    {
      p() -> buffs.freezing_rain -> trigger();
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );
    player_t* t = s -> target;
    double x = t -> x_position;
    double y = t -> y_position;

    timespan_t ground_aoe_duration = timespan_t::from_seconds( 9.5 );
    p() -> ground_aoe_expiration[ name_str ]
      = sim -> current_time() + ground_aoe_duration;

    if ( result_is_hit( s -> result ) )
    {
      trigger_fof( 1.0 );
      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .pulse_time( timespan_t::from_seconds( 0.5 ) )
        .target( t )
        .duration( ground_aoe_duration )
        .action( frozen_orb_bolt )
        .expiration_callback( [ this, t, x, y ] () {
          if ( ice_time )
          {
            ice_time_nova -> set_target( target );
            action_state_t* state = ice_time_nova -> get_state();
            ice_time_nova -> snapshot_state( state, ice_time_nova -> amount_type( state ) );
            // Make sure Ice Time works correctly with distance targetting, e.g.
            // when the target moves out of Frozen Orb.
            state -> target     = t;
            state -> original_x = x;
            state -> original_y = y;

            ice_time_nova -> schedule_execute( state );
          }
        } ), true );
    }
  }
};

// Glacial Spike Spell ==============================================================

struct glacial_spike_t : public frost_mage_spell_t
{
  glacial_spike_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "glacial_spike", p, p -> talents.glacial_spike )
  {
    parse_options( options_str );
    parse_effect_data( p -> find_spell( 228600 ) -> effectN( 1 ) );
    if ( p -> talents.splitting_ice -> ok() )
    {
      aoe = as<int>( 1 + p -> talents.splitting_ice -> effectN( 1 ).base_value() );
      base_aoe_multiplier *= p -> talents.splitting_ice -> effectN( 2 ).percent();
    }
    calculate_on_impact = true;
    track_shatter = true;
  }

  virtual bool ready() override
  {
    if ( p() -> buffs.icicles -> check() < p() -> buffs.icicles -> max_stack() )
    {
      return false;
    }

    return frost_mage_spell_t::ready();
  }

  virtual double spell_direct_power_coefficient( const action_state_t* ) const override
  {
    double extra_sp_coef = icicle_sp_coefficient();

    extra_sp_coef *=       p() -> spec.icicles -> effectN( 2 ).base_value();
    extra_sp_coef *= 1.0 + p() -> talents.splitting_ice -> effectN( 3 ).percent();

    return spell_power_mod.direct + extra_sp_coef;
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    p() -> icicles.clear();
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

    p() -> buffs.ice_floes -> trigger();
  }
};


// Ice Lance Spell ==========================================================

struct ice_lance_state_t : public mage_spell_state_t
{
  bool fingers_of_frost;

  ice_lance_state_t( action_t* action, player_t* target ) :
    mage_spell_state_t( action, target ),
    fingers_of_frost( false )
  { }

  virtual void initialize() override
  {
    mage_spell_state_t::initialize();
    fingers_of_frost = false;
  }

  virtual std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    mage_spell_state_t::debug_str( s ) << " fingers_of_frost=" << fingers_of_frost;
    return s;
  }

  virtual void copy_state( const action_state_t* s ) override
  {
    mage_spell_state_t::copy_state( s );
    auto ils = debug_cast<const ice_lance_state_t*>( s );

    fingers_of_frost = ils -> fingers_of_frost;
  }
};

struct ice_lance_t : public frost_mage_spell_t
{
  proc_source_t* extension_source;

  ice_lance_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "ice_lance", p, p -> find_specialization_spell( "Ice Lance" ) ),
    extension_source( nullptr )
  {
    parse_options( options_str );
    parse_effect_data( p -> find_spell( 228598 ) -> effectN( 1 ) );

    base_multiplier *= 1.0 + p -> talents.lonely_winter -> effectN( 1 ).percent();

    base_dd_adder += p -> azerite.whiteout.value( 3 );

    // TODO: Cleave distance for SI seems to be 8 + hitbox size.
    if ( p -> talents.splitting_ice -> ok() )
    {
      aoe = as<int>( 1 + p -> talents.splitting_ice -> effectN( 1 ).base_value() );
      base_multiplier *= 1.0 + p -> talents.splitting_ice -> effectN( 3 ).percent();
      base_aoe_multiplier *= p -> talents.splitting_ice -> effectN( 2 ).percent();
    }
    calculate_on_impact = true;
    track_shatter = true;
  }

  void init_finished() override
  {
    if ( p() -> talents.thermal_void -> ok() && sim -> report_details != 0 )
    {
      extension_source = p() -> get_proc_source( "Shatter/Thermal Void extension", FROZEN_MAX );
    }

    frost_mage_spell_t::init_finished();
  }

  virtual action_state_t* new_state() override
  { return new ice_lance_state_t( this, target ); }

  virtual unsigned frozen( const action_state_t* s ) const override
  {
    unsigned source = frost_mage_spell_t::frozen( s );

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
    if ( p() -> bugs )
    {
      if ( p() -> state.fingers_of_frost_active )
        source |= FF_FINGERS_OF_FROST;
    }
    else
    {
      if ( debug_cast<const ice_lance_state_t*>( s ) -> fingers_of_frost )
        source |= FF_FINGERS_OF_FROST;
    }

    return source;
  }

  virtual void execute() override
  {
    p() -> state.fingers_of_frost_active = p() -> buffs.fingers_of_frost -> up();

    frost_mage_spell_t::execute();

    p() -> buffs.magtheridons_might -> trigger();
    p() -> buffs.fingers_of_frost -> decrement();

    // Begin casting all Icicles at the target, beginning 0.25 seconds after the
    // Ice Lance cast with remaining Icicles launching at intervals of 0.4
    // seconds, the latter adjusted by haste. Casting continues until all
    // Icicles are gone, including new ones that accumulate while they're being
    // fired. If target dies, Icicles stop.
    if ( ! p() -> talents.glacial_spike -> ok() )
    {
      p() -> trigger_icicle( execute_state, true, target );
    }
    if ( p() -> azerite.whiteout.enabled() )
    {
      p() -> cooldowns.frozen_orb -> adjust(
        timespan_t::from_seconds( -0.1 * p() -> azerite.whiteout.spell_ref().effectN( 2 ).base_value() ),
        false );
    }
  }

  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  {
    debug_cast<ice_lance_state_t*>( s ) -> fingers_of_frost = p() -> buffs.fingers_of_frost -> check() != 0;
    frost_mage_spell_t::snapshot_state( s, rt );
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t t = frost_mage_spell_t::travel_time();

    if ( p() -> allow_shimmer_lance && p() -> buffs.shimmer -> check() )
    {
      double shimmer_distance = p() -> talents.shimmer -> effectN( 1 ).radius_max();
      t = std::max( t - timespan_t::from_seconds( shimmer_distance / travel_speed ), timespan_t::zero() );
    }

    return t;
  }

  virtual void impact( action_state_t* s ) override
  {
    frost_mage_spell_t::impact( s );

    if ( ! result_is_hit( s -> result ) )
      return;

    bool     primary = s -> chain_target == 0;
    unsigned frozen  = debug_cast<mage_spell_state_t*>( s ) -> frozen;

    if ( primary && frozen )
    {
      if ( p() -> talents.thermal_void -> ok() && p() -> buffs.icy_veins -> check() )
      {
        timespan_t tv_extension = p() -> talents.thermal_void
                                      -> effectN( 1 ).time_value() * 1000;

        p() -> buffs.icy_veins -> extend_duration( p(), tv_extension );

        if ( extension_source )
        {
          record_shatter_source( s, extension_source );
        }
      }

      if ( p() -> talents.chain_reaction -> ok() )
      {
        p() -> buffs.chain_reaction -> trigger();
      }

      if ( frozen &  FF_FINGERS_OF_FROST
        && frozen & ~FF_FINGERS_OF_FROST )
      {
        p() -> procs.fingers_of_frost_wasted -> occur();
      }
    }

    if ( primary )
    {
      if ( p() -> buffs.magtheridons_might -> default_chance != 0.0 )
        p() -> benefits.magtheridons_might -> update();
    }

    p() -> buffs.arctic_blast -> expire( timespan_t::from_seconds( 0.5 ) );
  }

  virtual double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.magtheridons_might -> check_stack_value();
    am *= 1.0 + p() -> buffs.arctic_blast -> check_value();
    am *= 1.0 + p() -> buffs.chain_reaction -> check_stack_value();

    return am;
  }

  virtual double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = frost_mage_spell_t::composite_da_multiplier( s );

    if ( debug_cast<const mage_spell_state_t*>( s ) -> frozen )
    {
      m *= 3.0;
    }

    return m;
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    double da = frost_mage_spell_t::bonus_da( s );

    const mage_td_t* td = p() -> target_data[ s -> target ];
    if ( td )
    {
      da += td -> debuffs.packed_ice -> check_value();
    }

    return da;
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

    double in_mult = p -> talents.ice_nova -> effectN( 3 ).percent();

    base_multiplier     *= in_mult;
    base_aoe_multiplier /= in_mult;
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
  icy_veins_t( mage_t* p, const std::string& options_str ) :
    frost_mage_spell_t( "icy_veins", p, p -> find_specialization_spell( "Icy Veins" ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  void init_finished() override
  {
    if ( p() -> buffs.lady_vashjs_grasp -> default_chance != 0.0 )
    {
      debug_cast<buffs::lady_vashjs_grasp_t*>( p() -> buffs.lady_vashjs_grasp )
        -> proc_fof = p() -> get_proc( "Fingers of Frost from Lady Vashj's Grasp" );
    }
    if ( p() -> azerite.frigid_grasp.enabled() )
    {
      proc_fof = p() -> get_proc( "Fingers of Frost from Frigid Grasp" );
    }

    frost_mage_spell_t::init_finished();
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    p() -> buffs.icy_veins -> trigger();

    // Refreshing infinite ticking buff doesn't quite work, remove
    // LVG manually and then trigger it again.
    p() -> buffs.lady_vashjs_grasp -> expire();
    p() -> buffs.lady_vashjs_grasp -> trigger();

    if ( p() -> azerite.frigid_grasp.enabled() )
    {
      trigger_fof( 1.0 );
      p() -> buffs.frigid_grasp -> trigger();
    }
  }
};

// Fire Blast Spell ======================================================

struct fire_blast_t : public fire_mage_spell_t
{
  fire_blast_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "fire_blast", p,
                       p -> find_specialization_spell( "Fire Blast" ) )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();

    cooldown -> charges = data().charges();
    cooldown -> charges += as<int>( p -> spec.fire_blast_3 -> effectN( 1 ).base_value() );
    cooldown -> charges += as<int>( p -> talents.flame_on -> effectN( 1 ).base_value() );

    cooldown -> duration = data().charge_cooldown();
    cooldown -> duration -= 1000 * p -> talents.flame_on -> effectN( 3 ).time_value();

    cooldown -> hasted = true;

    triggers_hot_streak = true;
    triggers_ignite = true;
    triggers_kindling = true;

    base_crit += p -> spec.fire_blast_2 -> effectN( 1 ).percent();
  }

  virtual void execute() override
  {
    fire_mage_spell_t::execute();

    // update_ready() assumes the ICD is affected by haste
    internal_cooldown -> start( data().cooldown() );

    p() -> buffs.blaster_master -> trigger();
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    double da = fire_mage_spell_t::bonus_da( s );

    const mage_td_t* td = p() -> target_data[ s -> target ];
    if ( td )
    {
      da += td -> debuffs.preheat -> check_value();
    }

    return da;
  }
};


// Living Bomb Spell ========================================================

struct living_bomb_explosion_t;
struct living_bomb_t;

struct living_bomb_explosion_t : public fire_mage_spell_t
{
  living_bomb_t* child_lb;

  living_bomb_explosion_t( mage_t* p, bool create_dot );
  virtual resource_e current_resource() const override;
  void impact( action_state_t* s ) override;
};

struct living_bomb_t : public fire_mage_spell_t
{
  living_bomb_explosion_t* explosion;

  living_bomb_t( mage_t* p, const std::string& options_str, bool casted );
  virtual timespan_t composite_dot_duration( const action_state_t* s ) const override;
  virtual void last_tick( dot_t* d ) override;
  virtual void init() override;
};

living_bomb_explosion_t::living_bomb_explosion_t( mage_t* p, bool create_dot ) :
  fire_mage_spell_t( "living_bomb_explosion", p, p -> find_spell( 44461 ) ),
  child_lb( create_dot ? new living_bomb_t( p, "", false ) : nullptr )
{
  aoe = -1;
  radius = 10;
  background = true;
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
    child_lb -> execute();
  }
}

living_bomb_t::living_bomb_t( mage_t* p, const std::string& options_str,
                              bool casted = true ) :
  fire_mage_spell_t( "living_bomb", p, p -> talents.living_bomb ),
  explosion( new living_bomb_explosion_t( p, casted ) )
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
  add_child( explosion );

  if ( ! casted )
  {
    background = true;
    base_costs[ RESOURCE_MANA ] = 0;
  }
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
  virtual dmg_e amount_type( const action_state_t* /* state */,
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
    fire_mage_spell_t( legendary ? "legendary_meteor_impact" : "meteor_impact",
                       p, p -> find_spell( 153564 ) ),
    meteor_burn( meteor_burn ),
    meteor_burn_duration( p -> find_spell( 175396 ) -> duration() )
  {
    background = true;
    aoe = targets;
    split_aoe_damage = true;
    triggers_ignite = true;

    meteor_burn_pulse_time = meteor_burn -> data().effectN( 1 ).period();

    // It seems that the 8th tick happens only very rarely in game.
    // As of build 25881, 2018-01-22.
    if ( p -> bugs )
    {
      meteor_burn_duration -= meteor_burn_pulse_time;
    }
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.0 );
  }

  virtual void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    p() -> ground_aoe_expiration[ meteor_burn -> name_str ]
      = sim -> current_time() + meteor_burn_duration;

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
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
      cooldown -> duration = timespan_t::zero();
    }
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t impact_time = meteor_delay * p() -> composite_spell_haste();
    timespan_t meteor_spawn = impact_time - meteor_impact -> travel_time();
    meteor_spawn = std::max( timespan_t::zero(), meteor_spawn );

    return meteor_spawn;
  }

  virtual void impact( action_state_t* s ) override
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

  void init_finished() override
  {
    std::vector<pet_t*> images = p() -> pets.mirror_images;

    for ( pet_t* image : images )
    {
      if ( ! image )
      {
        continue;
      }

      stats -> add_child( image -> get_stats( "arcane_blast" ) );
      stats -> add_child( image -> get_stats( "fireball" ) );
      stats -> add_child( image -> get_stats( "frostbolt" ) );
    }

    mage_spell_t::init_finished();
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
    return timespan_t::from_seconds( data().missile_speed() );
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
    p() -> benefits.arcane_charge.nether_tempest -> update();

    arcane_mage_spell_t::execute();

    if ( hit_any_target )
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

    nether_tempest_aoe -> set_target( d -> target );
    action_state_t* aoe_state = nether_tempest_aoe -> get_state();
    nether_tempest_aoe -> snapshot_state( aoe_state, nether_tempest_aoe -> amount_type( aoe_state ) );

    aoe_state -> persistent_multiplier *= d -> state -> persistent_multiplier;
    aoe_state -> da_multiplier *= d -> get_last_tick_factor();
    aoe_state -> ta_multiplier *= d -> get_last_tick_factor();

    nether_tempest_aoe -> schedule_execute( aoe_state );
  }

  virtual double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = arcane_mage_spell_t::composite_persistent_multiplier( state );

    m *= arcane_charge_damage_bonus();

    return m;
  }
};


// Phoenix Flames Spell ======================================================

struct phoenix_flames_splash_t : public fire_mage_spell_t
{
  phoenix_flames_splash_t( mage_t* p ) :
    fire_mage_spell_t( "phoenix_flames_splash", p, p -> find_spell( 257542 ) )
  {
    aoe = -1;
    background = true;
    triggers_ignite = true;
    // Phoenix Flames always crits
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
};

struct phoenix_flames_t : public fire_mage_spell_t
{
  phoenix_flames_splash_t* phoenix_flames_splash;

  bool pyrotex_ignition_cloth;
  timespan_t pyrotex_ignition_cloth_reduction;

  phoenix_flames_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "phoenix_flames", p, p -> talents.phoenix_flames ),
    phoenix_flames_splash( new phoenix_flames_splash_t( p ) ),
    pyrotex_ignition_cloth( false ),
    pyrotex_ignition_cloth_reduction( timespan_t::zero() )
  {
    parse_options( options_str );
    // Phoenix Flames always crits
    base_crit = 1.0;

    triggers_hot_streak = true;
    triggers_ignite = true;
    triggers_kindling = true;
    add_child( phoenix_flames_splash );
  }

  virtual void execute() override
  {
    fire_mage_spell_t::execute();

    if ( pyrotex_ignition_cloth )
    {
      p() -> cooldowns.combustion
          -> adjust( -1000 * pyrotex_ignition_cloth_reduction );
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    fire_mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      phoenix_flames_splash -> set_target( s -> target );
      phoenix_flames_splash -> execute();
    }
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t t = fire_mage_spell_t::travel_time();
    return std::min( t, timespan_t::from_seconds( 0.75 ) );
  }
};

// Pyroblast Spell ===================================================================
struct trailing_embers_t : public fire_mage_spell_t
{
  trailing_embers_t( mage_t* p ) :
    fire_mage_spell_t( "trailing_embers", p, p -> find_spell( 277703 ) )
  {
    base_td = p -> azerite.trailing_embers.value();
    background = true;
    hasted_ticks = false;
  }
};

struct pyroblast_t : public fire_mage_spell_t
{
  trailing_embers_t* trailing_embers;

  pyroblast_t( mage_t* p, const std::string& options_str ) :
    fire_mage_spell_t( "pyroblast", p, p -> find_specialization_spell( "Pyroblast" ) ),
    trailing_embers( nullptr )
  {
    parse_options( options_str );

    triggers_ignite = true;
    triggers_hot_streak = true;
    triggers_kindling = true;

    if ( p -> azerite.trailing_embers.enabled() ) 
    {
      trailing_embers = new trailing_embers_t( p );
      add_child( trailing_embers );
    }
  }

  virtual double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    if ( ! benefits_from_hot_streak() )
    {
      am *= 1.0 + p() -> buffs.kaelthas_ultimate_ability -> check_value()
                + p() -> buffs.pyroclasm -> check_value();
    }

    am *= 1.0 + p() -> buffs.critical_massive -> value();

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
    bool hot_streak = benefits_from_hot_streak( true );

    fire_mage_spell_t::execute();

    // Ignition/Critical Massive buffs are removed shortly after Flamestrike/Pyroblast cast.
    // In a situation where you're hardcasting FS/PB followed by a Hot Streak FS/FB, both
    // spells actually benefit. As of build 25881, 2018-01-22.
    p() -> buffs.ignition -> expire( p() -> bugs ? timespan_t::from_millis( 15 ) : timespan_t::zero() );
    p() -> buffs.critical_massive -> expire( p() -> bugs ? timespan_t::from_millis( 15 ) : timespan_t::zero() );

    if ( hot_streak )
    {
      p() -> buffs.hot_streak -> expire();

      p() -> buffs.kaelthas_ultimate_ability -> trigger();
      p() -> buffs.pyroclasm -> trigger();
      p() -> buffs.firemind -> trigger();

      if ( p() -> talents.pyromaniac -> ok()
        && rng().roll( p() -> talents.pyromaniac -> effectN( 1 ).percent() ) )
      {
        p() -> procs.hot_streak -> occur();
        p() -> procs.hot_streak_pyromaniac -> occur();
        p() -> buffs.hot_streak -> trigger();
      }
    }
    else
    {
      p() -> buffs.kaelthas_ultimate_ability -> decrement();
      p() -> buffs.pyroclasm -> decrement();
    }
  }

  virtual void snapshot_state( action_state_t* s, dmg_e rt ) override
  {
    fire_mage_spell_t::snapshot_state( s, rt );
    debug_cast<ignite_spell_state_t*>( s ) -> hot_streak = benefits_from_hot_streak();
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
      if ( p() -> sets -> has_set_bonus( MAGE_FIRE, T20, B4 ) && s -> result == RESULT_CRIT )
      {
        p() -> buffs.critical_massive -> trigger();
      }

      trigger_infernal_core( s -> target );
    }

    if ( p() -> azerite.trailing_embers.enabled() )
    {
      std::vector<player_t*> tl = target_list();
      for ( size_t i = 0, actors = tl.size(); i < actors; i++ )
      {
        trailing_embers -> set_target( tl[ i ] );
        trailing_embers -> execute();
      }
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

  virtual double composite_ignite_multiplier( const action_state_t* s ) const override
  {
    return debug_cast<const ignite_spell_state_t*>( s ) -> hot_streak ? 2.0 : 1.0;
  }

  virtual double composite_target_crit_chance( player_t* target ) const override
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

    channeled = true;
    // Triggers on execute as well as each tick.
    chills = true;
  }

  void init_finished() override
  {
    proc_fof = p() -> get_proc( std::string( "Fingers of Frost from " ) + data().name_cstr() );
    frost_mage_spell_t::init_finished();
  }

  virtual void tick( dot_t* d ) override
  {
    frost_mage_spell_t::tick( d );
    p() -> buffs.ray_of_frost -> trigger();
    if ( p() -> talents.bone_chilling -> ok() )
    {
      p() -> buffs.bone_chilling -> trigger();
    }

    // TODO: Now happens at 2.5 and 5.
    if ( d -> current_tick == 3 || d -> current_tick == 5 )
    {
      trigger_fof( 1.0 );
    }
  }

  virtual void last_tick( dot_t* d ) override
  {
    frost_mage_spell_t::last_tick( d );
    p() -> buffs.ray_of_frost -> expire();
    p() -> buffs.ice_floes -> decrement();
  }

  virtual double action_multiplier() const override
  {
    double am = frost_mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.ray_of_frost -> check_stack_value();

    return am;
  }
};


// Rune of Power Spell ==============================================================

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
    fire_mage_spell_t( "scorch", p, p -> find_specialization_spell( "Scorch" ) ),
    koralons_burning_touch( false ),
    koralons_burning_touch_threshold( 0.0 ),
    koralons_burning_touch_multiplier( 0.0 )
  {
    parse_options( options_str );

    triggers_hot_streak = true;
    triggers_ignite = true;
  }

  virtual double action_multiplier() const override
  {
    double am = fire_mage_spell_t::action_multiplier();

    double extra_multiplier = 0.0;

    if ( koralons_burning_touch && ( target -> health_percentage() <= koralons_burning_touch_threshold ) )
    {
      extra_multiplier += koralons_burning_touch_multiplier;
    }

    if ( p() -> talents.searing_touch -> ok()
      && target -> health_percentage() <= p() -> talents.searing_touch -> effectN( 1 ).base_value() )
    {
      extra_multiplier += p() -> talents.searing_touch -> effectN( 2 ).percent();
    }

    am *= 1.0 + extra_multiplier;

    return am;
  }

  virtual double composite_crit_chance() const override
  {
    double c = fire_mage_spell_t::composite_crit_chance();

    if ( koralons_burning_touch && ( target -> health_percentage() <= koralons_burning_touch_threshold ) )
    {
      c = 1.0;
    }

    if ( p() -> talents.searing_touch -> ok()
      && target -> health_percentage() <= p() -> talents.searing_touch -> effectN( 1 ).base_value() )
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

    if ( p() -> azerite.preheat.enabled() )
    {
      td( s -> target ) -> debuffs.preheat -> trigger();
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
    base_teleport_distance = data().effectN( 1 ).radius_max();

    movement_directionality = MOVEMENT_OMNI;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    p() -> buffs.shimmer -> trigger();
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
    track_cd_waste = false;
  }

  virtual void execute() override
  {
    frost_mage_spell_t::execute();

    p() -> pets.water_elemental -> summon();
  }

  virtual bool ready() override
  {
    if ( ! p() -> pets.water_elemental )
      return false;

    if ( ! p() -> pets.water_elemental -> is_sleeping() )
      return false;

    if ( p() -> talents.lonely_winter -> ok() )
      return false;

    return frost_mage_spell_t::ready();
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
    track_cd_waste = false;
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

    if ( ! shard && sim -> overrides.bloodlust )
      return false;

    if ( ! shard && player -> buffs.exhaustion -> check() )
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
    base_dd_min = base_dd_max = 1.0;
  }

  virtual void init() override
  {
    arcane_mage_spell_t::init();
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
    double mult = p() -> talents.touch_of_the_magi -> effectN( 1 ).percent();
    base_dd_min *= mult;
    base_dd_max *= mult;

    arcane_mage_spell_t::execute();
  }
};

// ============================================================================
// Mage Custom Actions
// ============================================================================

// Arcane Mage "Burn" State Switch Action =====================================

void report_burn_switch_error( action_t* a )
{
  throw std::runtime_error(fmt::format("{} action {} infinite loop detected "
                           "(no time passing between executes) at '{}'",
                           a -> player -> name(), a -> name(), a -> signature_str ));
}

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
    if ( ! success )
    {
      report_burn_switch_error( this );
      return;
    }

    p -> sample_data.burn_initial_mana -> add( p -> resources.current[ RESOURCE_MANA ] / p -> resources.max[ RESOURCE_MANA ] * 100);
    p -> uptime.burn_phase -> update( true, sim -> current_time() );
    p -> uptime.conserve_phase -> update( false, sim -> current_time() );
  }

  virtual bool ready() override
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

    p -> sample_data.burn_duration_history -> add( p -> burn_phase.duration( sim -> current_time() ).total_seconds() );

    bool success = p -> burn_phase.disable( sim -> current_time() );
    if ( ! success )
    {
      report_burn_switch_error( this );
      return;
    }

    p -> uptime.burn_phase -> update( false, sim -> current_time() );
    p -> uptime.conserve_phase -> update( true, sim -> current_time() );
  }

  virtual bool ready() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    if ( ! p -> burn_phase.on() )
    {
      return false;
    }

    return action_t::ready();
  }
};

// Proxy Freeze action ========================================================

struct freeze_t : public action_t
{
  action_t* pet_freeze;

  freeze_t( mage_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "freeze", p ),
    pet_freeze( nullptr )
  {
    parse_options( options_str );

    may_miss = may_crit = callbacks = false;
    dual = true;
    trigger_gcd = timespan_t::zero();
    ignore_false_positive = true;
    action_skill = 1;

    if ( p -> talents.lonely_winter -> ok() )
      background = true;
  }

  virtual void init_finished() override
  {
    action_t::init_finished();

    mage_t* m = debug_cast<mage_t*>( player );

    if ( ! m -> pets.water_elemental )
    {

      throw std::invalid_argument("Initializing freeze without a water elemental.");
    }

    pet_freeze = m -> pets.water_elemental -> find_action( "freeze" );

    if ( ! pet_freeze )
    {
      pet_freeze = new pets::water_elemental::freeze_t( m -> pets.water_elemental );
      pet_freeze -> init();
    }
  }

  virtual void execute() override
  {
    assert( pet_freeze );

    pet_freeze -> set_target( target );
    pet_freeze -> execute();
  }

  virtual bool ready() override
  {
    mage_t* m = debug_cast<mage_t*>( player );

    if ( ! m -> pets.water_elemental )
      return false;

    if ( m -> pets.water_elemental -> is_sleeping() )
      return false;

    if ( ! pet_freeze )
      return false;

    if ( ! pet_freeze -> ready() )
      return false;

    return action_t::ready();
  }
};

} // namespace actions


namespace events {
struct icicle_event_t : public event_t
{
  mage_t* mage;
  action_t* icicle_action;
  player_t* target;

  icicle_event_t( mage_t& m, action_t* a, player_t* t, bool first = false ) :
    event_t( m ), mage( &m ), icicle_action( a ), target( t )
  {
    double cast_time = first ? 0.25 : ( 0.4 * mage -> cache.spell_speed() );

    schedule( timespan_t::from_seconds( cast_time ) );
  }
  virtual const char* name() const override
  { return "icicle_event"; }

  virtual void execute() override
  {
    mage -> icicle_event = nullptr;

    // If the target of the icicle is dead, stop the chain
    if ( target -> is_sleeping() )
    {
      if ( mage -> sim -> debug )
        mage -> sim -> out_debug.printf( "%s icicle use on %s (sleeping target), stopping",
            mage -> name(), target -> name() );
      return;
    }

    icicle_action -> set_target( target );
    icicle_action -> execute();

    mage -> buffs.icicles -> decrement();

    action_t* new_action = mage -> get_icicle();
    if ( new_action )
    {
      mage -> icicle_event = make_event<icicle_event_t>( sim(), *mage, new_action, target );
      if ( mage -> sim -> debug )
        mage -> sim -> out_debug.printf( "%s icicle use on %s (chained), total=%u",
                               mage -> name(), target -> name(), as<unsigned>( mage -> icicles.size() ) );
    }
  }
};

struct ignite_spread_event_t : public event_t
{
  mage_t* mage;

  static double ignite_bank( dot_t* ignite )
  {
    if ( ! ignite -> is_ticking() )
    {
      return 0.0;
    }

    auto ignite_state = debug_cast<residual_action::residual_periodic_state_t*>(
        ignite -> state );
    return ignite_state -> tick_amount * ignite -> ticks_left();
  }

  static bool ignite_compare ( dot_t* a, dot_t* b )
  {
    double lv = ignite_bank( a ), rv = ignite_bank( b );
    if ( lv == rv )
    {
      timespan_t lr = a->remains(), rr = b->remains();
      if ( lr == rr )
      {
        return a < b;
      }
      return lr > rr;
    }
    return lv > rv;
  }

  ignite_spread_event_t( mage_t& m, timespan_t delta_time ) :
    event_t( m, delta_time ), mage( &m )
  { }

  virtual const char* name() const override
  { return "ignite_spread_event"; }

  virtual void execute() override
  {
    mage -> ignite_spread_event = nullptr;
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
    range::sort( active_ignites, ignite_compare );

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

      if ( ! candidates.empty() )
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

struct time_anomaly_tick_event_t : public event_t
{
  mage_t* mage;

  enum ta_proc_type_e
  {
    TA_ARCANE_POWER,
    TA_EVOCATION,
    TA_ARCANE_CHARGE
  };

  time_anomaly_tick_event_t( mage_t& m, timespan_t delta_time ) :
    event_t( m, delta_time ), mage( &m )
  { }

  virtual const char* name() const override
  { return "time_anomaly_tick_event"; }

  virtual void execute() override
  {
    mage -> time_anomaly_tick_event = nullptr;

    if ( mage -> sim -> log )
    {
      sim().out_log.printf( "%s Time Anomaly tick event occurs.", mage -> name() );
    }

    if ( mage -> shuffled_rng.time_anomaly -> trigger() )
    {
      // Proc was successful, figure out which effect to apply.
      if ( mage -> sim -> log )
      {
        sim().out_log.printf( "%s Time Anomaly proc successful, triggering effects.", mage -> name() );
      }

      std::vector<ta_proc_type_e> possible_procs;

      if ( mage -> buffs.arcane_power -> check() == 0 ) // TODO: Adjust the condition or remove when we have more info.
        possible_procs.push_back( TA_ARCANE_POWER );

      if ( mage -> buffs.evocation -> check() == 0 )
        possible_procs.push_back( TA_EVOCATION );

      if ( mage -> buffs.arcane_charge -> check() < 3 )
        possible_procs.push_back( TA_ARCANE_CHARGE );

      if ( ! possible_procs.empty() )
      {
        auto random_index = static_cast<unsigned>( rng().range( 0, as<double>( possible_procs.size() ) ) );
        auto proc = possible_procs[ random_index ];

        switch ( proc )
        {
          case TA_ARCANE_POWER:
          {
            timespan_t duration = timespan_t::from_seconds( mage -> talents.time_anomaly -> effectN( 1 ).base_value() );
            mage -> buffs.arcane_power -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
            break;
          }
          case TA_EVOCATION:
          {
            timespan_t duration = timespan_t::from_seconds( mage -> talents.time_anomaly -> effectN( 2 ).base_value() );
            mage -> trigger_evocation( duration, false );
            break;
          }
          case TA_ARCANE_CHARGE:
          {
            unsigned charges = as<unsigned>( mage -> talents.time_anomaly -> effectN( 3 ).base_value() );
            mage -> trigger_arcane_charge( charges );
            break;
          }
          default:
            assert( 0 );
            break;
        }
      }
    }

    mage -> time_anomaly_tick_event = make_event<events::time_anomaly_tick_event_t>(
      sim(), *mage, mage -> talents.time_anomaly -> effectN( 1 ).period() );
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
  dots.nether_tempest = target -> get_dot( "nether_tempest", mage );

  debuffs.frozen            = make_buff( *this, "frozen" )
                                -> set_duration( timespan_t::from_seconds( 0.5 ) );
  debuffs.winters_chill     = make_buff( *this, "winters_chill", mage -> find_spell( 228358 ) )
                                -> set_chance( mage -> spec.brain_freeze_2 -> ok() ? 1.0 : 0.0 );
  debuffs.touch_of_the_magi = make_buff<buffs::touch_of_the_magi_t>( this );
  debuffs.packed_ice        = make_buff( *this, "packed_ice", mage -> find_spell( 272970 ) )
                                -> set_chance( mage -> azerite.packed_ice.enabled() ? 1.0 : 0.0 )
                                -> set_default_value( mage -> azerite.packed_ice.value() );
  debuffs.preheat           = make_buff( *this, "preheat", mage -> find_spell( 273333 ) )
                                -> set_chance( mage -> azerite.preheat.enabled() ? 1.0 : 0.0 )
                                -> set_default_value( mage -> azerite.preheat.value() );
}

mage_t::mage_t( sim_t* sim, const std::string& name, race_e r ) :
  player_t( sim, MAGE, name, r ),
  icicle_event( nullptr ),
  icicle( icicles_t() ),
  ignite( nullptr ),
  ignite_spread_event( nullptr ),
  time_anomaly_tick_event( nullptr ),
  last_bomb_target( nullptr ),
  last_frostbolt_target( nullptr ),
  distance_from_rune( 0.0 ),
  firestarter_time( timespan_t::zero() ),
  blessing_of_wisdom_count( 0 ),
  allow_shimmer_lance( false ),
  action( actions_t() ),
  benefits( benefits_t() ),
  buffs( buffs_t() ),
  cooldowns( cooldowns_t() ),
  gains( gains_t() ),
  pets( pets_t() ),
  procs( procs_t() ),
  shuffled_rng( shuffled_rngs_t() ),
  sample_data( sample_data_t() ),
  spec( specializations_t() ),
  state( state_t() ),
  talents( talents_list_t() ),
  azerite( azerite_powers_t() )
{
  // Cooldowns
  cooldowns.combustion       = get_cooldown( "combustion"       );
  cooldowns.cone_of_cold     = get_cooldown( "cone_of_cold"     );
  cooldowns.evocation        = get_cooldown( "evocation"        );
  cooldowns.frost_nova       = get_cooldown( "frost_nova"       );
  cooldowns.frozen_orb       = get_cooldown( "frozen_orb"       );
  cooldowns.presence_of_mind = get_cooldown( "presence_of_mind" );
  cooldowns.time_warp        = get_cooldown( "time_warp"        );

  // Options
  regen_type = REGEN_DYNAMIC;

  talent_points.register_validity_fn( [ this ] ( const spell_data_t* spell )
  {
    // Soul of the Archmage
    if ( find_item( 151642 ) )
    {
      switch ( specialization() )
      {
        case MAGE_ARCANE:
          return spell -> id() == 236628; // Amplification
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
  delete benefits.arcane_charge.arcane_barrage;
  delete benefits.arcane_charge.arcane_blast;
  delete benefits.arcane_charge.nether_tempest;
  delete benefits.magtheridons_might;
  delete benefits.zannesu_journey;

  delete sample_data.burn_duration_history;
  delete sample_data.burn_initial_mana;

  delete sample_data.blizzard;
  delete sample_data.t20_4pc;
  delete sample_data.icy_veins_duration;
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
  if ( name == "arcane_barrage"         ) return new         arcane_barrage_t( this, options_str );
  if ( name == "arcane_blast"           ) return new           arcane_blast_t( this, options_str );
  if ( name == "arcane_explosion"       ) return new       arcane_explosion_t( this, options_str );
  if ( name == "arcane_missiles"        ) return new        arcane_missiles_t( this, options_str );
  if ( name == "arcane_orb"             ) return new             arcane_orb_t( this, options_str );
  if ( name == "arcane_power"           ) return new           arcane_power_t( this, options_str );
  if ( name == "charged_up"             ) return new             charged_up_t( this, options_str );
  if ( name == "evocation"              ) return new              evocation_t( this, options_str );
  if ( name == "nether_tempest"         ) return new         nether_tempest_t( this, options_str );
  if ( name == "presence_of_mind"       ) return new       presence_of_mind_t( this, options_str );
  if ( name == "slow"                   ) return new                   slow_t( this, options_str );
  if ( name == "summon_arcane_familiar" ) return new summon_arcane_familiar_t( this, options_str );
  if ( name == "supernova"              ) return new              supernova_t( this, options_str );

  if ( name == "start_burn_phase"       ) return new       start_burn_phase_t( this, options_str );
  if ( name == "stop_burn_phase"        ) return new        stop_burn_phase_t( this, options_str );

  // Fire
  if ( name == "blast_wave"             ) return new             blast_wave_t( this, options_str );
  if ( name == "combustion"             ) return new             combustion_t( this, options_str );
  if ( name == "dragons_breath"         ) return new         dragons_breath_t( this, options_str );
  if ( name == "fireball"               ) return new               fireball_t( this, options_str );
  if ( name == "flamestrike"            ) return new            flamestrike_t( this, options_str );
  if ( name == "fire_blast"             ) return new             fire_blast_t( this, options_str );
  if ( name == "living_bomb"            ) return new            living_bomb_t( this, options_str );
  if ( name == "meteor"                 ) return new                 meteor_t( this, options_str );
  if ( name == "phoenix_flames"         ) return new         phoenix_flames_t( this, options_str );
  if ( name == "pyroblast"              ) return new              pyroblast_t( this, options_str );
  if ( name == "scorch"                 ) return new                 scorch_t( this, options_str );

  // Frost
  if ( name == "blizzard"               ) return new               blizzard_t( this, options_str );
  if ( name == "cold_snap"              ) return new              cold_snap_t( this, options_str );
  if ( name == "comet_storm"            ) return new            comet_storm_t( this, options_str );
  if ( name == "cone_of_cold"           ) return new           cone_of_cold_t( this, options_str );
  if ( name == "ebonbolt"               ) return new               ebonbolt_t( this, options_str );
  if ( name == "flurry"                 ) return new                 flurry_t( this, options_str );
  if ( name == "frostbolt"              ) return new              frostbolt_t( this, options_str );
  if ( name == "frozen_orb"             ) return new             frozen_orb_t( this, options_str );
  if ( name == "glacial_spike"          ) return new          glacial_spike_t( this, options_str );
  if ( name == "ice_floes"              ) return new              ice_floes_t( this, options_str );
  if ( name == "ice_lance"              ) return new              ice_lance_t( this, options_str );
  if ( name == "ice_nova"               ) return new               ice_nova_t( this, options_str );
  if ( name == "icy_veins"              ) return new              icy_veins_t( this, options_str );
  if ( name == "ray_of_frost"           ) return new           ray_of_frost_t( this, options_str );
  if ( name == "water_elemental"        ) return new summon_water_elemental_t( this, options_str );

  if ( name == "freeze"                 ) return new                 freeze_t( this, options_str );

  // Shared spells
  if ( name == "arcane_intellect"       ) return new       arcane_intellect_t( this, options_str );
  if ( name == "blink" )
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
  if ( name == "counterspell"           ) return new           counterspell_t( this, options_str );
  if ( name == "frost_nova"             ) return new             frost_nova_t( this, options_str );
  if ( name == "time_warp"              ) return new              time_warp_t( this, options_str );

  // Shared talents
  if ( name == "mirror_image"           ) return new           mirror_image_t( this, options_str );
  if ( name == "rune_of_power"          ) return new          rune_of_power_t( this, options_str );
  if ( name == "shimmer"                ) return new                shimmer_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// mage_t::create_actions =====================================================

void mage_t::create_actions()
{
  using namespace actions;

  if ( spec.ignite -> ok() )
  {
    ignite = new ignite_t( this );
  }

  if ( spec.icicles -> ok() )
  {
    icicle.frostbolt = new icicle_t( this, "frostbolt" );
    icicle.flurry    = new icicle_t( this, "flurry" );
  }

  if ( talents.arcane_familiar -> ok() )
  {
    action.arcane_assault = new arcane_assault_t( this );
  }

  if ( talents.conflagration -> ok() )
  {
    action.conflagration_flare_up = new conflagration_flare_up_t( this );
  }

  if ( talents.touch_of_the_magi -> ok() )
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

  if ( azerite.glacial_assault.enabled() )
  {
    action.glacial_assault = new glacial_assault_t( this );
  }

  player_t::create_actions();
}

// mage_t::create_options =====================================================
void mage_t::create_options()
{
  add_option( opt_timespan( "firestarter_time", firestarter_time ) );
  add_option( opt_int( "blessing_of_wisdom_count", blessing_of_wisdom_count ) );
  add_option( opt_bool( "allow_shimmer_lance", allow_shimmer_lance ) );
  player_t::create_options();
}

// mage_t::create_profile ================================================

std::string mage_t::create_profile( save_e save_type )
{
  std::string profile = player_t::create_profile( save_type );

  if ( save_type & SAVE_PLAYER )
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

  firestarter_time          = p -> firestarter_time;
  blessing_of_wisdom_count  = p -> blessing_of_wisdom_count;
  allow_shimmer_lance       = p -> allow_shimmer_lance;
}

// mage_t::merge =========================================================

void mage_t::merge( player_t& other )
{
  player_t::merge( other );

  mage_t& mage = dynamic_cast<mage_t&>( other );

  for ( size_t i = 0; i < cooldown_waste_data_list.size(); i++ )
  {
    cooldown_waste_data_list[ i ] -> merge( *mage.cooldown_waste_data_list[ i ] );
  }

  for ( size_t i = 0; i < proc_source_list.size(); i++ )
  {
    proc_source_list[ i ] -> merge( *mage.proc_source_list[ i ] );
  }

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      sample_data.burn_duration_history -> merge ( *mage.sample_data.burn_duration_history );
      sample_data.burn_initial_mana -> merge( *mage.sample_data.burn_initial_mana );
      break;

    case MAGE_FIRE:
      break;

    case MAGE_FROST:
      if ( talents.thermal_void -> ok() )
      {
        sample_data.icy_veins_duration -> merge( *mage.sample_data.icy_veins_duration );
      }
      break;

    default:
      break;
  }
}

// mage_t::analyze =======================================================

void mage_t::analyze( sim_t& s )
{
  player_t::analyze( s );

  range::for_each( cooldown_waste_data_list, std::mem_fn( &cooldown_waste_data_t::analyze ) );

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      sample_data.burn_duration_history -> analyze();
      sample_data.burn_initial_mana -> analyze();
      break;

    case MAGE_FIRE:
      break;

    case MAGE_FROST:
      if ( talents.thermal_void -> ok() )
      {
        sample_data.icy_veins_duration -> analyze();
      }
      break;

    default:
      break;
  }
}

// mage_t::datacollection_begin ===============================================

void mage_t::datacollection_begin()
{
  player_t::datacollection_begin();

  range::for_each( cooldown_waste_data_list, std::mem_fn( &cooldown_waste_data_t::datacollection_begin ) );
  range::for_each( proc_source_list, std::mem_fn( &proc_source_t::datacollection_begin ) );
}

// mage_t::datacollection_end =================================================

void mage_t::datacollection_end()
{
  player_t::datacollection_end();

  range::for_each( cooldown_waste_data_list, std::mem_fn( &cooldown_waste_data_t::datacollection_end ) );
  range::for_each( proc_source_list, std::mem_fn( &proc_source_t::datacollection_end ) );
}

// mage_t::regen ==============================================================

void mage_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( resources.is_active( RESOURCE_MANA ) && buffs.evocation -> check() )
  {
    double base = resource_regen_per_second( RESOURCE_MANA );
    if ( base )
    {
      // Base regen was already done, subtract 1.0 from Evocation's mana regen multiplier to make
      // sure we don't apply it twice.
      resource_gain(
        RESOURCE_MANA,
        ( buffs.evocation -> check_value() - 1.0 ) * base * periodicity.total_seconds(),
        gains.evocation );
    }
  }
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
    int image_num = as<int>( talents.mirror_image -> effectN( 2 ).base_value() );
    for ( int i = 0; i < image_num; i++ )
    {
      pets.mirror_images.push_back( new pets::mirror_image::mirror_image_pet_t( sim, this ) );
      if ( i > 0 )
      {
        pets.mirror_images[ i ] -> quiet = true;
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
  talents.amplification      = find_talent_spell( "Amplification"      );
  talents.rule_of_threes     = find_talent_spell( "Rule of Threes"     );
  talents.arcane_familiar    = find_talent_spell( "Arcane Familiar"    );
  talents.firestarter        = find_talent_spell( "Firestarter"        );
  talents.pyromaniac         = find_talent_spell( "Pyromaniac"         );
  talents.searing_touch      = find_talent_spell( "Searing Touch"      );
  talents.bone_chilling      = find_talent_spell( "Bone Chilling"      );
  talents.lonely_winter      = find_talent_spell( "Lonely Winter"      );
  talents.ice_nova           = find_talent_spell( "Ice Nova"           );
  // Tier 30
  talents.shimmer            = find_talent_spell( "Shimmer"            );
  talents.mana_shield        = find_talent_spell( "Mana Shield"        );
  talents.slipstream         = find_talent_spell( "Slipstream"         );
  talents.blazing_soul       = find_talent_spell( "Blazing Soul"       );
  talents.blast_wave         = find_talent_spell( "Blast Wave"         );
  talents.glacial_insulation = find_talent_spell( "Glacial Insulation" );
  talents.ice_floes          = find_talent_spell( "Ice Floes"          );
  // Tier 45
  talents.incanters_flow     = find_talent_spell( "Incanter's Flow"    );
  talents.mirror_image       = find_talent_spell( "Mirror Image"       );
  talents.rune_of_power      = find_talent_spell( "Rune of Power"      );
  // Tier 60
  talents.resonance          = find_talent_spell( "Resonance"          );
  talents.charged_up         = find_talent_spell( "Charged Up"         );
  talents.supernova          = find_talent_spell( "Supernova"          );
  talents.flame_on           = find_talent_spell( "Flame On"           );
  talents.alexstraszas_fury  = find_talent_spell( "Alexstrasza's Fury" );
  talents.phoenix_flames     = find_talent_spell( "Phoenix Flames"     );
  talents.frozen_touch       = find_talent_spell( "Frozen Touch"       );
  talents.chain_reaction     = find_talent_spell( "Chain Reaction"     );
  talents.ebonbolt           = find_talent_spell( "Ebonbolt"           );
  // Tier 75
  talents.ice_ward           = find_talent_spell( "Ice Ward"           );
  talents.ring_of_frost      = find_talent_spell( "Ring of Frost"      );
  talents.chrono_shift       = find_talent_spell( "Chrono Shift"       );
  talents.frenetic_speed     = find_talent_spell( "Frenetic Speed"     );
  talents.frigid_winds       = find_talent_spell( "Frigid Winds"       );
  // Tier 90
  talents.reverberate        = find_talent_spell( "Reverberate"        );
  talents.touch_of_the_magi  = find_talent_spell( "Touch of the Magi"  );
  talents.nether_tempest     = find_talent_spell( "Nether Tempest"     );
  talents.flame_patch        = find_talent_spell( "Flame Patch"        );
  talents.conflagration      = find_talent_spell( "Conflagration"      );
  talents.living_bomb        = find_talent_spell( "Living Bomb"        );
  talents.freezing_rain      = find_talent_spell( "Freezing Rain"      );
  talents.splitting_ice      = find_talent_spell( "Splitting Ice"      );
  talents.comet_storm        = find_talent_spell( "Comet Storm"        );
  // Tier 100
  talents.overpowered        = find_talent_spell( "Overpowered"        );
  talents.time_anomaly       = find_talent_spell( "Time Anomaly"       );
  talents.arcane_orb         = find_talent_spell( "Arcane Orb"         );
  talents.kindling           = find_talent_spell( "Kindling"           );
  talents.pyroclasm          = find_talent_spell( "Pyroclasm"          );
  talents.meteor             = find_talent_spell( "Meteor"             );
  talents.thermal_void       = find_talent_spell( "Thermal Void"       );
  talents.ray_of_frost       = find_talent_spell( "Ray of Frost"       );
  talents.glacial_spike      = find_talent_spell( "Glacial Spike"      );

  // Spec Spells
  spec.arcane_barrage_2      = find_specialization_spell( 231564 );
  spec.arcane_charge         = find_spell( 36032 );
  spec.arcane_mage           = find_specialization_spell( 137021 );
  spec.clearcasting          = find_specialization_spell( "Clearcasting" );
  spec.evocation_2           = find_specialization_spell( 231565 );

  spec.critical_mass         = find_specialization_spell( "Critical Mass"    );
  spec.critical_mass_2       = find_specialization_spell( 231630 );
  spec.enhanced_pyrotechnics = find_specialization_spell( 157642 );
  spec.fire_blast_2          = find_specialization_spell( 231568 );
  spec.fire_blast_3          = find_specialization_spell( 231567 );
  spec.fire_mage             = find_specialization_spell( 137019 );
  spec.hot_streak            = find_specialization_spell( 195283 );

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

  // Azerite
  azerite.anomalous_impact         = find_azerite_spell( "Anomalous Impact"         );
  azerite.arcane_pressure          = find_azerite_spell( "Arcane Pressure"          );
  azerite.arcane_pummeling         = find_azerite_spell( "Arcane Pummeling"         );
  azerite.brain_storm              = find_azerite_spell( "Brain Storm"              );
  azerite.explosive_echo           = find_azerite_spell( "Explosive Echo"           );
  azerite.galvanizing_spark        = find_azerite_spell( "Galvanizing Spark"        );

  azerite.blaster_master           = find_azerite_spell( "Blaster Master"           );
  azerite.duplicative_incineration = find_azerite_spell( "Duplicative Incineration" );
  azerite.firemind                 = find_azerite_spell( "Firemind"                 );
  azerite.flames_of_alacrity       = find_azerite_spell( "Flames of Alacrity"       );
  azerite.preheat                  = find_azerite_spell( "Preheat"                  );
  azerite.trailing_embers          = find_azerite_spell( "Trailing Embers"          );

  azerite.frigid_grasp             = find_azerite_spell( "Frigid Grasp"             );
  azerite.glacial_assault          = find_azerite_spell( "Glacial Assault"          );
  azerite.packed_ice               = find_azerite_spell( "Packed Ice"               );
  azerite.tunnel_of_ice            = find_azerite_spell( "Tunnel of Ice"            );
  azerite.whiteout                 = find_azerite_spell( "Whiteout"                 );
  azerite.winters_reach            = find_azerite_spell( "Winter's Reach"           );
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

  // Mana Attunement
  resources.base_regen_per_second[ RESOURCE_MANA ] *= 1.0 + find_spell( 121039 ) -> effectN( 1 ).percent();

  if ( specialization() == MAGE_ARCANE )
  {
    regen_caches[ CACHE_MASTERY ] = true;
  }
}

// mage_t::create_buffs =======================================================

void mage_t::create_buffs()
{
  player_t::create_buffs();

  // Arcane
  buffs.arcane_charge        = make_buff( this, "arcane_charge", spec.arcane_charge );
  buffs.arcane_power         = make_buff( this, "arcane_power", find_spell( 12042 ) )
                                 -> set_cooldown( timespan_t::zero() )
                                 -> set_default_value( find_spell( 12042 ) -> effectN( 1 ).percent()
                                                     + talents.overpowered -> effectN( 1 ).percent() );
  buffs.clearcasting         = make_buff( this, "clearcasting", find_spell( 263725 ) )
                                 -> set_default_value( find_spell( 263725 ) -> effectN( 1 ).percent() );
  buffs.clearcasting_channel = make_buff( this, "clearcasting_channel", find_spell( 277726 ) )
                                 -> set_quiet( true );
  buffs.evocation            = make_buff( this, "evocation", find_spell( 12051 ) )
                                 -> set_default_value( find_spell( 12051 ) -> effectN( 1 ).percent() )
                                 -> set_cooldown( timespan_t::zero() )
                                 -> set_affects_regen( true );
  buffs.presence_of_mind     = make_buff( this, "presence_of_mind", find_spell( 205025 ) )
                                 -> set_cooldown( timespan_t::zero() )
                                 -> set_stack_change_callback( [ this ] ( buff_t*, int, int cur )
                                    { if ( cur == 0 ) cooldowns.presence_of_mind -> start(); } );

  buffs.arcane_familiar      = make_buff( this, "arcane_familiar", find_spell( 210126 ) )
                                 -> set_default_value( find_spell( 210126 ) -> effectN( 1 ).percent() )
                                 -> set_period( timespan_t::from_seconds( 3.0 ) )
                                 -> set_tick_time_behavior( buff_tick_time_behavior::HASTED )
                                 -> set_tick_callback( [ this ] ( buff_t*, int, const timespan_t& )
                                    {
                                      assert( action.arcane_assault );
                                      action.arcane_assault -> set_target( target );
                                      action.arcane_assault -> execute();
                                    } )
                                 -> set_stack_change_callback( [ this ] ( buff_t*, int, int )
                                    { recalculate_resource_max( RESOURCE_MANA ); } );
  buffs.chrono_shift         = make_buff( this, "chrono_shift", find_spell( 236298 ) )
                                 -> set_default_value( find_spell( 236298 ) -> effectN( 1 ).percent() )
                                 -> add_invalidate( CACHE_RUN_SPEED );
  buffs.rule_of_threes       = make_buff( this, "rule_of_threes", find_spell( 264774 ) )
                                 -> set_default_value( find_spell( 264774 ) -> effectN( 1 ).percent() );

  buffs.crackling_energy     = make_buff( this, "crackling_energy", find_spell( 246224 ) )
                                 -> set_default_value( find_spell( 246224 ) -> effectN( 1 ).percent() );
  buffs.expanding_mind       = make_buff( this, "expanding_mind", find_spell( 253262 ) )
                                 -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.quick_thinker        = make_buff( this, "quick_thinker", find_spell( 253299 ) )
                                 -> set_default_value( find_spell( 253299 ) -> effectN( 1 ).percent() )
                                 -> set_chance( sets -> set( MAGE_ARCANE, T21, B4 ) -> proc_chance() )
                                 -> add_invalidate( CACHE_SPELL_HASTE );


  // Fire
  buffs.combustion             = make_buff( this, "combustion", find_spell( 190319 ) )
                                   -> set_cooldown( timespan_t::zero() )
                                   -> add_invalidate( CACHE_MASTERY )
                                   -> set_default_value( find_spell( 190319 ) -> effectN( 1 ).percent() );
  buffs.combustion -> buff_duration += sets -> set( MAGE_FIRE, T21, B2 ) -> effectN( 1 ).time_value();
  buffs.enhanced_pyrotechnics  = make_buff( this, "enhanced_pyrotechnics", find_spell( 157644 ) )
                                   -> set_chance( spec.enhanced_pyrotechnics -> ok() ? 1.0 : 0.0 )
                                   -> set_default_value( find_spell( 157644 ) -> effectN( 1 ).percent()
                                        + sets -> set( MAGE_FIRE, T19, B2 ) -> effectN( 1 ).percent() );
  buffs.heating_up             = make_buff( this, "heating_up",  find_spell( 48107 ) );
  buffs.hot_streak             = make_buff( this, "hot_streak",  find_spell( 48108 ) );

  buffs.frenetic_speed         = make_buff( this, "frenetic_speed", find_spell( 236060 ) )
                                   -> set_default_value( find_spell( 236060 ) -> effectN( 1 ).percent() )
                                   -> add_invalidate( CACHE_RUN_SPEED );
  buffs.pyroclasm              = make_buff( this, "pyroclasm", find_spell( 269651 ) )
                                   -> set_default_value( find_spell( 269651 ) -> effectN( 1 ).percent() )
                                   -> set_chance( talents.pyroclasm -> effectN( 1 ).percent() );

  buffs.streaking              = make_buff( this, "streaking", find_spell( 211399 ) )
                                   -> set_default_value( find_spell( 211399 ) -> effectN( 1 ).percent() )
                                   -> add_invalidate( CACHE_SPELL_HASTE );
  buffs.ignition               = make_buff( this, "ignition", find_spell( 246261 ) )
                                   -> set_trigger_spell( sets -> set( MAGE_FIRE, T20, B2 ) );
  buffs.critical_massive       = make_buff( this, "critical_massive", find_spell( 242251 ) )
                                   -> set_default_value( find_spell( 242251 ) -> effectN( 1 ).percent() );
  buffs.inferno                = make_buff( this, "inferno", find_spell( 253220 ) )
                                   -> set_default_value( find_spell( 253220 ) -> effectN( 1 ).percent() )
                                   -> set_duration( buffs.combustion -> buff_duration );

  buffs.erupting_infernal_core = make_buff( this, "erupting_infernal_core", find_spell( 248147 ) );

  // Frost
  buffs.brain_freeze           = make_buff<buffs::brain_freeze_buff_t>( this );
  buffs.fingers_of_frost       = make_buff( this, "fingers_of_frost", find_spell( 44544 ) );
  // Buff to track icicles. This does not, however, track the true amount of icicles present.
  // Instead, as it does in game, it tracks icicle buff stack count based on the number of *casts*
  // of icicle generating spells. icicles are generated on impact, so they are slightly de-synced.
  //
  // A note about in-game implementation. At first, it might seem that each stack has an independent
  // expiration timer, but the timing is a bit off and it just doesn't happen in the cases where
  // Icicle buff is incremented but the actual Icicle never created.
  //
  // Instead, the buff is incremented when:
  //   * Frostbolt executes
  //   * Ice Nine creates another Icicle
  //   * One of the Icicles overflows
  //
  // It is unclear if the buff is incremented twice or three times when Ice Nine procs and two Icicles
  // overflow (combat log doesn't track refreshes for Icicles buff).
  //
  // The buff is decremented when:
  //   * One of the Icicles is removed
  //     - Launched after Ice Lance
  //     - Launched on overflow
  //     - Removed as a part of Glacial Spike execute
  //     - Expired after 60 sec
  //
  // This explains why some Icicle stacks remain if Glacial Spike executes with 5 Icicle stacks but less
  // than 5 actual Icicles.
  buffs.icicles                = make_buff( this, "icicles", find_spell( 205473 ) );
  buffs.icy_veins              = make_buff<buffs::icy_veins_buff_t>( this );

  buffs.bone_chilling          = make_buff( this, "bone_chilling", find_spell( 205766 ) )
                                   -> set_default_value( talents.bone_chilling -> effectN( 1 ).percent() / 10 );
  buffs.chain_reaction         = make_buff( this, "chain_reaction", find_spell( 278310 ) )
                                   -> set_default_value( find_spell( 278310 ) -> effectN( 1 ).percent() );
  buffs.freezing_rain          = make_buff( this, "freezing_rain", find_spell( 270232 ) )
                                   -> set_default_value( find_spell( 270232 ) -> effectN( 2 ).percent() );
  buffs.ice_floes              = make_buff( this, "ice_floes", talents.ice_floes );
  buffs.ray_of_frost           = make_buff( this, "ray_of_frost", find_spell( 208141 ) )
                                   -> set_default_value( find_spell( 208141 ) -> effectN( 1 ).percent() );

  buffs.frozen_mass            = make_buff( this, "frozen_mass", find_spell( 242253 ) )
                                   -> set_default_value( find_spell( 242253 ) -> effectN( 1 ).percent() );
  buffs.arctic_blast           = make_buff( this, "arctic_blast", find_spell( 253257 ) )
                                   -> set_default_value( find_spell( 253257 ) -> effectN( 1 ).percent() )
                                   -> set_chance( sets -> has_set_bonus( MAGE_FROST, T21, B4 ) ? 1.0 : 0.0 );

  buffs.lady_vashjs_grasp      = make_buff<buffs::lady_vashjs_grasp_t>( this );
  buffs.rage_of_the_frost_wyrm = make_buff( this, "rage_of_the_frost_wyrm", find_spell( 248177 ) );


  // Shared
  buffs.incanters_flow = make_buff<buffs::incanters_flow_t>( this );
  buffs.rune_of_power  = make_buff( this, "rune_of_power", find_spell( 116014 ) )
                           -> set_default_value( find_spell( 116014 ) -> effectN( 1 ).percent() );

  // Azerite
  buffs.arcane_pummeling   = make_buff( this, "arcane_pummeling", find_spell( 270670 ) )
                               -> set_default_value( azerite.arcane_pummeling.value() )
                               -> set_chance( azerite.arcane_pummeling.enabled() ? 1.0 : 0.0 );
  buffs.brain_storm        = make_buff<stat_buff_t>( this, "brain_storm", find_spell( 273330 ) )
                               -> add_stat( STAT_INTELLECT, azerite.brain_storm.value() );

  buffs.blaster_master     = make_buff<stat_buff_t>( this, "blaster_master", find_spell( 274598 ) )
                               -> add_stat( STAT_MASTERY_RATING, azerite.blaster_master.value() )
                               -> set_chance( azerite.blaster_master.enabled() ? 1.0 : 0.0 );
  buffs.firemind           = make_buff<stat_buff_t>( this, "firemind", find_spell( 279715 ) )
                               -> add_stat( STAT_INTELLECT, azerite.firemind.value() )
                               -> set_chance( azerite.firemind.enabled() ? 1.0 : 0.0 );
  buffs.flames_of_alacrity = make_buff<stat_buff_t>( this, "flames_of_alacrity", find_spell( 272934 ) )
                               -> add_stat( STAT_HASTE_RATING, azerite.flames_of_alacrity.value() )
                               -> set_chance( azerite.flames_of_alacrity.enabled() ? 1.0 : 0.0 );

  buffs.frigid_grasp       = make_buff<stat_buff_t>( this, "frigid_grasp", find_spell( 279684 ) )
                               -> add_stat( STAT_INTELLECT, azerite.frigid_grasp.value() );
  buffs.tunnel_of_ice      = make_buff( this, "tunnel_of_ice", find_spell( 277904 ) )
                               -> set_chance( azerite.tunnel_of_ice.enabled() ? 1.0 : 0.0 )
                               -> set_default_value( azerite.tunnel_of_ice.value() );
  buffs.winters_reach      = make_buff( this, "winters_reach", find_spell( 273347 ) )
                               -> set_chance( azerite.winters_reach.spell_ref().effectN( 2 ).percent() )
                               -> set_default_value( azerite.winters_reach.value() );

  // Misc
  // N active GBoWs are modeled by a single buff that gives N times as much mana.
  buffs.greater_blessing_of_widsom =
    make_buff( this, "greater_blessing_of_wisdom", find_spell( 203539 ) )
      -> set_tick_callback( [ this ]( buff_t*, int, const timespan_t& )
         { resource_gain( RESOURCE_MANA,
                          resources.max[ RESOURCE_MANA ] * 0.002 * blessing_of_wisdom_count,
                          gains.greater_blessing_of_wisdom ); } )
      -> set_period( find_spell( 203539 ) -> effectN( 2 ).period() )
      -> set_tick_behavior( buff_tick_behavior::CLIP );
  buffs.t19_oh_buff = make_buff<stat_buff_t>( this, "ancient_knowledge", sets -> set( specialization(), T19OH, B8 ) -> effectN( 1 ).trigger() )
                        -> set_trigger_spell( sets -> set( specialization(), T19OH, B8 ) );
  buffs.shimmer     = make_buff( this, "shimmer", find_spell( 212653 ) );
}

// mage_t::init_gains =======================================================

void mage_t::init_gains()
{
  player_t::init_gains();

  gains.evocation                      = get_gain( "Evocation"                      );
  gains.mystic_kilt_of_the_rune_master = get_gain( "Mystic Kilt of the Rune Master" );
  gains.greater_blessing_of_wisdom     = get_gain( "Greater Blessing of Wisdom"     );
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
      procs.brain_freeze_flurry     = get_proc( "Brain Freeze Flurries cast" );
      procs.fingers_of_frost_wasted = get_proc( "Fingers of Frost wasted due to Winter's Chill" );
      break;
    case MAGE_FIRE:
      procs.heating_up_generated         = get_proc( "Heating Up generated" );
      procs.heating_up_removed           = get_proc( "Heating Up removed" );
      procs.heating_up_ib_converted      = get_proc( "IB conversions of HU" );
      procs.hot_streak                   = get_proc( "Total Hot Streak procs" );
      procs.hot_streak_pyromaniac        = get_proc( "Total Hot Streak procs from Pyromaniac" );
      procs.hot_streak_spell             = get_proc( "Hot Streak spells used" );
      procs.hot_streak_spell_crit        = get_proc( "Hot Streak spell crits" );
      procs.hot_streak_spell_crit_wasted = get_proc( "Wasted Hot Streak spell crits" );

      procs.ignite_applied    = get_proc( "Direct Ignite applications" );
      procs.ignite_spread     = get_proc( "Ignites spread" );
      procs.ignite_new_spread = get_proc( "Ignites spread to new targets" );
      procs.ignite_overwrite  = get_proc( "Ignites spread to target with existing ignite" );
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

// mage_t::init_benefits ======================================================

void mage_t::init_benefits()
{
  player_t::init_benefits();

  if ( specialization() == MAGE_ARCANE )
  {
    benefits.arcane_charge.arcane_barrage =
      new buff_stack_benefit_t( buffs.arcane_charge, "Arcane Barrage" );
    benefits.arcane_charge.arcane_blast =
      new buff_stack_benefit_t( buffs.arcane_charge, "Arcane Blast" );
    if ( talents.nether_tempest -> ok() )
    {
      benefits.arcane_charge.nether_tempest =
        new buff_stack_benefit_t( buffs.arcane_charge, "Nether Tempest" );
    }
  }

  if ( specialization() == MAGE_FROST )
  {
    if ( buffs.magtheridons_might -> default_chance != 0.0 )
    {
      benefits.magtheridons_might =
        new buff_stack_benefit_t( buffs.magtheridons_might, "Ice Lance +" );
    }

    if ( buffs.zannesu_journey -> default_chance != 0.0 )
    {
      benefits.zannesu_journey =
        new buff_stack_benefit_t( buffs.zannesu_journey, "Blizzard +" );
    }
  }
}

// mage_t::init_uptimes =======================================================

void mage_t::init_uptimes()
{
  player_t::init_uptimes();

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      uptime.burn_phase     = get_uptime( "Burn Phase" );
      uptime.conserve_phase = get_uptime( "Conserve Phase" );

      sample_data.burn_duration_history = new extended_sample_data_t( "Burn duration history", false );
      sample_data.burn_initial_mana     = new extended_sample_data_t( "Burn initial mana", false );
      break;

    case MAGE_FROST:
      sample_data.blizzard     = new cooldown_reduction_data_t( cooldowns.frozen_orb, "Blizzard" );

      if ( talents.thermal_void -> ok() )
      {
        sample_data.icy_veins_duration = new extended_sample_data_t( "Icy Veins duration", false );
      }

      if ( sets -> has_set_bonus( MAGE_FROST, T20, B4 ) )
      {
        sample_data.t20_4pc = new cooldown_reduction_data_t( cooldowns.frozen_orb, "T20 4pc" );
      }
      break;

    case MAGE_FIRE:
      break;

    default:
      break;
  }

}

void mage_t::init_rng()
{
  player_t::init_rng();

  // TODO: There's no data about this in game. Keep an eye out in case Blizzard
  // changes this behind the scenes.
  shuffled_rng.time_anomaly = get_shuffled_rng( "time_anomaly", 1, 16 );
}

// mage_t::init_assessors =====================================================
void mage_t::init_assessors()
{
  player_t::init_assessors();

  if ( talents.touch_of_the_magi -> ok() )
  {
    auto assessor_fn = [ this ] ( dmg_e, action_state_t* s ) {
      auto buff = debug_cast<buffs::touch_of_the_magi_t*>( get_target_data( s -> target ) -> debuffs.touch_of_the_magi );

      if ( buff -> check() )
      {
        buff -> accumulate_damage( s );
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
  precombat -> add_action( this, "Arcane Intellect" );

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
      precombat -> add_action( this, "Arcane Blast" );
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
    ( specialization() == MAGE_FIRE )   ? "prolonged_power" :
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
         ( true_level >= 80 ) ? "severed_sagefish_head" :
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
  std::vector<std::string> racial_actions = get_racial_actions();

  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* conserve = get_action_priority_list( "conserve" );
  action_priority_list_t* burn = get_action_priority_list( "burn" );
  action_priority_list_t* movement = get_action_priority_list( "movement" );


  default_list -> add_action( this, "Counterspell", "if=target.debuff.casting.react", "Interrupt the boss when possible." );
  default_list -> add_action( this, "Time Warp", "if=time=0&buff.bloodlust.down" );
  default_list -> add_action( "call_action_list,name=burn,if=burn_phase|target.time_to_die<variable.average_burn_length|(cooldown.arcane_power.remains=0&cooldown.evocation.remains<=variable.average_burn_length&(buff.arcane_charge.stack=buff.arcane_charge.max_stack|(talent.charged_up.enabled&cooldown.charged_up.remains=0)))", "Start a burn phase when important cooldowns are available. Start with 4 arcane charges, unless there's a good reason not to. (charged up)" );
  default_list -> add_action( "call_action_list,name=conserve,if=!burn_phase" );
  default_list -> add_action( "call_action_list,name=movement" );

  burn -> add_action( "variable,name=total_burns,op=add,value=1,if=!burn_phase", "Increment our burn phase counter. Whenever we enter the `burn` actions without being in a burn phase, it means that we are about to start one." );
  burn -> add_action( "start_burn_phase,if=!burn_phase" );
  burn -> add_action( "stop_burn_phase,if=burn_phase&(prev_gcd.1.evocation|(equipped.gravity_spiral&cooldown.evocation.charges=0&prev_gcd.1.evocation))&target.time_to_die>variable.average_burn_length&burn_phase_duration>0", "End the burn phase when we just evocated." );
  burn -> add_talent( this, "Mirror Image" );
  burn -> add_talent( this, "Charged Up", "if=buff.arcane_charge.stack<=1&(!set_bonus.tier20_2pc|cooldown.presence_of_mind.remains>5)" );
  burn -> add_talent( this, "Nether Tempest", "if=(refreshable|!ticking)&buff.arcane_charge.stack=buff.arcane_charge.max_stack&buff.rune_of_power.down&buff.arcane_power.down" );
  burn -> add_action( this, "Time Warp", "if=buff.bloodlust.down&((buff.arcane_power.down&cooldown.arcane_power.remains=0)|(target.time_to_die<=buff.bloodlust.duration))" );
  burn -> add_action( "lights_judgment,if=buff.arcane_power.down" );
  burn -> add_talent( this, "Rune of Power", "if=!buff.arcane_power.up&(mana.pct>=50|cooldown.arcane_power.remains=0)&(buff.arcane_charge.stack=buff.arcane_charge.max_stack)" );
  burn -> add_action( this, "Arcane Power" );
  burn -> add_action( "use_items,if=buff.arcane_power.up|target.time_to_die<cooldown.arcane_power.remains" );
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] == "lights_judgment" || racial_actions[ i ] == "arcane_torrent" )
    {
      continue;
    }
    burn -> add_action( racial_actions[ i ] );
  }
  burn -> add_action( this, "Presence of Mind" );
  burn -> add_talent( this, "Arcane Orb", "if=buff.arcane_charge.stack=0|(active_enemies<3|(active_enemies<2&talent.resonance.enabled))" );
  burn -> add_action( this, "Arcane Blast", "if=buff.presence_of_mind.up&set_bonus.tier20_2pc&talent.overpowered.enabled&buff.arcane_power.up" );
  burn -> add_action( this, "Arcane Barrage", "if=(active_enemies>=3|(active_enemies>=2&talent.resonance.enabled))&(buff.arcane_charge.stack=buff.arcane_charge.max_stack)" );
  burn -> add_action( this, "Arcane Explosion", "if=active_enemies>=3|(active_enemies>=2&talent.resonance.enabled)" );
  burn -> add_action( this, "Arcane Missiles", "if=(buff.clearcasting.react&mana.pct<=95),chain=1" );
  burn -> add_action( this, "Arcane Blast" );
  burn -> add_action( "variable,name=average_burn_length,op=set,value=(variable.average_burn_length*variable.total_burns-variable.average_burn_length+(burn_phase_duration))%variable.total_burns", "Now that we're done burning, we can update the average_burn_length with the length of this burn." );
  burn -> add_action( this, "Evocation", "interrupt_if=mana.pct>=97|(buff.clearcasting.react&mana.pct>=92)" );
  burn -> add_action( this, "Arcane Barrage", "", "For the rare occasion where we go oom before evocation is back up. (Usually because we get very bad rng so the burn is cut very short)" );


  conserve -> add_talent( this, "Mirror Image" );
  conserve -> add_talent( this, "Charged Up", "if=buff.arcane_charge.stack=0" );
  conserve -> add_action( this, "Presence of Mind", "if=set_bonus.tier20_2pc&buff.arcane_charge.stack=0" );
  conserve -> add_talent( this, "Nether Tempest", "if=(refreshable|!ticking)&buff.arcane_charge.stack=buff.arcane_charge.max_stack&buff.rune_of_power.down&buff.arcane_power.down" );
  conserve -> add_action( this, "Arcane Orb", "if=buff.arcane_charge.stack<=2&(cooldown.arcane_power.remains>10|active_enemies<=2)" );
  conserve -> add_action( this, "Arcane Blast", "if=(buff.rule_of_threes.up|buff.rhonins_assaulting_armwraps.react)&buff.arcane_charge.stack>=3", "Arcane Blast shifts up in priority when running rule of threes. (Or Rhonin's)" );
  conserve -> add_talent( this, "Rune of Power", "if=buff.arcane_charge.stack=buff.arcane_charge.max_stack&(full_recharge_time<=execute_time|recharge_time<=cooldown.arcane_power.remains|target.time_to_die<=cooldown.arcane_power.remains)" );
  conserve -> add_action( this, "Arcane Missiles", "if=mana.pct<=95&buff.clearcasting.react,chain=1" );
  conserve -> add_action( this, "Arcane Blast", "if=equipped.mystic_kilt_of_the_rune_master&buff.arcane_charge.stack=0" );
  conserve -> add_action( this, "Arcane Barrage", "if=(buff.arcane_charge.stack=buff.arcane_charge.max_stack)&(mana.pct<=35|(talent.arcane_orb.enabled&cooldown.arcane_orb.remains<=gcd))", "During conserve, we still just want to continue not dropping charges as long as possible.So keep 'burning' as long as possible and then swap to a 4x AB->Abarr conserve rotation. This is mana neutral for RoT, mana negative with arcane familiar. Only use arcane barrage with less than 4 Arcane charges if we risk going too low on mana for our next burn" );
  conserve -> add_talent( this, "Supernova", "if=mana.pct<=95", "Supernova is barely worth casting, which is why it is so far down, only just above AB. " );
  conserve -> add_action( this, "Arcane Explosion", "if=active_enemies>=3&(mana.pct>=40|buff.arcane_charge.stack=3)", "Keep 'burning' in aoe situations until 40%. After that only cast AE with 3 Arcane charges, since it's almost equal mana cost to a 3 stack AB anyway. At that point AoE rotation will be AB x3 -> AE -> Abarr" );
  conserve -> add_action( "arcane_torrent");
  conserve -> add_action( this, "Arcane Blast" );
  conserve -> add_action( this, "Arcane Barrage" );


  movement -> add_talent( this, "Shimmer", "if=movement.distance>=10" );
  movement -> add_action( this, "Blink", "if=movement.distance>=10" );
  movement -> add_action( this, "Presence of Mind" );
  movement -> add_action( this, "Arcane Missiles" );
  movement -> add_talent( this, "Arcane Orb" );
  movement -> add_talent( this, "Supernova" );
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
  default_list -> add_action( this, "Time Warp", "if=(time=0&buff.bloodlust.down)|(buff.bloodlust.down&equipped.132410&(cooldown.combustion.remains<1|target.time_to_die<50))" );
  default_list -> add_talent( this, "Mirror Image", "if=buff.combustion.down" );
  default_list -> add_talent( this, "Rune of Power", "if=firestarter.active&action.rune_of_power.charges=2|cooldown.combustion.remains>40&buff.combustion.down&!talent.kindling.enabled|target.time_to_die<11|talent.kindling.enabled&(charges_fractional>1.8|time<40)&cooldown.combustion.remains>40",
    "Standard Talent RoP Logic." );
  default_list -> add_talent( this, "Rune of Power", "if=((buff.kaelthas_ultimate_ability.react|buff.pyroclasm.react)&(cooldown.combustion.remains>40|action.rune_of_power.charges>1))|(buff.erupting_infernal_core.up&(cooldown.combustion.remains>40|action.rune_of_power.charges>1))",
    "RoP use while using Legendary Items." );
  default_list -> add_action( mage_t::get_special_use_items( "horn_of_valor", true ) );
  default_list -> add_action( mage_t::get_special_use_items( "obelisk_of_the_void", true ) );
  default_list -> add_action( mage_t::get_special_use_items( "mrrgrias_favor" ) );
  default_list -> add_action( mage_t::get_special_use_items( "pharameres_forbidden_grimoire" ) );
  default_list -> add_action( mage_t::get_special_use_items( "kiljaedens_burning_wish" ) );

  default_list -> add_action( "call_action_list,name=combustion_phase,if=cooldown.combustion.remains<=action.rune_of_power.cast_time+(!talent.kindling.enabled*gcd)&(!talent.firestarter.enabled|!firestarter.active|active_enemies>=4|active_enemies>=2&talent.flame_patch.enabled)|buff.combustion.up" );
  default_list -> add_action( "call_action_list,name=rop_phase,if=buff.rune_of_power.up&buff.combustion.down" );
  default_list -> add_action( "call_action_list,name=standard_rotation" );

  combustion_phase -> add_action( "lights_judgment,if=buff.combustion.down" );
  combustion_phase -> add_talent( this, "Rune of Power", "if=buff.combustion.down" );
  combustion_phase -> add_action( "call_action_list,name=active_talents" );
  combustion_phase -> add_action( this, "Combustion" );
  combustion_phase -> add_action( "potion" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] == "lights_judgment" || racial_actions[ i ] == "arcane_torrent" )
      continue;  // Handled manually.

    combustion_phase -> add_action( racial_actions[i] );
  }

  combustion_phase -> add_action( "use_items" );
  combustion_phase -> add_action( mage_t::get_special_use_items( "obelisk_of_the_void" ) );
  combustion_phase -> add_action( this, "Flamestrike", "if=((talent.flame_patch.enabled&active_enemies>2)|active_enemies>6)&buff.hot_streak.react" );
  combustion_phase -> add_action( this, "Pyroblast", "if=(buff.kaelthas_ultimate_ability.react|buff.pyroclasm.react)&buff.combustion.remains>execute_time" );
  combustion_phase -> add_action( this, "Pyroblast", "if=buff.hot_streak.react" );
  combustion_phase -> add_action( this, "Fire Blast", "if=buff.heating_up.react" );
  combustion_phase -> add_talent( this, "Phoenix Flames" );
  combustion_phase -> add_action( this, "Scorch", "if=buff.combustion.remains>cast_time" );
  combustion_phase -> add_action( this, "Dragon's Breath", "if=!buff.hot_streak.react&action.fire_blast.charges<1" );
  combustion_phase -> add_action( this, "Scorch", "if=target.health.pct<=30&(equipped.132454|talent.searing_touch.enabled)");

  rop_phase        -> add_talent( this, "Rune of Power" );
  rop_phase        -> add_action( this, "Flamestrike", "if=((talent.flame_patch.enabled&active_enemies>1)|active_enemies>4)&buff.hot_streak.react" );
  rop_phase        -> add_action( this, "Pyroblast", "if=buff.hot_streak.react" );
  rop_phase        -> add_action( "call_action_list,name=active_talents" );
  rop_phase        -> add_action( this, "Pyroblast", "if=buff.kaelthas_ultimate_ability.react&execute_time<buff.kaelthas_ultimate_ability.remains&buff.rune_of_power.remains>cast_time" );
  rop_phase        -> add_action( this, "Pyroblast", "if=buff.pyroclasm.react&execute_time<buff.pyroclasm.remains&buff.rune_of_power.remains>cast_time" );
  rop_phase        -> add_action( this, "Fire Blast", "if=!prev_off_gcd.fire_blast&buff.heating_up.react&firestarter.active&charges_fractional>1.7" );
  rop_phase        -> add_talent( this, "Phoenix Flames", "if=!prev_gcd.1.phoenix_flames&charges_fractional>2.7&firestarter.active" );
  rop_phase        -> add_action( this, "Fire Blast", "if=!prev_off_gcd.fire_blast&!firestarter.active" );
  rop_phase        -> add_talent( this, "Phoenix Flames", "if=!prev_gcd.1.phoenix_flames" );
  rop_phase        -> add_action( this, "Scorch", "if=target.health.pct<=30&(equipped.132454|talent.searing_touch.enabled)" );
  rop_phase        -> add_action( this, "Dragon's Breath", "if=active_enemies>2" );
  rop_phase        -> add_action( this, "Flamestrike", "if=(talent.flame_patch.enabled&active_enemies>2)|active_enemies>5" );
  rop_phase        -> add_action( this, "Fireball" );

  active_talents   -> add_talent( this, "Blast Wave", "if=(buff.combustion.down)|(buff.combustion.up&action.fire_blast.charges<1)" );
  active_talents   -> add_talent( this, "Meteor", "if=cooldown.combustion.remains>40|(cooldown.combustion.remains>target.time_to_die)|buff.rune_of_power.up|firestarter.active" );
  active_talents   -> add_action( this, "Dragon's Breath", "if=equipped.132863|(talent.alexstraszas_fury.enabled&!buff.hot_streak.react)" );
  active_talents   -> add_talent( this, "Living Bomb", "if=active_enemies>1&buff.combustion.down" );

  standard    -> add_action( this, "Flamestrike", "if=((talent.flame_patch.enabled&active_enemies>1)|active_enemies>4)&buff.hot_streak.react" );
  standard    -> add_action( this, "Pyroblast", "if=buff.hot_streak.react&buff.hot_streak.remains<action.fireball.execute_time" );
  standard    -> add_action( this, "Pyroblast", "if=buff.hot_streak.react&firestarter.active&!talent.rune_of_power.enabled" );
  standard    -> add_talent( this, "Phoenix Flames", "if=charges_fractional>2.7&active_enemies>2" );
  standard    -> add_action( this, "Pyroblast", "if=buff.hot_streak.react&(!prev_gcd.1.pyroblast|action.pyroblast.in_flight)" );
  standard    -> add_action( this, "Pyroblast", "if=buff.hot_streak.react&target.health.pct<=30&equipped.132454" );
  standard    -> add_action( this, "Pyroblast", "if=buff.kaelthas_ultimate_ability.react&execute_time<buff.kaelthas_ultimate_ability.remains" );
  standard    -> add_action( this, "Pyroblast", "if=buff.pyroclasm.react&execute_time<buff.pyroclasm.remains" );
  standard    -> add_action( "call_action_list,name=active_talents" );
  standard    -> add_action( this, "Fire Blast", "if=!talent.kindling.enabled&buff.heating_up.react&(!talent.rune_of_power.enabled|charges_fractional>1.4|cooldown.combustion.remains<40)&(3-charges_fractional)*(12*spell_haste)<cooldown.combustion.remains+3|target.time_to_die<4" );
  standard    -> add_action( this, "Fire Blast", "if=talent.kindling.enabled&buff.heating_up.react&(!talent.rune_of_power.enabled|charges_fractional>1.5|cooldown.combustion.remains<40)&(3-charges_fractional)*(18*spell_haste)<cooldown.combustion.remains+3|target.time_to_die<4" );
  standard    -> add_talent( this, "Phoenix Flames", "if=(buff.combustion.up|buff.rune_of_power.up|buff.incanters_flow.stack>3|talent.mirror_image.enabled)&(4-charges_fractional)*13<cooldown.combustion.remains+5|target.time_to_die<10" );
  standard    -> add_talent( this, "Phoenix Flames", "if=(buff.combustion.up|buff.rune_of_power.up)&(4-charges_fractional)*30<cooldown.combustion.remains+5" );
  standard    -> add_talent( this, "Phoenix Flames", "if=charges_fractional>2.5&cooldown.combustion.remains>23" );
  standard    -> add_action( this, "Scorch", "if=target.health.pct<=30&(equipped.132454|talent.searing_touch.enabled)" );
  standard    -> add_action( this, "Fireball" );
  standard    -> add_action( this, "Scorch" );


}

// Frost Mage Action List ==============================================================================================================

void mage_t::apl_frost()
{
  std::vector<std::string> racial_actions = get_racial_actions();

  action_priority_list_t* default_list = get_action_priority_list( "default"   );
  action_priority_list_t* single       = get_action_priority_list( "single"    );
  action_priority_list_t* aoe          = get_action_priority_list( "aoe"       );
  action_priority_list_t* cooldowns    = get_action_priority_list( "cooldowns" );
  action_priority_list_t* movement     = get_action_priority_list( "movement"  );

  default_list -> add_action( this, "Counterspell" );
  default_list -> add_action( this, "Ice Lance", "if=prev_gcd.1.flurry&!buff.fingers_of_frost.react",
    "If the mage has FoF after casting instant Flurry, we can delay the Ice Lance and use other high priority action, if available." );
  default_list -> add_action( this, "Time Warp", "if=buff.bloodlust.down&(buff.exhaustion.down|equipped.shard_of_the_exodar)&(prev_gcd.1.icy_veins|target.time_to_die<50)" );
  default_list -> add_action( "call_action_list,name=cooldowns" );
  default_list -> add_action( "call_action_list,name=aoe,if=active_enemies>3&talent.freezing_rain.enabled|active_enemies>4",
    "The target threshold isn't exact. Between 3-5 targets, the differences between the ST and AoE action lists are rather small. "
    "However, Freezing Rain prefers using AoE action list sooner as it benefits greatly from the high priority Blizzard action." );
  default_list -> add_action( "call_action_list,name=single" );

  single -> add_talent( this, "Ice Nova", "if=cooldown.ice_nova.ready&debuff.winters_chill.up",
    "In some situations, you can shatter Ice Nova even after already casting Flurry and Ice Lance. "
    "Otherwise this action is used when the mage has FoF after casting Flurry, see above." );
  single -> add_action( this, "Flurry", "if=!talent.glacial_spike.enabled&(prev_gcd.1.ebonbolt|buff.brain_freeze.react&prev_gcd.1.frostbolt)",
    "Without GS, the mage just tries to shatter as many Frostbolts and Ebonbolts as possible. Forcing shatter on Frostbolt is still a small gain, "
    "so is not caring about FoF. Ice Lance is too weak to warrant delaying Brain Freeze Flurry." );
  single -> add_action( this, "Flurry", "if=talent.glacial_spike.enabled&buff.brain_freeze.react&(prev_gcd.1.frostbolt&buff.icicles.stack<4|prev_gcd.1.glacial_spike|prev_gcd.1.ebonbolt)",
    "With GS, the mage only shatters Frostbolt that would put them at 1-3 Icicle stacks, Ebonbolt if it would waste Brain Freeze charge (i.e. when the mage "
    "starts casting Ebonbolt with Brain Freeze active) and of course Glacial Spike. Difference between shattering Frostbolt with 1-3 Icicles and 1-4 Icicles is "
    "small, but 1-3 tends to be better in more situations (the higher GS damage is, the more it leans towards 1-3)." );
  single -> add_action( this, "Frozen Orb" );
  single -> add_action( this, "Blizzard", "if=active_enemies>2|active_enemies>1&cast_time=0&buff.fingers_of_frost.react<2",
    "With Freezing Rain and at least 2 targets, Blizzard needs to be used with higher priority to make sure you can fit both instant Blizzards "
    "into a single Freezing Rain. Starting with three targets, Blizzard leaves the low priority filler role and is used on cooldown (and just making "
    "sure not to waste Brain Freeze charges) with or without Freezing Rain." );
  single -> add_action( this, "Ice Lance", "if=buff.fingers_of_frost.react",
    "Trying to pool charges of FoF for anything isn't worth it. Use them as they come." );
  single -> add_talent( this, "Ray of Frost", "if=!action.frozen_orb.in_flight&ground_aoe.frozen_orb.remains=0",
    "Ray of Frost is used after all Fingers of Frost charges have been used and there isn't active Frozen Orb that could generate more. "
    "This is only a small gain, as Ray of Frost isn't too impactful." );
  single -> add_talent( this, "Comet Storm" );
  single -> add_talent( this, "Ebonbolt", "if=!talent.glacial_spike.enabled|buff.icicles.stack=5&!buff.brain_freeze.react",
    "Without GS, Ebonbolt is used on cooldown. With GS, Ebonbolt is only used to fill in the blank spots when fishing for a Brain Freeze proc, i.e. "
    "the mage reaches 5 Icicles but still doesn't have a Brain Freeze proc." );
  single -> add_talent( this, "Glacial Spike", "if=buff.brain_freeze.react|prev_gcd.1.ebonbolt|active_enemies>1&talent.splitting_ice.enabled",
    "Glacial Spike is used when there's a Brain Freeze proc active (i.e. only when it can be shattered). This is a small to medium gain "
    "in most situations. Low mastery leans towards using it when available. When using Splitting Ice and having another target nearby, "
    "it's slightly better to use GS when available, as the second target doesn't benefit from shattering the main target." );
  single -> add_action( this, "Blizzard", "if=cast_time=0|active_enemies>1|buff.zannesu_journey.stack=5&buff.zannesu_journey.remains>cast_time",
    "Blizzard is used as low priority filler against 2 targets. When using Freezing Rain, it's a medium gain to use the instant Blizzard even "
    "against a single target, especially with low mastery.");
  single -> add_talent( this, "Ice Nova" );
  single -> add_action( this, "Frostbolt" );
  single -> add_action( "call_action_list,name=movement" );
  single -> add_action( this, "Ice Lance" );

  aoe -> add_action( this, "Frozen Orb", "",
    "With Freezing Rain, it's better to prioritize using Frozen Orb when both FO and Blizzard are off cooldown. "
    "Without Freezing Rain, the converse is true although the difference is miniscule until very high target counts." );
  aoe -> add_action( this, "Blizzard" );
  aoe -> add_talent( this, "Comet Storm" );
  aoe -> add_talent( this, "Ice Nova" );
  aoe -> add_action( this, "Flurry", "if=prev_gcd.1.ebonbolt|buff.brain_freeze.react&(prev_gcd.1.frostbolt&(buff.icicles.stack<4|!talent.glacial_spike.enabled)|prev_gcd.1.glacial_spike)",
    "Simplified Flurry conditions from the ST action list. Since the mage is generating far less Brain Freeze charges, the exact "
    "condition here isn't all that important." );
  aoe -> add_action( this, "Ice Lance", "if=buff.fingers_of_frost.react" );
  aoe -> add_talent( this, "Ray of Frost", "",
    "The mage will generally be generating a lot of FoF charges when using the AoE action list. Trying to delay Ray of Frost "
    "until there are no FoF charges and no active Frozen Orbs would lead to it not being used at all." );
  aoe -> add_talent( this, "Ebonbolt" );
  aoe -> add_talent( this, "Glacial Spike" );
  aoe -> add_action( this, "Cone of Cold", "",
    "Using Cone of Cold is mostly DPS neutral with the AoE target thresholds. It only becomes decent gain with roughly 7 or more targets." );
  aoe -> add_action( this, "Frostbolt" );
  aoe -> add_action( "call_action_list,name=movement" );
  aoe -> add_action( this, "Ice Lance" );

  cooldowns -> add_action( this, "Icy Veins" );
  cooldowns -> add_talent( this, "Mirror Image" );
  cooldowns -> add_talent( this, "Rune of Power", "if=time_to_die>10+cast_time&time_to_die<25" );
  cooldowns -> add_talent( this, "Rune of Power",
    "if=active_enemies=1&talent.glacial_spike.enabled&buff.icicles.stack=5&("
    "!talent.ebonbolt.enabled&buff.brain_freeze.react"
    "|talent.ebonbolt.enabled&(full_recharge_time<=cooldown.ebonbolt.remains&buff.brain_freeze.react"
    "|cooldown.ebonbolt.remains<cast_time&!buff.brain_freeze.react))",
    "With Glacial Spike, Rune of Power should be used right before the Glacial Spike combo (i.e. with 5 Icicles and a Brain Freeze). "
    "When Ebonbolt is off cooldown, Rune of Power can also be used just with 5 Icicles." );
  cooldowns -> add_talent( this, "Rune of Power",
    "if=active_enemies=1&!talent.glacial_spike.enabled&("
    "prev_gcd.1.frozen_orb|talent.ebonbolt.enabled&cooldown.ebonbolt.remains<cast_time"
    "|talent.comet_storm.enabled&cooldown.comet_storm.remains<cast_time"
    "|talent.ray_of_frost.enabled&cooldown.ray_of_frost.remains<cast_time|charges_fractional>1.9)",
    "Without Glacial Spike, Rune of Power should be used before any bigger cooldown (Frozen Orb, Ebonbolt, Comet Storm, Ray of Frost) or "
    "when Rune of Power is about to reach 2 charges." );
  cooldowns -> add_talent( this, "Rune of Power", "if=active_enemies>1&prev_gcd.1.frozen_orb",
    "With 2 or more targets, use Rune of Power exclusively with Frozen Orb. This is the case even with Glacial Spike." );
  cooldowns -> add_action( "potion,if=prev_gcd.1.icy_veins|target.time_to_die<70" );
  cooldowns -> add_action( "use_items" );
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] == "arcane_torrent" )
      continue;

    cooldowns -> add_action( racial_actions[ i ] );
  }

  movement -> add_action( this, "Blink", "if=movement.distance>10" );
  movement -> add_talent( this, "Ice Floes", "if=buff.ice_floes.down" );
}

// Default Action List ========================================================

void mage_t::apl_default()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );

  default_list -> add_action( "Frostbolt" );
}


// mage_t::mana_regen_per_second ==============================================

double mage_t::resource_regen_per_second( resource_e r ) const
{
  double reg = player_t::resource_regen_per_second( r );

  if ( r == RESOURCE_MANA )
  {
    if ( spec.savant -> ok() )
    {
      reg *= 1.0 + cache.mastery() * spec.savant -> effectN( 1 ).mastery_value();
    }
  }

  return reg;
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

  double current_mana = resources.current[ rt ];
  double current_mana_max = resources.max[ rt ];
  double mana_percent = current_mana / current_mana_max;

  player_t::recalculate_resource_max( rt );

  if ( spec.savant -> ok() )
  {
    resources.max[ rt ] *= 1.0 + cache.mastery() * spec.savant -> effectN( 1 ).mastery_value();
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
    resources.max[ rt ] *= 1.0 + buffs.arcane_familiar -> check_value();
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

  m *= 1.0 + buffs.inferno -> check_value();
  m *= 1.0 + buffs.frozen_mass -> check_value();

  return m;
}

// mage_t::composite_player_pet_damage_multiplier ============================

double mage_t::composite_player_pet_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s );

  m *= 1.0 + spec.arcane_mage -> effectN( 3 ).percent();
  m *= 1.0 + spec.fire_mage -> effectN( 3 ).percent();
  m *= 1.0 + spec.frost_mage -> effectN( 3 ).percent();

  m *= 1.0 + buffs.bone_chilling -> check_stack_value();
  m *= 1.0 + buffs.incanters_flow -> check_stack_value();
  m *= 1.0 + buffs.rune_of_power -> check_value();

  return m;
}

// mage_t::composite_player_multiplier =======================================

double mage_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  m *= 1.0 + buffs.expanding_mind -> check_value();

  return m;
}

// mage_t::composite_mastery_rating =============================================

double mage_t::composite_mastery_rating() const
{
  double m = player_t::composite_mastery_rating();

  if ( buffs.combustion -> check() )
  {
    m += composite_spell_crit_rating() * buffs.combustion -> data().effectN( 3 ).percent();
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

  if ( spec.critical_mass -> ok() )
  {
    c += spec.critical_mass -> effectN( 1 ).percent();
  }

  return c;
}

// mage_t::composite_spell_haste ==============================================

double mage_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  h /= 1.0 + buffs.icy_veins -> check_value();
  h /= 1.0 + buffs.streaking -> check_value();
  h /= 1.0 + buffs.quick_thinker -> check_value();
  h /= 1.0 + buffs.sephuzs_secret -> check_value();

  if ( buffs.sephuzs_secret -> default_chance != 0.0 )
  {
    h /= 1.0 + buffs.sephuzs_secret -> data().driver() -> effectN( 3 ).percent();
  }

  return h;
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
  event_t::cancel( time_anomaly_tick_event );

  if ( spec.savant -> ok() )
  {
    recalculate_resource_max( RESOURCE_MANA );
  }

  last_bomb_target = nullptr;
  last_frostbolt_target = nullptr;
  ground_aoe_expiration.clear();
  burn_phase.reset();

  range::for_each( proc_source_list, std::mem_fn( &proc_source_t::reset ) );
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

  if ( buffs.sephuzs_secret -> check() )
  {
    tmm = std::max( buffs.sephuzs_secret -> data().effectN( 1 ).percent(), tmm );
  }

  return tmm;
}

// mage_t::passive_movement_modifier ====================================

double mage_t::passive_movement_modifier() const
{
  double pmm = player_t::passive_movement_modifier();

  if ( buffs.sephuzs_secret -> default_chance != 0.0 )
  {
    pmm += buffs.sephuzs_secret -> data().driver() -> effectN( 2 ).percent();
  }

  pmm += buffs.chrono_shift -> check_value();
  pmm += buffs.frenetic_speed -> check_value();

  return pmm;
}

// mage_t::arise ============================================================

void mage_t::arise()
{
  player_t::arise();

  if ( talents.incanters_flow -> ok() )
    buffs.incanters_flow -> trigger();

  if ( blessing_of_wisdom_count > 0 )
  {
    buffs.greater_blessing_of_widsom -> trigger();
  }
  if ( spec.ignite -> ok()  )
  {
    timespan_t first_spread = timespan_t::from_seconds( rng().real() * 2.0 );
    ignite_spread_event = make_event<events::ignite_spread_event_t>( *sim, *this, first_spread );
  }
  if ( talents.time_anomaly -> ok() )
  {
    timespan_t first_tick = rng().real() * talents.time_anomaly -> effectN( 1 ).period();
    time_anomaly_tick_event = make_event<events::time_anomaly_tick_event_t>( *sim, *this, first_tick );
  }
}

void mage_t::combat_begin()
{
  player_t::combat_begin();

  if ( specialization() == MAGE_ARCANE )
  {
    uptime.burn_phase -> update( false, sim -> current_time() );
    uptime.conserve_phase -> update( true, sim -> current_time() );
  }
}

void mage_t::combat_end()
{
  player_t::combat_end();

  if ( specialization() == MAGE_ARCANE )
  {
    uptime.burn_phase -> update( false, sim -> current_time() );
    uptime.conserve_phase -> update( false, sim -> current_time() );
  }
}

/**
 * Mage specific action expressions
 *
 * Use this function for expressions which are bound to some action property (eg. target, cast_time, etc.) and not just
 * to the player itself. For those use the normal mage_t::create_expression override.
 */
expr_t* mage_t::create_action_expression( action_t& action, const std::string& name )
{
  struct mage_action_expr_t : public expr_t
  {
    action_t& action;
    mage_t& mage;
    mage_action_expr_t( mage_t& mage, action_t& action, const std::string& n ) :
      expr_t( n ), action( action ), mage( mage )
    { }
  };

  std::vector<std::string> splits = util::string_split( name, "." );

  // Firestarter expressions ==================================================
  if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "firestarter" ) )
  {
    enum firestarter_expr_type_e
    {
      FIRESTARTER_ACTIVE,
      FIRESTARTER_REMAINS
    };

    struct firestarter_expr_t : public mage_action_expr_t
    {
      firestarter_expr_type_e type;

      firestarter_expr_t( mage_t& mage, action_t& action, const std::string& name, firestarter_expr_type_e type ) :
        mage_action_expr_t( mage, action, name ), type( type )
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
          remains = action.target -> time_to_percent( mage.talents.firestarter -> effectN( 1 ).base_value() );
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

    if ( util::str_compare_ci( splits[ 1 ], "active" ) )
    {
      return new firestarter_expr_t( *this, action, name, FIRESTARTER_ACTIVE );
    }
    else if ( util::str_compare_ci( splits[ 1 ], "remains" ) )
    {
      return new firestarter_expr_t( *this, action, name, FIRESTARTER_REMAINS );
    }
    else
    {
      throw std::invalid_argument( fmt::format( "Unknown firestarer operation '{}'", splits[ 1 ] ) );
    }
  }

  return player_t::create_action_expression( action, name );
}

// mage_t::create_expression ================================================

expr_t* mage_t::create_expression( const std::string& name_str )
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

      virtual double evaluate() override
      { return mage.icicle_event != nullptr; }
    };

    return new sicicles_expr_t( *this );
  }

  std::vector<std::string> splits = util::string_split( name_str, "." );

  // Ground AoE expressions ===================================================
  if ( splits.size() == 3 && util::str_compare_ci( splits[ 0 ], "ground_aoe" ) )
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

      virtual double evaluate() override
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

    if ( util::str_compare_ci( splits[ 2 ], "remains" ) )
    {
      return new ground_aoe_expr_t( *this, name_str, splits[ 1 ] );
    }
    else
    {
      sim -> errorf( "Player %s ground_aoe expression: unknown operation '%s'", name(), splits[ 2 ].c_str() );
    }
  }

  return player_t::create_expression( name_str );
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

// mage_t::get_icicle =======================================================

action_t* mage_t::get_icicle()
{
  if ( icicles.empty() )
    return nullptr;

  // All Icicles created before the treshold timed out.
  timespan_t threshold = sim -> current_time() - buffs.icicles -> buff_duration;

  // Find first icicle which did not time out
  auto idx = range::find_if( icicles, [ threshold ] ( const icicle_tuple_t& t ) { return t.timestamp > threshold; } );

  // Remove all timed out icicles
  icicles.erase( icicles.begin(), idx );

  if ( ! icicles.empty() )
  {
    action_t* icicle_action = icicles.front().icicle_action;
    icicles.erase( icicles.begin() );
    return icicle_action;
  }

  return nullptr;
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
    action_t* icicle_action = get_icicle();
    if ( ! icicle_action )
      return;

    assert( icicle_target );
    icicle_event = make_event<events::icicle_event_t>( *sim, *this, icicle_action, icicle_target, true );

    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s icicle use on %s%s, total=%u",
                               name(), icicle_target -> name(),
                               chain ? " (chained)" : "",
                               as<unsigned>( icicles.size() ) );
    }
  }
  else if ( ! chain )
  {
    action_t* icicle_action = get_icicle();
    if ( ! icicle_action )
      return;

    icicle_action -> set_target( icicle_target );
    icicle_action -> execute();

    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s icicle use on %s%s, total=%u",
                               name(), icicle_target -> name(),
                               chain ? " (chained)" : "",
                               as<unsigned>( icicles.size() ) );
    }
  }
}

void mage_t::trigger_evocation( timespan_t duration_override, bool hasted )
{
  double mana_regen_multiplier = 1.0 + buffs.evocation -> default_value;

  timespan_t duration = duration_override;
  if ( duration <= timespan_t::zero() )
  {
    duration = buffs.evocation -> buff_duration;
  }

  if ( hasted )
  {
    mana_regen_multiplier /= cache.spell_speed();
    duration *= cache.spell_speed();
  }

  buffs.evocation -> trigger( 1, mana_regen_multiplier, -1.0, duration );
}

void mage_t::trigger_arcane_charge( int stacks )
{
  buff_t* ac = buffs.arcane_charge;

  int before = ac -> check();
  ac -> trigger( stacks );
  int after = ac -> check();

  if ( talents.rule_of_threes -> ok() && before < 3 && after >= 3 )
  {
    buffs.rule_of_threes -> trigger();
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
  { }

  void html_customsection_cd_waste( report::sc_html_stream& os )
  {
    if ( p.cooldown_waste_data_list.empty() )
      return;

    os << "<div class=\"player-section custom_section\">\n"
       << "<h3 class=\"toggle open\">Cooldown waste</h3>\n"
       << "<div class=\"toggle-content\">\n";

    os << "<table class=\"sc\" style=\"margin-top: 5px;\">\n"
       << "<tr>\n"
       << "<th></th>\n"
       << "<th colspan=\"3\">Seconds per Execute</th>\n"
       << "<th colspan=\"3\">Seconds per Iteration</th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th>Ability</th>\n"
       << "<th>Average</th>\n"
       << "<th>Minimum</th>\n"
       << "<th>Maximum</th>\n"
       << "<th>Average</th>\n"
       << "<th>Minimum</th>\n"
       << "<th>Maximum</th>\n"
       << "</tr>\n";

    size_t row = 0;
    for ( const cooldown_waste_data_t* data : p.cooldown_waste_data_list )
    {
      if ( ! data -> active() )
        continue;

      std::string name = data -> cd -> name_str;
      if ( action_t* a = p.find_action( name ) )
      {
        name = report::action_decorator_t( a ).decorate();
      }

      std::string row_class;
      if ( ++row & 1 )
        row_class = " class=\"odd\"";

      os.printf( "<tr%s>", row_class.c_str() );
      os << "<td class=\"left\">" << name << "</td>";
      os.printf( "<td class=\"right\">%.3f</td>", data -> normal.mean() );
      os.printf( "<td class=\"right\">%.3f</td>", data -> normal.min() );
      os.printf( "<td class=\"right\">%.3f</td>", data -> normal.max() );
      os.printf( "<td class=\"right\">%.3f</td>", data -> cumulative.mean() );
      os.printf( "<td class=\"right\">%.3f</td>", data -> cumulative.min() );
      os.printf( "<td class=\"right\">%.3f</td>", data -> cumulative.max() );
      os << "</tr>\n";
    }

    os << "</table>\n";

    os << "</div>\n"
       << "</div>\n";
  }

  void html_customsection_burn_phases( report::sc_html_stream& os )
  {
    os << "<div class=\"player-section custom_section\">\n"
       << "<h3 class=\"toggle open\">Burn Phases</h3>\n"
       << "<div class=\"toggle-content\">\n";

    os << "<p>Burn phase duration tracks the amount of time spent in each burn phase. This is defined as the time between a "
       << "start_burn_phase and stop_burn_phase action being executed. Note that \"execute\" burn phases, i.e., the "
       << "final burn of a fight, is also included.</p>\n";

    os << "<div style=\"display: flex;\">\n"
       << "<table class=\"sc\" style=\"margin-top: 5px;\">\n"
       << "<thead>\n"
       << "<tr>\n"
       << "<th>Burn Phase Duration</th>\n"
       << "</tr>\n"
       << "<tbody>\n";

    os.printf("<tr><td class=\"left\">Count</td><td>%d</td></tr>\n", p.sample_data.burn_duration_history -> count() );
    os.printf("<tr><td class=\"left\">Minimum</td><td>%.3f</td></tr>\n", p.sample_data.burn_duration_history -> min() );
    os.printf("<tr><td class=\"left\">5<sup>th</sup> percentile</td><td>%.3f</td></tr>\n", p.sample_data.burn_duration_history -> percentile( 0.05 ) );
    os.printf("<tr><td class=\"left\">Mean</td><td>%.3f</td></tr>\n", p.sample_data.burn_duration_history -> mean() );
    os.printf("<tr><td class=\"left\">95<sup>th</sup> percentile</td><td>%.3f</td></tr>\n", p.sample_data.burn_duration_history -> percentile( 0.95 ) );
    os.printf("<tr><td class=\"left\">Max</td><td>%.3f</td></tr>\n", p.sample_data.burn_duration_history -> max() );
    os.printf("<tr><td class=\"left\">Variance</td><td>%.3f</td></tr>\n", p.sample_data.burn_duration_history -> variance );
    os.printf("<tr><td class=\"left\">Mean Variance</td><td>%.3f</td></tr>\n", p.sample_data.burn_duration_history -> mean_variance );
    os.printf("<tr><td class=\"left\">Mean Std. Dev</td><td>%.3f</td></tr>\n", p.sample_data.burn_duration_history -> mean_std_dev );

    os << "</tbody>\n"
       << "</table>\n";

    highchart::histogram_chart_t burn_duration_history_chart( highchart::build_id( p, "burn_duration_history" ), *p.sim );
    if ( chart::generate_distribution(
        burn_duration_history_chart, &p, p.sample_data.burn_duration_history -> distribution, "Burn Duration",
        p.sample_data.burn_duration_history -> mean(),
        p.sample_data.burn_duration_history -> min(),
        p.sample_data.burn_duration_history -> max() ) )
    {
      burn_duration_history_chart.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      burn_duration_history_chart.set( "chart.width", "575" );
      os << burn_duration_history_chart.to_target_div();
      p.sim -> add_chart_data( burn_duration_history_chart );
    }

    os << "</div>\n";

    os << "<p>Mana at burn start is the mana level recorded (in percentage of total mana) when a start_burn_phase command is executed.</p>\n";

    os << "<table class=\"sc\">\n"
       << "<thead>\n"
       << "<tr>\n"
       << "<th>Mana at Burn Start</th>\n"
       << "</tr>\n"
       << "<tbody>\n";

    os.printf("<tr><td class=\"left\">Count</td><td>%d</td></tr>\n", p.sample_data.burn_initial_mana -> count() );
    os.printf("<tr><td class=\"left\">Minimum</td><td>%.3f</td></tr>\n", p.sample_data.burn_initial_mana -> min() );
    os.printf("<tr><td class=\"left\">5<sup>th</sup> percentile</td><td>%.3f</td></tr>\n", p.sample_data.burn_initial_mana -> percentile( 0.05 ) );
    os.printf("<tr><td class=\"left\">Mean</td><td>%.3f</td></tr>\n", p.sample_data.burn_initial_mana -> mean() );
    os.printf("<tr><td class=\"left\">95<sup>th</sup> percentile</td><td>%.3f</td></tr>\n", p.sample_data.burn_initial_mana -> percentile( 0.95 ) );
    os.printf("<tr><td class=\"left\">Max</td><td>%.3f</td></tr>\n", p.sample_data.burn_initial_mana -> max() );
    os.printf("<tr><td class=\"left\">Variance</td><td>%.3f</td></tr>\n", p.sample_data.burn_initial_mana -> variance );
    os.printf("<tr><td class=\"left\">Mean Variance</td><td>%.3f</td></tr>\n", p.sample_data.burn_initial_mana -> mean_variance );
    os.printf("<tr><td class=\"left\">Mean Std. Dev</td><td>%.3f</td></tr>\n", p.sample_data.burn_initial_mana -> mean_std_dev );

    os << "</tbody>\n"
       << "</table>\n";

    os << "</div>\n"
       << "</div>\n";
  }

  void html_customsection_icy_veins( report::sc_html_stream& os )
  {
    os << "<div class=\"player-section custom_section\">\n"
       << "<h3 class=\"toggle open\">Icy Veins</h3>\n"
       << "<div class=\"toggle-content\">\n";

    int num_buckets = as<int>( p.sample_data.icy_veins_duration -> max() - p.sample_data.icy_veins_duration -> min() ) + 1;

    highchart::histogram_chart_t chart( highchart::build_id( p, "iv_dist" ), *p.sim );
    p.sample_data.icy_veins_duration -> create_histogram( num_buckets );

    if ( chart::generate_distribution(
             chart, &p, p.sample_data.icy_veins_duration -> distribution, "Icy Veins Duration",
             p.sample_data.icy_veins_duration -> mean(), p.sample_data.icy_veins_duration -> min(),
             p.sample_data.icy_veins_duration -> max() ) )
    {
      chart.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart.set( "chart.width", std::to_string( 80 + num_buckets * 13 ) );
      os << chart.to_target_div();
      p.sim -> add_chart_data( chart );
    }

    os << "</div>\n"
       << "</div>\n";
  }

  void html_customsection_shatter( report::sc_html_stream& os )
  {
    if ( p.proc_source_list.empty() )
      return;

    os << "<div class=\"player-section custom_section\">\n"
       << "<h3 class=\"toggle open\">Shatter</h3>\n"
       << "<div class=\"toggle-content\">\n";

    os << "<table class=\"sc\" style=\"margin-top: 5px;\">\n"
       << "<tr>\n"
       << "<th>Ability</th>\n"
       << "<th>Winter's Chill (utilization)</th>\n"
       << "<th>Fingers of Frost</th>\n"
       << "<th>Other effects</th>\n"
       << "</tr>\n";

    double bff = p.procs.brain_freeze_flurry -> count.pretty_mean();

    size_t row = 0;
    for ( const proc_source_t* data : p.proc_source_list )
    {
      if ( ! data -> active() )
        continue;

      const std::vector<std::string> splits = util::string_split( data -> name_str, "/" );
      if ( splits.size() != 2 || splits[ 0 ] != "Shatter" )
        continue;

      std::string name = splits[ 1 ];
      if ( action_t* a = p.find_action( name ) )
      {
        name = report::action_decorator_t( a ).decorate();
      }

      std::string row_class;
      if ( ++row & 1 )
        row_class = " class=\"odd\"";

      os.printf( "<tr%s>", row_class.c_str() );

      auto format_cell = [ bff, &os ] ( double mean, bool wc_util )
      {
        std::string format_str;
        format_str += "<td class=\"right\">";
        if ( mean > 0.0 )
          format_str += "%.1f";
        if ( mean > 0.0 && wc_util )
          format_str += " (%.1f%%)";
        format_str += "</td>";

        os.printf( format_str.c_str(), mean, bff ? 100.0 * mean / bff : 0.0 );
      };

      assert( data -> procs.size() == actions::mage_spell_t::FROZEN_MAX );

      os << "<td class=\"left\">" << name << "</td>";
      format_cell( data -> get( actions::mage_spell_t::FROZEN_WINTERS_CHILL ).count.pretty_mean(), true );
      format_cell( data -> get( actions::mage_spell_t::FROZEN_FINGERS_OF_FROST ).count.pretty_mean(), false );
      format_cell( data -> get( actions::mage_spell_t::FROZEN_ROOT ).count.pretty_mean(), false );
      os << "</tr>\n";
    }

    os << "</table>\n";

    os << "</div>\n"
       << "</div>\n";
  }

  virtual void html_customsection( report::sc_html_stream& os ) override
  {
    if ( p.sim -> report_details == 0 )
      return;

    html_customsection_cd_waste( os );

    switch ( p.specialization() )
    {
      case MAGE_ARCANE:
        html_customsection_burn_phases( os );
        break;

      case MAGE_FIRE:
        break;

      case MAGE_FROST:
        html_customsection_shatter( os );

        if ( p.talents.thermal_void -> ok() )
        {
          html_customsection_icy_veins( os );
        }

        break;

      default:
        break;
    }
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
struct sephuzs_secret_t : public class_buff_cb_t<mage_t, buff_t>
{
  sephuzs_secret_t(): super( MAGE, "sephuzs_secret" )
  { }

  virtual buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<mage_t*>( e.player ) -> buffs.sephuzs_secret; }

  buff_t* creator( const special_effect_t& e ) const override
  {
    auto buff = make_buff( e.player, buff_name, e.trigger() );
    buff -> set_cooldown( e.player -> find_spell( 226262 ) -> duration() )
         -> set_default_value( e.trigger() -> effectN( 2 ).percent() )
         -> add_invalidate( CACHE_RUN_SPEED )
         -> add_invalidate( CACHE_HASTE );
    return buff;
  }
};

struct shard_of_the_exodar_t : public scoped_actor_callback_t<mage_t>
{
  shard_of_the_exodar_t() : super( MAGE )
  { }

  virtual void manipulate( mage_t* actor, const special_effect_t& /* e */ ) override
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

  virtual void manipulate( arcane_barrage_t* action, const special_effect_t& e ) override
  { action -> mystic_kilt_of_the_rune_master_regen = e.driver() -> effectN( 1 ).percent(); }
};

struct rhonins_assaulting_armwraps_t : public class_buff_cb_t<mage_t>
{
  rhonins_assaulting_armwraps_t() : super( MAGE_ARCANE, "rhonins_assaulting_armwraps" )
  { }

  virtual buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<mage_t*>( e.player ) -> buffs.rhonins_assaulting_armwraps; }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.trigger() )
      -> set_chance( e.driver() -> effectN( 1 ).percent() );
  }
};

struct cord_of_infinity_t : public scoped_action_callback_t<arcane_missiles_tick_t>
{
  cord_of_infinity_t() : super( MAGE_ARCANE, "arcane_missiles_tick" )
  { }

  virtual void manipulate( arcane_missiles_tick_t* action, const special_effect_t& e ) override
  { action -> base_multiplier *= 1.0 + e.driver() -> effectN( 1 ).percent(); }
};

struct gravity_spiral_t : public scoped_actor_callback_t<mage_t>
{
  gravity_spiral_t() : super( MAGE_ARCANE )
  { }

  virtual void manipulate( mage_t* actor, const special_effect_t& e ) override
  { actor -> cooldowns.evocation -> charges += as<int>( e.driver() -> effectN( 1 ).base_value() ); }
};

struct mantle_of_the_first_kirin_tor_t : public scoped_action_callback_t<arcane_barrage_t>
{
  mantle_of_the_first_kirin_tor_t() : super( MAGE_ARCANE, "arcane_barrage" )
  { }

  virtual void manipulate( arcane_barrage_t* action, const special_effect_t& e ) override
  { action -> mantle_of_the_first_kirin_tor_chance = e.driver() -> effectN( 1 ).percent(); }
};

// Fire Legendary Items
struct koralons_burning_touch_t : public scoped_action_callback_t<scorch_t>
{
  koralons_burning_touch_t() : super( MAGE_FIRE, "scorch" )
  { }

  virtual void manipulate( scorch_t* action, const special_effect_t& e ) override
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

  virtual void manipulate( dragons_breath_t* action, const special_effect_t& e ) override
  {
    action -> radius += e.driver() -> effectN( 1 ).base_value();
    action -> base_multiplier *= 1.0 + e.driver() -> effectN( 2 ).percent();
  }
};


struct marquee_bindings_of_the_sun_king_t : public class_buff_cb_t<mage_t>
{
  marquee_bindings_of_the_sun_king_t() : super( MAGE_FIRE, "kaelthas_ultimate_ability" )
  { }

  virtual buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<mage_t*>( e.player ) -> buffs.kaelthas_ultimate_ability; }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.player -> find_spell( 209455 ) )
      -> set_chance( e.driver() -> effectN( 1 ).percent() )
      -> set_default_value( e.player -> find_spell( 209455 ) -> effectN( 1 ).percent() );
  }
};

struct pyrotex_ignition_cloth_t : public scoped_action_callback_t<phoenix_flames_t>
{
  pyrotex_ignition_cloth_t() : super( MAGE_FIRE, "phoenix_flames" )
  { }

  virtual void manipulate( phoenix_flames_t* action, const special_effect_t& e ) override
  {
    action -> pyrotex_ignition_cloth = true;
    action -> pyrotex_ignition_cloth_reduction = e.driver() -> effectN( 1 ).time_value();
  }
};

struct contained_infernal_core_t : public class_buff_cb_t<mage_t>
{
  contained_infernal_core_t() : super( MAGE_FIRE, "contained_infernal_core" )
  { }

  virtual buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<mage_t*>( e.player ) -> buffs.contained_infernal_core; }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.player -> find_spell( 248146 ) );
  }
};

// Frost Legendary Items
struct magtheridons_banished_bracers_t : public class_buff_cb_t<mage_t>
{
  magtheridons_banished_bracers_t() : super( MAGE_FROST, "magtheridons_might" )
  { }

  virtual buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<mage_t*>( e.player ) -> buffs.magtheridons_might; }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.trigger() )
      -> set_default_value( e.trigger() -> effectN( 1 ).percent() );
  }
};

struct zannesu_journey_t : public class_buff_cb_t<mage_t>
{
  zannesu_journey_t() : super( MAGE_FROST, "zannesu_journey" )
  { }

  virtual buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<mage_t*>( e.player ) -> buffs.zannesu_journey; }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.trigger() )
      -> set_default_value( e.trigger() -> effectN( 1 ).percent() );
  }
};

struct lady_vashjs_grasp_t : public scoped_actor_callback_t<mage_t>
{
  lady_vashjs_grasp_t() : super( MAGE_FROST )
  { }

  virtual void manipulate( mage_t* actor, const special_effect_t& /* e */ ) override
  { actor -> buffs.lady_vashjs_grasp -> set_chance( 1.0 ); }
};

struct ice_time_t : public scoped_action_callback_t<frozen_orb_t>
{
  ice_time_t() : super( MAGE_FROST, "frozen_orb" )
  { }

  virtual void manipulate( frozen_orb_t* action, const special_effect_t& /* e */ ) override
  { action -> ice_time = true; }
};

struct shattered_fragments_of_sindragosa_t : public class_buff_cb_t<mage_t>
{
  shattered_fragments_of_sindragosa_t() : super( MAGE_FROST, "shattered_fragments_of_sindragosa" )
  { }

  virtual buff_t*& buff_ptr( const special_effect_t& e ) override
  { return debug_cast<mage_t*>( e.player ) -> buffs.shattered_fragments_of_sindragosa; }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.player -> find_spell( 248176 ) );
  }
};
// MAGE MODULE INTERFACE ====================================================

struct mage_module_t : public module_t
{
public:
  mage_module_t() : module_t( MAGE ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto p = new mage_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new mage_report_t( *p ) );
    return p;
  }

  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 281263, cord_of_infinity_t()                        );
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
    hotfix::register_spell( "Mage", "2018-05-02", "Incorrect spell level for Icicle buff.", 205473 )
      .field( "spell_level" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 78 )
      .verification_value( 80 );

    hotfix::register_spell( "Mage", "2017-11-06", "Incorrect spell level for Icicle.", 148022 )
      .field( "spell_level" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 78 )
      .verification_value( 80 );

    hotfix::register_spell( "Mage", "2017-11-08", "Incorrect spell level for Ignite.", 12654 )
      .field( "spell_level" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 78 )
      .verification_value( 99 );

    hotfix::register_spell( "Mage", "2017-03-20", "Manually set Frozen Orb's travel speed.", 84714 )
      .field( "prj_speed" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 20.0 )
      .verification_value( 0.0 );

    hotfix::register_spell( "Mage", "2017-06-21", "Ice Lance is slower than spell data suggests.", 30455 )
      .field( "prj_speed" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 47.0 )
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
