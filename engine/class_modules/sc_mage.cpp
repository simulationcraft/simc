// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Mage
// ==========================================================================

struct mage_t;
struct ignite_t;
struct icicle_t;

enum mage_rotation_e { ROTATION_NONE = 0, ROTATION_DPS, ROTATION_DPM, ROTATION_MAX };
typedef std::pair< timespan_t, double > icicle_data_t;

struct mage_td_t : public actor_pair_t
{
  struct dots_t
  {
    dot_t* combustion;
    dot_t* flamestrike;
    dot_t* frost_bomb;
    dot_t* ignite;
    dot_t* living_bomb;
    dot_t* nether_tempest;
    dot_t* pyroblast;
  } dots;

  struct debuffs_t
  {
    buff_t* frostbolt;
    buff_t* pyromaniac;
    buff_t* slow;
  } debuffs;

  mage_td_t( player_t* target, mage_t* mage );
};

struct mage_t : public player_t
{
public:
  // Icicles
  std::vector< icicle_data_t > icicles;
  icicle_t* icicle;
  core_event_t* icicle_event;

  // Active
  ignite_t* active_ignite;
  int active_living_bomb_targets;
  player_t* last_bomb_target;

  // Benefits
  struct benefits_t
  {
    benefit_t* arcane_charge[ 4 ]; // CHANGED 2014/4/15 - Arcane Charges max stack is 4 now, not 7.
    benefit_t* dps_rotation;
    benefit_t* dpm_rotation;
    benefit_t* water_elemental;
  } benefits;

  // Buffs
  struct buffs_t
  {
    buff_t* arcane_charge;
    buff_t* arcane_missiles;
    buff_t* arcane_power;
    buff_t* brain_freeze;
    buff_t* fingers_of_frost;
    buff_t* frost_armor;
    buff_t* heating_up;
    buff_t* ice_floes;
    buff_t* icy_veins;
    buff_t* invokers_energy;
    stat_buff_t* mage_armor;
    buff_t* molten_armor;
    buff_t* presence_of_mind;
    buff_t* pyroblast;
    buff_t* rune_of_power;
    buff_t* tier13_2pc;
    //buff_t* alter_time;
    absorb_buff_t* incanters_ward;
    buff_t* incanters_absorption;
    buff_t* tier15_2pc_haste;
    buff_t* tier15_2pc_crit;
    buff_t* tier15_2pc_mastery;
    buff_t* profound_magic;
    buff_t* potent_flames;
    buff_t* frozen_thoughts;
    buff_t* fiery_adept;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* evocation;
    cooldown_t* inferno_blast;
    cooldown_t* incanters_ward;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* evocation;
    gain_t* incanters_ward_passive;
    gain_t* rune_of_power;
  } gains;

  // Glyphs
  struct glyphs_t
  {

    // Major
    const spell_data_t* arcane_power;
    const spell_data_t* combustion;
    const spell_data_t* frostfire;
    const spell_data_t* ice_lance;
    const spell_data_t* icy_veins;
    const spell_data_t* inferno_blast;
    const spell_data_t* living_bomb;
    const spell_data_t* loose_mana;
    const spell_data_t* mirror_image;
    const spell_data_t* splitting_ice;


    // Minor
    const spell_data_t* arcane_brilliance;
  } glyphs;


  // Passives
  struct passives_t
  {
    const spell_data_t* nether_attunement;
    const spell_data_t* shatter;
  } passives;

  // Pets
  struct pets_t
  {
    pet_t* water_elemental;
    pet_t* mirror_images[ 3 ];
  } pets;

  // Procs
  struct procs_t
  {
    proc_t* test_for_crit_hotstreak;
    proc_t* crit_for_hotstreak;
    proc_t* hotstreak;
  } procs;

  // Rotation (DPS vs DPM)
  struct rotation_t
  {
    mage_rotation_e current;
    double mana_gain;
    double dps_mana_loss;
    double dpm_mana_loss;
    timespan_t dps_time;
    timespan_t dpm_time;
    timespan_t last_time;

    void reset() { memset( this, 0, sizeof( *this ) ); current = ROTATION_DPS; }
    rotation_t() { reset(); }
  } rotation;

  // Spell Data
  struct spells_t
  {
    const spell_data_t* arcane_missiles;
    const spell_data_t* arcane_power;
    const spell_data_t* icy_veins;

    const spell_data_t* mana_adept;

    const spell_data_t* stolen_time;

    const spell_data_t* arcane_charge_arcane_blast;

    const spell_data_t* icicles_driver;

  } spells;

  // Specializations
  struct specializations_t
  {
    // Arcane
    const spell_data_t* arcane_charge;
    const spell_data_t* mana_adept;
    const spell_data_t* slow;

    // Fire
    const spell_data_t* critical_mass;
    const spell_data_t* ignite;
    const spell_data_t* pyromaniac;

    // Frost
    const spell_data_t* frostbolt;
    const spell_data_t* brain_freeze;
    const spell_data_t* fingers_of_frost;
    const spell_data_t* icicles;

  } spec;

  // Talents
  struct talents_list_t
  {
    const spell_data_t* presence_of_mind;
    const spell_data_t* scorch;
    const spell_data_t* ice_floes;
    const spell_data_t* temporal_shield; // NYI
    const spell_data_t* blazing_speed; // NYI
    const spell_data_t* ice_barrier; // NYI
    const spell_data_t* ring_of_frost; // NYI
    const spell_data_t* ice_ward; // NYI
    const spell_data_t* frostjaw; // NYI
    const spell_data_t* greater_invis; // NYI
    const spell_data_t* cauterize; // NYI
    const spell_data_t* cold_snap;
    const spell_data_t* nether_tempest; // Extra target NYI
    const spell_data_t* living_bomb;
    const spell_data_t* frost_bomb;
    const spell_data_t* invocation;
    const spell_data_t* rune_of_power;
    const spell_data_t* incanters_ward;

  } talents;

public:
  int current_arcane_charges;

  mage_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, MAGE, name, r ),
    icicle( 0 ),
    icicle_event( 0 ),
    active_ignite( 0 ),
    active_living_bomb_targets( 0 ),
    last_bomb_target( 0 ),
    benefits( benefits_t() ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    glyphs( glyphs_t() ),
    passives( passives_t() ),
    pets( pets_t() ),
    procs( procs_t() ),
    rotation( rotation_t() ),
    spells( spells_t() ),
    spec( specializations_t() ),
    talents( talents_list_t() ),
    current_arcane_charges()
  {
    // Cooldowns
    cooldowns.evocation      = get_cooldown( "evocation"     );
    cooldowns.inferno_blast  = get_cooldown( "inferno_blast" );
    cooldowns.incanters_ward = get_cooldown( "incanters_ward" );

    // Options
    base.distance = 40;
  }

  // Character Definition
  virtual void      init_spells();
  virtual void      init_base_stats();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_benefits();
  virtual void      init_action_list();
  virtual void      reset();
  virtual expr_t*   create_expression( action_t*, const std::string& name );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual set_e       decode_set( const item_t& item ) const;
  virtual resource_e primary_resource() const { return RESOURCE_MANA; }
  virtual role_e    primary_role() const { return ROLE_SPELL; }
  virtual stat_e    convert_hybrid_stat( stat_e s ) const;
  virtual double    mana_regen_per_second() const;
  virtual double    composite_player_multiplier( school_e school ) const;
  virtual void      invalidate_cache( cache_e );
  virtual double    composite_multistrike() const;
  virtual double    composite_spell_crit() const;
  virtual double    composite_spell_haste() const;
  virtual double    matching_gear_multiplier( attribute_e attr ) const;
  virtual void      stun();
  virtual void      moving();

  target_specific_t<mage_td_t*> target_data;

  virtual mage_td_t* get_target_data( player_t* target ) const
  {
    mage_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new mage_td_t( target, const_cast<mage_t*>(this) );
    }
    return td;
  }

  // Event Tracking
  virtual void   regen( timespan_t periodicity );
  virtual double resource_gain( resource_e, double amount, gain_t* = 0, action_t* = 0 );
  virtual double resource_loss( resource_e, double amount, gain_t* = 0, action_t* = 0 );

  void add_action( std::string action, std::string options = "", std::string alist = "default" );
  void add_action( const spell_data_t* s, std::string options = "", std::string alist = "default" );

};

namespace pets {

// ==========================================================================
// Pet Water Elemental
// ==========================================================================

struct water_elemental_pet_t : public pet_t
{
  struct freeze_t : public spell_t
  {
    freeze_t( water_elemental_pet_t* p, const std::string& options_str ):
      spell_t( "freeze", p, p -> find_pet_spell( "Freeze" ) )
    {
      parse_options( NULL, options_str );
      aoe = -1;
      may_crit = true;
    }

    virtual void impact( action_state_t* s )
    {
      spell_t::impact( s );

      water_elemental_pet_t* p = static_cast<water_elemental_pet_t*>( player );

      if ( result_is_hit( s -> result ) )
      {
        p -> o() -> buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), 1 );
      }
    }
  };

  struct mini_waterbolt_t : public spell_t
  {
    mini_waterbolt_t( water_elemental_pet_t* p , int bolt_count = 1 ) :
      spell_t( "mini_waterbolt", p, p -> find_spell( 31707 ) )
    {
      may_crit = true;
      background = true;
      dual = true;
      base_costs[ RESOURCE_MANA ] = 0;

      if ( bolt_count < 3 )
      {
        execute_action = new mini_waterbolt_t( p, bolt_count + 1 );
      }

    }

    virtual timespan_t execute_time() const
    { return timespan_t::from_seconds( 0.25 ); }
  };

  struct waterbolt_t : public spell_t
  {
    mini_waterbolt_t* mini_waterbolt;

    waterbolt_t( water_elemental_pet_t* p, const std::string& options_str ):
      spell_t( "waterbolt", p, p -> find_pet_spell( "Waterbolt" ) )
    {
      parse_options( NULL, options_str );
      may_crit = true;

      mini_waterbolt = new mini_waterbolt_t( p );
      add_child( mini_waterbolt );
    }

    virtual void impact( action_state_t* s )
    {
      water_elemental_pet_t* p = static_cast<water_elemental_pet_t*>( player );

      if ( p -> o() -> glyphs.icy_veins -> ok() && p -> o() -> buffs.icy_veins -> up() )
        return;

      spell_t::impact( s );
    }

    void execute()
    {
      spell_t::execute();

      water_elemental_pet_t* p = static_cast<water_elemental_pet_t*>( player );
      if ( p -> o() -> glyphs.icy_veins -> ok() && p -> o() -> buffs.icy_veins -> up() )
      {
        mini_waterbolt -> schedule_execute( mini_waterbolt -> get_state( execute_state ) );
      }
    }

    virtual double action_multiplier() const
    {
      double am = spell_t::action_multiplier();

      water_elemental_pet_t* p = static_cast<water_elemental_pet_t*>( player );

      if ( p -> o() -> glyphs.icy_veins -> ok() && p -> o() -> buffs.icy_veins -> up() )
      {
        am *= 0.4;
      }

      return am;
    }
  };

  water_elemental_pet_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "water_elemental" )
  {
    action_list_str  = "waterbolt";

    owner_coeff.sp_from_sp = 1.0;
  }

  mage_t* o()
  { return static_cast<mage_t*>( owner ); }
  const mage_t* o() const
  { return static_cast<mage_t*>( owner ); }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "freeze"     ) return new     freeze_t( this, options_str );
    if ( name == "waterbolt" ) return new waterbolt_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }

  virtual double composite_player_multiplier( school_e school ) const
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( o() -> spec.icicles -> ok() )
      m *= 1.0 + o() -> cache.mastery_value();

    if ( o() -> buffs.invokers_energy -> up() )
    {
      m *= 1.0 + o() -> buffs.invokers_energy -> data().effectN( 1 ).percent();
    }
    else if ( o() -> buffs.rune_of_power -> check() )
    {
      m *= 1.0 + o() -> buffs.rune_of_power -> data().effectN( 2 ).percent();
    }
    else if ( o() -> talents.incanters_ward -> ok() && o() -> cooldowns.incanters_ward -> up() )
    {
      m *= 1.0 + find_spell( 118858 ) -> effectN( 1 ).percent();
    }
    else if ( o() -> talents.incanters_ward -> ok() )
    {
      m *= 1.0 + o() -> buffs.incanters_absorption -> value() * o() -> buffs.incanters_absorption -> data().effectN( 1 ).percent();
    }

    // Orc racial
    if ( owner -> race == RACE_ORC )
      m *= 1.0 + find_spell( 21563 ) -> effectN( 1 ).percent();

    return m;
  }

};

// ==========================================================================
// Pet Mirror Image
// ==========================================================================

struct mirror_image_pet_t : public pet_t
{
  struct mirror_image_spell_t : public spell_t
  {
    mirror_image_spell_t( const std::string& n, mirror_image_pet_t* p, const spell_data_t* s ):
      spell_t( n, p, s )
    {
      may_crit = true;

      if ( p -> o() -> pets.mirror_images[ 0 ] )
      {
        stats = p -> o() -> pets.mirror_images[ 0 ] -> get_stats( n );
      }
    }

    mirror_image_pet_t* p() const
    { return static_cast<mirror_image_pet_t*>( player ); }
  };

  struct arcane_blast_t : public mirror_image_spell_t
  {
    arcane_blast_t( mirror_image_pet_t* p, const std::string& options_str ):
      mirror_image_spell_t( "arcane_blast", p, p -> find_pet_spell( "Arcane Blast" ) )
    {
      parse_options( NULL, options_str );
    }

    virtual void execute()
    {
      mirror_image_spell_t::execute();

      p() -> arcane_charge -> trigger();
    }

    virtual double action_multiplier() const
    {
      double am = mirror_image_spell_t::action_multiplier();

      am *= 1.0 + p() -> arcane_charge -> stack() * p() -> o() -> spells.arcane_charge_arcane_blast -> effectN( 1 ).percent() *
            ( 1.0 + p() -> o() -> sets.set( SET_T15_4PC_CASTER ) -> effectN( 1 ).percent() );

      return am;
    }
  };

  struct fire_blast_t : public mirror_image_spell_t
  {
    fire_blast_t( mirror_image_pet_t* p, const std::string& options_str ):
      mirror_image_spell_t( "fire_blast", p, p -> find_pet_spell( "Fire Blast" ) )
    {
      parse_options( NULL, options_str );
    }
  };

  struct fireball_t : public mirror_image_spell_t
  {
    fireball_t( mirror_image_pet_t* p, const std::string& options_str ):
      mirror_image_spell_t( "fireball", p, p -> find_pet_spell( "Fireball" ) )
    {
      parse_options( NULL, options_str );
    }
  };

  struct frostbolt_t : public mirror_image_spell_t
  {
    frostbolt_t( mirror_image_pet_t* p, const std::string& options_str ):
      mirror_image_spell_t( "frostbolt", p, p -> find_pet_spell( "Frostbolt" ) )
    {
      parse_options( NULL, options_str );
    }
  };

  buff_t* arcane_charge;

  mirror_image_pet_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "mirror_image", true ),
    arcane_charge( NULL )
  {
    owner_coeff.sp_from_sp = 0.05;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "arcane_blast" ) return new arcane_blast_t( this, options_str );
    if ( name == "fireball"     ) return new     fireball_t( this, options_str );
    if ( name == "frostbolt"    ) return new    frostbolt_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }

  mage_t* o() const
  { return static_cast<mage_t*>( owner ); }

  virtual void init_action_list()
  {
    if ( o() -> glyphs.mirror_image -> ok() && o() -> specialization() != MAGE_FROST )
    {
      if ( o() -> specialization() == MAGE_FIRE )
      {
        action_list_str = "fireball";
      }
      else
      {
        action_list_str = "arcane_blast";
      }
    }
    else
    {
      action_list_str = "frostbolt";
    }

    pet_t::init_action_list();
  }

  virtual void create_buffs()
  {
    pet_t::create_buffs();

    arcane_charge = buff_creator_t( this, "arcane_charge", o() -> spec.arcane_charge )
                    .max_stack( find_spell( 36032 ) -> max_stacks() )
                    .duration( find_spell( 36032 ) -> duration() );
  }


  virtual double composite_player_multiplier( school_e school ) const
  {
    double m = pet_t::composite_player_multiplier( school );

    // Orc racial
    if ( owner -> race == RACE_ORC )
      m *= 1.0 + find_spell( 21563 ) -> effectN( 1 ).percent();

    return m;
  }
};

} // pets

namespace buffs {

namespace alter_time {

// Class to save buff state information relevant to alter time for a buff
struct buff_state_t
{
  buff_t* buff;
  int stacks;
  timespan_t remain_time;
  double value;

  buff_state_t( buff_t* b ) :
    buff( b ),
    stacks( b -> current_stack ),
    remain_time( b -> remains() ),
    value( b -> current_value )
  {
    if ( b -> sim -> debug )
    {
      b -> sim -> out_debug.printf( "Creating buff_state_t for buff %s of player %s",
                          b -> name_str.c_str(), b -> player ? b -> player -> name() : "" );

      b -> sim -> out_debug.printf( "Snapshoted values are: current_stacks=%d remaining_time=%.4f current_value=%.2f",
                          stacks, remain_time.total_seconds(), value );
    }
  }

  void write_back_state() const
  {
    if ( buff -> sim -> debug )
      buff -> sim -> out_debug.printf( "Writing back buff_state_t for buff %s of player %s",
                             buff -> name_str.c_str(), buff -> player ? buff -> player -> name() : "" );

    timespan_t save_buff_cd = buff -> cooldown -> duration; // Temporarily save the buff cooldown duration
    buff -> cooldown -> duration = timespan_t::zero(); // Don't restart the buff cooldown

    if ( stacks )
      buff -> execute( stacks, value, remain_time ); // Reset the buff
    else
      buff -> expire();

    buff -> cooldown -> duration = save_buff_cd; // Restore the buff cooldown duration
  }
};

/*
 * dynamic mage state, to collect data about the mage and all his buffs
 */
struct mage_state_t
{
  mage_t& mage;
  std::array<double, RESOURCE_MAX > resources;
  // location
  std::vector<buff_state_t> buff_states;
  std::vector<icicle_data_t> icicle_states;


  mage_state_t( mage_t& m ) : // Snapshot and start 6s event
    mage( m )
  {
    range::fill( resources, 0.0 );
  }

  void snapshot_current_state()
  {
    resources = mage.resources.current;

    if ( mage.sim -> debug )
      mage.sim -> out_debug.printf( "Creating mage_state_t for mage %s", mage.name() );

    for ( size_t i = 0; i < mage.buff_list.size(); ++i )
    {
      buff_t* b = mage.buff_list[ i ];

      if ( b == static_cast<player_t&>( mage ).buffs.exhaustion )
        continue;

      buff_states.push_back( buff_state_t( b ) );
    }

    for ( size_t i = 0, end = mage.icicles.size(); i < end; i++ )
      icicle_states.push_back( mage.icicles[ i ] );
  }

  void write_back_state()
  {
    // Do not restore state under any circumstances to a mage that is not
    // active
    if ( mage.is_sleeping() )
      return;

    mage.resources.current = resources;

    for ( size_t i = 0; i < buff_states.size(); ++ i )
    {
      buff_states[ i ].write_back_state();
    }

    mage.icicles.clear();
    for ( size_t i = 0, end = icicle_states.size(); i < end; i++ )
      mage.icicles[ i ] = icicle_states[ i ];

    clear_state();
  }

  void clear_state()
  {
    range::fill( resources, 0.0 );
    buff_states.clear();
    icicle_states.clear();
  }
};


} // alter_time namespace

/*struct alter_time_t : public buff_t
{
  alter_time::mage_state_t mage_state;

  alter_time_t( mage_t* player ) :
    buff_t( buff_creator_t( player, "alter_time" ).spell( player -> find_spell( 110909 ) ) ),
    mage_state( *player )
  { }

  mage_t* p() const
  { return static_cast<mage_t*>( player ); }

  virtual bool trigger( int        stacks,
                        double     value,
                        double     chance,
                        timespan_t duration )
  {
    mage_state.snapshot_current_state();

    return buff_t::trigger( stacks, value, chance, duration );
  }

  virtual void expire_override()
  {
    buff_t::expire_override();

    mage_state.write_back_state();

    if ( p() -> sets.has_set_bonus( SET_T15_2PC_CASTER ) )
    {
      p() -> buffs.tier15_2pc_crit -> trigger();
      p() -> buffs.tier15_2pc_haste -> trigger();
      p() -> buffs.tier15_2pc_mastery -> trigger();
    }
  }

  virtual void reset()
  {
    buff_t::reset();

    mage_state.clear_state();
  }
};
*/
// Arcane Power Buff ========================================================

struct arcane_power_t : public buff_t
{
  arcane_power_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "arcane_power", p -> find_class_spell( "Arcane Power" ) )
            .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) )
  {
    cooldown -> duration = timespan_t::zero(); // CD is managed by the spell

    buff_duration *= 1.0 + p -> glyphs.arcane_power -> effectN( 1 ).percent();
  }

  virtual void expire_override()
  {
    buff_t::expire_override();

    mage_t* p = static_cast<mage_t*>( player );
    p -> buffs.tier13_2pc -> expire();
  }
};

// Icy Veins Buff ===========================================================

struct icy_veins_t : public buff_t
{
  icy_veins_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "icy_veins", p -> find_class_spell( "Icy Veins" ) ).add_invalidate( CACHE_SPELL_HASTE ) )
  {
    cooldown -> duration = timespan_t::zero(); // CD is managed by the spell
  }

  virtual void expire_override()
  {
    buff_t::expire_override();

    mage_t* p = debug_cast<mage_t*>( player );
    p -> buffs.tier13_2pc -> expire();
  }
};

struct incanters_ward_t : public absorb_buff_t
{
  double max_absorb;
  double break_after;
  double absorbed;
  gain_t* gain;

  incanters_ward_t( mage_t* p ) :
    absorb_buff_t( absorb_buff_creator_t( p, "incanters_ward" ).spell( p -> talents.incanters_ward ) ),
    max_absorb( 0.0 ), break_after ( -1.0 ), absorbed( 0.0 ),
    gain( p -> get_gain( "incanters_ward mana gain" ) )
  {}

  mage_t* p() const
  { return static_cast<mage_t*>( player ); }

  virtual bool trigger( int        stacks,
                        double    /* value */,
                        double     chance,
                        timespan_t duration )
  {
    max_absorb = p() -> dbc.effect_average( data().effectN( 1 ).id(), p() -> level );
    // coeff hardcoded into tooltip
    max_absorb += p() -> cache.spell_power( SCHOOL_MAX ) * p() -> composite_spell_power_multiplier();

    // If ``break_after'' specified and greater than 1.0, then Incanter's Ward
    // must be broken early
    if ( break_after > 1.0 )
    {
      return absorb_buff_t::trigger( stacks, max_absorb, chance, p() -> buffs.incanters_ward->data().duration() / break_after );
    }
    else
    {
      return absorb_buff_t::trigger( stacks, max_absorb, chance, duration );
    }
  }

  virtual void absorb_used( double amount )
  {
    // if ``break_after'' is specified, then mana will be returned when
    // the shield wears off.
    if ( max_absorb > 0 && break_after < 0.0 )
    {
      absorbed += amount;
      double resource_gain = mana_to_return ( amount / max_absorb ) ;
      p() -> resource_gain( RESOURCE_MANA, resource_gain, gain );
    }
  }

  virtual void expire_override()
  {
    // Trigger Incanter's Absorption with value between 0 and 1, depending on how
    // much absorb has been used, or depending on the value of ``break_after''.
    double absorb_pct;
    if ( break_after >= 1 )
    {
      absorb_pct = 1.0;
    }
    else if ( break_after >= 0 )
    {
      absorb_pct = break_after;
    }
    else if ( max_absorb > 0.0 )
    {
      absorb_pct = absorbed / max_absorb;
    }
    else
    {
      absorb_pct = 0.0;
    }

    if ( absorb_pct > 0.0 )
    {
      p() -> buffs.incanters_absorption -> trigger( 1, absorb_pct );

      // Mana return on expire when ``break_after'' is specified
      if ( break_after >= 0 )
      {
        p() -> resource_gain( RESOURCE_MANA, mana_to_return( absorb_pct ), gain );
      }
    }

    absorbed = 0.0;
    max_absorb = 0.0;

    absorb_buff_t::expire_override();
  }

  virtual void reset() /* override */
  {
    absorb_buff_t::reset();

    absorbed = 0.0;
    max_absorb = 0.0;
  }

  void set_break_after ( double break_after_ )
  {
    break_after = break_after_;
  }

  double mana_to_return ( double absorb_pct )
  {
    double mana = absorb_pct * 0.18 * p() -> resources.max[ RESOURCE_MANA ];

    if ( p() -> passives.nether_attunement -> ok() )
      mana /= p() -> cache.spell_speed();

    return mana;
  }
};

} // end buffs namespace

// ==========================================================================
// Mage Spell
// ==========================================================================

struct mage_spell_t : public spell_t
{
  bool frozen, may_hot_streak, may_proc_missiles, is_copy, consumes_ice_floes, fof_active;
  bool hasted_by_pom; // True if the spells time_to_execute was set to zero exactly because of Presence of Mind
private:
  bool pom_enabled;
  // Helper variable to disable the functionality of PoM in mage_spell_t::execute_time(),
  // to check if the spell would be instant or not without PoM.
public:
  int dps_rotation;
  int dpm_rotation;

  mage_spell_t( const std::string& n, mage_t* p,
                const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( n, p, s ),
    frozen( false ),
    may_hot_streak( false ),
    may_proc_missiles( true ),
    is_copy( false ),
    consumes_ice_floes( true ),
    fof_active( false ),
    hasted_by_pom( false ),
    pom_enabled( true ),
    dps_rotation( 0 ),
    dpm_rotation( 0 )
  {
    may_crit      = ( base_dd_min > 0 ) && ( base_dd_max > 0 );
    tick_may_crit = true;
  }

  mage_t* p()
  { return static_cast<mage_t*>( player ); }
  const mage_t* p() const
  { return static_cast<mage_t*>( player ); }

  mage_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual void parse_options( option_t*          options,
                              const std::string& options_str )
  {
    option_t base_options[] =
    {
      opt_bool( "dps", dps_rotation ),
      opt_bool( "dpm", dpm_rotation ),
      opt_null()
    };
    std::vector<option_t> merged_options;
    spell_t::parse_options( option_t::merge( merged_options, options, base_options ), options_str );
  }

  virtual bool ready()
  {
    if ( dps_rotation )
      if ( p() -> rotation.current != ROTATION_DPS )
        return false;

    if ( dpm_rotation )
      if ( p() -> rotation.current != ROTATION_DPM )
        return false;

    return spell_t::ready();
  }

  virtual double cost() const
  {
    double c = spell_t::cost();

    if ( p() -> buffs.arcane_power -> check() )
    {
      double m = 1.0 + p() -> buffs.arcane_power -> data().effectN( 2 ).percent();

      c *= m;
    }

    return c;
  }
  virtual timespan_t execute_time() const
  {
    timespan_t t = spell_t::execute_time();

    if ( ! channeled && pom_enabled && t > timespan_t::zero() && p() -> buffs.presence_of_mind -> check() )
      return timespan_t::zero();

    return t;
  }
  // Ensures mastery for Arcane is only added to spells which call mage_spell_t, so things like the Legendary Cloak do not get modified. Added 4/15/2014
  virtual double action_multiplier() const
  {
    double am=spell_t::action_multiplier();
    if ( p() -> specialization() == MAGE_ARCANE )
    {
      double mana_pct= p() -> resources.pct( RESOURCE_MANA );
      am *= 1.0 + mana_pct * p() -> composite_mastery_value();
    }
    return am;
  }
  //
  virtual void schedule_execute( action_state_t* state = 0 )
  {
    spell_t::schedule_execute( state );

    if ( ! channeled )
    {
      assert( pom_enabled );
      pom_enabled = false;
      if ( execute_time() > timespan_t::zero() && p() -> buffs.presence_of_mind -> up() )
      {
        hasted_by_pom = true;
      }
      pom_enabled = true;
    }
  }

  virtual bool usable_moving() const
  {
    bool um = spell_t::usable_moving();
    timespan_t t = base_execute_time;
    if ( p() -> talents.ice_floes -> ok() &&
         t < timespan_t::from_seconds( 4.0 ) &&
         ( p() -> buffs.ice_floes -> up() || p() -> buffs.ice_floes -> cooldown -> up() ) )
      um = true;

    return um;
  }

  virtual double composite_crit_multiplier() const
  {
    double m = spell_t::composite_crit_multiplier();
    if ( frozen && p() -> passives.shatter -> ok() )
      m *= 2.0;
    return m;
  }

  void snapshot_internal( action_state_t* s, uint32_t flags, dmg_e rt )
  {
    spell_t::snapshot_internal( s, flags, rt );
    // Shatter's +50% crit bonus needs to be added after multipliers etc
    if ( flags & STATE_CRIT && frozen && p() -> passives.shatter -> ok() )
      s -> crit += p() -> passives.shatter -> effectN( 2 ).percent();
  }

  virtual void expire_heating_up() // delay 0.25s the removal of heating up on non-critting spell with travel time or scorch
  {
    mage_t* p = this -> p();

    if ( ! travel_speed )
    {
      p -> buffs.heating_up -> expire();
    }
    else
    {
      // delay heating up removal
      if ( sim -> log ) sim -> out_log << "Heating up delay by 0.25s";
      p -> buffs.heating_up -> expire( timespan_t::from_millis( 250 ) );
    }
  }

  void trigger_hot_streak( action_state_t* s )
  {
    mage_t* p = this -> p();

    if ( ! may_hot_streak )
      return;

    if ( p -> specialization() != MAGE_FIRE )
      return;

    p -> procs.test_for_crit_hotstreak -> occur();

    if ( s -> result == RESULT_CRIT )
    {
      p -> procs.crit_for_hotstreak -> occur();
      // Reference: http://elitistjerks.com/f75/t110326-cataclysm_fire_mage_compendium/p6/#post1831143

      if ( ! p -> buffs.heating_up -> up() )
      {
        p -> buffs.heating_up -> trigger();
      }
      else
      {
        p -> procs.hotstreak  -> occur();
        p -> buffs.heating_up -> expire();
        p -> buffs.pyroblast  -> trigger();
      }
    }
    else
    {
      if ( p -> buffs.heating_up -> up() ) expire_heating_up();
    }
  }
  // mage_spell_t::execute ==================================================

  virtual void execute()
  {
    p() -> benefits.dps_rotation -> update( p() -> rotation.current == ROTATION_DPS );
    p() -> benefits.dpm_rotation -> update( p() -> rotation.current == ROTATION_DPM );

    spell_t::execute();

    if ( background )
      return;


    if ( ! channeled && !is_copy && hasted_by_pom )
    {
      p() -> buffs.presence_of_mind -> expire();
      hasted_by_pom = false;
    }

    if ( !is_copy && execute_time() > timespan_t::zero() && consumes_ice_floes )
    {
      p() -> buffs.ice_floes -> decrement();
    }

    if ( !harmful )
    {
      may_proc_missiles = false;
    }
    if ( p() -> specialization() == MAGE_ARCANE && may_proc_missiles )
    {
      p() -> buffs.arcane_missiles -> trigger();
    }
  }

  virtual void impact( action_state_t* s )
  {
    spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_hot_streak( s );
    }
  }

  virtual void reset()
  {
    spell_t::reset();

    hasted_by_pom = false;
  }

  void trigger_ignite( action_state_t* state );
};

// Icicles ==================================================================

struct icicle_t : public mage_spell_t
{
  icicle_t( mage_t* p ) : mage_spell_t( "icicle", p, p -> find_spell( 148022 ) )
  {
    may_crit = false;
    proc = background = true;

    if ( p -> glyphs.splitting_ice -> ok() )
      aoe = p -> glyphs.splitting_ice -> effectN( 1 ).base_value() + 1;

    base_aoe_multiplier *= p -> glyphs.splitting_ice -> effectN( 2 ).percent();
  }

  void init()
  {
    mage_spell_t::init();

    snapshot_flags &= ~( STATE_MUL_DA | STATE_SP | STATE_CRIT | STATE_TGT_CRIT );
  }
};

static double icicle_damage( mage_t* mage )
{
  if ( mage -> icicles.size() == 0 )
    return 0;

  timespan_t threshold = mage -> spells.icicles_driver -> duration();

  std::vector< icicle_data_t >::iterator idx = mage -> icicles.begin(),
                                         end = mage -> icicles.end();
  for ( ; idx < end; ++idx )
  {
    if ( mage -> sim -> current_time - ( *idx ).first < threshold )
      break;
  }

  // Set of icicles timed out
  if ( idx != mage -> icicles.begin() )
    mage -> icicles.erase( mage -> icicles.begin(), idx );

  if ( mage -> icicles.size() > 0 )
  {
    double d = mage -> icicles.front().second;
    mage -> icicles.erase( mage -> icicles.begin() );
    return d;
  }

  return 0;
}

struct icicle_event_t : public event_t
{
  mage_t* mage;
  double damage;

  icicle_event_t( mage_t& m, double d, bool first = false ) :
    event_t( m, "icicle_event" ), mage( &m ), damage( d )
  {
    double cast_time = first ? 0.25 : 0.75;
    cast_time *= mage -> cache.spell_speed();

    add_event( timespan_t::from_seconds( cast_time ) );
  }

  void execute()
  {
    mage -> icicle -> base_dd_min = mage -> icicle -> base_dd_max = damage;
    mage -> icicle -> schedule_execute();

    double d = icicle_damage( mage );
    if ( d > 0 )
    {
      mage -> icicle_event = new ( sim() ) icicle_event_t( *mage, d );
      if ( mage -> sim -> debug )
        mage -> sim -> out_debug.printf( "%s icicle use (chained), damage=%f, total=%u",
                               mage -> name(), d, as<unsigned>( mage -> icicles.size() ) );
    }
    else
      mage -> icicle_event = 0;
  }
};

static void trigger_icicle( mage_t* mage, bool chain = false )
{
  if ( ! mage -> spec.icicles -> ok() )
    return;

  if ( mage -> icicles.size() == 0 )
    return;

  double d = 0;

  if ( chain && ! mage -> icicle_event )
  {
    d = icicle_damage( mage );
    mage -> icicle_event = new ( *mage -> sim ) icicle_event_t( *mage, d, true );
  }
  else if ( ! chain )
  {
    d = icicle_damage( mage );
    mage -> icicle -> base_dd_min = mage -> icicle -> base_dd_max = d;
    mage -> icicle -> schedule_execute();
  }

  if ( mage -> sim -> debug )
    mage -> sim -> out_debug.printf( "%s icicle use%s, damage=%f, total=%u",
                           mage -> name(), chain ? " (chained)" : "", d,
                           as<unsigned>( mage -> icicles.size() ) );
}

static void trigger_icicle_gain( action_state_t* state )
{
  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  mage_t* mage = debug_cast< mage_t* >( state -> action -> player );
  if ( ! mage -> spec.icicles -> ok() )
    return;

  // Icicles do not double dip on target based multipliers
  double amount = state -> result_amount / state -> target_da_multiplier * mage -> cache.mastery_value();

  assert( as<int>( mage -> icicles.size() ) <= mage -> spec.icicles -> effectN( 2 ).base_value() );
  // Shoot one
  if ( as<int>( mage -> icicles.size() ) == mage -> spec.icicles -> effectN( 2 ).base_value() )
    trigger_icicle( mage );
  mage -> icicles.push_back( icicle_data_t( mage -> sim -> current_time, amount ) );
  if ( mage -> sim -> debug )
    mage -> sim -> out_debug.printf( "%s icicle gain, damage=%f, total=%u",
                           mage -> name(),
                           amount,
                           as<unsigned>( mage -> icicles.size() ) );
}

// Ignite ===================================================================

struct ignite_t : public residual_dot_action< mage_spell_t >
{
  ignite_t( mage_t* player ) :
    residual_dot_action_t( "ignite", player, player -> find_spell( 12846 ) )
  {
  }
};

void mage_spell_t::trigger_ignite( action_state_t* state )
{
  mage_t& p = *this -> p();
  if ( ! p.active_ignite ) return;
  double amount = state -> result_amount * p.cache.mastery_value();
  p.active_ignite -> trigger( state -> target, amount );
}

// Arcane Barrage Spell =====================================================

struct arcane_barrage_t : public mage_spell_t
{
  arcane_barrage_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_barrage", p, p -> find_class_spell( "Arcane Barrage" ) )
  {
    parse_options( NULL, options_str );

    base_aoe_multiplier *= data().effectN( 2 ).percent();
  }

  virtual void execute()
  {
    int charges = p() -> buffs.arcane_charge -> check();
    aoe = charges <= 0 ? 0 : 1 + charges;

    for ( int i = 0; i < ( int ) sizeof_array( p() -> benefits.arcane_charge ); i++ )
    {
      p() -> benefits.arcane_charge[ i ] -> update( i == charges );
    }

    mage_spell_t::execute();

    p() -> buffs.arcane_charge -> expire();
  }

  virtual double action_multiplier() const
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.arcane_charge -> stack() * p() -> spells.arcane_charge_arcane_blast -> effectN( 1 ).percent() *
          ( 1.0 + p() -> sets.set( SET_T15_4PC_CASTER ) -> effectN( 1 ).percent() );

    return am;
  }
};

// Arcane Blast Spell =======================================================

struct arcane_blast_t : public mage_spell_t
{
  arcane_blast_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_blast", p, p -> find_class_spell( "Arcane Blast" ) )
  {
    parse_options( NULL, options_str );

    if ( p -> sets.has_set_bonus( SET_PVP_4PC_CASTER ) )
      base_multiplier *= 1.05;
  }

  virtual double cost() const
  {
    double c = mage_spell_t::cost();

    if ( p() -> buffs.arcane_charge -> check() )
    {
      c *= 1.0 +  p() -> buffs.arcane_charge -> check() * p() -> spells.arcane_charge_arcane_blast -> effectN( 2 ).percent() *
           ( 1.0 + p() -> sets.set( SET_T15_4PC_CASTER ) -> effectN( 1 ).percent() );
    }

    if ( p() -> buffs.profound_magic -> check() )
    {
      c *= 1.0 - p() -> buffs.profound_magic -> stack() * 0.25;
    }

    return c;
  }

  virtual void execute()
  {
    for ( unsigned i = 0; i < sizeof_array( p() -> benefits.arcane_charge ); i++ )
    {
      p() -> benefits.arcane_charge[ i ] -> update( as<int>( i ) == p() -> buffs.arcane_charge -> check() );
    }

    mage_spell_t::execute();

    p() -> buffs.arcane_charge -> trigger();
    p() -> buffs.profound_magic -> expire();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> buffs.tier13_2pc -> trigger( 1, buff_t::DEFAULT_VALUE(), p() -> buffs.tier13_2pc -> default_chance * 2.0 );
    }
  }

  virtual double action_multiplier() const
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.arcane_charge -> stack() * p() -> spells.arcane_charge_arcane_blast -> effectN( 1 ).percent() *
          ( 1.0 + p() -> sets.set( SET_T15_4PC_CASTER ) -> effectN( 1 ).percent() );

    return am;
  }
};

// Arcane Brilliance Spell ==================================================

struct arcane_brilliance_t : public mage_spell_t
{
  arcane_brilliance_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_brilliance", p, p -> find_class_spell( "Arcane Brilliance" ) )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ] *= 1.0 + p -> glyphs.arcane_brilliance -> effectN( 1 ).percent();
    harmful = false;
    background = ( sim -> overrides.spell_power_multiplier != 0 && sim -> overrides.critical_strike != 0 );
  }

  virtual void execute()
  {
    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    if ( ! sim -> overrides.spell_power_multiplier )
      sim -> auras.spell_power_multiplier -> trigger();

    if ( ! sim -> overrides.critical_strike )
      sim -> auras.critical_strike -> trigger();
  }
};

// Arcane Explosion Spell ===================================================

struct arcane_explosion_t : public mage_spell_t
{
  arcane_explosion_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_explosion", p, p -> find_class_spell( "Arcane Explosion" ) )
  {
    parse_options( NULL, options_str );
    aoe = -1;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( rng().roll ( p() -> find_class_spell( "Arcane Explosion" ) -> effectN( 2 ).percent() ) )
      {
        p() -> buffs.arcane_charge -> trigger();
      }
      else p() -> buffs.arcane_charge -> refresh();
    }
  }
};

// Arcane Missiles Spell ====================================================

struct arcane_missiles_tick_t : public mage_spell_t
{
  arcane_missiles_tick_t( mage_t* p ) :
    mage_spell_t( "arcane_missiles_tick", p, p -> find_class_spell( "Arcane Missiles" ) -> effectN( 2 ).trigger() )
  {
    background  = true;
  }
};

struct arcane_missiles_t : public mage_spell_t
{
  arcane_missiles_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_missiles", p, p -> find_class_spell( "Arcane Missiles" ) )
  {
    parse_options( NULL, options_str );
    may_miss = false;
    may_proc_missiles = false;

    base_tick_time    = timespan_t::from_seconds( 0.4 );
    num_ticks         = 5;
    channeled         = true;
    hasted_ticks      = false;

    dynamic_tick_action = true;
    tick_action = new arcane_missiles_tick_t( p );
  }

  virtual double action_multiplier() const
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.arcane_charge -> stack() * p() -> spells.arcane_charge_arcane_blast -> effectN( 1 ).percent() *
          ( 1.0 + p() -> sets.set( SET_T15_4PC_CASTER ) -> effectN( 1 ).percent() );

    if ( p() -> sets.has_set_bonus( SET_T14_2PC_CASTER ) )
    {
      am *= 1.07;
    }

    return am;
  }

  virtual void execute()
  {
    for ( unsigned i = 0; i < sizeof_array( p() -> benefits.arcane_charge ); i++ )
    {
      p() -> benefits.arcane_charge[ i ] -> update( as<int>( i ) == p() -> buffs.arcane_charge -> check() );
    }

    p() -> buffs.arcane_charge   -> trigger(); // Comes before action_t::execute(). See Issue 1189. Changed on 18/12/2012

    mage_spell_t::execute();

    p() -> buffs.arcane_missiles -> up();

    if ( p() -> sets.has_set_bonus( SET_T16_2PC_CASTER ) )
    {
      p() -> buffs.profound_magic -> trigger();
    }

    // T16 4pc has a chance not to consume arcane missiles buff
    if ( !p() -> sets.has_set_bonus( SET_T16_4PC_CASTER ) || ! rng().roll( p() -> sets.set( SET_T16_4PC_CASTER ) -> effectN( 1 ).percent() ) )
    {
      p() -> buffs.arcane_missiles -> decrement();
    }
  }

  virtual bool ready()
  {
    if ( ! p() -> buffs.arcane_missiles -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Arcane Power Spell =======================================================

struct arcane_power_t : public mage_spell_t
{
  arcane_power_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_power", p, p -> find_class_spell( "Arcane Power" ) )
  {
    parse_options( NULL, options_str );
    harmful = false;

    cooldown -> duration *= 1.0 + p -> glyphs.arcane_power -> effectN( 2 ).percent();
  }

  virtual void update_ready( timespan_t cd_override )
  {
    cd_override = cooldown -> duration;

    if ( p() -> sets.has_set_bonus( SET_T13_4PC_CASTER ) )
      cd_override *= ( 1.0 - p() -> buffs.tier13_2pc -> check() * p() -> spells.stolen_time -> effectN( 1 ).base_value() );

    mage_spell_t::update_ready( cd_override );
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> buffs.arcane_power -> trigger( 1, data().effectN( 1 ).percent() );
  }
};

// Blink Spell ==============================================================

struct blink_t : public mage_spell_t
{
  blink_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "blink", p, p -> find_class_spell( "Blink" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
    base_teleport_distance = 20;
    movement_directionality = MOVEMENT_OMNI;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    player -> buffs.stunned -> expire();
  }
};

// Blizzard Spell ===========================================================

struct blizzard_shard_t : public mage_spell_t
{
  blizzard_shard_t( mage_t* p ) :
    mage_spell_t( "blizzard_shard", p, p -> find_class_spell( "Blizzard" ) -> effectN( 2 ).trigger() )
  {
    aoe = -1;
    background = true;
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      double fof_proc_chance = p() -> buffs.fingers_of_frost -> data().effectN( 2 ).percent();
      if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
      {
        fof_proc_chance *= 1.2;
      }
      p() -> buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), fof_proc_chance );
    }
  }
};

struct blizzard_t : public mage_spell_t
{
  blizzard_shard_t* shard;

  blizzard_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "blizzard", p, p -> find_class_spell( "Blizzard" ) ),
    shard( new blizzard_shard_t( p ) )
  {
    parse_options( NULL, options_str );

    channeled    = true;
    hasted_ticks = false;
    may_miss     = false;

    add_child( shard );
  }

  void tick( dot_t* d )
  {
    mage_spell_t::tick( d );

    shard -> execute();
  }
};

// Cold Snap Spell ==========================================================

struct cold_snap_t : public mage_spell_t
{
  cooldown_t* cooldown_cofc;

  cold_snap_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "cold_snap", p, p -> talents.cold_snap ),
    cooldown_cofc( p -> get_cooldown( "cone_of_cold" ) )
  {
    parse_options( NULL, options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> resource_gain( RESOURCE_HEALTH, p() -> resources.base[ RESOURCE_HEALTH ] * p() -> talents.cold_snap -> effectN( 2 ).percent() );

    cooldown_cofc -> reset( false );
  }
};

// Combustion Spell =========================================================

struct combustion_t : public mage_spell_t
{
  combustion_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "combustion", p, p -> find_class_spell( "Combustion" ) )
  {
    parse_options( NULL, options_str );

    may_hot_streak = true;

    // The "tick" portion of spell is specified in the DBC data in an alternate version of Combustion
    const spell_data_t& tick_spell = *p -> find_spell( 83853, "combustion_dot" );
    base_tick_time = tick_spell.effectN( 1 ).period();
    num_ticks      = static_cast<int>( tick_spell.duration() / base_tick_time );
    tick_may_crit  = true;

    if ( p -> sets.has_set_bonus( SET_T14_4PC_CASTER ) )
    {
      cooldown -> duration *= 0.8;
    }

    if ( p -> glyphs.combustion -> ok() )
    {
      num_ticks = static_cast<int>( num_ticks * ( 1.0 + p -> glyphs.combustion -> effectN( 1 ).percent() ) );
      cooldown -> duration *= 1.0 + p -> glyphs.combustion -> effectN( 2 ).percent();
      base_dd_multiplier *= 1.0 + p -> glyphs.combustion -> effectN( 3 ).percent();
    }
  }

  virtual double calculate_tick_amount( action_state_t* s )
  {
    double a = 0.0;

    if ( dot_t* d = find_dot( s -> target ) )
      a += d -> tick_amount;

    if ( s -> result == RESULT_CRIT )
      a *= 1.0 + total_crit_bonus();
    return a;
  }

  virtual void trigger_dot( action_state_t* s )
  {
    mage_td_t* this_td = td( s -> target );

    dot_t* ignite_dot     = this_td -> dots.ignite;
    dot_t* combustion_dot = this_td -> dots.combustion;

    if ( ignite_dot -> ticking )
    {
      mage_spell_t::trigger_dot( s );

      combustion_dot -> tick_amount = ignite_dot -> tick_amount * 0.2; // 0.2 modifier hardcoded into tooltip 2013/08/14 PTR
    }
  }

  virtual void update_ready( timespan_t cd_override )
  {
    cd_override = cooldown -> duration;

    if ( p() -> sets.has_set_bonus( SET_T13_4PC_CASTER ) )
      cd_override *= ( 1.0 - p() -> buffs.tier13_2pc -> check() * p() -> spells.stolen_time -> effectN( 1 ).base_value() );

    mage_spell_t::update_ready( cd_override );
  }

  virtual void execute()
  {
    p() -> cooldowns.inferno_blast -> reset( false );

    mage_spell_t::execute();
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );

    p() -> buffs.tier13_2pc -> expire();
  }
};

// Cone of Cold Spell =======================================================

struct cone_of_cold_t : public mage_spell_t
{
  cone_of_cold_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "cone_of_cold", p, p -> find_class_spell( "Cone of Cold" ) )
  {
    parse_options( NULL, options_str );
    aoe = -1;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    p() -> buffs.frozen_thoughts -> expire();
  }

  virtual double action_multiplier() const
  {
    double am = mage_spell_t::action_multiplier();

    if ( p() -> buffs.frozen_thoughts -> up() )
    {
      am *= ( 1.0 + p() -> buffs.frozen_thoughts -> data().effectN( 1 ).percent() );
    }

    return am;
  }
};

// Counterspell Spell =======================================================

struct counterspell_t : public mage_spell_t
{
  counterspell_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "counterspell", p, p -> find_class_spell( "Counterspell" ) )
  {
    parse_options( NULL, options_str );
    may_miss = may_crit = false;
  }
};

// Dragon's Breath Spell ====================================================

struct dragons_breath_t : public mage_spell_t
{
  dragons_breath_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "dragons_breath", p, p -> find_class_spell( "Dragon's Breath" ) )
  {
    parse_options( NULL, options_str );
    aoe = -1;
  }
};

// Evocation Spell ==========================================================

class evocation_t : public mage_spell_t
{
  timespan_t pre_cast;
  int arcane_charges;

public:
  evocation_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "evocation", p, p -> talents.rune_of_power -> ok() ? spell_data_t::not_found() : p -> find_class_spell( "Evocation" ) ),
    pre_cast( timespan_t::zero() ), arcane_charges( 0 )
  {
    option_t options[] =
    {
      opt_timespan( "precast", pre_cast ),
      opt_null()
    };

    parse_options( options, options_str );
    pre_cast = std::max( pre_cast, timespan_t::zero() );

    base_tick_time    = timespan_t::from_seconds( 2.0 );
    tick_zero         = true;
    channeled         = true;
    harmful           = false;
    hasted_ticks      = false;

    cooldown = p -> cooldowns.evocation;
    cooldown -> duration = data().cooldown();

    timespan_t duration = data().duration();

    if ( p -> talents.invocation -> ok() )
    {
      cooldown -> duration = timespan_t::zero();

      base_tick_time *= 1.0 + p -> talents.invocation -> effectN( 2 ).percent();
      duration       *= 1.0 + p -> talents.invocation -> effectN( 2 ).percent();
    }

    num_ticks = ( int ) ( duration / base_tick_time );
  }

  virtual void tick( dot_t* d )
  {
    mage_spell_t::tick( d );

    double mana = p() -> resources.max[ RESOURCE_MANA ];

    if ( p() -> passives.nether_attunement -> ok() )
      mana /= p() -> cache.spell_speed();

    if ( p() -> specialization() == MAGE_ARCANE )
    {
      mana *= 0.1;

      mana *= 1.0 + arcane_charges * p() -> spells.arcane_charge_arcane_blast -> effectN( 4 ).percent() *
              ( 1.0 + p() -> sets.set( SET_T15_4PC_CASTER ) -> effectN( 1 ).percent() );
    }
    else
    {
      mana *= data().effectN( 1 ).percent();
    }


    p() -> resource_gain( RESOURCE_MANA, mana, p() -> gains.evocation );
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );

    if ( d -> current_tick == d -> num_ticks ) // only trigger invokers_energy if dot has successfully finished all ticks
      p() -> buffs.invokers_energy -> trigger();
  }

  virtual void execute()
  {
    mage_t& p = *this -> p();

    if ( ! p.in_combat )
    {
      if ( p.talents.invocation -> ok() )
      {
        // Trigger buff with temporarily reduced duration
        if ( p.buffs.invokers_energy -> buff_duration - pre_cast > timespan_t::zero() )
          p.buffs.invokers_energy -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0,
                                              p.buffs.invokers_energy -> buff_duration - pre_cast );
        if ( cooldown -> duration - pre_cast > timespan_t::zero() )
          cooldown -> start( cooldown -> duration - pre_cast );
      }

      return;
    }

    arcane_charges = p.buffs.arcane_charge -> check();
    p.buffs.arcane_charge -> expire();
    mage_spell_t::execute();


    // evocation automatically causes a switch to dpm rotation
    if ( p.rotation.current == ROTATION_DPS )
    {
      p.rotation.dps_time += ( sim -> current_time - p.rotation.last_time );
    }
    else if ( p.rotation.current == ROTATION_DPM )
    {
      p.rotation.dpm_time += ( sim -> current_time - p.rotation.last_time );
    }
    p.rotation.last_time = sim -> current_time;

    if ( sim -> log )
      sim -> out_log.printf( "%s switches to DPM spell rotation", player -> name() );
    p.rotation.current = ROTATION_DPM;
  }
};

// Fire Blast Spell =========================================================

struct fire_blast_t : public mage_spell_t
{
  fire_blast_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "fire_blast", p, p -> find_class_spell( "Fire Blast" ) )
  {
    parse_options( NULL, options_str );
    may_hot_streak = true;
  }
};

// Fireball Spell ===========================================================

struct fireball_t : public mage_spell_t
{
  fireball_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "fireball", p, p -> find_class_spell( "Fireball" ) )
  {
    parse_options( NULL, options_str );
    may_hot_streak = true;

    if ( p -> sets.has_set_bonus( SET_PVP_4PC_CASTER ) )
      base_multiplier *= 1.05;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> buffs.tier13_2pc -> trigger();
    }
  }

  virtual timespan_t travel_time() const
  {
    timespan_t t = mage_spell_t::travel_time();
    return ( t > timespan_t::from_seconds( 0.75 ) ? timespan_t::from_seconds( 0.75 ) : t );
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    trigger_ignite( s );
  }

  double composite_crit_multiplier() const
  {
    double m = mage_spell_t::composite_crit_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double tm = mage_spell_t::composite_target_multiplier( t );

    if ( td( t ) -> debuffs.pyromaniac -> up() )
    {
      tm *= 1.1;
    }

    return tm;
  }
};

// Flamestrike Spell ========================================================

struct flamestrike_t : public mage_spell_t
{
  flamestrike_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "flamestrike", p, p -> find_specialization_spell( "Flamestrike" ) )
  {
    parse_options( NULL, options_str );

    aoe = -1;
  }
};

// Frost Armor Spell ========================================================

struct frost_armor_t : public mage_spell_t
{
  frost_armor_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frost_armor", p, p -> find_class_spell( "Frost Armor" ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> buffs.molten_armor -> expire();
    p() -> buffs.mage_armor -> expire();
    p() -> buffs.frost_armor -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.frost_armor -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Frost Bomb ===============================================================

struct frost_bomb_explosion_t : public mage_spell_t
{
  frost_bomb_explosion_t( mage_t* p ) :
    mage_spell_t( "frost_bomb_explosion", p, p -> find_spell( p -> talents.frost_bomb -> effectN( 2 ).base_value() ) )
  {
    aoe = -1;
    base_aoe_multiplier = 0.5; // This is stored in effectN( 2 ), but it's just 50% of effectN( 1 )
    background = true;
    parse_effect_data( data().effectN( 1 ) );
  }

  virtual resource_e current_resource() const
  { return RESOURCE_NONE; }
};

struct frost_bomb_t : public mage_spell_t
{
  timespan_t original_cooldown;

  frost_bomb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frost_bomb", p, p -> talents.frost_bomb ),
    original_cooldown( timespan_t::zero() )
  {
    parse_options( NULL, options_str );
    base_tick_time = data().duration();
    num_ticks = 1; // Fake a tick, so we can trigger the explosion at the end of it
    hasted_ticks = true; // Haste decreases the 'tick' time to explosion

    dynamic_tick_action = true;
    tick_action = new frost_bomb_explosion_t( p );

    original_cooldown = cooldown -> duration;
  }

  virtual void execute()
  {
    // Cooldown is reduced by haste
    cooldown -> duration = original_cooldown * composite_haste();
    mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
      p() -> last_bomb_target = execute_state -> target;
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    if ( p() -> specialization() == MAGE_FIRE && result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs.pyromaniac -> trigger( 1, buff_t::DEFAULT_VALUE(), 1 );
    }
  }

  virtual void tick( dot_t* d )
  {
    mage_spell_t::tick( d );
    if ( p() -> last_bomb_target == d -> state -> target )
      p() -> buffs.brain_freeze -> trigger();
    d -> cancel();
  }
};

// Frostbolt Spell ==========================================================

struct mini_frostbolt_t : public mage_spell_t
{
  mini_frostbolt_t( mage_t* p, int bolt_count = 1 ) :
    mage_spell_t( "mini_frostbolt", p, p -> find_spell( 31707 ) )
  {
    background = true;
    //dual = true;
    base_costs[ RESOURCE_MANA ] = 0;

    if ( p -> sets.has_set_bonus( SET_PVP_4PC_CASTER ) )
      base_multiplier *= 1.05;

    if ( bolt_count < 3 )
    {
      execute_action = new mini_frostbolt_t( p, bolt_count + 1 );
    }
  }

  void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );
    trigger_icicle_gain( s );
  }

  virtual timespan_t execute_time() const
  { return timespan_t::from_seconds( 0.25 ); }
};

struct frostbolt_t : public mage_spell_t
{
  struct state_t : public action_state_t
  {
    bool mini_version;
    state_t( action_t* a, player_t* t ) : action_state_t( a, t ),
      mini_version( false ) { }

    std::ostringstream& debug_str( std::ostringstream& s )
    { action_state_t::debug_str( s ) << " mini_version=" << std::boolalpha << mini_version; return s; }

    void initialize()
    { action_state_t::initialize(); mini_version = false; }

    void copy_state( const action_state_t* o )
    {
      action_state_t::copy_state( o );
      mini_version = static_cast<const state_t&>( *o ).mini_version;
    }
  };
  mini_frostbolt_t* mini_frostbolt;

  frostbolt_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frostbolt", p, p -> find_class_spell( "Frostbolt" ) ),
    mini_frostbolt( new mini_frostbolt_t( p ) )
  {
    parse_options( NULL, options_str );

    if ( p -> sets.has_set_bonus( SET_PVP_4PC_CASTER ) )
      base_multiplier *= 1.05;

    add_child( mini_frostbolt );
  }

  virtual void snapshot_state( action_state_t* s, dmg_e type )
  {
    state_t& state = static_cast<state_t&>( *s );

    state.mini_version = p() -> glyphs.icy_veins -> ok() && p() -> buffs.icy_veins -> check();

    mage_spell_t::snapshot_state( s, type );
  }

  virtual action_state_t* new_state()
  { return new state_t( this, target ); }

  virtual void execute()
  {
    dual = p() -> glyphs.icy_veins -> ok() && p() -> buffs.icy_veins -> check();
    mage_spell_t::execute();
    dual = false;

    if ( result_is_hit( execute_state -> result ) )
    {
      double fof_proc_chance = p() -> buffs.fingers_of_frost -> data().effectN( 1 ).percent();

      fof_proc_chance += p() -> sets.set( SET_T15_4PC_CASTER ) -> effectN( 3 ).percent();

      if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
      {
        fof_proc_chance *= 1.2;
      }

      p() -> buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), fof_proc_chance );

      p() -> buffs.tier13_2pc -> trigger();

      if ( p() -> glyphs.icy_veins -> ok() && p() -> buffs.icy_veins -> up() )
      {
        mini_frostbolt -> schedule_execute( mini_frostbolt -> get_state( execute_state ) );
      }
    }

    p() -> buffs.frozen_thoughts -> expire();
  }

  virtual void impact( action_state_t* s )
  {
    if ( ! static_cast<const state_t&>( *s ).mini_version ) // Bail out if mini spells get casted
    {
      mage_spell_t::impact( s );
      trigger_icicle_gain( s );
    }
  }

  virtual double action_multiplier() const
  {
    double am = mage_spell_t::action_multiplier();

    if ( p() -> glyphs.icy_veins -> ok() && p() -> buffs.icy_veins -> up() )
    {
      am *= 0.4;
    }

    if ( p() -> buffs.frozen_thoughts -> up() )
    {
      am *= ( 1.0 + p() -> buffs.frozen_thoughts -> data().effectN( 1 ).percent() );
    }

    return am;
  }
};

// Frostfire Bolt Spell =====================================================

// Used when glyphed Icy Veins is active
struct mini_frostfire_bolt_t : public mage_spell_t
{
  mini_frostfire_bolt_t( mage_t* p , int bolt_count = 1 ) :
    mage_spell_t( "mini_frostfire_bolt", p, p -> find_spell( 131081 ) )
  {
    background = true;
    //dual = true;
    base_costs[ RESOURCE_MANA ] = 0;

    if ( p -> sets.has_set_bonus( SET_PVP_4PC_CASTER ) )
      base_multiplier *= 1.05;

    if ( bolt_count < 3 )
    {
      execute_action = new mini_frostfire_bolt_t( p, bolt_count + 1 );
    }
  }

  void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );
    trigger_icicle_gain( s );
  }

  virtual timespan_t execute_time() const
  { return timespan_t::from_seconds( 0.25 ); }

  virtual timespan_t travel_time() const
  {
    timespan_t t = mage_spell_t::travel_time();
    return ( t > timespan_t::from_seconds( 0.75 ) ? timespan_t::from_seconds( 0.75 ) : t );
  }
};

// Cast by Frost T16 4pc bonus when Brain Freeze FFB is cast
struct frigid_blast_t : public mage_spell_t
{
  frigid_blast_t( mage_t* p ) :
    mage_spell_t( "frigid_blast", p, p -> find_spell( 145264 ) )
  {
    background = true;
    may_crit = true;
  }
};

struct frostfire_bolt_t : public mage_spell_t
{
  struct state_t : public action_state_t
  {
    bool mini_version;
    state_t( action_t* a, player_t* t ) : action_state_t( a, t ),
      mini_version( false ) { }

    std::ostringstream& debug_str( std::ostringstream& s )
    { action_state_t::debug_str( s ) << " mini_version=" << std::boolalpha << mini_version; return s; }

    void initialize()
    { action_state_t::initialize(); mini_version = false; }

    void copy_state( const action_state_t* o )
    {
      action_state_t::copy_state( o );
      mini_version = static_cast<const state_t&>( *o ).mini_version;
    }
  };

  mini_frostfire_bolt_t* mini_frostfire_bolt;
  frigid_blast_t* frigid_blast;

  frostfire_bolt_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frostfire_bolt", p, p -> find_spell( 44614 ) ),
    mini_frostfire_bolt( new mini_frostfire_bolt_t( p ) ),
    frigid_blast( new frigid_blast_t( p ) )
  {
    parse_options( NULL, options_str );

    may_hot_streak = true;
    base_execute_time += p -> glyphs.frostfire -> effectN( 1 ).time_value();

    add_child( mini_frostfire_bolt );

    if ( p -> sets.has_set_bonus( SET_PVP_4PC_CASTER ) )
      base_multiplier *= 1.05;

    if ( p -> sets.has_set_bonus( SET_T16_2PC_CASTER ) )
    {
      add_child( frigid_blast );
    }
  }

  virtual void snapshot_state( action_state_t* s, dmg_e type )
  {
    state_t& state = static_cast<state_t&>( *s );

    state.mini_version = p() -> glyphs.icy_veins -> ok() && p() -> buffs.icy_veins -> check();

    mage_spell_t::snapshot_state( s, type );
  }

  virtual action_state_t* new_state()
  { return new state_t( this, target ); }

  virtual double cost() const
  {
    if ( p() -> buffs.brain_freeze -> check() )
      return 0.0;

    return mage_spell_t::cost();
  }

  virtual timespan_t execute_time() const
  {
    if ( p() -> buffs.brain_freeze -> check() )
      return timespan_t::zero();

    return mage_spell_t::execute_time();
  }

  virtual void execute()
  {
    // Brain Freeze treats the target as frozen
    frozen = p() -> buffs.brain_freeze -> check() > 0;

    dual = p() -> glyphs.icy_veins -> ok() && p() -> buffs.icy_veins -> check();
    mage_spell_t::execute();
    dual = false;

    if ( result_is_hit( execute_state -> result ) )
    {
      double fof_proc_chance = p() -> buffs.fingers_of_frost -> data().effectN( 1 ).percent();
      if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
      {
        fof_proc_chance *= 1.2;
      }
      p() -> buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), fof_proc_chance );

      if ( p() -> glyphs.icy_veins -> ok() && p() -> buffs.icy_veins -> up() )
      {
        mini_frostfire_bolt -> schedule_execute( mini_frostfire_bolt -> get_state( execute_state ) );
      }
    }
    p() -> buffs.frozen_thoughts -> expire();
    if ( p() -> buffs.brain_freeze -> check() && p() -> sets.has_set_bonus( SET_T16_2PC_CASTER ) )
    {
      p() -> buffs.frozen_thoughts -> trigger();
    }
    // FIX ME: Instead of hardcoding 0.3, should use effect 2 of 145257
    if ( rng().roll( p() -> sets.set( SET_T16_4PC_CASTER ) -> effectN( 2 ).percent() ) )
    {
      frigid_blast -> schedule_execute();
    }
    p() -> buffs.brain_freeze -> expire();
  }

  virtual timespan_t travel_time() const
  {
    timespan_t t = mage_spell_t::travel_time();
    return ( t > timespan_t::from_seconds( 0.75 ) ? timespan_t::from_seconds( 0.75 ) : t );
  }

  virtual void impact( action_state_t* s )
  {
    if ( static_cast<const state_t&>( *s ).mini_version ) // Bail out if mini spells get casted
      return;

    mage_spell_t::impact( s );
    // If there are five Icicles, launch the oldest at this spell's target
    // Create an Icicle, stashing damage equal to mastery * value
    // Damage should be based on damage spell would have done without any
    // target-based damage increases or decreases, except Frostbolt debuff
    // Should also apply to mini version

    if ( result_is_hit( s -> result ) )
    {
      p() -> buffs.tier13_2pc -> trigger();

      trigger_ignite( s );
    }

    trigger_icicle_gain( s );
  }

  virtual double composite_crit_multiplier() const
  {
    double m = mage_spell_t::composite_crit_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }

  virtual double action_multiplier() const
  {
    double am = mage_spell_t::action_multiplier();

    if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
    {
      am *= 0.4;
    }

//FFB's damage can be increase by 2pT16 too
    if ( p() -> buffs.frozen_thoughts -> up() )
    {
      am *= ( 1.0 + p() -> buffs.frozen_thoughts -> data().effectN( 1 ).percent() );
    }

    return am;
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double tm = mage_spell_t::composite_target_multiplier( t );

    if ( td( t ) -> debuffs.pyromaniac -> check() )
    {
      tm *= 1.1;
    }

    return tm;
  }

};

// Frozen Orb Spell =========================================================

struct frozen_orb_bolt_t : public mage_spell_t
{
  frozen_orb_bolt_t( mage_t* p ) :
    mage_spell_t( "frozen_orb_bolt", p, p -> find_class_spell( "Frozen Orb" ) -> ok() ? p -> find_spell( 84721 ) : spell_data_t::not_found() )
  {
    aoe = -1;
    background = true;
    dual = true;
    cooldown -> duration = timespan_t::zero(); // dbc has CD of 6 seconds
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    double fof_proc_chance = p() -> buffs.fingers_of_frost -> data().effectN( 1 ).percent();

    if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
    {
      fof_proc_chance *= 1.2;
    }
    p() -> buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), fof_proc_chance );
  }
};

struct frozen_orb_t : public mage_spell_t
{
  frozen_orb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frozen_orb", p, p -> find_class_spell( "Frozen Orb" ) )
  {
    parse_options( NULL, options_str );

    hasted_ticks = false;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    num_ticks      = ( int ) ( data().duration() / base_tick_time );
    may_miss       = false;
    may_crit       = false;

    dynamic_tick_action = true;
    tick_action = new frozen_orb_bolt_t( p );
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    p() -> buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), 1 );
  }
};

// Ice Floes Spell ==========================================================

struct ice_floes_t : public mage_spell_t
{
  ice_floes_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "ice_floes", p, p -> talents.ice_floes )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> buffs.ice_floes -> trigger( 2 );
  }
};

// Mini Ice Lance Spell =====================================================

struct mini_ice_lance_t : public mage_spell_t
{

  mini_ice_lance_t( mage_t* p , int bolt_count = 1 ) :
    mage_spell_t( "mini_ice_lance", p, p -> find_spell( 30455 ) )
  {
    background = true;
    //dual = true;
    base_costs[ RESOURCE_MANA ] = 0;

    if ( p -> glyphs.ice_lance -> ok() )
      aoe = p -> glyphs.ice_lance -> effectN( 1 ).base_value() + 1;
    else if ( p -> glyphs.splitting_ice -> ok() )
      aoe = p -> glyphs.splitting_ice -> effectN( 1 ).base_value() + 1;

    if ( p -> glyphs.ice_lance -> ok() )
      base_aoe_multiplier *= p -> glyphs.ice_lance -> effectN( 2 ).percent();
    else if ( p -> glyphs.splitting_ice -> ok() )
      base_aoe_multiplier *= p -> glyphs.splitting_ice -> effectN( 2 ).percent();

    if ( bolt_count < 3 )
    {
      execute_action = new mini_ice_lance_t( p, bolt_count + 1 );
    }
  }

  virtual timespan_t execute_time() const
  { return timespan_t::from_seconds( 0.25 ); }

};

// Ice Lance Spell ==========================================================

struct ice_lance_t : public mage_spell_t
{
  struct state_t : public action_state_t
  {
    bool mini_version;
    state_t( action_t* a, player_t* t ) : action_state_t( a, t ),
      mini_version( false ) { }

    std::ostringstream& debug_str( std::ostringstream& s )
    { action_state_t::debug_str( s ) << " mini_version=" << std::boolalpha << mini_version; return s; }

    void initialize()
    { action_state_t::initialize(); mini_version = false; }

    void copy_state( const action_state_t* o )
    {
      action_state_t::copy_state( o );
      mini_version = static_cast<const state_t&>( *o ).mini_version;
    }
  };
  double fof_multiplier;
  mini_ice_lance_t* mini_ice_lance;

  ice_lance_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "ice_lance", p, p -> find_class_spell( "Ice Lance" ) ),
    fof_multiplier( 0 ),
    mini_ice_lance( new mini_ice_lance_t( p ) )
  {
    parse_options( NULL, options_str );

    if ( p -> glyphs.ice_lance -> ok() )
      aoe = p -> glyphs.ice_lance -> effectN( 1 ).base_value() + 1;
    else if ( p -> glyphs.splitting_ice -> ok() )
      aoe = p -> glyphs.splitting_ice -> effectN( 1 ).base_value() + 1;

    if ( p -> glyphs.ice_lance -> ok() )
      base_aoe_multiplier *= p -> glyphs.ice_lance -> effectN( 2 ).percent();
    else if ( p -> glyphs.splitting_ice -> ok() )
      base_aoe_multiplier *= p -> glyphs.splitting_ice -> effectN( 2 ).percent();

    fof_multiplier = p -> find_specialization_spell( "Fingers of Frost" ) -> ok() ? p -> find_spell( 44544 ) -> effectN( 2 ).percent() : 0.0;

    add_child( mini_ice_lance );
  }

  virtual int n_targets() const
  {
    // If we're shooting mini ice lances, the main ice lance action will always
    // be single targeted, since the mini ice lances will handle the aoe damage
    // due toe glyph of splitting ice
    if ( p() -> glyphs.icy_veins -> ok() && p() -> buffs.icy_veins -> check() )
      return 1;

    return mage_spell_t::n_targets();
  }

  virtual action_state_t* new_state()
  { return new state_t( this, target ); }

  virtual void execute()
  {
    // Ice Lance treats the target as frozen with FoF up
    frozen = p() -> buffs.fingers_of_frost -> check() > 0;

    mage_spell_t::execute();

    if ( static_cast<const state_t*>( execute_state ) -> mini_version )
      mini_ice_lance -> schedule_execute( mini_ice_lance -> get_state( execute_state ) );

    // Begin casting all Icicles at the target, beginning 0.25 seconds after the
    // Ice Lance cast with remaining Icicles launching at intervals of 0.75
    // seconds, both values adjusted by haste. Casting continues until all
    // Icicles are gone, including new ones that accumulate while they're being
    // fired. If target dies, Icicles stop. If Ice Lance is cast again, the
    // current sequence is interrupted and a new one begins.

    p() -> buffs.fingers_of_frost -> decrement();
    p() -> buffs.frozen_thoughts -> expire();
    trigger_icicle( p(), true );
  }

  virtual void impact( action_state_t* s )
  {
    if ( static_cast<const state_t&>( *s ).mini_version ) // Bail out if mini spells get casted
      return;

    mage_spell_t::impact( s );
  }

  virtual void snapshot_state( action_state_t* s, dmg_e type )
  {
    state_t& state = static_cast<state_t&>( *s );

    state.mini_version = p() -> glyphs.icy_veins -> ok() && p() -> buffs.icy_veins -> check();

    mage_spell_t::snapshot_state( s, type );
  }

  virtual double action_multiplier() const
  {
    double am = mage_spell_t::action_multiplier();

    if ( p() -> buffs.fingers_of_frost -> up() )
    {
      am *= 4.0; // Built in bonus against frozen targets
      am *= 1.0 + fof_multiplier; // Buff from Fingers of Frost
    }

    if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
    {
      am *= 0.4;
    }

    if ( p() -> sets.has_set_bonus( SET_T14_2PC_CASTER ) )
    {
      am *= 1.12;
    }

    if ( p() -> buffs.frozen_thoughts -> up() )
    {
      am *= ( 1.0 + p() -> buffs.frozen_thoughts -> data().effectN( 1 ).percent() );
    }

    return am;
  }
};

// Icy Veins Spell ==========================================================

struct icy_veins_t : public mage_spell_t
{
  icy_veins_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "icy_veins", p, p -> find_class_spell( "Icy Veins" ) )
  {
    check_spec( MAGE_FROST );
    parse_options( NULL, options_str );
    harmful = false;

    if ( player -> sets.has_set_bonus( SET_T14_4PC_CASTER ) )
    {
      cooldown -> duration *= 0.5;
    }
  }

  virtual void update_ready( timespan_t cd_override )
  {
    cd_override = cooldown -> duration;

    if ( p() -> sets.has_set_bonus( SET_T13_4PC_CASTER ) )
      cd_override *= ( 1.0 - p() -> buffs.tier13_2pc -> check() * p() -> spells.stolen_time -> effectN( 1 ).base_value() );

    mage_spell_t::update_ready( cd_override );
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> buffs.icy_veins -> trigger();
  }
};

// Inferno Blast Spell ======================================================

struct inferno_blast_t : public mage_spell_t
{
  int max_spread_targets;
  inferno_blast_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "inferno_blast", p, p -> find_class_spell( "Inferno Blast" ) )
  {
    parse_options( NULL, options_str );
    may_hot_streak = true;
    cooldown = p -> cooldowns.inferno_blast;
    max_spread_targets = 3;
    max_spread_targets += p -> glyphs.inferno_blast -> ok() ? p -> glyphs.inferno_blast -> effectN( 1 ).base_value() : 0;
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      mage_td_t* this_td = td( s -> target );

      dot_t* ignite_dot     = this_td -> dots.ignite;
      dot_t* combustion_dot = this_td -> dots.combustion;
      dot_t* pyroblast_dot  = this_td -> dots.pyroblast;

      int spread_remaining = max_spread_targets;

      for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
      {
        player_t* t = sim -> actor_list[ i ];

        if ( t -> is_sleeping() || ! t -> is_enemy() || ( t == s -> target ) )
          continue;

        if ( ignite_dot -> ticking )
        {
          if ( td( t ) -> dots.ignite -> ticking ) //is already ticking on target spell, so merge it
          {
            p() -> active_ignite -> trigger( t, ignite_dot -> state -> result_amount * ignite_dot -> ticks() );
          }
          else
          {
            ignite_dot -> copy( t );
          }
        }
        if ( combustion_dot -> ticking ) //just copy, regardless of target dots. This is the actual ingame behavior as of 22.03.13
        {
          combustion_dot -> copy( t );
        }
        if ( pyroblast_dot -> ticking ) //just copy, regardless of target dots. This is the actual ingame behavior as of 22.03.13
        {
          pyroblast_dot -> copy( t );
        }

        if ( --spread_remaining == 0 )
          break;
      }

      trigger_ignite( s ); //Assuming that the ignite from inferno_blast isn't spread by itself
    }
  }

  virtual double crit_chance( double /* crit */, int /* delta_level */ ) const
  {
    // Inferno Blast always crits
    return 1.0;
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double tm = mage_spell_t::composite_target_multiplier( t );

    if ( td( t ) -> debuffs.pyromaniac -> check() )
    {
      tm *= 1.1;
    }

    return tm;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    if ( p() -> sets.has_set_bonus( SET_T16_4PC_CASTER ) )
    {
      p() -> buffs.fiery_adept -> trigger();
    }
  }
};

// Living Bomb Spell ========================================================

struct living_bomb_explosion_t : public mage_spell_t
{
  living_bomb_explosion_t( mage_t* p ) :
    mage_spell_t( "living_bomb_explosion", p, p -> find_spell( p -> talents.living_bomb -> effectN( 2 ).base_value() ) )
  {
    aoe = -1;
    background = true;

    base_multiplier *= 4;
  }

  virtual resource_e current_resource() const
  { return RESOURCE_NONE; }
};

struct living_bomb_t : public mage_spell_t
{
  living_bomb_explosion_t* explosion_spell;

  living_bomb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "living_bomb", p, p -> talents.living_bomb ),
    explosion_spell( new living_bomb_explosion_t( p ) )
  {
    parse_options( NULL, options_str );

    dot_behavior = DOT_REFRESH;
    hasted_ticks = true;

    trigger_gcd = timespan_t::from_seconds( 1.0 );

    add_child( explosion_spell );
  }

  virtual void impact( action_state_t* s )
  {
    if ( result_is_hit( s -> result ) )
    {
      dot_t* dot = get_dot( s -> target );
      if ( dot -> ticking && dot -> remains() < dot -> current_action -> base_tick_time )
      {
        explosion_spell -> execute();
        mage_t& p = *this -> p();
        p.active_living_bomb_targets--;
      }
    }

    mage_spell_t::impact( s );


    if ( p() -> specialization() == MAGE_FIRE && result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs.pyromaniac -> trigger( 1, buff_t::DEFAULT_VALUE(), 1 );
    }
  }

  virtual void tick( dot_t* d )
  {
    mage_spell_t::tick( d );

    if ( p() -> last_bomb_target == d -> state -> target )
      p() -> buffs.brain_freeze -> trigger();
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );


    explosion_spell -> execute();
    mage_t& p = *this -> p();
    p.active_living_bomb_targets--;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      mage_t& p = *this -> p();
      p.active_living_bomb_targets++;
      p.last_bomb_target = execute_state -> target;
    }
  }

  virtual bool ready()
  {
    mage_t& p = *this -> p();

    assert( p.active_living_bomb_targets <= 3 && p.active_living_bomb_targets >= 0 );

    if ( p.active_living_bomb_targets == 3 )
    {
      return false;
    }

    return mage_spell_t::ready();
  }
};

// Mage Armor Spell =========================================================

struct mage_armor_t : public mage_spell_t
{
  mage_armor_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "mage_armor", p, p -> find_class_spell( "Mage Armor" ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> buffs.frost_armor -> expire();
    p() -> buffs.molten_armor -> expire();
    p() -> buffs.mage_armor -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.mage_armor -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Mirror Image Spell =======================================================

struct mirror_image_t : public mage_spell_t
{
  mirror_image_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "mirror_image", p, p -> find_spell( 55342 ) )
  {
    parse_options( NULL, options_str );
    num_ticks = 0;
    harmful = false;
  }

  virtual void init()
  {
    mage_spell_t::init();

    for ( unsigned i = 0; i < sizeof_array( p() -> pets.mirror_images ); i++ )
    {
      stats -> add_child( p() -> pets.mirror_images[ i ] -> get_stats( "arcane_blast" ) );
      stats -> add_child( p() -> pets.mirror_images[ i ] -> get_stats( "fire_blast" ) );
      stats -> add_child( p() -> pets.mirror_images[ i ] -> get_stats( "fireball" ) );
      stats -> add_child( p() -> pets.mirror_images[ i ] -> get_stats( "frostbolt" ) );
    }
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    if ( p() -> pets.mirror_images[ 0 ] )
    {
      for ( unsigned i = 0; i < sizeof_array( p() -> pets.mirror_images ); i++ )
      {
        p() -> pets.mirror_images[ i ] -> summon( data().duration() );
      }
    }
  }
};

// Molten Armor Spell =======================================================

struct molten_armor_t : public mage_spell_t
{
  molten_armor_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "molten_armor", p, p -> find_class_spell( "Molten Armor" ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> buffs.frost_armor -> expire();
    p() -> buffs.mage_armor -> expire();
    p() -> buffs.molten_armor -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.molten_armor -> check() )
      return false;

    return mage_spell_t::ready();
  }
};



// Nether Tempest Cleave ====================================================
//FIXME_cleave: take actual distances between main_target and cleave_target into account
struct nether_tempest_cleave_t: public mage_spell_t
{
  player_t* main_target;

  nether_tempest_cleave_t( mage_t* p ) :
    mage_spell_t( "nether_tempest_cleave", p, p -> find_spell( 114954 ) ),
    main_target( nullptr )
  {
    background = true;
  }

  virtual resource_e current_resource() const
  { return RESOURCE_NONE; }

  virtual void execute()
  {
    assert( main_target ); // main target needs to be set to parent actions target
    target = main_target;

    std::vector< player_t* >& tl = target_list();

    if ( tl.size() <= 1 )
      return; // single target; back out completely

    // obtain random target until it is not equal to main target
    while ( target == main_target )
    {
      // Randomly select target index
      unsigned t = static_cast<unsigned>( rng().range( 0, as<double>( tl.size() ) ) );
      if ( t >= tl.size() ) --t; // dsfmt range should not give a value actually equal to max, but be paranoid
      target = tl[ t ];
    }

    mage_spell_t::execute();
  }

  virtual timespan_t travel_time() const
  {
    return timespan_t::from_seconds( travel_speed ); // assuming 1 yard to the cleave target
  }

};

// Nether Tempest ===========================================================

struct nether_tempest_t : public mage_spell_t
{
  nether_tempest_cleave_t *add_cleave;

  nether_tempest_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "nether_tempest", p, p -> talents.nether_tempest ),
    add_cleave( nullptr )
  {
    parse_options( NULL, options_str );
    add_cleave = new nether_tempest_cleave_t( p );
    add_child( add_cleave );
  }

  void execute()
  {
    mage_spell_t::execute();
    if ( result_is_hit( execute_state -> result ) )
      p() -> last_bomb_target = execute_state -> target;
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    if ( p() -> specialization() == MAGE_FIRE && result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs.pyromaniac -> trigger( 1, buff_t::DEFAULT_VALUE(), 1 );
    }
  }

  virtual void tick( dot_t* d )
  {
    mage_spell_t::tick( d );

    add_cleave -> main_target = target;
    add_cleave -> execute();

    if ( d -> state -> target == p() -> last_bomb_target )
      p() -> buffs.brain_freeze -> trigger();
  }
};

// Presence of Mind Spell ===================================================

struct presence_of_mind_t : public mage_spell_t
{
  presence_of_mind_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "presence_of_mind", p, p -> talents.presence_of_mind )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> buffs.presence_of_mind -> trigger();
  }
};

// Pyroblast Spell ==========================================================

struct pyroblast_t : public mage_spell_t
{
  pyroblast_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "pyroblast", p, p -> find_class_spell( "Pyroblast" ) )
  {
    parse_options( NULL, options_str );
    may_hot_streak = true;
    dot_behavior = DOT_REFRESH;
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    mage_spell_t::schedule_execute( state );

    p() -> buffs.pyroblast -> up();
  }

  virtual timespan_t execute_time() const
  {
    if ( p() -> buffs.pyroblast -> check() )
    {
      return timespan_t::zero();
    }

    return mage_spell_t::execute_time();
  }

  virtual double cost() const
  {
    if ( p() -> buffs.pyroblast -> check() )
      return 0.0;

    return mage_spell_t::cost();
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    if ( p() -> buffs.pyroblast -> check() && p() -> sets.has_set_bonus( SET_T16_2PC_CASTER ) )
    {
      p() -> buffs.potent_flames -> trigger();
    }
    p() -> buffs.pyroblast -> expire();
    p() -> buffs.fiery_adept -> expire();
  }

  virtual timespan_t travel_time() const
  {
    timespan_t t = mage_spell_t::travel_time();
    return ( t > timespan_t::from_seconds( 0.75 ) ? timespan_t::from_seconds( 0.75 ) : t );
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    trigger_ignite( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> buffs.tier13_2pc -> trigger();
    }
  }

  virtual double composite_crit_multiplier() const
  {
    double m = mage_spell_t::composite_crit_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }

  virtual double composite_crit() const
  {
    double c = mage_spell_t::composite_crit();

    c += p() -> sets.set( SET_T15_4PC_CASTER ) -> effectN( 2 ).percent();

    if ( p() -> buffs.fiery_adept -> check() )
      c += 100.0;

    return c;
  }

  virtual double action_multiplier() const
  {
    double am = mage_spell_t::action_multiplier();

    if ( p() -> buffs.pyroblast -> up() )
    {
      am *= 1.25;
    }

    if ( p() -> sets.has_set_bonus( SET_T14_2PC_CASTER ) )
    {
      am *= 1.08;
    }

    return am;
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double tm = mage_spell_t::composite_target_multiplier( t );

    if ( td( t ) -> debuffs.pyromaniac -> check() )
    {
      tm *= 1.1;
    }

    return tm;
  }
};

// Rune of Power ============================================================

struct rune_of_power_t : public mage_spell_t
{
  rune_of_power_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "rune_of_power", p, p -> talents.rune_of_power )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    // Assume they're always in it
    p() -> buffs.rune_of_power -> trigger();
  }
};

// Scorch Spell =============================================================

struct scorch_t : public mage_spell_t
{
  scorch_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "scorch", p, p -> find_specialization_spell( "Scorch" ) )
  {
    parse_options( NULL, options_str );

    may_hot_streak = true;
    consumes_ice_floes = false;

    if ( p -> sets.has_set_bonus( SET_PVP_4PC_CASTER ) )
      base_multiplier *= 1.05;
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_ignite( s );
      if ( p() -> specialization() == MAGE_FROST )
      {
        double fof_proc_chance = p() -> buffs.fingers_of_frost -> data().effectN( 3 ).percent();
        if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
        {
          fof_proc_chance *= 1.2;
        }
        p() -> buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), fof_proc_chance );
      }
    }
  }

  double composite_crit_multiplier() const
  {
    double m = mage_spell_t::composite_crit_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }

  virtual bool usable_moving() const
  { return true; }

  // delay 0.25s the removal of heating up on non-critting spell with travel time or scorch
  virtual void expire_heating_up()
  {
    // we should delay heating up removal here
    mage_t* p = this -> p();
    if ( sim -> log ) sim -> out_log << "Heating up delay by 0.25s";
    p -> buffs.heating_up -> expire( timespan_t::from_millis( 250 ) );
  }
};

// Slow Spell ===============================================================

struct slow_t : public mage_spell_t
{
  slow_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "slow", p, p -> find_class_spell( "Slow" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    td( s -> target ) -> debuffs.slow -> trigger();
  }
};

// Time Warp Spell ==========================================================

struct time_warp_t : public mage_spell_t
{
  time_warp_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "time_warp", p, p -> find_class_spell( "Time Warp" ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if ( p -> buffs.exhaustion -> check() || p -> is_pet() || p -> is_enemy() )
        continue;

      p -> buffs.bloodlust -> trigger(); // Bloodlust and Timewarp are the same
      p -> buffs.exhaustion -> trigger();
    }
  }

  virtual bool ready()
  {
    if ( player -> buffs.exhaustion -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Water Elemental Spell ====================================================

struct summon_water_elemental_t : public mage_spell_t
{
  summon_water_elemental_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "water_elemental", p, p -> find_class_spell( "Summon Water Elemental" ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
    consumes_ice_floes = false;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> pets.water_elemental -> summon();
  }

  virtual bool ready()
  {
    if ( ! mage_spell_t::ready() )
      return false;

    return ! ( p() -> pets.water_elemental && ! p() -> pets.water_elemental -> is_sleeping() );
  }
};

// Choose Rotation ==========================================================

struct choose_rotation_t : public action_t
{
  double evocation_target_mana_percentage;
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

    option_t options[] =
    {
      opt_timespan( "cooldown", ( cooldown -> duration ) ),
      opt_float( "evocation_pct", evocation_target_mana_percentage ),
      opt_int( "force_dps", force_dps ),
      opt_int( "force_dpm", force_dpm ),
      opt_timespan( "final_burn_offset", ( final_burn_offset ) ),
      opt_float( "oom_offset", oom_offset ),
      opt_null()
    };
    parse_options( options, options_str );

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
        sim -> out_log.printf( "%f burn mps, %f time to die", ( p -> rotation.dps_mana_loss / p -> rotation.dps_time.total_seconds() ) - ( p -> rotation.mana_gain / sim -> current_time.total_seconds() ), sim -> target -> time_to_die().total_seconds() );
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

    timespan_t ttd = sim -> target -> time_to_die();
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
        if ( p -> resources.current[ RESOURCE_MANA ] / p -> resources.max[ RESOURCE_MANA ] < evocation_target_mana_percentage / 100.0 )
        {
          if ( sim -> log ) sim -> out_log.printf( "%s switches to DPM spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPM;
        }
      }
      else
      {
        // We're going until OOM, stop when we can no longer cast full stack AB (approximately, 4 stack with AP can be 6177)
        if ( p -> resources.current[ RESOURCE_MANA ] < 6200 )
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
    // NOTE this delierately avoids calling the supreclass ready method;
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
/*
// Alter Time Spell =========================================================

struct alter_time_t : public mage_spell_t
{
  alter_time_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "alter_time_activate", p, p -> find_spell( 108978 ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    // Buff trigger / Snapshot happens before resource is spent
    if ( p() -> buffs.alter_time -> check() > 0 )
      p() -> buffs.alter_time -> expire();
    else
      p() -> buffs.alter_time -> trigger();

    mage_spell_t::execute();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.alter_time -> check() ) // Allow execution if the buff is up, even tough cooldown already is started.
    {
      timespan_t cd_ready = cooldown -> ready;

      cooldown -> ready = sim -> current_time;

      bool ready = mage_spell_t::ready();

      cooldown -> ready = cd_ready;

      return ready;
    }

    return mage_spell_t::ready();
  }
};
*/
// Incanters_ward Spell =====================================================

struct incanters_ward_t : public mage_spell_t
{
  // Option used to specify the duration of Incanter's Ward and the strength
  // of the subsequent spell power buff from Incanter's Absorption.
  // If 0 <= break_after <= 1, then Incanter's Ward breaks after 8 seconds
  //   and ``break_after'' represents the percentage of the shield that
  //   has been consumed.
  // If 1 < break_after, then Incanter's Ward breaks after 8 / break_after
  // seconds. For example:
  //   - break_after = 2, Incanter's Ward breaks after 4 seconds;
  //   - break_after = 4, Incanter's Ward breaks after 2 seconds;
  //   - break_after = 8, Incanter's Ward breaks after 1 second.
  // If break_after < 0, then, it has no effect on Incanter's Ward
  double break_after;

  incanters_ward_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "incanters_ward", p, p -> talents.incanters_ward ), break_after ( -1.0 )
  {
    option_t options[] =
    {
      opt_float( "break_after", break_after ),
      opt_null()
    };
    parse_options( options, options_str );
    ( static_cast<buffs::incanters_ward_t*>( p -> buffs.incanters_ward ) ) -> set_break_after( break_after );
    harmful = false;

    base_dd_min = base_dd_max = 0.0;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    p() -> buffs.incanters_ward -> trigger();
    p() -> invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  }
};


// ==========================================================================
// Mage Character Definition
// ==========================================================================

// mage_td_t ================================================================

mage_td_t::mage_td_t( player_t* target, mage_t* mage ) :
  actor_pair_t( target, mage ),
  dots( dots_t() ),
  debuffs( debuffs_t() )
{
  dots.combustion     = target -> get_dot( "combustion",     mage );
  dots.flamestrike    = target -> get_dot( "flamestrike",    mage );
  dots.frost_bomb     = target -> get_dot( "frost_bomb",     mage );
  dots.ignite         = target -> get_dot( "ignite",         mage );
  dots.living_bomb    = target -> get_dot( "living_bomb",    mage );
  dots.nether_tempest = target -> get_dot( "nether_tempest", mage );
  dots.pyroblast      = target -> get_dot( "pyroblast",      mage );

  debuffs.frostbolt = buff_creator_t( *this, "frostbolt" ).spell( mage -> spec.frostbolt ).duration( timespan_t::from_seconds( 15.0 ) ).max_stack( 3 );
  debuffs.pyromaniac = buff_creator_t( *this, "pyromaniac" ).spell( mage -> spec.pyromaniac ).duration( timespan_t::from_seconds( 15.0 ) ).max_stack( 1 );
  debuffs.slow = buff_creator_t( *this, "slow" ).spell( mage -> spec.slow );
}

// mage_t::create_action ====================================================

action_t* mage_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  if ( name == "arcane_barrage"    ) return new          arcane_barrage_t( this, options_str );
  if ( name == "arcane_blast"      ) return new            arcane_blast_t( this, options_str );
  if ( name == "arcane_brilliance" ) return new       arcane_brilliance_t( this, options_str );
  if ( name == "arcane_explosion"  ) return new        arcane_explosion_t( this, options_str );
  if ( name == "arcane_missiles"   ) return new         arcane_missiles_t( this, options_str );
  if ( name == "arcane_power"      ) return new            arcane_power_t( this, options_str );
  if ( name == "blink"             ) return new                   blink_t( this, options_str );
  if ( name == "blizzard"          ) return new                blizzard_t( this, options_str );
  if ( name == "choose_rotation"   ) return new         choose_rotation_t( this, options_str );
  if ( name == "cold_snap"         ) return new               cold_snap_t( this, options_str );
  if ( name == "combustion"        ) return new              combustion_t( this, options_str );
  if ( name == "cone_of_cold"      ) return new            cone_of_cold_t( this, options_str );
  if ( name == "counterspell"      ) return new            counterspell_t( this, options_str );
  if ( name == "dragons_breath"    ) return new          dragons_breath_t( this, options_str );
  if ( name == "evocation"         ) return new               evocation_t( this, options_str );
  if ( name == "fire_blast"        ) return new              fire_blast_t( this, options_str );
  if ( name == "fireball"          ) return new                fireball_t( this, options_str );
  if ( name == "flamestrike"       ) return new             flamestrike_t( this, options_str );
  if ( name == "frost_armor"       ) return new             frost_armor_t( this, options_str );
  if ( name == "frost_bomb"        ) return new              frost_bomb_t( this, options_str );
  if ( name == "frostbolt"         ) return new               frostbolt_t( this, options_str );
  if ( name == "frostfire_bolt"    ) return new          frostfire_bolt_t( this, options_str );
  if ( name == "frozen_orb"        ) return new              frozen_orb_t( this, options_str );
  if ( name == "ice_floes"         ) return new               ice_floes_t( this, options_str );
  if ( name == "ice_lance"         ) return new               ice_lance_t( this, options_str );
  if ( name == "icy_veins"         ) return new               icy_veins_t( this, options_str );
  if ( name == "inferno_blast"     ) return new           inferno_blast_t( this, options_str );
  if ( name == "living_bomb"       ) return new             living_bomb_t( this, options_str );
  if ( name == "mage_armor"        ) return new              mage_armor_t( this, options_str );
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
  if ( name == "molten_armor"      ) return new            molten_armor_t( this, options_str );
  if ( name == "nether_tempest"    ) return new          nether_tempest_t( this, options_str );
  if ( name == "presence_of_mind"  ) return new        presence_of_mind_t( this, options_str );
  if ( name == "pyroblast"         ) return new               pyroblast_t( this, options_str );
  if ( name == "rune_of_power"     ) return new           rune_of_power_t( this, options_str );
  if ( name == "invocation"        )
  {
    sim -> errorf( "The behavior of \"invocation\" has been subsumed into evocation." );
    return new evocation_t( this, options_str );
  }
  if ( name == "scorch"            ) return new                  scorch_t( this, options_str );
  if ( name == "slow"              ) return new                    slow_t( this, options_str );
  if ( name == "time_warp"         ) return new               time_warp_t( this, options_str );
  if ( name == "water_elemental"   ) return new  summon_water_elemental_t( this, options_str );
  //if ( name == "alter_time"        ) return new              alter_time_t( this, options_str );
  if ( name == "incanters_ward"    ) return new          incanters_ward_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// mage_t::create_pet =======================================================

pet_t* mage_t::create_pet( const std::string& pet_name,
                           const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "water_elemental" ) return new pets::water_elemental_pet_t( sim, this );

  return 0;
}

// mage_t::create_pets ======================================================

void mage_t::create_pets()
{
  pets.water_elemental = create_pet( "water_elemental" );

  for ( unsigned i = 0; i < sizeof_array( pets.mirror_images ); i++ )
  {
    pets.mirror_images[ i ] = new pets::mirror_image_pet_t( sim, this );
    if ( i > 0 )
    {
      pets.mirror_images[ i ] -> quiet = 1;
    }
  }
}

// mage_t::init_spells ======================================================

void mage_t::init_spells()
{
  player_t::init_spells();

  // Talents
  talents.blazing_speed      = find_talent_spell( "Blazing Speed" );
  talents.cauterize          = find_talent_spell( "Cauterize" );
  talents.cold_snap          = find_talent_spell( "Cold Snap" );
  talents.frostjaw           = find_talent_spell( "Frostjaw" );
  talents.frost_bomb         = find_talent_spell( "Frost Bomb" );
  talents.greater_invis      = find_talent_spell( "Greater Invisibility" );
  talents.ice_barrier        = find_talent_spell( "Ice Barrier" );
  talents.ice_floes          = find_talent_spell( "Ice Floes" );
  talents.ice_ward           = find_talent_spell( "Ice Ward" );
  talents.incanters_ward     = find_talent_spell( "Incanter's Ward" );
  talents.invocation         = find_talent_spell( "Invocation" );
  talents.living_bomb        = find_talent_spell( "Living Bomb" );
  talents.nether_tempest     = find_talent_spell( "Nether Tempest" );
  talents.presence_of_mind   = find_talent_spell( "Presence of Mind" );
  talents.ring_of_frost      = find_talent_spell( "Ring of Frost" );
  talents.rune_of_power      = find_talent_spell( "Rune of Power" );
  talents.scorch             = find_talent_spell( "Scorch" );
  talents.temporal_shield    = find_talent_spell( "Temporal Shield" );

  // Passive Spells
  passives.nether_attunement = find_specialization_spell( "Nether Attunement" ); // BUG: Not in spell lists at present.
  passives.nether_attunement = ( find_spell( 117957 ) -> is_level( level ) ) ? find_spell( 117957 ) : spell_data_t::not_found();
  passives.shatter           = find_specialization_spell( "Shatter" ); // BUG: Doesn't work at present as Shatter isn't tagged as a spec of Frost.
  passives.shatter           = ( find_spell( 12982 ) -> is_level( level ) ) ? find_spell( 12982 ) : spell_data_t::not_found();

  // Spec Spells
  spec.arcane_charge         = find_specialization_spell( "Arcane Charge" );
  spells.arcane_charge_arcane_blast = spec.arcane_charge -> ok() ? find_spell( 36032 ) : spell_data_t::not_found();

  spec.slow                  = find_class_spell( "Slow" );

  spec.brain_freeze          = find_specialization_spell( "Brain Freeze" );
  spec.critical_mass         = find_specialization_spell( "Critical Mass" );
  spec.fingers_of_frost      = find_specialization_spell( "Fingers of Frost" );
  spec.frostbolt             = find_specialization_spell( "Frostbolt" );
  spec.pyromaniac            = find_specialization_spell( "Pyromaniac" );

  // Mastery
  spec.icicles               = find_mastery_spell( MAGE_FROST );
  spec.ignite                = find_mastery_spell( MAGE_FIRE );
  spec.mana_adept            = find_mastery_spell( MAGE_ARCANE );

  spells.stolen_time         = spell_data_t::find( 105791, "Stolen Time" );
  spells.icicles_driver      = find_spell( 148012 );

  // Glyphs
  glyphs.arcane_brilliance   = find_glyph_spell( "Glyph of Arcane Brilliance" );
  glyphs.arcane_power        = find_glyph_spell( "Glyph of Arcane Power" );
  glyphs.combustion          = find_glyph_spell( "Glyph of Combustion" );
  glyphs.frostfire           = find_glyph_spell( "Glyph of Frostfire" );
  glyphs.ice_lance           = find_glyph_spell( "Glyph of Ice Lance" );
  glyphs.icy_veins           = find_glyph_spell( "Glyph of Icy Veins" );
  glyphs.inferno_blast       = find_glyph_spell( "Glyph of Inferno Blast" );
  glyphs.living_bomb         = find_glyph_spell( "Glyph of Living Bomb" );
  glyphs.loose_mana          = find_glyph_spell( "Glyph of Loose Mana" );
  glyphs.mirror_image        = find_glyph_spell( "Glyph of Mirror Image" );
  glyphs.splitting_ice       = find_glyph_spell( "Glyph of Splitting Ice" );

  static const set_bonus_description_t set_bonuses =
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
    { 105788, 105790,     0,     0,     0,     0,     0,     0 }, // Tier13
    { 123097, 123101,     0,     0,     0,     0,     0,     0 }, // Tier14
    { 138316, 138376,     0,     0,     0,     0,     0,     0 }, // Tier15
    { 145251, 145257,     0,     0,     0,     0,     0,     0 }, // Tier16
  };

  sets.register_spelldata( set_bonuses );

  // Active spells
  if ( spec.ignite -> ok()  ) active_ignite = new ignite_t( this );
  if ( spec.icicles -> ok() ) icicle = new icicle_t( this );

}

// mage_t::init_base ========================================================

void mage_t::init_base_stats()
{
  player_t::init_base_stats();

  base.spell_power_per_intellect = 1.0;

  //base.stats.attack_power = -10; Removed in WoD
  base.attack_power_per_strength = 1.0;

  diminished_kfactor    = 0.009830;
  diminished_dodge_cap = 0.006650;
  diminished_parry_cap = 0.006650;
}

// mage_t::init_scaling =====================================================

void mage_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_SPIRIT ] = false;
}

// mage_t::init_buffs =======================================================

void mage_t::create_buffs()
{
  player_t::create_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buffs.arcane_charge        = buff_creator_t( this, "arcane_charge", spec.arcane_charge )
                               .max_stack( find_spell( 36032 ) -> max_stacks() )
                               .duration( find_spell( 36032 ) -> duration() );
  buffs.arcane_missiles      = buff_creator_t( this, "arcane_missiles", find_class_spell( "Arcane Missiles" ) -> ok() ? find_spell( 79683 ) : spell_data_t::not_found() ).chance( 0.3 );
  buffs.arcane_power         = new buffs::arcane_power_t( this );
  buffs.brain_freeze         = buff_creator_t( this, "brain_freeze", spec.brain_freeze )
                               .duration( find_spell( 57761 ) -> duration() )
                               .default_value( spec.brain_freeze -> effectN( 1 ).percent() )
                               .chance( spec.brain_freeze      -> ok() ?
                                      ( talents.nether_tempest -> ok() ? 0.09 :
                                      ( talents.living_bomb    -> ok() ? 0.25 :
                                      ( talents.frost_bomb     -> ok() ? 1.00 : 0.0 ) ) ) : 0 );

  buffs.fingers_of_frost     = buff_creator_t( this, "fingers_of_frost", find_spell( 112965 ) ).chance( find_spell( 112965 ) -> effectN( 1 ).percent() )
                               .duration( timespan_t::from_seconds( 15.0 ) )
                               .max_stack( 2 );
  buffs.frost_armor          = buff_creator_t( this, "frost_armor", find_spell( 7302 ) ).add_invalidate( CACHE_MULTISTRIKE );
  buffs.icy_veins            = new buffs::icy_veins_t( this );
  buffs.ice_floes            = buff_creator_t( this, "ice_floes", talents.ice_floes );
  buffs.invokers_energy      = buff_creator_t( this, "invokers_energy", find_spell( 116257 ) )
                               .chance( talents.invocation -> ok() ? 1.0 : 0 )
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.mage_armor           = stat_buff_creator_t( this, "mage_armor" ).spell( find_spell( 6117 ) );
  buffs.molten_armor         = buff_creator_t( this, "molten_armor", find_spell( 30482 ) ).add_invalidate( CACHE_SPELL_CRIT );
  buffs.presence_of_mind     = buff_creator_t( this, "presence_of_mind", talents.presence_of_mind ).duration( timespan_t::zero() ).activated( true );
  buffs.rune_of_power        = buff_creator_t( this, "rune_of_power", find_spell( 116014 ) )
                               .duration( timespan_t::from_seconds( 60 ) )
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.heating_up           = buff_creator_t( this, "heating_up", find_class_spell( "Pyroblast" ) -> ok() ? find_spell( 48107 ) : spell_data_t::not_found() );
  buffs.pyroblast            = buff_creator_t( this, "pyroblast",  find_class_spell( "Pyroblast" ) -> ok() ? find_spell( 48108 ) : spell_data_t::not_found() );

  buffs.tier13_2pc           = stat_buff_creator_t( this, "tier13_2pc" )
                               .spell( find_spell( 105785 ) )
                               .chance( sets.has_set_bonus( SET_T13_2PC_CASTER ) ? 0.5 : 0.0 );

  //buffs.alter_time           = new buffs::alter_time_t( this );
  buffs.incanters_ward       = new buffs::incanters_ward_t( this );
  buffs.incanters_absorption  = buff_creator_t( this, "incanters_absorption" )
                                .spell( find_spell( 116267 ) )
                                .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.tier15_2pc_crit      = stat_buff_creator_t( this, "tier15_2pc_crit", find_spell( 138317 ) )
                               .add_stat( STAT_CRIT_RATING, find_spell( 138317 ) -> effectN( 1 ).base_value() );
  buffs.tier15_2pc_haste     = stat_buff_creator_t( this, "tier15_2pc_haste", find_spell( 138317 ) )
                               .add_stat( STAT_HASTE_RATING, find_spell( 138317 ) -> effectN( 1 ).base_value() );
  buffs.tier15_2pc_mastery   = stat_buff_creator_t( this, "tier15_2pc_mastery", find_spell( 138317 ) )
                               .add_stat( STAT_MASTERY_RATING, find_spell( 138317 ) -> effectN( 1 ).base_value() );

  buffs.profound_magic       = buff_creator_t( this, "profound_magic" )
                               .spell( find_spell( 145252 ) );
  buffs.potent_flames        = stat_buff_creator_t( this, "potent_flames" )
                               .spell( find_spell( 145254 ) );
  buffs.frozen_thoughts      = buff_creator_t( this, "frozen_thoughts" )
                               .spell( find_spell( 146557 ) );
  buffs.fiery_adept          = buff_creator_t( this, "fiery_adept" )
                               .spell( find_spell( 145261 ) )
                               .chance( 1.0 );
}

// mage_t::init_gains =======================================================

void mage_t::init_gains()
{
  player_t::init_gains();

  gains.evocation              = get_gain( "evocation"              );
  gains.incanters_ward_passive = get_gain( "incanters_ward_passive" );
  gains.rune_of_power          = get_gain( "rune_of_power"          );
}

// mage_t::init_procs =======================================================

void mage_t::init_procs()
{
  player_t::init_procs();

  procs.test_for_crit_hotstreak = get_proc( "test_for_crit_hotstreak" );
  procs.crit_for_hotstreak      = get_proc( "crit_test_hotstreak"     );
  procs.hotstreak               = get_proc( "hotstreak"               );
}

// mage_t::init_uptimes =====================================================

void mage_t::init_benefits()
{
  player_t::init_benefits();

  for ( unsigned i = 0; i < sizeof_array( benefits.arcane_charge ); ++i )
  {
    benefits.arcane_charge[ i ] = get_benefit( "Arcane Charge " + util::to_string( i )  );
  }
  benefits.dps_rotation      = get_benefit( "dps rotation"    );
  benefits.dpm_rotation      = get_benefit( "dpm rotation"    );
  benefits.water_elemental   = get_benefit( "water_elemental" );
}

void mage_t::add_action( std::string action, std::string options, std::string alist )
{
  add_action( find_talent_spell( action ) -> ok() ? find_talent_spell( action ) : find_class_spell( action ), options, alist );
}

void mage_t::add_action( const spell_data_t* s, std::string options, std::string alist )
{
  std::string *str = ( alist == "default" ) ? &action_list_str : &( get_action_priority_list( alist ) -> action_list_str );
  if ( s -> ok() )
  {
    *str += "/" + dbc::get_token( s -> id() );
    if ( ! options.empty() ) *str += "," + options;
  }
}

// mage_t::init_actions =====================================================

void mage_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    std::string& precombat = get_action_priority_list( "precombat" ) -> action_list_str;
    std::string& aoe_list_str = get_action_priority_list( "aoe" ) -> action_list_str;
    std::string& st_list_str = get_action_priority_list( "single_target" ) -> action_list_str;

    if ( level >= 80 )
    {
      if ( sim -> allow_flasks )
      {
        // Flask
        precombat += "/flask,type=";
        precombat += ( level > 85 ) ? "warm_sun" : "draconic_mind";
      }

      if ( sim -> allow_food )
      {
        // Food
        precombat += "/food,type=";
        precombat += ( level > 85 ) ? "mogu_fish_stew" : "seafood_magnifique_feast";
      }
    }

    // Arcane Brilliance
    add_action( "Arcane Brilliance", "", "precombat" );

    // Armor
    if ( specialization() == MAGE_ARCANE && !sets.has_set_bonus( SET_T16_4PC_CASTER ) ) // use Frost Armor for arcane mages with 4p T16
    {
      add_action( "Mage Armor", "", "precombat" );
    }
    else if ( specialization() == MAGE_FIRE )
    {
      add_action( "Molten Armor", "", "precombat" );
    }
    else
    {
      add_action( "Frost Armor", "", "precombat" );
    }

    // Water Elemental
    if ( specialization() == MAGE_FROST )
      precombat += "/water_elemental";

    // Snapshot Stats
    precombat += "/snapshot_stats";

    // Prebuff L90 talents
    if ( talents.invocation -> ok() )
    {
      precombat += "/evocation";
    }
    else if ( talents.rune_of_power -> ok() )
    {
      precombat += "/rune_of_power";
    }

    //Potions
    if ( ( level >= 80 ) && ( sim -> allow_potions ) )
    {
      precombat += ( level > 85 ) ? "/jade_serpent_potion" : "/volcanic_potion";
    }

    precombat += "/mirror_image";

    // Counterspell
    add_action( "Counterspell", "if=target.debuff.casting.react" );

    // Cold Snap
    if ( talents.cold_snap -> ok() )
    {
      add_action( "Cold Snap", "if=health.pct<30" );
    }


    //not useful if bloodlust is check in option.
    if ( level >= 85 )
      action_list_str += "/time_warp,if=target.health.pct<25|time>5";

    // Spec-specific actions

    // Arcane
    if ( specialization() == MAGE_ARCANE )
    {
      if ( talents.rune_of_power -> ok() )
      {
        action_list_str += "/rune_of_power,if=buff.rune_of_power.remains<cast_time";
        action_list_str += "/rune_of_power,if=cooldown.arcane_power.remains=0&buff.rune_of_power.remains<buff.arcane_power.duration";
      }
      else if ( talents.invocation -> ok() )
      {
        action_list_str += "/evocation,if=buff.invokers_energy.down";
        action_list_str += "/evocation,if=cooldown.arcane_power.remains=0&buff.invokers_energy.remains<buff.arcane_power.duration";
        action_list_str += "/evocation,if=mana.pct<50,interrupt_if=mana.pct>95&buff.invokers_energy.remains>10";
      }
      else
      {
        action_list_str += "/evocation,if=mana.pct<50,interrupt_if=mana.pct>95";
      }

      action_list_str += "/mirror_image";

      //check for Arcane power glyph for use with time_to_bloodlust and Alter Time
      if (glyphs.arcane_power -> ok())
      {
        if ( talents.rune_of_power -> ok() )
          action_list_str += "/arcane_power,if=time_to_bloodlust>180&((buff.rune_of_power.remains>=buff.arcane_power.duration&buff.arcane_missiles.stack=2&buff.arcane_charge.stack>2)|target.time_to_die<buff.arcane_power.duration+5),moving=0";
        else if ( talents.invocation -> ok() )
          action_list_str += "/arcane_power,if=time_to_bloodlust>180&((buff.invokers_energy.remains>=buff.arcane_power.duration&buff.arcane_missiles.stack=2&buff.arcane_charge.stack>2)|target.time_to_die<buff.arcane_power.duration+5),moving=0";
        else
          action_list_str += "/arcane_power,if=time_to_bloodlust>180&((buff.arcane_missiles.stack=2&buff.arcane_charge.stack>2)|target.time_to_die<buff.arcane_power.duration+5),moving=0";
      }
      else
      {
        if ( talents.rune_of_power -> ok() )
          action_list_str += "/arcane_power,if=time_to_bloodlust>90&((buff.rune_of_power.remains>=buff.arcane_power.duration&buff.arcane_missiles.stack=2&buff.arcane_charge.stack>2)|target.time_to_die<buff.arcane_power.duration+5),moving=0";
        else if ( talents.invocation -> ok() )
          action_list_str += "/arcane_power,if=time_to_bloodlust>90&((buff.invokers_energy.remains>=buff.arcane_power.duration&buff.arcane_missiles.stack=2&buff.arcane_charge.stack>2)|target.time_to_die<buff.arcane_power.duration+5),moving=0";
        else
          action_list_str += "/arcane_power,if=time_to_bloodlust>90&((buff.arcane_missiles.stack=2&buff.arcane_charge.stack>2)|target.time_to_die<buff.arcane_power.duration+5),moving=0";
      }

      // The arcane action list for < 87 is terribly gimped, level instead
      if ( level >= 87 )
      {
        if ( race == RACE_ORC )         action_list_str += "/blood_fury,if=(buff.arcane_power.up|cooldown.arcane_power.remains>15|target.time_to_die<18)";
        else if ( race == RACE_TROLL )  action_list_str += "/berserking,if=(buff.arcane_power.up|target.time_to_die<18)";

        if ( sim -> allow_potions )      action_list_str += "/jade_serpent_potion,if=(buff.arcane_power.up|target.time_to_die<50)";

        if ( talents.rune_of_power -> ok() )
          action_list_str += init_use_item_actions( ",if=target.time_to_die<25&buff.rune_of_power.remains>20" );
        else if ( talents.invocation -> ok() )
          action_list_str += init_use_item_actions( ",if=target.time_to_die<25&buff.invokers_energy.remains>20" );
        else
          action_list_str += init_use_item_actions( ",if=target.time_to_die<25" );

        //decide between single_target and aoe rotation
        action_list_str += "/run_action_list,name=aoe,if=active_enemies>=6";
        action_list_str += "/run_action_list,name=single_target,if=active_enemies<6";
        
        //modify APL for T16 4p
        st_list_str += "/arcane_blast,if=buff.profound_magic.up&buff.arcane_charge.stack>3&mana.pct>93";

      }

      st_list_str += "/arcane_missiles,if=(buff.arcane_missiles.stack=2&cooldown.arcane_power.remains>0)|(buff.arcane_charge.stack>=4&cooldown.arcane_power.remains>8)";

      if ( talents.nether_tempest -> ok() )   st_list_str += "/nether_tempest,cycle_targets=1,if=(!ticking|remains<tick_time)&target.time_to_die>6";
      else if ( talents.living_bomb -> ok() ) st_list_str += "/living_bomb,cycle_targets=1,if=(!ticking|remains<tick_time)&target.time_to_die>tick_time*3";
      else if ( talents.frost_bomb -> ok() )  st_list_str += "/frost_bomb,if=!ticking&target.time_to_die>cast_time+tick_time";

      st_list_str += "/arcane_barrage,if=buff.arcane_charge.stack>=4&mana.pct<95";

      if ( talents.presence_of_mind -> ok() ) st_list_str += "/presence_of_mind";

      st_list_str += "/arcane_blast";

      if ( talents.ice_floes -> ok() ) st_list_str += "/ice_floes,moving=1";

      st_list_str += "/arcane_barrage,moving=1";
      st_list_str += "/fire_blast,moving=1";

      //AoE

      if ( talents.nether_tempest -> ok() )   aoe_list_str += "/nether_tempest,cycle_targets=1,if=(!ticking|remains<tick_time)&target.time_to_die>6";
      else if ( talents.living_bomb -> ok() ) aoe_list_str += "/living_bomb,cycle_targets=1,if=(!ticking|remains<tick_time)&target.time_to_die>tick_time*3";
      else if ( talents.frost_bomb -> ok() )  aoe_list_str += "/frost_bomb,if=!ticking&target.time_to_die>cast_time+tick_time";

      aoe_list_str += "/arcane_barrage,if=buff.arcane_charge.stack>=4";
      aoe_list_str += "/arcane_explosion";
    }

    // Fire
    else if ( specialization() == MAGE_FIRE )
    {
      if ( talents.rune_of_power -> ok() )
      {
        action_list_str += "/rune_of_power,if=buff.rune_of_power.remains<cast_time";
        action_list_str += "/rune_of_power,if=buff.rune_of_power.remains<6";
      }
      else if ( talents.invocation -> ok() )
      {
        action_list_str += "/evocation,if=(buff.invokers_energy.down|mana.pct<20)";
        action_list_str += "/evocation,if=buff.invokers_energy.remains<6";
      }
      else
      {
        if ( level > 87 )
          action_list_str += "/evocation,if=mana.pct<20,interrupt_if=mana.pct>95";
        else
          action_list_str += "/evocation,if=mana.pct<20,interrupt_if=mana.pct>95";
      }

      if ( level > 87 )
      {
        if ( race == RACE_ORC )                 action_list_str += "/blood_fury";
        else if ( race == RACE_TROLL )          action_list_str += "/berserking";
      }

      if ( sim -> allow_potions && level > 87 )
        action_list_str += "/jade_serpent_potion,if=target.time_to_die<45";

      action_list_str += init_use_profession_actions( level >= 87 ? ",if=target.time_to_die<25)" : "" );
      action_list_str += "/mirror_image";

//hardcoding this calculation for improving performance ???
      action_list_str += "/combustion,if=target.time_to_die<22";
      action_list_str += "/combustion,if=dot.ignite.tick_dmg>=((3*action.pyroblast.crit_damage)*mastery_value*0.5)";
      action_list_str += "/combustion,if=dot.ignite.tick_dmg>=((action.fireball.crit_damage+action.inferno_blast.crit_damage+action.pyroblast.hit_damage)*mastery_value*0.5)&dot.pyroblast.ticking&buff.pyroblast.down";
      if ( talents.presence_of_mind -> ok() )
        action_list_str += "&buff.presence_of_mind.down";

      if ( race == RACE_ORC )
      {
        action_list_str += "/blood_fury";
      }
      else if ( race == RACE_TROLL )
      {
        action_list_str += "/berserking";
      }

      if ( talents.presence_of_mind -> ok() )
      {
        action_list_str += "/presence_of_mind";
      }
      if ( sim -> allow_potions )
      {
        action_list_str += "/jade_serpent_potion";
      }

      if ( talents.presence_of_mind -> ok() ) action_list_str += "/presence_of_mind,if=target.time_to_die<5";

      action_list_str += "/flamestrike,if=active_enemies>=5";

      //if more than 1 target, cleave dot after every combustion
      action_list_str += "/inferno_blast,if=dot.combustion.ticking&active_enemies>1";
      action_list_str += "/pyroblast,if=buff.pyroblast.react|buff.presence_of_mind.up";
      action_list_str += "/inferno_blast,if=buff.heating_up.react&buff.pyroblast.down";

      if ( talents.nether_tempest -> ok() )   action_list_str += "/nether_tempest,cycle_targets=1,if=(!ticking|remains<tick_time)&target.time_to_die>6";
      else if ( talents.living_bomb -> ok() ) action_list_str += "/living_bomb,cycle_targets=1,if=(!ticking|remains<tick_time)&target.time_to_die>tick_time*3";
      else if ( talents.frost_bomb -> ok() )  action_list_str += "/frost_bomb,if=target.time_to_die>cast_time+tick_time";

      action_list_str += "/fireball";
      action_list_str += "/scorch,moving=1";
    }

    // Frost
    else if ( specialization() == MAGE_FROST )
    {
      if ( talents.rune_of_power -> ok() )
      {
        action_list_str += "/rune_of_power,if=buff.rune_of_power.remains<cast_time";
        action_list_str += "/rune_of_power,if=cooldown.icy_veins.remains=0&buff.rune_of_power.remains<20";
      }
      else if ( talents.invocation -> ok() )
      {
        action_list_str += "/evocation,if=(buff.invokers_energy.down|mana.pct<20)";
        action_list_str += "/evocation,if=cooldown.icy_veins.remains=0&buff.invokers_energy.remains<20";
      }
      else
      {
        action_list_str += "/evocation,if=mana.pct<50,interrupt_if=mana.pct>95";
      }

      action_list_str += "/mirror_image";

      //remove condition because of T16
      action_list_str += "/frozen_orb";

      action_list_str += "/icy_veins,if=time_to_bloodlust>180&((buff.brain_freeze.react|buff.fingers_of_frost.react)|target.time_to_die<22),moving=0";


      if ( race == RACE_ORC )                 action_list_str += "/blood_fury,if=buff.icy_veins.up|cooldown.icy_veins.remains>30|target.time_to_die<18";
      else if ( race == RACE_TROLL )          action_list_str += "/berserking,if=buff.icy_veins.up|target.time_to_die<18";

      if ( sim -> allow_potions && level > 85 )
        action_list_str += "/jade_serpent_potion,if=buff.icy_veins.up|target.time_to_die<45";

      if ( talents.presence_of_mind -> ok() ) action_list_str += "/presence_of_mind,if=buff.icy_veins.up|cooldown.icy_veins.remains>15|target.time_to_die<15";
      
      if ( talents.rune_of_power -> ok() )
        action_list_str += init_use_item_actions( ",if=buff.rune_of_power.remains>20|target.time_to_die<25" );
      else if ( talents.invocation -> ok() )
        action_list_str += init_use_item_actions( ",if=buff.invokers_energy.remains>20|target.time_to_die<25" );
      else
        action_list_str += init_use_item_actions( ",if=target.time_to_die<25" );

      if ( level >= 87 )
      {
        action_list_str += "/frostfire_bolt,if=buff.brain_freeze.up";
        action_list_str += "/ice_lance,if=buff.fingers_of_frost.up";
      }

      if ( talents.nether_tempest -> ok() )   action_list_str += "/nether_tempest,cycle_targets=1,if=(!ticking|remains<tick_time)&target.time_to_die>6";
      else if ( talents.living_bomb -> ok() ) action_list_str += "/living_bomb,cycle_targets=1,if=(!ticking|remains<tick_time)&target.time_to_die>tick_time*3";
      else if ( talents.frost_bomb -> ok() )  action_list_str += "/frost_bomb,if=target.time_to_die>cast_time+tick_time";

      action_list_str += "/frostfire_bolt,if=buff.brain_freeze.react&cooldown.icy_veins.remains>2";

      //with 2pT16, keep 1 FoF to use with Frozen Thoughts
      action_list_str += "/ice_lance,if=buff.frozen_thoughts.react&buff.fingers_of_frost.up";
      action_list_str += "/ice_lance,if=buff.fingers_of_frost.up&(buff.fingers_of_frost.remains<2|(buff.fingers_of_frost.stack>1&cooldown.icy_veins.remains>2))";
      action_list_str += "/frostbolt";

      if ( talents.ice_floes -> ok() ) action_list_str += "/ice_floes,moving=1";

      action_list_str += "/fire_blast,moving=1";
      action_list_str += "/ice_lance,moving=1";
    }

    use_default_action_list = true;
  }

  player_t::init_action_list();
}

// mage_t::mana_regen_per_second ============================================

double mage_t::mana_regen_per_second() const
{
  double mp5 = player_t::mana_regen_per_second();

  if ( passives.nether_attunement -> ok() )
    mp5 /= cache.spell_speed();


  if ( buffs.invokers_energy -> check() )
    mp5 *= 1.0 + buffs.invokers_energy -> data().effectN( 3 ).percent();

  return mp5;
}

// mage_t::composite_player_multipler =======================================

double mage_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.arcane_power -> check() )
  {
    double v = buffs.arcane_power -> value();
    if ( sets.has_set_bonus( SET_T14_4PC_CASTER ) )
    {
      v += 0.1;
    }
    m *= 1.0 + v;
  }

  if ( buffs.invokers_energy -> up() )
  {
    m *= 1.0 + buffs.invokers_energy -> data().effectN( 1 ).percent();
  }
  else if ( buffs.rune_of_power -> check() )
  {
    m *= 1.0 + buffs.rune_of_power -> data().effectN( 2 ).percent();
  }
  else if ( talents.incanters_ward -> ok() && cooldowns.incanters_ward -> up() )
  {
    m *= 1.0 + find_spell( 118858 ) -> effectN( 1 ).percent();
  }
  else if ( buffs.incanters_absorption -> up() )
  {
    m *= 1.0 + buffs.incanters_absorption -> value() * buffs.incanters_absorption -> data().effectN( 1 ).percent();
  }

  if ( specialization() == MAGE_ARCANE )
    cache.player_mult_valid[ school ] = false;

  return m;
}


void mage_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_MASTERY:
      if ( spec.mana_adept -> ok() )
      {
        player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      }
      break;
    default: break;
  }
}

// mage_t::composite_spell_crit =============================================

double mage_t::composite_spell_crit() const
{
  double c = player_t::composite_spell_crit();

  // These also increase the water elementals crit chance

  if ( buffs.molten_armor -> up() )
  {
    c += buffs.molten_armor -> data().effectN( 1 ).percent();
  }

  return c;
}

//mage_t::composite_multistrike =============================================

double mage_t::composite_multistrike() const
{
  double ms = player_t::composite_multistrike();

  if ( buffs.frost_armor -> up() )
    ms += buffs.frost_armor -> data().effectN( 1 ).percent();

  return ms;
}

// mage_t::composite_spell_haste ============================================

double mage_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.icy_veins -> up() && !glyphs.icy_veins -> ok() )
  {
    h *= 1.0 / ( 1.0 + buffs.icy_veins -> data().effectN( 1 ).percent() );
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


}
// mage_t::reset ============================================================

void mage_t::reset()
{
  player_t::reset();

  rotation.reset();
  icicles.clear();
  core_event_t::cancel( icicle_event );
  active_living_bomb_targets = 0;
  last_bomb_target = 0;
}

// mage_t::regen  ===========================================================

void mage_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( buffs.rune_of_power -> up() )
  {
    resource_gain( RESOURCE_MANA, mana_regen_per_second() * periodicity.total_seconds() * buffs.rune_of_power -> data().effectN( 1 ).percent(), gains.rune_of_power );
  }
  else if ( talents.incanters_ward -> ok() && cooldowns.incanters_ward -> up() )
  {
    resource_gain( RESOURCE_MANA, mana_regen_per_second() * periodicity.total_seconds() * find_spell( 118858 ) -> effectN( 2 ).percent(), gains.incanters_ward_passive );
  }

  if ( pets.water_elemental )
    benefits.water_elemental -> update( pets.water_elemental -> is_sleeping() == 0 );
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

// mage_t::moving============================================================

void mage_t::moving()
{
  //FIXME, only remove the buff if we are moving more than RoPs radius
  buffs.rune_of_power -> expire();
  if ( sim -> debug ) sim -> out_debug.printf( "%s lost Rune of Power due to movement.", name() );

  player_t::moving();
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

  struct rotation_expr_t : public mage_expr_t
  {
    mage_rotation_e rt;
    rotation_expr_t( const std::string& n, mage_t& m, mage_rotation_e r ) :
      mage_expr_t( n, m ), rt( r ) {}
    virtual double evaluate() { return mage.rotation.current == rt; }
  };

  if ( name_str == "dps" )
    return new rotation_expr_t( name_str, *this, ROTATION_DPS );

  if ( name_str == "dpm" )
    return new rotation_expr_t( name_str, *this, ROTATION_DPM );

  if ( name_str == "burn_mps" )
  {
    struct burn_mps_expr_t : public mage_expr_t
    {
      burn_mps_expr_t( mage_t& m ) : mage_expr_t( "burn_mps", m ) {}
      virtual double evaluate()
      {
        timespan_t now = mage.sim -> current_time;
        timespan_t delta = now - mage.rotation.last_time;
        mage.rotation.last_time = now;
        if ( mage.rotation.current == ROTATION_DPS )
          mage.rotation.dps_time += delta;
        else if ( mage.rotation.current == ROTATION_DPM )
          mage.rotation.dpm_time += delta;

        return ( mage.rotation.dps_mana_loss / mage.rotation.dps_time.total_seconds() ) -
               ( mage.rotation.mana_gain / mage.sim -> current_time.total_seconds() );
      }
    };
    return new burn_mps_expr_t( *this );
  }

  if ( name_str == "regen_mps" )
  {
    struct regen_mps_expr_t : public mage_expr_t
    {
      regen_mps_expr_t( mage_t& m ) : mage_expr_t( "regen_mps", m ) {}
      virtual double evaluate()
      {
        return mage.rotation.mana_gain /
               mage.sim -> current_time.total_seconds();
      }
    };
    return new regen_mps_expr_t( *this );
  }

  if ( util::str_compare_ci( name_str, "shooting_icicles" ) )
  {
    struct sicicles_expr_t : public mage_expr_t
    {
      sicicles_expr_t( mage_t& m ) : mage_expr_t( "shooting_icicles", m )
      { }
      double evaluate()
      { return mage.icicle_event != 0; }
    };

    return new sicicles_expr_t( *this );
  }

  if ( util::str_compare_ci( name_str, "icicles" ) )
  {
    struct icicles_expr_t : public mage_expr_t
    {
      icicles_expr_t( mage_t& m ) : mage_expr_t( "icicles", m )
      { }

      double evaluate()
      {
        if ( mage.icicles.size() == 0 )
          return 0;
        else if ( mage.sim -> current_time - mage.icicles[ 0 ].first < mage.spells.icicles_driver -> duration() )
          return static_cast<double>(mage.icicles.size());
        else
        {
          size_t icicles = 0;
          for ( int i = as<int>( mage.icicles.size() - 1 ); i >= 0; i-- )
          {
            if ( mage.sim -> current_time - mage.icicles[ i ].first >= mage.spells.icicles_driver -> duration() )
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

// mage_t::decode_set =======================================================

set_e mage_t::decode_set( const item_t& item ) const
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const char* s = item.name();

  if ( strstr( s, "time_lords_"       ) ) return SET_T13_CASTER;

  if ( strstr( s, "burning_scroll"    ) ) return SET_T14_CASTER;

  if ( strstr( s, "_chromatic_hydra"  ) ) return SET_T15_CASTER;

  if ( strstr( s, "chronomancer_"     ) ) return SET_T16_CASTER;

  if ( strstr( s, "gladiators_silk_"  ) ) return SET_PVP_CASTER;

  return SET_NONE;
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
  case STAT_AGI_INT: 
    return STAT_INTELLECT; 
  case STAT_STR_AGI:
    return STAT_NONE;
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_SPIRIT:
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
      return STAT_NONE;     
  default: return s; 
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

// MAGE MODULE INTERFACE ====================================================

struct mage_module_t : public module_t
{
  mage_module_t() : module_t( MAGE ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    mage_t* p = new mage_t( sim, name, r );
    p -> report_extension = std::shared_ptr<player_report_extension_t>( new mage_report_t( *p ) );
    return p;
  }
  virtual bool valid() const { return true; }
  virtual void init        ( sim_t* ) const {}
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end  ( sim_t* ) const {}
};

 // UNNAMED NAMESPACE

const module_t* module_t::mage()
{
  static mage_module_t m;
  return &m;
}

