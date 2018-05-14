// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

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
struct melee_t;
struct shadow_blade_t;
}

namespace buffs
{
struct rogue_poison_buff_t;
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
    dot_t* internal_bleeding;
    dot_t* killing_spree; // Strictly speaking, this should probably be on player
    dot_t* mutilated_flesh; // Assassination T19 2PC
    dot_t* nightblade;
    dot_t* rupture;
    dot_t* crimson_tempest;
  } dots;

  struct debuffs_t
  {
    buff_t* vendetta;
    buff_t* wound_poison;
    buff_t* crippling_poison;
    buff_t* leeching_poison;
    buffs::marked_for_death_debuff_t* marked_for_death;
    buff_t* ghostly_strike;
    buff_t* garrote; // Hidden proxy buff for garrote to get Thuggee working easily(ish)
    buff_t* toxic_blade;
    buff_t* expose_weakness;
  } debuffs;

  rogue_td_t( player_t* target, rogue_t* source );

  bool lethal_poisoned() const
  {
    return dots.deadly_poison -> is_ticking() ||
           debuffs.wound_poison -> check();
  }

  timespan_t lethal_poison_remains() const
  {
    if ( dots.deadly_poison -> is_ticking() ) {
      return dots.deadly_poison -> remains();
    } else if ( debuffs.wound_poison -> check() ) {
      return debuffs.wound_poison -> remains();
    } else {
      return timespan_t::from_seconds( 0.0 );
    }
  }

  bool non_lethal_poisoned() const
  {
    return debuffs.crippling_poison -> check() ||
           debuffs.leeching_poison -> check();
  }

  timespan_t non_lethal_poison_remains() const
  {
    if ( debuffs.crippling_poison -> check() ) {
      return debuffs.crippling_poison -> remains();
    } else if ( debuffs.leeching_poison -> check() ) {
      return debuffs.leeching_poison -> remains();
    } else {
      return timespan_t::from_seconds( 0.0 );
    }
  }

  bool poisoned() const
  {
    return lethal_poisoned() || non_lethal_poisoned();
  }
};

struct rogue_t : public player_t
{
  // Custom options
  std::vector<size_t> fixed_rtb;
  std::vector<double> fixed_rtb_odds;

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
  action_t* poison_bomb;
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

  // Buffs
  struct buffs_t
  {
    // Baseline
    // Shared
    buff_t* feint;
    buff_t* shadowstep;
    buff_t* sprint;
    buff_t* stealth;
    buff_t* vanish;
    // Assassination
    buff_t* envenom;
    buff_t* vendetta;
    // Outlaw
    haste_buff_t* adrenaline_rush;
    buff_t* blade_flurry;
    buff_t* blade_rush;
    buff_t* opportunity;
    buff_t* roll_the_bones;
    // Roll the bones buffs
    buff_t* broadsides;
    buff_t* buried_treasure;
    haste_buff_t* grand_melee;
    buff_t* jolly_roger;
    buff_t* shark_infested_waters;
    buff_t* true_bearing;
    // Subtlety
    buff_t* shuriken_combo;
    buff_t* shadow_blades;
    buff_t* shadow_dance;
    buff_t* symbols_of_death;


    // Talents
    // Shared
    haste_buff_t* alacrity;
    buff_t* subterfuge;
    // Assassination
    buff_t* elaborate_planning;
    buff_t* blindside;
    buff_t* master_assassin;
    buff_t* master_assassin_aura;
    buff_t* hidden_blades_driver;
    buff_t* hidden_blades;
    // Outlaw
    buff_t* killing_spree;
    buff_t* loaded_dice;
    buff_t* slice_and_dice;
    // Subtlety
    buff_t* master_of_shadows;
    buff_t* death_from_above;


    // Legendaries
    // Shared
    buff_t* mantle_of_the_master_assassin_aura;
    buff_t* mantle_of_the_master_assassin;
    buff_t* the_dreadlords_deceit_driver;
    buff_t* the_dreadlords_deceit;
    haste_buff_t* sephuzs_secret;
    // Assassination
    buff_t* the_empty_crown;
    // Outlaw
    buff_t* greenskins_waterlogged_wristcuffs;
    buff_t* shivarran_symmetry;
    // Subtlety
    buff_t* the_first_of_the_dead;


    // Tiers
    // T19 Outdoor
    buff_t* t19oh_8pc;
    // T19 Raid
    buff_t* t19_4pc_outlaw;
    // T20 Raid
    buff_t* t20_2pc_outlaw;
    haste_buff_t* t20_4pc_outlaw;
    // T21 Raid
    buff_t* t21_2pc_assassination;
    buff_t* t21_2pc_outlaw;
    buff_t* t21_4pc_subtlety;

    // Azerite powers
    buff_t* deadshot;
    buff_t* nights_vengeance;
    buff_t* sharpened_blades;
    buff_t* storm_of_steel;

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
    cooldown_t* blade_rush;
    cooldown_t* blind;
    cooldown_t* gouge;
    cooldown_t* cloak_of_shadows;
    cooldown_t* riposte;
    cooldown_t* ghostly_strike;
    cooldown_t* grappling_hook;
    cooldown_t* marked_for_death;
    cooldown_t* death_from_above;
    cooldown_t* weaponmaster;
    cooldown_t* vendetta;
    cooldown_t* toxic_blade;
    cooldown_t* symbols_of_death;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* adrenaline_rush;
    gain_t* blade_rush;
    gain_t* buried_treasure;
    gain_t* combat_potency;
    gain_t* energy_refund;
    gain_t* master_of_shadows;
    gain_t* vendetta;
    gain_t* venom_rush;
    gain_t* venomous_wounds;
    gain_t* venomous_wounds_death;
    gain_t* relentless_strikes;
    gain_t* shadow_satyrs_walk;
    gain_t* the_empty_crown;
    gain_t* the_first_of_the_dead;
    gain_t* symbols_of_death;
    gain_t* t20_4pc_outlaw;
    gain_t* t21_4pc_assassination;

    // CP Gains
    gain_t* seal_fate;
    gain_t* quick_draw;
    gain_t* broadsides;
    gain_t* ruthlessness;
    gain_t* shadow_techniques;
    gain_t* shadow_blades;
    gain_t* t19_4pc_subtlety;
    gain_t* t21_4pc_subtlety;
  } gains;

  // Spec passives
  struct spec_t
  {
    // Shared
    const spell_data_t* shadowstep;

    // Generic
    const spell_data_t* assassination_rogue;
    const spell_data_t* outlaw_rogue;
    const spell_data_t* subtlety_rogue;

    // Assassination
    const spell_data_t* improved_poisons;
    const spell_data_t* seal_fate;
    const spell_data_t* venomous_wounds;
    const spell_data_t* vendetta;
    const spell_data_t* master_assassin;
    const spell_data_t* garrote;
    const spell_data_t* garrote_2;

    // Outlaw
    const spell_data_t* blade_flurry;
    const spell_data_t* combat_potency;
    const spell_data_t* combat_potency_reg;
    const spell_data_t* restless_blades;
    const spell_data_t* roll_the_bones;
    const spell_data_t* ruthlessness;
    const spell_data_t* sinister_strike;

    // Subtlety
    const spell_data_t* deepening_shadows;
    const spell_data_t* relentless_strikes;
    const spell_data_t* shadow_blades;
    const spell_data_t* shadow_dance;
    const spell_data_t* shadow_techniques;
    const spell_data_t* shadow_techniques_effect;
    const spell_data_t* symbols_of_death;
    const spell_data_t* eviscerate;
    const spell_data_t* eviscerate_2;
    const spell_data_t* shadowstrike;
    const spell_data_t* shadowstrike_2;
    const spell_data_t* shuriken_combo;
    const spell_data_t* t20_2pc_subtlety;
  } spec;

  // Spell Data
  struct spells_t
  {
    const spell_data_t* poison_bomb_driver;
    const spell_data_t* critical_strikes;
    const spell_data_t* death_from_above;
    const spell_data_t* fan_of_knives;
    const spell_data_t* fleet_footed;
    const spell_data_t* master_of_shadows;
    const spell_data_t* sprint;
    const spell_data_t* sprint_2;
    const spell_data_t* relentless_strikes_energize;
    const spell_data_t* ruthlessness_cp_driver;
    const spell_data_t* ruthlessness_driver;
    const spell_data_t* ruthlessness_cp;
    const spell_data_t* shadow_focus;
    const spell_data_t* subterfuge;
    const spell_data_t* insignia_of_ravenholdt;
    const spell_data_t* master_assassins_initiative;
    const spell_data_t* master_assassins_initiative_2;
    const spell_data_t* expose_armor;
  } spell;

  // Talents
  struct talents_t
  {
    // Shared
    const spell_data_t* weaponmaster;

    const spell_data_t* nightstalker;
    const spell_data_t* subterfuge;

    const spell_data_t* vigor;
    const spell_data_t* deeper_stratagem;
    const spell_data_t* marked_for_death;

    const spell_data_t* alacrity;

    // Assassination
    const spell_data_t* master_poisoner;
    const spell_data_t* elaborate_planning;
    const spell_data_t* blindside;

    const spell_data_t* master_assassin;

    const spell_data_t* thuggee;
    const spell_data_t* internal_bleeding;

    const spell_data_t* venom_rush;
    const spell_data_t* toxic_blade;
    const spell_data_t* exsanguinate;

    const spell_data_t* poison_bomb;
    const spell_data_t* hidden_blades;
    const spell_data_t* crimson_tempest;

    // Outlaw
    const spell_data_t* ghostly_strike;
    const spell_data_t* quick_draw;

    const spell_data_t* hit_and_run;

    const spell_data_t* dirty_tricks;

    const spell_data_t* loaded_dice;
    const spell_data_t* slice_and_dice;

    const spell_data_t* dancing_steel;
    const spell_data_t* blade_rush;
    const spell_data_t* killing_spree;

    // Subtlety
    const spell_data_t* expose_weakness;
    const spell_data_t* gloomblade;

    const spell_data_t* shadow_focus;

    const spell_data_t* enveloping_shadows;
    const spell_data_t* dark_shadow;

    const spell_data_t* master_of_shadows;
    const spell_data_t* death_from_above;
  } talent;

  // Masteries
  struct masteries_t
  {
    // Assassination
    const spell_data_t* potent_assassin;
    // Outlaw
    const spell_data_t* main_gauche;
    // Subtlety
    const spell_data_t* executioner;
  } mastery;

  // Azerite powers
  struct azerite_powers_t
  {
    azerite_power_t deadshot;
    azerite_power_t nights_vengeance;
    azerite_power_t sharpened_blades;
    azerite_power_t storm_of_steel;
    azerite_power_t twist_the_knife;
  } azerite;

  // Procs
  struct procs_t
  {
    // Assassination
    proc_t* seal_fate;

    // Outlaw
    proc_t* roll_the_bones_1;
    proc_t* roll_the_bones_2;
    proc_t* roll_the_bones_3;
    proc_t* roll_the_bones_4;
    proc_t* roll_the_bones_5;
    proc_t* roll_the_bones_6;
    proc_t* t21_4pc_outlaw;

    // Subtlety
    proc_t* deepening_shadows;
    proc_t* weaponmaster;
  } procs;

  struct legendary_t
  {
    const spell_data_t* duskwalker_footpads;
    const spell_data_t* denial_of_the_halfgiants;
    const spell_data_t* zoldyck_family_training_shackles;
    const spell_data_t* the_dreadlords_deceit;
    const spell_data_t* insignia_of_ravenholdt;
    const spell_data_t* mantle_of_the_master_assassin;
    const spell_data_t* the_curse_of_restlessness;
    const spell_data_t* the_empty_crown;
    const spell_data_t* the_first_of_the_dead;
    const spell_data_t* sephuzs_secret;
    const spell_data_t* thraxis_tricksy_treads;
  } legendary;

  // Options
  int initial_combo_points;
  int ssw_refund_offset;
  bool fok_rotation;
  bool rogue_optimize_expressions;
  bool rogue_ready_trigger;

  rogue_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, ROGUE, name, r ),
    df_counter( 0 ),
    shadow_techniques( 0 ),
    poisoned_enemies( 0 ),
    active_blade_flurry( nullptr ),
    active_lethal_poison( nullptr ),
    active_nonlethal_poison( nullptr ),
    active_main_gauche( nullptr ),
    poison_bomb( nullptr ),
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
    legendary( legendary_t() ),
    initial_combo_points( 0 ),
    ssw_refund_offset( 0 ),
    fok_rotation( false ),
    rogue_optimize_expressions( true ),
    rogue_ready_trigger( true )
  {
    // Cooldowns
    cooldowns.adrenaline_rush          = get_cooldown( "adrenaline_rush"          );
    cooldowns.garrote                  = get_cooldown( "garrote"                  );
    cooldowns.killing_spree            = get_cooldown( "killing_spree"            );
    cooldowns.shadow_dance             = get_cooldown( "shadow_dance"             );
    cooldowns.sprint                   = get_cooldown( "sprint"                   );
    cooldowns.vanish                   = get_cooldown( "vanish"                   );
    cooldowns.between_the_eyes         = get_cooldown( "between_the_eyes"         );
    cooldowns.blade_rush               = get_cooldown( "blade_rush"               );
    cooldowns.blind                    = get_cooldown( "blind"                    );
    cooldowns.gouge                    = get_cooldown( "gouge"                    );
    cooldowns.cloak_of_shadows         = get_cooldown( "cloak_of_shadows"         );
    cooldowns.death_from_above         = get_cooldown( "death_from_above"         );
    cooldowns.ghostly_strike           = get_cooldown( "ghostly_strike"           );
    cooldowns.grappling_hook           = get_cooldown( "grappling_hook"           );
    cooldowns.marked_for_death         = get_cooldown( "marked_for_death"         );
    cooldowns.riposte                  = get_cooldown( "riposte"                  );
    cooldowns.weaponmaster             = get_cooldown( "weaponmaster"             );
    cooldowns.vendetta                 = get_cooldown( "vendetta"                 );
    cooldowns.toxic_blade              = get_cooldown( "toxic_blade"              );
    cooldowns.symbols_of_death         = get_cooldown( "symbols_of_death"         );

    regen_type = REGEN_DYNAMIC;
    regen_caches[CACHE_HASTE] = true;
    regen_caches[CACHE_ATTACK_HASTE] = true;

    // Register a custom talent validity function that allows Vigor to be used when the user has
    // Soul of the Shadowblade.
    talent_points.register_validity_fn( [ this ]( const spell_data_t* spell ) {
      return spell -> id() == 14983 && find_item( 150936 ) != nullptr;
    } );
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
  stat_e    primary_stat() const override { return STAT_AGILITY; }

  // Default consumables
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;

  double    composite_melee_speed() const override;
  double    composite_melee_haste() const override;
  double    composite_melee_crit_chance() const override;
  double    composite_spell_crit_chance() const override;
  double    composite_spell_haste() const override;
  double    matching_gear_multiplier( attribute_e attr ) const override;
  double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double    resource_regen_per_second( resource_e ) const override;
  double    passive_movement_modifier() const override;
  double    temporary_movement_modifier() const override;

  bool poisoned_enemy( player_t* target, bool deadly_fade = false ) const;

  // Custom class expressions
  expr_t* create_rtb_buff_t21_expression( const buff_t* rtb_buff );

  void trigger_auto_attack( const action_state_t* );
  void trigger_seal_fate( const action_state_t* );
  void trigger_main_gauche( const action_state_t* );
  void trigger_combat_potency( const action_state_t* );
  void trigger_energy_refund( const action_state_t* );
  void trigger_poison_bomb( const action_state_t* );
  void trigger_venomous_wounds( const action_state_t* );
  void trigger_blade_flurry( const action_state_t* );
  void trigger_ruthlessness_cp( const action_state_t* );
  void trigger_combo_point_gain( int, gain_t* gain = nullptr, action_t* action = nullptr );
  void spend_combo_points( const action_state_t* );
  void trigger_t19oh_8pc( const action_state_t* );
  void trigger_elaborate_planning( const action_state_t* );
  void trigger_alacrity( const action_state_t* );
  void trigger_deepening_shadows( const action_state_t* );
  void trigger_shadow_techniques( const action_state_t* );
  void trigger_weaponmaster( const action_state_t* );
  void trigger_true_bearing( const action_state_t* );
  void trigger_restless_blades( const action_state_t* );
  void trigger_exsanguinate( const action_state_t* );
  void trigger_relentless_strikes( const action_state_t* );
  void trigger_insignia_of_ravenholdt( action_state_t* );
  void trigger_sephuzs_secret( const action_state_t* state, spell_mechanic mechanic, double proc_chance = -1.0 );
  void trigger_t21_4pc_assassination( const action_state_t* state );
  void trigger_t21_4pc_outlaw( const action_state_t* state );
  void trigger_t21_4pc_subtlety( const action_state_t* state );

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
  // Expiry delayed by 1ms in order to have it processed on the next tick. This seems to be what the server does.
  if ( p -> buffs.stealth -> check() )
    p -> buffs.stealth -> expire( timespan_t::from_millis( 1 ) );

  if ( p -> buffs.vanish -> check() )
    p -> buffs.vanish -> expire( timespan_t::from_millis( 1 ) );
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

  // Affect flags for various dynamic effects
  struct
  {
    bool shadow_blades;
    bool ruthlessness;
    bool relentless_strikes;
    bool deepening_shadows;
    bool vendetta;
    bool alacrity;
    bool adrenaline_rush_gcd;
    bool lesser_adrenaline_rush_gcd;
    bool broadsides;
    bool t21_2pc_assassination;
    bool master_assassin;
    bool toxic_blade;
  } affected_by;

  rogue_attack_t( const std::string& token, rogue_t* p,
                  const spell_data_t* s = spell_data_t::nil(),
                  const std::string& options = std::string() ):
    melee_attack_t( token, p, s ),
    requires_stealth( false ), requires_position( POSITION_NONE ),
    requires_weapon( WEAPON_NONE ), secondary_trigger( TRIGGER_NONE )
  {
    parse_options( options );

    may_crit = true;
    may_glance = false;
    special = true;
    tick_may_crit = true;
    hasted_ticks = false;

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
      {
        base_ta_adder = effect.bonus( player );
      }
      else if ( effect.type() == E_SCHOOL_DAMAGE )
      {
        base_dd_adder = effect.bonus( player );
      }
    }

    // Assassination Class Passive
    if ( data().affected_by( p->spec.assassination_rogue->effectN( 1 ) ) )
    {
      base_dd_multiplier *= 1.0 + p->spec.assassination_rogue->effectN( 1 ).percent();
    }
    if ( data().affected_by( p->spec.assassination_rogue->effectN( 2 ) ) )
    {
      base_td_multiplier *= 1.0 + p->spec.assassination_rogue->effectN( 2 ).percent();
    }

    // Outlaw Class Passive
    if ( data().affected_by( p->spec.outlaw_rogue->effectN( 1 ) ) )
    {
      base_dd_multiplier *= 1.0 + p->spec.outlaw_rogue->effectN( 1 ).percent();
    }
    if ( data().affected_by( p->spec.outlaw_rogue->effectN( 2 ) ) )
    {
      base_td_multiplier *= 1.0 + p->spec.outlaw_rogue->effectN( 2 ).percent();
    }

    // Subtlety Class Passive
    if ( data().affected_by( p->spec.subtlety_rogue->effectN( 1 ) ) )
    {
      base_dd_multiplier *= 1.0 + p->spec.subtlety_rogue->effectN( 1 ).percent();
    }
    if ( data().affected_by( p->spec.subtlety_rogue->effectN( 2 ) ) )
    {
      base_td_multiplier *= 1.0 + p->spec.subtlety_rogue->effectN( 2 ).percent();
    }

    // Deeper Stratagem
    if ( p->talent.deeper_stratagem->ok() )
    {
      if ( data().affected_by( p->talent.deeper_stratagem->effectN( 4 ) ) )
      {
        base_dd_multiplier *= 1.0 + p->talent.deeper_stratagem->effectN( 4 ).percent();
      }
      if ( data().affected_by( p->talent.deeper_stratagem->effectN( 5 ) ) )
      {
        base_td_multiplier *= 1.0 + p->talent.deeper_stratagem->effectN( 5 ).percent();
      }
    }

    // Master Poisoner
    if ( p->talent.master_poisoner->ok() )
    {
      if ( data().affected_by( p->talent.master_poisoner->effectN( 1 ) ) )
      {
        base_dd_multiplier *= 1.0 + p->talent.master_poisoner->effectN( 1 ).percent();
      }
      if ( data().affected_by( p->talent.master_poisoner->effectN( 2 ) ) )
      {
        base_td_multiplier *= 1.0 + p->talent.master_poisoner->effectN( 2 ).percent();
      }
    }
  }

  void init() override
  {
    melee_attack_t::init();

    // Figure out the affected flags
    affected_by.shadow_blades = data().affected_by( p() -> spec.shadow_blades -> effectN( 2 ) ) ||
                                data().affected_by( p() -> spec.shadow_blades -> effectN( 3 ) ) ||
                                data().affected_by( p() -> spec.shadow_blades -> effectN( 4 ) ) ||
                                data().affected_by( p() -> spec.shadow_blades -> effectN( 5 ) );

    affected_by.ruthlessness = base_costs[ RESOURCE_COMBO_POINT ] > 0;
    affected_by.relentless_strikes = base_costs[ RESOURCE_COMBO_POINT ] > 0;
    affected_by.deepening_shadows = base_costs[ RESOURCE_COMBO_POINT ] > 0;
    affected_by.vendetta = data().affected_by( p() -> spec.vendetta -> effectN( 1 ) );
    affected_by.alacrity = base_costs[ RESOURCE_COMBO_POINT ] > 0;
    affected_by.adrenaline_rush_gcd = data().affected_by( p() -> buffs.adrenaline_rush -> data().effectN( 3 ) );
    affected_by.lesser_adrenaline_rush_gcd = data().affected_by( p() -> buffs.t20_4pc_outlaw -> data().effectN( 3 ) );
    affected_by.broadsides = data().affected_by( p() -> buffs.broadsides -> data().effectN( 4 ) );
    affected_by.t21_2pc_assassination = data().affected_by( p()->sets->set( ROGUE_ASSASSINATION, T21, B2 )->effectN( 1 ).trigger()->effectN( 1 ) );
    affected_by.master_assassin = data().affected_by( p() -> spec.master_assassin -> effectN( 1 ) );
    affected_by.toxic_blade = data().affected_by( p() -> talent.toxic_blade -> effectN( 4 ).trigger() -> effectN( 1 ) );
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

    if ( affected_by.adrenaline_rush_gcd && t != timespan_t::zero() && p() -> buffs.adrenaline_rush -> check() )
    {
      t += p() -> buffs.adrenaline_rush -> data().effectN( 3 ).time_value();
    }

    if ( affected_by.lesser_adrenaline_rush_gcd && t != timespan_t::zero() && p() -> buffs.t20_4pc_outlaw -> check() )
    {
      t += p() -> buffs.t20_4pc_outlaw -> data().effectN( 3 ).time_value();
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

  // Generic rules for snapshotting the Nightstalker pmultiplier, default to DoTs only
  virtual bool snapshots_nightstalker() const
  { 
    return dot_duration > timespan_t::zero() && base_tick_time > timespan_t::zero();
  }

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
  void   schedule_travel( action_state_t* state ) override;
  void   tick( dot_t* d ) override;

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return attack_power_mod.direct * cast_state( s ) -> cp;
    return melee_attack_t::attack_direct_power_coefficient( s );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return spell_power_mod.direct * cast_state( s ) -> cp;
    return melee_attack_t::spell_direct_power_coefficient( s );
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

    // Subtlety
    if ( p()->mastery.executioner->ok() && data().affected_by( p()->mastery.executioner->effectN( 1 ) ) )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( p()->buffs.symbols_of_death->check() && data().affected_by( p()->buffs.symbols_of_death->data().effectN( 1 ) ) )
    {
      m *= 1.0 + p()->buffs.symbols_of_death->data().effectN( 1 ).percent()
        + p()->spec.t20_2pc_subtlety->effectN( 1 ).percent();
    }

    if ( p()->buffs.shadow_dance->up() && data().affected_by( p()->buffs.shadow_dance->data().effectN( 4 ) ) )
    {
      m *= 1.0 + p()->buffs.shadow_dance->data().effectN( 4 ).percent()
        + p()->talent.dark_shadow->effectN( 2 ).percent();
    }

    // Assassination
    if ( p()->mastery.potent_assassin->ok() && data().affected_by( p()->mastery.potent_assassin->effectN( 1 ) ) )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }
    
    if ( p()->buffs.elaborate_planning->check() && data().affected_by( p()->buffs.elaborate_planning->data().effectN( 1 ) ) )
    {
      m *= 1.0 + p()->buffs.elaborate_planning->data().effectN( 1 ).percent();
    }

    return m;
  }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = melee_attack_t::composite_ta_multiplier( state );

    // Subtlety
    if ( p()->mastery.executioner->ok() && data().affected_by( p()->mastery.executioner->effectN( 2 ) ) )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( p()->buffs.symbols_of_death->check() && data().affected_by( p()->buffs.symbols_of_death->data().effectN( 2 ) ) )
    {
      m *= 1.0 + p()->buffs.symbols_of_death->data().effectN( 2 ).percent()
        + p()->spec.t20_2pc_subtlety->effectN( 2 ).percent();
    }

    if ( p()->buffs.shadow_dance->up() && data().affected_by( p()->buffs.shadow_dance->data().effectN( 5 ) ) )
    {
      // TOCHECK: The spell data for Dark Shadow effect 3 seems like it might not be configured correctly
      m *= 1.0 + p()->buffs.shadow_dance->data().effectN( 5 ).percent() 
        + p()->talent.dark_shadow->effectN( 3 ).percent();
    }

    // Assassination
    if ( p()->mastery.potent_assassin->ok() && data().affected_by( p()->mastery.potent_assassin->effectN( 2 ) ) )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( p()->buffs.elaborate_planning->check() && data().affected_by( p()->buffs.elaborate_planning->data().effectN( 2 ) ) )
    {
      m *= 1.0 + p()->buffs.elaborate_planning->data().effectN( 2 ).percent();
    }

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

    if ( tdata -> dots.nightblade -> is_ticking() && data().affected_by( tdata -> dots.nightblade -> current_action -> data().effectN( 6 ) ) )
    {
      m *= 1.0 + tdata -> dots.nightblade -> current_action -> data().effectN( 6 ).percent();
    }

    if ( affected_by.toxic_blade )
    {
      m *= 1.0 + tdata -> debuffs.toxic_blade -> value();
    }

    return m;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = melee_attack_t::composite_persistent_multiplier( state );

    // Apply Nightstalker as a Persistent Multiplier for things that snapshot
    if ( p()->talent.nightstalker->ok() && snapshots_nightstalker() &&
      ( p()->buffs.stealth->check() || p()->buffs.shadow_dance->check() || p()->buffs.vanish->check() ) )
    {
      m *= 1.0 + ( p()->talent.nightstalker->effectN( 2 ).percent() + p()->spec.subtlety_rogue->effectN( 4 ).percent() );
    }

    return m;
  }

  double action_multiplier() const override
  {
    double m = melee_attack_t::action_multiplier();

    if (affected_by.broadsides && p() -> buffs.broadsides -> up())
    {
      m *= 1.0 + p() -> buffs.broadsides -> data().effectN( 4 ).percent();
    }

    // Apply Nightstalker as an Action Multiplier for things that don't snapshot
    if ( p()->talent.nightstalker->ok() && !snapshots_nightstalker() &&
      ( p()->buffs.stealth->check() || p()->buffs.shadow_dance->check() || p()->buffs.vanish->check() ) )
    {
      m *= 1.0 + ( p()->talent.nightstalker->effectN( 2 ).percent() + p()->spec.subtlety_rogue->effectN( 4 ).percent() );
    }

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = melee_attack_t::composite_crit_chance();

    if ( affected_by.master_assassin )
    {
      c += p() -> buffs.master_assassin -> stack_value();
      c += p() -> buffs.master_assassin_aura -> stack_value();
    }

    return c;
  }

  double target_armor( player_t* target ) const override
  {
    double a = melee_attack_t::target_armor( target );
    a *= 1.0 - td( target ) -> debuffs.expose_weakness -> value();
    return a;
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

  void snapshot_internal( action_state_t* state, unsigned flags, dmg_e rt ) override
  {
    // Exsanguinated bleeds snapshot hasted tick time when the ticks are rescheduled.
    // This will make snapshot_internal on haste updates discard the new value.
    if ( cast_state( state ) -> exsanguinated )
      flags &= ~STATE_HASTE;

    melee_attack_t::snapshot_internal( state, flags, rt );
  }

  virtual double composite_poison_flat_modifier( const action_state_t* ) const
  { return 0.0; }

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
      action_state_t* s = attack -> get_state();
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
    attack_power_mod.direct = p -> mastery.main_gauche -> ok() ? 1.0 : 0.0; // Mastery mod below
    special = background = may_crit = true;
    proc = true; // it's proc; therefore it cannot trigger main_gauche for chain-procs
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    // TEMP, very close to alpha value
    m *= 2.0 * p() -> cache.mastery_value();

    return m;
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
    aoe        = -1;
    radius     = 5;
    range      = -1.0;
  }

  bool procs_main_gauche() const override
  { return false; }

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
    hasted_ticks = true;
    // Need to fake this here so it uses the correct AP coefficient
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
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
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t duration = rogue_attack_t::composite_dot_duration( s );

    if ( cast_state( s ) -> exsanguinated )
    {
      duration *= 1.0 / ( 1.0 + p() -> talent.exsanguinate -> effectN( 1 ).percent() );
    }

    return duration;
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    p() -> trigger_venomous_wounds( d -> state );
  }
};

struct poison_bomb_t : public rogue_attack_t
{
  poison_bomb_t( rogue_t* p ) :
    rogue_attack_t( "poison_bomb", p, p -> find_spell( 255546 ) )
  {
    background = true;
    aoe = -1;
    may_miss = may_dodge = may_parry = may_block = false;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    // Legendary Bracers works since 7.1.5
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

// Legendary 7.0
struct insignia_of_ravenholdt_attack_t : public rogue_attack_t
{
  insignia_of_ravenholdt_attack_t( rogue_t* p ) :
    rogue_attack_t( "insignia_of_ravenholdt", p, p -> find_spell( 209043 ) )
  {
    background = true;
    aoe = -1;
  }

  void init() override
  {
    rogue_attack_t::init();

    // We do not want Versatility applied to Insignia dmg again
    snapshot_flags &= ~STATE_VERSATILITY;
  }

  double composite_da_multiplier( const action_state_t* ) const override
  {
    double m = p() -> spell.insignia_of_ravenholdt -> effectN( 1 ).percent();

    // Rogue Assassination Specific
    if ( p() -> specialization() == ROGUE_ASSASSINATION )
    {
      // Hidden Passive (Additive, it's +15% on the primary effect)
      m += p() -> spec.assassination_rogue -> effectN( 3 ).percent();
    }

    return m;
  }

  double composite_target_multiplier( player_t* ) const override
  {
    // Target Modifier aren't taken in account for the proc (else double dip)
    return 1.0;
  }

  double composite_persistent_multiplier(const action_state_t* ) const override
  {
    // 1/15/2018 - Does not double-dip from Nightstalker bonus
    return 1.0;
  }
};

// As of 04/08/2017 it acts like an Ignite (i.e. remaining damage are added).
// Also it doesn't pandemic and doesn't work with any Assassination modifier (else it would double dip).
// It doesn't work with Hemorrhage nor with Venomous Wounds nor Zoldyck Family Training Shackles.
// The only time it is counted "as a bleed" is for T19 4PC (it increases envenom damage).
using namespace residual_action;
struct mutilated_flesh_t : public residual_periodic_action_t<melee_attack_t>
{
  rogue_t* rogue;
  mutilated_flesh_t( rogue_t* rogue_ ) :
    residual_periodic_action_t<melee_attack_t>( "mutilated_flesh", rogue_, rogue_ -> find_spell( 211672 ) ), rogue( rogue_ )
  {
    background = true;
  }

  double calculate_tick_amount( action_state_t* state, double dmg_multiplier ) const override
  {
    //rogue_td_t* tdata = rogue -> get_target_data( state -> target );

    return residual_periodic_action_t::calculate_tick_amount( state, dmg_multiplier );
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

    set_target( source_state -> target );
    execute();
  }

  double composite_crit_chance() const override
  {
    double c = rogue_attack_t::composite_crit_chance();

    if ( affected_by.t21_2pc_assassination && p() -> buffs.t21_2pc_assassination -> up() )
    {
      c += p() -> buffs.t21_2pc_assassination -> value();
    }

    return c;
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
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = rogue_poison_t::composite_target_multiplier( target );

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
      rogue_poison_t::impact( state );

      p() -> trigger_t21_4pc_assassination( state );
    }
  };

  struct deadly_poison_dot_t : public rogue_poison_t
  {
    deadly_poison_dot_t( rogue_t* p ) :
      rogue_poison_t( "deadly_poison_dot", p, p -> find_specialization_spell( "Deadly Poison" ) -> effectN( 1 ).trigger() )
    {
      may_crit       = false;
      harmful        = true;
      hasted_ticks   = true;
    }

    timespan_t calculate_dot_refresh_duration(const dot_t* dot, timespan_t /* triggered_duration */) const override
    {
      // 12/29/2017 - Deadly Poison uses an older style of refresh, adding the origial duration worth of ticks up to 50% more than the base number of ticks
      //              Deadly Poison shouldn't have partial ticks, so we just add the amount of time relative to how many additional ticks we want to add
      const int additional_ticks = (int)(data().duration() / dot->time_to_tick);
      const int max_ticks = (int)(additional_ticks * 1.5);
      return dot->remains() + std::min( max_ticks - dot->ticks_left(), additional_ticks ) * dot->time_to_tick;
    }

    void impact( action_state_t* state ) override
    {
      if ( ! p() -> poisoned_enemy( state -> target ) && result_is_hit( state -> result ) )
      {
        p() -> poisoned_enemies++;
      }

      rogue_poison_t::impact( state );
    }

    void tick( dot_t* d ) override
    {
      rogue_poison_t::tick( d );

      p() -> trigger_t21_4pc_assassination( d -> state );
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
      double m = rogue_poison_t::composite_target_multiplier( target );

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
      proc_dot -> set_target( state -> target );
      proc_dot -> execute();
      if ( is_up )
      {
        proc_instant -> set_target( state -> target );
        proc_instant -> execute();
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
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = rogue_poison_t::composite_target_multiplier( target );

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

        if ( ! sim -> overrides.mortal_wounds && state -> target -> debuffs.mortal_wounds )
        {
          state -> target -> debuffs.mortal_wounds -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );
        }

        p() -> trigger_t21_4pc_assassination( state );
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

    proc_dd -> set_target( state -> target );
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

    proc -> set_target( state -> target );
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

    proc -> set_target( state -> target );
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
    LEECHING_POISON
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
      if      ( lethal_str.empty()        ) lethal_poison = DEADLY_POISON;
      else if ( lethal_str == "deadly"    ) lethal_poison = DEADLY_POISON;
      else if ( lethal_str == "wound"     ) lethal_poison = WOUND_POISON;

      if ( nonlethal_str == "crippling" ) nonlethal_poison = CRIPPLING_POISON;
      if ( nonlethal_str == "leeching"  ) nonlethal_poison = LEECHING_POISON;
    }

    if ( ! p -> active_lethal_poison )
    {
      if ( lethal_poison == DEADLY_POISON  ) p -> active_lethal_poison = new deadly_poison_t( p );
      if ( lethal_poison == WOUND_POISON   ) p -> active_lethal_poison = new wound_poison_t( p );
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

  p() -> trigger_main_gauche( state );
  p() -> trigger_combat_potency( state );
  p() -> trigger_blade_flurry( state );
  p() -> trigger_shadow_techniques( state );
  p() -> trigger_insignia_of_ravenholdt( state );

  if ( result_is_hit( state -> result ) )
  {
    if ( procs_poison() && p() -> active_lethal_poison )
      p() -> active_lethal_poison -> trigger( state );

    if ( procs_poison() && p() -> active_nonlethal_poison )
      p() -> active_nonlethal_poison -> trigger( state );
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
    c *= 1.0 + p() -> spell.shadow_focus -> effectN( 1 ).percent();
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

  if ( result_is_miss( execute_state -> result ) && last_resource_cost > 0 )
    p() -> trigger_energy_refund( execute_state );

  if ( last_resource_cost > 0 && p() -> legendary.duskwalker_footpads )
  {
    p() -> df_counter += last_resource_cost;
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

  p() -> trigger_auto_attack( execute_state );

  p() -> trigger_ruthlessness_cp( execute_state );

  if ( energize_type != ENERGIZE_NONE && energize_resource == RESOURCE_COMBO_POINT &&
    affected_by.shadow_blades && p() -> buffs.shadow_blades -> up() )
  {
    p() -> trigger_combo_point_gain( 1, p() -> gains.shadow_blades, this );
  }

  p() -> trigger_relentless_strikes( execute_state );

  p() -> trigger_elaborate_planning( execute_state );
  p() -> trigger_alacrity( execute_state );
  p() -> trigger_t19oh_8pc( execute_state );

  if ( harmful && stealthed() )
  {
    player -> buffs.shadowmeld -> expire();

    // Check stealthed again after shadowmeld is popped. If we're still
    // stealthed, trigger subterfuge
    if ( stealthed() && p() -> talent.subterfuge -> ok() && ! p() -> buffs.subterfuge -> check() )
      p() -> buffs.subterfuge -> trigger();
    else
      break_stealth( p() );
  }

  p() -> trigger_deepening_shadows( execute_state );
}

// rogue_attack_t::schedule_travel ==========================================

void rogue_attack_t::schedule_travel( action_state_t* state )
{
  melee_attack_t::schedule_travel( state );

  if ( energize_type != ENERGIZE_NONE && energize_resource == RESOURCE_COMBO_POINT )
    p() -> trigger_seal_fate( state );
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
}

// Melee Attack =============================================================

struct melee_t : public rogue_attack_t
{
  int sync_weapons;
  bool first;

  melee_t( const char* name, rogue_t* p, int sw ) :
    rogue_attack_t( name, p ), sync_weapons( sw ), first( true )
  {
    background = repeating = may_glance = true;
    special           = false;
    school            = SCHOOL_PHYSICAL;
    trigger_gcd       = timespan_t::zero();
    weapon_multiplier = 1.0;

    if ( p -> dual_wield() )
      base_hit -= 0.19;

    p -> auto_attack = this;
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

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> buffs.sharpened_blades -> trigger();
    }
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    rogue_td_t* tdata = td( target );
    if ( tdata->debuffs.vendetta->check() )
    {
      m *= 1.0 + tdata->debuffs.vendetta->data().effectN( 2 ).percent();
    }

    return m;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    // Subtlety
    if ( p()->buffs.symbols_of_death->check() )
    {
      m *= 1.0 + p()->buffs.symbols_of_death->data().effectN( 2 ).percent()
        + p()->spec.t20_2pc_subtlety->effectN( 3 ).percent();
    }

    if ( p()->buffs.shadow_dance->up() )
    {
      m *= 1.0 + p()->buffs.shadow_dance->data().effectN( 2 ).percent()
        + p()->talent.dark_shadow->effectN( 1 ).percent();
    }

    // Assassination
    if ( p()->buffs.elaborate_planning->check() )
    {
      m *= 1.0 + p()->buffs.elaborate_planning->data().effectN( 3 ).percent();
    }

    return m;
  }

  // Auto Attacks don't double-dip from the bonus AP from WDPS
  double composite_attack_power() const override
  {
    return melee_attack_t::composite_attack_power();
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
    if ( target -> is_sleeping() )
    {
      return false;
    }

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
    use_off_gcd = p -> talent.death_from_above -> ok();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.adrenaline_rush -> trigger();
    if ( p() -> talent.loaded_dice -> ok() )
      p() -> buffs.loaded_dice -> trigger();
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

  void execute() override
  {
    rogue_attack_t::execute();
    if ( p() -> buffs.broadsides -> up() )
    {
      p() -> trigger_combo_point_gain( p() -> buffs.broadsides -> data().effectN( 1 ).base_value(),
          p() -> gains.broadsides, this );
    }
  }
};

// Backstab =================================================================

struct backstab_t : public rogue_attack_t
{
  backstab_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "backstab", p, p -> find_specialization_spell( "Backstab" ), options_str )
  {
    requires_weapon = WEAPON_DAGGER;
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

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p() -> buffs.the_first_of_the_dead -> up() )
    {
      p() -> trigger_combo_point_gain( (int)p() -> buffs.the_first_of_the_dead -> data().effectN( 2 ).resource( RESOURCE_COMBO_POINT ),
                                       p() -> gains.the_first_of_the_dead, this );
    }

    p() -> trigger_t21_4pc_subtlety( execute_state );
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    p() -> trigger_weaponmaster( state );
  }
};

// Between the Eyes =========================================================

struct between_the_eyes_t : public rogue_attack_t
{
  const spell_data_t* greenskins_waterlogged_wristcuffs; // 7.0 legendary Greenskin's Waterlogged Wristcuffs

  between_the_eyes_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "between_the_eyes", p, p -> find_specialization_spell( "Between the Eyes" ),
                    options_str ), greenskins_waterlogged_wristcuffs( nullptr )
  {
    crit_bonus_multiplier *= 1.0 + p -> find_specialization_spell( 235484 ) -> effectN( 1 ).percent();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> trigger_restless_blades( execute_state );

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

    p() -> trigger_sephuzs_secret( execute_state, MECHANIC_STUN );

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> buffs.deadshot -> trigger();
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
    internal_cooldown -> duration += p -> talent.dancing_steel -> effectN( 4 ).time_value();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.blade_flurry -> trigger();
    if ( shivarran_symmetry )
    {
      p() -> buffs.shivarran_symmetry -> trigger();
    }
  }
};

// Blade Rush ===============================================================

struct blade_rush_t : public rogue_attack_t
{
  struct blade_rush_attack_t : public rogue_attack_t
  {
    blade_rush_attack_t( rogue_t* p ) :
      rogue_attack_t( "blade_rush_attack", p, p -> find_spell( 271881 ) )
    {
      aoe = -1;
    }

    double composite_da_multiplier( const action_state_t* state ) const override
    {
      double m = rogue_attack_t::composite_da_multiplier( state );

      if ( state -> target == state -> action -> target )
        m *= data().effectN( 2 ).percent();
      else if ( p() -> buffs.blade_flurry -> up() )
        m *= 1.0 + p() -> talent.blade_rush -> effectN( 1 ).percent();

      return m;
    }
  };

  rogue_attack_t* blade_rush_attack;

  blade_rush_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "blade_rush", p, p -> talent.blade_rush, options_str )
  {
    blade_rush_attack = new blade_rush_attack_t( p );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    blade_rush_attack -> execute();
    p() -> buffs.blade_rush -> trigger();
  }
};

// Blindside ================================================================

struct blindside_t: public rogue_attack_t
{
  blindside_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "blindside", p, p -> talent.blindside, options_str )
  {
    requires_weapon = WEAPON_DAGGER;
  }

  bool ready() override
  {
    if ( ! p() -> buffs.blindside -> check() && target -> health_percentage() >= data().effectN( 4 ).base_value() )
      return false;

    return rogue_attack_t::ready();
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.blindside -> check() )
      c = 0;

    return c;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p() -> buffs.blindside -> up() )
      p() -> buffs.blindside -> expire();
  }
};

// Crimson Tempest ==========================================================

struct crimson_tempest_t : public rogue_attack_t
{
  crimson_tempest_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "crimson_tempest", p, p -> talent.crimson_tempest, options_str )
  {
    aoe = -1;
    hasted_ticks = true;
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

  // Base damage of Crimson Tempest does scale with CP+1, calling melee_attack_t instead of rogue parent on purpose
  double attack_direct_power_coefficient( const action_state_t* s ) const override
  { 
    return melee_attack_t::attack_direct_power_coefficient( s ) * ( cast_state( s ) -> cp + 1 );
  }
};

// Dispatch =================================================================

struct dispatch_t: public rogue_attack_t
{
  dispatch_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "dispatch", p, p -> find_specialization_spell( "Dispatch" ), options_str )
  {
  }

  bool procs_main_gauche() const override
  {
    return false;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = rogue_attack_t::bonus_da( s );
    b += p() -> buffs.storm_of_steel -> stack_value();
    return b;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p() -> buffs.death_from_above -> up() )
      m *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 2 ).percent();

    if ( p() -> legendary.thraxis_tricksy_treads )
    {
      const double increased_speed = ( p() -> cache.run_speed() / p() -> base_movement_speed ) - 1.0;
      m *= 1.0 + increased_speed * p() -> legendary.thraxis_tricksy_treads -> effectN( 1 ).percent();
    }

    if ( p() -> buffs.t21_2pc_outlaw -> up() )
    {
      m *= 1.0 + p() -> buffs.t21_2pc_outlaw -> stack_value();
    }

    return m;
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.death_from_above -> check() )
      c = 0;

    return c;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> trigger_restless_blades( execute_state );

    if ( p() -> buffs.true_bearing -> up() )
    {
      p() -> trigger_true_bearing( execute_state );
    }

    p() -> buffs.t19_4pc_outlaw -> trigger();

    if ( p() -> buffs.t21_2pc_outlaw -> check() )
    {
      p() -> buffs.t21_2pc_outlaw -> expire();
    }

    p() -> trigger_t21_4pc_outlaw( execute_state );

    p() -> buffs.storm_of_steel -> expire();
  }
};

// Envenom ==================================================================

struct envenom_t : public rogue_attack_t
{
  envenom_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "envenom", p, p -> find_specialization_spell( "Envenom" ), options_str )
  {
    dot_duration = timespan_t::zero();
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = rogue_attack_t::bonus_da( s );

    if ( td( s -> target ) -> dots.garrote -> is_ticking() )
      b += p() -> azerite.twist_the_knife.value();

    return b;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    if ( p() -> sets -> has_set_bonus( ROGUE_ASSASSINATION, T19, B4 ) )
    {
      size_t bleeds = 0;
      rogue_td_t* tdata = td( target );
      bleeds += tdata -> dots.garrote -> is_ticking();
      bleeds += tdata -> dots.internal_bleeding -> is_ticking();
      bleeds += tdata -> dots.rupture -> is_ticking();
      // As of 04/08/2017, Mutilated Flesh works on T19 4PC.
      bleeds += tdata -> dots.mutilated_flesh -> is_ticking();

      m *= 1.0 + p() -> sets -> set( ROGUE_ASSASSINATION, T19, B4 ) -> effectN( 1 ).percent() * bleeds;
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

  double cost() const override
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.death_from_above -> check() )
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

    p() -> trigger_poison_bomb( execute_state );

    if ( p() -> sets -> has_set_bonus( ROGUE_ASSASSINATION, T21, B2 ) )
    {
      p() -> buffs.t21_2pc_assassination -> trigger();
    }
  }
};

// Eviscerate ===============================================================

struct eviscerate_t : public rogue_attack_t
{
  eviscerate_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "eviscerate", p, p -> spec.eviscerate, options_str )
  {
    base_multiplier *= 1.0 + p -> spec.eviscerate_2 -> effectN( 1 ).percent();
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = rogue_attack_t::bonus_da( s );
    b += p() -> buffs.nights_vengeance -> stack_value();
    return b;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p() -> buffs.death_from_above -> up() )
      m *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 2 ).percent();

    m *= 1.0 + p() -> buffs.shuriken_combo -> check_stack_value();

    return m;
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.death_from_above -> check() )
    {
      c = 0;
    }

    return c;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p() -> buffs.shuriken_combo -> up() )
    {
      p() -> buffs.shuriken_combo -> expire();
    }

    p() -> buffs.nights_vengeance -> expire();
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

    m *= 1.0 + p() -> buffs.the_dreadlords_deceit -> check_stack_value();

    m *= 1.0 + p() -> buffs.hidden_blades -> check_stack_value();

    return m;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p() -> buffs.the_dreadlords_deceit -> up() )
      p() -> buffs.the_dreadlords_deceit -> expire();

    if ( p() -> buffs.hidden_blades -> up() )
      p() -> buffs.hidden_blades -> expire();
  }

  void schedule_travel( action_state_t* state ) override
  {
    rogue_attack_t::schedule_travel( state );

    if ( result_is_hit( state -> result ) )
    {
      // 2018-01-25: Poisons are applied on cast as well
      // Note: Usual application on impact will not happen because this attack has no weapon assigned
      if ( p() -> active_lethal_poison )
        p() -> active_lethal_poison -> trigger( state );

      if ( p() -> active_nonlethal_poison )
        p() -> active_nonlethal_poison -> trigger( state );
    }
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
    may_crit = false;
    hasted_ticks = true;

    if ( p -> sets -> has_set_bonus( ROGUE_ASSASSINATION, T20, B2 ) ) {
      base_costs[ RESOURCE_ENERGY ] += p -> sets -> set( ROGUE_ASSASSINATION, T20, B2 ) -> effectN( 2 ).base_value();
      cooldown -> duration = data().cooldown() + p -> sets -> set( ROGUE_ASSASSINATION, T20, B2 ) -> effectN( 1 ).time_value();
    }

    if ( p -> sets -> has_set_bonus( ROGUE_ASSASSINATION, T20, B4 ) )
      base_multiplier *= 1.0 + p -> sets -> set( ROGUE_ASSASSINATION, T20, B4 ) -> effectN( 1 ).percent();
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_persistent_multiplier( state );

    if ( p() -> talent.subterfuge -> ok() &&
         ( p() -> buffs.stealth -> up() || p() -> buffs.vanish -> up() || p() -> buffs.subterfuge -> up() ) )
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
    if ( p() -> talent.subterfuge -> ok() &&
         ( p() -> buffs.stealth -> check() || p() -> buffs.vanish -> check() || p() -> buffs.subterfuge -> check() ) )
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

    if ( p -> talent.dirty_tricks -> ok() )
      base_costs[ RESOURCE_ENERGY ] = 0;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( result_is_hit (execute_state -> result ) && p() -> buffs.broadsides -> up() )
    {
      p() -> trigger_combo_point_gain( p() -> buffs.broadsides -> data().effectN( 1 ).base_value(),
          p() -> gains.broadsides, this );

      p() -> trigger_sephuzs_secret( execute_state, MECHANIC_INCAPACITATE );
    }
  }
};

// Ghostly Strike ===========================================================

struct ghostly_strike_t : public rogue_attack_t
{
  ghostly_strike_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "ghostly_strike", p, p -> talent.ghostly_strike, options_str )
  {
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
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p() -> buffs.shadow_blades -> up() )
      m *= 1.0 + p() -> buffs.shadow_blades -> data().effectN( 5 ).percent();

    return m;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p() -> buffs.the_first_of_the_dead -> up() )
    {
      p() -> trigger_combo_point_gain( (int)p() -> buffs.the_first_of_the_dead -> data().effectN( 2 ).resource( RESOURCE_COMBO_POINT ),
                                       p() -> gains.the_first_of_the_dead, this );
    }

    p() -> trigger_t21_4pc_subtlety( execute_state );
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
    if ( target -> debuffs.casting && ! target -> debuffs.casting -> check() )
      return false;

    return rogue_attack_t::ready();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> trigger_sephuzs_secret( execute_state, MECHANIC_INTERRUPT );
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
    add_child( attack_mh );

    if ( player -> off_hand_weapon.type != WEAPON_NONE )
    {
      attack_oh = new killing_spree_tick_t( p, "killing_spree_oh", p -> find_spell( 57841 ) -> effectN( 2 ).trigger() );
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

// Pistol Shot ==============================================================

struct pistol_shot_t : public rogue_attack_t
{
  pistol_shot_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "pistol_shot", p, p -> find_specialization_spell( "Pistol Shot" ), options_str )
  {
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.opportunity -> check() )
    {
      c *= 1.0 + p() -> buffs.opportunity -> data().effectN( 1 ).percent();
    }

    return c;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p() -> buffs.opportunity -> up() )
    {
      double ps_mod = 1.0 + p() -> buffs.opportunity -> data().effectN( 3 ).percent();

      if ( p() -> talent.quick_draw -> ok() )
        ps_mod += p() -> talent.quick_draw -> effectN( 1 ).percent();

      m *= ps_mod;
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

  double bonus_da( const action_state_t* s ) const override
  {
    double b = rogue_attack_t::bonus_da( s );
    b += p() -> buffs.deadshot -> stack_value();
    return b;
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

      if ( p() -> sets -> has_set_bonus( ROGUE_OUTLAW, T20, B2 ) && p() -> buffs.opportunity -> check() )
      {
        p() -> buffs.t20_2pc_outlaw -> trigger();
      }
    }

    // Expire buffs.
    p() -> buffs.opportunity -> expire();
    p() -> buffs.greenskins_waterlogged_wristcuffs -> expire();
    p() -> buffs.deadshot -> expire();
  }
};

// Marked for Death =========================================================

struct marked_for_death_t : public rogue_attack_t
{
  bool precombat;

  marked_for_death_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "marked_for_death", p, p -> find_talent_spell( "Marked for Death" ) ),
    precombat (false)
  {
    add_option( opt_bool( "precombat", precombat ) );
    parse_options( options_str );

    may_miss = may_crit = harmful = callbacks = false;
    energize_type = ENERGIZE_ON_CAST;
    cooldown -> duration += timespan_t::from_millis( p -> spec.subtlety_rogue -> effectN( 6 ).base_value() );
    cooldown -> duration += timespan_t::from_millis( p -> spec.assassination_rogue -> effectN( 4 ).base_value() );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( precombat && ! p() -> in_combat ) {
      p() -> cooldowns.marked_for_death -> adjust( - timespan_t::from_seconds( 15.0 ), false );
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

    return c;
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    p() -> trigger_seal_fate( state );

    if ( p() -> sets -> has_set_bonus( ROGUE_ASSASSINATION, T19, B2 ) && result_is_hit( state -> result ) )
    {
      double amount = state -> result_amount * p() -> sets -> set( ROGUE_ASSASSINATION, T19, B2 ) -> effectN( 1 ).percent();

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
    add_child( mh_strike );

    oh_strike = new mutilate_strike_t( p, "mutilate_oh", data().effectN( 4 ).trigger(), toxic_mutilator_crit_chance );
    add_child( oh_strike );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> talent.blindside -> ok() && rng().roll( p() -> talent.blindside -> effectN( 5 ).percent() ) )
        p() -> buffs.blindside -> trigger();

      mh_strike -> set_target( execute_state -> target );
      mh_strike -> execute();

      oh_strike -> set_target( execute_state -> target );
      oh_strike -> execute();

      if ( p() -> talent.venom_rush->ok() && p() -> get_target_data( execute_state -> target ) -> poisoned() )
      {
          p() -> resource_gain( RESOURCE_ENERGY,
              p() -> talent.venom_rush -> effectN(1).base_value(),
              p() -> gains.venom_rush );
      }
    }
  }
};

// Nightblade ===============================================================

struct nightblade_t : public rogue_attack_t
{
  nightblade_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "nightblade", p, p -> find_specialization_spell( "Nightblade" ), options_str )
  {
    may_crit = false;
    hasted_ticks = true;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t base_per_tick = data().effectN( 1 ).period();
    base_per_tick += timespan_t::from_seconds( p() -> sets -> set( ROGUE_SUBTLETY, T19, B2 ) -> effectN( 1 ).base_value() );

    return data().duration() + base_per_tick * cast_state( s ) -> cp;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
      p() -> buffs.nights_vengeance -> trigger();
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

    p() -> trigger_restless_blades( execute_state );

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
    hasted_ticks = true;
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

    p() -> trigger_poison_bomb( execute_state );
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

// Shadow Blades ============================================================

struct shadow_blade_t : public rogue_attack_t
{
  shadow_blade_t( const std::string& name_str, rogue_t* p, const spell_data_t* s ) :
    rogue_attack_t( name_str, p, s )
  {
    school  = SCHOOL_SHADOW;
    special = false;
    repeating = true;
    background = true;
    may_glance = false;
    base_execute_time = weapon -> swing_time;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> buffs.sharpened_blades -> trigger();
    }
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
      p -> shadow_blade_main_hand = 
        new shadow_blade_t( "shadow_blade_mh", p, data().effectN( 1 ).trigger() );
      add_child( p -> shadow_blade_main_hand );
    }

    if ( ! p -> shadow_blade_off_hand && p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> shadow_blade_off_hand = 
        new shadow_blade_t( "shadow_blade_offhand", p, p -> find_spell( data().effectN( 1 ).misc_value1() ) );
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
    if ( p -> talent.enveloping_shadows -> ok() )
    {
      cooldown -> charges += p -> talent.enveloping_shadows -> effectN( 2 ).base_value();
    }

    // Note: Let usage of Shadow Dance while DfA is in flight.
    // We disable it if we don't have DfA and DSh to improve the simulation speed.
    use_off_gcd = p -> talent.death_from_above -> ok() && p -> talent.dark_shadow -> ok();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.shadow_dance -> trigger();

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

  shadowstrike_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadowstrike", p, p -> spec.shadowstrike, options_str ),
    shadow_satyrs_walk( nullptr )
  {
    requires_weapon = WEAPON_DAGGER;
    requires_stealth = true;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) && rng().roll( p() -> sets -> set( ROGUE_SUBTLETY, T19, B4 ) -> proc_chance() ) )
    {
      p() -> trigger_combo_point_gain( p() -> sets -> set( ROGUE_SUBTLETY, T19, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).base_value(),
          p() -> gains.t19_4pc_subtlety, this );
    }

    if ( p() -> buffs.the_first_of_the_dead -> up() )
    {
      p() -> trigger_combo_point_gain( (int)p() -> buffs.the_first_of_the_dead -> data().effectN( 1 ).resource( RESOURCE_COMBO_POINT ),
                                       p() -> gains.the_first_of_the_dead, this );
    }

    if ( shadow_satyrs_walk )
    {
      const spell_data_t* base_proc = p() -> find_spell( 224914 );
      // Distance set to 10y as a default value, use the offset for custom value instead of distance
      // Due to the SSW bug (still present as of 01/12/17), we always get a 3 energy refund
      // when properly placed, so we'll use 10 (so 9y for the computation) as default value.
      // On larger bosses (like Helya or Krosus), this value is higher and can be increased with the offset.
      // Bug is that it computes the distance from the center of the boss instead of the edge,
      // so it ignores the hitbox (or combat reach as it is often said).
      double distance = 10;
      double grant_energy = base_proc -> effectN( 1 ).base_value();
      while ( distance > shadow_satyrs_walk -> effectN( 2 ).base_value() )
      {
        grant_energy += shadow_satyrs_walk -> effectN( 1 ).base_value();
        distance -= shadow_satyrs_walk -> effectN( 2 ).base_value();
      }
      // Add the refund offset option
      grant_energy += p() -> ssw_refund_offset;
      p() -> resource_gain( RESOURCE_ENERGY, grant_energy, p() -> gains.shadow_satyrs_walk );
    }

    p() -> trigger_t21_4pc_subtlety( execute_state );
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( p() -> talent.expose_weakness ->ok() )
      td( state -> target ) -> debuffs.expose_weakness -> trigger();

    p() -> trigger_weaponmaster( state );
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( ( p() -> buffs.stealth -> up() || p() -> buffs.vanish -> up() ) )
    {
      m *= 1.0 + p() -> spec.shadowstrike_2 -> effectN( 2 ).percent();
    }

    return m;
  }

  // TODO: Distance movement support, should teleport up to 30 yards, with distance targeting, next
  // to the target
  double composite_teleport_distance( const action_state_t* ) const override
  {
    if ( secondary_trigger != TRIGGER_NONE )
    {
      return 0;
    }

    if ( ( p() -> buffs.stealth -> up() || p() -> buffs.vanish -> up() ) )
    {
      return range + p() -> spec.shadowstrike_2 -> effectN( 1 ).base_value();
    }

    return 0;
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

  void init() override
  {
    rogue_attack_t::init();

    // As of 2017-07-28, Shuriken Storm also grants an additional CP with Shadow Blades, but it was
    // removed from spell data during 7.2.5 PTR for some reason. We have to hardcode it here.
    affected_by.shadow_blades = true;
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

    m *= 1.0 + p() -> buffs.the_dreadlords_deceit -> check_stack_value();

    return m;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p() -> spec.shuriken_combo -> ok() && execute_state -> n_targets > 1 )
    {
      p() -> buffs.shuriken_combo -> trigger((int)(execute_state -> n_targets) - 1);
    }

    if ( p() -> buffs.the_dreadlords_deceit -> up() )
      p() -> buffs.the_dreadlords_deceit -> expire();
  }
};

// Shuriken Toss ============================================================

struct shuriken_toss_t : public rogue_attack_t
{
  shuriken_toss_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shuriken_toss", p, p -> find_specialization_spell( "Shuriken Toss" ), options_str )
  { }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = rogue_attack_t::bonus_da( s );

    b += p() -> buffs.sharpened_blades -> stack_value();

    return b;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.sharpened_blades -> expire();
  }

  // 1/15/2018 - Confirmed Shuriken Toss procs Insignia hits in-game, although Poisoned Knife does not
};

// Sinister Strike ==========================================================

struct sinister_strike_t : public rogue_attack_t
{
  struct sinister_strike_proc_event_t : public event_t
  {
    rogue_t* rogue;
    sinister_strike_t* spell;
    player_t* target;

    sinister_strike_proc_event_t( rogue_t* p, sinister_strike_t* s, player_t* t ) :
      event_t( *p, s -> delay ), rogue( p ), spell( s ), target( t )
    {
    }

    const char* name() const override
    { return "sinister_strike_proc_execute"; }

    void execute() override
    {
      spell -> set_target( target );
      spell -> execute();
      spell -> sinister_strike_proc_event = nullptr;

      if ( rogue -> sets -> has_set_bonus( ROGUE_OUTLAW, T21, B2 ) )
      {
        rogue -> buffs.t21_2pc_outlaw -> trigger();
      }

      rogue -> buffs.storm_of_steel -> trigger();
    }
  };

  sinister_strike_proc_event_t* sinister_strike_proc_event;
  timespan_t delay;

  sinister_strike_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "sinister_strike", p, p -> find_specialization_spell( "Sinister Strike" ), options_str ),
    sinister_strike_proc_event( nullptr ), delay( data().duration() )
  {
  }

  double proc_chance_main_gauche() const override
  {
    return rogue_attack_t::proc_chance_main_gauche() +
           p() -> sets -> set( ROGUE_OUTLAW, T19, B2 ) -> effectN( 1 ).percent();
  }

  void reset() override
  {
    rogue_attack_t::reset();
    sinister_strike_proc_event = nullptr;
  }

  double cost() const override
  {
    if ( p() -> buffs.t19_4pc_outlaw -> check() || sinister_strike_proc_event )
    {
      return 0;
    }

    return rogue_attack_t::cost();
  }

  double composite_energize_amount( const action_state_t* state ) const override
  {
    // Do not grant CP on extra proc event
    if ( sinister_strike_proc_event )
      return 0;

    return rogue_attack_t::composite_energize_amount( state );
  }

  double sinister_strike_proc_chance() const
  {
    double opportunity_proc_chance = data().effectN( 3 ).percent();
    opportunity_proc_chance += p() -> talent.weaponmaster -> effectN( 1 ).percent();
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

    if ( ! sinister_strike_proc_event &&
         ( p() -> buffs.opportunity -> trigger( 1, buff_t::DEFAULT_VALUE(), sinister_strike_proc_chance() ) ) )
    {
      sinister_strike_proc_event = make_event<sinister_strike_proc_event_t>( *sim, p(), this, execute_state -> target );
    }

    p() -> buffs.t19_4pc_outlaw -> decrement();

    if ( ! sinister_strike_proc_event && p() -> buffs.broadsides -> up() )
    {
      p() -> trigger_combo_point_gain( p() -> buffs.broadsides -> data().effectN( 1 ).base_value(),
          p() -> gains.broadsides, this );
    }
  }
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

    timespan_t snd_duration = ( cast_state( execute_state ) -> cp + 1 ) * p() -> buffs.slice_and_dice -> data().duration();

    double snd_mod = 1.0; // Multiplier for the SnD effects. Was changed in Legion for Loaded Dice artifact trait.
    p() -> buffs.slice_and_dice -> trigger( 1, snd_mod, -1.0, snd_duration );
  }
};

// Sprint ===================================================================

struct sprint_t : public rogue_attack_t
{
  sprint_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "sprint", p, p -> spell.sprint, options_str )
  {
    harmful = callbacks = false;
    cooldown = p -> cooldowns.sprint;
    ignore_false_positive = true;
    use_off_gcd = p -> talent.death_from_above -> ok();

    cooldown -> duration = data().cooldown()
                            + p -> spell.sprint_2 -> effectN( 1 ).time_value();
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
    requires_stealth = false;

    dot_duration = timespan_t::zero(); // TODO: Check ticking in later builds

    if ( p -> sets -> has_set_bonus( ROGUE_SUBTLETY, T20, B4 ) )
      cooldown -> duration -= timespan_t::from_seconds( p -> sets -> set( ROGUE_SUBTLETY, T20, B4 ) -> effectN( 3 ).base_value() );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.symbols_of_death -> trigger();

    if ( p() -> legendary.the_first_of_the_dead )
    {
      p() -> buffs.the_first_of_the_dead -> trigger();
    }
  }
};

// Toxic Blade ==============================================================

struct toxic_blade_t : public rogue_attack_t
{
  toxic_blade_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "toxic_blade", p, p -> talent.toxic_blade, options_str )
  {
    requires_weapon = WEAPON_DAGGER;
  }

  void impact( action_state_t* s ) override
  {
    rogue_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs.toxic_blade -> trigger();
    }
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
  }

  bool ready() override
  {
    if ( p() -> buffs.vanish -> check() )
      return false;

    return rogue_attack_t::ready();
  }
};

// Vendetta =================================================================

struct vendetta_t : public rogue_attack_t
{
  vendetta_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "vendetta", p, p -> find_specialization_spell( "Vendetta" ), options_str )
  {
    harmful = may_miss = may_crit = false;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p()->buffs.vendetta->trigger();

    rogue_td_t* td = this -> td( execute_state -> target );
    td -> debuffs.vendetta -> trigger();
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
      case ROGUE_SUBTLETY:
      {
        finisher = new eviscerate_t( p, "" );
        finisher -> stats = player -> get_stats( "eviscerate_dfa", finisher );
        stats -> add_child( finisher -> stats );
        break;
      }
      case ROGUE_OUTLAW:
      {
        finisher = new dispatch_t( p, "" );
        finisher -> stats = player -> get_stats( "dispatch_dfa", finisher );
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

    timespan_t oor_delay = timespan_t::from_seconds( rng().gauss( 1.475, 0.025 ) );

    // Make Dfa buff longer than driver since driver tick will expire it and DfA should not run out first.
    p() -> buffs.death_from_above -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, oor_delay + timespan_t::from_millis( 50 ) );

    adjust_attack( player -> main_hand_attack, oor_delay );
    adjust_attack( player -> off_hand_attack, oor_delay );

    // DfA spell used to be implemented as a DoT where the finisher would
    // trigger on the first tick. This is no longer the case, but as a bandaid fix,
    // we're going to continue to model it as one, so force the DfA driver to
    // behave like a DoT.
    driver -> base_tick_time = oor_delay;
    driver -> dot_duration = oor_delay;

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

    p() -> trigger_sephuzs_secret( execute_state, MECHANIC_STUN );
  }
};

// ==========================================================================
// Poisoned Knife
// ==========================================================================

struct poisoned_knife_t : public rogue_attack_t
{
  poisoned_knife_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "poisoned_knife", p, p -> find_specialization_spell( "Poisoned Knife" ), options_str )
  { }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = rogue_attack_t::bonus_da( s );

    b += p() -> buffs.sharpened_blades -> stack_value();

    return b;
  }

  bool procs_insignia_of_ravenholdt() const override
  { return false; }

  double composite_poison_flat_modifier( const action_state_t* ) const override
  { return 1.0; }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.sharpened_blades -> expire();
  }
};

// ==========================================================================
// Cancel AutoAttack
// ==========================================================================

struct cancel_autoattack_t : public action_t
{
  rogue_t* rogue;
  cancel_autoattack_t( rogue_t* rogue_, const std::string& options_str ) :
    action_t( ACTION_OTHER, "cancel_autoattack", rogue_ ),
    rogue( rogue_ )
  {
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
  }

  result_e calculate_result( action_state_t* ) const override
  { return RESULT_HIT; }

  block_result_e calculate_block_result( action_state_t* ) const override
  { return BLOCK_RESULT_UNBLOCKED; }

  void execute() override
  {
    action_t::execute();

    // Cancel the scheduled AA
    if ( rogue -> main_hand_attack && rogue -> main_hand_attack -> execute_event )
      event_t::cancel( rogue -> main_hand_attack -> execute_event );

    if ( rogue -> off_hand_attack && rogue -> off_hand_attack -> execute_event )
      event_t::cancel( rogue -> off_hand_attack -> execute_event );
  }

  bool ready() override
  {
    if ( ( rogue -> main_hand_attack && rogue -> main_hand_attack -> execute_event ) ||
         ( rogue -> off_hand_attack && rogue -> off_hand_attack -> execute_event ) )
    {
      return action_t::ready();
    }

    return false;
  }
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

// rogue_attack_t::create_expression =========================================

expr_t* actions::rogue_attack_t::create_expression( const std::string& name_str )
{
  if ( util::str_compare_ci( name_str, "cp_gain" ) )
  {
    return make_mem_fn_expr( "cp_gain", *this, &rogue_attack_t::generate_cp );
  }
  // Garrote and Rupture and APL lines using "exsanguinated"
  // TODO: Add Internal Bleeding (not the same id as Kidney Shot)
  else if ( util::str_compare_ci( name_str, "exsanguinated" ) &&
            ( data().id() == 703 || data().id() == 1943 ) )
  {
    return new exsanguinated_expr_t( this );
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

      secondary_weapon_uptime = make_buff( rogue, "secondary_weapon" + prefix );
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

namespace buffs {
// ==========================================================================
// Buffs
// ==========================================================================

struct proxy_garrote_t : public buff_t
{
  proxy_garrote_t( rogue_td_t& r ) :
    buff_t( r, "garrote", r.source -> find_specialization_spell( "Garrote" ) )
  {
    set_quiet( true );
    set_cooldown( timespan_t::zero() );
  }

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

struct adrenaline_rush_t : public haste_buff_t
{
  rogue_t* r;

  adrenaline_rush_t( rogue_t* p ) :
    haste_buff_t( p, "adrenaline_rush", p -> find_class_spell( "Adrenaline Rush" ) ),
    r( p )
  { 
    set_cooldown( timespan_t::zero() );
    set_default_value( p -> find_class_spell( "Adrenaline Rush" ) -> effectN( 2 ).percent() );
    set_affects_regen( true );
    add_invalidate( CACHE_ATTACK_SPEED );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( r -> sets -> has_set_bonus( ROGUE_OUTLAW, T20, B4) )
    {
      r -> buffs.t20_4pc_outlaw -> trigger();
    }
  }
};

struct subterfuge_t : public buff_t
{
  rogue_t* rogue;

  subterfuge_t( rogue_t* r ) :
    buff_t( r, "subterfuge", r -> find_spell( 115192 ) ),
    rogue( r )
  { }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    actions::break_stealth( rogue );
  }
};

struct stealth_like_buff_t : public buff_t
{
  rogue_t* rogue;
  bool procs_mantle_of_the_master_assassin;

  stealth_like_buff_t( rogue_t* r, const std::string& name, const spell_data_t* spell ) :
    buff_t( r, name, spell ), rogue( r ), procs_mantle_of_the_master_assassin ( true )
  { }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    if ( rogue -> in_combat && rogue -> talent.master_of_shadows -> ok() )
    {
      rogue -> buffs.master_of_shadows -> trigger();
      rogue -> resource_gain( RESOURCE_ENERGY, rogue -> buffs.master_of_shadows -> data().effectN( 2 ).base_value(),
                              rogue -> gains.master_of_shadows );
    }

    if ( rogue->talent.master_assassin->ok() )
    {
        rogue->buffs.master_assassin->expire();
        rogue->buffs.master_assassin_aura->trigger();
    }

    if ( procs_mantle_of_the_master_assassin &&
         rogue -> legendary.mantle_of_the_master_assassin )
    {
      rogue -> buffs.mantle_of_the_master_assassin -> expire();
      rogue -> buffs.mantle_of_the_master_assassin_aura -> trigger();
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( procs_mantle_of_the_master_assassin &&
         rogue -> legendary.mantle_of_the_master_assassin )
    {
      rogue -> buffs.mantle_of_the_master_assassin_aura -> expire();
      rogue -> buffs.mantle_of_the_master_assassin -> trigger();
    }

    if ( rogue->talent.master_assassin->ok() )
    {
      rogue->buffs.master_assassin_aura->expire();
      rogue->buffs.master_assassin->trigger();
    }
  }
};

// Note, stealth buff is set a max time of half the nominal fight duration, so it can be
// forced to show in sample sequence tables.
struct stealth_t : public stealth_like_buff_t
{
  stealth_t( rogue_t* r ) :
    stealth_like_buff_t( r, "stealth", r -> find_spell( 1784 ) )
  {
    buff_duration = sim -> max_time / 2;
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    if ( rogue -> in_combat && rogue -> talent.master_of_shadows -> ok() &&
         // As of 04/08/2017, it does not proc Master of Shadows talent if Stealth is procced from Vanish
         // (that's why we also proc Stealth before Vanish expires).
         // As of 04/20/2017 on 7.2.5 PTR, this hold also true for the new Master of Shadows talent.
      ( !rogue -> bugs || !rogue -> buffs.vanish -> check() ) )
    {
      rogue -> buffs.master_of_shadows -> trigger();
    }

    if ( rogue->talent.master_assassin->ok() )
    {
        rogue->buffs.master_assassin->expire();
        rogue->buffs.master_assassin_aura->trigger();
    }

    if ( procs_mantle_of_the_master_assassin &&
         rogue -> legendary.mantle_of_the_master_assassin )
    {
      rogue -> buffs.mantle_of_the_master_assassin -> expire();
      rogue -> buffs.mantle_of_the_master_assassin_aura -> trigger();
    }
  }
};

// Vanish now acts like "stealth like abilities".
struct vanish_t : public stealth_like_buff_t
{
  vanish_t( rogue_t* r ) :
    stealth_like_buff_t( r, "vanish", r -> find_spell( 11327 ) )
  { }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    // Stealth proc if Vanish fully end (i.e. isn't break before the expiration)
    // We do it before the normal Vanish expiration to avoid on-stealth buff bugs (MoS, MoSh, Mantle).
    if ( remaining_duration == timespan_t::zero() )
    {
      rogue -> buffs.stealth -> trigger();
    }

    stealth_like_buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// Shadow dance acts like "stealth like abilities" except for Mantle of the Master
// Assassin legendary.
struct shadow_dance_t : public stealth_like_buff_t
{
  shadow_dance_t( rogue_t* p ) :
    stealth_like_buff_t( p, "shadow_dance", p -> spec.shadow_dance )
  {
    buff_duration += p -> talent.subterfuge -> effectN( 2 ).time_value();
    procs_mantle_of_the_master_assassin = false;

    if ( p -> talent.dark_shadow -> ok() )
    {
      add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    }
  }
};

struct rogue_poison_buff_t : public buff_t
{
  rogue_poison_buff_t( rogue_td_t& r, const std::string& name, const spell_data_t* spell ) :
    buff_t( r, name, spell )
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

struct marked_for_death_debuff_t : public buff_t
{
  cooldown_t* mod_cd;

  marked_for_death_debuff_t( rogue_td_t& r ) :
    buff_t( r, "marked_for_death", r.source -> find_talent_spell( "Marked for Death" ) ),
    mod_cd( r.source -> get_cooldown( "marked_for_death" ) )
  {
    set_cooldown( timespan_t::zero() );
  }

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

    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

struct roll_the_bones_t : public buff_t
{
  rogue_t* rogue;
  std::array<buff_t*, 6> buffs;

  roll_the_bones_t( rogue_t* r ) :
    buff_t( r, "roll_the_bones", r -> spec.roll_the_bones ),
    rogue( r )
  {
    set_period( timespan_t::zero() ); // Disable ticking
    set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

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

  std::vector<buff_t*> random_roll()
  {
    std::vector<buff_t*> rolled;

    if ( rogue -> fixed_rtb_odds.empty() )
    {
      // RtB uses hardcoded probabilities since 7.2.5
      // As of 2017-05-18 assume these:
      // -- The current proposal is to reduce the number of dice being rolled from 6 to 5
      // -- and to hand-set probabilities to something like 79% chance for 1-buff, 20% chance
      // -- for 2-buffs, and 1% chance for 5-buffs (yahtzee), bringing the expected value of
      // -- a roll down to 1.24 buffs (plus additional value for synergies between buffs).
      // Source: https://us.battle.net/forums/en/wow/topic/20753815486?page=2#post-21
      rogue -> fixed_rtb_odds = { 79.0, 20.0, 0.0, 0.0, 1.0, 0.0 };
    }

    if ( ! rogue -> fixed_rtb_odds.empty() )
    {
      double roll = rng().range( 0.0, 100.0 );
      size_t num_buffs = 0;
      double aggregate = 0.0;
      for ( const double& chance : rogue -> fixed_rtb_odds )
      {
        aggregate += chance;
        num_buffs++;
        if ( roll < aggregate )
        {
          break;
        }
      }

      std::vector<unsigned> pool = { 0, 1, 2, 3, 4, 5 };
      for ( size_t i = 0; i < num_buffs; i++ )
      {
        unsigned buff = (unsigned)rng().range( 0, (double)pool.size() );
        auto buff_idx = pool[ buff ];
        rolled.push_back( buffs[ buff_idx ] );
        pool.erase( pool.begin() + buff );
      }
    }

    return rolled;
  }

  std::vector<buff_t*> fixed_roll()
  {
    std::vector<buff_t*> rolled;
    range::for_each( rogue -> fixed_rtb, [this, &rolled]( size_t idx )
    { rolled.push_back( buffs[ idx ] ); } );
    return rolled;
  }

  unsigned roll_the_bones( timespan_t duration )
  {
    std::vector<buff_t*> rolled;
    if ( rogue -> fixed_rtb.size() == 0 )
    {
      do
      {
        rolled = random_roll();
      }
      while ( rogue -> buffs.loaded_dice -> up() && rolled.size() < 2 );
    }
    else
    {
      rolled = fixed_roll();
    }

    for ( size_t i = 0; i < rolled.size(); ++i )
    {
      rolled[ i ] -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
    }

    return as<unsigned>( rolled.size() );
  }

  void trigger_inactive_buff( timespan_t duration )
  {
    std::vector<buff_t*> inactive_buffs;
    for ( buff_t* buff : buffs )
    {
      if ( ! buff -> check() )
        inactive_buffs.push_back( buff );
    }
    if ( inactive_buffs.empty() )
      return;
    unsigned add_idx = (unsigned)rng().range( 0, (double)inactive_buffs.size() );
    inactive_buffs[ add_idx ] -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    // For the T21 4pc bonus, let's collect our buffs with different duration, first.
    std::vector<timespan_t> t21_4pc_buff_remains;
    for ( buff_t* buff : buffs )
    {
      if ( buff -> check() && buff -> remains() != remains() )
      {
        t21_4pc_buff_remains.push_back( buff -> remains() );
      }
    }

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
      case 4:
        rogue -> procs.roll_the_bones_4 -> occur();
        break;
      case 5:
        rogue -> procs.roll_the_bones_5 -> occur();
        break;
      case 6:
        rogue -> procs.roll_the_bones_6 -> occur();
        break;
      default:
        assert( 0 );
    }

    if ( rogue -> buffs.loaded_dice -> check() )
        rogue -> buffs.loaded_dice -> expire();

    // For the T21 4pc bonus, trigger random inactive buffs with the remaining times.
    for ( timespan_t duration : t21_4pc_buff_remains )
    {
      trigger_inactive_buff( duration );
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    // Remove all secondary buffs when regular RtB expires. (see https://github.com/Ravenholdt-TC/Rogue/issues/84)
    expire_secondary_buffs();
  }
};

struct shadow_blades_t : public buff_t
{
  shadow_blades_t( rogue_t* p ) :
    buff_t( p, "shadow_blades", p -> find_specialization_spell( "Shadow Blades" ) )
  {
    set_duration( p -> find_specialization_spell( "Shadow Blades" ) -> duration() );
    set_cooldown( timespan_t::zero() );
  }

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

void rogue_t::trigger_main_gauche( const action_state_t* state )
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

  if ( ! rng().roll( mastery.main_gauche -> proc_chance() ) )
    return;

  active_main_gauche -> set_target( state -> target );
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

  double chance = spec.combat_potency -> effectN( 1 ).percent();
  // Looks like CP proc chance is normalized by weapon speed (i.e. penalty for using daggers)
  if ( state -> action != active_main_gauche )
    chance *= state -> action -> weapon -> swing_time.total_seconds() / 2.6;
  if ( ! rng().roll( chance ) )
    return;

  // energy gain value is in the proc trigger spell
  double gain = spec.combat_potency -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_ENERGY );

  resource_gain( RESOURCE_ENERGY, gain, gains.combat_potency, state -> action );
}

void rogue_t::trigger_energy_refund( const action_state_t* state )
{
  double energy_restored = state -> action -> last_resource_cost * 0.80;

  resource_gain( RESOURCE_ENERGY, energy_restored, gains.energy_refund );
}

void rogue_t::trigger_poison_bomb( const action_state_t* state )
{
  if ( ! talent.poison_bomb -> ok() || ! state -> action -> result_is_hit( state -> result ) )
    return;

  // They put 25 as value in spell data and divide it by 10 later, it's due to the int restriction.
  const actions::rogue_attack_state_t* s = cast_attack( state -> action ) -> cast_state( state );
  if ( rng().roll( talent.poison_bomb -> effectN( 1 ).percent() / 10 * s -> cp ) )
  {
    make_event<ground_aoe_event_t>( *sim, this, ground_aoe_params_t()
                                    .target( state -> target )
                                    .x( state -> target -> x_position)
                                    .y( state -> target -> y_position)
                                    .pulse_time( spell.poison_bomb_driver -> duration() / talent.poison_bomb -> effectN( 2 ).base_value() )
                                    .duration( spell.poison_bomb_driver -> duration() )
                                    .start_time( sim -> current_time() )
                                    .action( poison_bomb ));
  }
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
                 spec.venomous_wounds -> effectN( 2 ).base_value(),
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
  unsigned full_ticks_remaining = (unsigned)(td -> dots.rupture -> remains() / td -> dots.rupture -> current_action -> base_tick_time);
  int replenish = spec.venomous_wounds -> effectN( 2 ).base_value();

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

  if ( !state -> action -> result_is_hit( state -> result ) )
    return;

  if ( sim -> active_enemies == 1 )
    return;

  if ( state -> action -> is_aoe() )
    return;

  // Compute Blade Flurry modifier
  double multiplier = 1.0;
  if ( state -> action -> name_str == "killing_spree_mh" || state -> action -> name_str == "killing_spree_oh" )
  {
    multiplier = talent.killing_spree -> effectN( 2 ).percent();
  }
  else
  {
    multiplier = spec.blade_flurry -> effectN( 2 ).percent() + talent.dancing_steel -> effectN( 3 ).percent();
    if ( buffs.shivarran_symmetry -> check() )
    {
      multiplier += buffs.shivarran_symmetry -> data().effectN( 1 ).percent();
    }
  }

  // Note, unmitigated damage
  double damage = state -> result_total * multiplier;
  active_blade_flurry -> base_dd_min = damage;
  active_blade_flurry -> base_dd_max = damage;
  active_blade_flurry -> set_target( state->target );
  active_blade_flurry -> execute();
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
  int cp_gain = 0;
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
  if ( !spec.deepening_shadows -> ok() )
  {
    return;
  }

  actions::rogue_attack_t* attack = rogue_t::cast_attack( state -> action );
  if ( !attack -> affected_by.deepening_shadows )
  {
    return;
  }

  const actions::rogue_attack_state_t* s = actions::rogue_attack_t::cast_state( state );
  if ( s -> cp == 0 )
  {
    return;
  }

  timespan_t adjustment;

    // Note: this changed to be 10 * seconds as of 2017-04-19
  int cdr = spec.deepening_shadows -> effectN( 1 ).base_value();
  if ( talent.enveloping_shadows -> ok() )
  {
    cdr += talent.enveloping_shadows -> effectN( 1 ).base_value();
  }
  adjustment = timespan_t::from_seconds( -0.1 * cdr * s -> cp );

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
  if (sim -> debug) sim -> out_debug.printf( "Melee attack landed, so shadow techniques increment from %d to %d", shadow_techniques, shadow_techniques+1);

  if ( ++shadow_techniques == 5 || ( shadow_techniques == 4 && rng().roll( 0.5 ) ) )
  {
    if (sim -> debug) sim -> out_debug.printf( "Shadow techniques proc'd at %d", shadow_techniques);
    trigger_combo_point_gain( spec.shadow_techniques_effect -> effectN( 1 ).base_value(), gains.shadow_techniques, state -> action );
    resource_gain( RESOURCE_ENERGY, spec.shadow_techniques_effect -> effectN( 2 ).base_value(), gains.shadow_techniques, state -> action );
    if (sim -> debug) sim -> out_debug.printf( "Resetting shadow_techniques counter to zero.");
    shadow_techniques = 0;
  }
}

void rogue_t::trigger_weaponmaster( const action_state_t* s )
{
  if ( ! talent.weaponmaster -> ok() )
  {
    return;
  }

  //actions::rogue_attack_t* attack = cast_attack( s -> action );
  if ( ! s -> action -> result_is_hit( s -> result ) )
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
  cooldowns.weaponmaster -> start( talent.weaponmaster -> internal_cooldown() );

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s procs weaponmaster for %s", name(), s -> action -> name() );
  }

  // Direct damage re-computes on execute
  make_event<actions::secondary_ability_trigger_t>( *sim, s -> target, s -> action,
      actions::rogue_attack_t::cast_state( s ) -> cp, TRIGGER_WEAPONMASTER );
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

  // Abilities
  cooldowns.adrenaline_rush -> adjust( v, false );
  cooldowns.between_the_eyes -> adjust( v, false );
  cooldowns.sprint -> adjust( v, false );
  cooldowns.vanish -> adjust( v, false );
  // Talents
  cooldowns.grappling_hook -> adjust( v, false );
  cooldowns.killing_spree -> adjust( v, false );
  cooldowns.marked_for_death -> adjust( v, false );
  cooldowns.death_from_above -> adjust( v, false );
}

void rogue_t::trigger_restless_blades( const action_state_t* state )
{
  timespan_t v = timespan_t::from_seconds( spec.restless_blades -> effectN( 1 ).base_value() / 10.0 );
  v *= - actions::rogue_attack_t::cast_state( state ) -> cp;

  // Abilities
  cooldowns.adrenaline_rush -> adjust( v, false );
  cooldowns.between_the_eyes -> adjust( v, false );
  cooldowns.grappling_hook -> adjust( v, false );
  cooldowns.sprint -> adjust( v, false );
  cooldowns.vanish -> adjust( v, false );
  // Talents
  cooldowns.blade_rush -> adjust( v, false );
  cooldowns.ghostly_strike -> adjust( v, false );
  cooldowns.killing_spree -> adjust( v, false );
  cooldowns.marked_for_death -> adjust( v, false );
}

void do_exsanguinate( dot_t* dot, double coeff )
{
  if ( ! dot -> is_ticking() )
  {
    return;
  }

  // Original and logical implementation. Since the advent of hasted bleed exsanguinate works differently though.
  // dot -> adjust( coeff );
  dot -> exsanguinate( coeff );

  actions::rogue_attack_t::cast_state( dot -> state ) -> exsanguinated = true;
}

void rogue_t::trigger_exsanguinate( const action_state_t* state )
{
  rogue_td_t* td = get_target_data( state -> target );

  double coeff = 1.0 / ( 1.0 + talent.exsanguinate -> effectN( 1 ).percent() );

  do_exsanguinate( td -> dots.garrote, coeff );
  do_exsanguinate( td -> dots.internal_bleeding, coeff );
  do_exsanguinate( td -> dots.rupture, coeff );
  do_exsanguinate( td -> dots.crimson_tempest, coeff );
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

  double grant_energy = 0;

  grant_energy += rogue_attack_t::cast_state( state ) -> cp * spell.relentless_strikes_energize -> effectN( 1 ).resource( RESOURCE_ENERGY );

  if ( grant_energy > 0 )
  {
    resource_gain( RESOURCE_ENERGY, grant_energy, gains.relentless_strikes, state -> action );
  }
}

void rogue_t::trigger_t19oh_8pc( const action_state_t* state )
{
  if ( ! sets -> has_set_bonus( specialization(), T19OH, B8 ) )
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

  double max_spend = std::min( resources.current[ RESOURCE_COMBO_POINT ], consume_cp_max() );

  if ( legendary.denial_of_the_halfgiants && buffs.shadow_blades -> up() )
  {
    // Shadow Blades duration extends is capped at the initial Shadow Blades duration
    timespan_t adjustment_base = timespan_t::from_seconds( max_spend * legendary.denial_of_the_halfgiants -> effectN( 1 ).base_value() / 10.0 );
    timespan_t adjustment_max = buffs.shadow_blades -> buff_duration - buffs.shadow_blades -> remains();
    timespan_t adjustment = std::min( adjustment_base, adjustment_max );
    buffs.shadow_blades -> extend_duration( this, adjustment );
  }

  if ( sets -> has_set_bonus( ROGUE_SUBTLETY, T21, B2 ) )
  {
    timespan_t v = timespan_t::from_seconds( sets -> set( ROGUE_SUBTLETY, T21, B2 ) -> effectN( 1 ).base_value() / 10.0 );
    v *= -max_spend;
    cooldowns.symbols_of_death -> adjust( v, false );
  }

  state -> action -> stats -> consume_resource( RESOURCE_COMBO_POINT, max_spend );
  resource_loss( RESOURCE_COMBO_POINT, max_spend, nullptr, state ? state -> action : nullptr );

  if ( sim -> log )
    sim -> out_log.printf( "%s consumes %.1f %s for %s (%.0f)", name(),
                   max_spend, util::resource_type_string( RESOURCE_COMBO_POINT ),
                   state -> action -> name(), resources.current[ RESOURCE_COMBO_POINT ] );

  if ( buffs.t21_4pc_subtlety -> up() )
  {
    trigger_combo_point_gain( static_cast<int>(max_spend), gains.t21_4pc_subtlety, state -> action );
    buffs.t21_4pc_subtlety -> expire();
  }
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

  // 1/15/2018 - Insignia only uses base damage and no target multipliers affect it.
  // This was previously handled by reversing the Nightblade and Vendetta in insignia_of_ravenholdt_attack_t
  // However, after testing this also appears to apply to Toxic Blade (w/ Kingsbane) and Ghostly Strike as well.
  // As such, we can just reverse the entire target multiplier when we snapshot the damage.
  amount /= state -> target_da_multiplier;

  if ( state -> action -> get_school() == SCHOOL_PHYSICAL )
  {
    // As of 2017-07-27, Insignia of Ravenholdt does 104.38% in boss fight logs and on the Raid dummy.
    // This only applies to physical trigger attacks and seems to scale with enemy armor mitigation.
    // However, I am unable to find out any relation or formula behind it.
    // Observations compared to ideal 15% by enemy level:
    // - Boss logs, Boss adds, Raid dummy: 104.38%
    // - Dungeon dummy: 103.32%
    // - Level 112 (Dungeon Boss): 69.95%
    // - Level 111: 69.23%
    // - Level 110: 68.52%
    // I will assume we have a raid boss fight and apply those 4.38%, for now.
    // Contact me if you are able to find out more. ~Mystler
    amount *= 1.0438;
  }

  insignia_of_ravenholdt_ -> base_dd_min = amount;
  insignia_of_ravenholdt_ -> base_dd_max = amount;
  insignia_of_ravenholdt_ -> set_target( state -> target );
  insignia_of_ravenholdt_ -> execute();
}

// Proudly copy pasta'd from the Shaman module, whee
void rogue_t::trigger_sephuzs_secret( const action_state_t* state,
                                      spell_mechanic        mechanic,
                                      double                override_proc_chance )
{
  switch ( mechanic )
  {
    // Interrupts will always trigger sephuz
    case MECHANIC_INTERRUPT:
      break;
    default:
      // By default, proc sephuz on persistent enemies if they are below the "boss level"
      // (playerlevel + 3), and on any kind of transient adds.
      if ( state -> target -> type != ENEMY_ADD &&
           ( state -> target -> level() >= sim -> max_player_level + 3 ) )
      {
        return;
      }
      break;
  }

  if ( legendary.sephuzs_secret )
  {
    buffs.sephuzs_secret -> trigger( 1, buff_t::DEFAULT_VALUE(), override_proc_chance );
  }
}

void rogue_t::trigger_t21_4pc_assassination( const action_state_t* state )
{
  if ( state -> result != RESULT_CRIT || ! sets -> has_set_bonus( ROGUE_ASSASSINATION, T21, B4 ) )
    return;

  if ( rng().roll( sets -> set( ROGUE_ASSASSINATION, T21, B4 ) -> proc_chance() ) )
  {
    resource_gain( RESOURCE_ENERGY, sets -> set( ROGUE_ASSASSINATION, T21, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).base_value(), gains.t21_4pc_assassination, state -> action );
  }
}

void rogue_t::trigger_t21_4pc_outlaw( const action_state_t* state )
{
  if ( ! state -> action -> result_is_hit( state -> result ) || ! sets -> has_set_bonus( ROGUE_OUTLAW, T21, B4 ) )
    return;

  if ( rng().roll( sets -> set( ROGUE_OUTLAW, T21, B4 ) -> effectN( 2 ).percent() ) )
  {
    debug_cast<buffs::roll_the_bones_t*>( buffs.roll_the_bones ) -> trigger_inactive_buff( timespan_t::from_seconds( sets -> set( ROGUE_OUTLAW, T21, B4 ) -> effectN( 3 ).base_value() ) );
    procs.t21_4pc_outlaw -> occur();
  }
}

void rogue_t::trigger_t21_4pc_subtlety( const action_state_t* state )
{
  if ( ! state -> action -> result_is_hit( state -> result ) || ! sets -> has_set_bonus( ROGUE_SUBTLETY, T21, B4 ) )
    return;

  if ( rng().roll( sets -> set( ROGUE_SUBTLETY, T21, B4 ) -> proc_chance() ) )
  {
    buffs.t21_4pc_subtlety -> trigger();
  }
}

// ==========================================================================
// Rogue Targetdata Definitions
// ==========================================================================

rogue_td_t::rogue_td_t( player_t* target, rogue_t* source ) :
  actor_target_data_t( target, source ),
  dots( dots_t() ),
  debuffs( debuffs_t() )
{

  dots.deadly_poison      = target -> get_dot( "deadly_poison_dot", source );
  dots.garrote            = target -> get_dot( "garrote", source );
  dots.internal_bleeding  = target -> get_dot( "internal_bleeding", source );
  dots.mutilated_flesh    = target -> get_dot( "mutilated_flesh", source );
  dots.rupture            = target -> get_dot( "rupture", source );
  dots.crimson_tempest    = target -> get_dot( "crimson_tempest", source );

  dots.nightblade         = target -> get_dot( "nightblade", source );


  debuffs.marked_for_death = new buffs::marked_for_death_debuff_t( *this );

  debuffs.wound_poison = new buffs::wound_poison_t( *this );
  debuffs.crippling_poison = new buffs::crippling_poison_t( *this );
  debuffs.leeching_poison = new buffs::leeching_poison_t( *this );
  debuffs.garrote = new buffs::proxy_garrote_t( *this );
  debuffs.vendetta = make_buff( *this, "vendetta", source->spec.vendetta )
    -> set_cooldown( timespan_t::zero() )
    -> set_default_value( source->spec.vendetta->effectN( 1 ).percent() );
  debuffs.toxic_blade = make_buff( *this, "toxic_blade", source -> talent.toxic_blade -> effectN( 4 ).trigger() )
    -> set_default_value( source -> talent.toxic_blade -> effectN( 4 ).trigger() -> effectN( 1 ).percent() );
  debuffs.ghostly_strike = make_buff( *this, "ghostly_strike", source -> talent.ghostly_strike )
    -> set_default_value( source -> talent.ghostly_strike -> effectN( 3 ).percent() );
  const spell_data_t* ew_debuff = source -> talent.expose_weakness -> effectN( 1 ).trigger();
  debuffs.expose_weakness = make_buff( *this, "expose_weakness", ew_debuff )
    -> set_default_value( ew_debuff -> effectN( 1 ).percent() );

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
    h *= 1.0 / ( 1.0 + talent.slice_and_dice -> effectN( 1 ).percent() * buffs.slice_and_dice -> value() );

  if ( buffs.adrenaline_rush -> check() )
    h *= 1.0 / ( 1.0 + buffs.adrenaline_rush -> value() );

  if ( buffs.t20_4pc_outlaw -> check() )
    h *= 1.0 / ( 1.0 + buffs.t20_4pc_outlaw -> value() );

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

  if ( buffs.sephuzs_secret -> check() )
  {
    h *= 1.0 / (1.0 + buffs.sephuzs_secret -> stack_value() );
  }

  if ( legendary.sephuzs_secret )
  {
    h *= 1.0 / ( 1.0 + legendary.sephuzs_secret -> effectN( 3 ).percent() );
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

  crit += buffs.mantle_of_the_master_assassin_aura -> stack_value(); // 7.1.5 Legendary

  crit += buffs.t20_2pc_outlaw -> stack_value();

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

  if ( buffs.sephuzs_secret -> check() )
  {
    h *= 1.0 / (1.0 + buffs.sephuzs_secret -> stack_value() );
  }

  if ( legendary.sephuzs_secret )
  {
    h *= 1.0 / ( 1.0 + legendary.sephuzs_secret -> effectN( 3 ).percent() );
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

  crit += buffs.mantle_of_the_master_assassin_aura -> stack_value(); // 7.1.5 Legendary

  crit += buffs.t20_2pc_outlaw -> stack_value();

  return crit;
}

// rogue_t::matching_gear_multiplier ========================================

double rogue_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0.0;
}

// rogue_t::composite_player_target_multiplier ==============================

double rogue_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  rogue_td_t* tdata = get_target_data( target );

  m *= 1.0 + tdata -> debuffs.ghostly_strike -> stack_value();

  return m;
}

// rogue_t::default_flask ===================================================

std::string rogue_t::default_flask() const
{
  return ( true_level >  100 ) ? "seventh_demon" :
         ( true_level >= 90  ) ? "greater_draenic_agility_flask" :
         ( true_level >= 85  ) ? "spring_blossoms" :
         ( true_level >= 80  ) ? "winds" :
         "disabled";
}

// rogue_t::default_potion ==================================================

std::string rogue_t::default_potion() const
{
  return ( true_level > 100 ) ? "prolonged_power" :
         ( true_level >= 90 ) ? "draenic_agility" :
         ( true_level >= 85 ) ? "virmens_bite" :
         ( true_level >= 80 ) ? "tolvir" :
         "disabled";
}

// rogue_t::default_food ====================================================

std::string rogue_t::default_food() const
{
  return ( true_level >  100 ) ? "lavish_suramar_feast" :
         ( true_level >  90  ) ? "jumbo_sea_dog" :
         ( true_level >= 90  ) ? "sea_mist_rice_noodles" :
         ( true_level >= 80  ) ? "seafood_magnifique_feast" :
         "disabled";
}

std::string rogue_t::default_rune() const
{
  return ( true_level >= 110 ) ? "defiled" :
         ( true_level >= 100 ) ? "hyper" :
         "disabled";
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

  std::vector<std::string> racial_actions = get_racial_actions();

  clear_action_priority_lists();

  // Flask
  precombat -> add_action( "flask" );

  // Rune
  precombat -> add_action( "augmentation" );

  // Food
  precombat -> add_action( "food" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  if ( specialization() == ROGUE_ASSASSINATION )
    precombat -> add_action( "apply_poison" );

  // Stealth before entering in combat
  if ( specialization() != ROGUE_SUBTLETY )
    precombat -> add_action( this, "Stealth" );

  // Potion
  if ( specialization() != ROGUE_SUBTLETY )
    precombat -> add_action( "potion" );

  // Potion
  std::string potion_action = "potion,if=buff.bloodlust.react|target.time_to_die<=60";
  if ( specialization() == ROGUE_ASSASSINATION )
    potion_action += "|debuff.vendetta.up&cooldown.vanish.remains<5";
  else if ( specialization() == ROGUE_OUTLAW )
    potion_action += "|buff.adrenaline_rush.up";
  else if ( specialization() == ROGUE_SUBTLETY )
    potion_action += "|(buff.vanish.up&(buff.shadow_blades.up|cooldown.shadow_blades.remains<=30))";
    

  if ( specialization() != ROGUE_SUBTLETY )
    precombat -> add_talent( this, "Marked for Death", "if=raid_event.adds.in>40" );

  if ( specialization() == ROGUE_ASSASSINATION )
  {
    def -> add_action( "variable,name=energy_regen_combined,value=energy.regen+poisoned_bleeds*(7)%2" );
    def -> add_action( "variable,name=energy_time_to_max_combined,value=energy.deficit%variable.energy_regen_combined" );
    def -> add_action( "call_action_list,name=cds" );
    def -> add_action( "run_action_list,name=stealthed,if=stealthed.rogue" );
    def -> add_action( "run_action_list,name=aoe,if=spell_targets.fan_of_knives>2" );
    def -> add_action( "call_action_list,name=maintain" );
    def -> add_action( "call_action_list,name=finish,if=(!talent.exsanguinate.enabled|cooldown.exsanguinate.remains>2)" );
    def -> add_action( "call_action_list,name=build,if=combo_points.deficit>1|energy.deficit<=25+variable.energy_regen_combined" );
    def -> add_action( "arcane_pulse");

    // Cooldowns
    action_priority_list_t* cds = get_action_priority_list( "cds", "Cooldowns" );
    cds -> add_action( potion_action );
    for ( size_t i = 0; i < items.size(); i++ )
    {
      if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
      {
        if ( items[i].name_str == "draught_of_souls" )
        {
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=energy.deficit>=35+variable.energy_regen_combined*2&(!equipped.mantle_of_the_master_assassin|cooldown.vanish.remains>8)" );
        }
        // TODO: A bit before Vanish with Mantle, whenever up w/o.
        else if ( items[i].name_str == "umbral_moonglaives" )
          cds -> add_action( "use_item,name=" + items[i].name_str );
        else
          cds -> add_action( "use_item,name=" + items[i].name_str );
      }
    }
    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "arcane_torrent" )
        cds -> add_action( racial_actions[i] + ",if=!buff.envenom.up&energy.deficit>=15+variable.energy_regen_combined*gcd.remains*1.1" );
      else if ( racial_actions[i] == "arcane_pulse" )
        continue; // Manually added
      else
        cds -> add_action( racial_actions[i] + ",if=debuff.vendetta.up" );
    }
    cds -> add_talent( this, "Marked for Death", "target_if=min:target.time_to_die,if=target.time_to_die<combo_points.deficit*1.5|(raid_event.adds.in>40&combo_points.deficit>=cp_max_spend)" );
    cds -> add_action( this, "Vendetta", "if=!talent.exsanguinate.enabled|dot.rupture.ticking" );
    cds -> add_talent( this, "Exsanguinate", "if=!set_bonus.tier20_4pc&(prev_gcd.1.rupture&dot.rupture.remains>4+4*cp_max_spend&!stealthed.rogue|dot.garrote.pmultiplier>1&!cooldown.vanish.up&buff.subterfuge.up)" );
    cds -> add_talent( this, "Exsanguinate", "if=set_bonus.tier20_4pc&dot.garrote.remains>20&dot.rupture.remains>4+4*cp_max_spend" );
    cds -> add_action( this, "Vanish", "if=talent.nightstalker.enabled&!talent.exsanguinate.enabled&combo_points>=cp_max_spend&mantle_duration=0&debuff.vendetta.up" );
    cds -> add_action( this, "Vanish", "if=talent.nightstalker.enabled&talent.exsanguinate.enabled&combo_points>=cp_max_spend&mantle_duration=0&cooldown.exsanguinate.remains<1" );
    cds -> add_action( this, "Vanish", "if=talent.subterfuge.enabled&equipped.mantle_of_the_master_assassin&(debuff.vendetta.up|target.time_to_die<10)&mantle_duration=0" );
    cds -> add_action( this, "Vanish", "if=talent.subterfuge.enabled&!equipped.mantle_of_the_master_assassin&!stealthed.rogue&dot.garrote.refreshable&((spell_targets.fan_of_knives<=3&combo_points.deficit>=1+spell_targets.fan_of_knives)|(spell_targets.fan_of_knives>=4&combo_points.deficit>=4))" );
    cds -> add_action( this, "Vanish", "if=talent.shadow_focus.enabled&variable.energy_time_to_max_combined>=2&combo_points.deficit>=4" );
    cds -> add_talent( this, "Toxic Blade", "if=combo_points.deficit>=1+(mantle_duration>=0.2)&dot.rupture.remains>8&cooldown.vendetta.remains>10" );
    
    // Builders
    action_priority_list_t* build = get_action_priority_list( "build", "Builders" );
    build -> add_talent( this, "Blindside" );
    build -> add_action( this, "Fan of Knives", "if=buff.the_dreadlords_deceit.stack>=29|buff.hidden_blades.stack>=19" );
    build -> add_action( this, "Mutilate", "if=talent.exsanguinate.enabled&(debuff.vendetta.up|combo_points<=2)", "Mutilate is worth using over FoK for Exsanguinate builds in some 2T scenarios." );
    build -> add_action( this, "Fan of Knives", "if=spell_targets>1+equipped.insignia_of_ravenholdt" );
    build -> add_action( this, "Fan of Knives", "if=combo_points>=3+talent.deeper_stratagem.enabled&artifact.poison_knives.rank>=5|fok_rotation" );
    // We want to apply poison on the unit that have the most bleeds on and that meet the condition for Venomous Wound (and also for T19 dmg bonus).
    // This would be done with target_if=max:bleeds but it seems to be bugged atm
    build -> add_action( this, "Mutilate", "cycle_targets=1,if=dot.deadly_poison_dot.refreshable" );
    build -> add_action( this, "Mutilate" );

    // Stealth
    action_priority_list_t* stealthed = get_action_priority_list( "stealthed", "Stealthed" );
    stealthed->add_action( this, "Mutilate", "if=talent.shadow_focus.enabled&dot.garrote.ticking" );
    stealthed->add_action( this, "Garrote", "cycle_targets=1,if=talent.subterfuge.enabled&combo_points.deficit>=1&set_bonus.tier20_4pc&((dot.garrote.remains<=13&!debuff.toxic_blade.up)|pmultiplier<=1)&!exsanguinated" );
    stealthed->add_action( this, "Garrote", "cycle_targets=1,if=talent.subterfuge.enabled&combo_points.deficit>=1&!set_bonus.tier20_4pc&refreshable&(!exsanguinated|remains<=tick_time*2)&target.time_to_die-remains>2" );
    stealthed->add_action( this, "Garrote", "cycle_targets=1,if=talent.subterfuge.enabled&combo_points.deficit>=1&!set_bonus.tier20_4pc&remains<=10&pmultiplier<=1&!exsanguinated&target.time_to_die-remains>2" );
    stealthed->add_action( this, "Rupture", "cycle_targets=1,if=combo_points>=4&refreshable&(pmultiplier<=1|remains<=tick_time)&(!exsanguinated|remains<=tick_time*2)&target.time_to_die-remains>6" );
    stealthed->add_action( this, "Rupture", "if=talent.exsanguinate.enabled&talent.nightstalker.enabled&target.time_to_die-remains>6" );
    stealthed->add_action( this, "Envenom", "if=combo_points>=cp_max_spend" );
    stealthed->add_action( this, "Garrote", "if=!talent.subterfuge.enabled&target.time_to_die-remains>4" );
    stealthed->add_action( this, "Mutilate" );

    // Maintain
    action_priority_list_t* maintain = get_action_priority_list( "maintain", "Maintain" );
    maintain -> add_action( this, "Rupture", "if=talent.exsanguinate.enabled&((combo_points>=cp_max_spend&cooldown.exsanguinate.remains<1)|(!ticking&(time>10|combo_points>=2+artifact.urge_to_kill.enabled)))" );
    maintain -> add_action( this, "Rupture", "cycle_targets=1,if=combo_points>=4&refreshable&(pmultiplier<=1|remains<=tick_time)&(!exsanguinated|remains<=tick_time*2)&target.time_to_die-remains>6" );
    maintain -> add_action( "pool_resource,for_next=1" );
    maintain -> add_action( this, "Garrote", "cycle_targets=1,if=(!talent.subterfuge.enabled|!(cooldown.vanish.up&cooldown.vendetta.remains<=4))&combo_points.deficit>=1&refreshable&(pmultiplier<=1|remains<=tick_time)&(!exsanguinated|remains<=tick_time*2)&target.time_to_die-remains>4" );
    maintain -> add_action( this, "Garrote", "if=set_bonus.tier20_4pc&talent.exsanguinate.enabled&prev_gcd.1.rupture&cooldown.exsanguinate.remains<1&(!cooldown.vanish.up|time>12)" );
    maintain -> add_action( this, "Rupture", "if=!talent.exsanguinate.enabled&combo_points>=3&!ticking&mantle_duration=0&target.time_to_die>6" );

    // Finishers
    action_priority_list_t* finish = get_action_priority_list( "finish", "Finishers" );
    finish -> add_talent( this, "Crimson Tempest", "if=combo_points>=cp_max_spend" );
    finish -> add_action( this, "Envenom", "if=combo_points>=4+(talent.deeper_stratagem.enabled&!set_bonus.tier19_4pc)&(debuff.vendetta.up|debuff.toxic_blade.up|mantle_duration>=0.2|energy.deficit<=25+variable.energy_regen_combined)" );
    finish -> add_action( this, "Envenom", "if=talent.elaborate_planning.enabled&combo_points>=3+!talent.exsanguinate.enabled&buff.elaborate_planning.remains<0.2" );

    // AoE Rotation
    action_priority_list_t* aoe = get_action_priority_list( "aoe", "AoE" );
    aoe -> add_talent( this, "Crimson Tempest", "if=combo_points>=cp_max_spend" );
    aoe -> add_action( this, "Envenom", "if=!buff.envenom.up&combo_points>=cp_max_spend" );
    aoe -> add_action( this, "Rupture", "cycle_targets=1,if=combo_points>=cp_max_spend&refreshable&(pmultiplier<=1|remains<=tick_time)&(!exsanguinated|remains<=tick_time*2)&target.time_to_die-remains>4" );
    aoe -> add_action( this, "Envenom", "if=combo_points>=cp_max_spend" );
    aoe -> add_action( this, "Fan of Knives" );
  }
  else if ( specialization() == ROGUE_OUTLAW )
  {
    // Pre-Combat
    precombat -> add_action( this, "Roll the Bones", "if=!talent.slice_and_dice.enabled" );

    // Main Rotation
    def -> add_action( "variable,name=rtb_reroll,value=!talent.slice_and_dice.enabled&buff.loaded_dice.up&(rtb_buffs<2|(rtb_buffs<4&!buff.true_bearing.up))", "Reroll when Loaded Dice is up and if you have less than 2 buffs or less than 4 and no True Bearing. With SnD, consider that we never have to reroll." );
    def -> add_action( "variable,name=ss_useable_noreroll,value=(combo_points<5+talent.deeper_stratagem.enabled)", "Condition to use Sinister Strike when not rerolling RtB or when using SnD" );
    def -> add_action( "variable,name=ss_useable,value=variable.rtb_reroll&combo_points<5+talent.deeper_stratagem.enabled|!variable.rtb_reroll&variable.ss_useable_noreroll", "Condition to use Sinister Strike, when you have RtB or not" );
    def -> add_action( "call_action_list,name=bf", "Normal rotation" );
    def -> add_action( "call_action_list,name=cds" );
    def -> add_action( "call_action_list,name=stealth,if=stealthed.rogue|cooldown.vanish.up|cooldown.shadowmeld.up", "Conditions are here to avoid worthless check if nothing is available" );
    def -> add_action( this, "Sprint", "if=equipped.thraxis_tricksy_treads&buff.death_from_above.up&buff.death_from_above.remains<=0.15" );
    def -> add_action( this, "Adrenaline Rush", "if=buff.death_from_above.up&buff.death_from_above.remains<=0.15" );
      // Pandemic is (6 + 6 * CP) * 0.3, ie (1 + CP) * 1.8
    def -> add_talent( this, "Slice and Dice", "if=!variable.ss_useable&buff.slice_and_dice.remains<target.time_to_die&buff.slice_and_dice.remains<(1+combo_points)*1.8" );
      // Reroll unless 3+ buffs or true bearing
    def -> add_action( this, "Roll the Bones", "if=!variable.ss_useable&(target.time_to_die>20|buff.roll_the_bones.remains<target.time_to_die)&(buff.roll_the_bones.remains<=3|variable.rtb_reroll)" );
    def -> add_talent( this, "Killing Spree", "if=energy.time_to_max>5|energy<15" );
    def -> add_talent( this, "Blade Rush" );
    def -> add_action( "call_action_list,name=build" );
    def -> add_action( "call_action_list,name=finish,if=!variable.ss_useable" );
    def -> add_action( this, "Gouge", "if=talent.dirty_tricks.enabled&combo_points.deficit>=1", "Gouge is used as a CP Generator while nothing else is available and you have Dirty Tricks talent. It's unlikely that you'll be able to do this optimally in-game since it requires to move in front of the target, but it's here so you can quantifiy its value." );
    def -> add_action( "arcane_pulse" );

    // Builders
    action_priority_list_t* build = get_action_priority_list( "build", "Builders" );
    build -> add_talent( this, "Ghostly Strike", "if=combo_points.deficit>=1+buff.broadsides.up&refreshable" );
    build -> add_action( this, "Pistol Shot", "if=combo_points.deficit>=1+buff.broadsides.up+talent.quick_draw.enabled&buff.opportunity.up&(energy.time_to_max>2-talent.quick_draw.enabled|(buff.greenskins_waterlogged_wristcuffs.up&buff.greenskins_waterlogged_wristcuffs.remains<2))" );
    build -> add_action( this, "Sinister Strike", "if=variable.ss_useable" );

    // Blade Flurry
    action_priority_list_t* bf = get_action_priority_list( "bf", "Blade Flurry" );
      // Cancels Blade Flurry buff on CD to maximize Shiarran Symmetry effect
    bf -> add_action( "cancel_buff,name=blade_flurry,if=equipped.shivarran_symmetry&cooldown.blade_flurry.charges>=1&buff.blade_flurry.up&spell_targets.blade_flurry>=2" );
    bf -> add_action( this, "Blade Flurry", "if=spell_targets.blade_flurry>=2&!buff.blade_flurry.up" );

    // Cooldowns
    action_priority_list_t* cds = get_action_priority_list( "cds", "Cooldowns" );
    cds -> add_action( potion_action );
    for ( size_t i = 0; i < items.size(); i++ )
    {
      if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
      {
        if ( items[i].name_str == "specter_of_betrayal" )
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=(mantle_duration>0|buff.curse_of_the_dreadblades.up|(cooldown.vanish.remains>11&cooldown.curse_of_the_dreadblades.remains>11))" );
        else if ( items[i].name_str == "void_stalkers_contract" )
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=mantle_duration>0|buff.curse_of_the_dreadblades.up" );
        else
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=buff.bloodlust.react|target.time_to_die<=20|combo_points.deficit<=2" );
      }
    }
    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "arcane_torrent" )
        cds -> add_action( racial_actions[i] + ",if=energy.deficit>40" );
      else if ( racial_actions[i] == "arcane_pulse" )
        continue; // Manually added
      else
        cds -> add_action( racial_actions[i] );
    }
    cds -> add_action( this, "Adrenaline Rush", "if=!buff.adrenaline_rush.up&energy.deficit>0" );
    cds -> add_talent( this, "Marked for Death", "target_if=min:target.time_to_die,if=target.time_to_die<combo_points.deficit|((raid_event.adds.in>40|buff.true_bearing.remains>15-buff.adrenaline_rush.up*5)&!stealthed.rogue&combo_points.deficit>=cp_max_spend-1)" );
    cds -> add_action( this, "Sprint", "if=!talent.death_from_above.enabled&equipped.thraxis_tricksy_treads&!variable.ss_useable" );
    cds -> add_action( "darkflight,if=equipped.thraxis_tricksy_treads&!variable.ss_useable&buff.sprint.down" );

    // Finishers
    action_priority_list_t* finish = get_action_priority_list( "finish", "Finishers" );
    finish -> add_action( this, "Between the Eyes", "if=equipped.greenskins_waterlogged_wristcuffs&!buff.greenskins_waterlogged_wristcuffs.up", "BTE in mantle used to be DPS neutral but is a loss due to t21" );
    finish -> add_action( this, "Dispatch", "if=!talent.death_from_above.enabled|energy.time_to_max<cooldown.death_from_above.remains+3.5" );

    // Stealth
    action_priority_list_t* stealth = get_action_priority_list( "stealth", "Stealth" );
    stealth -> add_action( "variable,name=ambush_condition,value=combo_points.deficit>=2+2*(talent.ghostly_strike.enabled&!debuff.ghostly_strike.up)+buff.broadsides.up&energy>60&!buff.jolly_roger.up" );
    stealth -> add_action( this, "Ambush", "if=variable.ambush_condition" );
    stealth -> add_action( this, "Vanish", "if=(variable.ambush_condition|equipped.mantle_of_the_master_assassin&!variable.rtb_reroll&!variable.ss_useable)&mantle_duration=0" );
    stealth -> add_action( "shadowmeld,if=variable.ambush_condition" );
  }
  else if ( specialization() == ROGUE_SUBTLETY )
  {
    // Pre-Combat
    precombat -> add_action( "variable,name=ssw_refund,value=equipped.shadow_satyrs_walk*(6+ssw_refund_offset)", "Defined variables that doesn't change during the fight." );
    precombat -> add_action( "variable,name=stealth_threshold,value=(65+talent.vigor.enabled*35+talent.master_of_shadows.enabled*10+variable.ssw_refund)" );
    precombat -> add_action( "variable,name=shd_fractional,value=1.725+0.725*talent.enveloping_shadows.enabled" );
    precombat -> add_action( this, "Stealth" );
    precombat -> add_talent( this, "Marked for Death", "precombat=1" );
    precombat -> add_action( "potion" );

    // Main Rotation
    def -> add_action( "variable,name=dsh_dfa,value=talent.death_from_above.enabled&talent.dark_shadow.enabled&spell_targets.death_from_above<4" );
    def -> add_action( this, "Shadow Dance", "if=talent.dark_shadow.enabled&(!stealthed.all|buff.subterfuge.up)&buff.death_from_above.up&buff.death_from_above.remains<=0.15", "This let us to use Shadow Dance right before the 2nd part of DfA lands. Only with Dark Shadow." );
    def -> add_action( "wait,sec=0.1,if=buff.shadow_dance.up&gcd.remains>0", "This is triggered only with DfA talent since we check shadow_dance even while the gcd is ongoing, it's purely for simulation performance." );
    def -> add_action( "call_action_list,name=cds" );
    def -> add_action( "run_action_list,name=stealthed,if=stealthed.all", "Fully switch to the Stealthed Rotation (by doing so, it forces pooling if nothing is available)." );
    def -> add_action( this, "Nightblade", "if=target.time_to_die>6&remains<gcd.max&combo_points>=4-(time<10)*2" );
    def -> add_action( "call_action_list,name=stealth_als,if=talent.dark_shadow.enabled&combo_points.deficit>=2+buff.shadow_blades.up&(dot.nightblade.remains>4+talent.subterfuge.enabled|cooldown.shadow_dance.charges_fractional>=1.9&(!equipped.denial_of_the_halfgiants|time>10))" );
    def -> add_action( "call_action_list,name=stealth_als,if=!talent.dark_shadow.enabled&(combo_points.deficit>=2+buff.shadow_blades.up|cooldown.shadow_dance.charges_fractional>=1.9+talent.enveloping_shadows.enabled)" );
    def -> add_action( "call_action_list,name=finish,if=combo_points>=5+(talent.deeper_stratagem.enabled&!buff.shadow_blades.up&(mantle_duration=0|set_bonus.tier20_4pc)&(!buff.the_first_of_the_dead.up|variable.dsh_dfa))|(combo_points>=4&combo_points.deficit<=2&spell_targets.shuriken_storm>=3&spell_targets.shuriken_storm<=4)|(target.time_to_die<=1&combo_points>=3)" );
    def -> add_action( "call_action_list,name=finish,if=buff.the_first_of_the_dead.remains>1&combo_points>=3&spell_targets.shuriken_storm<2&!buff.shadow_gestures.up" );
    def -> add_action( "call_action_list,name=finish,if=variable.dsh_dfa&equipped.the_first_of_the_dead&dot.nightblade.remains<=(cooldown.symbols_of_death.remains+10)&cooldown.symbols_of_death.remains<=2&combo_points>=2" );
    def -> add_action( "wait,sec=time_to_sht.5,if=combo_points=5&time_to_sht.5<=1&energy.deficit>=30&!buff.shadow_blades.up" );
    def -> add_action( "call_action_list,name=build,if=energy.deficit<=variable.stealth_threshold" );
    def -> add_action( "arcane_pulse");

    // Builders
    action_priority_list_t* build = get_action_priority_list( "build", "Builders" );
    build -> add_action( this, "Shuriken Storm", "if=spell_targets.shuriken_storm>=2+(buff.the_first_of_the_dead.up|buff.symbols_of_death.up)" );
    build -> add_action( this, "Shuriken Toss", "if=buff.sharpened_blades.stack>=19" );
    build -> add_talent( this, "Gloomblade" );
    build -> add_action( this, "Backstab" );

    // Cooldowns
    action_priority_list_t* cds = get_action_priority_list( "cds", "Cooldowns" );
    cds -> add_action( potion_action );
    for ( size_t i = 0; i < items.size(); i++ )
    {
      if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
      {
        if ( items[i].name_str == "draught_of_souls" )
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=!stealthed.rogue&energy.deficit>30+talent.vigor.enabled*10" );
        else if ( items[i].name_str == "specter_of_betrayal" )
        {
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=talent.dark_shadow.enabled&buff.shadow_dance.up&(!set_bonus.tier20_4pc|buff.symbols_of_death.up|(!talent.death_from_above.enabled&((mantle_duration>=3|!equipped.mantle_of_the_master_assassin)|cooldown.vanish.remains>=43)))" );
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=!talent.dark_shadow.enabled&!buff.stealth.up&!buff.vanish.up&(mantle_duration>=3|!equipped.mantle_of_the_master_assassin)" );
        }
        else if ( items[i].name_str == "umbral_moonglaives" || items[i].name_str == "void_stalkers_contract" || items[i].name_str == "forgefiends_fabricator" )
        {
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=talent.dark_shadow.enabled&!buff.stealth.up&!buff.vanish.up&buff.shadow_dance.up&(buff.symbols_of_death.up|(!talent.death_from_above.enabled&(mantle_duration>=3|!equipped.mantle_of_the_master_assassin)))" );
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=!talent.dark_shadow.enabled&!buff.stealth.up&!buff.vanish.up&(mantle_duration>=3|!equipped.mantle_of_the_master_assassin)" );
        }
        else if ( items[i].name_str == "vial_of_ceaseless_toxins" )
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=(!talent.dark_shadow.enabled|buff.shadow_dance.up)&(buff.symbols_of_death.up|(!talent.death_from_above.enabled&((mantle_duration>=3|!equipped.mantle_of_the_master_assassin)|cooldown.vanish.remains>=60)))" );
        else if ( items[i].name_str == "tome_of_unraveling_sanity" )
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=cooldown.symbols_of_death.remains<=12|(!stealthed.all&!buff.vanish.up&time<=10)" );
        else
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=(buff.shadow_blades.up&stealthed.rogue)|target.time_to_die<20" );
      }
    }
    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "arcane_torrent" )
        cds -> add_action( racial_actions[i] + ",if=energy.deficit>70" );
      else if ( racial_actions[i] == "arcane_pulse" )
        continue; // Manually added
      else
        cds -> add_action( racial_actions[i] + ",if=stealthed.rogue" );
    }
    cds -> add_action( this, "Symbols of Death", "if=!talent.death_from_above.enabled" );
    cds -> add_action( this, "Symbols of Death", "if=(talent.death_from_above.enabled&cooldown.death_from_above.remains<=1&(dot.nightblade.remains>=cooldown.death_from_above.remains+3|target.time_to_die-dot.nightblade.remains<=6)&(time>=3|set_bonus.tier20_4pc|equipped.the_first_of_the_dead))|target.time_to_die-remains<=10" );
    cds -> add_talent( this, "Marked for Death", "target_if=min:target.time_to_die,if=target.time_to_die<combo_points.deficit" );
    cds -> add_talent( this, "Marked for Death", "if=raid_event.adds.in>40&!stealthed.all&combo_points.deficit>=cp_max_spend" );
    cds -> add_action( this, "Shadow Blades", "if=(time>10&combo_points.deficit>=2+stealthed.all-equipped.mantle_of_the_master_assassin)|(time<10&(!talent.marked_for_death.enabled|combo_points.deficit>=3|dot.nightblade.ticking))" );
    cds -> add_action( "pool_resource,for_next=1,extra_amount=55-talent.shadow_focus.enabled*10" );
    cds -> add_action( this, "Vanish", "if=energy>=55-talent.shadow_focus.enabled*10&variable.dsh_dfa&(!equipped.mantle_of_the_master_assassin|buff.symbols_of_death.up)&cooldown.shadow_dance.charges_fractional<=variable.shd_fractional&!buff.shadow_dance.up&!buff.stealth.up&mantle_duration=0&(dot.nightblade.remains>=cooldown.death_from_above.remains+6&!(buff.the_first_of_the_dead.remains>1&combo_points>=5)|target.time_to_die-dot.nightblade.remains<=6)&cooldown.death_from_above.remains<=1|target.time_to_die<=7" );
    cds -> add_action( this, "Shadow Dance", "if=!buff.shadow_dance.up&target.time_to_die<=4+talent.subterfuge.enabled" );

    // Finishers
    action_priority_list_t* finish = get_action_priority_list( "finish", "Finishers" );
    finish -> add_action( this, "Nightblade", "if=(!talent.dark_shadow.enabled|!buff.shadow_dance.up)&target.time_to_die-remains>6&(mantle_duration=0|remains<=mantle_duration)&((refreshable&variable.dsh_dfa)|remains<tick_time*2)&(spell_targets.shuriken_storm<4&!variable.dsh_dfa|!buff.symbols_of_death.up)" );
    finish -> add_action( this, "Nightblade", "cycle_targets=1,if=(!talent.dark_shadow.enabled|!buff.shadow_dance.up)&target.time_to_die-remains>12&mantle_duration=0&(refreshable&variable.dsh_dfa|remains<tick_time*2)&(spell_targets.shuriken_storm<4&!variable.dsh_dfa|!buff.symbols_of_death.up)" );
    finish -> add_action( this, "Nightblade", "if=remains<cooldown.symbols_of_death.remains+10&cooldown.symbols_of_death.remains<=5+(combo_points=6)&target.time_to_die-remains>cooldown.symbols_of_death.remains+5" );
    finish -> add_talent( this, "Death from Above", "if=!talent.dark_shadow.enabled|(!buff.shadow_dance.up|spell_targets>=4)&(buff.symbols_of_death.up|cooldown.symbols_of_death.remains>=10+set_bonus.tier20_4pc*5)&buff.the_first_of_the_dead.remains<1&spell_targets.shuriken_storm<4" );
    finish -> add_action( this, "Eviscerate" );

    // Stealth Action List Starter
    action_priority_list_t* stealth_als = get_action_priority_list( "stealth_als", "Stealth Action List Starter" );
    stealth_als -> add_action( "call_action_list,name=stealth_cds,if=energy.deficit<=variable.stealth_threshold-25&(!equipped.shadow_satyrs_walk|cooldown.shadow_dance.charges_fractional>=variable.shd_fractional|energy.deficit>=10)" );
    stealth_als -> add_action( "call_action_list,name=stealth_cds,if=mantle_duration>2.3" );
    stealth_als -> add_action( "call_action_list,name=stealth_cds,if=spell_targets.shuriken_storm>=4" );
    stealth_als -> add_action( "call_action_list,name=stealth_cds,if=(cooldown.shadowmeld.up&!cooldown.vanish.up&cooldown.shadow_dance.charges<=1)" );
    stealth_als -> add_action( "call_action_list,name=stealth_cds,if=target.time_to_die<12*cooldown.shadow_dance.charges_fractional*(1+equipped.shadow_satyrs_walk*0.5)" );

    // Stealth Cooldowns
    action_priority_list_t* stealth_cds = get_action_priority_list( "stealth_cds", "Stealth Cooldowns" );
    stealth_cds -> add_action( this, "Vanish", "if=!variable.dsh_dfa&mantle_duration=0&cooldown.shadow_dance.charges_fractional<variable.shd_fractional+(equipped.mantle_of_the_master_assassin&time<30)*0.3&(!equipped.mantle_of_the_master_assassin|buff.symbols_of_death.up)" );
    stealth_cds -> add_action( this, "Shadow Dance", "if=dot.nightblade.remains>=5&charges_fractional>=variable.shd_fractional|target.time_to_die<cooldown.symbols_of_death.remains" );
    stealth_cds -> add_action( "pool_resource,for_next=1,extra_amount=40" );
    stealth_cds -> add_action( "shadowmeld,if=energy>=40&energy.deficit>=10+variable.ssw_refund" );
    stealth_cds -> add_action( this, "Shadow Dance", "if=!variable.dsh_dfa&combo_points.deficit>=2+talent.subterfuge.enabled*2&(buff.symbols_of_death.remains>=1.2|cooldown.symbols_of_death.remains>=12+(talent.dark_shadow.enabled&set_bonus.tier20_4pc)*3-(!talent.dark_shadow.enabled&set_bonus.tier20_4pc)*4|mantle_duration>0)&(spell_targets.shuriken_storm>=4|!buff.the_first_of_the_dead.up)" );

    // Stealthed Rotation
    action_priority_list_t* stealthed = get_action_priority_list( "stealthed", "Stealthed Rotation" );
    stealthed -> add_action( this, "Shadowstrike", "if=buff.stealth.up", "If stealth is up, we really want to use Shadowstrike to benefits from the passive bonus, even if we are at max cp (from the precombat MfD)." );
    stealthed -> add_action( "call_action_list,name=finish,if=combo_points>=5+(talent.deeper_stratagem.enabled&buff.vanish.up)&(spell_targets.shuriken_storm>=3+equipped.shadow_satyrs_walk|(mantle_duration<=1.3&mantle_duration>=0.3))" );
    stealthed -> add_action( this, "Shuriken Storm", "if=buff.shadowmeld.down&((combo_points.deficit>=2+equipped.insignia_of_ravenholdt&spell_targets.shuriken_storm>=3+equipped.shadow_satyrs_walk)|(combo_points.deficit>=1&buff.the_dreadlords_deceit.stack>=29))" );
    stealthed -> add_action( "call_action_list,name=finish,if=combo_points>=5+(talent.deeper_stratagem.enabled&buff.vanish.up)&combo_points.deficit<3+buff.shadow_blades.up-equipped.mantle_of_the_master_assassin" );
    stealthed -> add_action( this, "Shadowstrike" );
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
  if ( name == "blade_rush"          ) return new blade_rush_t         ( this, options_str );
  if ( name == "blindside"           ) return new blindside_t          ( this, options_str );
  if ( name == "crimson_tempest"     ) return new crimson_tempest_t    ( this, options_str );
  if ( name == "death_from_above"    ) return new death_from_above_t   ( this, options_str );
  if ( name == "dispatch"            ) return new dispatch_t           ( this, options_str );
  if ( name == "envenom"             ) return new envenom_t            ( this, options_str );
  if ( name == "eviscerate"          ) return new eviscerate_t         ( this, options_str );
  if ( name == "exsanguinate"        ) return new exsanguinate_t       ( this, options_str );
  if ( name == "fan_of_knives"       ) return new fan_of_knives_t      ( this, options_str );
  if ( name == "feint"               ) return new feint_t              ( this, options_str );
  if ( name == "garrote"             ) return new garrote_t            ( this, options_str );
  if ( name == "gouge"               ) return new gouge_t              ( this, options_str );
  if ( name == "ghostly_strike"      ) return new ghostly_strike_t     ( this, options_str );
  if ( name == "gloomblade"          ) return new gloomblade_t         ( this, options_str );
  if ( name == "kick"                ) return new kick_t               ( this, options_str );
  if ( name == "kidney_shot"         ) return new kidney_shot_t        ( this, options_str );
  if ( name == "killing_spree"       ) return new killing_spree_t      ( this, options_str );
  if ( name == "marked_for_death"    ) return new marked_for_death_t   ( this, options_str );
  if ( name == "mutilate"            ) return new mutilate_t           ( this, options_str );
  if ( name == "nightblade"          ) return new nightblade_t         ( this, options_str );
  if ( name == "pistol_shot"         ) return new pistol_shot_t        ( this, options_str );
  if ( name == "poisoned_knife"      ) return new poisoned_knife_t     ( this, options_str );
  if ( name == "roll_the_bones"      ) return new roll_the_bones_t     ( this, options_str );
  if ( name == "rupture"             ) return new rupture_t            ( this, options_str );
  if ( name == "shadow_blades"       ) return new shadow_blades_t      ( this, options_str );
  if ( name == "shadow_dance"        ) return new shadow_dance_t       ( this, options_str );
  if ( name == "shadowstep"          ) return new shadowstep_t         ( this, options_str );
  if ( name == "shadowstrike"        ) return new shadowstrike_t       ( this, options_str );
  if ( name == "shuriken_storm"      ) return new shuriken_storm_t     ( this, options_str );
  if ( name == "shuriken_toss"       ) return new shuriken_toss_t      ( this, options_str );
  if ( name == "sinister_strike"     ) return new sinister_strike_t    ( this, options_str );
  if ( name == "slice_and_dice"      ) return new slice_and_dice_t     ( this, options_str );
  if ( name == "sprint"              ) return new sprint_t             ( this, options_str );
  if ( name == "stealth"             ) return new stealth_t            ( this, options_str );
  if ( name == "symbols_of_death"    ) return new symbols_of_death_t   ( this, options_str );
  if ( name == "toxic_blade"         ) return new toxic_blade_t        ( this, options_str );
  if ( name == "vanish"              ) return new vanish_t             ( this, options_str );
  if ( name == "vendetta"            ) return new vendetta_t           ( this, options_str );
  if ( name == "cancel_autoattack"   ) return new cancel_autoattack_t  ( this, options_str );
  if ( name == "swap_weapon"         ) return new weapon_swap_t        ( this, options_str );

  return player_t::create_action( name, options_str );
}

// rogue_t::create_rtb_buff_t21_expression ==============================

expr_t* rogue_t::create_rtb_buff_t21_expression( const buff_t* rtb_buff )
{
  return make_fn_expr( "t21", [ this, rtb_buff ]() {
    return rtb_buff -> check() && rtb_buff -> remains() != buffs.roll_the_bones -> remains();
  } );
}

// rogue_t::create_expression ===============================================

expr_t* rogue_t::create_expression( action_t* a, const std::string& name_str )
{
  std::vector<std::string> split = util::string_split( name_str, "." );

  if ( name_str == "combo_points" )
    return make_ref_expr( name_str, resources.current[ RESOURCE_COMBO_POINT ] );
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
  else if ( util::str_compare_ci(name_str, "mantle_duration") )
  {
    return make_fn_expr( name_str, [ this ]() {
      if ( buffs.mantle_of_the_master_assassin_aura -> check() )
      {
        timespan_t nominal_master_assassin_duration = timespan_t::from_seconds( spell.master_assassins_initiative -> effectN( 1 ).base_value() );
        timespan_t gcd_remains = timespan_t::from_seconds( std::max( ( gcd_ready - sim -> current_time() ).total_seconds(), 0.0 ) );
        return gcd_remains + nominal_master_assassin_duration;
      }
      else if ( buffs.mantle_of_the_master_assassin -> check() )
        return buffs.mantle_of_the_master_assassin -> remains();
      else
        return timespan_t::from_seconds( 0.0 );
    } );
  }
  else if ( util::str_compare_ci( name_str, "poisoned_enemies" ) )
    return make_ref_expr( name_str, poisoned_enemies );
  else if ( util::str_compare_ci( name_str, "poisoned" ) )
    return make_fn_expr( name_str, [ this ]() {
      rogue_td_t* tdata = get_target_data( target );
      return tdata -> lethal_poisoned();
    } );
  else if ( util::str_compare_ci( name_str, "poison_remains" ) )
    return make_fn_expr( name_str, [ this ]() {
      rogue_td_t* tdata = get_target_data( target );
      return tdata -> lethal_poison_remains();
    } );
  else if ( util::str_compare_ci( name_str, "bleeds" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      rogue_td_t* tdata = get_target_data( target );
      return tdata -> dots.garrote -> is_ticking() +
             tdata -> dots.internal_bleeding -> is_ticking() +
             tdata -> dots.rupture -> is_ticking();
    } );
  }
  else if ( util::str_compare_ci( name_str, "poisoned_bleeds" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      int poisoned_bleeds = 0;
      for ( size_t i = 0, actors = sim -> target_non_sleeping_list.size(); i < actors; i++ )
      {
        player_t* t = sim -> target_non_sleeping_list[i];
        rogue_td_t* tdata = get_target_data( t );
        if ( tdata -> lethal_poisoned() ) {
          poisoned_bleeds += tdata -> dots.garrote -> is_ticking() +
                             tdata -> dots.internal_bleeding -> is_ticking() +
                             tdata -> dots.rupture -> is_ticking();
        }
      }
      return poisoned_bleeds;
    } );
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
  else if ( util::str_compare_ci(name_str, "ssw_refund_offset") )
  {
    return make_ref_expr(name_str, ssw_refund_offset);
  }
  else if ( util::str_compare_ci(name_str, "fok_rotation") )
  {
    return make_ref_expr(name_str, fok_rotation);
  }

  // Split expressions

  // stealthed.(rogue|all)
  // rogue: all rogue abilities are checked (stealth, vanish, shadow_dance, subterfuge)
  // mantle: all abilities that maintain Mantle of the Master Assassin aura are checked (stealth, vanish)
  // all: all abilities that allow stealth are checked (rogue + shadowmeld)
  if ( split.size() == 2 && util::str_compare_ci( split[ 0 ], "stealthed" ) )
  {
    if ( util::str_compare_ci( split[ 1 ], "rogue" ) )
    {
      return make_fn_expr( split[ 0 ], [ this ]() {
        return buffs.stealth -> check() || buffs.vanish -> check() || buffs.shadow_dance -> check() || buffs.subterfuge -> check();
      } );
    }
    else if ( util::str_compare_ci( split[ 1 ], "mantle" ) )
    {
      return make_fn_expr( split[ 0 ], [ this ]() {
        return buffs.stealth -> check() || buffs.vanish -> check();
      } );
    }
    else if ( util::str_compare_ci( split[ 1 ], "all" ) )
    {
      return make_fn_expr( split[ 0 ], [ this ]() {
        return buffs.stealth -> check() || buffs.vanish -> check() || buffs.shadow_dance -> check() || buffs.subterfuge -> check() || this -> player_t::buffs.shadowmeld -> check();
      } );
    }
  }
  // dot.(garrote|internal_bleeding|rupture).exsanguinated
  else if ( split.size() == 3 && util::str_compare_ci( split[ 2 ], "exsanguinated" ) &&
    ( util::str_compare_ci( split[ 1 ], "garrote" ) ||
      util::str_compare_ci( split[ 1 ], "internal_bleeding" ) ||
      util::str_compare_ci( split[ 1 ], "rupture" ) ) )
  {
    action_t* action = find_action( split[ 1 ] );
    if ( !action )
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
          if ( type == RTB_ANY && rtb_buffs[ list_values[ i ] ]->check() )
          {
            return 1;
          }
          else if ( type == RTB_ALL && !rtb_buffs[ list_values[ i ] ]->check() )
          {
            return 0;
          }
        }

        return type == RTB_ANY ? 0 : 1;
      } );
    }
  }
  // time_to_sht.(1|2|3|4|5)
  // x: returns time until we will do the xth attack since last ShT proc.
  if ( split.size() == 2 && util::str_compare_ci( split[ 0 ], "time_to_sht" ) )
  {
    return make_fn_expr( split[ 0 ], [ this, split ]() {
      timespan_t return_value = timespan_t::from_seconds( 0.0 );
      unsigned attack_x = strtoul( split[ 1 ].c_str(), nullptr, 0 );
      if ( main_hand_attack && attack_x > shadow_techniques && attack_x <= 5 )
      {
        unsigned remaining_aa = attack_x - shadow_techniques;
        if ( sim->debug ) sim->out_debug.printf( "Inside the shadowtechniques handler, attack_x = %u, remaining_aa = %u", attack_x, remaining_aa );

        timespan_t mh_swing_time = main_hand_attack->execute_time();
        if ( sim->debug )
        {
          sim->out_debug.printf( "mh_swing_time, %.3f", mh_swing_time.total_seconds() );
        }
        timespan_t mh_next_swing = timespan_t::from_seconds( 0.0 );
        if ( main_hand_attack->execute_event == nullptr )
        {
          mh_next_swing = mh_swing_time;
        }
        else
        {
          mh_next_swing = main_hand_attack->execute_event->remains();
        }
        if ( sim->debug ) sim->out_debug.printf( "Main hand next_swing in: %.3f", mh_next_swing.total_seconds() );

        timespan_t oh_swing_time = timespan_t::from_seconds( 0.0 );
        timespan_t oh_next_swing = timespan_t::from_seconds( 0.0 );
        if ( off_hand_attack )
        {
          oh_swing_time = off_hand_attack->execute_time();
          if ( sim->debug )
          {
            sim->out_debug.printf( "oh_swing_time:%.3f", oh_swing_time.total_seconds() );
          }
          if ( off_hand_attack->execute_event == nullptr )
          {
            oh_next_swing = oh_swing_time;
          }
          else
          {
            oh_next_swing = off_hand_attack->execute_event->remains();
          }
          if ( sim->debug ) sim->out_debug.printf( "Off hand next_swing in: %.3f", oh_next_swing.total_seconds() );
        }
        else
        {
          if ( sim->debug ) sim->out_debug.printf( "Off hand attack not found, using only main hand timers" );
        }

        // Store upcoming attack timers and sort
        std::vector<timespan_t> attacks;
        attacks.reserve( 2 * remaining_aa );
        for ( size_t i = 0; i < remaining_aa; i++ )
        {
          attacks.push_back( mh_next_swing + i * mh_swing_time );
          if ( off_hand_attack )
            attacks.push_back( oh_next_swing + i * oh_swing_time );
        }
        std::sort( attacks.begin(), attacks.end() );
        return_value = attacks.at( remaining_aa - 1 );
      }
      else if ( main_hand_attack == nullptr )
      {
        return_value = timespan_t::from_seconds( 0.0 );
        if ( sim->debug ) sim->out_debug.printf( "Main hand attack is required but was not found" );
      }
      else if ( attack_x > 5 )
      {
        return_value = timespan_t::from_seconds( 0.0 );
        if ( sim->debug ) sim->out_debug.printf( "Invalid value %u for attack_x (must be 5 or less)", attack_x );
      }
      else
      {
        return_value = timespan_t::from_seconds( 0.0 );
        if ( sim->debug ) sim->out_debug.printf( "attack_x value %u is not greater than shadow techniques count %u, returning %.3f", attack_x, shadow_techniques, return_value.total_seconds() );
      }
      if ( sim->debug ) sim->out_debug.printf( "Shadow techniques return value is: %.3f", return_value.total_seconds() );
      return return_value;
    } );
  }
  if ( util::str_compare_ci( name_str, "buff.broadsides.t21" ) )
  {
    return create_rtb_buff_t21_expression( buffs.broadsides );
  }
  if ( util::str_compare_ci( name_str, "buff.buried_treasure.t21" ) )
  {
    return create_rtb_buff_t21_expression( buffs.buried_treasure );
  }
  if ( util::str_compare_ci( name_str, "buff.grand_melee.t21" ) )
  {
    return create_rtb_buff_t21_expression( buffs.grand_melee );
  }
  if ( util::str_compare_ci( name_str, "buff.jolly_roger.t21" ) )
  {
    return create_rtb_buff_t21_expression( buffs.jolly_roger );
  }
  if ( util::str_compare_ci( name_str, "buff.shark_infested_waters.t21" ) )
  {
    return create_rtb_buff_t21_expression( buffs.shark_infested_waters );
  }
  if ( util::str_compare_ci( name_str, "buff.true_bearing.t21" ) )
  {
    return create_rtb_buff_t21_expression( buffs.true_bearing );
  }

  return player_t::create_expression( a, name_str );
}

// rogue_t::init_base =======================================================

void rogue_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 5;

  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;

  resources.base[ RESOURCE_COMBO_POINT ] = 5;
  resources.base[ RESOURCE_COMBO_POINT ] += talent.deeper_stratagem -> effectN( 1 ).base_value() ;

  resources.base[ RESOURCE_ENERGY ] = 100;
  resources.base[ RESOURCE_ENERGY ] += talent.vigor -> effectN( 1 ).base_value();
  resources.base[ RESOURCE_ENERGY ] += spec.assassination_rogue -> effectN( 5 ).base_value();

  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *= 1.0 + spec.combat_potency_reg -> effectN( 1 ).percent();
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *= 1.0 + talent.vigor -> effectN( 2 ).percent();

  base_gcd = timespan_t::from_seconds( 1.0 );
  min_gcd  = timespan_t::from_seconds( 1.0 );

  // Force ready trigger if there is a rogue player
  if ( rogue_ready_trigger )
  {
    for ( size_t i = 0; i < sim -> player_list.size(); ++i )
    {
      player_t* p = sim -> player_list[i];
      if ( p -> specialization() != ROGUE_ASSASSINATION && p -> specialization() != ROGUE_OUTLAW && p -> specialization() != ROGUE_SUBTLETY )
      {
        rogue_ready_trigger = false;
        break;
      }
    }
    if ( rogue_ready_trigger )
    {
      ready_type = READY_TRIGGER;
      // Disabled for now
      // sim -> errorf( "[Rogue] ready_trigger=1 has been enabled. You can disable by adding rogue_ready_trigger=0 to your actor" );
    }
  }
}

// rogue_t::init_spells =====================================================

void rogue_t::init_spells()
{
  player_t::init_spells();

  // Shared
  spec.shadowstep           = find_specialization_spell( "Shadowstep" );

  // Generic
  spec.assassination_rogue  = find_specialization_spell( "Assassination Rogue" );
  spec.outlaw_rogue         = find_specialization_spell( "Outlaw Rogue" );
  spec.subtlety_rogue       = find_specialization_spell( "Subtlety Rogue" );

  // Assassination
  spec.improved_poisons     = find_specialization_spell( "Improved Poisons" );
  spec.seal_fate            = find_specialization_spell( "Seal Fate" );
  spec.venomous_wounds      = find_specialization_spell( "Venomous Wounds" );
  spec.vendetta             = find_specialization_spell( "Vendetta" );
  spec.master_assassin      = find_spell( 256735 );
  spec.garrote              = find_specialization_spell( "Garrote" );
  spec.garrote_2            = find_specialization_spell( 231719 );

  // Outlaw
  spec.blade_flurry         = find_specialization_spell( "Blade Flurry" );
  spec.combat_potency       = find_specialization_spell( 35551 );
  spec.combat_potency_reg   = find_specialization_spell( 61329 );
  spec.restless_blades      = find_specialization_spell( "Restless Blades" );
  spec.roll_the_bones       = find_specialization_spell( "Roll the Bones" );
  spec.ruthlessness         = find_specialization_spell( "Ruthlessness" );
  spec.sinister_strike      = find_specialization_spell( "Sinister Strike" );

  // Subtlety
  spec.deepening_shadows    = find_specialization_spell( "Deepening Shadows" );
  spec.relentless_strikes   = find_specialization_spell( "Relentless Strikes" );
  spec.shadow_blades        = find_specialization_spell( "Shadow Blades" );
  spec.shadow_dance         = find_specialization_spell( "Shadow Dance" );
  spec.shadow_techniques    = find_specialization_spell( "Shadow Techniques" );
  spec.shadow_techniques_effect = find_spell( 196911 );
  spec.symbols_of_death     = find_specialization_spell( "Symbols of Death" );
  spec.eviscerate           = find_specialization_spell( "Eviscerate" );
  spec.eviscerate_2         = find_specialization_spell( 231716 );
  spec.shadowstrike         = find_specialization_spell( "Shadowstrike" );
  spec.shadowstrike_2       = find_spell( 245623 );
  spec.shuriken_combo       = find_specialization_spell( "Shuriken Combo" );
  spec.t20_2pc_subtlety     = sets->has_set_bonus( ROGUE_SUBTLETY, T20, B2 ) ?
                                sets->set( ROGUE_SUBTLETY, T20, B2 ) : spell_data_t::not_found();

  // Masteries
  mastery.potent_assassin   = find_mastery_spell( ROGUE_ASSASSINATION );
  mastery.main_gauche       = find_mastery_spell( ROGUE_OUTLAW );
  mastery.executioner       = find_mastery_spell( ROGUE_SUBTLETY );

  // Misc spells
  spell.poison_bomb_driver            = find_spell( 255545 );
  spell.critical_strikes              = find_spell( 157442 );
  spell.death_from_above              = find_spell( 163786 );
  spell.fan_of_knives                 = find_class_spell( "Fan of Knives" );
  spell.fleet_footed                  = find_spell( 31209 );
  spell.master_of_shadows             = find_spell( 196980 );
  spell.sprint                        = find_class_spell( "Sprint" );
  spell.sprint_2                      = find_spell( 231691 );
  spell.ruthlessness_driver           = find_spell( 14161 );
  spell.ruthlessness_cp               = spec.ruthlessness -> effectN( 1 ).trigger();
  spell.shadow_focus                  = find_spell( 112942 );
  spell.subterfuge                    = find_spell( 115192 );
  spell.relentless_strikes_energize   = find_spell( 98440 );
  spell.insignia_of_ravenholdt        = find_spell( 209041 );
  spell.master_assassins_initiative   = find_spell( 235022 );
  spell.master_assassins_initiative_2 = find_spell( 235027 );
  spell.expose_armor                  = find_spell( 8647 );

  // Talents
  // Shared
  talent.weaponmaster       = find_talent_spell( "Weaponmaster" ); // Note: this will return a different spell depending on the spec.

  talent.nightstalker       = find_talent_spell( "Nightstalker" );
  talent.subterfuge         = find_talent_spell( "Subterfuge" );

  talent.vigor              = find_talent_spell( "Vigor" );
  talent.deeper_stratagem   = find_talent_spell( "Deeper Stratagem" );
  talent.marked_for_death   = find_talent_spell( "Marked for Death" );

  talent.alacrity           = find_talent_spell( "Alacrity" );

  // Assassination
  talent.master_poisoner    = find_talent_spell( "Master Poisoner" );
  talent.elaborate_planning = find_talent_spell( "Elaborate Planning" );
  talent.blindside          = find_talent_spell( "Blindside" );

  talent.master_assassin    = find_talent_spell( "Master Assassin" );

  talent.thuggee            = find_talent_spell( "Thuggee" );
  talent.internal_bleeding  = find_talent_spell( "Internal Bleeding" );

  talent.venom_rush         = find_talent_spell( "Venom Rush" );
  talent.toxic_blade        = find_talent_spell( "Toxic Blade" );
  talent.exsanguinate       = find_talent_spell( "Exsanguinate" );

  talent.poison_bomb        = find_talent_spell( "Poison Bomb" );
  talent.hidden_blades      = find_talent_spell( "Hidden Blades" );
  talent.crimson_tempest    = find_talent_spell( "Crimson Tempest" );

  // Outlaw
  talent.quick_draw         = find_talent_spell( "Quick Draw" );
  talent.ghostly_strike     = find_talent_spell( "Ghostly Strike" );

  talent.hit_and_run        = find_talent_spell( "Hit and Run" );

  talent.dirty_tricks       = find_talent_spell( "Dirty Tricks" );

  talent.loaded_dice        = find_talent_spell( "Loaded Dice" );
  talent.slice_and_dice     = find_talent_spell( "Slice and Dice" );

  talent.dancing_steel      = find_talent_spell( "Dancing Steel" );
  talent.blade_rush         = find_talent_spell( "Blade Rush" );
  talent.killing_spree      = find_talent_spell( "Killing Spree" );

  // Subtlety
  talent.expose_weakness    = find_talent_spell( "Expose Weakness" );
  talent.gloomblade         = find_talent_spell( "Gloomblade" );

  talent.shadow_focus       = find_talent_spell( "Shadow Focus" );

  talent.enveloping_shadows = find_talent_spell( "Enveloping Shadows" );
  talent.dark_shadow        = find_talent_spell( "Dark Shadow" );

  talent.master_of_shadows  = find_talent_spell( "Master of Shadows" );
  talent.death_from_above   = find_talent_spell( "Death from Above" );

  azerite.deadshot          = find_azerite_spell( "Deadshot" );
  azerite.nights_vengeance  = find_azerite_spell( "Night's Vengeance" );
  azerite.sharpened_blades  = find_azerite_spell( "Sharpened Blades" );
  azerite.storm_of_steel    = find_azerite_spell( "Storm of Steel" );
  azerite.twist_the_knife   = find_azerite_spell( "Twist the Knife" );

  auto_attack = new actions::auto_melee_attack_t( this, "" );

  // Legendaries
  insignia_of_ravenholdt_ = new actions::insignia_of_ravenholdt_attack_t( this );

  if ( mastery.main_gauche -> ok() )
    active_main_gauche = new actions::main_gauche_t( this );

  if ( spec.blade_flurry -> ok() )
    active_blade_flurry = new actions::blade_flurry_attack_t( this );

  if ( talent.poison_bomb -> ok() )
  {
    poison_bomb = new actions::poison_bomb_t( this );
  }

  if ( sets -> has_set_bonus( ROGUE_ASSASSINATION, T19, B2 ) )
  {
    t19_2pc_assassination = new actions::mutilated_flesh_t( this );
  }
}

// rogue_t::init_gains ======================================================

void rogue_t::init_gains()
{
  player_t::init_gains();

  gains.adrenaline_rush          = get_gain( "Adrenaline Rush"          );
  gains.blade_rush               = get_gain( "Blade Rush"               );
  gains.buried_treasure          = get_gain( "Buried Treasure"          );
  gains.combat_potency           = get_gain( "Combat Potency"           );
  gains.energy_refund            = get_gain( "Energy Refund"            );
  gains.seal_fate                = get_gain( "Seal Fate"                );
  gains.vendetta                 = get_gain( "Vendetta"                 );
  gains.venom_rush               = get_gain( "Venom Rush"               );
  gains.venomous_wounds          = get_gain( "Venomous Vim"             );
  gains.venomous_wounds_death    = get_gain( "Venomous Vim (death)"     );
  gains.quick_draw               = get_gain( "Quick Draw"               );
  gains.broadsides               = get_gain( "Broadsides"               );
  gains.ruthlessness             = get_gain( "Ruthlessness"             );
  gains.shadow_techniques        = get_gain( "Shadow Techniques"        );
  gains.master_of_shadows        = get_gain( "Master of Shadows"        );
  gains.shadow_blades            = get_gain( "Shadow Blades"            );
  gains.relentless_strikes       = get_gain( "Relentless Strikes"       );
  gains.t19_4pc_subtlety         = get_gain( "Tier 19 4PC Set Bonus"    );
  gains.shadow_satyrs_walk       = get_gain( "Shadow Satyr's Walk"      );
  gains.the_empty_crown          = get_gain( "The Empty Crown"          );
  gains.the_first_of_the_dead    = get_gain( "The First of the Dead"    );
  gains.symbols_of_death         = get_gain( "Symbols of Death"         );
  gains.t20_4pc_outlaw           = get_gain( "Lesser Adrenaline Rush"   );
  gains.t21_4pc_subtlety         = get_gain( "Tier 21 4PC Set Bonus"    );
  gains.t21_4pc_assassination    = get_gain( "Tier 21 4PC Set Bonus"    );
}

// rogue_t::init_procs ======================================================

void rogue_t::init_procs()
{
  player_t::init_procs();

  procs.seal_fate                = get_proc( "Seal Fate"               );
  procs.weaponmaster             = get_proc( "Weaponmaster"            );

  procs.roll_the_bones_1         = get_proc( "Roll the Bones: 1 buff"  );
  procs.roll_the_bones_2         = get_proc( "Roll the Bones: 2 buffs" );
  procs.roll_the_bones_3         = get_proc( "Roll the Bones: 3 buffs" );
  procs.roll_the_bones_4         = get_proc( "Roll the Bones: 4 buffs" );
  procs.roll_the_bones_5         = get_proc( "Roll the Bones: 5 buffs" );
  procs.roll_the_bones_6         = get_proc( "Roll the Bones: 6 buffs" );
  procs.t21_4pc_outlaw           = get_proc( "Tier 21 4PC Set Bonus"   );

  procs.deepening_shadows        = get_proc( "Deepening Shadows"       );

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

  scaling -> enable( STAT_WEAPON_OFFHAND_DPS );
  scaling -> disable( STAT_STRENGTH );

  if ( find_item( "thraxis_tricksy_treads" ) )
  {
    scaling -> enable( STAT_SPEED_RATING );
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
  player_t::create_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )


  // Baseline
  // Shared
  buffs.feint                 = make_buff( this, "feint", find_specialization_spell( "Feint" ) )
                                -> set_duration( find_class_spell( "Feint" ) -> duration() );
  buffs.shadowstep            = make_buff( this, "shadowstep", spec.shadowstep )
                                -> set_cooldown( timespan_t::zero() );
  buffs.sprint                = make_buff( this, "sprint", spell.sprint )
                                -> set_cooldown( timespan_t::zero() )
                                -> add_invalidate( CACHE_RUN_SPEED );
  buffs.stealth               = new buffs::stealth_t( this );
  buffs.vanish                = new buffs::vanish_t( this );
  // Assassination
  buffs.envenom               = make_buff( this, "envenom", find_specialization_spell( "Envenom" ) )
                                -> set_duration( timespan_t::min() )
                                -> set_period( timespan_t::zero() )
                                -> set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
  buffs.vendetta              = make_buff( this, "vendetta_energy", find_spell( 256495 ) )
                                -> set_default_value( find_spell( 256495 ) -> effectN( 1 ).base_value() / 5.0 )
                                -> set_affects_regen( true );

  // Outlaw
  buffs.adrenaline_rush       = new buffs::adrenaline_rush_t( this );
  buffs.blade_flurry          = make_buff( this, "blade_flurry", spec.blade_flurry )
                                -> set_cooldown( timespan_t::zero() )
                                -> set_duration( spec.blade_flurry -> duration() + talent.dancing_steel -> effectN( 2 ).time_value() );
  buffs.blade_rush            = make_buff( this, "blade_rush", find_spell( 271896 ) )
                                -> set_period( find_spell( 271896 ) -> effectN( 1 ).period() )
                                -> set_tick_callback( [ this ]( buff_t* b, int, const timespan_t& ) {
                                  resource_gain( RESOURCE_ENERGY, b -> data().effectN( 1 ).base_value(), gains.blade_rush );
                                } );
  buffs.opportunity           = make_buff( this, "opportunity", find_spell( 195627 ) );
  // Roll the bones buffs
  buffs.broadsides            = make_buff( this, "broadsides", find_spell( 193356 ) )
                                -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.buried_treasure       = make_buff( this, "buried_treasure", find_spell( 199600 ) )
                                -> set_affects_regen( true )
                                -> set_default_value( find_spell( 199600 ) -> effectN( 1 ).base_value() / 5.0 );
  buffs.grand_melee           = make_buff<haste_buff_t>( this, "grand_melee", find_spell( 193358 ) );
  buffs.grand_melee -> add_invalidate( CACHE_ATTACK_SPEED )
                    -> add_invalidate( CACHE_LEECH )
                    -> set_default_value( 1.0 / ( 1.0 + find_spell( 193358 ) -> effectN( 1 ).percent() ) );
  buffs.jolly_roger           = make_buff( this, "jolly_roger", find_spell( 199603 ) )
                                -> set_default_value( find_spell( 199603 ) -> effectN( 1 ).percent() );
  buffs.shark_infested_waters = make_buff( this, "shark_infested_waters", find_spell( 193357 ) )
                                -> set_default_value( find_spell( 193357 ) -> effectN( 1 ).percent() )
                                -> add_invalidate( CACHE_CRIT_CHANCE );
  buffs.true_bearing          = make_buff( this, "true_bearing", find_spell( 193359 ) )
                                -> set_default_value( find_spell( 193359 ) -> effectN( 1 ).base_value() * 0.1 );
  // Note, since I (navv) am a slacker, this needs to be constructed after the secondary buffs.
  buffs.roll_the_bones        = new buffs::roll_the_bones_t( this );
  // Subtlety
  buffs.shuriken_combo        = make_buff( this, "shuriken_combo", find_spell( 245640 ) )
                                -> set_default_value( find_spell( 245640 ) -> effectN( 1 ).percent() );
  buffs.shadow_blades         = new buffs::shadow_blades_t( this );
  buffs.shadow_dance          = new buffs::shadow_dance_t( this );
  buffs.symbols_of_death      = make_buff( this, "symbols_of_death", spec.symbols_of_death )
                                -> set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
                                -> set_period( spec.symbols_of_death -> effectN( 3 ).period() )
                                -> set_tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
                                  if ( sets -> has_set_bonus( ROGUE_SUBTLETY, T20, B4 ) ) {
                                    resource_gain( RESOURCE_ENERGY, sets -> set( ROGUE_SUBTLETY, T20, B4 ) -> effectN( 1 ).base_value(), gains.symbols_of_death );
                                  }
                                } )
                                -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );


  // Talents
  // Shared
  buffs.alacrity                = make_buff<haste_buff_t>( this, "alacrity", find_spell( 193538 ) );
  buffs.alacrity -> set_default_value( find_spell( 193538 ) -> effectN( 1 ).percent() )
                 -> set_chance( talent.alacrity -> ok() );
  buffs.subterfuge              = new buffs::subterfuge_t( this );
  // Assassination
  buffs.elaborate_planning      = make_buff( this, "elaborate_planning", talent.elaborate_planning -> effectN( 1 ).trigger() )
                                  -> set_default_value( 1.0 + talent.elaborate_planning -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
                                  -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.blindside                = make_buff( this, "blindside", talent.blindside )
                                  -> set_duration( timespan_t::from_seconds( 10.0 ) ); // I see no buff spell in spell data yet, hardcode for now.
  buffs.master_assassin_aura    = make_buff(this, "master_assassin_aura", talent.master_assassin)
                                  -> set_default_value( spec.master_assassin -> effectN( 1 ).percent() );
  buffs.master_assassin         = make_buff( this, "master_assassin", talent.master_assassin )
                                  -> set_default_value( spec.master_assassin -> effectN( 1 ).percent() )
                                  -> set_duration( timespan_t::from_seconds( talent.master_assassin -> effectN( 1 ).base_value() ) );
  buffs.hidden_blades_driver    = make_buff( this, "hidden_blades_driver", talent.hidden_blades )
                                  -> set_period( talent.hidden_blades -> effectN( 1 ).period() )
                                  -> set_quiet( true )
                                  -> set_tick_callback( [this]( buff_t*, int, const timespan_t& ) { buffs.hidden_blades -> trigger(); } )
                                  -> set_tick_time_behavior( buff_tick_time_behavior::UNHASTED );
  buffs.hidden_blades           = make_buff( this, "hidden_blades", find_spell( 270070 ) )
                                  -> set_default_value( find_spell( 270070 ) -> effectN( 1 ).percent() );
  // Outlaw
  buffs.loaded_dice             = make_buff( this, "loaded_dice", talent.loaded_dice -> effectN( 1 ).trigger() );
  buffs.slice_and_dice          = make_buff( this, "slice_and_dice", talent.slice_and_dice )
                                  -> set_period( timespan_t::zero() )
                                  -> set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
                                  -> set_affects_regen( true )
                                  -> add_invalidate( CACHE_ATTACK_SPEED );
  buffs.killing_spree           = make_buff( this, "killing_spree", talent.killing_spree )
                                  -> set_duration( talent.killing_spree -> duration() );
  // Subtlety
  buffs.master_of_shadows       = make_buff( this, "master_of_shadows", find_spell( 196980 ) )
                                  -> set_period( find_spell( 196980 ) -> effectN( 1 ).period() )
                                  -> set_tick_callback( [ this ]( buff_t* b, int, const timespan_t& ) {
                                    resource_gain( RESOURCE_ENERGY, b -> data().effectN( 1 ).base_value(), gains.master_of_shadows );
                                  } )
                                  -> set_refresh_behavior( buff_refresh_behavior::DURATION );
  buffs.death_from_above        = make_buff( this, "death_from_above", spell.death_from_above )
                                  // Note: Duration is set to 1.475s (+/- gauss RNG) on action execution in order to match the current model
                                  // and then let it be trackable in the APL. The driver will also expire this buff when the finisher is scheduled.
                                  //.duration( timespan_t::from_seconds( 1.475 ) )
                                  -> set_quiet( true );


  // Legendaries
  // Shared
  buffs.mantle_of_the_master_assassin_aura = make_buff( this, "master_assassins_initiative_aura", spell.master_assassins_initiative )
                                             -> set_duration( sim -> max_time / 2 )
                                             -> set_default_value( spell.master_assassins_initiative_2 -> effectN( 1 ).percent() )
                                             -> add_invalidate( CACHE_CRIT_CHANCE );
  buffs.mantle_of_the_master_assassin      = make_buff( this, "master_assassins_initiative", spell.master_assassins_initiative )
                                             -> set_duration( timespan_t::from_seconds( spell.master_assassins_initiative -> effectN( 1 ).base_value() ) )
                                             -> set_default_value( spell.master_assassins_initiative_2 -> effectN( 1 ).percent() )
                                             -> add_invalidate( CACHE_CRIT_CHANCE );
  buffs.the_dreadlords_deceit_driver       = make_buff( this, "the_dreadlords_deceit_driver", find_spell( 208692 ) )
                                             -> set_period( find_spell( 208692 ) -> effectN( 1 ).period() )
                                             -> set_quiet( true )
                                             -> set_tick_callback( [this]( buff_t*, int, const timespan_t& ) {
                                                buffs.the_dreadlords_deceit -> trigger(); } )
                                             -> set_tick_time_behavior( buff_tick_time_behavior::UNHASTED );
  const spell_data_t* tddid                = ( specialization() == ROGUE_ASSASSINATION ) ? find_spell( 208693 ): ( specialization() == ROGUE_SUBTLETY ) ? find_spell( 228224 ): spell_data_t::not_found();
  buffs.the_dreadlords_deceit              = make_buff( this, "the_dreadlords_deceit", tddid )
                                             -> set_default_value( tddid -> effectN( 1 ).percent() );
  buffs.sephuzs_secret                     = make_buff<haste_buff_t>( this, "sephuzs_secret", find_spell( 208052 ) );
  buffs.sephuzs_secret -> set_cooldown( find_spell( 226262 ) -> duration() )
                       -> set_default_value( find_spell( 208052 ) -> effectN( 2 ).percent() )
                       -> add_invalidate( CACHE_RUN_SPEED );
  // Assassination
  buffs.the_empty_crown                    = make_buff( this, "the_empty_crown", find_spell(248201) )
                                             -> set_period( find_spell(248201) -> effectN( 1 ).period() )
                                             -> set_tick_callback( [ this ]( buff_t* b, int, const timespan_t& ) {
                                               resource_gain( RESOURCE_ENERGY, b -> data().effectN( 1 ).base_value(), gains.the_empty_crown );
                                             } );
  // Outlaw
  buffs.greenskins_waterlogged_wristcuffs  = make_buff( this, "greenskins_waterlogged_wristcuffs", find_spell( 209423 ) );
  buffs.shivarran_symmetry                 = make_buff( this, "shivarran_symmetry", find_spell( 226318 ) );
  // Subtlety
  buffs.the_first_of_the_dead              = make_buff( this, "the_first_of_the_dead", find_spell( 248210 ) );


  // Tiers
  // T19 Outdoor
  buffs.t19oh_8pc                          = make_buff<stat_buff_t>( this, "shadowstalkers_avidity", sets -> set( specialization(), T19OH, B8 ) -> effectN( 1 ).trigger() )
                                             -> set_trigger_spell( sets -> set( specialization(), T19OH, B8 ) );
  // T19 Raid
  buffs.t19_4pc_outlaw                     = make_buff( this, "swordplay", sets -> set( ROGUE_OUTLAW, T19, B4 ) -> effectN( 1 ).trigger() )
                                             -> set_trigger_spell( sets -> set( ROGUE_OUTLAW, T19, B4 ) );
  // T20 Raid
  buffs.t20_2pc_outlaw                     = make_buff( this, "headshot", find_spell( 242277 ) )
                                             -> set_default_value( find_spell( 242277 ) -> effectN( 1 ).percent() )
                                             -> add_invalidate( CACHE_CRIT_CHANCE );
  buffs.t20_4pc_outlaw                     = make_buff<haste_buff_t>( this, "lesser_adrenaline_rush", find_spell( 246558 ) );
  buffs.t20_4pc_outlaw -> set_cooldown( timespan_t::zero() )
                       -> set_default_value( find_spell( 246558 ) -> effectN( 2 ).percent() )
                       -> set_affects_regen( true )
                       -> add_invalidate( CACHE_ATTACK_SPEED );
  // T21 Raid
  buffs.t21_2pc_assassination              = make_buff( this, "virulent_poisons", sets -> set( ROGUE_ASSASSINATION, T21, B2 ) -> effectN( 1 ).trigger() )
                                             -> set_default_value( sets -> set( ROGUE_ASSASSINATION, T21, B2 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );
  buffs.t21_2pc_outlaw                     = make_buff( this, "sharpened_sabers", find_spell( 252285 ) )
                                             -> set_default_value( find_spell( 252285 ) -> effectN( 1 ).percent() );
  buffs.t21_4pc_subtlety                   = make_buff( this, "shadow_gestures", sets -> set( ROGUE_SUBTLETY, T21, B4 ) -> effectN( 1 ).trigger() );

  // Azerite
  buffs.deadshot                           = make_buff( this, "deadshot", find_spell( 272940 ) )
                                             -> set_trigger_spell( azerite.deadshot.spell_ref().effectN( 1 ).trigger() )
                                             -> set_default_value( azerite.deadshot.value() );
  buffs.nights_vengeance                   = make_buff( this, "nights_vengeance", find_spell( 273424 ) )
                                             -> set_trigger_spell( azerite.nights_vengeance.spell_ref().effectN( 1 ).trigger() )
                                             -> set_default_value( azerite.nights_vengeance.value() );
  buffs.sharpened_blades                   = make_buff( this, "sharpened_blades", find_spell( 272916 ) )
                                             -> set_trigger_spell( azerite.sharpened_blades.spell_ref().effectN( 1 ).trigger() )
                                             -> set_default_value( azerite.sharpened_blades.value() );
  buffs.storm_of_steel                     = make_buff( this, "storm_of_steel", find_spell( 273455 ) )
                                             -> set_trigger_spell( azerite.storm_of_steel.spell_ref().effectN( 1 ).trigger() )
                                             -> set_default_value( azerite.storm_of_steel.value() );
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

  if ( buffs.size() == 0 || buffs.size() > 6 )
  {
    sim -> errorf( "%s: No valid 'fixed_rtb' buffs given by string '%s'", sim -> active_player -> name(),
        value.c_str() );
    return false;
  }

  debug_cast< rogue_t* >( sim -> active_player ) -> fixed_rtb = buffs;

  return true;
}

static bool parse_fixed_rtb_odds( sim_t* sim,
                                  const std::string& /* name */,
                                  const std::string& value )
{
  std::vector<std::string> odds = util::string_split( value, "," );
  if ( odds.size() != 6 )
  {
    sim -> errorf( "%s: Expected 6 comma-separated values for 'fixed_rtb_odds'", sim -> active_player -> name());
    return false;
  }

  std::vector<double> buff_chances;
  buff_chances.resize( 6, 0.0 );
  double sum = 0.0;
  for ( size_t i = 0; i < odds.size(); i++ )
  {
    buff_chances[ i ] = strtod( odds[ i ].c_str(), nullptr );
    sum += buff_chances[ i ];
  }

  if ( sum != 100.0 )
  {
    sim -> errorf( "Warning: %s: 'fixed_rtb_odds' adding up to %f instead of 100, re-scaling accordingly", sim -> active_player -> name(), sum );
    for ( size_t i = 0; i < odds.size(); i++ )
    {
      buff_chances[ i ] = buff_chances[ i ] / sum * 100.0;
    }
  }

  debug_cast< rogue_t* >( sim -> active_player ) -> fixed_rtb_odds = buff_chances;
  return true;
}

void rogue_t::create_options()
{
  add_option( opt_func( "off_hand_secondary", parse_offhand_secondary ) );
  add_option( opt_func( "main_hand_secondary", parse_mainhand_secondary ) );
  add_option( opt_int( "initial_combo_points", initial_combo_points ) );
  add_option( opt_func( "fixed_rtb", parse_fixed_rtb ) );
  add_option( opt_func( "fixed_rtb_odds", parse_fixed_rtb_odds ) );
  add_option( opt_int( "ssw_refund_offset", ssw_refund_offset ) );
  add_option( opt_bool( "fok_rotation", fok_rotation ) );
  add_option( opt_bool( "rogue_optimize_expressions", rogue_optimize_expressions ) );
  add_option( opt_bool( "rogue_ready_trigger", rogue_ready_trigger ) );

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

  if ( rogue -> initial_combo_points != 0 )
  {
    initial_combo_points = rogue -> initial_combo_points;
  }

  if ( rogue -> ssw_refund_offset != 0 )
  {
    ssw_refund_offset = rogue -> ssw_refund_offset;
  }

  if ( rogue -> fok_rotation != false )
  {
    fok_rotation = rogue -> fok_rotation;
  }

  fixed_rtb = rogue -> fixed_rtb;
  fixed_rtb_odds = rogue -> fixed_rtb_odds;
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

  if ( stype == SAVE_ALL )
  {
    if ( fok_rotation )
    {
      profile_str += "fok_rotation=1";
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
  shadow_techniques = 0;

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
  if ( !sim -> optimize_expressions )
  {
    if ( rogue_optimize_expressions )
    {
      for ( size_t i = 0; i < sim -> player_list.size(); ++i )
      {
        player_t* p = sim -> player_list[i];
        if ( p -> specialization() != ROGUE_ASSASSINATION && p -> specialization() != ROGUE_OUTLAW && p -> specialization() != ROGUE_SUBTLETY )
        {
          rogue_optimize_expressions = false;
          break;
        }
      }
      if ( rogue_optimize_expressions )
      {
        sim -> optimize_expressions = true;
        // Disabled for now
        // sim -> errorf( "[Rogue] optimize_expressions=1 has been enabled. You can disable by adding rogue_ready_trigger=0 to every rogue actor" );
      }
    }
  }

  player_t::combat_begin();

  if ( legendary.the_dreadlords_deceit )
  {
    buffs.the_dreadlords_deceit -> trigger( buffs.the_dreadlords_deceit -> data().max_stacks() );
    buffs.the_dreadlords_deceit_driver -> trigger();
  }

  if ( talent.hidden_blades -> ok() )
  {
    buffs.hidden_blades -> trigger( buffs.hidden_blades -> data().max_stacks() );
    buffs.hidden_blades_driver -> trigger();
  }
}

// rogue_t::energy_regen_per_second =========================================

double rogue_t::resource_regen_per_second( resource_e r ) const
{
  double reg = player_t::resource_regen_per_second( r );

  if ( r == RESOURCE_ENERGY )
  {
    if ( buffs.slice_and_dice -> up() )
      reg *= 1.0 + talent.slice_and_dice -> effectN( 3 ).percent() * buffs.slice_and_dice -> check_value();
  }

  return reg;
}

// rogue_t::temporary_movement_modifier ==================================

double rogue_t::temporary_movement_modifier() const
{
  double temporary = player_t::temporary_movement_modifier();

  if ( buffs.sprint -> up() )
    temporary = std::max( buffs.sprint -> data().effectN( 1 ).percent(), temporary );

  if ( buffs.shadowstep -> up() )
    temporary = std::max( buffs.shadowstep -> data().effectN( 2 ).percent(), temporary );

  if ( buffs.sephuzs_secret -> up() )
  {
    temporary = std::max( buffs.sephuzs_secret -> data().effectN( 1 ).percent(), temporary );
  }

  return temporary;
}

// rogue_t::passive_movement_modifier===================================

double rogue_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  ms += spell.fleet_footed -> effectN( 1 ).percent();
  ms += talent.hit_and_run -> effectN( 1 ).percent();

  if ( buffs.stealth -> up() || buffs.shadow_dance -> up() ) // Check if nightstalker is temporary or passive.
  {
    ms += talent.nightstalker -> effectN( 1 ).percent();
  }

  if ( legendary.sephuzs_secret )
  {
    ms += legendary.sephuzs_secret -> effectN( 2 ).percent();
  }

  return ms;
}

// rogue_t::regen ===========================================================

void rogue_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  // We handle some energy gain increases here instead of the resource_regen_per_second method in order to better track their benefits.
  if ( ! resources.is_infinite( RESOURCE_ENERGY ) )
  {
    // Adrenaline Rush and Lesser AR T20 bonus
    if ( buffs.adrenaline_rush -> up() )
    {
      double energy_regen = periodicity.total_seconds() * resource_regen_per_second( RESOURCE_ENERGY ) * buffs.adrenaline_rush -> data().effectN( 1 ).percent();
      resource_gain( RESOURCE_ENERGY, energy_regen, gains.adrenaline_rush );
    }
    else if (buffs.t20_4pc_outlaw -> up() )
    {
      double energy_regen = periodicity.total_seconds() * resource_regen_per_second( RESOURCE_ENERGY ) * buffs.t20_4pc_outlaw -> data().effectN( 1 ).percent();
      resource_gain( RESOURCE_ENERGY, energy_regen, gains.t20_4pc_outlaw );
    }

    // Additional energy gains
    if ( buffs.buried_treasure -> up() )
      resource_gain( RESOURCE_ENERGY, buffs.buried_treasure -> check_value() * periodicity.total_seconds(), gains.buried_treasure );
    if ( buffs.vendetta -> up() )
      resource_gain( RESOURCE_ENERGY, buffs.vendetta -> check_value() * periodicity.total_seconds(), gains.vendetta );
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
             timespan_t::from_seconds( ( 25 - energy ) / resource_regen_per_second( RESOURCE_ENERGY ) ),
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
      os.printf("<td class=\"right\">%.3f</td>", p.dfa_mh -> min() );
      os.printf("<td class=\"right\">%.3f</td>", p.dfa_mh -> mean() );
      os.printf("<td class=\"right\">%.3f</td>", p.dfa_mh -> max() );
      os << "</tr>";

      os << "<tr>";
      os << "<td class=\"left\">Off hand</td>";
      os.printf("<td class=\"right\">%.3f</td>", p.dfa_oh -> min() );
      os.printf("<td class=\"right\">%.3f</td>", p.dfa_oh -> mean() );
      os.printf("<td class=\"right\">%.3f</td>", p.dfa_oh -> max() );
      os << "</tr>";

      os << "</table>";

      os << "</div>\n";

      os << "<div class=\"clear\"></div>\n";
    }
    os << "</div>\n";

    os << "<div class=\"player-section custom_section\">\n";
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
  eviscerating_blade_t() : super( ROGUE, "dispatch" ) { }

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

struct thraxis_tricksy_treads_t : public unique_gear::scoped_actor_callback_t<rogue_t>
{
  thraxis_tricksy_treads_t() : super( ROGUE )
  { }
  
  void manipulate( rogue_t* rogue, const special_effect_t& e ) override
  { rogue -> legendary.thraxis_tricksy_treads = e.driver(); }
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

struct the_curse_of_restlessness_t : public unique_gear::scoped_actor_callback_t<rogue_t>
{
  the_curse_of_restlessness_t() : super( ROGUE )
  { }

  void manipulate( rogue_t* rogue, const special_effect_t& e ) override
  { rogue -> legendary.the_curse_of_restlessness = e.driver(); }
};

struct the_empty_crown_t : public unique_gear::scoped_actor_callback_t<rogue_t>
{
  the_empty_crown_t() : super( ROGUE )
  { }

  void manipulate( rogue_t* rogue, const special_effect_t& e ) override
  { rogue -> legendary.the_empty_crown = e.driver(); }
};

struct the_first_of_the_dead_t : public unique_gear::scoped_actor_callback_t<rogue_t>
{
  the_first_of_the_dead_t() : super( ROGUE )
  { }

  void manipulate( rogue_t* rogue, const special_effect_t& e ) override
  { rogue -> legendary.the_first_of_the_dead = e.driver(); }
};

struct sephuzs_secret_t : public unique_gear::scoped_actor_callback_t<rogue_t>
{
  sephuzs_secret_t() : super( ROGUE )
  { }

  void manipulate( rogue_t* rogue, const special_effect_t& e ) override
  { rogue -> legendary.sephuzs_secret = e.driver(); }
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
    unique_gear::register_special_effect( 248107, the_curse_of_restlessness_t()         );
    unique_gear::register_special_effect( 248106, the_empty_crown_t()                   );
    unique_gear::register_special_effect( 248110, the_first_of_the_dead_t()             );
    unique_gear::register_special_effect( 208051, sephuzs_secret_t()                    );
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
