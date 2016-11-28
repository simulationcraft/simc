// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// TODO + BlizzardFeatures + Bugs
// Subtlety
// - Dreadlord's Deceit doesn't work on weaponmastered Shuriken Storm (Blizzard Bug ?)
// - Insignia of Ravenholdt doesn't proc from Shuriken Storm nor Shuriken Toss (Blizzard Bug ?)
// - Akaari's Soul Rip action doesn't benefits from SoD and MoS (Blizzard Bug) despite showing the increases in the tooltip.
//
// Assassination
// - Balanced Blades [artifact power] spell data claims it's not flat modifier?
// - Poisoned Knives [artifact power] does the damage doubledip in any way?
// - Does Kingsbane debuff get procced 2x on Mutilate? (If both hands apply lethal poison).
// - Insignia of Ravenholdt doesn't proc from Fan of Knives nor Poisoned Knife nor Kingsbane (Blizzard Bug ?)

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

enum
{
  COMBO_POINT_MAX = 5
};

enum secondary_trigger_e
{
  TRIGGER_NONE = 0U,
  TRIGGER_DEATH_FROM_ABOVE,
  TRIGGER_WEAPONMASTER
};

struct rogue_t;
namespace actions
{
struct rogue_attack_state_t;
struct residual_damage_state_t;
struct rogue_poison_t;
struct rogue_attack_t;
struct rogue_poison_buff_t;
struct melee_t;
struct shadow_blade_t;
}

namespace buffs
{
struct marked_for_death_debuff_t;
}

enum current_weapon_e
{
  WEAPON_PRIMARY = 0u,
  WEAPON_SECONDARY
};

enum weapon_slot_e
{
  WEAPON_MAIN_HAND = 0u,
  WEAPON_OFF_HAND
};

struct weapon_info_t
{
  // State of the hand, i.e., primary or secondary weapon currently equipped
  current_weapon_e     current_weapon;
  // Pointers to item data
  const item_t*        item_data[ 2 ];
  // Computed weapon data
  weapon_t             weapon_data[ 2 ];
  // Computed stats data
  gear_stats_t         stats_data[ 2 ];
  // Callbacks, associated with special effects on each weapons
  std::vector<dbc_proc_callback_t*> cb_data[ 2 ];

  // Item data storage for secondary weapons
  item_t               secondary_weapon_data;

  // Protect against multiple initialization since the init is done in an action_t object.
  bool                 initialized;

  // Track secondary weapon uptime through a buff
  buff_t*              secondary_weapon_uptime;

  weapon_info_t() :
    current_weapon( WEAPON_PRIMARY ), initialized( false ), secondary_weapon_uptime( nullptr )
  {
    range::fill( item_data, nullptr );
  }

  weapon_slot_e slot() const;
  void initialize();
  void reset();

  // Enable/disable callbacks on the primary/secondary weapons.
  void callback_state( current_weapon_e weapon, bool state );
};

// ==========================================================================
// Rogue
// ==========================================================================

struct rogue_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* deadly_poison;
    dot_t* garrote;
    dot_t* killing_spree; // Strictly speaking, this should probably be on player
    dot_t* kingsbane;
    dot_t* mutilated_flesh; // Assassination T19 2PC
    dot_t* nightblade;
    dot_t* rupture;
  } dots;

  struct debuffs_t
  {
    buff_t* hemorrhage;
    buff_t* vendetta;
    buff_t* wound_poison;
    buff_t* crippling_poison;
    buff_t* leeching_poison;
    buff_t* agonizing_poison;
    buffs::marked_for_death_debuff_t* marked_for_death;
    buff_t* ghostly_strike;
    buff_t* garrote; // Hidden proxy buff for garrote to get Thuggee working easily(ish)
    buff_t* surge_of_toxins;
    buff_t* kingsbane;
    buff_t* blood_of_the_assassinated;
  } debuffs;

  rogue_td_t( player_t* target, rogue_t* source );

  bool poisoned() const
  {
    return dots.deadly_poison -> is_ticking() ||
           debuffs.wound_poison -> check() ||
           debuffs.crippling_poison -> check() ||
           debuffs.leeching_poison -> check() ||
           debuffs.agonizing_poison -> check();
  }
};

struct rogue_t : public player_t
{
  // Custom options
  std::vector<size_t> fixed_rtb;

  // Duskwalker footpads counter
  double df_counter;

  // Shadow techniques swing counter;
  unsigned shadow_techniques;

  // Venom Rush poison tracking
  unsigned poisoned_enemies;

  // Active
  attack_t* active_blade_flurry;
  actions::rogue_poison_t* active_lethal_poison;
  actions::rogue_poison_t* active_nonlethal_poison;
  action_t* active_main_gauche;
  action_t* weaponmaster_dot_strike;
  action_t* shadow_nova;
  action_t* second_shuriken;
  action_t* poison_knives;
  action_t* from_the_shadows_;
  action_t* poison_bomb;
  action_t* greed;
  action_t* soul_rip; // Akaari
  action_t* t19_2pc_assassination;

  // Legendary
  action_t* insignia_of_ravenholdt_;

  // Autoattacks
  action_t* auto_attack;
  actions::melee_t* melee_main_hand;
  actions::melee_t* melee_off_hand;
  actions::shadow_blade_t* shadow_blade_main_hand;
  actions::shadow_blade_t* shadow_blade_off_hand;

  // Data collection
  luxurious_sample_data_t* dfa_mh, *dfa_oh;

  // Experimental weapon swapping
  weapon_info_t weapon_data[ 2 ];

  // Blurred time cooldown shenanigans
  std::vector<cooldown_t*> blurred_time_cooldowns;

  // Buffs
  struct buffs_t
  {
    haste_buff_t* adrenaline_rush;
    buff_t* blade_flurry;
    buff_t* deadly_poison;
    buff_t* death_from_above;
    buff_t* feint;
    buff_t* killing_spree;
    buff_t* master_of_subtlety;
    buff_t* master_of_subtlety_passive;
    buff_t* opportunity; // TODO: Not its real name, need to see in game what it is
    buff_t* shadow_dance;
    buff_t* shadowstep;
    buff_t* sprint;
    buff_t* stealth;
    buff_t* subterfuge;
    buff_t* tot_trigger;
    buff_t* vanish;
    buff_t* wound_poison;

    // Ticking buffs
    buff_t* envenom;
    buff_t* slice_and_dice;

    // Legendary buffs
    buff_t* fof_fod; // Fangs of the Destroyer
    stat_buff_t* fof_p1; // Phase 1
    stat_buff_t* fof_p2;
    stat_buff_t* fof_p3;

    // Legendary 7.0 buffs
    buff_t* shivarran_symmetry;
    buff_t* greenskins_waterlogged_wristcuffs;
    buff_t* the_dreadlords_deceit;
    buff_t* the_dreadlords_deceit_driver;
    buff_t* mantle_of_the_master_assassin;
    buff_t* mantle_of_the_master_assassin_passive;

    buff_t* deceit;
    buff_t* shadow_strikes;
    buff_t* deathly_shadows;

    buff_t* elaborate_planning;
    haste_buff_t* alacrity;
    buff_t* symbols_of_death;
    buff_t* shadow_blades;
    buff_t* enveloping_shadows;
    buff_t* goremaws_bite;
    buff_t* hidden_blade;
    buff_t* curse_of_the_dreadblades;
    buff_t* blurred_time;
    buff_t* death;
    buff_t* t19_4pc_outlaw;
    buff_t* t19oh_8pc;
    buff_t* blunderbuss;
    buff_t* finality_eviscerate;
    buff_t* finality_nightblade;

    // Roll the bones
    buff_t* roll_the_bones;
    buff_t* jolly_roger;
    haste_buff_t* grand_melee;
    buff_t* shark_infested_waters;
    buff_t* true_bearing;
    buff_t* broadsides;
    buff_t* buried_treasure;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* adrenaline_rush;
    cooldown_t* garrote;
    cooldown_t* killing_spree;
    cooldown_t* shadow_dance;
    cooldown_t* sprint;
    cooldown_t* vanish;
    cooldown_t* between_the_eyes;
    cooldown_t* blind;
    cooldown_t* gouge;
    cooldown_t* cloak_of_shadows;
    cooldown_t* riposte;
    cooldown_t* grappling_hook;
    cooldown_t* cannonball_barrage;
    cooldown_t* marked_for_death;
    cooldown_t* death_from_above;
    cooldown_t* weaponmaster;
    cooldown_t* vendetta;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* adrenaline_rush;
    gain_t* combat_potency;
    gain_t* deceit;
    gain_t* energetic_recovery;
    gain_t* energy_refund;
    gain_t* murderous_intent;
    gain_t* overkill;
    gain_t* master_of_shadows;
    gain_t* shadow_strikes;
    gain_t* t17_2pc_assassination;
    gain_t* t17_4pc_assassination;
    gain_t* t17_2pc_subtlety;
    gain_t* t17_4pc_subtlety;
    gain_t* t18_2pc_assassination;
    gain_t* venomous_wounds;
    gain_t* venomous_wounds_death;
    gain_t* vitality;
    gain_t* energetic_stabbing;
    gain_t* urge_to_kill;
    gain_t* goremaws_bite;
    gain_t* curse_of_the_dreadblades;
    gain_t* relentless_strikes;
    gain_t* shadow_satyrs_walk;

    // CP Gains
    gain_t* empowered_fan_of_knives;
    gain_t* seal_fate;
    gain_t* legendary_daggers;
    gain_t* quick_draw;
    gain_t* broadsides;
    gain_t* ruthlessness;
    gain_t* shadow_techniques;
    gain_t* shadow_blades;
    gain_t* enveloping_shadows;
    gain_t* t19_4pc_subtlety;
  } gains;

  // Spec passives
  struct spec_t
  {
    // Shared
    const spell_data_t* shadowstep;

    // Generic
    const spell_data_t* subtlety_rogue;
    const spell_data_t* outlaw_rogue;

    // Assassination
    const spell_data_t* assassins_resolve;
    const spell_data_t* improved_poisons;
    const spell_data_t* seal_fate;
    const spell_data_t* venomous_wounds;
    const spell_data_t* vendetta;
    const spell_data_t* garrote;
    const spell_data_t* garrote_2;

    // Outlaw
    const spell_data_t* blade_flurry;
    const spell_data_t* combat_potency;
    const spell_data_t* roll_the_bones;
    const spell_data_t* ruthlessness;
    const spell_data_t* saber_slash;
    const spell_data_t* vitality;

    // Subtlety
    const spell_data_t* deepening_shadows;
    const spell_data_t* energetic_recovery; // TODO: Not used?
    const spell_data_t* relentless_strikes;
    const spell_data_t* shadow_blades;
    const spell_data_t* shadow_dance;
    const spell_data_t* shadow_techniques;
    const spell_data_t* symbols_of_death;
    const spell_data_t* eviscerate;
    const spell_data_t* eviscerate_2;
    const spell_data_t* shadowstrike;
    const spell_data_t* shadowstrike_2;
  } spec;

  // Spell Data
  struct spells_t
  {
    const spell_data_t* bag_of_tricks_driver;
    const spell_data_t* critical_strikes;
    const spell_data_t* death_from_above;
    const spell_data_t* fan_of_knives;
    const spell_data_t* fleet_footed;
    const spell_data_t* master_of_shadows;
    const spell_data_t* sprint;
    const spell_data_t* relentless_strikes_energize;
    const spell_data_t* ruthlessness_cp_driver;
    const spell_data_t* ruthlessness_driver;
    const spell_data_t* ruthlessness_cp;
    const spell_data_t* shadow_focus;
    const spell_data_t* subterfuge;
    const spell_data_t* tier18_2pc_combat_ar;
    const spell_data_t* insignia_of_ravenholdt;
  } spell;

  // Talents
  struct talents_t
  {
    // Shared - Level 45
    const spell_data_t* deeper_stratagem;
    const spell_data_t* anticipation;
    const spell_data_t* vigor;

    // Shared - Level 90
    const spell_data_t* alacrity;

    // Shared - Level 100
    const spell_data_t* marked_for_death;
    const spell_data_t* death_from_above;

    // Assassination/Subtlety - Level 30
    const spell_data_t* nightstalker;
    const spell_data_t* subterfuge;
    const spell_data_t* shadow_focus;

    // Assassination - Level 15
    const spell_data_t* master_poisoner;
    const spell_data_t* elaborate_planning;
    const spell_data_t* hemorrhage;

    // Assassination - Level 60
    const spell_data_t* thuggee;
    const spell_data_t* internal_bleeding;

    // Assassination - Level 90
    const spell_data_t* agonizing_poison;
    const spell_data_t* exsanguinate;

    // Assassination - Level 100
    const spell_data_t* venom_rush;

    // Outlaw - Level 15
    const spell_data_t* ghostly_strike;
    const spell_data_t* swordmaster;
    const spell_data_t* quick_draw;

    // Outlaw - Level 30 (NYI)
    const spell_data_t* hit_and_run;

    // Outlaw - Level 75 (Partially Implemented for Gouge)
    const spell_data_t* dirty_tricks;

    // Outlaw - Level 90
    const spell_data_t* cannonball_barrage;
    const spell_data_t* killing_spree;

    // Outlaw - Level 100
    const spell_data_t* slice_and_dice;

    // Subtlety - Level 15
    const spell_data_t* master_of_subtlety;
    const spell_data_t* weaponmaster;
    const spell_data_t* gloomblade;

    // Subtlety - Level 90
    const spell_data_t* premeditation;
    const spell_data_t* enveloping_shadows;

    // Subtlety - Level 100
    const spell_data_t* master_of_shadows;
  } talent;

  // Masteries
  struct masteries_t
  {
    const spell_data_t* executioner;
    const spell_data_t* main_gauche;
    const spell_data_t* potent_poisons;
  } mastery;

  // Artifact powers
  struct artifact_t
  {
    // Subtlety
    artifact_power_t goremaws_bite;
    artifact_power_t shadow_fangs;
    artifact_power_t the_quiet_knife;
    artifact_power_t demons_kiss;
    artifact_power_t gutripper;
    artifact_power_t precision_strike;
    artifact_power_t fortunes_bite;
    artifact_power_t soul_shadows;
    artifact_power_t energetic_stabbing;
    artifact_power_t catwalk;
    artifact_power_t second_shuriken;
    artifact_power_t akaaris_soul;
    artifact_power_t shadow_nova;
    artifact_power_t finality;
    artifact_power_t legionblade;

    // Assassination
    artifact_power_t kingsbane;
    artifact_power_t assassins_blades;
    artifact_power_t toxic_blades;
    artifact_power_t urge_to_kill;
    artifact_power_t shadow_walker;
    artifact_power_t bag_of_tricks;
    artifact_power_t master_alchemist;
    artifact_power_t master_assassin;
    artifact_power_t gushing_wound;
    artifact_power_t blood_of_the_assassinated;
    artifact_power_t balanced_blades;
    artifact_power_t poison_knives;
    artifact_power_t surge_of_toxins;
    artifact_power_t serrated_edge;
    artifact_power_t from_the_shadows;
    artifact_power_t slayers_precision;

    // Swashbuckler
    artifact_power_t curse_of_the_dreadblades;
    artifact_power_t cursed_edges;
    artifact_power_t black_powder;
    artifact_power_t fortune_strikes;
    artifact_power_t gunslinger;
    artifact_power_t blunderbuss;
    artifact_power_t fatebringer;
    artifact_power_t blade_dancer;
    artifact_power_t blurred_time;
    artifact_power_t greed;
    artifact_power_t hidden_blade;
    artifact_power_t fortunes_boon;
    artifact_power_t fates_thirst;
    artifact_power_t fortunes_strike;
    artifact_power_t cursed_steel;
  } artifact;

  // Procs
  struct procs_t
  {
    proc_t* deepening_shadows;
    proc_t* seal_fate;
    proc_t* t18_2pc_combat;
    proc_t* weaponmaster;

    proc_t* roll_the_bones_1;
    proc_t* roll_the_bones_2;
    proc_t* roll_the_bones_3;
    proc_t* roll_the_bones_6;
  } procs;

  struct prng_t
  {
    real_ppm_t* bag_of_tricks;
  } prng;

  struct legendary_t
  {
    const spell_data_t* duskwalker_footpads;
    const spell_data_t* denial_of_the_halfgiants;
    const spell_data_t* zoldyck_family_training_shackles;
    const spell_data_t* the_dreadlords_deceit;
    const spell_data_t* insignia_of_ravenholdt;
    const spell_data_t* mantle_of_the_master_assassin;
  } legendary;

  // Options
  uint32_t fof_p1, fof_p2, fof_p3;
  int initial_combo_points;

  rogue_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, ROGUE, name, r ),
    df_counter( 0 ),
    shadow_techniques( 0 ),
    poisoned_enemies( 0 ),
    active_blade_flurry( nullptr ),
    active_lethal_poison( nullptr ),
    active_nonlethal_poison( nullptr ),
    active_main_gauche( nullptr ),
    weaponmaster_dot_strike( nullptr ),
    shadow_nova( nullptr ),
    second_shuriken( nullptr ),
    poison_knives( nullptr ),
    from_the_shadows_( nullptr ),
    poison_bomb( nullptr ),
    greed( nullptr ),
    soul_rip( nullptr ),
    insignia_of_ravenholdt_( nullptr ),
    auto_attack( nullptr ), melee_main_hand( nullptr ), melee_off_hand( nullptr ),
    shadow_blade_main_hand( nullptr ), shadow_blade_off_hand( nullptr ),
    dfa_mh( nullptr ), dfa_oh( nullptr ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    spec( spec_t() ),
    spell( spells_t() ),
    talent( talents_t() ),
    mastery( masteries_t() ),
    procs( procs_t() ),
    prng( prng_t() ),
    legendary( legendary_t() ),
    fof_p1( 0 ), fof_p2( 0 ), fof_p3( 0 ),
    initial_combo_points( 0 )
  {
    // Cooldowns
    cooldowns.adrenaline_rush     = get_cooldown( "adrenaline_rush"     );
    cooldowns.garrote             = get_cooldown( "garrote"             );
    cooldowns.killing_spree       = get_cooldown( "killing_spree"       );
    cooldowns.shadow_dance        = get_cooldown( "shadow_dance"        );
    cooldowns.sprint              = get_cooldown( "sprint"              );
    cooldowns.vanish              = get_cooldown( "vanish"              );
    cooldowns.between_the_eyes    = get_cooldown( "between_the_eyes"    );
    cooldowns.blind               = get_cooldown( "blind"               );
    cooldowns.gouge               = get_cooldown( "gouge"               );
    cooldowns.cannonball_barrage  = get_cooldown( "cannon_ball_barrage" );
    cooldowns.cloak_of_shadows    = get_cooldown( "cloak_of_shadows"    );
    cooldowns.death_from_above    = get_cooldown( "death_from_above"    );
    cooldowns.grappling_hook      = get_cooldown( "grappling_hook"      );
    cooldowns.marked_for_death    = get_cooldown( "marked_for_death"    );
    cooldowns.riposte             = get_cooldown( "riposte"             );
    cooldowns.weaponmaster        = get_cooldown( "weaponmaster"        );
    cooldowns.vendetta            = get_cooldown( "vendetta"            );

    base.distance = 3;
    regen_type = REGEN_DYNAMIC;
    regen_caches[CACHE_HASTE] = true;
    regen_caches[CACHE_ATTACK_HASTE] = true;
  }

  // Character Definition
  void      init_spells() override;
  void      init_base_stats() override;
  void      init_gains() override;
  void      init_procs() override;
  void      init_scaling() override;
  void      init_resources( bool force ) override;
  bool      init_items() override;
  bool      init_special_effects() override;
  void      init_rng() override;
  bool      init_finished() override;
  void      create_buffs() override;
  void      create_options() override;
  void      copy_from( player_t* source ) override;
  std::string      create_profile( save_e stype ) override;
  void      init_action_list() override;
  void      reset() override;
  void      arise() override;
  void      combat_begin() override;
  void      regen( timespan_t periodicity ) override;
  timespan_t available() const override;
  action_t* create_action( const std::string& name, const std::string& options ) override;
  expr_t*   create_expression( action_t* a, const std::string& name_str ) override;
  resource_e primary_resource() const override { return RESOURCE_ENERGY; }
  role_e    primary_role() const override  { return ROLE_ATTACK; }
  stat_e    convert_hybrid_stat( stat_e s ) const override;

  double    composite_melee_speed() const override;
  double    composite_melee_haste() const override;
  double    composite_melee_crit_chance() const override;
  double    composite_spell_crit_chance() const override;
  double    composite_spell_haste() const override;
  double    matching_gear_multiplier( attribute_e attr ) const override;
  double    composite_attack_power_multiplier() const override;
  double    composite_player_multiplier( school_e school ) const override;
  double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double    energy_regen_per_second() const override;
  double    passive_movement_modifier() const override;
  double    temporary_movement_modifier() const override;

  bool poisoned_enemy( player_t* target, bool deadly_fade = false ) const;

  void trigger_auto_attack( const action_state_t* );
  void trigger_seal_fate( const action_state_t* );
  void trigger_main_gauche( const action_state_t*, double = 0 );
  void trigger_combat_potency( const action_state_t* );
  void trigger_energy_refund( const action_state_t* );
  void trigger_venomous_wounds( const action_state_t* );
  void trigger_blade_flurry( const action_state_t* );
  void trigger_ruthlessness_cp( const action_state_t* );
  void trigger_combo_point_gain( int, gain_t* gain = nullptr, action_t* action = nullptr );
  void spend_combo_points( const action_state_t* );
  bool trigger_t17_4pc_combat( const action_state_t* );
  void trigger_t19oh_8pc( const action_state_t* );
  void trigger_elaborate_planning( const action_state_t* );
  void trigger_alacrity( const action_state_t* );
  void trigger_deepening_shadows( const action_state_t* );
  void trigger_shadow_techniques( const action_state_t* );
  void trigger_weaponmaster( const action_state_t* );
  void trigger_energetic_stabbing( const action_state_t* );
  void trigger_second_shuriken( const action_state_t* );
  void trigger_surge_of_toxins( const action_state_t* );
  void trigger_poison_knives( const action_state_t* );
  void trigger_true_bearing( const action_state_t* );
  void trigger_exsanguinate( const action_state_t* );
  void trigger_relentless_strikes( const action_state_t* );
  void trigger_insignia_of_ravenholdt( action_state_t* );

  // Computes the composite Agonizing Poison stack multiplier for Assassination Rogue
  double agonizing_poison_stack_multiplier( const rogue_td_t* ) const;

  // On-death trigger for Venomous Wounds energy replenish
  void trigger_venomous_wounds_death( player_t* );

  double consume_cp_max() const
  { return COMBO_POINT_MAX + as<double>( talent.deeper_stratagem -> effectN( 1 ).base_value() ); }

  target_specific_t<rogue_td_t> target_data;

  virtual rogue_td_t* get_target_data( player_t* target ) const override
  {
    rogue_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new rogue_td_t( target, const_cast<rogue_t*>(this) );
    }
    return td;
  }

  static actions::rogue_attack_t* cast_attack( action_t* action )
  { return debug_cast<actions::rogue_attack_t*>( action ); }

  static const actions::rogue_attack_t* cast_attack( const action_t* action )
  { return debug_cast<const actions::rogue_attack_t*>( action ); }

  void swap_weapon( weapon_slot_e slot, current_weapon_e to_weapon, bool in_combat = true );
};

namespace actions { // namespace actions

// ==========================================================================
// Static Functions
// ==========================================================================

// break_stealth ============================================================

static void break_stealth( rogue_t* p )
{
  if ( p -> buffs.stealth -> check() )
    p -> buffs.stealth -> expire();

  if ( p -> buffs.vanish -> check() )
    p -> buffs.vanish -> expire();
}

// ==========================================================================
// Rogue Attack
// ==========================================================================

struct rogue_attack_state_t : public action_state_t
{
  int cp;
  bool exsanguinated;

  rogue_attack_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ), cp( 0 ), exsanguinated( false )
  { }

  void initialize() override
  { action_state_t::initialize(); cp = 0; exsanguinated = false; }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s ) << " cp=" << cp << " exsanguinated=" << exsanguinated;
    return s;
  }

  void copy_state( const action_state_t* o ) override
  {
    action_state_t::copy_state( o );
    const rogue_attack_state_t* st = debug_cast<const rogue_attack_state_t*>( o );
    cp = st -> cp;
    exsanguinated = st -> exsanguinated;
  }
};

struct rogue_attack_t : public melee_attack_t
{
  bool         requires_stealth;
  position_e   requires_position;
  weapon_e     requires_weapon;

  // Secondary triggered ability, due to Weaponmaster talent or Death from Above. Secondary
  // triggered abilities cost no resources or incur cooldowns.
  secondary_trigger_e secondary_trigger;

  // Akaari's Soul stats object. Created for every "opener" (requires_stealth = true) that does
  // damage. Swapped into the action when Akaari's Soul secondary trigger event executes.
  stats_t* akaari;

  // Affect flags for various dynamic effects
  struct
  {
    bool blurred_time;
    bool shadow_blades;
    bool ruthlessness;
    bool relentless_strikes;
    bool deepening_shadows;
    bool weaponmaster;
    bool ghostly_strike;
    bool vendetta;
    bool agonizing_poison;
    bool alacrity;
    bool adrenaline_rush_gcd;
  } affected_by;

  rogue_attack_t( const std::string& token, rogue_t* p,
                  const spell_data_t* s = spell_data_t::nil(),
                  const std::string& options = std::string() ) :
    melee_attack_t( token, p, s ),
    requires_stealth( false ), requires_position( POSITION_NONE ),
    requires_weapon( WEAPON_NONE ), secondary_trigger( TRIGGER_NONE ),
    akaari( nullptr )
  {
    parse_options( options );

    may_crit                  = true;
    may_glance                = false;
    special                   = true;
    tick_may_crit             = true;
    hasted_ticks              = false;

    memset( &affected_by, 0, sizeof( affected_by ) );

    for ( size_t i = 1; i <= s -> effect_count(); i++ )
    {
      const spelleffect_data_t& effect = s -> effectN( i );

      switch ( effect.type() )
      {
        case E_ADD_COMBO_POINTS:
          if ( energize_type != ENERGIZE_NONE )
          {
            energize_type = ENERGIZE_ON_HIT;
            energize_amount = effect.base_value();
            energize_resource = RESOURCE_COMBO_POINT;
          }
          break;
        default:
          break;
      }

      if ( effect.type() == E_APPLY_AURA && effect.subtype() == A_PERIODIC_DAMAGE )
        base_ta_adder = effect.bonus( player );
      else if ( effect.type() == E_SCHOOL_DAMAGE )
        base_dd_adder = effect.bonus( player );
    }
  }

  void init() override
  {
    melee_attack_t::init();

    // If the ability is a finisher, just disable weapon damage by default
    if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 )
    {
      weapon_multiplier = 0;
      weapon_power_mod = 0;
    }

    // Figure out the affected flags
    affected_by.blurred_time = p() -> artifact.blurred_time.rank() && ! background &&
                               cooldown -> duration > timespan_t::zero();
    affected_by.shadow_blades = data().affected_by( p() -> spec.shadow_blades -> effectN( 2 ) ) ||
                                data().affected_by( p() -> spec.shadow_blades -> effectN( 3 ) ) ||
                                data().affected_by( p() -> spec.shadow_blades -> effectN( 4 ) );

    affected_by.ruthlessness = base_costs[ RESOURCE_COMBO_POINT ] > 0;
    affected_by.relentless_strikes = base_costs[ RESOURCE_COMBO_POINT ] > 0;
    affected_by.deepening_shadows = base_costs[ RESOURCE_COMBO_POINT ] > 0;
    affected_by.ghostly_strike = data().affected_by( p() -> talent.ghostly_strike -> effectN( 5 ) );
    affected_by.vendetta = data().affected_by( p() -> spec.vendetta -> effectN( 1 ) );
    affected_by.weaponmaster = p() -> talent.weaponmaster -> ok() && harmful && special &&
                               ( weapon_multiplier > 0 || attack_power_mod.direct > 0 );
    affected_by.agonizing_poison = p() -> talent.agonizing_poison -> ok();
    affected_by.alacrity = base_costs[ RESOURCE_COMBO_POINT ] > 0;
    affected_by.adrenaline_rush_gcd = data().affected_by( p() -> buffs.adrenaline_rush -> data().effectN( 3 ) );
  }

  bool init_finished() override
  {
    if ( p() -> artifact.akaaris_soul.rank() && harmful && requires_stealth && ! background &&
         ( weapon_multiplier > 0 || attack_power_mod.direct > 0 ) )
    {
      akaari = player -> get_stats( name_str + "_akaari", this );
      akaari -> school = school;
      stats -> add_child( akaari );
    }

    if ( affected_by.blurred_time )
    {
      p() -> blurred_time_cooldowns.push_back( cooldown );
    }

    return melee_attack_t::init_finished();
  }

  void snapshot_state( action_state_t* state, dmg_e rt ) override
  {
    melee_attack_t::snapshot_state( state, rt );

    if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 )
    {
      double max_cp = std::min( player -> resources.current[ RESOURCE_COMBO_POINT ], p() -> consume_cp_max() );
      cast_state( state ) -> cp = static_cast<int>( max_cp );
    }
  }

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    if ( secondary_trigger != TRIGGER_NONE )
    {
      cd_duration = timespan_t::zero();
    }

    melee_attack_t::update_ready( cd_duration );
  }

  timespan_t gcd() const override
  {
    timespan_t t = melee_attack_t::gcd();

    if ( affected_by.adrenaline_rush_gcd &&
         t != timespan_t::zero() && p() -> buffs.adrenaline_rush -> check() )
    {
      t += p() -> buffs.adrenaline_rush -> data().effectN( 3 ).time_value();
    }

    return t;
  }


  bool stealthed()
  {
    return p() -> buffs.vanish -> check() || p() -> buffs.stealth -> check() || player -> buffs.shadowmeld -> check();
  }

  // Helper function for expressions. Returns the number of guaranteed generated combo points for
  // this ability, taking into account any potential buffs.
  virtual double generate_cp() const
  {
    double cp = 0;

    if ( energize_type != ENERGIZE_NONE && energize_resource == RESOURCE_COMBO_POINT )
    {
      cp += energize_amount;
    }

    if ( cp > 0 )
    {
      if ( p() -> buffs.broadsides -> check() )
      {
        cp += 1;
      }

      if ( affected_by.shadow_blades && p() -> buffs.shadow_blades -> check() )
      {
        cp += 1;
      }
    }

    return cp;
  }

  virtual bool procs_poison() const
  { return weapon != nullptr; }

  // Generic rules for proccing Main Gauche, used by rogue_t::trigger_main_gauche()
  virtual bool procs_main_gauche() const
  { return callbacks && ! proc && weapon != nullptr && weapon -> slot == SLOT_MAIN_HAND; }

  // Generic rules for proccing Combat Potency, used by rogue_t::trigger_combat_potency()
  virtual bool procs_combat_potency() const
  { return callbacks && ! proc && weapon != nullptr && weapon -> slot == SLOT_OFF_HAND; }

  // Generic rules for proccing Insignia of Ravenholdt, used by
  // rogue_t::trigger_insignia_of_ravenholdt()
  virtual bool procs_insignia_of_ravenholdt() const
  { return energize_type != ENERGIZE_NONE && energize_resource == RESOURCE_COMBO_POINT; }

  virtual double proc_chance_main_gauche() const
  { return p() -> cache.mastery_value(); }

  action_state_t* new_state() override
  { return new rogue_attack_state_t( this, target ); }

  static const rogue_attack_state_t* cast_state( const action_state_t* st )
  { return debug_cast< const rogue_attack_state_t* >( st ); }

  static rogue_attack_state_t* cast_state( action_state_t* st )
  { return debug_cast< rogue_attack_state_t* >( st ); }

  rogue_t* p()
  { return debug_cast< rogue_t* >( player ); }
  const rogue_t* p() const
  { return debug_cast< rogue_t* >( player ); }

  rogue_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  double cost() const override;
  void   execute() override;
  void   consume_resource() override;
  bool   ready() override;
  void   impact( action_state_t* state ) override;
  void   tick( dot_t* d ) override;

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return attack_power_mod.direct * cast_state( s ) -> cp;
    return melee_attack_t::attack_direct_power_coefficient( s );
  }

  double attack_tick_power_coefficient( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return attack_power_mod.tick * cast_state( s ) -> cp;
    return melee_attack_t::attack_tick_power_coefficient( s );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return spell_power_mod.direct * cast_state( s ) -> cp;
    return melee_attack_t::spell_direct_power_coefficient( s );
  }

  double spell_tick_power_coefficient( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return spell_power_mod.tick * cast_state( s ) -> cp;
    return melee_attack_t::spell_tick_power_coefficient( s );
  }

  double base_da_min( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return base_dd_min * cast_state( s ) -> cp;
    return melee_attack_t::base_da_min( s );
  }

  double base_da_max( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return base_dd_max * cast_state( s ) -> cp;
    return melee_attack_t::base_da_max( s );
  }

  double base_ta( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return base_td * cast_state( s ) -> cp;
    return melee_attack_t::base_ta( s );
  }

  double bonus_da( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return base_dd_adder * cast_state( s ) -> cp;
    return melee_attack_t::bonus_da( s );
  }

  double bonus_ta( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return base_ta_adder * cast_state( s ) -> cp;
    return melee_attack_t::bonus_ta( s );
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = melee_attack_t::composite_da_multiplier( state );

    if ( base_costs[ RESOURCE_COMBO_POINT ] && p() -> mastery.executioner -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = melee_attack_t::composite_ta_multiplier( state );

    if ( base_costs[ RESOURCE_COMBO_POINT ] && p() -> mastery.executioner -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = melee_attack_t::composite_target_multiplier( target );

    rogue_td_t* tdata = td( target );

    if ( affected_by.vendetta )
    {
      m *= 1.0 + tdata -> debuffs.vendetta -> value();
    }

    return m;
  }

  double composite_target_ta_multiplier( player_t* target ) const override
  {
    double m = melee_attack_t::composite_target_ta_multiplier( target );

    rogue_td_t* tdata = td( target );
    if ( dbc::is_school( school, SCHOOL_PHYSICAL ) && tdata -> debuffs.hemorrhage -> up() )
    {
      m *= tdata -> debuffs.hemorrhage -> check_value();
    }

    return m;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = melee_attack_t::composite_persistent_multiplier( state );

    if ( p() -> talent.nightstalker -> ok() &&
         ( p() -> buffs.stealth -> check() || p() -> buffs.shadow_dance -> check() ||
           p() -> buffs.vanish -> check() ) )
    {
      m *= 1.0 + ( p() -> talent.nightstalker -> effectN( 2 ).percent() +
                   p() -> spec.subtlety_rogue -> effectN( 1 ).percent() );
    }

    return m;
  }

  double action_multiplier() const override
  {
    double m = melee_attack_t::action_multiplier();

    if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 && harmful &&
         ( weapon_multiplier > 0 || attack_power_mod.direct > 0 || attack_power_mod.tick > 0 ) )
    {
      m *= 1.0 + p() -> talent.deeper_stratagem -> effectN( 4 ).percent();
    }

    return m;
  }

  double recharge_multiplier() const override
  {
    double m = melee_attack_t::recharge_multiplier();
    if ( p() -> buffs.blurred_time -> up() )
    {
      m *= 1.0 - p() -> buffs.blurred_time -> data().effectN( 1 ).percent();
    }

    return m;
  }

  timespan_t tick_time( const action_state_t* state ) const override
  {
    timespan_t tt = melee_attack_t::tick_time( state );

    if ( cast_state( state ) -> exsanguinated )
    {
      tt *= 1.0 / ( 1.0 + p() -> talent.exsanguinate -> effectN( 1 ).percent() );
    }

    return tt;
  }

  virtual double composite_poison_flat_modifier( const action_state_t* ) const
  { return 0.0; }

  expr_t* create_nightblade_expression();
  expr_t* create_expression( const std::string& name_str ) override;
};

struct secondary_ability_trigger_t : public event_t
{
  action_t* action;
  action_state_t* state;
  player_t* target;
  int cp;
  secondary_trigger_e source;

  secondary_ability_trigger_t( action_state_t* s, secondary_trigger_e source_, const timespan_t& delay = timespan_t::zero() ) :
    event_t( *s -> action -> sim, delay ), action( s -> action ), state( s ), target( nullptr ), cp( 0 ), source( source_ )
  { }

  secondary_ability_trigger_t( player_t* target, action_t* action, int cp, secondary_trigger_e source_, const timespan_t& delay = timespan_t::zero() ) :
    event_t( *action -> sim, delay ), action( action ), state( nullptr ), target( target ), cp( cp ), source( source_ )
  { }

  const char* name() const override
  { return "secondary_ability_trigger"; }

  void execute() override
  {
    actions::rogue_attack_t* attack = rogue_t::cast_attack( action );
    auto bg = attack -> background, d = attack -> dual, r = attack -> repeating;

    attack -> background = attack -> dual = true;
    attack -> repeating = false;
    attack -> secondary_trigger = source;
    if ( state )
    {
      attack -> pre_execute_state = state;
    }
    // No state, construct one by snapshotting, but also grap combo points from the event instead of
    // current CP amount.
    else
    {
      auto s = attack -> get_state();
      s -> target = target;
      attack -> snapshot_state( s, attack -> amount_type( s ) );
      actions::rogue_attack_t::cast_state( s ) -> cp = cp;

      attack -> pre_execute_state = s;
    }
    attack -> execute();
    attack -> background = bg;
    attack -> dual = d;
    attack -> repeating = r;
    attack -> secondary_trigger = TRIGGER_NONE;
    state = nullptr;
  }

  ~secondary_ability_trigger_t()
  { if ( state ) action_state_t::release( state ); }
};

// ==========================================================================
// Rogue Secondary Abilities
// ==========================================================================

struct main_gauche_t : public rogue_attack_t
{
  main_gauche_t( rogue_t* p ) :
    rogue_attack_t( "main_gauche", p, p -> find_spell( 86392 ) )
  {
    weapon          = &( p -> off_hand_weapon );
    special         = true;
    background      = true;
    may_crit        = true;
    proc = true; // it's proc; therefore it cannot trigger main_gauche for chain-procs

    base_multiplier *= 1.0 + p -> artifact.fortunes_strike.percent();
  }

  bool procs_combat_potency() const override
  { return true; }

  bool procs_poison() const override
  { return false; }
};

struct blade_flurry_attack_t : public rogue_attack_t
{
  blade_flurry_attack_t( rogue_t* p ) :
    rogue_attack_t( "blade_flurry_attack", p, p -> find_spell( 22482 ) )
  {
    may_miss = may_crit = proc = callbacks = may_dodge = may_parry = may_block = false;
    background = true;
    aoe = -1;
    weapon = &p -> main_hand_weapon;
    weapon_multiplier = 0;
    radius = 5;
    range = -1.0;

    snapshot_flags |= STATE_MUL_DA;
  }

  bool procs_main_gauche() const override
  { return false; }

  double composite_da_multiplier( const action_state_t* ) const override
  {
    double multiplier = p() -> spec.blade_flurry -> effectN( 3 ).percent();
    if ( p() -> buffs.shivarran_symmetry -> check() )
    {
      multiplier += p() -> buffs.shivarran_symmetry -> data().effectN( 1 ).percent();
    }
    return multiplier;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    rogue_attack_t::available_targets( tl );

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

struct internal_bleeding_t : public rogue_attack_t
{
  internal_bleeding_t( rogue_t* p ) :
    rogue_attack_t( "internal_bleeding", p, p -> find_spell( 154953 ) )
  {
    background = true;
    // Need to fake this here so it uses the correct AP coefficient
    base_costs[ RESOURCE_COMBO_POINT ] = 1; 
  }
};

struct weaponmaster_strike_t : public rogue_attack_t
{
  weaponmaster_strike_t( rogue_t* p ) :
    rogue_attack_t( "weaponmaster", p, p -> find_spell( 193536 ) )
  {
    background = true;
    callbacks = may_crit = may_miss = may_dodge = may_parry = false;
  }

  double target_armor( player_t* ) const override
  { return 0; }

  double calculate_direct_amount( action_state_t* ) const override
  { return base_dd_min; }
};

struct second_shuriken_t : public rogue_attack_t
{
  second_shuriken_t( rogue_t* p ) :
    rogue_attack_t( "second_shuriken", p, p -> find_spell( 197611 ) )
  {
    background = true;
    may_crit = true;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    // Stealth Buff
    if ( p() -> buffs.stealth -> up() || p() -> buffs.shadow_dance -> up() || p() -> buffs.vanish -> up() )
    {
      m *= 1.0 + 2.0; //FIXME Hotfix 09-24: Hardcoded to 200% until they add it in Spell Data like Shuriken Storm. Still the case as of 10/22 (7.1 22882).
    }

    return m;
  }
};

struct shadow_nova_t : public rogue_attack_t
{
  shadow_nova_t( rogue_t* p ) :
    rogue_attack_t( "shadow_nova", p, p -> artifact.shadow_nova.data().effectN( 1 ).trigger() )
  {
    background = true;
    aoe = -1;
  }
};

struct soul_rip_t : public rogue_attack_t
{
  soul_rip_t( rogue_t* p ) :
    rogue_attack_t( "soul_rip", p, p -> find_spell( 220893 ) )
  {
    background = true;
    callbacks = false;
    weapon_multiplier = weapon_power_mod = 0;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    //FIXME: Probably not the best way to do it, but since it's done by the pet it does ignore those two player multiplier.
    if ( p() -> buffs.master_of_subtlety -> check() || p() -> buffs.master_of_subtlety_passive -> check() )
    {
      m /= 1.0 + p() -> talent.master_of_subtlety -> effectN( 1 ).percent();
    }
    if ( p() -> buffs.symbols_of_death -> up() )
    {
      m /= p() -> buffs.symbols_of_death -> check_value();
    }

    return m; 
  }

  void init() override
  {
    rogue_attack_t::init();

    // Soul Rip in game is done by the pet, so presume it procs nothing
    memset( &affected_by, 0, sizeof( affected_by ) );
  }
};

struct poison_knives_t : public rogue_attack_t
{
  poison_knives_t( rogue_t* p ) :
    rogue_attack_t( "poison_knives", p, p -> find_spell( 192380 ) )
  {
    background = true;
    may_crit = callbacks = may_miss = false;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    m *= 1.0 + td( target ) -> debuffs.surge_of_toxins -> stack_value();

    return m;
  }

  void init() override
  {
    rogue_attack_t::init();

    snapshot_flags = update_flags = STATE_TGT_MUL_DA;
  }
};

struct from_the_shadows_damage_t : public rogue_attack_t
{
  from_the_shadows_damage_t( rogue_t* p ) :
    rogue_attack_t( "from_the_shadows", p, p -> find_spell( 192434 ) )
  {
    background = true;
    callbacks = false;
  }
};

struct from_the_shadows_driver_t : public rogue_attack_t
{
  from_the_shadows_damage_t* damage;

  from_the_shadows_driver_t( rogue_t* p ) :
    rogue_attack_t( "from_the_shadows_driver", p, p -> find_spell( 192432 ) ),
    damage( new from_the_shadows_damage_t( p ) )
  {
    background = quiet = true;
    callbacks = may_miss = may_crit = false;
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    damage -> target = d -> target;
    damage -> execute();
    damage -> execute();
  }
};

struct poison_bomb_t : public rogue_attack_t
{
  poison_bomb_t( rogue_t* p ) :
    rogue_attack_t( "poison_bomb", p, p -> find_spell( 192660 ) )
  {
    background = true;
    aoe = -1;
    base_multiplier *= 1.0 + p -> talent.master_poisoner -> effectN( 1 ).percent();
  }

  
  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    // Scale on Mastery since 09-24 Hotfix
    if ( p() -> mastery.potent_poisons -> ok() )
    {
      m *= 1.0 + p() -> cache.mastery_value();
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    m *= 1.0 + td( target ) -> debuffs.surge_of_toxins -> stack_value();

    return m;
  }
};

struct greed_t : public rogue_attack_t
{
  greed_t* oh;

  greed_t( rogue_t* p ) :
    rogue_attack_t( "greed", p, p -> find_spell( 202822 ) ),
    oh( new greed_t( p, "greed_oh", data().effectN( 3 ).trigger() ) )
  {
    aoe = -1;
    background = true;
    weapon = &( p -> main_hand_weapon );
    add_child( oh );
  }

  greed_t( rogue_t* p, const std::string& name, const spell_data_t* spell ) :
    rogue_attack_t( name, p, spell ),
    oh( nullptr )
  {
    aoe = -1;
    background = true;
    weapon = &( p -> off_hand_weapon );
  }

  bool procs_main_gauche() const override
  { return false; }

  // Greed off hand hit cannot proc combat potency, main hand would not proc anyhow
  bool procs_combat_potency() const override
  { return false; }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( oh )
    {
      oh -> target = execute_state -> target;
      oh -> schedule_execute();
    }
  }
};

// Legendary 7.0
struct insignia_of_ravenholdt_attack_t : public rogue_attack_t
{
  insignia_of_ravenholdt_attack_t( rogue_t* p ) :
    rogue_attack_t( "insignia_of_ravenholdt", p, p -> find_spell( 209043 ) )
  {
    background = true;
    aoe = -1;
  }

  double composite_da_multiplier( const action_state_t* ) const override
  {
    double m = p() -> spell.insignia_of_ravenholdt -> effectN( 1 ).percent();

    // Rogue Assassination Hidden Passive (Additive, it's +15% on the primary effect)
    if ( p() -> specialization() == ROGUE_ASSASSINATION )
    {
      m += p() -> find_spell( 137037 ) -> effectN( 1 ).percent(); 
    }

    return m;
  }

  double composite_target_multiplier( player_t* ) const override
  {
    // Target Modifier aren't taken in account for the proc (else double dip)
    return 1.0;
  }

};

// TODO: Check if this is an ignite or a normal dot
using namespace residual_action;
struct mutilated_flesh_t : public residual_periodic_action_t<melee_attack_t>
{
  mutilated_flesh_t( rogue_t* p ) :
    residual_periodic_action_t<melee_attack_t>( "mutilated_flesh", p, p -> find_spell( 211672 ) )
  {
    background = true;
  }
};

// ==========================================================================
// Poisons
// ==========================================================================

struct rogue_poison_t : public rogue_attack_t
{
  bool lethal_;
  double proc_chance_;

  rogue_poison_t( const std::string& token, rogue_t* p,
                  const spell_data_t* s = spell_data_t::nil() ) :
    actions::rogue_attack_t( token, p, s ),
    proc_chance_( 0 )
  {
    proc              = true;
    background        = true;
    trigger_gcd       = timespan_t::zero();
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;
    callbacks         = false;

    weapon_multiplier = 0;

    proc_chance_  = data().proc_chance();
    if ( s -> affected_by( p -> spec.improved_poisons -> effectN( 1 ) ) )
    {
      proc_chance_ += p -> spec.improved_poisons -> effectN( 1 ).percent();
    }
  }

  timespan_t execute_time() const override
  { return timespan_t::zero(); }

  virtual double proc_chance( const action_state_t* source_state ) const
  {
    double chance = proc_chance_;

    if ( p() -> buffs.envenom -> up() )
    {
      chance += p() -> buffs.envenom -> data().effectN( 2 ).percent();
    }

    const rogue_attack_t* attack = rogue_t::cast_attack( source_state -> action );
    chance += attack -> composite_poison_flat_modifier( source_state );

    return chance;
  }

  virtual void trigger( const action_state_t* source_state )
  {
    bool result = rng().roll( proc_chance( source_state ) );

    if ( sim -> debug )
      sim -> out_debug.printf( "%s attempts to proc %s, target=%s source_action=%s proc_chance=%.3f: %d",
          player -> name(), name(), source_state -> target -> name(), source_state -> action -> name(), proc_chance( source_state ), result );

    if ( ! result )
      return;

    target = source_state -> target;
    execute();
  }

  double action_da_multiplier() const override
  {
    double m = rogue_attack_t::action_da_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    m *= 1.0 + p() -> artifact.master_alchemist.percent();

    return m;
  }

  double action_ta_multiplier() const override
  {
    double m = rogue_attack_t::action_ta_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    m *= 1.0 + p() -> artifact.master_alchemist.percent();

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    m *= 1.0 + td( target ) -> debuffs.surge_of_toxins -> stack_value();

    return m;
  }

};

// Deadly Poison ============================================================

struct deadly_poison_t : public rogue_poison_t
{
  struct deadly_poison_dd_t : public rogue_poison_t
  {
    deadly_poison_dd_t( rogue_t* p ) :
      rogue_poison_t( "deadly_poison_instant", p, p -> find_spell( 113780 ) )
    {
      harmful = true;
      base_multiplier *= 1.0 + p -> talent.master_poisoner -> effectN( 1 ).percent();
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = rogue_attack_t::composite_target_multiplier( target );

      if ( p() -> legendary.zoldyck_family_training_shackles )
      {
        if ( target -> health_percentage() < p() -> legendary.zoldyck_family_training_shackles -> effectN( 2 ).base_value() )
        {
          m *= 1.0 + p() -> legendary.zoldyck_family_training_shackles -> effectN( 1 ).percent();
        }
      }

      return m;
    };
  };

  struct deadly_poison_dot_t : public rogue_poison_t
  {
    deadly_poison_dot_t( rogue_t* p ) :
      rogue_poison_t( "deadly_poison_dot", p, p -> find_specialization_spell( "Deadly Poison" ) -> effectN( 1 ).trigger() )
    {
      may_crit       = false;
      harmful        = true;
      base_multiplier *= 1.0 + p -> talent.master_poisoner -> effectN( 1 ).percent();
    }

    void impact( action_state_t* state ) override
    {
      if ( ! p() -> poisoned_enemy( state -> target ) && result_is_hit( state -> result ) )
      {
        p() -> poisoned_enemies++;
      }

      rogue_poison_t::impact( state );
    }

    void last_tick( dot_t* d ) override
    {
      player_t* t = d -> state -> target;

      rogue_poison_t::last_tick( d );

      // Due to DOT system behavior, deliver "Deadly Poison DOT fade event" as
      // a separate parmeter to poisoned_enemy() call.
      if ( ! p() -> poisoned_enemy( t, true ) )
      {
        p() -> poisoned_enemies--;
      }
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = rogue_attack_t::composite_target_multiplier( target );

      if ( p() -> legendary.zoldyck_family_training_shackles )
      {
        if ( target -> health_percentage() < p() -> legendary.zoldyck_family_training_shackles -> effectN( 2 ).base_value() )
        {
          m *= 1.0 + p() -> legendary.zoldyck_family_training_shackles -> effectN( 1 ).percent();
        }
      }

      return m;
    };
  };

  deadly_poison_dd_t*  proc_instant;
  deadly_poison_dot_t* proc_dot;

  deadly_poison_t( rogue_t* player ) :
    rogue_poison_t( "deadly_poison", player, player -> find_specialization_spell( "Deadly Poison" ) ),
    proc_instant( nullptr ), proc_dot( nullptr )
  {
    dual = true;
    may_miss = may_crit = false;

    proc_instant = new deadly_poison_dd_t( player );
    proc_dot     = new deadly_poison_dot_t( player );
  }

  void impact( action_state_t* state ) override
  {
    bool is_up = ( td( state -> target ) -> dots.deadly_poison -> is_ticking() != 0 );

    rogue_poison_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      proc_dot -> target = state -> target;
      proc_dot -> execute();
      if ( is_up )
      {
        proc_instant -> target = state -> target;
        proc_instant -> execute();
      }

      if ( td( state -> target ) -> dots.kingsbane -> is_ticking() )
      {
        td( state -> target ) -> debuffs.kingsbane -> trigger();
      }
    }
  }
};

// Wound Poison =============================================================

struct wound_poison_t : public rogue_poison_t
{
  struct wound_poison_dd_t : public rogue_poison_t
  {
    wound_poison_dd_t( rogue_t* p ) :
      rogue_poison_t( "wound_poison", p, p -> find_specialization_spell( "Wound Poison" ) -> effectN( 1 ).trigger() )
    {
      harmful          = true;
      base_multiplier *= 1.0 + p -> talent.master_poisoner -> effectN( 1 ).percent();
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = rogue_attack_t::composite_target_multiplier( target );

      if ( p() -> legendary.zoldyck_family_training_shackles )
      {
        if ( target -> health_percentage() < p() -> legendary.zoldyck_family_training_shackles -> effectN( 2 ).base_value() )
        {
          m *= 1.0 + p() -> legendary.zoldyck_family_training_shackles -> effectN( 1 ).percent();
        }
      }

      return m;
    };

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );

      if ( result_is_hit( state -> result ) )
      {
        td( state -> target ) -> debuffs.wound_poison -> trigger();

        if ( ! sim -> overrides.mortal_wounds )
          state -> target -> debuffs.mortal_wounds -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );

        if ( td( state -> target ) -> dots.kingsbane -> is_ticking() )
        {
          td( state -> target ) -> debuffs.kingsbane -> trigger();
        }
      }
    }
  };

  wound_poison_dd_t* proc_dd;

  wound_poison_t( rogue_t* player ) :
    rogue_poison_t( "wound_poison_driver", player, player -> find_specialization_spell( "Wound Poison" ) )
  {
    dual           = true;
    may_miss = may_crit = false;

    proc_dd = new wound_poison_dd_t( player );
  }

  void impact( action_state_t* state ) override
  {
    rogue_poison_t::impact( state );

    proc_dd -> target = state -> target;
    proc_dd -> execute();
  }
};

// Crippling poison =========================================================

struct crippling_poison_t : public rogue_poison_t
{
  struct crippling_poison_proc_t : public rogue_poison_t
  {
    crippling_poison_proc_t( rogue_t* rogue ) :
      rogue_poison_t( "crippling_poison", rogue, rogue -> find_spell( 3409 ) )
    { }

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );

      td( state -> target ) -> debuffs.crippling_poison -> trigger();
    }
  };

  crippling_poison_proc_t* proc;

  crippling_poison_t( rogue_t* player ) :
    rogue_poison_t( "crippling_poison_driver", player, player -> find_specialization_spell( "Crippling Poison" ) ),
    proc( new crippling_poison_proc_t( player ) )
  {
    dual = true;
    may_miss = may_crit = false;
  }

  void impact( action_state_t* state ) override
  {
    rogue_poison_t::impact( state );

    proc -> target = state -> target;
    proc -> execute();
  }
};

// Leeching poison =========================================================

struct leeching_poison_t : public rogue_poison_t
{
  struct leeching_poison_proc_t : public rogue_poison_t
  {
    leeching_poison_proc_t( rogue_t* rogue ) :
      rogue_poison_t( "leeching_poison", rogue, rogue -> find_spell( 112961 ) )
    { }

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );

      td( state -> target ) -> debuffs.leeching_poison -> trigger();
    }
  };

  leeching_poison_proc_t* proc;

  leeching_poison_t( rogue_t* player ) :
    rogue_poison_t( "leeching_poison_driver", player, player -> find_talent_spell( "Leeching Poison" ) ),
    proc( new leeching_poison_proc_t( player ) )
  {
    dual = true;
    may_miss = may_crit = false;
  }

  void impact( action_state_t* state ) override
  {
    rogue_poison_t::impact( state );

    proc -> target = state -> target;
    proc -> execute();
  }
};

// Numbing poison =========================================================

struct agonizing_poison_t : public rogue_poison_t
{
  struct agonizing_poison_proc_t : public rogue_poison_t
  {
    agonizing_poison_proc_t( rogue_t* rogue ) :
      rogue_poison_t( "agonizing_poison", rogue, rogue -> find_spell( 200803 ) )
    { }

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );

      td( state -> target ) -> debuffs.agonizing_poison -> trigger();
      if ( result_is_hit( state -> result ) &&
           td( state -> target ) -> dots.kingsbane -> is_ticking() )
      {
        td( state -> target ) -> debuffs.kingsbane -> trigger();
      }
    }
  };

  agonizing_poison_proc_t* proc;

  agonizing_poison_t( rogue_t* player ) :
    rogue_poison_t( "agonizing_poison_driver", player, player -> find_talent_spell( "Agonizing Poison" ) ),
    proc( new agonizing_poison_proc_t( player ) )
  {
    dual = true;
    may_miss = may_crit = false;
  }

  void impact( action_state_t* state ) override
  {
    rogue_poison_t::impact( state );

    proc -> target = state -> target;
    proc -> execute();
  }
};

// Apply Poison =============================================================

struct apply_poison_t : public action_t
{
  enum poison_e
  {
    POISON_NONE = 0,
    DEADLY_POISON,
    WOUND_POISON,
    CRIPPLING_POISON,
    LEECHING_POISON,
    AGONIZING_POISON
  };

  poison_e lethal_poison;
  poison_e nonlethal_poison;
  bool executed;

  apply_poison_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "apply_poison", p ),
    lethal_poison( POISON_NONE ), nonlethal_poison( POISON_NONE ),
    executed( false )
  {
    std::string lethal_str;
    std::string nonlethal_str;

    add_option( opt_string( "lethal", lethal_str ) );
    add_option( opt_string( "nonlethal", nonlethal_str ) );
    parse_options( options_str );
    ignore_false_positive = true;

    trigger_gcd = timespan_t::zero();
    harmful = false;

    if ( p -> main_hand_weapon.type != WEAPON_NONE || p -> off_hand_weapon.type != WEAPON_NONE )
    {
      // Default to agonizing -> deadly, if no option given
      if      ( lethal_str.empty()        ) lethal_poison = p -> talent.agonizing_poison -> ok() ? AGONIZING_POISON : DEADLY_POISON;
      else if ( lethal_str == "deadly"    ) lethal_poison = DEADLY_POISON;
      else if ( lethal_str == "wound"     ) lethal_poison = WOUND_POISON;
      else if ( lethal_str == "agonizing" ) lethal_poison = AGONIZING_POISON;

      if ( nonlethal_str == "crippling" ) nonlethal_poison = CRIPPLING_POISON;
      if ( nonlethal_str == "leeching"  ) nonlethal_poison = LEECHING_POISON;
    }

    if ( ! p -> active_lethal_poison )
    {
      if ( lethal_poison == DEADLY_POISON  ) p -> active_lethal_poison = new deadly_poison_t( p );
      if ( lethal_poison == WOUND_POISON   ) p -> active_lethal_poison = new wound_poison_t( p );
      if ( lethal_poison == AGONIZING_POISON ) p -> active_lethal_poison = new agonizing_poison_t( p );
    }

    if ( ! p -> active_nonlethal_poison )
    {
      if ( nonlethal_poison == CRIPPLING_POISON ) p -> active_nonlethal_poison = new crippling_poison_t( p );
      if ( nonlethal_poison == LEECHING_POISON  ) p -> active_nonlethal_poison = new leeching_poison_t( p );
    }
  }

  void reset() override
  {
    action_t::reset();

    executed = false;
  }

  virtual void execute() override
  {
    executed = true;

    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", player -> name(), name() );
  }

  virtual bool ready() override
  {
    if ( player -> specialization() != ROGUE_ASSASSINATION )
    {
      return false;
    }

    return ! executed;
  }
};

// ==========================================================================
// Attacks
// ==========================================================================

// rogue_attack_t::impact ===================================================

void rogue_attack_t::impact( action_state_t* state )
{
  melee_attack_t::impact( state );

  p() -> trigger_main_gauche( state, proc_chance_main_gauche() );
  p() -> trigger_combat_potency( state );
  p() -> trigger_blade_flurry( state );
  p() -> trigger_shadow_techniques( state );
  p() -> trigger_weaponmaster( state );
  p() -> trigger_surge_of_toxins( state );
  p() -> trigger_insignia_of_ravenholdt( state );

  if ( energize_type != ENERGIZE_NONE && energize_resource == RESOURCE_COMBO_POINT )
    p() -> trigger_seal_fate( state );

  if ( result_is_hit( state -> result ) )
  {
    if ( procs_poison() && p() -> active_lethal_poison )
      p() -> active_lethal_poison -> trigger( state );

    if ( procs_poison() && p() -> active_nonlethal_poison )
      p() -> active_nonlethal_poison -> trigger( state );

    // Legendary Daggers buff handling
    // Proc rates from: https://github.com/Aldriana/ShadowCraft-Engine/blob/master/shadowcraft/objects/proc_data.py#L504
    // Logic from: https://github.com/simulationcraft/simc/issues/1117
    double fof_chance = ( p() -> specialization() == ROGUE_ASSASSINATION ) ? 0.23139 : ( p() -> specialization() == ROGUE_OUTLAW ) ? 0.09438 : 0.28223;
    if ( state -> target && state -> target -> level() > 88 )
    {
      fof_chance *= ( 1.0 - 0.1 * ( state -> target -> level() - 88 ) );
    }
    if ( rng().roll( fof_chance ) )
    {
      p() -> buffs.fof_p1 -> trigger();
      p() -> buffs.fof_p2 -> trigger();
      p() -> buffs.fof_p3 -> trigger();

      if ( ! p() -> buffs.fof_fod -> check() && p() -> buffs.fof_p3 -> check() > 30 )
      {
        // Trigging FoF and the Stacking Buff are mutually exclusive
        if ( rng().roll( 1.0 / ( 51.0 - p() -> buffs.fof_p3 -> check() ) ) )
        {
          p() -> buffs.fof_fod -> trigger();
          p() -> trigger_combo_point_gain( 5, p() -> gains.legendary_daggers );
        }
      }
    }
  }
}

// rogue_attack_t::cost =====================================================

double rogue_attack_t::cost() const
{
  double c = melee_attack_t::cost();

  if ( c <= 0 )
    return 0;

  if ( p() -> talent.shadow_focus -> ok() &&
       ( p() -> buffs.stealth -> check() || p() -> buffs.vanish -> check() || p() -> buffs.shadow_dance -> check() ) )
  {
    c *= 1.0 + ( p() -> spell.shadow_focus -> effectN( 1 ).percent() +
                 p() -> spec.subtlety_rogue -> effectN( 3 ).percent() );
  }

  if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 )
  {
    c += p() -> artifact.fatebringer.value();
  }

  if ( c <= 0 )
    c = 0;

  return c;
}

// rogue_attack_t::consume_resource =========================================

void rogue_attack_t::consume_resource()
{
  // Abilities triggered as part of another ability (secondary triggers) do not consume resources
  if ( secondary_trigger != TRIGGER_NONE )
  {
    return;
  }

  melee_attack_t::consume_resource();

  p() -> spend_combo_points( execute_state );

  if ( result_is_miss( execute_state -> result ) && resource_consumed > 0 )
    p() -> trigger_energy_refund( execute_state );

  if ( resource_consumed > 0 && p() -> legendary.duskwalker_footpads )
  {
    p() -> df_counter += resource_consumed;
    while ( p() -> df_counter >= p() -> legendary.duskwalker_footpads -> effectN( 2 ).base_value() )
    {
      timespan_t adjustment = -timespan_t::from_seconds( p() -> legendary.duskwalker_footpads -> effectN( 1 ).base_value() );
      p() -> cooldowns.vendetta -> adjust( adjustment );
      p() -> df_counter -= p() -> legendary.duskwalker_footpads -> effectN( 2 ).base_value();
    }
  }
}

// rogue_attack_t::execute ==================================================

void rogue_attack_t::execute()
{
  melee_attack_t::execute();

  // T17 4PC combat has to occur before combo point gain, so we can get
  // Ruthlessness to function properly with Anticipation
  //bool combat_t17_4pc_triggered = p() -> trigger_t17_4pc_combat( execute_state );

  p() -> trigger_auto_attack( execute_state );

  p() -> trigger_ruthlessness_cp( execute_state );

  if ( energize_type_() == ENERGIZE_ON_HIT && energize_resource == RESOURCE_COMBO_POINT &&
    affected_by.shadow_blades && p() -> buffs.shadow_blades -> up() )
  {
    p() -> trigger_combo_point_gain( 1, p() -> gains.shadow_blades, this );
  }

  // Anticipation only refreshes Combo Points, if the Combat and Subtlety T17
  // 4pc set bonuses are not in effect. Note that currently in game, Shadow
  // Strikes (Sub 4PC) does not prevent the consumption of Anticipation, but
  // presuming here that it is a bug.
  //if ( ! combat_t17_4pc_triggered && ! p() -> buffs.shadow_strikes -> check() )
  //  p() -> trigger_anticipation_replenish( execute_state );

  // Subtlety T17 4PC set bonus processing on the "next finisher"
  if ( result_is_hit( execute_state -> result ) &&
       base_costs[ RESOURCE_COMBO_POINT ] > 0 &&
       p() -> buffs.shadow_strikes -> check() )
  {
    p() -> buffs.shadow_strikes -> expire();
    double cp = player -> resources.max[ RESOURCE_COMBO_POINT ] - player -> resources.current[ RESOURCE_COMBO_POINT ];

    if ( cp > 0 ) 
    {
      player -> resource_gain( RESOURCE_COMBO_POINT, cp, p() -> gains.t17_4pc_subtlety );
    }
  }

  p() -> trigger_relentless_strikes( execute_state );

  p() -> trigger_elaborate_planning( execute_state );
  p() -> trigger_alacrity( execute_state );
  p() -> trigger_t19oh_8pc( execute_state );

  if ( harmful && stealthed() )
  {
    player -> buffs.shadowmeld -> expire();

    if ( ! p() -> talent.subterfuge -> ok() )
      break_stealth( p() );
    // Check stealthed again after shadowmeld is popped. If we're still
    // stealthed, trigger subterfuge
    else if ( stealthed() && ! p() -> buffs.subterfuge -> check() )
      p() -> buffs.subterfuge -> trigger();
  }

  p() -> trigger_deepening_shadows( execute_state );
}

// rogue_attack_t::ready() ==================================================

inline bool rogue_attack_t::ready()
{
  if ( ! melee_attack_t::ready() )
    return false;

  if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 &&
       player -> resources.current[ RESOURCE_COMBO_POINT ] < base_costs[ RESOURCE_COMBO_POINT ] )
    return false;

  if ( requires_stealth )
  {
    if ( ! p() -> buffs.shadow_dance -> check() &&
         ! p() -> buffs.stealth -> check() &&
         ! player -> buffs.shadowmeld -> check() &&
         ! p() -> buffs.vanish -> check() &&
         ! p() -> buffs.subterfuge -> check() )
    {
      return false;
    }
  }

  if ( requires_position != POSITION_NONE )
    if ( p() -> position() != requires_position )
      return false;

  if ( requires_weapon != WEAPON_NONE )
    if ( ! weapon || weapon -> type != requires_weapon )
      return false;

  return true;
}

void rogue_attack_t::tick( dot_t* d )
{
  melee_attack_t::tick( d );

  p() -> trigger_weaponmaster( d -> state );
}

// Melee Attack =============================================================

struct melee_t : public rogue_attack_t
{
  int sync_weapons;
  bool first;

  melee_t( const char* name, rogue_t* p, int sw ) :
    rogue_attack_t( name, p ), sync_weapons( sw ), first( true )
  {
    school          = SCHOOL_PHYSICAL;
    background      = true;
    repeating       = true;
    trigger_gcd     = timespan_t::zero();
    special         = false;
    may_glance      = true;

    if ( p -> dual_wield() )
      base_hit -= 0.19;

    p -> auto_attack = this;
  }

  void init() override
  {
    rogue_attack_t::init();

    affected_by.vendetta = true;
  }

  void reset() override
  {
    rogue_attack_t::reset();

    first = true;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = rogue_attack_t::execute_time();
    if ( first )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 ) : timespan_t::zero();
    }
    return t;
  }

  virtual void execute() override
  {
    if ( first )
    {
      first = false;
    }
    rogue_attack_t::execute();
  }
};

// Auto Attack ==============================================================

struct auto_melee_attack_t : public action_t
{
  int sync_weapons;

  auto_melee_attack_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "auto_attack", p ),
    sync_weapons( 0 )
  {
    trigger_gcd = timespan_t::zero();

    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    parse_options( options_str );

    if ( p -> main_hand_weapon.type == WEAPON_NONE )
    {
      background = true;
      return;
    }

    p -> melee_main_hand = debug_cast<melee_t*>( p -> find_action( "auto_attack_mh" ) );
    if ( ! p -> melee_main_hand )
      p -> melee_main_hand = new melee_t( "auto_attack_mh", p, sync_weapons );

    p -> main_hand_attack = p -> melee_main_hand;
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> melee_off_hand = debug_cast<melee_t*>( p -> find_action( "auto_attack_oh" ) );
      if ( ! p -> melee_off_hand )
        p -> melee_off_hand = new melee_t( "auto_attack_oh", p, sync_weapons );

      p -> off_hand_attack = p -> melee_off_hand;
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
      p -> off_hand_attack -> id = 1;
    }
  }

  virtual void execute() override
  {
    player -> main_hand_attack -> schedule_execute();

    if ( player -> off_hand_attack )
      player -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready() override
  {
    if ( player -> is_moving() )
      return false;

    return ( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// Adrenaline Rush ==========================================================

struct adrenaline_rush_t : public rogue_attack_t
{
  adrenaline_rush_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "adrenaline_rush", p, p -> find_specialization_spell( "Adrenaline Rush" ), options_str )
  {
    harmful = may_miss = may_crit = false;

    cooldown -> duration += p -> artifact.fortunes_boon.time_value();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.adrenaline_rush -> trigger();
    p() -> buffs.blurred_time -> trigger();
  }
};

// Ambush ===================================================================

struct ambush_t : public rogue_attack_t
{
  ambush_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "ambush", p, p -> find_specialization_spell( "Ambush" ), options_str )
  {
    requires_stealth  = true;
  }

  bool procs_main_gauche() const override
  { return false; }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( weapon -> type == WEAPON_DAGGER )
      m *= 1.40;

    return m;
  }

  void execute() override
  {
    rogue_attack_t::execute();
    if ( p() -> buffs.broadsides -> up() )
    {
      p() -> trigger_combo_point_gain( p() -> buffs.broadsides -> data().effectN( 1 ).base_value(),
          p() -> gains.broadsides, this );
    }

    p() -> buffs.hidden_blade -> trigger();
  }
};

// Backstab =================================================================

struct backstab_t : public rogue_attack_t
{
  backstab_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "backstab", p, p -> find_specialization_spell( "Backstab" ), options_str )
  {
    requires_weapon = WEAPON_DAGGER;

    base_multiplier *= 1.0 + p -> artifact.the_quiet_knife.percent();
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_da_multiplier( state );

    if ( p() -> position() == POSITION_BACK )
    {
      m *= 1.0 + data().effectN( 4 ).percent();
    }

    return m;
  }

  bool ready() override
  {
    if ( p() -> talent.gloomblade -> ok() )
    {
      return false;
    }

    return rogue_attack_t::ready();
  }
};

// Between the Eyes =========================================================

struct between_the_eyes_t : public rogue_attack_t
{
  const spell_data_t* greenskins_waterlogged_wristcuffs; // 7.0 legendary Greenskin's Waterlogged Wristcuffs

  between_the_eyes_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "between_the_eyes", p, p -> find_specialization_spell( "Between the Eyes" ),
        options_str ), greenskins_waterlogged_wristcuffs( nullptr )
  {
    if ( ! maybe_ptr( player -> dbc.ptr ) )
    {
      crit_bonus_multiplier *= 1.0 + p -> spec.outlaw_rogue -> effectN( 1 ).percent();
    }
    else
    {
      crit_bonus_multiplier *= 1.0 + 2.0; // FIXME: Hardcoded for the moment, had issue to get the R2 (id: 235484)
    }
    base_multiplier *= 1.0 + p -> artifact.black_powder.percent();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p() -> buffs.true_bearing -> up() )
    {
      p() -> trigger_true_bearing( execute_state );
    }
    if ( greenskins_waterlogged_wristcuffs )
    {
      const rogue_attack_state_t* rs = rogue_attack_t::cast_state( execute_state );
      if ( rng().roll( greenskins_waterlogged_wristcuffs -> effectN( 1 ).percent() * rs -> cp ) )
      {
        p() -> buffs.greenskins_waterlogged_wristcuffs -> trigger();
      }
    }
  }
};

// Blade Flurry =============================================================

struct blade_flurry_t : public rogue_attack_t
{
  const spell_data_t* shivarran_symmetry; // 7.0 legendary Shivarran Symmetry

  blade_flurry_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "blade_flurry", p, p -> find_specialization_spell( "Blade Flurry" ), options_str ),
    shivarran_symmetry( nullptr )
  {
    harmful = may_miss = may_crit = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( ! p() -> buffs.blade_flurry -> check() )
    {
      p() -> buffs.blade_flurry -> trigger();
      if ( shivarran_symmetry )
      {
        p() -> buffs.shivarran_symmetry -> trigger();
      }
    }
    else
    {
      p() -> buffs.blade_flurry -> expire();
      p() -> buffs.shivarran_symmetry -> expire(); 
      // To be confirmed that turning Blade Flurry off removes also Shivarran 
      // Symmetry
    }
  }
};

// Cannonball Barrage =======================================================

struct cannonball_barrage_damage_t : public rogue_attack_t
{
  cannonball_barrage_damage_t( rogue_t* p ) :
    rogue_attack_t( "cannonball_barrage_damage", p, p -> find_spell( 185779 ) )
  {
    background = true;
  }

  double target_armor( player_t* ) const override
  { return 0; }
};

// TODO: Velocity is fubard in spell data, probably
struct cannonball_barrage_t : public rogue_attack_t
{
  cannonball_barrage_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "cannonball_barrage", p, p -> talent.cannonball_barrage, options_str )
  {
    tick_action = new cannonball_barrage_damage_t( p );
  }
};

// Curse of the Dreadblades =================================================

struct curse_of_the_dreadblades_t : public rogue_attack_t
{
  curse_of_the_dreadblades_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "curse_of_the_dreadblades", p, p -> artifact.curse_of_the_dreadblades, options_str )
  { }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.curse_of_the_dreadblades -> trigger();
  }
};

// Enveloping Shadows =======================================================

struct enveloping_shadows_t : public rogue_attack_t
{
  enveloping_shadows_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "enveloping_shadows", p, p -> talent.enveloping_shadows, options_str )
  {
    harmful = false;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    timespan_t duration = cast_state( execute_state ) -> cp * data().effectN( 1 ).period() * 2;

    p() -> buffs.enveloping_shadows -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
  }
};

// Envenom ==================================================================

struct envenom_t : public rogue_attack_t
{
  envenom_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "envenom", p, p -> find_specialization_spell( "Envenom" ), options_str )
  {
    weapon = &( p -> main_hand_weapon );
    dot_duration = timespan_t::zero();
    base_multiplier *= 1.0 + p -> artifact.toxic_blades.percent();
  }

  void consume_resource() override
  {
    rogue_attack_t::consume_resource();

    if ( secondary_trigger == TRIGGER_NONE &&
         p() -> sets.has_set_bonus( ROGUE_ASSASSINATION, T17, B4 ) )
    {
      p() -> trigger_combo_point_gain( 1, p() -> gains.t17_4pc_assassination, this );
    }
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    if ( p() -> sets.has_set_bonus( ROGUE_ASSASSINATION, T19, B4 ) )
    {
      size_t bleeds = 0;
      rogue_td_t* tdata = td( target );
      bleeds += tdata -> dots.garrote -> is_ticking();
      bleeds += tdata -> dots.rupture -> is_ticking();
      bleeds += tdata -> dots.mutilated_flesh -> is_ticking(); // TODO: Presumably?

      m *= 1.0 + p() -> sets.set( ROGUE_ASSASSINATION, T19, B4 ) -> effectN( 1 ).percent() * bleeds;
    }

    return m;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p() -> buffs.death_from_above -> up() )
      m *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 2 ).percent();

    return m;
  }

  double action_da_multiplier() const override
  {
    double m = rogue_attack_t::action_da_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.death_from_above -> check() )
      c *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 1 ).percent();

    if ( c < 0 )
      c = 0;

    return c;
  }

  virtual void execute() override
  {
    rogue_attack_t::execute();

    timespan_t envenom_duration = p() -> buffs.envenom -> data().duration() * ( 1 + cast_state( execute_state ) -> cp );

    p() -> buffs.envenom -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, envenom_duration );

    if ( p() -> buffs.death_from_above -> check() )
    {
      timespan_t extend_increase = p() -> buffs.envenom -> remains() * p() -> buffs.death_from_above -> data().effectN( 4 ).percent();
      p() -> buffs.envenom -> extend_duration( player, extend_increase );
    }

    if ( p() -> artifact.bag_of_tricks.rank() && p() -> prng.bag_of_tricks -> trigger() )
    {
      make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
          .target( execute_state -> target )
          .x( player -> x_position )
          .y( player -> y_position )
          .pulse_time( timespan_t::from_seconds( 0.5 ) ) //FIXME Hotfix 09-24: Hardcoded to 500ms instead of 1s since duration halved but damage conserved, still the case as of 10/22 (7.1 22882).
          .duration( p() -> spell.bag_of_tricks_driver -> duration() )
          .start_time( sim -> current_time() )
          .action( p() -> poison_bomb ), true );
    }
  }
};

// Eviscerate ===============================================================

struct eviscerate_t : public rogue_attack_t
{
  eviscerate_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "eviscerate", p, p -> spec.eviscerate, options_str )
  {
    weapon = &( player -> main_hand_weapon );
    base_crit += p -> artifact.gutripper.percent();
    base_multiplier *= 1.0 + p -> spec.eviscerate_2 -> effectN( 1 ).percent(); // As of 10/24 (7.1 22882), it is put as a Generic Modifier.
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p() -> buffs.death_from_above -> up() )
      m *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 2 ).percent();

    m *= 1.0 + p() -> buffs.finality_eviscerate -> stack_value();

    return m;
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.death_from_above -> check() )
      c *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 1 ).percent();

    if ( c < 0 )
      c = 0;

    return c;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p() -> sets.has_set_bonus( ROGUE_SUBTLETY, T18, B4 ) )
    {
      timespan_t v = timespan_t::from_seconds( -p() -> sets.set( ROGUE_SUBTLETY, T18, B4 ) -> effectN( 1 ).base_value() );
      v *= cast_state( execute_state ) -> cp;
      p() -> cooldowns.vanish -> adjust( v, false );
    }

    if ( p() -> buffs.finality_eviscerate -> check() )
    {
      p() -> buffs.finality_eviscerate -> expire();
    }
    else
    {
      p() -> buffs.finality_eviscerate -> trigger( cast_state( execute_state ) -> cp );
    }
  }
};

// Exsanguinate =============================================================

struct exsanguinate_t : public rogue_attack_t
{
  exsanguinate_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "exsanguinate", p, p -> talent.exsanguinate, options_str )
  { }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    p() -> trigger_exsanguinate( state );
  }
};

// Fan of Knives ============================================================

struct fan_of_knives_t: public rogue_attack_t
{
  fan_of_knives_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "fan_of_knives", p, p -> find_specialization_spell( "Fan of Knives" ), options_str )
  {
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier = 0;
    aoe = -1;
    energize_type     = ENERGIZE_ON_HIT;
    energize_resource = RESOURCE_COMBO_POINT;
    energize_amount   = data().effectN( 2 ).base_value();
  }

  bool procs_insignia_of_ravenholdt() const override
  { return false; }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    // The Dreadlord's Deceit Legendary
    if ( p() -> buffs.the_dreadlords_deceit -> up() )
    {
      m *= 1.0 + p() -> buffs.the_dreadlords_deceit -> check_stack_value();
      p() -> buffs.the_dreadlords_deceit -> expire();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    p() -> trigger_poison_knives( state );
  }
};

// Feint ====================================================================

struct feint_t : public rogue_attack_t
{
  feint_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "feint", p, p -> find_class_spell( "Feint" ), options_str )
  { }

  void execute() override
  {
    rogue_attack_t::execute();
    p() -> buffs.feint -> trigger();
  }
};

// Garrote ==================================================================

struct garrote_t : public rogue_attack_t
{
  garrote_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "garrote", p, p -> spec.garrote, options_str )
  {
    may_crit          = false;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_persistent_multiplier( state );

    if ( p() -> buffs.subterfuge -> up() )
    {
      m *= 1.0 + p() -> spell.subterfuge -> effectN( 2 ).percent();
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    if ( p() -> legendary.zoldyck_family_training_shackles )
    {
      if ( target -> health_percentage() < p() -> legendary.zoldyck_family_training_shackles -> effectN( 2 ).base_value() )
      {
        m *= 1.0 + p() -> legendary.zoldyck_family_training_shackles -> effectN( 1 ).percent();
      }
    }

    return m;
  };


  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t duration = rogue_attack_t::composite_dot_duration( s );

    if ( cast_state( s ) -> exsanguinated )
    {
      duration *= 1.0 / ( 1.0 + p() -> talent.exsanguinate -> effectN( 1 ).percent() );
    }

    return duration;
  }

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    if ( p() -> buffs.subterfuge -> check() )
    {
      cd_duration = timespan_t::zero();
    }

    rogue_attack_t::update_ready( cd_duration );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    td( execute_state -> target ) -> debuffs.garrote -> trigger();
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    p() -> trigger_venomous_wounds( d -> state );
  }
};

// Gouge ==================================================================
struct gouge_t : public rogue_attack_t
{
  gouge_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "gouge", p, p -> find_specialization_spell( "Gouge" ), options_str )
  {
    requires_stealth  = false;
  }

  double cost() const override
  {
    if ( p() -> talent.dirty_tricks -> ok() )
    {
      return 0;
    }

    return rogue_attack_t::cost();
  }

  void execute() override
  {
    rogue_attack_t::execute();
    if ( result_is_hit (execute_state -> result ) && p() -> buffs.broadsides -> up() )
    {
      p() -> trigger_combo_point_gain( p() -> buffs.broadsides -> data().effectN( 1 ).base_value(),
          p() -> gains.broadsides, this );
    }
  }
};

// Ghostly Strike ===========================================================

struct ghostly_strike_t : public rogue_attack_t
{
  ghostly_strike_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "ghostly_strike", p, p -> talent.ghostly_strike, options_str )
  {
    weapon = &( p -> main_hand_weapon );
  }

  bool procs_main_gauche() const override
  { return false; }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) && p() -> buffs.broadsides -> up() )
    {
      p() -> trigger_combo_point_gain( p() -> buffs.broadsides -> data().effectN( 2 ).base_value(),
          p() -> gains.broadsides, this );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      td( state -> target ) -> debuffs.ghostly_strike -> trigger();
    }
  }
};


// Gloomblade ===============================================================

struct gloomblade_t : public rogue_attack_t
{
  gloomblade_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "gloomblade", p, p -> talent.gloomblade, options_str )
  {
    requires_weapon = WEAPON_DAGGER;
    weapon = &( p -> main_hand_weapon );
    base_multiplier *= 1.0 + p -> artifact.the_quiet_knife.percent();
  }
};

// Goremaw's Bite ===========================================================

struct goremaws_bite_strike_t : public rogue_attack_t
{
  goremaws_bite_strike_t( rogue_t* p, const std::string& name, const spell_data_t* spell, weapon_t* w ) :
    rogue_attack_t( name, p, spell )
  {
    background = true;
    weapon = w;
  }

};

struct goremaws_bite_t:  public rogue_attack_t
{
  goremaws_bite_strike_t* mh, * oh;

  goremaws_bite_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "goremaws_bite", p, &( p -> artifact.goremaws_bite.data() ), options_str ),
    mh( new goremaws_bite_strike_t( p, "goremaws_bite_mh", p -> artifact.goremaws_bite.data().effectN( 1 ).trigger(), &( p -> main_hand_weapon ) ) ),
    oh( new goremaws_bite_strike_t( p, "goremaws_bite_oh", p -> artifact.goremaws_bite.data().effectN( 2 ).trigger(), &( p -> off_hand_weapon ) ) )
  {
    school = SCHOOL_SHADOW;
    weapon_multiplier = weapon_power_mod = 0;

    add_child( mh );
    add_child( oh );

    energize_type     = ENERGIZE_ON_HIT;
    energize_resource = RESOURCE_COMBO_POINT;
    energize_amount   = data().effectN( 4 ).trigger() -> effectN( 1 ).resource( RESOURCE_COMBO_POINT );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    mh -> target = target;
    mh -> schedule_execute();

    oh -> target = target;
    oh -> schedule_execute();

    if ( secondary_trigger != TRIGGER_WEAPONMASTER ) // As of 7.0.3.22810 it doesn't trigger the buff on the weaponmaster proc.
    {
      p() -> buffs.goremaws_bite -> trigger();
    }
  }
};

// Hemorrhage ===============================================================

struct hemorrhage_t : public rogue_attack_t
{
  hemorrhage_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "hemorrhage", p, p -> talent.hemorrhage, options_str )
  {
    weapon = &( p -> main_hand_weapon );
  }

  void impact( action_state_t* s ) override
  {
    rogue_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs.hemorrhage -> trigger();
    }
  }
};

// Kick =====================================================================

struct kick_t : public rogue_attack_t
{
  kick_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "kick", p, p -> find_class_spell( "Kick" ), options_str )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
    ignore_false_positive = true;
  }

  virtual bool ready() override
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return rogue_attack_t::ready();
  }
};

// Killing Spree ============================================================

struct killing_spree_tick_t : public rogue_attack_t
{
  killing_spree_tick_t( rogue_t* p, const char* name, const spell_data_t* s ) :
    rogue_attack_t( name, p, s )
  {
    school      = SCHOOL_PHYSICAL;
    background  = true;
    may_crit    = true;
    direct_tick = true;
  }

  bool procs_main_gauche() const override
  { return true; }
};

struct killing_spree_t : public rogue_attack_t
{
  melee_attack_t* attack_mh;
  melee_attack_t* attack_oh;

  killing_spree_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "killing_spree", p, p -> talent.killing_spree, options_str ),
    attack_mh( nullptr ), attack_oh( nullptr )
  {
    may_miss  = false;
    may_crit  = false;
    channeled = true;
    tick_zero = true;

    attack_mh = new killing_spree_tick_t( p, "killing_spree_mh", p -> find_spell( 57841 ) );
    attack_mh -> weapon = &( player -> main_hand_weapon );
    add_child( attack_mh );

    if ( player -> off_hand_weapon.type != WEAPON_NONE )
    {
      attack_oh = new killing_spree_tick_t( p, "killing_spree_oh", p -> find_spell( 57841 ) -> effectN( 2 ).trigger() );
      attack_oh -> weapon = &( player -> off_hand_weapon );
      add_child( attack_oh );
    }
  }

  timespan_t tick_time( const action_state_t* ) const override
  { return base_tick_time; }

  virtual void execute() override
  {
    p() -> buffs.killing_spree -> trigger();

    rogue_attack_t::execute();
  }

  virtual void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    attack_mh -> pre_execute_state = attack_mh -> get_state( d -> state );
    attack_mh -> execute();

    if ( attack_oh && result_is_hit( attack_mh -> execute_state -> result ) )
    {
      attack_oh -> pre_execute_state = attack_oh -> get_state( d -> state );
      attack_oh -> execute();
    }
  }
};

// Kingsbane ===========================================================

struct kingsbane_strike_t : public rogue_attack_t
{
  kingsbane_strike_t( rogue_t* p, const std::string& name, const spell_data_t* spell, weapon_t* w ) :
    rogue_attack_t( name, p, spell )
  {
    background = true;
    weapon = w;
    base_multiplier *= 1.0 + p -> talent.master_poisoner -> effectN( 3 ).percent();
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
    {
      m *= 1.0 + p() -> cache.mastery_value();
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    // TODO: Does this work?
    //m *= 1.0 + td( target ) -> debuffs.surge_of_toxins -> stack_value();
    if ( p() -> legendary.zoldyck_family_training_shackles )
    {
      if ( target -> health_percentage() < p() -> legendary.zoldyck_family_training_shackles -> effectN( 2 ).base_value() )
      {
        m *= 1.0 + p() -> legendary.zoldyck_family_training_shackles -> effectN( 1 ).percent();
      }
    }

    return m;
  }
};

struct kingsbane_t : public rogue_attack_t
{
  kingsbane_strike_t* mh, *oh;

  kingsbane_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "kingsbane", p, p -> artifact.kingsbane, options_str ),
    mh( new kingsbane_strike_t( p, "kingsbane_mh", p -> artifact.kingsbane.data().effectN( 2 ).trigger(), &( p -> main_hand_weapon ) ) ),
    oh( new kingsbane_strike_t( p, "kingsbane_oh", p -> artifact.kingsbane.data().effectN( 3 ).trigger(), &( p -> off_hand_weapon ) ) )
  {
    add_child( mh );
    add_child( oh );
    base_multiplier *= 1.0 + p -> talent.master_poisoner -> effectN( 3 ).percent();
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
    {
      m *= 1.0 + p() -> cache.mastery_value();
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    m *= 1.0 + td( target ) -> debuffs.kingsbane -> stack_value();
    // TODO: Does this work?
    //m *= 1.0 + td( target ) -> debuffs.surge_of_toxins -> stack_value();

    if ( p() -> legendary.zoldyck_family_training_shackles )
    {
      if ( target -> health_percentage() < p() -> legendary.zoldyck_family_training_shackles -> effectN( 2 ).base_value() )
      {
        m *= 1.0 + p() -> legendary.zoldyck_family_training_shackles -> effectN( 1 ).percent();
      }
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    mh -> target = state -> target;
    mh -> execute();
    oh -> target = state -> target;
    oh -> execute();
  }
};

// Shot Base ================================================================
// Shared base for pistol_shot_t and blunderbuss_t.

struct shot_base_t : public rogue_attack_t
{
  shot_base_t( rogue_t* p, const std::string& name, const spell_data_t* spell, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, spell, options_str )
  {
    base_crit += p -> artifact.gunslinger.percent();
  }

  double cost() const override
  {
    if ( p() -> buffs.opportunity -> check() )
    {
      return 0;
    }

    return rogue_attack_t::cost();
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p() -> talent.quick_draw -> ok() && p() -> buffs.opportunity -> up() )
    {
      m *= 1.0 + p() -> talent.quick_draw -> effectN( 1 ).percent();
    }

    if ( p() -> buffs.greenskins_waterlogged_wristcuffs -> up() )
    {
      m *= 1.0 + p() -> buffs.greenskins_waterlogged_wristcuffs -> data().effectN( 1 ).percent();
    }

    return m;
  }

  double generate_cp() const override
  {
    double g = rogue_attack_t::generate_cp();

    if ( g == 0.0 )
    {
      return g;
    }

    if ( p() -> talent.quick_draw -> ok() && p() -> buffs.opportunity -> check() )
    {
      g += p() -> talent.quick_draw -> effectN( 2 ).base_value();
    }

    return g;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    // Extra CP only if the initial attack grants CP (Blunderbuss damage events do not).
    if ( generate_cp() > 0 )
    {
      if ( result_is_hit( execute_state -> result ) && p() -> buffs.broadsides -> up() )
      {
        p() -> trigger_combo_point_gain( p() -> buffs.broadsides -> data().effectN( 3 ).base_value(),
            p() -> gains.broadsides, this );
      }

      if ( p() -> talent.quick_draw -> ok() && p() -> buffs.opportunity -> check() )
      {
        p() -> trigger_combo_point_gain( static_cast<int>( p() -> talent.quick_draw -> effectN( 2 ).base_value() ),
            p() -> gains.quick_draw, this );
      }

      if ( p() -> buffs.curse_of_the_dreadblades -> up() )
      {
        double n_cp = p() -> resources.max[ RESOURCE_COMBO_POINT ] - p() -> resources.current[ RESOURCE_COMBO_POINT ];
        if ( n_cp > 0 )
        {
          p() -> resource_gain( RESOURCE_COMBO_POINT, n_cp, p() -> gains.curse_of_the_dreadblades, this );
        }
      }
    }

    // Expire buffs.
    p() -> buffs.opportunity -> expire();
    p() -> buffs.greenskins_waterlogged_wristcuffs -> expire();
  }
};

// Blunderbuss ==============================================================
// Blunderbuss hits 4 times for 55%, 110%, 110%, and 110% AP respectively.

struct blunderbuss_damage_t : public shot_base_t
{
  blunderbuss_damage_t( rogue_t* p ) :
    shot_base_t( p, "blunderbuss", p -> find_spell( 202895 ) )
  {
    callbacks = false;
    background = dual = true;
    attack_power_mod.direct = data().effectN( 4 ).ap_coeff();

    // Main spell does the energizing
    energize_type = ENERGIZE_NONE;
    energize_amount = 0;
    base_costs[ RESOURCE_ENERGY ] = 0;
  }
};

struct blunderbuss_t : public shot_base_t
{
  blunderbuss_damage_t* damage;

  blunderbuss_t( rogue_t* p ) :
    shot_base_t( p, "blunderbuss", p -> find_spell( 202895 ) ),
    damage( new blunderbuss_damage_t( p ) )
  {
    background = true;
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
  }

  void execute() override
  {
    /* Snapshot and schedule the additional attacks *before* we call base_t.
    Some buffs are expired in base_t::execute and we need to calculate the
    damage with the presence of those buffs included. */
    for ( unsigned i = 0; i < 3; i++ )
    {
      action_state_t* s = damage -> get_state();
      s -> target = target;
      damage -> snapshot_state( s, DMG_DIRECT );
      damage -> schedule_execute( s );
    }

    shot_base_t::execute();

    p() -> buffs.blunderbuss -> decrement();
  }
};

// Pistol Shot ==============================================================

struct pistol_shot_t : public shot_base_t
{
  blunderbuss_t* blunderbuss;

  pistol_shot_t( rogue_t* p, const std::string& options_str ) :
    shot_base_t( p, "pistol_shot", p -> find_specialization_spell( "Pistol Shot" ), options_str ),
    blunderbuss( new blunderbuss_t( p ) )
  {
    add_child( blunderbuss );
  }

  void execute() override
  {
    if ( p() -> buffs.blunderbuss -> up() )
    {
      blunderbuss -> target = target;
      blunderbuss -> execute();
    }
    else
    {
      shot_base_t::execute();
    }
  }
};

// Run Through ==============================================================

struct run_through_t: public rogue_attack_t
{
  double ttt_multiplier; // 7.0 legendary Thraxi's Tricksy Treads multiplier

  run_through_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "run_through", p, p -> find_specialization_spell( "Run Through" ), options_str ),
    ttt_multiplier( 0 )
  {
    base_multiplier *= 1.0 + p -> artifact.fates_thirst.percent();
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p() -> buffs.death_from_above -> up() )
      m *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 2 ).percent();

    if ( ttt_multiplier > 0 )
    {
      double movement_speed = p() -> passive_movement_modifier() + p() -> temporary_movement_modifier();
      m *= 1.0 + movement_speed * ttt_multiplier;
    }

    return m;
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.death_from_above -> check() )
      c *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 1 ).percent();

    if ( p() -> buffs.deceit -> check() )
      c *= 1.0 + p() -> buffs.deceit -> data().effectN( 1 ).percent();

    if ( c < 0 )
      c = 0;

    return c;
  }

  void consume_resource() override
  {
    rogue_attack_t::consume_resource();

    if ( secondary_trigger == TRIGGER_NONE )
    {
      p() -> buffs.deceit -> expire();
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( rng().roll( p() -> artifact.greed.data().proc_chance() ) )
    {
      p() -> greed -> target = execute_state -> target;
      p() -> greed -> schedule_execute();
    }

    if ( p() -> buffs.true_bearing -> up() )
    {
      p() -> trigger_true_bearing( execute_state );
    }

    p() -> buffs.t19_4pc_outlaw -> trigger();
  }
};


// Marked for Death =========================================================

struct marked_for_death_t : public rogue_attack_t
{
  marked_for_death_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "marked_for_death", p, p -> find_talent_spell( "Marked for Death" ), options_str )
  {
    may_miss = may_crit = harmful = callbacks = false;
    energize_type = ENERGIZE_ON_CAST;
    if ( maybe_ptr( player -> dbc.ptr ) )
    {
      energize_amount += p -> talent.deeper_stratagem -> effectN( 6 ).base_value();
    }
  }

  // Defined after marked_for_death_debuff_t. Sigh.
  void impact( action_state_t* state ) override;
};



// Mutilate =================================================================

struct mutilate_strike_t : public rogue_attack_t
{
  const double& toxic_mutilator_crit_chance;

  mutilate_strike_t( rogue_t* p, const char* name, const spell_data_t* s, double& tmcc ) :
    rogue_attack_t( name, p, s ), toxic_mutilator_crit_chance( tmcc )
  {
    background  = true;
    may_miss = may_dodge = may_parry = false;

    base_multiplier *= 1.0 + p -> artifact.assassins_blades.percent();
  }

  bool procs_insignia_of_ravenholdt() const override
  { return true; }

  double composite_crit_chance() const override
  {
    double c = rogue_attack_t::composite_crit_chance();

    if ( p() -> buffs.envenom -> check() )
    {
      c += toxic_mutilator_crit_chance;
    }

    c += p() -> artifact.balanced_blades.percent();

    return c;
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    p() -> trigger_seal_fate( state );

    if ( p() -> sets.has_set_bonus( ROGUE_ASSASSINATION, T17, B2 ) && state -> result == RESULT_CRIT )
      p() -> resource_gain( RESOURCE_ENERGY,
                            p() -> sets.set( ROGUE_ASSASSINATION, T17, B2 ) -> effectN( 1 ).base_value(),
                            p() -> gains.t17_2pc_assassination,
                            this );

    if ( result_is_hit( state -> result ) && p() -> sets.has_set_bonus( ROGUE_ASSASSINATION, T19, B2 ) )
    {
      double amount = state -> result_amount * p() -> sets.set( ROGUE_ASSASSINATION, T19, B2 ) -> effectN( 1 ).percent();

      residual_action::trigger( p() -> t19_2pc_assassination, state -> target, amount );
    }
  }
};

struct mutilate_t : public rogue_attack_t
{
  rogue_attack_t* mh_strike;
  rogue_attack_t* oh_strike;
  double toxic_mutilator_crit_chance;

  mutilate_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "mutilate", p, p -> find_specialization_spell( "Mutilate" ), options_str ),
    mh_strike( nullptr ), oh_strike( nullptr ), toxic_mutilator_crit_chance( 0 )
  {
    may_crit = false;

    if ( p -> main_hand_weapon.type != WEAPON_DAGGER ||
         p ->  off_hand_weapon.type != WEAPON_DAGGER )
    {
      sim -> errorf( "Player %s attempting to execute Mutilate without two daggers equipped.", p -> name() );
      background = true;
    }

    mh_strike = new mutilate_strike_t( p, "mutilate_mh", data().effectN( 3 ).trigger(), toxic_mutilator_crit_chance );
    mh_strike -> weapon = &( p -> main_hand_weapon );
    add_child( mh_strike );

    oh_strike = new mutilate_strike_t( p, "mutilate_oh", data().effectN( 4 ).trigger(), toxic_mutilator_crit_chance );
    oh_strike -> weapon = &( p -> off_hand_weapon );
    add_child( oh_strike );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      mh_strike -> target = execute_state -> target;
      mh_strike -> execute();

      oh_strike -> target = execute_state -> target;
      oh_strike -> execute();
    }

    if ( execute_state -> target -> health_percentage() < 35 &&
         p() -> sets.has_set_bonus( ROGUE_ASSASSINATION, T18, B4 ) )
    {
      p() -> trigger_combo_point_gain( 1, p() -> gains.t18_2pc_assassination, this );
    }
  }
};

// Nightblade ===============================================================

struct nightblade_state_t : public rogue_attack_state_t
{
  bool finality;

  nightblade_state_t( action_t* action, player_t* target ) :
    rogue_attack_state_t( action, target ), finality( false )
  { }

  void initialize() override
  { rogue_attack_state_t::initialize(); finality = false; }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    rogue_attack_state_t::debug_str( s ) << " finality=" << finality;
    return s;
  }

  void copy_state( const action_state_t* o ) override
  {
    rogue_attack_state_t::copy_state( o );
    const nightblade_state_t* st = debug_cast<const nightblade_state_t*>( o );
    finality = st -> finality;
  }
};

struct nightblade_t : public rogue_attack_t
{
  nightblade_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "nightblade", p, p -> find_specialization_spell( "Nightblade" ), options_str )
  {
    may_crit = false;
    base_multiplier *= 1.0 + p -> artifact.demons_kiss.percent();
    if ( p -> talent.weaponmaster -> ok() )
    {
      add_child( p -> weaponmaster_dot_strike );
    }
  }

  action_state_t* new_state() override
  { return new nightblade_state_t( this, target ); }

  void snapshot_state( action_state_t* state, dmg_e type ) override
  {
    rogue_attack_t::snapshot_state( state, type );

    if ( p() -> buffs.finality_nightblade -> check() )
    {
      debug_cast<nightblade_state_t*>( state ) -> finality = true;
    }
  }

  void init() override
  {
    rogue_attack_t::init();

    affected_by.weaponmaster = true;
  }

  // Nightblade does not gain power from combo points like typical damage effects
  double attack_tick_power_coefficient( const action_state_t* s ) const override
  { return melee_attack_t::attack_tick_power_coefficient( s ); }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_persistent_multiplier( state );

    m *= 1.0 + p() -> buffs.finality_nightblade -> stack_value();

    return m;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p() -> sets.has_set_bonus( ROGUE_SUBTLETY, T18, B4 ) )
    {
      timespan_t v = timespan_t::from_seconds( -p() -> sets.set( ROGUE_SUBTLETY, T18, B4 ) -> effectN( 1 ).base_value() );
      v *= cast_state( execute_state ) -> cp;
      p() -> cooldowns.vanish -> adjust( v, false );
    }

    if ( ! p() -> buffs.finality_nightblade -> check() )
    {
      p() -> buffs.finality_nightblade -> trigger( cast_state( execute_state ) -> cp );
    }
    else
    {
      p() -> buffs.finality_nightblade -> expire();
    }
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t base_per_tick = data().effectN( 1 ).period();
    base_per_tick += timespan_t::from_seconds( p() -> sets.set( ROGUE_SUBTLETY, T19, B2 ) -> effectN( 1 ).base_value() );

    return data().duration() + base_per_tick * cast_state( s ) -> cp;
  }

  expr_t* create_expression( const std::string& name_str ) override
  {
    if ( util::str_compare_ci( name_str, "finality" ) )
    {
      return create_nightblade_expression();
    }

    return rogue_attack_t::create_expression( name_str );
  }
};

// Roll the Bones ===========================================================

struct roll_the_bones_t : public rogue_attack_t
{
  roll_the_bones_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "roll_the_bones", p, p -> spec.roll_the_bones, options_str )
  {
    harmful = false;
    dot_duration = timespan_t::zero();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    timespan_t d = ( cast_state( execute_state ) -> cp + 1 ) * p() -> buffs.roll_the_bones -> data().duration();

    p() -> buffs.roll_the_bones -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, d );

    if ( p() -> buffs.true_bearing -> up() )
    {
      p() -> trigger_true_bearing( execute_state );
    }
  }

  bool ready() override
  {
    if ( p() -> talent.slice_and_dice -> ok() )
    {
      return false;
    }

    return rogue_attack_t::ready();
  }
};

// Rupture ==================================================================

struct rupture_t : public rogue_attack_t
{
  rupture_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "rupture", p, p -> find_specialization_spell( "Rupture" ), options_str )
  {
    may_crit = false;
    base_multiplier *= 1.0 + p -> artifact.gushing_wound.percent();
    base_crit += p -> artifact.serrated_edge.percent();
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_crit_chance( target );

    if ( target -> health_percentage() < 35 )
    {
      m += p() -> sets.set( ROGUE_ASSASSINATION, T18, B2 ) -> effectN( 1 ).percent();
    }

    return m;
  }


  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    m *= 1.0 + td( target ) -> debuffs.blood_of_the_assassinated -> stack_value();

    if ( p() -> legendary.zoldyck_family_training_shackles )
    {
      if ( target -> health_percentage() < p() -> legendary.zoldyck_family_training_shackles -> effectN( 2 ).base_value() )
      {
        m *= 1.0 + p() -> legendary.zoldyck_family_training_shackles -> effectN( 1 ).percent();
      }
    }

    return m;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    const rogue_attack_state_t* state = cast_state( s );

    timespan_t duration = data().duration() * ( 1 + state -> cp );
    if ( state -> exsanguinated )
    {
      duration *= 1.0 / ( 1.0 + p() -> talent.exsanguinate -> effectN( 1 ).percent() );
    }

    return duration;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    td( execute_state -> target ) -> debuffs.blood_of_the_assassinated -> trigger();
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    p() -> trigger_venomous_wounds( d -> state );
  }

  expr_t* create_expression( const std::string& name ) override
  {
    if ( util::str_compare_ci( name, "new_duration" ) )
    {
      struct new_duration_expr_t : public expr_t
      {
        rupture_t* rupture;
        double cp_max_spend;
        double base_duration;

        new_duration_expr_t( rupture_t* a ) :
          expr_t( "new_duration" ), rupture( a ), cp_max_spend( a -> p() -> consume_cp_max() ),
          base_duration( a -> data().duration().total_seconds() )
        {}

        double evaluate() override
        {
          double duration = base_duration * ( 1.0 + std::min( cp_max_spend, rupture -> p() -> resources.current[ RESOURCE_COMBO_POINT ] ) );

          return std::min( duration * 1.3, duration + rupture -> td( rupture -> target ) -> dots.rupture -> remains().total_seconds() );
        }
      };

      return new new_duration_expr_t( this );
    }

    return rogue_attack_t::create_expression( name );
  }
};

// Saber Slash ==========================================================

struct saber_slash_t : public rogue_attack_t
{
  struct saberslash_proc_event_t : public event_t
  {
    rogue_t* rogue;
    saber_slash_t* spell;
    player_t* target;

    saberslash_proc_event_t( rogue_t* p, saber_slash_t* s, player_t* t ) :
      event_t( *p, s -> delay ), rogue( p ), spell( s ), target( t )
    {
    }

    const char* name() const override
    { return "saberslash_proc_execute"; }

    void execute() override
    {
      spell -> target = target;
      spell -> execute();
      spell -> saberslash_proc_event = nullptr;

      rogue -> buffs.blunderbuss -> trigger();
    }
  };

  saberslash_proc_event_t* saberslash_proc_event;
  timespan_t delay;

  saber_slash_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "saber_slash", p, p -> find_specialization_spell( "Saber Slash" ), options_str ),
    saberslash_proc_event( nullptr ), delay( data().duration() )
  {
    weapon = &( player -> main_hand_weapon );
    base_multiplier *= 1.0 + p -> artifact.cursed_edges.percent();
  }

  double proc_chance_main_gauche() const override
  {
    return rogue_attack_t::proc_chance_main_gauche() +
           p() -> sets.set( ROGUE_OUTLAW, T19, B2 ) -> effectN( 1 ).percent();
  }

  void reset() override
  {
    rogue_attack_t::reset();
    saberslash_proc_event = nullptr;
  }

  double cost() const override
  {
    if ( p() -> buffs.t19_4pc_outlaw -> check() || saberslash_proc_event )
    {
      return 0;
    }

    return rogue_attack_t::cost();
  }

  double saber_slash_proc_chance() const
  {
    double opportunity_proc_chance = data().effectN( 5 ).percent();
    opportunity_proc_chance += p() -> talent.swordmaster -> effectN( 1 ).percent();
    opportunity_proc_chance += p() -> buffs.jolly_roger -> stack_value();
    return opportunity_proc_chance;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( ! result_is_hit( execute_state -> result ) )
    {
      return;
    }

    if ( ! saberslash_proc_event &&
         ( p() -> buffs.opportunity -> trigger( 1, buff_t::DEFAULT_VALUE(), saber_slash_proc_chance() ) ||
         p() -> buffs.hidden_blade -> up() ) )
    {
      saberslash_proc_event = make_event<saberslash_proc_event_t>( *sim, p(), this, execute_state -> target );
    }

    p() -> buffs.hidden_blade -> decrement();
    p() -> buffs.t19_4pc_outlaw -> decrement();

    if ( p() -> buffs.broadsides -> up() )
    {
      p() -> trigger_combo_point_gain( p() -> buffs.broadsides -> data().effectN( 1 ).base_value(),
          p() -> gains.broadsides, this );
    }

    if ( p() -> buffs.curse_of_the_dreadblades -> up() )
    {
      double n_cp = p() -> resources.max[ RESOURCE_COMBO_POINT ] - p() -> resources.current[ RESOURCE_COMBO_POINT ];
      if ( n_cp > 0 )
      {
        p() -> resource_gain( RESOURCE_COMBO_POINT, n_cp, p() -> gains.curse_of_the_dreadblades, this );
      }
    }
  }
};

// Shadow Blades ============================================================

struct shadow_blade_t : public rogue_attack_t
{
  shadow_blade_t( const std::string& name_str, rogue_t* p, const spell_data_t* s, weapon_t* w ) :
    rogue_attack_t( name_str, p, s )
  {
    weapon = w;
    school  = SCHOOL_SHADOW;
    special = false;
    repeating = true;
    background = true;
    may_glance = false;
    base_execute_time = w -> swing_time;
  }

  void init() override
  {
    rogue_attack_t::init();

    affected_by.weaponmaster = true;
  }
};

struct shadow_blades_t : public rogue_attack_t
{
  shadow_blades_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadow_blades", p, p -> find_specialization_spell( "Shadow Blades" ), options_str )
  {
    harmful = may_miss = may_crit = false;

    if ( ! p -> shadow_blade_main_hand )
    {
      p -> shadow_blade_main_hand = new shadow_blade_t( "shadow_blade_mh",
          p,data().effectN( 1 ).trigger(), &( p -> main_hand_weapon ) );
      add_child( p -> shadow_blade_main_hand );
    }

    if ( ! p -> shadow_blade_off_hand && p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> shadow_blade_off_hand = new shadow_blade_t( "shadow_blade_offhand",
          p, p -> find_spell( data().effectN( 1 ).misc_value1() ), &( p -> off_hand_weapon ) );
      add_child( p -> shadow_blade_off_hand );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.shadow_blades -> trigger();
  }
};

// Shadow Dance =============================================================

struct shadow_dance_t : public rogue_attack_t
{
  cooldown_t* icd;

  shadow_dance_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadow_dance", p, p -> spec.shadow_dance, options_str ),
    icd( p -> get_cooldown( "shadow_dance_icd" ) )
  {
    harmful = may_miss = may_crit = false;
    dot_duration = timespan_t::zero(); // No need to have a tick here
    icd -> duration = data().cooldown();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.shadow_dance -> trigger();
    if ( p() -> sets.has_set_bonus( ROGUE_SUBTLETY, T17, B2 ) )
    {
      p() -> resource_gain( RESOURCE_ENERGY,
          p() -> sets.set( ROGUE_SUBTLETY, T17, B2 ) -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_ENERGY ),
          p() -> gains.t17_2pc_subtlety );
    }

    icd -> start();
  }

  bool ready() override
  {
    if ( icd -> down() )
    {
      return false;
    }

    return rogue_attack_t::ready();
  }
};

// Shadowstep ===============================================================

struct shadowstep_t : public rogue_attack_t
{
  shadowstep_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadowstep", p, p -> spec.shadowstep, options_str )
  {
    harmful = false;
    dot_duration = timespan_t::zero();
    base_teleport_distance = data().max_range();
    movement_directionality = MOVEMENT_OMNI;
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p() -> buffs.shadowstep -> trigger();
  }
};

// Shadowstrike =============================================================

struct shadowstrike_t : public rogue_attack_t
{
  const spell_data_t* shadow_satyrs_walk; // 7.0 legendary Shadow Satyr's Walk

  struct akaaris_soul_event_t : public event_t
  {
    rogue_t* rogue;
    player_t* target;
    akaaris_soul_event_t( rogue_t* r, player_t* target )
      : event_t(
            *r,
            timespan_t::from_seconds(
                r->artifact.akaaris_soul.data().effectN( 1 ).base_value() ) ),
        rogue( r ),
        target( target )
    {
    }

    const char* name() const override
    { return "akaaris_soul_event"; }

    void execute() override
    {
      rogue -> soul_rip -> target = target;
      rogue -> soul_rip -> schedule_execute();
    }
  };

  shadowstrike_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadowstrike", p, p -> spec.shadowstrike, options_str ),
    shadow_satyrs_walk( nullptr )
  {
    requires_weapon = WEAPON_DAGGER;
    requires_stealth = true;
    energize_amount += p -> talent.premeditation -> effectN( 2 ).base_value();
    base_multiplier *= 1.0 + p -> artifact.precision_strike.percent();
    range += p -> spec.shadowstrike_2 -> effectN( 1 ).base_value();

    if ( p -> soul_rip )
    {
      add_child( p -> soul_rip );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> trigger_energetic_stabbing( execute_state );

    if ( p() -> artifact.akaaris_soul.rank() )
    {
      make_event<akaaris_soul_event_t>( *sim, p(), execute_state -> target );
    }

    // TODO: Check if it triggers when the hit is failed
    // FIXME: Probably better to register the spell rather than doing find_spell here
    if ( maybe_ptr( p() -> dbc.ptr ) &&
         p() -> artifact.shadow_nova.rank() &&
         rng().roll( p() -> find_spell( 209781 ) -> proc_chance() ) )
    {
      p() -> shadow_nova -> schedule_execute();
    }

    p() -> buffs.death -> decrement();

    if ( result_is_hit( execute_state -> result ) && rng().roll( p() -> sets.set( ROGUE_SUBTLETY, T19, B4 ) -> proc_chance() ) )
    {
      p() -> trigger_combo_point_gain( p() -> sets.set( ROGUE_SUBTLETY, T19, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).base_value(),
          p() -> gains.t19_4pc_subtlety, this );
    }

    if ( shadow_satyrs_walk )
    {
      const spell_data_t* base_proc = p() -> find_spell( 224914 );
      // To be fixed
      // Distance set to 5y as a default value
      double distance = 5;
      double grant_energy = base_proc -> effectN( 1 ).base_value();
      while (distance > shadow_satyrs_walk -> effectN( 2 ).base_value())
      {
        grant_energy += shadow_satyrs_walk -> effectN( 1 ).base_value();
        distance -= shadow_satyrs_walk -> effectN( 2 ).base_value();
      }
      p() -> resource_gain( RESOURCE_ENERGY, grant_energy, p() -> gains.shadow_satyrs_walk );
    }
  }

  double composite_crit_chance() const override
  {
    if ( p() -> buffs.death -> up() )
    {
      return 1.0;
    }

    return rogue_attack_t::composite_crit_chance();
  }

  // TODO: Distance movement support, should teleport up to 30 yards, with distance targeting, next
  // to the target
  double composite_teleport_distance( const action_state_t* ) const override
  {
    if ( secondary_trigger != TRIGGER_NONE )
    {
      return 0;
    }

    return data().max_range();
  }
};

// Shuriken Storm ===========================================================

struct shuriken_storm_t: public rogue_attack_t
{
  shuriken_storm_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "shuriken_storm", p, p -> find_specialization_spell( "Shuriken Storm" ), options_str )
  {
    aoe = -1;
    energize_type = ENERGIZE_PER_HIT;
    energize_resource = RESOURCE_COMBO_POINT;
    energize_amount = 1;
  }

  bool procs_insignia_of_ravenholdt() const override
  { return false; }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    // Stealth Buff
    if ( p() -> buffs.stealth -> up() || p() -> buffs.shadow_dance -> up() || p() -> buffs.vanish -> up() )
    {
      m *= 1.0 + data().effectN( 3 ).percent();
    }

    // The Dreadlord's Deceit Legendary
    if ( p() -> buffs.the_dreadlords_deceit -> up() )
    {
      m *= 1.0 + p() -> buffs.the_dreadlords_deceit -> check_stack_value();
      p() -> buffs.the_dreadlords_deceit -> expire();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    p() -> trigger_second_shuriken( state );
  }

};

// Shuriken Toss ============================================================

struct shuriken_toss_t : public rogue_attack_t
{
  shuriken_toss_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shuriken_toss", p, p -> find_specialization_spell( "Shuriken Toss" ), options_str )
  { }

  bool procs_insignia_of_ravenholdt() const override
  { return false; }
};

// Slice and Dice ===========================================================

struct slice_and_dice_t : public rogue_attack_t
{
  slice_and_dice_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "slice_and_dice", p, p -> talent.slice_and_dice, options_str )
  {
    base_costs[ RESOURCE_COMBO_POINT ] = 1; // No resource cost in the spell .. sigh
    harmful = false;
    dot_duration = timespan_t::zero();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    double snd = p() -> buffs.slice_and_dice -> data().effectN( 1 ).percent();
    timespan_t snd_duration = ( cast_state( execute_state ) -> cp + 1 ) * p() -> buffs.slice_and_dice -> data().duration();

    p() -> buffs.slice_and_dice -> trigger( 1, snd, -1.0, snd_duration );
  }
};

// Sprint ===================================================================

struct sprint_t: public rogue_attack_t
{
  sprint_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "sprint", p, p -> spell.sprint, options_str )
  {
    harmful = callbacks = false;
    cooldown = p -> cooldowns.sprint;
    ignore_false_positive = true;

    cooldown -> duration += p -> artifact.shadow_walker.time_value();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.sprint -> trigger();
  }
};

// Symbols of Death =========================================================

struct symbols_of_death_t : public rogue_attack_t
{
  symbols_of_death_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "symbols_of_death", p, p -> spec.symbols_of_death, options_str )
  {
    harmful = callbacks = false;
    requires_stealth = true;

    dot_duration = timespan_t::zero(); // TODO: Check ticking in later builds
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.symbols_of_death -> trigger();
    p() -> buffs.death -> trigger();
  }
};


// Vanish ===================================================================

struct vanish_t : public rogue_attack_t
{
  vanish_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "vanish", p, p -> find_class_spell( "Vanish" ), options_str )
  {
    may_miss = may_crit = harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.vanish -> trigger();

    // Vanish stops autoattacks
    if ( p() -> main_hand_attack && p() -> main_hand_attack -> execute_event )
      event_t::cancel( p() -> main_hand_attack -> execute_event );

    if ( p() -> off_hand_attack && p() -> off_hand_attack -> execute_event )
      event_t::cancel( p() -> off_hand_attack -> execute_event );

    p() -> buffs.deathly_shadows -> trigger();

    if ( p() -> sets.has_set_bonus( ROGUE_SUBTLETY, T18, B2 ) )
    {
      p() -> trigger_combo_point_gain( p() -> sets.set( ROGUE_SUBTLETY, T18, B2 ) -> effectN( 1 ).base_value(),
                                       gain, this );
    }
  }
};

// Vendetta =================================================================

struct vendetta_t : public rogue_attack_t
{
  vendetta_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "vendetta", p, p -> find_specialization_spell( "Vendetta" ), options_str )
  {
    harmful = may_miss = may_crit = false;
    cooldown -> duration += p -> artifact.master_assassin.time_value();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    rogue_td_t* td = this -> td( execute_state -> target );

    td -> debuffs.vendetta -> trigger();

    if ( p() -> artifact.urge_to_kill.rank() )
    {
      p() -> resource_gain( RESOURCE_ENERGY, p() -> resources.max[ RESOURCE_ENERGY ],
          p() -> gains.urge_to_kill, this );
    }

    if ( p() -> from_the_shadows_ )
    {
      p() -> from_the_shadows_ -> target = execute_state -> target;
      p() -> from_the_shadows_ -> schedule_execute();
    }
  }
};

// Death From Above

struct death_from_above_driver_t : public rogue_attack_t
{
  action_t* ability;

  death_from_above_driver_t( rogue_t* p , action_t* finisher) :
    rogue_attack_t( "death_from_above_driver", p, p -> talent.death_from_above ),
    ability( finisher )
  {
    callbacks = tick_may_crit = false;
    quiet = dual = background = harmful = true;
    attack_power_mod.direct = 0;
    base_dd_min = base_dd_max = 0;
    base_costs[ RESOURCE_ENERGY ] = 0;
    base_costs[ RESOURCE_COMBO_POINT ] = 0;
  }

  void init() override
  {
    rogue_attack_t::init();

    memset( &affected_by, 0, sizeof( affected_by ) );
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    action_state_t* ability_state = ability -> get_state();
    ability -> snapshot_state( ability_state, DMG_DIRECT );
    ability_state -> target = d -> target;
    cast_state( ability_state ) -> cp = cast_state( d -> state ) -> cp;
    make_event<secondary_ability_trigger_t>( *sim, ability_state, TRIGGER_DEATH_FROM_ABOVE );

    p() -> buffs.death_from_above -> expire();
  }
};

struct death_from_above_t : public rogue_attack_t
{
  death_from_above_driver_t* driver;

  death_from_above_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "death_from_above", p, p -> talent.death_from_above, options_str )
  {
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;
    attack_power_mod.direct /= 5;

    base_tick_time = timespan_t::zero();
    dot_duration = timespan_t::zero();

    aoe = -1;

    // Create appropriate finisher for the players spec. Associate its stats
    // with the DfA's stats.
    action_t* finisher = nullptr;
    switch ( p -> specialization() )
    {
      case ROGUE_ASSASSINATION:
      {
        finisher = new envenom_t( p, "" );
        finisher -> stats = player -> get_stats( "envenom_dfa", finisher );
        stats -> add_child( finisher->stats );
        break;
      }
      // Subtlety needs special handling because of Finality artifact power
      case ROGUE_SUBTLETY:
      {
        finisher = new eviscerate_t( p, "" );
        finisher -> stats = player -> get_stats( "eviscerate_dfa", finisher );
        stats -> add_child( finisher -> stats );
        break;
      }
      case ROGUE_OUTLAW:
      {
        finisher = new run_through_t( p, "" );
        finisher -> stats = player -> get_stats( "run_through_dfa", finisher );
        stats -> add_child( finisher->stats );
        break;
      }
      default:
        background = true;
        assert(0);
    }
    if ( finisher )
    {
      driver = new death_from_above_driver_t( p, finisher );
    }
  }

  void init() override
  {
    rogue_attack_t::init();

    affected_by.ruthlessness = false;
    affected_by.relentless_strikes = false;
    affected_by.deepening_shadows = false;
    affected_by.alacrity = false;
    // 06/26/2016 Weaponmaster won't proc a second DFA, however the finisher can weaponmaster proc
    affected_by.weaponmaster = false;
  }

  void adjust_attack( attack_t* attack, const timespan_t& oor_delay )
  {
    if ( ! attack || ! attack -> execute_event )
    {
      return;
    }

    if ( attack -> execute_event -> remains() >= oor_delay )
    {
      return;
    }

    timespan_t next_swing = attack -> execute_event -> remains();
    timespan_t initial_next_swing = next_swing;
    // Fit the next autoattack swing into a set of increasing 500ms values,
    // which seems to be what is occurring with OOR+autoattacks in game.
    while ( next_swing <= oor_delay )
    {
      next_swing += timespan_t::from_millis( 500 );
    }

    if ( attack == player -> main_hand_attack )
    {
      p() -> dfa_mh -> add( ( next_swing - oor_delay ).total_seconds() );
    }
    else if ( attack == player -> off_hand_attack )
    {
      p() -> dfa_oh -> add( ( next_swing - oor_delay ).total_seconds() );
    }

    attack -> execute_event -> reschedule( next_swing );
    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s %s swing pushback: oor_time=%.3f orig_next=%.3f next=%.3f lands=%.3f",
          player -> name(), name(), oor_delay.total_seconds(), initial_next_swing.total_seconds(),
          next_swing.total_seconds(),
          attack -> execute_event -> occurs().total_seconds() );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.death_from_above -> trigger();

    timespan_t oor_delay = timespan_t::from_seconds( rng().gauss( 1.3, 0.025 ) );

    adjust_attack( player -> main_hand_attack, oor_delay );
    adjust_attack( player -> off_hand_attack, oor_delay );

    // DfA spell used to be implemented as a DoT where the finisher would
    // trigger on the first tick. This is no longer the case, but as a bandaid fix,
    // we're going to continue to model it as one, so force the DfA driver to
    // behave like a DoT.
    driver->base_tick_time = oor_delay;
    driver->dot_duration = oor_delay;

/*
    // Apparently DfA is out of range for ~0.8 seconds during the "attack", so
    // ensure that we have a swing timer of at least 800ms on both hands. Note
    // that this can sync autoattacks which also happens in game.
    if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
    {
      if ( player -> main_hand_attack -> execute_event -> remains() < timespan_t::from_seconds( 0.8 ) )
        player -> main_hand_attack -> execute_event -> reschedule( timespan_t::from_seconds( 0.8 ) );
    }

    if ( player -> off_hand_attack && player -> off_hand_attack -> execute_event )
    {
      if ( player -> off_hand_attack -> execute_event -> remains() < timespan_t::from_seconds( 0.8 ) )
        player -> off_hand_attack -> execute_event -> reschedule( timespan_t::from_seconds( 0.8 ) );
    }
*/
    action_state_t* driver_state = driver -> get_state( execute_state );
    driver_state -> target = target;
    driver -> schedule_execute( driver_state );
  }
};

// ==========================================================================
// Stealth
// ==========================================================================

struct stealth_t : public spell_t
{
  bool used;

  stealth_t( rogue_t* p, const std::string& options_str ) :
    spell_t( "stealth", p, p -> find_class_spell( "Stealth" ) ), used( false )
  {
    harmful = false;
    ignore_false_positive = true;

    parse_options( options_str );
  }

  virtual void execute() override
  {
    rogue_t* p = debug_cast< rogue_t* >( player );

    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", p -> name(), name() );

    p -> buffs.stealth -> trigger();
    used = true;
  }

  virtual bool ready() override
  {
    return ! used;
  }

  virtual void reset() override
  {
    spell_t::reset();
    used = false;
  }
};

// ==========================================================================
// Kidney Shot
// ==========================================================================

struct kidney_shot_t : public rogue_attack_t
{
  internal_bleeding_t* internal_bleeding;

  kidney_shot_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "kidney_shot", p, p -> find_class_spell( "Kidney Shot" ), options_str ),
    internal_bleeding( p -> talent.internal_bleeding ? new internal_bleeding_t( p ) : nullptr )
  {
    may_crit = false;
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;
    if ( internal_bleeding )
    {
      add_child( internal_bleeding );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( internal_bleeding )
    {
      internal_bleeding -> schedule_execute( internal_bleeding -> get_state( state ) );
    }
  }
};

// ==========================================================================
// Poisoned Knife
// ==========================================================================

struct poisoned_knife_t : public rogue_attack_t
{
  poisoned_knife_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "poisoned_knife", p, p -> find_specialization_spell( "Poisoned Knife" ), options_str )
  {
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;
  }

  bool procs_insignia_of_ravenholdt() const override
  { return false; }

  double composite_poison_flat_modifier( const action_state_t* ) const override
  { return 1.0; }
};

// ==========================================================================
// Experimental weapon swapping
// ==========================================================================

struct weapon_swap_t : public action_t
{
  enum swap_slot_e
  {
    SWAP_MAIN_HAND,
    SWAP_OFF_HAND,
    SWAP_BOTH
  };

  std::string slot_str, swap_to_str;

  swap_slot_e swap_type;
  current_weapon_e swap_to_type;
  rogue_t* rogue;

  weapon_swap_t( rogue_t* rogue_, const std::string& options_str ) :
    action_t( ACTION_OTHER, "weapon_swap", rogue_ ),
    swap_type( SWAP_MAIN_HAND ), swap_to_type( WEAPON_SECONDARY ),
    rogue( rogue_ )
  {
    may_miss = may_crit = may_dodge = may_parry = may_glance = callbacks = harmful = false;

    add_option( opt_string( "slot", slot_str ) );
    add_option( opt_string( "swap_to", swap_to_str ) );

    parse_options( options_str );

    if ( slot_str.empty() )
    {
      background = true;
    }
    else if ( util::str_compare_ci( slot_str, "main" ) ||
              util::str_compare_ci( slot_str, "main_hand" ) )
    {
      swap_type = SWAP_MAIN_HAND;
    }
    else if ( util::str_compare_ci( slot_str, "off" ) ||
              util::str_compare_ci( slot_str, "off_hand" ) )
    {
      swap_type = SWAP_OFF_HAND;
    }
    else if ( util::str_compare_ci( slot_str, "both" ) )
    {
      swap_type = SWAP_BOTH;
    }

    if ( util::str_compare_ci( swap_to_str, "primary" ) )
    {
      swap_to_type = WEAPON_PRIMARY;
    }
    else if ( util::str_compare_ci( swap_to_str, "secondary" ) )
    {
      swap_to_type = WEAPON_SECONDARY;
    }

    if ( swap_type != SWAP_BOTH )
    {
      if ( ! rogue -> weapon_data[ swap_type ].item_data[ swap_to_type ] )
      {
        background = true;
        sim -> errorf( "Player %s weapon_swap: No weapon info for %s/%s",
            player -> name(), slot_str.c_str(), swap_to_str.c_str() );
      }
    }
    else
    {
      if ( ! rogue -> weapon_data[ WEAPON_MAIN_HAND ].item_data[ swap_to_type ] ||
           ! rogue -> weapon_data[ WEAPON_OFF_HAND ].item_data[ swap_to_type ] )
      {
        background = true;
        sim -> errorf( "Player %s weapon_swap: No weapon info for %s/%s",
            player -> name(), slot_str.c_str(), swap_to_str.c_str() );
      }
    }
  }

  result_e calculate_result( action_state_t* ) const override
  { return RESULT_HIT; }

  block_result_e calculate_block_result( action_state_t* ) const override
  { return BLOCK_RESULT_UNBLOCKED; }

  void execute() override
  {
    action_t::execute();

    if ( swap_type == SWAP_MAIN_HAND )
    {
      rogue -> swap_weapon( WEAPON_MAIN_HAND, swap_to_type );
    }
    else if ( swap_type == SWAP_OFF_HAND )
    {
      rogue -> swap_weapon( WEAPON_OFF_HAND, swap_to_type );
    }
    else if ( swap_type == SWAP_BOTH )
    {
      rogue -> swap_weapon( WEAPON_MAIN_HAND, swap_to_type );
      rogue -> swap_weapon( WEAPON_OFF_HAND, swap_to_type );
    }
  }

  bool ready() override
  {
    if ( swap_type == SWAP_MAIN_HAND &&
         rogue -> weapon_data[ WEAPON_MAIN_HAND ].current_weapon == swap_to_type )
    {
      return false;
    }
    else if ( swap_type == SWAP_OFF_HAND &&
              rogue -> weapon_data[ WEAPON_OFF_HAND ].current_weapon == swap_to_type )
    {
      return false;
    }
    else if ( swap_type == SWAP_BOTH &&
              rogue -> weapon_data[ WEAPON_MAIN_HAND ].current_weapon == swap_to_type &&
              rogue -> weapon_data[ WEAPON_OFF_HAND ].current_weapon == swap_to_type )
    {
      return false;
    }

    return action_t::ready();
  }

};

} // end namespace actions

// ==========================================================================
// Expressions
// ==========================================================================

struct exsanguinated_expr_t : public expr_t
{
  action_t* action;

  exsanguinated_expr_t( action_t* a ) :
    expr_t( "exsanguinated_expr" ), action( a )
  { }

  double evaluate() override
  {
    dot_t* d = action -> get_dot( action -> target );
    return d -> is_ticking() && actions::rogue_attack_t::cast_state( d -> state ) -> exsanguinated;
  }
};

// rogue_attack_t::create_nightblade_expression ==============================

expr_t* actions::rogue_attack_t::create_nightblade_expression()
{
  return make_fn_expr( "finality", [ this ]() {
    rogue_td_t* td_ = td( target );
    if ( ! td_ -> dots.nightblade -> is_ticking() )
    {
      return 0.0;
    }

    return debug_cast<const nightblade_state_t*>( td_ -> dots.nightblade -> state ) -> finality
      ? 1.0
      : 0.0;
  } );
}

// rogue_attack_t::create_expression =========================================

expr_t* actions::rogue_attack_t::create_expression( const std::string& name_str )
{
  if ( util::str_compare_ci( name_str, "cp_gain" ) )
  {
    return make_mem_fn_expr( "cp_gain", *this, &rogue_attack_t::generate_cp );
  }
  // Rupture and Garrote APL lines using "exsanguinated"
  else if ( util::str_compare_ci( name_str, "exsanguinated" ) &&
            ( data().id() == 1943 || data().id() == 703 ) )
  {
    return new exsanguinated_expr_t( this );
  }
  else if ( util::str_compare_ci( name_str, "bleeds" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      rogue_td_t* tdata = td( target );
      return tdata -> dots.garrote -> is_ticking() +
             tdata -> dots.rupture -> is_ticking() +
             tdata -> dots.mutilated_flesh -> is_ticking();
    } );
  }
  else if ( util::str_compare_ci( name_str, "dot.nightblade.finality" ) )
  {
    return create_nightblade_expression();
  }

  return melee_attack_t::create_expression( name_str );
}

// ==========================================================================
// Secondary weapon handling
// ==========================================================================

weapon_slot_e weapon_info_t::slot() const
{
  if ( item_data[ WEAPON_PRIMARY ] -> slot == SLOT_MAIN_HAND )
  {
    return WEAPON_MAIN_HAND;
  }
  else
  {
    return WEAPON_OFF_HAND;
  }
}

void weapon_info_t::callback_state( current_weapon_e weapon, bool state )
{
  sim_t* sim = item_data[ WEAPON_PRIMARY ] -> sim;

  for ( size_t i = 0, end = cb_data[ weapon ].size(); i < end; ++i )
  {
    if ( state )
    {
      cb_data[ weapon ][ i ] -> activate();
      if ( cb_data[ weapon ][ i ] -> rppm )
      {
        cb_data[ weapon ][ i ] -> rppm -> set_last_trigger_success( sim -> current_time() );
        cb_data[ weapon ][ i ] -> rppm -> set_last_trigger_attempt( sim -> current_time() );
      }

      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s enabling callback %s on item %s",
            item_data[ WEAPON_PRIMARY ] -> player -> name(),
            cb_data[ weapon ][ i ] -> effect.name().c_str(),
            item_data[ weapon ] -> name() );
      }
    }
    else
    {
      cb_data[ weapon ][ i ] -> deactivate();
      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s disabling callback %s on item %s",
            item_data[ WEAPON_PRIMARY ] -> player -> name(),
            cb_data[ weapon ][ i ] -> effect.name().c_str(),
            item_data[ weapon ] -> name() );
      }
    }
  }
}

void weapon_info_t::initialize()
{
  if ( initialized )
  {
    return;
  }

  rogue_t* rogue = debug_cast<rogue_t*>( item_data[ WEAPON_PRIMARY ] -> player );

  // Compute stats and initialize the callback data for the weapon. This needs to be done
  // reasonably late (currently in weapon_swap_t action init) to ensure that everything has been
  // initialized.
  if ( item_data[ WEAPON_PRIMARY ] )
  {
    // Find primary weapon callbacks from the actor list of all callbacks
    for ( size_t i = 0; i < item_data[ WEAPON_PRIMARY ] -> parsed.special_effects.size(); ++i )
    {
      special_effect_t* effect = item_data[ WEAPON_PRIMARY ] -> parsed.special_effects[ i ];

      for ( size_t j = 0; j < rogue -> callbacks.all_callbacks.size(); ++j )
      {
        dbc_proc_callback_t* cb = debug_cast<dbc_proc_callback_t*>( rogue -> callbacks.all_callbacks[ j ] );

        if ( &( cb -> effect ) == effect )
        {
          cb_data[ WEAPON_PRIMARY ].push_back( cb );
        }
      }
    }

    // Pre-compute primary weapon stats
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      stats_data[ WEAPON_PRIMARY ].add_stat( rogue -> convert_hybrid_stat( i ),
                                             item_data[ WEAPON_PRIMARY ] -> stats.get_stat( i ) );
    }
  }

  if ( item_data[ WEAPON_SECONDARY ] )
  {
    // Find secondary weapon callbacks from the actor list of all callbacks
    for ( size_t i = 0; i < item_data[ WEAPON_SECONDARY ] -> parsed.special_effects.size(); ++i )
    {
      special_effect_t* effect = item_data[ WEAPON_SECONDARY ] -> parsed.special_effects[ i ];

      for ( size_t j = 0; j < rogue -> callbacks.all_callbacks.size(); ++j )
      {
        dbc_proc_callback_t* cb = debug_cast<dbc_proc_callback_t*>( rogue -> callbacks.all_callbacks[ j ] );

        if ( &( cb -> effect ) == effect )
        {
          cb_data[ WEAPON_SECONDARY ].push_back( cb );
        }
      }
    }

    // Pre-compute secondary weapon stats
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      stats_data[ WEAPON_SECONDARY ].add_stat( rogue -> convert_hybrid_stat( i ),
                                               item_data[ WEAPON_SECONDARY ] -> stats.get_stat( i ) );
    }

    if ( item_data[ WEAPON_SECONDARY ] )
    {
      std::string prefix = slot() == WEAPON_MAIN_HAND ? "_mh" : "_oh";

      secondary_weapon_uptime = buff_creator_t( rogue, "secondary_weapon" + prefix );
    }
  }

  initialized = true;
}

void weapon_info_t::reset()
{
  rogue_t* rogue = debug_cast<rogue_t*>( item_data[ WEAPON_PRIMARY ] -> player );

  // Reset swaps back to primary weapon for the slot
  rogue -> swap_weapon( slot(), WEAPON_PRIMARY, false );

  // .. and always deactivates secondary weapon callback(s).
  callback_state( WEAPON_SECONDARY, false );
}

// Due to how our DOT system functions, at the time when last_tick() is called
// for Deadly Poison, is_ticking() for the dot object will still return true.
// This breaks the is_ticking() check below, creating an inconsistent state in
// the sim, if Deadly Poison was the only poison up on the target. As a
// workaround, deliver the "Deadly Poison fade event" as an extra parameter.
inline bool rogue_t::poisoned_enemy( player_t* target, bool deadly_fade ) const
{
  const rogue_td_t* td = get_target_data( target );

  if ( ! deadly_fade && td -> dots.deadly_poison -> is_ticking() )
    return true;

  if ( td -> debuffs.wound_poison -> check() )
    return true;

  if ( td -> debuffs.crippling_poison -> check() )
    return true;

  if ( td -> debuffs.leeching_poison -> check() )
    return true;

  return false;
}

// ==========================================================================
// Rogue Triggers
// ==========================================================================

void rogue_t::trigger_auto_attack( const action_state_t* state )
{
  if ( main_hand_attack -> execute_event || ! off_hand_attack || off_hand_attack -> execute_event )
    return;

  if ( ! state -> action -> harmful )
    return;

  melee_main_hand -> first = true;
  if ( melee_off_hand )
    melee_off_hand -> first = true;

  auto_attack -> execute();
}

void rogue_t::trigger_seal_fate( const action_state_t* state )
{
  if ( ! spec.seal_fate -> ok() )
    return;

  if ( state -> result != RESULT_CRIT )
    return;

  trigger_combo_point_gain( 1, gains.seal_fate, state -> action );

  procs.seal_fate -> occur();
}

void rogue_t::trigger_main_gauche( const action_state_t* state, double chance )
{
  if ( ! mastery.main_gauche -> ok() )
    return;

  if ( state -> result_total <= 0 )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  actions::rogue_attack_t* attack = cast_attack( state -> action );
  if ( ! attack -> procs_main_gauche() )
    return;

  if ( chance == 0 )
  {
    chance = cache.mastery_value();
  }

  if ( ! rng().roll( chance ) )
    return;

  active_main_gauche -> target = state -> target;
  active_main_gauche -> schedule_execute();
}

void rogue_t::trigger_combat_potency( const action_state_t* state )
{
  if ( ! spec.combat_potency -> ok() )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  actions::rogue_attack_t* attack = cast_attack( state -> action );
  if ( ! attack -> procs_combat_potency() )
    return;

  //double chance = spec.combat_potency -> effectN( 1 ).percent();
  // http://us.battle.net/wow/en/forum/topic/20743504316?page=21#416
  double chance = 0.3;
  if ( state -> action != active_main_gauche )
    chance *= state -> action -> weapon -> swing_time.total_seconds() / 1.4;

  if ( ! rng().roll( chance ) )
    return;

  // energy gain value is in the proc trigger spell
  double gain = spec.combat_potency -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_ENERGY ) +
                artifact.fortune_strikes.value();

  resource_gain( RESOURCE_ENERGY, gain, gains.combat_potency, state -> action );
}

void rogue_t::trigger_energy_refund( const action_state_t* state )
{
  double energy_restored = state -> action -> resource_consumed * 0.80;

  resource_gain( RESOURCE_ENERGY, energy_restored, gains.energy_refund );
}

void rogue_t::trigger_venomous_wounds( const action_state_t* state )
{
  if ( ! spec.venomous_wounds -> ok() )
    return;

  if ( ! get_target_data( state -> target ) -> poisoned() )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  double chance = spec.venomous_wounds -> proc_chance();

  if ( ! rng().roll( chance ) )
    return;

  resource_gain( RESOURCE_ENERGY,
                 spec.venomous_wounds -> effectN( 2 ).base_value() +
                 talent.venom_rush -> effectN( 1 ).base_value(),
                 gains.venomous_wounds );
}

void rogue_t::trigger_venomous_wounds_death( player_t* target )
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( sim -> event_mgr.canceled )
  {
    return;
  }

  if ( ! spec.venomous_wounds -> ok() )
    return;

  rogue_td_t* td = get_target_data( target );
  if ( ! td -> poisoned() )
    return;

  // No end event means it naturally expired
  if ( ! td -> dots.rupture -> is_ticking() || td -> dots.rupture -> remains() == timespan_t::zero() )
  {
    return;
  }

  // TODO: Exact formula?
  unsigned full_ticks_remaining = td -> dots.rupture -> remains() / td -> dots.rupture -> current_action -> base_tick_time;
  int replenish = spec.venomous_wounds -> effectN( 2 ).base_value() +
                  talent.venom_rush -> effectN( 1 ).base_value();

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s venomous_wounds replenish on target death: full_ticks=%u, ticks_left=%u, vw_replenish=%d, remaining_time=%.3f",
      name(), full_ticks_remaining, td -> dots.rupture -> ticks_left(), replenish, td -> dots.rupture -> remains().total_seconds() );

  }

  resource_gain( RESOURCE_ENERGY, full_ticks_remaining * replenish, gains.venomous_wounds_death, td -> dots.rupture -> current_action );
}

void rogue_t::trigger_blade_flurry( const action_state_t* state )
{
  if ( state -> result_total <= 0 )
    return;

  if ( !buffs.blade_flurry -> check() )
    return;

  if ( !state -> action -> weapon )
    return;

  if ( !state -> action -> result_is_hit( state -> result ) )
    return;

  if ( sim -> active_enemies == 1 )
    return;

  if ( state -> action -> n_targets() != 0 )
    return;

  // Invalidate target cache if target changes
  if ( active_blade_flurry -> target != state -> target )
    active_blade_flurry -> target_cache.is_valid = false;
  active_blade_flurry -> target = state -> target;

  // Note, unmitigated damage
  active_blade_flurry -> base_dd_min = state -> result_total;
  active_blade_flurry -> base_dd_max = state -> result_total;
  active_blade_flurry -> schedule_execute();
}

void rogue_t::trigger_combo_point_gain( int     cp,
                                        gain_t* gain,
                                        action_t* action )
{
  resource_gain( RESOURCE_COMBO_POINT, cp, gain, action );
}

void rogue_t::trigger_ruthlessness_cp( const action_state_t* state )
{
  if ( ! spec.ruthlessness -> ok() )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  actions::rogue_attack_t* attack = cast_attack( state -> action );
  if ( ! attack -> affected_by.ruthlessness )
    return;

  const actions::rogue_attack_state_t* s = attack -> cast_state( state );
  if ( s -> cp == 0 )
    return;

  double cp_chance = spec.ruthlessness -> effectN( 1 ).pp_combo_points() * s -> cp / 100.0;
  double cp_gain = 0;
  if ( cp_chance > 1 )
  {
    cp_gain += 1;
    cp_chance -= 1;
  }

  if ( rng().roll( cp_chance ) )
  {
    cp_gain += 1;
  }

  if ( cp_gain > 0 )
  {
    trigger_combo_point_gain( cp_gain, gains.ruthlessness, state -> action );
  }
}

void rogue_t::trigger_deepening_shadows( const action_state_t* state )
{
  if ( ! spec.deepening_shadows -> ok() )
  {
    return;
  }

  actions::rogue_attack_t* attack = rogue_t::cast_attack( state -> action );
  if ( ! attack -> affected_by.deepening_shadows )
  {
    return;
  }

  if ( attack -> secondary_trigger == TRIGGER_WEAPONMASTER )
  {
    return;
  }

  const actions::rogue_attack_state_t* s = actions::rogue_attack_t::cast_state( state );
  if ( s -> cp == 0 )
  {
    return;
  }

  timespan_t adjustment = timespan_t::from_seconds( -1 * spec.deepening_shadows -> effectN( 2 ).base_value() * s -> cp );
  cooldowns.shadow_dance -> adjust( adjustment, s -> cp >= 5 );
}

void rogue_t::trigger_shadow_techniques( const action_state_t* state )
{
  if ( ! spec.shadow_techniques -> ok() )
  {
    return;
  }

  if ( state -> action -> special )
  {
    return;
  }

  if ( ! state -> action -> result_is_hit( state -> result ) )
  {
    return;
  }

  if ( --shadow_techniques == 0 )
  {
    double cp = 1;
    if ( rng().roll( artifact.fortunes_bite.percent() ) )
    {
      cp++;
    }

    trigger_combo_point_gain( cp, gains.shadow_techniques, state -> action );
    shadow_techniques = rng().range( 3, 5 );
  }
}

void rogue_t::trigger_weaponmaster( const action_state_t* s )
{
  if ( ! talent.weaponmaster -> ok() )
  {
    return;
  }

  actions::rogue_attack_t* attack = cast_attack( s -> action );
  if ( ! s -> action -> result_is_hit( s -> result ) || s -> result_amount <= 0 ||
       ! attack -> affected_by.weaponmaster )
  {
    return;
  }

  if ( cooldowns.weaponmaster -> down() )
  {
    return;
  }

  if ( ! rng().roll( talent.weaponmaster -> proc_chance() ) )
  {
    return;
  }

  procs.weaponmaster -> occur();
  // Direct damage re-computes on execute
  if ( s -> result_type == DMG_DIRECT )
  {
    make_event<actions::secondary_ability_trigger_t>( *sim, s -> target, s -> action,
        actions::rogue_attack_t::cast_state( s ) -> cp, TRIGGER_WEAPONMASTER );
  }
  // Dot damage is always a "snapshot"
  else
  {
    weaponmaster_dot_strike -> base_dd_min = weaponmaster_dot_strike -> base_dd_max = s -> result_amount;
    weaponmaster_dot_strike -> target = s -> target;
    weaponmaster_dot_strike -> schedule_execute();
  }

  cooldowns.weaponmaster -> start( talent.weaponmaster -> internal_cooldown() );
}

void rogue_t::trigger_energetic_stabbing( const action_state_t* s )
{
  if ( artifact.energetic_stabbing.rank() == 0 )
  {
    return;
  }

  if ( ! s -> action -> result_is_hit( s -> result ) )
  {
    return;
  }

  if ( rng().roll( artifact.energetic_stabbing.data().proc_chance() ) )
  {
    resource_gain( RESOURCE_ENERGY, artifact.energetic_stabbing.value(), gains.energetic_stabbing, s -> action );
  }
}

void rogue_t::trigger_second_shuriken( const action_state_t* state )
{
  if ( ! artifact.second_shuriken.rank() )
  {
    return;
  }

  if ( ! state -> action -> result_is_hit( state -> result ) )
  {
    return;
  }

  if ( rng().roll( artifact.second_shuriken.data().proc_chance() ) )
  {
    second_shuriken -> execute();
  }
}

void rogue_t::trigger_surge_of_toxins( const action_state_t* s )
{
  using namespace actions;
  if ( ! artifact.surge_of_toxins.rank() )
  {
    return;
  }

  if ( s -> action -> base_costs[ RESOURCE_COMBO_POINT ] == 0 )
  {
    return;
  }

  int max_cp = rogue_attack_t::cast_state( s ) -> cp;
  // In game apparently DS extra spent CP does not work on surge bonus
  if ( bugs )
  {
    max_cp = std::min( max_cp, static_cast<int>( COMBO_POINT_MAX ) );
  }

  // 2% per combo point, nowhere to be found in spell data. Agonizing Poison halves it to 1%.
  get_target_data( s -> target ) -> debuffs.surge_of_toxins -> trigger( 1, max_cp * 0.02 );
}

void rogue_t::trigger_poison_knives( const action_state_t* state )
{
  if ( ! artifact.poison_knives.rank() )
  {
    return;
  }

  rogue_td_t* td = get_target_data( state -> target );
  if ( ! td -> dots.deadly_poison -> is_ticking() )
  {
    return;
  }

  unsigned ticks_left = td -> dots.deadly_poison -> ticks_left();
  timespan_t tick_time = td -> dots.deadly_poison -> current_action -> tick_time( td -> dots.deadly_poison -> state );
  double partial_tick = 0;
  if ( ticks_left > 1 && ticks_left * tick_time > td -> dots.deadly_poison -> remains() )
  {
    partial_tick = ( td -> dots.deadly_poison -> remains() - ( ticks_left - 1 ) * tick_time ) / tick_time;
  }

  // Recompute tick damage with current stats
  td -> dots.deadly_poison -> current_action -> calculate_tick_amount( td -> dots.deadly_poison -> state,
      td -> dots.deadly_poison -> current_stack() );

  double tick_base_damage = td -> dots.deadly_poison -> state -> result_raw;
  // Poison knives double dips into some multipliers

  // .. then, apparently the Master Alchemist talent
  tick_base_damage *= 1.0 + artifact.master_alchemist.percent();

  // Target multipliers get applied on execute, they also work

  double total_damage = ( partial_tick + ticks_left ) * tick_base_damage * artifact.poison_knives.percent();
  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s poison_knives dot_remains=%.3f duration=%.3f ticks_left=%u partial=%.3f amount=%.3f total=%.3f",
      name(), td -> dots.deadly_poison -> remains().total_seconds(),
      td -> dots.deadly_poison -> duration().total_seconds(),
      td -> dots.deadly_poison -> ticks_left(), partial_tick, tick_base_damage,
      total_damage );
  }

  poison_knives -> base_dd_min = poison_knives -> base_dd_max = total_damage;
  poison_knives -> target = state -> target;
  poison_knives -> execute();
}

void rogue_t::trigger_elaborate_planning( const action_state_t* s )
{
  if ( ! talent.elaborate_planning -> ok() )
  {
    return;
  }

  if ( s -> action -> base_costs[ RESOURCE_COMBO_POINT ] == 0 || s -> action -> background )
  {
    return;
  }

  buffs.elaborate_planning -> trigger();
}

void rogue_t::trigger_alacrity( const action_state_t* s )
{
  using namespace actions;

  if ( ! talent.alacrity -> ok() )
  {
    return;
  }

  rogue_attack_t* attack = cast_attack( s -> action );
  if ( ! attack -> affected_by.alacrity )
  {
    return;
  }

  const rogue_attack_state_t* rs = rogue_attack_t::cast_state( s );
  double chance = talent.alacrity -> effectN( 2 ).percent() * rs -> cp;
  int stacks = 0;
  if ( chance > 1 )
  {
    stacks += 1;
    chance -= 1;
  }

  if ( rng().roll( chance ) )
  {
    stacks += 1;
  }

  if ( stacks > 0 )
  {
    buffs.alacrity -> trigger( stacks );
  }
}

void rogue_t::trigger_true_bearing( const action_state_t* state )
{
  timespan_t v = timespan_t::from_seconds( buffs.true_bearing -> default_value );
  v *= - actions::rogue_attack_t::cast_state( state ) -> cp;

  cooldowns.adrenaline_rush -> adjust( v, false );
  cooldowns.sprint -> adjust( v, false );
  // As of 10/27 (7.1 22908), Between the Eyes (199804) doesn't reduce its own CD.
  // FIXME: Will be removed in 7.1.5
  if ( ! maybe_ptr( this -> dbc.ptr ) &&
       (! bugs || state -> action -> id != 199804 ) )
  {
    cooldowns.between_the_eyes -> adjust( v, false );
  }
  cooldowns.vanish -> adjust( v, false );
  // FIXME: Will be removed in 7.1.5
  if ( ! maybe_ptr( this -> dbc.ptr ) )
  {
    cooldowns.blind -> adjust( v, false );
    cooldowns.cloak_of_shadows -> adjust( v, false );
    cooldowns.riposte -> adjust( v, false );
  }
  cooldowns.grappling_hook -> adjust( v, false );
  cooldowns.cannonball_barrage -> adjust( v, false );
  cooldowns.killing_spree -> adjust( v, false );
  cooldowns.marked_for_death -> adjust( v, false );
  cooldowns.death_from_above -> adjust( v, false );
}

void do_exsanguinate( dot_t* dot, double coeff )
{
  if ( ! dot -> is_ticking() )
  {
    return;
  }

  dot -> adjust( coeff );
  actions::rogue_attack_t::cast_state( dot -> state ) -> exsanguinated = true;
}

void rogue_t::trigger_exsanguinate( const action_state_t* state )
{
  rogue_td_t* td = get_target_data( state -> target );

  double coeff = 1.0 / ( 1.0 + talent.exsanguinate -> effectN( 1 ).percent() );
  do_exsanguinate( td -> dots.rupture, coeff );
  do_exsanguinate( td -> dots.garrote, coeff );
}

void rogue_t::trigger_relentless_strikes( const action_state_t* state )
{
  using namespace actions;

  if ( ! spec.relentless_strikes -> ok() )
  {
    return;
  }

  const rogue_attack_t* attack = cast_attack( state -> action );
  if ( ! attack -> affected_by.relentless_strikes )
  {
    return;
  }

  double proc_chance = rogue_attack_t::cast_state( state ) -> cp * spec.relentless_strikes -> effectN( 2 ).percent();
  double grant_energy = 0;
  if ( proc_chance > 1 )
  {
    grant_energy += spell.relentless_strikes_energize -> effectN( 1 ).resource( RESOURCE_ENERGY );
    proc_chance -= 1;
  }

  if ( rng().roll( proc_chance ) )
  {
    grant_energy += spell.relentless_strikes_energize -> effectN( 1 ).resource( RESOURCE_ENERGY );
  }

  if ( grant_energy > 0 )
  {
    resource_gain( RESOURCE_ENERGY, grant_energy, gains.relentless_strikes, state -> action );
  }
}

void rogue_t::trigger_t19oh_8pc( const action_state_t* state )
{
  if ( ! sets.has_set_bonus( specialization(), T19OH, B8 ) )
  {
    return;
  }

  if ( actions::rogue_attack_t::cast_state( state ) -> cp == 0 )
  {
    return;
  }

  buffs.t19oh_8pc -> trigger();
}

void rogue_t::spend_combo_points( const action_state_t* state )
{
  if ( state -> action -> base_costs[ RESOURCE_COMBO_POINT ] == 0 )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  if ( buffs.fof_fod -> up() )
    return;

  double max_spend = std::min( resources.current[ RESOURCE_COMBO_POINT ], consume_cp_max() );

  if ( legendary.denial_of_the_halfgiants && buffs.shadow_blades -> up() )
  {
    timespan_t adjustment = timespan_t::from_seconds( max_spend / 10.0 * legendary.denial_of_the_halfgiants -> effectN( 1 ).base_value() );
    buffs.shadow_blades -> extend_duration( this, adjustment );
  }

  state -> action -> stats -> consume_resource( RESOURCE_COMBO_POINT, max_spend );
  resource_loss( RESOURCE_COMBO_POINT, max_spend, nullptr, state ? state -> action : nullptr );

  if ( sim -> log )
    sim -> out_log.printf( "%s consumes %.1f %s for %s (%.0f)", name(),
                   max_spend, util::resource_type_string( RESOURCE_COMBO_POINT ),
                   state -> action -> name(), resources.current[ RESOURCE_COMBO_POINT ] );

}

bool rogue_t::trigger_t17_4pc_combat( const action_state_t* state )
{
  using namespace actions;

  if ( ! sets.has_set_bonus( ROGUE_OUTLAW, T17, B4 ) )
    return false;

  if ( state -> action -> base_costs[ RESOURCE_COMBO_POINT ] == 0 )
    return false;

  if ( ! state -> action -> harmful )
    return false;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return false;

  const rogue_attack_state_t* rs = rogue_attack_t::cast_state( state );
  if ( ! rng().roll( sets.set( ROGUE_OUTLAW, T17, B4 ) -> proc_chance() / 5.0 * rs -> cp ) )
    return false;

  trigger_combo_point_gain( buffs.deceit -> data().effectN( 2 ).base_value(), gains.deceit, state -> action );
  buffs.deceit -> trigger();
  return true;
}

void rogue_t::trigger_insignia_of_ravenholdt( action_state_t* state )
{
  if ( ! legendary.insignia_of_ravenholdt )
  {
    return;
  }

  if ( state -> result_total <= 0 )
  {
    return;
  }

  if ( ! state -> action -> result_is_hit( state -> result ) )
  {
    return;
  }

  const actions::rogue_attack_t* attack = cast_attack( state -> action );
  if ( ! attack -> procs_insignia_of_ravenholdt() )
  {
    return;
  }

  // As of 10/29 (7.1 22908), Insignia takes in account the amount before the crit roll.
  double amount = state -> result_mitigated;
  if ( state -> result == RESULT_CRIT )
  {
    amount /= 1.0 + state -> action -> total_crit_bonus( state );
  }
  insignia_of_ravenholdt_ -> base_dd_min = amount; 
  insignia_of_ravenholdt_ -> base_dd_max = amount;
  insignia_of_ravenholdt_ -> target = state -> target;
  insignia_of_ravenholdt_ -> execute();
}

namespace buffs {
// ==========================================================================
// Buffs
// ==========================================================================

struct proxy_garrote_t : public buff_t
{
  proxy_garrote_t( rogue_td_t& r ) :
    buff_t( buff_creator_t( r, "garrote", r.source -> find_specialization_spell( "Garrote" ) )
            .quiet( true ).cd( timespan_t::zero() ) )
  { }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( remaining_duration > timespan_t::zero() )
    {
      rogue_t* rogue = debug_cast<rogue_t*>( source );
      if ( rogue -> talent.thuggee -> ok() )
      {
        rogue -> cooldowns.garrote -> reset( false );
      }
    }
  }
};

struct blurred_time_t : public buff_t
{
  rogue_t* r;

  blurred_time_t( rogue_t* p ) :
    buff_t( buff_creator_t( p, "blurred_time", p -> artifact.blurred_time.data().effectN( 1 ).trigger() )
        .trigger_spell( p -> artifact.blurred_time ) ),
    r( p )
  { }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    range::for_each( r -> blurred_time_cooldowns, []( cooldown_t* cd ) { cd -> adjust_recharge_multiplier(); } );
    player -> adjust_action_queue_time();
  }

  void expire( timespan_t delay ) override
  {
    bool expired = check() != 0;

    buff_t::expire( delay );

    if ( expired )
    {
      range::for_each( r -> blurred_time_cooldowns, []( cooldown_t* cd ) { cd -> adjust_recharge_multiplier(); } );
      player -> adjust_action_queue_time();
    }
  }
};

struct fof_fod_t : public buff_t
{
  fof_fod_t( rogue_t* p ) :
    buff_t( buff_creator_t( p, "legendary_daggers" ).duration( timespan_t::from_seconds( 6.0 ) ).cd( timespan_t::zero() ) )
  { }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    rogue_t* p = debug_cast< rogue_t* >( player );
    p -> buffs.fof_p3 -> expire();
  }
};

struct subterfuge_t : public buff_t
{
  rogue_t* rogue;

  subterfuge_t( rogue_t* r ) :
    buff_t( buff_creator_t( r, "subterfuge", r -> find_spell( 115192 ) ) ),
    rogue( r )
  { }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
    // The Glyph of Vanish bug is back, so if Vanish is still up when
    // Subterfuge fades, don't cancel stealth. Instead, the next offensive
    // action in the sim will trigger a new (3 seconds) of Subterfuge.
    if ( ( rogue -> bugs && (
            rogue -> buffs.vanish -> remains() == timespan_t::zero() ||
            rogue -> buffs.vanish -> check() == 0 ) ) ||
        ! rogue -> bugs )
    {
      actions::break_stealth( rogue );
    }
  }
};

struct stealth_like_buff_t : public buff_t
{
  rogue_t* rogue;

  stealth_like_buff_t( rogue_t* r, const std::string& name, const spell_data_t* spell ) :
    buff_t( buff_creator_t( r, name, spell ) ), rogue( r )
  { }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    if ( rogue -> in_combat && rogue -> talent.master_of_shadows -> ok() )
    {
      rogue -> resource_gain( RESOURCE_ENERGY, rogue -> spell.master_of_shadows -> effectN( 1 ).base_value(),
          rogue -> gains.master_of_shadows );
    }

    if ( rogue -> legendary.mantle_of_the_master_assassin )
    {
      rogue -> buffs.mantle_of_the_master_assassin -> expire();
      rogue -> buffs.mantle_of_the_master_assassin_passive -> trigger();
    }

    rogue -> buffs.master_of_subtlety -> expire();
    rogue -> buffs.master_of_subtlety_passive -> trigger();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( rogue -> legendary.mantle_of_the_master_assassin )
    {
      rogue -> buffs.mantle_of_the_master_assassin_passive -> expire();
      rogue -> buffs.mantle_of_the_master_assassin -> trigger();
    }

    rogue -> buffs.master_of_subtlety_passive -> expire();
    rogue -> buffs.master_of_subtlety -> trigger();

    if ( ! maybe_ptr( player -> dbc.ptr ) &&
         rogue -> artifact.shadow_nova.rank() )
    {
      rogue -> shadow_nova -> schedule_execute();
    }
  }
};

// Vanish does not give "stealth like abilities", except for some reason it does give Master of
// Subtlety buff.
struct vanish_t : public buff_t
{
  rogue_t* rogue;

  vanish_t( rogue_t* r ) :
    buff_t( buff_creator_t( r, "vanish", r -> find_spell( 11327 ) ) ),
    rogue( r )
  { }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    if ( rogue -> legendary.mantle_of_the_master_assassin )
    {
      rogue -> buffs.mantle_of_the_master_assassin -> expire();
      rogue -> buffs.mantle_of_the_master_assassin_passive -> trigger();
    }

    rogue -> buffs.master_of_subtlety -> expire();
    rogue -> buffs.master_of_subtlety_passive -> trigger();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( rogue -> legendary.mantle_of_the_master_assassin )
    {
      rogue -> buffs.mantle_of_the_master_assassin_passive -> expire();
      rogue -> buffs.mantle_of_the_master_assassin -> trigger();
    }

    rogue -> buffs.master_of_subtlety_passive -> expire();
    rogue -> buffs.master_of_subtlety -> trigger();

    if ( remaining_duration == timespan_t::zero() )
    {
      rogue -> buffs.stealth -> trigger();
    }
  }
};

// Note, stealth buff is set a max time of half the nominal fight duration, so it can be forced to
// show in sample sequence tables.
struct stealth_t : public stealth_like_buff_t
{
  stealth_t( rogue_t* r ) :
    stealth_like_buff_t( r, "stealth", r -> find_spell( 1784 ) )
  {
    buff_duration = sim -> max_time / 2;
  }
};

struct shadow_dance_t : public stealth_like_buff_t
{
  shadow_dance_t( rogue_t* p ) :
    stealth_like_buff_t( p, "shadow_dance", p -> spec.shadow_dance )
  {
    buff_duration += p -> talent.subterfuge -> effectN( 2 ).time_value();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    stealth_like_buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( remaining_duration == timespan_t::zero() )
    {
      rogue -> buffs.shadow_strikes -> trigger();
    }
  }
};

struct rogue_poison_buff_t : public buff_t
{
  rogue_poison_buff_t( rogue_td_t& r, const std::string& name, const spell_data_t* spell ) :
    buff_t( buff_creator_t( r, name, spell ) )
  { }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    rogue_t* rogue = debug_cast< rogue_t* >( source );
    if ( ! rogue -> poisoned_enemy( player ) )
      rogue -> poisoned_enemies++;

    buff_t::execute( stacks, value, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    rogue_t* rogue = debug_cast< rogue_t* >( source );
    if ( ! rogue -> poisoned_enemy( player ) )
      rogue -> poisoned_enemies--;
  }
};

struct wound_poison_t : public rogue_poison_buff_t
{
  wound_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "wound_poison", r.source -> find_spell( 8680 ) )
  { }
};

struct crippling_poison_t : public rogue_poison_buff_t
{
  crippling_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "crippling_poison", r.source -> find_spell( 3409 ) )
  { }
};

struct leeching_poison_t : public rogue_poison_buff_t
{
  leeching_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "leeching_poison", r.source -> find_spell( 112961 ) )
  { }
};

struct agonizing_poison_t : public rogue_poison_buff_t
{
  agonizing_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "agonizing_poison", r.source -> find_spell( 200803 ) )
  {
    default_value = data().effectN( 1 ).percent();
    refresh_behavior = BUFF_REFRESH_PANDEMIC;
  }
};

struct marked_for_death_debuff_t : public debuff_t
{
  cooldown_t* mod_cd;

  marked_for_death_debuff_t( rogue_td_t& r ) :
    debuff_t( buff_creator_t( r, "marked_for_death", r.source -> find_talent_spell( "Marked for Death" ) ).cd( timespan_t::zero() ) ),
    mod_cd( r.source -> get_cooldown( "marked_for_death" ) )
  { }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    if ( remaining_duration > timespan_t::zero() )
    {
      if ( sim -> debug )
      {
        sim -> out_debug.printf("%s marked_for_death cooldown reset", player -> name() );
      }

      mod_cd -> reset( false );
    }

    debuff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

struct roll_the_bones_t : public buff_t
{
  rogue_t* rogue;
  std::array<buff_t*, 6> buffs;

  roll_the_bones_t( rogue_t* r, buff_creator_t& b ) :
    buff_t( b ), rogue( r )
  {
    buffs[ 0 ] = rogue -> buffs.broadsides;
    buffs[ 1 ] = rogue -> buffs.buried_treasure;
    buffs[ 2 ] = rogue -> buffs.grand_melee;
    buffs[ 3 ] = rogue -> buffs.jolly_roger;
    buffs[ 4 ] = rogue -> buffs.shark_infested_waters;
    buffs[ 5 ] = rogue -> buffs.true_bearing;
  }

  void expire_secondary_buffs()
  {
    rogue -> buffs.jolly_roger -> expire();
    rogue -> buffs.grand_melee -> expire();
    rogue -> buffs.shark_infested_waters -> expire();
    rogue -> buffs.true_bearing -> expire();
    rogue -> buffs.broadsides -> expire();
    rogue -> buffs.buried_treasure -> expire();
  }

  unsigned random_roll( timespan_t duration )
  {
    std::array<unsigned, 6> rolls = { { 0, 0, 0, 0, 0, 0 } };
    for ( size_t i = 0; i < rolls.size(); ++i )
    {
      rolls[ rng().range( 0, buffs.size() ) ]++;
    }

    unsigned largest_group = *std::max_element( rolls.begin(), rolls.end() );

    unsigned n_groups = 0;
    for ( size_t i = 0; i < buffs.size(); ++i )
    {
      if ( rolls[ i ] != largest_group )
      {
        continue;
      }

      buffs[ i ] -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
      n_groups++;
    }

    return n_groups;
  }

  unsigned fixed_roll( timespan_t duration )
  {
    range::for_each( rogue -> fixed_rtb, [this, duration]( size_t idx )
    { buffs[idx] -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration ); } );
    return as<unsigned>( rogue -> fixed_rtb.size() );
  }

  unsigned roll_the_bones( timespan_t duration )
  {
    if ( rogue -> fixed_rtb.size() == 0 )
    {
      return random_roll( duration );
    }
    else
    {
      return fixed_roll( duration );
    }
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    expire_secondary_buffs();

    switch ( roll_the_bones( remains() ) )
    {
      case 1:
        rogue -> procs.roll_the_bones_1 -> occur();
        break;
      case 2:
        rogue -> procs.roll_the_bones_2 -> occur();
        break;
      case 3:
        rogue -> procs.roll_the_bones_3 -> occur();
        break;
      case 6:
        rogue -> procs.roll_the_bones_6 -> occur();
        break;
      default:
        assert( 0 );
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
    expire_secondary_buffs();
  }
};

struct shadow_blades_t : public buff_t
{
  shadow_blades_t( rogue_t* p ) :
    buff_t( buff_creator_t( p, "shadow_blades", p -> find_specialization_spell( "Shadow Blades" ) )
        .duration( p -> find_specialization_spell( "Shadow Blades" ) -> duration() +
                   p -> artifact.soul_shadows.time_value() )
        .cd( timespan_t::zero() ) )
  { }

  void change_auto_attack( attack_t*& hand, attack_t* a )
  {
    if ( hand == 0 )
      return;

    bool executing = hand -> execute_event != 0;
    timespan_t time_to_hit = timespan_t::zero();

    if ( executing )
    {
      time_to_hit = hand -> execute_event -> occurs() - sim -> current_time();
      event_t::cancel( hand -> execute_event );
    }

    hand = a;

    // Kick off the new attack, by instantly scheduling and rescheduling it to
    // the remaining time to hit. We cannot use normal reschedule mechanism
    // here (i.e., simply use event_t::reschedule() and leave it be), because
    // the rescheduled event would be triggered before the full swing time
    // (of the new auto attack) in most cases.
    if ( executing )
    {
      timespan_t old_swing_time = hand -> base_execute_time;
      hand -> base_execute_time = timespan_t::zero();
      hand -> schedule_execute();
      hand -> base_execute_time = old_swing_time;
      hand -> execute_event -> reschedule( time_to_hit );
    }
  }

  void execute( int stacks = 1, double value = buff_t::DEFAULT_VALUE(), timespan_t duration = timespan_t::min() ) override
  {
    buff_t::execute( stacks, value, duration );

    rogue_t* p = debug_cast< rogue_t* >( player );
    change_auto_attack( p -> main_hand_attack, p -> shadow_blade_main_hand );
    if ( p -> off_hand_attack )
      change_auto_attack( p -> off_hand_attack, p -> shadow_blade_off_hand );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    rogue_t* p = debug_cast< rogue_t* >( player );
    change_auto_attack( p -> main_hand_attack, p -> melee_main_hand );
    if ( p -> off_hand_attack )
      change_auto_attack( p -> off_hand_attack, p -> melee_off_hand );
  }
};

} // end namespace buffs

inline void actions::marked_for_death_t::impact( action_state_t* state )
{
  rogue_attack_t::impact( state );

  td( state -> target ) -> debuffs.marked_for_death -> trigger();
}

// ==========================================================================
// Rogue Targetdata Definitions
// ==========================================================================

rogue_td_t::rogue_td_t( player_t* target, rogue_t* source ) :
  actor_target_data_t( target, source ),
  dots( dots_t() ),
  debuffs( debuffs_t() )
{

  dots.deadly_poison    = target -> get_dot( "deadly_poison_dot", source );
  dots.garrote          = target -> get_dot( "garrote", source );
  dots.mutilated_flesh  = target -> get_dot( "mutilated_flesh", source );
  dots.rupture          = target -> get_dot( "rupture", source );
  dots.kingsbane        = target -> get_dot( "kingsbane", source );
  dots.nightblade       = target -> get_dot( "nightblade", source );
  dots.killing_spree    = target -> get_dot( "killing_spree", source );

  const spell_data_t* vd = source -> find_specialization_spell( "Vendetta" );
  debuffs.vendetta =           buff_creator_t( *this, "vendetta", vd )
                               .cd( timespan_t::zero() )
                               .default_value( vd -> effectN( 1 ).percent() );

  debuffs.agonizing_poison = new buffs::agonizing_poison_t( *this );
  debuffs.wound_poison = new buffs::wound_poison_t( *this );
  debuffs.crippling_poison = new buffs::crippling_poison_t( *this );
  debuffs.leeching_poison = new buffs::leeching_poison_t( *this );
  debuffs.marked_for_death = new buffs::marked_for_death_debuff_t( *this );
  debuffs.hemorrhage = buff_creator_t( *this, "hemorrhage", source -> talent.hemorrhage )
    .default_value( 1.0 + source -> talent.hemorrhage -> effectN( 4 ).percent() )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  debuffs.ghostly_strike = buff_creator_t( *this, "ghostly_strike", source -> talent.ghostly_strike )
    .default_value( source -> talent.ghostly_strike -> effectN( 5 ).percent() );
  debuffs.garrote = new buffs::proxy_garrote_t( *this );
  debuffs.surge_of_toxins = buff_creator_t( *this, "surge_of_toxins", source -> find_spell( 192425 ) )
    .default_value( source -> find_spell( 192425 ) -> effectN( 1 ).base_value() * 0.002 )
    .trigger_spell( source -> artifact.surge_of_toxins );
  debuffs.kingsbane = buff_creator_t( *this, "kingsbane", source -> artifact.kingsbane.data().effectN( 5 ).trigger() )
    .default_value( source -> artifact.kingsbane.data().effectN( 5 ).trigger() -> effectN( 1 ).percent() )
    .refresh_behavior( BUFF_REFRESH_DISABLED );

  debuffs.blood_of_the_assassinated = buff_creator_t( *this, "blood_of_the_assassinated" )
    .spell( source -> artifact.blood_of_the_assassinated.data().effectN( 1 ).trigger() )
    .trigger_spell( source -> artifact.blood_of_the_assassinated )
    .default_value( source -> artifact.blood_of_the_assassinated.data().effectN( 1 ).trigger() -> effectN( 1 ).percent() );

  // Register on-demise callback for assassination to perform Venomous Wounds energy replenish on
  // death.
  if ( source -> specialization() == ROGUE_ASSASSINATION && source -> spec.venomous_wounds -> ok() )
  {
    target -> callbacks_on_demise.push_back( std::bind( &rogue_t::trigger_venomous_wounds_death, source, std::placeholders::_1 ) );
  }
}

// ==========================================================================
// Rogue Character Definition
// ==========================================================================

// rogue_t::composite_attack_speed ==========================================

double rogue_t::composite_melee_speed() const
{
  double h = player_t::composite_melee_speed();

  if ( buffs.slice_and_dice -> check() )
    h *= 1.0 / ( 1.0 + buffs.slice_and_dice -> value() );

  if ( buffs.adrenaline_rush -> check() )
    h *= 1.0 / ( 1.0 + buffs.adrenaline_rush -> value() );

  if ( buffs.grand_melee -> up() )
  {
    h *= buffs.grand_melee -> check_value();
  }

  return h;
}

// rogue_t::composite_melee_haste ==========================================

double rogue_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.alacrity -> check() )
  {
    h *= 1.0 / ( 1.0 + buffs.alacrity -> stack_value() );
  }

  return h;
}

// rogue_t::composite_melee_crit_chance =========================================

double rogue_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  crit += spell.critical_strikes -> effectN( 1 ).percent();

  crit += buffs.shark_infested_waters -> stack_value();

  crit += buffs.mantle_of_the_master_assassin -> stack_value(); // 7.1.5 Legendary

  crit += buffs.mantle_of_the_master_assassin_passive -> stack_value(); // 7.1.5 Legendary

  return crit;
}

// rogue_t::composite_spell_haste ==========================================

double rogue_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.alacrity -> check() )
  {
    h *= 1.0 / ( 1.0 + buffs.alacrity -> stack_value() );
  }

  return h;
}

// rogue_t::composite_spell_crit_chance =========================================

double rogue_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  crit += spell.critical_strikes -> effectN( 1 ).percent();

  crit += buffs.shark_infested_waters -> stack_value();

  crit += buffs.mantle_of_the_master_assassin -> stack_value(); // 7.1.5 Legendary

  crit += buffs.mantle_of_the_master_assassin_passive -> stack_value(); // 7.1.5 Legendary

  return crit;
}

// rogue_t::matching_gear_multiplier ========================================

double rogue_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0.0;
}

// rogue_t::composite_attack_power_multiplier ===============================

double rogue_t::composite_attack_power_multiplier() const
{
  double m = player_t::composite_attack_power_multiplier();

  if ( spec.vitality -> ok() )
  {
    m *= 1.0 + spec.vitality -> effectN( 2 ).percent();
  }

  return m;
}

// rogue_t::composite_player_multiplier =====================================

double rogue_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( artifact.shadow_fangs.rank() &&
       ( dbc::is_school( school, SCHOOL_PHYSICAL ) ||
         dbc::is_school( school, SCHOOL_SHADOW) ) )
  {
    m *= 1.0 + artifact.shadow_fangs.data().effectN( 1 ).percent();
  }

  m *= 1.0 + artifact.slayers_precision.percent();
  m *= 1.0 + artifact.legionblade.percent();
  m *= 1.0 + artifact.cursed_steel.percent();

  if ( buffs.master_of_subtlety -> check() || buffs.master_of_subtlety_passive -> check() )
  {
    m *= 1.0 + talent.master_of_subtlety -> effectN( 1 ).percent();
  }

  if ( main_hand_weapon.type == WEAPON_DAGGER && off_hand_weapon.type == WEAPON_DAGGER && spec.assassins_resolve -> ok() )
  {
    m *= 1.0 + spec.assassins_resolve -> effectN( 2 ).percent();
  }

  if ( sets.has_set_bonus( ROGUE_OUTLAW, T18, B4 ) )
  {
    if ( buffs.adrenaline_rush -> up() )
    {
      m *= 1.0 + sets.set( ROGUE_OUTLAW, T18, B4 ) -> effectN( 1 ).percent();
    }
  }

  if ( buffs.deathly_shadows -> up() )
  {
    m *= 1.0 + buffs.deathly_shadows -> data().effectN( 1 ).percent();
  }

  if ( buffs.elaborate_planning -> up() )
  {
    m *= buffs.elaborate_planning -> check_value();
  }

  if ( buffs.symbols_of_death -> up() )
  {
    m *= buffs.symbols_of_death -> check_value();
  }

  return m;
}

// rogue_t::agonizing_poison_stack_multiplier ===============================

double rogue_t::agonizing_poison_stack_multiplier( const rogue_td_t* td ) const
{
  if ( ! td -> debuffs.agonizing_poison -> check() )
  {
    return 0;
  }

  // 0.04 to 0.20 base
  double multiplier = td -> debuffs.agonizing_poison -> stack_value();

  // Mastery, Master Alchemist, and Poison Knives apply additively
  double additive_multiplier = 1.0;

  additive_multiplier += cache.mastery() * mastery.potent_poisons -> effectN( 4 ).mastery_value();
  if ( artifact.master_alchemist.rank() )
  {
    // Hardcoded divisor for now, not available in the spell data
    additive_multiplier += artifact.master_alchemist.percent() / 5;
  }

  if ( artifact.poison_knives.rank() )
  {
    // Hardcoded divisor for now, not available in the spell data
    additive_multiplier += artifact.poison_knives.percent() / 2;
  }

  multiplier *= additive_multiplier;

  // Master Poisoner and Surge of Toxins apply as normal multipliers

  if ( talent.master_poisoner -> ok() )
  {
    multiplier *= 1.0 + talent.master_poisoner -> effectN( 3 ).percent();
  }

  // Technically in game "features" and this should only apply after triggering a poison after the
  // Surge of Toxing buff goes up.
  if ( td -> debuffs.surge_of_toxins -> up() )
  {
    // Note, half effectiveness on Agonizing Poison
    multiplier *= 1.0 + td -> debuffs.surge_of_toxins -> stack_value() / 2;
  }

  // To be confirmed: behavior of Zoldyck Family Training Shackles with Agonizing Poison
  if ( legendary.zoldyck_family_training_shackles &&
       td -> target -> health_percentage() < legendary.zoldyck_family_training_shackles -> effectN( 2 ).base_value() )
  {
    multiplier *= 1.0 + legendary.zoldyck_family_training_shackles -> effectN( 1 ).percent();
  }

  return multiplier;
}

// rogue_t::composite_player_target_multiplier ==============================

double rogue_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  rogue_td_t* tdata = get_target_data( target );

  m *= 1.0 + agonizing_poison_stack_multiplier( tdata );

  m *= 1.0 + tdata -> debuffs.ghostly_strike -> stack_value();

  return m;
}

// rogue_t::init_actions ====================================================

void rogue_t::init_action_list()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default" );

  std::vector<std::string> item_actions = get_item_actions();
  std::vector<std::string> profession_actions = get_profession_actions();
  std::vector<std::string> racial_actions = get_racial_actions();

  clear_action_priority_lists();

  // Flask
  if ( sim -> allow_flasks && true_level >= 85 )
  {
    std::string flask_action = "flask,name=";
    flask_action += ( ( true_level >= 110 ) ? "flask_of_the_seventh_demon" : ( true_level >= 100 ) ? "greater_draenic_agility_flask" : ( true_level >= 90 ) ? "spring_blossoms" : ( true_level >= 85 ) ? "winds" : "" );

    precombat -> add_action( flask_action );

    // Added Rune if Flask are allowed since there is no "allow_runes" bool.
    if ( true_level >= 100 )
    {
      std::string rune_action = "augmentation,name=";
      rune_action += ( ( true_level >= 110 ) ? "defiled" : ( true_level >= 100 ) ? "hyper" : "" );

      precombat -> add_action( rune_action );
    }
  }

  // Food
  if ( sim -> allow_food && level() >= 85 )
  {
    std::string food_action = "food,name=";
    if ( specialization() == ROGUE_ASSASSINATION )
      food_action += ( ( level() >= 110 ) ? "seedbattered_fish_plate" : ( level() >= 100 ) ? "jumbo_sea_dog" : ( level() >= 90 ) ? "sea_mist_rice_noodles" : ( level() >= 85 ) ? "seafood_magnifique_feast" : "" );
    else if ( specialization() == ROGUE_OUTLAW )
      food_action += ( ( level() >= 110 ) ? "seedbattered_fish_plate" : ( level() >= 100 ) ? "jumbo_sea_dog" : ( level() >= 90 ) ? "sea_mist_rice_noodles" : ( level() >= 85 ) ? "seafood_magnifique_feast" : "" );
    else if ( specialization() == ROGUE_SUBTLETY )
      food_action += ( ( level() >= 110 ) ? "seedbattered_fish_plate" : ( level() >= 100 ) ? "jumbo_sea_dog" : ( level() >= 90 ) ? "sea_mist_rice_noodles" : ( level() >= 85 ) ? "seafood_magnifique_feast" : "" );

    precombat -> add_action( food_action );
  }

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  if ( specialization() == ROGUE_ASSASSINATION )
  {
    precombat -> add_action( "apply_poison" );
  }

  // Stealth before entering in combat
  precombat -> add_action( this, "Stealth" );

  // Potion
  std::string potion_action = "potion,name=";
  if ( sim -> allow_potions && true_level >= 85 )
  {
    potion_action += ( ( true_level >= 110 ) ? "old_war" : ( true_level >= 100 ) ? "draenic_agility" : ( true_level >= 90 ) ? "virmens_bite" : ( true_level >= 85 ) ? "tolvir" : "" );

    // Pre-Pot
    precombat -> add_action( potion_action );

    // In-Combat Potion
    potion_action += ",if=buff.bloodlust.react|target.time_to_die<=25";
    if ( specialization() == ROGUE_ASSASSINATION )
      potion_action += "|debuff.vendetta.up";
    else if ( specialization() == ROGUE_OUTLAW )
      potion_action += "|buff.adrenaline_rush.up";
    else if ( specialization() == ROGUE_SUBTLETY )
      potion_action += "|buff.shadow_blades.up";
  }

  precombat -> add_talent( this, "Marked for Death", "if=raid_event.adds.in>40" );

  if ( specialization() == ROGUE_ASSASSINATION )
  {
    // New Assa APL WIP
    def -> add_action( "call_action_list,name=cds" );
    def -> add_action( "call_action_list,name=maintain" );
    def -> add_action( "call_action_list,name=finish,if=(!talent.exsanguinate.enabled|cooldown.exsanguinate.remains>2)&(!dot.rupture.refreshable|(dot.rupture.exsanguinated&dot.rupture.remains>=3.5)|target.time_to_die-dot.rupture.remains<=4)&active_dot.rupture>=spell_targets.rupture", "The 'active_dot.rupture>=spell_targets.rupture' means that we don't want to envenom as long as we can multi-rupture (i.e. units that don't have rupture yet)." );
    def -> add_action( "call_action_list,name=build,if=(combo_points.deficit>0|energy.time_to_max<1)" );

    // Builders
    action_priority_list_t* build = get_action_priority_list( "build", "Builders" );
    build -> add_talent( this, "Hemorrhage", "if=refreshable" );
    build -> add_talent( this, "Hemorrhage", "cycle_targets=1,if=refreshable&dot.rupture.ticking&spell_targets.fan_of_knives<=3" );
    build -> add_action( this, "Fan of Knives", "if=spell_targets>=3|buff.the_dreadlords_deceit.stack>=29" );
      // We want to apply poison on the unit that have the most bleeds on and that meet the condition for Venomous Wound (and also for T19 dmg bonus).
      // This would be done with target_if=max:bleeds but it seems to be bugged atm
    build -> add_action( this, "Mutilate", "cycle_targets=1,if=(!talent.agonizing_poison.enabled&dot.deadly_poison_dot.refreshable)|(talent.agonizing_poison.enabled&debuff.agonizing_poison.remains<debuff.agonizing_poison.duration*0.3)|(set_bonus.tier19_2pc=1&dot.mutilated_flesh.refreshable)" );
    build -> add_action( this, "Mutilate" );

    // Cooldowns
    action_priority_list_t* cds = get_action_priority_list( "cds", "Cooldowns" );
    cds -> add_action( potion_action );
    for ( size_t i = 0; i < items.size(); i++ )
    {
      if ( items[ i ].has_use_special_effect() )
      {
        std::string item_action = std::string( "use_item,slot=" ) + items[ i ].slot_name();
        cds -> add_action( item_action + ",if=buff.bloodlust.react|target.time_to_die<=20|debuff.vendetta.up" );
      }
    }
    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "arcane_torrent" )
      {
        cds -> add_action( racial_actions[i] + ",if=debuff.vendetta.up&energy.deficit>50" );
      }
      else
      {
        cds -> add_action( racial_actions[i] + ",if=debuff.vendetta.up" );
      }
    }
    cds -> add_talent( this, "Marked for Death", "target_if=min:target.time_to_die,if=target.time_to_die<combo_points.deficit|combo_points.deficit>=5" );
    cds -> add_action( this, "Vendetta", "if=talent.exsanguinate.enabled&cooldown.exsanguinate.remains<5&dot.rupture.ticking" );
    cds -> add_action( this, "Vendetta", "if=!talent.exsanguinate.enabled&(!artifact.urge_to_kill.enabled|energy.deficit>=70)" );
    cds -> add_action( this, "Vanish", "if=talent.nightstalker.enabled&combo_points>=cp_max_spend&((talent.exsanguinate.enabled&cooldown.exsanguinate.remains<1&(dot.rupture.ticking|time>10))|(!talent.exsanguinate.enabled&dot.rupture.refreshable))" );
    cds -> add_action( this, "Vanish", "if=talent.subterfuge.enabled&dot.garrote.refreshable&((spell_targets.fan_of_knives<=3&combo_points.deficit>=1+spell_targets.fan_of_knives)|(spell_targets.fan_of_knives>=4&combo_points.deficit>=4))" );
    cds -> add_action( this, "Vanish", "if=talent.shadow_focus.enabled&energy.time_to_max>=2&combo_points.deficit>=4" );
    cds -> add_talent( this, "Exsanguinate", "if=prev_gcd.rupture&dot.rupture.remains>4+4*cp_max_spend" );

    // Finishers
    action_priority_list_t* finish = get_action_priority_list( "finish", "Finishers" );
    finish -> add_talent( this, "Death from Above", "if=combo_points>=cp_max_spend" );
    finish -> add_action( this, "Envenom", "if=combo_points>=cp_max_spend-talent.master_poisoner.enabled|(talent.elaborate_planning.enabled&combo_points>=3+!talent.exsanguinate.enabled&buff.elaborate_planning.remains<2)" );

    // Maintain
    action_priority_list_t* maintain = get_action_priority_list( "maintain", "Maintain" );
    maintain -> add_action( this, "Rupture", "if=(talent.nightstalker.enabled&stealthed.rogue)|(talent.exsanguinate.enabled&((combo_points>=cp_max_spend&cooldown.exsanguinate.remains<1)|(!ticking&(time>10|combo_points>=2+artifact.urge_to_kill.enabled))))" );
    maintain -> add_action( this, "Rupture", "cycle_targets=1,if=combo_points>=cp_max_spend-talent.exsanguinate.enabled&refreshable&(!exsanguinated|remains<=1.5)&target.time_to_die-remains>4" );
    maintain -> add_action( this, "Kingsbane", "if=(talent.exsanguinate.enabled&dot.rupture.exsanguinated)|(!talent.exsanguinate.enabled&(debuff.vendetta.up|cooldown.vendetta.remains>10))" );
    maintain -> add_action( "pool_resource,for_next=1" );
    maintain -> add_action( this, "Garrote", "cycle_targets=1,if=refreshable&(!exsanguinated|remains<=1.5)&target.time_to_die-remains>4" );

    /* Skasch APL
    for ( size_t i = 0; i < items.size(); i++ )
    {
      if ( items[ i ].has_use_special_effect() )
      {
        std::string item_action = std::string( "use_item,slot=" ) + items[ i ].slot_name();
        def -> add_action( item_action + ",if=buff.bloodlust.react|target.time_to_die<=20|debuff.vendetta.up" );
      }
    }
    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "arcane_torrent" )
      {
        def -> add_action( racial_actions[i] + ",if=debuff.vendetta.up&energy.deficit>50" );
      }
      else
      {
        def -> add_action( racial_actions[i] + ",if=debuff.vendetta.up" );
      }
    }
    def -> add_action( "call_action_list,name=cds" );
    def -> add_action( this, "Rupture", "if=talent.exsanguinate.enabled&combo_points>=2+artifact.urge_to_kill.enabled*2&!ticking&(artifact.urge_to_kill.enabled|time<10)" );
    def -> add_action( "pool_resource,for_next=1" );
    def -> add_action( this, "Kingsbane", "if=(!talent.exsanguinate.enabled&(debuff.vendetta.up|cooldown.vendetta.remains>10))|(talent.exsanguinate.enabled&dot.rupture.exsanguinated)" );
    // run_action_list forbids the simulator from running the following actions
    def -> add_action( "run_action_list,name=exsang_combo,if=talent.exsanguinate.enabled&cooldown.exsanguinate.remains<3&(debuff.vendetta.up|cooldown.vendetta.remains>25)" );
    def -> add_action( "call_action_list,name=garrote,if=spell_targets.fan_of_knives<=8-artifact.bag_of_tricks.enabled" );
    def -> add_action( "call_action_list,name=exsang,if=dot.rupture.exsanguinated" );
    // Refresh Rupture early to ensure a full pandemic Rupture when casting 
    // Exsanguinate if the timing is not good
    def -> add_action( this, "Rupture", "if=talent.exsanguinate.enabled&remains-cooldown.exsanguinate.remains<(4+cp_max_spend*4)*0.3&new_duration-cooldown.exsanguinate.remains>=(4+cp_max_spend*4)*0.3+3" );
    def -> add_action( "call_action_list,name=finish_ex,if=talent.exsanguinate.enabled" );
    def -> add_action( "call_action_list,name=finish_noex,if=!talent.exsanguinate.enabled" );
    def -> add_action( "call_action_list,name=build_ex,if=talent.exsanguinate.enabled" );
    def -> add_action( "call_action_list,name=build_noex,if=!talent.exsanguinate.enabled" );

    // Cooldowns
    action_priority_list_t* cds = get_action_priority_list( "cds", "Cooldowns" );
      // Targets the target who will die the sooner to fresh MfD
    cds -> add_talent( this, "Marked for Death", "target_if=min:target.time_to_die,if=target.time_to_die<combo_points.deficit|combo_points.deficit>=5" );
      // If Urge to Kill, cast Vendetta sooner to have the time to dump the
      // energy before Exsanguinate
    cds -> add_action( this, "Vendetta", "if=target.time_to_die<20" );
    cds -> add_action( this, "Vendetta", "if=dot.rupture.ticking&(!talent.exsanguinate.enabled|cooldown.exsanguinate.remains<1+4*!artifact.urge_to_kill.enabled)&(energy<55|time<10|spell_targets.fan_of_knives>=2|!artifact.urge_to_kill.enabled)" );
      // Gives as much time as possible to spam Garrote if Subterfuge enabled 
      // (only useful on AoE)
    cds -> add_action( this, "Vanish", "if=!dot.rupture.exsanguinated&((talent.subterfuge.enabled&combo_points<=2)|(talent.shadow_focus.enabled&combo_points.deficit>=2))" );
    cds -> add_action( this, "Vanish", "if=!talent.exsanguinate.enabled&talent.nightstalker.enabled&combo_points>=cp_max_spend&gcd.remains=0&energy>=25" );

    // Exsanguinate Combo
    action_priority_list_t* exsang_combo = get_action_priority_list( "exsang_combo", "Exsanguinate Combo" );
      // Syncing Vanish with Rupture
    exsang_combo -> add_action( this, "Vanish", "if=talent.nightstalker.enabled&combo_points>=cp_max_spend&cooldown.exsanguinate.remains<1&gcd.remains=0&energy>=25" );
    exsang_combo -> add_action( this, "Rupture", "if=combo_points>=cp_max_spend&cooldown.exsanguinate.remains<1&(!talent.nightstalker.enabled|buff.vanish.up|cooldown.vanish.remains>15)" );
      // Some safeguards to make sure Exsanguinate is casted at the right moment
    exsang_combo -> add_talent( this, "Exsanguinate", "if=prev_gcd.rupture&dot.rupture.remains>22+4*talent.deeper_stratagem.enabled&cooldown.vanish.remains>10" );
    exsang_combo -> add_action( "call_action_list,name=garrote,if=spell_targets.fan_of_knives<=8-artifact.bag_of_tricks.enabled" );
      // AoE
    exsang_combo -> add_talent( this, "Hemorrhage", "if=spell_targets.fan_of_knives>=2&!ticking" );
    exsang_combo -> add_action( "call_action_list,name=build_ex" );

    // Exsanguinated Finishers
    action_priority_list_t* exsang = get_action_priority_list( "exsang", "Exsanguinated Finishers" );
      // AoE
    exsang -> add_action( this, "Rupture", "cycle_targets=1,max_cycle_targets=14-2*artifact.bag_of_tricks.enabled,if=!ticking&combo_points>=cp_max_spend-1&spell_targets.fan_of_knives>1&target.time_to_die>6" );
      // Single
      // Wait for Combo Points if Rupture is about to expire to reapply it as soon as possible
    exsang -> add_action( this, "Rupture", "if=combo_points>=cp_max_spend&ticks_remain<2" );
    exsang -> add_talent( this, "Death from Above", "if=combo_points>=cp_max_spend-1&(dot.rupture.remains>3|(dot.rupture.remains>2&spell_targets.fan_of_knives>=3))&(artifact.bag_of_tricks.enabled|spell_targets.fan_of_knives<=6+2*debuff.vendetta.up)" );
    exsang -> add_action( this, "Envenom", "if=combo_points>=cp_max_spend-1&(dot.rupture.remains>3|(dot.rupture.remains>2&spell_targets.fan_of_knives>=3))&(artifact.bag_of_tricks.enabled|spell_targets.fan_of_knives<=6+2*debuff.vendetta.up)" );

    // Builders Exsanguinate
    action_priority_list_t* build_ex = get_action_priority_list( "build_ex", "Builders Exsanguinate" );
      // AoE
    build_ex -> add_talent( this, "Hemorrhage", "cycle_targets=1,if=combo_points.deficit>=1&refreshable&dot.rupture.remains>6&spell_targets.fan_of_knives>1&spell_targets.fan_of_knives<=4" );
    build_ex -> add_talent( this, "Hemorrhage", "cycle_targets=1,max_cycle_targets=3,if=combo_points.deficit>=1&refreshable&dot.rupture.remains>6&spell_targets.fan_of_knives>1&spell_targets.fan_of_knives=5" );
      // Replaces Envenom with Fan of Knives after 7 targets / 9 with Vendetta
    build_ex -> add_action( this, "Fan of Knives", "if=(spell_targets>=2+debuff.vendetta.up&(combo_points.deficit>=1|energy.deficit<=30))|(!artifact.bag_of_tricks.enabled&spell_targets>=7+2*debuff.vendetta.up)" );
      // Single
    build_ex -> add_action( this, "Fan of Knives", "if=(debuff.vendetta.up&buff.the_dreadlords_deceit.stack>=29-(debuff.vendetta.remains<=3)*14)|(cooldown.vendetta.remains>60&cooldown.vendetta.remains<65&buff.the_dreadlords_deceit.stack>=5)" );
    build_ex -> add_talent( this, "Hemorrhage", "if=(combo_points.deficit>=1&refreshable)|(combo_points.deficit=1&((dot.rupture.exsanguinated&dot.rupture.remains<=2)|cooldown.exsanguinate.remains<=2))" );
    build_ex -> add_action( this, "Mutilate", "if=combo_points.deficit<=1&energy.deficit<=30" );
    build_ex -> add_action( this, "Mutilate", "if=combo_points.deficit>=2&cooldown.garrote.remains>2" );

    // Builder no Exsanguinate
    action_priority_list_t* build_noex = get_action_priority_list( "build_noex", "Builders no Exsanguinate" );
    build_noex -> add_talent( this, "Hemorrhage", "cycle_targets=1,if=combo_points.deficit>=1&refreshable&dot.rupture.remains>6&spell_targets.fan_of_knives>1&spell_targets.fan_of_knives<=4" );
    build_noex -> add_talent( this, "Hemorrhage", "cycle_targets=1,max_cycle_targets=3,if=combo_points.deficit>=1&refreshable&dot.rupture.remains>6&spell_targets.fan_of_knives>1&spell_targets.fan_of_knives=5" );
    build_noex -> add_action( this, "Fan of Knives", "if=(spell_targets>=2+debuff.vendetta.up&(combo_points.deficit>=1|energy.deficit<=30))|(!artifact.bag_of_tricks.enabled&spell_targets>=7+2*debuff.vendetta.up)" );
    build_noex -> add_action( this, "Fan of Knives", "if=(debuff.vendetta.up&buff.the_dreadlords_deceit.stack>=29-(debuff.vendetta.remains<=3)*14)|(cooldown.vendetta.remains>60&cooldown.vendetta.remains<65&buff.the_dreadlords_deceit.stack>=5)" );
    build_noex -> add_talent( this, "Hemorrhage", "if=combo_points.deficit>=1&refreshable" );
    build_noex -> add_action( this, "Mutilate", "if=combo_points.deficit>=1&cooldown.garrote.remains>2" );

    // Finishers Exsanguinate
    action_priority_list_t* finish_ex = get_action_priority_list( "finish_ex", "Finishers Exsanguinate" );
      // AoE
    finish_ex -> add_action( this, "Rupture", "cycle_targets=1,max_cycle_targets=14-2*artifact.bag_of_tricks.enabled,if=!ticking&combo_points>=cp_max_spend-1&spell_targets.fan_of_knives>1&target.time_to_die-remains>6" );
      // Single
    finish_ex -> add_action( this, "Rupture", "if=combo_points>=cp_max_spend-1&refreshable&!exsanguinated" );
    finish_ex -> add_talent( this, "Death from Above", "if=combo_points>=cp_max_spend-1&(artifact.bag_of_tricks.enabled|spell_targets.fan_of_knives<=6)" );
    finish_ex -> add_action( this, "Envenom", "if=combo_points>=cp_max_spend-1&!dot.rupture.refreshable&buff.elaborate_planning.remains<2&energy.deficit<40&(artifact.bag_of_tricks.enabled|spell_targets.fan_of_knives<=6)" );
    finish_ex -> add_action( this, "Envenom", "if=combo_points>=cp_max_spend&!dot.rupture.refreshable&buff.elaborate_planning.remains<2&cooldown.garrote.remains<1&(artifact.bag_of_tricks.enabled|spell_targets.fan_of_knives<=6)" );

    // Finishers no Exsanguinate
    action_priority_list_t* finish_noex = get_action_priority_list( "finish_noex", "Finishers no Exsanguinate" );
    finish_noex -> add_action( "variable,name=envenom_condition,value=!(dot.rupture.refreshable&dot.rupture.pmultiplier<1.5)&(!talent.nightstalker.enabled|cooldown.vanish.remains>=6)&dot.rupture.remains>=6&buff.elaborate_planning.remains<1.5&(artifact.bag_of_tricks.enabled|spell_targets.fan_of_knives<=6)" );
    finish_noex -> add_action( this, "Rupture", "cycle_targets=1,max_cycle_targets=14-2*artifact.bag_of_tricks.enabled,if=!ticking&combo_points>=cp_max_spend&spell_targets.fan_of_knives>1&target.time_to_die-remains>6" );
    finish_noex -> add_action( this, "Rupture", "if=combo_points>=cp_max_spend&((dot.rupture.refreshable|dot.rupture.ticks_remain<=1)|(talent.nightstalker.enabled&buff.vanish.up))" );
    finish_noex -> add_talent( this, "Death from Above", "if=variable.envenom_condition&(combo_points>=cp_max_spend-2*talent.elaborate_planning.enabled)&(refreshable|(talent.elaborate_planning.enabled&!buff.elaborate_planning.up)|cooldown.garrote.remains<1)" );
    finish_noex -> add_action( this, "Envenom", "if=variable.envenom_condition&(combo_points>=cp_max_spend-2*talent.elaborate_planning.enabled)&(refreshable|(talent.elaborate_planning.enabled&!buff.elaborate_planning.up)|cooldown.garrote.remains<1)" );

    // Garrote
    action_priority_list_t* garrote = get_action_priority_list( "garrote", "Garrote" );
    garrote -> add_action( "pool_resource,for_next=1" );
    garrote -> add_action( this, "Garrote", "cycle_targets=1,if=talent.subterfuge.enabled&!ticking&combo_points.deficit>=1&spell_targets.fan_of_knives>=2" );
    garrote -> add_action( "pool_resource,for_next=1" );
    garrote -> add_action( this, "Garrote", "if=combo_points.deficit>=1&!exsanguinated&refreshable" );
    */
  }
  else if ( specialization() == ROGUE_OUTLAW )
  {
    // Pre-Combat
    precombat -> add_action( this, "Roll the Bones", "if=!talent.slice_and_dice.enabled" );

    // Main Rotation
    def -> add_action( "variable,name=rtb_reroll,value=!talent.slice_and_dice.enabled&(rtb_buffs<=1&!rtb_list.any.6&((!buff.curse_of_the_dreadblades.up&!buff.adrenaline_rush.up)|!rtb_list.any.5))", "Condition to continue rerolling RtB (2- or not TB alone or not SIW alone during CDs); If SnD: consider that you never have to reroll." );
      // variable,name=rtb_reroll,value=!talent.slice_and_dice.enabled&(rtb_buffs<=1|rtb_buffs=2&!rtb_list.any.56) is better in average but not really good in practical. (Fish 3+ or 2+ TB or 2+ SIW)
    def -> add_action( "variable,name=ss_useable_noreroll,value=(combo_points<5+talent.deeper_stratagem.enabled-(buff.broadsides.up|buff.jolly_roger.up)-(talent.alacrity.enabled&buff.alacrity.stack<=4))", "Condition to use Saber Slash when not rerolling RtB or when using SnD" );
    def -> add_action( "variable,name=ss_useable,value=(talent.anticipation.enabled&combo_points<4)|(!talent.anticipation.enabled&((variable.rtb_reroll&combo_points<4+talent.deeper_stratagem.enabled)|(!variable.rtb_reroll&variable.ss_useable_noreroll)))", "Condition to use Saber Slash, when you have RtB or not" );
    def -> add_action( "call_action_list,name=bf", "Normal rotation" );
    def -> add_action( "call_action_list,name=cds" );
    def -> add_action( "call_action_list,name=stealth,if=stealthed.rogue|cooldown.vanish.up|cooldown.shadowmeld.up", "Conditions are here to avoid worthless check if nothing is available" );
      // Make DfA have priority over RtB
    def -> add_talent( this, "Death from Above", "if=energy.time_to_max>2&!variable.ss_useable_noreroll" );
      // Pandemic is (6 + 6 * CP) * 0.3, ie (1 + CP) * 1.8
    def -> add_talent( this, "Slice and Dice", "if=!variable.ss_useable&buff.slice_and_dice.remains<target.time_to_die&buff.slice_and_dice.remains<(1+combo_points)*1.8" );
      // Reroll unless 2+ buffs or legendary boot & true bearing
    def -> add_action( this, "Roll the Bones", "if=!variable.ss_useable&buff.roll_the_bones.remains<target.time_to_die&(buff.roll_the_bones.remains<=3|variable.rtb_reroll)" );
    def -> add_talent( this, "Killing Spree", "if=energy.time_to_max>5|energy<15" );
    def -> add_action( "call_action_list,name=build" );
    def -> add_action( "call_action_list,name=finish,if=!variable.ss_useable" );
    def -> add_action( this, "Gouge", "if=talent.dirty_tricks.enabled&combo_points.deficit>=1", "Gouge is used as a CP Generator while nothing else is available and you have Dirty Tricks talent. It's unlikely that you'll be able to do this optimally in-game since it requires to move in front of the target, but it's here so you can quantifiy its value." );

    // Builders
    action_priority_list_t* build = get_action_priority_list( "build", "Builders" );
    build -> add_talent( this, "Ghostly Strike", "if=combo_points.deficit>=1+buff.broadsides.up&!buff.curse_of_the_dreadblades.up&(debuff.ghostly_strike.remains<debuff.ghostly_strike.duration*0.3|(cooldown.curse_of_the_dreadblades.remains<3&debuff.ghostly_strike.remains<14))&(combo_points>=3|(variable.rtb_reroll&time>=10))" );
    build -> add_action( this, "Pistol Shot", "if=combo_points.deficit>=1+buff.broadsides.up&buff.opportunity.up&(energy.time_to_max>2-talent.quick_draw.enabled|(buff.blunderbuss.up&buff.greenskins_waterlogged_wristcuffs.up))" );
    build -> add_action( this, "Saber Slash", "if=variable.ss_useable" );

    // Blade Flurry
    action_priority_list_t* bf = get_action_priority_list( "bf", "Blade Flurry" );
      // Cancels Blade Flurry buff on CD to maximize Shiarran Symmetry effect
    bf -> add_action( "cancel_buff,name=blade_flurry,if=equipped.shivarran_symmetry&cooldown.blade_flurry.up&buff.blade_flurry.up&spell_targets.blade_flurry>=2|spell_targets.blade_flurry<2&buff.blade_flurry.up" );
    bf -> add_action( this, "Blade Flurry", "if=spell_targets.blade_flurry>=2&!buff.blade_flurry.up" );

    // Cooldowns
    action_priority_list_t* cds = get_action_priority_list( "cds", "Cooldowns" );
    cds -> add_action( potion_action );
    for ( size_t i = 0; i < item_actions.size(); i++ )
    {
      cds -> add_action( item_actions[i] + ",if=buff.bloodlust.react|target.time_to_die<=20|combo_points.deficit<=2" );
    }
    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "arcane_torrent" )
        cds -> add_action( racial_actions[i] + ",if=energy.deficit>40" );
      else
        cds -> add_action( racial_actions[i] );
    }
    cds -> add_talent( this, "Cannonball Barrage", "if=spell_targets.cannonball_barrage>=1" );
    cds -> add_action( this, "Adrenaline Rush", "if=!buff.adrenaline_rush.up&energy.deficit>0" );
    cds -> add_talent( this, "Marked for Death", "target_if=min:target.time_to_die,if=target.time_to_die<combo_points.deficit|((raid_event.adds.in>40|buff.true_bearing.remains>15)&combo_points.deficit>=4+talent.deeper_strategem.enabled+talent.anticipation.enabled)" );
    cds -> add_action( this, "Sprint", "if=equipped.thraxis_tricksy_treads&!variable.ss_useable" );
    cds -> add_action( this, "Curse of the Dreadblades", "if=combo_points.deficit>=4&(!talent.ghostly_strike.enabled|debuff.ghostly_strike.up)" );

    // Finishers
    action_priority_list_t* finish = get_action_priority_list( "finish", "Finishers" );
    finish -> add_action( this, "Between the Eyes", "if=equipped.greenskins_waterlogged_wristcuffs&!buff.greenskins_waterlogged_wristcuffs.up" );
    finish -> add_action( this, "Run Through", "if=!talent.death_from_above.enabled|energy.time_to_max<cooldown.death_from_above.remains+3.5" );

    // Stealth
    action_priority_list_t* stealth = get_action_priority_list( "stealth", "Stealth" );
    stealth -> add_action( "variable,name=stealth_condition,value=(combo_points.deficit>=2+2*(talent.ghostly_strike.enabled&!debuff.ghostly_strike.up)+buff.broadsides.up&energy>60&!buff.jolly_roger.up&!buff.hidden_blade.up&!buff.curse_of_the_dreadblades.up)", "Condition to use stealth abilities" );
    stealth -> add_action( this, "Ambush" );
    stealth -> add_action( this, "Vanish", "if=variable.stealth_condition" );
    stealth -> add_action( "shadowmeld,if=variable.stealth_condition" );
  }
  else if ( specialization() == ROGUE_SUBTLETY )
  {
    // Pre-Combat
    precombat -> add_action( this, "Enveloping Shadows", "if=combo_points>=5" );
    precombat -> add_action( this, "Symbols of Death" );

    // Main Rotation
    def -> add_action( "variable,name=ssw_er,value=equipped.shadow_satyrs_walk*(10+floor(target.distance*0.5))" );
    def -> add_action( "variable,name=ed_threshold,value=energy.deficit<=(20+talent.vigor.enabled*35+talent.master_of_shadows.enabled*25+variable.ssw_er)" );
    def -> add_action( "call_action_list,name=cds" );
    def -> add_action( "run_action_list,name=stealthed,if=stealthed.all", "Fully switch to the Stealthed Rotation (by doing so, it forces pooling if nothing is available)" );
    def -> add_action( "call_action_list,name=finish,if=combo_points>=5|(combo_points>=4&spell_targets.shuriken_storm>=3&spell_targets.shuriken_storm<=4)" );
    def -> add_action( "call_action_list,name=stealth_cds,if=combo_points.deficit>=2+talent.premeditation.enabled&(variable.ed_threshold|(cooldown.shadowmeld.up&!cooldown.vanish.up&cooldown.shadow_dance.charges<=1)|target.time_to_die<12|spell_targets.shuriken_storm>=5)" );
    def -> add_action( "call_action_list,name=build,if=variable.ed_threshold" );

    // Cooldowns
    action_priority_list_t* cds = get_action_priority_list( "cds", "Cooldowns" );
    cds -> add_action( potion_action );
    for ( size_t i = 0; i < item_actions.size(); i++ )
      cds -> add_action( item_actions[i] + ",if=(buff.shadow_blades.up&stealthed.rogue)|target.time_to_die<20" );
    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "arcane_torrent" )
        cds -> add_action( racial_actions[i] + ",if=stealthed.rogue&energy.deficit>70" );
      else
        cds -> add_action( racial_actions[i] + ",if=stealthed.rogue" );
    }
    cds -> add_action( this, "Shadow Blades", "if=!stealthed.all" );
    cds -> add_action( this, "Goremaw's Bite", "if=!stealthed.all&((combo_points.deficit>=4-(time<10)*2&energy.deficit>50+talent.vigor.enabled*25-(time>=10)*15)|target.time_to_die<8)" );
    cds -> add_talent( this, "Marked for Death", "target_if=min:target.time_to_die,if=target.time_to_die<combo_points.deficit|(raid_event.adds.in>40&combo_points.deficit>=4+talent.deeper_strategem.enabled+talent.anticipation.enabled)" );

    // Builders
    action_priority_list_t* build = get_action_priority_list( "build", "Builders" );
    build -> add_action( this, "Shuriken Storm", "if=spell_targets.shuriken_storm>=2" );
    build -> add_talent( this, "Gloomblade" );
    build -> add_action( this, "Backstab" );

    // Finishers
    action_priority_list_t* finish = get_action_priority_list( "finish", "Finishers" );
      // Pandemic is 6 * CP * 0.3, ie CP * 1.8
    finish -> add_talent( this, "Enveloping Shadows", "if=buff.enveloping_shadows.remains<target.time_to_die&buff.enveloping_shadows.remains<=combo_points*1.8" );
    finish -> add_talent( this, "Death from Above", "if=spell_targets.death_from_above>=6" );
      // It is not worth to override a normal nightblade for a finality one outside of pandemic threshold, it is worth to wait the end of the finality to refresh it unless you already got the finality buff.
    finish -> add_action( this, "Nightblade", "cycle_targets=1,if=target.time_to_die>8&((refreshable&(!finality|buff.finality_nightblade.up))|remains<tick_time)" );
    finish -> add_talent( this, "Death from Above" );
    finish -> add_action( this, "Eviscerate" );

    // Stealthed Rotation
    action_priority_list_t* stealthed = get_action_priority_list( "stealthed", "Stealthed Rotation" );
      // Added buff.shadowmeld.down to avoid using it since it's not usable while shadowmelded "yet" (soonTM ?)
    stealthed -> add_action( this, "Symbols of Death", "if=buff.shadowmeld.down&((buff.symbols_of_death.remains<target.time_to_die-4&buff.symbols_of_death.remains<=buff.symbols_of_death.duration*0.3)|(equipped.shadow_satyrs_walk&energy.time_to_max<0.25))" );
    stealthed -> add_action( "call_action_list,name=finish,if=combo_points>=5" );
    stealthed -> add_action( this, "Shuriken Storm", "if=buff.shadowmeld.down&((combo_points.deficit>=3&spell_targets.shuriken_storm>=2+talent.premeditation.enabled+equipped.shadow_satyrs_walk)|buff.the_dreadlords_deceit.stack>=29)" );
    stealthed -> add_action( this, "Shadowstrike" );

    // Stealth Cooldowns
    action_priority_list_t* stealth_cds = get_action_priority_list( "stealth_cds", "Stealth Cooldowns" );
    stealth_cds -> add_action( this, "Shadow Dance", "if=charges_fractional>=2.45" );
    stealth_cds -> add_action( this, "Vanish" );
    stealth_cds -> add_action( this, "Shadow Dance", "if=charges>=2&combo_points<=1" );
    stealth_cds -> add_action( "pool_resource,for_next=1,extra_amount=40-variable.ssw_er" );
    stealth_cds -> add_action( "shadowmeld,if=energy>=40-variable.ssw_er&energy.deficit>10" );
    stealth_cds -> add_action( this, "Shadow Dance", "if=combo_points<=1" );
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// rogue_t::create_action  ==================================================

action_t* rogue_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  using namespace actions;

  if ( name == "adrenaline_rush"     ) return new adrenaline_rush_t    ( this, options_str );
  if ( name == "ambush"              ) return new ambush_t             ( this, options_str );
  if ( name == "apply_poison"        ) return new apply_poison_t       ( this, options_str );
  if ( name == "auto_attack"         ) return new auto_melee_attack_t  ( this, options_str );
  if ( name == "backstab"            ) return new backstab_t           ( this, options_str );
  if ( name == "between_the_eyes"    ) return new between_the_eyes_t   ( this, options_str );
  if ( name == "blade_flurry"        ) return new blade_flurry_t       ( this, options_str );
  if ( name == "cannonball_barrage"  ) return new cannonball_barrage_t ( this, options_str );
  if ( name == "curse_of_the_dreadblades" ) return new curse_of_the_dreadblades_t( this, options_str );
  if ( name == "death_from_above"    ) return new death_from_above_t   ( this, options_str );
  if ( name == "enveloping_shadows"  ) return new enveloping_shadows_t ( this, options_str );
  if ( name == "envenom"             ) return new envenom_t            ( this, options_str );
  if ( name == "eviscerate"          ) return new eviscerate_t         ( this, options_str );
  if ( name == "exsanguinate"        ) return new exsanguinate_t       ( this, options_str );
  if ( name == "fan_of_knives"       ) return new fan_of_knives_t      ( this, options_str );
  if ( name == "feint"               ) return new feint_t              ( this, options_str );
  if ( name == "garrote"             ) return new garrote_t            ( this, options_str );
  if ( name == "gouge"               ) return new gouge_t              ( this, options_str );
  if ( name == "ghostly_strike"      ) return new ghostly_strike_t     ( this, options_str );
  if ( name == "gloomblade"          ) return new gloomblade_t         ( this, options_str );
  if ( name == "goremaws_bite"       ) return new goremaws_bite_t      ( this, options_str );
  if ( name == "hemorrhage"          ) return new hemorrhage_t         ( this, options_str );
  if ( name == "kick"                ) return new kick_t               ( this, options_str );
  if ( name == "kidney_shot"         ) return new kidney_shot_t        ( this, options_str );
  if ( name == "killing_spree"       ) return new killing_spree_t      ( this, options_str );
  if ( name == "kingsbane"           ) return new kingsbane_t          ( this, options_str );
  if ( name == "marked_for_death"    ) return new marked_for_death_t   ( this, options_str );
  if ( name == "mutilate"            ) return new mutilate_t           ( this, options_str );
  if ( name == "nightblade"          ) return new nightblade_t         ( this, options_str );
  if ( name == "pistol_shot"         ) return new pistol_shot_t        ( this, options_str );
  if ( name == "poisoned_knife"      ) return new poisoned_knife_t     ( this, options_str );
  if ( name == "roll_the_bones"      ) return new roll_the_bones_t     ( this, options_str );
  if ( name == "run_through"         ) return new run_through_t        ( this, options_str );
  if ( name == "rupture"             ) return new rupture_t            ( this, options_str );
  if ( name == "saber_slash"         ) return new saber_slash_t        ( this, options_str );
  if ( name == "shadow_blades"       ) return new shadow_blades_t      ( this, options_str );
  if ( name == "shadow_dance"        ) return new shadow_dance_t       ( this, options_str );
  if ( name == "shadowstep"          ) return new shadowstep_t         ( this, options_str );
  if ( name == "shadowstrike"        ) return new shadowstrike_t       ( this, options_str );
  if ( name == "shuriken_storm"      ) return new shuriken_storm_t     ( this, options_str );
  if ( name == "shuriken_toss"       ) return new shuriken_toss_t      ( this, options_str );
  if ( name == "slice_and_dice"      ) return new slice_and_dice_t     ( this, options_str );
  if ( name == "sprint"              ) return new sprint_t             ( this, options_str );
  if ( name == "stealth"             ) return new stealth_t            ( this, options_str );
  if ( name == "symbols_of_death"    ) return new symbols_of_death_t   ( this, options_str );
  if ( name == "vanish"              ) return new vanish_t             ( this, options_str );
  if ( name == "vendetta"            ) return new vendetta_t           ( this, options_str );

  if ( name == "swap_weapon"         ) return new weapon_swap_t        ( this, options_str );

  return player_t::create_action( name, options_str );
}

// rogue_t::create_expression ===============================================

expr_t* rogue_t::create_expression( action_t* a, const std::string& name_str )
{
  std::vector<std::string> split = util::string_split( name_str, "." );

  if ( name_str == "combo_points" )
    return make_ref_expr( name_str, resources.current[ RESOURCE_COMBO_POINT ] );
  else if ( util::str_compare_ci( name_str, "poisoned_enemies" ) )
    return make_ref_expr( name_str, poisoned_enemies );
  else if ( util::str_compare_ci( name_str, "cp_max_spend" ) )
  {
    if ( ! dynamic_cast<actions::rogue_attack_t*>( a ) )
    {
      return expr_t::create_constant( name_str, 0 );
    }
    else
    {
      return make_mem_fn_expr( name_str, *this, &rogue_t::consume_cp_max );
    }
  }
  else if ( util::str_compare_ci( name_str, "rtb_buffs" ) )
  {
    if ( specialization() != ROGUE_OUTLAW || talent.slice_and_dice -> ok() )
    {
      return expr_t::create_constant( name_str, 0 );
    }

    return make_fn_expr( name_str, [ this ]() {
      double n_buffs = 0;
      n_buffs += buffs.jolly_roger -> check() != 0;
      n_buffs += buffs.grand_melee -> check() != 0;
      n_buffs += buffs.shark_infested_waters -> check() != 0;
      n_buffs += buffs.true_bearing -> check() != 0;
      n_buffs += buffs.broadsides -> check() != 0;
      n_buffs += buffs.buried_treasure -> check() != 0;
      return n_buffs;
    } );
  }

  // Split expressions

  // stealthed.(rogue|all)
  // rogue: all rogue abilities are checked (stealth, vanish, shadow_dance)
  // all: all abilities that allow stealth are checked (rogue + shadowmeld)
  if ( split.size() == 2 && util::str_compare_ci( split[ 0 ], "stealthed" ) )
  {
    if ( util::str_compare_ci( split[ 1 ], "rogue" ) )
    {
      return make_fn_expr( split[ 0 ], [ this ]() {
        return buffs.stealth -> check() || buffs.vanish -> check() || buffs.shadow_dance -> check();
      } );
    }
    else if ( util::str_compare_ci( split[ 1 ], "all" ) )
    {
      return make_fn_expr( split[ 0 ], [ this ]() {
        return buffs.stealth -> check() || buffs.vanish -> check() || buffs.shadow_dance -> check() || this -> player_t::buffs.shadowmeld -> check();
      } );
    }
  }
  // dot.(rupture|garrote).exsanguinated
  else if ( split.size() == 3 && util::str_compare_ci( split[ 2 ], "exsanguinated" ) &&
       ( util::str_compare_ci( split[ 1 ], "rupture" ) || util::str_compare_ci( split[ 1 ], "garrote" ) ) )
  {
    action_t* action = find_action( split[ 1 ] );
    if ( ! action )
    {
      return expr_t::create_constant( "exsanguinated_expr", 0 );
    }

    return new exsanguinated_expr_t( action );
  }
  // rtb_list.<buffs>
  else if ( split.size() == 3 && util::str_compare_ci( split[ 0 ], "rtb_list" ) && ! split[ 1 ].empty() )
  {
    enum rtb_list_type
    {
      RTB_NONE = 0,
      RTB_ANY,
      RTB_ALL
    };

    const std::array<const buff_t*, 6> rtb_buffs = { {
      buffs.broadsides,
      buffs.buried_treasure,
      buffs.grand_melee,
      buffs.jolly_roger,
      buffs.shark_infested_waters,
      buffs.true_bearing
    } };

    // Figure out the type
    rtb_list_type type = RTB_NONE;
    if ( util::str_compare_ci( split[ 1 ], "any" ) )
    {
      type = RTB_ANY;
    }
    else if ( util::str_compare_ci( split[ 1 ], "all" ) )
    {
      type = RTB_ALL;
    }

    // Parse the buff numeric list to values that index rtb_buffs above
    std::vector<unsigned> list_values;
    range::for_each( split[ 2 ], [ &list_values ]( char v ) {
      if ( v < '1' || v > '6' )
      {
        return;
      }

      list_values.push_back( v - '1' );
    } );

    // If we have buffs and an operating mode, make an expression
    if ( type != RTB_NONE && list_values.size() > 0 )
    {
      return make_fn_expr( split[ 0 ], [ type, rtb_buffs, list_values ]() {
        for ( size_t i = 0, end = list_values.size(); i < end; ++i )
        {
          if  ( type == RTB_ANY && rtb_buffs[ list_values[ i ] ] -> check() )
          {
            return 1;
          }
          else if ( type == RTB_ALL && ! rtb_buffs[ list_values[ i ] ] -> check() )
          {
            return 0;
          }
        }

        return type == RTB_ANY ? 0 : 1;
      } );
    }
  }

  return player_t::create_expression( a, name_str );
}

// rogue_t::init_base =======================================================

void rogue_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;

  resources.base[ RESOURCE_COMBO_POINT ] = 5;
  resources.base[ RESOURCE_COMBO_POINT ] += talent.deeper_stratagem -> effectN( 1 ).base_value() ;
  resources.base[ RESOURCE_COMBO_POINT ] += talent.anticipation -> effectN( 1 ).base_value();

  resources.base[ RESOURCE_ENERGY ] = 100;
  resources.base[ RESOURCE_ENERGY ] += talent.vigor -> effectN( 1 ).base_value();

  if ( main_hand_weapon.type == WEAPON_DAGGER && off_hand_weapon.type == WEAPON_DAGGER )
    resources.base[ RESOURCE_ENERGY ] += spec.assassins_resolve -> effectN( 1 ).base_value();
  //if ( sets.has_set_bonus( SET_MELEE, PVP, B2 ) )
  //  resources.base[ RESOURCE_ENERGY ] += 10;

  base_energy_regen_per_second = 10 * ( 1.0 + spec.vitality -> effectN( 1 ).percent() );
  base_energy_regen_per_second *= 1.0 + talent.vigor -> effectN( 2 ).percent();

  base_gcd = timespan_t::from_seconds( 1.0 );
  min_gcd  = timespan_t::from_seconds( 1.0 );
}

// rogue_t::init_spells =====================================================

void rogue_t::init_spells()
{
  player_t::init_spells();

  // Shared
  spec.shadowstep           = find_specialization_spell( "Shadowstep" );

  // Generic
  spec.subtlety_rogue       = find_specialization_spell( "Subtlety Rogue" );
  spec.outlaw_rogue         = find_specialization_spell( "Outlaw Rogue" );

  // Assassination
  spec.assassins_resolve    = find_specialization_spell( "Assassin's Resolve" );
  spec.improved_poisons     = find_specialization_spell( "Improved Poisons" );
  spec.seal_fate            = find_specialization_spell( "Seal Fate" );
  spec.venomous_wounds      = find_specialization_spell( "Venomous Wounds" );
  spec.vendetta             = find_specialization_spell( "Vendetta" );
  spec.garrote              = find_specialization_spell( "Garrote" );
  spec.garrote_2            = find_specialization_spell( 231719 );

  // Outlaw
  spec.blade_flurry         = find_specialization_spell( "Blade Flurry" );
  spec.combat_potency       = find_specialization_spell( "Combat Potency" );
  spec.roll_the_bones       = find_specialization_spell( "Roll the Bones" );
  spec.ruthlessness         = find_specialization_spell( "Ruthlessness" );
  spec.saber_slash          = find_specialization_spell( "Saber Slash" );
  spec.vitality             = find_specialization_spell( "Vitality" );

  // Subtlety
  spec.deepening_shadows    = find_specialization_spell( "Deepening Shadows" );
  spec.energetic_recovery   = find_specialization_spell( "Energetic Recovery" );
  spec.relentless_strikes   = find_specialization_spell( "Relentless Strikes" );
  spec.shadow_blades        = find_specialization_spell( "Shadow Blades" );
  spec.shadow_dance         = find_specialization_spell( "Shadow Dance" );
  spec.shadow_techniques    = find_specialization_spell( "Shadow Techniques" );
  spec.symbols_of_death     = find_specialization_spell( "Symbols of Death" );
  spec.eviscerate           = find_specialization_spell( "Eviscerate" );
  spec.eviscerate_2         = find_specialization_spell( 231716 );
  spec.shadowstrike         = find_specialization_spell( "Shadowstrike" );
  spec.shadowstrike_2       = find_specialization_spell( 231718 );

  // Masteries
  mastery.potent_poisons    = find_mastery_spell( ROGUE_ASSASSINATION );
  mastery.main_gauche       = find_mastery_spell( ROGUE_OUTLAW );
  mastery.executioner       = find_mastery_spell( ROGUE_SUBTLETY );

  // Misc spells
  spell.bag_of_tricks_driver        = find_spell( 192661 );
  spell.critical_strikes            = find_spell( 157442 );
  spell.death_from_above            = find_spell( 163786 );
  spell.fan_of_knives               = find_class_spell( "Fan of Knives" );
  spell.fleet_footed                = find_spell( 31209 );
  spell.master_of_shadows           = find_spell( 196980 );
  spell.sprint                      = find_class_spell( "Sprint" );
  spell.ruthlessness_driver         = find_spell( 14161 );
  spell.ruthlessness_cp             = spec.ruthlessness -> effectN( 1 ).trigger();
  spell.shadow_focus                = find_spell( 112942 );
  spell.subterfuge                  = find_spell( 115192 );
  spell.tier18_2pc_combat_ar        = find_spell( 186286 );
  spell.relentless_strikes_energize = find_spell( 98440 );
  spell.insignia_of_ravenholdt      = find_spell( 209041 );

  // Talents
  talent.deeper_stratagem   = find_talent_spell( "Deeper Stratagem" );
  talent.anticipation       = find_talent_spell( "Anticipation" );
  talent.vigor              = find_talent_spell( "Vigor" );

  talent.alacrity           = find_talent_spell( "Alacrity" );

  talent.marked_for_death   = find_talent_spell( "Marked for Death" );
  talent.death_from_above   = find_talent_spell( "Death from Above" );

  talent.nightstalker       = find_talent_spell( "Nightstalker" );
  talent.subterfuge         = find_talent_spell( "Subterfuge" );
  talent.shadow_focus       = find_talent_spell( "Shadow Focus" );

  talent.master_poisoner    = find_talent_spell( "Master Poisoner" );
  talent.elaborate_planning = find_talent_spell( "Elaborate Planning" );
  talent.hemorrhage         = find_talent_spell( "Hemorrhage" );

  talent.thuggee            = find_talent_spell( "Thuggee" );
  talent.internal_bleeding  = find_talent_spell( "Internal Bleeding" );

  talent.agonizing_poison   = find_talent_spell( "Agonizing Poison" );
  talent.exsanguinate       = find_talent_spell( "Exsanguinate" );

  talent.venom_rush         = find_talent_spell( "Venom Rush" );

  talent.ghostly_strike     = find_talent_spell( "Ghostly Strike" );
  talent.swordmaster        = find_talent_spell( "Swordmaster" );
  talent.quick_draw         = find_talent_spell( "Quick Draw" );

  talent.dirty_tricks       = find_talent_spell( "Dirty Tricks" );

  talent.cannonball_barrage = find_talent_spell( "Cannonball Barrage" );
  talent.killing_spree      = find_talent_spell( "Killing Spree" );

  talent.slice_and_dice     = find_talent_spell( "Slice and Dice" );

  talent.master_of_subtlety = find_talent_spell( "Master of Subtlety" );
  talent.weaponmaster       = find_talent_spell( "Weaponmaster" );
  talent.gloomblade         = find_talent_spell( "Gloomblade" );

  talent.premeditation      = find_talent_spell( "Premeditation" );
  talent.enveloping_shadows = find_talent_spell( "Enveloping Shadows" );

  talent.master_of_shadows  = find_talent_spell( "Master of Shadows" );

  talent.hit_and_run        = find_talent_spell( "Hit and Run" );

  artifact.goremaws_bite    = find_artifact_spell( "Goremaw's Bite" );
  artifact.shadow_fangs     = find_artifact_spell( "Shadow Fangs" );
  artifact.the_quiet_knife  = find_artifact_spell( "The Quiet Knife" );
  artifact.demons_kiss      = find_artifact_spell( "Demon's Kiss" );
  artifact.gutripper        = find_artifact_spell( "Gutripper" );
  artifact.precision_strike = find_artifact_spell( "Precision Strike" );
  artifact.fortunes_bite    = find_artifact_spell( "Fortune's Bite" );
  artifact.soul_shadows     = find_artifact_spell( "Soul Shadows" );
  artifact.energetic_stabbing = find_artifact_spell( "Energetic Stabbing" );
  artifact.catwalk          = find_artifact_spell( "Catwalk" );
  artifact.second_shuriken  = find_artifact_spell( "Second Shuriken" );
  artifact.akaaris_soul     = find_artifact_spell( "Akaari's Soul" );
  artifact.shadow_nova      = find_artifact_spell( "Shadow Nova" );
  artifact.finality         = find_artifact_spell( "Finality" );
  artifact.legionblade      = find_artifact_spell( "Legionblade" );

  artifact.kingsbane        = find_artifact_spell( "Kingsbane" );
  artifact.assassins_blades = find_artifact_spell( "Assassin's Blades" );
  artifact.toxic_blades     = find_artifact_spell( "Toxic Blades" );
  artifact.urge_to_kill     = find_artifact_spell( "Urge to Kill" );
  artifact.shadow_walker    = find_artifact_spell( "Shadow Walker" );
  artifact.bag_of_tricks    = find_artifact_spell( "Bag of Tricks" );
  artifact.master_alchemist = find_artifact_spell( "Master Alchemist" );
  artifact.master_assassin  = find_artifact_spell( "Master Assassin" );
  artifact.gushing_wound    = find_artifact_spell( "Gushing Wound" );
  artifact.blood_of_the_assassinated = find_artifact_spell( "Blood of the Assassinated" );
  artifact.balanced_blades  = find_artifact_spell( "Balanced Blades" );
  artifact.poison_knives    = find_artifact_spell( "Poison Knives" );
  artifact.surge_of_toxins  = find_artifact_spell( "Surge of Toxins" );
  artifact.serrated_edge    = find_artifact_spell( "Serrated Edge" );
  artifact.from_the_shadows = find_artifact_spell( "From the Shadows" );
  artifact.slayers_precision= find_artifact_spell( "Slayer's Precision" );

  artifact.curse_of_the_dreadblades = find_artifact_spell( "Curse of the Dreadblades" );
  artifact.cursed_edges     = find_artifact_spell( "Cursed Edges" );
  artifact.black_powder     = find_artifact_spell( "Black Powder" );
  artifact.fortune_strikes  = find_artifact_spell( "Fortune Strikes" );
  artifact.gunslinger       = find_artifact_spell( "Gunslinger" );
  artifact.blunderbuss      = find_artifact_spell( "Blunderbuss" );
  artifact.fatebringer      = find_artifact_spell( "Fatebringer" );
  artifact.blade_dancer     = find_artifact_spell( "Blade Dancer" );
  artifact.blurred_time     = find_artifact_spell( "Blurred Time" );
  artifact.greed            = find_artifact_spell( "Greed" );
  artifact.hidden_blade     = find_artifact_spell( "Hidden Blade" );
  artifact.fortunes_boon    = find_artifact_spell( "Fortune's Boon");
  artifact.fates_thirst     = find_artifact_spell( "Fate's Thirst" );
  artifact.fortunes_strike  = find_artifact_spell( "Fortune's Strike" );
  artifact.cursed_steel     = find_artifact_spell( "Cursed Steel" );

  auto_attack = new actions::auto_melee_attack_t( this, "" );

  // Legendaries
  insignia_of_ravenholdt_ = new actions::insignia_of_ravenholdt_attack_t( this );

  if ( mastery.main_gauche -> ok() )
    active_main_gauche = new actions::main_gauche_t( this );

  if ( spec.blade_flurry -> ok() )
    active_blade_flurry = new actions::blade_flurry_attack_t( this );

  if ( talent.weaponmaster -> ok() )
  {
    weaponmaster_dot_strike = new actions::weaponmaster_strike_t( this );
  }

  if ( artifact.shadow_nova.rank() )
  {
    shadow_nova = new actions::shadow_nova_t( this );
  }

  if ( artifact.second_shuriken.rank() )
  {
    second_shuriken = new actions::second_shuriken_t( this );
  }

  if ( artifact.poison_knives.rank() )
  {
    poison_knives = new actions::poison_knives_t( this );
  }

  if ( artifact.from_the_shadows.rank() )
  {
    from_the_shadows_ = new actions::from_the_shadows_driver_t( this );
  }

  if ( artifact.bag_of_tricks.rank() )
  {
    poison_bomb = new actions::poison_bomb_t( this );
  }

  if ( artifact.greed.rank() )
  {
    greed = new actions::greed_t( this );
  }

  if ( artifact.akaaris_soul.rank() )
  {
    soul_rip = new actions::soul_rip_t( this );
  }

  if ( sets.has_set_bonus( ROGUE_ASSASSINATION, T19, B2 ) )
  {
    t19_2pc_assassination = new actions::mutilated_flesh_t( this );
  }
}

// rogue_t::init_gains ======================================================

void rogue_t::init_gains()
{
  player_t::init_gains();

  gains.adrenaline_rush          = get_gain( "adrenaline_rush"    );
  gains.combat_potency           = get_gain( "combat_potency"     );
  gains.deceit                   = get_gain( "deceit" );
  gains.empowered_fan_of_knives  = get_gain( "empowered_fan_of_knives" );
  gains.energetic_recovery       = get_gain( "energetic_recovery" );
  gains.energy_refund            = get_gain( "energy_refund"      );
  gains.legendary_daggers        = get_gain( "legendary_daggers" );
  gains.murderous_intent         = get_gain( "murderous_intent"   );
  gains.overkill                 = get_gain( "overkill"           );
  gains.seal_fate                = get_gain( "seal_fate" );
  gains.shadow_strikes           = get_gain( "shadow_strikes" );
  gains.t17_2pc_assassination    = get_gain( "t17_2pc_assassination" );
  gains.t17_4pc_assassination    = get_gain( "t17_4pc_assassination" );
  gains.t17_2pc_subtlety         = get_gain( "t17_2pc_subtlety" );
  gains.t17_4pc_subtlety         = get_gain( "t17_4pc_subtlety" );
  gains.t18_2pc_assassination    = get_gain( "Assassination T18 2PC" );
  gains.venomous_wounds          = get_gain( "Venomous Vim"       );
  gains.venomous_wounds_death    = get_gain( "Venomous Vim (death) ");
  gains.quick_draw               = get_gain( "Quick Draw" );
  gains.broadsides               = get_gain( "Broadsides" );
  gains.ruthlessness             = get_gain( "Ruthlessness" );
  gains.shadow_techniques        = get_gain( "Shadow Techniques" );
  gains.master_of_shadows        = get_gain( "Master of Shadows" );
  gains.shadow_blades            = get_gain( "Shadow Blades" );
  gains.energetic_stabbing       = get_gain( "Energetic Stabbing" );
  gains.enveloping_shadows       = get_gain( "Enveloping Shadows" );
  gains.urge_to_kill             = get_gain( "Urge to Kill" );
  gains.goremaws_bite            = get_gain( "Goremaw's Bite" );
  gains.curse_of_the_dreadblades = get_gain( "Curse of the Dreadblades" );
  gains.relentless_strikes       = get_gain( "Relentless Strikes" );
  gains.t19_4pc_subtlety         = get_gain( "Tier 19 4PC Set Bonus" );
  gains.shadow_satyrs_walk       = get_gain( "Shadow Satyr's Walk" );
}

// rogue_t::init_procs ======================================================

void rogue_t::init_procs()
{
  player_t::init_procs();

  procs.seal_fate                = get_proc( "Seal Fate"           );
  procs.t18_2pc_combat           = get_proc( "Adrenaline Rush (T18 2PC)" );
  procs.weaponmaster             = get_proc( "Weaponmaster" );

  procs.roll_the_bones_1         = get_proc( "Roll the Bones: 1 buff" );
  procs.roll_the_bones_2         = get_proc( "Roll the Bones: 2 buffs" );
  procs.roll_the_bones_3         = get_proc( "Roll the Bones: 3 buffs" );
  procs.roll_the_bones_6         = get_proc( "Roll the Bones: 6 buffs" );

  procs.deepening_shadows        = get_proc( "Deepening Shadows" );

  if ( talent.death_from_above -> ok() )
  {
    dfa_mh = get_sample_data( "dfa_mh" );
    dfa_oh = get_sample_data( "dfa_oh" );
  }
}

// rogue_t::init_scaling ====================================================

void rogue_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = items[ SLOT_OFF_HAND ].active();
  scales_with[ STAT_STRENGTH              ] = false;

  if ( find_item( "thraxis_tricksy_treads" ) )
  {
    scales_with[ STAT_SPEED_RATING ] = true;
  }

  // Break out early if scaling is disabled on this player, or there's no
  // scaling stat
  if ( ! scale_player || sim -> scaling -> scale_stat == STAT_NONE )
  {
    return;
  }

  // If weapon swapping is used, adjust the weapon_t object damage values in the weapon state
  // information if this simulator is scaling the corresponding weapon DPS (main or offhand). This
  // is necessary, as weapon swapping overwrites player_t::main_hand_weapon and
  // player_t::ofF_hand_weapon, which is where player_t::init_scaling originally injects the
  // increased scaling value.
  if ( sim -> scaling -> scale_stat == STAT_WEAPON_DPS &&
       weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.active() )
  {
    double v = sim -> scaling -> scale_value;
    double pvalue = weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_PRIMARY ].swing_time.total_seconds() * v;
    double svalue = weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_SECONDARY ].swing_time.total_seconds() * v;

    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_PRIMARY ].damage += pvalue;
    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_PRIMARY ].min_dmg += pvalue;
    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_PRIMARY ].max_dmg += pvalue;

    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_SECONDARY ].damage += svalue;
    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_SECONDARY ].min_dmg += svalue;
    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_SECONDARY ].max_dmg += svalue;
  }

  if ( sim -> scaling -> scale_stat == STAT_WEAPON_OFFHAND_DPS &&
       weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.active() )
  {
    double v = sim -> scaling -> scale_value;
    double pvalue = weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_PRIMARY ].swing_time.total_seconds() * v;
    double svalue = weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_SECONDARY ].swing_time.total_seconds() * v;

    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_PRIMARY ].damage += pvalue;
    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_PRIMARY ].min_dmg += pvalue;
    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_PRIMARY ].max_dmg += pvalue;

    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_SECONDARY ].damage += svalue;
    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_SECONDARY ].min_dmg += svalue;
    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_SECONDARY ].max_dmg += svalue;
  }
}

// rogue_t::init_resources =================================================

void rogue_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_COMBO_POINT ] = initial_combo_points;
}

// rogue_t::init_buffs ======================================================

void rogue_t::create_buffs()
{
  // Handle the Legendary here, as it's called after init_items()
  if ( find_item( "vengeance" ) && find_item( "fear" ) )
  {
    fof_p1 = 1;
  }
  else if ( find_item( "the_sleeper" ) && find_item( "the_dreamer" ) )
  {
    fof_p2 = 1;
  }
  else if ( find_item( "golad_twilight_of_aspects" ) && find_item( "tiriosh_nightmare_of_ages" ) )
  {
    fof_p3 = 1;
  }

  player_t::create_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buffs.blade_flurry        = buff_creator_t( this, "blade_flurry", spec.blade_flurry )
    .cd( timespan_t::zero() );
  buffs.adrenaline_rush     = haste_buff_creator_t( this, "adrenaline_rush", find_class_spell( "Adrenaline Rush" ) )
                              .cd( timespan_t::zero() )
                              .default_value( find_class_spell( "Adrenaline Rush" ) -> effectN( 2 ).percent() )
                              .affects_regen( true )
                              .add_invalidate( CACHE_ATTACK_SPEED )
                              .add_invalidate( sets.has_set_bonus( ROGUE_OUTLAW, T18, B4 ) ? CACHE_PLAYER_DAMAGE_MULTIPLIER : CACHE_NONE );
  buffs.opportunity         = buff_creator_t( this, "opportunity", find_spell( 195627 ) );
  buffs.feint               = buff_creator_t( this, "feint", find_specialization_spell( "Feint" ) )
    .duration( find_class_spell( "Feint" ) -> duration() );
  buffs.master_of_subtlety_passive = buff_creator_t( this, "master_of_subtlety_passive", talent.master_of_subtlety )
                                     .duration( sim -> max_time / 2 )
                                     .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.master_of_subtlety  = buff_creator_t( this, "master_of_subtlety", talent.master_of_subtlety )
                              .duration( timespan_t::from_seconds( 6 ) ) // FIXME: Should be Effect #1 from Spell (id: 31223)
                              .default_value( talent.master_of_subtlety -> effectN( 1 ).percent() )
                              .chance( talent.master_of_subtlety -> ok() )
                              .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  // Killing spree buff has only 2 sec duration, main spell has 3, check.
  buffs.killing_spree       = buff_creator_t( this, "killing_spree", talent.killing_spree )
                              .duration( talent.killing_spree -> duration() + timespan_t::from_seconds( 0.001 ) );
  buffs.shadow_dance       = new buffs::shadow_dance_t( this );
  //buffs.stealth            = buff_creator_t( this, "stealth" ).add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.stealth            = new buffs::stealth_t( this );
  buffs.vanish             = new buffs::vanish_t( this );
  buffs.subterfuge         = new buffs::subterfuge_t( this );

  buffs.envenom            = buff_creator_t( this, "envenom", find_specialization_spell( "Envenom" ) )
                             .duration( timespan_t::min() )
                             .period( timespan_t::zero() )
                             .refresh_behavior( BUFF_REFRESH_PANDEMIC );

  buff_creator_t snd_creator = buff_creator_t( this, "slice_and_dice", talent.slice_and_dice )
                               .period( timespan_t::zero() )
                               .refresh_behavior( BUFF_REFRESH_PANDEMIC )
                               .add_invalidate( CACHE_ATTACK_SPEED );
  buff_creator_t rtb_creator = buff_creator_t( this, "roll_the_bones", spec.roll_the_bones )
                               .period( timespan_t::zero() ) // Disable ticking
                               .refresh_behavior( BUFF_REFRESH_PANDEMIC );

  // Presume that combat re-uses the ticker for the T18 2pc set bonus
  if ( sets.has_set_bonus( ROGUE_OUTLAW, T18, B2 ) )
  {
    std::function<void(buff_t*, int, const timespan_t&)> f( [ this ]( buff_t*, int, const timespan_t& ) {
      if ( ! rng().roll( sets.set( ROGUE_OUTLAW, T18, B2 ) -> proc_chance() ) )
        return;

      if ( buffs.adrenaline_rush -> check() )
      {
        buffs.adrenaline_rush -> extend_duration( this, spell.tier18_2pc_combat_ar -> duration() );
      }
      else
      {
        if ( buffs.adrenaline_rush -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0,
             spell.tier18_2pc_combat_ar -> duration() ) )
        {
          procs.t18_2pc_combat -> occur();
        }
      }
    } );

    snd_creator.period( talent.slice_and_dice -> effectN( 2 ).period() );
    snd_creator.tick_behavior( BUFF_TICK_REFRESH );
    snd_creator.tick_callback( f );

    rtb_creator.period( spec.roll_the_bones -> effectN( 2 ).period() );
    rtb_creator.tick_behavior( BUFF_TICK_REFRESH );
    rtb_creator.tick_callback( f );
  }

  buffs.slice_and_dice = snd_creator;

  // Legendary buffs
  buffs.fof_p1            = stat_buff_creator_t( this, "suffering", find_spell( 109959 ) )
                            .add_stat( STAT_AGILITY, find_spell( 109959 ) -> effectN( 1 ).base_value() )
                            .chance( fof_p1 );
  buffs.fof_p2            = stat_buff_creator_t( this, "nightmare", find_spell( 109955 ) )
                            .add_stat( STAT_AGILITY, find_spell( 109955 ) -> effectN( 1 ).base_value() )
                            .chance( fof_p2 );
  buffs.fof_p3            = stat_buff_creator_t( this, "shadows_of_the_destroyer", find_spell( 109939 ) -> effectN( 1 ).trigger() )
                            .add_stat( STAT_AGILITY, find_spell( 109939 ) -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() )
                            .chance( fof_p3 );

  // Legendary 7.0 buffs
  buffs.shivarran_symmetry = buff_creator_t( this, "shivarran_symmetry", find_spell( 226318 ) );
  buffs.greenskins_waterlogged_wristcuffs = buff_creator_t( this, "greenskins_waterlogged_wristcuffs", find_spell( 209423 ) );
  const spell_data_t* tddid = ( specialization() == ROGUE_ASSASSINATION ) ? find_spell( 208693 ) : ( specialization() == ROGUE_SUBTLETY ) ? find_spell( 228224 ) : spell_data_t::not_found();
  buffs.the_dreadlords_deceit = buff_creator_t( this, "the_dreadlords_deceit", tddid )
                                .default_value( tddid -> effectN( 1 ).percent() );
  buffs.the_dreadlords_deceit_driver = buff_creator_t( this, "the_dreadlords_deceit_driver", find_spell( 208692 ) )
                                     .period( find_spell( 208692 ) -> effectN( 1 ).period() )
                                     .quiet( true )
                                     .tick_callback( [this]( buff_t*, int, const timespan_t& ) {
                                      buffs.the_dreadlords_deceit -> trigger(); } )
                                     .tick_time_behavior( BUFF_TICK_TIME_UNHASTED );
  buffs.mantle_of_the_master_assassin_passive = buff_creator_t( this, "master_assassins_initiative_passive", find_spell( 235022 ) )
                                              .duration( sim -> max_time / 2 )
                                              .default_value( find_spell( 235027 ) -> effectN( 1 ).percent() )
                                              .add_invalidate( CACHE_CRIT_CHANCE );
  buffs.mantle_of_the_master_assassin  = buff_creator_t( this, "master_assassins_initiative", find_spell( 235022 ) )
                                      .duration( timespan_t::from_seconds( 6 ) ) // FIXME: Should be Effect #1 from Spell (id: 235022)
                                      .default_value( find_spell( 235027 ) -> effectN( 1 ).percent() )
                                      .add_invalidate( CACHE_CRIT_CHANCE );

  buffs.fof_fod           = new buffs::fof_fod_t( this );

  buffs.death_from_above  = buff_creator_t( this, "death_from_above", spell.death_from_above )
                            .quiet( true );

  buffs.deceit            = buff_creator_t( this, "deceit", sets.set( ROGUE_OUTLAW, T17, B4 ) -> effectN( 1 ).trigger() )
                            .chance( sets.has_set_bonus( ROGUE_OUTLAW, T17, B4 ) );
  buffs.shadow_strikes    = buff_creator_t( this, "shadow_strikes", find_spell( 170107 ) )
                            .chance( sets.has_set_bonus( ROGUE_SUBTLETY, T17, B4 ) );

  buffs.sprint            = buff_creator_t( this, "sprint", spell.sprint )
    .cd( timespan_t::zero() );
  buffs.shadowstep        = buff_creator_t( this, "shadowstep", spec.shadowstep )
    .cd( timespan_t::zero() );
  buffs.deathly_shadows = buff_creator_t( this, "deathly_shadows", find_spell( 188700 ) )
                         .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                         .chance( sets.has_set_bonus( ROGUE_SUBTLETY, T18, B2 ) );

  buffs.elaborate_planning = buff_creator_t( this, "elaborate_planning", talent.elaborate_planning -> effectN( 1 ).trigger() )
                             .default_value( 1.0 + talent.elaborate_planning -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
                             .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.alacrity = haste_buff_creator_t( this, "alacrity", find_spell( 193538 ) )
    .default_value( find_spell( 193538 ) -> effectN( 1 ).percent() )
    .chance( talent.alacrity -> ok() );

  buffs.jolly_roger = buff_creator_t( this, "jolly_roger", find_spell( 199603 ) )
                      .default_value( find_spell( 199603 ) -> effectN( 1 ).percent() );
  buffs.grand_melee = haste_buff_creator_t( this, "grand_melee", find_spell( 193358 ) )
                      .add_invalidate( CACHE_ATTACK_SPEED )
                      .default_value( 1.0 / ( 1.0 + find_spell( 193358 ) -> effectN( 1 ).percent() ) );
  buffs.shark_infested_waters = buff_creator_t( this, "shark_infested_waters", find_spell( 193357 ) )
                                .default_value( find_spell( 193357 ) -> effectN( 1 ).percent() )
                                .add_invalidate( CACHE_CRIT_CHANCE );
  buffs.true_bearing = buff_creator_t( this, "true_bearing", find_spell( 193359 ) )
                       .default_value( find_spell( 193359 ) -> effectN( 1 ).base_value() );
  buffs.broadsides = buff_creator_t( this, "broadsides", find_spell( 193356 ) );
  buffs.buried_treasure = buff_creator_t( this, "buried_treasure", find_spell( 199600 ) )
                       .default_value( find_spell( 199600 ) -> effectN( 1 ).percent() );
  // Note, since I (navv) am a slacker, this needs to be constructed after the secondary buffs.
  buffs.roll_the_bones = new buffs::roll_the_bones_t( this, rtb_creator );

  buffs.symbols_of_death = buff_creator_t( this, "symbols_of_death", spec.symbols_of_death )
                           .refresh_behavior( BUFF_REFRESH_PANDEMIC )
                           .period( timespan_t::zero() )
                           .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                           .default_value( 1.0 + spec.symbols_of_death -> effectN( 1 ).percent() );
  buffs.shadow_blades = new buffs::shadow_blades_t( this );

  buffs.enveloping_shadows = buff_creator_t( this, "enveloping_shadows", talent.enveloping_shadows )
            .tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
               resource_gain( RESOURCE_COMBO_POINT, 1, gains.enveloping_shadows );
            } );

  buffs.goremaws_bite = buff_creator_t( this, "goremaws_bite", artifact.goremaws_bite.data().effectN( 4 ).trigger() )
      .tick_callback( [ this ]( buff_t* b, int, const timespan_t& ) {
        resource_gain( RESOURCE_ENERGY, b -> data().effectN( 2 ).resource( RESOURCE_ENERGY ), gains.goremaws_bite );
      } );

  buffs.hidden_blade = buff_creator_t( this, "hidden_blade", artifact.hidden_blade.data().effectN( 1 ).trigger() )
    .trigger_spell( artifact.hidden_blade );
  buffs.curse_of_the_dreadblades = buff_creator_t( this, "curse_of_the_dreadblades", artifact.curse_of_the_dreadblades )
    .cd( timespan_t::zero() ); // Handled by the action
  buffs.blurred_time = new buffs::blurred_time_t( this );
  buffs.death = buff_creator_t( this, "death", spec.symbols_of_death -> effectN( 3 ).trigger() );
  buffs.t19_4pc_outlaw = buff_creator_t( this, "swordplay" )
    .spell( sets.set( ROGUE_OUTLAW, T19, B4 ) -> effectN( 1 ).trigger() )
    .trigger_spell( sets.set( ROGUE_OUTLAW, T19, B4 ) );
  buffs.t19oh_8pc = stat_buff_creator_t( this, "shadowstalkers_avidity" )
    .spell( sets.set( specialization(), T19OH, B8 ) -> effectN( 1 ).trigger() )
    .trigger_spell( sets.set( specialization(), T19OH, B8 ) );
  buffs.blunderbuss = buff_creator_t( this, "blunderbuss", find_spell( 202848 ) )
    .chance( artifact.blunderbuss.data().effectN( 2 ).percent() );
  buffs.finality_eviscerate = buff_creator_t( this, "finality_eviscerate", find_spell( 197496 ) )
    .trigger_spell( artifact.finality )
    .default_value( find_spell( 197496 ) -> effectN( 1 ).percent() / COMBO_POINT_MAX )
    .max_stack( consume_cp_max() );
  buffs.finality_nightblade = buff_creator_t( this, "finality_nightblade", find_spell( 197498 ) )
    .trigger_spell( artifact.finality )
    .default_value( find_spell( 197498 ) -> effectN( 1 ).percent() / COMBO_POINT_MAX )
    .max_stack( consume_cp_max() );
}

// rogue_t::create_options ==================================================

static bool do_parse_secondary_weapon( rogue_t* rogue,
                                       const std::string& value,
                                       slot_e slot )
{
  switch ( slot )
  {
    case SLOT_MAIN_HAND:
      rogue -> weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data = item_t( rogue, value );
      rogue -> weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.slot = slot;
      break;
    case SLOT_OFF_HAND:
      rogue -> weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data = item_t( rogue, value );
      rogue -> weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.slot = slot;
      break;
    default:
      break;
  }

  return true;
}

static bool parse_offhand_secondary( sim_t* sim,
                                     const std::string& /* name */,
                                     const std::string& value )
{
  rogue_t* rogue = static_cast<rogue_t*>( sim -> active_player );
  return do_parse_secondary_weapon( rogue, value, SLOT_OFF_HAND );
}

static bool parse_mainhand_secondary( sim_t* sim,
                                      const std::string& /* name */,
                                      const std::string& value )
{
  rogue_t* rogue = static_cast<rogue_t*>( sim -> active_player );
  return do_parse_secondary_weapon( rogue, value, SLOT_MAIN_HAND );
}

static bool parse_fixed_rtb( sim_t* sim,
                             const std::string& /* name */,
                             const std::string& value )
{
  std::vector<size_t> buffs;
  for ( auto it = value.begin(); it < value.end(); ++it )
  {
    if ( *it < '1' or *it > '6' )
    {
      continue;
    }

    size_t buff_index = *it - '1';
    if ( range::find( buffs, buff_index ) != buffs.end() )
    {
      sim -> errorf( "%s: Duplicate 'fixed_rtb' buff %c", sim -> active_player -> name(), *it );
      return false;
    }

    buffs.push_back( buff_index );
  }

  if ( buffs.size() == 0 || buffs.size() == 4 || buffs.size() == 5 )
  {
    sim -> errorf( "%s: No valid 'fixed_rtb' buffs given by string '%s'", sim -> active_player -> name(),
        value.c_str() );
    return false;
  }

  debug_cast< rogue_t* >( sim -> active_player ) -> fixed_rtb = buffs;

  return true;
}

void rogue_t::create_options()
{
  add_option( opt_func( "off_hand_secondary", parse_offhand_secondary ) );
  add_option( opt_func( "main_hand_secondary", parse_mainhand_secondary ) );
  add_option( opt_int( "initial_combo_points", initial_combo_points ) );
  add_option( opt_func( "fixed_rtb", parse_fixed_rtb ) );

  player_t::create_options();
}

// rogue_t::copy_from =======================================================

void rogue_t::copy_from( player_t* source )
{
  rogue_t* rogue = static_cast<rogue_t*>( source );
  player_t::copy_from( source );
  if ( ! rogue -> weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.options_str.empty() )
  {
    weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.options_str = \
      rogue -> weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.options_str;
  }

  if ( ! rogue -> weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.options_str.empty() )
  {
    weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.options_str = \
      rogue -> weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.options_str;
  }
}

// rogue_t::create_profile  =================================================

std::string rogue_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  // Break out early if we are not saving everything, or gear
  if ( stype != SAVE_ALL && stype != SAVE_GEAR )
  {
    return profile_str;
  }

  std::string term = "\n";

  if ( weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.active() ||
       weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.active() )
  {
    profile_str += term;
    profile_str += "# Secondary weapons used in conjunction with weapon swapping are defined below.";
    profile_str += term;
  }

  if ( weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.active() )
  {
    profile_str += "main_hand_secondary=";
    profile_str += weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.encoded_item() + term;
    if ( sim -> save_gear_comments &&
         ! weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.encoded_comment().empty() )
    {
      profile_str += "# ";
      profile_str += weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.encoded_comment();
      profile_str += term;
    }
  }

  if ( weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.active() )
  {
    profile_str += "off_hand_secondary=";
    profile_str += weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.encoded_item() + term;
    if ( sim -> save_gear_comments &&
         ! weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.encoded_comment().empty() )
    {
      profile_str += "# ";
      profile_str += weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.encoded_comment();
      profile_str += term;
    }
  }

  return profile_str;
}

// rogue_t::init_items ======================================================

bool rogue_t::init_items()
{
  bool ret = player_t::init_items();
  if ( ! ret )
  {
    return ret;
  }

  // Initialize weapon swapping data structures for primary weapons here
  weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_PRIMARY ] = main_hand_weapon;
  weapon_data[ WEAPON_MAIN_HAND ].item_data[ WEAPON_PRIMARY ] = &( items[ SLOT_MAIN_HAND ] );
  weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_PRIMARY ] = off_hand_weapon;
  weapon_data[ WEAPON_OFF_HAND ].item_data[ WEAPON_PRIMARY ] = &( items[ SLOT_OFF_HAND ] );

  if ( ! weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.options_str.empty() )
  {
    ret = weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.init();
    if ( ! ret )
    {
      return false;
    }
    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_SECONDARY ] = main_hand_weapon;
    weapon_data[ WEAPON_MAIN_HAND ].item_data[ WEAPON_SECONDARY ] = &( weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data );

    // Restore primary main hand weapon after secondary weapon init
    main_hand_weapon = weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_PRIMARY ];
  }

  if ( ! weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.options_str.empty() )
  {
    ret = weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.init();
    if ( ! ret )
    {
      return false;
    }
    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_SECONDARY ] = off_hand_weapon;
    weapon_data[ WEAPON_OFF_HAND ].item_data[ WEAPON_SECONDARY ] = &( weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data );

    // Restore primary off hand weapon after secondary weapon init
    main_hand_weapon = weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_PRIMARY ];
  }

  return ret;
}

// rogue_t::init_special_effects ============================================

bool rogue_t::init_special_effects()
{
  bool ret = player_t::init_special_effects();

  if ( weapon_data[ WEAPON_MAIN_HAND ].item_data[ WEAPON_SECONDARY ] )
  {
    for ( size_t i = 0, end = weapon_data[ WEAPON_MAIN_HAND ].item_data[ WEAPON_SECONDARY ] -> parsed.special_effects.size();
          i < end; ++i )
    {
      special_effect_t* effect = weapon_data[ WEAPON_MAIN_HAND ].item_data[ WEAPON_SECONDARY ] -> parsed.special_effects[ i ];
      unique_gear::initialize_special_effect_2( effect );
    }
  }

  if ( weapon_data[ WEAPON_OFF_HAND ].item_data[ WEAPON_SECONDARY ] )
  {
    for ( size_t i = 0, end = weapon_data[ WEAPON_OFF_HAND ].item_data[ WEAPON_SECONDARY ] -> parsed.special_effects.size();
          i < end; ++i )
    {
      special_effect_t* effect = weapon_data[ WEAPON_OFF_HAND ].item_data[ WEAPON_SECONDARY ] -> parsed.special_effects[ i ];
      unique_gear::initialize_special_effect_2( effect );
    }
  }

  return ret;
}

// rogue_t::init_rng ========================================================

void rogue_t::init_rng()
{
  player_t::init_rng();

  if ( artifact.bag_of_tricks.rank() )
  {
    prng.bag_of_tricks = get_rppm( "bag_of_tricks", artifact.bag_of_tricks );
  }
}

// rogue_t::init_finished ===================================================

bool rogue_t::init_finished()
{
  weapon_data[ WEAPON_MAIN_HAND ].initialize();
  weapon_data[ WEAPON_OFF_HAND ].initialize();

  return player_t::init_finished();
}

// rogue_t::reset ===========================================================

void rogue_t::reset()
{
  player_t::reset();

  poisoned_enemies = 0;

  df_counter = 0;
  shadow_techniques = rng().range( 3, 5 );

  weapon_data[ WEAPON_MAIN_HAND ].reset();
  weapon_data[ WEAPON_OFF_HAND ].reset();
}

void rogue_t::swap_weapon( weapon_slot_e slot, current_weapon_e to_weapon, bool in_combat )
{
  if ( weapon_data[ slot ].current_weapon == to_weapon )
  {
    return;
  }

  if ( ! weapon_data[ slot ].item_data[ to_weapon ] )
  {
    return;
  }

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s performing weapon swap from %s to %s",
        name(), weapon_data[ slot ].item_data[ ! to_weapon ] -> name(),
        weapon_data[ slot ].item_data[ to_weapon ] -> name() );
  }

  // First, swap stats on actor, but only if it is in combat. Outside of combat (basically
  // during iteration reset) there is no need to adjust actor stats, as they are always reset to
  // the primary weapons.
  for ( stat_e i = STAT_NONE; in_combat && i < STAT_MAX; i++ )
  {
    stat_loss( i, weapon_data[ slot ].stats_data[ ! to_weapon ].get_stat( i ) );
    stat_gain( i, weapon_data[ slot ].stats_data[ to_weapon ].get_stat( i ) );
  }

  weapon_t* target_weapon = &( slot == WEAPON_MAIN_HAND ? main_hand_weapon : off_hand_weapon );
  action_t* target_action = slot == WEAPON_MAIN_HAND ? main_hand_attack : off_hand_attack;

  // Swap the actor weapon object
  *target_weapon = weapon_data[ slot ].weapon_data[ to_weapon ];
  target_action -> base_execute_time = target_weapon -> swing_time;

  // Enable new weapon callback(s)
  weapon_data[ slot ].callback_state( to_weapon, true );

  // Disable old weapon callback(s)
  weapon_data[ slot ].callback_state( static_cast<current_weapon_e>( ! to_weapon ), false );

  // Reset swing timer of the weapon
  if ( target_action -> execute_event )
  {
    event_t::cancel( target_action -> execute_event );
    target_action -> schedule_execute();
  }

  // Track uptime
  if ( to_weapon == WEAPON_PRIMARY )
  {
    weapon_data[ slot ].secondary_weapon_uptime -> expire();
  }
  else
  {
    weapon_data[ slot ].secondary_weapon_uptime -> trigger();
  }

  // Set the current weapon wielding state for the slot
  weapon_data[ slot ].current_weapon = to_weapon;
}

// rogue_t::arise ===========================================================

void rogue_t::arise()
{
  player_t::arise();

  resources.current[ RESOURCE_COMBO_POINT ] = 0;
}

// rogue_t::combat_begin ====================================================

void rogue_t::combat_begin()
{
  player_t::combat_begin();

  if ( legendary.the_dreadlords_deceit )
  {
    buffs.the_dreadlords_deceit -> trigger(30);
    buffs.the_dreadlords_deceit_driver -> trigger();
  }
}

// rogue_t::energy_regen_per_second =========================================

double rogue_t::energy_regen_per_second() const
{
  double r = player_t::energy_regen_per_second();

  if ( buffs.blade_flurry -> check() )
  {
    double penalty = spec.blade_flurry -> effectN( 1 ).percent();
    penalty += artifact.blade_dancer.percent();
    r *= 1.0 + penalty;
  }

  if ( buffs.buried_treasure -> up() )
  {
    r *= 1.0 + buffs.buried_treasure -> check_value();
  }

  return r;
}

// rogue_t::temporary_movement_modifier ==================================

double rogue_t::temporary_movement_modifier() const
{
  double temporary = player_t::temporary_movement_modifier();

  if ( buffs.sprint -> up() )
    temporary = std::max( buffs.sprint -> data().effectN( 1 ).percent(), temporary );

  if ( buffs.shadowstep -> up() )
    temporary = std::max( buffs.shadowstep -> data().effectN( 2 ).percent(), temporary );

  return temporary;
}

// rogue_t::passive_movement_modifier===================================

double rogue_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  ms += spell.fleet_footed -> effectN( 1 ).percent();
  ms += artifact.catwalk.percent();
  ms += talent.hit_and_run -> effectN( 1 ).percent();

  if ( buffs.stealth -> up() || buffs.shadow_dance -> up() ) // Check if nightstalker is temporary or passive.
    ms += talent.nightstalker -> effectN( 1 ).percent();

  return ms;
}

// rogue_t::regen ===========================================================

void rogue_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( buffs.adrenaline_rush -> up() )
  {
    if ( ! resources.is_infinite( RESOURCE_ENERGY ) )
    {
      double energy_regen = periodicity.total_seconds() * energy_regen_per_second() * buffs.adrenaline_rush -> data().effectN( 1 ).percent();

      resource_gain( RESOURCE_ENERGY, energy_regen, gains.adrenaline_rush );
    }
  }
}

// rogue_t::available =======================================================

timespan_t rogue_t::available() const
{
  if ( ready_type != READY_POLL )
  {
    return player_t::available();
  }
  else
  {
    double energy = resources.current[ RESOURCE_ENERGY ];

    if ( energy > 25 )
      return timespan_t::from_seconds( 0.1 );

    return std::max(
             timespan_t::from_seconds( ( 25 - energy ) / energy_regen_per_second() ),
             timespan_t::from_seconds( 0.1 )
           );
  }
}

// rogue_t::convert_hybrid_stat ==============================================

stat_e rogue_t::convert_hybrid_stat( stat_e s ) const
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

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class rogue_report_t : public player_report_extension_t
{
public:
  rogue_report_t( rogue_t& player ) :
      p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& os ) override
  {
    os << "<div class=\"player-section custom_section\">\n";
    if ( p.talent.death_from_above -> ok() )
    {
      os << "<h3 class=\"toggle open\">Death from Above swing time loss</h3>\n"
         << "<div class=\"toggle-content\">\n";

      os << "<p>";
      os <<
        "Death from Above causes out of range time for the Rogue while the"
        " animation is performing. This out of range time translates to a"
        " potential loss of auto-attack swing time. The following table"
        " represents the total auto-attack swing time loss, when performing Death"
        " from Above during the length of the combat. It is computed as the"
        " interval between the out of range delay (an average of 1.3 seconds in"
        " simc), and the next time the hand swings after the out of range delay"
        " elapsed.";
      os << "</p>";
      os << "<table class=\"sc\" style=\"float: left;margin-right: 10px;\">\n";

      os << "<tr><th></th><th colspan=\"3\">Lost time per iteration (sec)</th></tr>";
      os << "<tr><th>Weapon hand</th><th>Minimum</th><th>Average</th><th>Maximum</th></tr>";

      os << "<tr>";
      os << "<td class=\"left\">Main hand</td>";
      os.format("<td class=\"right\">%.3f</td>", p.dfa_mh -> min() );
      os.format("<td class=\"right\">%.3f</td>", p.dfa_mh -> mean() );
      os.format("<td class=\"right\">%.3f</td>", p.dfa_mh -> max() );
      os << "</tr>";

      os << "<tr>";
      os << "<td class=\"left\">Off hand</td>";
      os.format("<td class=\"right\">%.3f</td>", p.dfa_oh -> min() );
      os.format("<td class=\"right\">%.3f</td>", p.dfa_oh -> mean() );
      os.format("<td class=\"right\">%.3f</td>", p.dfa_oh -> max() );
      os << "</tr>";

      os << "</table>";

      os << "</div>\n";

      os << "<div class=\"clear\"></div>\n";
    }
    os << "</div>\n";
  }
private:
  rogue_t& p;
};

// ROGUE MODULE INTERFACE ===================================================

struct toxic_mutilator_t : public unique_gear::scoped_action_callback_t<actions::mutilate_t>
{
  toxic_mutilator_t() : super( ROGUE, "mutilate" ) { }

  void manipulate( actions::mutilate_t* action, const special_effect_t& effect ) override
  { action -> toxic_mutilator_crit_chance = effect.driver() -> effectN( 1 ).average( effect.item ) / 100.0; }
};

struct eviscerating_blade_t : public unique_gear::scoped_action_callback_t<>
{
  eviscerating_blade_t() : super( ROGUE, "run_through" ) { }

  void manipulate( action_t* action, const special_effect_t& effect ) override
  { action -> base_multiplier *= 1.0 + effect.driver() -> effectN( 2 ).average( effect.item ) / 100.0; }
};

struct from_the_shadows_t : public unique_gear::scoped_action_callback_t<>
{
  from_the_shadows_t() : super( ROGUE, "shadowstrike" ) { }

  void manipulate( action_t* action, const special_effect_t& effect ) override
  { action -> base_multiplier *= 1.0 + effect.driver() -> effectN( 1 ).average( effect.item ) / 100.0; }
};

struct duskwalker_footpads_t : public unique_gear::scoped_actor_callback_t<rogue_t>
{
  duskwalker_footpads_t() : super( ROGUE )
  { }

  void manipulate( rogue_t* rogue, const special_effect_t& e ) override
  { rogue -> legendary.duskwalker_footpads = e.driver(); }
};

struct thraxis_tricksy_treads_t : public unique_gear::scoped_action_callback_t<actions::run_through_t>
{
  thraxis_tricksy_treads_t() : super( ROGUE, "run_through" )
  { }

  void manipulate( actions::run_through_t* action, const special_effect_t& e ) override
  { action -> ttt_multiplier = e.driver() -> effectN( 1 ).percent(); }
};

struct denial_of_the_halfgiants_t : public unique_gear::scoped_actor_callback_t<rogue_t>
{
  denial_of_the_halfgiants_t() : super( ROGUE )
  { }

  void manipulate( rogue_t* rogue, const special_effect_t& e ) override
  { rogue -> legendary.denial_of_the_halfgiants = e.driver(); }
};

struct shivarran_symmetry_t : public unique_gear::scoped_action_callback_t<actions::blade_flurry_t>
{
  shivarran_symmetry_t() : super( ROGUE, "blade_flurry" )
  { }

  void manipulate( actions::blade_flurry_t* action, const special_effect_t& e ) override
  { action -> shivarran_symmetry = e.driver(); }
};

struct greenskins_waterlogged_wristcuffs_t : public unique_gear::scoped_action_callback_t<actions::between_the_eyes_t>
{
  greenskins_waterlogged_wristcuffs_t() : super( ROGUE, "between_the_eyes" )
  { }

  void manipulate( actions::between_the_eyes_t* action, const special_effect_t& e ) override
  { action -> greenskins_waterlogged_wristcuffs = e.driver(); }
};

struct zoldyck_family_training_shackles_t : public unique_gear::scoped_actor_callback_t<rogue_t>
{
  zoldyck_family_training_shackles_t() : super( ROGUE )
  { }

  void manipulate( rogue_t* rogue, const special_effect_t& e ) override
  { rogue -> legendary.zoldyck_family_training_shackles = e.driver(); }
};

struct shadow_satyrs_walk_t : public unique_gear::scoped_action_callback_t<actions::shadowstrike_t>
{
  shadow_satyrs_walk_t() : super( ROGUE, "shadowstrike" )
  { }

  void manipulate( actions::shadowstrike_t* action, const special_effect_t& e ) override
  { action -> shadow_satyrs_walk = e.driver(); }
};

struct the_dreadlords_deceit_t : public unique_gear::scoped_actor_callback_t<rogue_t>
{
  the_dreadlords_deceit_t() : super( ROGUE )
  { }

  void manipulate( rogue_t* rogue, const special_effect_t& e ) override
  { rogue -> legendary.the_dreadlords_deceit = e.driver(); }
};

struct insignia_of_ravenholdt_t : public unique_gear::scoped_actor_callback_t<rogue_t>
{
  insignia_of_ravenholdt_t() : super( ROGUE )
  { }

  void manipulate( rogue_t* rogue, const special_effect_t& e ) override
  { rogue -> legendary.insignia_of_ravenholdt = e.driver(); }
};

struct mantle_of_the_master_assassin_t : public unique_gear::scoped_actor_callback_t<rogue_t>
{
  mantle_of_the_master_assassin_t() : super( ROGUE )
  { }

  void manipulate( rogue_t* rogue, const special_effect_t& e ) override
  { rogue -> legendary.mantle_of_the_master_assassin = e.driver(); }
};

struct rogue_module_t : public module_t
{
  rogue_module_t() : module_t( ROGUE ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto p = new rogue_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new rogue_report_t( *p ) );
    return p;
  }

  virtual bool valid() const override
  { return true; }

  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 184916, toxic_mutilator_t()                   );
    unique_gear::register_special_effect( 184917, eviscerating_blade_t()                );
    unique_gear::register_special_effect( 184918, from_the_shadows_t()                  );
    unique_gear::register_special_effect( 208895, duskwalker_footpads_t()               );
    unique_gear::register_special_effect( 212539, thraxis_tricksy_treads_t()            );
    unique_gear::register_special_effect( 208892, denial_of_the_halfgiants_t()          );
    unique_gear::register_special_effect( 226045, shivarran_symmetry_t()                );
    unique_gear::register_special_effect( 209420, greenskins_waterlogged_wristcuffs_t() );
    unique_gear::register_special_effect( 214569, zoldyck_family_training_shackles_t()  );
    unique_gear::register_special_effect( 208436, shadow_satyrs_walk_t()                );
    unique_gear::register_special_effect( 208692, the_dreadlords_deceit_t()             );
    unique_gear::register_special_effect( 209041, insignia_of_ravenholdt_t()            );
    unique_gear::register_special_effect( 235022, mantle_of_the_master_assassin_t()     );
  }

  void register_hotfixes() const override
  {
  }

  virtual void init( player_t* ) const override {}
  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::rogue()
{
  static rogue_module_t m;
  return &m;
}
