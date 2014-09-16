// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
// WoD To-do
// OH MY GOD CLEAN UP THE TODO SECTION - lots of love, collision.
// Do perks that effect crit (such as enhanced pyrotechnics) also effect the crit chance of multistrike?
// Extensive Test - At a glance, enhanced pyrotechnics works properly. Need to test more in depth though (remember CM interacts with this!)
// multistrike triggering ignite? - CONFIRMED BY CELESTALON TO INTERACT WITH EACHOTHER
// change the syntax around frostfirebolts implimentation of Enhanced pyrotechnics to match fireballs
// Need to add in Improved Scorch
// Shatter is changed Shatter: Now Frost only. Multiplies the critical strike chance of all your spells against frozen targets by 1.5 plus an additional 50%. needs to be coded.
// Improved Arcane Power needs to have a check for the perk to exist so it functions pre-90 correctly.
// Need to Add Improved Blink
// Evocation: need to test that it works, with a character template that does not have RoP.
// The IV glyph works by making IV gives 35% MS. The perk then improves this to 45% MS.
// All new spells need to have their damage cross-checked with in game values.
// Is the ignite from Inferno Blast spread?
// Automate which spells use Unstable Magic
// Do not hardcode 15second duration for enhanced frostbolt perk
// Arcane Orb needs to be treated as a flying object that can hit multiple targets in a line, instead of as something which is basically just an explosion around a single target.
// Need to do some basic d=vt calcs to have a more realistic travel time for AO.
// Improve the delay between tick and aoe for NT by applying a guassian distribution centered around 1.25s with stddev such that travel time is ~1.2-1.3s
// Removing hardcoding of Inferno Blast CD once it has returned to the spell data
// Water elemental waterbolt hitting ~30% too hard.

// Are Meteor ticks effected by haste? - Maybe? They are bugged on Beta as of 8/11/2014 (http://us.battle.net/wow/en/forum/topic/13780228135)

// To-do Completed:
//  Frost bomb is hitting too hard - DONE!
//  BUG IGNITE TRIGGERS ON MISSES. Fixing this breaks icicles. Need to investigate - DONE!
//  Enhanced Pyrotechnics is giving global crit chance increase (not just FB/FFB). Fix this! - DONE!
// Enhanced Frostbolt keeps trying to trigger for non-Frost specs (causes debug log to look ugly, prolly slows down the sim) - DONE!
//  Multistrike triggering ignite? - Confirmed by celestalon to interact with one another
//  Ice Lance MS should not be procing Frost Bomb explosion (as per in game testing 7/20/2014) - DONE!
//  Need to figure out how to not hard-code 2 charges and their CD for the "nova" spells. - This value is hardcoded in the spelldata it seems.
//  Frostbomb should be working correctly.
//  All "nova" talents have the 100% damage mod applied to the primary and AoE effects - it should only be on the primary target. - DONE!
//  Need to not hardcode Overpowered - DONE!
//  Add the delay between tick and aoe for NT - DONE!
//  Does the IV glyph still have a hidden proc-rate increase for FoF? - Assuming no, DONE!
//  Brainfreeze doesn't look correct in the spelldata (and isn't correct on the beta) in regards to proc chance. Fixed. -DONE!
//  Imp. Arcane Barrage needs to be tested. - DONE!
//  Fix how Ivy Veins interacts with spells. - DONE!
//  Multi-strikes proc UM. Add this! - DONE!
//  Improved Evocation is no longer hardcoded.
//  Can Meteor ticks crit and miss? - Miss no, Crit yes. - DONE!
//  Remove pyromaniac - DONE!
//  Imp. Arcane Explosion needs to be tested. -- DONE
//  Arcane Blast cast time reduction perk testing? - DONE! Working correctly. (I think. may be very minor problems due to it not effecting base_execute_time. But tools to do that easily are not inplace yet.)
//  Living Bomb spell data has completely changed - need to re-do the entire thing. - DONE! (I think?)
//  Comet Storm now have meteor split applied to its damage.
// Misc Notes:
//  Unstable Magic Trigger is very sensative to double dipping - as we encounter new modifiers, need to check there is double dipping going on!
//  Multistrike triggering ignite/Icicles? - Yes.



#include "simulationcraft.hpp"

using namespace residual_action;



namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Mage
// ==========================================================================

struct mage_t;
namespace actions {
struct ignite_t;
} // actions

namespace pets {
struct water_elemental_pet_t;
}

enum mage_rotation_e { ROTATION_NONE = 0, ROTATION_DPS, ROTATION_DPM, ROTATION_MAX };
// Icicle data, stored in an icicle container object. Contains a source stats object and the damage
typedef std::pair< double, stats_t* > icicle_data_t;
// Icicle container object, stored in a list to launch icicles at unsuspecting enemies!
typedef std::pair< timespan_t, icicle_data_t > icicle_tuple_t;

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
    buff_t* slow;
    buff_t* frost_bomb;
    buff_t* firestarter;
  } debuffs;

  mage_td_t( player_t* target, mage_t* mage );
};

struct mage_t : public player_t
{
public:
  // Current target
  player_t* current_target;

  // Icicles
  std::vector<icicle_tuple_t> icicles;
  action_t* icicle;
  core_event_t* icicle_event;

  // Active
  actions::ignite_t* active_ignite;
  int active_bomb_targets;
  action_t* explode; // Explode helps with handling Unstable Magic.
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
    buff_t* blazing_speed;
    buff_t* brain_freeze;
    buff_t* fingers_of_frost;
    buff_t* frost_armor;
    buff_t* heating_up;
    buff_t* ice_floes;
    buff_t* icy_veins;
    buff_t* improved_blink;
    stat_buff_t* mage_armor;
    buff_t* molten_armor;
    buff_t* presence_of_mind;
    buff_t* pyroblast;
    buff_t* rune_of_power;
    //buff_t* alter_time;
    buff_t* profound_magic;
    buff_t* potent_flames;
    buff_t* frozen_thoughts;
    buff_t* fiery_adept;
    buff_t* enhanced_pyrotechnics;
    buff_t* enhanced_frostbolt;
    buff_t* incanters_flow;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* evocation;
    cooldown_t* inferno_blast;
    cooldown_t* combustion;
    cooldown_t* bolt; // Cooldown to handle enhanced_frostbolt perk.
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* evocation;
  } gains;

  // Glyphs
  struct glyphs_t
  {
    // Major
    const spell_data_t* arcane_power;
    const spell_data_t* blink;
    const spell_data_t* combustion;
    const spell_data_t* cone_of_cold;
    const spell_data_t* frostfire;
    const spell_data_t* icy_veins;
    const spell_data_t* inferno_blast;
    const spell_data_t* living_bomb;
    const spell_data_t* rapid_displacement;
    const spell_data_t* splitting_ice;

    // Minor
    const spell_data_t* arcane_brilliance;
  } glyphs;


  // Passives
  struct passives_t
  {
    const spell_data_t* nether_attunement;
    const spell_data_t* shatter;
    const spell_data_t* frost_armor;
    const spell_data_t* mage_armor;
    const spell_data_t* molten_armor;
  } passives;

  // Perks
  struct perks_t
  {
      //Arcane
      const spell_data_t* enhanced_arcane_blast;
      const spell_data_t* improved_arcane_power;
      const spell_data_t* improved_evocation;
      const spell_data_t* improved_blink;

      //Fire
      const spell_data_t* enhanced_pyrotechnics;
      const spell_data_t* improved_flamestrike;
      const spell_data_t* improved_inferno_blast;
      const spell_data_t* improved_scorch;

      //Frost
      const spell_data_t* enhanced_frostbolt;
      const spell_data_t* improved_blizzard;
      const spell_data_t* improved_water_elemental;
      const spell_data_t* improved_icy_veins;

  } perks;

  // Pets
  struct pets_t
  {
    pets::water_elemental_pet_t* water_elemental;
    pet_t* mirror_images[ 3 ];
    pet_t* prismatic_crystal;
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
    const spell_data_t* presence_of_mind;
    const spell_data_t* slow;
    const spell_data_t* arcane_mind;

    // Fire
    const spell_data_t* critical_mass;
    const spell_data_t* ignite;
    const spell_data_t* incineration;

    // Frost
    const spell_data_t* frostbolt;
    const spell_data_t* brain_freeze;
    const spell_data_t* fingers_of_frost;
    const spell_data_t* icicles;
    const spell_data_t* ice_shards;

  } spec;

  // Talents
  struct talents_list_t
  {
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
    const spell_data_t* nether_tempest;
    const spell_data_t* living_bomb;
    const spell_data_t* frost_bomb;
    const spell_data_t* rune_of_power;
    const spell_data_t* supernova;
    const spell_data_t* blast_wave;
    const spell_data_t* ice_nova;
    const spell_data_t* kindling;
    const spell_data_t* overpowered;
    const spell_data_t* arcane_orb;
    const spell_data_t* meteor;
    const spell_data_t* unstable_magic;
    const spell_data_t* mirror_image;
    const spell_data_t* incanters_flow;
    const spell_data_t* prismatic_crystal;
    const spell_data_t* thermal_void;
    const spell_data_t* comet_storm;

  } talents;

  struct pyro_switch_t
  {
      bool dump_state;
      void reset() { dump_state = 0; }
      pyro_switch_t() { reset(); }
  } pyro_switch;



public:
  int current_arcane_charges;

  mage_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, MAGE, name, r ),
    current_target( target ),
    icicle( 0 ),
    icicle_event( 0 ),
    active_ignite( 0 ),
    active_bomb_targets( 0 ),
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
    pyro_switch( pyro_switch_t() ),
    current_arcane_charges()
  {

    //Active
    explode                  = 0;

    // Cooldowns
    cooldowns.evocation      = get_cooldown( "evocation"     );
    cooldowns.inferno_blast  = get_cooldown( "inferno_blast" );
    cooldowns.combustion     = get_cooldown( "combustion"    );
    cooldowns.bolt           = get_cooldown( "enhanced_frostbolt" );

    // Options
    base.distance = 40;
    regen_type = REGEN_DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_SPELL_HASTE ] = true;
  }

  // Character Definition
  virtual void      init_spells();
  virtual void      init_base_stats();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_benefits();
  virtual void      reset();
  virtual expr_t*   create_expression( action_t*, const std::string& name );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual void      create_pets();
  virtual resource_e primary_resource() const { return RESOURCE_MANA; }
  virtual role_e    primary_role() const { return ROLE_SPELL; }
  virtual stat_e    convert_hybrid_stat( stat_e s ) const;
  virtual double    mana_regen_per_second() const;
  virtual double    composite_player_multiplier( school_e school ) const;
  virtual void      invalidate_cache( cache_e );
  virtual double    composite_multistrike() const;
  virtual double    composite_spell_crit() const;
  virtual double    composite_spell_haste() const;
  virtual double    composite_rating_multiplier( rating_e rating ) const;
  virtual double    matching_gear_multiplier( attribute_e attr ) const;
  virtual void      stun();
  virtual void      moving();
  virtual double    temporary_movement_modifier() const;
  virtual void      arise();
  virtual action_t* select_action( const action_priority_list_t& );

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

  // Public mage functions:
  icicle_data_t get_icicle_object();
  void trigger_icicle( bool chain = false );

  void              apl_precombat();
  void              apl_arcane();
  void              apl_fire();
  void              apl_frost();
  void              apl_default();
  virtual void      init_action_list();

  std::string       get_potion_action();
};

namespace pets {

// ==========================================================================
// Pet Water Elemental
// ==========================================================================

struct water_elemental_pet_t;

struct water_elemental_pet_td_t: public actor_pair_t
{
  buff_t* water_jet;
public:
  water_elemental_pet_td_t( player_t* target, water_elemental_pet_t* welly );
};

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
        p -> o() -> buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), 1 );
    }
  };

  struct water_jet_tick_t : public spell_t
  {
    water_jet_tick_t( water_elemental_pet_t* p ) :
      spell_t( "water_jet_tick", p, p -> find_pet_spell( "water_jet" ) )
    {
      background = true;
      dot_duration = timespan_t::zero();
    }
  };
  struct water_jet_t : public spell_t
  {
    water_jet_t( water_elemental_pet_t* p, const std::string& options_str ) :
      spell_t( "water_jet", p, p -> find_pet_spell( "water_jet" ) )
    {
      parse_options( NULL, options_str );
      channeled = true;
      dynamic_tick_action = true;
      tick_action = new water_jet_tick_t( p );
    }

    water_elemental_pet_td_t* td( player_t* t = 0 ) const
    { return p() -> get_target_data( t ? t : target ); }

    water_elemental_pet_t* p()
    { return static_cast<water_elemental_pet_t*>( player ); }

    const water_elemental_pet_t* p() const
    { return static_cast<water_elemental_pet_t*>( player ); }

    virtual void impact( action_state_t* s )
    {
      spell_t::impact( s );

      td( s -> target ) -> water_jet -> trigger();
    }
  };

  water_elemental_pet_td_t* td( player_t* t ) const
  { return get_target_data( t ); }

  target_specific_t<water_elemental_pet_td_t*> target_data;

  virtual water_elemental_pet_td_t* get_target_data( player_t* target ) const
  {
    water_elemental_pet_td_t*& td = target_data[ target ];
    if ( ! td )
      td = new water_elemental_pet_td_t( target, const_cast<water_elemental_pet_t*>(this) );
    return td;
  }
  struct waterbolt_t: public spell_t
  {
    waterbolt_t( water_elemental_pet_t* p, const std::string& options_str ):
      spell_t( "waterbolt", p, p -> find_pet_spell( "Waterbolt" ) )
    {
      parse_options( NULL, options_str );
      may_crit = true;
    }
  };

  water_elemental_pet_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "water_elemental" )
  {
    action_list_str  = "waterbolt";

    owner_coeff.sp_from_sp = 0.75;
  }

  mage_t* o()
  { return static_cast<mage_t*>( owner ); }
  const mage_t* o() const
  { return static_cast<mage_t*>( owner ); }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "freeze"     ) return new     freeze_t( this, options_str );
    if ( name == "waterbolt"  ) return new  waterbolt_t( this, options_str );
    if ( name == "water_jet"  ) return new  water_jet_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }

  virtual double composite_player_multiplier( school_e school ) const
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( o() -> spec.icicles -> ok() )
      m *= 1.0 + o() -> cache.mastery_value();

    if ( o() -> buffs.rune_of_power -> check() )
    {
      m *= 1.0 + o() -> buffs.rune_of_power -> data().effectN( 3 ).percent();
    }
    if ( o() -> talents.incanters_flow -> ok() )
    {
      m *= 1.0 + ( o() -> buffs.incanters_flow -> current_stack ) * ( find_spell( 116267 ) -> effectN( 1 ).percent() );
    }
    // Orc racial
    if ( owner -> race == RACE_ORC )
      m *= 1.0 + find_spell( 21563 ) -> effectN( 1 ).percent();

    return m;
  }
};

water_elemental_pet_td_t::water_elemental_pet_td_t( player_t* target, water_elemental_pet_t* welly ) :
  actor_pair_t( target, welly )
{
  water_jet = buff_creator_t( *this, "water_jet", welly -> find_spell( 135029 ) ).cd( timespan_t::zero() );
}
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

      am *= 1.0 + p() -> arcane_charge -> stack() * p() -> o() -> spells.arcane_charge_arcane_blast -> effectN( 4 ).percent() *
            ( 1.0 + p() -> o() -> sets.set( SET_CASTER, T15, B4 ) -> effectN( 1 ).percent() );
        
      return am;
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
    owner_coeff.sp_from_sp = 1.00;
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

// ==========================================================================
// Pet Prismatic Crystal
// ==========================================================================

struct prismatic_crystal_t : public pet_t 
{
  struct prismatic_crystal_aoe_state_t : public action_state_t
  {
    action_t* owner_action;

    prismatic_crystal_aoe_state_t( action_t* action, player_t* target ) :
      action_state_t( action, target ), owner_action( 0 )
    { }

    void initialize()
    { action_state_t::initialize(); owner_action = 0; }

    std::ostringstream& debug_str( std::ostringstream& s )
    { action_state_t::debug_str( s ) << " owner_action=" << ( owner_action ? owner_action -> name_str : "unknown" ); return s; }

    void copy_state( const action_state_t* other )
    {
      action_state_t::copy_state( other );

      owner_action = debug_cast<const prismatic_crystal_aoe_state_t*>( other ) -> owner_action;
    }
  };

  struct prismatic_crystal_aoe_t : public spell_t
  {
    prismatic_crystal_aoe_t( prismatic_crystal_t* p ) :
      spell_t( "prismatic_crystal", p, p -> find_spell( 155152 ) )
    {
      school = SCHOOL_ARCANE;
      may_crit = may_miss = callbacks = false;
      background = true;
      aoe = -1;
      split_aoe_damage = true;
    }

    prismatic_crystal_t* p()
    { return debug_cast<prismatic_crystal_t*>( player ); }

    void execute()
    {
      // Note, the pre_execute_state is guaranteed to be set, as
      // prismatic_crystal_t::assess_damage() will always schedule the aoe
      // execute with a state
      const prismatic_crystal_aoe_state_t* st = debug_cast<const prismatic_crystal_aoe_state_t*>( pre_execute_state );

      assert( p() -> proxy_stats[ st -> owner_action -> internal_id ] != 0 );
      stats = p() -> proxy_stats[ st -> owner_action -> internal_id ];

      spell_t::execute();
    }

    // Damage gets fully inherited from the Mage's own spell
    double calculate_direct_amount( action_state_t* state )
    { return state -> result_amount; }

    action_state_t* new_state()
    { return new prismatic_crystal_aoe_state_t( this, target ); }
  };

  prismatic_crystal_aoe_t* aoe_spell;
  const spell_data_t* damage_taken;
  std::vector<stats_t*> proxy_stats;

  prismatic_crystal_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "prismatic_crystal", true ),
    aoe_spell( 0 ),
    damage_taken( owner -> find_spell( 155153 ) )
  { }

  void add_proxy_stats( action_t* owner_action )
  {
    if ( proxy_stats.size() <= owner_action -> internal_id )
      proxy_stats.resize( owner_action -> internal_id + 1, 0 );

    if ( proxy_stats[ owner_action -> internal_id ] )
      return;

    proxy_stats[ owner_action -> internal_id ] = get_stats( owner_action -> name_str );
    proxy_stats[ owner_action -> internal_id ] -> action_list.push_back( owner_action );
    proxy_stats[ owner_action -> internal_id ] -> school = owner_action -> school;
  }

  mage_t* o() const
  { return debug_cast<mage_t*>( owner ); }

  void init_spells()
  {
    pet_t::init_spells();

    aoe_spell = new prismatic_crystal_aoe_t( this );
  }

  void arise()
  {
    pet_t::arise();

    // For now, when Prismatic Crystal is summoned, adjust all mage targets to it.
    o() -> current_target = this;
    for ( size_t i = 0, end = o() -> action_list.size(); i < end; i++ )
      o() -> action_list[ i ] -> target_cache.is_valid = false;
  }

  void demise()
  {
    pet_t::demise();

    // For now, when Prismatic Crystal despawns, adjust all mage targets back to fluffy pillow.
    o() -> current_target = o() -> target;
    for ( size_t i = 0, end = o() -> action_list.size(); i < end; i++ )
      o() -> action_list[ i ] -> target_cache.is_valid = false;
  }

  double composite_player_vulnerability( school_e school ) const
  {
    double m = pet_t::composite_player_vulnerability( school );

    m *= 1.0 + damage_taken -> effectN( 1 ).percent();

    return m;
  }

  void assess_damage( school_e school, dmg_e type, action_state_t* state )
  {
    base_t::assess_damage( school, type, state );

    if ( state -> result_amount == 0 )
      return;

    // NOTE: We first use get_state() here to get a correct state object
    // (prismatic_crystal_aoe_state_t), and then EXPLICITLY USE the base class
    // copy_state to copy the owner's damage state to the PC damage state
    // object. After that, we assign the owner_action member variable to the PC
    // state, so we can do correct reporting.
    //
    // This is done so we can have a clean prismatic_crystal_aoe_state_t
    // implementation, where copy_state() behaves in a logical way, because the
    // rest of the execution path will only use prismatic_crystal_aoe_state_t
    // objects.
    action_state_t* new_state = aoe_spell -> get_state();
    new_state -> action_state_t::copy_state( state );
    prismatic_crystal_aoe_state_t* st = debug_cast<prismatic_crystal_aoe_state_t*>( new_state );
    st -> owner_action = state -> action;

    // Reset target to an enemy to avoid infinite looping
    new_state -> target = aoe_spell -> target;

    aoe_spell -> schedule_execute( new_state );
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
  std::vector<icicle_tuple_t> icicle_states;


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
      mage.icicles.push_back( icicle_states[ i ] );

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
};

// Icy Veins Buff ===========================================================

struct icy_veins_t : public buff_t
{
  icy_veins_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "icy_veins", p -> find_class_spell( "Icy Veins" ) ).add_invalidate( CACHE_SPELL_HASTE ) )
  {
    cooldown -> duration = timespan_t::zero(); // CD is managed by the spell
  }
};



} // end buffs namespace

namespace actions {
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
    may_crit      = true;
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

    if ( p() -> cooldowns.bolt -> up() && p() -> specialization() == MAGE_FROST )
      p() -> buffs.enhanced_frostbolt -> trigger();



    return spell_t::ready();
  }

  virtual double cost() const
  {
    double c = spell_t::cost();

    if ( p() -> buffs.arcane_power -> check() )
    {
      double m = 1.0 + p() -> buffs.arcane_power -> data().effectN( 2 ).percent() + p() -> perks.improved_arcane_power -> effectN( 1 ).percent();

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
    // If there is no state to schedule, make one and put the actor's current
    // target into it. This guarantees that:
    // 1) action_execute_event_t::execute() does not execute on dead targets, if the target died during cast
    // 2) We do not modify action's variables this early in the game
    //
    // If this is a tick action, there's going to be a state object passed to
    // it, that will have the correct target anyhow, since the parent will have
    // had it's target adjusted during its execution.
    if ( state == 0 )
    {
      state = get_state();

      // If cycle_targets is used, we need to target the spell to the (found)
      // target of the action, as it was selected during the ready() call.
      if ( cycle_targets == 1 )
        state -> target = target;
      // Put the actor's current target into the state object always.
      else
        state -> target = p() -> current_target;
    }

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
      m *= 1.5;
    return m;
  }

  void snapshot_internal( action_state_t* s, uint32_t flags, dmg_e rt )
  {
    spell_t::snapshot_internal( s, flags, rt );
    // Shatter's +50% crit bonus needs to be added after multipliers etc
    if ( ( flags & STATE_CRIT ) && frozen && p() -> passives.shatter -> ok() )
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

  void trigger_icicle_gain( action_state_t* state, stats_t* stats )
  {
    if ( ! p() -> spec.icicles -> ok() )
      return;

    if ( ! result_is_hit( state -> result ) )
      return;

    // Icicles do not double dip on target based multipliers
    double amount = state -> result_amount / state -> target_da_multiplier * p() -> cache.mastery_value();

    assert( as<int>( p() -> icicles.size() ) <= p() -> spec.icicles -> effectN( 2 ).base_value() );
    // Shoot one
    if ( as<int>( p() -> icicles.size() ) == p() -> spec.icicles -> effectN( 2 ).base_value() )
      p() -> trigger_icicle();
    p() -> icicles.push_back( icicle_tuple_t( p() -> sim -> current_time, icicle_data_t( amount, stats ) ) );

    if ( p() -> sim -> debug )
      p() -> sim -> out_debug.printf( "%s icicle gain, damage=%f, total=%u",
                                      p() -> name(),
                                      amount,
                                      as<unsigned>(p() -> icicles.size() ) );
  }

  virtual void execute()
  {
    p() -> benefits.dps_rotation -> update( p() -> rotation.current == ROTATION_DPS );
    p() -> benefits.dpm_rotation -> update( p() -> rotation.current == ROTATION_DPM );

    player_t* original_target = 0;
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
        pre_execute_state = 0;
      }
    }

    // Regenerate just before executing the spell, so Arcane mages have a 100%
    // correct mana level to snapshot their multiplier with
    if ( p() -> regen_type == REGEN_DYNAMIC )
      p() -> do_dynamic_regen();

    spell_t::execute();

    // Restore original target if necessary
    if ( original_target )
      target = original_target;

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

  size_t available_targets( std::vector< player_t* >& tl ) const
  {
    tl.clear();
    tl.push_back( target );

    for ( size_t i = 0, actors = sim -> target_non_sleeping_list.size(); i < actors; i++ )
    {
      player_t* t = sim -> target_non_sleeping_list[ i ];

      if ( t -> is_enemy() && ( t != target ) )
        tl.push_back( t );
    }

    if ( ! p() -> pets.prismatic_crystal -> is_sleeping() )
      tl.push_back( p() -> pets.prismatic_crystal );

    return tl.size();
  }

  void record_data( action_state_t* data )
  {
    if ( data -> target == p() -> pets.prismatic_crystal )
      return;

    spell_t::record_data( data );
  }

  void init()
  {
    spell_t::init();

    pets::prismatic_crystal_t* pc = debug_cast<pets::prismatic_crystal_t*>( p() -> pets.prismatic_crystal );
    pc -> add_proxy_stats( this );
  }
};

typedef residual_action::residual_periodic_action_t< mage_spell_t > residual_action_t;

// Icicles ==================================================================

struct icicle_state_t : public action_state_t
{
  stats_t* source;

  icicle_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ), source( 0 )
  { }

  void initialize()
  { action_state_t::initialize(); source = 0; }

  std::ostringstream& debug_str( std::ostringstream& s )
  { action_state_t::debug_str( s ) << " source=" << ( source ? source -> name_str : "unknown" ); return s; }

  void copy_state( const action_state_t* other )
  {
    action_state_t::copy_state( other );

    source = debug_cast<const icicle_state_t*>( other ) -> source;
  }
};

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

  void execute()
  {
    assert( pre_execute_state );

    const icicle_state_t* is = debug_cast<const icicle_state_t*>( pre_execute_state );
    stats = is -> source;

    mage_spell_t::execute();
  }

  action_state_t* new_state()
  { return new icicle_state_t( this, target ); }

  void init()
  {
    mage_spell_t::init();

    snapshot_flags &= ~( STATE_MUL_DA | STATE_SP | STATE_CRIT | STATE_TGT_CRIT );
  }
};

// Ignite ===================================================================

struct ignite_t : public residual_action_t
{
  ignite_t( mage_t* player ) :
    residual_action_t( "ignite", player, player -> find_spell( 12846 ) )
  {
    dot_duration = player -> dbc.spell( 12654 ) -> duration();
    base_tick_time = player -> dbc.spell( 12654 ) -> effectN( 1 ).period();
    school = SCHOOL_FIRE;
  }
};

void mage_spell_t::trigger_ignite( action_state_t* state )
{
  mage_t& p = *this -> p();
  if ( ! p.active_ignite ) return;
  double amount = state -> result_amount * p.cache.mastery_value();
  trigger( p.active_ignite, state -> target, amount );
}

// Unstable Magic Trigger ====================================================

static void trigger_unstable_magic( action_state_t* s )
{
  struct unstable_magic_explosion_t : public mage_spell_t
  {
    double pct_damage;
    unstable_magic_explosion_t( mage_t* p ) :
      mage_spell_t( "unstable_magic_explosion", p, p -> talents.unstable_magic )
    {
      may_miss = may_dodge = may_parry = may_crit = may_block = callbacks = false;
      may_multistrike = 0;
      aoe = -1;
      base_costs[ RESOURCE_MANA ] = 0;
      cooldown -> duration  = timespan_t::zero();
      pct_damage = p -> talents.unstable_magic -> effectN( 2 ).percent();
      trigger_gcd = timespan_t::zero();
    }

  virtual double composite_player_multipler() const
  { return 1; }

  virtual double composite_target_multipler() const
  { return 1; }

  virtual double composite_versaility() const
  { return 1; }

  virtual void init()
  {
    mage_spell_t::init();
    // disable the snapshot_flags for all multipliers
    snapshot_flags &= STATE_NO_MULTIPLIER;
  }

  virtual void execute()
  {
    base_dd_max *= pct_damage; // Deals 50% of original triggering spell damage
    base_dd_min *= pct_damage;

    mage_spell_t::execute();
  }
  };

  mage_t* p = debug_cast<mage_t*>( s -> action -> player );

  if ( !p -> explode )
  {
   p -> explode = new unstable_magic_explosion_t( p );
   p -> explode -> init();
  }

  p -> explode -> base_dd_max = s -> result_amount;
  p -> explode -> base_dd_min = s -> result_amount;
  p -> explode -> execute();
  
  return;
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
      ( 1.0 + p() -> sets.set( SET_CASTER, T15, B4 ) -> effectN( 1 ).percent() );

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

  }

  virtual double cost() const
  {
    double c = mage_spell_t::cost();

    if ( p() -> buffs.arcane_charge -> check() )
    {
      c *= 1.0 +  p() -> buffs.arcane_charge -> check() * p() -> spells.arcane_charge_arcane_blast -> effectN( 2 ).percent() *
        ( 1.0 + p() -> sets.set( SET_CASTER, T15, B4 ) -> effectN( 1 ).percent() );
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
  }

  virtual double action_multiplier() const
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.arcane_charge -> stack() * p() -> spells.arcane_charge_arcane_blast -> effectN( 1 ).percent() *
      ( 1.0 + p() -> sets.set( SET_CASTER, T15, B4 ) -> effectN( 1 ).percent() );

    return am;
  }

  virtual timespan_t execute_time() const
  {
      if ( p() -> buffs.arcane_charge -> check() )
      {
          timespan_t t = mage_spell_t::execute_time();
          t *= ( 1.0 - ( p() -> buffs.arcane_charge -> stack() * p() -> perks.enhanced_arcane_blast -> effectN( 1 ).percent() ) );
          return t;
      }
      else
          return mage_spell_t::execute_time();
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) || result_is_multistrike( s -> result ) )
    {
        if ( p() -> talents.unstable_magic -> ok() && rng().roll( p() -> talents.unstable_magic -> effectN( 2 ).percent() ) )
          trigger_unstable_magic( s );
    }
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

// Arcane Explosion Spell ===========================================================

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
    dot_duration = timespan_t::zero();
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
    dot_duration      = timespan_t::from_seconds( 2.0 );
    channeled         = true;
    hasted_ticks      = false;

    dynamic_tick_action = true;
    tick_action = new arcane_missiles_tick_t( p );
  }

  virtual double action_multiplier() const
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.arcane_charge -> stack() * p() -> spells.arcane_charge_arcane_blast -> effectN( 1 ).percent() *
      ( 1.0 + p() -> sets.set( SET_CASTER, T15, B4 ) -> effectN( 1 ).percent() );
    if ( p() -> sets.has_set_bonus( SET_CASTER, T14, B2 ) )
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


    mage_spell_t::execute();


    if ( p() -> buffs.arcane_power -> up() && p() -> talents.overpowered -> ok() )
      p() -> buffs.arcane_power -> extend_duration( p(), timespan_t::from_seconds( p() -> talents.overpowered -> effectN( 1 ).base_value() ) );

    p() -> buffs.arcane_missiles -> up();

    if ( p() -> sets.has_set_bonus( SET_CASTER, T16, B2 ) )
    {
      p() -> buffs.profound_magic -> trigger();
    }

    // T16 4pc has a chance not to consume arcane missiles buff
    if ( !p() -> sets.has_set_bonus( SET_CASTER, T16, B4 ) || !rng().roll( p() -> sets.set( SET_CASTER, T16, B4 ) -> effectN( 1 ).percent() ) )
    {
      p() -> buffs.arcane_missiles -> decrement();
    }
  }

  virtual void last_tick (dot_t * d)
  {
    mage_spell_t::last_tick( d );
    p() -> buffs.arcane_charge -> trigger();
  }

  virtual bool ready()
  {
    if ( ! p() -> buffs.arcane_missiles -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Arcane Orb Spell =========================================================
struct arcane_orb_bolt_t : public mage_spell_t
{
  arcane_orb_bolt_t( mage_t* p ) :
    mage_spell_t( "arcane_orb_bolt", p, p -> find_spell( 153640 ) )
  {
    aoe = -1;
    background = true;
    dual = true;
    cooldown -> duration = timespan_t::zero(); // dbc has CD of 15 seconds
  }

  virtual void impact( action_state_t* s )
  {
    for ( unsigned i = 0; i < sizeof_array( p() -> benefits.arcane_charge ); i++)
    {
      p() -> benefits.arcane_charge[ i ] -> update( as<int>( i ) == p() -> buffs.arcane_charge -> check() );
    }
    
    mage_spell_t::impact( s );
    p() -> buffs.arcane_charge -> trigger();
  }
};

struct arcane_orb_t : public mage_spell_t
{
  arcane_orb_bolt_t* orb_bolt;

  arcane_orb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_orb", p, p -> find_talent_spell( "Arcane Orb" ) ),
    orb_bolt( new arcane_orb_bolt_t( p ) )
  {
    parse_options( NULL, options_str );

    may_miss       = false;
    may_crit       = false;
    add_child( orb_bolt );
  }

  virtual void execute()
  {
    for ( unsigned i = 0; i < sizeof_array( p() -> benefits.arcane_charge ); i++)
    {
      p() -> benefits.arcane_charge[ i ] -> update( as<int>( i ) == p() -> buffs.arcane_charge -> check() );
    }
    mage_spell_t::execute();

    p() -> buffs.arcane_charge -> trigger();
  }


  virtual timespan_t travel_time() const
  {
    timespan_t t = mage_spell_t::travel_time();
    double distance = mage_spell_t::player -> current.distance;
    t  = timespan_t::from_seconds( ( distance - 10 ) / 16 );
    return t;
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    orb_bolt -> execute();
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

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> buffs.arcane_power -> trigger( 1, data().effectN( 1 ).percent() );
  }
};

// Blast Wave Spell ==========================================================

struct blast_wave_t : public mage_spell_t
{
    blast_wave_t( mage_t* p, const std::string& options_str ) :
       mage_spell_t( "blast_wave", p, p -> talents.blast_wave )
    {
        parse_options( NULL, options_str );
        base_multiplier *= 1.0 + p -> talents.blast_wave -> effectN( 1 ).percent();
        base_aoe_multiplier *= 0.5;
        aoe = -1;
    }

    virtual void init()
    {
        mage_spell_t::init();

        // NOTE: Cooldown missing from tooltip since WoD beta build 18379
        cooldown -> duration = timespan_t::from_seconds( 25.0 );
        cooldown -> charges = 2;
    }
};

// Blazing Speed Spell ============================================================

struct blazing_speed_t: public mage_spell_t
{
  blazing_speed_t( mage_t* p, const std::string& options_str ):
    mage_spell_t( "blazing_speed", p, p -> talents.blazing_speed )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    p() -> buffs.blazing_speed -> trigger();
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
    base_teleport_distance += p -> glyphs.blink -> effectN( 1 ).base_value();
    if ( p -> glyphs.rapid_displacement -> ok() )
      cooldown -> charges = p -> glyphs.rapid_displacement -> effectN( 1 ).base_value();

    movement_directionality = MOVEMENT_OMNI;
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    if ( !p() -> glyphs.rapid_displacement -> ok() )
      player -> buffs.stunned -> expire();

    p() -> buffs.improved_blink -> trigger();
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

      // TODO: Verify that hidden FoF proc increase from glyph of IV is removed
      /*if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
      {
        fof_proc_chance *= 1.2;
      }*/
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
    dot_duration      = tick_spell.duration();
    tick_may_crit  = true;

    if ( p -> sets.has_set_bonus( SET_CASTER, T14, B4 ) )
    {
      cooldown -> duration = data().cooldown() * 0.8;
    }

    if ( p -> glyphs.combustion -> ok() )
    {
      dot_duration *= ( 1.0 + p -> glyphs.combustion -> effectN( 1 ).percent() );
      cooldown -> duration *= 1.0 + p -> glyphs.combustion -> effectN( 2 ).percent();
      base_dd_multiplier *= 1.0 + p -> glyphs.combustion -> effectN( 3 ).percent();
    }
  }

  action_state_t* new_state()
  { return new residual_periodic_state_t( this, target ); }

  virtual double calculate_tick_amount( action_state_t* state,
                                        double dmg_multiplier )
  {
    double amount = 0.0;

    if ( dot_t* d = find_dot( state -> target ) )
    {

      const residual_periodic_state_t* dps_t =
        debug_cast<const residual_periodic_state_t*>( d -> state );
      amount += dps_t -> tick_amount;
    }

    state -> result_raw = amount;

    if ( state -> result == RESULT_CRIT )
      amount *= 1.0 + total_crit_bonus();
    else if ( state -> result == RESULT_MULTISTRIKE )
      amount *= composite_multistrike_multiplier( state );
    else if ( state -> result == RESULT_MULTISTRIKE_CRIT )
      amount *= composite_multistrike_multiplier( state ) *
                ( 1.0 + total_crit_bonus() );


    amount *= dmg_multiplier;

    state -> result_total = amount;

    return amount;
  }

  virtual void trigger_dot( action_state_t* s )
  {
    mage_td_t* this_td = td( s -> target );

    dot_t* ignite_dot     = this_td -> dots.ignite;
    dot_t* combustion_dot = this_td -> dots.combustion;

    if ( ignite_dot -> is_ticking() )
    {
      mage_spell_t::trigger_dot( s );

      residual_periodic_state_t* combustion_dot_state_t = debug_cast<residual_periodic_state_t*>( combustion_dot -> state );
      const residual_periodic_state_t* ignite_dot_state_t = debug_cast<const residual_periodic_state_t*>( ignite_dot -> state );
      combustion_dot_state_t -> tick_amount = ignite_dot_state_t -> tick_amount * 0.4; // Changed in 40% in WoD beta.
    }
  }

  virtual void execute()
  {
    p() -> cooldowns.inferno_blast -> reset( false );

    mage_spell_t::execute();
  }

};

// Comet Storm Spell =======================================================

struct comet_storm_projectile_t : public mage_spell_t
{
  comet_storm_projectile_t( mage_t* p) :
    mage_spell_t( "comet_storm_projectile", p, p -> find_spell( 153596 ) )
  {
    aoe = -1;
    split_aoe_damage = true;
    background = true;
  }

  virtual timespan_t travel_time() const
  {
    timespan_t t = mage_spell_t::travel_time();
    t = timespan_t::from_seconds( 1.0 );
    return t;
  }

};

struct comet_storm_t : public mage_spell_t
{
  comet_storm_projectile_t* projectile;

  comet_storm_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "comet_storm", p, p -> talents.comet_storm ),
    projectile( new comet_storm_projectile_t( p ) )
  {

    parse_options( NULL, options_str );

    may_miss = false;

    base_tick_time    = timespan_t::from_seconds( 0.2 );
    dot_duration      = timespan_t::from_seconds( 1.2 );
    hasted_ticks      = false;

    dynamic_tick_action = true;
    add_child( projectile );
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    projectile -> execute();
  }
  void tick( dot_t* d )
  {
    mage_spell_t::tick( d );
    projectile -> execute();
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

    if ( p() -> glyphs.cone_of_cold -> ok() )
    {
      am *=  1.0 + p() -> glyphs.cone_of_cold -> effectN( 1 ).percent();
    }

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


  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return mage_spell_t::ready();
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
    mage_spell_t( "evocation", p,  p -> find_class_spell( "Evocation" ) ),
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
    cooldown -> duration = data().cooldown() + timespan_t::from_millis( p -> perks.improved_evocation -> effectN( 1 ).base_value() );

    timespan_t duration = data().duration();


    dot_duration = duration;
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
              ( 1.0 + p() -> sets.set( SET_CASTER, T15, B4) -> effectN( 1 ).percent() );
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
  }

  virtual void execute()
  {
    mage_t& p = *this -> p();

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


  }


  virtual timespan_t travel_time() const
  {
    timespan_t t = mage_spell_t::travel_time();
    return ( t > timespan_t::from_seconds( 0.75 ) ? timespan_t::from_seconds( 0.75 ) : t );
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
        if ( s -> result == RESULT_CRIT )
            p() -> buffs.enhanced_pyrotechnics -> expire();

        else
            p() -> buffs.enhanced_pyrotechnics -> trigger();

    }

    if ( p() -> talents.kindling -> ok() &&  s -> result == RESULT_CRIT )
        p() -> cooldowns.combustion -> adjust( timespan_t::from_seconds( - p() -> talents.kindling -> effectN( 1 ).base_value() ) );

    if ( result_is_hit( s -> result) || result_is_multistrike( s -> result) )
    {
        if ( p() -> talents.unstable_magic -> ok() && rng().roll( p() -> talents.unstable_magic -> effectN( 3 ).percent() ) )
          trigger_unstable_magic( s );
        trigger_ignite( s );
    }

  }

  double composite_target_crit( player_t* target ) const
  {
    double c = mage_spell_t::composite_target_crit( target );

    // Fire PvP 4pc set bonus
    if ( td( target ) -> debuffs.firestarter -> check() )
      c += p() -> find_spell( 171170 ) -> effectN( 1 ).percent();
    return c;
  }

  virtual double composite_crit() const
  {
      double c = mage_spell_t::composite_crit();

      c += ( p() -> buffs.enhanced_pyrotechnics -> check() ) * p() -> perks.enhanced_pyrotechnics -> effectN( 1 ).trigger() -> effectN( 1 ).percent();

      return c;
  }

  double composite_crit_multiplier() const
  {
    double m = mage_spell_t::composite_crit_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }
};

// Flamestrike Spell ========================================================

struct flamestrike_t : public mage_spell_t
{
  flamestrike_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "flamestrike", p, p -> find_specialization_spell( "Flamestrike" ) )
  {
    parse_options( NULL, options_str );
    cooldown -> duration = timespan_t::zero(); // Flamestrike Perk modifying the cooldown
    aoe = -1;
  }
};

// Frost Bomb Spell ===============================================================

struct frost_bomb_explosion_t : public mage_spell_t
{
  frost_bomb_explosion_t( mage_t* p ) :
    mage_spell_t( "frost_bomb_explosion", p, p -> find_spell( 113092 ) )
  {
    aoe = -1;
    parse_effect_data( data().effectN( 1 ) );
    base_aoe_multiplier *= ( p -> find_spell( 113092 ) -> effectN( 2 ).sp_coeff() / p -> find_spell( 113092 ) -> effectN( 1 ).sp_coeff() );
    background = true;
  }

  virtual resource_e current_resource() const
  { return RESOURCE_NONE; }

  virtual timespan_t travel_time() const
  { return timespan_t::zero(); }
};

struct frost_bomb_t : public mage_spell_t
{
  frost_bomb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frost_bomb", p, p -> talents.frost_bomb )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    mage_t& p = *this -> p();

    mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if (p.last_bomb_target != execute_state -> target && p.last_bomb_target != 0)
      {
        td(p.last_bomb_target) -> dots.frost_bomb -> cancel();
        td(p.last_bomb_target) -> debuffs.frost_bomb -> expire();
      }
      p.last_bomb_target = execute_state -> target;
    }
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    td( s -> target ) -> debuffs.frost_bomb -> trigger();
  }
};

// Frostbolt Spell ==========================================================


struct frostbolt_t : public mage_spell_t
{
  double bf_proc_chance;
  // Icicle stats variable to parent icicle damage to Frostbolt, instead of
  // clumping FB/FFB icicle damage together in reports.
  stats_t* icicle;

  frostbolt_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frostbolt", p, p -> find_specialization_spell( "Frostbolt" ) ),
    bf_proc_chance( 0.1 ),
    icicle( p -> get_stats( "icicle_fb" ) )
  {
    parse_options( NULL, options_str );

    stats -> add_child( icicle );
    icicle -> action_list.push_back( p -> icicle );
  }

  virtual int schedule_multistrike( action_state_t* s, dmg_e dmg_type, double tick_multiplier )
  {
    int sm = mage_spell_t::schedule_multistrike( s, dmg_type, tick_multiplier );

    if ( sm == 1 )
      bf_proc_chance += p() -> spec.brain_freeze -> effectN( 2 ).percent();
    if ( sm == 2 )
      bf_proc_chance +=  ( p() -> spec.brain_freeze -> effectN( 2 ).percent() * 2 );
    return sm;
  }

  virtual timespan_t execute_time() const
  {
    timespan_t cast = mage_spell_t::execute_time();

    if ( p() -> buffs.enhanced_frostbolt -> check() )
      cast *= 1 + p() -> perks.enhanced_frostbolt -> effectN(1).time_value().total_seconds() /
                  base_execute_time.total_seconds();

    return cast;
  }


  virtual void execute()
  {

    mage_spell_t::execute();

    if ( p() -> buffs.enhanced_frostbolt -> up() )
    {
      p() -> cooldowns.bolt -> duration = timespan_t::from_seconds( 15.0 );
      p() -> cooldowns.bolt -> start();
      p() -> buffs.enhanced_frostbolt -> expire();
    }

    if ( result_is_hit( execute_state -> result ) )
    {
      double fof_proc_chance = p() -> buffs.fingers_of_frost -> data().effectN( 1 ).percent();

      fof_proc_chance += p() -> sets.set( SET_CASTER, T15, B4 ) -> effectN( 3 ).percent();

      // TODO: Verify that hidden FoF proc increase from glyph of IV is removed
      /*if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
      {
        fof_proc_chance *= 1.2;
      }*/

      p() -> buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), fof_proc_chance );
      p() -> buffs.brain_freeze -> trigger(1, buff_t::DEFAULT_VALUE(), bf_proc_chance );

    }

    p() -> buffs.frozen_thoughts -> expire();
    bf_proc_chance = 0.1;
  }

  virtual void impact( action_state_t* s )
  {
      mage_spell_t::impact( s );
      if ( result_is_hit( s -> result ) || result_is_multistrike( s -> result) )
      {
        if ( p() -> talents.unstable_magic -> ok() && rng().roll( p() -> talents.unstable_magic -> effectN( 3 ).percent() ) )
          trigger_unstable_magic( s );
        trigger_icicle_gain( s, icicle );
      }

    if ( ! p() -> pets.water_elemental -> is_sleeping() )
    {
      pets::water_elemental_pet_td_t* we_td = p() -> pets.water_elemental -> get_target_data( execute_state -> target );
      if ( we_td -> water_jet -> up() )
      {
        p() -> buffs.fingers_of_frost -> trigger();
      }
    }

  }

  virtual timespan_t travel_time() const
  {
    timespan_t t = mage_spell_t::travel_time();
    return ( t > timespan_t::from_seconds( 0.75 ) ? timespan_t::from_seconds( 0.75 ) : t );
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

// Frostfire Bolt Spell =====================================================

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
  frigid_blast_t* frigid_blast;
  // Icicle stats variable to parent icicle damage to Frostfire Bolt, instead of
  // clumping FB/FFB icicle damage together in reports.
  stats_t* icicle;

  frostfire_bolt_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frostfire_bolt", p, p -> find_spell( 44614 ) ),
    frigid_blast( new frigid_blast_t( p ) ),
    icicle( p -> get_stats( "icicle_ffb" ) )
  {
    parse_options( NULL, options_str );
    may_hot_streak = true;
    base_execute_time += p -> glyphs.frostfire -> effectN( 1 ).time_value();


    if ( p -> sets.has_set_bonus( SET_CASTER, T16, B2 ) )
    {
      add_child( frigid_blast );
    }

    stats -> add_child( icicle );
    icicle -> action_list.push_back( p -> icicle );
  }

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
    mage_spell_t::execute();


    if ( result_is_hit( execute_state -> result ) )
    {
      double fof_proc_chance = p() -> buffs.fingers_of_frost -> data().effectN( 1 ).percent();
      // TODO: Verify that hidden FoF proc increase from glyph of IV is removed
      /*if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
      {
        fof_proc_chance *= 1.2;
      }*/
      p() -> buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), fof_proc_chance );
    }
    p() -> buffs.frozen_thoughts -> expire();
    if ( p() -> buffs.brain_freeze -> check() && p() -> sets.has_set_bonus( SET_CASTER, T16, B2 ) )
    {
      p() -> buffs.frozen_thoughts -> trigger();
    }
    if ( rng().roll( p() -> sets.set( SET_CASTER, T16, B4 ) -> effectN( 2 ).percent() ) )
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
    mage_spell_t::impact( s );
    // If there are five Icicles, launch the oldest at this spell's target
    // Create an Icicle, stashing damage equal to mastery * value
    // Damage should be based on damage spell would have done without any
    // target-based damage increases or decreases, except Frostbolt debuff
    // Should also apply to mini version

    if ( s -> result == RESULT_CRIT && p() -> specialization() == MAGE_FIRE )
        p() -> buffs.enhanced_pyrotechnics -> expire();
    else if ( s -> result == RESULT_HIT && p() -> specialization() == MAGE_FIRE )
        p() -> buffs.enhanced_pyrotechnics -> trigger();

    //Unstable Magic Trigger
    if ( result_is_hit( s -> result ) || result_is_multistrike( s -> result ) )
    {
      if ( p() -> talents.unstable_magic -> ok() && rng().roll( p() -> talents.unstable_magic -> effectN( 2 ).percent() ) )
        trigger_unstable_magic( s );
    }

    if ( ( result_is_hit( s-> result) || result_is_multistrike( s -> result ) ) && p() -> specialization() == MAGE_FIRE )
      trigger_ignite( s );


    if ( ( result_is_hit( s-> result) || result_is_multistrike( s -> result ) ) && p() -> specialization() == MAGE_FROST )
        trigger_icicle_gain( s, icicle );

    if ( p() -> talents.kindling -> ok() && s -> result == RESULT_CRIT )
      p() -> cooldowns.combustion -> adjust( timespan_t::from_seconds( - p() -> talents.kindling -> effectN( 1 ).base_value() ) );
  }

  virtual double composite_crit() const
  {
      double c = mage_spell_t::composite_crit();

      if ( p() -> specialization() == MAGE_FIRE )  c += ( p() -> buffs.enhanced_pyrotechnics -> check() ) * p() -> perks.enhanced_pyrotechnics -> effectN( 1 ).trigger() -> effectN( 1 ).percent();

      return c;
  }

  double composite_target_crit( player_t* target ) const
  {
    double c = mage_spell_t::composite_target_crit( target );

    // 4pc Fire PvP bonus
    if ( td( target ) -> debuffs.firestarter -> check() )
      c += p() -> find_spell( 171170 ) -> effectN( 1 ).percent();

    return c;
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

    // 2T16
    if ( p() -> buffs.frozen_thoughts -> up() )
    {
      am *= ( 1.0 + p() -> buffs.frozen_thoughts -> data().effectN( 1 ).percent() );
    }

    am *= 1.0 + p() -> find_spell( 44549 ) -> effectN( 3 ).percent();

    return am;
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

    // TODO: Verify that hidden FoF proc increase from glyph of IV is removed
    /*if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
    {
      fof_proc_chance *= 1.2;
    }*/
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
    dot_duration      = data().duration();
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

// Ice Lance Spell ==========================================================

struct ice_lance_t : public mage_spell_t
{

  double fof_multiplier;
  frost_bomb_explosion_t* frost_bomb_explode;

  ice_lance_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "ice_lance", p, p -> find_class_spell( "Ice Lance" ) ),
    fof_multiplier( 0 ),
    frost_bomb_explode( new frost_bomb_explosion_t( p ) )
  {
    parse_options( NULL, options_str );

    if ( p -> glyphs.splitting_ice -> ok() )
      aoe = p -> glyphs.splitting_ice -> effectN( 1 ).base_value() + 1;

    if ( p -> glyphs.splitting_ice -> ok() )
      base_aoe_multiplier *= p -> glyphs.splitting_ice -> effectN( 2 ).percent();

    fof_multiplier = p -> find_specialization_spell( "Fingers of Frost" ) -> ok() ? p -> find_spell( 44544 ) -> effectN( 2 ).percent() : 0.0;
  }

  virtual void execute()
  {
    // Ice Lance treats the target as frozen with FoF up
    frozen = p() -> buffs.fingers_of_frost -> check() > 0;

    mage_spell_t::execute();

    if ( p() -> talents.thermal_void -> ok() && p() -> buffs.icy_veins -> up() )
      p() -> buffs.icy_veins -> extend_duration( p(), timespan_t::from_seconds( p() -> talents.thermal_void -> effectN( 1 ).base_value() ) );



    // Begin casting all Icicles at the target, beginning 0.25 seconds after the
    // Ice Lance cast with remaining Icicles launching at intervals of 0.75
    // seconds, both values adjusted by haste. Casting continues until all
    // Icicles are gone, including new ones that accumulate while they're being
    // fired. If target dies, Icicles stop. If Ice Lance is cast again, the
    // current sequence is interrupted and a new one begins.

    p() -> buffs.fingers_of_frost -> decrement();
    p() -> buffs.frozen_thoughts -> expire();
    p() -> trigger_icicle( true );
  }

  virtual void impact( action_state_t* s )
  {

    mage_spell_t::impact( s );

    if ( p() -> talents.frost_bomb -> ok() )
    {
      if ( td( s -> target ) -> debuffs.frost_bomb -> up() && frozen && !result_is_multistrike( s -> result) )
        frost_bomb_explode -> execute();
    }
  }

  virtual double action_multiplier() const
  {
    double am = mage_spell_t::action_multiplier();

    if ( p() -> buffs.fingers_of_frost -> up() )
    {
      am *= 2.0; // Built in bonus against frozen targets
      am *= 1.0 + fof_multiplier; // Buff from Fingers of Frost
    }

    if ( p() -> sets.has_set_bonus( SET_CASTER, T14, B2 ) )
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

// Ice Nova Spell ==========================================================

struct ice_nova_t : public mage_spell_t
{
    ice_nova_t( mage_t* p, const std::string& options_str ) :
       mage_spell_t( "ice_nova", p, p -> talents.ice_nova )
    {
        parse_options( NULL, options_str );
        base_multiplier *= 1.0 + p -> talents.ice_nova -> effectN( 1 ).percent();
        base_aoe_multiplier *= 0.5;
        aoe = -1;
    }

    virtual void init()
    {
        mage_spell_t::init();

        // NOTE: Cooldown missing from tooltip since WoD beta build 18379
        cooldown -> duration = timespan_t::from_seconds( 25.0 );
        cooldown -> charges = 2;
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

    if ( player -> sets.has_set_bonus( SET_CASTER, T14, B4 ) )
    {
      cooldown -> duration *= 0.5;
    }
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
    mage_spell_t( "inferno_blast", p,
                  p -> find_class_spell( "Inferno Blast" ) )
  {
    parse_options( NULL, options_str );

    may_hot_streak = true;
    cooldown -> duration = timespan_t::from_seconds( 8.0 );

    max_spread_targets = 3;
    max_spread_targets += p -> glyphs.inferno_blast -> ok() ? p -> glyphs.inferno_blast -> effectN( 1 ).base_value() : 0;
    max_spread_targets += p -> perks.improved_inferno_blast ? p -> perks.improved_inferno_blast -> effectN( 1 ).base_value() : 0;
  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      mage_td_t* this_td = td( s -> target );

      dot_t* combustion_dot  = this_td -> dots.combustion,
           * ignite_dot      = this_td -> dots.ignite,
           * living_bomb_dot = this_td -> dots.living_bomb,
           * pyroblast_dot   = this_td -> dots.pyroblast;

      int spread_remaining = max_spread_targets;
      std::vector< player_t* >& tl = target_list();
      // Randomly choose spread targets
      std::random_shuffle( tl.begin(), tl.end() );

      for ( size_t i = 0, actors = tl.size(); i < actors; i++ )
      {
        player_t* t = tl[ i ];

        if ( t == s -> target )
          continue;

        if ( combustion_dot -> is_ticking() )
        {
          // Combustion does not spread to targets already afflicted
          // Source : http://goo.gl/tCsaqr
          dot_t* target_combustion = t -> get_dot( "combustion", this -> p() );
          if ( !target_combustion -> is_ticking() )
            combustion_dot -> copy( t, DOT_COPY_CLONE );
        }

        if ( ignite_dot -> is_ticking() )
        {
          if ( td( t ) -> dots.ignite -> is_ticking() )
          {
            // TODO: This does nothing for now, fix this when bug is resolved
            // Source: https://twitter.com/Celestalon/status/502030039240552448

            // residual_periodic_state_t* dot_state = debug_cast<residual_periodic_state_t*>( ignite_dot -> state );
            // residual_action::trigger( p() -> active_ignite, t, dot_state -> tick_amount * ignite_dot -> ticks_left() );
          }
          else
          {
            ignite_dot -> copy( t, DOT_COPY_START );
          }
        }

        if ( living_bomb_dot -> is_ticking() )
        {
          living_bomb_dot -> copy( t, DOT_COPY_CLONE );
        }

        if ( pyroblast_dot -> is_ticking() )
        {
          pyroblast_dot -> copy( t, DOT_COPY_CLONE );
        }

        if ( --spread_remaining == 0 )
          break;
      }

      if ( s -> result == RESULT_CRIT && p() -> talents.kindling -> ok() )
          p() -> cooldowns.combustion -> adjust( timespan_t::from_seconds( - p() -> talents.kindling -> effectN( 1 ).base_value() ) );

      trigger_ignite( s );
    }
  }

  // Inferno Blast always crits
  virtual double composite_crit() const
  { return 1.0; }

  virtual double composite_target_crit( player_t* ) const
  { return 0.0; }

  virtual void execute()
  {
    mage_spell_t::execute();

    if ( p() -> sets.has_set_bonus( SET_CASTER, T16, B4 ) )
    {
      p() -> buffs.fiery_adept -> trigger();
    }
  }
};

// Living Bomb Spell ========================================================

struct living_bomb_explosion_t : public mage_spell_t
{
  living_bomb_explosion_t( mage_t* p ) :
    mage_spell_t( "living_bomb_explosion", p, p -> find_spell( 44461 ) )
  {
    aoe = -1;
    background = true;
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
  }

  void impact( action_state_t* s )
  {
    if ( result_is_hit( s -> result ) )
    {
      dot_t* dot = get_dot( s -> target );
      if ( dot -> is_ticking() && dot -> remains() < dot_duration * 0.3 )
      {
        explosion_spell -> execute();
      }
    }
    mage_spell_t::impact( s );
  }

  void tick( dot_t* d )
  {
    mage_spell_t::tick( d );
    if ( d -> ticks_left() == 0 )
      explosion_spell -> execute();
  }
};

// Meteor Spell ========================================================

struct meteor_impact_t : public mage_spell_t
{
  int targets_hit;
  meteor_impact_t( mage_t* p, int targets ) :
    mage_spell_t( "meteor_burn", p )
  {
    // Sp_Coeff is stored in 153564 for the impact
    parse_spell_data( *p -> dbc.spell( 155158 ) );
    spell_power_mod.direct = p -> find_spell( 153564 ) -> effectN( 1 ).sp_coeff();
    dot_duration = timespan_t::from_seconds( 7.0 );
    hasted_ticks = may_miss = false;
    targets_hit = targets;
    may_crit = tick_may_crit = split_aoe_damage = dual = true;
    school = SCHOOL_FIRE;
  }

  void execute()
  { 
    aoe = targets_hit;
    mage_spell_t::execute();
  }

  virtual timespan_t travel_time() const
  { return timespan_t::from_seconds( 1.0 ); }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );
    if ( result_is_hit( s -> result) || result_is_multistrike( s -> result) )
      trigger_ignite( s );
  }
};

struct meteor_t : public mage_spell_t
{
  int targets;
  meteor_impact_t* meteor_impact;
  meteor_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "meteor", p, p -> find_talent_spell( "Meteor") ),
    targets( -1 ),
    meteor_impact( new meteor_impact_t( p, targets ) )
  {
    option_t options[] =
    {
      opt_int( "targets", targets ),
      opt_null()
    };
    parse_options( options, options_str );
    callbacks = false;
    add_child( meteor_impact );
    dot_duration = timespan_t::zero();
    school = SCHOOL_FIRE;
  }

  virtual timespan_t travel_time() const
  { return timespan_t::from_seconds( ( 3 * p() ->  composite_spell_haste() ) - 1.0 ); }

  void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );
    meteor_impact -> execute();
  }
};

// Mirror Image Spell =======================================================

struct mirror_image_t : public mage_spell_t
{
  mirror_image_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "mirror_image", p, p -> find_spell( 55342 ) )
  {
    parse_options( NULL, options_str );
    dot_duration = timespan_t::zero();
    harmful = false;
    if ( !p -> talents.mirror_image -> ok() )
      background = true;
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

    if (!p() -> talents.mirror_image -> ok() )
      return;

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

// Nether Tempest AoE Spell ====================================================
struct nether_tempest_aoe_t: public mage_spell_t
{
  nether_tempest_aoe_t( mage_t* p ) :
    mage_spell_t( "nether_tempest_aoe", p, p -> find_spell( 114954 ) )
  {
    aoe = -1;
    background = true;
  }

  virtual resource_e current_resource() const
  { return RESOURCE_NONE; }

  virtual timespan_t travel_time() const
  {
    timespan_t t = mage_spell_t::travel_time();
    t = timespan_t::from_seconds( 1.3 );
    return t;
  }

};

// Nether Tempest Spell ===========================================================
struct nether_tempest_t : public mage_spell_t
{
  nether_tempest_aoe_t *add_aoe;

  nether_tempest_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "nether_tempest", p, p -> talents.nether_tempest ),
    add_aoe( new nether_tempest_aoe_t( p ) )
  {
    parse_options( NULL, options_str );
    add_child( add_aoe );
  }

  virtual void execute()
  {
    mage_t& p = *this -> p();


    mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if (p.last_bomb_target != execute_state -> target && p.last_bomb_target != 0)
      {
        td(p.last_bomb_target) -> dots.nether_tempest -> cancel();
      }
      p.last_bomb_target = execute_state -> target;
    }
  }

  virtual void tick( dot_t* d )
  {
    mage_spell_t::tick( d );

    action_state_t* aoe_state = add_aoe -> get_state( d -> state );
    aoe_state -> target = d -> target;

    add_aoe -> schedule_execute( aoe_state );
  }

  virtual void last_tick( dot_t* d )
  {
    mage_spell_t::last_tick( d );
  }

  double composite_persistent_multiplier( const action_state_t* state ) const
  {
    double m = mage_spell_t::composite_persistent_multiplier( state );

    m *= 1.0 +  p() -> buffs.arcane_charge -> stack() * p() -> spells.arcane_charge_arcane_blast -> effectN( 1 ).percent();

    return m;
  }
};

// Presence of Mind Spell ===================================================

struct presence_of_mind_t : public mage_spell_t
{
  presence_of_mind_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "presence_of_mind", p, p -> find_class_spell( "Presence of Mind" )  )
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
  bool is_hot_streak,
       dot_is_hot_streak;

  pyroblast_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "pyroblast", p, p -> find_class_spell( "Pyroblast" ) ),
    is_hot_streak( false ), dot_is_hot_streak( false )
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

    if ( p() -> buffs.pyroblast -> check() && p() -> sets.has_set_bonus( SET_CASTER, T16, B2 ) )
    {
      p() -> buffs.potent_flames -> trigger();
    }

    is_hot_streak = p() -> buffs.pyroblast -> check() != 0;

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
    dot_is_hot_streak = is_hot_streak;

    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result) || result_is_multistrike( s -> result) )
      trigger_ignite( s );

    if ( s -> result == RESULT_CRIT && p() -> talents.kindling -> ok() )
      p() -> cooldowns.combustion -> adjust( timespan_t::from_seconds( - p() -> talents.kindling -> effectN( 1 ).base_value() ) );

    if ( p() -> sets.has_set_bonus( SET_CASTER, PVP, B4 ) && is_hot_streak )
      td( s -> target ) -> debuffs.firestarter -> trigger();
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

    c += p() -> sets.set( SET_CASTER, T15, B4 ) -> effectN( 2 ).percent();

    if ( p() -> buffs.fiery_adept -> check() )
      c += 1.0;

    return c;
  }

  virtual double action_da_multiplier() const
  {
    double am = mage_spell_t::action_da_multiplier();

    if ( p() -> buffs.pyroblast -> up() )
    {
      am *= 1.0 + p() -> find_spell( 48108 ) -> effectN( 3 ).percent();
    }
    return am;
  }

  virtual double action_ta_multiplier() const
  {
    double am = mage_spell_t::action_ta_multiplier();

    if ( dot_is_hot_streak )
    {
      am *= 1.0 + p() -> find_spell( 48108 ) -> effectN( 3 ).percent();
    }
    return am;
  }

  void reset()
  {
    is_hot_streak = false;
    dot_is_hot_streak = false;
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

  }

  virtual void impact( action_state_t* s )
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result) || result_is_multistrike( s -> result) )
      trigger_ignite( s );

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

// Supernova Spell ==========================================================

struct supernova_t : public mage_spell_t
{
    supernova_t( mage_t* p, const std::string& options_str ) :
       mage_spell_t( "supernova", p, p -> talents.supernova )
    {
        parse_options( NULL, options_str );
        aoe = 1;
        base_multiplier *= 1.0 + p -> talents.supernova -> effectN( 1 ).percent();
        base_aoe_multiplier *= 0.5;
    }

    virtual void init()
    {
        mage_spell_t::init();

        // NOTE: Cooldown missing from tooltip since WoD beta build 18379
        cooldown -> duration = timespan_t::from_seconds( 25.0 );
        cooldown -> charges = 2;
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
    if ( sim -> overrides.bloodlust )
      return false;

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

// Prismatic Crystal =========================================================

struct prismatic_crystal_t : public mage_spell_t
{
  prismatic_crystal_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "prismatic_crystal", p, p -> find_talent_spell( "Prismatic Crystal" ) )
  {
    parse_options( NULL, options_str );
    may_miss = may_crit = harmful = callbacks = false;
  }

  void execute()
  {
    mage_spell_t::execute();

    p() -> pets.prismatic_crystal -> summon( data().duration() );
  }
};

// Choose target ============================================================

struct choose_target_t : public action_t
{
  bool check_selected;
  player_t* selected_target;

  // Infinite loop protection
  timespan_t last_execute;

  choose_target_t( mage_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "choose_target", p ),
    check_selected( false ), selected_target( 0 ),
    last_execute( timespan_t::min() )
  {
    std::string target_name;

    option_t options[] = {
      opt_string( "name", target_name ),
      opt_bool( "check_selected", check_selected ),
      opt_null()
    };

    parse_options( options, options_str );

    if ( ! target_name.empty() && ! util::str_compare_ci( target_name, "default" ) )
    {
      player_t* target = p -> actor_by_name_str( target_name );
      if ( ! target )
        sim -> errorf( "%s unable to find target named '%s'", p -> name(), target_name.c_str() );
      else
        selected_target = target;
    }
    else
      selected_target = p -> target;

    if ( ! selected_target )
      background = true;

    trigger_gcd = timespan_t::zero();

    harmful = may_miss = may_crit = callbacks = false;
  }

  result_e calculate_result( action_state_t* )
  { return RESULT_HIT; }

  block_result_e calculate_block_result( action_state_t* )
  { return BLOCK_RESULT_UNBLOCKED; }

  void execute()
  {
    action_t::execute();

    mage_t* p = debug_cast<mage_t*>( player );

    if ( sim -> current_time == last_execute )
    {
      sim -> errorf( "%s choose_target infinite loop detected (due to no time passing between executes) at '%s'",
        p -> name(), signature_str.c_str() );
      sim -> cancel();
      return;
    }

    last_execute = sim -> current_time;

    if ( sim -> debug )
      sim -> out_debug.printf( "%s swapping target from %s to %s", player -> name(), p -> current_target -> name(), selected_target -> name() );

    p -> current_target = selected_target;
  }

  bool ready()
  {
    mage_t* p = debug_cast<mage_t*>( player );

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

    player_t* original_target = 0;
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

  void reset()
  {
    action_t::reset();
    last_execute = timespan_t::min();
  }
};


// Combustion Pyroblast Chaining Switch ==========================================================


struct start_pyro_chain_t : public action_t
{
  // Infinite loop protection
  timespan_t last_execute;

  start_pyro_chain_t( mage_t* p, const std::string& options_str ):
    action_t( ACTION_USE, "start_pyro_chain", p ),
    last_execute( timespan_t::min() )
  {
    parse_options( NULL, options_str );
    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = debug_cast<mage_t*>( player );

    if ( sim -> current_time == last_execute )
    {
      sim -> errorf( "%s start_pyro_chain infinite loop detected (no time passing between executes) at '%s'",
        p -> name(), signature_str.c_str() );
      sim -> cancel();
      return;
    }

    last_execute = sim -> current_time;

    p -> pyro_switch.dump_state = true;
  }

  void reset()
  {
    action_t::reset();
    last_execute = timespan_t::min();
  }
};

struct stop_pyro_chain_t : public action_t
{
  stop_pyro_chain_t( mage_t* p, const std::string& options_str ):
     action_t( ACTION_USE, "stop_pyro_chain", p )
  {
      parse_options( NULL, options_str );
      trigger_gcd = timespan_t::zero();
      harmful = false;
  }

  virtual void execute()
  {
      mage_t* p = debug_cast<mage_t*>( player );
      p -> pyro_switch.dump_state = false;
  }
};

// Choose Rotation =================================================================================

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


} // namespace actions

namespace events {

struct icicle_event_t : public event_t
{
  mage_t* mage;
  icicle_data_t state;

  icicle_event_t( mage_t& m, const icicle_data_t& s, bool first = false ) :
    event_t( m, "icicle_event" ), mage( &m ), state( s )
  {
    double cast_time = first ? 0.25 : 0.75;
    cast_time *= mage -> cache.spell_speed();

    add_event( timespan_t::from_seconds( cast_time ) );
  }

  void execute()
  {
    actions::icicle_state_t* new_s = debug_cast<actions::icicle_state_t*>( mage -> icicle -> get_state() );
    new_s -> source = state.second;

    mage -> icicle -> base_dd_min = mage -> icicle -> base_dd_max = state.first;
    mage -> icicle -> schedule_execute( new_s );

    icicle_data_t new_state = mage -> get_icicle_object();
    if ( new_state.first > 0 )
    {
      mage -> icicle_event = new ( sim() ) icicle_event_t( *mage, new_state );
      if ( mage -> sim -> debug )
        mage -> sim -> out_debug.printf( "%s icicle use (chained), damage=%f, total=%u",
                               mage -> name(), new_state.first, as<unsigned>( mage -> icicles.size() ) );
    }
    else
      mage -> icicle_event = 0;
  }
};

} // namespace events

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

  debuffs.slow = buff_creator_t( *this, "slow" ).spell( mage -> spec.slow );
  debuffs.frost_bomb = buff_creator_t( *this, "frost_bomb" ).spell( mage -> talents.frost_bomb );
  debuffs.firestarter = buff_creator_t( *this, "firestarter" ).chance( 1.0 ).duration( timespan_t::from_seconds( 10.0 ) );

}

// mage_t::create_action ====================================================

action_t* mage_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  using namespace actions;

  if ( name == "arcane_barrage"    ) return new          arcane_barrage_t( this, options_str );
  if ( name == "arcane_blast"      ) return new            arcane_blast_t( this, options_str );
  if ( name == "arcane_brilliance" ) return new       arcane_brilliance_t( this, options_str );
  if ( name == "arcane_explosion"  ) return new        arcane_explosion_t( this, options_str );
  if ( name == "arcane_missiles"   ) return new         arcane_missiles_t( this, options_str );
  if ( name == "arcane_power"      ) return new            arcane_power_t( this, options_str );
  if ( name == "blazing_speed"     ) return new           blazing_speed_t( this, options_str );
  if ( name == "blink"             ) return new                   blink_t( this, options_str );
  if ( name == "blizzard"          ) return new                blizzard_t( this, options_str );
  if ( name == "choose_rotation"   ) return new         choose_rotation_t( this, options_str );
  if ( name == "start_pyro_chain"  ) return new        start_pyro_chain_t( this, options_str );
  if ( name == "stop_pyro_chain"   ) return new        stop_pyro_chain_t(  this, options_str );
  if ( name == "cold_snap"         ) return new               cold_snap_t( this, options_str );
  if ( name == "combustion"        ) return new              combustion_t( this, options_str );
  if ( name == "cone_of_cold"      ) return new            cone_of_cold_t( this, options_str );
  if ( name == "counterspell"      ) return new            counterspell_t( this, options_str );
  if ( name == "dragons_breath"    ) return new          dragons_breath_t( this, options_str );
  if ( name == "evocation"         ) return new               evocation_t( this, options_str );
  if ( name == "fire_blast"        ) return new              fire_blast_t( this, options_str );
  if ( name == "fireball"          ) return new                fireball_t( this, options_str );
  if ( name == "flamestrike"       ) return new             flamestrike_t( this, options_str );
  if ( name == "frost_bomb"        ) return new              frost_bomb_t( this, options_str );
  if ( name == "frostbolt"         ) return new               frostbolt_t( this, options_str );
  if ( name == "frostfire_bolt"    ) return new          frostfire_bolt_t( this, options_str );
  if ( name == "frozen_orb"        ) return new              frozen_orb_t( this, options_str );
  if ( name == "ice_floes"         ) return new               ice_floes_t( this, options_str );
  if ( name == "ice_lance"         ) return new               ice_lance_t( this, options_str );
  if ( name == "icy_veins"         ) return new               icy_veins_t( this, options_str );
  if ( name == "inferno_blast"     ) return new           inferno_blast_t( this, options_str );
  if ( name == "living_bomb"       ) return new             living_bomb_t( this, options_str );
  if ( name == "arcane_orb"        ) return new              arcane_orb_t( this, options_str );
  if ( name == "meteor"            ) return new                  meteor_t( this, options_str );
  if ( name == "comet_storm"       ) return new             comet_storm_t( this, options_str );
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
  if ( name == "nether_tempest"    ) return new          nether_tempest_t( this, options_str );
  if ( name == "presence_of_mind"  ) return new        presence_of_mind_t( this, options_str );
  if ( name == "pyroblast"         ) return new               pyroblast_t( this, options_str );
  if ( name == "rune_of_power"     ) return new           rune_of_power_t( this, options_str );
  if ( name == "scorch"            ) return new                  scorch_t( this, options_str );
  if ( name == "slow"              ) return new                    slow_t( this, options_str );
  if ( name == "supernova"         ) return new               supernova_t( this, options_str );
  if ( name == "blast_wave"        ) return new              blast_wave_t( this, options_str );
  if ( name == "ice_nova"          ) return new                ice_nova_t( this, options_str );
  if ( name == "time_warp"         ) return new               time_warp_t( this, options_str );
  if ( name == "water_elemental"   ) return new  summon_water_elemental_t( this, options_str );
  if ( name == "prismatic_crystal" ) return new prismatic_crystal_t( this, options_str );
  if ( name == "choose_target"     ) return new choose_target_t( this, options_str );
  //if ( name == "alter_time"        ) return new              alter_time_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// mage_t::create_pets ======================================================

void mage_t::create_pets()
{
  pets.water_elemental = new pets::water_elemental_pet_t( sim, this ); 
  pets.prismatic_crystal = new pets::prismatic_crystal_t( sim, this );

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
  talents.blast_wave         = find_talent_spell( "Blast Wave" );
  talents.cauterize          = find_talent_spell( "Cauterize" );
  talents.cold_snap          = find_talent_spell( "Cold Snap" );
  talents.frostjaw           = find_talent_spell( "Frostjaw" );
  talents.frost_bomb         = find_talent_spell( "Frost Bomb" );
  talents.greater_invis      = find_talent_spell( "Greater Invisibility" );
  talents.ice_barrier        = find_talent_spell( "Ice Barrier" );
  talents.ice_floes          = find_talent_spell( "Ice Floes" );
  talents.ice_nova           = find_talent_spell( "Ice Nova" );
  talents.ice_ward           = find_talent_spell( "Ice Ward" );
  talents.kindling           = find_talent_spell( "Kindling" );
  talents.living_bomb        = find_talent_spell( "Living Bomb" );
  talents.nether_tempest     = find_talent_spell( "Nether Tempest" );
  talents.ring_of_frost      = find_talent_spell( "Ring of Frost" );
  talents.rune_of_power      = find_talent_spell( "Rune of Power" );
  talents.scorch             = find_talent_spell( "Scorch" );
  talents.temporal_shield    = find_talent_spell( "Temporal Shield" );
  talents.supernova          = find_talent_spell( "Supernova" );
  talents.overpowered        = find_talent_spell( "Overpowered" );
  talents.arcane_orb         = find_talent_spell( "Arcane Orb" );
  talents.unstable_magic     = find_talent_spell( "Unstable Magic" );
  talents.mirror_image       = find_talent_spell( "Mirror Image" );
  talents.incanters_flow     = find_talent_spell( "Incanter's Flow" );
  talents.prismatic_crystal  = find_talent_spell( "Prismatic Crystal" );
  talents.thermal_void       = find_talent_spell( "Thermal Void" );
  talents.comet_storm        = find_talent_spell( "Comet Storm" );


  // Passive Spells
  passives.nether_attunement = find_specialization_spell( "Nether Attunement" ); // BUG: Not in spell lists at present.
  passives.nether_attunement = ( find_spell( 117957 ) -> is_level( level ) ) ? find_spell( 117957 ) : spell_data_t::not_found();
  passives.shatter           = find_specialization_spell( "Shatter" ); // BUG: Doesn't work at present as Shatter isn't tagged as a spec of Frost.
  passives.shatter           = ( find_spell( 12982 ) -> is_level( level ) ) ? find_spell( 12982 ) : spell_data_t::not_found();
  passives.frost_armor       = find_specialization_spell( "Frost Armor" );
  passives.mage_armor        = find_specialization_spell( "Mage Armor" );
  passives.molten_armor      = find_specialization_spell( "Molten Armor" );


  // Perks - Fire
  perks.enhanced_pyrotechnics                = find_perk_spell( "Enhanced Pyrotechnics" );
  perks.improved_flamestrike                 = find_perk_spell( "Improved Flamestrike" );
  perks.improved_inferno_blast               = find_perk_spell( "Improved Inferno Blast" );
  perks.improved_scorch                      = find_perk_spell( "Improved Scorch" );
  // Perks - Arcane
  perks.enhanced_arcane_blast                = find_perk_spell( "Enhanced Arcane Blast" );
  perks.improved_arcane_power                = find_perk_spell( "Improved Arcane Power" );
  perks.improved_evocation                   = find_perk_spell( "Improved Evocation" );
  perks.improved_blink                       = find_perk_spell( "Improved Blink" );
  // Perks - Frost
  perks.enhanced_frostbolt                   = find_perk_spell( "Enhanced Frostbolt" );
  perks.improved_blizzard                    = find_perk_spell( "Improved Blizzard" );
  perks.improved_water_elemental             = find_perk_spell( "Improved Water Elemental" );
  perks.improved_icy_veins                   = find_perk_spell( "Improved Icy Veins" );

  // Spec Spells
  spec.arcane_charge         = find_specialization_spell( "Arcane Charge" );
  spells.arcane_charge_arcane_blast = spec.arcane_charge -> ok() ? find_spell( 36032 ) : spell_data_t::not_found();
  spec.presence_of_mind      = find_specialization_spell( "Presence of Mind" );
  spec.slow                  = find_class_spell( "Slow" );

  spec.brain_freeze          = find_specialization_spell( "Brain Freeze" );
  spec.critical_mass         = find_specialization_spell( "Critical Mass" );
  spec.fingers_of_frost      = find_specialization_spell( "Fingers of Frost" );
  spec.frostbolt             = find_specialization_spell( "Frostbolt" );

  // Attunments
  spec.arcane_mind           = find_specialization_spell( "Arcane Mind"  );
  spec.ice_shards            = find_specialization_spell( "Ice Shards"   );
  spec.incineration          = find_specialization_spell( "Incineration" );
  
  // Mastery
  spec.icicles               = find_mastery_spell( MAGE_FROST );
  spec.ignite                = find_mastery_spell( MAGE_FIRE );
  spec.mana_adept            = find_mastery_spell( MAGE_ARCANE );

  spells.stolen_time         = spell_data_t::find( 105791, "Stolen Time" );
  spells.icicles_driver      = find_spell( 148012 );

  // Glyphs
  glyphs.arcane_brilliance   = find_glyph_spell( "Glyph of Arcane Brilliance" );
  glyphs.arcane_power        = find_glyph_spell( "Glyph of Arcane Power" );
  glyphs.blink               = find_glyph_spell( "Glyph of Blink" );
  glyphs.combustion          = find_glyph_spell( "Glyph of Combustion" );
  glyphs.cone_of_cold        = find_glyph_spell( "Glyph of Cone of Cold" );
  glyphs.frostfire           = find_glyph_spell( "Glyph of Frostfire" );
  glyphs.icy_veins           = find_glyph_spell( "Glyph of Icy Veins" );
  glyphs.inferno_blast       = find_glyph_spell( "Glyph of Inferno Blast" );
  glyphs.living_bomb         = find_glyph_spell( "Glyph of Living Bomb" );
  glyphs.rapid_displacement  = find_glyph_spell( "Glyph of Rapid Displacement" );
  glyphs.splitting_ice       = find_glyph_spell( "Glyph of Splitting Ice" );

  // Active spells
  if ( spec.ignite -> ok()  ) active_ignite = new actions::ignite_t( this );
  if ( spec.icicles -> ok() ) icicle = new actions::icicle_t( this );

}

// mage_t::init_base ========================================================

void mage_t::init_base_stats()
{
  player_t::init_base_stats();

  base.spell_power_per_intellect = 1.0;

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility = 0.0;
  
  base.mana_regen_per_second = resources.base[ RESOURCE_MANA ] * 0.018;
  
  // Reduce fire mage distance to avoid proc munching at high haste
  // 2 yards was selected through testing with T16H profile
  if ( specialization() == MAGE_FIRE )
    base.distance = 20;
}

// mage_t::init_scaling =====================================================

void mage_t::init_scaling()
{
  player_t::init_scaling();
}

// mage_t::init_buffs =======================================================

struct incanters_flow_t : public buff_t
{
  incanters_flow_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "incanters_flow", p -> find_spell( 116267 ) ) // Buff is a separate spell
            .duration( p -> sim -> max_time * 3 ) // Long enough duration to trip twice_expected_event
            .period( p -> talents.incanters_flow -> effectN( 1 ).period() ) ) // Period is in the talent
  { }

  void bump( int stacks, double value )
  {
    int before_stack = current_stack;
    buff_t::bump( stacks, value );
    // Reverse direction if max stacks achieved before bump
    if ( before_stack == current_stack )
      reverse = true;
  }

  void decrement( int stacks, double value )
  {
    // This buff will never fade; reverse direction at 1 stack.
    // Buff uptime reporting _should_ work ok with this solution
    if ( current_stack > 1 )
      buff_t::decrement( stacks, value );
    else
      reverse = false;
  }
};


void mage_t::create_buffs()
{
  player_t::create_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buffs.arcane_charge        = buff_creator_t( this, "arcane_charge", spec.arcane_charge )
                               .max_stack( find_spell( 36032 ) -> max_stacks() )
                               .duration( find_spell( 36032 ) -> duration() );
  buffs.arcane_missiles      = buff_creator_t( this, "arcane_missiles", find_class_spell( "Arcane Missiles" ) -> ok() ? find_spell( 79683 ) : spell_data_t::not_found() )
                                .chance( find_spell( 79684 ) -> proc_chance() ).max_stack( find_spell( 79683 ) -> max_stacks() );

  buffs.arcane_power         = new buffs::arcane_power_t( this );
  buffs.blazing_speed        = buff_creator_t( this, "blazing_speed", talents.blazing_speed )
                               .default_value( talents.blazing_speed -> effectN( 1 ).percent() );
  buffs.brain_freeze         = buff_creator_t( this, "brain_freeze", spec.brain_freeze )
                               .duration( find_spell( 57761 ) -> duration() )
                               .default_value( spec.brain_freeze -> effectN( 1 ).percent() ).max_stack( 2 ).chance( find_spell( 44549 ) -> effectN( 1 ).percent() );;

  buffs.fingers_of_frost     = buff_creator_t( this, "fingers_of_frost", find_spell( 112965 ) ).chance( find_spell( 112965 ) -> effectN( 1 ).percent() )
                               .duration( timespan_t::from_seconds( 15.0 ) )
                               .max_stack( 2 );
  buffs.frost_armor          = buff_creator_t( this, "frost_armor", find_spell( 7302 ) ).add_invalidate( CACHE_MULTISTRIKE );
  if( glyphs.icy_veins -> ok() )
    buffs.icy_veins            = buff_creator_t( this, "icy_veins", find_spell( 12472 ) ).add_invalidate( CACHE_MULTISTRIKE );
  else
    buffs.icy_veins            = buff_creator_t( this, "icy_veins", find_spell( 12472 ) ).add_invalidate( CACHE_SPELL_HASTE );

  buffs.enhanced_frostbolt   = buff_creator_t( this, "enhanced_frostbolt", find_spell( 157646 ) ).duration( timespan_t::from_seconds( 15.0 ) );
  buffs.ice_floes            = buff_creator_t( this, "ice_floes", talents.ice_floes );
  buffs.improved_blink       = buff_creator_t( this, "improved_blink", perks.improved_blink )
                               .default_value( perks.improved_blink -> effectN( 1 ).percent() );
  buffs.mage_armor           = stat_buff_creator_t( this, "mage_armor" ).spell( find_spell( 6117 ) );
  buffs.molten_armor         = buff_creator_t( this, "molten_armor", find_spell( 30482 ) ).add_invalidate( CACHE_SPELL_CRIT );
  buffs.presence_of_mind     = buff_creator_t( this, "presence_of_mind", find_spell(12043) ).duration( timespan_t::zero() ).activated( true );
  buffs.rune_of_power        = buff_creator_t( this, "rune_of_power", find_spell( 116014 ) )
                               .duration( timespan_t::from_minutes( 3 ) )
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.heating_up           = buff_creator_t( this, "heating_up", find_specialization_spell( "Pyroblast" ) -> ok() ? find_spell( 48107 ) : spell_data_t::not_found() );
  buffs.pyroblast            = buff_creator_t( this, "pyroblast",  find_specialization_spell( "Pyroblast" ) -> ok() ? find_spell( 48108 ) : spell_data_t::not_found() );



  //buffs.alter_time           = new buffs::alter_time_t( this );
  buffs.enhanced_pyrotechnics = buff_creator_t( this, "enhanced_pyrotechnics", find_spell( 157644 ) );


  buffs.profound_magic       = buff_creator_t( this, "profound_magic" )
                               .spell( find_spell( 145252 ) );
  buffs.potent_flames        = stat_buff_creator_t( this, "potent_flames" )
                               .spell( find_spell( 145254 ) );
  buffs.frozen_thoughts      = buff_creator_t( this, "frozen_thoughts" )
                               .spell( find_spell( 146557 ) );
  buffs.fiery_adept          = buff_creator_t( this, "fiery_adept" )
                               .spell( find_spell( 145261 ) )
                               .chance( 1.0 );

  buffs.incanters_flow = new incanters_flow_t( this );
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


//Pre-combat Action Priority List============================================

void mage_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  if( sim -> allow_flasks && level >= 80 )
  {
    std::string flask_action = "flask,type=";

    if ( level <= 85 )
      flask_action += "draconic_mind" ;
    else if ( level <= 90 )
      flask_action += "warm_sun" ;
    else
      flask_action += "greater_draenic_intellect_flask" ;

    precombat -> add_action( flask_action );
  }
    // Food
  if ( sim -> allow_food && level >= 80 )
  {
    std::string food_action = "food,type=";

    if ( level <= 85 )
      food_action += "seafood_magnifique_feast" ;
    else if ( level <= 90 )
      food_action += "mogu_fish_stew" ;
    else if ( specialization() == MAGE_ARCANE )
      food_action += "sleeper_surprise" ;
    else if ( specialization() == MAGE_FIRE )
      food_action += "blackrock_barbecue" ;
    else
      food_action += "calamari_crepes" ;

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
  if ( sim -> allow_potions && level >= 80 )
  {
    precombat -> add_action( get_potion_action() );
  }

  if ( specialization() == MAGE_ARCANE )
    precombat -> add_action( this, "Arcane Blast" );
  else if ( specialization() == MAGE_FIRE )
    precombat -> add_action( this, "Pyroblast" );
  else
    precombat -> add_action( this, "Frostbolt" );
}


// Util for using level appropriate potion

std::string mage_t::get_potion_action()
{
  std::string potion_action = "potion,name=";

  if ( level <= 85 )
    potion_action += "volcanic" ;
  else if ( level <= 90 )
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

  action_priority_list_t* init_crystal        = get_action_priority_list( "init_crystal"     );
  action_priority_list_t* crystal_sequence    = get_action_priority_list( "crystal_sequence" );
  action_priority_list_t* cooldowns           = get_action_priority_list( "cooldowns"        );
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe"              );
  action_priority_list_t* burn                = get_action_priority_list( "burn"             );
  action_priority_list_t* conserve            = get_action_priority_list( "conserve"         );


  default_list -> add_action( this, "Counterspell",
                              "if=target.debuff.casting.react" );
  default_list -> add_action( this, "Blink",
                              "if=movement.distance>10" );
  default_list -> add_talent( this, "Blazing Speed",
                              "if=movement.remains>0" );
  default_list -> add_talent( this, "Cold Snap",
                              "if=health.pct<30" );
  default_list -> add_action( this, "Time Warp",
                              "if=target.health.pct<25|time>5" );
  default_list -> add_talent( this, "Rune of Power",
                              "if=buff.rune_of_power.remains<cast_time" );
  default_list -> add_talent( this, "Mirror Image" );
  default_list -> add_action( "call_action_list,name=init_crystal,if=talent.prismatic_crystal.enabled" );
  default_list -> add_action( "call_action_list,name=crystal_sequence,if=pet.prismatic_crystal.active" );
  default_list -> add_action( "call_action_list,name=aoe,if=active_enemies>=6" );
  default_list -> add_action( "call_action_list,name=burn,if=buff.arcane_power.up&cooldown.evocation.remains<buff.arcane_power.remains&mana.pct>15&talent.prismatic_crystal.enabled" );
  default_list -> add_action( "call_action_list,name=conserve" );


  init_crystal -> add_talent( this, "Prismatic Crystal",
                              "if=cooldown.arcane_power.remains=0&buff.arcane_charge.stack=4",
                              "Conditions for initiating Prismatic Crystal burn phase" );
  init_crystal -> add_talent( this, "Prismatic Crystal",
                              "if=buff.arcane_charge.stack=4&(glyph.arcane_power.enabled&cooldown.arcane_power.remains>45)|(!glyph.arcane_power.enabled&cooldown.arcane_power.remains>15)" );


  crystal_sequence -> add_action( "call_action_list,name=cooldowns",
                                  "Actions while Prismatic Crystal is active" );
  crystal_sequence -> add_talent( this, "Nether Tempest",
                                  "if=current_target=prismatic_crystal&buff.arcane_charge.stack=4&!ticking&cooldown.prismatic_crystal.remains>58" );
  crystal_sequence -> add_action( "call_action_list,name=burn,if=cooldown.evocation.remains<cooldown.prismatic_crystal.remains-50" );
  crystal_sequence -> add_action( "call_action_list,name=conserve" );


  cooldowns -> add_action( this, "Arcane Power",
                           "",
                           "Consolidated damage cooldown abilities" );

  for( size_t i = 0; i < racial_actions.size(); i++ )
    cooldowns -> add_action( racial_actions[i] );

  cooldowns -> add_action( get_potion_action() + ",if=buff.arcane_power.up&(!talent.prismatic_crystal.enabled|pet.prismatic_crystal.active)" );

  for( size_t i = 0; i < item_actions.size(); i++ )
    cooldowns -> add_action( item_actions[i] );


  aoe -> add_talent( this, "Nether Tempest",
                     "if=buff.arcane_charge.stack=4&(active_dot.nether_tempest=0|(ticking&remains<3.6))",
                     "AoE sequence" );
  aoe -> add_talent( this, "Supernova" );
  aoe -> add_action( this, "Arcane Barrage",
                     "if=buff.arcane_charge.stack=4" );
  aoe -> add_talent( this, "Arcane Orb",
                     "if=buff.arcane_charge.stack<4" );
  aoe -> add_action( this, "Arcane Explosion" );


  burn -> add_action( this, "Arcane Missiles",
                      "if=buff.arcane_missiles.react=3",
                      "High mana usage, \"Burn\" sequence" );
  burn -> add_talent( this, "Supernova",
                      "if=charges=2" );
  burn -> add_talent( this, "Nether Tempest",
                      "cycle_targets=1,if=target!=prismatic_crystal&buff.arcane_charge.stack=4&(active_dot.nether_tempest=0|(ticking&remains<3.6))" );
  burn -> add_action( this, "Arcane Blast",
                      "if=mana.pct>95&buff.arcane_charge.stack=4" );
  burn -> add_action( this, "Arcane Missiles",
                      "if=buff.arcane_charge.stack=4" );
  burn -> add_talent( this, "Supernova" );
  burn -> add_action( this, "Arcane Barrage",
                      "if=buff.presence_of_mind.up&buff.arcane_charge.stack=4" );
  burn -> add_action( this, "Presence of Mind" );
  burn -> add_action( this, "Arcane Blast" );


  conserve -> add_action( this, "Evocation",
                          "interrupt_if=mana.pct>92,if=mana.pct<65",
                          "Low mana usage, \"Conserve\" sequence" );
  conserve -> add_action( "call_action_list,name=cooldowns,if=time_to_die<30|(buff.arcane_charge.stack=4&!(glyph.arcane_power.enabled&talent.prismatic_crystal.enabled)&(!talent.prismatic_crystal.enabled|cooldown.prismatic_crystal.remains>20))" );
  conserve -> add_action( this, "Arcane Missiles",
                          "if=buff.arcane_missiles.react=3|(talent.overpowered.enabled&buff.arcane_power.up&buff.arcane_power.remains<3)" );
  conserve -> add_action( this, "Arcane Blast",
                          "if=buff.arcane_charge.stack=4&(mana.pct>95|(talent.unstable_magic.enabled&mana.pct>92))" );
  conserve -> add_talent( this, "Nether Tempest",
                          "cycle_targets=1,if=target!=prismatic_crystal&buff.arcane_charge.stack=4&(active_dot.nether_tempest=0|(ticking&remains<3.6))" );
  conserve -> add_talent( this, "Supernova",
                          "if=time_to_die<8|charges=2" );
  conserve -> add_action( this, "Arcane Missiles",
                          "if=buff.arcane_charge.stack=4&(!talent.overpowered.enabled|cooldown.arcane_power.remains>8)" );
  conserve -> add_talent( this, "Supernova",
                          "if=(buff.arcane_missiles.stack<2|buff.arcane_charge.stack=4)&(buff.arcane_power.up|current_target=prismatic_crystal|(charges=1&cooldown.arcane_power.remains>recharge_time)&(!talent.prismatic_crystal.enabled|(charges=1&cooldown.prismatic_crystal.remains>recharge_time)))" );
  conserve -> add_talent( this, "Arcane Orb",
                          "if=buff.arcane_charge.stack<2" );
  conserve -> add_talent( this, "Nether Tempest",
                          "cycle_targets=1,if=target!=prismatic_crystal&buff.arcane_charge.stack=4&(active_dot.nether_tempest=0|(ticking&remains<8*spell_haste))" );
  conserve -> add_action( this, "Arcane Barrage",
                          "if=buff.arcane_charge.stack=4&mana.pct<95" );
  conserve -> add_action( this, "Presence of Mind",
                          "if=buff.arcane_charge.stack<2&cooldown.arcane_power.remains>15&(!talent.prismatic_crystal.enabled|cooldown.prismatic_crystal.remains>15)" );
  conserve -> add_action( this, "Arcane Blast" );
  conserve -> add_talent( this, "Ice Floes", "moving=1" );
  conserve -> add_action( this, "Arcane Barrage", "moving=1" );
}

// Fire Mage Action List ===================================================================================================

void mage_t::apl_fire()
{
  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  action_priority_list_t* default_list        = get_action_priority_list( "default"           );

  action_priority_list_t* crystal_targeting   = get_action_priority_list( "crystal_targeting" );
  action_priority_list_t* init_crystal        = get_action_priority_list( "init_crystal"      );
  action_priority_list_t* crystal_sequence    = get_action_priority_list( "crystal_sequence"  );
  action_priority_list_t* init_combust        = get_action_priority_list( "init_combust"      );
  action_priority_list_t* combust_sequence    = get_action_priority_list( "combust_sequence"  );
  action_priority_list_t* st_combust          = get_action_priority_list( "st_combust"        );
  action_priority_list_t* crystal_combust     = get_action_priority_list( "crystal_combust"   );
  action_priority_list_t* active_talents      = get_action_priority_list( "active_talents"    );
  action_priority_list_t* living_bomb         = get_action_priority_list( "living_bomb"       );
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe"               );
  action_priority_list_t* single_target       = get_action_priority_list( "single_target"     );


  default_list -> add_action( this, "Counterspell",
                              "if=target.debuff.casting.react" );
  default_list -> add_action( this, "Blink",
                              "if=movement.distance>10" );
  default_list -> add_talent( this, "Blazing Speed",
                              "if=movement.remains>0" );
  default_list -> add_talent( this, "Cold Snap",
                              "if=health.pct<30" );
  default_list -> add_action( this, "Time Warp",
                              "if=target.health.pct<25|time>5" );
  default_list -> add_talent( this, "Rune of Power",
                              "if=buff.rune_of_power.remains<cast_time" );
  default_list -> add_action( "call_action_list,name=crystal_targeting,if=pet.prismatic_crystal.active" );
  default_list -> add_action( "call_action_list,name=combust_sequence,if=pyro_chain" );
  default_list -> add_action( "call_action_list,name=crystal_sequence,if=pet.prismatic_crystal.active" );
  default_list -> add_action( "call_action_list,name=init_combust" );
  default_list -> add_action( "call_action_list,name=init_crystal,if=talent.prismatic_crystal.enabled&glyph.combustion.enabled" );
  default_list -> add_talent( this, "Rune of Power",
                              "if=buff.rune_of_power.remains<2*(action.fireball.execute_time+gcd)&(buff.heating_up.down|buff.pyroblast.down|!action.fireball.in_flight)",
                              "Utilize level 90 active talents while avoiding pyro munching" );
  default_list -> add_talent( this, "Mirror Image",
                              "if=buff.heating_up.down|buff.pyroblast.down|!action.fireball.in_flight" );
  default_list -> add_action( "call_action_list,name=aoe,if=active_enemies>=5" );
  default_list -> add_action( "call_action_list,name=single_target");


  crystal_targeting -> add_action( "choose_target,if=!pet.prismatic_crystal.dot.combustion.ticking&((active_dot.combustion>0&cooldown.inferno_blast.up)|cooldown.combustion.up)",
                                   "Target selection when Prismatic Crystal is up\n"
                                   "# PC is mostly used in combo with Combustion for double ticks on single target, or supercharged ticks for spreading in AOE" );
  crystal_targeting -> add_action( "choose_target,name=prismatic_crystal,if=pet.prismatic_crystal.dot.combustion.ticking" );


// TODO: Add multi LB explosions on multitarget fights.
  init_crystal -> add_action( "prismatic_crystal,if=cooldown.combustion.remains>0&buff.pyroblast.up&buff.heating_up.up&(!talent.living_bomb.enabled|(dot.living_bomb.ticking&dot.living_bomb.remains<10))",
                              "Prismatic Crystal sequence initialization\n"
                              "# This sequence is triggered only when Combustion is glyphed, on even numbered PCs which are not used in conjunction with it" );


  crystal_sequence -> add_action( this, "Inferno Blast",
                                  "cycle_targets=1,if=dot.combustion.ticking&!pet.prismatic_crystal.dot.combustion.ticking",
                                  "Action list while Prismatic Crystal is up\n"
                                  "# This line attempts to spread any ticking Combustion onto PC" );
  crystal_sequence -> add_action( this, "Inferno Blast",
                                  "cycle_targets=1,if=talent.living_bomb.enabled&dot.living_bomb.ticking&dot.living_bomb.remains<cooldown.prismatic_crystal.remains-50&!pet.prismatic_crystal.dot.living_bomb.ticking",
                                  "This line attempts to spread any LB that will explode within PC's duration onto it" );
  crystal_sequence -> add_action( this, "Pyroblast",
                                  "if=current_target=prismatic_crystal&buff.pyroblast.up&cooldown.prismatic_crystal.remains-50<gcd+travel_time&cooldown.prismatic_crystal.remains-50>travel_time",
                                  "Use pyros before PC's expiration" );
  crystal_sequence -> add_action( "call_action_list,name=single_target,if=current_target=prismatic_crystal" );


  init_combust -> add_action( "start_pyro_chain,if=talent.meteor.enabled&cooldown.combustion.remains<action.scorch.gcd*3&cooldown.meteor.up&buff.pyroblast.up&((buff.heating_up.down&action.fireball.in_flight)|(buff.heating_up.up&!action.fireball.in_flight))",
                              "Combustion sequence initialization\n"
                              "# This sequence lists the requirements for preparing a Combustion combo with each talent choice" );
  init_combust -> add_action( "start_pyro_chain,if=talent.prismatic_crystal.enabled&(!talent.living_bomb.enabled|dot.living_bomb.remains>action.scorch.execute_time*3)&cooldown.combustion.remains<action.scorch.gcd*2&cooldown.prismatic_crystal.up&buff.pyroblast.up&buff.heating_up.up&action.fireball.in_flight" );
  init_combust -> add_action( "start_pyro_chain,if=!(talent.prismatic_crystal.enabled|talent.meteor.enabled)&cooldown.combustion.remains<action.scorch.gcd*4&buff.pyroblast.up&buff.heating_up.up&action.fireball.in_flight" );


  combust_sequence -> add_action( "call_action_list,name=crystal_combust,if=glyph.combustion.enabled&talent.prismatic_crystal.enabled&active_enemies>=3",
                                  "Combustion sequence\n"
                                  "# This sequence decides whether Combustion should be used on the current target or PC for supercharged AOE ticks" );
  combust_sequence -> add_action( "call_action_list,name=crystal_combust,if=!glyph.combustion.enabled&talent.prismatic_crystal.enabled&active_enemies>=6" );
  combust_sequence -> add_action( "call_action_list,name=st_combust" );


  st_combust -> add_action( "stop_pyro_chain,if=talent.prismatic_crystal.enabled&cooldown.prismatic_crystal.remains>0",
                            "Single target Combustion" );
  st_combust -> add_action( "stop_pyro_chain,if=!talent.prismatic_crystal.enabled&cooldown.combustion.remains>10" );
  st_combust -> add_talent( this, "Meteor",
                            "if=spell_haste>0.667",
                            "At low haste, Meteor should be casted before using CDs. This is due to Meteor's snapshot timing" );

  for( size_t i = 0; i < racial_actions.size(); i++ )
    st_combust -> add_action( racial_actions[i] );
  for( size_t i = 0; i < item_actions.size(); i++ )
    st_combust -> add_action( item_actions[i] );
  st_combust -> add_action( get_potion_action() );

  st_combust -> add_talent( this, "Meteor" );
  st_combust -> add_action( this, "Combustion",
                            "if=talent.prismatic_crystal.enabled&buff.heating_up.down",
                            "Single target PC Combustions utilize PC for double ticks, and avoid spending all pyro buffs, leaving some for use on PC" );
  st_combust -> add_talent( this, "Prismatic Crystal",
                            "if=cooldown.combustion.remains>10" );
  st_combust -> add_action( this, "Combustion",
                            "if=talent.meteor.enabled&dot.meteor_burn.ticking",
                            "Meteor-talented Combustions need to wait for Meteor impact for maximum ignite size" );
  st_combust -> add_action( this, "Inferno Blast",
                            "if=!talent.meteor.enabled&((buff.heating_up.up&buff.pyroblast.down|buff.heating_up.down&buff.pyroblast.down))",
                            "Kindling, PC or regular combustions can use IB for additional Pyro procs during setup" );
  st_combust -> add_action( this, "Pyroblast",
                            "if=buff.pyroblast.up" );
  st_combust -> add_action( this, "Inferno Blast",
                            "if=talent.meteor.enabled",
                            "Meteor Combustions may run out of Pyro procs before impact. IB fills the GCD before Combustion should be triggered" );
  st_combust -> add_action( this, "Combustion",
                            "if=!talent.meteor.enabled" );


  crystal_combust -> add_action( "stop_pyro_chain,if=(!cooldown.prismatic_crystal.up&!pet.prismatic_crystal.active)|(cooldown.combustion.remains>10&!cooldown.inferno_blast.up)",
                                 "Prismatic Crystal Combustion\n"
                                 "# This sequence is used for creating large combustions on PC, to be spread on multiple target fights" );
  crystal_combust -> add_talent( this, "Prismatic Crystal" );
  crystal_combust -> add_action( this, "Inferno Blast",
                                 "if=dot.combustion.ticking&active_dot.combustion<active_enemies" );
  crystal_combust -> add_action( this, "Inferno Blast",
                                 "cycle_targets=1,if=talent.living_bomb.enabled&dot.living_bomb.ticking&dot.living_bomb.remains<cooldown.prismatic_crystal.remains-50&!pet.prismatic_crystal.dot.living_bomb.ticking&buff.heating_up.down" );

  for( size_t i = 0; i < racial_actions.size(); i++ )
    crystal_combust -> add_action( racial_actions[i] );
  for( size_t i = 0; i < item_actions.size(); i++ )
    crystal_combust -> add_action( item_actions[i] );
  crystal_combust -> add_action( get_potion_action() );

  crystal_combust -> add_action( this, "Fireball",
                                 "if=!dot.ignite.ticking&!action.fireball.in_flight" );
  crystal_combust -> add_action( this, "Pyroblast",
                                 "if=buff.pyroblast.up" );
  crystal_combust -> add_action( this, "Combustion" );


  active_talents -> add_talent( this, "Meteor",
                                "if=glyph.combustion.enabled&(!talent.incanters_flow.enabled|buff.incanters_flow.stack+incanters_flow_dir>=4)&cooldown.meteor.duration-cooldown.combustion.remains<10",
                                "Active talents usage\n"
                                "# This sequence summarizes all usage of active talents (BW, LB, Met), and their optimizations with Incanter's Flow and Prismatic Crystal" );
  active_talents -> add_action( "call_action_list,name=living_bomb,if=talent.living_bomb.enabled" );
  active_talents -> add_talent( this, "Blast Wave",
                                "if=(!talent.incanters_flow.enabled|buff.incanters_flow.stack>=4)&(time_to_die<10|!talent.prismatic_crystal.enabled|(charges=1&cooldown.prismatic_crystal.remains>recharge_time)|charges=2|current_target=prismatic_crystal)" );


  living_bomb -> add_action( this, "Inferno Blast",
                             "cycle_targets=1,if=dot.living_bomb.ticking&active_dot.living_bomb<active_enemies",
                             "Living Bomb application" );
  living_bomb -> add_talent( this, "Living Bomb",
                             "cycle_targets=1,if=(((!talent.incanters_flow.enabled|incanters_flow_dir<0|buff.incanters_flow.stack=5)&remains<3.6)|((incanters_flow_dir>0|buff.incanters_flow.stack=1)&remains<gcd)|!ticking)&target.time_to_die>remains+12&current_target!=prismatic_crystal" );


  aoe -> add_action( this, "Inferno Blast",
                     "cycle_targets=1,if=(dot.combustion.ticking&active_dot.combustion<active_enemies)|(dot.living_bomb.ticking&active_dot.living_bomb<active_enemies)|(dot.pyroblast.ticking&active_dot.pyroblast<active_enemies)",
                     "AoE sequence" );
  aoe -> add_action( "call_action_list,name=active_talents" );
  aoe -> add_action( this, "Dragon's Breath",
                     "if=glyph.dragons_breath.enabled" );
  aoe -> add_action( this, "Pyroblast",
                     "if=active_dot.pyroblast=0" );
  aoe -> add_action( this, "Flamestrike" );


  single_target -> add_action( this, "Inferno Blast",
                               "if=(dot.combustion.ticking&active_dot.combustion<active_enemies)|(dot.living_bomb.ticking&active_dot.living_bomb<active_enemies)",
                               "Single target sequence" );
  single_target -> add_action( this, "Pyroblast",
                               "if=buff.pyroblast.up&buff.pyroblast.remains<action.fireball.execute_time",
                               "Use Pyro procs before they run out" );
  single_target -> add_action( this, "Pyroblast",
                               "if=buff.pyroblast.up&buff.heating_up.up&action.fireball.in_flight",
                               "Pyro camp during regular sequence; Do not use Pyro procs without HU and first using fireball" );
  single_target -> add_action( "call_action_list,name=active_talents" );
  single_target -> add_action( this, "Inferno Blast",
                               "if=(buff.pyroblast.down&buff.heating_up.up)|(buff.pyroblast.up&buff.heating_up.down&!action.fireball.in_flight)" );
  single_target -> add_action( this, "Fireball" );
  single_target -> add_action( this, "Scorch", "moving=1" );
}

// Frost Mage Action List ==============================================================================================================

void mage_t::apl_frost()
{
  std::vector<std::string> item_actions = get_item_actions();
  std::vector<std::string> racial_actions = get_racial_actions();

  action_priority_list_t* default_list        = get_action_priority_list( "default" );
  // action_priority_list_t* prismatic_crystal   = get_action_priority_list( "prismatic_crystal" );
  // action_priority_list_t* incanters_flow      = get_action_priority_list( "incanters_flow" );

  default_list -> add_action( this, "Counterspell", "if=target.debuff.casting.react" );
  default_list -> add_action( this, "Blink", "if=movement.distance>10" );
  default_list -> add_talent( this, "Blazing Speed", "if=movement.remains>0" );
  default_list -> add_talent( this, "Cold Snap", "if=health.pct<30" );
  default_list -> add_action( this, "Time Warp", "if=target.health.pct<25|time>5" );
  //not useful if bloodlust is check in option.

  default_list -> add_talent( this, "Mirror Image" );
  default_list -> add_talent( this, "Rune of Power", "if=buff.rune_of_power.remains<cast_time" );
  default_list -> add_talent( this, "Rune of Power", "if=cooldown.icy_veins.remains=0&buff.rune_of_power.remains<20" );

  default_list -> add_action( this, "Frozen Orb", "if=talent.prismatic_crystal.enabled&cooldown.prismatic_crystal.remains=0" );
  default_list -> add_talent( this, "Prismatic Crystal");
  default_list -> add_action( this, "Icy Veins", "if=(time_to_bloodlust>160&(buff.brain_freeze.react|buff.fingers_of_frost.react))|target.time_to_die<22,moving=0" );

  for( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[i] + ",if=buff.icy_veins.up|target.time_to_die<18" );

  for( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[i] );


  default_list -> add_action( this, "Flamestrike", "if=active_enemies>=5" );
  default_list -> add_talent( this, "Comet Storm" );
  default_list -> add_action( this, "Frozen Orb", "if=!talent.prismatic_crystal.enabled&buff.fingers_of_frost.stack<2" );
  default_list -> add_talent( this, "Frost Bomb", "if=debuff.frost_bomb.remains<cast_time&buff.fingers_of_frost.stack>=1" );
  default_list -> add_action( this, "Frostfire Bolt", "if=!pet.prismatic_crystal.active&buff.brain_freeze.react" );
  default_list -> add_action( this, "Ice Lance", "if=talent.frost_bomb.enabled&buff.fingers_of_frost.react&debuff.frost_bomb.remains>travel_time" );
  default_list -> add_action( this, "Ice Lance", "if=!talent.frost_bomb.enabled&buff.fingers_of_frost.react" );
  default_list -> add_action( this, "Frostfire Bolt", "if=buff.brain_freeze.react" );
  default_list -> add_talent( this, "Ice Nova", "if=(charges=1&recharge_time<3&cooldown.icy_veins.remains>5&cooldown.prismatic_crystal.remains>5)|buff.icy_veins.react|pet.prismatic_crystal.active" );
  default_list -> add_action( this, "Frostbolt" );
  default_list -> add_talent( this, "Ice Floes", "moving=1" );
  default_list -> add_action( this, "Ice Lance", "moving=1" );

}

// Default Action List ===============================================================================================

void mage_t::apl_default()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );

  default_list -> add_action( "Frostfire Bolt" );
}

// mage_t::mana_regen_per_second ============================================

double mage_t::mana_regen_per_second() const
{
  double mp5 = player_t::mana_regen_per_second();

  if ( passives.nether_attunement -> ok() )
    mp5 /= cache.spell_speed();


  return mp5;
}

double mage_t::composite_rating_multiplier( rating_e rating) const
{
  double m = player_t::composite_rating_multiplier( rating );

  switch ( rating )
  {
  case RATING_MULTISTRIKE:
    return m *= 1.0 + spec.ice_shards -> effectN( 1 ).percent();
  case RATING_SPELL_CRIT:
    return m *= 1.0 + spec.incineration -> effectN( 1 ).percent();
  case RATING_MASTERY:
    return m *= 1.0 + spec.arcane_mind -> effectN( 1 ).percent();
  default:
    break;
  }

  return m;

}


// mage_t::composite_player_multipler =======================================

double mage_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.arcane_power -> check() )
  {
    double v = buffs.arcane_power -> value();
    if ( sets.has_set_bonus( SET_CASTER, T14, B4 ) )
    {
      v += 0.1;
    }
    m *= 1.0 + v;
  }


  if ( buffs.rune_of_power -> check() )
  {
    m *= 1.0 + buffs.rune_of_power -> data().effectN( 1 ).percent();
  }

 
  if ( talents.incanters_flow -> ok() )
  {
    m *= 1.0 + ( buffs.incanters_flow -> stack() ) * ( find_spell( 116267 ) -> effectN( 1 ).percent() );
  }
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

  if ( buffs.icy_veins -> up() && glyphs.icy_veins -> ok() )
  {
    ms += buffs.icy_veins -> data().effectN( 3 ).percent();
    if ( perks.improved_icy_veins -> ok() )
      ms += perks.improved_icy_veins -> effectN( 2 ).percent();
  }


  return ms;
}

// mage_t::composite_spell_haste ============================================

double mage_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.icy_veins -> up() && !glyphs.icy_veins -> ok() && perks.improved_icy_veins -> ok() )
    h *= 1.0 / ( 1.0 + buffs.icy_veins -> data().effectN( 1 ).percent() + perks.improved_icy_veins -> effectN( 1 ).percent() );
  else if ( buffs.icy_veins -> up() && !glyphs.icy_veins -> ok() )
    h *= 1.0 / ( 1.0 + buffs.icy_veins -> data().effectN( 1 ).percent() );
  
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

  current_target = target;

  rotation.reset();
  icicles.clear();
  core_event_t::cancel( icicle_event );
  active_bomb_targets = 0;
  last_bomb_target = 0;
  pyro_switch.reset();
}

// mage_t::regen  ===========================================================

void mage_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

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

// mage_t::temporary_movement_modifier ==================================

double mage_t::temporary_movement_modifier() const
{
  double temporary = player_t::temporary_movement_modifier();

  if ( buffs.blazing_speed -> up() )
    temporary = std::max( buffs.blazing_speed -> default_value, temporary );

  if ( buffs.improved_blink -> up() )
    temporary = std::max( buffs.improved_blink -> default_value, temporary );

  return temporary;
}

// mage_t::arise ============================================================

void mage_t::arise()
{
  player_t::arise();

  if ( talents.incanters_flow -> ok() )
    buffs.incanters_flow -> trigger();

  if ( perks.enhanced_frostbolt -> ok() && specialization() == MAGE_FROST )
    buffs.enhanced_frostbolt -> trigger();

  if ( passives.molten_armor -> ok() )
    buffs.molten_armor -> trigger();
  else if ( passives.frost_armor -> ok() )
    buffs.frost_armor -> trigger();
  else if ( passives.mage_armor -> ok() )
    buffs.mage_armor -> trigger();

}

// Copypasta, execept for target selection. This is a massive kludge. Buyer
// beware!

action_t* mage_t::select_action( const action_priority_list_t& list )
{
  player_t* action_target = 0;

  // Mark this action list as visited with the APL internal id
  visited_apls_ |= list.internal_id_mask;

  // Cached copy for recursion, we'll need it if we come back from a
  // call_action_list tree, with nothing to show for it.
  uint64_t _visited = visited_apls_;

  for ( size_t i = 0, num_actions = list.foreground_action_list.size(); i < num_actions; ++i )
  {
    visited_apls_ = _visited;
    action_t* a = list.foreground_action_list[ i ];

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
          action_target = 0;
        }

        // If the called APLs bitflag (based on internal id) is up, we're in an
        // infinite loop, and need to cancel the sim
        if ( visited_apls_ & call -> alist -> internal_id_mask )
        {
          sim -> errorf( "%s action list in infinite loop", name() );
          sim -> cancel();
          return 0;
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
      action_target = 0;
    }
  }

  return 0;
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

  if ( util::str_compare_ci( name_str, "current_target" ) )
  {
    struct current_target_expr_t : public mage_expr_t
    {
      current_target_expr_t( const std::string& n, mage_t& m ) :
        mage_expr_t( n, m )
      { }

      double evaluate()
      {
        return mage.current_target ? static_cast<double>( mage.current_target -> actor_index ) : 0;
      }
    };

    return new current_target_expr_t( name_str, *this );
  }

  if ( util::str_compare_ci( name_str, "default_target" ) )
    return make_ref_expr( name_str, target -> actor_index );

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

      virtual double evaluate()
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


  if ( name_str == "pyro_chain" )
  {
    struct pyro_chain_expr_t : public mage_expr_t
    {
      mage_t* mage;
      pyro_chain_expr_t( mage_t& m ) : mage_expr_t( "pyro_chain", m ), mage( &m )
      {}
      virtual double evaluate()
    { return mage -> pyro_switch.dump_state; }
    };

    return new pyro_chain_expr_t( *this );
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

  // Fallback for string based lookup to "target name" expression. Bad things
  // will happen if you name your target a valid expression in the sim, but who
  // would do that anyhow.
  player_t* p = actor_by_name_str( name_str );
  if ( p )
    return make_ref_expr( name_str, p -> actor_index );

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

icicle_data_t mage_t::get_icicle_object()
{
  if ( icicles.size() == 0 )
    return icicle_data_t( 0, 0 );

  timespan_t threshold = spells.icicles_driver -> duration();

  std::vector< icicle_tuple_t >::iterator idx = icicles.begin(),
                                         end = icicles.end();
  for ( ; idx < end; ++idx )
  {
    if ( sim -> current_time - ( *idx ).first < threshold )
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

  return icicle_data_t( 0, 0 );
}

void mage_t::trigger_icicle( bool chain )
{
  if ( ! spec.icicles -> ok() )
    return;

  if (icicles.size() == 0 )
    return;

  std::pair<double, stats_t*> d;

  if ( chain && ! icicle_event )
  {
    d = get_icicle_object();
    icicle_event = new ( *sim ) events::icicle_event_t( *this, d, true );
  }
  else if ( ! chain )
  {
    d = get_icicle_object();
    icicle -> base_dd_min = icicle -> base_dd_max = d.first;

    actions::icicle_state_t* new_state = debug_cast<actions::icicle_state_t*>( icicle -> get_state() );
    new_state -> source = d.second;
    icicle -> schedule_execute( new_state );
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "%s icicle use%s, damage=%f, total=%u",
                           name(), chain ? " (chained)" : "", d.first,
                           as<unsigned>( icicles.size() ) );
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

} // UNNAMED NAMESPACE

const module_t* module_t::mage()
{
  static mage_module_t m;
  return &m;
}

