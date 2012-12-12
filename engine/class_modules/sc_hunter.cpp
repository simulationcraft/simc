// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

// The purpose of these namespaces is to allow modern IDEs to collapse sections of code.
// Is neither intended nor desired to provide name-uniqueness, hence the global uplift.

namespace spells {}
namespace attacks  {}
namespace pet_actions  {}

using namespace attacks;
using namespace spells;
using namespace pet_actions;

// ==========================================================================
// Hunter
// ==========================================================================

struct hunter_t;

struct hunter_pet_t;

enum aspect_type { ASPECT_NONE=0, ASPECT_HAWK, ASPECT_MAX };

struct hunter_td_t : public actor_pair_t
{
  struct dots_t
  {
    dot_t* serpent_sting;
  } dots;

  hunter_td_t( player_t* target, hunter_t* p );
};

struct hunter_t : public player_t
{
public:
  // Active
  hunter_pet_t* active_pet;
  std::vector<hunter_pet_t*> hunter_main_pets;
  aspect_type   active_aspect;
  action_t*     active_piercing_shots;
  action_t*     active_explosive_ticks;
  action_t*     active_vishanka;
  attack_t*     wild_quiver_shot;

  // Secondary pets
  // need an extra beast for readiness
  pet_t*  pet_dire_beasts[ 2 ];

  // Buffs
  struct buffs_t
  {
    buff_t* aspect_of_the_hawk;
    buff_t* beast_within;
    buff_t* bombardment;
    buff_t* cobra_strikes;
    buff_t* focus_fire;
    buff_t* steady_focus;
    buff_t* lock_and_load;
    buff_t* thrill_of_the_hunt;
    buff_t* master_marksman;
    buff_t* master_marksman_fire;
    buff_t* pre_steady_focus;
    buff_t* rapid_fire;
    // buff_t* trueshot_aura;
    buff_t* tier13_4pc;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* explosive_shot;
    cooldown_t* viper_venom;
    cooldown_t* vishanka;
  } cooldowns;

  // Custom Parameters
  std::string summon_pet_str;

  // Gains
  struct gains_t
  {
    gain_t* invigoration;
    gain_t* fervor;
    gain_t* focus_fire;
    gain_t* rapid_recuperation;
    gain_t* thrill_of_the_hunt;
    gain_t* steady_shot;
    gain_t* steady_focus;
    gain_t* cobra_shot;
    gain_t* dire_beast;
    gain_t* viper_venom;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* invigoration;
    proc_t* thrill_of_the_hunt;
    proc_t* wild_quiver;
    proc_t* lock_and_load;
    proc_t* explosive_shot_focus_starved;
    proc_t* black_arrow_focus_starved;
  } procs;

  // Random Number Generation
  struct rngs_t
  {
    rng_t* frenzy;
    rng_t* invigoration;
  } rngs;

  // Talents
  struct talents_t
  {
    const spell_data_t* posthaste;
    const spell_data_t* narrow_escape;
    const spell_data_t* exhilaration;

    const spell_data_t* silencing_shot;
    const spell_data_t* wyvern_sting;
    const spell_data_t* binding_shot;

    const spell_data_t* crouching_tiger_hidden_chimera;
    const spell_data_t* aspect_of_the_iron_hawk;
    const spell_data_t* spirit_bond;

    const spell_data_t* fervor;
    const spell_data_t* blink_strike;
    const spell_data_t* thrill_of_the_hunt;

    const spell_data_t* a_murder_of_crows;
    const spell_data_t* dire_beast;
    const spell_data_t* lynx_rush;

    const spell_data_t* glaive_toss;
    const spell_data_t* powershot;
    const spell_data_t* barrage;
  } talents;

  // Specialization Spells
  struct specs_t
  {
    // Baseline
    // const spell_data_t* trueshot_aura;

    // Shared
    //const spell_data_t* cobra_shot;

    // Beast Mastery
    // const spell_data_t* kill_command;
    // const spell_data_t* intimidation;
    const spell_data_t* go_for_the_throat;
    const spell_data_t* beast_cleave;
    const spell_data_t* frenzy;
    const spell_data_t* focus_fire;
    // const spell_data_t* bestial_wrath;
    const spell_data_t* cobra_strikes;
    const spell_data_t* the_beast_within;
    const spell_data_t* kindred_spirits;
    const spell_data_t* invigoration;
    const spell_data_t* exotic_beasts;
    //
    // // Marksmanship
    // const spell_data_t* aimed_shot;
    const spell_data_t* careful_aim;
    // const spell_data_t* concussive_barrage;
    const spell_data_t* bombardment;
    const spell_data_t* rapid_recuperation;
    const spell_data_t* master_marksman;
    // const spell_data_t* chimera_shot;
    const spell_data_t* steady_focus;
    const spell_data_t* piercing_shots;
    // // const spell_data_t* wild quiver;
    //
    // // Survival
    const spell_data_t* explosive_shot;
    const spell_data_t* lock_and_load;
    const spell_data_t* improved_serpent_sting;
    // const spell_data_t* black_arrow;
    // const spell_data_t* entrapment;
    const spell_data_t* viper_venom;
    const spell_data_t* trap_mastery;
    const spell_data_t* serpent_spread;
  } specs;

  // Glyphs
  struct glyphs_t
  {
    // Major
    const spell_data_t* aimed_shot;
    const spell_data_t* animal_bond;
    const spell_data_t* black_ice;
    const spell_data_t* camouflage;
    const spell_data_t* chimera_shot;
    const spell_data_t* deterrence;
    const spell_data_t* disengage;
    const spell_data_t* distracting_shot;
    const spell_data_t* endless_wrath;
    const spell_data_t* explosive_trap;
    const spell_data_t* freezing_trap;
    const spell_data_t* ice_trap;
    const spell_data_t* icy_solace;
    const spell_data_t* marked_for_death;
    const spell_data_t* masters_call;
    const spell_data_t* mend_pet;
    const spell_data_t* mending;
    const spell_data_t* mirrored_blades;
    const spell_data_t* misdirection;
    const spell_data_t* no_escape;
    const spell_data_t* pathfinding;
    const spell_data_t* scatter_shot;
    const spell_data_t* scattering;
    const spell_data_t* snake_trap;
    const spell_data_t* tranquilizing_shot;

    // Minor
    const spell_data_t* aspects;
    const spell_data_t* aspect_of_the_pack;
    const spell_data_t* direction;
    const spell_data_t* fetch;
    const spell_data_t* fireworks;
    const spell_data_t* lesser_proportion;
    const spell_data_t* marking;
    const spell_data_t* revive_pet;
    const spell_data_t* stampede;
    const spell_data_t* tame_beast;
    const spell_data_t* the_cheetah;
  } glyphs;

  struct mastery_spells_t
  {
    const spell_data_t* master_of_beasts;
    const spell_data_t* wild_quiver;
    const spell_data_t* essence_of_the_viper;
  } mastery;

  stats_t* stats_stampede;

private:
  target_specific_t<hunter_td_t> target_data;
public:
  double merge_piercing_shots;
  double tier13_4pc_cooldown;
  double pet_multiplier;
  uint32_t vishanka;

  hunter_t( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) :
    player_t( sim, HUNTER, name, r == RACE_NONE ? RACE_NIGHT_ELF : r ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    procs( procs_t() ),
    rngs( rngs_t() ),
    talents( talents_t() ),
    specs( specs_t() ),
    glyphs( glyphs_t() ),
    mastery( mastery_spells_t() )
  {
    target_data.init( "target_data", this );

    // Active
    active_pet             = 0;
    active_aspect          = ASPECT_NONE;
    active_piercing_shots  = 0;
    active_explosive_ticks = 0;
    active_vishanka        = 0;
    wild_quiver_shot       = 0;

    merge_piercing_shots = 0;
    pet_multiplier = 1.0;

    // Cooldowns
    cooldowns.explosive_shot = get_cooldown( "explosive_shot" );
    cooldowns.viper_venom    = get_cooldown( "viper_venom" );
    cooldowns.vishanka       = get_cooldown( "vishanka"       );

    summon_pet_str = "";
    initial.distance = 40;
    base_gcd = timespan_t::from_seconds( 1.0 );

    tier13_4pc_cooldown = 105.0;
    vishanka = 0;
  }

  // Character Definition
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      create_buffs();
  virtual void      init_values();
  virtual void      init_gains();
  virtual void      init_position();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      init_actions();
  virtual void      init_items();
  virtual void      combat_begin();
  virtual void      reset();
  virtual double    composite_attack_power_multiplier();
  virtual double    composite_attack_haste();
  virtual double    ranged_haste_multiplier();
  virtual double    composite_player_multiplier( school_e school, action_t* a = NULL );
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual int       decode_set( item_t& );
  virtual resource_e primary_resource() { return RESOURCE_FOCUS; }
  virtual role_e primary_role() { return ROLE_ATTACK; }
  virtual bool      create_profile( std::string& profile_str, save_e=SAVE_ALL, bool save_html=false );
  virtual void      copy_from( player_t* source );
  virtual void      armory_extensions( const std::string& r, const std::string& s, const std::string& c, cache::behavior_e );
  virtual void      moving();

  virtual hunter_td_t* get_target_data( player_t* target )
  {
    hunter_td_t*& td = target_data[ target ];
    if ( ! td ) td = new hunter_td_t( target, this );
    return td;
  }

  // Event Tracking
  virtual void regen( timespan_t periodicity );

  double careful_aim_crit( player_t* target )
  {
    int threshold = specs.careful_aim -> effectN( 2 ).base_value();
    if ( specs.careful_aim -> ok() && target -> health_percentage() > threshold )
    {
      return specs.careful_aim -> effectN( 1 ).percent();
    }
    return 0.0;
  }

  double beast_multiplier()
  {
    double pm = pet_multiplier;
    if ( mastery.master_of_beasts -> ok() )
      pm *= 1.0 + mastery.master_of_beasts -> effectN( 1 ).mastery_value() * composite_mastery();
    return pm;
  }
};

// ==========================================================================
// Hunter Pet
// ==========================================================================

struct hunter_pet_td_t : public actor_pair_t
{
  buff_t* debuffs_lynx_bleed;

  hunter_pet_td_t( player_t* target, hunter_pet_t* p );

  void init()
  {
    debuffs_lynx_bleed -> init();
  }
};

struct hunter_pet_t : public pet_t
{
public:
  action_t* kill_command;
  action_t* blink_strike;
  action_t* lynx_rush;

  struct specs_t
  {
    // ferocity
    const spell_data_t* rabid;
    const spell_data_t* hearth_of_the_phoenix;
    const spell_data_t* spiked_collar;
    // tenacity
    const spell_data_t* last_stand;
    const spell_data_t* charge;
    const spell_data_t* thunderstomp;
    const spell_data_t* blood_of_the_rhino;
    const spell_data_t* great_stamina;
    // cunning
    const spell_data_t* roar_of_sacrifice;
    const spell_data_t* bullhead;
    const spell_data_t* cornered;
    const spell_data_t* boars_speed;

    const spell_data_t* dash; // ferocity, cunning

    // base for all pets
    const spell_data_t* wild_hunt;
    const spell_data_t* combat_experience;
  } specs;

  // Buffs
  struct buffs_t
  {
    buff_t* bestial_wrath;
    buff_t* frenzy;
    buff_t* rabid;
    buff_t* stampede;
  } buffs;

  // Gains
  struct gains_t
  {
    gain_t* fervor;
    gain_t* focus_fire;
    gain_t* go_for_the_throat;
  } gains;

  // Benefits
  struct benefits_t
  {
    benefit_t* wild_hunt;
  } benefits;

private:
  target_specific_t<hunter_pet_td_t> target_data;
public:
  hunter_pet_t( sim_t* sim, hunter_t* owner, const std::string& pet_name, pet_e pt ) :
    pet_t( sim, owner, pet_name, pt ),
    specs( specs_t() ),
    buffs( buffs_t() ),
    gains( gains_t() ),
    benefits( benefits_t() )
  {
    owner -> hunter_main_pets.push_back( this );

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( owner -> type, owner -> level ) * 0.25;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( owner -> type, owner -> level ) * 0.25;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    stamina_per_owner = 0.45;

    //health_per_stamina *= 1.05; // 3.1.0 change # Cunning, Ferocity and Tenacity pets now all have +5% damage, +5% armor and +5% health bonuses
    initial.armor_multiplier *= 1.05;

    target_data.init( "target_data", this );

    // FIXME work around assert in pet specs
    // Set default specs
    _spec = default_spec();
  }

  hunter_t* cast_owner() const
  { return static_cast<hunter_t*>( owner ); }

  specialization_e default_spec()
  {
    if ( pet_type > PET_NONE          && pet_type < PET_FEROCITY_TYPE ) return PET_FEROCITY;
    if ( pet_type > PET_FEROCITY_TYPE && pet_type < PET_TENACITY_TYPE ) return PET_TENACITY;
    if ( pet_type > PET_TENACITY_TYPE && pet_type < PET_CUNNING_TYPE  ) return PET_CUNNING;
    return PET_FEROCITY;
  }

  virtual const char* unique_special()
  {
    switch ( pet_type )
    {
    case PET_CARRION_BIRD: return "demoralizing_screech";
    case PET_CAT:          return "roar_of_courage";
    case PET_CORE_HOUND:   return NULL;
    case PET_DEVILSAUR:    return "furious_howl/monstrous_bite";
    case PET_HYENA:        return "cackling_howl";
    case PET_MOTH:         return NULL;
    case PET_RAPTOR:       return "tear_armor";
    case PET_SPIRIT_BEAST: return "roar_of_courage";
    case PET_TALLSTRIDER:  return NULL;
    case PET_WASP:         return NULL;
    case PET_WOLF:         return "furious_howl";
    case PET_BEAR:         return NULL;
    case PET_BOAR:         return NULL;
    case PET_CRAB:         return NULL;
    case PET_CROCOLISK:    return NULL;
    case PET_GORILLA:      return NULL;
    case PET_RHINO:        return NULL;
    case PET_SCORPID:      return NULL;
    case PET_SHALE_SPIDER: return NULL;
    case PET_TURTLE:       return NULL;
    case PET_WARP_STALKER: return NULL;
    case PET_WORM:         return NULL;
    case PET_BAT:          return NULL;
    case PET_BIRD_OF_PREY: return NULL;
    case PET_CHIMERA:      return "froststorm_breath";
    case PET_DRAGONHAWK:   return "lightning_breath";
    case PET_NETHER_RAY:   return NULL;
    case PET_RAVAGER:      return "ravage";
    case PET_SERPENT:      return "corrosive_spit";
    case PET_SILITHID:     return "qiraji_fortitude";
    case PET_SPIDER:       return NULL;
    case PET_SPOREBAT:     return NULL;
    case PET_WIND_SERPENT: return "lightning_breath";
    case PET_FOX:          return "tailspin";
    default: break;
    }
    return NULL;
  }

  virtual bool ooc_buffs() { return true; }

  virtual void init_base()
  {
    pet_t::init_base();

    base.attribute[ ATTR_STRENGTH  ] = rating_t::interpolate( level, 0, 162, 331, 476 );
    base.attribute[ ATTR_AGILITY   ] = rating_t::interpolate( level, 0, 54, 113, 438 );
    base.attribute[ ATTR_STAMINA   ] = rating_t::interpolate( level, 0, 307, 361 ); // stamina is different for every pet type
    base.attribute[ ATTR_INTELLECT ] = 100; // FIXME: is 61 at lvl 75. Use /script print(UnitStats("pet",x)); 1=str,2=agi,3=stam,4=int,5=spi
    base.attribute[ ATTR_SPIRIT    ] = 100; // FIXME: is 101 at lvl 75. Values are equal for a cat and a gorilla.

    base.attack_crit = 0.05; // Assume 5% base crit as for most other pets. 19/10/2011

    resources.base[ RESOURCE_HEALTH ] = rating_t::interpolate( level, 0, 4253, 6373 );
    resources.base[ RESOURCE_FOCUS ] = 100 + cast_owner() -> specs.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS );

    base_focus_regen_per_second = 5;  // per Astrylian

    if ( owner -> set_bonus.pvp_4pc_melee() )
      base_focus_regen_per_second *= 1.25;

    base_gcd = timespan_t::from_seconds( 1.20 );

    resources.infinite_resource[ RESOURCE_FOCUS ] = cast_owner() -> resources.infinite_resource[ RESOURCE_FOCUS ];

    world_lag = timespan_t::from_seconds( 0.3 ); // Pet AI latency to get 3.3s claw cooldown as confirmed by Rivkah on EJ, August 2011
    world_lag_override = true;

    owner_coeff.ap_from_ap = 1.0;
    owner_coeff.sp_from_ap = 0.5;
  }

  virtual void create_buffs()
  {
    pet_t::create_buffs();
    buffs.bestial_wrath     = buff_creator_t( this, 19574, "bestial_wrath" );
    buffs.bestial_wrath -> buff_duration += owner -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).time_value();

    buffs.frenzy            = buff_creator_t( this, 19615, "frenzy_effect" ).chance( cast_owner() -> specs.frenzy -> effectN( 2 ).percent() );
    buffs.rabid             = buff_creator_t( this, 53401, "rabid" );

    // Use buff to indicate whether the pet is a stampede summon
    buffs.stampede          = buff_creator_t( this, 130201, "stampede" )
                              .activated( true )
                              /*.quiet( true )*/;
  }

  virtual void init_gains()
  {
    pet_t::init_gains();

    gains.fervor            = get_gain( "fervor"            );
    gains.focus_fire        = get_gain( "focus_fire"        );
    gains.go_for_the_throat = get_gain( "go_for_the_throat" );
  }

  virtual void init_benefits()
  {
    pet_t::init_benefits();

    benefits.wild_hunt = get_benefit( "wild_hunt" );
  }

  virtual void init_actions()
  {
    if ( action_list_str.empty() )
    {
      action_list_str += "/auto_attack";
      action_list_str += "/snapshot_stats";

      // Comment out to avoid procs
      if ( specs.rabid -> ok() )
        action_list_str += "/rabid";

      const char* special = unique_special();
      if ( special )
      {
        action_list_str += "/";
        action_list_str += special;
      }
      action_list_str += "/claw";
      action_list_str += "/wait_until_ready";
      action_list_default = 1;
    }

    pet_t::init_actions();
  }

  virtual double composite_attack_power_multiplier()
  {
    double mult = pet_t::composite_attack_power_multiplier();

    // TODO pet charge should show up here.

    return mult;
  }

  virtual double composite_attack_crit( weapon_t* /* w */ )
  {
    double ac = pet_t::composite_attack_crit();
    ac += specs.spiked_collar -> effectN( 3 ).percent();
    return ac;
  }

  double focus_regen_per_second()
  {
    // pet focus regen seems to be based solely off the regen multiplier form the owner
    double r = base_focus_regen_per_second * ( 1.0 / cast_owner() -> composite_attack_haste() );
    return r;
  }

  virtual double composite_attack_haste()
  {
    double ah = pet_t::composite_attack_haste();
    // remove the portions of speed that were ranged only.
    ah /= cast_owner() -> ranged_haste_multiplier();
    return ah;
  }

  virtual double composite_attack_speed()
  {
    double ah = pet_t::composite_attack_speed();
    // remove the portions of speed that were ranged only.
    ah /= cast_owner() -> ranged_haste_multiplier();
    ah *= 1.0 / ( 1.0 + cast_owner() -> specs.frenzy -> effectN( 1 ).percent() * buffs.frenzy -> stack() );
    ah *= 1.0 / ( 1.0 + specs.spiked_collar -> effectN( 2 ).percent() );
    if ( buffs.rabid -> up() )
      ah *= 1.0 / ( 1.0 + buffs.rabid -> data().effectN( 1 ).percent() );

    return ah;
  }

  virtual void summon( timespan_t duration=timespan_t::zero() )
  {
    pet_t::summon( duration );

    cast_owner() -> active_pet = this;
  }

  void stampede_summon( timespan_t duration )
  {
    if ( this == cast_owner() -> active_pet )
    {
      // The active pet does not get its damage reduced
      //buffs.stampede -> trigger( 1, -1.0, 1.0, duration );
      return;
    }

    for ( size_t i = 0; i < stats_list.size(); ++i )
      if ( ! ( stats_list[ i ] -> parent ) )
        cast_owner() -> stats_stampede -> add_child( stats_list[ i ] );

    pet_t::summon( duration );

    buffs.stampede -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );
  }

  virtual void demise()
  {
    pet_t::demise();

    if ( cast_owner() -> active_pet == this )
      cast_owner() -> active_pet = 0;
  }

  virtual double composite_player_multiplier( school_e school, action_t* a )
  {
    double m = pet_t::composite_player_multiplier( school, a );

    m *= cast_owner() -> beast_multiplier();

    if ( buffs.bestial_wrath -> up() )
      m *= 1.0 + buffs.bestial_wrath -> data().effectN( 2 ).percent();

    // Pet combat experience
    m *= 1.0 + specs.combat_experience -> effectN( 2 ).percent();

    if ( buffs.stampede -> up() )
      m *= 1.0 + buffs.stampede -> data().effectN( 1 ).percent();

    return m;
  }

  virtual hunter_pet_td_t* get_target_data( player_t* target )
  {
    hunter_pet_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new hunter_pet_td_t( target, this );
      td -> init();
    }
    return td;
  }

  virtual resource_e primary_resource() { return RESOURCE_FOCUS; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str );

  virtual void init_spells();
};

// Template for common hunter action code. See priest_action_t.
template <class Base>
struct hunter_action_t : public Base
{
  typedef Base ab;
  typedef hunter_action_t base_t;

  hunter_action_t( const std::string& n, hunter_t* player,
                   const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
  { }

  hunter_t* p() const { return static_cast<hunter_t*>( ab::player ); }

  hunter_td_t* cast_td( player_t* t = 0 ) { return p() -> get_target_data( t ? t : ab::target ); }

  virtual double cost()
  {
    double c = ab::cost();

    if ( c == 0 )
      return 0;

    if ( p() -> buffs.beast_within -> check() )
      c *= ( 1.0 + p() -> buffs.beast_within -> data().effectN( 1 ).percent() );

    return c;
  }

  // thrill_of_the_hunt support ===============================================

  void trigger_thrill_of_the_hunt()
  {
    if ( p() -> talents.thrill_of_the_hunt -> ok() && cost() > 0 )
      if ( p() -> buffs.thrill_of_the_hunt -> trigger() )
        p() -> procs.thrill_of_the_hunt -> occur();
  }

  double thrill_discount( double cost )
  {
    double result = cost;

    if ( p() -> buffs.thrill_of_the_hunt -> check() )
      result += p() -> buffs.thrill_of_the_hunt -> data().effectN( 1 ).base_value();

    return result;
  }

  void consume_thrill_of_the_hunt()
  {
    if ( p() -> buffs.thrill_of_the_hunt -> up() )
    {
      double cost = hunter_action_t::cost();
      p() -> resource_gain( RESOURCE_FOCUS, cost, p() -> gains.thrill_of_the_hunt );
      p() -> buffs.thrill_of_the_hunt -> decrement();
    }
  }
};

namespace attacks {

// ==========================================================================
// Hunter Attack
// ==========================================================================

struct hunter_ranged_attack_t : public hunter_action_t<ranged_attack_t>
{
  rng_t* wild_quiver;
  bool can_trigger_wild_quiver;

  hunter_ranged_attack_t( const std::string& n, hunter_t* player,
                          const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s ),
    wild_quiver ( 0 ),
    can_trigger_wild_quiver( true )
  {
    special                = true;
    may_crit               = true;
    may_parry			   = false;
    may_block			   = false;
    tick_may_crit          = true;
    normalize_weapon_speed = true;
    dot_behavior           = DOT_REFRESH;
  }

  virtual void init()
  {
    if ( player -> main_hand_weapon.group() != WEAPON_RANGED )
    {
      background = true;
    }

    wild_quiver = p() -> get_rng( "wild_quiver" );

    base_t::init();
  }

  virtual void trigger_steady_focus()
  {
    // Most ranged attacks reset the counter for two steady shots in a row
    p() -> buffs.pre_steady_focus -> expire();
  }

  virtual void trigger_wild_quiver( double multiplier = 1.0 )
  {
    if ( p() -> wild_quiver_shot == 0 )
      return;

    if ( ! can_trigger_wild_quiver )
      return;

    double chance = multiplier * p() -> composite_mastery() * p() -> mastery.wild_quiver -> effectN( 1 ).coeff() / 100.0;
    if ( wild_quiver -> roll( chance ) )
    {
      p() -> wild_quiver_shot -> execute();
      p() -> procs.wild_quiver -> occur();
    }
  }

  virtual void add_scope()
  {
    if ( player -> items[ weapon -> slot ].encoded_enchant_str == "scope" )
    {
      double scope_damage = util::ability_rank( player -> level, 15.0,72,  12.0,67,  7.0,0 );

      base_dd_min += scope_damage;
      base_dd_max += scope_damage;
    }
  }

  virtual void execute();

  virtual timespan_t execute_time()
  {
    timespan_t t = base_t::execute_time();

    if ( t == timespan_t::zero()  || base_execute_time < timespan_t::zero() )
      return timespan_t::zero();

    return t;
  }
};

// trigger_go_for_the_throat ================================================

void trigger_go_for_the_throat( hunter_ranged_attack_t* a )
{
  if ( ! a -> p() -> specs.go_for_the_throat -> ok() )
    return;

  if ( ! a -> p() -> active_pet )
    return;

  int gain = a -> p() -> specs.go_for_the_throat -> effectN( 1 ).trigger() -> effectN( 1 ).base_value();
  a -> p() -> active_pet -> resource_gain( RESOURCE_FOCUS, gain, a -> p() -> active_pet -> gains.go_for_the_throat );
}

// trigger_piercing_shots ===================================================

struct piercing_shots_t : public ignite::pct_based_action_t< attack_t >
{
  piercing_shots_t( hunter_t* p ) :
    base_t( "piercing_shots", p, p -> find_spell( 63468 ) )
  {
    // school = SCHOOL_BLEED;
  }
};

// Hunter Piercing Shots template specialization
template <class HUNTER_ACTION>
void trigger_piercing_shots( HUNTER_ACTION* s, player_t* t, double dmg )
{
  hunter_t* p = s -> p();
  if ( ! p -> specs.piercing_shots -> ok() ) return;

  ignite::trigger_pct_based(
    p -> active_piercing_shots, // ignite spell
    t, // target
    p -> specs.piercing_shots -> effectN( 1 ).percent() * dmg ); // dw damage
}

// trigger_vishanka =========================================================

struct vishanka_t : public hunter_ranged_attack_t
{
  vishanka_t( hunter_t* p, uint32_t id ) :
    hunter_ranged_attack_t( "vishanka_jaws_of_the_earth", p, p -> find_spell( id ) )
  {
    background  = true;
    proc        = true;
    trigger_gcd = timespan_t::zero();
    init();

    // FIX ME: Can this crit, miss, etc?
    may_miss    = false;
  }
};

static void trigger_vishanka( hunter_ranged_attack_t* a )
{
  if ( ! a -> p() -> vishanka )
    return;

  if ( a -> p() -> cooldowns.vishanka -> down() )
    return;

  assert( a -> p() -> active_vishanka );

  if ( a -> sim -> roll( a -> p() -> dbc.spell( a -> p() -> vishanka ) -> proc_chance() ) )
  {
    a -> p() -> active_vishanka -> execute();
    a -> p() -> cooldowns.vishanka -> duration = timespan_t::from_seconds( 15.0 ); // Assume a ICD until testing proves one way or another
    a -> p() -> cooldowns.vishanka -> start();
  }
}

void hunter_ranged_attack_t::execute()
{
  ranged_attack_t::execute();

  if ( p() -> specs.steady_focus -> ok() )
    trigger_steady_focus();

  if ( p() -> buffs.pre_steady_focus -> stack() == 2 )
  {
    double haste_buff = p() -> buffs.steady_focus -> data().effectN( 1 ).percent();
    haste_buff += p() -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 3 ).percent();

    p() -> buffs.steady_focus -> trigger( 1, haste_buff );
    p() -> buffs.pre_steady_focus -> expire();
  }

  trigger_thrill_of_the_hunt();

  if ( result_is_hit( execute_state -> result ) )
    trigger_wild_quiver();

  if ( result_is_hit( execute_state -> result ) )
    trigger_vishanka( this );
}

// Ranged Attack ============================================================

struct ranged_t : public hunter_ranged_attack_t
{
  ranged_t( hunter_t* player, const char* name="ranged", const spell_data_t* s = spell_data_t::nil() ) :
    hunter_ranged_attack_t( name, player, s )
  {
    school = SCHOOL_PHYSICAL;
    weapon = &( player -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;

    normalize_weapon_speed = false;
    background  = true;
    repeating   = true;
    special     = false;
  }

  virtual timespan_t execute_time()
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return hunter_ranged_attack_t::execute_time();
  }

  virtual void trigger_steady_focus()
  { }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( s -> result == RESULT_CRIT )
        trigger_go_for_the_throat( this );
    }
  }
};

// Auto Shot ================================================================

struct auto_shot_t : public ranged_t
{
  auto_shot_t( hunter_t* p ) : ranged_t( p, "auto_shot", p -> find_class_spell( "Auto Shot" ) )
  {
  }
};

struct start_attack_t : public hunter_ranged_attack_t
{
  start_attack_t( hunter_t* p, const std::string& options_str ) :
    hunter_ranged_attack_t( "start_auto_shot", p, p -> find_class_spell( "Auto Shot" ) )
  {
    parse_options( NULL, options_str );

    p -> main_hand_attack = new auto_shot_t( p );
    stats = p -> main_hand_attack -> stats;

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    p() -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    return( player -> main_hand_attack -> execute_event == 0 ); // not swinging
  }

  virtual timespan_t execute_time()
  {
    double h = 1.0;

    h *= 1.0 / ( 1.0 + sim -> auras.attack_haste -> value() );

    return hunter_ranged_attack_t::execute_time() * h;
  }
};

// Aimed Shot ===============================================================

struct aimed_shot_t : public hunter_ranged_attack_t
{

  // Aimed Shot - Master Marksman ===========================================

  struct aimed_shot_mm_t : public hunter_ranged_attack_t
  {
    aimed_shot_mm_t( hunter_t* p ) :
      hunter_ranged_attack_t( "aimed_shot_mm", p, p -> find_spell( 82928 ) )
    {
      check_spec ( HUNTER_MARKSMANSHIP );

      // Don't know why these values aren't 0 in the database.
      base_execute_time = timespan_t::zero();

      normalize_weapon_speed = true;
    }

    virtual double composite_target_crit( player_t* t )
    {
      double cc = hunter_ranged_attack_t::composite_target_crit( t );
      cc += p() -> careful_aim_crit( t );
      return cc;
    }

    virtual void execute()
    {
      hunter_ranged_attack_t::execute();

      p() -> buffs.master_marksman_fire -> expire();
    }

    virtual void impact( action_state_t* s )
    {
      hunter_ranged_attack_t::impact( s );

      if ( s -> result == RESULT_CRIT )
        trigger_piercing_shots( this, s -> target, s -> result_amount );
    }
  };

  aimed_shot_mm_t* as_mm;

  aimed_shot_t( hunter_t* p, const std::string& options_str ) :
    hunter_ranged_attack_t( "aimed_shot", p, p -> find_class_spell( "Aimed Shot" ) ),
    as_mm( 0 )
  {
    check_spec ( HUNTER_MARKSMANSHIP );
    parse_options( NULL, options_str );

    normalize_weapon_speed = true;

    as_mm = new aimed_shot_mm_t( p );
    as_mm -> background = true;
  }

  bool master_marksman_check()
  {
    return p() -> buffs.master_marksman_fire -> check() != 0;
  }

  virtual double cost()
  {
    // NOTE that master_marksman_fire is now specified to reduce the time and cost by a percentage, though the current number is 100%
    if ( master_marksman_check() )
      return 0;

    return hunter_ranged_attack_t::cost();
  }

  virtual double composite_target_crit( player_t* t )
  {
    double cc = hunter_ranged_attack_t::composite_target_crit( t );
    cc += p() -> careful_aim_crit( t );
    return cc;
  }

  virtual timespan_t execute_time()
  {
    // NOTE that master_marksman_fire is now specified to reduce the time and cost by a percentage, though the current number is 100%
    if ( master_marksman_check() )
      return timespan_t::zero();

    return hunter_ranged_attack_t::execute_time();
  }

  virtual void schedule_execute()
  {
    hunter_ranged_attack_t::schedule_execute();

    if ( time_to_execute > timespan_t::zero() )
    {
      if ( p() -> main_hand_attack ) p() -> main_hand_attack -> cancel();
    }
  }

  virtual void execute()
  {
    if ( master_marksman_check() )
      as_mm -> execute();
    else
      hunter_ranged_attack_t::execute();
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( s -> result == RESULT_CRIT )
      trigger_piercing_shots( this, s -> target, s -> result_amount );
  }
};

// Arcane Shot Attack =======================================================

struct arcane_shot_t : public hunter_ranged_attack_t
{
  arcane_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "arcane_shot", player, player -> find_class_spell( "Arcane Shot" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual double cost()
  {
    return thrill_discount( ab::cost() );
  }

  virtual void execute()
  {
    hunter_ranged_attack_t::execute();
    consume_thrill_of_the_hunt();
  }

  virtual void impact( action_state_t* state )
  {
    hunter_ranged_attack_t::impact( state );
    if ( result_is_hit( state -> result ) )
    {
      p() -> buffs.cobra_strikes -> trigger( 2 );

      // Needs testing
      p() -> buffs.tier13_4pc -> trigger();
    }
  }
};

// Glaive Toss Attack =======================================================

struct glaive_toss_strike_t : public ranged_attack_t
{
  // use ranged_attack_to to avoid triggering other hunter behaviors (like thrill of the hunt
  // TotH should be triggered by the glaive toss itself.
  glaive_toss_strike_t( hunter_t* player ) : ranged_attack_t( "glaive_toss_strike", player, player -> find_spell( 120761 ) )
  {
    repeating   = false;
    normalize_weapon_speed = true;
    background = true;
    dual = true;
    may_crit = true;    special = true;
    travel_speed = player -> talents.glaive_toss -> effectN( 3 ).trigger() -> missile_speed();

    // any attack with the weapon may trigger wild_quiver, but don't actually include weapon damage
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier = 0;

    direct_power_mod = data().extra_coeff();

    // This is the strike against the main target
    base_multiplier *= player -> talents.glaive_toss -> effectN( 1 ).base_value();

    // FIXME I think that the glaive is supposed to hit each secondary target twice but for half damage
    // Thus each target will get the same amount of damage, however wild quiver and other effects
    // won't proc quite enough.
    aoe = -1;
  }
};

struct glaive_toss_t : public hunter_ranged_attack_t
{
  glaive_toss_strike_t* primary_strike;

  glaive_toss_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "glaive_toss", player, player -> talents.glaive_toss )
  {
    parse_options( NULL, options_str );
    may_miss = false;
    may_crit = false;
    school = SCHOOL_PHYSICAL;

    primary_strike = new glaive_toss_strike_t( p() );
    //add_child( primary_strike );
    primary_strike -> stats = stats;
    // FIXME add different attacks and stats for each glaive
    // suppress ticks, since they are only for the slow effect
    num_ticks = 0;
  }

  virtual void execute()
  {
    hunter_ranged_attack_t::execute();

    primary_strike -> execute();
    primary_strike -> execute();
  }

  void impact( action_state_t* /* s */ )
  {
    // No-op because executes the pair of glaive strikes explicitly
  }
};

// Powershot Attack =======================================================

struct powershot_t : public hunter_ranged_attack_t
{
  powershot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "powershot", player, player -> find_talent_spell( "Powershot" ) )
  {
    parse_options( NULL, options_str );

    aoe = -1;
    // based on tooltip
    base_aoe_multiplier *= 0.5;

    can_trigger_wild_quiver = false;
  }

  virtual double action_multiplier()
  {
    double am = ab::action_multiplier();

    // for primary target
    am *= 2.0;  // from the tooltip

    return am;
  }
};

// Black Arrow ==============================================================

struct black_arrow_t : public hunter_ranged_attack_t
{
  black_arrow_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "black_arrow", player, player -> find_class_spell( "Black Arrow" ) )
  {
    parse_options( NULL, options_str );

    cooldown = p() -> get_cooldown( "traps" );
    cooldown -> duration = data().cooldown();

    cooldown -> duration += p() -> specs.trap_mastery -> effectN( 4 ).time_value();
    base_multiplier *= 1.0 + p() -> specs.trap_mastery -> effectN( 2 ).percent();

    tick_power_mod = data().extra_coeff();
  }

  virtual bool ready()
  {
    if ( cooldown -> up() && ! p() -> resource_available( RESOURCE_FOCUS, cost() ) )
    {
      if ( sim -> log ) sim -> output( "Player %s was focus starved when Black Arrow was ready.", p() -> name() );
      p() -> procs.black_arrow_focus_starved -> occur();
    }

    return hunter_ranged_attack_t::ready();
  }

  virtual void tick( dot_t* d )
  {
    hunter_ranged_attack_t::tick( d );

    if ( p() -> buffs.lock_and_load -> trigger( 2 ) )
    {
      p() -> procs.lock_and_load -> occur();
      p() -> cooldowns.explosive_shot -> reset( true );
    }
  }
};

// Explosive Trap ===========================================================

struct explosive_trap_effect_t : public hunter_ranged_attack_t
{
  explosive_trap_effect_t( hunter_t* player )
    : hunter_ranged_attack_t( "explosive_trap", player, player -> find_spell( 13812 ) )
  {
    aoe = -1;
    background = true;
    tick_power_mod = data().extra_coeff();

    cooldown -> duration += p() -> specs.trap_mastery -> effectN( 4 ).time_value();
    base_multiplier *= 1.0 + p() -> specs.trap_mastery -> effectN( 2 ).percent();

    may_miss = false;
    may_crit = false;
    tick_may_crit = true;
  }

  virtual void tick( dot_t* d )
  {
    hunter_ranged_attack_t::tick( d );

    if ( p() -> buffs.lock_and_load -> trigger( 2 ) )
    {
      p() -> procs.lock_and_load -> occur();
      p() -> cooldowns.explosive_shot -> reset( true );
    }
  }
};

struct explosive_trap_t : public hunter_ranged_attack_t
{
  attack_t* trap_effect;
  int trap_launcher;

  explosive_trap_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "explosive_trap", player, player -> find_spell( 13813 ) ), trap_effect( 0 ),
    trap_launcher( 0 )
  {
    option_t options[] =
    {
      // Launched traps have a focus cost
      opt_bool( "trap_launcher", trap_launcher ),
      opt_null()
    };
    parse_options( options, options_str );

    //TODO: Split traps cooldown into fire/frost/snakes
    cooldown = p() -> get_cooldown( "traps" );
    cooldown -> duration = data().cooldown();

    may_miss = false;

    trap_effect = new explosive_trap_effect_t( p() );
    add_child( trap_effect );
  }

  virtual void execute()
  {
    hunter_ranged_attack_t::execute();

    trap_effect -> execute();
  }
};

// Chimera Shot =============================================================

struct chimera_shot_t : public hunter_ranged_attack_t
{
  chimera_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "chimera_shot", player, player -> find_class_spell( "Chimera Shot" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual double action_multiplier()
  {
    double am = ab::action_multiplier();
    am *= 1.0 + p() -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 2 ).percent();
    return am;
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      cast_td( s -> target ) -> dots.serpent_sting -> refresh_duration();

      if ( s -> result == RESULT_CRIT )
        trigger_piercing_shots( this, s -> target, s -> result_amount );

    }
  }
};

// Cobra Shot Attack ========================================================

struct cobra_shot_t : public hunter_ranged_attack_t
{
  double focus_gain;

  cobra_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "cobra_shot", player, player -> find_class_spell( "Cobra Shot" ) )
  {
    parse_options( NULL, options_str );

    focus_gain = p() -> dbc.spell( 91954 ) -> effectN( 1 ).base_value();

    // Needs testing
    if ( p() -> set_bonus.tier13_2pc_melee() )
      focus_gain *= 2.0;
  }

  virtual bool usable_moving()
  {
    return true;
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      cast_td( s -> target ) -> dots.serpent_sting -> extend_duration_seconds( data().effectN( 3 ).time_value() );

      double focus = focus_gain;
      p() -> resource_gain( RESOURCE_FOCUS, focus, p() -> gains.cobra_shot );
    }
  }
};

// Explosive Shot ===========================================================

struct explosive_shot_tick_t : public ignite::pct_based_action_t< attack_t >
{
  explosive_shot_tick_t( hunter_t* p ) :
    base_t( "explosive_shot_tick", p, p -> specs.explosive_shot )
  {
    tick_may_crit = true;
    dual = true;

    // suppress direct damage in the dot.
    base_dd_min = base_dd_max = 0;
  }

  virtual void assess_damage( dmg_e type, action_state_t* s )
  {
    if ( type == DMG_DIRECT )
      return;

    base_t::assess_damage( type, s );
  }

  void init()
  {
    base_t::init();

    snapshot_flags |= STATE_CRIT | STATE_TGT_CRIT;
  }
};

struct explosive_shot_t : public hunter_ranged_attack_t
{
  explosive_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "explosive_shot", player, player -> specs.explosive_shot )
  {
    parse_options( NULL, options_str );
    may_block = false;

    tick_power_mod = player -> specs.explosive_shot -> effectN( 3 ).base_value() / 1000.0;
    direct_power_mod = tick_power_mod;

    // the inital impact is not part of the rolling dot
    num_ticks = 0;

  }

  void init()
  {
    hunter_ranged_attack_t::init();

    assert( p() -> active_explosive_ticks );

    p() -> active_explosive_ticks -> stats = stats;

    stats -> action_list.push_back( p() -> active_explosive_ticks );
  }

  virtual double cost()
  {
    if ( p() -> buffs.lock_and_load -> check() )
      return 0;

    return hunter_ranged_attack_t::cost();
  }

  virtual bool ready()
  {
    if ( cooldown -> up() && ! p() -> resource_available( RESOURCE_FOCUS, cost() ) )
    {
      if ( sim -> log ) sim -> output( "Player %s was focus starved when Explosive Shot was ready.", p() -> name() );
      p() -> procs.explosive_shot_focus_starved -> occur();
    }

    return hunter_ranged_attack_t::ready();
  }

  virtual void update_ready()
  {
    cooldown -> duration = ( p() -> buffs.lock_and_load -> check() ? timespan_t::zero() : data().cooldown() );

    hunter_ranged_attack_t::update_ready();
  }

  virtual double action_multiplier()
  {
    double am = ab::action_multiplier();
    am *= 1.0 + p() -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 3 ).percent();
    return am;
  }

  virtual void execute()
  {
    hunter_ranged_attack_t::execute();

    p() -> buffs.lock_and_load -> up();
    p() -> buffs.lock_and_load -> decrement();
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      double damage = calculate_tick_damage( RESULT_HIT, s -> composite_power(), s -> composite_ta_multiplier(), s -> target );
      ignite::trigger_pct_based(
        p() -> active_explosive_ticks, // ignite spell
        s -> target,                   // target
        damage * 2 );  // 2 ticks left of the dot.
    }
  }
};

// Kill Shot ================================================================

struct kill_shot_t : public hunter_ranged_attack_t
{
  cooldown_t* cd_glyph_kill_shot;

  kill_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "kill_shot", player, player -> find_class_spell( "Kill Shot" ) )
  {
    parse_options( NULL, options_str );

    base_dd_min *= weapon_multiplier; // Kill Shot's weapon multiplier applies to the base damage as well
    base_dd_max *= weapon_multiplier;

    cd_glyph_kill_shot = player -> get_cooldown( "cooldowns.glyph_kill_shot" );
    cd_glyph_kill_shot -> duration = player -> dbc.spell( 90967 ) -> duration();

    normalize_weapon_speed = true;
  }

  virtual void execute()
  {
    hunter_ranged_attack_t::execute();

    if ( cd_glyph_kill_shot -> up() )
    {
      cooldown -> reset();
      cd_glyph_kill_shot -> start();
    }
  }

  virtual bool ready()
  {
    if ( target -> health_percentage() > 20 )
      return false;

    return hunter_ranged_attack_t::ready();
  }
};

// Scatter Shot Attack ======================================================

struct scatter_shot_t : public hunter_ranged_attack_t
{
  scatter_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "scatter_shot", player, player -> find_class_spell( "Scatter Shot" ) )
  {
    parse_options( NULL, options_str );

    normalize_weapon_speed = true;
  }
};

// Serpent Sting Attack =====================================================

// FIXME does this extra dmg count as a ranged attack for purposes of thrill, trinket procs, etc.
struct serpent_sting_burst_t : public hunter_ranged_attack_t
{
  serpent_sting_burst_t( hunter_t* player ) :
    hunter_ranged_attack_t( "improved_serpent_sting", player, player -> find_specialization_spell( "Improved Serpent Sting" ) )
  {
    school = SCHOOL_NATURE;
    proc       = true;
    background = true;
  }
};

struct serpent_sting_t : public hunter_ranged_attack_t
{
  serpent_sting_burst_t* serpent_sting_burst;

  serpent_sting_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "serpent_sting", player, player -> find_class_spell( "Serpent Sting" ) )
  {
    parse_options( NULL, options_str );

    const spell_data_t* scaling_data = data().effectN( 1 ).trigger(); // id 118253
    parse_effect_data( scaling_data -> effectN( 1 ) );
    tick_power_mod = scaling_data -> extra_coeff();

    base_td_multiplier *= 1.0 + player -> specs.improved_serpent_sting -> effectN( 2 ).percent();

    may_crit  = false;

    if ( player -> specs.improved_serpent_sting -> ok() )
    {
      serpent_sting_burst = new serpent_sting_burst_t( player );
      add_child( serpent_sting_burst );
    }
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) && serpent_sting_burst && p() -> specs.improved_serpent_sting -> ok() )
    {
      double tick_damage = calculate_tick_damage( RESULT_HIT, s -> composite_power(), s -> composite_ta_multiplier(), s -> target );
      double t = tick_damage * hasted_num_ticks( execute_state -> haste ) * p() -> specs.improved_serpent_sting -> effectN( 1 ).percent();

      serpent_sting_burst -> base_dd_min = t;
      serpent_sting_burst -> base_dd_max = t;
      serpent_sting_burst -> execute();
    }
  }

  virtual void tick( dot_t* d )
  {
    hunter_ranged_attack_t::tick( d );

    if ( p() -> specs.viper_venom -> ok() && p() -> cooldowns.viper_venom -> up() )
    {
      double focus_gain = p() -> specs.viper_venom -> effectN( 1 ).trigger() -> effectN( 1 ).base_value();
      p() -> resource_gain( RESOURCE_FOCUS, focus_gain, p() -> gains.viper_venom );
      p() -> cooldowns.viper_venom -> start();
    }
  }
};

struct serpent_sting_spread_t : public serpent_sting_t
{
  serpent_sting_spread_t( hunter_t* player, const std::string& options_str ) :
    serpent_sting_t( player, options_str )
  {
    travel_speed = 0.0;
    background = true;
    aoe = -1;
    if ( serpent_sting_burst )
      serpent_sting_burst -> aoe = -1;
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( serpent_sting_burst && p() -> specs.improved_serpent_sting -> ok() )
    {
      double tick_damage = calculate_tick_damage( RESULT_HIT, s -> composite_power(), s -> composite_ta_multiplier(), s -> target );
      double t = tick_damage * hasted_num_ticks( execute_state -> haste ) * p() -> specs.improved_serpent_sting -> effectN( 1 ).percent();

      serpent_sting_burst -> base_dd_min = t;
      serpent_sting_burst -> base_dd_max = t;
      serpent_sting_burst -> execute();
    }
  }
};

// Multi Shot Attack ========================================================

struct multi_shot_t : public hunter_ranged_attack_t
{
  serpent_sting_spread_t* spread_sting;

  multi_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "multi_shot", player, player -> find_class_spell( "Multi-Shot" ) ),
    spread_sting( 0 )
  {
    assert( p() -> main_hand_weapon.type != WEAPON_NONE );
    parse_options( NULL, options_str );

    aoe = -1;

    if ( p() -> specs.serpent_spread -> ok() )
      spread_sting = new serpent_sting_spread_t( player, options_str );
  }

  virtual double cost()
  {
    double cost = hunter_ranged_attack_t::cost();

    cost = thrill_discount( cost );

    if ( p() -> buffs.bombardment -> check() )
      cost += 1 + p() -> buffs.bombardment -> data().effectN( 1 ).base_value();

    return cost;
  }

  virtual double action_multiplier()
  {
    double am = hunter_ranged_attack_t::action_multiplier();
    if ( p() -> buffs.bombardment -> up() )
      am *= 1 + p() -> buffs.bombardment -> data().effectN( 2 ).percent();
    return am;
  }

  virtual void execute()
  {
    hunter_ranged_attack_t::execute();
    consume_thrill_of_the_hunt();
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( spread_sting )
        spread_sting -> execute();
      if ( s -> result == RESULT_CRIT && p() -> specs.bombardment -> ok() )
        p() -> buffs.bombardment -> trigger();

      // TODO determine multishot adds in execute.
      /*for ( int i=0; i < q -> adds_nearby; i++ ) {
        // Calculate a result for each nearby add to determine whether to proc
        // bombardment
        int delta_level = q -> level - p() -> level;
        if ( sim -> real() <= crit_chance( delta_level ) )
          crit_occurred++;
      }*/
    }
  }
};

// Silencing Shot Attack ====================================================

struct silencing_shot_t : public hunter_ranged_attack_t
{
  silencing_shot_t( hunter_t* player, const std::string& /* options_str */ ) :
    hunter_ranged_attack_t( "silencing_shot", player, player -> find_class_spell( "Silencing Shot" ) )
  {
    weapon_multiplier = 0.0;

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }
};

// Steady Shot Attack =======================================================

struct steady_shot_t : public hunter_ranged_attack_t
{
  double focus_gain;
  double steady_focus_gain;

  steady_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "steady_shot", player, player -> find_class_spell( "Steady Shot" ) )
  {
    parse_options( NULL, options_str );

    focus_gain = p() -> dbc.spell( 77443 ) -> effectN( 1 ).base_value();
    steady_focus_gain = p() -> buffs.steady_focus -> data().effectN( 2 ).base_value();

    // Needs testing
    if ( p() -> set_bonus.tier13_2pc_melee() )
      focus_gain *= 2.0;
  }

  virtual void trigger_steady_focus()
  {
    p() -> buffs.pre_steady_focus -> trigger( 1 );
  }

  virtual void execute()
  {
    hunter_ranged_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> resource_gain( RESOURCE_FOCUS, focus_gain, p() -> gains.steady_shot );
      if ( p() -> buffs.steady_focus -> up() )
        p() -> resource_gain( RESOURCE_FOCUS, steady_focus_gain, p() -> gains.steady_focus );

      if ( ! p() -> buffs.master_marksman_fire -> up() && p() -> buffs.master_marksman -> trigger() )
      {
        if ( p() -> buffs.master_marksman -> stack() == p() -> buffs.master_marksman -> max_stack() )
        {
          p() -> buffs.master_marksman_fire -> trigger();
          p() -> buffs.master_marksman -> expire();
        }
      }
    }
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( s -> result == RESULT_CRIT )
      trigger_piercing_shots( this, s -> target, s -> result_amount );
  }

  virtual bool usable_moving()
  {
    return true;
  }

  virtual double composite_target_crit( player_t* t )
  {
    double cc = hunter_ranged_attack_t::composite_target_crit( t );
    cc += p() -> careful_aim_crit( t );
    return cc;
  }
};

// Wild Quiver ==============================================================

struct wild_quiver_shot_t : public ranged_t
{
  wild_quiver_shot_t( hunter_t* p ) : ranged_t( p, "wild_quiver_shot", p -> find_spell( 76663 ) )
  {
    repeating   = false;
    proc = true;
    normalize_weapon_speed = true;
  }

  virtual void trigger_wild_quiver( double /*multiplier = 1.0*/ )
  {
    // suppress recursive wild quiver
  }
};

} // end attacks

// ==========================================================================
// Hunter Spells
// ==========================================================================

namespace spells {
struct hunter_spell_t : public hunter_action_t<spell_t>
{
public:
  hunter_spell_t( const std::string& n, hunter_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s )
  {
  }

  virtual timespan_t gcd()
  {
    if ( ! harmful && ! player -> in_combat )
      return timespan_t::zero();

    // Hunter gcd unaffected by haste
    return trigger_gcd;
  }
};

// Barrage =================================================================

// This is a spell because that's the only way to support "channeled" effects
struct barrage_t : public hunter_spell_t
{
  struct barrage_damage_t : public hunter_ranged_attack_t
  {
    barrage_damage_t( hunter_t* player ) :
      hunter_ranged_attack_t( "barrage_primary", player, player -> talents.barrage -> effectN( 2 ).trigger() )
    {
      background  = true;
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      may_crit    = true;

      // FIXME AoE is just approximate from tooltips
      aoe = -1;
      base_aoe_multiplier = 0.5;
    }

    virtual void trigger_wild_quiver( double multiplier = 1.0 )
    {
      hunter_ranged_attack_t::trigger_wild_quiver( multiplier / 6.0 );
    }
  };

  barrage_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "barrage", player, player -> talents.barrage )
  {
    parse_options( NULL, options_str );

    may_block  = false;
    travel_speed = 0.0;

    channeled    = true;
    hasted_ticks = false;
    tick_zero = true;

    // FIXME still needs to AoE component
    dynamic_tick_action = true;
    tick_action = new barrage_damage_t( player );
  }

  virtual void schedule_execute()
  {
    hunter_spell_t::schedule_execute();

    // To suppress autoshot, just delay it by the execute time of the barrage
    if ( p() -> main_hand_attack && p() -> main_hand_attack -> execute_event )
    {
      timespan_t time_to_next_hit = p() -> main_hand_attack -> execute_event -> remains();
      time_to_next_hit += num_ticks * tick_time( p() -> composite_attack_speed() );
      p() -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
    }
  }

  virtual bool usable_moving()
  {
    return true;
  }
};

// A Murder of Crows ==============================================================

struct moc_t : public ranged_attack_t
{
  struct peck_t : public ranged_attack_t
  {
    peck_t( hunter_t* player ) :
      ranged_attack_t( "crow_peck", player, player -> find_spell( 131900 ) )
    {
      background  = true;
      may_crit   = true;
      may_parry = false;
      may_block = false;
      school = SCHOOL_PHYSICAL;

      direct_power_mod = data().extra_coeff();
      tick_power_mod = data().extra_coeff();
    }
  };

  moc_t( hunter_t* player, const std::string& options_str ) :
    ranged_attack_t( "a_murder_of_crows", player, player -> talents.a_murder_of_crows )
  {
    parse_options( NULL, options_str );

    hasted_ticks = false;
    may_crit = false;
    may_miss = false;
    school = SCHOOL_PHYSICAL;

    base_spell_power_multiplier    = 0.0;
    base_attack_power_multiplier   = 1.0;

    dynamic_tick_action = true;
    tick_action = new peck_t( player );
  }

  hunter_t* p() const { return static_cast<hunter_t*>( player ); }

  virtual double action_multiplier()
  {
    double am = ranged_attack_t::action_multiplier();
    am *= p() -> beast_multiplier();
    return am;
  }

  virtual double cost()
  {
    double c = ranged_attack_t::cost();
    if ( p() -> buffs.beast_within -> check() )
      c *= ( 1.0 + p() -> buffs.beast_within -> data().effectN( 1 ).percent() );
    return c;
  }

  virtual void execute()
  {
    cooldown -> duration = data().cooldown();

    if ( target -> health_percentage() < 20 )
      cooldown -> duration = timespan_t::from_seconds( data().effectN( 1 ).base_value() );

    ranged_attack_t::execute();
  }
};

// Dire Beast ==============================================================

struct dire_critter_t : public pet_t
{
  struct melee_t : public melee_attack_t
  {
    int focus_gain;

    melee_t( dire_critter_t* player ) :
      melee_attack_t( "dire_beast_melee", player )
    {
      if ( player -> o() -> pet_dire_beasts[ 0 ] )
        stats = player -> o() -> pet_dire_beasts[ 0 ] -> get_stats( "dire_beast_melee" );

      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = 1;
      school = SCHOOL_PHYSICAL;

      // dire beast does double melee dmg
      base_multiplier *= 2.0;

      trigger_gcd = timespan_t::zero();

      background = true;
      repeating  = true;
      special    = false;
      may_glance = true;
      may_crit   = true;

      focus_gain = player -> find_spell( 120694 ) -> effectN( 1 ).base_value();
    }

    dire_critter_t* p() const
    { return static_cast<dire_critter_t*>( player ); }

    virtual void impact( action_state_t* s )
    {
      melee_attack_t::impact( s );

      if ( result_is_hit( s -> result ) )
        p() -> o() -> resource_gain( RESOURCE_FOCUS, focus_gain, p() -> o() -> gains.dire_beast );
    }
  };

  dire_critter_t( hunter_t* owner ) :
    pet_t( owner -> sim, owner, "dire_beast", true /*GUARDIAN*/ )
  {
    // FIX ME
    owner_coeff.ap_from_ap = 1.00;
  }

  hunter_t* o() const
  { return static_cast< hunter_t* >( owner ); }

  virtual void init_base()
  {
    pet_t::init_base();

    // FIXME numbers are from treant. correct them.
    resources.base[ RESOURCE_HEALTH ] = 9999; // Level 85 value
    resources.base[ RESOURCE_MANA   ] = 0;

    stamina_per_owner = 0;

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, o() -> level );
    main_hand_weapon.max_dmg    = main_hand_weapon.min_dmg;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2 );

    main_hand_attack = new melee_t( this );
  }

  virtual void summon( timespan_t duration=timespan_t::zero() )
  {
    pet_t::summon( duration );

    // attack immediately on summons
    main_hand_attack -> execute();
  }

  virtual double composite_player_multiplier( school_e school, action_t* a )
  {
    double m = pet_t::composite_player_multiplier( school, a );
    m *= o() -> beast_multiplier();
    return m;
  }

  virtual double composite_attack_haste()
  {
    double ah = pet_t::composite_attack_haste();
    // remove the portions of speed that were ranged only.
    ah /= o() -> ranged_haste_multiplier();
    return ah;
  }

  virtual double composite_attack_speed()
  {
    double ah = pet_t::composite_attack_speed();
    // remove the portions of speed that were ranged only.
    ah /= o() -> ranged_haste_multiplier();
    return ah;
  }
};

struct dire_beast_t : public hunter_spell_t
{
  dire_beast_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "dire_beast", player, player -> talents.dire_beast )
  {
    parse_options( NULL, options_str );
    harmful = false;
    school = SCHOOL_PHYSICAL;
    hasted_ticks = false;
    may_crit = false;
    may_miss = false;
  }

  virtual void init()
  {
    hunter_spell_t::init();

    stats -> add_child( p() -> pet_dire_beasts[ 0 ] -> get_stats( "dire_beast_melee" ) );
  }

  virtual void execute()
  {
    hunter_spell_t::execute();

    pet_t* beast = p() -> pet_dire_beasts[ 0 ];
    if ( ! beast -> current.sleeping )
      beast = p() -> pet_dire_beasts[ 1 ];

    // should be data().duration()
    timespan_t duration = timespan_t::from_seconds( 15 );
    beast -> summon( duration );
  }
};

// Aspect of the Hawk =======================================================

struct aspect_of_the_hawk_t : public hunter_spell_t
{
  aspect_of_the_hawk_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "aspect_of_the_hawk", player, player -> talents.aspect_of_the_iron_hawk -> ok() ? player -> find_talent_spell( "Aspect of the Iron Hawk" ) : player -> find_class_spell( "Aspect of the Hawk" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    hunter_spell_t::execute();

    if ( !p() -> active_aspect )
    {
      p() -> active_aspect = ASPECT_HAWK;
      double value = p() -> dbc.effect_average( data().effect_id( 1 ), p() -> level );
      p() -> buffs.aspect_of_the_hawk -> trigger( 1, value );
    }
    else if ( p() -> active_aspect == ASPECT_HAWK )
    {
      p() -> active_aspect = ASPECT_NONE;
      p() -> buffs.aspect_of_the_hawk -> expire();
    }

  }
  virtual bool ready()
  {
    if ( p() -> active_aspect == ASPECT_HAWK )
      return false;

    return hunter_spell_t::ready();
  }
};

// Bestial Wrath ============================================================

struct bestial_wrath_t : public hunter_spell_t
{
  bestial_wrath_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "bestial_wrath", player, player -> find_class_spell( "Bestial Wrath" ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    p() -> buffs.beast_within  -> trigger();
    p() -> active_pet -> buffs.bestial_wrath -> trigger();

    hunter_spell_t::execute();
  }

  virtual bool ready()
  {
    if ( ! p() -> active_pet )
      return false;

    return hunter_spell_t::ready();
  }
};

// Fervor ===================================================================

struct fervor_t : public hunter_spell_t
{
  int base_gain;
  int tick_gain;

  fervor_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "fervor", player, player -> talents.fervor )
  {
    parse_options( NULL, options_str );

    harmful = false;

    trigger_gcd = timespan_t::zero();

    hasted_ticks = false;
    base_gain = data().effectN( 1 ).base_value();
    tick_gain = data().effectN( 2 ).base_value();
  }

  virtual void execute()
  {
    p() -> resource_gain( RESOURCE_FOCUS, base_gain, p() -> gains.fervor );

    if ( p() -> active_pet )
      p() -> active_pet -> resource_gain( RESOURCE_FOCUS, base_gain, p() -> active_pet -> gains.fervor );

    hunter_spell_t::execute();
  }

  virtual void tick( dot_t* d )
  {
    hunter_spell_t::tick( d );

    p() -> resource_gain( RESOURCE_FOCUS, tick_gain, p() -> gains.fervor );

    if ( p() -> active_pet )
      p() -> active_pet -> resource_gain( RESOURCE_FOCUS, tick_gain, p() -> active_pet -> gains.fervor );
  }
};

// Focus Fire ===============================================================

struct focus_fire_t : public hunter_spell_t
{
  int five_stacks;
  focus_fire_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "focus_fire", player, player -> find_spell( 82692 ) ),
    five_stacks( 0 )
  {
    option_t options[] =
    {
      opt_bool( "five_stacks", five_stacks ),
      opt_null()
    };
    parse_options( options, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    double stacks = p() -> active_pet -> buffs.frenzy -> stack();
    double value = stacks * p() -> specs.focus_fire -> effectN( 1 ).percent();
    p() -> buffs.focus_fire -> trigger( 1, value );

    double gain = stacks * p() -> specs.focus_fire -> effectN( 2 ).resource( RESOURCE_FOCUS );
    p() -> active_pet -> resource_gain( RESOURCE_FOCUS, gain, p() -> active_pet -> gains.focus_fire );

    hunter_spell_t::execute();

    p() -> active_pet -> buffs.frenzy -> expire();
  }

  virtual bool ready()
  {
    if ( ! p() -> active_pet )
      return false;

    if ( ! p() -> active_pet -> buffs.frenzy -> check() )
      return false;

    if ( five_stacks && p() -> active_pet -> buffs.frenzy -> stack_react() < 5 )
      return false;

    return hunter_spell_t::ready();
  }
};

// Hunter's Mark ============================================================

struct hunters_mark_t : public hunter_spell_t
{
  hunters_mark_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "hunters_mark", player, player -> find_class_spell( "Hunter's Mark" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
    background = ( sim -> overrides.ranged_vulnerability != 0 );
  }

  virtual void impact( action_state_t* s )
  {
    hunter_spell_t::impact( s );

    if ( ! sim -> overrides.ranged_vulnerability )
      s -> target -> debuffs.ranged_vulnerability -> trigger();
  }
};

// Blink Strike =============================================================

struct blink_strike_t : public hunter_spell_t
{
  blink_strike_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "blink_strike", player, player -> talents.blink_strike )
  {
    parse_options( NULL, options_str );

    base_spell_power_multiplier    = 0.0;
    base_attack_power_multiplier   = 1.0;

    harmful = false;

    for ( size_t i = 0, pets = p() -> pet_list.size(); i < pets; ++i )
    {
      pet_t* pet = p() -> pet_list[ i ];
      stats -> add_child( pet -> get_stats( "blink_strike" ) );
    }
  }

  virtual void execute()
  {
    hunter_spell_t::execute();

    if ( p() -> active_pet )
    {
      // teleport the pet to the target
      p() -> active_pet -> current.distance = 0;
      p() -> active_pet -> blink_strike -> execute();
    }
  }

  virtual bool ready()
  {
    return p() -> active_pet && hunter_spell_t::ready();
  }
};

// Lynx Rush ==============================================================

struct lynx_rush_t : public hunter_spell_t
{
  struct lynx_rush_bite_t : public hunter_spell_t
  {
    rng_t* position_rng;
    bool last_tick;

    lynx_rush_bite_t( hunter_t* player )  : hunter_spell_t( "lynx_rush_bite", player )
    {
      background = true;
      dual = true;
      school = SCHOOL_PHYSICAL;

      special    = true;
      may_glance = false;
      may_crit   = true;

      position_rng = player -> get_rng( "lynx_rush_position" );
      last_tick = false;
    }

    virtual void execute()
    {
      hunter_spell_t::execute();

      hunter_pet_t* pet = p() -> active_pet;
      if ( pet )
      {
        // Hardcoded 50% for randomly jumping around
        set_position( position_rng -> roll( 0.5 ) ? POSITION_BACK : POSITION_FRONT );
        pet -> lynx_rush -> execute();
        if ( last_tick ) set_position( POSITION_BACK );
        last_tick = false;
      }
    }

    void set_position( position_e pos )
    {
      hunter_pet_t* pet = p() -> active_pet;
      if ( !pet || pos == pet -> position() )
        return;

      pet -> change_position( pos );
      bool flag = POSITION_FRONT == pet -> position();
      for ( size_t i = 0; i < pet -> action_list.size(); ++i )
      {
        action_t* a = pet -> action_list[ i ];
        a -> may_parry = a -> may_block = flag;
      }
    }
  };

  lynx_rush_bite_t* bite;

  lynx_rush_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "lynx_rush", player, player -> talents.lynx_rush )
  {
    parse_options( NULL, options_str );

    special = true;
    may_glance = false;
    harmful = false;
    school = SCHOOL_PHYSICAL;

    base_spell_power_multiplier    = 0.0;
    base_attack_power_multiplier   = 1.0;

    for ( size_t i = 0, pets = p() -> pet_list.size(); i < pets; ++i )
    {
      pet_t* pet = p() -> pet_list[ i ];
      stats -> add_child( pet -> get_stats( "lynx_rush" ) );
    }

    hasted_ticks = false;
    tick_zero = true;
    dynamic_tick_action = true;
    bite = new lynx_rush_bite_t( player );
    tick_action = bite;
  }

  virtual bool ready()
  {
    return p() -> active_pet && hunter_spell_t::ready();
  }

  virtual void last_tick( dot_t* d )
  {
    hunter_spell_t::last_tick( d );

    bite -> last_tick = true;
  }
};

// Kill Command =============================================================

struct kill_command_t : public hunter_spell_t
{
  kill_command_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "kill_command", player, player -> find_class_spell( "Kill Command" ) )
  {
    parse_options( NULL, options_str );

    base_spell_power_multiplier    = 0.0;
    base_attack_power_multiplier   = 1.0;

    harmful = false;

    for ( size_t i = 0, pets = p() -> pet_list.size(); i < pets; ++i )
    {
      pet_t* pet = p() -> pet_list[ i ];
      stats -> add_child( pet -> get_stats( "kill_command" ) );
    }
  }

  virtual void execute()
  {
    hunter_spell_t::execute();

    if ( p() -> active_pet )
    {
      p() -> active_pet -> kill_command -> execute();
    }
  }

  virtual bool ready()
  {
    return p() -> active_pet && hunter_spell_t::ready();
  }
};


// Rapid Fire ===============================================================

struct rapid_fire_t : public hunter_spell_t
{
  rapid_fire_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "rapid_fire", player, player -> find_class_spell( "Rapid Fire" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    double value = data().effectN( 1 ).percent();
    p() -> buffs.rapid_fire -> trigger( 1, value );

    hunter_spell_t::execute();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.rapid_fire -> check() )
      return false;

    return hunter_spell_t::ready();
  }
};

// Readiness ================================================================

struct readiness_t : public hunter_spell_t
{
  int wait_for_rf;
  std::vector<cooldown_t*> cooldown_list;

  readiness_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "readiness", player, player -> find_class_spell( "Readiness" ) ),
    wait_for_rf( false )
  {
    option_t options[] =
    {
      // Only perform Readiness while Rapid Fire is up, allows the sequence
      // Rapid Fire, Readiness, Rapid Fire, for better RF uptime
      opt_bool( "wait_for_rapid_fire", wait_for_rf ),
      opt_null()
    };
    parse_options( options, options_str );

    harmful = false;

    cooldown_list.push_back( p() -> get_cooldown( "traps"             ) );
    cooldown_list.push_back( p() -> get_cooldown( "chimera_shot"      ) );
    cooldown_list.push_back( p() -> get_cooldown( "kill_shot"         ) );
    cooldown_list.push_back( p() -> get_cooldown( "scatter_shot"      ) );
    cooldown_list.push_back( p() -> get_cooldown( "silencing_shot"    ) );
    cooldown_list.push_back( p() -> get_cooldown( "kill_command"      ) );
    cooldown_list.push_back( p() -> get_cooldown( "blink_strike"      ) );
    cooldown_list.push_back( p() -> get_cooldown( "rapid_fire"        ) );
    cooldown_list.push_back( p() -> get_cooldown( "bestial_wrath"     ) );
    cooldown_list.push_back( p() -> get_cooldown( "concussive_shot"   ) );
    cooldown_list.push_back( p() -> get_cooldown( "dire_beast"        ) );
    cooldown_list.push_back( p() -> get_cooldown( "powershot"         ) );
    cooldown_list.push_back( p() -> get_cooldown( "barrage"           ) );
    cooldown_list.push_back( p() -> get_cooldown( "lynx_rush"         ) );
    cooldown_list.push_back( p() -> get_cooldown( "a_murder_of_crows" ) );
    cooldown_list.push_back( p() -> get_cooldown( "glaive_toss"       ) );
    cooldown_list.push_back( p() -> get_cooldown( "deterrence"        ) );
    cooldown_list.push_back( p() -> get_cooldown( "distracting_shot"  ) );
    cooldown_list.push_back( p() -> get_cooldown( "freezing_trap"     ) );
    cooldown_list.push_back( p() -> get_cooldown( "frost_trap"        ) );
    cooldown_list.push_back( p() -> get_cooldown( "explosive_trap"    ) );
    cooldown_list.push_back( p() -> get_cooldown( "explosive_shot"    ) );
    // FIX ME: does this ICD get reset?
    // cooldown_list.push_back( p() -> get_cooldown( "lock_and_load"     ) );
  }

  virtual void execute()
  {
    hunter_spell_t::execute();

    for ( unsigned int i=0; i < cooldown_list.size(); i++ )
    {
      cooldown_list[ i ] -> reset();
    }
  }

  virtual bool ready()
  {

    if ( wait_for_rf && ! p() -> buffs.rapid_fire -> check() )
      return false;

    return hunter_spell_t::ready();
  }
};

// Summon Pet ===============================================================

struct summon_pet_t : public hunter_spell_t
{
  pet_t* pet;

  summon_pet_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "summon_pet", player ),
    pet( 0 )
  {
    harmful = false;

    std::string pet_name = options_str.empty() ? p() -> summon_pet_str : options_str;
    pet = p() -> find_pet( pet_name );
    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", p() -> name(), pet_name.c_str() );
      sim -> cancel();
    }
  }

  virtual void execute()
  {
    hunter_spell_t::execute();

    pet -> summon();

    if ( p() -> main_hand_attack ) p() -> main_hand_attack -> cancel();
  }

  virtual bool ready()
  {
    if ( p() -> active_pet == pet )
      return false;

    return hunter_spell_t::ready();
  }
};

// Trueshot Aura ============================================================

struct trueshot_aura_t : public hunter_spell_t
{
  trueshot_aura_t( hunter_t* player, const std::string& /* options_str */ ) :
    hunter_spell_t( "trueshot_aura", player, player -> find_spell( 19506 ) )
  {
    trigger_gcd = timespan_t::zero();
    harmful = false;
    background = ( sim -> overrides.attack_power_multiplier != 0 && sim -> overrides.critical_strike != 0 );
  }

  virtual void execute()
  {
    hunter_spell_t::execute();

    if ( ! sim -> overrides.attack_power_multiplier )
      sim -> auras.attack_power_multiplier -> trigger();

    if ( ! sim -> overrides.critical_strike )
      sim -> auras.critical_strike -> trigger();
  }
};

// Stampede ============================================================

struct stampede_t : public hunter_spell_t
{
  stampede_t( hunter_t* p, const std::string& options_str ) :
    hunter_spell_t( "stampede", p, p -> find_class_spell( "Stampede" ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
    school = SCHOOL_PHYSICAL;
  }

  virtual void execute()
  {
    hunter_spell_t::execute();

    for ( unsigned int i = 0; i < p() -> hunter_main_pets.size() && i < 5; ++i )
    {
      hunter_pet_t* pet = p() -> hunter_main_pets[ i ];

      pet -> stampede_summon( data().duration() );
    }
  }
};

} // spells

namespace pet_actions {

// Template for common hunter pet action code. See priest_action_t.
template <class Base>
struct hunter_pet_action_t : public Base
{
  typedef Base ab;
  typedef hunter_pet_action_t base_t;

  bool special_ability;

  hunter_pet_action_t( const std::string& n, hunter_pet_t* player,
                       const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    special_ability( false )
  {
    if ( ab::data().rank_str() && !strcmp( ab::data().rank_str(), "Special Ability" ) )
      special_ability = true;
  }

  hunter_pet_t* p() const
  { return static_cast<hunter_pet_t*>( ab::player ); }

  hunter_t* o() const
  { return static_cast<hunter_t*>( p() -> cast_owner() ); }

  hunter_pet_td_t* cast_td( player_t* t = 0 )
  { return p() -> get_target_data( t ? t : ab::target ); }

  void apply_exotic_beast_cd()
  {
    ab::cooldown -> duration *= 1.0 + o() -> specs.exotic_beasts -> effectN( 2 ).percent();
  }
};

// ==========================================================================
// Hunter Pet Attacks
// ==========================================================================

struct hunter_pet_attack_t : public hunter_pet_action_t<melee_attack_t>
{
  hunter_pet_attack_t( const std::string& n, hunter_pet_t* player,
                       const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s )
  {
    special = true;
    may_crit = true;
  }
};

// Pet Melee ================================================================

struct pet_melee_t : public hunter_pet_attack_t
{
  pet_melee_t( hunter_pet_t* player ) :
    hunter_pet_attack_t( "melee", player )
  {
    special = false;
    weapon = &( player -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    background        = true;
    repeating         = true;
    school = SCHOOL_PHYSICAL;
  }
};

// Pet Auto Attack ==========================================================

struct pet_auto_attack_t : public hunter_pet_attack_t
{
  pet_auto_attack_t( hunter_pet_t* player, const std::string& options_str ) :
    hunter_pet_attack_t( "auto_attack", player, 0 )
  {
    parse_options( NULL, options_str );

    p() -> main_hand_attack = new pet_melee_t( player );
    trigger_gcd = timespan_t::zero();
    school = SCHOOL_PHYSICAL;
  }

  virtual void execute()
  {
    p() -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    return( p() -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Pet Claw/Bite/Smack ============================================================

struct basic_attack_t : public hunter_pet_attack_t
{
  rng_t* rng_invigoration;
  double chance_invigoration;
  double gain_invigoration;

  basic_attack_t( hunter_pet_t* p, const std::string& name, const std::string& options_str ) :
    hunter_pet_attack_t( name, p, p -> find_pet_spell( name ) )
  {
    parse_options( NULL, options_str );
    school = SCHOOL_PHYSICAL;

    // hardcoded into tooltip
    direct_power_mod = 0.168;

    base_multiplier *= 1.0 + p -> specs.spiked_collar -> effectN( 1 ).percent();
    rng_invigoration = player -> get_rng( "invigoration" );
    chance_invigoration = p -> find_spell( 53397 ) -> proc_chance();
    gain_invigoration = p -> find_spell( 53398 ) -> effectN( 1 ).resource( RESOURCE_FOCUS );
  }

  virtual void execute()
  {
    hunter_pet_attack_t::execute();

    if ( p() == o() -> active_pet )
      o() -> buffs.cobra_strikes -> decrement();

    if ( o() -> specs.frenzy -> ok() )
      p() -> buffs.frenzy -> trigger( 1 );
  }

  virtual void impact( action_state_t* s )
  {
    hunter_pet_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( o() -> specs.invigoration -> ok() && rng_invigoration -> roll( chance_invigoration ) )
      {
        o() -> resource_gain( RESOURCE_FOCUS, gain_invigoration, o() -> gains.invigoration );
        o() -> procs.invigoration -> occur();
      }
    }
  }

  virtual double composite_crit()
  {
    double cc = hunter_pet_attack_t::composite_crit();

    if ( o() -> buffs.cobra_strikes -> up() && p() == o() -> active_pet )
      cc += o() -> buffs.cobra_strikes -> data().effectN( 1 ).percent();

    return cc;
  }

  bool use_wild_hunt()
  {
    // comment out to avoid procs
    return p() -> resources.current[ RESOURCE_FOCUS ] > 50;
  }

  virtual double action_multiplier()
  {
    double am = ab::action_multiplier();

    if ( use_wild_hunt() )
    {
      p() -> benefits.wild_hunt -> update( true );
      am *= 1.0 + p() -> specs.wild_hunt -> effectN( 1 ).percent();
    }
    else
      p() -> benefits.wild_hunt -> update( false );

    return am;
  }

  virtual double cost()
  {
    double c = ab::cost();

    if ( use_wild_hunt() )
      c *= 1.0 + p() -> specs.wild_hunt -> effectN( 2 ).percent();

    return c;
  }

};

// Devilsaur Monstrous Bite =================================================

struct monstrous_bite_t : public hunter_pet_attack_t
{
  monstrous_bite_t( hunter_pet_t* p, const std::string& options_str ) :
    hunter_pet_attack_t( "monstrous_bite", p, p -> find_spell( 54680 ) )
  {
    parse_options( NULL, options_str );
    auto_cast = true;
    apply_exotic_beast_cd();
    school = SCHOOL_PHYSICAL;
    stats -> school = SCHOOL_PHYSICAL;
  }
};

// Lynx Rush (pet) =======================================================

struct pet_lynx_rush_t : public hunter_pet_attack_t
{
  pet_lynx_rush_t( hunter_pet_t* p ) :
    hunter_pet_attack_t( "lynx_rush", p, p -> find_spell( 120699 ) )
  {
    proc = true;
    special = true;
    background        = true;
    may_crit   = true;

    // school = SCHOOL_BLEED;
    tick_may_crit = true;
    tick_power_mod = data().extra_coeff();
    dot_behavior = DOT_REFRESH;

    repeating         = false;
    // weapon = &( player -> main_hand_weapon );
    // base_execute_time = weapon -> swing_time;
  }

  virtual void tick( dot_t* d )
  {
    hunter_pet_attack_t::tick( d );
  }

  virtual void impact( action_state_t* s )
  {
    if ( result_is_hit( s -> result ) )
    {
      cast_td( s -> target ) -> debuffs_lynx_bleed -> trigger();
      snapshot_state( s, snapshot_flags, DMG_OVER_TIME );
      dot_t* dot = get_dot( s -> target );
      event_t* ticker = dot -> tick_event;
      if ( ticker )
      {
        ticker -> reschedule_time = ticker -> time;
        //sim -> reschedule_event( ticker );
      }
    }

    hunter_pet_attack_t::impact( s );
  }

  virtual double composite_target_ta_multiplier( player_t* t )
  {
    double am = hunter_pet_attack_t::composite_target_ta_multiplier( t );
    double stack = cast_td( t -> target ) -> debuffs_lynx_bleed -> stack();
    am *= stack;
    return am;
  }

  virtual void last_tick( dot_t* d )
  {
    cast_td( d -> state -> target ) -> debuffs_lynx_bleed -> expire();
    hunter_pet_attack_t::last_tick( d );
  }
};

// Blink Strike (pet) =======================================================

struct pet_blink_strike_t : public hunter_pet_attack_t
{
  pet_blink_strike_t( hunter_pet_t* p ) :
    hunter_pet_attack_t( "blink_strike", p, p -> find_spell( 130393 ) )
  {
    background = true;
    proc = true;

    special = false;
    weapon = &( player -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    school = SCHOOL_PHYSICAL;
    background        = true;
    repeating         = false;
  }

};

// Kill Command (pet) =======================================================

struct pet_kill_command_t : public hunter_pet_attack_t
{
  pet_kill_command_t( hunter_pet_t* p ) :
    hunter_pet_attack_t( "kill_command", p, p -> find_spell( 83381 ) )
  {
    background = true;
    proc = true;
    school = SCHOOL_PHYSICAL;

    // hardcoded into hunter kill command tooltip
    direct_power_mod = 0.7;
  }

  virtual double action_multiplier()
  {
    double am = hunter_pet_attack_t::action_multiplier();
    am *= 1.0 + o() -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();
    return am;
  }
};

// ==========================================================================
// Hunter Pet Spells
// ==========================================================================

struct hunter_pet_spell_t : public hunter_pet_action_t<spell_t>
{
  hunter_pet_spell_t( const std::string& n, hunter_pet_t* player,
                      const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s )
  {
    apply_exotic_beast_cd();
  }
};

// Rabid ====================================================================

struct rabid_t : public hunter_pet_spell_t
{
  rabid_t( hunter_pet_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "rabid", player, player -> find_spell( 53401 ) )
  {
    parse_options( NULL, options_str );

    may_miss = false;
  }

  virtual void execute()
  {
    p() -> buffs.rabid -> trigger();

    hunter_pet_spell_t::execute();
  }
};

// ==========================================================================
// Unique Pet Specials
// ==========================================================================

// Wolf/Devilsaur Furious Howl ==============================================

struct furious_howl_t : public hunter_pet_spell_t
{
  furious_howl_t( hunter_pet_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "furious_howl", player, player -> find_pet_spell( "Furious Howl" ) )
  {

    parse_options( NULL, options_str );

    harmful = false;
    background = ( sim -> overrides.critical_strike != 0 );
  }

  virtual void execute()
  {
    hunter_pet_spell_t::execute();

    if ( ! sim -> overrides.critical_strike )
      sim -> auras.critical_strike -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );
  }
};

// Cat/Spirit Beast Roar of Courage =========================================

struct roar_of_courage_t : public hunter_pet_spell_t
{
  roar_of_courage_t( hunter_pet_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "roar_of_courage", player, player -> find_pet_spell( "Roar of Courage" ) )
  {

    parse_options( NULL, options_str );

    harmful = false;
    background = ( sim -> overrides.str_agi_int != 0 );
  }

  virtual void execute()
  {
    hunter_pet_spell_t::execute();
    double mastery_rating = data().effectN( 1 ).average( player );
    if ( ! sim -> overrides.mastery )
      sim -> auras.mastery -> trigger( 1, mastery_rating, -1.0, data().duration() );
  }
};

// Silithid Qiraji Fortitude  ===============================================

struct qiraji_fortitude_t : public hunter_pet_spell_t
{
  qiraji_fortitude_t( hunter_pet_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "qiraji_fortitude", player, player -> find_pet_spell( "Qiraji Fortitude" ) )
  {

    parse_options( NULL, options_str );

    harmful = false;
    background = ( sim -> overrides.stamina != 0 );
  }

  virtual void execute()
  {
    hunter_pet_spell_t::execute();

    if ( ! sim -> overrides.stamina )
      sim -> auras.stamina -> trigger();
  }
};

// Wind Serpent Lightning Breath ============================================

struct lightning_breath_t : public hunter_pet_spell_t
{
  lightning_breath_t( hunter_pet_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "lightning_breath", player, player -> find_pet_spell( "Lightning Breath" ) )
  {

    parse_options( 0, options_str );

    auto_cast = true;
    background = ( sim -> overrides.magic_vulnerability != 0 );
  }

  virtual void impact( action_state_t* s )
  {
    hunter_pet_spell_t::impact( s );

    if ( result_is_hit( s -> result ) && ! sim -> overrides.magic_vulnerability )
      s -> target -> debuffs.magic_vulnerability -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );
  }
};

// Serpent Corrosive Spit  ==================================================

struct corrosive_spit_t : public hunter_pet_spell_t
{
  corrosive_spit_t( hunter_pet_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "corrosive_spit", player, player -> find_spell( 95466 ) )
  {

    parse_options( 0, options_str );

    auto_cast = true;
    background = ( sim -> overrides.weakened_armor != 0 );
  }

  virtual void impact( action_state_t* s )
  {
    hunter_pet_spell_t::impact( s );

    if ( result_is_hit( s -> result ) && ! sim -> overrides.weakened_armor )
      s -> target -> debuffs.weakened_armor -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );
  }
};

// Demoralizing Screech  ====================================================

struct demoralizing_screech_t : public hunter_pet_spell_t
{
  demoralizing_screech_t( hunter_pet_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "demoralizing_screech", player, player -> find_spell( 24423 ) )
  {
    parse_options( 0, options_str );

    aoe         = -1;
    auto_cast = true;
    background = ( sim -> overrides.weakened_blows != 0 );
  }

  virtual void impact( action_state_t* s )
  {
    hunter_pet_spell_t::impact( s );

    // TODO: Is actually an aoe ability
    if ( result_is_hit( s -> result ) && ! sim -> overrides.weakened_blows )
      s -> target -> debuffs.weakened_blows -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );
  }
};

// Ravager Ravage ===========================================================

struct ravage_t : public hunter_pet_spell_t
{
  ravage_t( hunter_pet_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "ravage", player, player -> find_spell( 50518 ) )
  {
    parse_options( 0, options_str );

    auto_cast = true;
  }
};

// Raptor Tear Armor  =======================================================

struct tear_armor_t : public hunter_pet_spell_t
{
  tear_armor_t( hunter_pet_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "tear_armor", player, player -> find_spell( 50498 ) )
  {
    parse_options( 0, options_str );

    auto_cast = true;

    background = ( sim -> overrides.weakened_armor != 0 );
  }

  virtual void impact( action_state_t* s )
  {
    hunter_pet_spell_t::impact( s );

    if ( result_is_hit( s -> result ) && ! sim -> overrides.weakened_armor )
      s -> target -> debuffs.weakened_armor -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );
  }
};

// Hyena Cackling Howl =========================================================

// TODO add attack speed to hyena
struct cackling_howl_t : public hunter_pet_spell_t
{
  cackling_howl_t( hunter_pet_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "cackling_howl", player, player -> find_pet_spell( "Cackling Howl" ) )
  {
    parse_options( 0, options_str );

    auto_cast = true;
  }
};

// Chimera Froststorm Breath ================================================

struct froststorm_breath_t : public hunter_pet_spell_t
{
  struct froststorm_breath_tick_t : public hunter_pet_spell_t
  {
    froststorm_breath_tick_t( hunter_pet_t* player ) :
      hunter_pet_spell_t( "froststorm_breath_tick", player, player -> find_spell( 95725 ) )
    {
      direct_power_mod = 0.144; // hardcoded into tooltip, 29/08/2012
      background  = true;
      direct_tick = true;
    }
  };

  froststorm_breath_tick_t* tick_spell;

  froststorm_breath_t( hunter_pet_t* player, const std::string& options_str ) :
    hunter_pet_spell_t( "froststorm_breath", player, player -> find_pet_spell( "Froststorm Breath" ) )
  {
    channeled = true;

    parse_options( NULL, options_str );

    auto_cast = true;

    tick_spell = new froststorm_breath_tick_t( player );

    add_child( tick_spell );
  }

  virtual void init()
  {
    hunter_pet_spell_t::init();

    tick_spell -> stats = stats;
  }

  virtual void tick( dot_t* d )
  {
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick );
  }
};

} // pet_actions

struct wild_quiver_trigger_t : public action_callback_t
{
  attack_t* attack;
  rng_t* rng;

  wild_quiver_trigger_t( hunter_t* p, attack_t* a ) :
    action_callback_t( p ), attack( a )
  {
    rng = p -> get_rng( "wild_quiver" );
  }

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    hunter_t* p = static_cast<hunter_t*>( listener );

    if ( ! a -> weapon )
      return;

    if ( a -> weapon -> slot != SLOT_MAIN_HAND )
      return;

    if ( a -> proc )
      return;

    if ( rng -> roll( p -> composite_mastery() * p -> mastery.wild_quiver -> effectN( 1 ).coeff() / 100.0 ) )
    {
      attack -> execute();
      p -> procs.wild_quiver -> occur();
    }
  }
};

hunter_td_t::hunter_td_t( player_t* target, hunter_t* p ) :
  actor_pair_t( target, p ),
  dots( dots_t() )
{
  dots.serpent_sting = target -> get_dot( "serpent_sting", p );
}

hunter_pet_td_t::hunter_pet_td_t( player_t* target, hunter_pet_t* p ) :
  actor_pair_t( target, p )
{
  // Make the duration indefinite and let the last tick turn off the debuff.
  // Otherwise the debuff might not be set for the last tick.
  debuffs_lynx_bleed = buff_creator_t( p, "lynx_rush", p -> find_spell( 120699 ) ).duration( timespan_t::zero() );
}
// hunter_pet_t::create_action ==============================================

action_t* hunter_pet_t::create_action( const std::string& name,
                                       const std::string& options_str )
{
  if ( name == "auto_attack"           ) return new      pet_auto_attack_t( this, options_str );
  if ( name == "claw"                  ) return new         basic_attack_t( this, "Claw", options_str );
  if ( name == "bite"                  ) return new         basic_attack_t( this, "Bite", options_str );
  if ( name == "smack"                 ) return new         basic_attack_t( this, "Smack", options_str );
  if ( name == "froststorm_breath"     ) return new    froststorm_breath_t( this, options_str );
  if ( name == "furious_howl"          ) return new         furious_howl_t( this, options_str );
  if ( name == "roar_of_courage"       ) return new      roar_of_courage_t( this, options_str );
  if ( name == "qiraji_fortitude"      ) return new     qiraji_fortitude_t( this, options_str );
  if ( name == "lightning_breath"      ) return new     lightning_breath_t( this, options_str );
  if ( name == "monstrous_bite"        ) return new       monstrous_bite_t( this, options_str );
  if ( name == "rabid"                 ) return new                rabid_t( this, options_str );
  if ( name == "corrosive_spit"        ) return new       corrosive_spit_t( this, options_str );
  if ( name == "demoralizing_screech"  ) return new demoralizing_screech_t( this, options_str );
  if ( name == "ravage"                ) return new               ravage_t( this, options_str );
  if ( name == "tear_armor"            ) return new           tear_armor_t( this, options_str );
  if ( name == "cackling_howl"         ) return new        cackling_howl_t( this, options_str );
  if ( name == "corrosive_spit"        ) return new       corrosive_spit_t( this, options_str );

  return pet_t::create_action( name, options_str );
}

// hunter_t::create_action ==================================================

action_t* hunter_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "auto_shot"             ) return new              start_attack_t( this, options_str );
  if ( name == "aimed_shot"            ) return new             aimed_shot_t( this, options_str );
  if ( name == "arcane_shot"           ) return new            arcane_shot_t( this, options_str );
  if ( name == "aspect_of_the_hawk"    ) return new     aspect_of_the_hawk_t( this, options_str );
  // make this a no-op for now to not break profiles.
  if ( name == "aspect_of_the_fox"     ) return new     aspect_of_the_hawk_t( this, options_str );
  if ( name == "bestial_wrath"         ) return new          bestial_wrath_t( this, options_str );
  if ( name == "black_arrow"           ) return new            black_arrow_t( this, options_str );
  if ( name == "chimera_shot"          ) return new           chimera_shot_t( this, options_str );
  if ( name == "explosive_shot"        ) return new         explosive_shot_t( this, options_str );
  if ( name == "explosive_trap"        ) return new         explosive_trap_t( this, options_str );
  if ( name == "fervor"                ) return new                 fervor_t( this, options_str );
  if ( name == "focus_fire"            ) return new             focus_fire_t( this, options_str );
  if ( name == "hunters_mark"          ) return new           hunters_mark_t( this, options_str );
  if ( name == "blink_strike"          ) return new           blink_strike_t( this, options_str );
  if ( name == "lynx_rush"             ) return new              lynx_rush_t( this, options_str );
  if ( name == "kill_command"          ) return new           kill_command_t( this, options_str );
  if ( name == "kill_shot"             ) return new              kill_shot_t( this, options_str );
  if ( name == "multi_shot"            ) return new             multi_shot_t( this, options_str );
  if ( name == "rapid_fire"            ) return new             rapid_fire_t( this, options_str );
  if ( name == "readiness"             ) return new              readiness_t( this, options_str );
  if ( name == "scatter_shot"          ) return new           scatter_shot_t( this, options_str );
  if ( name == "serpent_sting"         ) return new          serpent_sting_t( this, options_str );
  if ( name == "silencing_shot"        ) return new         silencing_shot_t( this, options_str );
  if ( name == "steady_shot"           ) return new            steady_shot_t( this, options_str );
  if ( name == "summon_pet"            ) return new             summon_pet_t( this, options_str );
  if ( name == "trueshot_aura"         ) return new          trueshot_aura_t( this, options_str );
  if ( name == "cobra_shot"            ) return new             cobra_shot_t( this, options_str );
  if ( name == "a_murder_of_crows"     ) return new                    moc_t( this, options_str );
  if ( name == "powershot"             ) return new              powershot_t( this, options_str );
  if ( name == "stampede"              ) return new               stampede_t( this, options_str );
  if ( name == "glaive_toss"           ) return new            glaive_toss_t( this, options_str );
  if ( name == "dire_beast"            ) return new             dire_beast_t( this, options_str );
  if ( name == "barrage"               ) return new                barrage_t( this, options_str );

#if 0
  if ( name == "trap_launcher"         ) return new          trap_launcher_t( this, options_str );
  if ( name == "binding_shot"          ) return new           binding_shot_t( this, options_str );
  if ( name == "aspect_of_the_iron_hawk" ) return new      aspect_of_the_iron_hawk_t( this, options_str );
  if ( name == "wyvern_sting"          ) return new           wyvern_sting_t( this, options_str );
#endif
  return player_t::create_action( name, options_str );
}

// hunter_t::create_pet =====================================================

pet_t* hunter_t::create_pet( const std::string& pet_name,
                             const std::string& pet_type )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  pet_e type = util::parse_pet_type( pet_type );

  if ( type > PET_NONE && type < PET_HUNTER )
  {
    return new hunter_pet_t( sim, this, pet_name, type );
  }
  else
  {
    sim -> errorf( "Player %s with pet %s has unknown type %s\n", name(), pet_name.c_str(), pet_type.c_str() );
    sim -> cancel();
  }

  return 0;
}

// hunter_t::create_pets ====================================================

void hunter_t::create_pets()
{
  create_pet( "cat",          "cat"          );
  create_pet( "devilsaur",    "devilsaur"    );
  create_pet( "raptor",       "raptor"       );
  create_pet( "hyena",        "hyena"        );
  create_pet( "wolf",         "wolf"         );
//  create_pet( "chimera",      "chimera"      );
//  create_pet( "wind_serpent", "wind_serpent" );

  pet_dire_beasts[0] = new dire_critter_t( this );
  pet_dire_beasts[1] = new dire_critter_t( this );
}

// hunter_t::init_spells ====================================================

void hunter_t::init_spells()
{
  player_t::init_spells();

  // Talents
  talents.posthaste                         = find_talent_spell( "Posthaste" );
  talents.narrow_escape                     = find_talent_spell( "Narrow Escape" );
  talents.exhilaration                      = find_talent_spell( "Exhilaration" );

  talents.silencing_shot                    = find_talent_spell( "Silencing Shot" );
  talents.wyvern_sting                      = find_talent_spell( "Wyvern Sting" );
  talents.binding_shot                      = find_talent_spell( "Binding Shot" );

  talents.crouching_tiger_hidden_chimera    = find_talent_spell( "Crouching Tiger, Hidden Chimera" );
  talents.aspect_of_the_iron_hawk           = find_talent_spell( "Aspect of the Iron Hawk" );
  talents.spirit_bond                       = find_talent_spell( "Spirit Bond" );

  talents.fervor                            = find_talent_spell( "Fervor" );
  talents.dire_beast                        = find_talent_spell( "Dire Beast" );
  talents.thrill_of_the_hunt                = find_talent_spell( "Thrill of the Hunt" );

  talents.a_murder_of_crows                 = find_talent_spell( "A Murder of Crows" );
  talents.blink_strike                      = find_talent_spell( "Blink Strike" );
  talents.lynx_rush                         = find_talent_spell( "Lynx Rush" );

  talents.glaive_toss                       = find_talent_spell( "Glaive Toss" );
  talents.powershot                         = find_talent_spell( "Powershot" );
  talents.barrage                           = find_talent_spell( "Barrage" );

  // Mastery
  mastery.master_of_beasts     = find_mastery_spell( HUNTER_BEAST_MASTERY );
  mastery.wild_quiver          = find_mastery_spell( HUNTER_MARKSMANSHIP );
  mastery.essence_of_the_viper = find_mastery_spell( HUNTER_SURVIVAL );

  // Major
  glyphs.aimed_shot          = find_glyph_spell( "Glyph of Aimed Shot"     ); // id=119465
  glyphs.endless_wrath       = find_glyph_spell( "Glyph of Endless Wrath"  );
  glyphs.animal_bond         = find_glyph_spell( "Glyph of Animal Bond"    );
  glyphs.black_ice           = find_glyph_spell( "Glyph of Block Ice"  );
  glyphs.camouflage          = find_glyph_spell( "Glyph of Camoflage"  );
  glyphs.chimera_shot        = find_glyph_spell( "Glyph of Chimera Shot"   );
  glyphs.deterrence          = find_glyph_spell( "Glyph of Deterrence"  );
  glyphs.disengage           = find_glyph_spell( "Glyph of Disengage"  );
  glyphs.distracting_shot    = find_glyph_spell( "Glyph of Distracting Shot"  );
  glyphs.explosive_trap      = find_glyph_spell( "Glyph of Explosive Trap"  );
  glyphs.freezing_trap       = find_glyph_spell( "Glyph of Freezing Trap"  );
  glyphs.ice_trap            = find_glyph_spell( "Glyph of Ice Trap"  );
  glyphs.icy_solace          = find_glyph_spell( "Glyph of Icy Solace"  );
  glyphs.marked_for_death    = find_glyph_spell( "Glyph of Marked for Death"  );
  glyphs.masters_call        = find_glyph_spell( "Glyph of Master's Call"  );
  glyphs.mend_pet            = find_glyph_spell( "Glyph of Mend Pet"  );
  glyphs.mending             = find_glyph_spell( "Glyph of Mending"  );
  glyphs.mirrored_blades     = find_glyph_spell( "Glyph of Mirrored Blades"  );
  glyphs.misdirection        = find_glyph_spell( "Glyph of Misdirection"  );
  glyphs.no_escape           = find_glyph_spell( "Glyph of No Escape"  );
  glyphs.pathfinding         = find_glyph_spell( "Glyph of Pathfinding"  );
  glyphs.scatter_shot        = find_glyph_spell( "Glyph of Scatter Shot"  );
  glyphs.scattering          = find_glyph_spell( "Glyph of Scattering"  );
  glyphs.snake_trap          = find_glyph_spell( "Glyph of Snake Trap"  );
  glyphs.tranquilizing_shot  = find_glyph_spell( "Glyph of Tranquilizing Shot"  );

  // Minor
  glyphs.aspects             = find_glyph_spell( "Glyph of Aspects"  );
  glyphs.aspect_of_the_pack  = find_glyph_spell( "Glyph of Aspect of the Pack"  );
  glyphs.direction           = find_glyph_spell( "Glyph of Direction"  );
  glyphs.fetch               = find_glyph_spell( "Glyph of Fetch"  );
  glyphs.fireworks           = find_glyph_spell( "Glyph of Fireworks"  );
  glyphs.lesser_proportion   = find_glyph_spell( "Glyph of Lesser Proportion"  );
  glyphs.marking             = find_glyph_spell( "Glyph of Marking"  );
  glyphs.revive_pet          = find_glyph_spell( "Glyph of Revive Pet"  );
  glyphs.stampede            = find_glyph_spell( "Glyph of Stampede"  );
  glyphs.tame_beast          = find_glyph_spell( "Glyph of Tame Beast"  );
  glyphs.the_cheetah         = find_glyph_spell( "Glyph of the Cheetah"  );

  specs.piercing_shots       = find_specialization_spell( "Piercing Shots" );
  specs.steady_focus         = find_specialization_spell( "Steady Focus" );
  specs.go_for_the_throat    = find_specialization_spell( "Go for the Throat" );
  specs.careful_aim          = find_specialization_spell( "Careful Aim" );
  specs.beast_cleave         = find_specialization_spell( "Beast Cleave" );
  specs.frenzy               = find_specialization_spell( "Frenzy" );
  specs.focus_fire           = find_specialization_spell( "Focus Fire" );
  specs.cobra_strikes        = find_specialization_spell( "Cobra Strikes" );
  specs.the_beast_within     = find_specialization_spell( "The Beast Within" );
  specs.exotic_beasts        = find_specialization_spell( "Exotic Beasts" );
  specs.invigoration         = find_specialization_spell( "Invigoration" );
  specs.kindred_spirits      = find_specialization_spell( "Kindred Spirits" );
  specs.careful_aim          = find_specialization_spell( "Careful Aim" );
  specs.improved_serpent_sting = find_specialization_spell( "Improved Serpent Sting" );
  specs.explosive_shot       = find_specialization_spell( "Explosive Shot" );
  specs.lock_and_load        = find_specialization_spell( "Lock and Load" );
  specs.viper_venom          = find_specialization_spell( "Viper Venom" );
  specs.bombardment          = find_specialization_spell( "Bombardment" );
  specs.rapid_recuperation   = find_specialization_spell( "Rapid Recuperation" );
  specs.master_marksman      = find_specialization_spell( "Master Marksman" );
  specs.serpent_spread       = find_specialization_spell( "Serpent Spread" );
  specs.trap_mastery         = find_specialization_spell( "Trap Mastery" );

  if ( specs.piercing_shots -> ok() )
    active_piercing_shots = new piercing_shots_t( this );

  if ( specs.explosive_shot -> ok() )
    active_explosive_ticks = new explosive_shot_tick_t( this );

  if ( mastery.wild_quiver -> ok() )
  {
    wild_quiver_shot = new wild_quiver_shot_t( this );
  }

  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P     M2P     M4P    T2P    T4P     H2P    H4P
    {     0,     0, 105732, 105921,     0,     0,     0,     0 }, // Tier13
    {     0,     0, 123090, 123091,     0,     0,     0,     0 }, // Tier14
    {     0,     0,      0,      0,     0,     0,     0,     0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// hunter_t::init_spells ====================================================

void hunter_pet_t::init_spells()
{
  pet_t::init_spells();

  // ferocity
  kill_command = new pet_kill_command_t( this );
  blink_strike = new pet_blink_strike_t( this );
  lynx_rush    = new pet_lynx_rush_t( this );

  specs.rabid= spell_data_t::not_found();
  specs.hearth_of_the_phoenix= spell_data_t::not_found();
  specs.spiked_collar= spell_data_t::not_found();
  // tenacity
  specs.last_stand= spell_data_t::not_found();
  specs.charge= spell_data_t::not_found();
  specs.thunderstomp= spell_data_t::not_found();
  specs.blood_of_the_rhino= spell_data_t::not_found();
  specs.great_stamina= spell_data_t::not_found();
  // cunning
  specs.roar_of_sacrifice= spell_data_t::not_found();
  specs.bullhead= spell_data_t::not_found();
  specs.cornered= spell_data_t::not_found();
  specs.boars_speed= spell_data_t::not_found();

  specs.dash = spell_data_t::not_found(); // ferocity, cunning

  // ferocity
  specs.rabid            = find_specialization_spell( "Rabid" );
  specs.spiked_collar    = find_specialization_spell( "Spiked Collar" );

  specs.wild_hunt = find_spell( 62762 );
  specs.combat_experience = find_specialization_spell( "Combat Experience" );
}

// hunter_t::init_base ======================================================

void hunter_t::init_base()
{
  player_t::init_base();

  base.attack_power = level * 2;

  initial.attack_power_per_strength = 0.0; // Prevents scaling from strength. Will need to separate melee and ranged AP if this is needed in the future.
  initial.attack_power_per_agility  = 2.0;

  base_focus_regen_per_second = 4;
  if ( set_bonus.pvp_4pc_melee() )
    base_focus_regen_per_second *= 1.25;


  resources.base[ RESOURCE_FOCUS ] = 100 + specs.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS );

  diminished_kfactor    = 0.009880;
  diminished_dodge_cap = 0.006870;
  diminished_parry_cap = 0.006870;

  stats_stampede = get_stats( "stampede" );


}

// hunter_t::init_buffs =====================================================

void hunter_t::create_buffs()
{
  player_t::create_buffs();

  buffs.aspect_of_the_hawk          = buff_creator_t( this, 13165, "aspect_of_the_hawk" );

  buffs.beast_within                = buff_creator_t( this, 34471, "beast_within" ).chance( specs.the_beast_within -> ok() );
  buffs.beast_within -> buff_duration += sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).time_value();

  buffs.bombardment                 = buff_creator_t( this, "bombardment", specs.bombardment -> effectN( 1 ).trigger() );
  buffs.cobra_strikes               = buff_creator_t( this, 53257, "cobra_strikes" ).chance( specs.cobra_strikes -> proc_chance() );
  buffs.focus_fire                  = buff_creator_t( this, 82692, "focus_fire" );
  buffs.steady_focus                = buff_creator_t( this, 53220, "steady_focus" ).chance( specs.steady_focus -> ok() );
  buffs.thrill_of_the_hunt          = buff_creator_t( this, 34720, "thrill_of_the_hunt" ).chance( talents.thrill_of_the_hunt -> proc_chance() );
  buffs.lock_and_load               = buff_creator_t( this, 56453, "lock_and_load" )
                                      .chance( specs.lock_and_load -> effectN( 1 ).percent() )
                                      .cd( timespan_t::from_seconds( specs.lock_and_load -> effectN( 2 ).base_value() ) ) ;
  buffs.lock_and_load -> default_chance += sets -> set( SET_T14_4PC_MELEE ) -> effectN( 2 ).percent();

  buffs.master_marksman             = buff_creator_t( this, 82925, "master_marksman" ).chance( specs.master_marksman -> proc_chance() );
  buffs.master_marksman_fire        = buff_creator_t( this, 82926, "master_marksman_fire" );

  buffs.rapid_fire                  = buff_creator_t( this, 3045, "rapid_fire" );
  buffs.rapid_fire -> cooldown -> duration = timespan_t::zero();
  buffs.pre_steady_focus            = buff_creator_t( this, "pre_steady_focus" ).max_stack( 2 ).quiet( true );

  buffs.tier13_4pc                  = buff_creator_t( this, 105919, "tier13_4pc" ).chance( sets -> set( SET_T13_4PC_MELEE ) -> proc_chance() ).cd( timespan_t::from_seconds( tier13_4pc_cooldown ) );
}

// hunter_t::init_values ====================================================

void hunter_t::init_values()
{
  player_t::init_values();

  // hardcoded in tooltip but the tooltip lies 18 Aug 2021
  cooldowns.viper_venom -> duration = timespan_t::from_seconds( 2.5 );

  // Orc racial
  if ( race == RACE_ORC )
    pet_multiplier *= 1.0 +  find_racial_spell( "Command" ) -> effectN( 1 ).percent();
}

// hunter_t::init_gains =====================================================

void hunter_t::init_gains()
{
  player_t::init_gains();

  gains.invigoration         = get_gain( "invigoration"         );
  gains.fervor               = get_gain( "fervor"               );
  gains.focus_fire           = get_gain( "focus_fire"           );
  gains.rapid_recuperation   = get_gain( "rapid_recuperation"   );
  gains.thrill_of_the_hunt   = get_gain( "thrill_of_the_hunt"   );
  gains.steady_shot          = get_gain( "steady_shot"          );
  gains.steady_focus         = get_gain( "steady_focus"         );
  gains.cobra_shot           = get_gain( "cobra_shot"           );
  gains.dire_beast           = get_gain( "dire_beast"           );
  gains.viper_venom          = get_gain( "viper_venom"          );
}

// hunter_t::init_position ==================================================

void hunter_t::init_position()
{
  player_t::init_position();

  if ( position() == POSITION_FRONT )
  {
    base.position = POSITION_RANGED_FRONT;
    position_str = util::position_type_string( position() );
  }
  else if ( position() == POSITION_BACK )
  {
    base.position = POSITION_RANGED_BACK;
    position_str = util::position_type_string( position() );
  }

  initial.position = base.position;
}

// hunter_t::init_procs =====================================================

void hunter_t::init_procs()
{
  player_t::init_procs();

  procs.invigoration                 = get_proc( "invigoration"                 );
  procs.thrill_of_the_hunt           = get_proc( "thrill_of_the_hunt"           );
  procs.wild_quiver                  = get_proc( "wild_quiver"                  );
  procs.lock_and_load                = get_proc( "lock_and_load"                );
  procs.explosive_shot_focus_starved = get_proc( "explosive_shot_focus_starved" );
  procs.black_arrow_focus_starved    = get_proc( "black_arrow_focus_starved"    );
}

// hunter_t::init_rng =======================================================

void hunter_t::init_rng()
{
  player_t::init_rng();

  rngs.invigoration         = get_rng( "invigoration"       );

  // Overlapping procs require the use of a "distributed" RNG-stream when normalized_roll=1
  // also useful for frequent checks with low probability of proc and timed effect

  rngs.frenzy               = get_rng( "frenzy" );
}

// hunter_t::init_scaling ===================================================

void hunter_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_STRENGTH ]         = false;
}

// hunter_t::init_actions ===================================================

void hunter_t::init_actions()
{
  if ( main_hand_weapon.group() != WEAPON_RANGED )
  {
    sim -> errorf( "Player %s does not have a ranged weapon at the Main Hand slot.", name() );
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    std::string& precombat = get_action_priority_list( "precombat" ) -> action_list_str;
    // optional CA action list
    std::string& CA_actions = get_action_priority_list( "CA" ) -> action_list_str;

    if ( level >= 80 )
    {
      if ( sim -> allow_flasks )
      {
        precombat += "/flask,type=";
        precombat += ( level > 85 ) ? "spring_blossoms" : "winds";
      }

      if ( sim -> allow_food )
      {
        precombat += "/food,type=";
        precombat += ( level > 85 ) ? "sea_mist_rice_noodles" : "seafood_magnifique_feast";
      }
    }

    precombat += "/aspect_of_the_hawk";
    precombat += "/hunters_mark,if=target.time_to_die>=21&!debuff.ranged_vulnerability.up";
    precombat += "/summon_pet";
    precombat += "/trueshot_aura";
    precombat += "/snapshot_stats";

    if ( ( level >= 80 ) && ( sim -> allow_potions ) )
    {
      precombat += ( level > 85 ) ? "/virmens_bite_potion" : "/tolvir_potion";

      action_list_str += ( level > 85 ) ? "/virmens_bite_potion" : "/tolvir_potion";
      action_list_str += ",if=buff.bloodlust.react|target.time_to_die<=60";
    }

    if ( specialization() == HUNTER_SURVIVAL )
      action_list_str += init_use_racial_actions();
    action_list_str += init_use_item_actions();
    action_list_str += init_use_profession_actions();
    action_list_str += "/auto_shot";
    action_list_str += "/explosive_trap,if=target.adds>0";

    switch ( specialization() )
    {
    // BEAST MASTERY
    case HUNTER_BEAST_MASTERY:
      action_list_str += "/focus_fire,five_stacks=1";
      action_list_str += "/serpent_sting,if=!ticking";

      action_list_str += init_use_racial_actions();
      action_list_str += "/fervor,if=enabled&!ticking&focus<=65";
      action_list_str += "/bestial_wrath,if=focus>60&!buff.beast_within.up";

      action_list_str += "/multi_shot,if=target.adds>5";
      action_list_str += "/cobra_shot,if=target.adds>5";

      action_list_str += "/rapid_fire,if=!buff.rapid_fire.up";
      if ( level >= 87 )
        action_list_str += "/stampede,if=buff.rapid_fire.up|buff.bloodlust.react|target.time_to_die<=25";

      action_list_str += "/kill_shot";
      action_list_str += "/kill_command";

      action_list_str += "/a_murder_of_crows,if=enabled&!ticking";
      action_list_str += "/glaive_toss,if=enabled";
      action_list_str += "/lynx_rush,if=enabled&!dot.lynx_rush.ticking";
      action_list_str += "/dire_beast,if=enabled&focus<=90";
      action_list_str += "/barrage,if=enabled";
      action_list_str += "/powershot,if=enabled";
      action_list_str += "/blink_strike,if=enabled";
      action_list_str += "/readiness,wait_for_rapid_fire=1";
      action_list_str += "/arcane_shot,if=buff.thrill_of_the_hunt.react";
      action_list_str += "/focus_fire,five_stacks=1,if=!ticking&!buff.beast_within.up";
      action_list_str += "/cobra_shot,if=dot.serpent_sting.remains<6";
      action_list_str += "/arcane_shot,if=focus>=61|buff.beast_within.up";

      if ( level >= 81 )
        action_list_str += "/cobra_shot";
      else
        action_list_str += "/steady_shot";
      break;

    // MARKSMANSHIP
    case HUNTER_MARKSMANSHIP:
      action_list_str += init_use_racial_actions();

      action_list_str += "/powershot,if=enabled";
      action_list_str += "/blink_strike,if=enabled";
      action_list_str += "/lynx_rush,if=enabled&!dot.lynx_rush.ticking";

      action_list_str += "/multi_shot,if=target.adds>5";
      action_list_str += "/steady_shot,if=target.adds>5";
      action_list_str += "/fervor,if=enabled&focus<=50";

      action_list_str += "/rapid_fire,if=!buff.rapid_fire.up";
      if ( level >= 87 )
        action_list_str += "/stampede,if=buff.rapid_fire.up|buff.bloodlust.react|target.time_to_die<=25";
      action_list_str += "/a_murder_of_crows,if=enabled&!ticking";
      action_list_str += "/dire_beast,if=enabled";

      action_list_str += "/run_action_list,name=CA,if=target.health.pct>90";
      // sub-action list for the CA phase. The action above here are also included
      CA_actions += "/serpent_sting,if=!ticking";
      CA_actions += "/chimera_shot";
      CA_actions += "/readiness";
      CA_actions += "/steady_shot,if=buff.pre_steady_focus.up&buff.steady_focus.remains<6";
      CA_actions += "/aimed_shot,if=buff.master_marksman_fire.react";
      CA_actions += "/aimed_shot";
      CA_actions += "/steady_shot";

      // actions for outside the CA phase
      action_list_str += "/glaive_toss,if=enabled";
      action_list_str += "/barrage,if=enabled";
      action_list_str += "/steady_shot,if=buff.pre_steady_focus.up&buff.steady_focus.remains<=5";
      action_list_str += "/serpent_sting,if=!ticking";
      action_list_str += "/chimera_shot";
      action_list_str += "/readiness";
      action_list_str += "/steady_shot,if=buff.steady_focus.remains<(action.steady_shot.cast_time+1)&!in_flight";
      action_list_str += "/kill_shot";
      action_list_str += "/aimed_shot,if=buff.master_marksman_fire.react";

      action_list_str += "/arcane_shot,if=buff.thrill_of_the_hunt.react";

      if ( set_bonus.tier13_4pc_melee() )
      {
        action_list_str += "/arcane_shot,if=(focus>=66|cooldown.chimera_shot.remains>=4)&(!buff.rapid_fire.up&!buff.bloodlust.react&!buff.berserking.up&!buff.tier13_4pc.react&cooldown.buff_tier13_4pc.remains<=0)";
        action_list_str += "/aimed_shot,if=(cooldown.chimera_shot.remains>5|focus>=80)&(buff.bloodlust.react|buff.tier13_4pc.react|cooldown.buff_tier13_4pc.remains>0)|buff.rapid_fire.up";
      }
      else
      {
        action_list_str += "/aimed_shot,if=buff.rapid_fire.up|buff.bloodlust.react";
        if ( race == RACE_TROLL )
          action_list_str += "|buff.berserking.up";
        action_list_str += "/arcane_shot,if=focus>=60|(focus>=43&(cooldown.chimera_shot.remains>=action.steady_shot.cast_time))&(!buff.rapid_fire.up&!buff.bloodlust.react";
        if ( race == RACE_TROLL )
          action_list_str += "&!buff.berserking.up)";
        else
          action_list_str += ")";
      }

      action_list_str += "/steady_shot";
      break;

      // SURVIVAL
    case HUNTER_SURVIVAL:
      action_list_str += "/a_murder_of_crows,if=enabled&!ticking";
      action_list_str += "/blink_strike,if=enabled";
      action_list_str += "/lynx_rush,if=enabled&!dot.lynx_rush.ticking";

      action_list_str += "/explosive_shot,if=buff.lock_and_load.react";

      action_list_str += "/glaive_toss,if=enabled";
      action_list_str += "/powershot,if=enabled";
      action_list_str += "/barrage,if=enabled";

      action_list_str += "/multi_shot,if=target.adds>2";
      action_list_str += "/cobra_shot,if=target.adds>2";
      action_list_str += "/serpent_sting,if=!ticking&target.time_to_die>=10";
      action_list_str += "/explosive_shot,if=cooldown_react";

      action_list_str += "/kill_shot";
      action_list_str += "/black_arrow,if=!ticking&target.time_to_die>=8";

      action_list_str += "/multi_shot,if=buff.thrill_of_the_hunt.react&dot.serpent_sting.remains<2";
      action_list_str += "/arcane_shot,if=buff.thrill_of_the_hunt.react";

      action_list_str += "/rapid_fire,if=!buff.rapid_fire.up";
      action_list_str += "/dire_beast,if=enabled";
      if ( level >= 87 )
        action_list_str += "/stampede,if=buff.rapid_fire.up|buff.bloodlust.react|target.time_to_die<=25";

      action_list_str += "/readiness,wait_for_rapid_fire=1";
      action_list_str += "/fervor,if=enabled&focus<=50";
      action_list_str += "/cobra_shot,if=dot.serpent_sting.remains<6";
      action_list_str += "/arcane_shot,if=focus>=67";

      if ( find_class_spell( "Cobra Shot" ) )
        action_list_str += "/cobra_shot";
      else if ( find_class_spell( "Steady Shot" ) )
        action_list_str += "/steady_shot";
      break;

      // DEFAULT
    default: break;
    }

    if ( summon_pet_str.empty() )
      summon_pet_str = "cat";

    action_list_default = 1;
  }

  player_t::init_actions();
}

void hunter_t::init_items()
{
  player_t::init_items();

  // Check for Vishanka, Jaws of the Earth
  items.size();
  for ( size_t i = 0; i < items.size(); ++i )
  {
    item_t& item = items[ i ];
    if ( item.slot == SLOT_MAIN_HAND && strstr( item.name(), "vishanka_jaws_of_the_earth" ) )
    {
      // Store the spell id, not just if we have it or not
      vishanka = item.heroic() ? 109859 : item.lfr() ? 109857 : 107822;
      uint32_t id = dbc.spell( vishanka ) -> effectN( 1 ).trigger_spell_id();
      active_vishanka = new vishanka_t( this, id );
    }
  }
}
// hunter_t::combat_begin ===================================================

void hunter_t::combat_begin()
{
  player_t::combat_begin();
}

// hunter_t::reset ==========================================================

void hunter_t::reset()
{
  player_t::reset();

  // Active
  active_pet            = 0;
  active_aspect         = ASPECT_NONE;
}

// hunter_t::composite_attack_power_multiplier ==============================

double hunter_t::composite_attack_power_multiplier()
{
  double mult = player_t::composite_attack_power_multiplier();

  if ( buffs.aspect_of_the_hawk -> up() )
  {
    mult *= 1.0 + buffs.aspect_of_the_hawk -> data().effectN( 1 ).percent();
  }

  return mult;
}

// hunter_t::ranged_haste_multiplier =========================================

// Buffs that increase hunter ranged attack haste (and thus regen) but not pet attack haste

double hunter_t::ranged_haste_multiplier()
{
  double h = 1.0;
  h *= 1.0 / ( 1.0 + buffs.focus_fire -> value() );
  h *= 1.0 / ( 1.0 + buffs.rapid_fire -> value() );
  h *= 1.0 / ( 1.0 + buffs.steady_focus -> value() );
  return h;
}

// hunter_t::composite_attack_haste =========================================

double hunter_t::composite_attack_haste()
{
  double h = player_t::composite_attack_haste();
  h *= 1.0 / ( 1.0 + buffs.tier13_4pc -> up() * buffs.tier13_4pc -> data().effectN( 1 ).percent() );
  h *= ranged_haste_multiplier();
  return h;
}

// hunter_t::composite_player_multiplier ====================================

double hunter_t::composite_player_multiplier( school_e school, action_t* a )
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( spell_data_t::is_school( school, SCHOOL_NATURE ) ||
       spell_data_t::is_school( school, SCHOOL_ARCANE ) ||
       spell_data_t::is_school( school, SCHOOL_SHADOW ) ||
       spell_data_t::is_school( school, SCHOOL_FIRE   ) )
  {
    m *= 1.0 + mastery.essence_of_the_viper -> effectN( 1 ).coeff() / 100.0 * composite_mastery();
  }

  if ( buffs.beast_within -> up() )
    m *= 1.0 + buffs.beast_within -> data().effectN( 2 ).percent();

  return m;
}

// hunter_t::matching_gear_multiplier =======================================

double hunter_t::matching_gear_multiplier( attribute_e attr )
{
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0.0;
}

// hunter_t::regen  =========================================================

void hunter_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );
  periodicity *= 1.0 + current.haste_rating / rating.attack_haste;
  if ( buffs.rapid_fire -> check() && specs.rapid_recuperation -> ok() )
  {
    // 2/4 focus per sec
    double rr_regen = periodicity.total_seconds() * specs.rapid_recuperation -> effectN( 1 ).base_value();

    resource_gain( RESOURCE_FOCUS, rr_regen, gains.rapid_recuperation );
  }
}

// hunter_t::create_options =================================================

void hunter_t::create_options()
{
  player_t::create_options();

  option_t hunter_options[] =
  {
    opt_string( "summon_pet", summon_pet_str ),
    opt_float( "merge_piercing_shots", merge_piercing_shots ),
    opt_float( "tier13_4pc_cooldown", tier13_4pc_cooldown ),
    opt_null()
  };

  option_t::copy( options, hunter_options );
}

// hunter_t::create_profile =================================================

bool hunter_t::create_profile( std::string& profile_str, save_e stype, bool save_html )
{
  player_t::create_profile( profile_str, stype, save_html );

#if 0
  for ( pet_t* pet = pet_list; pet; pet = pet -> next_pet )
  {
    hunter_pet_t* cast = ( hunter_pet_t* ) pet;

    if ( pet -> talents_str.empty() )
    {
      for ( int j=0; j < MAX_TALENT_TREES; j++ )
        for ( int k=0; k < ( int ) pet -> talent_trees[ j ].size(); k++ )
          pet -> talents_str += ( char ) ( pet -> talent_trees[ j ][ k ] -> ok() + ( int ) '0' );
    }

    profile_str += "pet=";
    profile_str += util::pet_type_string( cast -> pet_type );
    profile_str += ",";
    profile_str += cast -> name_str + "\n";
    profile_str += "talents=" + cast -> talents_str + "\n";
    profile_str += "active=owner\n";
  }
#endif

  profile_str += "summon_pet=" + summon_pet_str + "\n";

  return true;
}

// hunter_t::copy_from ======================================================

void hunter_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  hunter_t* p = debug_cast<hunter_t*>( source );

  summon_pet_str = p -> summon_pet_str;
  merge_piercing_shots = p -> merge_piercing_shots;
}

// hunter_t::armory_extensions ==============================================

void hunter_t::armory_extensions( const std::string& /* region */,
                                  const std::string& /* server */,
                                  const std::string& /* character */,
                                  cache::behavior_e  /* caching */ )
{
#if 0
  // Pet support
  static pet_e pet_types[] =
  {
    /* 0*/ PET_NONE,         PET_WOLF,         PET_CAT,          PET_SPIDER,   PET_BEAR,
    /* 5*/ PET_BOAR,         PET_CROCOLISK,    PET_CARRION_BIRD, PET_CRAB,     PET_GORILLA,
    /*10*/ PET_NONE,         PET_RAPTOR,       PET_TALLSTRIDER,  PET_NONE,     PET_NONE,
    /*15*/ PET_NONE,         PET_NONE,         PET_NONE,         PET_NONE,     PET_NONE,
    /*20*/ PET_SCORPID,      PET_TURTLE,       PET_NONE,         PET_NONE,     PET_BAT,
    /*25*/ PET_HYENA,        PET_BIRD_OF_PREY, PET_WIND_SERPENT, PET_NONE,     PET_NONE,
    /*30*/ PET_DRAGONHAWK,   PET_RAVAGER,      PET_WARP_STALKER, PET_SPOREBAT, PET_NETHER_RAY,
    /*35*/ PET_SERPENT,      PET_NONE,         PET_MOTH,         PET_CHIMERA,  PET_DEVILSAUR,
    /*40*/ PET_NONE,         PET_SILITHID,     PET_WORM,         PET_RHINO,    PET_WASP,
    /*45*/ PET_CORE_HOUND,   PET_SPIRIT_BEAST, PET_NONE,         PET_NONE,     PET_NONE,
    /*50*/ PET_FOX,          PET_MONKEY,       PET_DOG,          PET_BEETLE,   PET_NONE,
    /*55*/ PET_SHALE_SPIDER, PET_NONE,         PET_NONE,         PET_NONE,     PET_NONE,
    /*60*/ PET_NONE,         PET_NONE,         PET_NONE,         PET_NONE,     PET_NONE,
    /*65*/ PET_NONE,         PET_WASP,         PET_NONE,         PET_NONE,     PET_NONE
  };
  int num_families = sizeof( pet_types ) / sizeof( pet_e );

  std::string url = "http://" + region + ".battle.net/wow/en/character/" + server + '/' + character + "/pet";
  xml_node_t* pet_xml = xml_t::get( sim, url, caching );
  if ( sim -> debug ) xml_t::print( pet_xml, sim -> output_file );

  xml_node_t* pet_list_xml = xml_t::get_node( pet_xml, "div", "class", "pets-list" );

  xml_node_t* pet_script_xml = xml_t::get_node( pet_list_xml, "script", "type", "text/javascript" );

  if ( ! pet_script_xml )
  {
    sim -> errorf( "Hunter %s unable to download pet data from Armory\n", name() );
    sim -> cancel();
    return;
  }

  std::string cdata_str;
  if ( xml_t::get_value( cdata_str, pet_script_xml, "cdata" ) )
  {
    std::string::size_type pos = cdata_str.find( '{' );
    if ( pos != std::string::npos ) cdata_str.erase( 0, pos+1 );
    pos = cdata_str.rfind( '}' );
    if ( pos != std::string::npos ) cdata_str.erase( pos );

    js_node_t* pet_js = js_t::create( sim, cdata_str );
    pet_js = js_t::get_node( pet_js, "Pet.data" );
    if ( sim -> debug ) js_t::print( pet_js, sim -> output_file );

    if ( ! pet_js )
    {
      sim -> errorf( "\nHunter %s unable to download pet data from Armory\n", name() );
      sim -> cancel();
      return;
    }

    std::vector<js_node_t*> pet_records;
    int num_pets = js_t::get_children( pet_records, pet_js );
    for ( int i=0; i < num_pets; i++ )
    {
      std::string pet_name, pet_talents;
      int pet_level, pet_family;

      if ( ! js_t::get_value( pet_name,    pet_records[ i ], "name"     ) ||
           ! js_t::get_value( pet_talents, pet_records[ i ], "build"    ) ||
           ! js_t::get_value( pet_level,   pet_records[ i ], "level"    ) ||
           ! js_t::get_value( pet_family,  pet_records[ i ], "familyId" ) )
      {
        sim -> errorf( "\nHunter %s unable to decode pet name/build/level/familyId for pet %s\n", name(), pet_name.c_str() );
        continue;
      }

      // Pets can have spaces in names, replace with underscore ..
      for ( unsigned j=0; j < pet_name.length(); j++ )
      {
        if ( pet_name[ j ] == ' ' )
          pet_name[ j ] = '_';
      }

      // Pets can have the same name, so suffix names with an unique
      // identifier from Battle.net, if one is found
      if ( js_t::get_name( pet_records[ i ] ) )
      {
        pet_name += '_';
        pet_name += js_t::get_name( pet_records[ i ] );
      }

      // Pets can have zero talents also, we should probably support it.
      /*
      bool all_zeros = true;
      for ( int j=pet_talents.size()-1; j >=0 && all_zeros; j-- )
        if ( pet_talents[ j ] != '0' )
          all_zeros = false;
      if ( all_zeros ) continue;
      */

      if ( pet_family > num_families || pet_types[ pet_family ] == PET_NONE )
      {
        sim -> errorf( "\nHunter %s unable to decode pet %s family id %d\n", name(), pet_name.c_str(), pet_family );
        continue;
      }

      hunter_pet_t* pet = new hunter_pet_t( sim, this, pet_name, pet_types[ pet_family ] );

      pet -> parse_talents_armory( pet_talents );

      pet -> talents_str.clear();
      for ( int j=0; j < MAX_TALENT_TREES; j++ )
      {
        for ( int k=0; k < ( int ) pet -> talent_trees[ j ].size(); k++ )
        {
          pet -> talents_str += ( char ) ( pet -> talent_trees[ j ][ k ] -> ok() + ( int ) '0' );
        }
      }
    }

    // If we have valid pets, figure out which to summon by parsing Battle.net
    if ( pet_list )
    {
      std::vector<xml_node_t*> pet_nodes;

      int n_pet_nodes = xml_t::get_nodes( pet_nodes, pet_list_xml, "a", "class", "pet" );
      for ( int i = 0; i < n_pet_nodes; i++ )
      {
        xml_node_t* summoned_node = xml_t::get_node( pet_nodes[ i ], "span", "class", "summoned" );
        std::string summoned_pet_name;
        std::string summoned_pet_id;

        if ( ! summoned_node )
          continue;

        xml_t::get_value( summoned_pet_id, pet_nodes[ i ], "data-id" );

        if ( ! xml_t::get_value( summoned_pet_name, xml_t::get_node( pet_nodes[ i ], "span", "class", "name" ), "." ) )
          continue;

        if ( ! summoned_pet_name.empty() )
        {
          summon_pet_str = util::decode_html( summoned_pet_name );
          if ( ! summoned_pet_id.empty() )
            summon_pet_str += '_' + summoned_pet_id;
        }
      }

      // Pick first pet on the list, if no pet is summoned in the battle net profile
      if ( summon_pet_str.empty() )
        summon_pet_str = pet_list -> name_str;
    }
  }
#endif
}

// hunter_t::decode_set =====================================================

int hunter_t::decode_set( item_t& item )
{
  const char* s = item.name();


  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  if ( strstr( s, "wyrmstalkers"       ) ) return SET_T13_MELEE;

  if ( strstr( s, "yaungol_slayer"     ) ) return SET_T14_MELEE;

  if ( strstr( s, "_gladiators_chain_" ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

// hunter_t::moving() =======================================================

void hunter_t::moving()
{
  player_t::interrupt();
}

// HUNTER MODULE INTERFACE ================================================

struct hunter_module_t : public module_t
{
  hunter_module_t() : module_t( HUNTER ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    return new hunter_t( sim, name, r );
  }

  virtual bool valid() const { return true; }
  virtual void init        ( sim_t* ) const {}
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end  ( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t& module_t::hunter()
{
  static hunter_module_t m = hunter_module_t();
  return m;
}
