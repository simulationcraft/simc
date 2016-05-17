//==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// TODO
//
// Beast Mastery
//   - Dire Beast (focus gain is passive now)
//  Talent
//   - Stomp
//   - Stampede (rework)
//   - Aspect of the Beast
//  Artifacts
//   - Jaws of Thunder
//   - Spitting Cobras
//   - Wilderness Expert
//   - Pack Leader
//   - Unleash the Beast
//   - Renewed Vigor
//   - Focus of the Titans
//   - Furious Swipes
//   - Titan's Thunder
//   - Master of Beasts
//   - Stormshot
//   - Surge of the Stormgod
//
// Marksmanship
//  Talents
//   - Black Arrow
//   - Heightened Vulnerability
//   - Volley
//   - Update Trick Shot behavior
//  Artifacts
//   - Call of the Hunter (NYI still)
//
// Survival
//   - Carve
//   - Flanking Strike
//   - Hatchet Toss (really?)
//   - Lacerate
//   - Mongoose Bite
//   - Raptor Strike
//   - Mastery: Hunting Companion
//  Talents
//   - Animal Instincts
//   - Throwing Axes
//   - Way of the Mok'Nathal
//   - Improved Traps
//   - Steel Trap
//   - Dragonsfire Grenade
//   - Snake Hunter
//   - Butchery
//   - Mortal Wounds
//   - Serpent Sting
//   - Spitting Cobra
//   - Expert Trapper
//  Artifacts
//   - Sharpened Beak
//   - Raptor's Cry
//   - Hellcarver
//   - My Beloved Monster
//   - Strength of the Mountain
//   - Fluffy, Go
//   - Jagged Claws
//   - Hunter's Guile
//   - Eagle's Bite
//   - Aspect of the Skylord
//   - Talon Strike
//
// ==========================================================================

namespace
{ // UNNAMED NAMESPACE

// ==========================================================================
// Hunter
// ==========================================================================

struct hunter_t;

namespace pets
{
struct hunter_main_pet_t;
}

enum aspect_type { ASPECT_NONE = 0, ASPECT_MAX };

enum exotic_munitions { NO_AMMO = 0, FROZEN_AMMO = 2, INCENDIARY_AMMO = 4, POISONED_AMMO = 8 };

struct hunter_td_t: public actor_target_data_t
{
  struct debuffs_t
  {
    buff_t* hunters_mark;
    buff_t* vulnerable;
    buff_t* marked_for_death;
    buff_t* deadeye;
    buff_t* true_aim;
  } debuffs;

  struct dots_t
  {
    dot_t* serpent_sting;
    dot_t* poisoned_ammo;
    dot_t* piercing_shots;
  } dots;

  hunter_td_t( player_t* target, hunter_t* p );
};

struct hunter_t: public player_t
{
public:
  timespan_t movement_ended;

  // Active
  std::vector<pets::hunter_main_pet_t*> hunter_main_pets;
  struct actives_t
  {
    pets::hunter_main_pet_t* pet;
    exotic_munitions    ammo;
    action_t*           serpent_sting;
    action_t*           piercing_shots;
    action_t*           frozen_ammo;
    action_t*           incendiary_ammo;
    action_t*           poisoned_ammo;
  } active;

  // Dire beasts last 8 seconds, so you'll have at max 8
  std::array< pet_t*, 8 >  pet_dire_beasts;
  // Tier 15 2-piece bonus: need 10 slots (just to be safe) because each
  // Steady Shot or Cobra Shot can trigger a Thunderhawk, which stays
  // for 10 seconds.
  std::array< pet_t*, 10 > thunderhawk;

  pet_t* hati;

  // Tier 15 4-piece bonus
  attack_t* action_lightning_arrow_aimed_shot;
  attack_t* action_lightning_arrow_arcane_shot;
  attack_t* action_lightning_arrow_multi_shot;

  // Tier 18 (WoD 6.2) trinket effects
  const special_effect_t* beastlord;
  const special_effect_t* longview;
  const special_effect_t* blackness;

  // Artifact abilities
  const special_effect_t* thasdorah;
  const special_effect_t* titanstrike;

  // Buffs
  struct buffs_t
  {
    buff_t* aspect_of_the_cheetah;
    buff_t* aspect_of_the_turtle;
    buff_t* aspect_of_the_wild;
    buff_t* beast_cleave;
    buff_t* bestial_wrath;
    buff_t* big_game_hunter;
    buff_t* bombardment;
    buff_t* careful_aim;
    buff_t* steady_focus;
    buff_t* pre_steady_focus;
    buff_t* hunters_mark_exists;
    buff_t* lock_and_load;
    buff_t* stampede;
    buff_t* trueshot;
    buff_t* rapid_killing;
    buff_t* bullseye;
    buff_t* heavy_shot; // t17 SV 4pc
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* explosive_shot;
    cooldown_t* black_arrow;
    cooldown_t* bestial_wrath;
    cooldown_t* kill_shot_reset;
    cooldown_t* trueshot;
    cooldown_t* dire_beast;
    cooldown_t* kill_command;
  } cooldowns;

  // Custom Parameters
  std::string summon_pet_str;

  // Gains
  struct gains_t
  {
    gain_t* arcane_shot;
    gain_t* steady_focus;
    gain_t* cobra_shot;
    gain_t* aimed_shot;
    gain_t* dire_beast;
    gain_t* multi_shot;
    gain_t* chimaera_shot;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* aimed_shot_artifact;
    proc_t* lock_and_load;
    proc_t* hunters_mark;
    proc_t* wild_call;
    proc_t* tier17_2pc_bm;
    proc_t* black_arrow_trinket_reset;
    proc_t* tier18_2pc_mm_wasted_proc;
    proc_t* tier18_2pc_mm_wasted_overwrite;
  } procs;

  real_ppm_t* ppm_hunters_mark;

  // Talents
  struct talents_t
  {
    // tier 1
    const spell_data_t* one_with_the_pack;
    const spell_data_t* way_of_the_cobra;
    const spell_data_t* dire_stable;
    
    const spell_data_t* lone_wolf;
    const spell_data_t* steady_focus;
    const spell_data_t* true_aim;

    const spell_data_t* animal_instincts;
    const spell_data_t* throwing_axes;
    const spell_data_t* way_of_moknathal;

    // tier 2
    const spell_data_t* stomp;
    const spell_data_t* exotic_munitions;
    const spell_data_t* chimaera_shot;

    const spell_data_t* black_arrow;
    const spell_data_t* explosive_shot;
    const spell_data_t* trick_shot;

    const spell_data_t* caltrops;
    const spell_data_t* steel_trap;
    const spell_data_t* improved_traps;

    // tier 3
    const spell_data_t* posthaste;
    const spell_data_t* farstrider;
    const spell_data_t* dash;
    
    // tier 4
    const spell_data_t* big_game_hunter;
    const spell_data_t* bestial_fury;
    const spell_data_t* blink_strikes;
    
    const spell_data_t* careful_aim;
    const spell_data_t* heightened_vulnerability;
    const spell_data_t* patient_sniper;

    const spell_data_t* dragonsfire_grenade;
    const spell_data_t* snake_hunter;

    // tier 5
    const spell_data_t* binding_shot;
    const spell_data_t* wyvern_sting;
    const spell_data_t* intimidation;

    const spell_data_t* camouflage;

    const spell_data_t* sticky_bomb;
    const spell_data_t* rangers_net;

    // tier 6
    const spell_data_t* a_murder_of_crows;
    const spell_data_t* barrage;
    const spell_data_t* volley;

    const spell_data_t* butchery;
    const spell_data_t* mortal_wounds;
    const spell_data_t* serpent_sting;

    // tier 7
    const spell_data_t* stampede;
    const spell_data_t* killer_cobra;
    const spell_data_t* aspect_of_the_beast;

    const spell_data_t* sidewinders;
    const spell_data_t* head_shot;
    const spell_data_t* lock_and_load;

    const spell_data_t* spitting_cobra;
    const spell_data_t* expert_trapper;
  } talents;

  // Specialization Spells
  struct specs_t
  {
    // Beast Mastery
    const spell_data_t* cobra_shot;
    const spell_data_t* kill_command;
    const spell_data_t* dire_beast;
    const spell_data_t* aspect_of_the_wild;
    const spell_data_t* beast_cleave;
    const spell_data_t* wild_call;
    const spell_data_t* bestial_wrath;
    const spell_data_t* kindred_spirits;
    const spell_data_t* exotic_beasts;
    
    // Marksmanship
    const spell_data_t* aimed_shot;
    const spell_data_t* bombardment;
    const spell_data_t* trueshot;
    const spell_data_t* lock_and_load;

    // Survival
    const spell_data_t* flanking_strike;
    const spell_data_t* raptor_strike;
    const spell_data_t* wing_clip;
    const spell_data_t* mongoose_bite;
    const spell_data_t* hatchet_toss;
    const spell_data_t* harpoon;
    const spell_data_t* muzzle;
    const spell_data_t* laceration;
    const spell_data_t* aspect_of_the_eagle;
    const spell_data_t* carve;
    const spell_data_t* tar_trap;
    const spell_data_t* survivalist;
  } specs;

  // Glyphs
  struct glyphs_t
  {
    const spell_data_t* arachnophobia;
    const spell_data_t* lesser_proportion;
    const spell_data_t* nesignwarys_nemesis;
    const spell_data_t* sparks;
    const spell_data_t* bullseye;
    const spell_data_t* firecracker;
    const spell_data_t* headhunter;
    const spell_data_t* hook;
    const spell_data_t* skullseye;
    const spell_data_t* trident;
  } glyphs;

  struct mastery_spells_t
  {
    const spell_data_t* master_of_beasts;
    const spell_data_t* sniper_training;
    const spell_data_t* hunting_companion;
  } mastery;

  struct artifact_spells_t
  {
    // Beast Mastery
    artifact_power_t titans_thunder;
    artifact_power_t master_of_beasts;
    artifact_power_t stormshot;
    artifact_power_t surge_of_the_stormgod;
    artifact_power_t spitting_cobras;
    artifact_power_t jaws_of_thunder;
    artifact_power_t wilderness_expert;
    artifact_power_t pack_leader;
    artifact_power_t unleash_the_beast;
    artifact_power_t focus_of_the_titans;
    artifact_power_t furious_swipes;

    // Marksmanship
    artifact_power_t windburst;
    artifact_power_t whispers_of_the_past;
    artifact_power_t call_of_the_hunter;
    artifact_power_t bullseye;
    artifact_power_t deadly_aim;
    artifact_power_t quick_shot;
    artifact_power_t critical_focus;
    artifact_power_t windrunners_guidance;
    artifact_power_t call_the_targets;
    artifact_power_t marked_for_death;
    artifact_power_t precision;
    artifact_power_t rapid_killing;

    // Survival
    artifact_power_t fury_of_the_eagle;
    artifact_power_t talon_strike;
    artifact_power_t eagles_bite;
    artifact_power_t aspect_of_the_skylord;
    artifact_power_t sharpened_beak;
    artifact_power_t raptors_cry;
    artifact_power_t hellcarver;
    artifact_power_t my_beloved_monster;
    artifact_power_t strength_of_the_mountain;
    artifact_power_t fluffy_go;
    artifact_power_t jagged_claws;
    artifact_power_t hunters_guile;
  } artifacts;

  stats_t* stats_stampede;
  stats_t* stats_tier17_4pc_bm;
  stats_t* stats_tier18_4pc_bm;

  double pet_multiplier;

  player_t* last_true_aim_target;

  hunter_t( sim_t* sim, const std::string& name, race_e r = RACE_NONE ):
    player_t( sim, HUNTER, name, r ),
    active( actives_t() ),
    pet_dire_beasts(),
    thunderhawk(),
    hati(),
    action_lightning_arrow_aimed_shot(),
    action_lightning_arrow_arcane_shot(),
    action_lightning_arrow_multi_shot(),
    beastlord( nullptr ),
    longview( nullptr ),
    blackness( nullptr ),
    thasdorah( nullptr ),
    titanstrike( nullptr ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    procs( procs_t() ),
    ppm_hunters_mark( nullptr ),
    talents( talents_t() ),
    specs( specs_t() ),
    glyphs( glyphs_t() ),
    mastery( mastery_spells_t() ),
    stats_stampede( nullptr ),
    stats_tier17_4pc_bm( nullptr ),
    stats_tier18_4pc_bm( nullptr ),
    pet_multiplier( 1.0 ),
    last_true_aim_target()
  {
    // Cooldowns
    cooldowns.explosive_shot  = get_cooldown( "explosive_shot" );
    cooldowns.black_arrow     = get_cooldown( "black_arrow" );
    cooldowns.bestial_wrath   = get_cooldown( "bestial_wrath" );
    cooldowns.kill_shot_reset = get_cooldown( "kill_shot_reset" );
    cooldowns.kill_shot_reset -> duration = find_spell( 90967 ) -> duration();
    cooldowns.trueshot        = get_cooldown( "trueshot" );
    cooldowns.dire_beast      = get_cooldown( "dire_beast" );
    cooldowns.kill_command    = get_cooldown( "kill_command" );

    summon_pet_str = "";
    base.distance = 40;
    base_gcd = timespan_t::from_seconds( 1.5 );

    regen_type = REGEN_DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  // Character Definition
  virtual void      init_spells() override;
  virtual void      init_base_stats() override;
  virtual void      create_buffs() override;
  virtual void      init_gains() override;
  virtual void      init_position() override;
  virtual void      init_procs() override;
  virtual void      init_rng() override;
  virtual void      init_scaling() override;
  virtual void      init_action_list() override;
  virtual void      arise() override;
  virtual void      reset() override;

  virtual void      regen( timespan_t periodicity = timespan_t::from_seconds( 0.25 ) ) override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_melee_crit() const override;
  virtual double    composite_spell_crit() const override;
  virtual double    composite_melee_haste() const override;
  virtual double    composite_player_critical_damage_multiplier() const override;
  virtual double    composite_rating_multiplier( rating_e rating ) const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    focus_regen_per_second() const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual void      invalidate_cache( cache_e ) override;
  virtual void      create_options() override;
  virtual expr_t*   create_expression( action_t*, const std::string& name ) override;
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() ) override;
  virtual void      create_pets() override;
  virtual resource_e primary_resource() const override { return RESOURCE_FOCUS; }
  virtual role_e    primary_role() const override { return ROLE_ATTACK; }
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual std::string      create_profile( save_e = SAVE_ALL ) override;
  virtual void      copy_from( player_t* source ) override;
  virtual void      armory_extensions( const std::string& r, const std::string& s, const std::string& c, cache::behavior_e ) override;

  virtual void      schedule_ready( timespan_t delta_time = timespan_t::zero( ), bool waiting = false ) override;
  virtual void      moving( ) override;
  virtual void      finish_moving() override;

  void              apl_default();
  void              apl_surv();
  void              apl_bm();
  void              apl_mm();

  void              add_item_actions( action_priority_list_t* list );
  void              add_racial_actions( action_priority_list_t* list );
  void              add_potion_action( action_priority_list_t* list, const std::string big_potion, const std::string little_potion, const std::string options = std::string() );

  target_specific_t<hunter_td_t> target_data;

  virtual hunter_td_t* get_target_data( player_t* target ) const override
  {
    hunter_td_t*& td = target_data[target];
    if ( !td ) td = new hunter_td_t( target, const_cast<hunter_t*>( this ) );
    return td;
  }

  double beast_multiplier()
  {
    double pm = pet_multiplier;
    if ( mastery.master_of_beasts -> ok() )
    {
      pm *= 1.0 + cache.mastery_value();
    }

    return pm;
  }

  // shared code to interrupt a steady focus pair if the ability is cast between steady/cobras
  virtual void no_steady_focus()
  {
    buffs.pre_steady_focus -> expire();
  }
};

static void do_trinket_init( hunter_t*                 r,
                             specialization_e          spec,
                             const special_effect_t*&  ptr,
                             const special_effect_t&   effect )
{
  if ( ! r -> find_spell( effect.spell_id ) -> ok() || r -> specialization() != spec )
  {
    return;
  }

  ptr = &( effect );
}

static void beastlord( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  do_trinket_init( hunter, HUNTER_BEAST_MASTERY, hunter -> beastlord, effect );
}

static void longview( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  do_trinket_init( hunter, HUNTER_MARKSMANSHIP, hunter -> longview, effect );
}

static void blackness( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  do_trinket_init( hunter, HUNTER_SURVIVAL, hunter -> blackness, effect );
}

static void thasdorah( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  do_trinket_init( hunter, HUNTER_MARKSMANSHIP, hunter -> thasdorah, effect );
}

static void titanstrike( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  do_trinket_init( hunter, HUNTER_BEAST_MASTERY, hunter -> titanstrike, effect );
}

// Template for common hunter action code. See priest_action_t.
template <class Base>
struct hunter_action_t: public Base
{
private:
  typedef Base ab;
public:
  typedef hunter_action_t base_t;

  bool lone_wolf;

  hunter_action_t( const std::string& n, hunter_t* player,
                   const spell_data_t* s = spell_data_t::nil() ):
                   ab( n, player, s ),
                   lone_wolf( player -> talents.lone_wolf -> ok() )
  {
  }

  virtual ~hunter_action_t() {}

  hunter_t* p()
  {
    return static_cast<hunter_t*>( ab::player );
  }
  const hunter_t* p() const
  {
    return static_cast<hunter_t*>( ab::player );
  }

  hunter_td_t* td( player_t* t ) const
  {
    return p() -> get_target_data( t );
  }

  virtual double action_multiplier() const
  {
    double am = ab::action_multiplier();

    if ( lone_wolf )
      am *= 1.0 + p() -> talents.lone_wolf -> effectN( 1 ).percent();

    return am;
  }

  virtual void impact( action_state_t* s ) override
  {
    ab::impact( s );

    // Bullseye artifact trait - procs from specials on targets below 20%
    // TODO: Apparently this only procs on a single impact for multi-shot
    if ( ab::special && p() -> thasdorah && p() -> artifacts.bullseye.rank() &&  s -> target -> health_percentage() < p() -> artifacts.bullseye.value() )
      p() -> buffs.bullseye -> trigger();
  }

  virtual double cast_regen() const
  {
    double cast_seconds = ab::execute_time().total_seconds();
    double sf_seconds = std::min( cast_seconds, p() -> buffs.steady_focus -> remains().total_seconds() );
    double regen = p() -> focus_regen_per_second();
    return ( regen * cast_seconds ) + ( regen * p() -> buffs.steady_focus -> current_value * sf_seconds );
  }

// action list expressions
  
  virtual expr_t* create_expression( const std::string& name )
  {
    if ( util::str_compare_ci( name, "cast_regen" ) )
    {
      // Return the focus that will be regenerated during the cast time or GCD of the target action.
      // This includes additional focus for the steady_shot buff if present, but does not include 
      // focus generated by dire beast.
      struct cast_regen_expr_t : public expr_t
      {
        hunter_action_t* action;
        cast_regen_expr_t( action_t* a ) :
          expr_t( "cast_regen" ),  action( debug_cast<hunter_action_t*>( a ) ) { }
        virtual double evaluate() override
        { return action -> cast_regen(); }
      };
      return new cast_regen_expr_t( this );
    }

    return ab::create_expression( name );
  }

  virtual void try_steady_focus()
  {
    if ( p() -> talents.steady_focus -> ok() )
      p() -> buffs.pre_steady_focus -> expire();
  }
};

// True Aim can only exist on one target at a time
void trigger_true_aim( hunter_t* p, player_t* t )
{
  if ( p -> talents.true_aim -> ok() )
  {
    hunter_td_t* td_curr = p -> get_target_data( t );

    // First attack, store target for later
    if ( p -> last_true_aim_target == nullptr )
      p -> last_true_aim_target = t;
    else
    {
      // Grab info about the previous target
      hunter_td_t* td_prev = p -> get_target_data( p -> last_true_aim_target );

      // Attacking a different target, reset the stacks, store current target for later
      if ( p -> last_true_aim_target != t )
      {
        td_prev -> debuffs.true_aim -> expire();
        p -> last_true_aim_target = t;
      }
    }

    // Apply one stack to current target
    td_curr -> debuffs.true_aim -> trigger();
  }
}

namespace pets
{

// ==========================================================================
// Hunter Pet
// ==========================================================================

struct hunter_pet_t: public pet_t
{
public:
  typedef pet_t base_t;

  hunter_pet_t( sim_t& sim, hunter_t& owner, const std::string& pet_name, pet_e pt = PET_HUNTER, bool guardian = false, bool dynamic = false ) :
    base_t( &sim, &owner, pet_name, pt, guardian, dynamic )
  {
  }

  hunter_t* o() const
  {
    return static_cast<hunter_t*>( owner );
  }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = base_t::composite_player_multiplier( school );
    m *= o() -> beast_multiplier();
    
    return m;
  }
};

// Template for common hunter pet action code. See priest_action_t.
template <class T_PET, class Base>
struct hunter_pet_action_t: public Base
{
private:
  typedef Base ab;
public:
  typedef hunter_pet_action_t base_t;

  hunter_pet_action_t( const std::string& n, T_PET& p,
                       const spell_data_t* s = spell_data_t::nil() ):
                       ab( n, &p, s )
  {

  }

  T_PET* p()
  {
    return static_cast<T_PET*>( ab::player );
  }
  const T_PET* p() const
  {
    return static_cast<T_PET*>( ab::player );
  }

  hunter_t* o()
  {
    return static_cast<hunter_t*>( p() -> o() );
  }
  const hunter_t* o() const
  {
    return static_cast<hunter_t*>( p() -> o() );
  }
};

// COPY PASTE of blademaster trinket code so we can support mastery for beastmaster
const std::string BLADEMASTER_PET_NAME = "mirror_image_(trinket)";

struct felstorm_tick_t : public melee_attack_t
{
  felstorm_tick_t( pet_t* p ) :
    melee_attack_t( "felstorm_tick", p, p -> find_spell( 184280 ) )
  {
    school = SCHOOL_PHYSICAL;

    background = special = may_crit = true;
    callbacks = false;
    aoe = -1;
    range = data().effectN( 1 ).radius();

    weapon = &( p -> main_hand_weapon );
  }

  bool init_finished() override
  {
    // Find first blademaster pet, it'll be the first trinket-created pet
    pet_t* main_pet = player -> cast_pet() -> owner -> find_pet( BLADEMASTER_PET_NAME );
    if ( player != main_pet )
    {
      stats = main_pet -> find_action( "felstorm_tick" ) -> stats;
    }

    return melee_attack_t::init_finished();
  }
};

struct felstorm_t : public melee_attack_t
{
  felstorm_t( pet_t* p, const std::string& opts ) :
    melee_attack_t( "felstorm", p, p -> find_spell( 184279 ) )
  {
    parse_options( opts );

    callbacks = may_miss = may_block = may_parry = false;
    dynamic_tick_action = hasted_ticks = true;
    trigger_gcd = timespan_t::from_seconds( 1.0 );

    tick_action = new felstorm_tick_t( p );
  }

  // Make dot long enough to last for the duration of the summon
  timespan_t composite_dot_duration( const action_state_t* ) const override
  { return sim -> expected_iteration_time; }

  bool init_finished() override
  {
    pet_t* main_pet = player -> cast_pet() -> owner -> find_pet( BLADEMASTER_PET_NAME );
    if ( player != main_pet )
    {
      stats = main_pet -> find_action( "felstorm" ) -> stats;
    }

    return melee_attack_t::init_finished();
  }
};

struct blademaster_pet_t : public hunter_pet_t
{
  action_t* felstorm;

  blademaster_pet_t( player_t* owner ) :
    hunter_pet_t( *(owner -> sim), *static_cast<hunter_t*>(owner), BLADEMASTER_PET_NAME, PET_NONE, true, true ),
    felstorm( nullptr )
  {
    main_hand_weapon.type = WEAPON_BEAST;
    // Verified 5/11/15, TODO: Check if this is still the same on live
    owner_coeff.ap_from_ap = 1.0;

    // Magical constants for base damage
    double damage_range = 0.4;
    double base_dps = owner -> dbc.spell_scaling( PLAYER_SPECIAL_SCALE, owner -> level() ) * 4.725;
    double min_dps = base_dps * ( 1 - damage_range / 2.0 );
    double max_dps = base_dps * ( 1 + damage_range / 2.0 );
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    main_hand_weapon.min_dmg =  min_dps * main_hand_weapon.swing_time.total_seconds();
    main_hand_weapon.max_dmg =  max_dps * main_hand_weapon.swing_time.total_seconds();
  }

  timespan_t available() const override
  { return timespan_t::from_seconds( 20.0 ); }

  void init_action_list() override
  {
    action_list_str = "felstorm,if=!ticking";

    pet_t::init_action_list();
  }

  void dismiss( bool expired = false ) override
  {
    hunter_pet_t::dismiss( expired );

    if ( dot_t* d = felstorm -> find_dot( felstorm -> target ) )
    {
      d -> cancel();
    }
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "felstorm" )
    {
      felstorm = new felstorm_t( this, options_str );
      return felstorm;
    }

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Hunter Main Pet
// ==========================================================================

struct hunter_main_pet_td_t: public actor_target_data_t
{
public:
  hunter_main_pet_td_t( player_t* target, hunter_main_pet_t* p );
};

struct hunter_main_pet_t: public hunter_pet_t
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
    buff_t* aspect_of_the_wild;
    buff_t* bestial_wrath;
    buff_t* stampede; 
    buff_t* beast_cleave;
    buff_t* tier17_4pc_bm;
    buff_t* tier18_4pc_bm;
  } buffs;

  // Gains
  struct gains_t
  {
    gain_t* steady_focus;
  } gains;

  // Benefits
  struct benefits_t
  {
    benefit_t* wild_hunt;
  } benefits;

  hunter_main_pet_t( sim_t& sim, hunter_t& owner, const std::string& pet_name, pet_e pt ):
    base_t( sim, owner, pet_name, pt ),
    active( actives_t() ),
    specs( specs_t() ),
    buffs( buffs_t() ),
    gains( gains_t() ),
    benefits( benefits_t() )
  {
    owner.hunter_main_pets.push_back( this );

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( owner.type, owner.level() ) * 0.25;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( owner.type, owner.level() ) * 0.25;
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
    if ( pet_type > PET_TENACITY_TYPE && pet_type < PET_CUNNING_TYPE ) return PET_CUNNING;
    return PET_FEROCITY;
  }

  //TODO: Reimplement Legion special abilities
  std::string unique_special()
  {
    switch ( pet_type )
    {
    case PET_CARRION_BIRD: return "";
    case PET_CAT:          return "";
    case PET_CORE_HOUND:   return "";
    case PET_DEVILSAUR:    return "";
    case PET_HYENA:        return "";
    case PET_MOTH:         return "";
    case PET_RAPTOR:       return "";
    case PET_SPIRIT_BEAST: return "";
    case PET_TALLSTRIDER:  return "";
    case PET_WASP:         return "";
    case PET_WOLF:         return "";
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
    case PET_CHIMAERA:     return "";
    case PET_DRAGONHAWK:   return "";
    case PET_NETHER_RAY:   return "";
    case PET_RAVAGER:      return "";
    case PET_SERPENT:      return "";
    case PET_SILITHID:     return "";
    case PET_SPIDER:       return "";
    case PET_SPOREBAT:     return "";
    case PET_WIND_SERPENT: return "";
    case PET_FOX:          return "";
    default: break;
    }
    return nullptr;
  }

  virtual void init_base_stats() override
  {
    base_t::init_base_stats();

    resources.base[RESOURCE_HEALTH] = util::interpolate( level(), 0, 4253, 6373 );
    resources.base[RESOURCE_FOCUS] = 100 + o() -> specs.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS );

    base_gcd = timespan_t::from_seconds( 1.50 );

    resources.infinite_resource[RESOURCE_FOCUS] = o() -> resources.infinite_resource[RESOURCE_FOCUS];

    owner_coeff.ap_from_ap = 0.6;
    owner_coeff.sp_from_ap = 0.6;
  }

  virtual void create_buffs() override
  {
    base_t::create_buffs();

    buffs.aspect_of_the_wild = buff_creator_t( this, 193530, "aspect_of_the_wild" );

    buffs.bestial_wrath = buff_creator_t( this, 19574, "bestial_wrath" ).activated( true );
    buffs.bestial_wrath -> cooldown -> duration = timespan_t::zero();
    buffs.bestial_wrath -> default_value = buffs.bestial_wrath -> data().effectN( 2 ).percent();
    if ( o() -> talents.bestial_fury -> ok() )
      buffs.bestial_wrath -> default_value += o() -> talents.bestial_fury -> effectN( 1 ).percent();

    // Use buff to indicate whether the pet is a stampede summon
    buffs.stampede          = buff_creator_t( this, 130201, "stampede" )
      .duration( timespan_t::from_millis( 40027 ) )
      // Added 0.027 seconds to properly reflect haste threshholds seen in game.
      .activated( true )
      /*.quiet( true )*/;

    double cleave_value     = o() -> find_specialization_spell( "Beast Cleave" ) -> effectN( 1 ).percent();
    buffs.beast_cleave      = buff_creator_t( this, 118455, "beast_cleave" ).activated( true ).default_value( cleave_value );

    buffs.tier17_4pc_bm = buff_creator_t( this, 178875, "tier17_4pc_bm" )
      .default_value( owner -> find_spell( 178875 ) -> effectN( 2 ).percent() )
      .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    buffs.tier18_4pc_bm = buff_creator_t( this, "tier18_4pc_bm" )
      .default_value( owner -> find_spell( 178875 ) -> effectN( 2 ).percent() )
      .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  virtual void init_special_effects() override
  {
    base_t::init_special_effects();
    hunter_t* hunter = debug_cast<hunter_t*>( owner );
    const special_effect_t* bl = hunter -> beastlord;
    if ( bl )
    {
      const spell_data_t* data = hunter -> find_spell( bl -> spell_id );
      buffs.bestial_wrath -> buff_duration += timespan_t::from_seconds( data -> effectN( 1 ).base_value() );
      double increase = data -> effectN( 2 ).average( bl -> item ) + data -> effectN( 3 ).average( bl -> item );
      buffs.bestial_wrath -> default_value += increase / 100.0;
    }
  }

  virtual void init_gains() override
  {
    base_t::init_gains();

    gains.steady_focus      = get_gain( "steady_focus" );
  }

  virtual void init_benefits() override
  {
    base_t::init_benefits();

    benefits.wild_hunt = get_benefit( "wild_hunt" );
  }

  virtual void init_action_list() override
  {
    if ( action_list_str.empty() )
    {
      action_list_str += "/auto_attack";
      action_list_str += "/snapshot_stats";

      std::string special = unique_special();
      if ( !special.empty() )
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

  virtual double composite_melee_crit() const override
  {
    double ac = base_t::composite_melee_crit();
    ac += specs.spiked_collar -> effectN( 3 ).percent();

    if ( buffs.aspect_of_the_wild -> check() )
      ac += buffs.aspect_of_the_wild -> check_value();

    return ac;
  }

  void regen( timespan_t periodicity ) override
  {
    player_t::regen( periodicity );
    if ( o() -> buffs.steady_focus -> up() )
    {
      double base = focus_regen_per_second() * o() -> buffs.steady_focus -> current_value;
      resource_gain( RESOURCE_FOCUS, base * periodicity.total_seconds(), gains.steady_focus );
    }
  }

  double focus_regen_per_second() const override
  {
    return o() -> focus_regen_per_second() * 1.25;
  }

  virtual double composite_melee_speed() const override
  {
    double ah = base_t::composite_melee_speed();

    ah *= 1.0 / ( 1.0 + specs.spiked_collar -> effectN( 2 ).percent() );

    return ah;
  }

  virtual void summon( timespan_t duration = timespan_t::zero() ) override
  {
    base_t::summon( duration );

    o() -> active.pet = this;
  }

  bool tier_pet_summon( timespan_t duration )
  {
    // More than one 4pc pet could be up at a time
    if ( this == o() -> active.pet || buffs.stampede -> check() || buffs.tier17_4pc_bm -> check() || buffs.tier18_4pc_bm -> check() )
      return false;

    type = PLAYER_GUARDIAN;

    bool is_t18 = o() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T18, B4 );

    for ( size_t i = 0; i < stats_list.size(); ++i )
    {
      if ( !( stats_list[i] -> parent ) )
      {
        if ( is_t18 )
          o() -> stats_tier18_4pc_bm -> add_child( stats_list[i] );
        else
          o() -> stats_tier17_4pc_bm -> add_child( stats_list[i] );
      }
    }
    base_t::summon( duration );
    
    double ap_coeff = 0.6; // default coefficient for pets
    if ( is_t18 )
    {
      ap_coeff = 0.4; //  (1.0 + o() -> buffs.focus_fire -> current_value) 
      buffs.tier18_4pc_bm -> trigger();
    }
    else if ( o() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T18, B4 ) )
    {
      // pet appears at the target
      current.distance = 0;
      buffs.tier17_4pc_bm -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );
    }

    owner_coeff.ap_from_ap = ap_coeff;
    owner_coeff.sp_from_ap = ap_coeff;

    // pet swings immediately (without an execute time)
    if ( !main_hand_attack -> execute_event ) main_hand_attack -> execute();
    return true;
  }

  void stampede_summon( timespan_t duration )
  {
    if ( this == o() -> active.pet || buffs.tier17_4pc_bm -> check() || buffs.tier18_4pc_bm -> check() )
      return;

    type = PLAYER_GUARDIAN;

    for ( size_t i = 0; i < stats_list.size(); ++i )
      if ( !( stats_list[i] -> parent ) )
        o() -> stats_stampede -> add_child( stats_list[i] );

    base_t::summon( duration );
    // pet appears at the target
    current.distance = 0;
    owner_coeff.ap_from_ap = 0.6;
    owner_coeff.sp_from_ap = 0.6;

    buffs.stampede -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );

    // pet swings immediately (without an execute time)
    if ( !main_hand_attack -> execute_event ) main_hand_attack -> execute();
  }

  virtual void demise() override
  {
    base_t::demise();

    if ( o() -> active.pet == this )
      o() -> active.pet = nullptr;
  }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = base_t::composite_player_multiplier( school );

    if ( !buffs.stampede -> check() && buffs.bestial_wrath -> up() )
      m *= 1.0 + buffs.bestial_wrath -> current_value;

    // from Nimox: 178875 is the 4pc BM pet damage buff
    if ( buffs.tier17_4pc_bm -> up() )
      m *= 1.0 + buffs.tier17_4pc_bm -> current_value;

    // Pet combat experience
    m *= 1.0 + specs.combat_experience -> effectN( 2 ).percent();

    return m;
  }

  target_specific_t<hunter_main_pet_td_t> target_data;

  virtual hunter_main_pet_td_t* get_target_data( player_t* target ) const override
  {
    hunter_main_pet_td_t*& td = target_data[target];
    if ( !td )
      td = new hunter_main_pet_td_t( target, const_cast<hunter_main_pet_t*>( this ) );
    return td;
  }

  virtual resource_e primary_resource() const override { return RESOURCE_FOCUS; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override;

  virtual void init_spells() override;

  void moving() override
  {
    return;
  }
};

namespace actions
{

// Template for common hunter main pet action code. See priest_action_t.
template <class Base>
struct hunter_main_pet_action_t: public hunter_pet_action_t < hunter_main_pet_t, Base >
{
private:
  typedef hunter_pet_action_t<hunter_main_pet_t, Base> ab;
public:
  typedef hunter_main_pet_action_t base_t;

  bool special_ability;

  hunter_main_pet_action_t( const std::string& n, hunter_main_pet_t* player,
                            const spell_data_t* s = spell_data_t::nil() ):
                            ab( n, *player, s ),
                            special_ability( false )
  {
    if ( ab::data().rank_str() && !strcmp( ab::data().rank_str(), "Special Ability" ) )
      special_ability = true;
  }

  hunter_main_pet_t* p() const
  {
    return static_cast<hunter_main_pet_t*>( ab::player );
  }

  hunter_t* o() const
  {
    return static_cast<hunter_t*>( p() -> o() );
  }

  hunter_main_pet_td_t* td( player_t* t = nullptr ) const
  {
    return p() -> get_target_data( t ? t : ab::target );
  }
};

// ==========================================================================
// Hunter Pet Attacks
// ==========================================================================

struct hunter_main_pet_attack_t: public hunter_main_pet_action_t < melee_attack_t >
{
  hunter_main_pet_attack_t( const std::string& n, hunter_main_pet_t* player,
                            const spell_data_t* s = spell_data_t::nil() ):
                            base_t( n, player, s )
  {
    special = true;
    may_crit = true;
  }

  virtual bool ready() override
  {
    // Stampede pets don't use abilities or spells
    if ( p() -> buffs.stampede -> check() || p() -> buffs.tier17_4pc_bm -> check() || p() -> buffs.tier18_4pc_bm -> check() )
      return false;

    return base_t::ready();
  }
};

// Beast Cleave ==============================================================

struct beast_cleave_attack_t: public hunter_main_pet_attack_t
{
  beast_cleave_attack_t( hunter_main_pet_t* p ):
    hunter_main_pet_attack_t( "beast_cleave_attack", p, p -> find_spell( 22482 ) )
  {
    may_miss = false;
    may_crit = false;
    proc = false;
    callbacks = false;
    background = true;
    school = SCHOOL_PHYSICAL;
    aoe = -1;
    range = -1.0;
    radius = 10.0;
    // The starting damage includes all the buffs
    weapon_multiplier = 0;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    hunter_main_pet_attack_t::available_targets( tl );

    for ( size_t i = 0; i < tl.size(); i++ )
    {
      if ( tl[i] == target ) // Cannot hit the original target.
      {
        tl.erase( tl.begin() + i );
        break;
      }
    }

    return tl.size();
  }
};

static void trigger_beast_cleave( action_state_t* s )
{
  if ( !s -> action -> result_is_hit( s -> result ) )
    return;

  if ( s -> action -> sim -> active_enemies == 1 )
    return;

  hunter_main_pet_t* p = debug_cast<hunter_main_pet_t*>( s -> action -> player );

  if ( !p -> buffs.beast_cleave -> check() )
    return;

  if ( p -> active.beast_cleave -> target != s -> target )
    p -> active.beast_cleave -> target_cache.is_valid = false;

  p -> active.beast_cleave -> target = s -> target;
  double cleave = s -> result_total * p -> buffs.beast_cleave -> current_value;

  p -> active.beast_cleave -> base_dd_min = cleave;
  p -> active.beast_cleave -> base_dd_max = cleave;
  p -> active.beast_cleave -> execute();
}

// Pet Melee ================================================================

struct pet_melee_t: public hunter_main_pet_attack_t
{
  pet_melee_t( hunter_main_pet_t* player ):
    hunter_main_pet_attack_t( "melee", player )
  {
    special = false;
    weapon = &( player -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    background = true;
    repeating = true;
    auto_attack = true;
    school = SCHOOL_PHYSICAL;

  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_main_pet_attack_t::impact( s );

    trigger_beast_cleave( s );
  }
};

// Pet Auto Attack ==========================================================

struct pet_auto_attack_t: public hunter_main_pet_attack_t
{
  pet_auto_attack_t( hunter_main_pet_t* player, const std::string& options_str ):
    hunter_main_pet_attack_t( "auto_attack", player, nullptr )
  {
    parse_options( options_str );

    p() -> main_hand_attack = new pet_melee_t( player );
    trigger_gcd = timespan_t::zero();
    school = SCHOOL_PHYSICAL;
  }

  virtual void execute() override
  {
    p() -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready() override
  {
    return( p() -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// Pet Claw/Bite/Smack ======================================================

struct basic_attack_t: public hunter_main_pet_attack_t
{
  double chance_invigoration;
  double gain_invigoration;

  basic_attack_t( hunter_main_pet_t* p, const std::string& name, const std::string& options_str ):
    hunter_main_pet_attack_t( name, p, p -> find_pet_spell( name ) )
  {
    parse_options( options_str );
    school = SCHOOL_PHYSICAL;

    attack_power_mod.direct = 1.0 / 3.0;
    base_multiplier *= 1.0 + p -> specs.spiked_collar -> effectN( 1 ).percent();
    chance_invigoration = p -> find_spell( 53397 ) -> proc_chance();
    gain_invigoration = p -> find_spell( 53398 ) -> effectN( 1 ).resource( RESOURCE_FOCUS );
  }

  // Override behavior so that Basic Attacks use hunter's attack power rather than the pet's
  double composite_attack_power() const override
  {
    return o() -> cache.attack_power() * o()->composite_attack_power_multiplier();
  }

  void impact( action_state_t* s ) override
  {
    hunter_main_pet_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_beast_cleave( s );
    }
  }

  bool use_wild_hunt() const
  {
    // comment out to avoid procs
    return p() -> resources.current[RESOURCE_FOCUS] > 50;
  }

  double action_multiplier() const override
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

  double cost() const override
  {
    double base_cost = hunter_main_pet_attack_t::cost();
    double c = base_cost;

    if ( use_wild_hunt() )
      c *= 1.0 + p() -> specs.wild_hunt -> effectN( 2 ).percent();

    return c;
  }
};

// Devilsaur Monstrous Bite =================================================

struct monstrous_bite_t: public hunter_main_pet_attack_t
{
  monstrous_bite_t( hunter_main_pet_t* p, const std::string& options_str ):
    hunter_main_pet_attack_t( "monstrous_bite", p, p -> find_spell( 54680 ) )
  {
    parse_options( options_str );
    school = SCHOOL_PHYSICAL;
    stats -> school = SCHOOL_PHYSICAL;
  }
};

// Kill Command (pet) =======================================================

struct kill_command_t: public hunter_main_pet_attack_t
{
  kill_command_t( hunter_main_pet_t* p ):
    hunter_main_pet_attack_t( "kill_command", p, p -> find_spell( 83381 ) )
  {
    background = true;
    proc = true;
    school = SCHOOL_PHYSICAL;
    range = 25;

    // The hardcoded parameter is taken from the $damage value in teh tooltip. e.g., 1.36 below
    // $damage = ${ 1.5*($83381m1 + ($RAP*  1.632   ))*$<bmMastery> }
    attack_power_mod.direct  = 1.632; // Hard-coded in tooltip.
  }

  // Override behavior so that Kill Command uses hunter's attack power rather than the pet's
  double composite_attack_power() const override
  {
    return o() -> cache.attack_power() * o()->composite_attack_power_multiplier();
  }

  bool usable_moving() const override
  {
    return true;
  }
};

// ==========================================================================
// Hunter Pet Spells
// ==========================================================================

struct hunter_main_pet_spell_t: public hunter_main_pet_action_t < spell_t >
{
  hunter_main_pet_spell_t( const std::string& n, hunter_main_pet_t* player,
                           const spell_data_t* s = spell_data_t::nil() ):
                           base_t( n, player, s )
  {
  }

  virtual bool ready() override
  {
    // Stampede pets don't use abilities or spells
    if ( p() -> buffs.stampede -> check() || p() -> buffs.tier17_4pc_bm -> check() || p() -> buffs.tier18_4pc_bm -> check() )
      return false;

    return base_t::ready();
  }
};

// ==========================================================================
// Unique Pet Specials
// ==========================================================================

// Wolf/Devilsaur Furious Howl ==============================================

struct furious_howl_t: public hunter_main_pet_spell_t
{
  furious_howl_t( hunter_main_pet_t* player, const std::string& options_str ):
    hunter_main_pet_spell_t( "furious_howl", player, player -> find_pet_spell( "Furious Howl" ) )
  {
    parse_options( options_str );

    harmful = false;
  }
};

// chimaera Froststorm Breath ================================================

struct froststorm_breath_t: public hunter_main_pet_spell_t
{
  struct froststorm_breath_tick_t: public hunter_main_pet_spell_t
  {
    froststorm_breath_tick_t( hunter_main_pet_t* player ):
      hunter_main_pet_spell_t( "froststorm_breath_tick", player, player -> find_spell( 95725 ) )
    {
      attack_power_mod.direct = 0.144; // hardcoded into tooltip, 2012/08 checked 2015/02/21
      background = true;
      direct_tick = true;
    }
  };

  froststorm_breath_tick_t* tick_spell;
  froststorm_breath_t( hunter_main_pet_t* player, const std::string& options_str ):
    hunter_main_pet_spell_t( "froststorm_breath", player, player -> find_pet_spell( "Froststorm Breath" ) )
  {
    parse_options( options_str );
    channeled = true;
    tick_spell = new froststorm_breath_tick_t( player );
    add_child( tick_spell );
  }

  virtual void init() override
  {
    hunter_main_pet_spell_t::init();

    tick_spell -> stats = stats;
  }

  virtual void tick( dot_t* d ) override
  {
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick, d -> state -> target );
  }
};

} // end namespace pets::actions


hunter_main_pet_td_t::hunter_main_pet_td_t( player_t* target, hunter_main_pet_t* p ):
actor_target_data_t( target, p )
{
}
// hunter_pet_t::create_action ==============================================

action_t* hunter_main_pet_t::create_action( const std::string& name,
                                            const std::string& options_str )
{
  using namespace actions;

  if ( name == "auto_attack" ) return new          pet_auto_attack_t( this, options_str );
  if ( name == "claw" ) return new                 basic_attack_t( this, "Claw", options_str );
  if ( name == "bite" ) return new                 basic_attack_t( this, "Bite", options_str );
  if ( name == "smack" ) return new                basic_attack_t( this, "Smack", options_str );
  if ( name == "froststorm_breath" ) return new    froststorm_breath_t( this, options_str );
  if ( name == "furious_howl" ) return new         furious_howl_t( this, options_str );
  if ( name == "monstrous_bite" ) return new       monstrous_bite_t( this, options_str );
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
  if ( o() -> specs.beast_cleave -> ok() )
  {
    active.beast_cleave = new actions::beast_cleave_attack_t( this );
  }
}

// ==========================================================================
// Dire Critter
// ==========================================================================

struct dire_critter_t: public hunter_pet_t
{
  struct melee_t: public hunter_pet_action_t < dire_critter_t, melee_attack_t >
  {
    int focus_gain;
    melee_t( dire_critter_t& p ):
      base_t( "dire_beast_melee", p )
    {
      weapon = &( player -> main_hand_weapon );
      weapon_multiplier = 0;
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = player -> dbc.spell_scaling( p.o() -> type, p.o() -> level() );
      attack_power_mod.direct = 0.5714;
      school = SCHOOL_PHYSICAL;
      trigger_gcd = timespan_t::zero();
      background = true;
      repeating = true;
      may_glance = true;
      may_crit = true;
      special = false;
      energize_type = ENERGIZE_ON_HIT;
      energize_resource = RESOURCE_FOCUS;
      energize_amount = player -> find_spell( 120694 ) -> effectN( 1 ).base_value();
      if ( o() -> talents.dire_stable -> ok() )
        focus_gain += o() -> talents.dire_stable -> effectN( 1 ).base_value();
      base_multiplier *= 1.15; // Hotfix
    }

    bool init_finished() override
    {
      if ( p() -> o() -> pet_dire_beasts[ 0 ] )
        stats = p() -> o() -> pet_dire_beasts[ 0 ] -> get_stats( "dire_beast_melee" );

      return hunter_pet_action_t<dire_critter_t, melee_attack_t>::init_finished();
    }
  };

  dire_critter_t( hunter_t& owner, size_t index ):
    hunter_pet_t( *owner.sim, owner, std::string( "dire_beast_" ) + util::to_string( index ), PET_HUNTER, true /*GUARDIAN*/ )
  {
    owner_coeff.ap_from_ap = 1.0;
    regen_type = REGEN_DISABLED;
  }

  virtual void init_base_stats() override
  {
    hunter_pet_t::init_base_stats();

    // FIXME numbers are from treant. correct them.
    resources.base[RESOURCE_HEALTH] = 9999; // Level 85 value
    resources.base[RESOURCE_MANA] = 0;

    stamina_per_owner = 0;
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, o() -> level() );
    main_hand_weapon.max_dmg    = main_hand_weapon.min_dmg;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2 );

    main_hand_attack = new melee_t( *this );
  }

  virtual void summon( timespan_t duration = timespan_t::zero() ) override
  {
    hunter_pet_t::summon( duration );

    // attack immediately on summons
    main_hand_attack -> execute();
  }
};

// ==========================================================================
// Hati 
// ==========================================================================

struct hati_t: public hunter_pet_t
{
  struct melee_t: public hunter_pet_action_t < hati_t, melee_attack_t >
  {
    melee_t( hati_t& p ):
      base_t( "hati_melee", p )
    {
      weapon = &( player -> main_hand_weapon );
      weapon_multiplier = 0;
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = player -> dbc.spell_scaling( p.o() -> type, p.o() -> level() );
      attack_power_mod.direct = 0.5714;
      school = SCHOOL_PHYSICAL;
      trigger_gcd = timespan_t::zero();
      background = true;
      repeating = true;
      may_glance = true;
      may_crit = true;
      special = false;
      base_multiplier *= 1.15;
    }
  };

  hati_t( hunter_t& owner ):
  hunter_pet_t( *owner.sim, owner, std::string( "hati" ), PET_HUNTER, true /*GUARDIAN*/ )
  {
    owner_coeff.ap_from_ap = 1.0;
    regen_type = REGEN_DISABLED;
  }

  virtual void init_base_stats() override
  {
    hunter_pet_t::init_base_stats();

    stamina_per_owner = 0;
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, o() -> level() );
    main_hand_weapon.max_dmg    = main_hand_weapon.min_dmg;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2 );

    main_hand_attack = new melee_t( *this );
  }

  virtual void summon( timespan_t duration = timespan_t::zero() ) override
  {
    hunter_pet_t::summon( duration );

    // attack immediately on summons
    main_hand_attack -> execute();
  }
};

} // end namespace pets

namespace attacks
{

// ==========================================================================
// Hunter Attacks
// ==========================================================================

struct hunter_ranged_attack_t: public hunter_action_t < ranged_attack_t >
{
  hunter_ranged_attack_t( const std::string& n, hunter_t* player,
                          const spell_data_t* s = spell_data_t::nil() ):
                          base_t( n, player, s )
  {
    if ( player -> main_hand_weapon.type == WEAPON_NONE )
      background = true;

    special = true;
    may_crit = true;
    tick_may_crit = true;
    may_parry = false;
    may_block = false;
  }

  virtual void init() override
  {
    if ( player -> main_hand_weapon.group() != WEAPON_RANGED )
      background = true;
    base_t::init();
  }

  virtual bool usable_moving() const override
  {
    return true;
  }

  virtual void execute() override
  {
    ranged_attack_t::execute();
    try_steady_focus();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = base_t::execute_time();

    if ( t == timespan_t::zero() || base_execute_time < timespan_t::zero() )
      return timespan_t::zero();

    return t;
  }

  virtual void trigger_steady_focus( bool require_pre )
  {
    if ( !p() -> talents.steady_focus -> ok() )
      return;

    if ( require_pre )
    {
      p() -> buffs.pre_steady_focus -> trigger( 1 );
      if ( p() -> buffs.pre_steady_focus -> stack() < 2 )
        return;
    }

    // either two required shots have happened or we don't require them
    double regen_buff = p() -> buffs.steady_focus -> data().effectN( 1 ).percent();
    p() -> buffs.steady_focus -> trigger( 1, regen_buff );
    p() -> buffs.pre_steady_focus -> expire();
  }

  void trigger_piercing_shots( action_state_t* s )
  {
    double amount = s -> result_amount;

    residual_action::trigger( p() -> active.piercing_shots, s -> target, amount );
  }
};

struct hunter_melee_attack_t: public hunter_action_t < melee_attack_t >
{
  hunter_melee_attack_t( const std::string& n, hunter_t* player,
                          const spell_data_t* s = spell_data_t::nil() ):
                          base_t( n, player, s )
  {
    if ( player -> main_hand_weapon.type == WEAPON_NONE )
      background = true;

    weapon = &( player -> main_hand_weapon );
    special = true;
    may_crit = true;
    tick_may_crit = true;
    may_parry = false;
    may_block = false;
  }
};

// Ranged Attack ============================================================

struct ranged_t: public hunter_ranged_attack_t
{
  bool first_shot;
  ranged_t( hunter_t* player, const char* name = "ranged", const spell_data_t* s = spell_data_t::nil() ):
    hunter_ranged_attack_t( name, player, s ), first_shot( true )
  {
    school = SCHOOL_PHYSICAL;
    weapon = &( player -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    background = true;
    repeating = true;
    special = false;
  }

  void reset() override
  {
    hunter_ranged_attack_t::reset();

    first_shot = true;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = hunter_ranged_attack_t::execute_time();
    if ( first_shot )
      return timespan_t::from_millis( 100 );
    else
      return t;
  }

  virtual void execute() override
  {
    if ( first_shot )
      first_shot = false;
    hunter_ranged_attack_t::execute();
  }

  virtual void try_steady_focus() override
  {
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> active.ammo != NO_AMMO )
      {
        if ( p() -> active.ammo == POISONED_AMMO )
        {
          const spell_data_t* poisoned_tick = p() -> find_spell( 170661 );
          double damage = s -> attack_power * ( poisoned_tick -> effectN( 1 ).ap_coeff() *
                                                ( poisoned_tick -> duration() / poisoned_tick -> effectN( 1 ).period() ) );
          residual_action::trigger(
            p() -> active.poisoned_ammo, // ignite spell
            s -> target, // target
            damage );
        }
        else if ( p() -> active.ammo == FROZEN_AMMO )
          p() -> active.frozen_ammo -> execute();
        else if ( p() -> active.ammo == INCENDIARY_AMMO )
          p() -> active.incendiary_ammo -> execute();
      }
    }
  }
};

// Melee attack ==============================================================

struct melee_t: public hunter_melee_attack_t
{
  bool first;
  melee_t( hunter_t* player, const char* name = "auto_attack", const spell_data_t* s = spell_data_t::nil() ):
    hunter_melee_attack_t( name, player, s ), first( true )
  {
    school             = SCHOOL_PHYSICAL;
    weapon             = &( player -> main_hand_weapon );
    base_execute_time  = player -> main_hand_weapon.swing_time;
    background         = true;
    repeating          = true;
    special            = false;
    trigger_gcd        = timespan_t::zero();
  }

  virtual timespan_t execute_time() const override
  {
  if ( ! player -> in_combat ) 
    return timespan_t::from_seconds( 0.01 );
    if ( first )
      return timespan_t::zero();
    else
      return hunter_melee_attack_t::execute_time();;
  }

  virtual void execute() override
  {
    if ( first )
      first = false;
    hunter_melee_attack_t::execute();
  }
};

// Auto Shot ================================================================

struct auto_shot_t: public ranged_t
{
  auto_shot_t( hunter_t* p ): ranged_t( p, "auto_shot", spell_data_t::nil() )
  {
    school = SCHOOL_PHYSICAL;
    range = 40.0;
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    if ( rng().roll( p() -> talents.lock_and_load -> proc_chance() ) )
    {
      p() -> buffs.lock_and_load -> trigger( 2 );
      p() -> procs.lock_and_load -> occur();
    }

    if ( s -> result == RESULT_CRIT && p() -> specialization() == HUNTER_BEAST_MASTERY )
    {
      double wild_call_chance = p() -> specs.wild_call -> proc_chance();

      if ( p() -> talents.one_with_the_pack -> ok() )
        wild_call_chance += p() -> talents.one_with_the_pack -> effectN( 1 ).percent();

      if ( rng().roll( wild_call_chance ) )
      {
        p() -> cooldowns.dire_beast -> reset( true );
        p() -> procs.wild_call -> occur();
      }
    }
  }

  virtual double composite_target_crit( player_t* t ) const override
  {
    double cc= ranged_t::composite_target_crit( t );

    cc += p() -> buffs.big_game_hunter -> value();

    return cc;
  }
};

struct start_attack_t: public hunter_ranged_attack_t
{
  start_attack_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "start_auto_shot", p, spell_data_t::nil() )
  {
    parse_options( options_str );

    p -> main_hand_attack = new auto_shot_t( p );
    stats = p -> main_hand_attack -> stats;
    ignore_false_positive = true;

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    p() -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready() override
  {
    return( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// Auto attack =======================================================================

struct auto_attack_t: public hunter_melee_attack_t
{
  auto_attack_t( hunter_t* player, const std::string& options_str ) :
    hunter_melee_attack_t( "auto_attack", player, spell_data_t::nil() )
  {
    parse_options( options_str );
    player -> main_hand_attack = new melee_t( player );
  }

  virtual void execute() override
  {
    player -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready() override
  {
    if ( player -> is_moving() )
      return false;

    return ( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

//==============================
// Shared attacks
//==============================

// Multi Shot Attack =================================================================

struct multi_shot_t: public hunter_ranged_attack_t
{
  double focus_gain;
  multi_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "multi_shot", p, p -> find_class_spell( "Multi-Shot" ) )
  {
    parse_options( options_str );

    aoe = -1;

    if ( p -> thasdorah )
      base_multiplier *= 1.0 + p -> artifacts.call_the_targets.percent();

    if ( p -> specialization() == HUNTER_MARKSMANSHIP )
    {
      energize_type = ENERGIZE_PER_HIT;
      energize_resource = RESOURCE_FOCUS;
      energize_amount = p -> find_spell( 213363 ) -> effectN( 1 ).resource( RESOURCE_FOCUS );
    }
  }
  
  virtual void try_steady_focus() override
  {
    trigger_steady_focus( true );
  }

  virtual double cost() const override
  {
    return focus_gain > 0 ? 0 : hunter_ranged_attack_t::cost();
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();
    if ( p() -> buffs.bombardment -> up() )
      am *= 1 + p() -> buffs.bombardment -> data().effectN( 2 ).percent();
    return am;
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();

    pets::hunter_main_pet_t* pet = p() -> active.pet;
    if ( pet && p() -> specs.beast_cleave -> ok() )
      pet -> buffs.beast_cleave -> trigger();
    
    if ( result_is_hit( execute_state -> result ) )
    {
      // Hunter's Mark applies on cast to all affected targets or none based on RPPM (8*haste).
      // This loop goes through the target list for multi-shot and applies the debuffs on proc.
      // Multi-shot also grants 2 focus per target hit on cast.
      if ( p() -> specialization() == HUNTER_MARKSMANSHIP )
      {
        if ( p() -> buffs.trueshot -> up() || p() -> ppm_hunters_mark -> trigger() )
        {
          std::vector<player_t*> multi_shot_targets = execute_state -> action -> target_list();
          for ( size_t i = 0; i < multi_shot_targets.size(); i++ )
            td( multi_shot_targets[i] ) -> debuffs.hunters_mark -> trigger();

          p() -> procs.hunters_mark -> occur();
          p() -> buffs.hunters_mark_exists -> trigger();
        }
      }
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( p() -> specialization() == HUNTER_MARKSMANSHIP && result_is_hit( s -> result ) )
    {
      if ( s -> result == RESULT_CRIT )
        p() -> buffs.bombardment -> trigger();
    }
  }
};

// Exotic Munitions ==================================================================

struct exotic_munitions_poisoned_ammo_t: public residual_action::residual_periodic_action_t < hunter_ranged_attack_t >
{
  exotic_munitions_poisoned_ammo_t( hunter_t* p, const char* name, const spell_data_t* s ):
    base_t( name, p, s )
  {
    may_crit = true;
    tick_may_crit = true;
  }

  void init() override
  {
    base_t::init();

    snapshot_flags |= STATE_CRIT | STATE_TGT_CRIT ;
  }
};

struct exotic_munitions_incendiary_ammo_t: public hunter_ranged_attack_t
{
  exotic_munitions_incendiary_ammo_t( hunter_t* p, const char* name, const spell_data_t* s ):
    hunter_ranged_attack_t( name, p, s )
  {
    aoe = -1;
  }
};

struct exotic_munitions_frozen_ammo_t: public hunter_ranged_attack_t
{
  exotic_munitions_frozen_ammo_t( hunter_t* p, const char* name, const spell_data_t* s ):
    hunter_ranged_attack_t( name, p, s )
  {
  }
};

struct exotic_munitions_t: public hunter_ranged_attack_t
{
  std::string ammo_type;
  exotic_munitions swap_ammo;
  exotic_munitions_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "exotic_munitions", player, player -> talents.exotic_munitions )
  {
    add_option( opt_string( "ammo_type", ammo_type ) );
    parse_options( options_str );

    callbacks = false;
    harmful = false;
    ignore_false_positive = true;

    if ( !ammo_type.empty() )
    {
      if ( ammo_type == "incendiary" || ammo_type == "incen" || ammo_type == "incendiary_ammo" || ammo_type == "fire" )
        swap_ammo = INCENDIARY_AMMO;
      else if ( ammo_type == "poisoned" || ammo_type == "poison" || ammo_type == "poisoned_ammo" )
        swap_ammo = POISONED_AMMO;
      else if ( ammo_type == "frozen" || ammo_type == "frozen_ammo" || ammo_type == "ice"  )
        swap_ammo = FROZEN_AMMO;
    }
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();
    p() -> active.ammo = swap_ammo;
  }

  bool ready() override
  {
    if ( p() -> active.ammo == swap_ammo )
      return false;

    return hunter_ranged_attack_t::ready();
  }
};

// Lightning Arrow (Tier 15 4-piece bonus) ==================================

struct lightning_arrow_t: public hunter_ranged_attack_t
{
  lightning_arrow_t( hunter_t* p, const std::string& attack_suffix ):
    hunter_ranged_attack_t( "lightning_arrow" + attack_suffix, p, p -> find_spell( 138366 ) )
  {
    school = SCHOOL_NATURE;
    proc = true;
    background = true;
  }
};

//==============================
// Beast Mastery attacks  
//==============================

// Chimaera Shot =====================================================================

struct chimaera_shot_impact_t: public hunter_ranged_attack_t
{
  chimaera_shot_impact_t( hunter_t* p, const char* name, const spell_data_t* s ):
    hunter_ranged_attack_t( name, p, s )
  {
    dual = true;
    aoe = 2;
    radius = 5.0;

    energize_type = ENERGIZE_PER_HIT;
    energize_resource = RESOURCE_FOCUS;
    energize_amount = p -> find_spell( 204304 ) -> effectN( 1 ).resource( RESOURCE_FOCUS );
  }
};

struct chimaera_shot_t: public hunter_ranged_attack_t
{
  chimaera_shot_impact_t* frost;
  chimaera_shot_impact_t* nature;

  chimaera_shot_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "chimaera_shot", player, player -> talents.chimaera_shot ),
    frost( nullptr ), nature( nullptr )
  {
    parse_options( options_str );
    callbacks = false;
    frost = new chimaera_shot_impact_t( player, "chimaera_shot_frost", player -> find_spell( 171454 ) );
    add_child( frost );
    nature = new chimaera_shot_impact_t( player, "chimaera_shot_nature", player -> find_spell( 171457 ) );
    add_child( nature );
    school = SCHOOL_FROSTSTRIKE; // Just so the report shows a mixture of the two colors.
    starved_proc = player -> get_proc( "starved: chimaera_shot" );
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( rng().roll( 0.5 ) ) // Chimaera shot has a 50/50 chance to roll frost or nature damage... for the flavorz.
      frost -> execute();
    else
      nature -> execute();
  }
};

// Cobra Shot Attack =================================================================

struct cobra_shot_t: public hunter_ranged_attack_t
{

  cobra_shot_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "cobra_shot", player, player -> find_specialization_spell( "Cobra Shot" ) )
  {
    parse_options( options_str );
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();

    // Cobra Shot has a chance to reset Kill Command when Bestial Wrath is up w/ Killer Cobra talent
    if ( p() -> talents.killer_cobra -> ok() && p() -> buffs.bestial_wrath -> up() && rng().roll( p() -> talents.killer_cobra -> effectN( 1 ).percent() ) )
      p() -> cooldowns.kill_command -> reset( true );
  }

  virtual double composite_target_crit( player_t* t ) const override
  {
    double cc = hunter_ranged_attack_t::composite_target_crit( t );

    cc += p() -> buffs.big_game_hunter -> value();

    return cc;
  }
};

//==============================
// Marksmanship attacks 
//==============================

// Black Arrow ==============================================================

struct black_arrow_t: public hunter_ranged_attack_t
{
  black_arrow_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "black_arrow", player, player -> find_class_spell( "Black Arrow" ) )
  {
    parse_options( options_str );
    proc = true;
    tick_may_crit = true;
    hasted_ticks = false;
    // TODO not correctly specified
  }
};

// Trick Shot =========================================================================

struct trick_shot_t: public hunter_ranged_attack_t
{
  trick_shot_t( hunter_t* p ):
    hunter_ranged_attack_t( "trick_shot", p, p -> find_talent_spell( "Trick Shot" ) )
  {
    // Simulated as aoe for simplicity
    aoe               = -1;
    background        = true;
    weapon            = &p -> main_hand_weapon;
    weapon_multiplier = p -> find_specialization_spell( "Aimed Shot" ) -> effectN( 2 ).percent();
    base_multiplier   = p -> find_talent_spell( "Trick Shot" ) -> effectN( 1 ).percent();
  }

  virtual void impact( action_state_t* s ) override
  {
    // Do not hit current target, and only deal damage to targets with Vulnerable
    if ( s -> target != p() -> target && td( s -> target ) -> debuffs.vulnerable -> up() )
      hunter_ranged_attack_t::impact( s );
  }

  // FIXME 21134 - this talent is bugged on alpha (does no damage), so not sure if mastery should affect this or not
  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    if ( p() -> mastery.sniper_training -> ok() )
      am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();

    return am;
  }
};

// Aimed Shot ========================================================================

// (WIP) This is the ability that is proc'd by the MM artifact. 
// Affected by:
//   Deadeye
//   True Aim (procs True Aim, as well)
//   Deadly Aim
//   Mastery
// Not affected by:
//   Vulnerable
//   Careful Aim
struct aimed_shot_artifact_proc_t: hunter_ranged_attack_t
{
  aimed_shot_artifact_proc_t( hunter_t* p, const std::string& name ):
    hunter_ranged_attack_t( name, p, p -> find_spell( 191043 ) )
  {
    background = true;
    proc = true;

    // No need to check for Artifact since this only triggers with Artifact equipped
    // Deadly Aim
    crit_bonus_multiplier *= 1.0 + p -> artifacts.deadly_aim.percent();
  }

  virtual void execute()
  {
    p() -> procs.aimed_shot_artifact -> occur();

    // One proc triggers 6 projectiles
    for ( int i = 0; i < p() -> thasdorah -> driver() -> effectN( 1 ).base_value(); i++ )
      hunter_ranged_attack_t::execute();
  }

  virtual void impact( action_state_t* s )
  {
    hunter_ranged_attack_t::impact( s );

    trigger_true_aim( p(), s -> target );
  }

  virtual double composite_target_crit( player_t* t ) const override
  {
    double cc = hunter_ranged_attack_t::composite_target_crit( t );

    cc += td( t ) -> debuffs.marked_for_death -> check_stack_value();

    return cc;
  }

  virtual double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_ranged_attack_t::composite_target_da_multiplier( t );

    hunter_td_t* td = this -> td( t );

    /* Bug? Does not benefit from Vulnerable on Alpha, but benefits from Deadeye
    if ( td -> debuffs.vulnerable -> up() )
      m *= 1.0 + td -> debuffs.vulnerable -> check_stack_value(); */

    if ( td -> debuffs.deadeye -> up() )
      m *= 1.0 + td -> debuffs.deadeye -> check_stack_value();

    if ( td -> debuffs.true_aim -> up() )
      m *= 1.0 + td -> debuffs.true_aim -> check_stack_value();

    return m;
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    if ( p() -> mastery.sniper_training -> ok() )
      am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();

    return am;
  }

  virtual double composite_crit_multiplier() const override
  {
    double cm = hunter_ranged_attack_t::composite_crit_multiplier();

    if ( p() -> thasdorah )
      cm *= 1.0 + p() -> find_artifact_spell( "Deadly Aim" ).rank() * 0.03;

    return cm;
  }
};

struct aimed_shot_t: public hunter_ranged_attack_t
{
  benefit_t* aimed_in_ca;

  aimed_shot_artifact_proc_t* aimed_shot_artifact_proc;

  aimed_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "aimed_shot", p, p -> find_specialization_spell( "Aimed Shot" ) ),
    aimed_in_ca( p -> get_benefit( "aimed_in_careful_aim" ) ),
    aimed_shot_artifact_proc( nullptr )
  {
    parse_options( options_str );

    if( p -> buffs.lock_and_load -> up() )
      base_execute_time *= 0.0;
    else
      base_execute_time *= 1.0 - ( p -> sets.set( HUNTER_MARKSMANSHIP, T18, B4 ) -> effectN( 2 ).percent() );

    if ( p -> talents.trick_shot -> ok() )
      impact_action = new trick_shot_t( p );
    
    if ( p -> thasdorah )
    {
      // Deadly Aim
      crit_bonus_multiplier *= 1.0 + p -> artifacts.deadly_aim.percent();

      // Precision
      base_costs[ RESOURCE_FOCUS ] += p -> artifacts.precision.value();

      // Weapon passive
      aimed_shot_artifact_proc = new aimed_shot_artifact_proc_t( p, "artifact" );
      add_child( aimed_shot_artifact_proc );
    }
  }

  virtual double composite_target_crit( player_t* t ) const override
  {
    double cc = hunter_ranged_attack_t::composite_target_crit( t );

    cc += p() -> buffs.careful_aim -> value();
    cc += td( t ) -> debuffs.marked_for_death -> check_stack_value();

    return cc;
  }

  virtual double cost() const override
  {
    double cost = hunter_ranged_attack_t::cost();

    if( p() -> buffs.lock_and_load -> check() )
      return 0;

    return cost;
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    trigger_true_aim( p(), s -> target );
    if( p() -> buffs.careful_aim -> value() && s -> result == RESULT_CRIT )
      trigger_piercing_shots( s );
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();
    aimed_in_ca -> update( p() -> buffs.careful_aim -> check() != 0 );
    if ( p() -> sets.has_set_bonus( HUNTER_MARKSMANSHIP, PVP, B4 ) )
    {
      p() -> cooldowns.trueshot -> adjust( -p() -> sets.set( HUNTER_MARKSMANSHIP, PVP, B4 ) -> effectN( 1 ).time_value() );
    }

    if ( p() -> buffs.lock_and_load -> up() )
      p() -> buffs.lock_and_load -> decrement();

    // Thas'dorah's proc has a 1/6 chance to fire 6 mini Aimed Shots. 
    // Proc chance missing from spell data.
    if ( aimed_shot_artifact_proc && rng().roll( 0.167 ) )
      aimed_shot_artifact_proc -> execute();
  }

  virtual double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_ranged_attack_t::composite_target_da_multiplier( t );

    hunter_td_t* td = this -> td( t );
    if ( td -> debuffs.vulnerable -> up() )
      m *= 1.0 + td -> debuffs.vulnerable -> check_stack_value();
    else if ( td -> debuffs.deadeye -> up() )
      m *= 1.0 + td -> debuffs.deadeye -> check_stack_value();

    if ( td -> debuffs.true_aim -> up() )
      m *= 1.0 + td -> debuffs.true_aim -> check_stack_value();

    return m;
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    if ( p() -> mastery.sniper_training -> ok() )
      am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();

    return am;
  }
};

// Arcane Shot Attack ================================================================

struct arcane_shot_t: public hunter_ranged_attack_t
{
  double focus_gain;
  arcane_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "arcane_shot", p, p -> find_specialization_spell( "Arcane Shot" ) )
  {
    parse_options( options_str );

    focus_gain = p -> find_spell( 187675 ) -> effectN( 1 ).base_value();
  }

  virtual void try_steady_focus() override
  {
    trigger_steady_focus( true );
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> buffs.trueshot -> up() || p() -> ppm_hunters_mark -> trigger() )
      {
        td( execute_state -> target ) -> debuffs.hunters_mark -> trigger();
        p() -> procs.hunters_mark -> occur();
        p() -> buffs.hunters_mark_exists -> trigger();
      }
      
      double focus_multiplier = 1.0;
      if ( p() -> thasdorah && execute_state -> result == RESULT_CRIT )
        focus_multiplier *= 1.0 + p() -> artifacts.critical_focus.percent();
      p() -> resource_gain( RESOURCE_FOCUS, focus_multiplier * focus_gain, gain );
    }
  }


  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    trigger_true_aim( p(), s -> target );
  }

  virtual double composite_target_crit( player_t* t ) const override
  {
    double cc = hunter_ranged_attack_t::composite_target_crit( t );

    cc += p() -> buffs.careful_aim -> value();

    return cc;
  }

  virtual double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_ranged_attack_t::composite_target_da_multiplier( t );

    hunter_td_t* td = this -> td( t );

    if ( td -> debuffs.true_aim -> up() )
      m *= 1.0 + td -> debuffs.true_aim -> check_stack_value();

    return m;
  }

  virtual bool ready() override
  {
    if( p() -> talents.sidewinders->ok() )
      return false;

    return hunter_ranged_attack_t::ready();
  }
};

// Marked Shot Attack =================================================================

struct marked_shot_t: public hunter_ranged_attack_t
{
  marked_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "marked_shot", p, p -> find_specialization_spell( "Marked Shot" ) )
  {
    parse_options( options_str );

    // Simulated as AOE for simplicity.
    aoe               = -1;
    weapon            = &p -> main_hand_weapon;
    weapon_multiplier = p -> find_spell( 212621 ) -> effectN( 2 ).percent();

    if ( p -> thasdorah )
      base_multiplier *= 1.0 + p -> artifacts.windrunners_guidance.percent();
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();

    // Consume Hunter's Mark and apply appropriate debuffs. Vulnerable and Deadeye apply on cast.
    std::vector<player_t*> marked_shot_targets = execute_state -> action -> target_list();
    for ( size_t i = 0; i < marked_shot_targets.size(); i++ )
    {
      if ( td( marked_shot_targets[i] ) -> debuffs.hunters_mark -> up() )
      {
        if( p() -> talents.patient_sniper -> ok() )
          td( marked_shot_targets[i] ) -> debuffs.deadeye -> trigger();
        else
          td( marked_shot_targets[i] ) -> debuffs.vulnerable -> trigger();
      }
    }
    p() -> buffs.hunters_mark_exists -> expire();
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_td_t* td = this -> td( s -> target );

    // Only register hits against targets with Hunter's Mark.
    if ( td -> debuffs.hunters_mark -> up() )
    {
      hunter_ranged_attack_t::impact( s );

      td -> debuffs.hunters_mark -> expire();

      // Marked for Death is applied on impact, unlike Vulnerable and Deadeye.
      if ( p() -> thasdorah && p() -> artifacts.marked_for_death.rank() )
        td -> debuffs.marked_for_death -> trigger();

      trigger_true_aim( p(), s -> target );
    }
  }

  // Only schedule the attack if a valid target
  virtual void schedule_travel( action_state_t* s ) override
  {
    if ( td( s -> target ) -> debuffs.hunters_mark -> up() ) 
      hunter_ranged_attack_t::schedule_travel( s );
  }

  // Marked Shot can only be used if a Hunter's Mark exists on any target.
  virtual bool ready() override
  {
    if ( p() -> buffs.hunters_mark_exists -> up() )
      return hunter_ranged_attack_t::ready();

    return false;
  }

  virtual double composite_target_crit( player_t* t ) const override
  {
    double cc = hunter_ranged_attack_t::composite_target_crit( t );

    cc += p() -> buffs.careful_aim -> value();

    return cc;
  }

  virtual double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_ranged_attack_t::composite_target_da_multiplier( t );

    hunter_td_t* td = this -> td( t );

    if ( td -> debuffs.true_aim -> up() )
      m *= 1.0 + td -> debuffs.true_aim -> check_stack_value();

    return m;
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    if ( p() -> mastery.sniper_training -> ok() )
      am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();

    return am;
  }
};

// Head Shot  =========================================================================

struct head_shot_t: public hunter_ranged_attack_t
{
  head_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "head_shot", p, p -> find_talent_spell( "Head Shot" ) )
  {
    parse_options( options_str );

    aoe = -1;

    // Spell data is currently bugged on alpha
    base_multiplier = 2.0;
    base_aoe_multiplier = 0.5;
  }
  
  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    if ( p() -> mastery.sniper_training -> ok() )
      am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();

    am *= std::min( 100.0, p() -> resources.current[RESOURCE_FOCUS] ) / 100;

    return am;
  }
};

// Explosive Shot  ====================================================================

struct explosive_shot_activate_t: public hunter_ranged_attack_t
{
  player_t* initial_target;
  explosive_shot_activate_t( hunter_t* p, const std::string& name ):
    hunter_ranged_attack_t( name, p, p -> find_spell( 212680 ) )
  {
    aoe = -1;
    dual = true;
  }

  virtual void execute() override
  {
    initial_target = p() -> target;

    hunter_ranged_attack_t::execute();
  }

  virtual double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_ranged_attack_t::composite_target_da_multiplier( t );

    if( sim -> distance_targeting_enabled )
      m /= 1 + ( initial_target -> get_position_distance( t -> x_position, t -> y_position ) ) / radius;

    return m;
  }
};

struct explosive_shot_t: public hunter_ranged_attack_t
{
  explosive_shot_activate_t* explosive_activate;
  explosive_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "explosive_shot", p, p -> find_talent_spell( "Explosive Shot" ) )
  {
    parse_options( options_str );
    explosive_activate = new explosive_shot_activate_t( p, "explosion" );
    add_child( explosive_activate );
    cooldown -> duration = p -> find_talent_spell( "Explosive Shot" ) -> cooldown();

    // FIXME - get a more accurate number
    travel_speed = 8.0;
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    explosive_activate -> execute();
  }
};

// Sidewinders =======================================================================

struct sidewinders_t: hunter_ranged_attack_t
{
  sidewinders_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "sidewinders", p, p -> find_talent_spell( "Sidewinders" ) )
  {
    parse_options( options_str );

    aoe                       = -1;
    cooldown -> hasted        = true;
    weapon                    = &p -> main_hand_weapon;
    attack_power_mod.direct   = p -> find_spell( 214581 ) -> effectN( 1 ).ap_coeff();
    weapon_multiplier         = 0;
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      // Hunter's Mark applies on cast to all affected targets or none based on RPPM (8*haste).
      // This loop goes through the target list for multi-shot and applies the debuffs on proc.
      if ( p() -> buffs.trueshot -> up() || p() -> ppm_hunters_mark -> trigger() )
      {
        std::vector<player_t*> sidewinder_targets = execute_state -> action -> target_list();
        for ( size_t i = 0; i < sidewinder_targets.size(); i++ )
          td( sidewinder_targets[i] ) -> debuffs.hunters_mark -> trigger();

        p() -> procs.hunters_mark -> occur();
        p() -> buffs.hunters_mark_exists -> trigger();
      }

      trigger_steady_focus( false );
    }
  }
};

// WindBurst =========================================================================

struct windburst_t: hunter_ranged_attack_t
{
  windburst_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "windburst", p, &p -> artifacts.windburst.data() )
  {
    parse_options( options_str );
  }
};

// Piercing Shots ====================================================================

typedef residual_action::residual_periodic_action_t< hunter_ranged_attack_t > residual_action_t;

struct piercing_shots_t: public residual_action_t
{
  piercing_shots_t( hunter_t* p, const char* name, const spell_data_t* s ):
    residual_action_t( name, p, s)
  {
  }
};

//==============================
// Survival attacks 
//==============================

// Freezing Trap =====================================================================
// Implemented here because often there are buffs associated with it

struct freezing_trap_t: public hunter_ranged_attack_t
{
  freezing_trap_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "freezing_trap", player, player -> find_class_spell( "Freezing Trap" ) )
  {
    parse_options( options_str );

    cooldown -> duration = data().cooldown();
    // BUG simulate slow velocity of launch
    travel_speed = 18.0;

    if ( player -> sets.has_set_bonus( p() -> specialization(), PVP, B2 ) )
    {
      energize_type = ENERGIZE_ON_HIT;
      energize_resource = RESOURCE_FOCUS;
      energize_amount = player -> sets.set( player -> specialization(), PVP, B2 ) ->
        effectN( 1 ).trigger() -> effectN( 1 ).base_value();
    }
  }
};

// Explosive Trap ====================================================================

struct explosive_trap_tick_t: public hunter_ranged_attack_t
{
  explosive_trap_tick_t( hunter_t* player ):
    hunter_ranged_attack_t( "explosive_trap_tick", player, player -> find_spell( 13812 ) )
  {
    aoe = -1;
    hasted_ticks = false;
    dot_duration = data().duration();
    base_tick_time = data().effectN( 2 ).period();
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
    // BUG in game it uses the direct damage AP mltiplier for ticks as well.
    attack_power_mod.tick = attack_power_mod.direct;
    ignore_false_positive = ground_aoe = background = true;
  }
};

struct explosive_trap_t: public hunter_ranged_attack_t
{
  explosive_trap_tick_t* tick;
  explosive_trap_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "explosive_trap", player, player -> find_class_spell( "Explosive Trap" ) ),
    tick( new explosive_trap_tick_t( player ) )
  {
    parse_options( options_str );

    cooldown -> duration = data().cooldown();
    range = 40.0;
    // BUG simulate slow velocity of launch
    travel_speed = 18.0;
    add_child( tick );
    impact_action = tick;
 }
};

// Serpent Sting Attack =====================================================

struct serpent_sting_t: public hunter_ranged_attack_t
{
  serpent_sting_t( hunter_t* player ):
    hunter_ranged_attack_t( "serpent_sting", player, player -> find_spell( 118253 ) )
  {
    background = true;
    proc = true;
    tick_may_crit = true;
    hasted_ticks = false;
  }
};

// Raptor Strike Attack ==============================================================

struct raptor_strike_t: public hunter_melee_attack_t
{
  raptor_strike_t( hunter_t* player, const std::string& options_str ):
    hunter_melee_attack_t( "raptor_strike", player, player -> find_spell( "Raptor Strike" ) )
  {
    parse_options( options_str );
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talents.serpent_sting -> ok() )
      {
        p() -> active.serpent_sting -> target = s -> target;
        p() -> active.serpent_sting -> execute();
      }
    }
  }
};

} // end attacks

// ==========================================================================
// Hunter Spells
// ==========================================================================

namespace spells
{
struct hunter_spell_t: public hunter_action_t < spell_t >
{
public:
  hunter_spell_t( const std::string& n, hunter_t* player,
                  const spell_data_t* s = spell_data_t::nil() ):
                  base_t( n, player, s )
  {
  }

  virtual timespan_t gcd() const override
  {
    if ( !harmful && !player -> in_combat )
      return timespan_t::zero();

    // Hunter gcd unaffected by haste
    return trigger_gcd;
  }

  virtual void execute() override
  {
    hunter_action_t<spell_t>::execute();
    
    this -> try_steady_focus();
  }
};

//==============================
// Shared spells
//==============================

// Barrage ==================================================================
// This is a spell because that's the only way to support "channeled" effects

struct barrage_t: public hunter_spell_t
{
  struct barrage_damage_t: public attacks::hunter_ranged_attack_t
  {
    barrage_damage_t( hunter_t* player ):
      attacks::hunter_ranged_attack_t( "barrage_primary", player, player -> talents.barrage -> effectN( 2 ).trigger() )
    {
      background = true;
      may_crit = true;
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      aoe = -1;
      base_aoe_multiplier = 0.5;
      range = radius;
      range = 0;
    }
  };

  barrage_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "barrage", player, player -> talents.barrage )
  {
    parse_options( options_str );

    may_block = false;
    hasted_ticks = false;
    channeled = true;

    tick_zero = true;
    dynamic_tick_action = true;
    travel_speed = 0.0;
    tick_action = new barrage_damage_t( player );

    starved_proc = player -> get_proc( "starved: barrage" );
  }

 void schedule_execute( action_state_t* state = nullptr ) override
  {
    hunter_spell_t::schedule_execute( state );

    // To suppress autoshot, just delay it by the execute time of the barrage
    if ( p() -> main_hand_attack && p() -> main_hand_attack -> execute_event )
    {
      timespan_t time_to_next_hit = p() -> main_hand_attack -> execute_event -> remains();
      time_to_next_hit += dot_duration;
      p() -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
    }
    p() -> no_steady_focus();
  }

 bool usable_moving() const override
  {
    return true;
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_spell_t::action_multiplier();

    if ( p() -> mastery.sniper_training -> ok() )
      am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();

    return am;
  }
};

// A Murder of Crows ========================================================

struct peck_t: public ranged_attack_t
{
  peck_t( hunter_t* player, const std::string& name ):
    ranged_attack_t( name, player, player -> find_spell( 131900 ) )
  {
    dual = true;
    may_crit = true;
    may_parry = false;
    may_block = false;
    may_dodge = false;
    travel_speed = 0.0;
  }

  hunter_t* p() const { return static_cast<hunter_t*>( player ); }

  virtual double action_multiplier() const override
  {
    double am = ranged_attack_t::action_multiplier();
    am *= p() -> beast_multiplier();

    if ( p() -> mastery.sniper_training -> ok() )
      am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();

    return am;
  }
};

// TODO this should reset CD if the target dies
struct moc_t: public ranged_attack_t
{
  peck_t* peck;
  moc_t( hunter_t* player, const std::string& options_str ):
    ranged_attack_t( "a_murder_of_crows", player, player -> talents.a_murder_of_crows ),
    peck( new peck_t( player, "crow_peck" ) )
  {
    parse_options( options_str );
    add_child( peck );
    hasted_ticks = false;
    callbacks = false;
    may_crit = false;
    may_miss = false;
    may_parry = false;
    may_block = false;
    may_dodge = false;

    starved_proc = player -> get_proc( "starved: a_murder_of_crows" );
  }

  hunter_t* p() const { return static_cast<hunter_t*>( player ); }

  void tick( dot_t*d ) override
  {
    ranged_attack_t::tick( d );
    peck -> execute();
  }

  virtual double cost() const override
  {
    double c = ranged_attack_t::cost();
    if ( p() -> buffs.bestial_wrath -> check() )
      c *= 1.0 + p() -> buffs.bestial_wrath -> data().effectN( 6 ).percent();
    return c;
  }

  virtual void execute() override
  {
    p() -> no_steady_focus();
    ranged_attack_t::execute();
  }
};

// Summon Pet ===============================================================

struct summon_pet_t: public hunter_spell_t
{
  std::string pet_name;
  pet_t* pet;
  summon_pet_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "summon_pet", player ),
    pet( nullptr )
  {
    harmful = false;
    callbacks = false;
    ignore_false_positive = true;
    pet_name = options_str.empty() ? p() -> summon_pet_str : options_str;
  }

  bool init_finished() override
  {
    if ( ! pet )
    {
      pet = player -> find_pet( pet_name );
    }

    if ( ! pet && p() -> specialization() != HUNTER_MARKSMANSHIP )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", p() -> name(), pet_name.c_str() );
      sim -> cancel();
    }

    return hunter_spell_t::init_finished();
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();
    pet -> type = PLAYER_PET;
    pet -> summon();

    if ( p() -> titanstrike && p() -> hati )
      p() -> hati -> summon();

    if ( p() -> main_hand_attack ) p() -> main_hand_attack -> cancel();
  }

  virtual bool ready() override
  {
    if ( p() -> active.pet == pet || p() -> talents.lone_wolf -> ok() ) 
      return false;

    return hunter_spell_t::ready();
  }
};

//==============================
// Beast Mastery spells
//==============================

// Dire Beast ===============================================================

struct dire_beast_t: public hunter_spell_t
{
  dire_beast_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "dire_beast", player, player -> specs.dire_beast )
  {
    parse_options( options_str );
    harmful = false;
    hasted_ticks = false;
    may_crit = false;
    may_miss = false;
    school = SCHOOL_PHYSICAL;
  }

  bool init_finished() override
  {
    if ( p() -> pet_dire_beasts[ 0 ] )
    {
      stats -> add_child( p() -> pet_dire_beasts[ 0 ] -> get_stats( "dire_beast_melee" ) );
    }

    return hunter_spell_t::init_finished();
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();
    p() -> no_steady_focus();

    timespan_t t = timespan_t::from_seconds( p() -> specs.dire_beast -> effectN( 1 ).base_value() );
    p() -> cooldowns.bestial_wrath -> adjust( -t );

    pet_t* beast = nullptr;
    for( size_t i = 0; i < p() -> pet_dire_beasts.size(); i++ )
    {
      if ( p() -> pet_dire_beasts[i] -> is_sleeping() )
      {
        beast = p() -> pet_dire_beasts[i];
        break;
      }
    }

    assert( beast );

    // Dire beast gets a chance for an extra attack based on haste
    // rather than discrete plateaus.  At integer numbers of attacks,
    // the beast actually has a 50% chance of n-1 attacks and 50%
    // chance of n.  It (apparently) scales linearly between n-0.5
    // attacks to n+0.5 attacks.  This uses beast duration to
    // effectively alter the number of attacks as the duration itself
    // isn't important and combat log testing shows some variation in
    // attack speeds.  This is not quite perfect but more accurate
    // than plateaus.
    const timespan_t base_duration = timespan_t::from_seconds( 8.0 );
    const timespan_t swing_time = beast -> main_hand_weapon.swing_time * beast -> composite_melee_speed();
    double partial_attacks_per_summon = base_duration / swing_time;
    double base_attacks_per_summon = floor( partial_attacks_per_summon - 0.5 ); // 8.4 -> 7, 8.5 -> 8, 8.6 -> 8, etc

    if ( rng().roll( partial_attacks_per_summon - base_attacks_per_summon - 0.5 ) )
      base_attacks_per_summon += 1;

    timespan_t duration = base_attacks_per_summon * swing_time;
    assert( beast );
    beast -> summon( duration );
  }
};

// Bestial Wrath ============================================================

struct bestial_wrath_t: public hunter_spell_t
{
  bestial_wrath_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "bestial_wrath", player, player -> specs.bestial_wrath )
  {
    parse_options( options_str );
    harmful = false;
  }

  virtual void execute() override
  {
    p() -> buffs.bestial_wrath  -> trigger();
    p() -> active.pet -> buffs.bestial_wrath -> trigger();
    if ( p() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T17, B4 ) )
    {
      const timespan_t duration = p() -> buffs.bestial_wrath -> buff_duration;
      // start from the back so we don't overlap stampede pets in reporting
      for ( size_t i = p() -> hunter_main_pets.size(); i-- > 0; )
      {
        if ( p() -> hunter_main_pets[i] -> tier_pet_summon( duration ) )
          break;
      }
    }
    hunter_spell_t::execute();
  }

  virtual bool ready() override
  {
    if ( !p() -> active.pet )
      return false;

    return hunter_spell_t::ready();
  }
};

// Kill Command =============================================================

struct kill_command_t: public hunter_spell_t
{
  kill_command_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "kill_command", player, player -> specs.kill_command )
  {
    parse_options( options_str );
    harmful = false;
  }

  bool init_finished() override
  {
    for (auto pet : p() -> pet_list)
    {
      
      stats -> add_child( pet -> get_stats( "kill_command" ) );
    }

    return hunter_spell_t::init_finished();
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();

    if ( p() -> active.pet )
    {
      p() -> active.pet -> active.kill_command -> execute();
      trigger_tier17_2pc_bm();
    }
    p() -> no_steady_focus();
  }

  virtual bool ready() override
  {
    if ( p() -> active.pet && p() -> active.pet -> active.kill_command -> ready() ) // Range check from the pet.
      return hunter_spell_t::ready();

    return false;
  }

  bool trigger_tier17_2pc_bm()
  {
    if ( !p() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T17, B2 ) )
      return false;

    bool procced = rng().roll( p() -> sets.set( HUNTER_BEAST_MASTERY, T17, B2 ) -> proc_chance() );
    if ( procced )
    {
      p() -> cooldowns.bestial_wrath -> reset( true );
      p() -> procs.tier17_2pc_bm -> occur();
    }

    return procced;
  }
};

// Aspect of the Wild =======================================================

struct aspect_of_the_wild_t: public hunter_spell_t
{
  aspect_of_the_wild_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "aspect_of_the_wild", p, p -> specs.aspect_of_the_wild )
  {
    parse_options( options_str );
    harmful = false;
  }

  virtual void execute() override
  {
    p() -> buffs.aspect_of_the_wild -> trigger();
    p() -> active.pet -> buffs.aspect_of_the_wild -> trigger();
    hunter_spell_t::execute();
  }

  virtual bool ready() override
  {
    if ( !p() -> active.pet )
      return false;

    return hunter_spell_t::ready();
  }
};

// Stampede =================================================================

struct stampede_t: public hunter_spell_t
{
  stampede_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "stampede", p, p -> talents.stampede )
  {
    parse_options( options_str );
    harmful = false;
    callbacks = false;
    school = SCHOOL_PHYSICAL;
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();
    p() -> buffs.stampede -> trigger();

    for ( unsigned int i = 0; i < p() -> hunter_main_pets.size() && i < 5; ++i )
    {
      p() -> hunter_main_pets[i] -> stampede_summon( p() -> buffs.stampede -> buff_duration + timespan_t::from_millis( 27 ) );
      // Added 0.027 seconds to properly reflect haste threshholds seen in game.
    }
    p() -> no_steady_focus();
  }
};

//==============================
// Marksmanship spells
//==============================

// Trueshot =================================================================

struct trueshot_t: public hunter_spell_t
{
  double value;
  trueshot_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "trueshot", p, p -> specs.trueshot )
  {
    parse_options( options_str );
    value = data().effectN( 1 ).percent();

    if ( p -> thasdorah )
      // Quick Shot: spell data adds -30 seconds to CD
      cooldown -> duration += p -> artifacts.quick_shot.time_value();
  }

  virtual void execute() override
  {
    p() -> buffs.trueshot -> trigger( 1, value );

    if ( p() -> thasdorah && p() -> artifacts.rapid_killing.rank() )
      p() -> buffs.rapid_killing -> trigger();

    hunter_spell_t::execute();
  }
};

//end spells
}

hunter_td_t::hunter_td_t( player_t* target, hunter_t* p ):
actor_target_data_t( target, p ),
dots( dots_t() )
{
  dots.serpent_sting = target -> get_dot( "serpent_sting", p );
  dots.poisoned_ammo = target -> get_dot( "poisoned_ammo", p );
  dots.piercing_shots = target -> get_dot( "piercing_shots", p );

  debuffs.hunters_mark      = buff_creator_t( *this, "hunters_mark" )
                                .spell( p -> find_spell( 185365 ) );
  debuffs.vulnerable        = buff_creator_t( *this, "vulnerable" )
                                .spell( p -> find_spell( 187131 ) )
                                .default_value( p -> find_spell( 187131 ) -> effectN( 2 ).percent() );
  debuffs.marked_for_death  = buff_creator_t( *this, "marked_for_death" )
                                .spell( p -> find_spell( 190533 ) )
                                .default_value( p -> find_artifact_spell( "Marked for Death" ).percent() );
  debuffs.deadeye           = buff_creator_t( *this, "deadeye" )
                                .spell( p -> find_spell( 213424 ) )
                                .default_value( p -> find_spell( 213424 ) -> effectN( 1 ).percent() );
  debuffs.true_aim          = buff_creator_t( *this, "true_aim" )
                                .spell( p -> find_spell( 199803 ) )
                                .default_value( p -> find_spell( 199803 ) -> effectN( 1 ).percent() );
}
 

expr_t* hunter_t::create_expression( action_t* a, const std::string& name )
{
  return player_t::create_expression( a, name );
}


// hunter_t::create_action ==================================================

action_t* hunter_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  using namespace attacks;
  using namespace spells;
  
  if ( name == "a_murder_of_crows"     ) return new                    moc_t( this, options_str );
  if ( name == "aimed_shot"            ) return new             aimed_shot_t( this, options_str );
  if ( name == "arcane_shot"           ) return new            arcane_shot_t( this, options_str );
  if ( name == "aspect_of_the_wild"    ) return new     aspect_of_the_wild_t( this, options_str );
  if ( name == "auto_attack"           ) return new            auto_attack_t( this, options_str );
  if ( name == "auto_shot"             ) return new           start_attack_t( this, options_str );
  if ( name == "barrage"               ) return new                barrage_t( this, options_str );
  if ( name == "bestial_wrath"         ) return new          bestial_wrath_t( this, options_str ); 
  if ( name == "black_arrow"           ) return new            black_arrow_t( this, options_str );
  if ( name == "chimaera_shot"         ) return new          chimaera_shot_t( this, options_str );
  if ( name == "cobra_shot"            ) return new             cobra_shot_t( this, options_str );
  if ( name == "dire_beast"            ) return new             dire_beast_t( this, options_str );
  if ( name == "exotic_munitions"      ) return new       exotic_munitions_t( this, options_str );
  if ( name == "explosive_shot"        ) return new         explosive_shot_t( this, options_str );
  if ( name == "explosive_trap"        ) return new         explosive_trap_t( this, options_str );
  if ( name == "freezing_trap"         ) return new          freezing_trap_t( this, options_str );
  if ( name == "head_shot"             ) return new              head_shot_t( this, options_str );
  if ( name == "kill_command"          ) return new           kill_command_t( this, options_str );
  if ( name == "marked_shot"           ) return new            marked_shot_t( this, options_str );
  if ( name == "multishot"             ) return new             multi_shot_t( this, options_str );
  if ( name == "multi_shot"            ) return new             multi_shot_t( this, options_str );
  if ( name == "sidewinders"           ) return new            sidewinders_t( this, options_str );
  if ( name == "stampede"              ) return new               stampede_t( this, options_str );
  if ( name == "steady_shot"           ) return new          raptor_strike_t( this, options_str );
  if ( name == "summon_pet"            ) return new             summon_pet_t( this, options_str );
  if ( name == "trueshot"              ) return new               trueshot_t( this, options_str );
  if ( name == "windburst"             ) return new              windburst_t( this, options_str );
  return player_t::create_action( name, options_str );
}

// hunter_t::create_pet =====================================================

pet_t* hunter_t::create_pet( const std::string& pet_name,
                             const std::string& pet_type )
{
  using namespace pets;
  // Blademaster pets have to always be explicitly created, cannot re-use the same pet as there are
  // many of them.
  if (pet_name == BLADEMASTER_PET_NAME) return new blademaster_pet_t(this);

  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  pet_e type = util::parse_pet_type( pet_type );
  if ( type > PET_NONE && type < PET_HUNTER )
    return new pets::hunter_main_pet_t( *sim, *this, pet_name, type );
  else if ( pet_type != "" )
  {
    sim -> errorf( "Player %s with pet %s has unknown type %s\n", name(), pet_name.c_str(), pet_type.c_str() );
    sim -> cancel();
  }

  return nullptr;
}

// hunter_t::create_pets ====================================================

void hunter_t::create_pets()
{
  size_t create_pets = 1;
  if ( talents.stampede -> ok() ) // We only need to create 1 main pet if we are not using the stampede talent.
    create_pets += 4;
  for ( size_t i = 0; i < create_pets; i++ )
  {
    if ( !talents.stampede -> ok() )
      create_pet( summon_pet_str, summon_pet_str );
    else
    {
      create_pet( "raptor", "raptor" );
      create_pet( "wolf", "wolf" );
      create_pet( "hyena", "hyena" );
      create_pet( "devilsaur", "devilsaur" );
      create_pet( "cat", "cat" );
      break;
    }
  }

  if ( sets.has_set_bonus( HUNTER_BEAST_MASTERY, T17, B4 ) )
  {
    create_pet( "t17_pet_2", "wolf" );
    create_pet( "t17_pet_1", "wolf" );
  }

  if ( sets.has_set_bonus( HUNTER_BEAST_MASTERY, T18, B4 ) )
    create_pet( "t18_fel_boar", "boar" );

  if ( specs.dire_beast -> ok() )
  {
    for ( size_t i = 0; i < pet_dire_beasts.size(); ++i )
    {
      pet_dire_beasts[i] = new pets::dire_critter_t( *this, i + 1 );
    }
  }

  if ( titanstrike )
    hati = new pets::hati_t( *this );
}

// hunter_t::init_spells ====================================================

void hunter_t::init_spells()
{
  player_t::init_spells();

  //Tier 1
  talents.one_with_the_pack                 = find_talent_spell( "One with the Pack" );
  talents.way_of_the_cobra                  = find_talent_spell( "Way of the Cobra" );
  talents.dire_stable                       = find_talent_spell( "Dire Stable" );

  talents.lone_wolf                         = find_talent_spell( "Lone Wolf" );
  talents.steady_focus                      = find_talent_spell( "Steady Focus" );
  talents.true_aim                          = find_talent_spell( "True Aim" );
  
  talents.animal_instincts                  = find_talent_spell( "Animal Instincts" );
  talents.way_of_moknathal                  = find_talent_spell( "Way of the Mok'Nathal" );
  talents.throwing_axes                     = find_talent_spell( "Throwing Axes" );

  //Tier 2
  talents.posthaste                         = find_talent_spell( "Posthaste" );
  talents.farstrider                        = find_talent_spell( "Farstrider" );
  talents.dash                              = find_talent_spell( "Dash" );

  //Tier 3
  talents.stomp                             = find_talent_spell( "Stomp" );
  talents.exotic_munitions                  = find_talent_spell( "Exotic Munitions" );
  talents.chimaera_shot                     = find_talent_spell( "Chimaera Shot" );

  talents.explosive_shot                    = find_talent_spell( "Explosive Shot" );
  talents.trick_shot                        = find_talent_spell( "Trick Shot" );

  talents.caltrops                          = find_talent_spell( "Caltrops" );
  talents.steel_trap                        = find_talent_spell( "Steel Trap" );
  talents.improved_traps                    = find_talent_spell( "Improved Traps" );

  //Tier 4
  talents.intimidation                      = find_talent_spell( "Intimidation" );
  talents.wyvern_sting                      = find_talent_spell( "Wyvern Sting" );
  talents.binding_shot                      = find_talent_spell( "Binding Shot" );

  //Tier 5
  talents.big_game_hunter                   = find_talent_spell( "Big Game Hunter" );
  talents.bestial_fury                      = find_talent_spell( "Bestial Fury" );
  talents.blink_strikes                     = find_talent_spell( "Blink Strikes" );

  talents.careful_aim                       = find_talent_spell( "Careful Aim" );
  talents.heightened_vulnerability          = find_talent_spell( "Heightened Vulnerability" );
  talents.patient_sniper                    = find_talent_spell( "Patient Sniper" );

  talents.dragonsfire_grenade               = find_talent_spell( "Dragonsfire Grenade" );
  talents.snake_hunter                      = find_talent_spell( "Snake Hunter" );

  //Tier 6
  talents.a_murder_of_crows                 = find_talent_spell( "A Murder of Crows" );
  talents.barrage                           = find_talent_spell( "Barrage" );
  talents.volley                            = find_talent_spell( "Volley" );
  talents.mortal_wounds                     = find_talent_spell( "Mortal Wounds" );
  talents.serpent_sting                     = find_talent_spell( "Serpent Sting" );

  //Tier 7
  talents.stampede                          = find_talent_spell( "Stampede" );
  talents.killer_cobra                      = find_talent_spell( "Killer Cobra" );
  talents.aspect_of_the_beast               = find_talent_spell( "Aspect of the Beast" );

  talents.sidewinders                       = find_talent_spell( "Sidewinders" );
  talents.head_shot                         = find_talent_spell( "Head Shot" );
  talents.lock_and_load                     = find_talent_spell( "Lock and Load" );

  talents.spitting_cobra                    = find_talent_spell( "Spitting Cobra" );
  talents.expert_trapper                    = find_talent_spell( "Expert Trapper" );
  
  // Mastery
  mastery.master_of_beasts     = find_mastery_spell( HUNTER_BEAST_MASTERY );
  mastery.sniper_training      = find_mastery_spell( HUNTER_MARKSMANSHIP );
  mastery.hunting_companion    = find_mastery_spell( HUNTER_SURVIVAL );

  // Glyphs
  glyphs.arachnophobia       = find_glyph_spell( "Glyph of Arachnophobia" );
  glyphs.lesser_proportion   = find_glyph_spell( "Glyph of Lesser Proportion" );
  glyphs.nesignwarys_nemesis = find_glyph_spell( "Glyph of Nesingwary's Nemesis'" );
  glyphs.sparks              = find_glyph_spell( "Glyph of Sparks" );
  glyphs.bullseye            = find_glyph_spell( "Glyph of the Bullseye" );
  glyphs.firecracker         = find_glyph_spell( "Glyph of the Firecracker" );
  glyphs.headhunter          = find_glyph_spell( "Glyph of the Headhunter" );
  glyphs.hook                = find_glyph_spell( "Glyph of the Hook" );
  glyphs.skullseye           = find_glyph_spell( "Glyph of the Skullseye" );
  glyphs.trident             = find_glyph_spell( "Glyph of the Trident" );

  // Spec spells
  specs.beast_cleave         = find_specialization_spell( "Beast Cleave" );
  specs.exotic_beasts        = find_specialization_spell( "Exotic Beasts" );
  specs.kindred_spirits      = find_specialization_spell( "Kindred Spirits" );
  specs.lock_and_load        = find_specialization_spell( "Lock and Load" );
  specs.bombardment          = find_specialization_spell( "Bombardment" );
  specs.aimed_shot           = find_specialization_spell( "Aimed Shot" );
  specs.bestial_wrath        = find_specialization_spell( "Bestial Wrath" );
  specs.kill_command         = find_specialization_spell( "Kill Command" );
  specs.trueshot             = find_specialization_spell( "Trueshot" );
  specs.survivalist          = find_specialization_spell( "Survivalist" );
  specs.dire_beast           = find_specialization_spell( "Dire Beast" );
  specs.wild_call            = find_specialization_spell( "Wild Call" );
  specs.aspect_of_the_wild   = find_specialization_spell( "Aspect of the Wild" );

  // Artifact spells
  artifacts.titans_thunder           = find_artifact_spell( "Titan's Thunder" );
  artifacts.master_of_beasts         = find_artifact_spell( "Master of Beasts" );
  artifacts.stormshot                = find_artifact_spell( "Stormshot" );
  artifacts.surge_of_the_stormgod    = find_artifact_spell( "Surge of the Stormgod" );
  artifacts.spitting_cobras          = find_artifact_spell( "Spitting Cobras" );
  artifacts.jaws_of_thunder          = find_artifact_spell( "Jaws of Thunder" );
  artifacts.wilderness_expert        = find_artifact_spell( "Wilderness Expert" );
  artifacts.pack_leader              = find_artifact_spell( "Pack Leader" );
  artifacts.unleash_the_beast        = find_artifact_spell( "Unleash the Beast" );
  artifacts.focus_of_the_titans      = find_artifact_spell( "Focus of the Titans" );
  artifacts.furious_swipes           = find_artifact_spell( "Furious Swipes" );

  artifacts.windburst                = find_artifact_spell( "Windburst" );
  artifacts.whispers_of_the_past     = find_artifact_spell( "Whispers of the Past" );
  artifacts.call_of_the_hunter       = find_artifact_spell( "Call of the Hunter" );
  artifacts.bullseye                 = find_artifact_spell( "Bullseye" );
  artifacts.deadly_aim               = find_artifact_spell( "Deadly Aim" );
  artifacts.quick_shot               = find_artifact_spell( "Quick Shot" );
  artifacts.critical_focus           = find_artifact_spell( "Critical Focus" );
  artifacts.windrunners_guidance     = find_artifact_spell( "Windrunner's Guidance" );
  artifacts.call_the_targets         = find_artifact_spell( "Call the Targets" );
  artifacts.marked_for_death         = find_artifact_spell( "Marked for Death" );
  artifacts.precision                = find_artifact_spell( "Precision" );
  artifacts.rapid_killing            = find_artifact_spell( "Rapid Killing" );

  artifacts.fury_of_the_eagle        = find_artifact_spell( "Fury of the Eagle" );
  artifacts.talon_strike             = find_artifact_spell( "Talon Strike" );
  artifacts.eagles_bite              = find_artifact_spell( "Eagle's Bite" );
  artifacts.aspect_of_the_skylord    = find_artifact_spell( "Aspect of the Skylord" );
  artifacts.sharpened_beak           = find_artifact_spell( "Sharpened Beak" );
  artifacts.raptors_cry              = find_artifact_spell( "Raptor's Cry" );
  artifacts.hellcarver               = find_artifact_spell( "Hellcarver" );
  artifacts.my_beloved_monster       = find_artifact_spell( "My Beloved Monster" );
  artifacts.strength_of_the_mountain = find_artifact_spell( "Strength of the Mountain" );
  artifacts.fluffy_go                = find_artifact_spell( "Fluffy, Go" );
  artifacts.jagged_claws             = find_artifact_spell( "Jagged Claws" );
  artifacts.hunters_guile            = find_artifact_spell( "Hunter's Guile" );
  
  if ( talents.serpent_sting -> ok() )
    active.serpent_sting = new attacks::serpent_sting_t( this );

  if ( talents.careful_aim -> ok() )
    active.piercing_shots = new attacks::piercing_shots_t( this, "piercing_shots", find_spell( 63468 ) );

  if ( talents.exotic_munitions -> ok() )
  {
    active.incendiary_ammo = new attacks::exotic_munitions_incendiary_ammo_t( this, "incendiary_ammo", find_spell( 162541 ) );
    active.frozen_ammo = new attacks::exotic_munitions_frozen_ammo_t( this, "frozen_ammo", find_spell( 162546 ) );
    active.poisoned_ammo = new attacks::exotic_munitions_poisoned_ammo_t( this, "poisoned_ammo", find_spell( 170661 ) );
  }

  action_lightning_arrow_aimed_shot = new attacks::lightning_arrow_t( this, "_aimed_shot" );
  action_lightning_arrow_arcane_shot = new attacks::lightning_arrow_t( this, "_arcane_shot" );
  action_lightning_arrow_multi_shot = new attacks::lightning_arrow_t( this, "_multi_shot" );
}

// hunter_t::init_base ======================================================

void hunter_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;

  base_focus_regen_per_second = 10.0;

  resources.base[RESOURCE_FOCUS] = 100 + specs.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS ) + talents.patient_sniper -> effectN( 1 ).resource( RESOURCE_FOCUS );

  // Orc racial
  if ( race == RACE_ORC )
    pet_multiplier *= 1.0 + find_racial_spell( "Command" ) -> effectN( 1 ).percent();

  stats_stampede = get_stats( "stampede" );
  stats_tier17_4pc_bm = get_stats( "tier17_4pc_bm" );
  stats_tier18_4pc_bm = get_stats( "tier18_4pc_bm" );
}

// hunter_t::init_buffs =====================================================

void hunter_t::create_buffs()
{
  player_t::create_buffs();

  buffs.aspect_of_the_wild           = buff_creator_t( this, 193530, "aspect_of_the_wild" )
    .affects_regen( true )
    .add_invalidate( CACHE_CRIT );

  buffs.bestial_wrath                = buff_creator_t( this, "bestial_wrath", specs.bestial_wrath )
    .cd( timespan_t::zero() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    buffs.bestial_wrath -> default_value = buffs.bestial_wrath -> data().effectN( 1 ).percent();

  if ( talents.bestial_fury -> ok() )
    buffs.bestial_wrath -> default_value += talents.bestial_fury -> effectN( 1 ).percent();

  buffs.bombardment                 = buff_creator_t( this, "bombardment", specs.bombardment -> effectN( 1 ).trigger() );

  double careful_aim_crit           = talents.careful_aim -> effectN( 1 ).percent( );
  buffs.careful_aim                 = buff_creator_t( this, "careful_aim", talents.careful_aim ).activated( true ).default_value( careful_aim_crit );

  double big_game_hunter_crit       = talents.big_game_hunter -> effectN( 1 ).percent( );
  buffs.big_game_hunter             = buff_creator_t( this, "big_game_hunter", talents.big_game_hunter ).activated( true ).default_value( big_game_hunter_crit );

  buffs.steady_focus                = buff_creator_t( this, 193534, "steady_focus" ).chance( talents.steady_focus -> ok() );
  buffs.pre_steady_focus            = buff_creator_t( this, "pre_steady_focus" ).max_stack( 2 ).quiet( true );

  buffs.hunters_mark_exists         = buff_creator_t( this, 185365, "hunters_mark_exists" );

  buffs.lock_and_load               = buff_creator_t( this, 194594, "lock_and_load" ).max_stack( 2 );

  buffs.trueshot                    = buff_creator_t( this, "trueshot", specs.trueshot )
    .add_invalidate( CACHE_ATTACK_HASTE );

  buffs.trueshot -> cooldown -> duration = timespan_t::zero();

  buffs.rapid_killing               = buff_creator_t( this, 191342, "rapid_killing" )
    .default_value( find_spell( 191342 ) -> effectN( 1 ).percent() );

  buffs.bullseye                    = buff_creator_t( this, 204090, "bullseye" )
    .default_value( find_spell( 204090 ) -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_CRIT ); 

  buffs.stampede = buff_creator_t( this, 130201, "stampede" ) // To allow action lists to react to stampede, rather than doing it in a roundabout way.
    .activated( true )
    .duration( timespan_t::from_millis( 40027 ) );
  // Added 0.027 seconds to properly reflect haste threshholds seen in game.
  /*.quiet( true )*/;

  buffs.heavy_shot   = buff_creator_t( this, 167165, "heavy_shot" )
    .default_value( find_spell( 167165 ) -> effectN( 1 ).percent() )
    .refresh_behavior( BUFF_REFRESH_EXTEND );
}

// hunter_t::init_gains =====================================================

void hunter_t::init_gains()
{
  player_t::init_gains();

  gains.arcane_shot          = get_gain( "arcane_shot" );
  gains.steady_focus         = get_gain( "steady_focus" );
  gains.cobra_shot           = get_gain( "cobra_shot" );
  gains.aimed_shot           = get_gain( "aimed_shot" );
  gains.dire_beast           = get_gain( "dire_beast" );
  gains.multi_shot           = get_gain( "multi_shot" );
  gains.chimaera_shot        = get_gain( "chimaera_shot" );
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

  procs.aimed_shot_artifact          = get_proc( "aimed_shot_artifact" );
  procs.lock_and_load                = get_proc( "lock_and_load" );
  procs.hunters_mark                 = get_proc( "hunters_mark" );
  procs.wild_call                    = get_proc( "wild_call" );
  procs.tier17_2pc_bm                = get_proc( "tier17_2pc_bm" );
  procs.black_arrow_trinket_reset    = get_proc( "black_arrow_trinket_reset" );
  procs.tier18_2pc_mm_wasted_proc    = get_proc( "tier18_2pc_mm_wasted_proc" );
  procs.tier18_2pc_mm_wasted_overwrite     = get_proc ("tier18_2pc_mm_wasted_overwrite");
}

// hunter_t::init_rng =======================================================

void hunter_t::init_rng()
{
  // RPPMS
  ppm_hunters_mark = get_rppm( "hunters_mark", find_specialization_spell( "Hunter's Mark" ) );

  player_t::init_rng();
}

// hunter_t::init_scaling ===================================================

void hunter_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[STAT_STRENGTH] = false;
}

// hunter_t::init_actions ===================================================

void hunter_t::init_action_list()
{
  if ( specialization() != HUNTER_SURVIVAL && main_hand_weapon.group() != WEAPON_RANGED )
  {
    sim -> errorf( "Player %s does not have a ranged weapon at the Main Hand slot.", name() );
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    action_priority_list_t* precombat = get_action_priority_list( "precombat" );

    // Flask
    if ( sim -> allow_flasks && true_level >= 80 )
    {
      std::string flask_action = "flask,type=";
      if ( true_level > 90 )
        flask_action += "greater_draenic_agility_flask";
      else
        flask_action += "spring_blossoms";
      precombat -> add_action( flask_action );
    }

    // Food
    if ( sim -> allow_food )
    {
      std::string food_action = "food,type=";
      if ( level() <= 90 )
        food_action += ( level() > 85 ) ? "sea_mist_rice_noodles" : "seafood_magnifique_feast";
      else if ( specialization() == HUNTER_BEAST_MASTERY )
        food_action += "sleeper_sushi";
      else if ( specialization() == HUNTER_MARKSMANSHIP )
        food_action += "pickled_eel";
      else if ( specialization() == HUNTER_SURVIVAL )
        food_action += "salty_squid_roll";
      else
        food_action += "salty_squid_roll";
      precombat -> add_action( food_action );
    }

    precombat -> add_action( "summon_pet" );
    precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
    precombat -> add_action( "exotic_munitions,ammo_type=poisoned,if=spell_targets.multi_shot<3" );
    precombat -> add_action( "exotic_munitions,ammo_type=incendiary,if=spell_targets.multi_shot>=3" );

    //Pre-pot
    add_potion_action( precombat, "draenic_agility", "virmens_bite" );

    switch ( specialization() )
    {
    case HUNTER_SURVIVAL:
      talent_overrides_str += "exotic_munitions,if=raid_event.adds.count>=3|enemies>2";
      apl_surv();
      break;
    case HUNTER_BEAST_MASTERY:
      talent_overrides_str += "steady_focus,if=raid_event.adds.count>=1|enemies>1";
      apl_bm();
      break;
    case HUNTER_MARKSMANSHIP:
      //talent_overrides_str += "lone_wolf,if=raid_event.adds.count>=3|enemies>1";
      apl_mm();
      break;
    default:
      apl_default(); // DEFAULT
      break;
    }

    if ( summon_pet_str.empty() )
      summon_pet_str = "cat";
    // Default
    use_default_action_list = true;
    player_t::init_action_list();
  }
}


// Item Actions =======================================================================

void hunter_t::add_item_actions( action_priority_list_t* list )
{
  int num_items = (int)items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
      list -> add_action( "use_item,name=" + items[i].name_str );
  }
}

// Racial Actions =======================================================================

void hunter_t::add_racial_actions( action_priority_list_t* list )
{
    list -> add_action( "arcane_torrent,if=focus.deficit>=30");
    list -> add_action( "blood_fury" );
    list -> add_action( "berserking" );
}

// Potions Actions =======================================================================

void hunter_t::add_potion_action( action_priority_list_t* list, const std::string big_potion, const std::string little_potion, const std::string options )
{
  std::string action_options = options.empty() ? options : "," + options;
  if ( sim -> allow_potions )
  {
    if ( true_level > 90 )
      list -> add_action( "potion,name=" + big_potion + action_options );
    else if ( true_level >= 85 )
      list -> add_action( "potion,name=" + little_potion + action_options );
  }
}

// Beastmastery Action List =============================================================

void hunter_t::apl_bm()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );

  default_list -> add_action( "auto_shot" );

  add_item_actions( default_list );
  add_racial_actions( default_list );
  add_potion_action( default_list, "draenic_agility", "virmens_bite",
   "if=!talent.stampede.enabled&((buff.bestial_wrath.up&(legendary_ring.up|!legendary_ring.has_cooldown)&target.health.pct<=20)|target.time_to_die<=20)" );

  default_list -> add_talent( this, "Dire Beast" );
  default_list -> add_action( this, "Bestial Wrath", "if=focus>30&!buff.bestial_wrath.up" );
  default_list -> add_action( this, "Multi-Shot", "if=spell_targets.multi_shot>1&pet.cat.buff.beast_cleave.remains<0.5" );
  default_list -> add_action( this, "Focus Fire", "min_frenzy=5" );
  default_list -> add_talent( this, "Barrage", "if=spell_targets.barrage>1" );
  default_list -> add_action( this, "Explosive Trap", "if=spell_targets.explosive_trap_tick>5" );
  default_list -> add_action( this, "Multi-Shot", "if=spell_targets.multi_shot>5" );
  default_list -> add_action( this, "Kill Command" );
  default_list -> add_talent( this, "A Murder of Crows" );
  default_list -> add_action( this, "Kill Shot","if=focus.time_to_max>gcd" );
  default_list -> add_talent( this, "Focusing Shot", "if=focus<50" );
  default_list -> add_action( this, "Cobra Shot", "if=buff.pre_steady_focus.up&buff.steady_focus.remains<7&(14+cast_regen)<focus.deficit", "Cast a second shot for steady focus if that won't cap us." );
  default_list -> add_action( this, "Explosive Trap", "if=spell_targets.explosive_trap_tick>1" );
  default_list -> add_action( this, "Cobra Shot", "if=talent.steady_focus.enabled&buff.steady_focus.remains<4&focus<50", "Prepare for steady focus refresh if it is running out." );
  default_list -> add_talent( this, "Barrage" );
  default_list -> add_talent( this, "Powershot", "if=focus.time_to_max>cast_time" );
  default_list -> add_action( this, "Cobra Shot", "if=spell_targets.multi_shot>5" );
  default_list -> add_action( this, "Arcane Shot", "if=buff.bestial_wrath.up" );
  default_list -> add_action( this, "Arcane Shot", "if=focus>=75" );
  if ( true_level >= 81 )
    default_list -> add_action( this, "Cobra Shot");
  else
    default_list -> add_action(this, "Steady Shot");
}

// Marksman Action List ======================================================================

void hunter_t::apl_mm()
{
  action_priority_list_t* default_list        = get_action_priority_list( "default" );
  action_priority_list_t* careful_aim         = get_action_priority_list( "careful_aim" );

  default_list -> add_action( "auto_shot" );

  add_item_actions( default_list );
  add_racial_actions( default_list );


  default_list -> add_action( this, "Chimaera Shot" );
  // "if=cast_regen+action.aimed_shot.cast_regen<focus.deficit"
  default_list -> add_action( this, "Kill Shot" );

  default_list -> add_action( this, "Rapid Fire");
  default_list -> add_action( "call_action_list,name=careful_aim,if=buff.careful_aim.up" );
  {
    careful_aim -> add_talent( this, "Barrage", "if=spell_targets.barrage>1" );
    // careful_aim -> add_action( this, "Steady Shot", "if=buff.pre_steady_focus.up&if=buff.pre_steady_focus.up&(14+cast_regen+action.aimed_shot.cast_regen)<=focus.deficit)" );
    careful_aim -> add_action( this, "Aimed Shot" );
    careful_aim -> add_action( this, "Steady Shot" );
  }
  default_list -> add_action( this, "Explosive Trap", "if=spell_targets.explosive_trap_tick>1" );

  default_list -> add_talent( this, "A Murder of Crows" );
  default_list -> add_talent( this, "Dire Beast", "if=cast_regen+action.aimed_shot.cast_regen<focus.deficit" );

  default_list -> add_talent( this, "Barrage" );
  default_list -> add_action( this, "Steady Shot", "if=buff.pre_steady_focus.up&(14+cast_regen+action.aimed_shot.cast_regen)<=focus.deficit", "Cast a second shot for steady focus if that won't cap us." );
  default_list -> add_action( this, "Multi-Shot", "if=spell_targets.multi_shot>6" );
  default_list -> add_action( this, "Aimed Shot", "if=talent.focusing_shot.enabled" );
  default_list -> add_action( this, "Aimed Shot", "if=focus+cast_regen>=85" );
  default_list -> add_talent( this, "Focusing Shot", "if=50+cast_regen-10<focus.deficit", "Allow FS to over-cap by 10 if we have nothing else to do" );
  default_list -> add_action( this, "Steady Shot" );
}

// Survival Action List ===================================================================

void hunter_t::apl_surv()
{
  action_priority_list_t* default_list  = get_action_priority_list( "default" );
  action_priority_list_t* aoe = get_action_priority_list( "aoe" );

  default_list -> add_action( "auto_shot" );

  add_racial_actions( default_list ); 
  add_item_actions( default_list ); 

  add_potion_action( default_list, "draenic_agility", "virmens_bite",
    "if=(((cooldown.stampede.remains<1)&(cooldown.a_murder_of_crows.remains<1))&(trinket.stat.any.up|buff.archmages_greater_incandescence_agi.up))|target.time_to_die<=25" );

  default_list -> add_action( "call_action_list,name=aoe,if=spell_targets.multi_shot>1" );

  default_list -> add_talent( this, "A Murder of Crows" );
  default_list -> add_talent( this, "Stampede", "if=buff.potion.up|(cooldown.potion.remains&(buff.archmages_greater_incandescence_agi.up|trinket.stat.any.up))|target.time_to_die<=45" );
  default_list -> add_action( this, "Black Arrow", "cycle_targets=1,if=remains<gcd*1.5" );
  default_list -> add_action( this, "Arcane Shot", "if=(trinket.proc.any.react&trinket.proc.any.remains<4)|dot.serpent_sting.remains<=3" );
  default_list -> add_action( this, "Explosive Shot" );
  default_list -> add_action( this, "Cobra Shot", "if=buff.pre_steady_focus.up" );
  default_list -> add_talent( this, "Dire Beast" );
  default_list -> add_action( this, "Arcane Shot", "if=target.time_to_die<4.5" );
  default_list -> add_talent( this, "Barrage" );
  default_list -> add_action( this, "Explosive Trap", "if=!trinket.proc.any.react&!trinket.stacking_proc.any.react" );
  default_list -> add_action( this, "Arcane Shot", "if=talent.steady_focus.enabled&!talent.focusing_shot.enabled&focus.deficit<action.cobra_shot.cast_regen*2+28" );
  default_list -> add_action( this, "Cobra Shot", "if=talent.steady_focus.enabled&buff.steady_focus.remains<5" );
  default_list -> add_talent( this, "Focusing Shot", "if=talent.steady_focus.enabled&buff.steady_focus.remains<=cast_time&focus.deficit>cast_regen" );
  default_list -> add_action( this, "Arcane Shot", "if=focus>=70|talent.focusing_shot.enabled|(talent.steady_focus.enabled&focus>=50)" );
  default_list -> add_talent( this, "Focusing Shot" );
  if ( true_level >= 81 )
    default_list -> add_action( this, "Cobra Shot" );
  else
    default_list -> add_action( this, "Steady Shot" );

  aoe -> add_talent( this, "Stampede", "if=buff.potion.up|(cooldown.potion.remains&(buff.archmages_greater_incandescence_agi.up|trinket.stat.any.up|buff.archmages_incandescence_agi.up))" );
  aoe -> add_action( this, "Explosive Shot", "if=buff.lock_and_load.react&(!talent.barrage.enabled|cooldown.barrage.remains>0)" );
  aoe -> add_talent( this, "Barrage" );
  aoe -> add_action( this, "Black Arrow", "cycle_targets=1,if=remains<gcd*1.5" );
  aoe -> add_action( this, "Explosive Shot", "if=spell_targets.multi_shot<5" );
  aoe -> add_action( this, "Explosive Trap", "if=dot.explosive_trap.remains<=5" );
  aoe -> add_talent( this, "A Murder of Crows" );
  aoe -> add_talent( this, "Dire Beast" );
  aoe -> add_action( this, "Multi-shot", "if=dot.serpent_sting.remains<=5|target.time_to_die<4.5" );
  aoe -> add_talent( this, "Powershot" );
  aoe -> add_action( this, "Cobra Shot", "if=buff.pre_steady_focus.up&buff.steady_focus.remains<5&focus+14+cast_regen<80" );
  aoe -> add_action( this, "Multi-shot", "if=focus>=70|talent.focusing_shot.enabled" );
  aoe -> add_talent( this, "Focusing Shot" );
  if ( true_level >= 81 )
    aoe -> add_action( this, "Cobra Shot" );
  else
    aoe -> add_action( this, "Steady Shot" );
}

// NO Spec Combat Action Priority List ======================================

void hunter_t::apl_default()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );

  default_list -> add_action( this, "Arcane Shot" );
}

// hunter_t::reset ==========================================================

void hunter_t::reset()
{
  player_t::reset();

  // Active
  active.pet            = nullptr;
  active.ammo           = NO_AMMO;
}

// hunter_t::arise ==========================================================

void hunter_t::arise()
{
  player_t::arise();
}

// hunter_t::composite_rating_multiplier =====================================

double hunter_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  return m;
}

// hunter_t::regen ===========================================================

void hunter_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );
  if ( buffs.steady_focus -> up() )
  {
    double base = focus_regen_per_second() * buffs.steady_focus -> current_value;
    resource_gain( RESOURCE_FOCUS, base * periodicity.total_seconds(), gains.steady_focus );
  }
}

// hunter_t::composite_attack_power_multiplier ==============================

double hunter_t::composite_attack_power_multiplier() const
{
  double mult = player_t::composite_attack_power_multiplier();

  return mult;
}

// hunter_t::composite_melee_crit ===========================================

double hunter_t::composite_melee_crit() const
{
  double crit = player_t::composite_melee_crit();

  if ( buffs.bullseye -> check() )
    crit += buffs.bullseye -> check_stack_value();

  if ( buffs.aspect_of_the_wild -> check() )
    crit += buffs.aspect_of_the_wild -> check_value();

  return crit;
}

// hunter_t::composite_spell_crit ===========================================

double hunter_t::composite_spell_crit() const
{
  double crit = player_t::composite_spell_crit();

  if ( buffs.bullseye -> check() )
    crit += buffs.bullseye -> check_stack_value();

  if ( buffs.aspect_of_the_wild -> check() )
    crit += buffs.aspect_of_the_wild -> check_value();

  return crit;
}

// Haste and speed buff computations ========================================

double hunter_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();
  return h;
}

// hunter_t::composite_player_critical_damage_multiplier ====================

double hunter_t::composite_player_critical_damage_multiplier() const
{
  double cdm = player_t::composite_player_critical_damage_multiplier();

  // we use check() for rapid_fire becuase it's usage is reported from value() above
  if ( sets.has_set_bonus( HUNTER_MARKSMANSHIP, T17, B4 ) && buffs.trueshot -> check() )
  {
    // deadly_aim_driver
    double seconds_buffed = floor( buffs.trueshot -> elapsed( sim -> current_time() ).total_seconds() );
    // from Nimox
    cdm += bugs ? std::min(15.0, seconds_buffed) * 0.03
                : seconds_buffed * sets.set( HUNTER_MARKSMANSHIP, T17, B4 ) -> effectN( 1 ).percent();
  }

  if( buffs.rapid_killing -> up() )
    cdm *= 1.0 + buffs.rapid_killing -> value();

  return cdm;
}

// hunter_t::composite_player_multiplier ====================================

double hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.bestial_wrath -> up() )
    m *= 1.0 + buffs.bestial_wrath -> current_value;

  if ( longview && specialization() == HUNTER_MARKSMANSHIP )
  {
    const spell_data_t* data = find_spell( longview -> spell_id ); 
    double factor = data -> effectN( 1 ).average( longview -> item ) / 100.0 / 100.0;
    if ( sim -> distance_targeting_enabled )
      m*= 1.0 + target -> get_position_distance( x_position, y_position ) * factor;
    else
      m *= 1.0 + base.distance * factor;
  }

  return m;
}

double hunter_t::focus_regen_per_second() const
{
  double regen = player_t::focus_regen_per_second();

  if ( buffs.aspect_of_the_wild -> check() )
    regen += buffs.aspect_of_the_wild -> data().effectN( 2 ).resource( RESOURCE_FOCUS );

  return regen;
}

// hunter_t::invalidate_cache ==============================================

void hunter_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_MASTERY:
    if ( mastery.sniper_training -> ok() )
    {
      player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      if ( sim -> distance_targeting_enabled && mastery.sniper_training -> ok() )
      {
        // Marksman is a unique butterfly, since mastery changes the max range of abilities. We need to regenerate every target cache.
        for ( size_t i = 0, end = action_list.size(); i < end; i++ )
        {
          action_list[i] -> target_cache.is_valid = false;
        }
      }
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

// hunter_t::create_options =================================================

void hunter_t::create_options()
{
  player_t::create_options();

  add_option( opt_string( "summon_pet", summon_pet_str ) );
}

// hunter_t::create_profile =================================================

std::string hunter_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  profile_str += "summon_pet=" + summon_pet_str + "\n";

  return profile_str;
}

// hunter_t::copy_from ======================================================

void hunter_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  hunter_t* p = debug_cast<hunter_t*>( source );

  summon_pet_str = p -> summon_pet_str;
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

// hunter_::convert_hybrid_stat ==============================================

stat_e hunter_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT:
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

// hunter_t::schedule_ready() =======================================================

/* Set the careful_aim buff state based on rapid fire and the enemy health. */
void hunter_t::schedule_ready( timespan_t delta_time, bool waiting )
{
  if ( talents.careful_aim -> ok() )
  {
    int ca_now = buffs.careful_aim -> check();
    int threshold = talents.careful_aim -> effectN( 2 ).base_value();
    if ( target -> health_percentage() > threshold )
    {
      if ( ! ca_now )
        buffs.careful_aim -> trigger();
    }
    else
    {
      if ( ca_now )
        buffs.careful_aim -> expire();
    }
  }
  else if ( talents.big_game_hunter -> ok() )
  {
    int bgh_now = buffs.big_game_hunter -> check();
    int threshold = talents.big_game_hunter -> effectN( 2 ).base_value();
    if ( target -> health_percentage() > threshold )
    {
      if ( ! bgh_now )
        buffs.big_game_hunter -> trigger();
    }
    else
    {
      if ( bgh_now )
        buffs.big_game_hunter -> expire();
    }
  }
  player_t::schedule_ready( delta_time, waiting );
}

// hunter_t::moving() =======================================================

/* Override moving() so that it doesn't suppress auto_shot and only interrupts the few shots that cannot be used while moving.
*/
void hunter_t::moving()
{
  if ( ( executing && ! executing -> usable_moving() ) || ( channeling && ! channeling -> usable_moving() ) )
    player_t::interrupt();
}

void hunter_t::finish_moving()
{
  player_t::finish_moving();

  movement_ended = sim -> current_time();
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class hunter_report_t: public player_report_extension_t
{
public:
  hunter_report_t( hunter_t& player ):
    p( player )
  {
  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void)p;
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

struct hunter_module_t: public module_t
{
  hunter_module_t(): module_t( HUNTER ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new hunter_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new hunter_report_t( *p ) );
    return p;
  }

  virtual bool valid() const override { return true; }
  
  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 184900, beastlord );
    unique_gear::register_special_effect( 184901, longview );
    unique_gear::register_special_effect( 184902, blackness );
    unique_gear::register_special_effect( 190852, thasdorah );
    unique_gear::register_special_effect( 197344, titanstrike );
  }

  virtual void init( player_t* p ) const override
  {
    p -> buffs.aspect_of_the_pack = buff_creator_t( p, "aspect_of_the_pack",
                                                    p -> find_class_spell( "Aspect of the Pack" ) );
    p -> buffs.aspect_of_the_fox  = buff_creator_t( p, "aspect_of_the_fox",
                                    p -> find_spell( 172106 ) )
      .cd( timespan_t::zero() );
  }

  virtual void register_hotfixes() const override
  {
  }

  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::hunter()
{
  static hunter_module_t m;
  return &m;
}
