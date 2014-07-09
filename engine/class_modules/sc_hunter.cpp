// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Hunter
// Improve lone wolf implementation -- Need to add a dismiss pet function.
// Black arrow implementation
// Merge effects of frenzy ---> Focus fire, go for the throat --> invigoration,
// Serpent spread + viper venom + imp. serpent sting ---> Serpent Sting,
// The beast within --> Bestial Wrath
// ==========================================================================

struct hunter_t;

namespace pets {
struct hunter_main_pet_t;
}

enum aspect_type { ASPECT_NONE = 0, ASPECT_MAX };

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
  std::vector<pets::hunter_main_pet_t*> hunter_main_pets;

  double locknload;// Select the chance for black arrow ticks to proc lock n load until Blizzard reveals the chance. Default is 15%.
  struct actives_t
  {
    pets::hunter_main_pet_t* pet;
    aspect_type         aspect;
    action_t*           explosive_ticks;
    action_t*           serpent_sting;
    attack_t*           wild_quiver_shot;
  } active;

  // Secondary pets
  // need an extra beast for readiness
  std::array<pet_t*, 2>  pet_dire_beasts;
  // Tier 15 2-piece bonus: need 10 slots (just to be safe) because each
  // Steady Shot or Cobra Shot can trigger a Thunderhawk, which stays
  // for 10 seconds.
  std::array< pet_t*, 10 > thunderhawk;

  // Tier 15 4-piece bonus
  attack_t* action_lightning_arrow_aimed_shot;
  attack_t* action_lightning_arrow_arcane_shot;
  attack_t* action_lightning_arrow_multi_shot;

  // Buffs
  struct buffs_t
  {
    buff_t* aspect_of_the_pack;
    buff_t* beast_cleave;
    buff_t* beast_within;
    buff_t* bombardment;
    buff_t* cobra_strikes;
    buff_t* focus_fire;
    buff_t* lock_and_load;
    buff_t* thrill_of_the_hunt;
    buff_t* stampede;
    buff_t* rapid_fire;
    buff_t* tier13_4pc;
    buff_t* tier16_4pc_mm_keen_eye;
    buff_t* tier16_4pc_bm_brutal_kinship;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* explosive_shot;
    cooldown_t* rapid_fire;
    cooldown_t* viper_venom;
  } cooldowns;

  // Custom Parameters
  std::string summon_pet_str;

  // Gains
  struct gains_t
  {
    gain_t* invigoration;
    gain_t* fervor;
    gain_t* focus_fire;
    gain_t* thrill_of_the_hunt;
    gain_t* steady_shot;
    gain_t* focusing_shot;
    gain_t* cobra_shot;
    gain_t* aimed_shot;
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
    proc_t* tier15_2pc_melee;
    proc_t* tier15_4pc_melee_aimed_shot;
    proc_t* tier15_4pc_melee_arcane_shot;
    proc_t* tier15_4pc_melee_multi_shot;
    proc_t* tier16_2pc_melee;
    proc_t* tier16_4pc_melee;
  } procs;

  real_ppm_t ppm_tier15_2pc_melee;
  real_ppm_t ppm_tier15_4pc_melee;

  // Talents
  struct talents_t
  {
    const spell_data_t* posthaste;
    const spell_data_t* narrow_escape;
    const spell_data_t* crouching_tiger_hidden_chimera;

    const spell_data_t* intimidation;
    const spell_data_t* wyvern_sting;
    const spell_data_t* binding_shot;

    const spell_data_t* exhilaration;
    const spell_data_t* iron_hawk;
    const spell_data_t* spirit_bond;

    const spell_data_t* fervor;
    const spell_data_t* dire_beast;
    const spell_data_t* thrill_of_the_hunt;

    const spell_data_t* a_murder_of_crows;
    const spell_data_t* blink_strikes;
    const spell_data_t* stampede;

    const spell_data_t* glaive_toss;
    const spell_data_t* powershot;
    const spell_data_t* barrage;

    const spell_data_t* flaming_shots;
    const spell_data_t* focusing_shot;
    const spell_data_t* versatility;
    const spell_data_t* lone_wolf;
  } talents;

  // Perks
  struct
  {
    const spell_data_t* enhanced_camouflage;
    //Beast Mastery
    const spell_data_t* improved_focus_fire;
    const spell_data_t* improved_kill_command;
    const spell_data_t* empowered_pets;
    const spell_data_t* improved_beast_cleave;
    const spell_data_t* enhanced_basic_attacks;
    const spell_data_t* enhanced_kill_shot;
    const spell_data_t* improved_arcane_shot;
    const spell_data_t* improved_cobra_shot;
    // Marks
    const spell_data_t* enhanced_chimera_shot;
    const spell_data_t* enhanced_aimed_shot;
    const spell_data_t* improved_focus;
    const spell_data_t* improved_aimed_shot;
    const spell_data_t* improved_steady_shot;
    const spell_data_t* enhanced_shots;
    // Survival
    const spell_data_t* empowered_explosive_shot;
    const spell_data_t* improved_black_arrow;
    const spell_data_t* enhanced_traps;
    const spell_data_t* enhanced_entrapment;
    const spell_data_t* improved_camouflage;
    const spell_data_t* improved_viper_venom;
  } perks;

  // Specialization Spells
  struct specs_t
  {
    // Baseline
    const spell_data_t* trueshot_aura;
    const spell_data_t* critical_strikes;

    // Shared
    const spell_data_t* crit_attunement;
    const spell_data_t* multistrike_attunement;
    const spell_data_t* mastery_attunement;

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
    // const spell_data_t* chimera_shot;
    // // const spell_data_t* wild quiver;
    //
    // // Survival
    const spell_data_t* explosive_shot;
    const spell_data_t* lock_and_load;
    // const spell_data_t* black_arrow;
    // const spell_data_t* entrapment;
    const spell_data_t* viper_venom;
    const spell_data_t* trap_mastery;
    const spell_data_t* serpent_sting;
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

  double pet_multiplier;

  hunter_t( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) :
    player_t( sim, HUNTER, name, r == RACE_NONE ? RACE_NIGHT_ELF : r ),
    active( actives_t() ),
    pet_dire_beasts(),
    thunderhawk(),
    action_lightning_arrow_aimed_shot(),
    action_lightning_arrow_arcane_shot(),
    action_lightning_arrow_multi_shot(),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    procs( procs_t() ),
    ppm_tier15_2pc_melee( *this, std::numeric_limits<double>::min(), RPPM_HASTE ),
    ppm_tier15_4pc_melee( *this, std::numeric_limits<double>::min(), RPPM_HASTE ),
    talents( talents_t() ),
    specs( specs_t() ),
    glyphs( glyphs_t() ),
    mastery( mastery_spells_t() ),
    stats_stampede( nullptr ),
    pet_multiplier( 1.0 )
  {
    // Cooldowns
    cooldowns.explosive_shot = get_cooldown( "explosive_shot" );
    cooldowns.rapid_fire     = get_cooldown( "rapid_fire" );
    cooldowns.viper_venom    = get_cooldown( "viper_venom" );

    locknload = 0.15;
    summon_pet_str = "";
    base.distance = 40;
    base_gcd = timespan_t::from_seconds( 1.0 );
  }

  // Character Definition
  virtual void      init_spells();
  virtual void      init_base_stats();
  virtual void      create_buffs();
  virtual void      init_gains();
  virtual void      init_position();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      init_action_list();
  virtual void      combat_begin();
  virtual void      arise();
  virtual void      reset();
  virtual double    composite_attack_power_multiplier() const;
  virtual double    composite_melee_crit() const;
  virtual double    composite_melee_haste() const;
  virtual double    composite_melee_speed() const;
  virtual double    ranged_haste_multiplier() const;
  virtual double    ranged_speed_multiplier() const;
  virtual double    composite_rating_multiplier( rating_e rating ) const;
  virtual double    composite_player_multiplier( school_e school ) const;
  virtual double    matching_gear_multiplier( attribute_e attr ) const;
  virtual void      invalidate_cache( cache_e );
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual set_e       decode_set( const item_t& ) const;
  virtual resource_e primary_resource() const { return RESOURCE_FOCUS; }
  virtual role_e    primary_role() const { return ROLE_ATTACK; }
  virtual stat_e    convert_hybrid_stat( stat_e s ) const;
  virtual bool      create_profile( std::string& profile_str, save_e = SAVE_ALL, bool save_html = false );
  virtual void      copy_from( player_t* source );
  virtual void      armory_extensions( const std::string& r, const std::string& s, const std::string& c, cache::behavior_e );
  virtual void      moving();

  target_specific_t<hunter_td_t*> target_data;

  virtual hunter_td_t* get_target_data( player_t* target ) const
  {
    hunter_td_t*& td = target_data[ target ];
    if ( ! td ) td = new hunter_td_t( target, const_cast<hunter_t*>(this) );
    return td;
  }

  // Event Tracking
  virtual void regen( timespan_t periodicity );

  double careful_aim_crit( player_t* target ) const
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
      pm *= 1.0 + cache.mastery_value();

    return pm;
  }
};

// Template for common hunter action code. See priest_action_t.
template <class Base>
struct hunter_action_t : public Base
{
private:
  typedef Base ab;
public:
  typedef hunter_action_t base_t;

  hunter_action_t( const std::string& n, hunter_t* player,
                   const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
  { }

  virtual ~hunter_action_t() {}

  hunter_t* p()
  { return static_cast<hunter_t*>( ab::player ); }
  const hunter_t* p() const
  { return static_cast<hunter_t*>( ab::player ); }

  hunter_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual double cost() const
  {
    double c = ab::cost();

    if ( c == 0 )
      return 0;

    if ( p() -> buffs.beast_within -> check() )
      c *= ( 1.0 + p() -> buffs.beast_within -> data().effectN( 1 ).percent() );

    return c;
  }

  // thrill_of_the_hunt support =============================================

  void trigger_thrill_of_the_hunt()
  {
    if ( p() -> talents.thrill_of_the_hunt -> ok() && cost() > 0 )
      // Stacks: 3 initial, 3 maximum
      if ( p() -> buffs.thrill_of_the_hunt -> trigger( p() -> buffs.thrill_of_the_hunt -> data().initial_stacks() ) )
        p() -> procs.thrill_of_the_hunt -> occur();
  }

  double thrill_discount( double cost ) const
  {
    double result = cost;

    if ( p() -> buffs.thrill_of_the_hunt -> check() )
      result += p() -> buffs.thrill_of_the_hunt -> data().effectN( 1 ).base_value();

    return std::max(0.0, result);
  }

  void consume_thrill_of_the_hunt()
  {
    // "up" is required to correctly track the buff benefit in reporting.
    // focus cost savings is reported as a "gain" so that comparison with fervor is easy
    if ( p() -> buffs.thrill_of_the_hunt -> up() )
    {
      double benefit = -(p() -> buffs.thrill_of_the_hunt -> data().effectN( 1 ).base_value());
      p() -> gains.thrill_of_the_hunt -> add( RESOURCE_FOCUS, benefit );
      p() -> buffs.thrill_of_the_hunt -> decrement();
    }
  }

  void trigger_tier16_bm_4pc_melee()
  {
    if ( ab::background )
      return;

    trigger_tier16_bm_4pc_brutal_kinship( p() );
  }
};

// SV Explosive Shot casts have a 40% chance to not consume a charge of Lock and Load.
// MM Instant Aimed shots reduce the cast time of your next Aimed Shot by 50%. (uses keen eye buff)
// TODO BM Offensive abilities used during Bestial Wrath increase all damage you deal by 2% and all
// damage dealt by your pet by 2%, stacking up to 15 times.
// 1 stack for MoC, Lynx Rush, Glaive Toss, Barrage, Powershot, Focus Fire
// no stack for Fervor or Dire Beast
void trigger_tier16_bm_4pc_brutal_kinship( hunter_t* p )
{
  if ( p -> specialization() != HUNTER_BEAST_MASTERY )
    return;
  if ( ! p -> sets.has_set_bonus( SET_T16_4PC_MELEE ) )
    return;
  if ( p -> buffs.beast_within -> check() )
    p -> buffs.tier16_4pc_bm_brutal_kinship -> trigger( 1, 0, 1, p -> buffs.beast_within -> remains() );
}

namespace pets {

// ==========================================================================
// Hunter Pet
// ==========================================================================

struct hunter_pet_t : public pet_t
{
public:
  typedef pet_t base_t;

  hunter_pet_t( sim_t& sim, hunter_t& owner, const std::string& pet_name, pet_e pt = PET_HUNTER, bool guardian = false ) :
    base_t( &sim, &owner, pet_name, pt, guardian )
  {
  }

  hunter_t* o() const
  { return static_cast< hunter_t* >( owner ); }

  virtual double composite_player_multiplier( school_e school ) const
  {
    double m = base_t::composite_player_multiplier( school );
    m *= o() -> beast_multiplier();
    return m;
  }

  virtual double composite_melee_haste() const
  {
    double ah = base_t::composite_melee_haste();
    // remove the portions of speed that were ranged only.
    ah /= o() -> ranged_haste_multiplier();
    return ah;
  }

  virtual double composite_melee_speed() const
  {
    double ah = base_t::composite_melee_speed();
    // remove the portions of speed that were ranged only.
    ah /= o() -> ranged_haste_multiplier();
    ah /= o() -> ranged_speed_multiplier();
    return ah;
  }
};

// Template for common hunter pet action code. See priest_action_t.
template <class T_PET, class Base>
struct hunter_pet_action_t : public Base
{
private:
  typedef Base ab;
public:
  typedef hunter_pet_action_t base_t;

  hunter_pet_action_t( const std::string& n, T_PET& p,
                       const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, &p, s )
  {

  }

  T_PET* p()
  { return static_cast<T_PET*>( ab::player ); }
  const T_PET* p() const
  { return static_cast<T_PET*>( ab::player ); }

  hunter_t* o()
  { return static_cast<hunter_t*>( p() -> o() ); }
  const hunter_t* o() const
  { return static_cast<hunter_t*>( p() -> o() ); }
};

// ==========================================================================
// Hunter Main Pet
// ==========================================================================

struct hunter_main_pet_td_t : public actor_pair_t
{
public:

  hunter_main_pet_td_t( player_t* target, hunter_main_pet_t* p );
};

struct hunter_main_pet_t : public hunter_pet_t
{
public:
  typedef hunter_pet_t base_t;

  struct actives_t
  {
    action_t* kill_command;
    attack_t* beast_cleave;
  } active;

  struct specs_t
  {
    // ferocity
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
    buff_t* stampede;
    buff_t* beast_cleave;
    buff_t* enhanced_basic_attacks;
    buff_t* tier16_4pc_bm_brutal_kinship;
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

  hunter_main_pet_t( sim_t& sim, hunter_t& owner, const std::string& pet_name, pet_e pt ) :
    base_t( sim, owner, pet_name, pt ),
    active( actives_t() ),
    specs( specs_t() ),
    buffs( buffs_t() ),
    gains( gains_t() ),
    benefits( benefits_t() )
  {
    owner.hunter_main_pets.push_back( this );

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( owner.type, owner.level ) * 0.25;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( owner.type, owner.level ) * 0.25;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    stamina_per_owner = 0.7;

    initial.armor_multiplier *= 1.05;

    // FIXME work around assert in pet specs
    // Set default specs
    _spec = default_spec();
  }

  specialization_e default_spec()
  {
    if ( pet_type > PET_NONE          && pet_type < PET_FEROCITY_TYPE ) return PET_FEROCITY;
    if ( pet_type > PET_FEROCITY_TYPE && pet_type < PET_TENACITY_TYPE ) return PET_TENACITY;
    if ( pet_type > PET_TENACITY_TYPE && pet_type < PET_CUNNING_TYPE  ) return PET_CUNNING;
    return PET_FEROCITY;
  }

  std::string unique_special()
  {
    switch ( pet_type )
    {
      case PET_CARRION_BIRD: return "demoralizing_screech";
      case PET_CAT:          return "roar_of_courage";
      case PET_CORE_HOUND:   return "";
      case PET_DEVILSAUR:    return "furious_howl/monstrous_bite";
      case PET_HYENA:        return "cackling_howl";
      case PET_MOTH:         return "";
      case PET_RAPTOR:       return "";
      case PET_SPIRIT_BEAST: return "roar_of_courage";
      case PET_TALLSTRIDER:  return "";
      case PET_WASP:         return "";
      case PET_WOLF:         return "furious_howl";
      case PET_BEAR:         return "";
      case PET_BOAR:         return "";
      case PET_CRAB:         return "";
      case PET_CROCOLISK:    return "";
      case PET_GORILLA:      return "";
      case PET_RHINO:        return "";
      case PET_SCORPID:      return "";
      case PET_SHALE_SPIDER: return "";
      case PET_TURTLE:       return "";
      case PET_WARP_STALKER: return "";
      case PET_WORM:         return "";
      case PET_BAT:          return "";
      case PET_BIRD_OF_PREY: return "";
      case PET_CHIMERA:      return "froststorm_breath";
      case PET_DRAGONHAWK:   return "lightning_breath";
      case PET_NETHER_RAY:   return "";
      case PET_RAVAGER:      return "ravage";
      case PET_SERPENT:      return "corrosive_spit";
      case PET_SILITHID:     return "qiraji_fortitude";
      case PET_SPIDER:       return "";
      case PET_SPOREBAT:     return "";
      case PET_WIND_SERPENT: return "lightning_breath";
      case PET_FOX:          return "tailspin";
      default: break;
    }
    return NULL;
  }

  virtual void init_base_stats()
  {
    base_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = util::interpolate( level, 0, 4253, 6373 );
    resources.base[ RESOURCE_FOCUS ] = 100 + o() -> specs.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS );

    base_gcd = timespan_t::from_seconds( 1.00 );

    resources.infinite_resource[ RESOURCE_FOCUS ] = o() -> resources.infinite_resource[ RESOURCE_FOCUS ];

    world_lag = timespan_t::from_seconds( 0.3 ); // Pet AI latency to get 3.3s claw cooldown as confirmed by Rivkah on EJ, August 2011
    world_lag_override = true;

    owner_coeff.ap_from_ap = 1.0;
    owner_coeff.sp_from_ap = 0.5;
  }

  virtual void create_buffs()
  {
    base_t::create_buffs();

    buffs.bestial_wrath     = buff_creator_t( this, 19574, "bestial_wrath" ).activated( true );
    buffs.bestial_wrath -> cooldown -> duration = timespan_t::zero();
    buffs.bestial_wrath -> buff_duration += owner -> sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).time_value();

    buffs.frenzy            = buff_creator_t( this, 19615, "frenzy_effect" ).chance( o() -> specs.frenzy -> effectN( 2 ).percent() );

    // Use buff to indicate whether the pet is a stampede summon
    buffs.stampede          = buff_creator_t( this, 130201, "stampede" )
                              .duration( timespan_t::from_millis( 20027 ) )
                              // Added 0.027 seconds to properly reflect haste threshholds seen in game.
                              .activated( true )
                              /*.quiet( true )*/;

    double cleave_value     = o() -> find_spell( "Beast Cleave" ) -> effectN( 1 ).percent() + o() -> perks.improved_beast_cleave -> effectN( 1 ).percent();
    buffs.beast_cleave      = buff_creator_t( this, 118455, "beast_cleave" ).activated( true ).default_value( cleave_value );

    buffs.enhanced_basic_attacks = buff_creator_t( this, "enhanced_basic_attacks", o() -> perks.enhanced_basic_attacks );

    buffs.tier16_4pc_bm_brutal_kinship = buff_creator_t( this, 145737, "tier16_4pc_brutal_kinship" )
                                         .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  virtual void init_gains()
  {
    base_t::init_gains();

    gains.fervor            = get_gain( "fervor"            );
    gains.focus_fire        = get_gain( "focus_fire"        );
    gains.go_for_the_throat = get_gain( "go_for_the_throat" );
  }

  virtual void init_benefits()
  {
    base_t::init_benefits();

    benefits.wild_hunt = get_benefit( "wild_hunt" );
  }

  virtual void init_action_list()
  {
    if ( action_list_str.empty() )
    {
      action_list_str += "/auto_attack";
      action_list_str += "/snapshot_stats";

      std::string special = unique_special();
      if ( ! special.empty() )
      {
        action_list_str += "/";
        action_list_str += special;
      }
      action_list_str += "/claw";
      action_list_str += "/wait_until_ready";
      use_default_action_list = true;
    }

    base_t::init_action_list();
  }

  virtual double composite_attack_power_multiplier() const
  {
    double mult = base_t::composite_attack_power_multiplier();

    // TODO pet charge should show up here.

    return mult;
  }

  virtual double composite_melee_crit() const
  {
    double ac = base_t::composite_melee_crit();
    ac += specs.spiked_collar -> effectN( 3 ).percent();
    return ac;
  }

  double focus_regen_per_second() const
  {
    return o() -> focus_regen_per_second() * 1.25;
  }

  virtual double composite_melee_speed() const
  {
    double ah = base_t::composite_melee_speed();

    ah *= 1.0 / ( 1.0 + o() -> specs.frenzy -> effectN( 1 ).percent() * buffs.frenzy -> stack() );
    ah *= 1.0 / ( 1.0 + specs.spiked_collar -> effectN( 2 ).percent() );


    return ah;
  }

  virtual void summon( timespan_t duration = timespan_t::zero() )
  {
    base_t::summon( duration );

    o() -> active.pet = this;
  }

  void stampede_summon( timespan_t duration )
  {
    if ( this == o() -> active.pet )
    {
      // The active pet does not get its damage reduced
      //buffs.stampede -> trigger( 1, -1.0, 1.0, duration );
      return;
    }

    type = PLAYER_GUARDIAN;

    for ( size_t i = 0; i < stats_list.size(); ++i )
      if ( ! ( stats_list[ i ] -> parent ) )
        o() -> stats_stampede -> add_child( stats_list[ i ] );

    base_t::summon( duration );
    // pet appears at the target
    current.distance = 0;

    buffs.stampede -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );
    
    // pet swings immediately (without an execute time)
    if ( ! main_hand_attack -> execute_event ) main_hand_attack -> execute();
  }

  virtual void demise()
  {
    base_t::demise();

    if ( o() -> active.pet == this )
      o() -> active.pet = nullptr;
  }

  virtual double composite_player_multiplier( school_e school ) const
  {
    double m = base_t::composite_player_multiplier( school );

    if ( ! buffs.stampede -> check() && buffs.bestial_wrath -> up() )
      m *= 1.0 + buffs.bestial_wrath -> data().effectN( 2 ).percent();

    if ( o() -> sets.has_set_bonus( SET_T16_4PC_MELEE ) )
      m *= 1.0 + buffs.tier16_4pc_bm_brutal_kinship -> stack() * buffs.tier16_4pc_bm_brutal_kinship -> data().effectN( 1 ).percent();

    // Pet combat experience
    if ( o() -> talents.versatility -> ok() )
      m *= 1.0 + ( specs.combat_experience -> effectN( 2 ).percent() + 0.2 );  // No spell data for the 20% from versatility.
    else
      m *= 1.0 + specs.combat_experience -> effectN( 2 ).percent();

    return m;
  }

  target_specific_t<hunter_main_pet_td_t*> target_data;

  virtual hunter_main_pet_td_t* get_target_data( player_t* target ) const
  {
    hunter_main_pet_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new hunter_main_pet_td_t( target, const_cast<hunter_main_pet_t*>(this) );
    }
    return td;
  }

  virtual resource_e primary_resource() const { return RESOURCE_FOCUS; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str );

  virtual void init_spells();
};

namespace actions {

// Template for common hunter main pet action code. See priest_action_t.
template <class Base>
struct hunter_main_pet_action_t : public hunter_pet_action_t<hunter_main_pet_t, Base>
{
private:
  typedef hunter_pet_action_t<hunter_main_pet_t, Base> ab;
public:
  typedef hunter_main_pet_action_t base_t;

  bool special_ability;

  hunter_main_pet_action_t( const std::string& n, hunter_main_pet_t* player,
                            const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, *player, s ),
    special_ability( false )
  {
    if ( ab::data().rank_str() && !strcmp( ab::data().rank_str(), "Special Ability" ) )
      special_ability = true;
  }

  hunter_main_pet_t* p() const
  { return static_cast<hunter_main_pet_t*>( ab::player ); }

  hunter_t* o() const
  { return static_cast<hunter_t*>( p() -> o() ); }

  hunter_main_pet_td_t* td( player_t* t = 0 ) const
  { return p() -> get_target_data( t ? t : ab::target ); }
};

// ==========================================================================
// Hunter Pet Attacks
// ==========================================================================

struct hunter_main_pet_attack_t : public hunter_main_pet_action_t<melee_attack_t>
{
  hunter_main_pet_attack_t( const std::string& n, hunter_main_pet_t* player,
                            const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s )
  {
    special = true;
    may_crit = true;
  }

  virtual bool ready()
  {
    // Stampede pets don't use abilities or spells
    if ( p() -> buffs.stampede -> check() )
      return false;

    return base_t::ready();
  }

  void trigger_tier16_bm_4pc_melee()
  {
    if ( o() -> specialization() != HUNTER_BEAST_MASTERY )
      return;
    if ( ! o() -> sets.has_set_bonus( SET_T16_4PC_MELEE ) )
      return;
    if ( p() -> buffs.bestial_wrath -> check() )
      p() -> buffs.tier16_4pc_bm_brutal_kinship -> trigger( 1, 0, 1, p() -> buffs.bestial_wrath -> remains() );
  }
};

// Beast Cleave

static bool trigger_beast_cleave( action_state_t* s )
{
  struct beast_cleave_attack_t : public hunter_main_pet_attack_t
  {
    beast_cleave_attack_t( hunter_main_pet_t* p ) :
      hunter_main_pet_attack_t( "beast_cleave_attack", p, p -> find_spell( 22482 ) )
    {
      may_miss = may_crit = proc = callbacks = false;
      background = true;
      school = SCHOOL_PHYSICAL;
      aoe = -1;
      // The starting damage includes all the buffs
      weapon_multiplier = 0;
    }

    size_t available_targets( std::vector< player_t* >& tl ) const
    {
      tl.clear();

      for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
      {
        player_t* t = sim -> actor_list[ i ];

        if ( ! t -> is_sleeping() && t -> is_enemy() && ( t != target ) )
          tl.push_back( t );
      }

      return tl.size();
    }
  };

  if ( ! s -> action -> result_is_hit( s -> result ) )
    return false;

  if ( s -> action -> sim -> active_enemies == 1 )
    return false;

  hunter_main_pet_t* p = debug_cast< hunter_main_pet_t* >( s -> action -> player );

  if ( ! p -> buffs.beast_cleave -> check() )
    return false;

  if ( ! p -> active.beast_cleave )
  {
    p -> active.beast_cleave = new beast_cleave_attack_t( p );
    p -> active.beast_cleave -> init();
  }

  double cleave = s -> result_total * p -> buffs.beast_cleave -> current_value;

  p -> active.beast_cleave -> base_dd_min = cleave;
  p -> active.beast_cleave -> base_dd_max = cleave;
  p -> active.beast_cleave -> execute();

  return true;
}

// Pet Melee ================================================================

struct pet_melee_t : public hunter_main_pet_attack_t
{
  pet_melee_t( hunter_main_pet_t* player ) :
    hunter_main_pet_attack_t( "melee", player )
  {
    special = false;
    weapon = &( player -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    background        = true;
    repeating         = true;
    school = SCHOOL_PHYSICAL;
  }

  virtual void impact( action_state_t* s )
  {
    hunter_main_pet_attack_t::impact( s );

    trigger_beast_cleave( s );
  }
};

// Pet Auto Attack ==========================================================

struct pet_auto_attack_t : public hunter_main_pet_attack_t
{
  pet_auto_attack_t( hunter_main_pet_t* player, const std::string& options_str ) :
    hunter_main_pet_attack_t( "auto_attack", player, 0 )
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

// Pet Claw/Bite/Smack ======================================================

struct basic_attack_t : public hunter_main_pet_attack_t
{
  double chance_invigoration;
  double gain_invigoration;

  basic_attack_t( hunter_main_pet_t* p, const std::string& name, const std::string& options_str ) :
    hunter_main_pet_attack_t( name, p, p -> find_pet_spell( name ) )
  {
    parse_options( NULL, options_str );
    school = SCHOOL_PHYSICAL;

    // hardcoded into tooltip

    attack_power_mod.direct = 0.168;
    base_multiplier *= 1.0 + p -> specs.spiked_collar -> effectN( 1 ).percent();
    chance_invigoration = p -> find_spell( 53397 ) -> proc_chance();
    gain_invigoration = p -> find_spell( 53398 ) -> effectN( 1 ).resource( RESOURCE_FOCUS );
  }

  virtual void execute()
  {
    hunter_main_pet_attack_t::execute();
    trigger_tier16_bm_4pc_melee();
    if ( p() == o() -> active.pet )
      o() -> buffs.cobra_strikes -> decrement();

    if ( o() -> specs.frenzy -> ok() )
      p() -> buffs.frenzy -> trigger( 1 );
  }

  virtual void impact( action_state_t* s )
  {
    hunter_main_pet_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> buffs.enhanced_basic_attacks -> expire();
      if ( o() -> specs.invigoration -> ok() && rng().roll( chance_invigoration ) )
      {
        o() -> resource_gain( RESOURCE_FOCUS, gain_invigoration, o() -> gains.invigoration );
        o() -> procs.invigoration -> occur();
      }
      trigger_beast_cleave( s );
     if ( o() -> rng().roll ( o() -> perks.enhanced_basic_attacks -> effectN( 1 ).percent() ) )
     {
       cooldown -> reset( true );
       p() -> buffs.enhanced_basic_attacks -> trigger();
     }
    }
  }

  virtual double composite_crit() const
  {
    double cc = hunter_main_pet_attack_t::composite_crit();

    if ( o() -> buffs.cobra_strikes -> up() && p() == o() -> active.pet )
      cc += o() -> buffs.cobra_strikes -> data().effectN( 1 ).percent();

    return cc;
  }

  bool use_wild_hunt() const
  {
    // comment out to avoid procs
    return p() -> resources.current[ RESOURCE_FOCUS ] > 50;
  }

  virtual double action_multiplier() const
  {
    double am = hunter_main_pet_attack_t::action_multiplier();

    if ( o() -> talents.blink_strikes -> ok() && p() == o() -> active.pet )
      am *= 1.0 + o() -> talents.blink_strikes -> effectN( 1 ).percent();

    if ( use_wild_hunt() )
    {
      p() -> benefits.wild_hunt -> update( true );
      am *= 1.0 + p() -> specs.wild_hunt -> effectN( 1 ).percent();
    }
    else
      p() -> benefits.wild_hunt -> update( false );

    return am;
  }

  virtual double cost() const
  {
    double c = hunter_main_pet_attack_t::cost();

    if ( use_wild_hunt() )
      c *= 1.0 + p() -> specs.wild_hunt -> effectN( 2 ).percent();

    if ( p() -> buffs.enhanced_basic_attacks -> up() )
      c = 0;

    return c;
  }

};

// Devilsaur Monstrous Bite =================================================

struct monstrous_bite_t : public hunter_main_pet_attack_t
{
  monstrous_bite_t( hunter_main_pet_t* p, const std::string& options_str ) :
    hunter_main_pet_attack_t( "monstrous_bite", p, p -> find_spell( 54680 ) )
  {
    parse_options( NULL, options_str );
    school = SCHOOL_PHYSICAL;
    stats -> school = SCHOOL_PHYSICAL;
  }
};


// Kill Command (pet) =======================================================

struct kill_command_t : public hunter_main_pet_attack_t
{
  kill_command_t( hunter_main_pet_t* p ) :
    hunter_main_pet_attack_t( "kill_command", p, p -> find_spell( 83381 ) )
  {
    background = true;
    proc = true;
    school = SCHOOL_PHYSICAL;

    attack_power_mod.direct  = 1.576 * 1.5; // Hard-coded in tooltip.
    attack_power_mod.direct *= 1.0 + o() -> perks.improved_kill_command -> effectN( 1 ).percent();
  }

  virtual double action_multiplier() const
  {
    double am = hunter_main_pet_attack_t::action_multiplier();
    am *= 1.0 + o() -> sets.set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();
    return am;
  }
};

// ==========================================================================
// Hunter Pet Spells
// ==========================================================================

struct hunter_main_pet_spell_t : public hunter_main_pet_action_t<spell_t>
{
  hunter_main_pet_spell_t( const std::string& n, hunter_main_pet_t* player,
                           const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s )
  {
  }

  virtual bool ready()
  {
    // Stampede pets don't use abilities or spells
    if ( p() -> buffs.stampede -> check() )
      return false;

    return base_t::ready();
  }
};


// ==========================================================================
// Unique Pet Specials
// ==========================================================================

// Wolf/Devilsaur Furious Howl ==============================================

struct furious_howl_t : public hunter_main_pet_spell_t
{
  furious_howl_t( hunter_main_pet_t* player, const std::string& options_str ) :
    hunter_main_pet_spell_t( "furious_howl", player, player -> find_pet_spell( "Furious Howl" ) )
  {

    parse_options( NULL, options_str );

    harmful = false;
    background = ( sim -> overrides.critical_strike != 0 );
  }

  virtual void execute()
  {
    hunter_main_pet_spell_t::execute();

    if ( ! sim -> overrides.critical_strike )
      sim -> auras.critical_strike -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );
  }
};

// Cat/Spirit Beast Roar of Courage =========================================

struct roar_of_courage_t : public hunter_main_pet_spell_t
{
  roar_of_courage_t( hunter_main_pet_t* player, const std::string& options_str ) :
    hunter_main_pet_spell_t( "roar_of_courage", player, player -> find_pet_spell( "Roar of Courage" ) )
  {

    parse_options( NULL, options_str );

    harmful = false;
    background = ( sim -> overrides.str_agi_int != 0 );
  }

  virtual void execute()
  {
    hunter_main_pet_spell_t::execute();
    double mastery_rating = data().effectN( 1 ).average( player );
    if ( ! sim -> overrides.mastery )
      sim -> auras.mastery -> trigger( 1, mastery_rating, -1.0, data().duration() );
  }
};

// Silithid Qiraji Fortitude  ===============================================

struct qiraji_fortitude_t : public hunter_main_pet_spell_t
{
  qiraji_fortitude_t( hunter_main_pet_t* player, const std::string& options_str ) :
    hunter_main_pet_spell_t( "qiraji_fortitude", player, player -> find_pet_spell( "Qiraji Fortitude" ) )
  {

    parse_options( NULL, options_str );

    harmful = false;
    background = ( sim -> overrides.stamina != 0 );
  }

  virtual void execute()
  {
    hunter_main_pet_spell_t::execute();

    if ( ! sim -> overrides.stamina )
      sim -> auras.stamina -> trigger();
  }
};

// Hyena Cackling Howl ======================================================

// TODO add attack speed to hyena
struct cackling_howl_t : public hunter_main_pet_spell_t
{
  cackling_howl_t( hunter_main_pet_t* player, const std::string& options_str ) :
    hunter_main_pet_spell_t( "cackling_howl", player, player -> find_pet_spell( "Cackling Howl" ) )
  {
    parse_options( 0, options_str );

  }
};

// Chimera Froststorm Breath ================================================

struct froststorm_breath_t : public hunter_main_pet_spell_t
{
  struct froststorm_breath_tick_t : public hunter_main_pet_spell_t
  {
    froststorm_breath_tick_t( hunter_main_pet_t* player ) :
      hunter_main_pet_spell_t( "froststorm_breath_tick", player, player -> find_spell( 95725 ) )
    {
      attack_power_mod.direct = 0.144; // hardcoded into tooltip, 29/08/2012
      background  = true;
      direct_tick = true;
    }
  };

  froststorm_breath_tick_t* tick_spell;

  froststorm_breath_t( hunter_main_pet_t* player, const std::string& options_str ) :
    hunter_main_pet_spell_t( "froststorm_breath", player, player -> find_pet_spell( "Froststorm Breath" ) )
  {
    channeled = true;

    parse_options( NULL, options_str );


    tick_spell = new froststorm_breath_tick_t( player );

    add_child( tick_spell );
  }

  virtual void init()
  {
    hunter_main_pet_spell_t::init();

    tick_spell -> stats = stats;
  }

  virtual void tick( dot_t* d )
  {
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick, d -> state -> target );
  }
};

} // end namespace pets::actions


hunter_main_pet_td_t::hunter_main_pet_td_t( player_t* target, hunter_main_pet_t* p ) :
  actor_pair_t( target, p )
{
}
// hunter_pet_t::create_action ==============================================

action_t* hunter_main_pet_t::create_action( const std::string& name,
                                            const std::string& options_str )
{
  using namespace actions;

  if ( name == "auto_attack"           ) return new      pet_auto_attack_t( this, options_str );
  if ( name == "claw"                  ) return new         basic_attack_t( this, "Claw", options_str );
  if ( name == "bite"                  ) return new         basic_attack_t( this, "Bite", options_str );
  if ( name == "smack"                 ) return new         basic_attack_t( this, "Smack", options_str );
  if ( name == "froststorm_breath"     ) return new    froststorm_breath_t( this, options_str );
  if ( name == "furious_howl"          ) return new         furious_howl_t( this, options_str );
  if ( name == "roar_of_courage"       ) return new      roar_of_courage_t( this, options_str );
  if ( name == "qiraji_fortitude"      ) return new     qiraji_fortitude_t( this, options_str );
  if ( name == "monstrous_bite"        ) return new       monstrous_bite_t( this, options_str );
  if ( name == "cackling_howl"         ) return new        cackling_howl_t( this, options_str );

  return base_t::create_action( name, options_str );
}
// hunter_t::init_spells ====================================================

void hunter_main_pet_t::init_spells()
{
  base_t::init_spells();

  // ferocity
  active.kill_command = new actions::kill_command_t( this );

  specs.hearth_of_the_phoenix = spell_data_t::not_found();
  specs.spiked_collar = spell_data_t::not_found();
  // tenacity
  specs.last_stand = spell_data_t::not_found();
  specs.charge = spell_data_t::not_found();
  specs.thunderstomp = spell_data_t::not_found();
  specs.blood_of_the_rhino = spell_data_t::not_found();
  specs.great_stamina = spell_data_t::not_found();
  // cunning
  specs.roar_of_sacrifice = spell_data_t::not_found();
  specs.bullhead = spell_data_t::not_found();
  specs.cornered = spell_data_t::not_found();
  specs.boars_speed = spell_data_t::not_found();

  specs.dash = spell_data_t::not_found(); // ferocity, cunning

  // ferocity
  specs.spiked_collar    = find_specialization_spell( "Spiked Collar" );

  specs.wild_hunt = find_spell( 62762 );
  specs.combat_experience = find_specialization_spell( "Combat Experience" );
}

// ==========================================================================
// Dire Critter
// ==========================================================================

struct dire_critter_t : public hunter_pet_t
{
  struct melee_t : public hunter_pet_action_t<dire_critter_t, melee_attack_t>
  {
    int focus_gain;

    melee_t( dire_critter_t& p ) :
      base_t( "dire_beast_melee", p )
    {
      if ( p.o() -> pet_dire_beasts[ 0 ] )
        stats = p.o() -> pet_dire_beasts[ 0 ] -> get_stats( "dire_beast_melee" );

      weapon = &( player -> main_hand_weapon );
      weapon_multiplier = 0;
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = player -> dbc.spell_scaling( p.o() -> type, p.o() -> level );
      attack_power_mod.direct = 0.2875;
      school = SCHOOL_PHYSICAL;

      trigger_gcd = timespan_t::zero();

      background = true;
      repeating  = true;
      special    = false;
      may_glance = true;
      may_crit   = true;

      focus_gain = player -> find_spell( 120694 ) -> effectN( 1 ).base_value();
    }

    virtual void impact( action_state_t* s )
    {
      melee_attack_t::impact( s );

      if ( result_is_hit( s -> result ) )
        p() -> o() -> resource_gain( RESOURCE_FOCUS, focus_gain, p() -> o() -> gains.dire_beast );
    }
  };

  dire_critter_t( hunter_t& owner, size_t index ) :
    hunter_pet_t( *owner.sim, owner, std::string( "dire_beast_" ) + util::to_string( index ), PET_HUNTER, true /*GUARDIAN*/ )
  {
    owner_coeff.ap_from_ap = 1.0;
  }

  virtual void init_base_stats()
  {
    hunter_pet_t::init_base_stats();

    // FIXME numbers are from treant. correct them.
    resources.base[ RESOURCE_HEALTH ] = 9999; // Level 85 value
    resources.base[ RESOURCE_MANA   ] = 0;

    stamina_per_owner = 0;

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, o() -> level );
    main_hand_weapon.max_dmg    = main_hand_weapon.min_dmg;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2 );

    main_hand_attack = new melee_t( *this );
  }

  virtual void summon( timespan_t duration = timespan_t::zero() )
  {
    hunter_pet_t::summon( duration );

    // attack immediately on summons
    main_hand_attack -> execute();
  }
};

// ==========================================================================
// Tier 15 2-piece bonus temporary pet
// ==========================================================================

struct tier15_thunderhawk_t : public hunter_pet_t
{
  tier15_thunderhawk_t( hunter_t& owner ) :
    hunter_pet_t( *owner.sim, owner, "tier15_thunderhawk", PET_HUNTER, true /*GUARDIAN*/ )
  {
  }

  virtual void init_base_stats()
  {
    hunter_pet_t::init_base_stats();

    action_list_str = "lightning_blast";
  }

  struct lightning_blast_t : public hunter_pet_action_t<tier15_thunderhawk_t, spell_t>
  {
    lightning_blast_t( tier15_thunderhawk_t& p ):
      base_t( "lightning_blast", p, p.find_spell( 138374 ) )
    {
      may_crit = true;
      base_costs[ RESOURCE_MANA ] = 0;
    }

    virtual double composite_haste() const { return 1.0; }
  };

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "lightning_blast" ) return new lightning_blast_t( *this );

    return hunter_pet_t::create_action( name, options_str );
  }
};

} // end namespace pets

namespace attacks {

// ==========================================================================
// Hunter Attack
// ==========================================================================

struct hunter_ranged_attack_t : public hunter_action_t<ranged_attack_t>
{
  bool can_trigger_wild_quiver;

  hunter_ranged_attack_t( const std::string& n, hunter_t* player,
                          const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s ),
    can_trigger_wild_quiver( true )
  {
    if ( player -> main_hand_weapon.type == WEAPON_NONE )
    {
      background = true;
    }

    special                = true;
    may_crit               = true;
    may_parry              = false;
    may_block              = false;
    tick_may_crit          = true;
    dot_behavior           = DOT_REFRESH;
  }

  virtual void init()
  {
    if ( player -> main_hand_weapon.group() != WEAPON_RANGED )
    {
      background = true;
    }

    base_t::init();
  }

  virtual void trigger_wild_quiver( double multiplier = 1.0 )
  {
    if ( ! p() -> active.wild_quiver_shot )
      return;

    if ( ! can_trigger_wild_quiver )
      return;

    double chance = p() -> mastery.wild_quiver -> ok() ? multiplier * p() -> cache.mastery_value() : 0.0;
    if ( rng().roll( chance ) )
    {
      p() -> active.wild_quiver_shot -> execute();
      p() -> procs.wild_quiver -> occur();
    }
  }

  virtual void execute()
  {
    ranged_attack_t::execute();

    trigger_thrill_of_the_hunt();

    trigger_tier16_bm_4pc_melee();

    if ( result_is_hit( execute_state -> result ) )
      trigger_wild_quiver();
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = base_t::execute_time();

    if ( t == timespan_t::zero()  || base_execute_time < timespan_t::zero() )
      return timespan_t::zero();

    return t;
  }

  void trigger_go_for_the_throat()
  {
    if ( ! p() -> specs.go_for_the_throat -> ok() )
      return;

    if ( ! p() -> active.pet )
      return;

    int gain = p() -> specs.go_for_the_throat -> effectN( 1 ).trigger() -> effectN( 1 ).base_value();
    p() -> active.pet -> resource_gain( RESOURCE_FOCUS, gain, p() -> active.pet -> gains.go_for_the_throat );
  }

  void trigger_tier15_2pc_melee()
  {
    if ( ! p() -> sets.has_set_bonus( SET_T15_2PC_MELEE ) )
      return;

    if ( ( p() -> ppm_tier15_2pc_melee.trigger() ) )
    {
      p() -> procs.tier15_2pc_melee -> occur();
      size_t i;

      for ( i = 0; i < p() -> thunderhawk.size(); i++ )
      {
        if ( ! p() -> thunderhawk[ i ] -> is_sleeping() )
          continue;

        p() -> thunderhawk[ i ] -> summon( timespan_t::from_seconds( 10 ) );
        break;
      }

      assert( i < p() -> thunderhawk.size() );
    }
  }

  void trigger_tier15_4pc_melee( proc_t* proc, attack_t* attack );

};

// Ranged Attack ============================================================

struct ranged_t : public hunter_ranged_attack_t
{
  ranged_t( hunter_t* player, const char* name = "ranged", const spell_data_t* s = spell_data_t::nil() ) :
    hunter_ranged_attack_t( name, player, s )
  {
    school = SCHOOL_PHYSICAL;
    weapon = &( player -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;

    background  = true;
    repeating   = true;
    special     = false;
  }

  virtual timespan_t execute_time() const
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return hunter_ranged_attack_t::execute_time();
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( s -> result == RESULT_CRIT )
        trigger_go_for_the_throat();
    }
  }
};

// Auto Shot ================================================================

struct auto_shot_t : public ranged_t
{
  auto_shot_t( hunter_t* p ) : ranged_t( p, "auto_shot", p -> find_class_spell( "Auto Shot" ) )
  {
    school = SCHOOL_PHYSICAL;
    if ( p -> talents.flaming_shots -> ok() )
      school = SCHOOL_FIRE;
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

};

// Aimed Shot ===============================================================

struct aimed_shot_t : public hunter_ranged_attack_t
{
  aimed_shot_t( hunter_t* p, const std::string& options_str ) :
    hunter_ranged_attack_t( "aimed_shot", p, p -> find_class_spell( "Aimed Shot" ) )
  {
    check_spec( HUNTER_MARKSMANSHIP );

    parse_options( NULL, options_str );

    base_multiplier *= 1.0 + p -> perks.improved_aimed_shot -> effectN( 1 ).percent();
  }
  
  virtual double cost() const
  {
   return thrill_discount( hunter_ranged_attack_t::cost() );
  }

  virtual timespan_t execute_time() const
  {
    timespan_t cast_time = hunter_ranged_attack_t::execute_time();
    if ( p() -> buffs.tier16_4pc_mm_keen_eye -> check() )
      cast_time *= 1.0 + p() -> buffs.tier16_4pc_mm_keen_eye -> data().effectN( 1 ).percent();

    return cast_time;
  }
  
  virtual double composite_target_crit( player_t* t ) const
  {
    double cc = hunter_ranged_attack_t::composite_target_crit( t );
    cc += p() -> careful_aim_crit( t );
    return cc;
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( s -> result == RESULT_CRIT )
    {
      p() -> resource_gain( RESOURCE_FOCUS, 
        p() -> perks.enhanced_aimed_shot -> effectN( 1 ).resource( RESOURCE_FOCUS ),
        p() -> gains.aimed_shot );
    }
  }

  virtual void execute()
  {
    hunter_ranged_attack_t::execute();
    consume_thrill_of_the_hunt();
    if ( p() -> buffs.tier16_4pc_mm_keen_eye -> up() )
      p() -> buffs.tier16_4pc_mm_keen_eye -> expire();
  }
};

// Glaive Toss Attack =======================================================

struct glaive_toss_strike_t : public ranged_attack_t
{
  // use ranged_attack_to to avoid triggering other hunter behaviors (like thrill of the hunt
  // TotH should be triggered by the glaive toss itself.
  glaive_toss_strike_t( hunter_t* player ) :
    ranged_attack_t( "glaive_toss_strike", player, player -> find_spell( 120761 ) )
  {
    repeating   = false;
    background = true;
    dual = true;
    may_crit = true;    special = true;
    travel_speed = player -> talents.glaive_toss -> effectN( 3 ).trigger() -> missile_speed();

    // any attack with the weapon may trigger wild_quiver, but don't actually include weapon damage
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier = 0;


    aoe = -1;
  }

  virtual double composite_target_multiplier( player_t* target ) const
  {
    double m = ranged_attack_t::composite_target_multiplier( target );

    if ( target == this -> target )
      m *= static_cast<hunter_t*>( player ) -> talents.glaive_toss -> effectN( 1 ).base_value();

    return m;
  }
};

struct glaive_toss_t : public hunter_ranged_attack_t
{
  glaive_toss_strike_t* primary_strike;

  glaive_toss_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "glaive_toss", player, player -> talents.glaive_toss ),
    primary_strike( new glaive_toss_strike_t( p() ) )
  {
    parse_options( NULL, options_str );
    may_miss = false;
    may_crit = false;
    school = SCHOOL_PHYSICAL;

    //add_child( primary_strike );
    primary_strike -> stats = stats;
    // FIXME add different attacks and stats for each glaive
    // suppress ticks, since they are only for the slow effect
    dot_duration = timespan_t::zero();
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

// Powershot Attack =========================================================

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

  virtual double action_multiplier() const
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    // for primary target
    am *= 2.0;  // from the tooltip

    return am;
  }

};

// Black Arrow ==============================================================

struct black_arrow_t : public hunter_ranged_attack_t
{
  double ticks_since_lnl;
  black_arrow_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "black_arrow", player, player -> find_class_spell( "Black Arrow" ) ),
    ticks_since_lnl ( 0 )
  {
    parse_options( NULL, options_str );

    cooldown -> duration += p() -> specs.trap_mastery -> effectN( 4 ).time_value();
    base_multiplier *= 1.0 + p() -> specs.trap_mastery -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p() -> perks.improved_black_arrow -> effectN( 1 ).percent();
  }

  virtual bool ready()
  {
    if ( cooldown -> up() && ! p() -> resource_available( RESOURCE_FOCUS, cost() ) )
    {
      if ( sim -> log ) sim -> out_log.printf( "Player %s was focus starved when Black Arrow was ready.", p() -> name() );
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
      ticks_since_lnl = 0;
    }
    else if ( ticks_since_lnl == 9 ) // This is actually the 10th tick without a proc, so force a proc. 
    {
      p() -> buffs.lock_and_load -> trigger( 2, buff_t::DEFAULT_VALUE(), 1.0 );
      p() -> procs.lock_and_load -> occur();
      p() -> cooldowns.explosive_shot -> reset( true );
      ticks_since_lnl = 0;
    }
    else
     ticks_since_lnl++;

  }

};

// Explosive Trap ==============================================================

struct explosive_trap_tick_t : public hunter_ranged_attack_t
{
  explosive_trap_tick_t( hunter_t* player, const std::string& name ) :
    hunter_ranged_attack_t( name , player, player -> find_spell( 13812 ) )
  {
    aoe           = -1;
    background    = true;
    direct_tick   = true;
    base_multiplier *= 1.0 + p() -> specs.trap_mastery -> effectN( 2 ).percent();
  }
};

struct explosive_trap_t : public hunter_ranged_attack_t
{
  attack_t* explosive_trap_tick;
  explosive_trap_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "explosive_trap", player, player -> find_class_spell( "Explosive Trap" ) ),
    explosive_trap_tick( new explosive_trap_tick_t( player, "explosive_trap_tick" ) )
  {
    parse_options( NULL, options_str );
    
    cooldown -> duration = data().cooldown();
    cooldown -> duration += p() -> specs.trap_mastery -> effectN( 4 ).time_value();
    if ( p() -> perks.enhanced_traps -> ok() )
      cooldown -> duration *= (1.0 + p() -> perks.enhanced_traps -> effectN( 1 ).percent() );

    tick_zero     = true;
    hasted_ticks  = false;
    harmful       = false;
    dot_duration  = p() -> find_spell( 13812 ) -> duration();
    base_tick_time = p() -> find_spell( 13812 ) -> effectN( 2 ).period();
    add_child( explosive_trap_tick );
  }

  virtual void tick( dot_t* d )
  {
    hunter_ranged_attack_t::tick( d );

    explosive_trap_tick -> execute();
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

  virtual double action_multiplier() const
  {
    double am = hunter_ranged_attack_t::action_multiplier();
    am *= 1.0 + p() -> sets.set( SET_T14_2PC_MELEE ) -> effectN( 2 ).percent();
    return am;
  }

  virtual double cost() const
  {
    double cost = hunter_ranged_attack_t::cost();
    cost += p() -> perks.enhanced_chimera_shot -> effectN( 1 ).base_value();
    return cost;
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
    if ( p() -> talents.focusing_shot -> ok() )
      background = true;
    focus_gain = p() -> dbc.spell( 91954 ) -> effectN( 1 ).base_value();

    // Needs testing
    if ( p() -> sets.has_set_bonus( SET_T13_2PC_MELEE ) )
      focus_gain *= 2.0;
    base_multiplier *= 1 + p() -> perks.improved_cobra_shot -> effectN ( 1 ).percent();
  }

  virtual bool usable_moving() const
  {
    return true;
  }

  virtual void execute()
  {
    hunter_ranged_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
      trigger_tier15_2pc_melee();
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      //Check later to see if they remove cobra shot extending serpent sting. Alex - 5-12-14
      double focus = focus_gain;
      p() -> resource_gain( RESOURCE_FOCUS, focus, p() -> gains.cobra_shot );
    }
  }
};

// Explosive Shot ===========================================================

struct explosive_shot_tick_t : public residual_action::residual_periodic_action_t< attack_t >
{
  explosive_shot_tick_t( hunter_t* p ) :
    base_t( "explosive_shot_tick", p, p -> specs.explosive_shot )
  {
    tick_may_crit = true;
    dual = true;

    // suppress direct damage in the dot.
    base_dd_min = base_dd_max = 0;

    if ( p -> perks.empowered_explosive_shot -> ok() ) 
      dot_duration += p -> perks.empowered_explosive_shot -> effectN( 1 ).time_value();
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
  double tick_count;

  explosive_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "explosive_shot", player, player -> specs.explosive_shot )
  {
    parse_options( NULL, options_str );
    may_block = false;

    attack_power_mod.tick = player -> specs.explosive_shot -> effectN( 1 ).ap_coeff();
    attack_power_mod.direct = attack_power_mod.tick;

    // the inital impact is not part of the rolling dot
    dot_duration = timespan_t::zero();

    tick_count = player -> active.explosive_ticks -> dot_duration.total_seconds();
  }

  void init()
  {
    hunter_ranged_attack_t::init();

    assert( p() -> active.explosive_ticks );

    p() -> active.explosive_ticks -> stats = stats;

    stats -> action_list.push_back( p() -> active.explosive_ticks );
  }

  virtual double cost() const
  {
    if ( p() -> buffs.lock_and_load -> check() )
      return 0;

    return hunter_ranged_attack_t::cost();
  }

  virtual bool ready()
  {
    if ( cooldown -> up() && ! p() -> resource_available( RESOURCE_FOCUS, cost() ) )
    {
      if ( sim -> log ) sim -> out_log.printf( "Player %s was focus starved when Explosive Shot was ready.", p() -> name() );
      p() -> procs.explosive_shot_focus_starved -> occur();
    }

    return hunter_ranged_attack_t::ready();
  }

  virtual void update_ready( timespan_t cd_override )
  {
    cd_override = ( p() -> buffs.lock_and_load -> check() ? timespan_t::zero() : timespan_t::min() );

    hunter_ranged_attack_t::update_ready( cd_override );
  }

  virtual double action_multiplier() const
  {
    double am = hunter_ranged_attack_t::action_multiplier();
    am *= 1.0 + p() -> sets.set( SET_T14_2PC_MELEE ) -> effectN( 3 ).percent();
    return am;
  }

  virtual void execute()
  {
    hunter_ranged_attack_t::execute();
    p() -> buffs.lock_and_load -> decrement();
    // Does saving the proc require round trip latency before we know?
    // TODO add reaction time for the continuation of the proc?
    if ( p() -> buffs.lock_and_load -> up()
         && rng().roll( p() -> sets.set( SET_T16_4PC_MELEE ) -> effectN( 1 ).percent() ) )
    {
      p() -> procs.tier16_4pc_melee -> occur();
    }
    else
    {
      p() -> buffs.lock_and_load -> decrement();
    }
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      // Force damage calculation to be hit based always
      result_e r = s -> result;
      s -> result = RESULT_HIT;
      double damage = calculate_tick_amount( s , 1.0);
      s -> result = r;

      residual_action::trigger(
        p() -> active.explosive_ticks, // ignite spell
        s -> target,                   // target
        damage * tick_count);
    }
  }
};

// Kill Shot ================================================================

struct kill_shot_t : public hunter_ranged_attack_t
{
  cooldown_t* cd_glyph_kill_shot;

  kill_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "kill_shot", player, player -> find_class_spell( "Kill Shot" ) ),
    cd_glyph_kill_shot( player -> get_cooldown( "cooldowns.glyph_kill_shot" ) )
  {
    parse_options( NULL, options_str );

    cd_glyph_kill_shot -> duration = player -> dbc.spell( 90967 ) -> duration();
  }

  virtual void execute()
  {
    hunter_ranged_attack_t::execute();

    if ( cd_glyph_kill_shot -> up() )
    {
      cooldown -> reset( false );
      cd_glyph_kill_shot -> start();
    }
  }

  virtual bool ready()
  {
    if ( target -> health_percentage() > ( p() -> perks.enhanced_kill_shot ? 35 : 20 ) )
      return false;

    return hunter_ranged_attack_t::ready();
  }
};

// Serpent Sting Attack =====================================================

struct serpent_sting_t : public hunter_ranged_attack_t
{
  serpent_sting_t( hunter_t* player ) : //find_specialization_spell( "Serpent Sting" ) returns the passive, which doesn't have a trigger.
    hunter_ranged_attack_t( "serpent_sting", player,  player -> find_spell( 118253 ) )
  {
    background = true;
    proc = true;
    tick_may_crit = true;
    hasted_ticks = false;
    tick_zero = true;
    dot_behavior = DOT_CLIP; // Serpent sting deals immediate damage every time it is cast, and the next tick is reset.
  }

  virtual void tick( dot_t* d )
  {
    hunter_ranged_attack_t::tick( d );

    if ( p() -> specs.viper_venom -> ok() && p() -> cooldowns.viper_venom -> up() )
    {
      double focus_gain = p() -> specs.viper_venom -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() + 
                          p() -> perks.improved_viper_venom -> effectN( 1 ).base_value();
      p() -> resource_gain( RESOURCE_FOCUS, focus_gain, p() -> gains.viper_venom );
      p() -> cooldowns.viper_venom -> start();
    }
  }
};

// Arcane Shot Attack =======================================================

struct arcane_shot_t : public hunter_ranged_attack_t
{
  arcane_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "arcane_shot", player, player -> find_class_spell( "Arcane Shot" ) )
  {
    parse_options( NULL, options_str );

    base_multiplier *= 1.0 + p() -> perks.improved_arcane_shot -> effectN( 1 ).percent();
  }
  
  virtual double cost() const
  {
    return thrill_discount( hunter_ranged_attack_t::cost() );
  }

  virtual void execute()
  {
    hunter_ranged_attack_t::execute();
    consume_thrill_of_the_hunt();

    if ( result_is_hit( execute_state -> result ) ) {
      trigger_tier15_4pc_melee( p() -> procs.tier15_4pc_melee_arcane_shot, p() -> action_lightning_arrow_arcane_shot );
    }
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> buffs.cobra_strikes -> trigger( 2 );
      if ( p() -> specs.serpent_sting -> ok() )
      {
        p() -> active.serpent_sting -> target = s -> target;
        p() -> active.serpent_sting -> execute();
      }
      p() -> buffs.tier13_4pc -> trigger();
    }
  }
};


// Multi Shot Attack ========================================================

struct multi_shot_t : public hunter_ranged_attack_t
{
  multi_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "multi_shot", player, player -> find_class_spell( "Multi-Shot" ) )
  {
    parse_options( NULL, options_str );

    aoe = -1;
  }

  virtual double cost() const
  {
    double cost = hunter_ranged_attack_t::cost();

    cost = thrill_discount( cost );

    if ( p() -> buffs.bombardment -> check() )
      cost += 1 + p() -> buffs.bombardment -> data().effectN( 1 ).base_value();

    return cost;
  }

  virtual double action_multiplier() const
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

    pets::hunter_main_pet_t* pet = p() -> active.pet;
    if ( pet && p() -> specs.beast_cleave -> ok() )
      pet -> buffs.beast_cleave -> trigger();

    if ( result_is_hit( execute_state -> result ) ) {
      trigger_tier15_4pc_melee( p() -> procs.tier15_4pc_melee_multi_shot, p() -> action_lightning_arrow_multi_shot );
    }
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> specs.serpent_sting -> ok() )
      {
        p() -> active.serpent_sting -> target = s -> target;
        p() -> active.serpent_sting -> execute();
      }
      if ( s -> result == RESULT_CRIT && p() -> specs.bombardment -> ok() )
        p() -> buffs.bombardment -> trigger();
    }
  }
};

// Focusing Shot ============================================================

struct focusing_shot_t : public hunter_ranged_attack_t
{
  double focus_gain;

  focusing_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "focusing_shot", player, player -> talents.focusing_shot )
  {
    parse_options( NULL, options_str );

    focus_gain = data().effectN( 2 ).base_value();
  }

  virtual void execute()
  {
    hunter_ranged_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
      p() -> resource_gain( RESOURCE_FOCUS, focus_gain, p() -> gains.focusing_shot );
  }
};

// Steady Shot Attack =======================================================

struct steady_shot_t : public hunter_ranged_attack_t
{
  double focus_gain;

  steady_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "steady_shot", player, player -> find_class_spell( "Steady Shot" ) )
  {
    parse_options( NULL, options_str );
    if ( p() -> talents.focusing_shot -> ok() )
      background = true;

    focus_gain = p() -> dbc.spell( 77443 ) -> effectN( 1 ).base_value();

    // Needs testing
    if ( p() -> sets.has_set_bonus( SET_T13_2PC_MELEE ) )
      focus_gain *= 2.0;

    base_multiplier *= 1.0 + p() -> perks.improved_steady_shot -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    hunter_ranged_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      trigger_tier15_2pc_melee();

      p() -> resource_gain( RESOURCE_FOCUS, focus_gain, p() -> gains.steady_shot );
    }
  }

  virtual bool usable_moving() const
  {
    return true;
  }

  virtual double composite_target_crit( player_t* t ) const
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
    repeating = false;
    proc = true;
  }

  virtual void trigger_wild_quiver( double /*multiplier = 1.0*/ )
  {
    // suppress recursive wild quiver
  }
};

// Lightning Arrow (Tier 15 4-piece bonus) ==================================

struct lightning_arrow_t : public hunter_ranged_attack_t
{
  lightning_arrow_t( hunter_t* p, const std::string& attack_suffix ) :
    hunter_ranged_attack_t( "lightning_arrow" + attack_suffix, p, p -> find_spell( 138366 ) )
  {
    school = SCHOOL_NATURE;
    proc = true;
    background = true;
    can_trigger_wild_quiver = false;
  }
};

void hunter_ranged_attack_t::trigger_tier15_4pc_melee( proc_t* proc, attack_t* attack )
{

  if ( ! p() -> sets.has_set_bonus( SET_T15_4PC_MELEE ) )
    return;

  if ( p() -> ppm_tier15_4pc_melee.trigger() )
  {
    proc -> occur();
    attack -> execute();
  }
}

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

  virtual timespan_t gcd() const
  {
    if ( ! harmful && ! player -> in_combat )
      return timespan_t::zero();

    // Hunter gcd unaffected by haste
    return trigger_gcd;
  }
};


struct aspect_of_the_pack_t : public hunter_spell_t
{
  std::string toggle;
  bool tog;
  aspect_of_the_pack_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "aspect_of_the_pack", player, player -> find_class_spell( "Aspect of the Pack" ) ),
    toggle( "" ), tog( true )
  {
    option_t options[] =
    {
      opt_string( "toggle", toggle ),
      opt_null()
    };

    parse_options( options, options_str );

    if ( !toggle.empty() )
    {
      if ( toggle == "on" )
        tog = true;
      else if ( toggle == "off" )
        tog = false;
    }

    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {

    if ( tog == true )
      for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
      {
      player_t* p = sim -> player_non_sleeping_list[i];
      if ( p -> is_enemy() || p -> type == PLAYER_GUARDIAN )
      break;
      p -> buffs.stampeding_roar -> trigger();
      }

    if ( tog == false )
      for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
      {
      player_t* p = sim -> player_non_sleeping_list[i];
      if ( p -> is_enemy() || p -> type == PLAYER_GUARDIAN )
      break;
      p -> buffs.stampeding_roar -> expire();
      }
  }

  virtual bool ready()
  {
    if ( ( p() -> buffs.aspect_of_the_pack -> check() && tog == true ) ||
       ( !p() -> buffs.aspect_of_the_pack -> check() && tog == false ) )
      return false;

    return hunter_spell_t::ready();
  }
};
// Barrage ==================================================================

// This is a spell because that's the only way to support "channeled" effects
struct barrage_t : public hunter_spell_t
{
  struct barrage_damage_t : public attacks::hunter_ranged_attack_t
  {
    barrage_damage_t( hunter_t* player ) :
      attacks::hunter_ranged_attack_t( "barrage_primary", player, player -> talents.barrage -> effectN( 2 ).trigger() )
    {
      background = may_crit = true;
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;

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

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    hunter_spell_t::schedule_execute( state );

    // To suppress autoshot, just delay it by the execute time of the barrage
    if ( p() -> main_hand_attack && p() -> main_hand_attack -> execute_event )
    {
      timespan_t time_to_next_hit = p() -> main_hand_attack -> execute_event -> remains();
      time_to_next_hit += dot_duration;
      p() -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
    }
    trigger_tier16_bm_4pc_melee();
  }

  virtual double composite_crit() const
  {
    return p() -> composite_melee_crit(); // Since barrage is a spell, we need to explicitly make it use attack crit.
  }

  virtual bool usable_moving() const
  {
    return true;
  }
};

// A Murder of Crows ========================================================

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
      travel_speed = 0.0;

      attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
      attack_power_mod.tick   = attack_power_mod.direct;
    }
  };

  moc_t( hunter_t* player, const std::string& options_str ) :
    ranged_attack_t( "a_murder_of_crows", player, player -> talents.a_murder_of_crows )
  {
    parse_options( NULL, options_str );

    hasted_ticks = false;
    may_crit = false;
    may_miss = false;

    dynamic_tick_action = true;
    tick_action = new peck_t( player );
  }

  hunter_t* p() const { return static_cast<hunter_t*>( player ); }

  virtual double action_multiplier() const
  {
    double am = ranged_attack_t::action_multiplier();
    am *= p() -> beast_multiplier();
    return am;
  }

  virtual double cost() const
  {
    double c = ranged_attack_t::cost();
    if ( p() -> buffs.beast_within -> check() )
      c *= ( 1.0 + p() -> buffs.beast_within -> data().effectN( 1 ).percent() );
    return c;
  }

  virtual void execute()
  {
    trigger_tier16_bm_4pc_brutal_kinship( p() );

    cooldown -> duration = data().cooldown();

    if ( target -> health_percentage() < 20 )
      cooldown -> duration = timespan_t::from_seconds( data().effectN( 1 ).base_value() );

    ranged_attack_t::execute();
  }

};

// Dire Beast ===============================================================

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
    if ( ! beast -> is_sleeping() )
      beast = p() -> pet_dire_beasts[ 1 ];

    // Dire beast gets a chance for an extra attack based on haste
    // rather than discrete plateaus.  At integer numbers of attacks,
    // the beast actually has a 50% chance of n-1 attacks and 50%
    // chance of n.  It (apparently) scales linearly between n-0.5
    // attacks to n+0.5 attacks.  This uses beast duration to
    // effectively alter the number of attacks as the duration itself
    // isn't important and combat log testing shows some variation in
    // attack speeds.  This is not quite perfect but more accurate
    // than plateaus.
    const timespan_t base_duration = timespan_t::from_seconds( 15.0 );
    const timespan_t swing_time = beast -> main_hand_weapon.swing_time * beast -> composite_melee_speed();
    double partial_attacks_per_summon = base_duration / swing_time;
    double base_attacks_per_summon = floor( partial_attacks_per_summon - 0.5 ); // 8.4 -> 7, 8.5 -> 8, 8.6 -> 8, etc

    if ( rng().roll( partial_attacks_per_summon - base_attacks_per_summon - 0.5 ) )
    {
      base_attacks_per_summon += 1;
    }

    timespan_t duration = base_attacks_per_summon * swing_time;
    beast -> summon( duration );
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
    p() -> active.pet -> buffs.bestial_wrath -> trigger();

    hunter_spell_t::execute();
  }

  virtual bool ready()
  {
    if ( ! p() -> active.pet )
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

    if ( p() -> active.pet )
      p() -> active.pet -> resource_gain( RESOURCE_FOCUS, base_gain, p() -> active.pet -> gains.fervor );

    hunter_spell_t::execute();
  }

  virtual void tick( dot_t* d )
  {
    hunter_spell_t::tick( d );

    p() -> resource_gain( RESOURCE_FOCUS, tick_gain, p() -> gains.fervor );

    if ( p() -> active.pet )
      p() -> active.pet -> resource_gain( RESOURCE_FOCUS, tick_gain, p() -> active.pet -> gains.fervor );
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
    double stacks = p() -> active.pet -> buffs.frenzy -> stack();
    double value = stacks * p() -> specs.focus_fire -> effectN( 1 ).percent();
    p() -> buffs.focus_fire -> trigger( 1, value );

    double gain = stacks * p() -> specs.focus_fire -> effectN( 2 ).resource( RESOURCE_FOCUS );
    p() -> active.pet -> resource_gain( RESOURCE_FOCUS, gain, p() -> active.pet -> gains.focus_fire );

    hunter_spell_t::execute();

    p() -> active.pet -> buffs.frenzy -> expire();

    trigger_tier16_bm_4pc_melee();
  }

  virtual bool ready()
  {
    if ( ! p() -> active.pet )
      return false;

    if ( ! p() -> active.pet -> buffs.frenzy -> check() )
      return false;

    if ( five_stacks && p() -> active.pet -> buffs.frenzy -> stack_react() < 5 )
      return false;

    return hunter_spell_t::ready();
  }
};

// Kill Command =============================================================

struct kill_command_t : public hunter_spell_t
{
  kill_command_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "kill_command", player, player -> find_class_spell( "Kill Command" ) )
  {
    parse_options( NULL, options_str );

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
    trigger_tier16_bm_4pc_melee();

    if ( p() -> active.pet )
    {
      p() -> active.pet -> active.kill_command -> execute();
    }
  }

  virtual bool ready()
  {
    return p() -> active.pet && hunter_spell_t::ready();
  }
};


// Rapid Fire ===============================================================

struct rapid_fire_t : public hunter_spell_t
{
  rapid_fire_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "rapid_fire", player, spell_data_t::nil() /*player -> find_class_spell( "Rapid Fire" )*/ )
  {
    parse_options( NULL, options_str );
    cooldown -> duration = timespan_t::from_seconds( 180 );
    check_spec ( HUNTER_MARKSMANSHIP );
    harmful = false;
  }

  virtual void execute()
  {
    double value =  0.40; /*data().effectN( 1 ).percent();*/
    p() -> buffs.rapid_fire -> trigger( 1, value );

    hunter_spell_t::execute();
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

    pet -> type = PLAYER_PET;
    pet -> summon();

    if ( p() -> main_hand_attack ) p() -> main_hand_attack -> cancel();
  }

  virtual bool ready()
  {
    if ( p() -> active.pet == pet || p() -> talents.lone_wolf -> ok() ) // For now, don't summon a pet if lone wolf talent is used.
      return false;

    return hunter_spell_t::ready();
  }
};

// Stampede =================================================================

struct stampede_t : public hunter_spell_t
{
  stampede_t( hunter_t* p, const std::string& options_str ) :
    hunter_spell_t( "stampede", p, p -> talents.stampede )
  {
    parse_options( NULL, options_str );
    harmful = false;
    school = SCHOOL_PHYSICAL;
  }

  virtual void execute()
  {
    hunter_spell_t::execute();
    trigger_tier16_bm_4pc_melee();
    p() -> buffs.stampede -> trigger();

    for ( unsigned int i = 0; i < p() -> hunter_main_pets.size() && i < 5; ++i )
    {
      p() -> hunter_main_pets[ i ] -> stampede_summon( timespan_t::from_millis( 20027 ) );
      // Added 0.027 seconds to properly reflect haste threshholds seen in game.
    }
  }

};
}

hunter_td_t::hunter_td_t( player_t* target, hunter_t* p ) :
  actor_pair_t( target, p ),
  dots( dots_t() )
{
  dots.serpent_sting = target -> get_dot( "serpent_sting", p );
}

// hunter_t::create_action ==================================================

action_t* hunter_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  using namespace attacks;
  using namespace spells;

  if ( name == "auto_shot"             ) return new              start_attack_t( this, options_str );
  if ( name == "aimed_shot"            ) return new             aimed_shot_t( this, options_str );
  if ( name == "arcane_shot"           ) return new            arcane_shot_t( this, options_str );
  // make this a no-op for now to not break profiles.
  if ( name == "bestial_wrath"         ) return new          bestial_wrath_t( this, options_str );
  if ( name == "black_arrow"           ) return new            black_arrow_t( this, options_str );
  if ( name == "chimera_shot"          ) return new           chimera_shot_t( this, options_str );
  if ( name == "explosive_shot"        ) return new         explosive_shot_t( this, options_str );
  if ( name == "explosive_trap"        ) return new         explosive_trap_t( this, options_str );
  if ( name == "fervor"                ) return new                 fervor_t( this, options_str );
  if ( name == "focus_fire"            ) return new             focus_fire_t( this, options_str );
  if ( name == "kill_command"          ) return new           kill_command_t( this, options_str );
  if ( name == "kill_shot"             ) return new              kill_shot_t( this, options_str );
  if ( name == "multi_shot"            ) return new             multi_shot_t( this, options_str );
  if ( name == "rapid_fire"            ) return new             rapid_fire_t( this, options_str );
  if ( name == "steady_shot"           ) return new            steady_shot_t( this, options_str );
  if ( name == "focusing_shot"         ) return new          focusing_shot_t( this, options_str );
  if ( name == "summon_pet"            ) return new             summon_pet_t( this, options_str );
  if ( name == "cobra_shot"            ) return new             cobra_shot_t( this, options_str );
  if ( name == "a_murder_of_crows"     ) return new                    moc_t( this, options_str );
  if ( name == "aspect_of_the_pack"    ) return new     aspect_of_the_pack_t( this, options_str );
  if ( name == "powershot"             ) return new              powershot_t( this, options_str );
  if ( name == "stampede"              ) return new               stampede_t( this, options_str );
  if ( name == "glaive_toss"           ) return new            glaive_toss_t( this, options_str );
  if ( name == "dire_beast"            ) return new             dire_beast_t( this, options_str );
  if ( name == "barrage"               ) return new                barrage_t( this, options_str );

#if 0
  if ( name == "trap_launcher"         ) return new          trap_launcher_t( this, options_str );
  if ( name == "binding_shot"          ) return new           binding_shot_t( this, options_str );
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
    return new pets::hunter_main_pet_t( *sim, *this, pet_name, type );
  }
  else
  {
    sim -> errorf( "Player %s with pet %s has unknown type %s\n", name(), pet_name.c_str(), pet_type.c_str() );
    sim -> cancel();
  }

  return nullptr;
}

// hunter_t::create_pets ====================================================

void hunter_t::create_pets()
{
  create_pet( "cat",          "cat"          );
  create_pet( "devilsaur",    "devilsaur"    );
  create_pet( "raptor",       "raptor"       );
  create_pet( "hyena",        "hyena"        );
  create_pet( "wolf",         "wolf"         );
  create_pet( "spider",       "spider"       );
//  create_pet( "chimera",      "chimera"      );
//  create_pet( "wind_serpent", "wind_serpent" );

  for ( size_t i = 0; i < pet_dire_beasts.size(); ++i )
  {
    pet_dire_beasts[ i ] = new pets::dire_critter_t( *this, i + 1 );
  }
  for ( int i = 0; i < 10; i++ )
  {
    thunderhawk[ i ] = new pets::tier15_thunderhawk_t( *this );
  }
}

// hunter_t::init_spells ====================================================

void hunter_t::init_spells()
{
  player_t::init_spells();

  // Baseline
  // Talents
  talents.posthaste                         = find_talent_spell( "Posthaste" );
  talents.narrow_escape                     = find_talent_spell( "Narrow Escape" );
  talents.exhilaration                      = find_talent_spell( "Exhilaration" );

  talents.intimidation                      = find_talent_spell( "Intimidation" );
  talents.wyvern_sting                      = find_talent_spell( "Wyvern Sting" );
  talents.binding_shot                      = find_talent_spell( "Binding Shot" );

  talents.crouching_tiger_hidden_chimera    = find_talent_spell( "Crouching Tiger, Hidden Chimera" );
  talents.iron_hawk                         = find_talent_spell( "Iron Hawk" );
  talents.spirit_bond                       = find_talent_spell( "Spirit Bond" );

  talents.fervor                            = find_talent_spell( "Fervor" );
  talents.dire_beast                        = find_talent_spell( "Dire Beast" );
  talents.thrill_of_the_hunt                = find_talent_spell( "Thrill of the Hunt" );

  talents.a_murder_of_crows                 = find_talent_spell( "A Murder of Crows" );
  talents.blink_strikes                     = find_talent_spell( "Blink Strikes" );
  talents.stampede                          = find_talent_spell( "Stampede" );

  talents.glaive_toss                       = find_talent_spell( "Glaive Toss" );
  talents.powershot                         = find_talent_spell( "Powershot" );
  talents.barrage                           = find_talent_spell( "Barrage" );

  talents.flaming_shots                     = find_talent_spell( "Flaming Shots" );
  talents.focusing_shot                     = find_talent_spell( "Focusing Shot" );
  talents.versatility                       = find_talent_spell( "Versatility" );
  talents.lone_wolf                         = find_talent_spell( "Lone Wolf" );

  // Perks
  perks.enhanced_camouflage               = find_perk_spell( "Enhanced Camouflage"      );
  perks.improved_focus_fire               = find_perk_spell( "Improved Focus Fire"      );
  perks.improved_kill_command             = find_perk_spell( "Improved Kill Command"    );
  perks.empowered_pets                    = find_perk_spell( "Empowered Pets"           );
  perks.improved_beast_cleave             = find_perk_spell( "Improved Beast Cleave"    );
  perks.enhanced_basic_attacks            = find_perk_spell( "Enhanced Basic Attacks"   );
  perks.enhanced_kill_shot                = find_perk_spell( "Enhanced Kill Shot"       );
  perks.improved_arcane_shot              = find_perk_spell( "Improved Arcane Shot"     );
  perks.improved_cobra_shot               = find_perk_spell( "Improved Cobra Shot"      );
  perks.enhanced_chimera_shot             = find_perk_spell( "Enhanced Chimera Shot"    );
  perks.enhanced_aimed_shot               = find_perk_spell( "Enhanced Aimed Shot"      );
  perks.improved_focus                    = find_perk_spell( "Improved Focus"           );
  perks.improved_aimed_shot               = find_perk_spell( "Improved Aimed Shot"      );
  perks.improved_steady_shot              = find_perk_spell( "Improved Steady Shot"     );
  perks.enhanced_shots                    = find_perk_spell( "Enhanced Shots"           );
  perks.empowered_explosive_shot          = find_perk_spell( "Empowered Explosive Shot" );
  perks.improved_black_arrow              = find_perk_spell( "Improved Black Arrow"     );
  perks.enhanced_traps                    = find_perk_spell( "Enhanced Traps"           );
  perks.enhanced_entrapment               = find_perk_spell( "Enhanced Entrapment"      );
  perks.improved_camouflage               = find_perk_spell( "Improved Camouflage"      );
  perks.improved_viper_venom              = find_perk_spell( "Improved Viper Venom"     );

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

  // Attunments
  specs.crit_attunement       = find_specialization_spell( "Critical Strike Attunement" );
  specs.mastery_attunement    = find_specialization_spell( "Mastery Attunement" );
  specs.multistrike_attunement = find_specialization_spell( "Multistrike Attunement" );

  // Spec spells
  specs.critical_strikes     = find_spell( 157443 );
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
  specs.explosive_shot       = find_specialization_spell( "Explosive Shot" );
  specs.lock_and_load        = find_specialization_spell( "Lock and Load" );
  specs.viper_venom          = find_specialization_spell( "Viper Venom" );
  specs.bombardment          = find_specialization_spell( "Bombardment" );
  specs.serpent_sting        = find_specialization_spell( "Serpent Sting" );
  specs.trap_mastery         = find_specialization_spell( "Trap Mastery" );
  specs.trueshot_aura        = find_spell( 19506 );
  
  if ( specs.explosive_shot -> ok() )
  {
    active.explosive_ticks = new attacks::explosive_shot_tick_t( this );
  }

  if ( specs.serpent_sting -> ok() )
    active.serpent_sting = new attacks::serpent_sting_t( this );

  if ( mastery.wild_quiver -> ok() )
  {
    //active.wild_quiver_shot = new attacks::wild_quiver_shot_t( this );
  }

  action_lightning_arrow_aimed_shot = new attacks::lightning_arrow_t( this, "_aimed_shot" );
  action_lightning_arrow_arcane_shot = new attacks::lightning_arrow_t( this, "_arcane_shot" );
  action_lightning_arrow_multi_shot = new attacks::lightning_arrow_t( this, "_multi_shot" );

  static const set_bonus_description_t set_bonuses =
  {
    //  C2P    C4P     M2P     M4P    T2P    T4P     H2P    H4P
    {     0,     0, 105732, 105921,     0,     0,     0,     0 }, // Tier13
    {     0,     0, 123090, 123091,     0,     0,     0,     0 }, // Tier14
    {     0,     0, 138365, 138367,     0,     0,     0,     0 }, // Tier15
    {     0,     0, 144637, 144641,     0,     0,     0,     0 }, // Tier16
  };

  sets.register_spelldata( set_bonuses );
}


// hunter_t::init_base ======================================================

void hunter_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;

  base_focus_regen_per_second = 4;
  if ( sets.has_set_bonus( SET_PVP_4PC_MELEE ) )
    base_focus_regen_per_second *= 1.25;


  resources.base[ RESOURCE_FOCUS ] = 100 + specs.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS );
  resources.base[ RESOURCE_FOCUS ] = 100 + perks.improved_focus -> effectN( 1 ).resource( RESOURCE_FOCUS );

  cooldowns.viper_venom -> duration = specs.viper_venom -> internal_cooldown();

  // Orc racial
  if ( race == RACE_ORC )
    pet_multiplier *= 1.0 +  find_racial_spell( "Command" ) -> effectN( 1 ).percent();

  pet_multiplier *= 1.0 + perks.empowered_pets -> effectN( 1 ).percent();
  
  stats_stampede = get_stats( "stampede" );
}

// hunter_t::init_buffs =====================================================

void hunter_t::create_buffs()
{
  player_t::create_buffs();

  buffs.beast_within                = buff_creator_t( this, 34471, "beast_within" )
                                      .chance( specs.the_beast_within -> ok() )
                                      .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.beast_within -> buff_duration += sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).time_value();

  buffs.bombardment                 = buff_creator_t( this, "bombardment", specs.bombardment -> effectN( 1 ).trigger() );
  buffs.cobra_strikes               = buff_creator_t( this, 53257, "cobra_strikes" ).chance( specs.cobra_strikes -> proc_chance() );
  buffs.focus_fire                  = buff_creator_t( this, 82692, "focus_fire" )
                                      .add_invalidate( CACHE_ATTACK_HASTE )
                                      .add_invalidate( CACHE_ATTACK_POWER );

  buffs.thrill_of_the_hunt          = buff_creator_t( this, 34720, "thrill_of_the_hunt" ).chance( talents.thrill_of_the_hunt -> proc_chance() );
  buffs.lock_and_load               = buff_creator_t( this, 56453, "lock_and_load" )
                                      .chance( locknload ); //Hard-coded until they actually tell us the proc rate.
  buffs.lock_and_load -> default_chance += sets.set( SET_T14_4PC_MELEE ) -> effectN( 2 ).percent();

  buffs.rapid_fire                  = buff_creator_t( this, "rapid_fire" )
                                      .duration( timespan_t::from_seconds( 15 ) )
                                      .add_invalidate( CACHE_ATTACK_HASTE );
  buffs.rapid_fire -> cooldown -> duration = timespan_t::zero();

  buffs.stampede          = buff_creator_t( this, 130201, "stampede" ) // To allow action lists to react to stampede, rather than doing it in a roundabout way.
                           .activated( true )
                           .duration( timespan_t::from_millis( 20027 ) ); 
                           // Added 0.027 seconds to properly reflect haste threshholds seen in game.
                           /*.quiet( true )*/;

  buffs.tier13_4pc                  = buff_creator_t( this, 105919, "tier13_4pc" )
                                      .chance( sets.set( SET_T13_4PC_MELEE ) -> proc_chance() )
                                      .cd( timespan_t::from_seconds( 105.0 ) )
                                      .add_invalidate( CACHE_ATTACK_HASTE );

  buffs.tier16_4pc_mm_keen_eye      = buff_creator_t( this, 144659, "tier16_4pc_keen_eye" );
  buffs.tier16_4pc_bm_brutal_kinship = buff_creator_t( this, 144670, "tier16_4pc_brutal_kinship" )
                                       .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
}

// hunter_t::init_gains =====================================================

void hunter_t::init_gains()
{
  player_t::init_gains();

  gains.invigoration         = get_gain( "invigoration"         );
  gains.fervor               = get_gain( "fervor"               );
  gains.focus_fire           = get_gain( "focus_fire"           );
  gains.thrill_of_the_hunt   = get_gain( "thrill_of_the_hunt_savings"   );
  gains.steady_shot          = get_gain( "steady_shot"          );
  gains.focusing_shot        = get_gain( "focusing_shot"        );
  gains.cobra_shot           = get_gain( "cobra_shot"           );
  gains.aimed_shot           = get_gain( "aimed_shot"           );
  gains.dire_beast           = get_gain( "dire_beast"           );
  gains.viper_venom          = get_gain( "viper_venom"          );
}

// hunter_t::init_position ==================================================

void hunter_t::init_position()
{
  player_t::init_position();

  if ( base.position == POSITION_FRONT )
  {
    base.position = POSITION_RANGED_FRONT;
    position_str = util::position_type_string( base.position );
  }
  else if ( initial.position == POSITION_BACK )
  {
    base.position = POSITION_RANGED_BACK;
    position_str = util::position_type_string( base.position );
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "%s: Position adjusted to %s", name(), position_str.c_str() );
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
  procs.tier15_2pc_melee             = get_proc( "tier15_2pc_melee"             );
  procs.tier15_4pc_melee_aimed_shot  = get_proc( "tier15_4pc_melee_aimed_shot"  );
  procs.tier15_4pc_melee_arcane_shot = get_proc( "tier15_4pc_melee_arcane_shot" );
  procs.tier15_4pc_melee_multi_shot  = get_proc( "tier15_4pc_melee_multi_shot"  );
  procs.tier16_2pc_melee             = get_proc( "tier16_2pc_melee"             );
  procs.tier16_4pc_melee             = get_proc( "tier16_4pc_melee"             );
}

// hunter_t::init_rng =======================================================

void hunter_t::init_rng()
{

  // RPPMS
  double tier15_2pc_melee_rppm;

  if ( specialization() == HUNTER_BEAST_MASTERY )
    tier15_2pc_melee_rppm = 0.7;
  else if ( specialization() == HUNTER_MARKSMANSHIP )
    tier15_2pc_melee_rppm = 1.0;
  else // HUNTER_SURVIVAL
    tier15_2pc_melee_rppm = 1.2;

  ppm_tier15_2pc_melee.set_frequency( tier15_2pc_melee_rppm );
  ppm_tier15_4pc_melee.set_frequency( 3.0 );

  player_t::init_rng();
}

// hunter_t::init_scaling ===================================================

void hunter_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_STRENGTH ] = false;
}

// hunter_t::init_actions ===================================================

void hunter_t::init_action_list()
{
  if ( main_hand_weapon.group() != WEAPON_RANGED )
  {
    sim -> errorf( "Player %s does not have a ranged weapon at the Main Hand slot.", name() );
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    std::string& precombat = get_action_priority_list( "precombat" ) -> action_list_str;

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

    precombat += "/summon_pet";
    precombat += "/snapshot_stats";

    if ( ( level >= 80 ) && ( sim -> allow_potions ) )
    {
      precombat += ( level > 85 ) ? "/potion,name=virmens_bite" : "/potion,name=tolvir";

      action_list_str += ( level > 85 ) ? "/potion,name=virmens_bite" : "/potion,name=tolvir";
      action_list_str += ",if=target.time_to_die<=25|buff.stampede.up";
    }

    if ( specialization() == HUNTER_SURVIVAL )
      action_list_str += init_use_racial_actions();
    action_list_str += init_use_item_actions();
    action_list_str += init_use_profession_actions();
    action_list_str += "/auto_shot";
    action_list_str += "/explosive_trap,if=active_enemies>1";

    switch ( specialization() )
    {
        // BEAST MASTERY
      case HUNTER_BEAST_MASTERY:

        action_list_str += init_use_racial_actions();
        action_list_str += "/dire_beast";
        action_list_str += "/fervor,if=focus<=65";
        action_list_str += "/bestial_wrath,if=focus>60&!buff.beast_within.up";
        action_list_str += "/multi_shot,if=active_enemies>5|(active_enemies>1&pet.cat.buff.beast_cleave.down)";

        action_list_str += "/stampede,if=(trinket.stat.agility.up|target.time_to_die<=20|(trinket.stacking_stat.agility.stack>10&trinket.stat.agility.cooldown_remains<=3))";

        action_list_str += "/barrage,if=active_enemies>5";
        action_list_str += "/kill_shot";
        action_list_str += "/kill_command";
        action_list_str += "/focusing_shot,if=focus<50";
        action_list_str += "/a_murder_of_crows,if=!ticking";
        action_list_str += "/glaive_toss";
        action_list_str += "/barrage";
        action_list_str += "/powershot";
        action_list_str += "/cobra_shot,if=active_enemies>5";
        action_list_str += "/arcane_shot,if=buff.thrill_of_the_hunt.react|buff.beast_within.up";
        action_list_str += "/focus_fire,five_stacks=1";
        action_list_str += "/arcane_shot,if=focus>=61";

        if ( level >= 81 )
          action_list_str += "/cobra_shot";
        else
          action_list_str += "/steady_shot";
        break;

        // MARKSMANSHIP
      case HUNTER_MARKSMANSHIP:
        action_list_str += init_use_racial_actions();

        action_list_str += "/powershot";
        action_list_str += "/fervor,if=focus<=50";
        action_list_str += "/rapid_fire,if=!buff.rapid_fire.up";

        action_list_str += "/stampede,if=(trinket.stat.agility.up|target.time_to_die<=20|(trinket.stacking_stat.agility.stack>10&trinket.stat.agility.cooldown_remains<=3))";

        action_list_str += "/a_murder_of_crows,if=!ticking";
        action_list_str += "/dire_beast";

        action_list_str += "/run_action_list,name=careful_aim,if=target.health.pct>";
        action_list_str += util::to_string( dbc.spell( 34483 ) -> effectN( 2 ).base_value() );
        {
          // sub-action list for the CA phase. The action above here are also included
          std::string& CA_actions = get_action_priority_list( "careful_aim" ) -> action_list_str;
          CA_actions += "/chimera_shot";
          CA_actions += "/aimed_shot";
          CA_actions += "/glaive_toss";
          CA_actions += "/focusing_shot,if=focus<60";
          CA_actions += "/steady_shot";
        }

        // actions for outside the CA phase
        action_list_str += "/glaive_toss";
        action_list_str += "/barrage";
        action_list_str += "/chimera_shot";
        action_list_str += "/focusing_shot,if=focus<50";
        action_list_str += "/kill_shot";
        action_list_str += "/multi_shot,if=active_enemies>=4";
        action_list_str += "/aimed_shot";
        action_list_str += "/steady_shot";
        break;

        // SURVIVAL
      case HUNTER_SURVIVAL:
        action_list_str += "/fervor,if=focus<=50";
        action_list_str += "/a_murder_of_crows,if=!ticking";
        action_list_str += "/explosive_shot,if=buff.lock_and_load.react";
        action_list_str += "/glaive_toss";
        action_list_str += "/powershot";
        action_list_str += "/barrage";
        action_list_str += "/explosive_shot,if=cooldown_react";
        action_list_str += "/black_arrow,if=!ticking&target.time_to_die>=8";
        action_list_str += "/multi_shot,if=active_enemies>3";
        action_list_str += "/focusing_shot,if=focus<50";
        action_list_str += "/arcane_shot,if=buff.thrill_of_the_hunt.react";
        action_list_str += "/dire_beast";

        action_list_str += "/stampede,if=(trinket.stat.agility.up|target.time_to_die<=20|(trinket.stacking_stat.agility.stack>10&trinket.stat.agility.cooldown_remains<=3))";

        action_list_str += "/arcane_shot,if=focus>=67&active_enemies<2";
        action_list_str += "/multi_shot,if=focus>=67&active_enemies>1";

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

    use_default_action_list = true;
  }

  player_t::init_action_list();
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
  active.pet            = nullptr;
  active.aspect         = ASPECT_NONE;
}

// hunter_t::arise ==========================================================

void hunter_t::arise()
{
  player_t::arise();

  if ( specs.trueshot_aura -> is_level( level ) && ! sim -> overrides.attack_power_multiplier )
    sim -> auras.attack_power_multiplier -> trigger();
}


// hunter_t::composite_rating_multiplier =====================================

double hunter_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  switch ( rating )
  {
  case RATING_MULTISTRIKE:
    return m *= 1.0 + specs.multistrike_attunement -> effectN( 1 ).percent();
  case RATING_MELEE_CRIT:
    return m *= 1.0 + specs.crit_attunement -> effectN( 1 ).percent();
  case RATING_SPELL_CRIT:
    return m *= 1.0 + specs.crit_attunement -> effectN( 1 ).percent();
  case RATING_MASTERY:
    return m *= 1.0 + specs.mastery_attunement -> effectN( 1 ).percent();
  default:
    break;
  }

  return m;
}

// hunter_t::composite_attack_power_multiplier ==============================

double hunter_t::composite_attack_power_multiplier() const
{
  double mult = player_t::composite_attack_power_multiplier();
  if ( perks.improved_focus_fire -> ok() && buffs.focus_fire -> check() )
  {
    double stacks = buffs.focus_fire -> current_value / specs.focus_fire -> effectN( 1 ).percent();
    mult += stacks * perks.improved_focus_fire -> effectN( 1 ).percent();
  }
  return mult;
}

// hunter_t::composite_melee_crit ===========================================

double hunter_t::composite_melee_crit() const
{
  double crit = player_t::composite_melee_crit();
  crit += specs.critical_strikes -> effectN( 1 ).percent();
  return crit;
}

// Haste and speed buff computations ========================================

double hunter_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();
  h *= 1.0 / ( 1.0 + buffs.tier13_4pc -> up() * buffs.tier13_4pc -> data().effectN( 1 ).percent() );
  h *= ranged_haste_multiplier();
  return h;
}

double hunter_t::composite_melee_speed() const
{
  double h = player_t::composite_melee_speed();
  h *= ranged_speed_multiplier();
  return h;
}

// Buffs that increase hunter ranged attack haste (and thus regen) but not
// melee attack haste (and so not pet attacks nor RPPM)
double hunter_t::ranged_haste_multiplier() const
{
  double h = 1.0;
  h *= 1.0 / ( 1.0 + buffs.focus_fire -> value() );
  h *= 1.0 / ( 1.0 + buffs.rapid_fire -> value() );
  return h;
}

// Buffs that increase hunter ranged attack speed but not
// melee attack speed (and so not pet attacks)
double hunter_t::ranged_speed_multiplier() const
{
  double h = 1.0;

  return h;
}

// hunter_t::composite_player_multiplier ====================================

double hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( mastery.essence_of_the_viper -> ok() &&
       ( dbc::is_school( school, SCHOOL_NATURE ) ||
         dbc::is_school( school, SCHOOL_ARCANE ) ||
         dbc::is_school( school, SCHOOL_SHADOW ) ||
         dbc::is_school( school, SCHOOL_FIRE   ) ) )
  {
    m *= 1.0 + cache.mastery_value();
  }

  if ( talents.lone_wolf && !active.pet )
    m *= 1.0 + talents.lone_wolf -> effectN( 1 ).percent();

  if ( buffs.beast_within -> up() )
    m *= 1.0 + buffs.beast_within -> data().effectN( 2 ).percent();
  
  if ( sets.set( SET_T16_4PC_MELEE) -> ok() )
    m *= 1.0 + buffs.tier16_4pc_bm_brutal_kinship -> stack() * buffs.tier16_4pc_bm_brutal_kinship -> data().effectN( 1 ).percent();

  return m;
}

void hunter_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_MASTERY:
      if ( mastery.essence_of_the_viper -> ok() )
      {
        player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      }
      break;
    default: break;
  }
}

// hunter_t::matching_gear_multiplier =======================================

double hunter_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0.0;
}

// hunter_t::regen  =========================================================

void hunter_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  periodicity *= 1.0 + composite_ranged_haste_rating() / current_rating().attack_haste;

}

// hunter_t::create_options =================================================

void hunter_t::create_options()
{
  player_t::create_options();

  option_t hunter_options[] =
  {
    opt_string( "summon_pet", summon_pet_str ),
    opt_float( "locknload", locknload ),
    opt_null()
  };

  option_t::copy( options, hunter_options );
}

// hunter_t::create_profile =================================================

bool hunter_t::create_profile( std::string& profile_str, save_e stype, bool save_html )
{
  player_t::create_profile( profile_str, stype, save_html );

  profile_str += "summon_pet=" + summon_pet_str + "\n";

  return true;
}

// hunter_t::copy_from ======================================================

void hunter_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  hunter_t* p = debug_cast<hunter_t*>( source );

  summon_pet_str = p -> summon_pet_str;
  locknload = p -> locknload;
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
    if ( pos != std::string::npos ) cdata_str.erase( 0, pos + 1 );
    pos = cdata_str.rfind( '}' );
    if ( pos != std::string::npos ) cdata_str.erase( pos );

    js::js_node* pet_js = js_t::create( sim, cdata_str );
    pet_js = js_t::get_node( pet_js, "Pet.data" );
    if ( sim -> debug ) js_t::print( pet_js, sim -> output_file );

    if ( ! pet_js )
    {
      sim -> errorf( "\nHunter %s unable to download pet data from Armory\n", name() );
      sim -> cancel();
      return;
    }

    std::vector<js::js_node*> pet_records;
    int num_pets = js_t::get_children( pet_records, pet_js );
    for ( int i = 0; i < num_pets; i++ )
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
      for ( unsigned j = 0; j < pet_name.length(); j++ )
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

      if ( pet_family > num_families || pet_types[ pet_family ] == PET_NONE )
      {
        sim -> errorf( "\nHunter %s unable to decode pet %s family id %d\n", name(), pet_name.c_str(), pet_family );
        continue;
      }

      hunter_main_pet_t* pet = new hunter_main_pet_t( sim, this, pet_name, pet_types[ pet_family ] );

      pet -> parse_talents_armory( pet_talents );

      pet -> talents_str.clear();
      for ( int j = 0; j < MAX_TALENT_TREES; j++ )
      {
        for ( int k = 0; k < ( int ) pet -> talent_trees[ j ].size(); k++ )
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

set_e hunter_t::decode_set( const item_t& item ) const
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

  if ( strstr( s, "wyrmstalkers"             ) ) return SET_T13_MELEE;

  if ( strstr( s, "yaungol_slayer"           ) ) return SET_T14_MELEE;
  
  if ( strstr( s, "saurok_stalker"           ) ) return SET_T15_MELEE;

  if ( strstr( s, "_of_the_unblinking_vigil" ) ) return SET_T16_MELEE;

  if ( strstr( s, "_gladiators_chain_"       ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

// hunter_::convert_hybrid_stat ==============================================

stat_e hunter_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_AGI_INT: 
    return STAT_AGILITY; 
  case STAT_STR_AGI:
    return STAT_AGILITY;
  // This is a guess at how STR/INT gear will work for Rogues, TODO: confirm  
  // This should probably never come up since rogues can't equip plate, but....
  case STAT_STR_INT:
    return STAT_NONE;
  case STAT_SPIRIT:
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
      return STAT_NONE;     
  default: return s; 
  }
}

// hunter_t::moving() =======================================================

void hunter_t::moving()
{
  player_t::interrupt();
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class hunter_report_t : public player_report_extension_t
{
public:
  hunter_report_t( hunter_t& player ) :
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
  hunter_t& p;
};

// HUNTER MODULE INTERFACE ==================================================

struct hunter_module_t : public module_t
{
  hunter_module_t() : module_t( HUNTER ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    hunter_t* p = new hunter_t( sim, name, r );
    p -> report_extension = std::shared_ptr<player_report_extension_t>( new hunter_report_t( *p ) );
    return p;
  }

  virtual bool valid() const { return true; }
  virtual void init( sim_t* sim ) const
  {
    for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[ i ];

      p -> buffs.aspect_of_the_pack    = buff_creator_t( p, "aspect_of_the_pack", p -> find_class_spell( "Aspect of the Pack" ) );
    }
  }
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end  ( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::hunter()
{
  static hunter_module_t m;
  return &m;
}
