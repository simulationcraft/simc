//==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// TODO
// General
//   - Cleanup old spells
//
// Survival
//   - Harpoon legendary
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
struct hunter_secondary_pet_t;
struct hati_t;
struct dire_critter_t;
}

enum aspect_type { ASPECT_NONE = 0, ASPECT_MAX };

enum exotic_munitions { NO_AMMO = 0, FROZEN_AMMO = 2, INCENDIARY_AMMO = 4, POISONED_AMMO = 8 };

struct hunter_td_t: public actor_target_data_t
{
  struct debuffs_t
  {
    buff_t* hunters_mark;
    buff_t* vulnerable;
    buff_t* true_aim;
    buff_t* lacerate;
    buff_t* t18_2pc_open_wounds;
    buff_t* mark_of_helbrine;
  } debuffs;

  struct dots_t
  {
    dot_t* serpent_sting;
    dot_t* poisoned_ammo;
    dot_t* piercing_shots;
    dot_t* lacerate;
    dot_t* on_the_trail;
  } dots;

  hunter_td_t( player_t* target, hunter_t* p );
};

struct hunter_t: public player_t
{
public:
  timespan_t movement_ended;

  // Active
  std::vector<pets::hunter_main_pet_t*> hunter_main_pets;
  std::vector<std::pair<cooldown_t*, proc_t*>> animal_instincts_cds;
  struct actives_t
  {
    pets::hunter_main_pet_t* pet;
    action_t*           serpent_sting;
    action_t*           piercing_shots;
    action_t*           surge_of_the_stormgod;
  } active;

  std::array< pets::dire_critter_t*, 8 >  pet_dire_beasts;
  pets::hati_t* hati;
  std::array< pets::hunter_secondary_pet_t*, 2 > dark_minion;
  std::array< pets::hunter_secondary_pet_t*, 10 > felboars;

  // Tier 18 (WoD 6.2) trinket effects
  const special_effect_t* beastlord;
  const special_effect_t* longview;
  const special_effect_t* blackness;

  double blackness_multiplier;

  struct legendary_t
  {
    // Survival
    const special_effect_t* sv_feet;
    const special_effect_t* sv_ring;
    const special_effect_t* sv_waist;
    const special_effect_t* wrist;

    // Beast Mastery
    const special_effect_t* bm_feet;
    const special_effect_t* bm_ring;
    const special_effect_t* bm_waist;

    // Marksmanship
    const special_effect_t* mm_feet;
    const special_effect_t* mm_ring;
    const special_effect_t* mm_waist;
    const special_effect_t* magnetized_blasting_cap_launcher;
  } legendary;

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
    buff_t* dire_beast;
    buff_t* steady_focus;
    buff_t* pre_steady_focus;
    buff_t* marking_targets;
    buff_t* hunters_mark_exists;
    buff_t* lock_and_load;
    buff_t* stampede;
    buff_t* trick_shot;
    buff_t* trueshot;
    buff_t* volley;
    buff_t* rapid_killing;
    buff_t* bullseye;
    buff_t* heavy_shot; // t17 SV 4pc
    buff_t* mongoose_fury;
    buff_t* fury_of_the_eagle;
    buff_t* aspect_of_the_eagle;
    buff_t* moknathal_tactics;
    buff_t* spitting_cobra;
    buff_t* t18_2p_rapid_fire;
    buff_t* t18_2p_dire_longevity;
    buff_t* t19_4p_mongoose_power;
    buff_t* sentinels_sight;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* explosive_shot;
    cooldown_t* black_arrow;
    cooldown_t* bestial_wrath;
    cooldown_t* trueshot;
    cooldown_t* dire_beast;
    cooldown_t* dire_frenzy;
    cooldown_t* kill_command;
    cooldown_t* mongoose_bite;
    cooldown_t* lacerate;
    cooldown_t* flanking_strike;
    cooldown_t* harpoon;
    cooldown_t* aspect_of_the_eagle;
    cooldown_t* aspect_of_the_wild;
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
    gain_t* aspect_of_the_wild;
    gain_t* spitting_cobra;
    gain_t* nesingwarys_trapping_treads;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* lock_and_load;
    proc_t* wild_call;
    proc_t* tier17_2pc_bm;
    proc_t* tier18_4pc_bm;
    proc_t* black_arrow_trinket_reset;
    proc_t* tier18_2pc_mm_wasted_proc;
    proc_t* tier18_2pc_mm_wasted_overwrite;
    proc_t* hunting_companion;
    proc_t* mortal_wounds;
    proc_t* t18_4pc_sv;
    proc_t* no_vuln_aimed_shot;
    proc_t* no_vuln_marked_shot;
    proc_t* zevrims_hunger;
    proc_t* convergence;
    proc_t* marking_targets;
    proc_t* wasted_marking_targets;
    proc_t* animal_instincts_mongoose;
    proc_t* animal_instincts_aspect;
    proc_t* animal_instincts_harpoon;
    proc_t* animal_instincts_flanking;
    proc_t* animal_instincts;
  } procs;

  real_ppm_t* ppm_hunters_mark;
  real_ppm_t* ppm_call_of_the_hunter;

  // Talents
  struct talents_t
  {
    // tier 1
    const spell_data_t* big_game_hunter;
    const spell_data_t* way_of_the_cobra;
    const spell_data_t* dire_stable;

    const spell_data_t* lone_wolf;
    const spell_data_t* steady_focus;
    const spell_data_t* careful_aim;

    const spell_data_t* animal_instincts;
    const spell_data_t* throwing_axes;
    const spell_data_t* way_of_the_moknathal;

    // tier 2
    const spell_data_t* stomp;
    const spell_data_t* dire_frenzy;
    const spell_data_t* chimaera_shot;
    
    const spell_data_t* lock_and_load;
    const spell_data_t* black_arrow;
    const spell_data_t* true_aim;
    
    const spell_data_t* mortal_wounds;
    const spell_data_t* snake_hunter;

    // tier 3
    const spell_data_t* posthaste;
    const spell_data_t* farstrider;
    const spell_data_t* dash;

    // tier 4
    const spell_data_t* one_with_the_pack;
    const spell_data_t* bestial_fury;
    const spell_data_t* blink_strikes;
    
    const spell_data_t* explosive_shot;
    const spell_data_t* sentinel;
    const spell_data_t* patient_sniper;

    const spell_data_t* caltrops;
    const spell_data_t* steel_trap;
    const spell_data_t* improved_traps;

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
    const spell_data_t* dragonsfire_grenade;
    const spell_data_t* serpent_sting;

    // tier 7
    const spell_data_t* stampede;
    const spell_data_t* killer_cobra;
    const spell_data_t* aspect_of_the_beast;

    const spell_data_t* sidewinders;
    const spell_data_t* piercing_shot;
    const spell_data_t* trick_shot;

    const spell_data_t* spitting_cobra;
    const spell_data_t* expert_trapper;
  } talents;

  // Specialization Spells
  struct specs_t
  {
    const spell_data_t* critical_strikes;

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
    const spell_data_t* lacerate;
    const spell_data_t* aspect_of_the_eagle;
    const spell_data_t* carve;
    const spell_data_t* tar_trap;
    const spell_data_t* survivalist;
    const spell_data_t* explosive_trap;
  } specs;

  // Glyphs
  struct glyphs_t
  {
    const spell_data_t* arachnophobia;
    const spell_data_t* lesser_proportion;
    const spell_data_t* nesingwarys_nemesis;
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
    artifact_power_t hatis_bond;
    artifact_power_t beast_master;
    artifact_power_t titans_thunder;
    artifact_power_t master_of_beasts;
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
    artifact_power_t wind_arrows;
    artifact_power_t legacy_of_the_windrunners;
    artifact_power_t call_of_the_hunter;
    artifact_power_t bullseye;
    artifact_power_t deadly_aim;
    artifact_power_t quick_shot;
    artifact_power_t critical_focus;
    artifact_power_t windrunners_guidance;
    artifact_power_t called_shot;
    artifact_power_t marked_for_death;
    artifact_power_t precision;
    artifact_power_t rapid_killing;
    artifact_power_t mark_of_the_windrunner;

    // Survival
    artifact_power_t fury_of_the_eagle;
    artifact_power_t iron_talons;
    artifact_power_t talon_strike;
    artifact_power_t eagles_bite;
    artifact_power_t aspect_of_the_skylord;
    artifact_power_t sharpened_fang;
    artifact_power_t raptors_cry;
    artifact_power_t hellcarver;
    artifact_power_t my_beloved_monster;
    artifact_power_t explosive_force;
    artifact_power_t fluffy_go;
    artifact_power_t jagged_claws;
    artifact_power_t lacerating_talons;
    artifact_power_t embrace_of_the_aspects;
    artifact_power_t hunters_guile;

    // Paragon points
    artifact_power_t windflight_arrows;
    artifact_power_t spiritbound;
    artifact_power_t voice_of_the_wild_gods;
  } artifacts;

  stats_t* stats_tier17_4pc_bm;
  stats_t* stats_tier18_4pc_bm;

  player_t* last_true_aim_target;

  bool clear_next_hunters_mark;

  hunter_t( sim_t* sim, const std::string& name, race_e r = RACE_NONE ):
    player_t( sim, HUNTER, name, r ),
    active( actives_t() ),
    pet_dire_beasts(),
    hati(),
    dark_minion(),
    felboars(),
    beastlord( nullptr ),
    longview( nullptr ),
    blackness( nullptr ),
    blackness_multiplier(),
    legendary( legendary_t() ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    procs( procs_t() ),
    ppm_hunters_mark( nullptr ),
    ppm_call_of_the_hunter( nullptr ),
    talents( talents_t() ),
    specs( specs_t() ),
    glyphs( glyphs_t() ),
    mastery( mastery_spells_t() ),
    stats_tier17_4pc_bm( nullptr ),
    stats_tier18_4pc_bm( nullptr ),
    last_true_aim_target( nullptr ),
    clear_next_hunters_mark( true )
  {
    // Cooldowns
    cooldowns.explosive_shot  = get_cooldown( "explosive_shot" );
    cooldowns.black_arrow     = get_cooldown( "black_arrow" );
    cooldowns.bestial_wrath   = get_cooldown( "bestial_wrath" );
    cooldowns.trueshot        = get_cooldown( "trueshot" );
    cooldowns.dire_beast      = get_cooldown( "dire_beast" );
    cooldowns.dire_frenzy     = get_cooldown( "dire_frenzy" );
    cooldowns.kill_command    = get_cooldown( "kill_command" );
    cooldowns.mongoose_bite   = get_cooldown( "mongoose_bite" );
    cooldowns.lacerate        = get_cooldown( "lacerate" );
    cooldowns.flanking_strike = get_cooldown( "flanking_strike" );
    cooldowns.harpoon         = get_cooldown( "harpoon" );
    cooldowns.aspect_of_the_eagle = get_cooldown( "aspect_of_the_eagle" );
    cooldowns.aspect_of_the_wild  = get_cooldown( "aspect_of_the_wild" );

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
  virtual bool      init_special_effects() override;
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
  virtual double    composite_melee_crit_chance() const override;
  virtual double    composite_spell_crit_chance() const override;
  virtual double    composite_melee_haste() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_mastery_value() const override;
  virtual double    composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  virtual double    composite_rating_multiplier( rating_e rating ) const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double            composite_player_pet_damage_multiplier( const action_state_t* ) const override;
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

  double beast_multiplier() const
  {
    double pm = 1.0;
    if ( mastery.master_of_beasts -> ok() )
      pm *= 1.0 + cache.mastery_value();

    if ( artifacts.spiritbound.rank() )
      pm *= 1.0 + artifacts.spiritbound.percent();

    return pm;
  }

  // shared code to interrupt a steady focus pair if the ability is cast between steady/cobras
  virtual void no_steady_focus()
  {
    buffs.pre_steady_focus -> expire();
  }
};

static void init_special_effect( hunter_t*                 r,
                             specialization_e          spec,
                             const special_effect_t*&  ptr,
                             const special_effect_t&   effect )
{
  if ( ! r -> find_spell( effect.spell_id ) -> ok() || ( r -> specialization() != spec && spec != SPEC_NONE ) )
  {
    return;
  }

  ptr = &( effect );
}

static void beastlord( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  init_special_effect( hunter, HUNTER_BEAST_MASTERY, hunter -> beastlord, effect );
}

static void longview( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  init_special_effect( hunter, HUNTER_MARKSMANSHIP, hunter -> longview, effect );
}

static void blackness( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  init_special_effect( hunter, HUNTER_SURVIVAL, hunter -> blackness, effect );
}

static void sv_feet( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  init_special_effect( hunter, HUNTER_SURVIVAL, hunter -> legendary.sv_feet, effect );
}

static void sv_ring( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  init_special_effect( hunter, HUNTER_SURVIVAL, hunter -> legendary.sv_ring, effect );
}

static void sv_waist( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  init_special_effect( hunter, HUNTER_SURVIVAL, hunter -> legendary.sv_waist, effect );
}

static void wrist( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  init_special_effect( hunter, SPEC_NONE, hunter -> legendary.wrist, effect );
}

static void bm_feet( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  init_special_effect( hunter, HUNTER_BEAST_MASTERY, hunter -> legendary.bm_feet, effect );
}

static void bm_ring( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  init_special_effect( hunter, HUNTER_BEAST_MASTERY, hunter -> legendary.bm_ring, effect );
}

static void bm_waist( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  init_special_effect( hunter, HUNTER_BEAST_MASTERY, hunter -> legendary.bm_waist, effect );
}

static void mm_feet( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  init_special_effect( hunter, HUNTER_MARKSMANSHIP, hunter -> legendary.mm_feet, effect );
}

static void mm_ring( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  init_special_effect( hunter, HUNTER_MARKSMANSHIP, hunter -> legendary.mm_ring, effect );
}

static void mm_waist( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  init_special_effect( hunter, HUNTER_MARKSMANSHIP, hunter -> legendary.mm_waist, effect );
}

static void magnetized_blasting_cap_launcher( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  init_special_effect( hunter, HUNTER_MARKSMANSHIP, hunter -> legendary.magnetized_blasting_cap_launcher, effect );
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

  double action_multiplier() const override
  {
    double am = ab::action_multiplier();

    if ( lone_wolf )
      am *= 1.0 + p() -> talents.lone_wolf -> effectN( 1 ).percent();

    if ( p() -> buffs.t19_4p_mongoose_power -> up() && ab::special )
      am *= 1.0 + p() -> buffs.t19_4p_mongoose_power -> default_value;

    return am;
  }

  virtual double cost() const override
  {
    double cost = ab::cost();

    if ( p() -> sets.has_set_bonus( HUNTER_MARKSMANSHIP, T19, B4 ) && p() -> buffs.trueshot -> check() )
      cost *= 0.50; //TODO: Wait to see if spell data is updated with a value

    if ( p() -> legendary.bm_waist && p() -> buffs.bestial_wrath -> check() )
      cost *= 1.0 + p() -> find_spell( 207318 ) -> effectN( 1 ).percent();

    return cost;
  }

  virtual timespan_t gcd() const override
  {
    timespan_t g = ab::gcd();
    timespan_t m = ab::min_gcd;

    if ( g == timespan_t::zero() )
      return timespan_t::zero();

    g *= p() -> cache.attack_haste();

    return g < m ? m : g;
  }

  virtual double cast_regen() const
  {
    double cast_seconds = ab::execute_time().total_seconds();
    double sf_seconds = std::min( cast_seconds, p() -> buffs.steady_focus -> remains().total_seconds() );
    double regen = p() -> focus_regen_per_second();
    return ( regen * cast_seconds ) + ( regen * p() -> buffs.steady_focus -> current_value * sf_seconds );
  }

// action list expressions

  expr_t* create_expression( const std::string& name ) override
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
void trigger_true_aim( hunter_t* p, player_t* t, int stacks = 1 )
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

    td_curr -> debuffs.true_aim -> trigger( stacks );
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
    aoe = -1;
    background = special = may_crit = true;
    callbacks = false;
    range = data().effectN( 1 ).radius();
    school = SCHOOL_PHYSICAL;
    weapon = &( p -> main_hand_weapon );
  }

  bool init_finished() override
  {
    // Find first blademaster pet, it'll be the first trinket-created pet
    pet_t* main_pet = player -> cast_pet() -> owner -> find_pet( BLADEMASTER_PET_NAME );

    if ( player != main_pet )
      stats = main_pet -> find_action( "felstorm_tick" ) -> stats;

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
      stats = main_pet -> find_action( "felstorm" ) -> stats;

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
      d -> cancel();
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
    action_t* dire_frenzy;
    action_t* kill_command;
    action_t* flanking_strike;
    attack_t* beast_cleave;
    action_t* titans_thunder;
  } active;

  struct specs_t
  {
    // ferocity
    const spell_data_t* heart_of_the_phoenix;
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
    buff_t* beast_cleave;
    buff_t* dire_frenzy;
    buff_t* titans_frenzy;
    buff_t* tier17_4pc_bm;
    buff_t* tier18_4pc_bm;
  } buffs;

  // Gains
  struct gains_t
  {
    gain_t* steady_focus;
    gain_t* aspect_of_the_wild;
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
    case PET_CARRION_BIRD: return "bloody_screech";
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
    case PET_CROCOLISK:    return "ankle_crack";
    case PET_GORILLA:      return "";
    case PET_RHINO:        return "";
    case PET_SCORPID:      return "deadly_sting";
    case PET_SHALE_SPIDER: return "";
    case PET_TURTLE:       return "";
    case PET_WARP_STALKER: return "warp_time";
    case PET_WORM:         return "";
    case PET_BAT:          return "";
    case PET_BIRD_OF_PREY: return "";
    case PET_CHIMAERA:     return "";
    case PET_DRAGONHAWK:   return "";
    case PET_NETHER_RAY:   return "";
    case PET_RAVAGER:      return "";
    case PET_SERPENT:      return "";
    case PET_SILITHID:     return "tendon_rip";
    case PET_SPIDER:       return "web_spray";
    case PET_SPOREBAT:     return "";
    case PET_WIND_SERPENT: return "";
    case PET_FOX:          return "";
    //case PET_RIVERBEAST:   return "gruesome_bite";
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

    buffs.aspect_of_the_wild = 
      buff_creator_t( this, "aspect_of_the_wild", o() -> specs.aspect_of_the_wild )
        .affects_regen( true )
        .cd( timespan_t::zero() )
        .default_value( o() -> specs.aspect_of_the_wild -> effectN( 1 ).percent() );

    if ( o() -> artifacts.wilderness_expert.rank() )
      buffs.aspect_of_the_wild -> buff_duration += o() -> artifacts.wilderness_expert.time_value();

    // Bestial Wrath
    buffs.bestial_wrath = 
      buff_creator_t( this, "bestial_wrath", o() -> specs.bestial_wrath )
        .activated( true )
        .cd( timespan_t::zero() )
        .default_value( o() -> specs.bestial_wrath 
                            -> effectN( 1 )
                              .percent() )
        .duration( timespan_t::from_seconds( 15.0 ) );

    if ( o() -> talents.bestial_fury -> ok() )
    {
      buffs.bestial_wrath 
        -> default_value += o() -> talents.bestial_fury 
                                -> effectN( 1 )
                                  .percent();
    }
    if ( o() -> artifacts.unleash_the_beast.rank() )
    {
      buffs.bestial_wrath 
        -> default_value += o() -> artifacts.unleash_the_beast
                                  .percent();
    }
    if ( o() -> beastlord )
    {
      buffs.bestial_wrath 
        -> buff_duration *= 1.0 + ( 
                              find_spell( o() -> beastlord -> spell_id ) 
                              -> effectN( 1 )
                              .average( o() -> beastlord -> item ) 
                              / 100.0 );
    }

    // Beast Cleave
    double cleave_value = o() -> find_specialization_spell( "Beast Cleave" ) 
                              -> effectN( 1 )
                                .percent();
    if ( o() -> artifacts.furious_swipes.rank() )
    {
      cleave_value *= 1.0 + 
                      o() -> artifacts.furious_swipes
                      .percent();
    }
    buffs.beast_cleave = 
      buff_creator_t( this, 118455, "beast_cleave" )
        .activated( true )
        .default_value( cleave_value );

    // Dire Frenzy
    double frenzy_value = o() -> find_talent_spell( "Dire Frenzy" ) 
                          -> effectN( 2 )
                          .percent();
    if ( o() -> artifacts.beast_master.rank() )
    {
      frenzy_value += o() -> artifacts.beast_master
                      .data()
                      .effectN( 2 )
                      .percent();
    }
    buffs.dire_frenzy = 
      buff_creator_t( this, "dire_frenzy", o() -> talents.dire_frenzy )
        .default_value ( frenzy_value )
        .cd( timespan_t::zero() )
        .add_invalidate( CACHE_ATTACK_HASTE );

    buffs.titans_frenzy = 
      buff_creator_t( this, "titans_frenzy", o() -> artifacts.titans_thunder )
        .duration( timespan_t::from_seconds( 30.0 ) );

    buffs.tier17_4pc_bm = 
      buff_creator_t( this, 178875, "tier17_4pc_bm" )
        .default_value( owner -> find_spell( 178875 ) -> effectN( 2 ).percent() )
        .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

    buffs.tier18_4pc_bm = 
      buff_creator_t( this, "tier18_4pc_bm" )
        .default_value( owner -> find_spell( 178875 ) -> effectN( 2 ).percent() )
        .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  virtual void init_gains() override
  {
    base_t::init_gains();

    gains.steady_focus       = get_gain( "steady_focus" );
    gains.aspect_of_the_wild = get_gain( "aspect_of_the_wild" );
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
      if ( o() -> specialization() == HUNTER_SURVIVAL )
        action_list_str += "/growl";
      action_list_str += "/claw";
      action_list_str += "/wait_until_ready";
      use_default_action_list = true;
    }

    base_t::init_action_list();
  }

  virtual double composite_melee_crit_chance() const override
  {
    double ac = base_t::composite_melee_crit_chance();
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
    if ( buffs.aspect_of_the_wild -> up() )
      resource_gain( RESOURCE_FOCUS, buffs.aspect_of_the_wild -> data().effectN( 2 ).resource( RESOURCE_FOCUS ) * periodicity.total_seconds(), gains.aspect_of_the_wild );
  }

  double focus_regen_per_second() const override
  { return o() -> focus_regen_per_second() * 1.25; }

  virtual double composite_melee_speed() const override
  {
    double ah = base_t::composite_melee_speed();

    ah *= 1.0 / ( 1.0 + specs.spiked_collar -> effectN( 2 ).percent() );

    if ( o() -> talents.dire_frenzy -> ok() )
      ah *= 1.0 / ( 1.0 + buffs.dire_frenzy -> check_stack_value() );

    if ( o() -> artifacts.fluffy_go.rank() )
      ah *= 1.0  / ( 1.0 + o() -> artifacts.fluffy_go.percent() );

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
    if ( this == o() -> active.pet || buffs.tier17_4pc_bm -> check() || buffs.tier18_4pc_bm -> check() )
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

  virtual void demise() override
  {
    base_t::demise();

    if ( o() -> active.pet == this )
      o() -> active.pet = nullptr;
  }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = base_t::composite_player_multiplier( school );

    if ( buffs.bestial_wrath -> up() )
      m *= 1.0 + buffs.bestial_wrath -> current_value;

    // from Nimox: 178875 is the 4pc BM pet damage buff
    if ( buffs.tier17_4pc_bm -> up() )
      m *= 1.0 + buffs.tier17_4pc_bm -> current_value;

    // Pet combat experience
    m *= 1.0 + specs.combat_experience -> effectN( 2 ).percent();

    if ( o() -> legendary.bm_ring )
      m *= 1.1; //TODO: Find corresponding spell, or wait for spell data to be updated

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

  virtual resource_e primary_resource() const override 
  { return RESOURCE_FOCUS; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override;

  virtual void init_spells() override;

  void moving() override
  { return; }
};

// ==========================================================================
// Secondary pets: Dire Beast, Hati, Black Arrow
// ==========================================================================

struct hunter_secondary_pet_action_t: hunter_pet_action_t < hunter_secondary_pet_t, melee_attack_t >
{
  hunter_secondary_pet_action_t( const std::string &attack_name, hunter_secondary_pet_t& p, const spell_data_t* s = spell_data_t::nil() ):
    base_t( attack_name, p, s )
  {
      may_crit = true;
      school = SCHOOL_PHYSICAL;
      weapon = &( player -> main_hand_weapon );
      weapon_multiplier = 0.0;
  }
};

struct secondary_pet_melee_t: public hunter_secondary_pet_action_t
{
  secondary_pet_melee_t( const std::string &attack_name, 
                         hunter_secondary_pet_t& p, 
                         const spell_data_t* s = spell_data_t::nil() );
};

struct hunter_secondary_pet_t: public hunter_pet_t
{

  hunter_secondary_pet_t( hunter_t& owner, const std::string &pet_name ):
    hunter_pet_t( *owner.sim, owner, pet_name, PET_HUNTER, true /*GUARDIAN*/ )
  {
    owner_coeff.ap_from_ap = 1.15;
    regen_type = REGEN_DISABLED;
  }

  virtual void init_base_stats() override
  {
    hunter_pet_t::init_base_stats();

    // FIXME numbers are from treant. correct them.
    resources.base[RESOURCE_HEALTH] = 9999; // Level 85 value
    resources.base[RESOURCE_MANA] = 0;

    stamina_per_owner = 0;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, o() -> level() );
    main_hand_weapon.max_dmg    = main_hand_weapon.min_dmg;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2 );
    main_hand_weapon.type       = WEAPON_BEAST;

    main_hand_attack = new secondary_pet_melee_t( name_str + "_melee", *this );
  }

  virtual void summon( timespan_t duration = timespan_t::zero() ) override
  {
    hunter_pet_t::summon( duration );

    // attack immediately on summons
    main_hand_attack -> execute();
  }
};

secondary_pet_melee_t::secondary_pet_melee_t( const std::string &attack_name, hunter_secondary_pet_t& p, const spell_data_t* s ): 
  hunter_secondary_pet_action_t( attack_name, p, s )
{
  attack_power_mod.direct = 0.5714;
  background = true;
  base_dd_min = base_dd_max = player -> dbc.spell_scaling( p.o() -> type, p.o() -> level() );
  base_execute_time = weapon -> swing_time;
  may_glance = true;
  may_crit = true;
  repeating = true;
  special = false;
  trigger_gcd = timespan_t::zero();
  weapon_multiplier = 0;
}

// ==========================================================================
// Dire Critter
// ==========================================================================

struct dire_critter_t: public hunter_secondary_pet_t
{
  struct dire_beast_stomp_t: public hunter_secondary_pet_action_t
  {
    dire_beast_stomp_t( hunter_secondary_pet_t &p ):
      hunter_secondary_pet_action_t( "stomp", p, p.find_spell( 201754 ) )
    {
      aoe = -1;
    }

    bool init_finished() override
    {
      if ( p() -> o() -> pet_dire_beasts[ 0 ] )
        stats = p() -> o() -> pet_dire_beasts[ 0 ] -> get_stats( "stomp" );

      return hunter_secondary_pet_action_t::init_finished();
    }
  };

  struct dire_beast_melee_t: public secondary_pet_melee_t
  {
    dire_beast_melee_t( dire_critter_t& p ):
      secondary_pet_melee_t( "dire_beast_melee", p )
    {
    }

    bool init_finished() override
    {
      if ( p() -> o() -> pet_dire_beasts[ 0 ] )
        stats = p() -> o() -> pet_dire_beasts[ 0 ] -> get_stats( "dire_beast_melee" );

      return secondary_pet_melee_t::init_finished();
    }
  };

  struct actives_t
  {
    action_t* jaws_of_thunder;
    action_t* stomp;
    action_t* titans_thunder;
  } active;

  struct buffs_t
  {
    buff_t* bestial_wrath;
  } buffs;

  dire_critter_t( hunter_t& owner ):
    hunter_secondary_pet_t( owner, std::string( "dire_beast" ) )
  {
    owner_coeff.ap_from_ap = 1.35;
  }

  virtual void init_base_stats() override
  {
    hunter_secondary_pet_t::init_base_stats();

    main_hand_attack = new dire_beast_melee_t( *this );
  }

  virtual void init_spells() override;

  virtual void summon( timespan_t duration = timespan_t::zero() ) override
  {
    hunter_secondary_pet_t::summon( duration );

    if ( o() -> talents.stomp -> ok() )
      active.stomp -> execute();
  }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double cpm = hunter_secondary_pet_t::composite_player_multiplier( school );

    if ( o() -> artifacts.beast_master.rank() )
      cpm *= 1.0 + o() -> artifacts.beast_master.percent();

    if ( buffs.bestial_wrath -> up() )
      cpm *= 1.0 + buffs.bestial_wrath -> default_value;

    return cpm;
  }

  virtual void create_buffs() override
  {
    hunter_secondary_pet_t::create_buffs();

    buffs.bestial_wrath = 
      buff_creator_t( this, 211183, "bestial_wrath" )
        .default_value( find_spell( 211183 ) -> effectN( 1 ).percent() );
  }
};

// ==========================================================================
// Hati
// ==========================================================================

struct hati_t: public hunter_secondary_pet_t
{
  struct actives_t
  {
    action_t* beast_cleave;
    action_t* jaws_of_thunder;
    action_t* kill_command;
    action_t* titans_thunder;
  } active;

  struct buffs_t
  {
    buff_t* beast_cleave;
    buff_t* bestial_wrath;
  } buffs;

  hati_t( hunter_t& owner ):
    hunter_secondary_pet_t( owner, std::string( "hati" ) )
  {
  }

  virtual void init_spells() override;

  virtual void create_buffs() override
  {
    hunter_secondary_pet_t::create_buffs();

    // Bestial Wrath
    buffs.bestial_wrath = 
      buff_creator_t( this, "bestial_wrath", o() -> specs.bestial_wrath )
        .activated( true )
        .cd( timespan_t::zero() )
        .default_value( o() -> specs.bestial_wrath -> effectN( 1 ).percent() )
        .duration( timespan_t::from_seconds( 15.0 ) );
    if ( o() -> talents.bestial_fury -> ok() )
    {
      buffs.bestial_wrath 
        -> default_value += o() -> talents.bestial_fury 
                                -> effectN( 1 )
                                  .percent();
    }
    if ( o() -> artifacts.unleash_the_beast.rank() )
    {
      buffs.bestial_wrath 
        -> default_value += o() -> artifacts.unleash_the_beast
                                  .percent();
    }
    if ( o() -> beastlord )
    {
      buffs.bestial_wrath 
        -> buff_duration *= 1.0 + ( 
                              find_spell( o() -> beastlord -> spell_id ) 
                              -> effectN( 1 )
                              .average( o() -> beastlord -> item ) 
                              / 100.0 );
    }

    // Beast Cleave
    double cleave_value = o() -> find_specialization_spell( "Beast Cleave" ) 
                              -> effectN( 1 )
                                .percent();
    if ( o() -> artifacts.furious_swipes.rank() )
    {
      cleave_value *= 1.0 + 
                      o() -> artifacts.furious_swipes
                      .percent();
    }
    buffs.beast_cleave = 
      buff_creator_t( this, 118455, "beast_cleave" )
        .activated( true )
        .default_value( cleave_value );
  }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = hunter_secondary_pet_t::composite_player_multiplier( school );

    if ( buffs.bestial_wrath -> up() )
      m *= 1.0 + buffs.bestial_wrath -> current_value;

    return m;
  }
};

// ==========================================================================
// BM T18 4P Fel Boar
// ==========================================================================
struct bm_t18_4pc_felboar: public hunter_secondary_pet_t
{  
  struct felboar_melee_t: public secondary_pet_melee_t
  {
    felboar_melee_t( bm_t18_4pc_felboar& p ):
      secondary_pet_melee_t( "felboar_melee", p )
    {
    }

    bool init_finished() override
    {
      if ( p() -> o() -> felboars[ 0 ] )
        stats = p() -> o() -> felboars[ 0 ] -> get_stats( "felboar_melee" );

      return secondary_pet_melee_t::init_finished();
    }
  };

  bm_t18_4pc_felboar( hunter_t& owner ):
    hunter_secondary_pet_t( owner, std::string( "felboar" ) )
  {
    owner_coeff.ap_from_ap = 1.38;
  }

  virtual void init_base_stats() override
  {
    hunter_secondary_pet_t::init_base_stats();

    main_hand_attack = new felboar_melee_t( *this );
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
  bool can_hunting_companion;
  double hunting_companion_multiplier;

  hunter_main_pet_action_t( const std::string& n, hunter_main_pet_t* player,
                            const spell_data_t* s = spell_data_t::nil() ):
                            ab( n, *player, s ),
                            special_ability( false )
  {
    if ( ab::data().rank_str() && !strcmp( ab::data().rank_str(), "Special Ability" ) )
      special_ability = true;
    can_hunting_companion = true;
    hunting_companion_multiplier = 1.0;
  }

  hunter_main_pet_t* p() const
  { return static_cast<hunter_main_pet_t*>( ab::player ); }

  hunter_t* o() const
  { return static_cast<hunter_t*>( p() -> o() ); }

  hunter_main_pet_td_t* td( player_t* t = nullptr ) const
  { return p() -> get_target_data( t ? t : ab::target ); }

  virtual void execute()
  {
    ab::execute();

    if ( p() -> o() -> specialization() == HUNTER_SURVIVAL && can_hunting_companion )
    {
      double proc_chance = p() -> o() -> cache.mastery_value() * hunting_companion_multiplier;

      if ( p() -> o() -> buffs.aspect_of_the_eagle -> up() )
        proc_chance *= 1.0 + p() -> o() -> specs.aspect_of_the_eagle -> effectN( 2 ).percent();

      if ( ab::rng().roll( proc_chance ) )
      {
        p() -> o() -> cooldowns.mongoose_bite -> reset( true );
        p() -> o() -> procs.hunting_companion -> occur();
      }
    }
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
    may_crit = true;
    special = true;
  }

  virtual bool ready() override
  {
    // Stampede pets don't use abilities or spells
    if ( p() -> buffs.tier17_4pc_bm -> check() || p() -> buffs.tier18_4pc_bm -> check() )
      return false;

    return base_t::ready();
  }

  virtual double composite_crit_chance() const override
  {
    double cc = base_t::composite_crit_chance();

    if ( p() -> o() -> buffs.aspect_of_the_eagle -> up() )
      cc += p() -> o() -> specs.aspect_of_the_eagle -> effectN( 1 ).percent();

    return cc;
  }
};

// Titan's Thunder ==============================================================

struct titans_thunder_t: public hunter_pet_action_t < hunter_pet_t, spell_t >
{
  struct titans_thunder_tick_t: public hunter_pet_action_t < hunter_pet_t, spell_t >
  {
    titans_thunder_tick_t( hunter_pet_t* p ):
      hunter_pet_action_t< hunter_pet_t, spell_t>( "titans_thunder_tick", *p, p -> find_spell( 207097 ) )
    {
      aoe = -1;
      background = true;
      may_crit = true;
    }
  };

  titans_thunder_t( hunter_pet_t* p ):
    hunter_pet_action_t< hunter_pet_t, spell_t >( "titans_thunder", *p, p -> find_spell( 207068 ) )
  {
    attack_power_mod.tick = p -> find_spell( 207097 ) -> effectN( 1 ).ap_coeff();
    base_tick_time = timespan_t::from_seconds( 1.0 );
    dot_duration = data().duration();
    hasted_ticks = false;
    school = SCHOOL_NATURE;
    tick_zero = false;
    tick_action = new titans_thunder_tick_t( p );
  }
};

// Jaws of Thunder ==============================================================

struct jaws_of_thunder_t: public hunter_pet_action_t < hunter_pet_t, attack_t >
{
  jaws_of_thunder_t( hunter_pet_t* p ):
    hunter_pet_action_t< hunter_pet_t, attack_t>( "jaws_of_thunder", *p, p -> find_spell( 197162 ) )
  {
    background = true;
    callbacks = false;
    may_crit = false;
    proc = true;
    school = SCHOOL_NATURE;
    weapon_multiplier = 0.0;
  }
};

// Bestial Ferocity (Aspect of the Beast) ===================================

struct bestial_ferocity_t: public hunter_main_pet_attack_t
{
  bestial_ferocity_t( hunter_main_pet_t* p ):
    hunter_main_pet_attack_t( "bestial_ferocity", p, p -> find_spell( 191413 ) )
  {
    background = true;
    can_hunting_companion = false;
  }
};

// Kill Command (pet) =======================================================

struct kill_command_t: public hunter_pet_action_t < hunter_pet_t, attack_t >
{
  jaws_of_thunder_t* jaws_of_thunder;
  kill_command_t( hunter_pet_t* p ):
    hunter_pet_action_t < hunter_pet_t, attack_t >( "kill_command", *p, p -> find_spell( 83381 ) ), jaws_of_thunder( nullptr )
  {
    background = true;
    may_crit = true;
    proc = true;
    range = 25;
    school = SCHOOL_PHYSICAL;

    // The hardcoded parameter is taken from the $damage value in teh tooltip. e.g., 1.36 below
    // $damage = ${ 1.5*($83381m1 + ($RAP*  1.632   ))*$<bmMastery> }
    attack_power_mod.direct  = 3.0; // Hard-coded in tooltip.

    if ( o() -> artifacts.jaws_of_thunder.rank() )
      jaws_of_thunder = new jaws_of_thunder_t( p );
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_pet_action_t < hunter_pet_t, attack_t >::impact( s );

    if ( rng().roll( o() -> artifacts.jaws_of_thunder.percent() ) )
    {
      jaws_of_thunder -> base_dd_min =  o() -> find_spell( 197163 ) 
                                            -> effectN( 2 )
                                            .percent() * 
                                            s -> result_amount;
      jaws_of_thunder -> base_dd_max = jaws_of_thunder -> base_dd_min;
      jaws_of_thunder -> execute();
    }
  }

  // Override behavior so that Kill Command uses hunter's attack power rather than the pet's
  double composite_attack_power() const override
  { return o() -> cache.attack_power() * o()->composite_attack_power_multiplier(); }

  bool usable_moving() const override
  { return true; }

  virtual double action_multiplier() const override
  {
    double am = hunter_pet_action_t < hunter_pet_t, attack_t >::action_multiplier();

    if ( p() -> o() -> artifacts.pack_leader.rank() )
      am *= 1.0 + p() -> o() -> artifacts.pack_leader.percent();

    return am;
  }
};

struct main_pet_kill_command_t: public kill_command_t
{
  main_pet_kill_command_t( hunter_main_pet_t* p ):
    kill_command_t( p )
  {
    if ( o() -> talents.aspect_of_the_beast -> ok() )
      impact_action = new bestial_ferocity_t( p );
  }
};

// Beast Cleave ==============================================================

struct beast_cleave_attack_t: public hunter_pet_action_t < hunter_pet_t, attack_t >
{
  beast_cleave_attack_t( hunter_pet_t* p ):
    hunter_pet_action_t < hunter_pet_t, attack_t >( "beast_cleave", *p, p -> find_spell( 118455 ) )
  {
    aoe = -1;
    background = true;
    callbacks = false;
    may_miss = false;
    may_crit = false;
    proc = false;
    radius = 10.0;
    range = -1.0;
    school = SCHOOL_PHYSICAL;
    // The starting damage includes all the buffs
    weapon_multiplier = 0;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    hunter_pet_action_t < hunter_pet_t, attack_t >::available_targets( tl );

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

  if ( p -> o() -> hati && p -> o() -> artifacts.master_of_beasts.rank() )
  {
    cleave *= 0.75; //Hotfix in game for Hati, no spelldata for it though. 2016-09-05
    p -> o() -> hati -> active.beast_cleave -> base_dd_min = cleave;
    p -> o() -> hati -> active.beast_cleave -> base_dd_max = cleave;
    p -> o() -> hati -> active.beast_cleave -> execute();
  }
}

// Pet Melee ================================================================

struct pet_melee_t: public hunter_main_pet_attack_t
{
  pet_melee_t( hunter_main_pet_t* p ):
    hunter_main_pet_attack_t( "melee", p )
  {
    weapon = &p -> main_hand_weapon;

    background = true;
    base_execute_time = weapon -> swing_time;
    repeating = true;
    school = SCHOOL_PHYSICAL;
    special = false;
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
    
    school = SCHOOL_PHYSICAL;
    trigger_gcd = timespan_t::zero();

    p() -> main_hand_attack = new pet_melee_t( player );
  }

  virtual void execute() override
  { p() -> main_hand_attack -> schedule_execute(); }

  virtual bool ready() override
  { return( p() -> main_hand_attack -> execute_event == nullptr ); } // not swinging
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
  { return o() -> cache.attack_power() * o() -> composite_attack_power_multiplier(); }

  bool use_wild_hunt() const
  {
    // comment out to avoid procs
    return p() -> resources.current[RESOURCE_FOCUS] > 50;
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_main_pet_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      trigger_beast_cleave( s );
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

// Flanking Strike (pet) ===================================================

struct flanking_strike_t: public hunter_main_pet_attack_t
{
  flanking_strike_t( hunter_main_pet_t* p ):
    hunter_main_pet_attack_t( "flanking_strike", p, p -> find_spell( 204740 ) )
  {
    attack_power_mod.direct = 2.152; //data hardcoded in tooltip
    base_multiplier = 1.62; // Hotfixed on 09/23/2016 - not in tooltip as of 11/01/2016
    background = true;
    hunting_companion_multiplier = 2.0;

    if ( p -> o() -> sets.has_set_bonus( HUNTER_SURVIVAL, T19, B2 ) )
      hunting_companion_multiplier *= p -> o() -> sets.set( HUNTER_SURVIVAL, T19, B2 ) -> effectN( 1 ).base_value();

    if ( p -> o() -> talents.aspect_of_the_beast -> ok() )
    {
      impact_action = new bestial_ferocity_t( p );
      add_child( impact_action );
    }
  }

  virtual double composite_crit_chance() const override
  {
    double cc = hunter_main_pet_attack_t::composite_crit_chance();

    if ( p() -> o() -> artifacts.my_beloved_monster.rank() )
      cc += p() -> o() -> artifacts.my_beloved_monster.percent();

    return cc;
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_main_pet_attack_t::action_multiplier();

    if ( p() -> target -> target == o() )
      am *= 1.0 + p() -> o() -> specs.flanking_strike -> effectN( 3 ).percent();

    return am;
  }  
  
  virtual double composite_attack_power() const override
  { return o() -> cache.attack_power() * o() -> composite_attack_power_multiplier(); }
};


// Dire Frenzy (pet) =======================================================

struct dire_frenzy_t: public hunter_main_pet_attack_t
{
  // This is the additional attack from Titan's Thunder
  struct titans_frenzy_t: public hunter_main_pet_attack_t
  {
    titans_frenzy_t( hunter_main_pet_t* p ):
      hunter_main_pet_attack_t( "titans_frenzy", p, p -> find_spell( 207068 ) )
    {
      attack_power_mod.direct = data().effectN( 1 ).trigger() -> effectN( 1 ).base_value();
      background = true;
      school = SCHOOL_NATURE;
    }
  };

  titans_frenzy_t* titans_frenzy;
  dire_frenzy_t( hunter_main_pet_t* p ):
    hunter_main_pet_attack_t( "dire_frenzy", p, p -> find_spell( 217207 ) )
  {
      background = true;
      weapon = &p -> main_hand_weapon;

      if ( p -> o() -> artifacts.titans_thunder.rank() )
        titans_frenzy = new titans_frenzy_t( p );

      // Not sure if this is intended, but beast master also buffs Dire Frenzy damage
      if ( p -> o() -> artifacts.beast_master.rank() )
        base_multiplier *= 1.0 + p -> o() -> artifacts.beast_master.data().effectN( 1 ).percent();
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    hunter_main_pet_attack_t::schedule_execute( state );

    if ( p() -> buffs.titans_frenzy -> up() && titans_frenzy )
      titans_frenzy -> schedule_execute();
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
    if ( p() -> buffs.tier17_4pc_bm -> check() || p() -> buffs.tier18_4pc_bm -> check() )
      return false;

    return base_t::ready();
  }
};

// Growl ===================================================================

struct growl_t : public hunter_main_pet_spell_t
{
  growl_t( hunter_main_pet_t* player, const std::string& options_str ) :
    hunter_main_pet_spell_t( "growl", player, player -> find_pet_spell( "growl" ) )
  {
    parse_options( options_str );
  }
};

// ==========================================================================
// Unique Pet Specials
// ==========================================================================

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

struct deadly_sting_t : public hunter_main_pet_spell_t
{
  deadly_sting_t( hunter_main_pet_t* player, const std::string& options_str ) :
    hunter_main_pet_spell_t( "deadly_sting", player, player -> find_pet_spell( "Deadly Sting" ) )
  {
    parse_options( options_str );
  }
};

struct bloody_screech_t : public hunter_main_pet_spell_t
{
  bloody_screech_t( hunter_main_pet_t* player, const std::string& options_str ) :
    hunter_main_pet_spell_t( "bloody_screech", player, player -> find_pet_spell( "Bloody Screech" ) )
  {
    parse_options( options_str );
  }
};

//struct gruesome_bite_t : public hunter_main_pet_spell_t
//{
//  gruesome_bite_t( hunter_main_pet_t* player, const std::string& options_str ) :
//    hunter_main_pet_spell_t( "gruesome_bite", player, player -> find_spell( 160018 ) )
//  {
//    parse_options( options_str );
//  }
//};

struct ankle_crack_t : public hunter_main_pet_spell_t
{
  ankle_crack_t( hunter_main_pet_t* player, const std::string& options_str ) :
    hunter_main_pet_spell_t( "ankle_crack", player, player -> find_pet_spell( "Ankle Crack" ) )
  {
    parse_options( options_str );
  }
};

struct web_spray_t : public hunter_main_pet_spell_t
{
  web_spray_t( hunter_main_pet_t* player, const std::string& options_str ) :
    hunter_main_pet_spell_t( "web_spray", player, player -> find_pet_spell( "Web Spray" ) )
  {
    parse_options( options_str );
  }
};

struct warp_time_t : public hunter_main_pet_spell_t
{
  warp_time_t( hunter_main_pet_t* player, const std::string& options_str ) :
    hunter_main_pet_spell_t( "warp_time", player, player -> find_pet_spell( "Warp Time" ) )
  {
    parse_options( options_str );
  }
};

struct tendon_rip_t : public hunter_main_pet_spell_t
{
  tendon_rip_t( hunter_main_pet_t* player, const std::string& options_str ) :
    hunter_main_pet_spell_t( "tendon_rip", player, player -> find_pet_spell( "Tendon Rip" ) )
  {
    parse_options( options_str );
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
  if ( name == "growl" ) return new                growl_t( this, options_str );
  if ( name == "deadly_sting" ) return new         deadly_sting_t( this, options_str );
  if ( name == "bloody_screech" ) return new       bloody_screech_t( this, options_str );
  //if ( name == "gruesome_bite" ) return new        gruesome_bite_t( this, options_str );
  if ( name == "ankle_crack" ) return new          ankle_crack_t( this, options_str );
  if ( name == "web_spray" ) return new            web_spray_t( this, options_str );
  if ( name == "warp_time" ) return new            warp_time_t( this, options_str );
  if ( name == "tendon_rip" ) return new           tendon_rip_t( this, options_str );
  return base_t::create_action( name, options_str );
}

// hunter_t::init_spells ====================================================

void hunter_main_pet_t::init_spells()
{
  base_t::init_spells();

  // ferocity
  active.kill_command = new actions::main_pet_kill_command_t( this );
  specs.heart_of_the_phoenix = spell_data_t::not_found();
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
  specs.spiked_collar = find_specialization_spell( "Spiked Collar" );
  specs.wild_hunt = find_spell( 62762 );
  specs.combat_experience = find_specialization_spell( "Combat Experience" );

  if ( o() -> specs.beast_cleave -> ok() )
    active.beast_cleave = new actions::beast_cleave_attack_t( this );

  if ( o() -> talents.dire_frenzy -> ok() )
    active.dire_frenzy = new actions::dire_frenzy_t( this );

  if ( o() -> artifacts.titans_thunder.rank() )
    active.titans_thunder = new actions::titans_thunder_t( this );

  if ( o() -> specialization() == HUNTER_SURVIVAL )
    active.flanking_strike = new actions::flanking_strike_t( this );
}

void dire_critter_t::init_spells()
{
  hunter_secondary_pet_t::init_spells();

  if ( o() -> talents.stomp -> ok() )
    active.stomp = new dire_beast_stomp_t( *this );

  if ( o() -> artifacts.titans_thunder.rank() )
    active.titans_thunder = new actions::titans_thunder_t( this );
}

void hati_t::init_spells() 
{
  hunter_secondary_pet_t::init_spells();

  if ( o() -> artifacts.master_of_beasts.rank() )
  {
    active.kill_command = new actions::kill_command_t( this );
    active.kill_command -> base_multiplier *= 0.5;
    active.beast_cleave = new actions::beast_cleave_attack_t( this );
  }

  if ( o() -> artifacts.titans_thunder.rank() )
    active.titans_thunder = new actions::titans_thunder_t( this );

  if ( o() -> artifacts.jaws_of_thunder.rank() )
    active.jaws_of_thunder = new actions::jaws_of_thunder_t( this );
}

} // end namespace pets

namespace attacks
{

// ==========================================================================
// Hunter Attacks
// ==========================================================================

struct hunter_ranged_attack_t: public hunter_action_t < ranged_attack_t >
{
  bool may_proc_mm_feet;
  hunter_ranged_attack_t( const std::string& n, hunter_t* player,
                          const spell_data_t* s = spell_data_t::nil() ):
                          base_t( n, player, s ),
                          may_proc_mm_feet( false )
  {
    if ( player -> main_hand_weapon.type == WEAPON_NONE )
      background = true;

    may_block = false;
    may_crit = true;
    may_parry = false;
    special = true;
    tick_may_crit = true;
  }

  virtual void init() override
  {
    if ( player -> main_hand_weapon.group() != WEAPON_RANGED )
      background = true;

    base_t::init();
  }

  virtual bool usable_moving() const override
  { return true; }

  virtual void execute() override
  {
    ranged_attack_t::execute();
    try_steady_focus();

    // TODO: Verify if only integer chunks of 20 focus reduce the CD, or if there's rollover
    if ( p() -> sets.has_set_bonus( HUNTER_MARKSMANSHIP, T19, B2 ) )
    {
      p() -> cooldowns.trueshot 
        -> adjust( timespan_t::from_seconds( -1.0 * 
                                             cost() / 
                                               p() -> sets.set( HUNTER_MARKSMANSHIP, T19, B2 ) 
                                            -> effectN( 1 )
                                              .base_value() ) );
    }

    if ( !background && p() -> legendary.mm_feet && may_proc_mm_feet )
    {
      p() -> cooldowns.trueshot 
        -> adjust( timespan_t::from_millis( p() -> legendary.mm_feet 
                                                -> driver() 
                                                -> effectN( 1 )
                                                  .base_value() ) );
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( p() -> artifacts.bullseye.rank() && s -> target -> health_percentage() <= p() -> artifacts.bullseye.value() )
      p() -> buffs.bullseye -> trigger();
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
      if ( p() -> buffs.pre_steady_focus -> stack() < 3 )
        return;
    }

    // either three required shots have happened or we don't require them
    double regen_buff = p() -> buffs.steady_focus -> data().effectN( 1 ).percent();
    p() -> buffs.steady_focus -> trigger( 1, regen_buff );
    p() -> buffs.pre_steady_focus -> expire();
  }

  void trigger_piercing_shots( action_state_t* s )
  {
    double amount = p() -> talents.careful_aim 
                        -> effectN( 3 )
                          .percent() * 
                      s -> result_amount;

    residual_action::trigger( p() -> active.piercing_shots, s -> target, amount );
  }
};

struct hunter_melee_attack_t: public hunter_action_t < melee_attack_t >
{
  hunter_melee_attack_t( const std::string& n, hunter_t* p,
                          const spell_data_t* s = spell_data_t::nil() ):
                          base_t( n, p, s )
  {
    if ( p -> main_hand_weapon.type == WEAPON_NONE )
      background = true;

    weapon = &p -> main_hand_weapon;
    may_block = false;
    may_crit = true;
    may_parry = false;
    special = true;
    tick_may_crit = true;
  }

  virtual double composite_crit_chance() const override
  {
    double cc = base_t::composite_crit_chance();

    if ( p() -> buffs.aspect_of_the_eagle -> up() )
      cc += p() -> specs.aspect_of_the_eagle -> effectN( 1 ).percent();

    return cc;
  }
};

// Ranged Attack ============================================================

struct ranged_t: public hunter_ranged_attack_t
{
  bool first_shot;
  ranged_t( hunter_t* p, const char* n = "ranged", const spell_data_t* s = spell_data_t::nil() ):
    hunter_ranged_attack_t( n, p, s ), first_shot( true )
  {
    school = SCHOOL_PHYSICAL;
    weapon = &p -> main_hand_weapon;
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
};

// Volley ============================================================================

struct volley_tick_t: hunter_ranged_attack_t
{
  volley_tick_t( hunter_t* p ):
    hunter_ranged_attack_t( "volley_tick", p, p -> find_spell( 194392 ) )
  {
    aoe = -1;
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
    travel_speed = 0.0;
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    if ( p() -> mastery.sniper_training -> ok() )
      am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();

    return am;
  }
};

struct volley_t: hunter_ranged_attack_t
{
  std::string onoff;
  bool onoffbool;
  volley_t( hunter_t* p, const std::string&  options_str  ):
    hunter_ranged_attack_t( "volley", p, p -> talents.volley )
  {
    harmful = false;
    add_option( opt_string( "toggle", onoff ) );
    parse_options( options_str );

    if ( onoff == "on" )
    {
      onoffbool = true;
    }
    else if ( onoff == "off" )
    {
      onoffbool = false;
    }
    else
    {
      sim -> errorf( "Volley must use the option 'toggle=on' or 'toggle=off'" );
      background = true;
    }
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( onoffbool )
      p() -> buffs.volley -> trigger();
    else
      p() -> buffs.volley -> expire();
  }

  bool ready() override
  {
    if ( onoffbool && p() -> buffs.volley -> check() )
    {
      return false;
    }
    else if ( !onoffbool && !p() -> buffs.volley -> check() )
    {
      return false;
    }
    return hunter_ranged_attack_t::ready();
  }
};

// Auto Shot ================================================================

struct auto_shot_t: public ranged_t
{
  volley_tick_t* volley_tick;
  double volley_tick_cost;
  auto_shot_t( hunter_t* p ): ranged_t( p, "auto_shot", spell_data_t::nil() ), volley_tick( nullptr ),
    volley_tick_cost( 0 )
  {
    school = SCHOOL_PHYSICAL;
    range = 40.0;

    if ( p -> talents.volley -> ok() )
    {
      volley_tick = new volley_tick_t( p );
      add_child( volley_tick );
      volley_tick_cost = p -> talents.volley -> effectN( 1 ).resource(RESOURCE_FOCUS);
    }
  }

  virtual void execute() override
  {
    ranged_t::execute();

    if ( p() -> specialization() == HUNTER_MARKSMANSHIP && p() -> ppm_hunters_mark -> trigger() )
    {
      if ( p() -> buffs.marking_targets -> up() )
        p() -> procs.wasted_marking_targets -> occur();

      p() -> buffs.marking_targets -> trigger();
      p() -> procs.marking_targets -> occur();
    }

    if ( p() -> buffs.volley -> up() )
    {
      if ( p() -> resources.current[RESOURCE_FOCUS] > volley_tick_cost )
      {
        volley_tick -> target = execute_state -> target;
        volley_tick -> execute();
      }
      else
      {
        p() -> buffs.volley -> expire();
      }
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    ranged_t::impact( s );

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
        p() -> cooldowns.dire_frenzy -> reset( true );
        p() -> cooldowns.dire_beast -> reset( true );
        p() -> procs.wild_call -> occur();
      }
    }
  }

  virtual double composite_target_crit_chance( player_t* t ) const override
  {
    double cc= ranged_t::composite_target_crit_chance( t );

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


//==============================
// Shared attacks
//==============================

// Barrage ==================================================================

struct barrage_t: public hunter_ranged_attack_t
{
  struct barrage_damage_t: public attacks::hunter_ranged_attack_t
  {
    barrage_damage_t( hunter_t* player ):
      attacks::hunter_ranged_attack_t( "barrage_primary", player, player -> talents.barrage -> effectN( 2 ).trigger() )
    {
      background = true;
      may_crit = true;
      weapon = &( player -> main_hand_weapon );
      aoe = -1;
      base_aoe_multiplier = 0.5;
      range = radius;
      range = 0;
      travel_speed = 0.0;
    }
  };

  barrage_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "barrage", player, player -> talents.barrage )
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
    hunter_ranged_attack_t::schedule_execute( state );

    // Delay auto shot, add 500ms to simulate "wind up"
    if ( p() -> main_hand_attack && p() -> main_hand_attack -> execute_event )
      p() -> main_hand_attack -> reschedule_execute( dot_duration * composite_haste() + timespan_t::from_millis( 500 ) );

    p() -> no_steady_focus();
  }

  bool usable_moving() const override
  { return true; }

  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    if ( p() -> mastery.sniper_training -> ok() )
      am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();

    return am;
  }
};

// Multi Shot Attack =================================================================

struct multi_shot_t: public hunter_ranged_attack_t
{
  multi_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "multi_shot", p, p -> find_class_spell( "Multi-Shot" ) )
  {
    parse_options( options_str );
    may_proc_mm_feet = true;
    aoe = -1;

    if ( p -> artifacts.called_shot.rank() )
      base_multiplier *= 1.0 + p -> artifacts.called_shot.percent();

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
    return p() -> specialization() == HUNTER_MARKSMANSHIP ? 0 : hunter_ranged_attack_t::cost();
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();
    if ( p() -> buffs.bombardment -> up() )
      am *= 1.0 + p() -> buffs.bombardment -> data().effectN( 2 ).percent();
    if ( p() -> artifacts.focus_of_the_titans.rank() )
      am *= 1.0 + p() -> artifacts.focus_of_the_titans.percent();
    return am;
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();

    pets::hunter_main_pet_t* pet = p() -> active.pet;
    if ( pet && p() -> specs.beast_cleave -> ok() )
    {
      pet -> buffs.beast_cleave -> trigger();
      if ( p() -> artifacts.master_of_beasts.rank() )
        p() -> hati -> buffs.beast_cleave -> trigger();
    }
    if ( result_is_hit( execute_state -> result ) )
    {
      // Hunter's Mark applies on cast to all affected targets or none based on RPPM (8*haste).
      // This loop goes through the target list for multi-shot and applies the debuffs on proc.
      // Multi-shot also grants 2 focus per target hit on cast.
      if ( p() -> specialization() == HUNTER_MARKSMANSHIP )
      {
        if ( p() -> buffs.trueshot -> up() || p() -> buffs.marking_targets -> up() )
        {
          std::vector<player_t*> multi_shot_targets = execute_state -> action -> target_list();
          for ( size_t i = 0; i < multi_shot_targets.size(); i++ )
            td( multi_shot_targets[i] ) -> debuffs.hunters_mark -> trigger();

          p() -> buffs.hunters_mark_exists -> trigger();
          p() -> buffs.marking_targets -> expire();
        }
      }
    }

    if ( p() -> artifacts.surge_of_the_stormgod.rank() && rng().roll( p() -> artifacts.surge_of_the_stormgod.data().proc_chance() ) )
    {
      if ( p() -> active.pet )
        p() -> active.surge_of_the_stormgod -> execute();
      if ( p() -> hati )
        p() -> active.surge_of_the_stormgod -> execute();
    }

    if ( p() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T18, B2 ) )
      p() -> buffs.t18_2p_dire_longevity -> trigger();
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( p() -> specialization() == HUNTER_MARKSMANSHIP && result_is_hit( s -> result ) )
    {
      if ( s -> result == RESULT_CRIT )
      {
        p() -> buffs.bombardment -> trigger();

        if ( p() -> buffs.careful_aim -> check() )
          trigger_piercing_shots( s );
      }
    }

    if ( p() -> legendary.mm_waist )
      p() -> buffs.sentinels_sight -> trigger();
  }

  virtual bool ready() override
  {
    if ( p() -> talents.sidewinders -> ok() )
      return false;

    return hunter_ranged_attack_t::ready();
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
    cooldown -> hasted = true;
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

    if ( p() -> talents.killer_cobra -> ok() && p() -> buffs.bestial_wrath -> up() )
      p() -> cooldowns.kill_command -> reset( true );

    if ( p() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T18, B2 ) )
      p() -> buffs.t18_2p_dire_longevity -> trigger();
  }

  virtual double composite_target_crit_chance( player_t* t ) const override
  {
    double cc = hunter_ranged_attack_t::composite_target_crit_chance( t );

    cc += p() -> buffs.big_game_hunter -> value();

    return cc;
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    if ( p() -> talents.way_of_the_cobra -> ok() )
    {
      int active_pets = 1;

      if ( p() -> hati )
        active_pets++;
      for ( size_t i = 0; i < p() -> pet_dire_beasts.size(); i++ )
      {
        if ( !p() -> pet_dire_beasts[ i ] -> is_sleeping() )
          active_pets++;
      }

      am *= 1.0 + active_pets * p() -> talents.way_of_the_cobra -> effectN( 1 ).percent();
    }

    if ( p() -> artifacts.spitting_cobras.rank() )
      am *= 1.0 + p() -> artifacts.spitting_cobras.percent();

    return am;
  }
};

// Surge of the Stormgod =====================================================

struct surge_of_the_stormgod_t: public hunter_ranged_attack_t
{
  surge_of_the_stormgod_t( hunter_t* p ):
    hunter_ranged_attack_t( "surge_of_the_stormgod", p, p -> artifacts.surge_of_the_stormgod )
  {
    aoe = -1;
    attack_power_mod.direct = 2.0;
    may_crit = true;
    school = SCHOOL_NATURE;
  }
};

//==============================
// Marksmanship attacks
//==============================

// Black Arrow ==============================================================

struct black_arrow_t: public hunter_ranged_attack_t
{
  timespan_t duration;
  black_arrow_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "black_arrow", player, player -> find_talent_spell( "Black Arrow" ) )
  {
    parse_options( options_str );
    cooldown -> hasted = true;
    tick_may_crit = true;
    hasted_ticks = false;
    duration = this -> dot_duration;
  }

  virtual void execute() override
  {
    p() -> no_steady_focus();
    hunter_ranged_attack_t::execute();

    if ( p() -> dark_minion[ 0 ] -> is_sleeping() )
      p() -> dark_minion[ 0 ] -> summon( duration );
    else
      p() -> dark_minion[ 1 ] -> summon( duration );
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();

    return am;
  }
};

// Bursting Shot ======================================================================

struct bursting_shot_t : public hunter_ranged_attack_t
{
  bursting_shot_t( hunter_t* player, const std::string& options_str ) :
    hunter_ranged_attack_t( "bursting_shot", player, player -> find_spell( 186387 ) )
  {
    parse_options( options_str );
  }


  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();
    am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();
    if ( p() -> legendary.magnetized_blasting_cap_launcher )
    {
      am *= 1.0 + p() -> legendary.magnetized_blasting_cap_launcher -> driver() -> effectN( 2 ).percent();
    }
    return am;
  }
};
// Aimed Shot base class ==============================================================

struct aimed_shot_base_t: public hunter_ranged_attack_t
{
  aimed_shot_base_t( const std::string& name, hunter_t* p, const spell_data_t* s):
    hunter_ranged_attack_t( name, p, s )
  {
    background = true;
    parse_spell_data( *p -> specs.aimed_shot );

    if ( p -> artifacts.wind_arrows.rank() )
      base_multiplier *= 1.0 +  p -> artifacts.wind_arrows.percent();

    if ( p -> artifacts.deadly_aim.rank() ) 
      crit_bonus_multiplier *= 1.0 + p -> artifacts.deadly_aim.percent();
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    if ( p() -> mastery.sniper_training -> ok() )
      am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();

    if ( p() -> buffs.trick_shot -> up() )
      am *= 1.0 + p() -> buffs.trick_shot -> default_value;

    if ( p() -> buffs.sentinels_sight -> up() )
      am *= 1.0 + p() -> buffs.sentinels_sight -> check_stack_value();

    return am;
  }

  virtual double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_ranged_attack_t::composite_target_da_multiplier( t );

    hunter_td_t* td = this -> td( t );

    if ( td -> debuffs.vulnerable -> up() )
      m *= 1.0 + td -> debuffs.vulnerable -> check_stack_value();

    if ( td -> debuffs.true_aim -> up() )
      m *= 1.0 + td -> debuffs.true_aim -> check_stack_value();

    return m;
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    trigger_true_aim( p(), s -> target );
  }

  virtual double composite_target_crit_chance( player_t* t ) const override
  {
    double cc = hunter_ranged_attack_t::composite_target_crit_chance( t );

    cc += p() -> buffs.careful_aim -> value();

    if ( td( t ) -> debuffs.vulnerable -> up() )
      cc += td( t ) -> debuffs.vulnerable -> stack() * p() -> artifacts.marked_for_death.percent();

    return cc;
  }
};

// Trick Shot =========================================================================

struct trick_shot_t: public aimed_shot_base_t
{
  trick_shot_t( hunter_t* p ):
    aimed_shot_base_t( "trick_shot", p, p -> find_talent_spell( "Trick Shot" ) )
  {
    // Simulated as aoe for simplicity
    aoe               = -1;
    background        = true;
    base_costs[ RESOURCE_FOCUS ] = 0;
    dual              = true;
    weapon_multiplier *= p -> find_talent_spell( "Trick Shot" ) -> effectN( 1 ).percent();
  }

  virtual void execute() override
  {
    aimed_shot_base_t::execute();

    int count = 0;
    std::vector<player_t*> trick_shot_targets = execute_state -> action -> target_list();
    for( size_t i = 0; i < trick_shot_targets.size(); i++ )
    {
      if ( trick_shot_targets[ i ] != p() -> target && td( trick_shot_targets[ i ] ) -> debuffs.vulnerable -> up() )
        count++;
    }
    if ( count == 0 )
      p() -> buffs.trick_shot -> trigger();
    else
      p() -> buffs.trick_shot -> expire();
  }

  virtual void impact( action_state_t* s ) override
  {
    // Do not hit current target, and only deal damage to targets with Vulnerable
    if ( s -> target != p() -> target && ( td( s -> target ) -> debuffs.vulnerable -> up() ) )
    {
      aimed_shot_base_t::impact( s );

      if ( p() -> buffs.careful_aim -> check() && s -> result == RESULT_CRIT )
        trigger_piercing_shots( s );
    }
  }
};

// Legacy of the Windrunners ==========================================================

struct legacy_of_the_windrunners_t: aimed_shot_base_t
{
  legacy_of_the_windrunners_t( hunter_t* p ):
    aimed_shot_base_t( "legacy_of_the_windrunners", p, p -> artifacts.legacy_of_the_windrunners )
  {
    background = true;
    dual = true;
    proc = true;
    weapon_multiplier = p -> find_spell( 191043 ) -> effectN( 2 ).percent();
  }
};

// Aimed Shot =========================================================================

struct aimed_shot_t: public aimed_shot_base_t
{
  benefit_t* aimed_in_ca;
  trick_shot_t* trick_shot;
  legacy_of_the_windrunners_t* legacy_of_the_windrunners;
  aimed_shot_t( hunter_t* p, const std::string& options_str ):
    aimed_shot_base_t( "aimed_shot", p, p -> find_specialization_spell( "Aimed Shot" ) ),
    aimed_in_ca( p -> get_benefit( "aimed_in_careful_aim" ) ),
    trick_shot( nullptr ), legacy_of_the_windrunners( nullptr )
  {
    parse_options( options_str );

    background = false;
    may_proc_mm_feet = true;
    if ( p -> sets.has_set_bonus( HUNTER_MARKSMANSHIP, T18, B4 ) )
      base_execute_time *= 1.0 - ( p -> sets.set( HUNTER_MARKSMANSHIP, T18, B4 ) -> effectN( 2 ).percent() );

    if ( p -> talents.trick_shot -> ok() )
    {
      trick_shot = new trick_shot_t( p );
      add_child( trick_shot );
    }

    if ( p -> artifacts.legacy_of_the_windrunners.rank() )
    {
      legacy_of_the_windrunners = new legacy_of_the_windrunners_t( p );
      add_child( legacy_of_the_windrunners );
    }
  }

  virtual double cost() const override
  {
    double cost = aimed_shot_base_t::cost();

    if ( p() -> buffs.lock_and_load -> check() )
      return 0;

    return cost;
  }

  virtual void impact( action_state_t* s ) override
  {
    aimed_shot_base_t::impact( s );

    if ( p() -> buffs.careful_aim -> value() && s -> result == RESULT_CRIT )
      trigger_piercing_shots( s );

    if ( !td( s -> target ) -> debuffs.vulnerable -> check() )
      p() -> procs.no_vuln_aimed_shot -> occur();
  }

  virtual void execute() override
  {
    p() -> no_steady_focus();

    aimed_shot_base_t::execute();
    
    if ( trick_shot )
      trick_shot -> execute();

    aimed_in_ca -> update( p() -> buffs.careful_aim -> check() != 0 );

    if ( p() -> sets.has_set_bonus( HUNTER_MARKSMANSHIP, PVP, B4 ) )
      p() -> cooldowns.trueshot -> adjust( -p() -> sets.set( HUNTER_MARKSMANSHIP, PVP, B4 ) -> effectN( 1 ).time_value() );

    if ( p() -> buffs.lock_and_load -> up() )
      p() -> buffs.lock_and_load -> decrement();

    if ( legacy_of_the_windrunners && rng().roll( p() -> artifacts.legacy_of_the_windrunners.data().proc_chance() ) )
    {
      legacy_of_the_windrunners -> schedule_execute();
      legacy_of_the_windrunners -> schedule_execute();
      legacy_of_the_windrunners -> schedule_execute();
      legacy_of_the_windrunners -> schedule_execute();
      legacy_of_the_windrunners -> schedule_execute();
      legacy_of_the_windrunners -> schedule_execute();
    }

    if ( p() -> buffs.sentinels_sight -> up() )
      p() -> buffs.sentinels_sight -> expire();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = aimed_shot_base_t::execute_time();

    if ( p() -> buffs.lock_and_load -> up() )
      t = timespan_t::zero();

    return t;
  }

  virtual bool usable_moving() const override
  { return false; }
};

// Arcane Shot Attack ================================================================

struct arcane_shot_t: public hunter_ranged_attack_t
{
  double focus_gain;
  arcane_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "arcane_shot", p, p -> find_specialization_spell( "Arcane Shot" ) )
  {
    parse_options( options_str );
    may_proc_mm_feet = true;
    focus_gain = p -> find_spell( 187675 ) -> effectN( 1 ).base_value();
  }

  void try_steady_focus() override
  {
    trigger_steady_focus( true );
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> buffs.trueshot -> up() || p() -> buffs.marking_targets -> up() )
      {
        td( execute_state -> target ) -> debuffs.hunters_mark -> trigger();
        p() -> buffs.hunters_mark_exists -> trigger();
        p() -> buffs.marking_targets -> expire();
      }

      double focus_multiplier = 1.0;
      if ( p() -> artifacts.critical_focus.rank() && execute_state -> result == RESULT_CRIT )
        focus_multiplier *= 1.0 + p() -> artifacts.critical_focus.percent();
      p() -> resource_gain( RESOURCE_FOCUS, focus_multiplier * focus_gain, gain );
    }
  }


  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    trigger_true_aim( p(), s -> target );
  }

  virtual double composite_target_crit_chance( player_t* t ) const override
  {
    double cc = hunter_ranged_attack_t::composite_target_crit_chance( t );

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
    if ( p() -> talents.sidewinders->ok() )
      return false;

    return hunter_ranged_attack_t::ready();
  }
};

// Marked Shot Attack =================================================================

struct marked_shot_t: public hunter_ranged_attack_t
{
  struct call_of_the_hunter_t: public hunter_ranged_attack_t
  {
    call_of_the_hunter_t( hunter_t* p ):
      hunter_ranged_attack_t( "call_of_the_hunter", p, p -> find_spell( 191070 ) )
    {
      aoe = -1;
      background = true;
    }
  };

  call_of_the_hunter_t* call_of_the_hunter;
  marked_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "marked_shot", p, p -> find_specialization_spell( "Marked Shot" ) ), call_of_the_hunter( nullptr )
  {
    parse_options( options_str );
    may_proc_mm_feet = true;
    // Simulated as AOE for simplicity.
    aoe               = -1;
    travel_speed      = 0.0;
    weapon            = &p -> main_hand_weapon;
    weapon_multiplier = p -> find_spell( 212621 ) -> effectN( 2 ).percent();
    normalize_weapon_speed = true;

    if ( p -> artifacts.windrunners_guidance.rank() )
      base_multiplier *= 1.0 + p -> artifacts.windrunners_guidance.percent();

    if ( p -> artifacts.call_of_the_hunter.rank() )
    {
      call_of_the_hunter = new call_of_the_hunter_t( p );
      add_child( call_of_the_hunter );
    }
  }

  virtual void execute() override
  {
    if ( p() -> legendary.mm_ring && rng().roll( p() -> legendary.mm_ring -> driver() -> effectN( 1 ).percent() ) )
    {
      p() -> clear_next_hunters_mark = false;
      p() -> procs.zevrims_hunger -> occur();
    }
    else
      p() -> clear_next_hunters_mark = true;

    p() -> no_steady_focus();
    hunter_ranged_attack_t::execute();

    // Consume Hunter's Mark and apply appropriate debuffs. Vulnerable applies on cast.
    bool no_vuln_check = true;
    std::vector<player_t*> marked_shot_targets = execute_state -> action -> target_list();
    for ( size_t i = 0; i < marked_shot_targets.size(); i++ )
    {
      if ( !td( marked_shot_targets[i] ) -> debuffs.vulnerable -> check() && no_vuln_check )
      {
        p() -> procs.no_vuln_marked_shot -> occur();
        no_vuln_check = false; // Don't show multiple procs for one cast
      }

      if ( td( marked_shot_targets[i] ) -> debuffs.hunters_mark -> up() )
        td( marked_shot_targets[i] ) -> debuffs.vulnerable -> trigger();

      if ( p() -> clear_next_hunters_mark )
        td( marked_shot_targets[i] ) -> debuffs.hunters_mark -> expire();
    }

    if ( p() -> clear_next_hunters_mark )
      p() -> buffs.hunters_mark_exists -> expire();

    if ( p() -> artifacts.call_of_the_hunter.rank() && p() -> ppm_call_of_the_hunter -> trigger() )
    {
      call_of_the_hunter -> schedule_execute();
      call_of_the_hunter -> schedule_execute();
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( s -> result == RESULT_CRIT )
    {
      if ( p() -> buffs.careful_aim -> check() )
        trigger_piercing_shots( s );

      if ( p() -> sets.has_set_bonus( HUNTER_MARKSMANSHIP, T18, B2 ) )
        p() -> buffs.t18_2p_rapid_fire -> trigger();
    }
  }

  // Only schedule the attack if a valid target
  virtual void schedule_travel( action_state_t* s ) override
  {
    if ( td( s -> target ) -> debuffs.hunters_mark -> up() )
    {
      hunter_ranged_attack_t::schedule_travel( s );
    }
    // If its not a valid target, the state needs to be released
    else
    {
      action_state_t::release( s );
    }
  }

  // Marked Shot can only be used if a Hunter's Mark exists on any target.
  virtual bool ready() override
  {
    if ( p() -> buffs.hunters_mark_exists -> up() )
      return hunter_ranged_attack_t::ready();

    return false;
  }

  virtual double composite_crit_chance() const override
  {
    double cc = hunter_ranged_attack_t::composite_crit_chance();

    if ( p() -> artifacts.precision.rank() )
      cc += p() -> artifacts.precision.percent();

    return cc;
  }

  virtual double composite_target_crit_chance( player_t* t ) const override
  {
    double cc = hunter_ranged_attack_t::composite_target_crit_chance( t );

    cc += p() -> buffs.careful_aim -> value();

    return cc;
  }

  virtual double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_ranged_attack_t::composite_target_da_multiplier( t );

    hunter_td_t* td = this -> td( t );
    if ( td -> debuffs.vulnerable -> up() )
      m *= 1.0 + td -> debuffs.vulnerable -> check_stack_value();

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

// Piercing Shot  =========================================================================

struct piercing_shot_t: public hunter_ranged_attack_t
{
  piercing_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "piercing_shot", p, p -> talents.piercing_shot )
  {
    parse_options( options_str );

    aoe = -1;

    // Spell data is currently bugged on alpha
    base_multiplier = 2.0;
    base_aoe_multiplier = 0.5;
  }

  virtual void execute() override
  {
    p() -> no_steady_focus();
    hunter_ranged_attack_t::execute();
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

struct explosive_shot_t: public hunter_ranged_attack_t
{
  player_t* initial_target;
  explosive_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "explosive_shot", p, p -> find_talent_spell( "Explosive Shot" ) ), initial_target( nullptr )
  {
    parse_options( options_str );

    aoe = -1;
    attack_power_mod.direct = p -> find_spell( 212680 ) -> effectN( 1 ).ap_coeff();
    weapon = &p -> main_hand_weapon;
    weapon_multiplier = 0.0;

    travel_speed = 20.0;
  }

  virtual void execute() override
  {
    p() -> no_steady_focus();
    initial_target = p() -> target;

    hunter_ranged_attack_t::execute();
  }

  virtual double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_ranged_attack_t::composite_target_da_multiplier( t );

    if ( sim -> distance_targeting_enabled )
      m /= 1 + ( initial_target -> get_position_distance( t -> x_position, t -> y_position ) ) / radius;

    return m;
  }
};

// Sidewinders =======================================================================

struct sidewinders_t: hunter_ranged_attack_t
{
  sidewinders_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "sidewinders", p, p -> talents.sidewinders )
  {
    parse_options( options_str );

    aoe                       = -1;
    attack_power_mod.direct   = p -> find_spell( 214581 ) -> effectN( 1 ).ap_coeff();
    cooldown -> hasted        = true;

    if ( p -> artifacts.critical_focus.rank() )
      energize_amount += p -> find_spell( 191328 ) -> effectN( 2 ).base_value();

    if ( p -> artifacts.called_shot.rank() )
      base_multiplier *= 1.0 + p -> artifacts.called_shot.percent();
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      bool marking = false;

      if ( p() -> buffs.trueshot -> up() || p() -> buffs.marking_targets -> up() )
        marking = true;

      std::vector<player_t*> sidewinder_targets = execute_state -> action -> target_list();
      for ( size_t i = 0; i < sidewinder_targets.size(); i++ )
      {
        if ( marking )
          td( sidewinder_targets[i] ) -> debuffs.hunters_mark -> trigger();

        td( sidewinder_targets[i] ) -> debuffs.vulnerable -> trigger();
      }

      if ( marking )
      {
        p() -> buffs.hunters_mark_exists -> trigger();
        p() -> buffs.marking_targets -> expire();
      }

      trigger_steady_focus( false );
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( p() -> legendary.mm_waist )
      p() -> buffs.sentinels_sight -> trigger();
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

  virtual void execute() override
  {
    p() -> no_steady_focus();
    hunter_ranged_attack_t::execute();

    if ( p() -> artifacts.mark_of_the_windrunner.rank() )
    {
      hunter_td_t* td = this -> td( execute_state -> target );
      td -> debuffs.vulnerable -> trigger();
    }
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();

    am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();

    return am;
  }

  virtual bool usable_moving() const override
  { return false; }
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

// Melee attack ==============================================================

// Talon Strike =============================================================

struct talon_strike_t: public hunter_melee_attack_t
{
  talon_strike_t( hunter_t* p ):
    hunter_melee_attack_t( "talon_strike", p, p -> find_spell( 203525 ) )
  {
    background = true;
    weapon_multiplier = 0.0;
  }
};

struct melee_t: public hunter_melee_attack_t
{
  bool first;
  talon_strike_t* talon_strike;
  melee_t( hunter_t* player, const std::string &name = "auto_attack_mh", const spell_data_t* s = spell_data_t::nil() ):
    hunter_melee_attack_t( name, player, s ), first( true ), talon_strike( nullptr )
  {
    school             = SCHOOL_PHYSICAL;
    base_execute_time  = player -> main_hand_weapon.swing_time;
    weapon = &( player -> main_hand_weapon );
    background         = true;
    repeating          = true;
    may_glance         = true;
    special            = false;
    trigger_gcd        = timespan_t::zero();

    if ( p() -> artifacts.talon_strike.rank() )
    {
      talon_strike = new talon_strike_t( player );
      add_child( talon_strike );
    }
  }

  timespan_t execute_time() const override
  {
  if ( ! player -> in_combat )
    return timespan_t::from_seconds( 0.01 );
    if ( first )
      return timespan_t::zero();
    else
      return hunter_melee_attack_t::execute_time();
  }

  void execute() override
  {
    if ( first )
      first = false;
    hunter_melee_attack_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    hunter_melee_attack_t::impact( s );

    if ( td( s -> target ) -> dots.on_the_trail -> is_ticking() )
    {
      // TODO: Wait for spell data to reflect actual values
      // Max extended duration of 36s
      timespan_t extension = std::min( timespan_t::from_seconds( 6.0 ), 
                                       timespan_t::from_seconds( 36.0 ) - td( s -> target ) -> dots.on_the_trail -> remains() );
      td( s -> target ) -> dots.on_the_trail -> extend_duration( extension );
    }

    if ( talon_strike && rng().roll( 0.10 ) ) // TODO: Update with spell data when possible
    {
      talon_strike -> schedule_execute();
      talon_strike -> schedule_execute();
    }
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
    ignore_false_positive = true;
    range = 5;
    trigger_gcd = timespan_t::zero();
  }

  void execute() override
  {
    player -> main_hand_attack -> schedule_execute();
  }

  bool ready() override
  {
    if ( player -> is_moving() )
      return false;

    return ( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// Mongoose Bite =======================================================================

struct mongoose_bite_t: hunter_melee_attack_t
{
  mongoose_bite_t( hunter_t* p, const std::string& options_str ):
    hunter_melee_attack_t( "mongoose_bite", p, p -> specs.mongoose_bite )
  {
    parse_options( options_str );
    cooldown -> hasted = true;
  }

  virtual void execute() override
  {
    hunter_melee_attack_t::execute();

    if ( p() -> sets.has_set_bonus( HUNTER_SURVIVAL, T19, B4 ) && p() -> buffs.mongoose_fury -> stack() == 5 )
      p() -> buffs.t19_4p_mongoose_power -> trigger();

    p() -> buffs.mongoose_fury -> trigger();

    if ( p() -> sets.has_set_bonus( HUNTER_SURVIVAL, T18, B4 ) && rng().roll( p() -> sets.set( HUNTER_SURVIVAL, T18, B4 ) -> proc_chance() ) )
    {
      p() -> cooldowns.lacerate -> reset( true );
      p() -> procs.t18_4pc_sv -> occur();
    }
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_melee_attack_t::action_multiplier();

    if ( p() -> buffs.mongoose_fury -> up() )
      am *= 1.0 + p() -> buffs.mongoose_fury -> check_stack_value();

    if ( p() -> artifacts.sharpened_fang.rank() )
      am *= 1.0 + p() -> artifacts.sharpened_fang.percent();

    if ( p() -> blackness )
      am *= 1.0 + p() -> blackness_multiplier;

    return am;
  }
};

// Flanking Strike =====================================================================

struct flanking_strike_t: hunter_melee_attack_t
{
  flanking_strike_t( hunter_t* p, const std::string& options_str ):
    hunter_melee_attack_t( "flanking_strike", p, p -> specs.flanking_strike )
  {
    parse_options( options_str );
    cooldown -> hasted = true;
  }

  virtual void execute() override
  {
    hunter_melee_attack_t::execute();

    timespan_t animal_instincts = -p() -> find_spell( 232646 ) -> effectN( 1 ).time_value() * 1000 * p() -> cache.spell_haste();

    if ( p() -> active.pet )
      p() -> active.pet -> active.flanking_strike -> execute();

    if ( p() -> talents.animal_instincts -> ok() )
    {
      p() -> animal_instincts_cds.clear();

      if ( !p() -> cooldowns.flanking_strike -> up() )
        p() -> animal_instincts_cds.push_back( std::make_pair( p() -> cooldowns.flanking_strike,
                                                               p() -> procs.animal_instincts_flanking ) );
      if ( !p() -> cooldowns.mongoose_bite -> up() )
        p() -> animal_instincts_cds.push_back( std::make_pair( p() -> cooldowns.mongoose_bite,
                                                               p() -> procs.animal_instincts_mongoose ) );
      if ( !p() -> cooldowns.aspect_of_the_eagle -> up() )
        p() -> animal_instincts_cds.push_back( std::make_pair( p() -> cooldowns.aspect_of_the_eagle,
                                                               p() -> procs.animal_instincts_aspect ) );
      if ( !p() -> cooldowns.harpoon -> up() )
        p() -> animal_instincts_cds.push_back( std::make_pair( p() -> cooldowns.harpoon,
                                                               p() -> procs.animal_instincts_harpoon ) );

      if ( !p() -> animal_instincts_cds.empty() )
      {
        unsigned roll =
          static_cast<unsigned>( p() -> rng().range( 0, p() -> animal_instincts_cds.size() ) );

        p() -> animal_instincts_cds[roll].first -> adjust( animal_instincts );
        p() -> animal_instincts_cds[roll].second -> occur();
      }
    }
  }

  virtual double composite_crit_chance() const override
  {
    double cc = hunter_melee_attack_t::composite_crit_chance();

    if ( p() -> artifacts.my_beloved_monster.rank() )
      cc += p() -> artifacts.my_beloved_monster.percent();

    return cc;
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_melee_attack_t::action_multiplier();

    if ( p() -> target -> target != p() )
      am *= 1.0 + p() -> specs.flanking_strike -> effectN( 3 ).percent();

    return am;
  }
};

// Lacerate ==========================================================================

struct lacerate_t: public hunter_melee_attack_t
{
  lacerate_t( hunter_t* p, const std::string& options_str ):
    hunter_melee_attack_t( "lacerate", p, p -> specs.lacerate )
  {
    parse_options( options_str );

    base_tick_time = data().effectN( 1 ).period();
    cooldown -> duration = data().cooldown();
    cooldown -> hasted = false;
    direct_tick = false;
    dot_duration = data().duration();
    tick_zero = false;
    weapon_multiplier = 0.0;
    weapon_power_mod = 0.0;

    if ( p -> artifacts.lacerating_talons.rank() )
      base_multiplier *= 1.0 + p -> artifacts.lacerating_talons.percent();
  }

  virtual void tick( dot_t* d ) override
  {
    hunter_melee_attack_t::tick( d );

    if ( p() -> talents.mortal_wounds -> ok() && rng().roll( p() -> talents.mortal_wounds -> proc_chance() ) )
    {
      p() -> cooldowns.mongoose_bite -> reset( true );
      p() -> procs.mortal_wounds -> occur();
    }
  }

  virtual void impact( action_state_t *s ) override
  {
    hunter_melee_attack_t::impact( s );
     
    td( s -> target ) -> debuffs.lacerate -> trigger();

    if ( p() -> sets.has_set_bonus( HUNTER_SURVIVAL, T18, B2 ) )
      td( s -> target ) -> debuffs.t18_2pc_open_wounds -> trigger();
  }
};

// Serpent Sting =====================================================================

struct serpent_sting_t: public hunter_melee_attack_t
{
  serpent_sting_t( hunter_t* player ):
    hunter_melee_attack_t( "serpent_sting", player, player -> find_spell( 118253 ) )
  {
    background = true;
    tick_may_crit = true;
    hasted_ticks = false;
    weapon_multiplier = 0;
  }
};

// Carve =============================================================================

struct carve_t: public hunter_melee_attack_t
{
  carve_t( hunter_t* p, const std::string& options_str ):
    hunter_melee_attack_t( "carve", p, p -> specs.carve )
  {
    parse_options( options_str );

    aoe = -1;
    radius = data().effectN( 1 ).radius();
    range = data().max_range();

    if ( p -> talents.serpent_sting -> ok() )
      impact_action = new serpent_sting_t( p );
  }

  virtual void execute() override
  {
    hunter_melee_attack_t::execute();

    if ( p() -> legendary.sv_ring && num_targets() > 1 )
    {
      std::vector<player_t*> carve_targets = execute_state -> action -> target_list();
      std::vector<player_t*> available_targets;
      std::vector<player_t*> lacerated_targets;

      // Split the target list into targets with and without debuffs
      for ( size_t i = 0; i < carve_targets.size(); i++ )
      {
        if ( td( carve_targets[ i ] ) -> dots.lacerate -> is_ticking() )
          lacerated_targets.push_back( carve_targets[ i ] );
        else
          available_targets.push_back( carve_targets[ i ] );
      }

      // Spread the dots to available targets
      for ( size_t i = 0; i < lacerated_targets.size(); i++ )
      {
        if ( available_targets.empty() ) 
          break;

        td( lacerated_targets[ i ] ) -> dots.lacerate -> copy( available_targets.back(), DOT_COPY_CLONE );
        available_targets.pop_back();
      }
    }
  }

  virtual bool ready() override
  {
    if ( p() -> talents.butchery -> ok() )
      return false;

    return hunter_melee_attack_t::ready();
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_melee_attack_t::action_multiplier();

    if ( p() -> artifacts.hellcarver.rank() )
      am *= 1.0 + num_targets() * p() -> artifacts.hellcarver.percent();

    return am;
  }
};



// Fury of the Eagle ================================================================

struct fury_of_the_eagle_t: public hunter_melee_attack_t
{
  struct fury_of_the_eagle_tick_t: public hunter_melee_attack_t
  {
    fury_of_the_eagle_tick_t( hunter_t* p ):
      hunter_melee_attack_t( "fury_of_the_eagle_tick", p, p -> find_spell( 203413 ) )
    {
      aoe = -1;
      weapon_multiplier = 0;
      background = true;
      may_crit = true;
      radius = data().max_range();
    }
  };

  fury_of_the_eagle_t( hunter_t* p, const std::string& options_str ):
    hunter_melee_attack_t( "fury_of_the_eagle", p, p -> artifacts.fury_of_the_eagle )
  {
    parse_options( options_str );

    channeled = true;
    tick_zero = true;
    weapon_multiplier = 0;
    tick_action = new fury_of_the_eagle_tick_t( p );
  }

  virtual void execute() override
  {
    hunter_melee_attack_t::execute();

    if ( p() -> buffs.mongoose_fury -> up() )
    {
      p() -> buffs.mongoose_fury -> extend_duration( p(), timespan_t::from_seconds( 5.0 ) );

      // Tracking purposes
      p() -> buffs.fury_of_the_eagle -> trigger( p() -> buffs.mongoose_fury -> stack(), p() -> buffs.fury_of_the_eagle -> DEFAULT_VALUE(), -1.0,
        this -> composite_dot_duration( execute_state ) );
    }
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_melee_attack_t::action_multiplier();

    if ( p() -> buffs.mongoose_fury -> up() )
      am *= 1.0 + p() -> buffs.mongoose_fury -> check_stack_value();

    return am;
  }
};

// Butchery ==========================================================================

struct butchery_t: public hunter_melee_attack_t
{
  butchery_t( hunter_t* p, const std::string& options_str ):
    hunter_melee_attack_t( "butchery", p, p -> talents.butchery )
  {
    parse_options( options_str );

    aoe = -1;
    cooldown -> hasted = true;
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_melee_attack_t::action_multiplier();

    if ( p() -> artifacts.hellcarver.rank() )
      am *= 1.0 + num_targets() * p() -> artifacts.hellcarver.percent();

    return am;
  }

  virtual void execute() override
  {
    hunter_melee_attack_t::execute();

    if ( p() -> legendary.sv_ring && num_targets() > 1 )
    {
      std::vector<player_t*> butchery_targets = execute_state -> action -> target_list();
      std::vector<player_t*> available_targets;
      std::vector<player_t*> lacerated_targets;

      // Split the target list into targets with and without debuffs
      for ( size_t i = 0; i < butchery_targets.size(); i++ )
      {
        if ( td( butchery_targets[ i ] ) -> dots.lacerate -> is_ticking() )
          lacerated_targets.push_back( butchery_targets[ i ] );
        else
          available_targets.push_back( butchery_targets[ i ] );
      }

      // Spread the dots to available targets
      for ( size_t i = 0; i < lacerated_targets.size(); i++ )
      {
        if ( available_targets.empty() ) 
          break;

        td( lacerated_targets[ i ] ) -> dots.lacerate -> copy( available_targets.back(), DOT_COPY_CLONE );
        available_targets.pop_back();
      }
    }
  }

};

// Throwing Axes =====================================================================

struct throwing_axes_t: public hunter_melee_attack_t
{
  struct throwing_axes_tick_t: public hunter_melee_attack_t
  {
    throwing_axes_tick_t( hunter_t* p ):
      hunter_melee_attack_t( "throwing_axes_tick", p, p -> find_spell( 200167 ) )
    {
      background = true;
      weapon_multiplier = 0.0;
    }
  };

  throwing_axes_t( hunter_t* p, const std::string& options_str ):
    hunter_melee_attack_t( "throwing_axes", p, p -> talents.throwing_axes )
  {
    parse_options( options_str );

    attack_power_mod.tick = p -> find_spell( 200167 ) -> effectN( 1 ).ap_coeff();
    base_tick_time = timespan_t::from_seconds( 0.2 );
    dot_duration = timespan_t::from_seconds( 0.6 );
    range = data().max_range();
    tick_action = new throwing_axes_tick_t( p );
    weapon_multiplier = 0.0;
    weapon_power_mod = 0.0;
  }
};


// Freezing Trap =====================================================================
// Implemented here because often there are buffs associated with it

struct freezing_trap_t: public hunter_melee_attack_t
{
  freezing_trap_t( hunter_t* p, const std::string& options_str ):
    hunter_melee_attack_t( "freezing_trap", p, p -> find_class_spell( "Freezing Trap" ) )
  {
    parse_options( options_str );

    cooldown -> duration = data().cooldown();

    if ( p -> talents.improved_traps -> ok() )
      cooldown -> duration *= 1.0 + p -> talents.improved_traps -> effectN( 1 ).percent();

    if ( p -> sets.has_set_bonus( p -> specialization(), PVP, B2 ) )
    {
      energize_type = ENERGIZE_ON_HIT;
      energize_resource = RESOURCE_FOCUS;
      energize_amount = p -> sets.set( p -> specialization(), PVP, B2 ) ->
        effectN( 1 ).trigger() -> effectN( 1 ).base_value();
    }
  }
};

// Raptor Strike Attack ==============================================================

struct raptor_strike_t: public hunter_melee_attack_t
{
  raptor_strike_t( hunter_t* p, const std::string& options_str ):
    hunter_melee_attack_t( "raptor_strike", p, p -> specs.raptor_strike )
  {
    parse_options( options_str );

    if ( p -> talents.serpent_sting -> ok() )
      impact_action = new serpent_sting_t( p );
  }

  virtual void execute() override
  {
    hunter_melee_attack_t::execute();

    if ( p() -> talents.way_of_the_moknathal -> ok() )
      p() -> buffs.moknathal_tactics -> trigger();
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_melee_attack_t::action_multiplier();

    if ( p() -> artifacts.raptors_cry.rank() )
      am *= 1.0 + p() -> artifacts.raptors_cry.percent();

    return am;
  }
};

// On the Trail =============================================================

struct on_the_trail_t: public hunter_melee_attack_t
{
  on_the_trail_t( hunter_t* p ):
    hunter_melee_attack_t( "on_the_trail", p, p -> find_spell( 204081 ) )
  {
    background = true;
    weapon_multiplier = 0.0;
    tick_may_crit = false;
  }

  virtual void execute() override
  {
    hunter_melee_attack_t::execute();

    td( execute_state -> target ) -> dots.on_the_trail -> trigger( data().duration() );
  }
};

// Harpoon ==================================================================

struct harpoon_t: public hunter_melee_attack_t
{
  bool first_harpoon;
  on_the_trail_t* on_the_trail;
  harpoon_t( hunter_t* p, const std::string& options_str ):
    hunter_melee_attack_t( "harpoon", p, p -> specs.harpoon ), first_harpoon( true ), on_the_trail( nullptr )
  {
    parse_options( options_str );
    harmful = false;
    weapon_multiplier = 0.0;

    if ( p -> artifacts.eagles_bite.rank() )
      on_the_trail = new on_the_trail_t( p );
  }

  virtual void execute() override
  {
    hunter_melee_attack_t::execute();

    first_harpoon = false;

    if ( on_the_trail )
      on_the_trail -> execute();

    if ( p() -> legendary.sv_waist )
      td( execute_state -> target ) -> debuffs.mark_of_helbrine -> trigger();
  }

  virtual bool ready() override
  {
    if ( first_harpoon )
      return hunter_melee_attack_t::ready();

    if ( p() -> current.distance_to_move < data().min_range() )
      return false;

    return hunter_melee_attack_t::ready();
  }

  virtual void reset() override
  {
    action_t::reset();
    first_harpoon = true;
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
    return hunter_action_t::gcd();
  }

  virtual void execute() override
  {
    hunter_action_t<spell_t>::execute();

    this -> try_steady_focus();
  }
};

// A Murder of Crows ========================================================

struct peck_t : public hunter_spell_t
{
  peck_t( hunter_t* player, const std::string& name ) :
    hunter_spell_t( name, player, player -> find_spell( 131900 ) )
  {
    background = true;
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
    double am = hunter_spell_t::action_multiplier();
    am *= p() -> beast_multiplier();

    if ( p() -> mastery.sniper_training -> ok() )
      am *= 1.0 + p() -> cache.mastery() * p() -> mastery.sniper_training -> effectN( 2 ).mastery_value();

    return am;
  }

  virtual void try_steady_focus() override
  {}
};

// TODO this should reset CD if the target dies
struct moc_t : public hunter_spell_t
{
  peck_t* peck;
  moc_t( hunter_t* player, const std::string& options_str ) :
    hunter_spell_t( "a_murder_of_crows", player, player -> talents.a_murder_of_crows ),
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
    tick_zero = true;

    starved_proc = player -> get_proc( "starved: a_murder_of_crows" );
  }

  hunter_t* p() const { return static_cast<hunter_t*>( player ); }

  void tick( dot_t*d ) override
  {
    hunter_spell_t::tick( d );
    peck -> execute();
  }

  virtual void execute() override
  {
    p() -> no_steady_focus();
    hunter_spell_t::execute();
  }
};

// Sentinel ==========================================================================

struct sentinel_t : public hunter_spell_t
{
  sentinel_t( hunter_t* p, const std::string& options_str ) :
    hunter_spell_t( "sentinel", p, p -> talents.sentinel )
  {
    harmful = false;
    parse_options( options_str );
    aoe = -1;
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.hunters_mark_exists -> trigger();
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_spell_t::impact( s );

    td( s -> target ) -> debuffs.hunters_mark -> trigger();
  }
};



//==============================
// Shared spells
//==============================




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

    if ( p() -> hati )
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

    cooldown -> hasted = true;
    harmful = false;
    hasted_ticks = false;
    may_crit = false;
    may_miss = false;
    school = SCHOOL_PHYSICAL;
  }

  bool init_finished() override
  {
    for (auto pet : p() -> pet_list)
    {
      stats -> add_child( pet -> get_stats( "dire_beast_melee" ) );
      stats -> add_child( pet -> get_stats( "felboar_melee" ) );
      stats -> add_child( pet -> get_stats( "stomp" ) );
    }

    return hunter_spell_t::init_finished();
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();
    p() -> no_steady_focus();

    // Trigger buffs
    timespan_t duration = p() -> buffs.dire_beast -> buff_duration + timespan_t::from_millis( p() -> buffs.t18_2p_dire_longevity -> check_stack_value() );
    p() -> buffs.dire_beast -> trigger( 1, p() -> buffs.dire_beast -> DEFAULT_VALUE(), -1.0, duration );
    p() -> buffs.t18_2p_dire_longevity -> expire();

    // Adjust BW cd
    timespan_t t = timespan_t::from_seconds( p() -> specs.dire_beast -> effectN( 1 ).base_value() );
    if ( p() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T19, B2 ) )
      t += timespan_t::from_seconds( p() -> sets.set( HUNTER_BEAST_MASTERY, T19, B2 ) -> effectN( 1 ).base_value() );
    p() -> cooldowns.bestial_wrath -> adjust( -t );

    if ( p() -> legendary.bm_feet )
      p() -> cooldowns.kill_command -> adjust( p() -> legendary.bm_feet -> driver() -> effectN( 1 ).time_value() );

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
    const timespan_t base_duration = duration;
    const timespan_t swing_time = beast -> main_hand_weapon.swing_time * beast -> composite_melee_speed();
    double partial_attacks_per_summon = base_duration / swing_time;
    double base_attacks_per_summon = floor( partial_attacks_per_summon + 0.5 ); // 8.4 -> 7, 8.5 -> 8, 8.6 -> 8, etc

    if ( rng().roll( partial_attacks_per_summon - base_attacks_per_summon ) )
      base_attacks_per_summon += 1;

    timespan_t summon_duration = base_attacks_per_summon * swing_time;
    assert( beast );
    beast -> summon( summon_duration );

    if ( p() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T18, B4 ) && rng().roll( 0.20 ) )
    {
      p() -> procs.tier18_4pc_bm -> occur();

      for( size_t i = 0; i < p() -> felboars.size(); i++ )
      {
        if ( p() -> felboars[ i ] -> is_sleeping() )
        {
          p() -> felboars[ i ] -> summon( p() -> find_spell( 188507 ) -> duration() );
          break;
        }
      }
    }
  }

  virtual bool ready() override
  {
    if ( p() -> talents.dire_frenzy -> ok() ) return false;

    return hunter_spell_t::ready();
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
    if ( p() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T19, B4 ) )
    {
      for ( size_t i = 0; i < p() -> pet_dire_beasts.size(); i++ )
      {
        if ( ! p() -> pet_dire_beasts[ i ] -> is_sleeping() )
          p() -> pet_dire_beasts[ i ] -> buffs.bestial_wrath -> trigger();
      }
    }
    if ( p() -> artifacts.master_of_beasts.rank() )
      p() -> hati -> buffs.bestial_wrath -> trigger();
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

    cooldown -> hasted = true;
    harmful = false;
  }

  bool init_finished() override
  {
    for (auto pet : p() -> pet_list)
    {
      stats -> add_child( pet -> get_stats( "kill_command" ) );
      stats -> add_child( pet -> get_stats( "bestial_ferocity" ) );
      stats -> add_child( pet -> get_stats( "jaws_of_thunder" ) );
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
    if ( p() -> artifacts.master_of_beasts.rank() )
      p() -> hati -> active.kill_command -> execute();
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

// Dire Frenzy ==============================================================

struct dire_frenzy_t: public hunter_spell_t
{
  dire_frenzy_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "dire_frenzy", p, p -> find_talent_spell( "Dire Frenzy" ) )
  {
    parse_options( options_str );

    cooldown -> hasted = true;
    harmful = false;
    school = SCHOOL_PHYSICAL;

    if ( p -> talents.dire_stable -> ok() )
      energize_amount += p -> talents.dire_stable -> effectN( 2 ).base_value();
  }

  bool init_finished() override
  {
    for (auto pet : p() -> pet_list)
    {
      stats -> add_child( pet -> get_stats( "dire_frenzy" ) );
      stats -> add_child( pet -> get_stats( "titans_frenzy" ) );
    }

    return hunter_spell_t::init_finished();
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();

    // Adjust BW cd
    timespan_t t = timespan_t::from_seconds( p() -> specs.dire_beast -> effectN( 1 ).base_value() );
    if ( p() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T19, B2 ) )
      t += timespan_t::from_seconds( p() -> sets.set( HUNTER_BEAST_MASTERY, T19, B2 ) -> effectN( 1 ).base_value() );
    p() -> cooldowns.bestial_wrath -> adjust( -t );

    if ( p() -> active.pet )
    {
      // Execute number of attacks listed in spell data
      for ( int i = 0; i < data().effectN( 1 ).base_value(); i++ )
        p() -> active.pet -> active.dire_frenzy -> schedule_execute();

      p() -> active.pet -> buffs.dire_frenzy -> trigger();
      p() -> active.pet -> buffs.titans_frenzy -> expire();
    }
  }
};

// Titan's Thunder ==========================================================

struct titans_thunder_t: public hunter_spell_t
{
  titans_thunder_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "titans_thunder", p, p -> artifacts.titans_thunder )
  {
    parse_options( options_str );
    harmful = false;
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();

    if ( p() -> talents.dire_frenzy -> ok() )
      p() -> active.pet -> buffs.titans_frenzy -> trigger();
    p() -> active.pet -> active.titans_thunder -> execute();
    p() -> hati -> active.titans_thunder -> execute();
    for ( size_t i = 0; i < p() -> pet_dire_beasts.size(); i++ )
    {
      if ( !p() -> pet_dire_beasts[ i ] -> is_sleeping() )
        p() -> pet_dire_beasts[ i ] -> active.titans_thunder -> execute();
    }
  }

  bool init_finished() override
  {
    for (auto pet : p() -> pet_list)
      stats -> add_child( pet -> get_stats( "titans_thunder" ) );

    return hunter_spell_t::init_finished();
  }

  virtual bool ready() override
  {
    if ( !p() -> active.pet )
      return false;

    return hunter_spell_t::ready();
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

  virtual void init() override
  {
    hunter_spell_t::init();

    if ( p() -> legendary.wrist )
      cooldown -> duration *= 1.0 + p() -> legendary.wrist -> driver() -> effectN( 1 ).percent();
  }
};

// Stampede =================================================================

struct stampede_t: public hunter_spell_t
{
  struct stampede_tick_t: public hunter_spell_t
  {
    stampede_tick_t( hunter_t* p ):
      hunter_spell_t( "stampede_tick", p, p -> find_spell( 201594 ) )
    {
      aoe = -1;
      background = true;
      may_crit = true;
    }
  };

  stampede_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "stampede", p, p -> talents.stampede )
  {
    parse_options( options_str );

    base_tick_time = timespan_t::from_millis( 667 );
    dot_duration = data().duration();
    hasted_ticks = false;
    radius = 8;
    school = SCHOOL_PHYSICAL;

    tick_action = new stampede_tick_t( p );

  }

  virtual double action_multiplier() const override
  {
    double am = hunter_spell_t::action_multiplier();

    if ( p() -> mastery.master_of_beasts -> ok() )
      am *= 1.0 + p() -> cache.mastery_value();

    return am;
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

    // Quick Shot: spell data adds -30 seconds to CD
    if ( p -> artifacts.quick_shot.rank() )
      cooldown -> duration += p -> artifacts.quick_shot.time_value();
  }

  virtual void execute() override
  {
    p() -> buffs.trueshot -> trigger( 1, value );

    if ( p() -> artifacts.rapid_killing.rank() )
      p() -> buffs.rapid_killing -> trigger();

    hunter_spell_t::execute();
  }
};

//==============================
// Survival spells
//==============================

// Aspect of the Eagle ===============================================================

struct aspect_of_the_eagle_t: public hunter_spell_t
{
  aspect_of_the_eagle_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "aspect_of_the_eagle", p, p -> specs.aspect_of_the_eagle )
  {
    parse_options( options_str );

    harmful = false;

    if ( p -> artifacts.embrace_of_the_aspects.rank() )
      cooldown -> duration *= 1.0 + p -> artifacts.embrace_of_the_aspects.percent();
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.aspect_of_the_eagle -> trigger();
  }

  virtual void init() override
  {
    hunter_spell_t::init();

    if ( p() -> legendary.wrist )
      cooldown -> duration *= 1.0 + p() -> legendary.wrist -> driver() -> effectN( 1 ).percent();
  }
};

// Snake Hunter ======================================================================

struct snake_hunter_t: public hunter_spell_t
{
  snake_hunter_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "snake_hunter", p, p -> talents.snake_hunter )
  {
    parse_options( options_str );

    harmful = false;
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();

    while( p() -> cooldowns.mongoose_bite -> current_charge != 3 )
      p() -> cooldowns.mongoose_bite -> reset( true );
  }
};

// Spitting Cobra ====================================================================

struct spitting_cobra_t: public hunter_spell_t
{
  struct spitting_cobra_tick_t: public hunter_spell_t
  {
    spitting_cobra_tick_t( hunter_t* p ):
      hunter_spell_t( "spitting_cobra_tick", p, p -> find_spell( 206685 ) )
    {
      background = true;
      may_crit = true;
    }
  };

  spitting_cobra_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "spitting_cobra", p, p -> talents.spitting_cobra )
  {
    parse_options( options_str );

    attack_power_mod.tick = p -> find_spell( 206685 ) -> effectN( 1 ).ap_coeff();
    base_tick_time = timespan_t::from_seconds( 2.0 );
    hasted_ticks = false;
    may_crit = true;
    weapon_multiplier = 0;

    tick_action = new spitting_cobra_tick_t( p );
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();

    p() -> buffs.spitting_cobra -> trigger();
  }
};

// Explosive Trap ====================================================================

struct explosive_trap_t: public hunter_spell_t
{
  explosive_trap_t( hunter_t* p, const std::string& options_str ):
      hunter_spell_t( "explosive_trap", p, p -> find_spell( 13812 ) )
    {
      parse_options( options_str );

      aoe = -1;
      attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
      attack_power_mod.tick = data().effectN( 2 ).ap_coeff();
      base_tick_time = data().effectN( 2 ).period();
      cooldown -> duration = p -> specs.explosive_trap -> cooldown();
      dot_duration = data().duration();
      hasted_ticks = false;
      may_crit = true;
      tick_may_crit = true;
      trigger_gcd = p -> specs.explosive_trap -> gcd();

      if ( p -> talents.improved_traps -> ok() )
        cooldown -> duration *= 1.0 + p -> talents.improved_traps -> effectN( 2 ).percent();

      if ( p -> artifacts.hunters_guile.rank() )
        cooldown -> duration *= 1.0 + p -> artifacts.hunters_guile.percent();
    }

  virtual double action_multiplier() const override
  {
    double am = hunter_spell_t::action_multiplier();

    if ( p() -> artifacts.explosive_force.rank() )
      am *= 1.0 + p() -> artifacts.explosive_force.percent();

    return am;
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();

    if ( p() -> legendary.sv_feet )
      p() -> resource_gain( RESOURCE_FOCUS, p() -> find_spell( 212575 ) -> effectN( 1 ).resource( RESOURCE_FOCUS ), p() -> gains.nesingwarys_trapping_treads );
  }

  virtual double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = hunter_spell_t::composite_target_da_multiplier( t );

    if ( t == p() -> target )
      m *= 1.0 + p() -> talents.expert_trapper -> effectN( 1 ).percent();

    return m;
  }  
  
  virtual double composite_target_ta_multiplier( player_t* t ) const override
  {
    double m = hunter_spell_t::composite_target_ta_multiplier( t );

    if ( t == p() -> target )
      m *= 1.0 + p() -> talents.expert_trapper -> effectN( 1 ).percent();

    return m;
  }
};

// Steel Trap =======================================================================

struct steel_trap_t: public hunter_spell_t
{
  steel_trap_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "steel_trap", p, p -> talents.steel_trap )
  {
    parse_options( options_str );

    attack_power_mod.direct = p -> talents.expert_trapper -> ok() ? p -> find_spell( 201199 ) -> effectN( 1 ).ap_coeff() : 0.0;
    attack_power_mod.tick = 1.0;
    base_tick_time = p -> find_spell( 162487 ) -> effectN( 1 ).period();
    dot_duration = p -> find_spell ( 162487 ) -> duration();
    hasted_ticks = false;
    may_crit = true;
    tick_may_crit = true;
    trigger_gcd = p -> talents.steel_trap -> gcd();
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();

    if ( p() -> legendary.sv_feet )
      p() -> resource_gain( RESOURCE_FOCUS, p() -> find_spell( 212575 ) -> effectN( 1 ).resource( RESOURCE_FOCUS ), p() -> gains.nesingwarys_trapping_treads );
  }
};

// Caltrops =========================================================================

struct caltrops_t: public hunter_spell_t
{
  struct caltrops_tick_t: public hunter_spell_t
  {
    caltrops_tick_t( hunter_t* p ):
      hunter_spell_t( "caltrops_tick", p, p -> find_spell( 194279 ) ) 
    {
      background = true;
      dot_behavior = DOT_CLIP;
      hasted_ticks = false;
      tick_may_crit = true;

      if ( p -> talents.expert_trapper -> ok() )
        base_multiplier = 1.0 + p -> talents.expert_trapper -> effectN( 2 ).percent();

      if ( p -> artifacts.hunters_guile.rank() )
        cooldown -> duration *= 1.0 + p -> artifacts.hunters_guile.percent();
    }
  };

  caltrops_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "caltrops", p, p -> talents.caltrops )
  {
    parse_options( options_str );

    aoe = -1;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    dot_behavior = DOT_CLIP;
    dot_duration = data().effectN( 1 ).trigger() -> duration();
    ground_aoe = true;
    hasted_ticks = false;
    tick_action = new caltrops_tick_t( p );
  }
};

// Dragonsfire Grenade ==============================================================

struct dragonsfire_grenade_t: public hunter_spell_t
{
  struct dragonsfire_conflagration_t: public hunter_spell_t
  {
    player_t* original_target;
    dragonsfire_conflagration_t( hunter_t* p ):
      hunter_spell_t( "dragonsfire_conflagration", p, p -> talents.dragonsfire_grenade -> effectN( 1 ).trigger() ), original_target( nullptr )
    {
      dot_duration = timespan_t::zero();
      attack_power_mod.tick = 0;
      attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
      aoe = -1;
      background = may_crit = true;
    }

    void impact( action_state_t* s ) override
    {
      if ( s -> target != original_target )
        hunter_spell_t::impact( s );
    }
  };

  dragonsfire_conflagration_t* conflag;
  dragonsfire_grenade_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "dragonsfire_grenade", p, p -> talents.dragonsfire_grenade ), conflag( nullptr )
  {
    parse_options( options_str );

    attack_power_mod.tick = data().effectN( 1 ).trigger() -> effectN( 1 ).ap_coeff();
    attack_power_mod.direct = data().effectN( 1 ).trigger() -> effectN( 3 ).ap_coeff();
    base_tick_time = data().effectN( 1 ).trigger() -> effectN( 1 ).period();
    dot_duration = data().effectN( 1 ).trigger() -> duration();
    hasted_ticks = false;
    harmful = tick_may_crit = true;
    school = data().effectN( 1 ).trigger() -> get_school_type();

    conflag = new dragonsfire_conflagration_t( p );
    add_child( conflag );
  }

  void tick( dot_t* d ) override
  {
    hunter_spell_t::tick( d );

    if ( conflag && conflag -> target_list().size() > 1 )
    {
      conflag -> target = d -> target;
      conflag -> original_target = d -> target;
      conflag -> execute();
    }
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
  dots.lacerate = target -> get_dot( "lacerate", p );
  dots.on_the_trail = target -> get_dot( "on_the_trail", p );

  debuffs.hunters_mark = 
    buff_creator_t( *this, "hunters_mark" )
      .spell( p -> find_spell( 185365 ) );
  if ( p -> talents.patient_sniper -> ok() )
  {
    debuffs.vulnerable = 
      buff_creator_t( *this, "vulnerability" )
        .spell( p -> find_spell( 187131 ) )
        .default_value( p -> find_spell( 187131 ) 
                          -> effectN( 2 )
                            .percent() + 
                              p -> talents.patient_sniper 
                                -> effectN( 2 )
                                  .percent() )
        .duration( timespan_t::from_seconds( 6.0 ) )
        .max_stack( 1 );
  }
  else
  {
    debuffs.vulnerable = 
      buff_creator_t( *this, "vulnerability" )
        .spell( p -> find_spell( 187131 ) )
        .default_value( p -> find_spell( 187131 ) 
                          -> effectN( 2 )
                            .percent() )
        .max_stack( 2 );
  }
  debuffs.true_aim = 
    buff_creator_t( *this, "true_aim" )
        .spell( p -> find_spell( 199803 ) )
        .default_value( p -> find_spell( 199803 ) 
                          -> effectN( 1 )
                            .percent() );

  debuffs.lacerate = 
    buff_creator_t( *this, "lacerate" )
        .cd( timespan_t::zero() )
        .spell( p -> find_specialization_spell( "Lacerate" ) );

  debuffs.t18_2pc_open_wounds = 
    buff_creator_t( *this, "open_wounds" )
        .spell( p -> find_spell( 188400 ) )
        .default_value( p -> find_spell( 188400 ) 
                          -> effectN( 1 )
                            .percent() );

  debuffs.mark_of_helbrine = 
    buff_creator_t( *this, "mark_of_helbrine" )
        .spell( p -> find_spell( 213156 ) )
        .default_value( p -> find_spell( 213154 ) 
                          -> effectN( 1 )
                            .percent() );
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
  if ( name == "aspect_of_the_eagle"   ) return new    aspect_of_the_eagle_t( this, options_str );
  if ( name == "aspect_of_the_wild"    ) return new     aspect_of_the_wild_t( this, options_str );
  if ( name == "auto_attack"           ) return new            auto_attack_t( this, options_str );
  if ( name == "auto_shot"             ) return new           start_attack_t( this, options_str );
  if ( name == "barrage"               ) return new                barrage_t( this, options_str );
  if ( name == "bestial_wrath"         ) return new          bestial_wrath_t( this, options_str );
  if ( name == "black_arrow"           ) return new            black_arrow_t( this, options_str );
  if ( name == "bursting_shot"         ) return new          bursting_shot_t( this, options_str );
  if ( name == "butchery"              ) return new               butchery_t( this, options_str );
  if ( name == "caltrops"              ) return new               caltrops_t( this, options_str );
  if ( name == "carve"                 ) return new                  carve_t( this, options_str );
  if ( name == "chimaera_shot"         ) return new          chimaera_shot_t( this, options_str );
  if ( name == "cobra_shot"            ) return new             cobra_shot_t( this, options_str );
  if ( name == "dire_beast"            ) return new             dire_beast_t( this, options_str );
  if ( name == "dire_frenzy"           ) return new            dire_frenzy_t( this, options_str );
  if ( name == "dragonsfire_grenade"   ) return new    dragonsfire_grenade_t( this, options_str );
  if ( name == "explosive_shot"        ) return new         explosive_shot_t( this, options_str );
  if ( name == "explosive_trap"        ) return new         explosive_trap_t( this, options_str );
  if ( name == "freezing_trap"         ) return new          freezing_trap_t( this, options_str );
  if ( name == "flanking_strike"       ) return new        flanking_strike_t( this, options_str );
  if ( name == "fury_of_the_eagle"     ) return new      fury_of_the_eagle_t( this, options_str );
  if ( name == "harpoon"               ) return new                harpoon_t( this, options_str );
  if ( name == "kill_command"          ) return new           kill_command_t( this, options_str );
  if ( name == "lacerate"              ) return new               lacerate_t( this, options_str );
  if ( name == "marked_shot"           ) return new            marked_shot_t( this, options_str );
  if ( name == "mongoose_bite"         ) return new          mongoose_bite_t( this, options_str );
  if ( name == "multishot"             ) return new             multi_shot_t( this, options_str );
  if ( name == "multi_shot"            ) return new             multi_shot_t( this, options_str );
  if ( name == "piercing_shot"         ) return new          piercing_shot_t( this, options_str );
  if ( name == "raptor_strike"         ) return new          raptor_strike_t( this, options_str );
  if ( name == "sentinel"              ) return new               sentinel_t( this, options_str );
  if ( name == "sidewinders"           ) return new            sidewinders_t( this, options_str );
  if ( name == "snake_hunter"          ) return new           snake_hunter_t( this, options_str );
  if ( name == "spitting_cobra"        ) return new         spitting_cobra_t( this, options_str );
  if ( name == "stampede"              ) return new               stampede_t( this, options_str );
  if ( name == "steel_trap"            ) return new             steel_trap_t( this, options_str );
  if ( name == "summon_pet"            ) return new             summon_pet_t( this, options_str );
  if ( name == "throwing_axes"         ) return new          throwing_axes_t( this, options_str );
  if ( name == "titans_thunder"        ) return new         titans_thunder_t( this, options_str );
  if ( name == "trueshot"              ) return new               trueshot_t( this, options_str );
  if ( name == "volley"                ) return new                 volley_t( this, options_str );
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
      pet_dire_beasts[ i ] = new pets::dire_critter_t( *this  );
  }

  if ( artifacts.hatis_bond.rank() )
    hati = new pets::hati_t( *this );

  if ( talents.black_arrow -> ok() )
  {
    //FIXME: Dark Minion should scale off MM Mastery
    dark_minion[ 0 ] = new pets::hunter_secondary_pet_t( *this, std::string( "dark_minion" ) );
    dark_minion[ 1 ] = new pets::hunter_secondary_pet_t( *this, std::string( "dark_minion_2" ) );
  }

  if ( sets.has_set_bonus( HUNTER_BEAST_MASTERY, T18, B4 ) )
  {
    for ( size_t i = 0; i < felboars.size(); i++ )
      felboars[ i ] = new pets::bm_t18_4pc_felboar( *this );
  }
}

// hunter_t::init_spells ====================================================

void hunter_t::init_spells()
{
  player_t::init_spells();

  //Tier 1
  talents.big_game_hunter                   = find_talent_spell( "Big Game Hunter" );
  talents.way_of_the_cobra                  = find_talent_spell( "Way of the Cobra" );
  talents.dire_stable                       = find_talent_spell( "Dire Stable" );

  talents.lone_wolf                         = find_talent_spell( "Lone Wolf" );
  talents.steady_focus                      = find_talent_spell( "Steady Focus" );
  talents.careful_aim                       = find_talent_spell( "Careful Aim" );

  talents.animal_instincts                  = find_talent_spell( "Animal Instincts" );
  talents.way_of_the_moknathal              = find_talent_spell( "Way of the Mok'Nathal" );
  talents.throwing_axes                     = find_talent_spell( "Throwing Axes" );

  //Tier 2
  talents.stomp                             = find_talent_spell( "Stomp" );
  talents.dire_frenzy                       = find_talent_spell( "Dire Frenzy" );
  talents.chimaera_shot                     = find_talent_spell( "Chimaera Shot" );
  
  talents.lock_and_load                     = find_talent_spell( "Lock and Load" );
  talents.black_arrow                       = find_talent_spell( "Black Arrow" );
  talents.true_aim                          = find_talent_spell( "True Aim" );
  
  talents.mortal_wounds                     = find_talent_spell( "Mortal Wounds" );
  talents.snake_hunter                      = find_talent_spell( "Snake Hunter" );

  //Tier 3
  talents.posthaste                         = find_talent_spell( "Posthaste" );
  talents.farstrider                        = find_talent_spell( "Farstrider" );
  talents.dash                              = find_talent_spell( "Dash" );

  //Tier 4
  talents.bestial_fury                      = find_talent_spell( "Bestial Fury" );
  talents.blink_strikes                     = find_talent_spell( "Blink Strikes" );
  talents.one_with_the_pack                 = find_talent_spell( "One with the Pack" );
  
  talents.explosive_shot                    = find_talent_spell( "Explosive Shot" );
  talents.sentinel                          = find_talent_spell( "Sentinel" );
  talents.patient_sniper                    = find_talent_spell( "Patient Sniper" );

  talents.caltrops                          = find_talent_spell( "Caltrops" );
  talents.steel_trap                        = find_talent_spell( "Steel Trap" );
  talents.improved_traps                    = find_talent_spell( "Improved Traps" );

  //Tier 5
  talents.intimidation                      = find_talent_spell( "Intimidation" );
  talents.wyvern_sting                      = find_talent_spell( "Wyvern Sting" );
  talents.binding_shot                      = find_talent_spell( "Binding Shot" );
  talents.camouflage                        = find_talent_spell( "Camouflage" );
  talents.sticky_bomb                       = find_talent_spell( "Sticky Bomb" );
  talents.rangers_net                       = find_talent_spell( "Ranger's Net" );

  //Tier 6
  talents.a_murder_of_crows                 = find_talent_spell( "A Murder of Crows" );
  talents.barrage                           = find_talent_spell( "Barrage" );
  talents.volley                            = find_talent_spell( "Volley" );

  talents.butchery                          = find_talent_spell( "Butchery" );
  talents.dragonsfire_grenade               = find_talent_spell( "Dragonsfire Grenade" );
  talents.serpent_sting                     = find_talent_spell( "Serpent Sting" );

  //Tier 7
  talents.stampede                          = find_talent_spell( "Stampede" );
  talents.killer_cobra                      = find_talent_spell( "Killer Cobra" );
  talents.aspect_of_the_beast               = find_talent_spell( "Aspect of the Beast" );

  talents.sidewinders                       = find_talent_spell( "Sidewinders" );
  talents.piercing_shot                     = find_talent_spell( "Piercing Shot" );
  talents.trick_shot                        = find_talent_spell( "Trick Shot" );

  talents.spitting_cobra                    = find_talent_spell( "Spitting Cobra" );
  talents.expert_trapper                    = find_talent_spell( "Expert Trapper" );

  // Mastery
  mastery.master_of_beasts     = find_mastery_spell( HUNTER_BEAST_MASTERY );
  mastery.sniper_training      = find_mastery_spell( HUNTER_MARKSMANSHIP );
  mastery.hunting_companion    = find_mastery_spell( HUNTER_SURVIVAL );

  // Glyphs
  glyphs.arachnophobia       = find_glyph_spell( "Glyph of Arachnophobia" );
  glyphs.lesser_proportion   = find_glyph_spell( "Glyph of Lesser Proportion" );
  glyphs.nesingwarys_nemesis = find_glyph_spell( "Glyph of Nesingwary's Nemesis'" );
  glyphs.sparks              = find_glyph_spell( "Glyph of Sparks" );
  glyphs.bullseye            = find_glyph_spell( "Glyph of the Bullseye" );
  glyphs.firecracker         = find_glyph_spell( "Glyph of the Firecracker" );
  glyphs.headhunter          = find_glyph_spell( "Glyph of the Headhunter" );
  glyphs.hook                = find_glyph_spell( "Glyph of the Hook" );
  glyphs.skullseye           = find_glyph_spell( "Glyph of the Skullseye" );
  glyphs.trident             = find_glyph_spell( "Glyph of the Trident" );

  // Spec spells
  specs.critical_strikes     = find_spell( 157443 );

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
  specs.flanking_strike      = find_specialization_spell( "Flanking Strike" );
  specs.raptor_strike        = find_specialization_spell( "Raptor Strike" );
  specs.mongoose_bite        = find_specialization_spell( "Mongoose Bite" );
  specs.harpoon              = find_specialization_spell( "Harpoon" );
  specs.lacerate             = find_specialization_spell( "Lacerate" );
  specs.aspect_of_the_eagle  = find_specialization_spell( "Aspect of the Eagle" );
  specs.carve                = find_specialization_spell( "Carve" );
  specs.tar_trap             = find_specialization_spell( "Tar Trap" );
  specs.explosive_trap       = find_specialization_spell( "Explosive Trap" );


  // Artifact spells
  artifacts.hatis_bond               = find_artifact_spell( "Hati's Bond" );
  artifacts.titans_thunder           = find_artifact_spell( "Titan's Thunder" );
  artifacts.beast_master             = find_artifact_spell( "Beast Master" );
  artifacts.master_of_beasts         = find_artifact_spell( "Master of Beasts" );
  artifacts.surge_of_the_stormgod    = find_artifact_spell( "Surge of the Stormgod" );
  artifacts.spitting_cobras          = find_artifact_spell( "Spitting Cobras" );
  artifacts.jaws_of_thunder          = find_artifact_spell( "Jaws of Thunder" );
  artifacts.wilderness_expert        = find_artifact_spell( "Wilderness Expert" );
  artifacts.pack_leader              = find_artifact_spell( "Pack Leader" );
  artifacts.unleash_the_beast        = find_artifact_spell( "Unleash the Beast" );
  artifacts.focus_of_the_titans      = find_artifact_spell( "Focus of the Titans" );
  artifacts.furious_swipes           = find_artifact_spell( "Furious Swipes" );

  artifacts.windburst                 = find_artifact_spell( "Windburst" );
  artifacts.wind_arrows               = find_artifact_spell( "Wind Arrows" );
  artifacts.legacy_of_the_windrunners = find_artifact_spell( "Legacy of the Windrunners" );
  artifacts.call_of_the_hunter        = find_artifact_spell( "Call of the Hunter" );
  artifacts.bullseye                  = find_artifact_spell( "Bullseye" );
  artifacts.mark_of_the_windrunner    = find_artifact_spell( "Mark of the Windrunner" );
  artifacts.deadly_aim                = find_artifact_spell( "Deadly Aim" );
  artifacts.quick_shot                = find_artifact_spell( "Quick Shot" );
  artifacts.critical_focus            = find_artifact_spell( "Critical Focus" );
  artifacts.windrunners_guidance      = find_artifact_spell( "Windrunner's Guidance" );
  artifacts.called_shot               = find_artifact_spell( "Called Shot" );
  artifacts.marked_for_death          = find_artifact_spell( "Marked for Death" );
  artifacts.precision                 = find_artifact_spell( "Precision" );
  artifacts.rapid_killing             = find_artifact_spell( "Rapid Killing" );

  artifacts.fury_of_the_eagle        = find_artifact_spell( "Fury of the Eagle" );
  artifacts.talon_strike             = find_artifact_spell( "Talon Strike" );
  artifacts.eagles_bite              = find_artifact_spell( "Eagle's Bite" );
  artifacts.aspect_of_the_skylord    = find_artifact_spell( "Aspect of the Skylord" );
  artifacts.iron_talons              = find_artifact_spell( "Iron Talons" );
  artifacts.sharpened_fang           = find_artifact_spell( "Sharpened Fang" );
  artifacts.raptors_cry              = find_artifact_spell( "Raptor's Cry" );
  artifacts.hellcarver               = find_artifact_spell( "Hellcarver" );
  artifacts.my_beloved_monster       = find_artifact_spell( "My Beloved Monster" );
  artifacts.explosive_force          = find_artifact_spell( "Explosive Force" );
  artifacts.fluffy_go                = find_artifact_spell( "Fluffy, Go" );
  artifacts.jagged_claws             = find_artifact_spell( "Jagged Claws" );
  artifacts.embrace_of_the_aspects   = find_artifact_spell( "Embrace of the Aspects" );
  artifacts.hunters_guile            = find_artifact_spell( "Hunter's Guile" );
  artifacts.lacerating_talons        = find_artifact_spell( "Lacerating Talons" );

  artifacts.windflight_arrows        = find_artifact_spell( "Windflight Arrows" );
  artifacts.spiritbound              = find_artifact_spell( "Spiritbound" );
  artifacts.voice_of_the_wild_gods   = find_artifact_spell( "Voice of the Wild Gods" );

  if ( talents.serpent_sting -> ok() )
    active.serpent_sting = new attacks::serpent_sting_t( this );

  if ( talents.careful_aim -> ok() )
    active.piercing_shots = new attacks::piercing_shots_t( this, "careful_aim", find_spell( 63468 ) );

  if ( artifacts.surge_of_the_stormgod.rank() )
    active.surge_of_the_stormgod = new attacks::surge_of_the_stormgod_t( this );
}

// hunter_t::init_base ======================================================

void hunter_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;

  base_focus_regen_per_second = 10.0;

  resources.base[RESOURCE_FOCUS] = 120 + specs.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS ) + talents.patient_sniper -> effectN( 1 ).resource( RESOURCE_FOCUS );

  stats_tier17_4pc_bm = get_stats( "tier17_4pc_bm" );
  stats_tier18_4pc_bm = get_stats( "tier18_4pc_bm" );
}

// hunter_t::init_buffs =====================================================

void hunter_t::create_buffs()
{
  player_t::create_buffs();

  // General
  
  buffs.volley = 
    buff_creator_t( this, "volley", talents.volley );

  // Beast Mastery

  buffs.aspect_of_the_wild           
    = buff_creator_t( this, 193530, "aspect_of_the_wild" )
      .cd( timespan_t::zero() )
      .affects_regen( true )
      .add_invalidate( CACHE_CRIT_CHANCE )
      .default_value( find_spell( 193530 ) -> effectN( 1 ).percent() );
  if ( artifacts.wilderness_expert.rank() )
  {
    buffs.aspect_of_the_wild 
      -> buff_duration += artifacts.wilderness_expert
                         .time_value();
  }

  buffs.bestial_wrath 
    = buff_creator_t( this, "bestial_wrath", specs.bestial_wrath )
        .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
        .cd( timespan_t::zero() )
        .default_value( specs.bestial_wrath 
                     -> effectN( 1 )
                       .percent() )
        .duration( timespan_t::from_seconds( 15.0 ) );
  if ( talents.bestial_fury -> ok() )
  {
    buffs.bestial_wrath 
      -> default_value += talents.bestial_fury 
                       -> effectN( 1 )
                         .percent();
  }
  if ( artifacts.unleash_the_beast.rank() )
  {
    buffs.bestial_wrath 
      -> default_value += artifacts.unleash_the_beast
                         .percent();
  }

  double big_game_hunter_crit = talents.big_game_hunter 
                             -> effectN( 1 )
                               .percent( );
  buffs.big_game_hunter = 
    buff_creator_t( this, "big_game_hunter", talents.big_game_hunter )
      .activated( true )
      .default_value( big_game_hunter_crit );

  buffs.dire_beast = 
    buff_creator_t( this, 120694, "dire_beast" )
      .max_stack( 8 )
      .stack_behavior( BUFF_STACK_ASYNCHRONOUS )
      .period( timespan_t::zero() )
      .tick_behavior( BUFF_TICK_NONE )
      .default_value( find_spell( 120694 ) 
                   -> effectN( 1 )
                     .resource( RESOURCE_FOCUS ) / 
                        find_spell( 120694 ) 
                     -> effectN( 1 )
                       .period().total_seconds() ); // focus per second
  if ( talents.dire_stable -> ok() )
  {
    buffs.dire_beast
      -> default_value += talents.dire_stable 
                       -> effectN( 1 )
                         .base_value() / 
                            buffs.dire_beast 
                         -> data().effectN( 1 )
                           .period().total_seconds(); // focus per second
  }

  buffs.t18_2p_dire_longevity = 
    buff_creator_t( this, 215911, "dire_longevity" )
      .default_value( find_spell( 215911 ) 
                   -> effectN( 1 )
                     .base_value() )
      .max_stack( 8 );

  // Marksmanship

  buffs.bullseye = 
    buff_creator_t( this, "bullseye", artifacts.bullseye.data()
                                     .effectN( 1 )
                                     .trigger() )
      .add_invalidate( CACHE_CRIT_CHANCE )
      .default_value( find_spell( 204090 ) 
                   -> effectN( 1 )
                     .percent() )
      .max_stack( 30 );

  buffs.bombardment = 
    buff_creator_t( this, "bombardment", specs.bombardment 
                                      -> effectN( 1 )
                                        .trigger() );

  double careful_aim_crit = talents.careful_aim 
                         -> effectN( 1 )
                           .percent( );
  buffs.careful_aim 
    = buff_creator_t( this, "careful_aim", talents.careful_aim )
        .activated( true )
        .default_value( careful_aim_crit );

  buffs.hunters_mark_exists 
    = buff_creator_t( this, 185365, "hunters_mark_exists" )
      .quiet( true );

  buffs.lock_and_load = 
    buff_creator_t( this, "lock_and_load", talents.lock_and_load 
                                        -> effectN( 1 )
                                          .trigger() )
      .max_stack( 2 );

  buffs.marking_targets = 
    buff_creator_t( this, 223138, "marking_targets" );

  buffs.pre_steady_focus = 
    buff_creator_t( this, "pre_steady_focus" )
      .max_stack( 3 )
      .quiet( true );

  buffs.rapid_killing = 
    buff_creator_t( this, 191342, "rapid_killing" )
      .default_value( find_spell( 191342 ) 
                   -> effectN( 1 )
                     .percent() );

  buffs.steady_focus 
    = buff_creator_t( this, 193534, "steady_focus" )
        .chance( talents.steady_focus -> ok() );

  buffs.t18_2p_rapid_fire = 
    buff_creator_t( this, 188202, "rapid_fire" )
      .add_invalidate( CACHE_HASTE )
      .chance( sets.set( HUNTER_MARKSMANSHIP, T18, B2 ) 
            -> proc_chance() )
      .default_value( find_spell( 188202 ) 
                   -> effectN( 1 )
                     .percent() );

  buffs.trick_shot = 
    buff_creator_t( this, 227272, "trick_shot" )
      .default_value( find_spell( 227272 ) 
                   -> effectN( 1 )
                     .percent() );

  buffs.trueshot = 
    buff_creator_t( this, "trueshot", specs.trueshot )
      .default_value( specs.trueshot -> effectN( 1 ).percent() )
      .cd( timespan_t::zero() )
      .add_invalidate( CACHE_HASTE );

  // Survival

  buffs.aspect_of_the_eagle = 
    buff_creator_t( this, 186289, "aspect_of_the_eagle" )
      .cd( timespan_t::zero() )
      .default_value( find_spell( 186289 ) 
                   -> effectN( 1 )
                     .percent() );
  if ( artifacts.aspect_of_the_skylord.rank() )
  {
    buffs.aspect_of_the_eagle 
      -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  buffs.fury_of_the_eagle = 
    buff_creator_t( this, 203415, "fury_of_the_eagle" )
      .cd( timespan_t::zero() )
      .max_stack( 6 );

  buffs.heavy_shot = 
    buff_creator_t( this, 167165, "heavy_shot" )
      .default_value( find_spell( 167165 ) 
                   -> effectN( 1 )
                     .percent() )
      .refresh_behavior( BUFF_REFRESH_EXTEND );

  buffs.moknathal_tactics = 
    buff_creator_t( this, "moknathal_tactics", talents.way_of_the_moknathal
                                            -> effectN( 1 )
                                              .trigger() )
      .default_value( find_spell( 201081 ) 
                   -> effectN( 1 )
                     .percent() )
      .max_stack( 4 );

  buffs.mongoose_fury = 
    buff_creator_t( this, 190931, "mongoose_fury" )
      .default_value( find_spell( 190931 ) 
                   -> effectN( 1 )
                     .percent() )
      .duration( timespan_t::from_seconds( 14.0 ) )
      .refresh_behavior( BUFF_REFRESH_DISABLED )
      .max_stack( 6 );

  buffs.sentinels_sight = 
    buff_creator_t( this, 208913, "sentinels_sight" )
      .default_value( find_spell( 208913 ) 
                   -> effectN( 1 )
                     .percent() )
      .max_stack( 20 );

  buffs.spitting_cobra = 
    buff_creator_t( this, "spitting_cobra", talents.spitting_cobra )
    .affects_regen( true )
    .default_value( find_spell( 194407 ) 
                 -> effectN( 2 )
                   .resource( RESOURCE_FOCUS ) );

  buffs.t19_4p_mongoose_power = 
    buff_creator_t( this, 211362, "mongoose_power" )
      .default_value( find_spell( 211362 ) 
                   -> effectN( 1 )
                     .percent() );
}

bool hunter_t::init_special_effects()
{
  bool ret = player_t::init_special_effects();

  if ( beastlord )
  {
    buffs.bestial_wrath 
      -> buff_duration *= 1.0 + (
                          find_spell( beastlord -> spell_id ) 
                       -> effectN( 1 )
                         .average( beastlord -> item ) 
                        / 100.0 );
  }

  if ( blackness )
    blackness_multiplier = find_spell( blackness -> spell_id ) -> effectN( 1 ).average( blackness -> item ) / 100.0;

  return ret;
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
  gains.dire_beast           = get_gain( "dire_beast" );
  gains.aspect_of_the_wild   = get_gain( "aspect_of_the_wild" );
  gains.spitting_cobra       = get_gain( "spitting_cobra" );
  gains.nesingwarys_trapping_treads = get_gain( "nesingwarys_trapping_treads" );
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

  procs.lock_and_load                = get_proc( "lock_and_load" );
  procs.wild_call                    = get_proc( "wild_call" );
  procs.tier17_2pc_bm                = get_proc( "tier17_2pc_bm" );
  procs.tier18_4pc_bm                = get_proc( "tier18_4pc_bm" );
  procs.black_arrow_trinket_reset    = get_proc( "black_arrow_trinket_reset" );
  procs.tier18_2pc_mm_wasted_proc    = get_proc( "tier18_2pc_mm_wasted_proc" );
  procs.tier18_2pc_mm_wasted_overwrite     = get_proc ( "tier18_2pc_mm_wasted_overwrite" );
  procs.hunting_companion            = get_proc( "hunting_companion" );
  procs.mortal_wounds                = get_proc( "mortal_wounds" );
  procs.t18_4pc_sv                   = get_proc( "t18_4pc_sv" );
  procs.no_vuln_aimed_shot           = get_proc( "no_vuln_aimed_shot" );
  procs.no_vuln_marked_shot          = get_proc( "no_vuln_marked_shot" );
  procs.zevrims_hunger               = get_proc( "zevrims_hunger" );
  procs.convergence                  = get_proc( "convergence" );
  procs.marking_targets              = get_proc( "marking_targets" );
  procs.wasted_marking_targets       = get_proc( "wasted_marking_targets" );
  procs.animal_instincts_mongoose    = get_proc( "animal_instincts_mongoose" );
  procs.animal_instincts_aspect      = get_proc( "animal_instincts_aspect" );
  procs.animal_instincts_harpoon     = get_proc( "animal_instincts_harpoon" );
  procs.animal_instincts_flanking    = get_proc( "animal_instincts_flanking" );
  procs.animal_instincts             = get_proc("animal_instincts");
}

// hunter_t::init_rng =======================================================

void hunter_t::init_rng()
{
  // RPPMS
  ppm_hunters_mark = get_rppm( "hunters_mark", find_specialization_spell( "Hunter's Mark" ) );
  ppm_call_of_the_hunter = get_rppm( "call_of_the_hunter", find_artifact_spell( "Call of the Hunter" ) );

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
      if ( true_level > 100 )
        flask_action += "flask_of_the_seventh_demon";
      else if ( true_level > 90 )
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
      else if ( level() <= 100 )
      {
        if ( specialization() == HUNTER_BEAST_MASTERY || specialization() == HUNTER_MARKSMANSHIP )
          food_action += "salty_squid_roll";
        else if ( specialization() == HUNTER_SURVIVAL )
          food_action += "pickled_eel";
      }
      else if ( specialization() == HUNTER_BEAST_MASTERY )
        food_action += "nightborne_delicacy_platter";
      else if ( specialization() == HUNTER_MARKSMANSHIP )
        food_action += "nightborne_delicacy_platter";
      else
        food_action += "seedbattered_fish_plate";
      precombat -> add_action( food_action );
    }

    precombat -> add_action( "summon_pet" );
    precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

    //Pre-pot
    if ( sim -> allow_potions )
    {
      if ( true_level > 100 )
      {
        if ( specialization() == HUNTER_SURVIVAL )
          precombat -> add_action( "potion,name=potion_of_the_old_war");
        else if ( specialization() == HUNTER_BEAST_MASTERY )
        {
          precombat -> add_action( "potion,name=prolonged_power" );
        }
        else
        {
          precombat -> add_action( "potion,name=prolonged_power,if=active_enemies>2" );
          precombat -> add_action( "potion,name=deadly_grace" );
        }
      }
      else if ( true_level > 90 )
        precombat -> add_action( "potion,name=draenic_agility" );
      else if ( true_level >= 80 )
        precombat -> add_action( "potion,name=virmens_bite" );
    }

    if ( true_level > 100 )
      precombat -> add_action( "augmentation,type=defiled" );

    if ( specialization() == HUNTER_BEAST_MASTERY )
    {
      precombat -> add_action( "volley,toggle=on" );
    }


    switch ( specialization() )
    {
    case HUNTER_SURVIVAL:
      apl_surv();
      break;
    case HUNTER_BEAST_MASTERY:
      apl_bm();
      break;
    case HUNTER_MARKSMANSHIP:
      apl_mm();
      break;
    default:
      apl_default(); // DEFAULT
      break;
    }

    if ( summon_pet_str.empty() && specialization() != HUNTER_SURVIVAL )
      summon_pet_str = "cat";
    if ( summon_pet_str.empty() && specialization() == HUNTER_SURVIVAL )
      summon_pet_str = "carrion_bird";

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
    if ( specialization() == HUNTER_MARKSMANSHIP )
      list -> add_action( "arcane_torrent,if=focus.deficit>=30&(!talent.sidewinders.enabled|cooldown.sidewinders.charges<2)");
    else
      list -> add_action( "arcane_torrent,if=focus.deficit>=30" );
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

  default_list -> add_action( "potion,name=prolonged_power,if=buff.bestial_wrath.remains|!cooldown.beastial_wrath.remains" );
  default_list -> add_action( "a_murder_of_crows" );
  default_list -> add_action( "stampede,if=buff.bloodlust.up|buff.bestial_wrath.up|cooldown.bestial_wrath.remains<=2|target.time_to_die<=14" );
  default_list -> add_action( "dire_beast,if=cooldown.bestial_wrath.remains>3" );
  default_list -> add_action( "dire_frenzy,if=cooldown.bestial_wrath.remains>6|target.time_to_die<9" );
  default_list -> add_action( "aspect_of_the_wild,if=buff.bestial_wrath.up|target.time_to_die<12" );
  default_list -> add_action( "barrage,if=spell_targets.barrage>1" );
  default_list -> add_action( "titans_thunder,if=talent.dire_frenzy.enabled|cooldown.dire_beast.remains>=3|buff.bestial_wrath.up&pet.dire_beast.active" );
  default_list -> add_action( "bestial_wrath" );
  default_list -> add_action( "multi_shot,if=spell_targets>4&(pet.buff.beast_cleave.remains<gcd.max|pet.buff.beast_cleave.down)" );
  default_list -> add_action( "kill_command" );
  default_list -> add_action( "multi_shot,if=spell_targets>1&(pet.buff.beast_cleave.remains<gcd.max*2|pet.buff.beast_cleave.down)" );
  default_list -> add_action( "chimaera_shot,if=focus<90" );
  default_list -> add_action( "cobra_shot,if=cooldown.kill_command.remains>focus.time_to_max&cooldown.bestial_wrath.remains>focus.time_to_max|(buff.bestial_wrath.up&focus.regen*cooldown.kill_command.remains>30)|target.time_to_die<cooldown.kill_command.remains" );
}

// Marksman Action List ======================================================================

void hunter_t::apl_mm()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* cooldowns  = get_action_priority_list( "cooldowns" );
  action_priority_list_t* open = get_action_priority_list( "open" );
  action_priority_list_t* targetdie = get_action_priority_list( "targetdie" );
  action_priority_list_t* trueshotaoe = get_action_priority_list( "trueshotaoe" );
  action_priority_list_t* precombat    = get_action_priority_list( "precombat" );
  action_priority_list_t* patient_sniper = get_action_priority_list( "patient_sniper" );
  action_priority_list_t* non_patient_sniper = get_action_priority_list( "non_patient_sniper" );

  if ( artifacts.windburst.rank() )
    precombat -> add_action( "windburst" );

  default_list -> add_action( "auto_shot" );

  add_item_actions( default_list );
  add_racial_actions( default_list );

  default_list -> add_action( "volley,toggle=on" );
  default_list -> add_action( "auto_shot" );


  default_list -> add_action( "variable,name=safe_to_build,value=debuff.hunters_mark.down|(buff.trueshot.down&buff.marking_targets.down)" );
  default_list -> add_action( "variable,name=use_multishot,value=((buff.marking_targets.up|buff.trueshot.up)&spell_targets.multishot>1)|(buff.marking_targets.down&buff.trueshot.down&spell_targets.multishot>2)" );
  default_list -> add_action( "call_action_list,name=open,if=active_enemies=1&time<=15" );
  default_list -> add_action( "a_murder_of_crows,if=(target.time_to_die>=cooldown+duration|target.health.pct<20)&(debuff.hunters_mark.down|(debuff.hunters_mark.remains>execute_time&debuff.vulnerability.remains>execute_time&focus+(focus.regen*debuff.vulnerability.remains)>=60&focus+(focus.regen*debuff.hunters_mark.remains)>=60))" );
  default_list -> add_action( "call_action_list,name=cooldowns" );
  default_list -> add_action( "call_action_list,name=trueshotaoe,if=(target.time_to_die>=cooldown+duration|target.health.pct<20)&(debuff.hunters_mark.down|(debuff.hunters_mark.remains>execute_time&debuff.vulnerability.remains>execute_time&focus+(focus.regen*debuff.vulnerability.remains)>=60&focus+(focus.regen*debuff.hunters_mark.remains)>=60))" );
  default_list -> add_action( "black_arrow,if=debuff.hunters_mark.down|(debuff.hunters_mark.remains>execute_time&debuff.vulnerability.remains>execute_time&focus+(focus.regen*debuff.vulnerability.remains)>=70&focus+(focus.regen*debuff.hunters_mark.remains)>=70)" );
  default_list -> add_action( "barrage,if=(target.time_to_20pct>10|target.health.pct<=20|spell_targets>1)&((buff.trueshot.down|(target.health.pct<=20&buff.bullseye.stack<29)|spell_targets>1)&debuff.hunters_mark.down|(debuff.hunters_mark.remains>execute_time&debuff.vulnerability.remains>execute_time&focus+(focus.regen*debuff.vulnerability.remains)>=90&focus+(focus.regen*debuff.hunters_mark.remains)>=90))" );

  default_list -> add_action( "call_action_list,name=targetdie,if=target.time_to_die<6&active_enemies=1" );
  default_list -> add_action( "call_action_list,name=patient_sniper,if=talent.patient_sniper.enabled" );
  default_list -> add_action( "call_action_list,name=non_patient_sniper,if=!talent.patient_sniper.enabled" );

  open -> add_action( "a_murder_of_crows" );
  open -> add_action( "trueshot" );
  open -> add_action( "piercing_shot" );
  open -> add_action( "explosive_shot" );
  open -> add_action( "barrage,if=!talent.patient_sniper.enabled" );
  open -> add_action( "arcane_shot,line_cd=16&!talent.patient_sniper.enabled" );
  open -> add_action( "sidewinders,if=(buff.marking_targets.down&buff.trueshot.remains<2)|(charges_fractional>=1.9&focus<80)" );
  open -> add_action( "marked_shot" );
  open -> add_action( "barrage,if=buff.bloodlust.up" );
  open -> add_action( "aimed_shot,if=(buff.lock_and_load.up&execute_time<debuff.vulnerability.remains)|focus>90&!talent.patient_sniper.enabled&talent.trick_shot.enabled" );
  open -> add_action( "aimed_shot,if=buff.lock_and_load.up&execute_time<debuff.vulnerability.remains" );
  open -> add_action( "black_arrow" );
  open -> add_action( "barrage" );
  open -> add_action( "arcane_shot" );
  open -> add_action( "aimed_shot,if=execute_time<debuff.vulnerability.remains" );
  open -> add_action( "sidewinders" );
  open -> add_action( "aimed_shot" );
  
  cooldowns -> add_action( "potion,name=prolonged_power,if=spell_targets.multishot>2&((buff.trueshot.react&buff.bloodlust.react)|buff.bullseye.react>=23|target.time_to_die<62)" );
  cooldowns -> add_action( "potion,name=deadly_grace,if=(buff.trueshot.react&buff.bloodlust.react)|buff.bullseye.react>=23|target.time_to_die<31" );
  cooldowns -> add_action( "trueshot,if=time<5|buff.bloodlust.react|target.time_to_die>=(cooldown+duration)|buff.bullseye.react>25|target.time_to_die<16" );

  patient_sniper -> add_action( "marked_shot,cycle_targets=1,if=(talent.sidewinders.enabled&talent.barrage.enabled&spell_targets>2)|debuff.hunters_mark.remains<2|((debuff.vulnerability.up|talent.sidewinders.enabled)&debuff.vulnerability.remains<gcd)" );
  patient_sniper -> add_action( "windburst,if=talent.sidewinders.enabled&(debuff.hunters_mark.down|(debuff.hunters_mark.remains>execute_time&focus+(focus.regen*debuff.hunters_mark.remains)>=50))|buff.trueshot.up" );
  patient_sniper -> add_action( "sidewinders,if=buff.trueshot.up&((buff.marking_targets.down&buff.trueshot.remains<2)|(charges_fractional>=1.9&(focus.deficit>70|spell_targets>1)))" );
  patient_sniper -> add_action( "multishot,if=buff.marking_targets.up&debuff.hunters_mark.down&variable.use_multishot&focus.deficit>2*spell_targets+gcd*focus.regen" );
  patient_sniper -> add_action( "aimed_shot,if=buff.lock_and_load.up&buff.trueshot.up&debuff.vulnerability.remains>execute_time" );
  patient_sniper -> add_action( "marked_shot,if=buff.trueshot.up&!talent.sidewinders.enabled" );
  patient_sniper -> add_action( "arcane_shot,if=buff.trueshot.up" );
  patient_sniper -> add_action( "aimed_shot,if=debuff.hunters_mark.down&debuff.vulnerability.remains>execute_time" );
  patient_sniper -> add_action( "aimed_shot,if=talent.sidewinders.enabled&debuff.hunters_mark.remains>execute_time&debuff.vulnerability.remains>execute_time&(buff.lock_and_load.up|(focus+debuff.hunters_mark.remains*focus.regen>=80&focus+focus.regen*debuff.vulnerability.remains>=80))&(!talent.piercing_shot.enabled|cooldown.piercing_shot.remains>5|focus>120)" );
  patient_sniper -> add_action( "aimed_shot,if=!talent.sidewinders.enabled&debuff.hunters_mark.remains>execute_time&debuff.vulnerability.remains>execute_time&(buff.lock_and_load.up|(buff.trueshot.up&focus>=80)|(buff.trueshot.down&focus+debuff.hunters_mark.remains*focus.regen>=80&focus+focus.regen*debuff.vulnerability.remains>=80))&(!talent.piercing_shot.enabled|cooldown.piercing_shot.remains>5|focus>120)" );
  patient_sniper -> add_action( "windburst,if=!talent.sidewinders.enabled&focus>80&(debuff.hunters_mark.down|(debuff.hunters_mark.remains>execute_time&focus+(focus.regen*debuff.hunters_mark.remains)>=50))" );
  patient_sniper -> add_action( "marked_shot,if=(talent.sidewinders.enabled&spell_targets>1)|focus.deficit<50|buff.trueshot.up|(buff.marking_targets.up&(!talent.sidewinders.enabled|cooldown.sidewinders.charges_fractional>=1.2))" );
  patient_sniper -> add_action( "piercing_shot,if=focus>80" );
  patient_sniper -> add_action( "sidewinders,if=variable.safe_to_build&((buff.trueshot.up&focus.deficit>70)|charges_fractional>=1.9)" );
  patient_sniper -> add_action( "sidewinders,if=(buff.marking_targets.up&debuff.hunters_mark.down&buff.trueshot.down)|(cooldown.sidewinders.charges_fractional>1&target.time_to_die<11)" );
  patient_sniper -> add_action( "arcane_shot,if=variable.safe_to_build&!variable.use_multishot&focus.deficit>5+gcd*focus.regen" );
  patient_sniper -> add_action( "multishot,if=variable.safe_to_build&variable.use_multishot&focus.deficit>2*spell_targets+gcd*focus.regen" );
  patient_sniper -> add_action( "aimed_shot,if=debuff.vulnerability.down&focus>80&cooldown.windburst.remains>focus.time_to_max" );

  non_patient_sniper -> add_action("windburst");
  non_patient_sniper -> add_action( "piercing_shot,if=focus>=100" );
  non_patient_sniper -> add_action( "sentinel,if=debuff.hunters_mark.down&focus>30&buff.trueshot.down" );
  non_patient_sniper -> add_action( "sidewinders,if=debuff.vulnerability.remains<gcd&time>6" );
  non_patient_sniper -> add_action( "aimed_shot,if=buff.lock_and_load.up&spell_targets.barrage<3" );
  non_patient_sniper -> add_action( "marked_shot" );
  non_patient_sniper -> add_action( "explosive_shot" );
  non_patient_sniper -> add_action( "sidewinders,if=((buff.marking_targets.up|buff.trueshot.up)&focus.deficit>70)|charges_fractional>=1.9" );
  non_patient_sniper -> add_action( "arcane_shot,if=!variable.use_multishot&(buff.marking_targets.up|(talent.steady_focus.enabled&(buff.steady_focus.down|buff.steady_focus.remains<2)))" );
  non_patient_sniper -> add_action( "multishot,if=variable.use_multishot&(buff.marking_targets.up|(talent.steady_focus.enabled&(buff.steady_focus.down|buff.steady_focus.remains<2)))" );
  non_patient_sniper -> add_action( "aimed_shot,if=!talent.piercing_shot.enabled|cooldown.piercing_shot.remains>3" );
  non_patient_sniper -> add_action( "arcane_shot,if=!variable.use_multishot" );
  non_patient_sniper -> add_action( "multishot,if=variable.use_multishot" );

  targetdie -> add_action( "marked_shot" );
  targetdie -> add_action( "windburst" );
  targetdie -> add_action( "aimed_shot,if=debuff.vulnerability.remains>execute_time&target.time_to_die>execute_time" );
  targetdie -> add_action( "sidewinders" );
  targetdie -> add_action( "aimed_shot" );
  targetdie -> add_action( "arcane_shot" );

  trueshotaoe -> add_action( "marked_shot" );
  trueshotaoe -> add_action( "barrage,if=!talent.patient_sniper.enabled" );
  trueshotaoe -> add_action( "piercing_shot" );
  trueshotaoe -> add_action( "explosive_shot" );
  trueshotaoe -> add_action( "aimed_shot,if=(!talent.patient_sniper.enabled|talent.trick_shot.enabled)&spell_targets.multishot=2&buff.lock_and_load.up&execute_time<debuff.vulnerability.remains" );
  trueshotaoe -> add_action( "multishot" );
}

// Survival Action List ===================================================================

void hunter_t::apl_surv()
{
  action_priority_list_t* default_list  = get_action_priority_list( "default" );
  action_priority_list_t* precombat     = get_action_priority_list( "precombat" );

  precombat -> add_action( "harpoon" );

  default_list -> add_action( "auto_attack" );

  add_racial_actions( default_list );
  add_item_actions( default_list );
  
  default_list -> add_action( "potion,name=old_war,if=buff.aspect_of_the_eagle.remains" );
  default_list -> add_action( "fury_of_the_eagle,if=(buff.mongoose_fury.up&buff.mongoose_fury.remains<=gcd)|target.time_to_die<=execute_time" );
  default_list -> add_action( "raptor_strike,if=talent.way_of_the_moknathal.enabled&buff.moknathal_tactics.remains&buff.moknathal_tactics.remains<gcd.max" );
  default_list -> add_action( "a_murder_of_crows" );
  default_list -> add_action( "steel_trap" );
  default_list -> add_action( "explosive_trap" );
  default_list -> add_action( "dragonsfire_grenade" );
  default_list -> add_action( "caltrops" );
  default_list -> add_action( "carve,cycle_targets=1,if=talent.serpent_sting.enabled&active_enemies>=3&(!dot.serpent_sting.ticking|dot.serpent_sting.remains<=gcd.max)" );
  default_list -> add_action( "raptor_strike,cycle_targets=1,if=talent.serpent_sting.enabled&active_enemies<=2&(!dot.serpent_sting.ticking|dot.serpent_sting.remains<=gcd.max)|talent.way_of_the_moknathal.enabled&buff.moknathal_tactics.down" );
  default_list -> add_action( "aspect_of_the_eagle" );
  default_list -> add_action( "snake_hunter,if=action.mongoose_bite.charges<=0&buff.mongoose_fury.remains" );
  default_list -> add_action( "mongoose_bite,if=buff.aspect_of_the_eagle.up&(charges>=2|charges>=1&cooldown.mongoose_bite.remains<=2)|(buff.mongoose_fury.up|cooldown.fury_of_the_eagle.remains<5|charges=3)" );
  default_list -> add_action( "lacerate,if=dot.lacerate.ticking&dot.lacerate.remains<=3|target.time_to_die>=5" );
  default_list -> add_action( "flanking_strike,if=talent.way_of_the_moknathal.enabled&(buff.moknathal_tactics.remains>=3)|!talent.way_of_the_moknathal.enabled" );
  default_list -> add_action( "butchery,if=spell_targets.butchery>=2" );
  default_list -> add_action( "carve,if=spell_targets.carve>=4" );
  default_list -> add_action( "spitting_cobra" );
  default_list -> add_action( "throwing_axes" );
  default_list -> add_action( "raptor_strike,if=!talent.throwing_axes.enabled&focus>75-cooldown.flanking_strike.remains*focus.regen" );
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
  if ( buffs.dire_beast -> up() )
    resource_gain( RESOURCE_FOCUS, buffs.dire_beast -> check_stack_value() * periodicity.total_seconds(), gains.dire_beast );
  if ( buffs.aspect_of_the_wild -> up() )
    resource_gain( RESOURCE_FOCUS, buffs.aspect_of_the_wild -> data().effectN( 2 ).resource( RESOURCE_FOCUS ) * periodicity.total_seconds(), gains.aspect_of_the_wild );
  if ( buffs.spitting_cobra -> up() )
    resource_gain( RESOURCE_FOCUS, buffs.spitting_cobra -> default_value * periodicity.total_seconds(), gains.spitting_cobra );
}

// hunter_t::composite_attack_power_multiplier ==============================

double hunter_t::composite_attack_power_multiplier() const
{
  double mult = player_t::composite_attack_power_multiplier();

  if ( buffs.moknathal_tactics -> check() )
    mult *= 1.0 + buffs.moknathal_tactics -> check_stack_value();

  return mult;
}

// hunter_t::composite_melee_crit_chance ===========================================

double hunter_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  if ( buffs.bullseye -> check() )
    crit += buffs.bullseye -> check_stack_value();

  if ( buffs.aspect_of_the_wild -> check() )
    crit += buffs.aspect_of_the_wild -> check_value();

  crit +=  specs.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// hunter_t::composite_spell_crit_chance ===========================================

double hunter_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  if ( buffs.bullseye -> check() )
    crit += buffs.bullseye -> check_stack_value();

  if ( buffs.aspect_of_the_wild -> check() )
    crit += buffs.aspect_of_the_wild -> check_value();

  crit +=  specs.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// hunter_t::composite_melee_haste ===========================================

double hunter_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.trueshot -> check() )
    h *= 1.0 / ( 1.0 + buffs.trueshot -> default_value );

  if ( buffs.t18_2p_rapid_fire -> check() )
    h *= 1.0 / ( 1.0 + buffs.t18_2p_rapid_fire -> default_value );

  return h;
}

// hunter_t::composite_spell_haste ===========================================

double hunter_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.trueshot -> check() )
    h *= 1.0 / ( 1.0 + buffs.trueshot -> default_value );

  if ( buffs.t18_2p_rapid_fire -> check() )
    h *= 1.0 / ( 1.0 + buffs.t18_2p_rapid_fire -> default_value );

  return h;
}

// hunter_t::composite_player_critical_damage_multiplier ====================

double hunter_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double cdm = player_t::composite_player_critical_damage_multiplier( s );

  // we use check() for rapid_fire becuase it's usage is reported from value() above
  if ( sets.has_set_bonus( HUNTER_MARKSMANSHIP, T17, B4 ) && buffs.trueshot -> check() )
  {
    // deadly_aim_driver
    double seconds_buffed = floor( buffs.trueshot -> elapsed( sim -> current_time() ).total_seconds() );
    // from Nimox
    cdm += bugs ? std::min(15.0, seconds_buffed) * 0.03
                : seconds_buffed * sets.set( HUNTER_MARKSMANSHIP, T17, B4 ) -> effectN( 1 ).percent();
  }

  if ( buffs.rapid_killing -> up() )
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

  if ( school == SCHOOL_PHYSICAL && artifacts.iron_talons.rank() )
    m *= 1.0 + artifacts.iron_talons.data().effectN( 1 ).percent();

  if ( artifacts.aspect_of_the_skylord.rank() && buffs.aspect_of_the_eagle -> check() )
    m *= 1.0 + find_spell( 203927 ) -> effectN( 1 ).percent();

  if ( artifacts.windflight_arrows.rank() )
    m *= 1.0 + artifacts.windflight_arrows.percent();

  if ( artifacts.voice_of_the_wild_gods.rank() )
    m *= 1.0 + artifacts.voice_of_the_wild_gods.percent();

  return m;
}

// composite_player_target_multiplier ====================================

double hunter_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double d = player_t::composite_player_target_multiplier( target, school );
  hunter_td_t* td = get_target_data( target );

  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) && td -> debuffs.t18_2pc_open_wounds -> up() )
    d *= 1.0 + td -> debuffs.t18_2pc_open_wounds -> value();

  if ( td -> debuffs.mark_of_helbrine -> up() )
    d *= 1.0 + td -> debuffs.mark_of_helbrine -> value();

  return d;
}

// hunter_t::composite_player_pet_damage_multiplier ======================

double hunter_t::composite_player_pet_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s );

  m *= beast_multiplier();

  return m;
}

// hunter_t::composite_mastery_value  ====================================

double hunter_t::composite_mastery_value() const
{
  double m = player_t::composite_mastery_value();

  return m;
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
    unique_gear::register_special_effect( 212574, sv_feet );
    unique_gear::register_special_effect( 225155, sv_ring );
    unique_gear::register_special_effect( 213154, sv_waist );
    unique_gear::register_special_effect( 212278, bm_feet );
    unique_gear::register_special_effect( 212329, bm_ring );
    unique_gear::register_special_effect( 207280, bm_waist );
    unique_gear::register_special_effect( 206889, mm_feet );
    unique_gear::register_special_effect( 224550, mm_ring );
    unique_gear::register_special_effect( 208912, mm_waist );
    unique_gear::register_special_effect( 226841, magnetized_blasting_cap_launcher );
    unique_gear::register_special_effect( 206332, wrist );
  }

  virtual void init( player_t* ) const override
  {
  }

  virtual void register_hotfixes() const override
  {
    /*
    hotfix::register_effect( "Hunter", "2016-09-23", "Bestial Wrath damage bonus increased to 25%", 10779 )
      .field( "base_value" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 25 )
      .verification_value( 20 );

    hotfix::register_effect( "Hunter", "2016-09-23", "Flanking Strike increased by 62%", 299019 )
      .field( "base_value" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.62 )
      .verification_value( 130 );

    hotfix::register_effect( "Hunter", "2016-09-23", "Barrage (non-Survival) damage reduced by 20%.", 139945 )
      .field( "base_value" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.2 )
      .verification_value( 100 );
      */

    hotfix::register_effect( "Hunter", "2016-10-10", "Instincts of the mongoose effect increased from 10% to 20%", 301586 )
      .field( "base_value" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 2 )
      .verification_value( 10 );

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
