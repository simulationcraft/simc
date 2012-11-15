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

namespace alter_time {
struct mage_state_t;
};

enum mage_rotation_e { ROTATION_NONE = 0, ROTATION_DPS, ROTATION_DPM, ROTATION_MAX };

struct mage_td_t : public actor_pair_t
{
  struct dots_t
  {
    dot_t* flamestrike;
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

  // Active
  spell_t* active_ignite;
  alter_time::mage_state_t* alter_time_state;

  // Benefits
  struct benefits_t
  {
    benefit_t* arcane_charge[ 7 ];
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
    buff_t* invocation;
    stat_buff_t* mage_armor;
    buff_t* molten_armor;
    buff_t* presence_of_mind;
    buff_t* pyroblast;
    buff_t* rune_of_power;
    buff_t* tier13_2pc;
    buff_t* alter_time;
    absorb_buff_t* incanters_ward;
    buff_t* incanters_ward_post;
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
    gain_t* mana_gem;
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
    const spell_data_t* living_bomb;
    const spell_data_t* mana_gem;
    const spell_data_t* mirror_image;

    // Minor
    const spell_data_t* arcane_brilliance;
    const spell_data_t* conjuring;
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
    proc_t* deferred_ignite;
    proc_t* mana_gem;
    proc_t* test_for_crit_hotstreak;
    proc_t* crit_for_hotstreak;
    proc_t* hotstreak;
  } procs;

  // Random Number Generation
  struct rngs_t
  {
  } rngs;

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
    const spell_data_t* hot_streak;
    const spell_data_t* icy_veins;

    const spell_data_t* flashburn;
    const spell_data_t* frostburn;
    const spell_data_t* mana_adept;

    const spell_data_t* arcane_specialization;
    const spell_data_t* fire_specialization;
    const spell_data_t* frost_specialization;

    const spell_data_t* blink;

    const spell_data_t* stolen_time;

    const spell_data_t* arcane_charge_arcane_blast;

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
    const spell_data_t* frostburn;

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
private:
  target_specific_t<mage_td_t> target_data;
public:
  int mana_gem_charges;
  int current_arcane_charges;

  mage_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, MAGE, name, r ),
    active_ignite( 0 ),
    alter_time_state( NULL ),
    benefits( benefits_t() ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    glyphs( glyphs_t() ),
    passives( passives_t() ),
    pets( pets_t() ),
    procs( procs_t() ),
    rngs( rngs_t() ),
    rotation( rotation_t() ),
    spells( spells_t() ),
    spec( specializations_t() ),
    talents( talents_list_t() ),
    mana_gem_charges( 0 ),
    current_arcane_charges()
  {
    target_data.init( "target_data", this );

    // Cooldowns
    cooldowns.evocation      = get_cooldown( "evocation"     );
    cooldowns.inferno_blast  = get_cooldown( "inferno_blast" );
    cooldowns.incanters_ward = get_cooldown( "incanters_ward" );

    // Options
    initial.distance = 40;
    mana_gem_charges = 3;
  }

  // Character Definition
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_benefits();
  virtual void      init_actions();
  virtual void      reset();
  virtual expr_t*   create_expression( action_t*, const std::string& name );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual resource_e primary_resource() { return RESOURCE_MANA; }
  virtual role_e primary_role() { return ROLE_SPELL; }
  virtual double    mana_regen_per_second();
  virtual double    composite_player_multiplier( school_e school, action_t* a = NULL );
  virtual double    composite_spell_crit();
  virtual double    composite_spell_haste();
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual void      stun();

  virtual mage_td_t* get_target_data( player_t* target )
  {
    mage_td_t*& td = target_data[ target ];
    if ( ! td ) td = new mage_td_t( target, this );
    return td;
  }

  // Event Tracking
  virtual void   regen( timespan_t periodicity );
  virtual double resource_gain( resource_e, double amount, gain_t* = 0, action_t* = 0 );
  virtual double resource_loss( resource_e, double amount, gain_t* = 0, action_t* = 0 );

  // Temporary
  virtual std::string set_default_talents()
  {
    switch ( specialization() )
    {
    case MAGE_ARCANE: return "111112";
    case MAGE_FIRE: return "122211";
    case MAGE_FROST: return "133331";
    case SPEC_NONE: break;
    default: break;
    }
    return player_t::set_default_talents();
  }

  virtual std::string set_default_glyphs()
  {
    switch ( specialization() )
    {
    case MAGE_ARCANE: return "evocation/mana_gem/slow/mirror_image";
    case MAGE_FIRE: return "evocation/fire_blast/counterspell/mirror_image";
    case MAGE_FROST: return "evocation/icy_veins/ice_lance";
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_glyphs();
  }

  virtual double composite_spell_power_multiplier();

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
    mini_waterbolt_t( water_elemental_pet_t* p, bool bolt_two = false ) :
      spell_t( "mini_waterbolt", p, p -> find_spell( 131581 ) )
    {
      may_crit = true;
      background = true;
      dual = true;
      base_costs[ RESOURCE_MANA ] = 0;

      if ( ! bolt_two )
      {
        execute_action = new mini_waterbolt_t( p, true );
      }
    }

    virtual timespan_t execute_time()
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

    void execute()
    {
      spell_t::execute();

      water_elemental_pet_t* p = static_cast<water_elemental_pet_t*>( player );
      if ( result_is_hit( execute_state -> result ) &&
           p -> o() -> glyphs.icy_veins -> ok() &&
           p -> o() -> buffs.icy_veins -> up() )
      {
        mini_waterbolt -> pre_execute_state = mini_waterbolt -> get_state( execute_state );
        mini_waterbolt -> schedule_execute();
      }
    }

    virtual double action_multiplier()
    {
      double am = spell_t::action_multiplier();

      water_elemental_pet_t* p = static_cast<water_elemental_pet_t*>( player );

      if ( p -> o() -> glyphs.icy_veins -> ok() && p -> o() -> buffs.icy_veins -> up() )
      {
        am *= 0.4;
      }

      return am;
    }

    virtual double composite_target_multiplier( player_t* target )
    {
      double tm = spell_t::composite_target_multiplier( target );

      water_elemental_pet_t* p = static_cast<water_elemental_pet_t*>( player );

      tm *= 1.0 + p -> o() -> get_target_data( target ) -> debuffs.frostbolt -> stack() * 0.05;

      return tm;
    }

  };

  water_elemental_pet_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "water_elemental" )
  {
    action_list_str  = "freeze,if=owner.buff.fingers_of_frost.stack=0";
    action_list_str += "/waterbolt";
    create_options();

    owner_coeff.sp_from_sp = 1.0;
  }

  mage_t* o()
  { return static_cast<mage_t*>( owner ); }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "freeze"     ) return new     freeze_t( this, options_str );
    if ( name == "waterbolt" ) return new waterbolt_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }

  virtual double composite_player_multiplier( school_e school, action_t* a )
  {
    double m = pet_t::composite_player_multiplier( school, a );
    m *= 1.0 + o() -> spec.frostburn -> effectN( 3 ).mastery_value() * o() -> composite_mastery();

    if ( o() -> buffs.invocation -> up() )
    {
      m *= 1.0 + o() -> buffs.invocation -> data().effectN( 1 ).percent();
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
      m *= 1.0 + o() -> buffs.incanters_ward_post -> value() * o() -> buffs.incanters_ward_post -> data().effectN( 1 ).percent();
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

    virtual double action_multiplier()
    {
      double am = mirror_image_spell_t::action_multiplier();

      am *= 1.0 + p() -> arcane_charge -> stack() * p() -> o() -> spells.arcane_charge_arcane_blast -> effectN( 1 ).percent();

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
    if ( name == "fire_blast"   ) return new   fire_blast_t( this, options_str );
    if ( name == "fireball"     ) return new     fireball_t( this, options_str );
    if ( name == "frostbolt"    ) return new    frostbolt_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }

  mage_t* o() const
  { return static_cast<mage_t*>( owner ); }

  virtual void init_actions()
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
      action_list_str = "fire_blast";
      action_list_str += "/frostbolt";
    }

    pet_t::init_actions();
  }

  virtual void init_buffs()
  {
    pet_t::init_buffs();

    arcane_charge = buff_creator_t( this, "arcane_charge", o() -> spec.arcane_charge )
                    .max_stack( find_spell( 36032 ) -> max_stacks() )
                    .duration( find_spell( 36032 ) -> duration() );
  }


  virtual double composite_player_multiplier( school_e school, action_t* a )
  {
    double m = pet_t::composite_player_multiplier( school, a );

    // Orc racial
    if ( owner -> race == RACE_ORC )
      m *= 1.0 + find_spell( 21563 ) -> effectN( 1 ).percent();

    return m;
  }
};

} // pets

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
      b -> sim -> output( "Creating buff_state_t for buff %s of player %s",
                          b -> name_str.c_str(), b -> player ? b -> player -> name() : "" );

      b -> sim -> output( "Snapshoted values are: current_stacks=%d remaining_time=%.4f current_value=%.2f",
                          stacks, remain_time.total_seconds(), value );
    }
  }

  void write_back_state() const
  {
    if ( buff -> sim -> debug )
      buff -> sim -> output( "Writing back buff_state_t for buff %s of player %s",
                             buff -> name_str.c_str(), buff -> player ? buff -> player -> name() : "" );

    timespan_t save_buff_cd = buff -> cooldown -> duration; // Temporarily save the buff cooldown duration
    buff -> cooldown -> duration = timespan_t::zero(); // Don't restart the buff cooldown

    buff -> execute( stacks, value, remain_time ); // Reset the buff

    buff -> cooldown -> duration = save_buff_cd; // Restore the buff cooldown duration
  }
};

struct mage_state_t
{
  mage_t* mage;
  std::array<double, RESOURCE_MAX > resources;
  // location
  std::vector<buff_state_t> buff_states;


  mage_state_t( mage_t* m ) : // Snapshot and start 6s event
    mage( m ),
    resources( m -> resources.current )
  {
    if ( m -> sim -> debug )
      m -> sim -> output( "Creating mage_state_t for mage %s", m -> name() );

    for ( size_t i = 0; i < m -> buff_list.size(); ++i )
    {
      buff_t* b = m -> buff_list[ i ];

      if ( b -> current_stack == 0 )
        continue;

      buff_states.push_back( buff_state_t( b ) );
    }
  }

  void write_back_state() const
  {
    // Do not restore state under any circumstances to a mage that is not
    // active
    if ( mage -> current.sleeping )
      return;

    mage -> resources.current = resources;

    for ( size_t i = 0; i < buff_states.size(); ++ i )
    {
      buff_states[ i ].write_back_state();
    }
  }
};

struct alter_time_buff_t : public buff_t
{
  alter_time_buff_t( mage_t* player ) :
    buff_t( buff_creator_t( player, "alter_time" ).spell( player -> find_spell( 110909 ) ) )
  { }

  mage_t* p() const
  { return static_cast<mage_t*>( player ); }

  virtual bool trigger( int        stacks,
                        double     value,
                        double     chance,
                        timespan_t duration )
  {
    assert( p() -> alter_time_state == NULL );

    p() -> alter_time_state = new alter_time::mage_state_t( p() );

    return buff_t::trigger( stacks, value, chance, duration );
  }

  virtual void expire()
  {
    if ( p() -> alter_time_state )
    {
      p() -> alter_time_state -> write_back_state();
      delete p() -> alter_time_state;
      p() -> alter_time_state = NULL;
    }

    buff_t::expire();
  }
};


}; // alter_time namespace

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Mage Spell
// ==========================================================================

struct mage_spell_t : public spell_t
{
  bool frozen, may_hot_streak, may_proc_missiles, is_copy, consumes_ice_floes, fof_active;
  int dps_rotation;
  int dpm_rotation;
  int pre_cast;

  mage_spell_t( const std::string& n, mage_t* p,
                const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( n, p, s ),
    frozen( false ), may_hot_streak( false ), may_proc_missiles( true ), is_copy( false ), consumes_ice_floes( true ), fof_active( false ), dps_rotation( 0 ), dpm_rotation( 0 ), pre_cast( 0 )
  {
    may_crit      = ( base_dd_min > 0 ) && ( base_dd_max > 0 );
    tick_may_crit = true;
  }

  mage_t* p() const { return static_cast<mage_t*>( player ); }

  mage_td_t* td( player_t* t = 0 ) { return p() -> get_target_data( t ? t : target ); }

  virtual void parse_options( option_t*          options,
                              const std::string& options_str )
  {
    option_t base_options[] =
    {
      { "dps", OPT_BOOL, &dps_rotation },
      { "dpm", OPT_BOOL, &dpm_rotation },
      { "precast", OPT_INT, &pre_cast },
      { NULL, OPT_UNKNOWN, NULL }
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

  virtual double cost()
  {
    double c = spell_t::cost();

    if ( p() -> buffs.arcane_power -> check() )
    {
      double m = 1.0 + p() -> buffs.arcane_power -> data().effectN( 2 ).percent();

      c *= m;
    }

    return c;
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = spell_t::execute_time();

    if ( ! channeled && t > timespan_t::zero() && p() -> buffs.presence_of_mind -> up() )
      return timespan_t::zero();

    return t;
  }

  virtual bool usable_moving()
  {
    bool um = spell_t::usable_moving();
    timespan_t t = base_execute_time;
    if ( p() -> talents.ice_floes -> ok() &&
         t < timespan_t::from_seconds( 4.0 ) &&
         ( p() -> buffs.ice_floes -> up() || p() -> buffs.ice_floes -> cooldown -> up() ) )
      um = true;

    return um;
  }

  virtual double action_multiplier()
  {
    double am = spell_t::action_multiplier();

    if ( frozen )
      am *= 1.0 + p() -> spec.frostburn -> effectN( 1 ).mastery_value() * p() -> composite_mastery();

    return am;
  }

  virtual double composite_crit()
  {
    double c = spell_t::composite_crit();

    if ( frozen && p() -> passives.shatter -> ok() )
    {
      c = c * 2.0 + 0.50;
    }

    return c;
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
      p -> buffs.heating_up -> expire();
    }
  }
  // mage_spell_t::execute ====================================================

  virtual void execute()
  {
    p() -> benefits.dps_rotation -> update( p() -> rotation.current == ROTATION_DPS );
    p() -> benefits.dpm_rotation -> update( p() -> rotation.current == ROTATION_DPM );

    spell_t::execute();

    if ( background )
      return;

    if ( ! channeled && spell_t::execute_time() > timespan_t::zero() )
    {
      if ( !is_copy && p() -> buffs.presence_of_mind -> check() )
      {
        p() -> buffs.presence_of_mind -> expire();
      }
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

};

// trigger_ignite ===========================================================

struct ignite_t : public ignite::pct_based_action_t< mage_spell_t, mage_t >
{
  ignite_t( mage_t* player ) :
    base_t( "ignite", player, player -> dbc.spell( 12654 )  )
    // Acessed through dbc.spell because it is a level 99 spell which will not be parsed with find_spell
  {
  }
};

// Mage Ignite specialization
void trigger_ignite( mage_spell_t* s, action_state_t* state )
{
  mage_t* p = s -> p();
  if ( ! p -> spec.ignite -> ok() ) return;
  ignite::trigger_pct_based(
    p -> active_ignite, // ignite spell
    state -> target, // target

    state -> result_amount * p -> spec.ignite -> effectN( 1 ).mastery_value() * p -> composite_mastery() ); // ignite damage
}
// ==========================================================================
// Mage Spell
// ==========================================================================

// ==========================================================================
// Mage Spells
// ==========================================================================

// Arcane Barrage Spell =====================================================

struct arcane_barrage_t : public mage_spell_t
{
  arcane_barrage_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "arcane_barrage", p, p -> find_class_spell( "Arcane Barrage" ) )
  {
    check_spec( MAGE_ARCANE );
    parse_options( NULL, options_str );
    base_aoe_multiplier *= data().effectN( 2 ).percent();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new arcane_barrage_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    int charges = p() -> buffs.arcane_charge -> check();
    aoe = charges <= 0 ? 0 : 1 + charges;

    for ( int i=0; i < 7; i++ )
    {
      p() -> benefits.arcane_charge[ i ] -> update( i == charges );
    }

    mage_spell_t::execute();

    p() -> buffs.arcane_charge -> expire();
  }

  virtual double action_multiplier()
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.arcane_charge -> stack() * p() -> spells.arcane_charge_arcane_blast -> effectN( 1 ).percent();

    return am;
  }
};

// Arcane Blast Spell =======================================================

struct arcane_blast_t : public mage_spell_t
{
  arcane_blast_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "arcane_blast", p, p -> find_class_spell( "Arcane Blast" ) )
  {
    parse_options( NULL, options_str );

    if ( p -> set_bonus.pvp_4pc_caster() )
      base_multiplier *= 1.05;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new arcane_blast_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual double cost()
  {
    double c = mage_spell_t::cost();

    if ( p() -> buffs.arcane_charge -> check() )
    {
      c *= 1.0 +  p() -> buffs.arcane_charge -> check() * p() -> spells.arcane_charge_arcane_blast -> effectN( 2 ).percent();
    }

    return c;
  }

  virtual void execute()
  {
    for ( int i=0; i < 7; i++ )
    {
      p() -> benefits.arcane_charge[ i ] -> update( i == p() -> buffs.arcane_charge -> check() );
    }

    mage_spell_t::execute();

    p() -> buffs.arcane_charge -> trigger();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> set_bonus.tier13_2pc_caster() )
        p() -> buffs.tier13_2pc -> trigger( 1, buff_t::DEFAULT_VALUE(), 1 );
    }
  }

  virtual double action_multiplier()
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.arcane_charge -> stack() * p() -> spells.arcane_charge_arcane_blast -> effectN( 1 ).percent();

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
    if ( sim -> log ) sim -> output( "%s performs %s", player -> name(), name() );

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
      p() -> buffs.arcane_charge -> trigger();
  }
};

// Arcane Missiles Spell ====================================================

struct arcane_missiles_tick_t : public mage_spell_t
{
  arcane_missiles_tick_t( mage_t* p, bool dtr=false ) :
    mage_spell_t( "arcane_missiles_tick", p, p -> find_class_spell( "Arcane Missiles" ) -> effectN( 2 ).trigger() )
  {
    background  = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new arcane_missiles_tick_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
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

  virtual double action_multiplier()
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.arcane_charge -> stack() * p() -> spells.arcane_charge_arcane_blast -> effectN( 1 ).percent();

    if ( p() -> set_bonus.tier14_2pc_caster() )
    {
      am *= 1.07;
    }

    return am;
  }

  virtual void execute()
  {
    for ( int i=0; i < 7; i++ )
    {
      p() -> benefits.arcane_charge[ i ] -> update( i == p() -> buffs.arcane_charge -> check() );
    }

    mage_spell_t::execute();

    p() -> buffs.arcane_missiles -> up();
    p() -> buffs.arcane_missiles -> decrement();
    p() -> buffs.arcane_charge   -> trigger();
  }

  virtual bool ready()
  {
    if ( ! p() -> buffs.arcane_missiles -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Arcane Power Buff ========================================================

struct arcane_power_buff_t : public buff_t
{
  arcane_power_buff_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "arcane_power", p -> find_class_spell( "Arcane Power" ) ) )
  {
    cooldown -> duration = timespan_t::zero(); // CD is managed by the spell

    if ( p -> glyphs.arcane_power -> ok() )
      buff_duration *= 2;
  }

  virtual void expire()
  {
    buff_t::expire();

    mage_t* p = static_cast<mage_t*>( player );
    p -> buffs.tier13_2pc -> expire();
  }
};

// Arcane Power Spell =======================================================

struct arcane_power_t : public mage_spell_t
{
  timespan_t orig_duration;

  arcane_power_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_power", p, p -> find_class_spell( "Arcane Power" ) ),
    orig_duration( timespan_t::zero() )
  {
    check_spec( MAGE_ARCANE );
    parse_options( NULL, options_str );
    harmful = false;

    if ( p -> glyphs.arcane_power -> ok() )
      cooldown -> duration *= 2;

    orig_duration = cooldown -> duration;
  }

  virtual void execute()
  {
    if ( p() -> set_bonus.tier13_4pc_caster() )
      cooldown -> duration = orig_duration * ( 1.0 - p() -> buffs.tier13_2pc -> check() * p() -> spells.stolen_time -> effectN( 1 ).base_value() );

    mage_spell_t::execute();

    p() -> buffs.arcane_power -> trigger( 1, data().effectN( 1 ).percent() );
  }

  virtual bool ready()
  {
    // FIXME: Is this still true?
    // Can't trigger AP if PoM is up
    if ( p() -> buffs.presence_of_mind -> check() )
      return false;

    return mage_spell_t::ready();
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
    shard( 0 )
  {
    parse_options( NULL, options_str );

    channeled    = true;
    hasted_ticks = false;
    may_miss     = false;

    shard = new blizzard_shard_t( p );
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
  std::vector<cooldown_t*> cooldown_list;

  cold_snap_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "cold_snap", p, p -> talents.cold_snap )
  {
    parse_options( NULL, options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;

    cooldown_list.push_back( p -> get_cooldown( "cone_of_cold"  ) );
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> resource_gain( RESOURCE_HEALTH, p() -> resources.base[ RESOURCE_HEALTH ] * p() -> talents.cold_snap -> effectN( 2 ).percent() );

    for ( size_t i = 0; i < cooldown_list.size(); i++ )
    {
      cooldown_list[ i ] -> reset();
    }
  }
};

// Combustion Spell =========================================================

struct combustion_t : public mage_spell_t
{
  timespan_t orig_duration;

  combustion_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "combustion", p, p -> find_class_spell( "Combustion" ) ),
    orig_duration( timespan_t::zero() )
  {
    check_spec( MAGE_FIRE );
    parse_options( NULL, options_str );
    may_hot_streak = true;

    // The "tick" portion of spell is specified in the DBC data in an alternate version of Combustion
    num_ticks      = 10;
    base_tick_time = timespan_t::from_seconds( 1.0 );

    if ( p -> set_bonus.tier14_4pc_caster() )
    {
      cooldown -> duration *= 0.8;
    }

    may_trigger_dtr = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new combustion_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }

    if ( p -> glyphs.combustion -> ok() )
    {
      base_tick_time *= 1.0 + p -> glyphs.combustion -> effectN( 1 ).percent();
      cooldown -> duration *= 1.0 + p -> glyphs.combustion -> effectN( 2 ).percent();
      base_multiplier *= 1.0 + p -> glyphs.combustion -> effectN( 3 ).percent();
    }

    orig_duration = cooldown -> duration;
  }

  // calculate_dot_dps ========================================================

  // Calculates tick damage / tick interval, on which Combustion damage
  // is based.
  double calculate_dot_dps( dot_t* d )
  {
    if ( ! d -> ticking ) return 0;

    action_t* a = d -> current_action;

    d -> state -> result = RESULT_HIT;

    return ( a -> calculate_tick_damage( d -> state -> result, d -> state -> composite_power(), 1.0, d -> state -> target ) / a -> base_tick_time.total_seconds() );
  }

  double calculate_pyroblast_dot_dps( action_state_t* combustion_state, dot_t* d )
  {
    if ( ! d -> ticking ) return 0;

    action_t* a = d -> current_action;

    d -> state -> result = RESULT_HIT;

    /* http://code.google.com/p/simulationcraft/issues/detail?id=1305
     * The fake DOT should:
     * - Always get * 1.25 (this represents the multiplier Pyroblast gets from the Pyroblast! proc, and is always assumed to be active).
     * - Any other caster multiplier that is active on the caster at the moment Combustion is cast
     * - Any target multiplier that is active on the target at the moment Combustion is cast
    */
    double multiplier = a -> composite_ta_multiplier() * composite_target_ta_multiplier( d -> state -> target );

    if ( ! p() -> buffs.pyroblast -> check() ) // Assumption: No double dip from the buff.
      multiplier *= 1.25;

    double dmg = ( a -> calculate_tick_damage( d -> state -> result, combustion_state -> composite_power(), multiplier, d -> state -> target ) / d -> time_to_tick.total_seconds() );

    if ( sim -> debug )
      sim -> output( "%s combustion: Recalculated fake pyroblast dot damage with sp=%.2f mult=%.2f resulting in dmg=%.2f\n",
                     player -> name(), combustion_state -> composite_power(), multiplier, dmg );

    return dmg;
  }

  virtual void impact( action_state_t* s )
  {
    double ignite_dmg = 0;

    if ( td() -> dots.ignite -> ticking )
    {
      ignite_dmg += calculate_dot_dps( td() -> dots.ignite );
    }

    base_td = 0;
    base_td += ignite_dmg;
    base_td += calculate_pyroblast_dot_dps( s, td() -> dots.pyroblast );

    mage_spell_t::impact( s );
  }

  virtual void execute()
  {
    if ( p() -> set_bonus.tier13_4pc_caster() )
    {
      cooldown -> duration = orig_duration * ( 1.0 - p() -> buffs.tier13_2pc -> check() * p() -> spells.stolen_time -> effectN( 1 ).base_value() );
    }

    p() -> cooldowns.inferno_blast -> reset();

    mage_spell_t::execute();
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );

    p() -> buffs.tier13_2pc -> expire();
  }

  virtual double composite_ta_multiplier()
  { return 1.0; }
  virtual double composite_target_multiplier( player_t* )
  { return 1.0; }
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
};

// Conjure Mana Gem Spell ===================================================

struct conjure_mana_gem_t : public mage_spell_t
{
  conjure_mana_gem_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "conjure_mana_gem", p, p -> find_class_spell( "Conjure Mana Gem" ) )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ] *= 1.0 + p -> glyphs.conjuring -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    if ( p() -> glyphs.mana_gem -> ok() )
    {
      p() -> mana_gem_charges = 10;
    }
    else
    {
      p() -> mana_gem_charges = 3;
    }
  }

  virtual bool ready()
  {
    if ( p() -> glyphs.mana_gem -> ok() && p() -> mana_gem_charges == 10 )
    {
      return false;
    }
    else if ( !p() -> glyphs.mana_gem -> ok() && p() -> mana_gem_charges == 3 )
    {
      return false;
    }

    return mage_spell_t::ready();
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

struct evocation_t : public mage_spell_t
{
  evocation_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "evocation", p, p -> talents.rune_of_power -> ok() ? spell_data_t::not_found() : p -> find_class_spell( "Evocation" ) )
  {
    parse_options( NULL, options_str );

    base_tick_time    = timespan_t::from_seconds( 2.0 );
    num_ticks         = ( int ) ( data().duration() / base_tick_time );
    tick_zero         = true;
    channeled         = true;
    harmful           = false;
    hasted_ticks      = false;

    cooldown = p -> cooldowns.evocation;
    cooldown -> duration += p -> talents.invocation -> effectN( 1 ).time_value();

  }

  virtual void tick( dot_t* d )
  {
    mage_spell_t::tick( d );

    // FIXME: Nether Attunement should increase Evocation gains. Something like
    //        this worked for mana gems, but doesn't seem to work here.
    double mana = player -> resources.max[ RESOURCE_MANA ] * data().effectN( 1 ).percent() / player -> composite_spell_haste();
    player -> resource_gain( RESOURCE_MANA, mana, p() -> gains.evocation );
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );

    if ( d -> current_tick == d -> num_ticks ) // only trigger invocation if dot has successfully finished all ticks
      p() -> buffs.invocation -> trigger();
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    // evocation automatically causes a switch to dpm rotation
    if ( p() -> rotation.current == ROTATION_DPS )
    {
      p() -> rotation.dps_time += ( sim -> current_time - p() -> rotation.last_time );
    }
    else if ( p() -> rotation.current == ROTATION_DPM )
    {
      p() -> rotation.dpm_time += ( sim -> current_time - p() -> rotation.last_time );
    }
    p() -> rotation.last_time = sim -> current_time;

    if ( sim -> log )
      sim -> output( "%s switches to DPM spell rotation", player -> name() );
    p() -> rotation.current = ROTATION_DPM;
  }
};

// Fire Blast Spell =========================================================

struct fire_blast_t : public mage_spell_t
{
  fire_blast_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "fire_blast", p, p -> find_class_spell( "Fire Blast" ) )
  {
    parse_options( NULL, options_str );
    may_hot_streak = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new fire_blast_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }
};

// Fireball Spell ===========================================================

struct fireball_t : public mage_spell_t
{
  fireball_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "fireball", p, p -> find_class_spell( "Fireball" ) )
  {
    parse_options( NULL, options_str );
    may_hot_streak = true;

    if ( p -> set_bonus.pvp_4pc_caster() )
      base_multiplier *= 1.05;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new fireball_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> set_bonus.tier13_2pc_caster() )
        p() -> buffs.tier13_2pc -> trigger( 1, buff_t::DEFAULT_VALUE(), 0.5 );
    }
  }

  virtual timespan_t travel_time()
  {
    timespan_t t = mage_spell_t::travel_time();
    return ( t > timespan_t::from_seconds( 0.75 ) ? timespan_t::from_seconds( 0.75 ) : t );
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    trigger_ignite( this, s );
  }

  virtual double composite_crit()
  {
    double c = mage_spell_t::composite_crit();

    c *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return c;
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double tm = mage_spell_t::composite_target_multiplier( target );

    if ( td( target ) -> debuffs.pyromaniac -> up() )
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
    mage_spell_t( "flamestrike", p, p -> find_class_spell( "Flamestrike" ) )
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
  frost_bomb_explosion_t( mage_t* p, bool dtr = false ) :
    mage_spell_t( "frost_bomb_explosion", p, p -> find_spell( p -> talents.frost_bomb -> effectN( 2 ).base_value() ) )
  {
    aoe = -1;
    base_aoe_multiplier = 0.5; // This is stored in effectN( 2 ), but it's just 50% of effectN( 1 )
    background = true;
    parse_effect_data( data().effectN( 1 ) );

    // FIXME: Assuming this triggers from dtr like Living Bomb's Explosion
    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new frost_bomb_explosion_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual resource_e current_resource()
  { return RESOURCE_NONE; }
};

struct frost_bomb_t : public mage_spell_t
{
  timespan_t original_cooldown;
  frost_bomb_explosion_t* explosion_spell;

  frost_bomb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frost_bomb", p, p -> talents.frost_bomb ),
    explosion_spell( 0 )
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
    p() -> buffs.brain_freeze -> trigger();
    d -> cancel();
  }
};

// Frostbolt Spell ==========================================================

struct mini_frostbolt_t : public mage_spell_t
{
  mini_frostbolt_t( mage_t* p, bool bolt_two = false ) :
    mage_spell_t( "mini_frostbolt", p, p -> find_spell( 131079 ) )
  {
    background = true;
    dual = true;
    base_costs[ RESOURCE_MANA ] = 0;

    if ( p -> set_bonus.pvp_4pc_caster() )
      base_multiplier *= 1.05;

    if ( ! bolt_two )
    {
      execute_action = new mini_frostbolt_t( p, true );
    }
  }

  virtual timespan_t execute_time()
  { return timespan_t::from_seconds( 0.25 ); }
};

struct frostbolt_t : public mage_spell_t
{
  mini_frostbolt_t* mini_frostbolt;

  frostbolt_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "frostbolt", p, p -> find_class_spell( "Frostbolt" ) )
  {
    parse_options( NULL, options_str );

    if ( p -> set_bonus.pvp_4pc_caster() )
      base_multiplier *= 1.05;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new frostbolt_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }

    mini_frostbolt = new mini_frostbolt_t( p );
    add_child( mini_frostbolt );
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      double fof_proc_chance = p() -> buffs.fingers_of_frost -> data().effectN( 1 ).percent();
      if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
      {
        fof_proc_chance *= 1.2;
      }
      p() -> buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), fof_proc_chance );
      if ( p() -> set_bonus.tier13_2pc_caster() )
      {
        p() -> buffs.tier13_2pc -> trigger( 1, buff_t::DEFAULT_VALUE(), 0.5 );
      }

      if ( p() -> glyphs.icy_veins -> ok() && p() -> buffs.icy_veins -> up() )
      {
        mini_frostbolt -> pre_execute_state = mini_frostbolt -> get_state( execute_state );
        mini_frostbolt -> schedule_execute();
      }
    }
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs.frostbolt -> trigger( 1, buff_t::DEFAULT_VALUE(), 1 );
    }
  }

  virtual double action_multiplier()
  {
    double am = mage_spell_t::action_multiplier();

    if ( p() -> glyphs.icy_veins -> ok() && p() -> buffs.icy_veins -> up() )
    {
      am *= 0.4;
    }

    return am;
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double tm = mage_spell_t::composite_target_multiplier( target );

    tm *= 1.0 + td( target ) -> debuffs.frostbolt -> stack() * 0.05;

    return tm;
  }

};

// Frostfire Bolt Spell =====================================================

struct mini_frostfire_bolt_t : public mage_spell_t
{
  mini_frostfire_bolt_t( mage_t* p, bool bolt_two = false ) :
    mage_spell_t( "mini_frostfire_bolt", p, p -> find_spell( 131081 ) )
  {
    background = true;
    dual = true;
    base_costs[ RESOURCE_MANA ] = 0;

    if ( p -> set_bonus.pvp_4pc_caster() )
      base_multiplier *= 1.05;

    if ( ! bolt_two )
    {
      execute_action = new mini_frostfire_bolt_t( p, true );
    }
  }

  virtual timespan_t execute_time()
  { return timespan_t::from_seconds( 0.25 ); }

  virtual timespan_t travel_time()
  {
    timespan_t t = mage_spell_t::travel_time();
    return ( t > timespan_t::from_seconds( 0.75 ) ? timespan_t::from_seconds( 0.75 ) : t );
  }
};

struct frostfire_bolt_t : public mage_spell_t
{
  mini_frostfire_bolt_t* mini_frostfire_bolt;

  frostfire_bolt_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "frostfire_bolt", p, p -> find_spell( 44614 ) )
  {
    parse_options( NULL, options_str );

    may_hot_streak = true;
    base_execute_time += p -> glyphs.frostfire -> effectN( 1 ).time_value();

    mini_frostfire_bolt = new mini_frostfire_bolt_t( p );
    add_child( mini_frostfire_bolt );

    if ( p -> set_bonus.pvp_4pc_caster() )
      base_multiplier *= 1.05;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new frostfire_bolt_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual double cost()
  {
    if ( p() -> buffs.brain_freeze -> check() )
      return 0;

    return mage_spell_t::cost();
  }

  virtual timespan_t execute_time()
  {
    if ( p() -> buffs.brain_freeze -> check() )
      return timespan_t::zero();

    return mage_spell_t::execute_time();
  }

  virtual void execute()
  {
    // Brain Freeze treats the target as frozen
    frozen = p() -> buffs.brain_freeze -> check() > 0;

    mage_spell_t::execute();

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
        mini_frostfire_bolt -> pre_execute_state = mini_frostfire_bolt -> get_state( execute_state );
        mini_frostfire_bolt -> schedule_execute();
      }
    }
    p() -> buffs.brain_freeze -> expire();
  }

  virtual timespan_t travel_time()
  {
    timespan_t t = mage_spell_t::travel_time();
    return ( t > timespan_t::from_seconds( 0.75 ) ? timespan_t::from_seconds( 0.75 ) : t );
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> set_bonus.tier13_2pc_caster() )
        p() -> buffs.tier13_2pc -> trigger( 1, buff_t::DEFAULT_VALUE(), 0.5 );

      trigger_ignite( this, s );
    }
  }

  virtual double composite_crit()
  {
    double c = mage_spell_t::composite_crit();

    c *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return c;
  }

  virtual double action_multiplier()
  {
    double am = mage_spell_t::action_multiplier();

    if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
    {
      am *= 0.4;
    }

    return am;
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double tm = mage_spell_t::composite_target_multiplier( target );

    if ( td( target ) -> debuffs.pyromaniac -> up() )
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
  frozen_orb_bolt_t* bolt;

  frozen_orb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frozen_orb", p, p -> find_class_spell( "Frozen Orb" ) ),
    bolt( 0 )
  {
    check_spec( MAGE_FROST );
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

// Ice Floes Spell ===================================================

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

  virtual bool ready()
  {
    return mage_spell_t::ready();
  }
};

// Ice Lance Spell ==========================================================

struct mini_ice_lance_t : public mage_spell_t
{
  mini_ice_lance_t( mage_t* p, bool lance_two = false ) :
    mage_spell_t( "mini_ice_lance", p, p -> find_spell( 131080 ) )
  {
    background = true;
    dual = true;
    base_costs[ RESOURCE_MANA ] = 0;

    if ( ! lance_two )
    {
      execute_action = new mini_ice_lance_t( p, true );
    }
  }

  virtual timespan_t execute_time()
  { return timespan_t::from_seconds( 0.25 ); }
};

struct ice_lance_t : public mage_spell_t
{
  double fof_multiplier;
  mini_ice_lance_t* mini_ice_lance;

  ice_lance_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "ice_lance", p, p -> find_class_spell( "Ice Lance" ) ),
    fof_multiplier( 0 )
  {
    parse_options( NULL, options_str );

    aoe = p -> glyphs.ice_lance -> effectN( 1 ).base_value();
    if ( aoe ) ++aoe;

    base_aoe_multiplier *= p -> glyphs.ice_lance -> effectN( 2 ).percent();

    fof_multiplier = p -> find_specialization_spell( "Fingers of Frost" ) -> ok() ? p -> find_spell( 44544 ) -> effectN( 2 ).percent() : 0.0;

    mini_ice_lance = new mini_ice_lance_t( p );
    add_child( mini_ice_lance );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new ice_lance_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    // Ice Lance treats the target as frozen with FoF up
    frozen = p() -> buffs.fingers_of_frost -> check() > 0;

    mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) &&
         p() -> glyphs.icy_veins -> ok() &&
         p() -> buffs.icy_veins -> up() )
    {
      mini_ice_lance -> pre_execute_state = mini_ice_lance -> get_state( execute_state );
      mini_ice_lance -> schedule_execute();
    }

    p() -> buffs.fingers_of_frost -> decrement();
  }

  virtual double action_multiplier()
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

    if ( p() -> set_bonus.tier14_2pc_caster() )
    {
      am *= 1.12;
    }

    return am;
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double tm = mage_spell_t::composite_target_multiplier( target );

    tm *= 1.0 + td( target ) -> debuffs.frostbolt -> stack() * 0.05;

    return tm;
  }

};

// Icy Veins Buff ===========================================================

struct icy_veins_buff_t : public buff_t
{
  icy_veins_buff_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "icy_veins", p -> find_class_spell( "Icy Veins" ) ) )
  {
    cooldown -> duration = timespan_t::zero(); // CD is managed by the spell
  }

  virtual void expire()
  {
    buff_t::expire();

    mage_t* p = debug_cast<mage_t*>( player );
    p -> buffs.tier13_2pc -> expire();
  }
};

// Icy Veins Spell ==========================================================

struct icy_veins_t : public mage_spell_t
{
  timespan_t orig_duration;

  icy_veins_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "icy_veins", p, p -> find_class_spell( "Icy Veins" ) ),
    orig_duration( timespan_t::zero() )
  {
    check_spec( MAGE_FROST );
    parse_options( NULL, options_str );
    harmful = false;

    if ( player -> set_bonus.tier14_4pc_caster() )
    {
      cooldown -> duration *= 0.5;
    }

    orig_duration = cooldown -> duration;
  }

  virtual void execute()
  {
    if ( player -> set_bonus.tier13_4pc_caster() )
    {
      cooldown -> duration = orig_duration * ( 1.0 - p() -> buffs.tier13_2pc -> check() * p() -> spells.stolen_time -> effectN( 1 ).base_value() );
    }

    mage_spell_t::execute();

    p() -> buffs.icy_veins -> trigger();
  }
};

// Inferno Blast Spell ======================================================

struct inferno_blast_t : public mage_spell_t
{
  inferno_blast_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "inferno_blast", p, p -> find_class_spell( "Inferno Blast" ) )
  {
    check_spec( MAGE_FIRE );
    parse_options( NULL, options_str );
    may_hot_streak = true;
    cooldown = p -> cooldowns.inferno_blast;
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    trigger_ignite( this, s );
  }

  virtual double crit_chance( double /* crit */, int /* delta_level */ )
  {
    // Inferno Blast always crits
    return 1.0;
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double tm = mage_spell_t::composite_target_multiplier( target );

    if ( td( target ) -> debuffs.pyromaniac -> up() )
    {
      tm *= 1.1;
    }

    return tm;
  }

  // FIX ME: Add spreading of Pyro, Ignite, Flamestrike, Combustion
};

// Living Bomb Spell ========================================================

struct living_bomb_explosion_t : public mage_spell_t
{
  living_bomb_explosion_t( mage_t* p, bool dtr=false ) :
    mage_spell_t( "living_bomb_explosion", p, p -> find_spell( p -> talents.living_bomb -> effectN( 2 ).base_value() ) )
  {
    aoe = 3;
    background = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new living_bomb_explosion_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual resource_e current_resource()
  { return RESOURCE_NONE; }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );
  }
};

struct living_bomb_t : public mage_spell_t
{
  living_bomb_explosion_t* explosion_spell;

  living_bomb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "living_bomb", p, p -> talents.living_bomb )
  {
    parse_options( NULL, options_str );

    dot_behavior = DOT_REFRESH;

    trigger_gcd = timespan_t::from_seconds( 1.0 );

    explosion_spell = new living_bomb_explosion_t( p );
    add_child( explosion_spell );
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

    p() -> buffs.brain_freeze -> trigger();
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );

    explosion_spell -> execute();
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

// Mana Gem =================================================================

struct mana_gem_t : public action_t
{
  double min;
  double max;

  mana_gem_t( mage_t* p, const std::string& options_str ) :
    action_t( ACTION_USE, "mana_gem", p ), min ( 0 ), max( 0 )
  {
    parse_options( NULL, options_str );

    // FIXME: These currently always return 1, either need to figure out the DBC information
    // or hardcode them
    min = p -> find_spell( 5405 ) -> effectN( 1 ).min( p );
    max = p -> find_spell( 5405 ) -> effectN( 1 ).max( p );

    cooldown -> duration = timespan_t::from_seconds( 120.0 );
    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = static_cast<mage_t*>( player );

    if ( sim -> log ) sim -> output( "%s uses Mana Gem", p -> name() );

    p -> procs.mana_gem -> occur();
    p -> mana_gem_charges--;

    double gain = sim -> range( min, max ) / p -> composite_spell_haste();

    player -> resource_gain( RESOURCE_MANA, gain, p -> gains.mana_gem );

    update_ready();
  }

  virtual bool ready()
  {
    mage_t* p = static_cast<mage_t*>( player );

    if ( p -> mana_gem_charges <= 0 )
      return false;

    if ( ( player -> resources.max[ RESOURCE_MANA ] - player -> resources.current[ RESOURCE_MANA ] ) < max )
      return false;

    return action_t::ready();
  }
};

// Mirror Image Spell ====================================================

struct mirror_image_t : public mage_spell_t
{
  mirror_image_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "mirror_image", p, p -> find_class_spell( "Mirror Image" ) )
  {
    parse_options( NULL, options_str );
    num_ticks = 0;
    harmful = false;
  }

  virtual void init()
  {
    mage_spell_t::init();

    for ( int i = 0; i < 3; i++ )
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
      for ( int i = 0; i < 3; i++ )
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

// Nether Tempest ===========================================================

struct nether_tempest_t : public mage_spell_t
{
  // FIXME: Add extra AOE component id = 114954
  // NOTE: Hits one extra target. Snapshots stats each time it fires.
  nether_tempest_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "nether_tempest", p, p -> talents.nether_tempest )
  {
    parse_options( NULL, options_str );
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

  virtual bool ready()
  {
    // Can't use PoM while AP is up
    if ( p() -> buffs.arcane_power -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Pyroblast Spell ==========================================================

struct pyroblast_t : public mage_spell_t
{
  pyroblast_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "pyroblast", p, p -> find_class_spell( "Pyroblast" ) )
  {
    check_spec( MAGE_FIRE );
    parse_options( NULL, options_str );
    may_hot_streak = true;
    dot_behavior = DOT_REFRESH;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new pyroblast_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual timespan_t execute_time()
  {
    timespan_t a = mage_spell_t::execute_time();

    if ( p() -> buffs.pyroblast -> up() )
    {
      return timespan_t::zero();
    }

    return a;
  }

  virtual double cost()
  {
    if ( p() -> buffs.pyroblast -> check() )
      return 0.0;

    return mage_spell_t::cost();
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> buffs.pyroblast -> expire();
  }

  virtual timespan_t travel_time()
  {
    timespan_t t = mage_spell_t::travel_time();
    return ( t > timespan_t::from_seconds( 0.75 ) ? timespan_t::from_seconds( 0.75 ) : t );
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    trigger_ignite( this, s );

    if ( result_is_hit( s -> result ) )
    {
      if ( player -> set_bonus.tier13_2pc_caster() )
        p() -> buffs.tier13_2pc -> trigger( 1, buff_t::DEFAULT_VALUE(), 0.5 );
    }
  }

  virtual double composite_crit()
  {
    double c = mage_spell_t::composite_crit();

    c *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return c;
  }

  virtual double action_multiplier()
  {
    double am = mage_spell_t::action_multiplier();

    if ( p() -> buffs.pyroblast -> up() )
    {
      am *= 1.25;
    }

    if ( p() -> set_bonus.tier14_2pc_caster() )
    {
      am *= 1.08;
    }

    return am;
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double tm = mage_spell_t::composite_target_multiplier( target );

    if ( td( target ) -> debuffs.pyromaniac -> up() )
    {
      tm *= 1.1;
    }

    return tm;
  }

};

// Invocation ============================================================

struct invocation_t : public mage_spell_t
{
  invocation_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "invocation", p, p -> talents.invocation )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    // Pre-invocation "spell" is only valid in pre-combat with the talent
    if ( ! p() -> in_combat && p() -> talents.invocation -> ok() )
    {
      // get option param
      timespan_t delta = timespan_t::from_seconds( pre_cast );

      // Trigger buff with temporarily reduced duration
      // Technically evocation cooldown should be triggered but irrelevent in practice
      p() -> buffs.invocation -> buff_duration -= delta;
      p() -> buffs.invocation -> trigger();

      mage_spell_t::execute();

      p() -> buffs.invocation -> buff_duration += delta;
    }
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
  scorch_t( mage_t* p, const std::string& options_str, bool dtr=false ) :
    mage_spell_t( "scorch", p, p -> talents.scorch )
  {
    parse_options( NULL, options_str );

    may_hot_streak = true;
    consumes_ice_floes = false;

    if ( p -> set_bonus.pvp_4pc_caster() )
      base_multiplier *= 1.05;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new scorch_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_ignite( this, s );
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

  virtual double composite_crit()
  {
    double c = mage_spell_t::composite_crit();

    c *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return c;
  }

  virtual bool usable_moving()
  { return true; }
};

// Slow Spell ===============================================================

struct slow_t : public mage_spell_t
{
  slow_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "slow", p, p -> find_class_spell( "Slow" ) )
  {
    check_spec( MAGE_ARCANE );
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    td() -> debuffs.slow -> trigger();
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

    for ( size_t i = 0; i < sim -> player_list.size(); ++i )
    {
      player_t* p = sim -> player_list[ i ];
      if ( p -> current.sleeping || p -> buffs.exhaustion -> check() || p -> is_pet() || p -> is_enemy() )
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
    check_spec( MAGE_FROST );
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

    return ! ( p() -> pets.water_elemental && ! p() -> pets.water_elemental -> current.sleeping );
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
      { "cooldown", OPT_TIMESPAN, &( cooldown -> duration ) },
      { "evocation_pct", OPT_FLT, &( evocation_target_mana_percentage ) },
      { "force_dps", OPT_INT, &( force_dps ) },
      { "force_dpm", OPT_INT, &( force_dpm ) },
      { "final_burn_offset", OPT_TIMESPAN, &( final_burn_offset ) },
      { "oom_offset", OPT_FLT, &( oom_offset ) },
      { NULL, OPT_UNKNOWN, NULL }
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
        sim -> output( "%f burn mps, %f time to die", ( p -> rotation.dps_mana_loss / p -> rotation.dps_time.total_seconds() ) - ( p -> rotation.mana_gain / sim -> current_time.total_seconds() ), sim -> target -> time_to_die().total_seconds() );
      }

      if ( force_dps )
      {
        if ( sim -> log ) sim -> output( "%s switches to DPS spell rotation", p -> name() );
        p -> rotation.current = ROTATION_DPS;
      }
      if ( force_dpm )
      {
        if ( sim -> log ) sim -> output( "%s switches to DPM spell rotation", p -> name() );
        p -> rotation.current = ROTATION_DPM;
      }

      update_ready();

      return;
    }

    if ( sim -> log ) sim -> output( "%s Considers Spell Rotation", p -> name() );

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
          if ( sim -> log ) sim -> output( "%s switches to DPM spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPM;
        }
      }
      else
      {
        // We're going until OOM, stop when we can no longer cast full stack AB (approximately, 4 stack with AP can be 6177)
        if ( p -> resources.current[ RESOURCE_MANA ] < 6200 )
        {
          if ( sim -> log ) sim -> output( "%s switches to DPM spell rotation", p -> name() );

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

      // Mana Gem, if we have uses left
      if ( p -> mana_gem_charges > 0 )
      {
        available_mana += p -> dbc.effect_max( 16856, p -> level );
      }

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
          if ( sim -> log ) sim -> output( "%s switches to DPS spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPS;
        }
      }
      else
      {
        // If dps rotation is regen, then obviously use it all the time.

        if ( sim -> log ) sim -> output( "%s switches to DPS spell rotation", p -> name() );

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

// Alter Time Spell ===============================================================

struct alter_time_t : public mage_spell_t
{
  alter_time_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "alter_time_activate", p, p -> find_class_spell( "Alter Time" ) )
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

struct incanters_ward_buff_t : public absorb_buff_t
{
  double max_absorb;
  gain_t* gain;

  incanters_ward_buff_t( mage_t* player ) :
    absorb_buff_t( absorb_buff_creator_t( player, "incanters_ward" ).spell( player -> talents.incanters_ward ) ),
    max_absorb( 0 )
  {
    gain = player -> get_gain( "incanters_ward mana gain" );
  }

  mage_t* p() const
  { return static_cast<mage_t*>( player ); }

  virtual bool trigger( int        stacks,
                        double    /* value */,
                        double     chance,
                        timespan_t duration )
  {
    max_absorb = player -> dbc.effect_average( data().effectN( 1 ).id(), player -> level );
    // coeff hardcoded into tooltip
    max_absorb += p() -> composite_spell_power( SCHOOL_MAX ) * p() -> composite_spell_power_multiplier();

    return absorb_buff_t::trigger( stacks, max_absorb, chance, duration );
  }

  virtual void absorb_used( double amount )
  {
    if ( max_absorb > 0 )
    {
      double resource_gain = amount / max_absorb * 0.18 * p() -> resources.current[ RESOURCE_MANA ];
      p() -> resource_gain( RESOURCE_MANA, resource_gain, gain );
    }

  }

  virtual void expire()
  {

    // Trigger second buff with value between 0 and 1, depending on how much absorb has been used.
    double post_sp_coeff = ( max_absorb - current_value ) / max_absorb;
    p() -> buffs.incanters_ward_post -> trigger( 1, post_sp_coeff );

    absorb_buff_t::expire();
  }
};

// incanters_ward Spell ===============================================================

struct incanters_ward_t : public mage_spell_t
{
  incanters_ward_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "incanters_ward", p, p -> talents.incanters_ward )
  {
    parse_options( NULL, options_str );

    harmful = false;

    base_dd_min = base_dd_max = 0.0;
  }

  virtual void execute()
  {
    p() -> buffs.incanters_ward -> trigger();

    mage_spell_t::execute();
  }
};

} // UNNAMED NAMESPACE

// ==========================================================================
// Mage Character Definition
// ==========================================================================

// mage_td_t ================================================================

mage_td_t::mage_td_t( player_t* target, mage_t* mage ) :
  actor_pair_t( target, mage ),
  dots( dots_t() ),
  debuffs( debuffs_t() )
{
  dots.flamestrike    = target -> get_dot( "flamestrike",    mage );
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
  if ( name == "conjure_mana_gem"  ) return new        conjure_mana_gem_t( this, options_str );
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
  if ( name == "mana_gem"          ) return new                mana_gem_t( this, options_str );
  if ( name == "mirror_image"      ) return new            mirror_image_t( this, options_str );
  if ( name == "molten_armor"      ) return new            molten_armor_t( this, options_str );
  if ( name == "nether_tempest"    ) return new          nether_tempest_t( this, options_str );
  if ( name == "presence_of_mind"  ) return new        presence_of_mind_t( this, options_str );
  if ( name == "pyroblast"         ) return new               pyroblast_t( this, options_str );
  if ( name == "rune_of_power"     ) return new           rune_of_power_t( this, options_str );
  if ( name == "invocation"        ) return new              invocation_t( this, options_str );
  if ( name == "scorch"            ) return new                  scorch_t( this, options_str );
  if ( name == "slow"              ) return new                    slow_t( this, options_str );
  if ( name == "time_warp"         ) return new               time_warp_t( this, options_str );
  if ( name == "water_elemental"   ) return new  summon_water_elemental_t( this, options_str );
  if ( name == "alter_time"        ) return new              alter_time_t( this, options_str );
  if ( name == "incanters_ward"    ) return new          incanters_ward_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// mage_t::create_pet =======================================================

pet_t* mage_t::create_pet( const std::string& pet_name,
                           const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "mirror_image"  ) return new pets::mirror_image_pet_t   ( sim, this );
  if ( pet_name == "water_elemental" ) return new pets::water_elemental_pet_t( sim, this );

  return 0;
}

// mage_t::create_pets ======================================================

void mage_t::create_pets()
{
  pets.water_elemental = create_pet( "water_elemental" );
  for ( int i = 0; i < 3; i++ )
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
  spec.frostburn             = find_mastery_spell( MAGE_FROST );
  spec.ignite                = find_mastery_spell( MAGE_FIRE );
  spec.mana_adept            = find_mastery_spell( MAGE_ARCANE );

  spells.stolen_time         = spell_data_t::find( 105791, "Stolen Time", dbc.ptr );

  if ( spec.ignite -> ok() )
    active_ignite = new ignite_t( this );

  // Glyphs
  glyphs.arcane_brilliance   = find_glyph_spell( "Glyph of Arcane Brilliance" );
  glyphs.arcane_power        = find_glyph_spell( "Glyph of Arcane Power" );
  glyphs.combustion          = find_glyph_spell( "Glyph of Combustion" );
  glyphs.conjuring           = find_glyph_spell( "Glyph of Conjuring" );
  glyphs.frostfire           = find_glyph_spell( "Glyph of Frostfire" );
  glyphs.ice_lance           = find_glyph_spell( "Glyph of Ice Lance" );
  glyphs.icy_veins           = find_glyph_spell( "Glyph of Icy Veins" );
  glyphs.living_bomb         = find_glyph_spell( "Glyph of Living Bomb" );
  glyphs.mana_gem            = find_glyph_spell( "Glyph of Mana Gem" );
  glyphs.mirror_image        = find_glyph_spell( "Glyph of Mirror Image" );

  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
    { 105788, 105790,     0,     0,     0,     0,     0,     0 }, // Tier13
    { 123097, 123101,     0,     0,     0,     0,     0,     0 }, // Tier14
    {      0,      0,     0,     0,     0,     0,     0,     0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// mage_t::init_base ========================================================

void mage_t::init_base()
{
  player_t::init_base();

  initial.spell_power_per_intellect = 1.0;

  base.attack_power = -10;
  initial.attack_power_per_strength = 1.0;

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

void mage_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buffs.arcane_charge        = buff_creator_t( this, "arcane_charge", spec.arcane_charge )
                               .max_stack( find_spell( 36032 ) -> max_stacks() )
                               .duration( find_spell( 36032 ) -> duration() );
  buffs.arcane_missiles      = buff_creator_t( this, "arcane_missiles", find_class_spell( "Arcane Missiles" ) -> ok() ? find_spell( 79683 ) : spell_data_t::not_found() ).chance( 0.3 );
  buffs.arcane_power         = new arcane_power_buff_t( this );
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
  buffs.frost_armor          = buff_creator_t( this, "frost_armor", find_spell( 7302 ) );
  buffs.icy_veins            = new icy_veins_buff_t( this );
  buffs.ice_floes            = buff_creator_t( this, "ice_floes", talents.ice_floes );
  buffs.invocation           = buff_creator_t( this, "invocation", find_spell( 116257 ) ).chance( talents.invocation -> ok() ? 1.0 : 0 );
  buffs.mage_armor           = stat_buff_creator_t( this, "mage_armor" ).spell( find_spell( 6117 ) );
  buffs.molten_armor         = buff_creator_t( this, "molten_armor", find_spell( 30482 ) );
  buffs.presence_of_mind     = buff_creator_t( this, "presence_of_mind", talents.presence_of_mind ).duration( timespan_t::zero() ).activated( true );
  buffs.rune_of_power        = buff_creator_t( this, "rune_of_power", find_spell( 116014 ) ).duration( timespan_t::from_seconds( 60 ) );

  buffs.heating_up           = buff_creator_t( this, "heating_up", find_class_spell( "Pyroblast" ) -> ok() ? find_spell( 48107 ) : spell_data_t::not_found() );
  buffs.pyroblast            = buff_creator_t( this, "pyroblast",  find_class_spell( "Pyroblast" ) -> ok() ? find_spell( 48108 ) : spell_data_t::not_found() );

  buffs.tier13_2pc           = stat_buff_creator_t( this, "tier13_2pc" )
                               .spell( find_spell( 105785 ) );

  buffs.alter_time           = new alter_time::alter_time_buff_t( this );
  buffs.incanters_ward       = new incanters_ward_buff_t( this );
  absorb_buffs.push_back( buffs.incanters_ward );
  buffs.incanters_ward_post  = buff_creator_t( this, "incanters_ward_post" )
                               .spell( find_spell( 116267 ) );

}

// mage_t::init_gains =======================================================

void mage_t::init_gains()
{
  player_t::init_gains();

  gains.evocation              = get_gain( "evocation"              );
  gains.incanters_ward_passive = get_gain( "incanters_ward_passive" );
  gains.mana_gem               = get_gain( "mana_gem"               );
  gains.rune_of_power          = get_gain( "rune_of_power"          );
}

// mage_t::init_procs =======================================================

void mage_t::init_procs()
{
  player_t::init_procs();

  procs.deferred_ignite         = get_proc( "deferred_ignite"         );
  procs.mana_gem                = get_proc( "mana_gem"                );
  procs.test_for_crit_hotstreak = get_proc( "test_for_crit_hotstreak" );
  procs.crit_for_hotstreak      = get_proc( "crit_test_hotstreak"     );
  procs.hotstreak               = get_proc( "hotstreak"               );
}

// mage_t::init_uptimes =====================================================

void mage_t::init_benefits()
{
  player_t::init_benefits();

  benefits.arcane_charge[ 0 ] = get_benefit( "arcane_charge_0"  );
  benefits.arcane_charge[ 1 ] = get_benefit( "arcane_charge_1"  );
  benefits.arcane_charge[ 2 ] = get_benefit( "arcane_charge_2"  );
  benefits.arcane_charge[ 3 ] = get_benefit( "arcane_charge_3"  );
  benefits.arcane_charge[ 4 ] = get_benefit( "arcane_charge_4"  );
  benefits.arcane_charge[ 5 ] = get_benefit( "arcane_charge_5"  );
  benefits.arcane_charge[ 6 ] = get_benefit( "arcane_charge_6"  );
  benefits.dps_rotation      = get_benefit( "dps_rotation"    );
  benefits.dpm_rotation      = get_benefit( "dpm_rotation"    );
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
    *str += "/" + dbc_t::get_token( s -> id() );
    if ( ! options.empty() ) *str += "," + options;
  }
}

// mage_t::init_actions =====================================================

void mage_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

#if 0 // UNUSED
    // Shard of Woe check for Arcane
    bool has_shard = false;
    for ( int i=0; i < SLOT_MAX; i++ )
    {
      item_t& item = items[ i ];
      if ( strstr( item.name(), "shard_of_woe" ) )
      {
        has_shard = true;
        break;
      }
    }
#endif

    std::string& precombat = get_action_priority_list( "precombat" ) -> action_list_str;
    std::string item_actions = init_use_item_actions();
    std::string profession_actions = init_use_profession_actions();

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
    if ( specialization() == MAGE_ARCANE )
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

    // Counterspell
    add_action( "Counterspell", "if=target.debuff.casting.react" );

    // Prevent unsafe Alter Time teleport while moving
    // FIXME: realistically for skilled players using DBM, warning would be available to suppress Alter Time for 6 seconds before moving
    action_list_str += "/cancel_buff,name=alter_time,moving=1";

    // Cold Snap
    if ( talents.cold_snap -> ok() )
    {
      add_action( "Cold Snap", "if=health.pct<30" );
    }

    // Refresh Gem during invuln phases
    if ( level >= 47 )
    {
      add_action( "Conjure Mana Gem", "if=mana_gem_charges<3&target.debuff.invulnerable.react" );
    }

    if ( level >= 85 )
      action_list_str +="/time_warp,if=target.health.pct<25|time>5";

    // Spec-specific actions

    // Arcane
    if ( specialization() == MAGE_ARCANE )
    {
      add_action( "Arcane Power", "if=target.time_to_die<18" );
      if ( race == RACE_ORC )
      {
        action_list_str += "/blood_fury,if=target.time_to_die<12";
      }
      else if ( race == RACE_TROLL )
      {
        action_list_str += "/berserking,if=target.time_to_die<18";
      }
      if ( talents.invocation -> ok() )
      {
        add_action( "Alter Time", "if=buff.alter_time.down&buff.arcane_power.up&buff.arcane_missiles.up&buff.arcane_charge.stack>3&buff.invocation.remains>6,moving=0" );
      }
      else if ( talents.rune_of_power -> ok() )
      {
        add_action( "Alter Time", "if=buff.alter_time.down&buff.arcane_power.up&buff.arcane_missiles.stack=2&buff.arcane_charge.stack>3&buff.rune_of_power.remains>6,moving=0" );
      }
      else if ( talents.incanters_ward -> ok() )
      {
        add_action( "Alter Time", "if=buff.alter_time.down&buff.arcane_power.up&buff.arcane_missiles.stack=2&buff.arcane_charge.stack>3&talent.incanters_ward.enabled,moving=0" );
      }
      else if ( level >= 87 )
      {
        add_action( "Alter Time", "if=buff.alter_time.down&buff.arcane_power.up&buff.arcane_missiles.stack=2&buff.arcane_charge.stack=6,moving=0" );
      }
      add_action( "Arcane Blast", "if=buff.alter_time.up&buff.presence_of_mind.up" );
      add_action( "Arcane MIssiles", "if=buff.alter_time.up|buff.arcane_missiles.stack=2" );
      if ( talents.invocation -> ok() )
      {
        add_action( "Arcane Barrage", "if=talent.invocation.enabled&buff.invocation.remains<gcd" );
      }
      if ( talents.invocation -> ok() )
      {
        if ( talents.frost_bomb -> ok() )
        {
          action_list_str += "/frost_bomb,if=!ticking";
        }
        add_action( "Evocation", "if=buff.invocation.down&buff.alter_time.down" );
      }
      else if ( talents.rune_of_power -> ok() )
      {
        action_list_str += "/rune_of_power,if=buff.rune_of_power.down&buff.alter_time.down";
      }
      else if ( talents.incanters_ward -> ok() )
      {
        action_list_str += "/incanters_ward,if=buff.alter_time.down";
      }
      if ( ( level >= 80 ) && ( sim -> allow_potions ) )
      {
        action_list_str += ( level > 85 ) ? "/jade_serpent_potion" : "/volcanic_potion";
        action_list_str += ",if=buff.arcane_power.up|target.time_to_die<=50";
      }
      action_list_str += "/mana_gem,if=mana.pct<84&buff.alter_time.down";
      add_action( "Mirror Image" );
      if ( !talents.rune_of_power -> ok() )
      {
        add_action( "Evocation", "if=mana.pct<30&target.time_to_die>=15" );
      }
      if ( talents.invocation -> ok() )
      {
        add_action( "Arcane Power", "if=buff.invocation.remains>15&buff.alter_time.down&mana.pct>70&buff.arcane_charge.up" );
        if ( race == RACE_ORC )
        {
          action_list_str += "/blood_fury,if=buff.invocation.remains>15&buff.alter_time.down&mana.pct>70&buff.arcane_charge.up";
        }
        else if ( race == RACE_TROLL )
        {
          action_list_str += "/berserking,if=buff.invocation.remains>10&buff.alter_time.down&mana.pct>70&buff.arcane_charge.up";
        }
        if ( !item_actions.empty() )
        {
          action_list_str += init_use_item_actions();
          action_list_str += ",if=buff.invocation.remains>=15&buff.alter_time.down";
        }
        if ( !profession_actions.empty() )
        {
          action_list_str += init_use_profession_actions();
          action_list_str += ",if=buff.invocation.remains>=15&buff.alter_time.down";
        }
      }
      else if ( talents.rune_of_power -> ok() )
      {
        add_action( "Arcane Power", "if=buff.rune_of_power.remains>15&buff.alter_time.down&buff.arcane_charge.stack>1" );
        if ( race == RACE_ORC )
        {
          action_list_str += "/blood_fury,if=buff.rune_of_power.remains>15&buff.alter_time.down&buff.arcane_charge.stack>2";
        }
        else if ( race == RACE_TROLL )
        {
          action_list_str += "/berserking,if=buff.rune_of_power.remains>10&buff.alter_time.down&buff.arcane_charge.stack>2";
        }
        if ( !item_actions.empty() )
        {
          action_list_str += init_use_item_actions();
          action_list_str += ",if=buff.rune_of_power.remains>15&buff.alter_time.down";
        }
        if ( !profession_actions.empty() )
        {
          action_list_str += init_use_profession_actions();
          action_list_str += ",if=buff.rune_of_power.remains>15&buff.alter_time.down";
        }
      }
      else if ( talents.incanters_ward -> ok() )
      {
        add_action( "Arcane Power", "if=buff.incanters_ward_post.react&buff.alter_time.down&buff.arcane_charge.stack>2" );
        if ( race == RACE_ORC )
        {
          action_list_str += "/blood_fury,if=buff.incanters_ward_post.react&buff.alter_time.down&buff.arcane_charge.stack>2";
        }
        else if ( race == RACE_TROLL )
        {
          action_list_str += "/berserking,if=buff.incanters_ward_post.react&buff.alter_time.down&buff.arcane_charge.stack>2";
        }
        if ( !item_actions.empty() )
        {
          action_list_str += init_use_item_actions();
          action_list_str += ",if=buff.alter_time.down";
        }
        if ( !profession_actions.empty() )
        {
          action_list_str += init_use_profession_actions();
          action_list_str += ",if=buff.alter_time.down";
        }
      }
      else if ( level >= 62 )
      {
        add_action( "Arcane Power", "if=buff.alter_time.down&buff.arcane_charge.stack>2" );
        if ( race == RACE_ORC )
        {
          action_list_str += "/blood_fury,if=buff.alter_time.down&buff.arcane_charge.stack>2";
        }
        else if ( race == RACE_TROLL )
        {
          action_list_str += "/berserking,if=buff.alter_time.down&buff.arcane_charge.stack>2";
        }
        if ( !item_actions.empty() )
        {
          action_list_str += init_use_item_actions();
          action_list_str += ",if=buff.alter_time.down";
        }
        if ( !profession_actions.empty() )
        {
          action_list_str += init_use_profession_actions();
          action_list_str += ",if=buff.alter_time.down";
        }
      }
      if ( talents.presence_of_mind -> ok() )
      {
        action_list_str += "/presence_of_mind,if=buff.alter_time.down";
      }
      if ( talents.nether_tempest -> ok() )
      {
        action_list_str += "/nether_tempest,if=!ticking";
      }
      else if ( talents.living_bomb -> ok() )
      {
        action_list_str += "/living_bomb,if=!ticking";
      }
      else if ( talents.frost_bomb -> ok() & !talents.invocation -> ok() )
      {
        action_list_str += "/frost_bomb,if=!ticking";
      }
      if ( talents.rune_of_power -> ok() )
      {
        add_action( "Arcane Blast", "if=mana.pct>92" );
      }
      add_action( "Arcane Missiles", "if=buff.arcane_missiles.react&(cooldown.alter_time_activate.remains>4|target.time_to_die<10)" );
      if ( talents.invocation -> ok() )
      {
        add_action( "Arcane Barrage", "if=buff.arcane_charge.stack=4&mana.pct<28&buff.arcane_power.down" );
      }
      else if ( talents.rune_of_power -> ok() )
      {
        add_action( "Arcane Barrage", "if=buff.arcane_charge.up&buff.arcane_power.down&buff.alter_time.down&target.time_to_die>25&(cooldown.mana_gem.remains>10|mana_gem_charges=0)" );
        add_action( "Arcane Barrage", "if=buff.arcane_charge.stack>=4&buff.arcane_missiles.down&target.time_to_die>25" );
      }
      else if ( level >= 12 )
      {
        add_action( "Arcane Barrage", "if=buff.arcane_charge.up&buff.arcane_power.down&buff.alter_time.down&(mana.pct<92|cooldown.mana_gem.remains>10|mana_gem_charges=0)" );
      }
      if ( talents.ice_floes -> ok() )
      {
        action_list_str += "/ice_floes,moving=1";
      }
      add_action( "Arcane Blast" );
      add_action( "Arcane Barrage", "moving=1" );
      if ( talents.scorch -> ok() )
      {
        action_list_str += "/scorch,moving=1";
      }
      if ( level >=5 )
      {
        add_action( "Fire Blast", "moving=1" );
      }
      if ( level >= 22 )
      {
        add_action( "Ice Lance", "moving=1" );
      }
    }

    // Fire
    else if ( specialization() == MAGE_FIRE )
    {
      if ( race == RACE_TROLL )
      {
        if ( talents.invocation -> ok() )
        {
          action_list_str += "/berserking,if=buff.invocation.remains>10&buff.alter_time.down&mana.pct>28";
        }
        else if ( talents.rune_of_power -> ok() )
        {
          action_list_str += "/berserking,if=buff.rune_of_power.remains>10&buff.alter_time.down";
        }
        else if ( talents.incanters_ward -> ok() )
        {
          action_list_str += "/berserking,if=buff.incanters_ward_post.react&buff.alter_time.down";
        }
        else
        {
          action_list_str += "/berserking,if=buff.alter_time.down";
        }
      }
      add_action( "Combustion", "if=target.time_to_die<12" );
      add_action( "Combustion", "if=set_bonus.tier14_4pc_caster&dot.ignite.ticking&dot.pyroblast.ticking" );
      add_action( "Combustion", "if=!set_bonus.tier14_4pc_caster&dot.ignite.tick_dmg>=12000&dot.pyroblast.ticking" );
      if ( talents.invocation -> ok() )
      {
        action_list_str += "/evocation,if=buff.invocation.down&buff.alter_time.down";
      }
      else if ( talents.rune_of_power -> ok() )
      {
        action_list_str += "/rune_of_power,if=buff.rune_of_power.down&target.time_to_die>12";
      }
      else if ( talents.incanters_ward -> ok() )
      {
        action_list_str += "/incanters_ward,if=buff.alter_time.down";
      }
      if ( ( level >= 80 ) && ( sim -> allow_potions ) )
      {
        action_list_str += ( level > 85 ) ? "/jade_serpent_potion" : "/volcanic_potion";
        action_list_str += ",if=buff.bloodlust.react|target.time_to_die<=40";
      }
      if ( race == RACE_ORC )
      {
        action_list_str += "/blood_fury,if=target.time_to_die<12";
      }
      else if ( race == RACE_TROLL )
      {
        action_list_str += "/berserking,if=target.time_to_die<18";
      }
      action_list_str += "/mana_gem,if=mana.pct<84&buff.alter_time.down";
      if ( talents.invocation -> ok() )
      {
        add_action( "Alter Time", "if=buff.alter_time.down&buff.pyroblast.react&buff.invocation.remains>6,moving=0" );
      }
      else if ( talents.rune_of_power -> ok() )
      {
        add_action( "Alter Time", "if=buff.alter_time.down&buff.pyroblast.react&buff.rune_of_power.remains>6,moving=0" );
      }
      else if ( level >= 87 )
      {
        add_action( "Alter Time", "if=buff.alter_time.down&buff.pyroblast.react,moving=0" );
      }
      if ( !talents.rune_of_power -> ok() )
      {
        add_action( "Evocation", "if=mana.pct<10&target.time_to_die>=30" );
      }
      if ( race == RACE_ORC )
      {
        if ( talents.invocation -> ok() )
        {
          action_list_str += "/blood_fury,if=buff.invocation.remains>15&buff.alter_time.down&mana.pct>28";
          if ( !item_actions.empty() )
          {
            action_list_str += init_use_item_actions();
            action_list_str += ",if=buff.invocation.remains>=15&buff.alter_time.down";
          }
          if ( !profession_actions.empty() )
          {
            action_list_str += init_use_profession_actions();
            action_list_str += ",if=buff.invocation.remains>=15&buff.alter_time.down";
          }
        }
        else if ( talents.rune_of_power -> ok() )
        {
          action_list_str += "/blood_fury,if=buff.rune_of_power.remains>15&buff.alter_time.down";
          if ( !item_actions.empty() )
          {
            action_list_str += init_use_item_actions();
            action_list_str += ",if=buff.rune_of_power.remains>15&buff.alter_time.down";
          }
          if ( !profession_actions.empty() )
          {
            action_list_str += init_use_profession_actions();
            action_list_str += ",if=buff.rune_of_power.remains>15&buff.alter_time.down";
          }
        }
        else if ( talents.incanters_ward -> ok() )
        {
          action_list_str += "/blood_fury,if=buff.incanters_ward_post.react&buff.alter_time.down";
          if ( !item_actions.empty() )
          {
            action_list_str += init_use_item_actions();
            action_list_str += ",if=buff.alter_time.down";
          }
          if ( !profession_actions.empty() )
          {
            action_list_str += init_use_profession_actions();
            action_list_str += ",if=buff.alter_time.down";
          }
        }
        else
        {
          action_list_str += "/blood_fury,if=buff.alter_time.down";
          if ( !item_actions.empty() )
          {
            action_list_str += init_use_item_actions();
            action_list_str += ",if=buff.alter_time.down";
          }
          if ( !profession_actions.empty() )
          {
            action_list_str += init_use_profession_actions();
            action_list_str += ",if=buff.alter_time.down";
          }
        }
      }
      if ( level >= 87 )
      {
        add_action( "Pyroblast", "if=buff.pyroblast.react&(cooldown.alter_time_activate.remains>4|buff.heating_up.react)" );
        add_action( "Pyroblast", "if=buff.presence_of_mind.up&cooldown.alter_time_activate.remains>4" );
      }
      else if ( level >= 10 )
      {
        add_action( "Pyroblast", "if=buff.pyroblast.react" );
      }
      add_action( "Inferno Blast", "if=buff.heating_up.react&buff.pyroblast.down" );
      add_action( "Mirror Image" );
      if ( talents.presence_of_mind -> ok() )
      {
        action_list_str += "/presence_of_mind,if=buff.alter_time.down";
      }
      if ( talents.nether_tempest -> ok() )
      {
        action_list_str += "/nether_tempest,if=!ticking";
      }
      else if ( talents.living_bomb -> ok() )
      {
        action_list_str += "/living_bomb,if=!ticking";
      }
      else if ( talents.frost_bomb -> ok() )
      {
        action_list_str += "/frost_bomb,if=!ticking";
      }
      if ( talents.ice_floes -> ok() )
      {
        action_list_str += "/ice_floes,moving=1";
      }
      add_action( "Fireball" );
      if ( talents.scorch -> ok() )
      {
        action_list_str += "/scorch,moving=1";
      }
      add_action( "Inferno Blast", "moving=1" );
      if ( level >= 22 )
      {
        add_action( "Ice Lance", "moving=1" );
      }
    }

    // Frost
    else if ( specialization() == MAGE_FROST )
    {
      if ( talents.presence_of_mind -> ok() )
      {
        action_list_str += "/presence_of_mind,if=buff.alter_time.down";
      }
      action_list_str += "/water_elemental:freeze,if=buff.alter_time.down&buff.fingers_of_frost.stack<2";
      add_action( "Icy Veins", "if=target.time_to_die<22" );
      if ( race == RACE_ORC )
      {
        action_list_str += "/blood_fury,if=target.time_to_die<12";
      }
      else if ( race == RACE_TROLL )
      {
        action_list_str += "/berserking,if=target.time_to_die<18";
      }
      if ( level >= 87 )
      {
        add_action( "Frostfire Bolt", "if=buff.alter_time.up&buff.brain_freeze.up" );
        add_action( "Ice Lance", "if=buff.alter_time.up&buff.fingers_of_frost.react" );
        add_action( "Frostbolt", "if=buff.alter_time.up&buff.presence_of_mind.up" );
      }
      if ( talents.invocation -> ok() )
      {
        add_action( "Ice Lance", "if=buff.fingers_of_frost.react&buff.fingers_of_frost.remains<5" );
        if ( level >= 81 ) add_action( "Frozen Orb", "if=target.time_to_die>=4&buff.fingers_of_frost.stack<2&cooldown.icy_veins.remains<gcd&buff.invocation.remains>20&buff.alter_time.down" );
        add_action( "Icy Veins", "if=set_bonus.tier14_4pc_caster&buff.invocation.remains>20&buff.alter_time.down" );
        add_action( "Icy Veins", "if=!set_bonus.tier14_4pc_caster&dot.frozen_orb.ticking" );
      }
      if ( talents.frost_bomb -> ok() )
      {
        action_list_str += "/frost_bomb,if=!ticking";
      }
      if ( talents.rune_of_power -> ok() )
      {
        if ( level >= 81 ) add_action( "Frozen Orb", "if=target.time_to_die>=4&buff.fingers_of_frost.stack<2&cooldown.icy_veins.remains<gcd&buff.rune_of_power.remains>20&buff.alter_time.down" );
        add_action( "Icy Veins", "if=set_bonus.tier14_4pc_caster&buff.rune_of_power.remains>20&buff.alter_time.down" );
        add_action( "Icy Veins", "if=!set_bonus.tier14_4pc_caster&dot.frozen_orb.ticking" );
      }
      else if ( talents.incanters_ward -> ok() )
      {
        if ( level >= 81 ) add_action( "Frozen Orb", "if=target.time_to_die>=4&buff.fingers_of_frost.stack<2&cooldown.icy_veins.remains<gcd&buff.alter_time.down" );
        add_action( "Icy Veins", "if=set_bonus.tier14_4pc_caster&buff.alter_time.down" );
        add_action( "Icy Veins", "if=!set_bonus.tier14_4pc_caster&dot.frozen_orb.ticking" );
      }
      else if ( level >= 62 )
      {
        add_action( "Icy Veins", "if=dot.frozen_orb.ticking&buff.alter_time.down" );
      }
      add_action( "Mirror Image" );
      if ( talents.invocation -> ok() )
      {
        add_action( "Evocation", "if=buff.invocation.down&buff.alter_time.down" );
      }
      add_action( "Ice Lance", "if=buff.fingers_of_frost.react&buff.fingers_of_frost.remains<2" );
      if ( talents.rune_of_power -> ok() )
      {
        action_list_str += "/rune_of_power,if=buff.rune_of_power.down&buff.alter_time.down";
      }
      if ( talents.incanters_ward -> ok() )
      {
        action_list_str += "/incanters_ward";
      }
      if ( ( level >= 80 ) && ( sim -> allow_potions ) )
      {
        action_list_str += ( level > 85 ) ? "/jade_serpent_potion" : "/volcanic_potion";
        action_list_str += ",if=buff.bloodlust.react|buff.icy_veins.up|target.time_to_die<=40";
      }
      if ( talents.invocation -> ok() )
      {
        if ( race == RACE_ORC )
        {
          action_list_str += "/blood_fury,if=buff.invocation.remains>15&buff.alter_time.down&mana.pct>28";
        }
        else if ( race == RACE_TROLL )
        {
          action_list_str += "/berserking,if=buff.invocation.remains>10&buff.alter_time.down&mana.pct>28";
        }
        if ( !item_actions.empty() )
        {
          action_list_str += init_use_item_actions();
          action_list_str += ",if=buff.invocation.remains>=15&buff.alter_time.down";
        }
        if ( !profession_actions.empty() )
        {
          action_list_str += init_use_profession_actions();
          action_list_str += ",if=buff.invocation.remains>=15&buff.alter_time.down";
        }
        add_action( "Frostbolt", "if=debuff.frostbolt.stack<3" );
        add_action( "Alter Time", "if=buff.alter_time.down&buff.brain_freeze.react&buff.fingers_of_frost.react&buff.invocation.remains>6,moving=0" );
      }
      else if ( talents.rune_of_power -> ok() )
      {
        if ( race == RACE_ORC )
        {
          action_list_str += "/blood_fury,if=buff.rune_of_power.remains>15&buff.alter_time.down";
        }
        else if ( race == RACE_TROLL )
        {
          action_list_str += "/berserking,if=buff.rune_of_power.remains>10&buff.alter_time.down";
        }
        if ( !item_actions.empty() )
        {
          action_list_str += init_use_item_actions();
          action_list_str += ",if=buff.rune_of_power.remains>15&buff.alter_time.down";
        }
        if ( !profession_actions.empty() )
        {
          action_list_str += init_use_profession_actions();
          action_list_str += ",if=buff.rune_of_power.remains>15&buff.alter_time.down";
        }
        add_action( "Frostbolt", "if=debuff.frostbolt.stack<3" );
        add_action( "Alter Time", "if=buff.alter_time.down&buff.brain_freeze.react&buff.fingers_of_frost.react&buff.rune_of_power.remains>6,moving=0" );
      }
      else if ( talents.incanters_ward -> ok() )
      {
        if ( race == RACE_ORC )
        {
          action_list_str += "/blood_fury,if=buff.incanters_ward_post.react&buff.alter_time.down";
        }
        else if ( race == RACE_TROLL )
        {
          action_list_str += "/berserking,if=buff.incanters_ward_post.react&buff.alter_time.down";
        }
        if ( !item_actions.empty() )
        {
          action_list_str += init_use_item_actions();
          action_list_str += ",if=buff.alter_time.down";
        }
        if ( !profession_actions.empty() )
        {
          action_list_str += init_use_profession_actions();
          action_list_str += ",if=buff.alter_time.down";
        }
        add_action( "Frostbolt", "if=debuff.frostbolt.stack<3" );
        add_action( "Alter Time", "if=buff.alter_time.down&buff.brain_freeze.react&buff.fingers_of_frost.react,moving=0" );
      }
      else
      {
        if ( race == RACE_ORC )
        {
          action_list_str += "/blood_fury,if=buff.alter_time.down";
        }
        else if ( race == RACE_TROLL )
        {
          action_list_str += "/berserking,if=buff.alter_time.down";
        }
        if ( !item_actions.empty() )
        {
          action_list_str += init_use_item_actions();
          action_list_str += ",if=buff.alter_time.down";
        }
        if ( !profession_actions.empty() )
        {
          action_list_str += init_use_profession_actions();
          action_list_str += ",if=buff.alter_time.down";
        }
        add_action( "Frostbolt", "if=debuff.frostbolt.stack<3" );
      }
      if ( level >= 62 )
      {
        add_action( "Alter Time", "if=buff.alter_time.down&buff.brain_freeze.react&buff.fingers_of_frost.react,moving=0" );
      }
      if ( talents.nether_tempest -> ok() )
      {
        action_list_str += "/nether_tempest,if=!ticking";
      }
      else if ( talents.living_bomb -> ok() )
      {
        action_list_str += "/living_bomb,if=!ticking";
      }
      if ( level >= 87 )
      {
        add_action( "Frostfire Bolt", "if=buff.brain_freeze.react&(buff.alter_time.up|cooldown.alter_time_activate.remains>4)" );
        add_action( "Ice Lance", "if=buff.fingers_of_frost.react&(buff.alter_time.up|cooldown.alter_time_activate.remains>4)" );
      }
      else
      {
        add_action( "Frostfire Bolt", "if=buff.brain_freeze.react" );
      }
      add_action( "Ice Lance", "if=buff.fingers_of_frost.react" );
      if ( level >= 81 ) add_action( "Frozen Orb", "if=target.time_to_die>=4&buff.fingers_of_frost.stack<2" );
      action_list_str += "/mana_gem,if=mana.pct<84&buff.alter_time.down";
      if ( !talents.invocation -> ok() && !talents.rune_of_power -> ok() )
      {
        add_action( "Evocation", "if=mana.pct<10&target.time_to_die>=30" );
      }
      if ( talents.ice_floes -> ok() )
      {
        action_list_str += "/ice_floes,moving=1";
      }
      add_action( "Frostbolt" );
      if ( talents.scorch -> ok() )
      {
        action_list_str += "/scorch,moving=1";
      }
      if ( level >= 5 )
      {
        add_action( "Fire Blast", "moving=1" );
      }
      if ( level >= 22 )
      {
        add_action( "Ice Lance", "moving=1" );
      }
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// mage_t::mana_regen_per_second ====================================================

double mage_t::mana_regen_per_second()
{
  double mp5 = player_t::mana_regen_per_second();

  if ( passives.nether_attunement -> ok() )
    mp5 /= mage_t::composite_spell_haste();

  if ( talents.invocation -> ok() )
    mp5 /= 2.0;

  return mp5;
}

// mage_t::composite_player_multipler =======================================

double mage_t::composite_player_multiplier( school_e school, action_t* a )
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( buffs.arcane_power -> check() )
  {
    double v = buffs.arcane_power -> value();
    if ( set_bonus.tier14_4pc_caster() )
    {
      v += 0.1;
    }
    m *= 1.0 + v;
  }

  if ( buffs.invocation -> up() )
  {
    m *= 1.0 + buffs.invocation -> data().effectN( 1 ).percent();
  }
  else if ( buffs.rune_of_power -> check() )
  {
    m *= 1.0 + buffs.rune_of_power -> data().effectN( 2 ).percent();
  }
  else if ( talents.incanters_ward -> ok() && cooldowns.incanters_ward -> up() )
  {
    m *= 1.0 + find_spell( 118858 ) -> effectN( 1 ).percent();
  }
  else if ( talents.incanters_ward -> ok() )
  {
    m *= 1.0 + buffs.incanters_ward_post -> value() * buffs.incanters_ward_post -> data().effectN( 1 ).percent();
  }

  double mana_pct = resources.pct( RESOURCE_MANA );
  m *= 1.0 + mana_pct * spec.mana_adept -> effectN( 1 ).mastery_value() * composite_mastery();

  return m;
}

// mage_t::composite_spell_crit =============================================

double mage_t::composite_spell_crit()
{
  double c = player_t::composite_spell_crit();

  // These also increase the water elementals crit chance

  if ( buffs.molten_armor -> up() )
  {
    c += buffs.molten_armor -> data().effectN( 1 ).percent();
  }

  return c;
}

// mage_t::composite_spell_haste ============================================

double mage_t::composite_spell_haste()
{
  double h = player_t::composite_spell_haste();

  if ( buffs.frost_armor -> up() )
  {
    h *= 1.0 / ( 1.0 + buffs.frost_armor -> data().effectN( 1 ).percent() );
  }

  if ( buffs.icy_veins -> up() && !glyphs.icy_veins -> ok() )
  {
    h *= 1.0 / ( 1.0 + buffs.icy_veins -> data().effectN( 1 ).percent() );
  }
  return h;
}

double mage_t::composite_spell_power_multiplier()
{
  double m = player_t::composite_spell_power_multiplier();

  return m;
}

// mage_t::matching_gear_multiplier =========================================

double mage_t::matching_gear_multiplier( attribute_e attr )
{
  if ( attr == ATTR_INTELLECT )
    return 0.05;

  return 0.0;
}

// mage_t::reset ============================================================

void mage_t::reset()
{
  player_t::reset();

  rotation.reset();
  if ( glyphs.mana_gem -> ok() )
  {
    mana_gem_charges = 10;
  }
  else
  {
    mana_gem_charges = 3;
  }
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
    benefits.water_elemental -> update( pets.water_elemental -> current.sleeping == 0 );
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
    if ( source != gains.evocation &&
         source != gains.mana_gem )
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

  if ( name_str == "mana_gem_charges" )
    return make_ref_expr( name_str, mana_gem_charges );

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

  return player_t::create_expression( a, name_str );
}

// mage_t::decode_set =======================================================

int mage_t::decode_set( item_t& item )
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

  if ( strstr( s, "gladiators_silk_"  ) ) return SET_PVP_CASTER;

  return SET_NONE;
}

// MAGE MODULE INTERFACE ================================================

struct mage_module_t : public module_t
{
  mage_module_t() : module_t( MAGE ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE )
  {
    return new mage_t( sim, name, r );
  }
  virtual bool valid() { return true; }
  virtual void init        ( sim_t* ) {}
  virtual void combat_begin( sim_t* ) {}
  virtual void combat_end  ( sim_t* ) {}
};

} // UNNAMED NAMESPACE

module_t* module_t::mage()
{
  static module_t* m = 0;
  if ( ! m ) m = new mage_module_t();
  return m;
}
