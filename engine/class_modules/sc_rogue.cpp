// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "player/covenant.hpp"
#include "util/util.hpp"

namespace { // UNNAMED NAMESPACE

// Forward Declarations
class rogue_t;

enum
{
  COMBO_POINT_MAX = 5
};

enum secondary_trigger_e
{
  TRIGGER_NONE = 0U,
  TRIGGER_SINISTER_STRIKE,
  TRIGGER_WEAPONMASTER,
  TRIGGER_SECRET_TECHNIQUE,
  TRIGGER_SHURIKEN_TORNADO,
  TRIGGER_REPLICATING_SHADOWS,
  TRIGGER_INTERNAL_BLEEDING,
  TRIGGER_AKAARIS_SOUL_FRAGMENT,
  TRIGGER_TRIPLE_THREAT,
  TRIGGER_CONCEALED_BLUNDERBUSS,
};

enum stealth_type_e
{
  STEALTH_NORMAL = 0x01,
  STEALTH_VANISH = 0x02,
  STEALTH_SHADOWMELD = 0x04,
  STEALTH_SUBTERFUGE = 0x08,
  STEALTH_SHADOWDANCE = 0x10,

  STEALTH_ROGUE = ( STEALTH_SUBTERFUGE | STEALTH_SHADOWDANCE ),   // Subterfuge + Shadowdance
  STEALTH_BASIC = ( STEALTH_NORMAL | STEALTH_VANISH ),            // Normal + Vanish

  STEALTH_ALL = 0xFF
};

namespace actions
{
  struct rogue_attack_t;
  struct rogue_heal_t;
  struct rogue_spell_t;

  struct rogue_poison_t;
  struct melee_t;
  struct shadow_blades_attack_t;
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
// Rogue Target Data
// ==========================================================================

class rogue_td_t : public actor_target_data_t
{
public:
  struct dots_t
  {
    dot_t* deadly_poison;
    dot_t* garrote;
    dot_t* internal_bleeding;
    dot_t* killing_spree; // Strictly speaking, this should probably be on player
    dot_t* rupture;
    dot_t* crimson_tempest;
    dot_t* nothing_personal;
    dot_t* sepsis;
    dot_t* slaughter_poison;
    dot_t* serrated_bone_spike;
    dot_t* mutilated_flesh;
  } dots;

  struct debuffs_t
  {
    buff_t* vendetta;
    buff_t* wound_poison;
    buff_t* crippling_poison;
    buff_t* numbing_poison;
    buff_t* marked_for_death;
    buff_t* ghostly_strike;
    buff_t* rupture; // Hidden proxy for handling Scent of Blood azerite trait
    buff_t* shiv;
    buff_t* find_weakness;
    buff_t* prey_on_the_weak;
    buff_t* between_the_eyes;
    buff_t* akaaris_soul_fragment;
  } debuffs;

  rogue_td_t( player_t* target, rogue_t* source );

  timespan_t lethal_poison_remains() const
  {
    if ( dots.deadly_poison->is_ticking() )
      return dots.deadly_poison->remains();

    if ( dots.slaughter_poison->is_ticking() )
      return dots.slaughter_poison->remains();

    if ( debuffs.wound_poison->check() )
      return debuffs.wound_poison->remains();

    return 0_s;
  }

  timespan_t non_lethal_poison_remains() const
  {
    if ( debuffs.crippling_poison->check() )
      return debuffs.crippling_poison->remains();

    if ( debuffs.numbing_poison->check() )
      return debuffs.numbing_poison->remains();

    return 0_s;
  }

  bool is_lethal_poisoned() const
  {
    return dots.deadly_poison->is_ticking() || dots.slaughter_poison->is_ticking() || debuffs.wound_poison->check();
  }

  bool is_non_lethal_poisoned() const
  {
    return debuffs.crippling_poison->check() || debuffs.numbing_poison->check();
  }

  bool is_poisoned() const
  {
    return is_lethal_poisoned() || is_non_lethal_poisoned();
  }

  bool is_bleeding() const
  {
    return dots.garrote->is_ticking() || dots.rupture->is_ticking() ||
      dots.crimson_tempest->is_ticking() || dots.internal_bleeding->is_ticking();
  }
};

// ==========================================================================
// Rogue
// ==========================================================================

class rogue_t : public player_t
{
public:
  // Shadow techniques swing counter;
  unsigned shadow_techniques;

  // Active
  struct
  {
    actions::rogue_attack_t* blade_flurry = nullptr;
    actions::rogue_poison_t* lethal_poison = nullptr;
    actions::rogue_poison_t* nonlethal_poison = nullptr;
    actions::rogue_attack_t* main_gauche = nullptr;
    actions::rogue_spell_t* poison_bomb = nullptr;
    actions::rogue_spell_t* replicating_shadows = nullptr;
    actions::shadow_blades_attack_t* shadow_blades_attack = nullptr;
    actions::rogue_attack_t* akaaris_soul_fragment = nullptr;
    actions::rogue_attack_t* bloodfang = nullptr;
    actions::rogue_attack_t* triple_threat_mh = nullptr;
    actions::rogue_attack_t* triple_threat_oh = nullptr;
    actions::rogue_attack_t* concealed_blunderbuss = nullptr;
    struct
    {
      actions::rogue_attack_t* backstab = nullptr;
      actions::rogue_attack_t* shadowstrike = nullptr;
    } weaponmaster;
  } active;

  // Autoattacks
  action_t* auto_attack;
  actions::melee_t* melee_main_hand;
  actions::melee_t* melee_off_hand;

  // Track target of last manual Rupture application for Replicating Shadows azerite power
  player_t* last_rupture_target;

  // Is using stealth during combat allowed? Relevant for Dungeon sims.
  bool restealth_allowed;

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
    buff_t* adrenaline_rush;
    buff_t* blade_flurry;
    buff_t* blade_rush;
    buff_t* opportunity;
    buff_t* roll_the_bones;
    // Roll the bones buffs
    buff_t* broadside;
    buff_t* buried_treasure;
    buff_t* grand_melee;
    buff_t* skull_and_crossbones;
    buff_t* ruthless_precision;
    buff_t* true_bearing;
    // Subtlety
    buff_t* shadow_blades;
    buff_t* shadow_dance;
    buff_t* symbols_of_death;
    buff_t* symbols_of_death_autocrit;

    // Talents
    // Shared
    buff_t* alacrity;
    buff_t* subterfuge;
    // Assassination
    buff_t* elaborate_planning;
    buff_t* blindside;
    buff_t* master_assassin;
    buff_t* master_assassin_aura;
    buff_t* hidden_blades_driver;
    buff_t* hidden_blades;
    // Outlaw
    buff_t* dreadblades;
    buff_t* killing_spree;
    buff_t* loaded_dice;
    buff_t* slice_and_dice;
    // Subtlety
    buff_t* premeditation;
    buff_t* master_of_shadows;
    buff_t* secret_technique; // Only to simplify APL tracking
    buff_t* shuriken_tornado;

    // Azerite powers
    buff_t* blade_in_the_shadows;
    buff_t* brigands_blitz;
    buff_t* brigands_blitz_driver;
    buff_t* deadshot;
    buff_t* double_dose; // Tracking Buff
    buff_t* keep_your_wits_about_you;
    buff_t* nights_vengeance;
    buff_t* nothing_personal;
    buff_t* paradise_lost;
    buff_t* perforate;
    buff_t* scent_of_blood;
    buff_t* snake_eyes;
    buff_t* the_first_dance;

    // Legendary
    buff_t* deathly_shadows;
    buff_t* concealed_blunderbuss;
    buff_t* finality_eviscerate;
    buff_t* finality_rupture;
    buff_t* finality_shadow_vault;
    buff_t* greenskins_wickers;
    buff_t* guile_charm_insight_1;
    buff_t* guile_charm_insight_2;
    buff_t* guile_charm_insight_3;
    buff_t* master_assassins_mark;
    buff_t* master_assassins_mark_aura;
    buff_t* the_rotten;

    // Covenant
    buff_t* echoing_reprimand_2;
    buff_t* echoing_reprimand_3;
    buff_t* echoing_reprimand_4;

    // Conduits
    buff_t* deeper_daggers;
    buff_t* perforated_veins;
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
    cooldown_t* blade_flurry;
    cooldown_t* blade_rush;
    cooldown_t* blind;
    cooldown_t* cloak_of_shadows;
    cooldown_t* dreadblades;
    cooldown_t* gouge;
    cooldown_t* riposte;
    cooldown_t* roll_the_bones;
    cooldown_t* ghostly_strike;
    cooldown_t* grappling_hook;
    cooldown_t* marked_for_death;
    cooldown_t* weaponmaster;
    cooldown_t* vendetta;
    cooldown_t* shiv;
    cooldown_t* symbols_of_death;
    cooldown_t* secret_technique;
    cooldown_t* shadow_blades;
    cooldown_t* sepsis;
    cooldown_t* serrated_bone_spike;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* adrenaline_rush;
    gain_t* adrenaline_rush_expiry;
    gain_t* blade_rush;
    gain_t* buried_treasure;
    gain_t* combat_potency;
    gain_t* energy_refund;
    gain_t* master_of_shadows;
    gain_t* memory_of_lucid_dreams;
    gain_t* nothing_personal;
    gain_t* vendetta;
    gain_t* venom_rush;
    gain_t* venomous_wounds;
    gain_t* venomous_wounds_death;
    gain_t* relentless_strikes;
    gain_t* symbols_of_death;

    // CP Gains
    gain_t* seal_fate;
    gain_t* quick_draw;
    gain_t* broadside;
    gain_t* ruthlessness;
    gain_t* shadow_techniques;
    gain_t* shadow_blades;
    gain_t* ace_up_your_sleeve;
    gain_t* shrouded_suffocation;
    gain_t* the_first_dance;
    gain_t* serrated_bone_spike;
    gain_t* dreadblades;
    gain_t* premeditation;

    // Legendary
    gain_t* dashing_scoundrel;
    gain_t* deathly_shadows;
    gain_t* the_rotten;
  } gains;

  // Spec passives
  struct spec_t
  {
    // Shared
    const spell_data_t* shadowstep;

    // Generic
    const spell_data_t* all_rogue;
    const spell_data_t* assassination_rogue;
    const spell_data_t* outlaw_rogue;
    const spell_data_t* subtlety_rogue;

    // Assassination
    const spell_data_t* deadly_poison;
    const spell_data_t* envenom;
    const spell_data_t* improved_poisons;
    const spell_data_t* improved_poisons_2;
    const spell_data_t* seal_fate;
    const spell_data_t* venomous_wounds;
    const spell_data_t* vendetta;
    const spell_data_t* master_assassin;
    const spell_data_t* garrote;
    const spell_data_t* garrote_2;
    const spell_data_t* shiv_2;
    const spell_data_t* shiv_2_debuff;
    const spell_data_t* slice_and_dice_2;
    const spell_data_t* wound_poison_2;

    // Outlaw
    const spell_data_t* adrenaline_rush;
    const spell_data_t* between_the_eyes;
    const spell_data_t* between_the_eyes_2;
    const spell_data_t* blade_flurry;
    const spell_data_t* blade_flurry_2;
    const spell_data_t* broadside;
    const spell_data_t* combat_potency;
    const spell_data_t* combat_potency_2;
    const spell_data_t* restless_blades;
    const spell_data_t* roll_the_bones;
    const spell_data_t* ruthlessness;
    const spell_data_t* ruthless_precision;
    const spell_data_t* sinister_strike;
    const spell_data_t* sinister_strike_2;

    // Subtlety
    const spell_data_t* backstab_2;
    const spell_data_t* deepening_shadows;
    const spell_data_t* find_weakness;
    const spell_data_t* relentless_strikes;
    const spell_data_t* shadow_blades;
    const spell_data_t* shadow_dance;
    const spell_data_t* shadow_techniques;
    const spell_data_t* shadow_techniques_effect;
    const spell_data_t* symbols_of_death;
    const spell_data_t* symbols_of_death_2;
    const spell_data_t* symbols_of_death_autocrit;
    const spell_data_t* eviscerate;
    const spell_data_t* eviscerate_2;
    const spell_data_t* shadowstrike;
    const spell_data_t* shadowstrike_2;
    const spell_data_t* shuriken_storm_2;
  } spec;

  // Spell Data
  struct spells_t
  {
    const spell_data_t* poison_bomb_driver;
    const spell_data_t* critical_strikes;
    const spell_data_t* fan_of_knives;
    const spell_data_t* fleet_footed;
    const spell_data_t* master_of_shadows;
    const spell_data_t* memory_of_lucid_dreams;
    const spell_data_t* nightstalker_dmg_amp;
    const spell_data_t* sprint;
    const spell_data_t* sprint_2;
    const spell_data_t* relentless_strikes_energize;
    const spell_data_t* ruthlessness_cp_driver;
    const spell_data_t* ruthlessness_driver;
    const spell_data_t* ruthlessness_cp;
    const spell_data_t* shadow_focus;
    const spell_data_t* subterfuge;
    const spell_data_t* slice_and_dice;
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

    const spell_data_t* prey_on_the_weak;

    const spell_data_t* alacrity;

    // Assassination
    const spell_data_t* master_poisoner;
    const spell_data_t* elaborate_planning;
    const spell_data_t* blindside;

    const spell_data_t* master_assassin;

    const spell_data_t* leeching_poison;

    const spell_data_t* internal_bleeding;

    const spell_data_t* venom_rush;
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
    const spell_data_t* dreadblades;

    const spell_data_t* dancing_steel;
    const spell_data_t* blade_rush;
    const spell_data_t* killing_spree;

    // Subtlety
    const spell_data_t* premeditation;
    const spell_data_t* gloomblade;

    const spell_data_t* shadow_focus;

    const spell_data_t* enveloping_shadows;
    const spell_data_t* dark_shadow;

    const spell_data_t* master_of_shadows;
    const spell_data_t* secret_technique;
    const spell_data_t* shuriken_tornado;
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
  struct azerite_t
  {
    azerite_power_t ace_up_your_sleeve;
    azerite_power_t blade_in_the_shadows;
    azerite_power_t brigands_blitz;
    azerite_power_t deadshot;
    azerite_power_t double_dose;
    azerite_power_t echoing_blades;
    azerite_power_t inevitability;
    azerite_power_t keep_your_wits_about_you;
    azerite_power_t nights_vengeance;
    azerite_power_t nothing_personal;
    azerite_power_t paradise_lost;
    azerite_power_t perforate;
    azerite_power_t replicating_shadows;
    azerite_power_t scent_of_blood;
    azerite_power_t shrouded_suffocation;
    azerite_power_t snake_eyes;
    azerite_power_t the_first_dance;
    azerite_power_t twist_the_knife;

    azerite_essence_t memory_of_lucid_dreams;
    azerite_essence_t vision_of_perfection;
    double            vision_of_perfection_percentage;
  } azerite;

  // Covenant powers
  struct covenant_t
  {
    const spell_data_t* echoing_reprimand;
    const spell_data_t* sepsis;
    const spell_data_t* serrated_bone_spike;
    const spell_data_t* slaughter;
  } covenant;

  // Conduits
  struct conduit_t
  {
    conduit_data_t lethal_poisons;
    conduit_data_t maim_mangle;
    conduit_data_t poisoned_katar;
    conduit_data_t well_placed_steel;

    conduit_data_t ambidexterity;
    conduit_data_t count_the_odds;
    conduit_data_t slight_of_hand;
    conduit_data_t triple_threat;

    conduit_data_t deeper_daggers;
    conduit_data_t perforated_veins;
    conduit_data_t planned_execution;
    conduit_data_t stiletto_staccato;

    conduit_data_t reverberation;
    conduit_data_t slaughter_scars;
    conduit_data_t sudden_fractures;
    conduit_data_t septic_shock;
  } conduit;

  // Legendary effects
  struct legendary_t
  {
    // Generic
    item_runeforge_t essence_of_bloodfang;
    item_runeforge_t master_assassins_mark;
    item_runeforge_t tiny_toxic_blades;       // NYI
    item_runeforge_t invigorating_shadowdust;

    // Assassination
    item_runeforge_t dashing_scoundrel;
    item_runeforge_t doomblade;
    item_runeforge_t dustwalkers_patch;
    item_runeforge_t zoldyck_insignia;

    // Outlaw
    item_runeforge_t greenskins_wickers;
    item_runeforge_t guile_charm;
    item_runeforge_t celerity;
    item_runeforge_t concealed_blunderbuss;
    
    // Subtlety
    item_runeforge_t akaaris_soul_fragment;
    item_runeforge_t deathly_shadows;
    item_runeforge_t finality;
    item_runeforge_t the_rotten;

    // Legendary Values
    double dashing_scoundrel_gain = 0.0;
    double dustwalkers_patch_counter = 0.0;
    int guile_charm_counter = 0;
  } legendary;

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

    // Subtlety
    proc_t* deepening_shadows;
    proc_t* weaponmaster;

    // Covenant
    proc_t* echoing_reprimand_2;
    proc_t* echoing_reprimand_3;
    proc_t* echoing_reprimand_4;

    // Conduits
    proc_t* count_the_odds;

    // Legendary
    proc_t* dustwalker_patch;
  } procs;

  // Options
  struct rogue_options_t
  {
    std::vector<size_t> fixed_rtb;
    std::vector<double> fixed_rtb_odds;
    int initial_combo_points = 0;
    bool rogue_optimize_expressions = true;
    bool rogue_ready_trigger = true;
    bool priority_rotation = false;
    double memory_of_lucid_dreams_proc_chance = -1.0;
  } options;
  
  rogue_t( sim_t* sim, util::string_view name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, ROGUE, name, r ),
    shadow_techniques( 0 ),
    auto_attack( nullptr ), melee_main_hand( nullptr ), melee_off_hand( nullptr ),
    last_rupture_target( nullptr ),
    restealth_allowed( false ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    spec( spec_t() ),
    spell( spells_t() ),
    talent( talents_t() ),
    mastery( masteries_t() ),
    procs( procs_t() ),
    options( rogue_options_t() )
  {
    // Cooldowns
    cooldowns.adrenaline_rush          = get_cooldown( "adrenaline_rush"          );
    cooldowns.garrote                  = get_cooldown( "garrote"                  );
    cooldowns.killing_spree            = get_cooldown( "killing_spree"            );
    cooldowns.shadow_dance             = get_cooldown( "shadow_dance"             );
    cooldowns.sprint                   = get_cooldown( "sprint"                   );
    cooldowns.vanish                   = get_cooldown( "vanish"                   );
    cooldowns.between_the_eyes         = get_cooldown( "between_the_eyes"         );
    cooldowns.blade_flurry             = get_cooldown( "blade_flurry"             );
    cooldowns.blade_rush               = get_cooldown( "blade_rush"               );
    cooldowns.blind                    = get_cooldown( "blind"                    );
    cooldowns.cloak_of_shadows         = get_cooldown( "cloak_of_shadows"         );
    cooldowns.gouge                    = get_cooldown( "gouge"                    );
    cooldowns.dreadblades              = get_cooldown( "dreadblades"              );
    cooldowns.ghostly_strike           = get_cooldown( "ghostly_strike"           );
    cooldowns.grappling_hook           = get_cooldown( "grappling_hook"           );
    cooldowns.marked_for_death         = get_cooldown( "marked_for_death"         );
    cooldowns.riposte                  = get_cooldown( "riposte"                  );
    cooldowns.roll_the_bones           = get_cooldown( "roll_the_bones"           );
    cooldowns.weaponmaster             = get_cooldown( "weaponmaster"             );
    cooldowns.vendetta                 = get_cooldown( "vendetta"                 );
    cooldowns.shiv                     = get_cooldown( "shiv"                     );
    cooldowns.symbols_of_death         = get_cooldown( "symbols_of_death"         );
    cooldowns.secret_technique         = get_cooldown( "secret_technique"         );
    cooldowns.shadow_blades            = get_cooldown( "shadow_blades"            );
    cooldowns.sepsis                   = get_cooldown( "sepsis"                   );
    cooldowns.serrated_bone_spike      = get_cooldown( "serrated_bone_spike"      );

    resource_regeneration = regen_type::DYNAMIC;
    regen_caches[CACHE_HASTE] = true;
    regen_caches[CACHE_ATTACK_HASTE] = true;
  }

  // Character Definition
  void        init_spells() override;
  void        init_base_stats() override;
  void        init_gains() override;
  void        init_procs() override;
  void        init_scaling() override;
  void        init_resources( bool force ) override;
  void        init_items() override;
  void        init_special_effects() override;
  void        init_finished() override;
  void        create_buffs() override;
  void        create_options() override;
  void        copy_from( player_t* source ) override;
  std::string create_profile( save_e stype ) override;
  void        init_action_list() override;
  void        reset() override;
  void        activate() override;
  void        arise() override;
  void        combat_begin() override;
  timespan_t  available() const override;
  action_t*   create_action( util::string_view name, const std::string& options ) override;
  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override;
  std::unique_ptr<expr_t> create_resource_expression( util::string_view name ) override;
  void        regen( timespan_t periodicity ) override;
  double      resource_gain( resource_e, double, gain_t* g = nullptr, action_t* a = nullptr ) override;
  resource_e  primary_resource() const override { return RESOURCE_ENERGY; }
  role_e      primary_role() const override  { return ROLE_ATTACK; }
  stat_e      convert_hybrid_stat( stat_e s ) const override;
  void        vision_of_perfection_proc() override;

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
  double    composite_leech() const override;
  double    matching_gear_multiplier( attribute_e attr ) const override;
  double    composite_player_multiplier( school_e school ) const override;
  double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double    resource_regen_per_second( resource_e ) const override;
  double    passive_movement_modifier() const override;
  double    temporary_movement_modifier() const override;
  void      apply_affecting_auras( action_t& action ) override;

  bool is_target_poisoned( player_t* target, bool deadly_fade = false ) const;

  void break_stealth();
  void cancel_auto_attack();

  // On-death trigger for Venomous Wounds energy replenish
  void trigger_venomous_wounds_death( player_t* );

  double consume_cp_max() const
  { return COMBO_POINT_MAX + as<double>( talent.deeper_stratagem -> effectN( 1 ).base_value() ); }

  target_specific_t<rogue_td_t> target_data;

  rogue_td_t* get_target_data( player_t* target ) const override
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
  bool stealthed( uint32_t stealth_mask = STEALTH_ALL ) const;

  // Secondary Action Tracking
private:
  std::vector<action_t*> background_actions;

public:
  template <typename T, typename... Ts>
  T* get_background_action( util::string_view n, Ts&&... args )
  {
    auto it = range::find( background_actions, n, &action_t::name_str );
    if ( it != background_actions.cend() )
    {
      return dynamic_cast<T*>( *it );
    }

    auto action = new T( n, this, std::forward<Ts>( args )... );
    action->background = true;
    background_actions.push_back( action );
    return action;
  }

  // Secondary Action Trigger Tracking
private:
  std::vector<action_t*> secondary_trigger_actions;

public:
  template <typename T, typename... Ts>
  T* find_secondary_trigger_action( secondary_trigger_e source )
  {
    T* found_action = nullptr;
    for ( auto action : secondary_trigger_actions )
    {
      found_action = dynamic_cast<T*>( action );
      if ( found_action && found_action->secondary_trigger == source )
        break;
    }
    return found_action;
  }

  template <typename T, typename... Ts>
  T* get_secondary_trigger_action( secondary_trigger_e source, util::string_view n, Ts&&... args )
  {
    T* found_action = find_secondary_trigger_action<T>( source );
    if ( !found_action )
    {
      // Actions will follow form of foo_t( util::string_view name, rogue_t* p, ... )
      found_action = new T( n, this, std::forward<Ts>( args )... );
      found_action->background = found_action->dual = true;
      found_action->repeating = false;
      found_action->secondary_trigger = source;
      secondary_trigger_actions.push_back( found_action );
    }
    return found_action;
  }
};

namespace actions { // namespace actions

// ==========================================================================
// Secondary Action Triggers
// ==========================================================================

template<typename Base>
struct secondary_action_trigger_t : public event_t
{
  Base* action;
  action_state_t* state;
  player_t* target;
  int cp;

  secondary_action_trigger_t( action_state_t* s, timespan_t delay = timespan_t::zero() ) :
    event_t( *s -> action -> sim, delay ), action( dynamic_cast<Base*>( s->action ) ), state( s ), target( nullptr ), cp( 0 )
  {}

  secondary_action_trigger_t( player_t* target, Base* action, int cp, timespan_t delay = timespan_t::zero() ) :
    event_t( *action -> sim, delay ), action( action ), state( nullptr ), target( target ), cp( cp )
  {}

  const char* name() const override
  { return "secondary_action_trigger"; }

  void execute() override
  {
    assert( action->is_secondary_action() );

    player_t* action_target = state ? state->target : target;

    // Ensure target is still available and did not demise during delay.
    if ( !action_target || action_target->is_sleeping() )
      return;

    action->set_target( action_target );

    // No state, construct one and grab combo points from the event instead of current CP amount.
    if ( !state )
    {
      state = action->get_state();
      action->cast_state( state )->cp = cp;
      // Calling snapshot_internal, snapshot_state would overwrite CP.
      action->snapshot_internal( state, action->snapshot_flags, action->amount_type( state ) );
    }

    action->pre_execute_state = state;
    action->execute();
    state = nullptr;
  }

  ~secondary_action_trigger_t() override
  { if ( state ) action_state_t::release( state ); }
};

// ==========================================================================
// Rogue Action State
// ==========================================================================

struct rogue_action_state_t : public action_state_t
{
  int cp;
  bool exsanguinated;

  rogue_action_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ), cp( 0 ), exsanguinated( false )
  {}

  void initialize() override
  {
    action_state_t::initialize();
    cp = 0;
    exsanguinated = false;
  }

  int get_combo_points( bool echoing_reprimand = true ) const
  {
    if ( echoing_reprimand )
    {
      const rogue_t* rogue = debug_cast<rogue_t*>( action->player );
      if ( rogue->covenant.echoing_reprimand->ok() )
      {
        if ( ( cp == 2 && rogue->buffs.echoing_reprimand_2->check() ) ||
             ( cp == 3 && rogue->buffs.echoing_reprimand_3->check() ) ||
             ( cp == 4 && rogue->buffs.echoing_reprimand_4->check() ) )
        {
          return as<int>( rogue->covenant.echoing_reprimand->effectN( 2 ).base_value() );
        }
      }
    }

    return cp;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s ) << " cp=" << cp << " exsanguinated=" << exsanguinated;
    return s;
  }

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );
    const rogue_action_state_t* rs = debug_cast<const rogue_action_state_t*>( s );
    cp = rs->cp;
    exsanguinated = rs->exsanguinated;
  }
};

// ==========================================================================
// Rogue Action Template
// ==========================================================================

template <typename Base>
class rogue_action_t : public Base
{
protected:
  /// typedef for rogue_action_t<action_base_t>
  using base_t = rogue_action_t<Base>;

private:
  /// typedef for the templated action type, eg. spell_t, attack_t, heal_t
  using ab = Base;
  bool _requires_stealth;

public:
  // Secondary triggered ability, due to Weaponmaster talent or Death from Above. Secondary
  // triggered abilities cost no resources or incur cooldowns.
  secondary_trigger_e secondary_trigger;

  // Affect flags for various dynamic effects
  struct damage_affect_data
  {
    bool direct = false;
    double direct_percent = 0.0;
    bool periodic = false;
    double periodic_percent = 0.0;
  };
  struct
  {
    bool shadow_blades = false;
    bool ruthlessness = false;
    bool relentless_strikes = false;
    bool deepening_shadows = false;
    bool vendetta = false;
    bool alacrity = false;
    bool adrenaline_rush_gcd = false;
    bool broadside_cp = false;
    bool master_assassin = false;
    bool shiv_2 = false;
    bool ruthless_precision = false;
    bool symbols_of_death_autocrit = false;
    bool perforated_veins = false;
    bool blindside = false;
    bool finality_eviscerate = false;
    bool finality_shadow_vault = false;
    bool dashing_scoundrel = false;
    bool zoldyck_insignia = false;

    damage_affect_data mastery_executioner;
    damage_affect_data mastery_potent_assassin;
    damage_affect_data nightstalker_dmg_amp;
    damage_affect_data broadside;
    damage_affect_data symbols_of_death;
    damage_affect_data shadow_dance;
    damage_affect_data elaborate_planning;
    damage_affect_data deathly_shadows;
  } affected_by;

  // Init =====================================================================

  rogue_action_t( util::string_view n, rogue_t* p, const spell_data_t* s = spell_data_t::nil(),
                  const std::string& options = std::string() )
    : ab( n, p, s ),
    _requires_stealth( false ),
    secondary_trigger( TRIGGER_NONE )
  {
    ab::parse_options( options );
    parse_spell_data( s );

    // rogue_t sets base and min GCD to 1s by default but let's also enforce non-hasted GCDs.
    // Even for rogue abilities that can be considered spells, hasted GCDs seem to be an exception rather than rule.
    // Those should be set explicitly. (see Vendetta, Shadow Blades, Detection)
    ab::gcd_type = gcd_haste_type::NONE;

    // Affecting Passive Auras
    // Put ability specific ones here; class/spec wide ones with labels that can effect things like trinkets in rogue_t::apply_affecting_auras.
    ab::apply_affecting_aura( p->spec.between_the_eyes_2 );
    ab::apply_affecting_aura( p->spell.sprint_2 );
    ab::apply_affecting_aura( p->talent.deeper_stratagem );
    ab::apply_affecting_aura( p->talent.master_poisoner );
    ab::apply_affecting_aura( p->talent.dancing_steel );

    // Affecting Passive Conduits
    ab::apply_affecting_conduit( p->conduit.lethal_poisons );
    ab::apply_affecting_conduit( p->conduit.poisoned_katar );
    ab::apply_affecting_conduit( p->conduit.reverberation );

    // Dynamically affected flags
    // Special things like CP, Energy, Crit, etc.
    bool costs_combo_points = ab::base_costs[ RESOURCE_COMBO_POINT ] > 0;
    affected_by.shadow_blades = ab::data().affected_by( p->spec.shadow_blades->effectN( 2 ) ) ||
      ab::data().affected_by( p->spec.shadow_blades->effectN( 3 ) );
    affected_by.ruthlessness = costs_combo_points;
    affected_by.relentless_strikes = costs_combo_points;
    affected_by.deepening_shadows = costs_combo_points;
    affected_by.vendetta = ab::data().affected_by( p->spec.vendetta->effectN( 1 ) );
    affected_by.alacrity = costs_combo_points;
    affected_by.adrenaline_rush_gcd = ab::data().affected_by( p->spec.adrenaline_rush->effectN( 3 ) );
    affected_by.master_assassin = ab::data().affected_by( p->spec.master_assassin->effectN( 1 ) );
    affected_by.shiv_2 = ab::data().affected_by( p->spec.shiv_2_debuff->effectN( 1 ) );
    affected_by.broadside_cp = ab::data().affected_by( p->spec.broadside->effectN( 1 ) ) ||
      ab::data().affected_by( p->spec.broadside->effectN( 2 ) ) ||
      ab::data().affected_by( p->spec.broadside->effectN( 3 ) );
    affected_by.ruthless_precision = ab::data().affected_by( p->spec.ruthless_precision->effectN( 1 ) );
    affected_by.symbols_of_death_autocrit = ab::data().affected_by( p->spec.symbols_of_death_autocrit->effectN( 1 ) );
    affected_by.blindside = ab::data().affected_by( p->find_spell( 121153 )->effectN( 1 ) );
    if ( p->conduit.perforated_veins.ok() )
    {
      affected_by.perforated_veins = ab::data().affected_by( p->conduit.perforated_veins->effectN( 1 ).trigger()->effectN( 1 ) );
    }
    if ( p->legendary.finality.ok() )
    {
      affected_by.finality_eviscerate = ab::data().affected_by( p->find_spell( 340600 )->effectN( 1 ) );
      affected_by.finality_shadow_vault = ab::data().affected_by( p->find_spell( 340603 )->effectN( 1 ) );
    }
    if ( p->legendary.dashing_scoundrel.ok() )
    {
      affected_by.dashing_scoundrel = ab::data().affected_by( p->spec.envenom->effectN( 3 ) );
    }
    if ( p->legendary.zoldyck_insignia.ok() )
    {
      // TOCHECK: This is just temporary, using the periodic whitelist from mastery until we are clear what it should work on
      //          Currently on beta, it doesn't work on much of anything, so this is quite unclear
      affected_by.zoldyck_insignia = ab::data().affected_by( p->mastery.potent_assassin->effectN( 1 ) ) ||
                                     ab::data().affected_by( p->mastery.potent_assassin->effectN( 2 ) );
    }

    // Auto-parsing for damage affecting dynamic flags, this reads IF direct/periodic dmg is affected and stores by how much.
    // Still requires manual impl below but removes need to hardcode effect numbers.
    parse_damage_affecting_spell( p->mastery.executioner, affected_by.mastery_executioner );
    parse_damage_affecting_spell( p->mastery.potent_assassin, affected_by.mastery_potent_assassin );
    parse_damage_affecting_spell( p->spec.broadside, affected_by.broadside );
    parse_damage_affecting_spell( p->spec.symbols_of_death, affected_by.symbols_of_death );
    parse_damage_affecting_spell( p->spec.shadow_dance, affected_by.shadow_dance );
    parse_damage_affecting_spell( p->talent.elaborate_planning->effectN( 1 ).trigger(), affected_by.elaborate_planning );
    if ( p->talent.nightstalker->ok() )
      parse_damage_affecting_spell( p->spell.nightstalker_dmg_amp, affected_by.nightstalker_dmg_amp );
    if ( p->legendary.deathly_shadows->ok() )
      parse_damage_affecting_spell( p->legendary.deathly_shadows->effectN( 1 ).trigger(), affected_by.deathly_shadows );
  }

  // Type Wrappers ============================================================

  static const rogue_action_state_t* cast_state( const action_state_t* st )
  { return debug_cast<const rogue_action_state_t*>( st ); }

  static rogue_action_state_t* cast_state( action_state_t* st )
  { return debug_cast<rogue_action_state_t*>( st ); }

  rogue_t* p()
  { return debug_cast<rogue_t*>( ab::player ); }
  
  const rogue_t* p() const
  { return debug_cast<const rogue_t*>( ab::player ); }

  rogue_td_t* td( player_t* t ) const
  { return p()->get_target_data( t ); }

  // Spell Data Helpers =======================================================

  void parse_spell_data( const spell_data_t* s )
  {
    if ( s->stance_mask() & 0x20000000)
      _requires_stealth = true;

    for ( size_t i = 1; i <= s->effect_count(); i++ )
    {
      const spelleffect_data_t& effect = s->effectN( i );

      switch ( effect.type() )
      {
        case E_ADD_COMBO_POINTS:
          if ( ab::energize_type != action_energize::NONE )
          {
            ab::energize_type = action_energize::ON_HIT;
            ab::energize_amount = effect.base_value();
            ab::energize_resource = RESOURCE_COMBO_POINT;
          }
          break;
        default:
          break;
      }

      if ( effect.type() == E_APPLY_AURA && effect.subtype() == A_PERIODIC_DAMAGE )
      {
        ab::base_ta_adder = effect.bonus( p() );
      }
      else if ( effect.type() == E_SCHOOL_DAMAGE )
      {
        ab::base_dd_adder = effect.bonus( p() );
      }
    }
  }

  void parse_damage_affecting_spell( const spell_data_t* spell, damage_affect_data& flags )
  {
    for ( const spelleffect_data_t& effect : spell->effects() )
    {
      if ( !effect.ok() || effect.type() != E_APPLY_AURA || effect.subtype() != A_ADD_PCT_MODIFIER )
        return;

      if ( ab::data().affected_by( effect ) )
      {
        switch ( effect.misc_value1() )
        {
          case P_GENERIC:
            flags.direct = true;
            flags.direct_percent = effect.percent();
            break;
          case P_TICK_DAMAGE:
            flags.periodic = true;
            flags.periodic_percent = effect.percent();
            break;
        }
      }
    }
  }

  // Action State =============================================================

  action_state_t* new_state() override
  {
    return new rogue_action_state_t( this, ab::target );
  }

  void snapshot_internal( action_state_t* state, unsigned flags, result_amount_type rt ) override
  {
    // Exsanguinated bleeds snapshot hasted tick time when the ticks are rescheduled.
    // This will make snapshot_internal on haste updates discard the new value.
    if ( cast_state( state )->exsanguinated )
      flags &= ~STATE_HASTE;

    ab::snapshot_internal( state, flags, rt );
  }

  void snapshot_state( action_state_t* state, result_amount_type rt ) override
  {
    double max_cp = std::min( p()->resources.current[ RESOURCE_COMBO_POINT ], p()->consume_cp_max() );
    cast_state( state )->cp = static_cast<int>( max_cp );

    ab::snapshot_state( state, rt );
  }

  // Secondary Trigger Functions ==============================================

  bool is_secondary_action()
  { return secondary_trigger != TRIGGER_NONE && ab::background == true; }

  virtual void trigger_secondary_action( player_t* target, int cp = 0, timespan_t delay = timespan_t::zero() )
  {
    assert( is_secondary_action() );
    make_event<secondary_action_trigger_t<base_t>>( *ab::sim, target, this, cp, delay );
  }

  virtual void trigger_secondary_action( action_state_t* s, timespan_t delay = timespan_t::zero() )
  {
    assert( is_secondary_action() && s->action == this );
    make_event<secondary_action_trigger_t<base_t>>( *ab::sim, s, delay );
  }

  // Helper Functions =========================================================

  // Helper function for expressions. Returns the number of guaranteed generated combo points for
  // this ability, taking into account any potential buffs.
  virtual double generate_cp() const
  {
    double cp = 0;

    if ( ab::energize_type != action_energize::NONE && ab::energize_resource == RESOURCE_COMBO_POINT )
    {
      cp += ab::energize_amount;
    }

    if ( cp > 0 )
    {
      if ( affected_by.broadside_cp && p()->buffs.broadside->check() )
      {
        cp += p()->buffs.broadside->data().effectN( 2 ).base_value();
      }

      if ( affected_by.shadow_blades && p()->buffs.shadow_blades->check() )
      {
        cp += p()->buffs.shadow_blades->data().effectN( 2 ).base_value();
      }
    }

    return cp;
  }

  virtual bool procs_poison() const
  { return ab::weapon != nullptr; }

  // Generic rules for proccing Main Gauche, used by rogue_t::trigger_main_gauche()
  virtual bool procs_main_gauche() const
  { return ab::callbacks && !ab::proc && ab::weapon != nullptr && ab::weapon->slot == SLOT_MAIN_HAND; }

  // Generic rules for proccing Combat Potency, used by rogue_t::trigger_combat_potency()
  virtual bool procs_combat_potency() const
  { return ab::callbacks && !ab::proc && ab::weapon != nullptr && ab::weapon->slot == SLOT_OFF_HAND; }

  virtual double proc_chance_main_gauche() const
  { return p()->mastery.main_gauche->proc_chance(); }

  // Generic rules for snapshotting the Nightstalker pmultiplier, default to DoTs only.
  // If a DoT with DD component is whitelisted in the direct damage effect #1 on 130493 this can double dip.
  virtual bool snapshots_nightstalker() const
  {
    return ab::dot_duration > timespan_t::zero() && ab::base_tick_time > timespan_t::zero();
  }

  // Overridable wrapper for checking stealth requirement
  virtual bool requires_stealth() const
  { 
    if ( affected_by.blindside && p()->buffs.blindside->check() )
      return false;

    return _requires_stealth; 
  }

private:
  void do_exsanguinate( dot_t* dot, double coeff );

public:
  // Ability triggers
  void spend_combo_points( const action_state_t* );
  void trigger_auto_attack( const action_state_t* );
  void trigger_seal_fate( const action_state_t* );
  void trigger_main_gauche( const action_state_t* );
  void trigger_combat_potency( const action_state_t* );
  void trigger_energy_refund( const action_state_t* );
  void trigger_poison_bomb( const action_state_t* );
  void trigger_venomous_wounds( const action_state_t* );
  void trigger_blade_flurry( const action_state_t* );
  void trigger_ruthlessness_cp( const action_state_t* );
  void trigger_combo_point_gain( int, gain_t* gain = nullptr );
  void trigger_elaborate_planning( const action_state_t* );
  void trigger_alacrity( const action_state_t* );
  void trigger_deepening_shadows( const action_state_t* );
  void trigger_shadow_techniques( const action_state_t* );
  void trigger_weaponmaster( const action_state_t*, rogue_attack_t* action );
  void trigger_restless_blades( const action_state_t* );
  void trigger_dreadblades( const action_state_t* );
  void trigger_exsanguinate( const action_state_t* );
  void trigger_relentless_strikes( const action_state_t* );
  void trigger_shadow_blades_attack( action_state_t* );
  void trigger_inevitability( const action_state_t* state );
  void trigger_prey_on_the_weak( const action_state_t* state );
  void trigger_find_weakness( const action_state_t* state, timespan_t duration = timespan_t::min() );
  void trigger_grand_melee( const action_state_t* state );
  void trigger_master_of_shadows();
  void trigger_akaaris_soul_fragment( const action_state_t* state );
  void trigger_bloodfang( const action_state_t* state );
  void trigger_count_the_odds( const action_state_t* state );
  void trigger_guile_charm( const action_state_t* state );

  // General Methods ==========================================================

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    if ( secondary_trigger != TRIGGER_NONE )
    {
      cd_duration = timespan_t::zero();
    }

    ab::update_ready( cd_duration );
  }

  timespan_t gcd() const override
  {
    timespan_t t = ab::gcd();

    if ( affected_by.adrenaline_rush_gcd && t != timespan_t::zero() && p()->buffs.adrenaline_rush->check() )
    {
      t += p()->buffs.adrenaline_rush->data().effectN( 3 ).time_value();
    }

    return t;
  }

  virtual double combo_point_da_multiplier( const action_state_t* s ) const
  {
    if ( ab::base_costs[ RESOURCE_COMBO_POINT ] )
      return static_cast<double>( cast_state( s )->get_combo_points() );

    return 1.0;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = ab::composite_da_multiplier( state );

    m *= combo_point_da_multiplier( state );

    // Subtlety
    if ( affected_by.mastery_executioner.direct )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.symbols_of_death.direct && p()->buffs.symbols_of_death->up() )
    {
      m *= 1.0 + affected_by.symbols_of_death.direct_percent;
    }

    if ( affected_by.shadow_dance.direct && p()->buffs.shadow_dance->up() )
    {
      m *= 1.0 + affected_by.shadow_dance.direct_percent + p()->talent.dark_shadow->effectN( 2 ).percent();
    }

    if ( affected_by.perforated_veins )
    {
      m *= 1.0 + p()->buffs.perforated_veins->stack_value();
    }

    if ( affected_by.finality_eviscerate )
    {
      m *= 1.0 + p()->buffs.finality_eviscerate->value();
    }

    if ( affected_by.finality_shadow_vault )
    {
      m *= 1.0 + p()->buffs.finality_shadow_vault->value();
    }

    // Assassination
    if ( affected_by.mastery_potent_assassin.direct )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.elaborate_planning.direct && p()->buffs.elaborate_planning->up() )
    {
      m *= 1.0 + affected_by.elaborate_planning.direct_percent;
    }

    // Outlaw
    if ( affected_by.broadside.direct && p()->buffs.broadside->up() )
    {
      m *= 1.0 + affected_by.broadside.direct_percent;
    }

    // General
    if ( affected_by.deathly_shadows.direct && p()->buffs.deathly_shadows->up() )
    {
      m *= 1.0 + affected_by.deathly_shadows.direct_percent;
    }

    // Apply Nightstalker direct damage increase via the corresponding driver spell.
    // And yes, this can cause double dips with the persistent multiplier on DoTs which is the case with Crimson Tempest.
    if ( affected_by.nightstalker_dmg_amp.direct && p()->stealthed( STEALTH_BASIC | STEALTH_SHADOWDANCE ) )
    {
      m *= 1.0 + ( affected_by.nightstalker_dmg_amp.direct_percent + p()->spec.subtlety_rogue->effectN( 4 ).percent() );
    }

    return m;
  }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = ab::composite_ta_multiplier( state );

    // Subtlety
    if ( affected_by.mastery_executioner.periodic )
    {
      // 08/17/2018 - Mastery: Executioner has a different coefficient for periodic
      if ( p()->bugs )
        m *= 1.0 + p()->cache.mastery() * p()->mastery.executioner->effectN( 2 ).mastery_value();
      else
        m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.symbols_of_death.periodic && p()->buffs.symbols_of_death->up() )
    {
      m *= 1.0 + affected_by.symbols_of_death.periodic_percent;
    }

    if ( affected_by.shadow_dance.periodic && p()->buffs.shadow_dance->up() )
    {
      m *= 1.0 + affected_by.shadow_dance.periodic_percent + p()->talent.dark_shadow->effectN( 3 ).percent();
    }

    // Assassination
    if ( affected_by.mastery_potent_assassin.periodic )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.elaborate_planning.periodic && p()->buffs.elaborate_planning->up() )
    {
      m *= 1.0 + affected_by.elaborate_planning.periodic_percent;
    }

    // Outlaw
    if ( affected_by.broadside.periodic && p()->buffs.broadside->up() )
    {
      m *= 1.0 + affected_by.broadside.periodic_percent;
    }

    // General
    if ( affected_by.deathly_shadows.periodic && p()->buffs.deathly_shadows->up() )
    {
      m *= 1.0 + affected_by.deathly_shadows.periodic_percent;
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = ab::composite_target_multiplier( target );

    if ( affected_by.vendetta )
    {
      m *= 1.0 + td( target )->debuffs.vendetta->value();
    }

    if ( affected_by.shiv_2 )
    {
      m *= 1.0 + td( target )->debuffs.shiv->value();
    }

    if ( affected_by.zoldyck_insignia && target->health_percentage() < p()->legendary.zoldyck_insignia->effectN( 2 ).base_value() )
    {
      m *= 1.0 + p()->legendary.zoldyck_insignia->effectN( 1 ).percent();
    }

    return m;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = ab::composite_persistent_multiplier( state );

    // Apply Nightstalker as a Persistent Multiplier for things that snapshot
    // This appears to be driven by the dummy effect #2 and there is no whitelist.
    // This can and will cause double dips on direct damage if a spell is whitelisted in effect #1.
    if ( p()->talent.nightstalker->ok() && snapshots_nightstalker() && p()->stealthed( STEALTH_BASIC | STEALTH_SHADOWDANCE ) )
    {
      double ns_mod = p()->spell.nightstalker_dmg_amp->effectN( 2 ).percent();
      ns_mod += p()->spec.subtlety_rogue->effectN( 4 ).percent();
      m *= 1.0 + ns_mod;
    }

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = ab::composite_crit_chance();

    if ( affected_by.master_assassin )
    {
      c += p()->buffs.master_assassin->stack_value();
      c += p()->buffs.master_assassin_aura->stack_value();
    }

    if ( affected_by.ruthless_precision )
    {
      c += p()->buffs.ruthless_precision->stack_value();
    }

    if ( affected_by.symbols_of_death_autocrit )
    {
      c += p()->buffs.symbols_of_death_autocrit->stack_value();
    }

    if ( affected_by.dashing_scoundrel && p()->buffs.envenom->check() )
    {
      c += p()->legendary.dashing_scoundrel->effectN( 1 ).percent();
    }

    return c;
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = ab::composite_target_crit_chance( target );
    c += td( target )->debuffs.between_the_eyes->stack_value();
    return c;
  }

  double target_armor( player_t* target ) const override
  {
    double a = ab::target_armor( target );
    
    a *= 1.0 - td( target )->debuffs.find_weakness->value();
    
    return a;
  }

  timespan_t tick_time( const action_state_t* state ) const override
  {
    timespan_t tt = ab::tick_time( state );

    if ( cast_state( state )->exsanguinated )
    {
      tt *= 1.0 / ( 1.0 + p()->talent.exsanguinate->effectN( 1 ).percent() );
    }

    return tt;
  }

  virtual double composite_poison_flat_modifier( const action_state_t* ) const
  { return 0.0; }

  double cost() const override
  {
    double c = ab::cost();

    if ( c <= 0 )
      return 0;

    if ( p()->talent.shadow_focus->ok() && p()->stealthed( STEALTH_BASIC | STEALTH_SHADOWDANCE ) )
    {
      c *= 1.0 + p()->spell.shadow_focus->effectN( 1 ).percent();
    }

    if ( affected_by.blindside )
    {
      c *= 1.0 + p()->buffs.blindside->check_value();
    }

    if ( c <= 0 )
      c = 0;

    return c;
  }

  void consume_resource() override
  {
    // Abilities triggered as part of another ability (secondary triggers) do not consume resources
    if ( secondary_trigger != TRIGGER_NONE )
    {
      return;
    }

    ab::consume_resource();

    spend_combo_points( ab::execute_state );

    if ( ab::result_is_miss( ab::execute_state->result ) && ab::last_resource_cost > 0 )
    {
      trigger_energy_refund( ab::execute_state );
    }
    else
    {
      if ( ab::current_resource() == RESOURCE_ENERGY && ab::last_resource_cost > 0 )
      {
        // Dustwalker's Patch Legendary
        if ( p()->legendary.dustwalkers_patch.ok() )
        {
          p()->legendary.dustwalkers_patch_counter += ab::last_resource_cost;
          if ( p()->legendary.dustwalkers_patch_counter > p()->legendary.dustwalkers_patch->effectN( 2 ).base_value() )
          {
            p()->cooldowns.vendetta->adjust( -timespan_t::from_seconds( p()->legendary.dustwalkers_patch->effectN( 1 ).base_value() ) );
            p()->legendary.dustwalkers_patch_counter -= p()->legendary.dustwalkers_patch->effectN( 2 ).base_value();
            p()->procs.dustwalker_patch->occur();
          }
        }

        // Memory of Lucid Dreams
        if ( p()->azerite.memory_of_lucid_dreams.enabled() )
        {
          if ( p()->rng().roll( p()->options.memory_of_lucid_dreams_proc_chance ) )
          {
            // Gains are rounded up to the nearest whole value, which can be seen with the Lucid Dreams active up
            const double amount = ceil( ab::last_resource_cost * p()->spell.memory_of_lucid_dreams->effectN( 1 ).percent() );
            p()->resource_gain( RESOURCE_ENERGY, amount, p()->gains.memory_of_lucid_dreams );

            if ( p()->azerite.memory_of_lucid_dreams.rank() >= 3 )
            {
              p()->player_t::buffs.lucid_dreams->trigger();
            }
          }
        }
      }
    }
  }

  void execute() override
  {
    ab::execute();

    if ( ab::harmful )
      p()->restealth_allowed = false;

    trigger_auto_attack( ab::execute_state );
    trigger_ruthlessness_cp( ab::execute_state );

    if ( ab::energize_type != action_energize::NONE && ab::energize_resource == RESOURCE_COMBO_POINT )
    {
      if ( affected_by.shadow_blades && p()->buffs.shadow_blades->up() )
      {
        trigger_combo_point_gain( as<int>( p()->buffs.shadow_blades->data().effectN( 2 ).base_value() ), p()->gains.shadow_blades );
      }

      if ( affected_by.broadside_cp && p()->buffs.broadside->up() )
      {
        trigger_combo_point_gain( as<int>( p()->buffs.broadside->data().effectN( 2 ).base_value() ), p()->gains.broadside );
      }
    }

    trigger_dreadblades( ab::execute_state );
    trigger_relentless_strikes( ab::execute_state );
    trigger_elaborate_planning( ab::execute_state );
    trigger_alacrity( ab::execute_state );

    if ( ab::harmful && p()->stealthed( STEALTH_BASIC | STEALTH_SHADOWMELD ) )
    {
      ab::player->buffs.shadowmeld->expire();

      // Check stealthed again after shadowmeld is popped. If we're still stealthed, trigger subterfuge
      if ( p()->talent.subterfuge->ok() && !p()->buffs.subterfuge->check() && p()->stealthed( STEALTH_BASIC ) )
        p()->buffs.subterfuge->trigger();
      else
        p()->break_stealth();
    }

    trigger_deepening_shadows( ab::execute_state );

    if ( affected_by.symbols_of_death_autocrit )
      p()->buffs.symbols_of_death_autocrit->expire();

    if ( affected_by.perforated_veins )
      p()->buffs.perforated_veins->expire();

    if ( affected_by.blindside )
      p()->buffs.blindside->expire();
  }

  void schedule_travel( action_state_t* state ) override
  {
    ab::schedule_travel( state );

    if ( ab::energize_type != action_energize::NONE && ab::energize_resource == RESOURCE_COMBO_POINT )
      trigger_seal_fate( state );
  }

  bool ready() override
  {
    if ( !ab::ready() )
      return false;

    if ( ab::base_costs[ RESOURCE_COMBO_POINT ] > 0 &&
      ab::player->resources.current[ RESOURCE_COMBO_POINT ] < ab::base_costs[ RESOURCE_COMBO_POINT ] )
      return false;

    if ( requires_stealth() && !p()->stealthed() )
    {
      return false;
    }

    return true;
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override;
};

// ==========================================================================
// Rogue Attack Classes
// ==========================================================================

struct rogue_heal_t : public rogue_action_t<heal_t>
{
  rogue_heal_t( util::string_view n, rogue_t* p,
                       const spell_data_t* s = spell_data_t::nil(),
                       const std::string& o = std::string() )
    : base_t( n, p, s, o )
  {
    harmful = false;
    set_target( p );
  }
};

struct rogue_spell_t : public rogue_action_t<spell_t>
{
  rogue_spell_t( util::string_view n, rogue_t* p,
                        const spell_data_t* s = spell_data_t::nil(),
                        const std::string& o = std::string() )
    : base_t( n, p, s, o )
  {}
};

struct rogue_attack_t : public rogue_action_t<melee_attack_t>
{
  rogue_attack_t( util::string_view n, rogue_t* p,
                         const spell_data_t* s = spell_data_t::nil(),
                         const std::string& o = std::string() )
    : base_t( n, p, s, o )
  {
    special = true;
  }

  void impact( action_state_t* state ) override;
};

// ==========================================================================
// Poisons
// ==========================================================================

struct rogue_poison_t : public rogue_attack_t
{
  double base_proc_chance;

  rogue_poison_t( util::string_view name, rogue_t* p, const spell_data_t* s = spell_data_t::nil(), bool triggers_procs = false ) :
    actions::rogue_attack_t( name, p, s ),
    base_proc_chance( 0.0 )
  {
    background = dual = true;
    proc = !triggers_procs;
    callbacks = triggers_procs;

    trigger_gcd = timespan_t::zero();

    base_proc_chance = data().proc_chance();
    if ( s->affected_by( p->spec.improved_poisons->effectN( 1 ) ) )
    {
      base_proc_chance += p->spec.improved_poisons->effectN( 1 ).percent();
    }
  }

  timespan_t execute_time() const override
  {
    return timespan_t::zero();
  }

  virtual double proc_chance( const action_state_t* source_state ) const
  {
    if ( base_proc_chance == 0.0 )
      return 0.0;

    if ( p()->spec.improved_poisons_2->ok() && p()->stealthed( STEALTH_BASIC | STEALTH_ROGUE ) )
    {
      return p()->spec.improved_poisons_2->effectN( 1 ).percent();
    }

    double chance = base_proc_chance;
    chance += p()->buffs.envenom->stack_value();
    chance += rogue_t::cast_attack( source_state->action )->composite_poison_flat_modifier( source_state );

    return chance;
  }

  virtual void trigger( const action_state_t* source_state )
  {
    bool result = rng().roll( proc_chance( source_state ) );

    if ( sim->debug )
      sim->out_debug.printf( "%s attempts to proc %s, target=%s source_action=%s proc_chance=%.3f: %d", player->name(), name(),
                             source_state->target->name(), source_state->action->name(), proc_chance( source_state ), result );

    if ( !result )
      return;

    set_target( source_state->target );
    execute();

    if ( p()->azerite.double_dose.ok() && p()->active.lethal_poison == this &&
        ( source_state->action->name_str == "mutilate_mh" || source_state->action->name_str == "mutilate_oh" ) )
    {
      p()->buffs.double_dose->trigger( 1 );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( state->result == RESULT_CRIT && p()->legendary.dashing_scoundrel->ok() && p()->buffs.envenom->check() )
    {
      p()->resource_gain( RESOURCE_ENERGY, p()->legendary.dashing_scoundrel_gain, p()->gains.dashing_scoundrel );
    }
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    if ( d->state->result == RESULT_CRIT && p()->legendary.dashing_scoundrel->ok() && p()->buffs.envenom->check() )
    {
      p()->resource_gain( RESOURCE_ENERGY, p()->legendary.dashing_scoundrel_gain, p()->gains.dashing_scoundrel );
    }
  }
};

// Deadly Poison ============================================================

struct deadly_poison_t : public rogue_poison_t
{
  struct deadly_poison_dd_t : public rogue_poison_t
  {
    deadly_poison_dd_t( util::string_view name, rogue_t* p ) :
      rogue_poison_t( name, p, p->find_spell( 113780 ), true )
    {
    }
  };

  struct deadly_poison_dot_t : public rogue_poison_t
  {
    deadly_poison_dot_t( util::string_view name, rogue_t* p ) :
      rogue_poison_t( name, p, p->spec.deadly_poison->effectN( 1 ).trigger(), true )
    {
    }

    timespan_t calculate_dot_refresh_duration(const dot_t* dot, timespan_t /* triggered_duration */) const override
    {
      // 12/29/2017 - Deadly Poison uses an older style of refresh, adding the origial duration worth of ticks up to 50% more than the base number of ticks
      //              Deadly Poison shouldn't have partial ticks, so we just add the amount of time relative to how many additional ticks we want to add
      const int additional_ticks = (int)(data().duration() / dot->time_to_tick);
      const int max_ticks = (int)(additional_ticks * 1.5);
      return dot->remains() + std::min( max_ticks - dot->ticks_left(), additional_ticks ) * dot->time_to_tick;
    }
  };

  deadly_poison_dd_t*  proc_instant;
  deadly_poison_dot_t* proc_dot;

  deadly_poison_t( util::string_view name, rogue_t* p ) :
    rogue_poison_t( name, p, p->spec.deadly_poison ),
    proc_instant( nullptr ), proc_dot( nullptr )
  {
    proc_instant = p->get_background_action<deadly_poison_dd_t>( "deadly_poison_instant" );
    proc_dot  = p->get_background_action<deadly_poison_dot_t>( "deadly_poison_dot" );

    add_child( proc_instant );
    add_child( proc_dot );
  }

  void impact( action_state_t* state ) override
  {
    rogue_poison_t::impact( state );

    if ( td( state->target )->dots.deadly_poison->is_ticking() )
    {
      proc_instant->set_target( state->target );
      proc_instant->execute();
    }
    proc_dot -> set_target( state -> target );
    proc_dot -> execute();
  }
};

// Slaughter Poison =========================================================
// TOCHECK: Just cloned from Deadly Poison for now since it appears to have the same mechanics currently

struct slaughter_poison_t : public rogue_poison_t
{
  struct slaughter_poison_dd_t : public rogue_poison_t
  {
    slaughter_poison_dd_t( util::string_view name, rogue_t* p ) :
      rogue_poison_t( name, p, p->find_spell( 323660 ), true )
    {
    }
  };

  struct slaughter_poison_dot_t : public rogue_poison_t
  {
    slaughter_poison_dot_t( util::string_view name, rogue_t* p ) :
      rogue_poison_t( name, p, p->covenant.slaughter->effectN( 3 ).trigger()->effectN( 1 ).trigger(), true )
    {
    }

    timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t /* triggered_duration */ ) const override
    {
      // 12/29/2017 - Deadly Poison uses an older style of refresh, adding the origial duration worth of ticks up to 50% more than the base number of ticks
      //              Deadly Poison shouldn't have partial ticks, so we just add the amount of time relative to how many additional ticks we want to add
      const int additional_ticks = (int)( data().duration() / dot->time_to_tick );
      const int max_ticks = (int)( additional_ticks * 1.5 );
      return dot->remains() + std::min( max_ticks - dot->ticks_left(), additional_ticks ) * dot->time_to_tick;
    }
  };

  slaughter_poison_dd_t*  proc_instant;
  slaughter_poison_dot_t* proc_dot;

  slaughter_poison_t( util::string_view name, rogue_t* p ) :
    rogue_poison_t( name, p, p->covenant.slaughter->effectN( 3 ).trigger() ),
    proc_instant( nullptr ), proc_dot( nullptr )
  {
    proc_instant = p->get_background_action<slaughter_poison_dd_t>( "slaughter_poison_instant" );
    proc_dot = p->get_background_action<slaughter_poison_dot_t>( "slaughter_poison_dot" );

    add_child( proc_instant );
    add_child( proc_dot );
  }

  void impact( action_state_t* state ) override
  {
    rogue_poison_t::impact( state );

    if ( td( state->target )->dots.slaughter_poison->is_ticking() )
    {
      proc_instant->set_target( state->target );
      proc_instant->execute();
    }
    proc_dot->set_target( state->target );
    proc_dot->execute();
  }
};

// Instant Poison ===========================================================

struct instant_poison_t : public rogue_poison_t
{
  struct instant_poison_dd_t : public rogue_poison_t
  {
    instant_poison_dd_t( util::string_view name, rogue_t* p ) :
      rogue_poison_t( name, p, p->find_class_spell( "Instant Poison" )->effectN( 1 ).trigger(), true )
    {
    }
  };

  instant_poison_t( util::string_view name, rogue_t* p ) :
    rogue_poison_t( name, p, p->find_class_spell( "Instant Poison" ) )
  {
    impact_action = p->get_background_action<instant_poison_dd_t>( "instant_poison" );
  }
};

// Wound Poison =============================================================

struct wound_poison_t : public rogue_poison_t
{
  struct wound_poison_dd_t : public rogue_poison_t
  {
    wound_poison_dd_t( util::string_view name, rogue_t* p ) :
      rogue_poison_t( name, p, p -> find_class_spell( "Wound Poison" ) -> effectN( 1 ).trigger(), true )
    {
    }

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );
      td( state->target )->debuffs.wound_poison->trigger();
      if ( !sim->overrides.mortal_wounds && state->target->debuffs.mortal_wounds )
      {
        state->target->debuffs.mortal_wounds->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );
      }
    }
  };

  wound_poison_t( util::string_view name, rogue_t* p ) :
    rogue_poison_t( name, p, p->find_class_spell( "Wound Poison" ) )
  {
    impact_action = p->get_background_action<wound_poison_dd_t>( "wound_poison" );
  }
};

// Crippling poison =========================================================

struct crippling_poison_t : public rogue_poison_t
{
  struct crippling_poison_proc_t : public rogue_poison_t
  {
    crippling_poison_proc_t( util::string_view name, rogue_t* p ) :
      rogue_poison_t( name, p, p->find_class_spell( "Crippling Poison" )->effectN( 1 ).trigger() )
    { }

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );
      td( state->target )->debuffs.crippling_poison->trigger();
    }
  };

  crippling_poison_t( util::string_view name, rogue_t* p ) :
    rogue_poison_t( name, p, p->find_class_spell( "Crippling Poison" ) )
  {
    impact_action = p->get_background_action<crippling_poison_proc_t>( "crippling_poison" );
  }
};

// Numbing poison ===========================================================

struct numbing_poison_t : public rogue_poison_t
{
  struct numbing_poison_proc_t : public rogue_poison_t
  {
    numbing_poison_proc_t( util::string_view name, rogue_t* p ) :
      rogue_poison_t( name, p, p->find_class_spell( "Numbing Poison" )->effectN( 1 ).trigger() )
    { }

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );
      td( state->target )->debuffs.numbing_poison->trigger();
    }
  };

  numbing_poison_t( util::string_view name, rogue_t* p ) :
    rogue_poison_t( name, p, p->find_class_spell( "Numbing Poison" ) )
  {
    impact_action = p->get_background_action<numbing_poison_proc_t>( "numbing_poison" );
  }
};

// Apply Poison =============================================================

struct apply_poison_t : public action_t
{
  bool executed;

  apply_poison_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "apply_poison", p ),
    executed( false )
  {
    std::string lethal_str;
    std::string nonlethal_str;

    add_option( opt_string( "lethal", lethal_str ) );
    add_option( opt_string( "nonlethal", nonlethal_str ) );
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;

    if ( p -> main_hand_weapon.type != WEAPON_NONE || p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( ! p -> active.lethal_poison )
      {
        if ( ( lethal_str.empty() && p->specialization() == ROGUE_ASSASSINATION ) || lethal_str == "deadly" )
          p->active.lethal_poison = p->get_background_action<deadly_poison_t>( "deadly_poison_driver" );
        else if ( ( lethal_str.empty() && p->specialization() != ROGUE_ASSASSINATION ) || lethal_str == "instant" )
          p->active.lethal_poison = p->get_background_action<instant_poison_t>( "instant_poison_driver" );
        else if ( lethal_str == "wound" )
          p->active.lethal_poison = p->get_background_action<wound_poison_t>( "wound_poison_driver" );
      }

      if ( ! p -> active.nonlethal_poison )
      {
        if ( nonlethal_str.empty() || nonlethal_str == "crippling" )
          p->active.nonlethal_poison = p->get_background_action<crippling_poison_t>( "crippling_poison_driver" );
        else if ( nonlethal_str == "numbing" )
          p->active.nonlethal_poison = p->get_background_action<numbing_poison_t>( "numbing_poison_driver" );
      }
    }
  }

  void reset() override
  {
    action_t::reset();
    executed = false;
  }

  void execute() override
  {
    executed = true;
    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", player -> name(), name() );
  }

  bool ready() override
  {
    return !executed;
  }
};

// rogue_attack_t ===========================================================

void rogue_attack_t::impact( action_state_t* state )
{
  base_t::impact( state );

  trigger_main_gauche( state );
  trigger_combat_potency( state );
  trigger_blade_flurry( state );
  trigger_shadow_blades_attack( state );
  trigger_bloodfang( state ); // TOCHECK: Is this on impact or execute?

  if ( result_is_hit( state->result ) )
  {
    if ( procs_poison() && p()->active.lethal_poison )
      p()->active.lethal_poison->trigger( state );

    if ( procs_poison() && p()->active.nonlethal_poison )
      p()->active.nonlethal_poison->trigger( state );
  }
}

// ==========================================================================
// Attacks
// ==========================================================================

// Melee Attack =============================================================

struct melee_t : public rogue_attack_t
{
  int sync_weapons;
  bool first;

  melee_t( const char* name, rogue_t* p, int sw ) :
    rogue_attack_t( name, p ), sync_weapons( sw ), first( true )
  {
    background = repeating = true;
    special = false;
    school = SCHOOL_PHYSICAL;
    trigger_gcd = timespan_t::zero();
    weapon_multiplier = 1.0;

    if ( p->dual_wield() )
      base_hit -= 0.19;

    p->auto_attack = this;
  }

  void reset() override
  {
    rogue_attack_t::reset();

    first = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = rogue_attack_t::execute_time();
    if ( first )
    {
      return ( weapon->slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 ) : timespan_t::zero();
    }
    return t;
  }

  void execute() override
  {
    if ( first )
    {
      first = false;
    }

    // If we are channeling (e.g. Cyclotronic Blast) cancel the attack
    if ( p()->channeling && p()->channeling->interrupt_auto_attack )
    {
      this->cancel();
      return;
    }

    rogue_attack_t::execute();
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );
    trigger_shadow_techniques( state );
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    rogue_td_t* tdata = td( target );
    if ( tdata->debuffs.vendetta->up() )
    {
      m *= 1.0 + tdata->debuffs.vendetta->data().effectN( 2 ).percent();
    }

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = rogue_attack_t::composite_crit_chance();

    // 3/3/2019 - Logs show that Master Assassin also affects melee auto attacks
    if ( p()->talent.master_assassin->ok() )
    {
      c += p()->buffs.master_assassin->stack_value();
      c += p()->buffs.master_assassin_aura->stack_value();
    }

    return c;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    // Subtlety
    if ( p()->buffs.symbols_of_death->up() )
    {
      m *= 1.0 + p()->buffs.symbols_of_death->data().effectN( 2 ).percent();
    }

    if ( p()->buffs.shadow_dance->up() )
    {
      m *= 1.0 + p()->buffs.shadow_dance->data().effectN( 2 ).percent() + p()->talent.dark_shadow->effectN( 1 ).percent();
    }

    // Assassination
    if ( p()->buffs.elaborate_planning->up() )
    {
      m *= 1.0 + p()->buffs.elaborate_planning->data().effectN( 3 ).percent();
    }

    // General
    if ( p()->buffs.deathly_shadows->up() )
    {
      m *= 1.0 + p()->buffs.deathly_shadows->data().effectN( 4 ).percent();
    }

    return m;
  }

  bool procs_poison() const override
  { return true; }
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

  void execute() override
  {
    player -> main_hand_attack -> schedule_execute();

    if ( player -> off_hand_attack )
      player -> off_hand_attack -> schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() )
      return false;

    return ( player->main_hand_attack->execute_event == nullptr ); // not swinging
  }
};

// Adrenaline Rush ==========================================================

struct adrenaline_rush_t : public rogue_spell_t
{
  double precombat_seconds;

  adrenaline_rush_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_spell_t( name, p, p->spec.adrenaline_rush ),
    precombat_seconds( 0.0 )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = false;

    if ( p->azerite.vision_of_perfection.enabled() )
    {
      cooldown->duration *= 1.0 + azerite::vision_of_perfection_cdr( p->azerite.vision_of_perfection );
    }
  }

  void execute() override
  {
    rogue_spell_t::execute();

    // 6/23/2019 - Casting while a Vision of Perfection proc is up cancels the existing buff
    //             This also means the existing Brigand's Blitz stack gets reset
    p()->buffs.adrenaline_rush->expire();
    p()->buffs.adrenaline_rush->trigger();

    if ( precombat_seconds && !p()->in_combat )
    {
      timespan_t precombat_lost_seconds = -timespan_t::from_seconds( precombat_seconds );
      p()->cooldowns.adrenaline_rush->adjust( precombat_lost_seconds, false );
      p()->buffs.adrenaline_rush->extend_duration( p(), precombat_lost_seconds );
      p()->buffs.loaded_dice->extend_duration( p(), precombat_lost_seconds );
      p()->buffs.brigands_blitz_driver->extend_duration( p(), precombat_lost_seconds );
      if ( p()->azerite.brigands_blitz.ok() )
        p()->buffs.brigands_blitz->trigger( as<int>( floor( -precombat_lost_seconds / p()->buffs.brigands_blitz_driver->buff_period ) ) );
    }
  }

  bool ready() override
  {
    if ( p()->specialization() != ROGUE_OUTLAW )
      return false;

    return rogue_spell_t::ready();
  }
};

// Ambush ===================================================================

struct ambush_t : public rogue_attack_t
{
  ambush_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> find_class_spell( "Ambush" ), options_str )
  {
  }

  void execute() override
  {
    rogue_attack_t::execute();
    trigger_count_the_odds( execute_state );
  }

  bool procs_main_gauche() const override
  { return false; }
};

// Backstab =================================================================

struct backstab_t : public rogue_attack_t
{
  backstab_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> find_specialization_spell( "Backstab" ), options_str )
  {
    if ( p->active.weaponmaster.backstab && p->active.weaponmaster.backstab != this )
    {
      add_child( p->active.weaponmaster.backstab );
    }
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double b = rogue_attack_t::bonus_da( state );
    b += p() -> buffs.perforate -> stack_value();

    if ( p() -> azerite.inevitability.ok() )
      b += p() -> azerite.inevitability.value( 3 );

    return b;
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
    if ( p()->talent.gloomblade->ok() )
    {
      return false;
    }

    return rogue_attack_t::ready();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p() -> position() == POSITION_BACK )
    {
      p() -> buffs.perforate -> trigger();
      if ( p() -> azerite.perforate.ok() )
      {
        timespan_t v = - p() -> azerite.perforate.spell_ref().effectN( 2 ).time_value();
        p() -> cooldowns.shadow_blades -> adjust( v, false );
      }
    }

    if ( p()->buffs.the_rotten->up() )
    {
      trigger_combo_point_gain( as<int>( p()->buffs.the_rotten->data().effectN( 2 ).base_value() ), p()->gains.the_rotten );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    trigger_weaponmaster( state, p()->active.weaponmaster.backstab );
    trigger_inevitability( state );

    if ( state->result == RESULT_CRIT && p()->spec.backstab_2->ok() && p()->position() == POSITION_BACK )
    {
      timespan_t duration = timespan_t::from_seconds( p()->spec.backstab_2->effectN( 1 ).base_value() );
      trigger_find_weakness( state, duration );
    }
  }
};

// Between the Eyes =========================================================

struct between_the_eyes_t : public rogue_attack_t
{
  between_the_eyes_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p->spec.between_the_eyes, options_str )
  {
    ap_type = attack_power_type::WEAPON_BOTH;
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double b = rogue_attack_t::bonus_da( state );

    if ( p()->azerite.ace_up_your_sleeve.ok() )
      b += p()->azerite.ace_up_your_sleeve.value(); // CP Mult is applied as a mod later.

    return b;
  }

  double composite_crit_chance() const override
  {
    double c = rogue_attack_t::composite_crit_chance();

    if ( p()->buffs.ruthless_precision->up() )
    {
      c += p()->buffs.ruthless_precision->data().effectN( 2 ).percent();
    }

    return c;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    trigger_restless_blades( execute_state );
    trigger_grand_melee( execute_state );

    const rogue_action_state_t* rs = cast_state( execute_state );
    if ( result_is_hit( execute_state->result ) )
    {
      // There is nothing about the debuff duration in spell data, so we have to hardcode the 3s base.
      td( execute_state->target )->debuffs.between_the_eyes->trigger(1, buff_t::DEFAULT_VALUE(), -1.0, 3_s * cast_state( execute_state )->cp );

      p()->buffs.deadshot->trigger();

      if ( p()->azerite.ace_up_your_sleeve.ok() && rng().roll( rs->cp * p()->azerite.ace_up_your_sleeve.spell_ref().effectN( 2 ).percent() ) )
        trigger_combo_point_gain( as<int>( p()->azerite.ace_up_your_sleeve.spell_ref().effectN( 3 ).base_value() ), p()->gains.ace_up_your_sleeve );

      if ( p()->legendary.greenskins_wickers.ok() && rng().roll( rs->cp * p()->legendary.greenskins_wickers->effectN( 1 ).percent() ) )
        p()->buffs.greenskins_wickers->trigger();
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );
    trigger_prey_on_the_weak( state );
  }
};

// Blade Flurry =============================================================

struct blade_flurry_attack_t : public rogue_attack_t
{
  blade_flurry_attack_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p -> find_spell( 22482 ) )
  {
    callbacks = false;
    radius = 5;
    range = -1.0;

    // Spell data has chain targets with mult 1 set to 5 on the damage effect, but it is 4 targets as in the dummy effect, so we set that here.
    aoe = static_cast<int>( p->spec.blade_flurry->effectN( 3 ).base_value() );
  }

  void init() override
  {
    rogue_attack_t::init();
    snapshot_flags |= STATE_TGT_MUL_DA;
  }

  bool procs_poison() const override
  { return false; }

  bool procs_main_gauche() const override
  { return false; }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    rogue_attack_t::available_targets( tl );

    for ( size_t i = 0; i < tl.size(); i++ )
    {
      if ( tl[ i ] == target ) // Cannot hit the original target.
      {
        tl.erase( tl.begin() + i );
        break;
      }
    }
    return tl.size();
  }
};

struct blade_flurry_t : public rogue_attack_t
{
  struct blade_flurry_instant_attack_t : public rogue_attack_t
  {
    blade_flurry_instant_attack_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p -> find_spell( 331850 ) )
    {
      range = -1.0;
    }

    bool procs_main_gauche() const override
    { return false; }
  };

  blade_flurry_instant_attack_t* instant_attack;

  blade_flurry_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> spec.blade_flurry, options_str ),
    instant_attack(nullptr)
  {
    harmful = false;

    add_child( p->active.blade_flurry );

    if ( p->spec.blade_flurry_2->ok() )
    {
      instant_attack = p->get_background_action<blade_flurry_instant_attack_t>( "blade_flurry_instant_attack" );
      add_child( instant_attack );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p()->buffs.blade_flurry->trigger();

    if ( instant_attack )
    {
      instant_attack->set_target( target );
      instant_attack->execute();
    }
  }
};

// Blade Rush ===============================================================

struct blade_rush_t : public rogue_attack_t
{
  struct blade_rush_attack_t : public rogue_attack_t
  {
    blade_rush_attack_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p -> find_spell( 271881 ) )
    {
      aoe = -1;
    }

    void execute() override
    {
      rogue_attack_t::execute();
      p()->buffs.blade_rush->trigger();
    }

    double composite_da_multiplier( const action_state_t* state ) const override
    {
      double m = rogue_attack_t::composite_da_multiplier( state );

      if ( state->target == state->action->target )
        m *= data().effectN( 2 ).percent();
      else if ( p()->buffs.blade_flurry->up() )
        m *= 1.0 + p()->talent.blade_rush->effectN( 1 ).percent();

      return m;
    }
  };

  blade_rush_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> talent.blade_rush, options_str )
  {
    execute_action = p->get_background_action<blade_rush_attack_t>( "blade_rush_attack" );
  }
};

// Bloodfang Legendary ======================================================

struct bloodfang_t : public rogue_attack_t
{
  bloodfang_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->legendary.essence_of_bloodfang->effectN( 1 ).trigger() )
  {}
};

// Crimson Tempest ==========================================================

struct crimson_tempest_t : public rogue_attack_t
{
  crimson_tempest_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> talent.crimson_tempest, options_str )
  {
    aoe = as<int>( data().effectN( 3 ).base_value() );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    const rogue_action_state_t* state = cast_state( s );

    timespan_t duration = data().duration() * ( 1 + state -> cp );
    if ( state -> exsanguinated )
    {
      duration *= 1.0 / ( 1.0 + p() -> talent.exsanguinate -> effectN( 1 ).percent() );
    }

    return duration;
  }

  double combo_point_da_multiplier( const action_state_t* s ) const override
  { 
    return static_cast<double>( cast_state( s )->get_combo_points() + 1 );
  }
};

// Detection ================================================================

// This ability does nothing but for some odd reasons throughout the history of Rogue spaghetti, we may want to look at using it. So, let's support it.
struct detection_t : public rogue_spell_t
{
  detection_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_spell_t( name, p, p -> find_class_spell( "Detection" ), options_str )
  {
    gcd_type = gcd_haste_type::ATTACK_HASTE;
    min_gcd = timespan_t::from_millis(750); // Force 750ms min gcd because rogue player base has a 1s min.
  }
};

// Dispatch =================================================================

struct dispatch_t: public rogue_attack_t
{
  dispatch_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> find_specialization_spell( "Dispatch" ), options_str )
  {
  }

  bool procs_main_gauche() const override
  {
    return false;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    trigger_restless_blades( execute_state );
    trigger_grand_melee( execute_state );
    trigger_count_the_odds( execute_state );
  }
};

// Dreadblades ==============================================================

struct dreadblades_t : public rogue_attack_t
{
  dreadblades_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p->talent.dreadblades, options_str )
  {}

  void execute() override
  {
    rogue_attack_t::execute();
    p()->buffs.dreadblades->trigger();
  }
};

// Envenom ==================================================================

struct envenom_t : public rogue_attack_t
{
  envenom_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p->spec.envenom, options_str )
  {
    dot_duration = timespan_t::zero();
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    if ( p()->legendary.doomblade.ok() )
    {
      // TOCHECK: On beta, this currently doesn't include Crimson Tempest
      rogue_td_t* td = this->td( target );
      int active_dots = td->dots.garrote->is_ticking() + td->dots.rupture->is_ticking() +
        td->dots.crimson_tempest->is_ticking() + td->dots.internal_bleeding->is_ticking() +
        td->dots.mutilated_flesh->is_ticking();
      m *= 1.0 + p()->legendary.doomblade->effectN( 2 ).percent() * active_dots;
    }

    return m;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = rogue_attack_t::bonus_da( s );
    if ( p() -> azerite.twist_the_knife.ok() )
      b += p() -> azerite.twist_the_knife.value(); // CP Mult is applied as a mod later.
    return b;
  }

  void execute() override
  {
    rogue_attack_t::execute();
    trigger_poison_bomb( execute_state );
  }

  void impact( action_state_t* state ) override
  {
    // Trigger Envenom buff before impact() so that poison procs from Envenom itself benefit
    timespan_t envenom_duration = p()->buffs.envenom->data().duration() * ( 1 + cast_state( state )->cp );
    if ( p()->azerite.twist_the_knife.ok() && state->result == RESULT_CRIT )
      envenom_duration += p()->azerite.twist_the_knife.spell_ref().effectN( 2 ).time_value();
    p()->buffs.envenom->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, envenom_duration );

    rogue_attack_t::impact( state );

    // TOCHECK: Envenom itself currently triggers this on beta, need to confirm closer to launch
    if ( state->result == RESULT_CRIT && p()->legendary.dashing_scoundrel->ok() )
    {
      p()->resource_gain( RESOURCE_ENERGY, p()->legendary.dashing_scoundrel_gain, p()->gains.dashing_scoundrel );
    }
  }
};

// Eviscerate ===============================================================

struct eviscerate_t : public rogue_attack_t
{
  struct eviscerate_bonus_t : public rogue_attack_t
  {
    int last_eviscerate_cp;

    eviscerate_bonus_t( util::string_view name, rogue_t* p ):
      rogue_attack_t( name, p, p->find_spell( 328082 ) ),
      last_eviscerate_cp( 1 )
    {}

    void reset() override
    {
      rogue_attack_t::reset();
      last_eviscerate_cp = 1;
    }

    double combo_point_da_multiplier( const action_state_t* ) const override
    {
      return as<double>( last_eviscerate_cp );
    }
  };

  eviscerate_bonus_t* bonus_attack;

  eviscerate_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ):
    rogue_attack_t( name, p, p->spec.eviscerate, options_str ),
    bonus_attack( nullptr )
  {
    if ( p->spec.eviscerate_2->ok() )
    {
      bonus_attack = p->get_background_action<eviscerate_bonus_t>( "eviscerate_bonus" );
      add_child( bonus_attack );
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = rogue_attack_t::bonus_da( s );
    b += p() -> buffs.nights_vengeance -> stack_value(); // CP Mult is applied as a mod later.
    return b;
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p()->buffs.nights_vengeance->expire();
    p()->buffs.deeper_daggers->trigger(); // TOCHECK: Does this happen before or after the bonus damage?

    if ( bonus_attack && td( target )->debuffs.find_weakness->up() )
    {
      // TOCHECK: If this works correctly with Echoing Reprimand
      bonus_attack->last_eviscerate_cp = cast_state( execute_state )->get_combo_points();
      bonus_attack->set_target( target );
      bonus_attack->execute();
    }

    if ( p()->legendary.finality.ok() )
    {
      if ( p()->buffs.finality_eviscerate->check() )
        p()->buffs.finality_eviscerate->expire();
      else
        p()->buffs.finality_eviscerate->trigger();
    }
  }
};

// Exsanguinate =============================================================

struct exsanguinate_t : public rogue_attack_t
{
  exsanguinate_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ):
    rogue_attack_t( name, p, p -> talent.exsanguinate, options_str )
  { }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    trigger_exsanguinate( state );
  }
};

// Fan of Knives ============================================================

struct fan_of_knives_t: public rogue_attack_t
{
  struct echoing_blades_t : public rogue_attack_t
  {
    echoing_blades_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p -> find_spell(287653) )
    {
      aoe = -1;
      base_dd_min = base_dd_max = p -> azerite.echoing_blades.value( 6 );
    }

    double composite_crit_chance() const override
    {
      return 1.0;
    }
  };

  echoing_blades_t* echoing_blades_attack;
  int echoing_blades_crit_count;

  fan_of_knives_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ):
    rogue_attack_t( name, p, p -> find_specialization_spell( "Fan of Knives" ), options_str ),
    echoing_blades_attack( nullptr ), echoing_blades_crit_count( 0 )
  {
    aoe = as<int>( data().effectN( 3 ).base_value() );
    energize_type     = action_energize::ON_HIT;
    energize_resource = RESOURCE_COMBO_POINT;
    // 09/25/2019 - 8.2.5 Spelldata seemingly erroneously removed this effect from the spell data
    energize_amount   = 1; // data().effectN( 2 ).base_value();

    if ( p -> azerite.echoing_blades.ok() )
    {
      echoing_blades_attack = p->get_background_action<echoing_blades_t>( "echoing_blades" );
      add_child( echoing_blades_attack );
    }
  }

  double composite_poison_flat_modifier( const action_state_t* state ) const override
  {
    double chance = rogue_attack_t::composite_poison_flat_modifier( state ); 
    chance += p()->conduit.poisoned_katar.percent(); // TOCHECK: Spell data is kinda iffy on if this works...
    return chance;
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double b = rogue_attack_t::bonus_da( state );
    b += p() -> azerite.echoing_blades.value( 2 );
    return b;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();
    m *= 1.0 + p() -> buffs.hidden_blades -> check_stack_value();
    return m;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p()->buffs.hidden_blades->expire();

    echoing_blades_crit_count = 0;
  }

  void schedule_travel( action_state_t* state ) override
  {
    rogue_attack_t::schedule_travel( state );

    if ( result_is_hit( state -> result ) )
    {
      // 2018-01-25: Poisons are applied on cast as well
      // Note: Usual application on impact will not happen because this attack has no weapon assigned
      if ( p()->active.lethal_poison )
        p()->active.lethal_poison->trigger( state );

      if ( p()->active.nonlethal_poison )
        p()->active.nonlethal_poison->trigger( state );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( echoing_blades_attack && state->result == RESULT_CRIT && ++echoing_blades_crit_count <= p()->azerite.echoing_blades.spell_ref().effectN( 4 ).base_value() )
    {
      echoing_blades_attack->set_target( state->target );
      echoing_blades_attack->execute();
    }
  }
};

// Feint ====================================================================

struct feint_t : public rogue_attack_t
{
  feint_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ):
    rogue_attack_t( name, p, p -> find_class_spell( "Feint" ), options_str )
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
  struct garrote_state_t : public rogue_action_state_t
  {
    bool shrouded_suffocation;

    garrote_state_t( action_t* action, player_t* target ) :
      rogue_action_state_t( action, target ), shrouded_suffocation( false )
    {}

    void initialize() override
    { rogue_action_state_t::initialize(); shrouded_suffocation = false; }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      rogue_action_state_t::debug_str( s ) << " shrouded_suffocation=" << shrouded_suffocation;
      return s;
    }

    void copy_state( const action_state_t* o ) override
    {
      rogue_action_state_t::copy_state( o );
      const garrote_state_t* st = debug_cast<const garrote_state_t*>( o );
      shrouded_suffocation = st->shrouded_suffocation;
    }
  };

  garrote_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> spec.garrote, options_str )
  {
  }

  action_state_t* new_state() override
  { return new garrote_state_t( this, target ); }

  void snapshot_state( action_state_t* state, result_amount_type type ) override
  {
    rogue_attack_t::snapshot_state( state, type );

    if ( p() -> azerite.shrouded_suffocation.ok() )
    {
      // Note: Looks like Shadowmeld works for the damage gain.
      debug_cast<garrote_state_t*>( state ) -> shrouded_suffocation = p() -> stealthed();
    }
  }

  double bonus_ta( const action_state_t* state ) const override
  {
    double b = rogue_attack_t::bonus_ta( state );

    if ( debug_cast<const garrote_state_t*>( state ) -> shrouded_suffocation )
      b += p() -> azerite.shrouded_suffocation.value();

    return b;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_persistent_multiplier( state );

    if ( p()->talent.subterfuge->ok() && p()->stealthed( STEALTH_BASIC | STEALTH_SUBTERFUGE ) )
    {
      m *= 1.0 + p() -> spell.subterfuge -> effectN( 2 ).percent();
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

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    if ( p()->talent.subterfuge->ok() && p()->stealthed( STEALTH_BASIC | STEALTH_SUBTERFUGE ) )
    {
      cd_duration = timespan_t::zero();
    }

    rogue_attack_t::update_ready( cd_duration );
  }

  void execute() override
  {
    bool castFromStealth = p() -> stealthed();

    rogue_attack_t::execute();

    if ( p() -> azerite.shrouded_suffocation.ok() )
    {
      // Note: Looks like Shadowmeld works for the CP gain.
      if ( castFromStealth )
        trigger_combo_point_gain( as<int>( p() -> azerite.shrouded_suffocation.spell_ref().effectN( 2 ).base_value() ), p() -> gains.shrouded_suffocation );
    }
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    trigger_venomous_wounds( d -> state );
  }
};

// Gouge =====================================================================

struct gouge_t : public rogue_attack_t
{
  gouge_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> find_specialization_spell( "Gouge" ), options_str )
  {
    if ( p -> talent.dirty_tricks -> ok() )
      base_costs[ RESOURCE_ENERGY ] = 0;
  }
};

// Ghostly Strike ===========================================================

struct ghostly_strike_t : public rogue_attack_t
{
  ghostly_strike_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> talent.ghostly_strike, options_str )
  {
  }

  bool procs_main_gauche() const override
  { return false; }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( result_is_hit( state->result ) )
    {
      td( state->target )->debuffs.ghostly_strike->trigger();
    }
  }
};


// Gloomblade ===============================================================

struct gloomblade_t : public rogue_attack_t
{
  gloomblade_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> talent.gloomblade, options_str )
  {
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double b = rogue_attack_t::bonus_da( state );
    b += p() -> buffs.perforate -> stack_value();

    if ( p() -> azerite.inevitability.ok() )
      b += p() -> azerite.inevitability.value( 3 );

    return b;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p() -> position() == POSITION_BACK )
    {
      p() -> buffs.perforate -> trigger();
      if ( p() -> azerite.perforate.ok() )
      {
        timespan_t v = - p() -> azerite.perforate.spell_ref().effectN( 2 ).time_value();
        p() -> cooldowns.shadow_blades -> adjust( v, false );
      }
    }

    if ( p()->buffs.the_rotten->up() )
    {
      trigger_combo_point_gain( as<int>( p()->buffs.the_rotten->data().effectN( 2 ).base_value() ), p()->gains.the_rotten );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    trigger_inevitability( state );

    if ( state->result == RESULT_CRIT && p()->spec.backstab_2->ok() )
    {
      timespan_t duration = timespan_t::from_seconds( p()->spec.backstab_2->effectN( 1 ).base_value() );
      trigger_find_weakness( state, duration );
    }
  }
};

// Kick =====================================================================

struct kick_t : public rogue_attack_t
{
  kick_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> find_class_spell( "Kick" ), options_str )
  {
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target -> debuffs.casting && ! candidate_target -> debuffs.casting -> check() )
      return false;

    return rogue_attack_t::target_ready( candidate_target );
  }
};

// Killing Spree ============================================================

struct killing_spree_tick_t : public rogue_attack_t
{
  killing_spree_tick_t( util::string_view name, rogue_t* p, const spell_data_t* s ) :
    rogue_attack_t( name, p, s )
  {
    direct_tick = true;
  }
};

struct killing_spree_t : public rogue_attack_t
{
  melee_attack_t* attack_mh;
  melee_attack_t* attack_oh;

  killing_spree_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> talent.killing_spree, options_str ),
    attack_mh( nullptr ), attack_oh( nullptr )
  {
    channeled = tick_zero = true;
    interrupt_auto_attack = false;

    attack_mh = p->get_background_action<killing_spree_tick_t>( "killing_spree_mh", p->find_spell( 57841 ) );
    add_child( attack_mh );  

    if ( player -> off_hand_weapon.type != WEAPON_NONE )
    {
      attack_oh = p->get_background_action<killing_spree_tick_t>( "killing_spree_oh",
                                                                  p->find_spell( 57841 )->effectN( 1 ).trigger() );
      add_child( attack_oh );
    }
  }

  timespan_t tick_time( const action_state_t* ) const override
  { return base_tick_time; }

  void execute() override
  {
    p()->buffs.killing_spree->trigger();

    rogue_attack_t::execute();
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    attack_mh->set_target( d->target );
    attack_oh->set_target( d->target );
    attack_mh->execute();
    attack_oh->execute();
  }
};

// Pistol Shot ==============================================================

struct pistol_shot_t : public rogue_attack_t
{
  pistol_shot_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> find_specialization_spell( "Pistol Shot" ), options_str )
  {
    ap_type = attack_power_type::WEAPON_BOTH;
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( !is_secondary_action() )
    {
      if ( p()->active.concealed_blunderbuss )
      {
        add_child( p()->active.concealed_blunderbuss );
      }
    }
  }

  // Probably a bug on Shadowlands beta but Pistol Shot now procs CP. -Mystler 2020-08-02
  bool procs_combat_potency() const override
  {
    // TOCHECK: On beta as of 8/28, Blunderbuss procs don't trigger. Possibly only "on cast".
    if ( secondary_trigger == TRIGGER_CONCEALED_BLUNDERBUSS )
      return false;

    return p()->bugs; 
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();

    if ( p()->buffs.opportunity->check() )
    {
      c *= 1.0 + p()->buffs.opportunity->data().effectN( 1 ).percent();
    }

    return c;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p()->buffs.opportunity->up() )
    {
      double ps_mod = 1.0 + p()->buffs.opportunity->data().effectN( 3 ).percent();

      if ( p()->talent.quick_draw->ok() )
        ps_mod += p()->talent.quick_draw->effectN( 1 ).percent();

      m *= ps_mod;
    }

    m *= 1.0 + p()->buffs.greenskins_wickers->value();

    return m;
  }

  double generate_cp() const override
  {
    double g = rogue_attack_t::generate_cp();
    if ( g == 0.0 )
      return 0.0;

    if ( p()->talent.quick_draw->ok() && p()->buffs.opportunity->check() )
    {
      g += p()->talent.quick_draw->effectN( 2 ).base_value();
    }

    return g;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = rogue_attack_t::bonus_da( s );
    b += p()->buffs.deadshot->stack_value();
    return b;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( generate_cp() > 0 && p()->talent.quick_draw->ok() && p()->buffs.opportunity->check() )
    {
      const int cp_gain = as<int>( p()->talent.quick_draw->effectN( 2 ).base_value() );
      trigger_combo_point_gain( cp_gain, p()->gains.quick_draw );
    }

    p()->buffs.opportunity->expire();
    p()->buffs.deadshot->expire();
    p()->buffs.greenskins_wickers->expire();

    // Concealed Blunderbuss Legendary
    if ( p()->active.concealed_blunderbuss && !is_secondary_action() )
    {
      unsigned int num_shots = as<unsigned>( p()->buffs.concealed_blunderbuss->value() );
      for ( unsigned i = 0; i < num_shots; ++i )
      {
        p()->active.concealed_blunderbuss->trigger_secondary_action( execute_state->target );
      }
      p()->buffs.concealed_blunderbuss->expire();
    }
  }
};

// Main Gauche ==============================================================

struct main_gauche_t : public rogue_attack_t
{
  main_gauche_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p -> find_spell( 86392 ) )
  {
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    double ap = rogue_attack_t::attack_direct_power_coefficient( s );

    // Main Gauche is not set up like most masteries as it is a flat mod instead of a percent mod
    // Need to reference the 2nd effect and directly apply the sp_coeff instead of dividing by 100
    ap += p()->cache.mastery() * p()->mastery.main_gauche->effectN( 2 ).sp_coeff();

    return ap;
  }

  bool procs_main_gauche() const override
  { return false; }

  bool procs_combat_potency() const override
  { return true; }

  bool procs_poison() const override
  { return false; }
};

// Marked for Death =========================================================

struct marked_for_death_t : public rogue_spell_t
{
  double precombat_seconds;

  marked_for_death_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ):
    rogue_spell_t( name, p, p -> find_talent_spell( "Marked for Death" ) ),
    precombat_seconds( 0.0 )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = false;
    energize_type = action_energize::ON_CAST;
  }

  void execute() override
  {
    rogue_spell_t::execute();

    if ( precombat_seconds && ! p() -> in_combat ) {
      p() -> cooldowns.marked_for_death -> adjust( - timespan_t::from_seconds( precombat_seconds ), false );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_spell_t::impact( state );

    td( state->target )->debuffs.marked_for_death->trigger();
  }
};

// Mutilate =================================================================

struct mutilate_t : public rogue_attack_t
{
  struct mutilate_strike_t : public rogue_attack_t
  {
    // Note: Uses spell_t instead of rogue_spell_t to avoid action_state casting issues
    struct doomblade_t : public residual_action::residual_periodic_action_t<spell_t>
    {
      doomblade_t( util::string_view name, rogue_t* p ) :
        base_t( name, p, p->find_spell( 340431 ) )
      {
        dual = true;
      }
    };

    doomblade_t* doomblade_dot;

    mutilate_strike_t( util::string_view name, rogue_t* p, const spell_data_t* s ) :
      rogue_attack_t( name, p, s ),
      doomblade_dot( nullptr )
    {
      if ( p->legendary.doomblade.ok() )
      {
        doomblade_dot = p->get_background_action<doomblade_t>( "mutilated_flesh" );
      }
    }

    void impact( action_state_t* state ) override
    {
      rogue_attack_t::impact( state );
      trigger_seal_fate( state );

      if ( doomblade_dot && result_is_hit( state->result ) )
      {
        // TOCHECK: This is very, very buggy on beta with the initial DoT lasting up to 4 ticks/12 seconds
        //          Subsequent DoTs seem correct, but the coefficient and residual behavior is quite odd
        //          For now, implementing how it "should" work with 20% coefficient and normal ignite behavior
        const double dot_damage = state->result_amount * p()->legendary.doomblade->effectN( 1 ).percent();
        residual_action::trigger( doomblade_dot, state->target, dot_damage );
      }
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = rogue_attack_t::composite_target_multiplier( target );

      if ( p()->conduit.maim_mangle.ok() && td( target )->dots.garrote->is_ticking() )
      {
        m *= 1.0 + p()->conduit.maim_mangle.percent();
      }

      return m;
    }
  };

  struct double_dose_t : public rogue_attack_t
  {
    double_dose_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p -> find_spell( 273009 ) )
    {
      base_dd_min = base_dd_max = p->azerite.double_dose.value();
    }
  };

  mutilate_strike_t* mh_strike;
  mutilate_strike_t* oh_strike;
  double_dose_t* double_dose;

  mutilate_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> find_specialization_spell( "Mutilate" ), options_str ),
    mh_strike( nullptr ), oh_strike( nullptr ), double_dose( nullptr)
  {
    if ( p->main_hand_weapon.type != WEAPON_DAGGER || p->off_hand_weapon.type != WEAPON_DAGGER )
    {
      sim -> errorf( "Player %s attempting to execute Mutilate without two daggers equipped.", p -> name() );
      background = true;
    }

    mh_strike = p->get_background_action<mutilate_strike_t>( "mutilate_mh", data().effectN( 3 ).trigger() );
    oh_strike = p->get_background_action<mutilate_strike_t>( "mutilate_oh", data().effectN( 4 ).trigger() );
    add_child( mh_strike );
    add_child( oh_strike );

    if ( mh_strike->doomblade_dot )
    {
      add_child( mh_strike->doomblade_dot );
    }

    if ( p->azerite.double_dose.ok() )
    {
      double_dose = p->get_background_action<double_dose_t>( "double_dose" );
      add_child( double_dose );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      if ( p()->talent.blindside->ok() )
      {
        double chance = p()->talent.blindside->effectN( 1 ).percent();
        if ( execute_state->target->health_percentage() < p()->talent.blindside->effectN( 3 ).base_value() )
        {
          chance = p()->talent.blindside->effectN( 2 ).percent();
        }
        if ( rng().roll( chance ) )
        {
          p()->buffs.blindside->trigger();
        }
      }

      // Reset Double Dose tracker before anything happens.
      p()->buffs.double_dose->expire();

      mh_strike->set_target( execute_state->target );
      mh_strike->execute();

      oh_strike->set_target( execute_state->target );
      oh_strike->execute();

      if ( double_dose && p()->buffs.double_dose->stack() == p()->buffs.double_dose->max_stack() )
      {
        double_dose->set_target( execute_state->target );
        double_dose->execute();
      }

      if ( p()->talent.venom_rush->ok() && p()->get_target_data( execute_state->target )->is_poisoned() )
      {
        p()->resource_gain( RESOURCE_ENERGY, p()->talent.venom_rush->effectN( 1 ).base_value(), p()->gains.venom_rush );
      }
    }
  }
};

// Roll the Bones ===========================================================

struct roll_the_bones_t : public rogue_spell_t
{
  double precombat_seconds;

  roll_the_bones_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_spell_t( name, p, p -> spec.roll_the_bones ),
    precombat_seconds( 0.0 )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = false;
    dot_duration = timespan_t::zero();
  }

  void execute() override
  {
    rogue_spell_t::execute();

    timespan_t d = p() -> buffs.roll_the_bones -> data().duration();
    if ( precombat_seconds && ! p() -> in_combat )
      d -= timespan_t::from_seconds( precombat_seconds );

    p() -> buffs.roll_the_bones -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, d );

    // Roll the Bones still triggers Snake Eyes on Shadowlands Beta but the damage increase is zero since it does not consume CP anymore.
    // - Mystler 2020-07-31
    p() -> buffs.snake_eyes -> trigger( p() -> buffs.snake_eyes -> max_stack(), 0 );

    if ( p()->conduit.slight_of_hand.ok() && p()->rng().roll( p()->conduit.slight_of_hand.percent() ) )
      cooldown->reset( true );
  }
};

// Rupture ==================================================================

struct rupture_t : public rogue_attack_t
{
  rupture_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> find_specialization_spell( "Rupture" ), options_str )
  {
    if ( p->active.replicating_shadows )
    {
      add_child( p->active.replicating_shadows );
    }
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    const rogue_action_state_t* state = cast_state( s );

    timespan_t duration = data().duration() * ( 1 + state->get_combo_points() );
    if ( state -> exsanguinated )
    {
      duration *= 1.0 / ( 1.0 + p() -> talent.exsanguinate -> effectN( 1 ).percent() );
    }

    return duration;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    // Copy the persistent multiplier from the origin of replications.
    if ( secondary_trigger == TRIGGER_REPLICATING_SHADOWS )
      return p()->get_target_data( p()->last_rupture_target )->dots.rupture->state->persistent_multiplier;

    double m = rogue_attack_t::composite_persistent_multiplier( state );
    m += 1.0 + p()->buffs.finality_rupture->value(); // Additive with Nightstalker
    return m;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    trigger_poison_bomb( execute_state );

    // Run quiet proxy buff that handles Scent of Blood
    td( execute_state->target )->debuffs.rupture->trigger();

    // Check if this is a manually applied Rupture that hits
    if ( secondary_trigger == TRIGGER_NONE && result_is_hit( execute_state->result ) )
    {
      // Night's Vengeance has not been updated to work with Rupture and does nothing at all.
      // Keeping impl for reference until we nuke azerite powers. -Mystler 2020-07-31
      //p()->buffs.nights_vengeance->trigger();

      // Save the target for Replicating Shadows
      p()->last_rupture_target = execute_state->target;
    }

    if ( p()->legendary.finality.ok() )
    {
      if ( p()->buffs.finality_rupture->check() )
        p()->buffs.finality_rupture->expire();
      else
        p()->buffs.finality_rupture->trigger();
    }
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    trigger_venomous_wounds( d->state );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( util::str_compare_ci( name, "new_duration" ) )
    {
      struct new_duration_expr_t : public expr_t
      {
        rupture_t* rupture;
        double cp_max_spend;
        double base_duration;

        new_duration_expr_t( rupture_t* a ) :
          expr_t( "new_duration" ),
          rupture( a ),
          cp_max_spend( a->p()->consume_cp_max() ),
          base_duration( a->data().duration().total_seconds() )
        {
        }

        double evaluate() override
        {
          double duration = base_duration * ( 1.0 + std::min( cp_max_spend, rupture->p()->resources.current[ RESOURCE_COMBO_POINT ] ) );
          return std::min( duration * 1.3, duration + rupture->td( rupture->target )->dots.rupture->remains().total_seconds() );
        }
      };

      return std::make_unique<new_duration_expr_t>( this );
    }

    return rogue_attack_t::create_expression( name );
  }
};

struct replicating_shadows_t : public rogue_spell_t
{
  rupture_t* rupture_action;

  replicating_shadows_t( util::string_view name, rogue_t* p ) :
    rogue_spell_t( name, p, p -> find_spell(286131) ),
    rupture_action( nullptr )
  {
    may_miss = false;
    base_dd_min = p -> azerite.replicating_shadows.value();
    base_dd_max = p -> azerite.replicating_shadows.value();
  }

  void execute() override
  {
    rogue_spell_t::execute();

    // Replicating Shadows has not been changed to work with Rupture and simply does nothing aside from the direct damage on SL Beta.
    // Keeping impl for reference until we nuke azerite powers. -Mystler 2020-07-31
    /*if ( ! p() -> last_rupture_target )
      return;

    // Get the last manually applied Rupture as origin. Has to be still up.
    rogue_td_t* last_nb_tdata = p() -> get_target_data( p() -> last_rupture_target );
    if ( last_nb_tdata -> dots.rupture -> is_ticking() )
    {
      // Find the closest target to that manual target without a Rupture debuff.
      double minDist = 0.0;
      player_t* minDistTarget = nullptr;
      for ( const auto enemy : sim -> target_non_sleeping_list )
      {
        rogue_td_t* tdata = p() -> get_target_data( enemy );
        if ( ! tdata -> dots.rupture -> is_ticking() )
        {
          double dist = enemy -> get_position_distance( p() -> last_rupture_target -> x_position, p() -> last_rupture_target -> y_position );
          if ( ! minDistTarget || dist < minDist)
          {
            minDist = dist;
            minDistTarget = enemy;
          }
        }
      }

      // If it exists, trigger a new rupture with 0 CP duration. We also copy the persistent multiplier in rupture_t.
      // Estimated 10 yd spread radius.
      if ( minDistTarget && minDist < 10.0 )
      {
        if ( !rupture_action )
          rupture_action = dynamic_cast<rupture_t*>( p()->find_action( "rupture" ) );
        if ( rupture_action )
          rupture_action->make_secondary_trigger( TRIGGER_REPLICATING_SHADOWS, minDistTarget,
                                                  cast_state( last_nb_tdata->dots.rupture->state )->cp );
      }
    }*/
  }
};

// Secret Technique =========================================================

struct secret_technique_t : public rogue_attack_t
{
  struct secret_technique_attack_t : public rogue_attack_t
  {
    secret_technique_attack_t( util::string_view name, rogue_t* p, const spell_data_t* s ) :
      rogue_attack_t( name, p, s )
    {
      aoe = aoe != 0 ? aoe : -1;
    }

    double combo_point_da_multiplier( const action_state_t* state ) const override
    {
      return static_cast<double>( cast_state( state )->get_combo_points() );
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = rogue_attack_t::composite_target_multiplier( target );

      if ( target != this -> target )
        m *= p() -> talent.secret_technique -> effectN( 3 ).percent();

      return m;
    }
  };

  secret_technique_attack_t* player_attack;
  secret_technique_attack_t* clone_attack;

  secret_technique_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> talent.secret_technique, options_str )
  {
    may_miss = false;

    player_attack = p->get_secondary_trigger_action<secret_technique_attack_t>(
      TRIGGER_SECRET_TECHNIQUE, "secret_technique_player", p->find_spell( 280720 ) );
    clone_attack = p->get_secondary_trigger_action<secret_technique_attack_t>(
      TRIGGER_SECRET_TECHNIQUE, "secret_technique_clones", p->find_spell( 282449 ) );

    add_child( player_attack );
    add_child( clone_attack );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    int cp = cast_state( execute_state )->cp;

    // Hit of the main char happens right on cast.
    player_attack->trigger_secondary_action( execute_state->target, cp );

    // The clones seem to hit 1s and 1.3s later (no time reference in spell data though)
    // Trigger tracking buff until first clone's damage
    p()->buffs.secret_technique->trigger( 1, buff_t::DEFAULT_VALUE(), ( -1.0 ), 1_s );
    clone_attack->trigger_secondary_action( execute_state->target, cp, 1_s );
    clone_attack->trigger_secondary_action( execute_state->target, cp, 1.3_s );
  }
};

// Shadow Blades ============================================================

struct shadow_blades_attack_t : public rogue_attack_t
{
  shadow_blades_attack_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p -> find_spell( 279043 ) )
  {
    may_dodge = may_block = may_parry = false;
    attack_power_mod.direct = 0;
  }

  void init() override
  {
    rogue_attack_t::init();

    snapshot_flags = update_flags = STATE_TGT_MUL_DA;
  }
};

struct shadow_blades_t : public rogue_spell_t
{
  double precombat_seconds;

  shadow_blades_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_spell_t( name, p, p -> find_specialization_spell( "Shadow Blades" ) ),
    precombat_seconds( 0.0 )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = false;
    school = SCHOOL_SHADOW;

    p->active.shadow_blades_attack = p->get_background_action<shadow_blades_attack_t>( "shadow_blades_attack" );
    add_child( p->active.shadow_blades_attack );

    if ( p->azerite.vision_of_perfection.enabled() )
    {
      cooldown->duration *= 1.0 + azerite::vision_of_perfection_cdr( p->azerite.vision_of_perfection );
    }
  }

  void execute() override
  {
    rogue_spell_t::execute();

    p() -> buffs.shadow_blades -> trigger();

    if ( precombat_seconds && ! p() -> in_combat ) {
      timespan_t precombat_lost_seconds = - timespan_t::from_seconds( precombat_seconds );
      p() -> cooldowns.shadow_blades -> adjust( precombat_lost_seconds, false );
      p() -> buffs.shadow_blades -> extend_duration( p(), precombat_lost_seconds );
    }
  }
};

// Shadow Dance =============================================================

struct shadow_dance_t : public rogue_spell_t
{
  //cooldown_t* icd;

  shadow_dance_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_spell_t( name, p, p->spec.shadow_dance, options_str )
    //icd( p -> get_cooldown( "shadow_dance_icd" ) )
  {
    harmful = false;
    dot_duration = timespan_t::zero(); // No need to have a tick here
    //icd -> duration = data().cooldown();
    if ( p -> talent.enveloping_shadows -> ok() )
    {
      cooldown -> charges += as<int>( p -> talent.enveloping_shadows -> effectN( 2 ).base_value() );
    }
  }

  void execute() override
  {
    rogue_spell_t::execute();

    p() -> buffs.shadow_dance -> trigger();
    trigger_master_of_shadows();

    if ( p()->azerite.the_first_dance.ok() )
    {
      p()->buffs.the_first_dance->trigger();
      trigger_combo_point_gain( as<int>( p()->buffs.the_first_dance->data().effectN( 3 ).resource( RESOURCE_COMBO_POINT ) ),
        p() -> gains.the_first_dance );
    }

    //icd -> start();
  }

  bool ready() override
  {
    /*if ( icd->down() )
    {
      return false;
    }*/

    // Cannot dance during stealth. Vanish works.
    if ( p()->buffs.stealth->check() )
      return false;

    return rogue_spell_t::ready();
  }
};

// Shadowstep ===============================================================

struct shadowstep_t : public rogue_spell_t
{
  shadowstep_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_spell_t( name, p, p -> spec.shadowstep, options_str )
  {
    harmful = false;
    dot_duration = timespan_t::zero();
    base_teleport_distance = data().max_range();
    movement_directionality = movement_direction_type::OMNI;
  }

  void execute() override
  {
    rogue_spell_t::execute();
    p() -> buffs.shadowstep -> trigger();
  }
};

// Shadowstrike =============================================================

struct shadowstrike_t : public rogue_attack_t
{
  shadowstrike_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> spec.shadowstrike, options_str )
  {
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( !is_secondary_action() )
    {
      if ( p()->active.weaponmaster.shadowstrike )
      {
        add_child( p()->active.weaponmaster.shadowstrike );
      }
      if ( p()->active.akaaris_soul_fragment )
      {
        add_child( p()->active.akaaris_soul_fragment );
      }
    }
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();

    if ( p() -> azerite.blade_in_the_shadows.ok() )
      c += p() -> azerite.blade_in_the_shadows.spell_ref().effectN( 1 ).trigger() -> effectN( 2 ).base_value();

    return c;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p()->buffs.premeditation->up() )
    {
      timespan_t premed_duration = timespan_t::from_seconds( p()->talent.premeditation->effectN( 1 ).base_value() );
      if ( p()->buffs.slice_and_dice->check() )
      {
        trigger_combo_point_gain( as<int>( p()->talent.premeditation->effectN( 2 ).base_value() ), p()->gains.premeditation );
        p()->buffs.slice_and_dice->extend_duration( p(), premed_duration );
      }
      else
      {
        p()->buffs.slice_and_dice->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, premed_duration );
      }
      p()->buffs.premeditation->expire();
    }

    p()->buffs.blade_in_the_shadows->trigger();
    p()->buffs.perforated_veins->trigger();

    if ( p()->buffs.the_rotten->up() )
    {
      trigger_combo_point_gain( as<int>( p()->buffs.the_rotten->data().effectN( 1 ).base_value() ), p()->gains.the_rotten );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    trigger_weaponmaster( state, p()->active.weaponmaster.shadowstrike );
    trigger_inevitability( state );
    // Appears to be applied after weaponmastered attacks.
    trigger_find_weakness( state );
    trigger_akaaris_soul_fragment( state );
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = rogue_attack_t::bonus_da( s );
    b += p() -> buffs.blade_in_the_shadows -> stack_value();

    if ( p() -> azerite.inevitability.ok() )
      b += p() -> azerite.inevitability.value( 3 );

    return b;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p()->stealthed( STEALTH_BASIC ) )
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

    if ( p()->stealthed( STEALTH_BASIC ) )
    {
      return range + p() -> spec.shadowstrike_2 -> effectN( 1 ).base_value();
    }

    return 0;
  }
};

// Shadow Vault =============================================================

struct shadow_vault_t: public rogue_attack_t
{
  struct shadow_vault_bonus_t : public rogue_attack_t
  {
    int last_cp;

    shadow_vault_bonus_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p -> find_spell( 319190 ) ),
      last_cp( 1 )
    {
    }

    void reset() override
    {
      rogue_attack_t::reset();
      last_cp = 1;
    }

    double combo_point_da_multiplier( const action_state_t* ) const override
    {
      return as<double>( last_cp );
    }
  };

  shadow_vault_bonus_t* bonus_attack;

  shadow_vault_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ):
    rogue_attack_t( name, p, p -> find_specialization_spell( "Shadow Vault" ), options_str ),
    bonus_attack( nullptr )
  {
    if ( p->find_rank_spell( "Shadow Vault", "Rank 2" )->ok() )
    {
      bonus_attack = p->get_background_action<shadow_vault_bonus_t>( "shadow_vault_bonus" );
      add_child( bonus_attack );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p()->buffs.deeper_daggers->trigger(); // TOCHECK: Does this happen before or after the impact damage?

    if ( p()->legendary.finality.ok() )
    {
      if ( p()->buffs.finality_shadow_vault->check() )
        p()->buffs.finality_shadow_vault->expire();
      else
        p()->buffs.finality_shadow_vault->trigger();
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( bonus_attack && result_is_hit( state->result ) && td( state->target )->debuffs.find_weakness->up() )
    {
      // TOCHECK: If this works correctly with Echoing Reprimand
      bonus_attack->last_cp = cast_state( state )->get_combo_points();
      bonus_attack->set_target( state->target );
      bonus_attack->execute();
    }
  }
};

// Shuriken Storm ===========================================================

struct shuriken_storm_t: public rogue_attack_t
{
  shuriken_storm_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ):
    rogue_attack_t( name, p, p -> find_specialization_spell( "Shuriken Storm" ), options_str )
  {
    energize_type = action_energize::PER_HIT;
    energize_resource = RESOURCE_COMBO_POINT;
    energize_amount = 1;
    ap_type = attack_power_type::WEAPON_BOTH;
  }

  void init() override
  {
    rogue_attack_t::init();

    // As of 2018-06-18 not in BfA spell data.
    affected_by.shadow_blades = true;
  }

  void impact(action_state_t* state) override
  {
    rogue_attack_t::impact( state );

    if ( state->result == RESULT_CRIT && p()->spec.shuriken_storm_2->ok() )
    {
      timespan_t duration = timespan_t::from_seconds( p()->spec.shuriken_storm_2->effectN( 1 ).base_value() );
      trigger_find_weakness( state, duration );
    }
  }
};

// Shuriken Tornado =========================================================

struct shuriken_tornado_t : public rogue_spell_t
{
  shuriken_tornado_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_spell_t( name, p, p -> talent.shuriken_tornado, options_str )
  {
    dot_duration = timespan_t::zero();
    aoe = -1;

    // Trigger action is created in the buff, but it can't be parented then, just look it up here
    action_t* trigger_action = p->find_secondary_trigger_action<shuriken_storm_t>( TRIGGER_SHURIKEN_TORNADO );
    if ( trigger_action )
    {
      add_child( trigger_action );
    }
  }

  void execute() override
  {
    rogue_spell_t::execute();
    p()->buffs.shuriken_tornado->trigger();
  }
};

// Shuriken Toss ============================================================

struct shuriken_toss_t : public rogue_attack_t
{
  shuriken_toss_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> find_specialization_spell( "Shuriken Toss" ), options_str )
  {
    ap_type = attack_power_type::WEAPON_BOTH;
  }
};

// Sinister Strike ==========================================================

struct sinister_strike_t : public rogue_attack_t
{
  struct triple_threat_t : public rogue_attack_t
  {
    triple_threat_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->find_spell( 341541 ) )
    {
    }

    void execute() override
    {
      rogue_attack_t::execute();
      trigger_guile_charm( execute_state );
      // TOCHECK: This needs to be totally re-tested at some point because it is exceptionally buggy in-game
      p()->active.triple_threat_mh->trigger_secondary_action( execute_state->target );
    }
  };

  struct sinister_strike_extra_attack_t : public rogue_attack_t
  {
    sinister_strike_extra_attack_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p -> find_spell( 197834 ) )
    {
      // CP generation is not in the spell data for some reason
      energize_type = action_energize::ON_HIT;
      energize_amount = 1;
      energize_resource = RESOURCE_COMBO_POINT;
    }

    double bonus_da( const action_state_t* s ) const override
    {
      double b = rogue_attack_t::bonus_da( s );

      b += p()->buffs.snake_eyes->value();
      b += p()->azerite.keep_your_wits_about_you.value( 2 );

      return b;
    }

    void execute() override
    {
      rogue_attack_t::execute();
      p()->buffs.snake_eyes->decrement();
      trigger_guile_charm( execute_state );

      // TOCHECK: This needs to be totally re-tested at some point because it is exceptionally buggy in-game
      if ( p()->active.triple_threat_oh && p()->rng().roll( p()->conduit.triple_threat.percent() ) )
      {
        p()->active.triple_threat_oh->trigger_secondary_action( execute_state->target, 0, 300_ms );
      }
    }
  };

  sinister_strike_extra_attack_t* extra_attack;

  sinister_strike_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p->spec.sinister_strike, options_str )
  {
    extra_attack = p->get_secondary_trigger_action<sinister_strike_extra_attack_t>( TRIGGER_SINISTER_STRIKE, "sinister_strike_extra_attack" );
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( !is_secondary_action() )
    {
      add_child( extra_attack );
      if ( p()->active.triple_threat_mh )
        add_child( p()->active.triple_threat_mh );
      if ( p()->active.triple_threat_oh )
        add_child( p()->active.triple_threat_oh );
    }
  }

  double extra_attack_proc_chance() const
  {
    double opportunity_proc_chance = data().effectN( 3 ).percent();
    opportunity_proc_chance += p()->talent.weaponmaster->effectN( 1 ).percent();
    opportunity_proc_chance += p()->buffs.skull_and_crossbones->stack_value();
    opportunity_proc_chance += p()->buffs.keep_your_wits_about_you->stack_value();
    return opportunity_proc_chance;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = rogue_attack_t::bonus_da( s );

    b += p()->buffs.snake_eyes->value();

    return b;
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p()->buffs.snake_eyes->decrement();
    trigger_guile_charm( execute_state );

    if ( !result_is_hit( execute_state->result ) )
      return;

    if ( p()->spec.sinister_strike_2->ok() && p()->buffs.opportunity->trigger( 1, buff_t::DEFAULT_VALUE(), extra_attack_proc_chance() ) )
    {
      extra_attack->trigger_secondary_action( execute_state->target, 0, 300_ms );
      p()->buffs.concealed_blunderbuss->trigger();
    }
  }
};

// Slice and Dice ===========================================================

struct slice_and_dice_t : public rogue_spell_t
{
  double precombat_seconds;

  slice_and_dice_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_spell_t( name, p, p -> spell.slice_and_dice ),
    precombat_seconds( 0.0 )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    base_costs[ RESOURCE_COMBO_POINT ] = 1; // No resource cost in the spell .. sigh
    harmful = false;
    dot_duration = timespan_t::zero();
  }

  timespan_t get_triggered_duration( int cp )
  {
    return ( cp + 1 ) * p()->buffs.slice_and_dice->data().duration();
  }

  void execute() override
  {
    rogue_spell_t::execute();

    trigger_restless_blades( execute_state );

    int cp = cast_state( execute_state ) -> cp;
    timespan_t snd_duration = get_triggered_duration( cp );

    if ( precombat_seconds && ! p() -> in_combat )
      snd_duration -= timespan_t::from_seconds( precombat_seconds );

    double snd_mod = 1.0; // Multiplier for the SnD effects. Was changed in Legion for Loaded Dice artifact trait.
    p() -> buffs.slice_and_dice -> trigger( 1, snd_mod, -1.0, snd_duration );

    // Grand melee extension goes on top of SnD buff application.
    trigger_grand_melee( execute_state );

    p() -> buffs.snake_eyes -> trigger( p() -> buffs.snake_eyes -> max_stack(), cp * p() -> azerite.snake_eyes.value() );

    // On Shadowlands Beta, Slice and Dice simply removes any active Paradise Lost buff. -Mystler 2020-07-31
    if ( p() -> azerite.paradise_lost.ok() )
      p() -> buffs.paradise_lost -> expire();
  }

  bool ready() override
  {
    // Grand Melee prevents refreshing if there would be a reduction in the post-pandemic buff duration
    if ( p()->buffs.slice_and_dice->remains() > get_triggered_duration( as<int>( p()->resources.current[ RESOURCE_COMBO_POINT ] ) ) * 1.3 )
      return false;

    return rogue_spell_t::ready();
  }
};

// Sprint ===================================================================

struct sprint_t : public rogue_spell_t
{
  sprint_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ):
    rogue_spell_t( name, p, p -> spell.sprint, options_str )
  {
    harmful = callbacks = false;
    cooldown = p->cooldowns.sprint;
  }

  void execute() override
  {
    rogue_spell_t::execute();
    p()->buffs.sprint->trigger();
  }
};

// Symbols of Death =========================================================

struct symbols_of_death_t : public rogue_spell_t
{
  symbols_of_death_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_spell_t( name, p, p -> spec.symbols_of_death, options_str )
  {
    harmful = callbacks = false;
    dot_duration = timespan_t::zero();
  }

  void execute() override
  {
    rogue_spell_t::execute();
    
    p()->buffs.symbols_of_death->trigger();
    if ( p()->spec.symbols_of_death_2->ok() )
      p()->buffs.symbols_of_death_autocrit->trigger();
    
    p()->buffs.the_rotten->trigger();
  }
};

// Shiv =====================================================================

struct shiv_t : public rogue_attack_t
{
  shiv_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p->find_class_spell( "Shiv" ), options_str )
  {
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    if ( p()->conduit.well_placed_steel.ok() && td( target )->is_bleeding() )
    {
      // TOCHECK: Assuming this is a percentage rather than a flat value, tooltip doesn't have % though
      m *= 1.0 + p()->conduit.well_placed_steel.percent();
    }

    return m;
  }

  void impact( action_state_t* s ) override
  {
    rogue_attack_t::impact( s );
    
    if ( result_is_hit( s->result ) )
    {
      if ( p()->spec.shiv_2->ok() )
        td( s->target )->debuffs.shiv->trigger();

      // TOCHECK: Does this add an Envenom buff even if one is not already up?
      if ( p()->conduit.well_placed_steel.ok() && p()->buffs.envenom->check() )
        p()->buffs.envenom->extend_duration( p(), p()->conduit.well_placed_steel.time_value( conduit_data_t::time_type::S ) );
    }    
  }
};

// Vanish ===================================================================

struct vanish_t : public rogue_spell_t
{

  vanish_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_spell_t( name, p, p -> find_class_spell( "Vanish" ), options_str )
  {
    harmful = false;
  }

  void execute() override
  {
    rogue_spell_t::execute();

    p()->buffs.vanish->trigger();
    trigger_master_of_shadows();
  }

  bool ready() override
  {
    if ( p()->buffs.vanish->check() )
      return false;

    return rogue_spell_t::ready();
  }
};

// Vendetta =================================================================

struct vendetta_t : public rogue_spell_t
{
  struct nothing_personal_t : rogue_spell_t
  {
    nothing_personal_t( util::string_view name, rogue_t* p ) :
      rogue_spell_t( name, p, p -> find_spell( 286581 ) )
    {
      base_td = p -> azerite.nothing_personal.value();
    }
  };

  double precombat_seconds;
  nothing_personal_t* nothing_personal_dot;

  vendetta_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_spell_t( name, p, p->spec.vendetta, options_str ),
    precombat_seconds( 0.0 ),
    nothing_personal_dot( nullptr )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = may_miss = may_crit = false;

    if ( p -> azerite.nothing_personal.ok() )
    {
      nothing_personal_dot = p->get_background_action<nothing_personal_t>( "nothing_personal" );
      add_child( nothing_personal_dot );
    }

    if ( p->azerite.vision_of_perfection.enabled() )
    {
      cooldown->duration *= 1.0 + azerite::vision_of_perfection_cdr( p->azerite.vision_of_perfection );
    }
  }

  void execute() override
  {
    rogue_spell_t::execute();

    rogue_td_t* td = this->td( execute_state->target );

    // Casting Vendetta when a VoP proc is up overwrites the buff durations rather than extending
    td->debuffs.vendetta->expire();
    td->debuffs.vendetta->trigger();

    if ( precombat_seconds && !p()->in_combat )
    {
      timespan_t precombat_lost_seconds = -timespan_t::from_seconds( precombat_seconds );
      p()->cooldowns.vendetta->adjust( precombat_lost_seconds, false );
      p()->buffs.vendetta->extend_duration( p(), precombat_lost_seconds );
      td->debuffs.vendetta->extend_duration( p(), precombat_lost_seconds );
    }
  }
};

// Stealth ==================================================================

struct stealth_t : public rogue_spell_t
{
  stealth_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_spell_t( name, p, p->find_class_spell( "Stealth" ), options_str )
  {
    harmful = false;
  }

  void execute() override
  {
    rogue_spell_t::execute();

    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", p() -> name(), name() );

    p()->buffs.stealth->trigger();
    trigger_master_of_shadows();
  }

  bool ready() override
  {
    if ( p() -> stealthed( STEALTH_BASIC | STEALTH_ROGUE ) )
      return false;

    if ( ! p() -> in_combat )
      return true;

    // HAX: Allow restealth for DungeonSlice against non-"boss" targets because Shadowmeld drops combat against trash.
    if ( p()->sim->fight_style == "DungeonSlice" && p()->player_t::buffs.shadowmeld->check() && target->type == ENEMY_ADD )
      return true;

    if ( !p()->restealth_allowed )
      return false;

    return rogue_spell_t::ready();
  }
};

// Kidney Shot ==============================================================

struct kidney_shot_t : public rogue_attack_t
{
  struct internal_bleeding_t : public rogue_attack_t
  {
    internal_bleeding_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->find_spell( 154953 ) )
    {
    }

    timespan_t composite_dot_duration( const action_state_t* s ) const override
    {
      timespan_t duration = rogue_attack_t::composite_dot_duration( s );

      if ( cast_state( s )->exsanguinated )
      {
        duration *= 1.0 / ( 1.0 + p()->talent.exsanguinate->effectN( 1 ).percent() );
      }

      return duration;
    }

    void tick( dot_t* d ) override
    {
      rogue_attack_t::tick( d );
      trigger_venomous_wounds( d->state );
    }

    double composite_persistent_multiplier( const action_state_t* state ) const override
    {
      double m = rogue_attack_t::composite_persistent_multiplier( state );
      m *= cast_state( state )->cp; // TOCHECK: Does this get affected by Echoing Reprimand?
      return m;
    }
  };

  internal_bleeding_t* internal_bleeding;

  kidney_shot_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> find_class_spell( "Kidney Shot" ), options_str ),
    internal_bleeding( nullptr )
  {
    if ( p->talent.internal_bleeding )
    {
      internal_bleeding = p->get_secondary_trigger_action<internal_bleeding_t>( TRIGGER_INTERNAL_BLEEDING, "internal_bleeding" );
      add_child( internal_bleeding );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    trigger_prey_on_the_weak( state );

    if ( state->target->type == ENEMY_ADD && internal_bleeding )
    {
      internal_bleeding->trigger_secondary_action( state->target, cast_state( state )->cp );
    }
  }
};

// Cheap Shot ===============================================================

struct cheap_shot_t : public rogue_attack_t
{
  cheap_shot_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> find_class_spell( "Cheap Shot" ), options_str )
  {
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();
    // TODO: Shot in the Dark talent reduces cost to free
    return c;
  }

  bool procs_main_gauche() const override
  { return false; }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );
    trigger_prey_on_the_weak( state );
    trigger_find_weakness( state );
    trigger_akaaris_soul_fragment( state );
  }
};

// Poison Bomb ==============================================================

struct poison_bomb_t : public rogue_spell_t
{
  poison_bomb_t( util::string_view name, rogue_t* p ) :
    rogue_spell_t( name, p, p -> find_spell( 255546 ) )
  {
    aoe = -1;
  }
};

// Poisoned Knife ===========================================================

struct poisoned_knife_t : public rogue_attack_t
{
  poisoned_knife_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p -> find_specialization_spell( "Poisoned Knife" ), options_str )
  { }

  double composite_poison_flat_modifier( const action_state_t* ) const override
  { return 1.0; }
};

// ==========================================================================
// Covenant Abilities
// ==========================================================================

// Echoing Reprimand ========================================================

struct echoing_reprimand_t : public rogue_attack_t
{
  std::vector<buff_t*> buffs;

  echoing_reprimand_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p->covenant.echoing_reprimand, options_str )
  {
    buffs = { p->buffs.echoing_reprimand_2, p->buffs.echoing_reprimand_3, p->buffs.echoing_reprimand_4 };
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( result_is_hit( state->result ) )
    {
      unsigned buff_idx = static_cast<int>( rng().range( 0, as<double>( buffs.size() ) ) );
      buffs[ buff_idx ]->trigger();
    }
  }
};

// Sepsis ===================================================================

struct sepsis_t : public rogue_attack_t
{
  struct sepsis_expire_damage_t : public rogue_attack_t
  {
    sepsis_expire_damage_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->find_spell( 328306 ) )
    {
      dual = true;
      base_multiplier *= p->covenant.sepsis->effectN( 4 ).base_value();
    }
  };

  sepsis_expire_damage_t* sepsis_expire_damage;

  sepsis_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p->covenant.sepsis, options_str )
  {
    sepsis_expire_damage = p->get_background_action<sepsis_expire_damage_t>( "sepsis_expire_damage" );
    sepsis_expire_damage->stats = stats;
  }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_ta_multiplier( state );

    // TOCHECK: Possibly refactor this when a proper buff is put into the game or logs are available
    if ( p()->conduit.septic_shock.ok() )
    {
      const dot_t* dot = td( state->target )->dots.sepsis;
      const double divisor = p()->conduit.septic_shock->effectN( 2 ).percent();
      const double multiplier = std::max( 1.0 / divisor - dot->current_tick, 0.0 ) * divisor; 
      m *= 1.0 + p()->conduit.septic_shock.percent() * multiplier;
    }

    return m;
  }

  void last_tick( dot_t* d ) override
  {
    rogue_attack_t::last_tick( d );
    p()->buffs.vanish->trigger(); // Vanish triggers before final burst damage
    trigger_master_of_shadows();
    sepsis_expire_damage->set_target( d->target );
    sepsis_expire_damage->execute();
  }
};

// Serrated Bone Spike ======================================================

struct serrated_bone_spike_t : public rogue_attack_t
{
  struct serrated_bone_spike_shatter_t : public rogue_attack_t
  {
    serrated_bone_spike_shatter_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->find_spell( 324074 ) )
    {
    }
  };

  struct serrated_bone_spike_dot_t : public rogue_attack_t
  {
    struct sudden_fractures_t : public rogue_attack_t
    {
      sudden_fractures_t( util::string_view name, rogue_t* p ) :
        rogue_attack_t( name, p, p->find_spell( 341277 ) )
      {
      }
    };

    sudden_fractures_t* sudden_fractures;

    serrated_bone_spike_dot_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->covenant.serrated_bone_spike->effectN( 2 ).trigger() ),
      sudden_fractures( nullptr )
    {
      dot_duration = timespan_t::from_seconds( sim->expected_max_time() * 2 );

      if ( p->conduit.sudden_fractures.ok() )
      {
        sudden_fractures = p->get_background_action<sudden_fractures_t>( "sudden_fractures" );
      }
    }

    void tick( dot_t* d ) override
    {
      rogue_attack_t::tick( d );

      if ( sudden_fractures && p()->rng().roll( p()->conduit.sudden_fractures.percent() ) )
      {
        sudden_fractures->set_target( d->target );
        sudden_fractures->execute();
      }
    }
  };

  serrated_bone_spike_dot_t* serrated_bone_spike_dot;
  serrated_bone_spike_shatter_t* serrated_bone_spike_shatter;

  serrated_bone_spike_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p->covenant.serrated_bone_spike, options_str )
  {
    serrated_bone_spike_dot = p->get_background_action<serrated_bone_spike_dot_t>( "serrated_bone_spike_dot" );
    serrated_bone_spike_shatter = p->get_background_action<serrated_bone_spike_shatter_t>( "serrated_bone_spike_shatter" );
    add_child( serrated_bone_spike_dot );
    add_child( serrated_bone_spike_shatter );
    if ( serrated_bone_spike_dot->sudden_fractures )
    {
      add_child( serrated_bone_spike_dot->sudden_fractures );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    unsigned active_dots = p()->get_active_dots( serrated_bone_spike_dot->internal_id );
    
    if ( !td( target )->dots.serrated_bone_spike->is_ticking() )
    {
      serrated_bone_spike_dot->set_target( target );
      serrated_bone_spike_dot->execute();
    }
    else
    {
      active_dots--;
    }

    // TOCHECK: Tooltip is quite ambiguous as to what is intended here with the shatter
    // Right now uses 200% damage spell which is obviously wrong, marking as bugged for now
    if ( p()->bugs )
    {
      for ( unsigned i = 0; i < active_dots; ++i )
      {
        serrated_bone_spike_shatter->set_target( target );
        serrated_bone_spike_shatter->execute();
      }
    }

    trigger_combo_point_gain( active_dots + 1, p()->gains.serrated_bone_spike );
  }
};

// Slaughter ================================================================

struct slaughter_t : public rogue_attack_t
{
  slaughter_poison_t* slaughter_poison;

  slaughter_t( util::string_view name, rogue_t* p, const std::string& options_str = "" ) :
    rogue_attack_t( name, p, p->covenant.slaughter, options_str )
  {
    slaughter_poison = p->get_background_action<slaughter_poison_t>( "slaughter_poison_driver" );
  }

  // TOCHECK: May need to move this to whitelist in base unless they fix it being applied to poison ticks
  double get_slaughter_scars_multiplier() const
  {
    if ( !p()->conduit.slaughter_scars.ok() )
      return 0.0;

    const double active_bonus = ( p()->active.lethal_poison == slaughter_poison ) ?
      1.0 + p()->conduit.slaughter_scars->effectN( 2 ).percent() : 1.0;
    
    return p()->conduit.slaughter_scars.percent() * active_bonus;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_da_multiplier( state );
    m *= 1.0 + get_slaughter_scars_multiplier();
    return m;
  }

  double composite_crit_chance() const override
  {
    return rogue_attack_t::composite_crit_chance() + get_slaughter_scars_multiplier();
  }

  void execute() override
  {
    rogue_attack_t::execute();
    trigger_count_the_odds( execute_state );
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );
    p()->active.lethal_poison = slaughter_poison; // TODO: Support expiry?
  }

  // TOCHECK: Copied from Ambush since this ability was largely cloned
  bool procs_main_gauche() const override
  { return false; }
};

// ==========================================================================
// Cancel AutoAttack
// ==========================================================================

struct cancel_autoattack_t : public action_t
{
  rogue_t* rogue;
  cancel_autoattack_t( rogue_t* rogue_, const std::string& options_str = "" ) :
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
    rogue->cancel_auto_attack();
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

template<typename Base>
std::unique_ptr<expr_t> actions::rogue_action_t<Base>::create_expression( util::string_view name_str )
{
  if ( util::str_compare_ci( name_str, "cp_gain" ) )
  {
    return make_mem_fn_expr( "cp_gain", *this, &base_t::generate_cp );
  }
  // Garrote and Rupture and APL lines using "exsanguinated"
  // TODO: Add Internal Bleeding (not the same id as Kidney Shot)
  else if ( util::str_compare_ci( name_str, "exsanguinated" ) &&
            ( ab::data().id() == 703 || ab::data().id() == 1943 || this -> name_str == "crimson_tempest" ) )
  {
    return std::make_unique<exsanguinated_expr_t>( this );
  }
  else if ( util::str_compare_ci( name_str, "ss_buffed") )
  {
    return make_fn_expr( "ss_buffed", [ this ]() {
    rogue_td_t* td_ = td( ab::target );
    if ( ! td_ -> dots.garrote -> is_ticking() )
    {
      return 0.0;
    }
    return debug_cast<const garrote_t::garrote_state_t*>( td_ -> dots.garrote -> state ) -> shrouded_suffocation ? 1.0 : 0.0;
  } );
  }

  return ab::create_expression( name_str );
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

namespace buffs {
// ==========================================================================
// Buffs
// ==========================================================================

struct proxy_rupture_t : public buff_t
{
  dot_t* rupture_dot;

  proxy_rupture_t( rogue_td_t& r ) :
    buff_t( r, "rupture", r.source -> find_specialization_spell( "Rupture" ) ),
    rupture_dot( r.dots.rupture )
  {
    set_quiet( true );
    set_cooldown( timespan_t::zero() );
    set_period( timespan_t::zero() );
    set_refresh_behavior( buff_refresh_behavior::DURATION );
  }

  void execute( int stacks, double value, timespan_t ) override
  {
    // Sync with Rup duration
    buff_t::execute( stacks, value, rupture_dot -> duration() );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    buff_t::start( stacks, value, duration );

    rogue_t* rogue = debug_cast<rogue_t*>( source );
    if ( rogue -> azerite.scent_of_blood.ok() )
      rogue -> buffs.scent_of_blood -> increment();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    rogue_t* rogue = debug_cast<rogue_t*>( source );
    rogue -> buffs.scent_of_blood -> decrement();
  }
};

struct adrenaline_rush_t : public buff_t
{
  adrenaline_rush_t( rogue_t* p ) :
    buff_t( p, "adrenaline_rush", p->spec.adrenaline_rush )
  { 
    set_cooldown( timespan_t::zero() );
    set_default_value( p->spec.adrenaline_rush->effectN( 2 ).percent() );
    set_affects_regen( true );
    add_invalidate( CACHE_ATTACK_SPEED );
    if ( p->legendary.celerity.ok() )
      add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  void trigger_secondary_procs()
  {
    // 6/23/2019 - Vision of Perfection refresh procs trigger Loaded Dice and extend Brigand's Blitz
    rogue_t* rogue = debug_cast<rogue_t*>( source );
    if ( rogue->talent.loaded_dice->ok() )
      rogue->buffs.loaded_dice->trigger();

    rogue->buffs.brigands_blitz_driver->trigger();
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    buff_t::start( stacks, value, duration );

    rogue_t* rogue = debug_cast<rogue_t*>( source );
    rogue->resources.temporary[ RESOURCE_ENERGY ] += data().effectN( 4 ).base_value();
    rogue->recalculate_resource_max( RESOURCE_ENERGY );
    trigger_secondary_procs();
  }

  void refresh( int stacks, double value, timespan_t duration ) override
  {
    buff_t::refresh( stacks, value, duration );
    trigger_secondary_procs();
  }

  void extend_duration( player_t* p, timespan_t extra_seconds ) override
  {
    buff_t::extend_duration( p, extra_seconds );
    trigger_secondary_procs();
  }

  void expire_override(int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    rogue_t* rogue = debug_cast<rogue_t*>( source );
    rogue->resources.temporary[ RESOURCE_ENERGY ] -= data().effectN( 4 ).base_value();
    rogue->recalculate_resource_max( RESOURCE_ENERGY, rogue->gains.adrenaline_rush_expiry );

    // 6/22/2019 - Brigand's Blitz expires when the Adrenaline Rush buff expires, regardless of duration
    //             This is mostly relevant due to Vision of Perfection procs
    rogue->buffs.brigands_blitz_driver->expire();
    rogue->buffs.brigands_blitz->expire();
  }
};

struct blade_flurry_t : public buff_t
{
  blade_flurry_t( rogue_t* p ) :
    buff_t( p, "blade_flurry", p -> spec.blade_flurry )
  {
    set_cooldown( timespan_t::zero() );
    set_default_value_from_effect( 2 );
    apply_affecting_aura( p->talent.dancing_steel );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    rogue_t* rogue = debug_cast<rogue_t*>( source );
    rogue -> buffs.keep_your_wits_about_you -> expire();
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
    rogue->break_stealth();
  }
};

struct stealth_like_buff_t : public buff_t
{
  rogue_t* rogue;

  stealth_like_buff_t( rogue_t* r, util::string_view name, const spell_data_t* spell ) :
    buff_t( r, name, spell ), rogue( r )
  { }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    if ( rogue->stealthed( STEALTH_BASIC ) )
    {
      if ( rogue->talent.master_assassin->ok() )
        rogue->buffs.master_assassin_aura->trigger();
      if ( rogue->legendary.master_assassins_mark->ok() )
        rogue->buffs.master_assassins_mark_aura->trigger();
    }

    if ( rogue->talent.premeditation->ok() && rogue->stealthed( STEALTH_BASIC | STEALTH_SHADOWDANCE ) )
      rogue->buffs.premeditation->trigger();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    // Don't swap these buffs around if we are still in stealth due to Vanish expiring
    if ( !rogue->stealthed( STEALTH_BASIC ) )
    {
      rogue->buffs.master_assassin_aura->expire();
      rogue->buffs.master_assassins_mark_aura->expire();
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
    set_duration( sim->max_time / 2 );
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    stealth_like_buff_t::execute( stacks, value, duration );
    rogue->cancel_auto_attack();

    // Activating stealth buff (via expiring Vanish) also removes Shadow Dance. Not an issue in general since Stealth cannot be used during Dance.
    rogue->buffs.shadow_dance->expire();
  }
};

// Vanish now acts like "stealth like abilities".
struct vanish_t : public stealth_like_buff_t
{
  std::vector<cooldown_t*> shadowdust_cooldowns;

  vanish_t( rogue_t* r ) :
    stealth_like_buff_t( r, "vanish", r->find_spell( 11327 ) )
  {
    if ( r->legendary.invigorating_shadowdust.ok() )
    {
      // TOCHECK: Double check what all this does not apply to
      shadowdust_cooldowns = { r->cooldowns.adrenaline_rush, r->cooldowns.between_the_eyes, r->cooldowns.blade_flurry,
        r->cooldowns.blade_rush, r->cooldowns.blind, r->cooldowns.cloak_of_shadows, r->cooldowns.dreadblades, r->cooldowns.garrote,
        r->cooldowns.ghostly_strike, r->cooldowns.gouge, r->cooldowns.grappling_hook, r->cooldowns.killing_spree,
        r->cooldowns.marked_for_death, r->cooldowns.riposte, r->cooldowns.roll_the_bones, r->cooldowns.secret_technique,
        r->cooldowns.sepsis, r->cooldowns.serrated_bone_spike, r->cooldowns.shadow_blades, r->cooldowns.shadow_dance,
        r->cooldowns.shiv, r->cooldowns.sprint, r->cooldowns.symbols_of_death, r->cooldowns.vendetta };
    }
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    stealth_like_buff_t::execute( stacks, value, duration );
    rogue->cancel_auto_attack();

    // Confirmed on beta Invigorating Shadowdust triggers from Vanish buff via Sepsis, not just Vanish casts
    if ( rogue->legendary.invigorating_shadowdust.ok() )
    {
      const timespan_t reduction = timespan_t::from_seconds( rogue->legendary.invigorating_shadowdust->effectN( 1 ).base_value() );
      for ( cooldown_t* c : shadowdust_cooldowns )
      {
        if ( c && c->down() )
          c->adjust( -reduction, false );
      }
    }

    // Confirmed on beta Deathly Shadows triggers from Vanish buff via Sepsis, not just Vanish casts
    if ( rogue->buffs.deathly_shadows->trigger() )
    {
      const int combo_points = as<int>( rogue->buffs.deathly_shadows->data().effectN( 3 ).base_value() );
      rogue->resource_gain( RESOURCE_COMBO_POINT, combo_points, rogue->gains.deathly_shadows );
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    // Stealth proc if Vanish fully ends (i.e. isn't broken before the expiration)
    if ( remaining_duration == timespan_t::zero() )
    {
      rogue->buffs.stealth->trigger();
    }

    stealth_like_buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// Shadow dance acts like "stealth like abilities"
struct shadow_dance_t : public stealth_like_buff_t
{
  shadow_dance_t( rogue_t* p ) :
    stealth_like_buff_t( p, "shadow_dance", p -> spec.shadow_dance )
  {
    apply_affecting_aura( p->talent.subterfuge );

    if ( p->talent.dark_shadow->ok() )
    {
      add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    stealth_like_buff_t::expire_override( expiration_stacks, remaining_duration );
    rogue->buffs.the_first_dance->expire();
  }
};

struct shuriken_tornado_t : public buff_t
{
  rogue_t* rogue;
  actions::shuriken_storm_t* shuriken_storm_action;

  shuriken_tornado_t( rogue_t* r ) :
    buff_t( r, "shuriken_tornado", r -> talent.shuriken_tornado ),
    rogue( r ),
    shuriken_storm_action( nullptr )
  {
    set_cooldown( timespan_t::zero() );
    set_period( timespan_t::from_seconds( 1.0 ) ); // Not explicitly in spell data
    
    shuriken_storm_action = r->get_secondary_trigger_action<actions::shuriken_storm_t>( TRIGGER_SHURIKEN_TORNADO, "shuriken_storm_tornado" );
    set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
      shuriken_storm_action->trigger_secondary_action( rogue->target );
    } );
  }
};

struct rogue_poison_buff_t : public buff_t
{
  rogue_poison_buff_t( rogue_td_t& r, util::string_view name, const spell_data_t* spell ) :
    buff_t( r, name, spell )
  { }
};

struct wound_poison_t : public rogue_poison_buff_t
{
  wound_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "wound_poison", r.source->find_class_spell( "Wound Poison" )->effectN( 1 ).trigger() )
  { 
    apply_affecting_aura( debug_cast<rogue_t*>( r.source )->spec.wound_poison_2 );
  }
};

struct crippling_poison_t : public rogue_poison_buff_t
{
  crippling_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "crippling_poison", r.source->find_class_spell( "Crippling Poison" )->effectN( 1 ).trigger() )
  { }
};

struct numbing_poison_t : public rogue_poison_buff_t
{
  numbing_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "numbing_poison", r.source->find_class_spell( "Numbing Poison" )->effectN( 1 ).trigger() )
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

struct vendetta_debuff_t : public buff_t
{
  action_t* nothing_personal;

  vendetta_debuff_t( rogue_td_t& r ) :
    buff_t( r, "vendetta", r.source->find_specialization_spell( "Vendetta" ) ),
    nothing_personal( r.source->find_action( "nothing_personal" ) )
  {
    set_cooldown( timespan_t::zero() );
    set_default_value( data().effectN( 1 ).percent() );
  }

  void trigger_nothing_personal( timespan_t duration )
  {
    // 3/25/2020 - Vision of Perfection refresh procs trigger/extend both Nothing Personal effects
    rogue_t* rogue = debug_cast<rogue_t*>( source );
    if ( !rogue->azerite.nothing_personal.ok() || nothing_personal == nullptr )
      return;

    // Debuff Trigger/Refresh
    rogue_td_t* td = rogue->get_target_data( player );
    if ( td->dots.nothing_personal->is_ticking() )
    {
      td->dots.nothing_personal->extend_duration( duration );
    }
    else
    {
      nothing_personal->set_target( player );
      nothing_personal->dot_duration = duration;
      nothing_personal->schedule_execute();
    }

    // Buff Trigger/Refresh
    if ( rogue->buffs.nothing_personal->check() )
    {
      rogue->buffs.nothing_personal->extend_duration( rogue, duration );
    }
    else
    {
      rogue->buffs.nothing_personal->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
    }
  }

  void expire_nothing_personal()
  {
    rogue_t* rogue = debug_cast<rogue_t*>( source );
    if ( !rogue->azerite.nothing_personal.ok() )
      return;

    rogue->buffs.nothing_personal->expire();
    rogue->get_target_data( player )->dots.nothing_personal->cancel();
  }

  void extend_duration( player_t* p, timespan_t extra_seconds ) override
  {
    buff_t::extend_duration( p, extra_seconds );
    trigger_nothing_personal( extra_seconds );
  }

  void refresh( int stacks, double value, timespan_t duration ) override
  {
    buff_t::refresh( stacks, value, duration );
    trigger_nothing_personal( duration );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    buff_t::start( stacks, value, duration );

    // 3/25/2020 - The base 3s duration regen buff does not re-apply on refreshes
    rogue_t* rogue = debug_cast<rogue_t*>( source );
    rogue->buffs.vendetta->trigger();
    trigger_nothing_personal( remains() );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
    expire_nothing_personal();
  }
};

struct roll_the_bones_t : public buff_t
{
  rogue_t* rogue;
  std::array<buff_t*, 6> buffs;
  std::array<proc_t*, 6> procs;
  std::vector<timespan_t> count_the_odds_remains;

  roll_the_bones_t( rogue_t* r ) :
    buff_t( r, "roll_the_bones", r -> spec.roll_the_bones ),
    rogue( r )
  {
    set_period( timespan_t::zero() ); // Disable ticking
    set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

    buffs = {
      rogue->buffs.broadside,
      rogue->buffs.buried_treasure,
      rogue->buffs.grand_melee,
      rogue->buffs.ruthless_precision,
      rogue->buffs.skull_and_crossbones,
      rogue->buffs.true_bearing
    };
  }

  void set_procs()
  {
    procs = {
      rogue->procs.roll_the_bones_1,
      rogue->procs.roll_the_bones_2,
      rogue->procs.roll_the_bones_3,
      rogue->procs.roll_the_bones_4,
      rogue->procs.roll_the_bones_5,
      rogue->procs.roll_the_bones_6
    };
  }

  void expire_secondary_buffs()
  {
    for ( auto buff : buffs )
    {
      buff->expire();
    }
    rogue -> buffs.paradise_lost -> expire();
  }

  void count_the_odds_trigger( timespan_t duration )
  {
    if ( !rogue->conduit.count_the_odds.ok() )
      return;

    std::vector<buff_t*> inactive_buffs;
    for ( buff_t* buff : buffs )
    {
      if ( !buff->check() )
        inactive_buffs.push_back( buff );
    }

    if ( inactive_buffs.empty() )
      return;

    unsigned buff_idx = static_cast<int>( rng().range( 0, as<double>( inactive_buffs.size() ) ) );
    inactive_buffs[ buff_idx ]->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
  }

  void count_the_odds_check()
  {
    if ( !rogue->conduit.count_the_odds.ok() )
      return;

    // TOCHECK: Assuming this works the same as the T21 4pc bonus for now
    // Collect a list of buffs with partially triggered durations
    count_the_odds_remains.clear();
    for ( buff_t* buff : buffs )
    {
      if ( buff->check() && buff->remains() != remains() )
        count_the_odds_remains.push_back( buff->remains() );
    }
  }

  void count_the_odds_reroll()
  {
    if ( count_the_odds_remains.empty() )
      return;

    // TOCHECK: Assuming this works the same as the T21 4pc bonus for now
    // Trigger random inactive buffs with the previously remaining durations
    for ( timespan_t remains : count_the_odds_remains )
    {
      count_the_odds_trigger( remains );
    }
  }

  std::vector<buff_t*> random_roll( bool loaded_dice )
  {
    std::vector<buff_t*> rolled;

    if ( rogue->options.fixed_rtb_odds.empty() )
    {
      // RtB uses hardcoded probabilities since 7.2.5
      // As of 2017-05-18 assume these:
      // -- The current proposal is to reduce the number of dice being rolled from 6 to 5
      // -- and to hand-set probabilities to something like 79% chance for 1-buff, 20% chance
      // -- for 2-buffs, and 1% chance for 5-buffs (yahtzee), bringing the expected value of
      // -- a roll down to 1.24 buffs (plus additional value for synergies between buffs).
      // Source: https://us.battle.net/forums/en/wow/topic/20753815486?page=2#post-21
      // Odds double checked on 2020-03-09.
      rogue->options.fixed_rtb_odds = { 79.0, 20.0, 0.0, 0.0, 1.0, 0.0 };
    }

    if ( !rogue->options.fixed_rtb_odds.empty() )
    {
      std::vector<double> current_odds = rogue->options.fixed_rtb_odds;
      if ( loaded_dice )
      {
        // At some point Loaded Dice were apparently changed to just convert 1 buffs straight into two buffs. (2020-03-09)
        current_odds[ 1 ] += current_odds[ 0 ];
        current_odds[ 0 ] = 0.0;
      }

      double odd_sum = 0.0;
      for ( const double& chance : current_odds )
        odd_sum += chance;

      double roll = rng().range( 0.0, odd_sum );
      size_t num_buffs = 0;
      double aggregate = 0.0;
      for ( const double& chance : current_odds )
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
    range::for_each( rogue->options.fixed_rtb, [ this, &rolled ]( size_t idx )
      { rolled.push_back( buffs[ idx ] ); } );
    return rolled;
  }

  unsigned roll_the_bones( timespan_t duration )
  {
    std::vector<buff_t*> rolled;
    if ( rogue->options.fixed_rtb.size() == 0 )
    {
      rolled = random_roll( rogue->buffs.loaded_dice->up() );
    }
    else
    {
      rolled = fixed_roll();
    }

    for ( auto buff : rolled )
    {
      buff->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
    }

    return as<unsigned>( rolled.size() );
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    count_the_odds_check();

    buff_t::execute( stacks, value, duration );

    expire_secondary_buffs();

    const timespan_t roll_duration = remains();
    const int buffs_rolled = roll_the_bones( roll_duration );

    procs[ buffs_rolled - 1 ]->occur();
    rogue->buffs.loaded_dice->expire();

    if ( buffs_rolled == 1 && rogue->azerite.paradise_lost.ok() )
    {
      rogue->buffs.paradise_lost->trigger( 1, buff_t::DEFAULT_VALUE(), ( -1.0 ), roll_duration );
    }

    count_the_odds_reroll();
  }
};

} // end namespace buffs

// ==========================================================================
// Rogue Triggers
// ==========================================================================

void rogue_t::trigger_venomous_wounds_death( player_t* target )
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( sim->event_mgr.canceled )
  {
    return;
  }

  if ( !spec.venomous_wounds->ok() )
    return;

  rogue_td_t* td = get_target_data( target );
  if ( !td->is_poisoned() )
    return;

  // No end event means it naturally expired
  if ( !td->dots.rupture->is_ticking() || td->dots.rupture->remains() == timespan_t::zero() )
  {
    return;
  }

  // TODO: Exact formula?
  unsigned full_ticks_remaining =
      (unsigned)( td->dots.rupture->remains() / td->dots.rupture->current_action->base_tick_time );
  int replenish = as<int>( spec.venomous_wounds->effectN( 2 ).base_value() );

  if ( sim->debug )
  {
    sim->out_debug.printf(
        "%s venomous_wounds replenish on target death: full_ticks=%u, ticks_left=%u, vw_replenish=%d, "
        "remaining_time=%.3f",
        name(), full_ticks_remaining, td->dots.rupture->ticks_left(), replenish,
        td->dots.rupture->remains().total_seconds() );
  }

  resource_gain( RESOURCE_ENERGY, full_ticks_remaining * replenish, gains.venomous_wounds_death,
                 td->dots.rupture->current_action );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_auto_attack( const action_state_t* /* state */ )
{
  if ( p()->main_hand_attack->execute_event || !p()->off_hand_attack || p()->off_hand_attack->execute_event )
    return;

  if ( !ab::data().flags( spell_attribute::SX_MELEE_COMBAT_START ) )
    return;

  p()->melee_main_hand->first = true;
  if ( p()->melee_off_hand )
    p()->melee_off_hand->first = true;

  p()->auto_attack->execute();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_seal_fate( const action_state_t* state )
{
  if ( !p()->spec.seal_fate->ok() )
    return;

  if ( state->result != RESULT_CRIT )
    return;

  trigger_combo_point_gain( 1, p()->gains.seal_fate );

  p()->procs.seal_fate->occur();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_main_gauche( const action_state_t* state )
{
  if ( !p()->mastery.main_gauche->ok() )
    return;

  if ( state->result_total <= 0 )
    return;

  if ( !ab::result_is_hit( state->result ) )
    return;

  if ( !procs_main_gauche() )
    return;

  double proc_chance = p()->mastery.main_gauche->proc_chance();
  if ( p()->conduit.ambidexterity.ok() && p()->buffs.blade_flurry->check() )
    proc_chance += p()->conduit.ambidexterity.percent();

  if ( !p()->rng().roll( proc_chance ) )
    return;

  p()->active.main_gauche->set_target( state->target );
  p()->active.main_gauche->schedule_execute();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_combat_potency( const action_state_t* state )
{
  if ( !p()->spec.combat_potency_2->ok() || !ab::result_is_hit( state->result ) || !procs_combat_potency() )
    return;

  double chance = p()->spec.combat_potency_2->effectN( 1 ).percent();
  // Looks like CP proc chance is normalized by weapon speed (i.e. penalty for using daggers)
  if ( state->action != p()->active.main_gauche && state->action->weapon )
    chance *= state->action->weapon->swing_time.total_seconds() / 2.6;
  if ( !p()->rng().roll( chance ) )
    return;

  // energy gain value is in the proc trigger spell
  double gain = p()->spec.combat_potency_2->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_ENERGY );

  p()->resource_gain( RESOURCE_ENERGY, gain, p()->gains.combat_potency, state->action );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_energy_refund( const action_state_t* /* state */ )
{
  double energy_restored = ab::last_resource_cost * 0.80;
  p()->resource_gain( RESOURCE_ENERGY, energy_restored, p()->gains.energy_refund );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_poison_bomb( const action_state_t* state )
{
  if ( !p()->talent.poison_bomb->ok() || !ab::result_is_hit( state->result ) )
    return;

  // They put 25 as value in spell data and divide it by 10 later, it's due to the int restriction.
  const actions::rogue_action_state_t* s = cast_state( state );
  if ( p()->rng().roll( p()->talent.poison_bomb->effectN( 1 ).percent() / 10 * s->cp ) )
  {
    make_event<ground_aoe_event_t>( *p()->sim, p(), ground_aoe_params_t()
      .target( state->target )
      .pulse_time( p()->spell.poison_bomb_driver->duration() / p()->talent.poison_bomb->effectN( 2 ).base_value() )
      .duration( p()->spell.poison_bomb_driver->duration() )
      .action( p()->active.poison_bomb ) );
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_venomous_wounds( const action_state_t* state )
{
  if ( !p()->spec.venomous_wounds->ok() )
    return;

  if ( !p()->get_target_data( state->target )->is_poisoned() )
    return;

  if ( !ab::result_is_hit( state->result ) )
    return;

  double chance = p()->spec.venomous_wounds->proc_chance();

  if ( !p()->rng().roll( chance ) )
    return;

  p()->resource_gain( RESOURCE_ENERGY, p()->spec.venomous_wounds->effectN( 2 ).base_value(),
                      p()->gains.venomous_wounds );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_blade_flurry( const action_state_t* state )
{
  if ( state->result_total <= 0 )
    return;

  if ( !p()->buffs.blade_flurry->check() )
    return;

  if ( !ab::result_is_hit( state->result ) )
    return;

  if ( ab::is_aoe() )
    return;

  if ( p()->sim->active_enemies == 1 )
    return;

  bool procs_keep_your_wits = p()->azerite.keep_your_wits_about_you.ok();

  // Compute Blade Flurry modifier
  double multiplier = 1.0;
  if ( ab::name_str == "killing_spree_mh" || ab::name_str == "killing_spree_oh" )
  {
    // 3/27/2020 -- Killing Spree no longer procs Wits twice per teleport
    if ( state->action->weapon == &( p()->off_hand_weapon ) )
    {
      procs_keep_your_wits = false;
    }

    multiplier = p()->talent.killing_spree->effectN( 2 ).percent();
  }
  else
  {
    multiplier = p()->buffs.blade_flurry->check_value();
    if ( p()->bugs )
    {
      // 3/37/2020 -- Dancing Steel increases Blade Flurry by 10% not 5%
      multiplier += p()->talent.dancing_steel->effectN( 3 ).percent();
    }
  }

  // Target multipliers do not replicate to secondary targets, need to reverse it out
  const double target_da_multiplier = ( 1.0 / state->target_da_multiplier );

  // Note, unmitigated damage
  double damage = state->result_total * multiplier * target_da_multiplier;
  p()->active.blade_flurry->base_dd_min = damage;
  p()->active.blade_flurry->base_dd_max = damage;
  p()->active.blade_flurry->set_target( state->target );
  p()->active.blade_flurry->execute();

  // Keep triggers once per instance with 8.2
  if ( procs_keep_your_wits )
  {
    p()->buffs.keep_your_wits_about_you->trigger();
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_combo_point_gain( int cp, gain_t* gain )
{
  p()->resource_gain( RESOURCE_COMBO_POINT, cp, gain, this );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_ruthlessness_cp( const action_state_t* state )
{
  if ( !p()->spec.ruthlessness->ok() )
    return;

  if ( !ab::result_is_hit( state->result ) )
    return;

  if ( !affected_by.ruthlessness )
    return;

  const rogue_action_state_t* s = cast_state( state );
  if ( s->cp == 0 )
    return;

  double cp_chance = p()->spec.ruthlessness->effectN( 1 ).pp_combo_points() * s->cp / 100.0;
  int cp_gain      = 0;
  if ( cp_chance > 1 )
  {
    cp_gain += 1;
    cp_chance -= 1;
  }

  if ( p()->rng().roll( cp_chance ) )
  {
    cp_gain += 1;
  }

  if ( cp_gain > 0 )
  {
    trigger_combo_point_gain( cp_gain, p()->gains.ruthlessness );
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_deepening_shadows( const action_state_t* state )
{
  if ( !p()->spec.deepening_shadows->ok() || !affected_by.deepening_shadows )
    return;

  auto s = cast_state( state );
  if ( s->cp == 0 )
    return;

  // Note: this changed to be 10 * seconds as of 2017-04-19
  int cdr = as<int>( p()->spec.deepening_shadows->effectN( 1 ).base_value() );
  if ( p()->talent.enveloping_shadows->ok() )
  {
    cdr += as<int>( p()->talent.enveloping_shadows->effectN( 1 ).base_value() );
  }
  timespan_t adjustment = timespan_t::from_seconds( -0.1 * cdr * s->cp );

  p()->cooldowns.shadow_dance->adjust( adjustment, s->cp >= 5 );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_shadow_techniques( const action_state_t* state )
{
  if ( !p()->spec.shadow_techniques->ok() || !ab::result_is_hit( state->result ) )
    return;

  if ( p()->sim->debug )
    p()->sim->out_debug.printf( "Melee attack landed, so shadow techniques increment from %d to %d", p()->shadow_techniques, p()->shadow_techniques + 1 );

  if ( ++p()->shadow_techniques == 5 || ( p()->shadow_techniques == 4 && p()->rng().roll( 0.5 ) ) )
  {
    if ( p()->sim->debug )
      p()->sim->out_debug.printf( "Shadow techniques proc'd at %d, resetting counter to 0", p()->shadow_techniques );
    
    p()->shadow_techniques = 0;
    p()->resource_gain( RESOURCE_ENERGY, p()->spec.shadow_techniques_effect->effectN( 2 ).base_value(), p()->gains.shadow_techniques, state->action );
    trigger_combo_point_gain( as<int>( p()->spec.shadow_techniques_effect->effectN( 1 ).base_value() ), p()->gains.shadow_techniques );
    if ( p()->conduit.stiletto_staccato.ok() )
      p()->cooldowns.shadow_blades->adjust( -p()->conduit.stiletto_staccato.time_value( conduit_data_t::time_type::S ), true );
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_weaponmaster( const action_state_t* state, actions::rogue_attack_t* action )
{
  if ( !p()->talent.weaponmaster->ok() || !ab::result_is_hit( state->result ) || p()->cooldowns.weaponmaster->down() || !action )
    return;

  if ( !p()->rng().roll( p()->talent.weaponmaster->proc_chance() ) )
    return;

  p()->procs.weaponmaster->occur();
  p()->cooldowns.weaponmaster->start( p()->talent.weaponmaster->internal_cooldown() );

  if ( p()->sim->debug )
  {
    p()->sim->out_debug.printf( "%s procs weaponmaster for %s", p()->name(), ab::name() );
  }

  // Direct damage re-computes on execute
  action->trigger_secondary_action( state->target, cast_state( state )->cp );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_elaborate_planning( const action_state_t* /* state */ )
{
  if ( !p()->talent.elaborate_planning->ok() || ab::base_costs[ RESOURCE_COMBO_POINT ] == 0 || ab::background )
    return;
  p()->buffs.elaborate_planning->trigger();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_alacrity( const action_state_t* state )
{
  if ( !p()->talent.alacrity->ok() || !affected_by.alacrity )
    return;

  double chance = p()->talent.alacrity->effectN( 2 ).percent() * cast_state( state )->cp;
  int stacks = 0;
  if ( chance > 1 )
  {
    stacks += 1;
    chance -= 1;
  }

  if ( p()->rng().roll( chance ) )
  {
    stacks += 1;
  }

  if ( stacks > 0 )
  {
    p()->buffs.alacrity->trigger( stacks );
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_restless_blades( const action_state_t* state )
{
  timespan_t v = timespan_t::from_seconds( p()->spec.restless_blades->effectN( 1 ).base_value() / 10.0 );
  v += timespan_t::from_seconds( p()->buffs.true_bearing->value() );
  v *= -cast_state( state )->cp;

  // Abilities
  p()->cooldowns.adrenaline_rush->adjust( v, false );
  p()->cooldowns.between_the_eyes->adjust( v, false );
  p()->cooldowns.blade_flurry->adjust( v, false );
  p()->cooldowns.grappling_hook->adjust( v, false );
  p()->cooldowns.roll_the_bones->adjust( v, false );
  p()->cooldowns.sprint->adjust( v, false );
  p()->cooldowns.vanish->adjust( v, false );
  // Talents
  p()->cooldowns.blade_rush->adjust( v, false );
  p()->cooldowns.ghostly_strike->adjust( v, false );
  p()->cooldowns.killing_spree->adjust( v, false );
  p()->cooldowns.marked_for_death->adjust( v, false );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_dreadblades( const action_state_t* state )
{
  if ( !p()->talent.dreadblades->ok() || !ab::result_is_hit( state->result ) )
    return;

  // TOCHECK: Double check everything triggers this correctly
  if ( ab::energize_type == action_energize::NONE || ab::energize_resource != RESOURCE_COMBO_POINT )
    return;

  if ( !p()->buffs.dreadblades->up() )
    return;

  trigger_combo_point_gain( as<int>( p()->buffs.dreadblades->check_value() ), p()->gains.dreadblades );
}

template <typename Base>
void actions::rogue_action_t<Base>::do_exsanguinate( dot_t* dot, double coeff )
{
  if ( !dot->is_ticking() )
  {
    return;
  }

  // Original and "logical" implementation.
  // dot -> adjust( coeff );
  // Since the advent of hasted bleed exsanguinate works differently though.
  dot->adjust_full_ticks( coeff );
  cast_state( dot->state )->exsanguinated = true;
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_exsanguinate( const action_state_t* state )
{
  rogue_td_t* td = p()->get_target_data( state->target );

  double coeff = 1.0 / ( 1.0 + p()->talent.exsanguinate->effectN( 1 ).percent() );

  do_exsanguinate( td->dots.garrote, coeff );
  do_exsanguinate( td->dots.internal_bleeding, coeff );
  do_exsanguinate( td->dots.rupture, coeff );
  do_exsanguinate( td->dots.crimson_tempest, coeff );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_relentless_strikes( const action_state_t* state )
{
  if ( !p()->spec.relentless_strikes->ok() || !affected_by.relentless_strikes )
    return;

  double grant_energy = 0;
  grant_energy += cast_state( state )->cp * p()->spell.relentless_strikes_energize->effectN( 1 ).resource( RESOURCE_ENERGY );

  if ( grant_energy > 0 )
  {
    p()->resource_gain( RESOURCE_ENERGY, grant_energy, p()->gains.relentless_strikes, state->action );
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::spend_combo_points( const action_state_t* state )
{
  if ( ab::base_costs[ RESOURCE_COMBO_POINT ] == 0 )
    return;

  if ( !ab::result_is_hit( state->result ) )
    return;

  double max_spend = std::min( p()->resources.current[ RESOURCE_COMBO_POINT ], p()->consume_cp_max() );
  ab::stats->consume_resource( RESOURCE_COMBO_POINT, max_spend );
  p()->resource_loss( RESOURCE_COMBO_POINT, max_spend );

  if ( p()->sim->log )
    p()->sim->out_log.printf( "%s consumes %.1f %s for %s (%.0f)", p()->name(), max_spend,
                              util::resource_type_string( RESOURCE_COMBO_POINT ), ab::name(),
                              p()->resources.current[ RESOURCE_COMBO_POINT ] );

  if ( ab::name_str != "secret_technique" )
  {
    // As of 2018-06-28 on beta, Secret Technique does not reduce its own cooldown. May be a bug or the cdr happening
    // before CD start.
    timespan_t sectec_cdr = timespan_t::from_seconds( p()->talent.secret_technique->effectN( 5 ).base_value() );
    sectec_cdr *= -max_spend;
    p()->cooldowns.secret_technique->adjust( sectec_cdr, false );
  }

  // Proc Replicating Shadows on the current target.
  if ( p()->specialization() == ROGUE_SUBTLETY && p()->active.replicating_shadows && 
       p()->rng().roll( max_spend * p()->azerite.replicating_shadows.spell_ref().effectN( 2 ).percent() ) )
  {
    p()->active.replicating_shadows->set_target( state->target );
    p()->active.replicating_shadows->execute();
  }

  // Remove Echoing Reprimand Buffs
  if ( p()->covenant.echoing_reprimand->ok() )
  {
    if ( max_spend == 2 && p()->buffs.echoing_reprimand_2->up() )
    {
      p()->buffs.echoing_reprimand_2->expire();
      p()->procs.echoing_reprimand_2->occur();
    }
    else if ( max_spend == 3 && p()->buffs.echoing_reprimand_3->up() )
    {
      p()->buffs.echoing_reprimand_3->expire();
      p()->procs.echoing_reprimand_3->occur();
    }
    else if ( max_spend == 4 && p()->buffs.echoing_reprimand_4->up() )
    {
      p()->buffs.echoing_reprimand_4->expire();
      p()->procs.echoing_reprimand_4->occur();
    }
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_shadow_blades_attack( action_state_t* state )
{
  if ( !p()->buffs.shadow_blades->check() || state->result_total <= 0 || !ab::result_is_hit( state->result ) || !affected_by.shadow_blades )
    return;

  double amount = state->result_amount * p()->buffs.shadow_blades->check_value();
  p()->active.shadow_blades_attack->base_dd_min = amount;
  p()->active.shadow_blades_attack->base_dd_max = amount;
  p()->active.shadow_blades_attack->set_target( state->target );
  p()->active.shadow_blades_attack->execute();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_inevitability( const action_state_t* state )
{
  if ( !ab::result_is_hit( state->result ) || !p()->azerite.inevitability.ok() )
    return;

  p()->buffs.symbols_of_death->extend_duration( p(), timespan_t::from_seconds( p()->azerite.inevitability.spell_ref().effectN( 2 ).base_value() / 10.0 ) );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_prey_on_the_weak( const action_state_t* state )
{
  if ( state->target->type != ENEMY_ADD || !ab::result_is_hit( state->result ) || !p()->talent.prey_on_the_weak->ok() )
    return;

  td( state->target )->debuffs.prey_on_the_weak->trigger();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_find_weakness( const action_state_t* state, timespan_t duration )
{
  if ( !ab::result_is_hit( state->result ) || !p()->spec.find_weakness->ok() )
    return;

  // If the new duration (e.g. from Backstab crits) is lower than the existing debuff duration, refresh without change.
  if ( duration > timespan_t::zero() && duration < td( state->target )->debuffs.find_weakness->remains() )
    duration = td( state->target )->debuffs.find_weakness->remains();

  td( state->target )->debuffs.find_weakness->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_grand_melee( const action_state_t* state )
{
  if ( !ab::result_is_hit( state->result ) || !p()->buffs.grand_melee->up() )
    return;

  timespan_t snd_extension = cast_state( state )->cp * timespan_t::from_seconds( p()->buffs.grand_melee->check_value() );
  if ( p()->buffs.slice_and_dice->check() )
    p()->buffs.slice_and_dice->extend_duration( p(), snd_extension );
  else
    p()->buffs.slice_and_dice->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, snd_extension );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_master_of_shadows()
{
  // Since Stealth from expiring Vanish cannot trigger this but expire_override already treats vanish as gone, we have to do this manually using this function.
  if ( p()->in_combat && p()->talent.master_of_shadows->ok() )
  {
    p()->buffs.master_of_shadows->trigger();
    p()->resource_gain( RESOURCE_ENERGY, p()->buffs.master_of_shadows->data().effectN( 2 ).base_value(), p()->gains.master_of_shadows );
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_akaaris_soul_fragment( const action_state_t* state )
{
  if ( !ab::result_is_hit( state->result ) || !p()->legendary.akaaris_soul_fragment->ok() || ab::background )
    return;

  td( state->target )->debuffs.akaaris_soul_fragment->trigger();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_bloodfang( const action_state_t* state )
{
  if ( !ab::result_is_hit( state->result ) || !p()->legendary.essence_of_bloodfang->ok() || !p()->active.bloodfang )
    return;

  // TOCHECK: Closer to launch, check for exceptions. Currently doesn't work with Garrote, Gloomblade, FoK, Storm
  if ( ab::energize_type == action_energize::NONE || ab::energize_resource != RESOURCE_COMBO_POINT )
    return;

  if ( !p()->rng().roll( p()->legendary.essence_of_bloodfang->proc_chance() ) )
    return;

  p()->active.bloodfang->set_target( state->target );
  p()->active.bloodfang->execute();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_count_the_odds( const action_state_t* state )
{
  if ( !ab::result_is_hit( state->result ) || !p()->conduit.count_the_odds.ok() || p()->specialization() != ROGUE_OUTLAW )
    return;

  if ( !p()->rng().roll( p()->conduit.count_the_odds.percent() ) )
    return;

  const timespan_t trigger_duration = timespan_t::from_seconds( p()->conduit.count_the_odds->effectN( 2 ).base_value() );
  debug_cast<buffs::roll_the_bones_t*>( p()->buffs.roll_the_bones )->count_the_odds_trigger( trigger_duration );
  p()->procs.count_the_odds->occur();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_guile_charm( const action_state_t* state )
{
  if ( !ab::result_is_hit( state->result ) || !p()->legendary.guile_charm.ok() )
    return;

  if ( p()->buffs.guile_charm_insight_3->check() )
    return;

  bool trigger_next_insight = ( ++p()->legendary.guile_charm_counter >= 4 );

  if ( p()->buffs.guile_charm_insight_1->check() )
  {
    if( trigger_next_insight )
      p()->buffs.guile_charm_insight_2->trigger();
    else
      p()->buffs.guile_charm_insight_1->refresh();
  }
  else if ( p()->buffs.guile_charm_insight_2->check() )
  {
    if ( trigger_next_insight )
      p()->buffs.guile_charm_insight_3->trigger();
    else
      p()->buffs.guile_charm_insight_2->refresh();
  }
  else if( trigger_next_insight )
  {
    p()->buffs.guile_charm_insight_1->trigger();
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
  dots.deadly_poison        = target->get_dot( "deadly_poison_dot", source );
  dots.garrote              = target->get_dot( "garrote", source );
  dots.internal_bleeding    = target->get_dot( "internal_bleeding", source );
  dots.rupture              = target->get_dot( "rupture", source );
  dots.crimson_tempest      = target->get_dot( "crimson_tempest", source );
  dots.nothing_personal     = target->get_dot( "nothing_personal", source );
  dots.sepsis               = target->get_dot( "sepsis", source );
  dots.slaughter_poison     = target->get_dot( "slaughter_poison_dot", source );
  dots.serrated_bone_spike  = target->get_dot( "serrated_bone_spike_dot", source );
  dots.mutilated_flesh      = target->get_dot( "mutilated_flesh", source );

  debuffs.marked_for_death  = new buffs::marked_for_death_debuff_t( *this );
  debuffs.wound_poison      = new buffs::wound_poison_t( *this );
  debuffs.crippling_poison  = new buffs::crippling_poison_t( *this );
  debuffs.numbing_poison    = new buffs::numbing_poison_t( *this );
  debuffs.rupture           = new buffs::proxy_rupture_t( *this );
  debuffs.vendetta          = new buffs::vendetta_debuff_t( *this );

  debuffs.shiv = make_buff( *this, "shiv", source->spec.shiv_2_debuff )
    ->set_default_value( source->spec.shiv_2_debuff->effectN( 1 ).percent() );
  debuffs.ghostly_strike = make_buff( *this, "ghostly_strike", source->talent.ghostly_strike )
    ->set_default_value( source->talent.ghostly_strike->effectN( 3 ).percent() );
  debuffs.find_weakness = make_buff( *this, "find_weakness", source->spec.find_weakness->effectN( 1 ).trigger() )
    ->set_default_value( source->spec.find_weakness->effectN( 1 ).trigger()->effectN( 1 ).percent() );
  debuffs.prey_on_the_weak = make_buff( *this, "prey_on_the_weak", source->find_spell( 255909 ) )
    ->set_default_value( source->find_spell( 255909 )->effectN( 1 ).percent() );
  debuffs.between_the_eyes = make_buff( *this, "between_the_eyes", source->spec.between_the_eyes )
    ->set_default_value( source->spec.between_the_eyes->effectN( 2 ).percent() )
    ->set_cooldown( timespan_t::zero() );

  if ( source->legendary.akaaris_soul_fragment.ok() )
  {
    debuffs.akaaris_soul_fragment = make_buff( *this, "akaaris_soul_fragment", source->find_spell( 341111 ) )
      ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
      ->set_tick_behavior( buff_tick_behavior::REFRESH )
      ->set_tick_callback( [ source, target ]( buff_t*, int, timespan_t ) {
        source->active.akaaris_soul_fragment->trigger_secondary_action( target );
      } )
      ->set_partial_tick( true );
  }

  // Register on-demise callback for assassination to perform Venomous Wounds energy replenish on death.
  if ( source->specialization() == ROGUE_ASSASSINATION && source->spec.venomous_wounds->ok() )
  {
    target->callbacks_on_demise.emplace_back( std::bind( &rogue_t::trigger_venomous_wounds_death, source, std::placeholders::_1 ) );
  }

  if ( source->covenant.sepsis->ok() )
  {
    target->callbacks_on_demise.emplace_back( [ this, source ]( player_t* ) {
      if ( dots.sepsis->is_ticking() )
        source->cooldowns.sepsis->adjust( -timespan_t::from_seconds( source->covenant.sepsis->effectN( 3 ).base_value() ) );
    } );
  }

  if ( source->covenant.serrated_bone_spike->ok() )
  {
    target->callbacks_on_demise.emplace_back( [ this, source ]( player_t* ) {
      if ( dots.serrated_bone_spike->is_ticking() )
        source->cooldowns.serrated_bone_spike->reset( false, 1 );
    } );
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
    h *= 1.0 / ( 1.0 + spell.slice_and_dice -> effectN( 1 ).percent() * buffs.slice_and_dice -> value() );

  if ( buffs.adrenaline_rush -> check() )
    h *= 1.0 / ( 1.0 + buffs.adrenaline_rush -> value() );

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

  // TOCHECK: Currently no whitelist on beta
  crit += buffs.master_assassins_mark->stack_value();
  crit += buffs.master_assassins_mark_aura->stack_value();

  if ( conduit.planned_execution.ok() && buffs.symbols_of_death->up() )
  {
    crit += conduit.planned_execution.percent();
  }

  return crit;
}

// rogue_t::composite_spell_crit_chance =========================================

double rogue_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  crit += spell.critical_strikes->effectN( 1 ).percent();

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

// rogue_t::composite_leech ===============================================

double rogue_t::composite_leech() const
{
  double l = player_t::composite_leech();

  if ( buffs.grand_melee->check() )
  {
    l += buffs.grand_melee->data().effectN( 2 ).percent();
  }

  l += talent.leeching_poison->effectN( 1 ).percent();

  return l;
}

// rogue_t::matching_gear_multiplier ========================================

double rogue_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0.0;
}

// rogue_t::composite_player_multiplier =====================================

double rogue_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.deeper_daggers->up() && buffs.deeper_daggers->data().effectN( 1 ).has_common_school( school ) )
    m *= 1.0 + buffs.deeper_daggers->check_value();

  if ( legendary.guile_charm.ok() )
  {
    m *= 1.0 + buffs.guile_charm_insight_1->value();
    m *= 1.0 + buffs.guile_charm_insight_2->value();
    m *= 1.0 + buffs.guile_charm_insight_3->value();
  }

  if ( legendary.celerity.ok() && buffs.adrenaline_rush->check() )
  {
    m *= 1.0 + legendary.celerity->effectN( 1 ).percent();
  }

  return m;
}

// rogue_t::composite_player_target_multiplier ==============================

double rogue_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  rogue_td_t* tdata = get_target_data( target );

  m *= 1.0 + tdata->debuffs.ghostly_strike->stack_value();
  m *= 1.0 + tdata->debuffs.prey_on_the_weak->stack_value();

  return m;
}

// rogue_t::default_flask ===================================================

std::string rogue_t::default_flask() const
{
  return ( true_level >= 40 ) ? "greater_flask_of_the_currents" :
         ( true_level >= 35 ) ? "greater_draenic_agility_flask" :
         "disabled";
}

// rogue_t::default_potion ==================================================

std::string rogue_t::default_potion() const
{
  return ( true_level >= 40 ) ? "potion_of_unbridled_fury" :
         ( true_level >= 35 ) ? "draenic_agility" :
         "disabled";
}

// rogue_t::default_food ====================================================

std::string rogue_t::default_food() const
{
  return ( true_level >= 45 ) ? "famine_evaluator_and_snack_table" :
         ( true_level >= 40 ) ? "lavish_suramar_feast" :
         "disabled";
}

// rogue_t::default_rune ====================================================

std::string rogue_t::default_rune() const
{
  return ( true_level >= 50 ) ? "battle_scarred" :
         ( true_level >= 45 ) ? "defiled" :
         ( true_level >= 40 ) ? "hyper" :
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

  // Poisons
  precombat->add_action( "apply_poison" );

  // Flask
  precombat -> add_action( "flask" );

  // Rune
  precombat -> add_action( "augmentation" );

  // Food
  precombat -> add_action( "food" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Potion
  if ( specialization() != ROGUE_SUBTLETY )
    precombat -> add_action( "potion" );

  // Potion
  std::string potion_action = "potion,if=buff.bloodlust.react";
  if ( specialization() == ROGUE_ASSASSINATION )
    potion_action += "|debuff.vendetta.up";
  else if ( specialization() == ROGUE_OUTLAW )
    potion_action += "|buff.adrenaline_rush.up";
  else if ( specialization() == ROGUE_SUBTLETY )
    potion_action += "|buff.symbols_of_death.up&(buff.shadow_blades.up|cooldown.shadow_blades.remains<=10)";

  if ( specialization() == ROGUE_ASSASSINATION )
    precombat -> add_talent( this, "Marked for Death", "precombat_seconds=5,if=raid_event.adds.in>15" );
  if ( specialization() == ROGUE_OUTLAW )
    precombat -> add_talent( this, "Marked for Death", "precombat_seconds=5,if=raid_event.adds.in>40" );

  // Make restealth first action in the default list.
  def -> add_action( this, "Stealth", "", "Restealth if possible (no vulnerable enemies in combat)" );

  // Reaping Flames Generic Lines
  const std::string reaping_flames_comment = "Hold Reaping Flames for execute range or kill buffs, if possible. Always try to get the lowest cooldown based on available enemies.";
  const std::string reaping_flames_condition = "cycling_variable,name=reaping_delay,op=min,if=essence.breath_of_the_dying.major,value=target.time_to_die";
  const std::string reaping_flames_action = "reaping_flames,target_if=target.time_to_die<1.5|((target.health.pct>80|target.health.pct<=20)&(active_enemies=1|variable.reaping_delay>29))|(target.time_to_pct_20>30&(active_enemies=1|variable.reaping_delay>44))";

  if ( specialization() == ROGUE_ASSASSINATION )
  {
    // Pre-Combat
    precombat->add_action( this, "Stealth" );
    precombat->add_action( this, "Slice and Dice", "precombat_seconds=1" );
    precombat->add_action( "use_item,name=azsharas_font_of_power" );
    precombat->add_action( "guardian_of_azeroth,if=talent.exsanguinate.enabled" );

    // Main Rotation
    def -> add_action( "variable,name=energy_regen_combined,value=energy.regen+poisoned_bleeds*7%(2*spell_haste)" );
    def -> add_action( "variable,name=single_target,value=spell_targets.fan_of_knives<2" );
    def -> add_action( "call_action_list,name=stealthed,if=stealthed.rogue" );
    def -> add_action( "call_action_list,name=cds,if=(!talent.master_assassin.enabled|dot.garrote.ticking)" );
    def -> add_action( "call_action_list,name=dot" );
    def -> add_action( this, "Slice and Dice", "if=buff.slice_and_dice.remains<target.time_to_die&buff.slice_and_dice.remains<(1+combo_points)*1.8" );
    def -> add_action( "call_action_list,name=direct" );
    def -> add_action( "arcane_torrent,if=energy.deficit>=15+variable.energy_regen_combined" );
    def -> add_action( "arcane_pulse");
    def -> add_action( "lights_judgment");
    def -> add_action( "bag_of_tricks");

    // Cooldowns
    action_priority_list_t* cds = get_action_priority_list( "cds", "Cooldowns" );
    cds -> add_action( "use_item,name=azsharas_font_of_power,if=!stealthed.all&master_assassin_remains=0&(cooldown.vendetta.remains<?(cooldown.shiv.remains*equipped.ashvanes_razor_coral))<10+10*equipped.ashvanes_razor_coral&!debuff.vendetta.up&!debuff.shiv.up" );
    cds -> add_action( "call_action_list,name=essences,if=!stealthed.all&dot.rupture.ticking&master_assassin_remains=0" );
    cds -> add_talent( this, "Marked for Death", "target_if=min:target.time_to_die,if=raid_event.adds.up&(target.time_to_die<combo_points.deficit*1.5|combo_points.deficit>=cp_max_spend)", "If adds are up, snipe the one with lowest TTD. Use when dying faster than CP deficit or without any CP." );
    cds -> add_talent( this, "Marked for Death", "if=raid_event.adds.in>30-raid_event.adds.duration&combo_points.deficit>=cp_max_spend", "If no adds will die within the next 30s, use MfD on boss without any CP." );
    cds -> add_action( "variable,name=vendetta_subterfuge_condition,value=!talent.subterfuge.enabled|!azerite.shrouded_suffocation.enabled|dot.garrote.pmultiplier>1&(spell_targets.fan_of_knives<6|!cooldown.vanish.up)", "Vendetta logical conditionals based on current spec" );
    cds -> add_action( "variable,name=vendetta_nightstalker_condition,value=!talent.nightstalker.enabled|!talent.exsanguinate.enabled|cooldown.exsanguinate.remains<5-2*talent.deeper_stratagem.enabled" );
    cds -> add_action( "variable,name=variable,name=vendetta_font_condition,value=!equipped.azsharas_font_of_power|azerite.shrouded_suffocation.enabled|debuff.razor_coral_debuff.down|trinket.ashvanes_razor_coral.cooldown.remains<10&(cooldown.shiv.remains<1|debuff.shiv.up)" );
    cds -> add_action( this, "Vendetta", "if=!stealthed.rogue&dot.rupture.ticking&!debuff.vendetta.up&variable.vendetta_subterfuge_condition&variable.vendetta_nightstalker_condition&variable.vendetta_font_condition" );
    cds -> add_action( this, "Vanish", "if=talent.exsanguinate.enabled&talent.nightstalker.enabled&combo_points>=cp_max_spend&cooldown.exsanguinate.remains<1", "Vanish with Exsg + Nightstalker: Maximum CP and Exsg ready for next GCD" );
    cds -> add_action( this, "Vanish", "if=talent.nightstalker.enabled&!talent.exsanguinate.enabled&combo_points>=cp_max_spend&(debuff.vendetta.up|essence.vision_of_perfection.enabled)", "Vanish with Nightstalker + No Exsg: Maximum CP and Vendetta up (unless using VoP)" );
    cds -> add_action( "variable,name=ss_vanish_condition,value=azerite.shrouded_suffocation.enabled&(non_ss_buffed_targets>=1|spell_targets.fan_of_knives=3)&(ss_buffed_targets_above_pandemic=0|spell_targets.fan_of_knives>=6)", "See full comment on https://github.com/Ravenholdt-TC/Rogue/wiki/Assassination-APL-Research." );
    cds -> add_action( "pool_resource,for_next=1,extra_amount=45" );
    cds -> add_action( this, "Vanish", "if=talent.subterfuge.enabled&!stealthed.rogue&cooldown.garrote.up&(variable.ss_vanish_condition|!azerite.shrouded_suffocation.enabled&(dot.garrote.refreshable|debuff.vendetta.up&dot.garrote.pmultiplier<=1))&combo_points.deficit>=((1+2*azerite.shrouded_suffocation.enabled)*spell_targets.fan_of_knives)>?4&raid_event.adds.in>12" );
    cds -> add_action( this, "Vanish", "if=(talent.master_assassin.enabled|runeforge.mark_of_the_master_assassin.equipped)&!stealthed.all&master_assassin_remains<=0&!dot.rupture.refreshable&dot.garrote.remains>3&(debuff.vendetta.up&debuff.shiv.up&(!essence.blood_of_the_enemy.major|debuff.blood_of_the_enemy.up)|essence.vision_of_perfection.enabled)", "Vanish with Master Assasin: No stealth and no active MA buff, Rupture not in refresh range, during Vendetta+TB+BotE (unless using VoP)" );
    cds -> add_action( "shadowmeld,if=!stealthed.all&azerite.shrouded_suffocation.enabled&dot.garrote.refreshable&dot.garrote.pmultiplier<=1&combo_points.deficit>=1", "Shadowmeld for Shrouded Suffocation" );
    cds -> add_talent( this, "Exsanguinate", "if=!stealthed.rogue&(!dot.garrote.refreshable&dot.rupture.remains>4+4*cp_max_spend|dot.rupture.remains*0.5>target.time_to_die)&target.time_to_die>4", "Exsanguinate when not stealthed and both Rupture and Garrote are up for long enough." );
    cds -> add_action( this, "Shiv", "if=dot.rupture.ticking&(!equipped.azsharas_font_of_power|cooldown.vendetta.remains>10)" );

    // Non-spec stuff with lower prio
    cds -> add_action( potion_action );
    cds -> add_action( "blood_fury,if=debuff.vendetta.up" );
    cds -> add_action( "berserking,if=debuff.vendetta.up" );
    cds -> add_action( "fireblood,if=debuff.vendetta.up" );
    cds -> add_action( "ancestral_call,if=debuff.vendetta.up" );

    cds -> add_action( "use_item,name=galecallers_boon,if=(debuff.vendetta.up|(!talent.exsanguinate.enabled&cooldown.vendetta.remains>45|talent.exsanguinate.enabled&(cooldown.exsanguinate.remains<6|cooldown.exsanguinate.remains>20&fight_remains>65)))&!exsanguinated.rupture" );
    cds -> add_action( "use_item,name=ashvanes_razor_coral,if=debuff.razor_coral_debuff.down|target.time_to_die<20" );
    cds -> add_action( "use_item,name=ashvanes_razor_coral,if=(!talent.exsanguinate.enabled|!talent.subterfuge.enabled)&debuff.vendetta.remains>10-4*equipped.azsharas_font_of_power" );
    cds -> add_action( "use_item,name=ashvanes_razor_coral,if=(talent.exsanguinate.enabled&talent.subterfuge.enabled)&debuff.vendetta.up&(exsanguinated.garrote|azerite.shrouded_suffocation.enabled&dot.garrote.pmultiplier>1)" );
    cds -> add_action( "use_item,effect_name=cyclotronic_blast,if=master_assassin_remains=0&!debuff.vendetta.up&!debuff.shiv.up&buff.memory_of_lucid_dreams.down&energy<80&dot.rupture.remains>4" );
    cds -> add_action( "use_item,name=lurkers_insidious_gift,if=debuff.vendetta.up" );
    cds -> add_action( "use_item,name=lustrous_golden_plumage,if=debuff.vendetta.up" );
    cds -> add_action( "use_item,effect_name=gladiators_medallion,if=debuff.vendetta.up" );
    cds -> add_action( "use_item,effect_name=gladiators_badge,if=debuff.vendetta.up" );
    cds -> add_action( "use_items", "Default fallback for usable items: Use on cooldown." );

    // Azerite Essences
    action_priority_list_t* essences = get_action_priority_list( "essences", "Essences" );
    essences->add_action( "concentrated_flame,if=energy.time_to_max>1&!debuff.vendetta.up&(!dot.concentrated_flame_burn.ticking&!action.concentrated_flame.in_flight|full_recharge_time<gcd.max)" );
    essences->add_action( "blood_of_the_enemy,if=debuff.vendetta.up&(exsanguinated.garrote|debuff.shiv.up&combo_points.deficit<=1|debuff.vendetta.remains<=10)|target.time_to_die<=10", "Always use Blood with Vendetta up. Hold for Exsanguinate. Use with TB up before a finisher as long as it runs for 10s during Vendetta." );
    essences->add_action( "guardian_of_azeroth,if=cooldown.vendetta.remains<3|debuff.vendetta.up|target.time_to_die<30", "Attempt to align Guardian with Vendetta as long as it won't result in losing a full-value cast over the remaining duration of the fight" );
    essences->add_action( "guardian_of_azeroth,if=floor((target.time_to_die-30)%cooldown)>floor((target.time_to_die-30-cooldown.vendetta.remains)%cooldown)" );
    essences->add_action( "focused_azerite_beam,if=spell_targets.fan_of_knives>=2|raid_event.adds.in>60&energy<70" );
    essences->add_action( "purifying_blast,if=spell_targets.fan_of_knives>=2|raid_event.adds.in>60" );
    essences->add_action( "the_unbound_force,if=buff.reckless_force.up|buff.reckless_force_counter.stack<10" );
    essences->add_action( "ripple_in_space" );
    essences->add_action( "worldvein_resonance" );
    essences->add_action( "memory_of_lucid_dreams,if=energy<50&!cooldown.vendetta.up" );
    essences->add_action( reaping_flames_condition, reaping_flames_comment );
    essences->add_action( reaping_flames_action );

    // Stealth
    action_priority_list_t* stealthed = get_action_priority_list( "stealthed", "Stealthed Actions" );
    stealthed -> add_action( this, "Rupture", "if=talent.nightstalker.enabled&combo_points>=4&target.time_to_die-remains>6", "Nighstalker on 1T: Snapshot Rupture" );
    stealthed -> add_action( "pool_resource,for_next=1", "Subterfuge + Shrouded Suffocation: Ensure we use one global to apply Garrote to the main target if it is not snapshot yet, so all other main target abilities profit." );
    stealthed -> add_action( this, "Garrote", "if=azerite.shrouded_suffocation.enabled&buff.subterfuge.up&buff.subterfuge.remains<1.3&!ss_buffed" );
    stealthed -> add_action( "pool_resource,for_next=1", "Subterfuge: Apply or Refresh with buffed Garrotes" );
    stealthed -> add_action( this, "Garrote", "target_if=min:remains,if=talent.subterfuge.enabled&(remains<12|pmultiplier<=1)&target.time_to_die-remains>2" );
    stealthed -> add_action( this, "Rupture", "if=talent.subterfuge.enabled&azerite.shrouded_suffocation.enabled&!dot.rupture.ticking&variable.single_target", "Subterfuge + Shrouded Suffocation in ST: Apply early Rupture that will be refreshed for pandemic" );
    stealthed -> add_action( "pool_resource,for_next=1", "Subterfuge w/ Shrouded Suffocation: Reapply for bonus CP and/or extended snapshot duration." );
    stealthed -> add_action( this, "Garrote", "target_if=min:remains,if=talent.subterfuge.enabled&azerite.shrouded_suffocation.enabled&(active_enemies>1|!talent.exsanguinate.enabled)&target.time_to_die>remains&(remains<18|!ss_buffed)" );
    stealthed -> add_action( "pool_resource,for_next=1", "Subterfuge + Exsg on 1T: Refresh Garrote at the end of stealth to get max duration before Exsanguinate" );
    stealthed -> add_action( this, "Garrote", "if=talent.subterfuge.enabled&talent.exsanguinate.enabled&active_enemies=1&buff.subterfuge.remains<1.3" );

    // Damage over time abilities
    action_priority_list_t* dot = get_action_priority_list( "dot", "Damage over time abilities" );
    dot -> add_action( "variable,name=skip_cycle_garrote,value=priority_rotation&spell_targets.fan_of_knives>3&(dot.garrote.remains<cooldown.garrote.duration|poisoned_bleeds>5)", "Limit Garrotes on non-primrary targets for the priority rotation if 5+ bleeds are already up" );
    dot -> add_action( "variable,name=skip_cycle_rupture,value=priority_rotation&spell_targets.fan_of_knives>3&(debuff.shiv.up|(poisoned_bleeds>5&!azerite.scent_of_blood.enabled))", "Limit Ruptures on non-primrary targets for the priority rotation if 5+ bleeds are already up" );
    dot -> add_action( "variable,name=skip_rupture,value=debuff.vendetta.up&(debuff.shiv.up|master_assassin_remains>0)&dot.rupture.remains>2", "Limit Ruptures if Vendetta+Shiv/Master Assassin is up and we have 2+ seconds left on the Rupture DoT" );
    dot -> add_action( this, "Garrote", "if=talent.exsanguinate.enabled&!exsanguinated.garrote&dot.garrote.pmultiplier<=1&cooldown.exsanguinate.remains<2&spell_targets.fan_of_knives=1&raid_event.adds.in>6&dot.garrote.remains*0.5<target.time_to_die", "Special Garrote and Rupture setup prior to Exsanguinate cast" );
    dot -> add_action( this, "Rupture", "if=talent.exsanguinate.enabled&(combo_points>=cp_max_spend&cooldown.exsanguinate.remains<1&dot.rupture.remains*0.5<target.time_to_die)" );
    dot -> add_action( "pool_resource,for_next=1", "Garrote upkeep, also tries to use it as a special generator for the last CP before a finisher" );
    dot -> add_action( this, "Garrote", "if=refreshable&combo_points.deficit>=1+3*(azerite.shrouded_suffocation.enabled&cooldown.vanish.up)&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3+azerite.shrouded_suffocation.enabled)&(!exsanguinated|remains<=tick_time*2&spell_targets.fan_of_knives>=3+azerite.shrouded_suffocation.enabled)&!ss_buffed&(target.time_to_die-remains)>4&(master_assassin_remains=0|!ticking&azerite.shrouded_suffocation.enabled)" );
    dot -> add_action( "pool_resource,for_next=1" );
    dot -> add_action( this, "Garrote", "cycle_targets=1,if=!variable.skip_cycle_garrote&target!=self.target&refreshable&combo_points.deficit>=1+3*(azerite.shrouded_suffocation.enabled&cooldown.vanish.up)&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3+azerite.shrouded_suffocation.enabled)&(!exsanguinated|remains<=tick_time*2&spell_targets.fan_of_knives>=3+azerite.shrouded_suffocation.enabled)&!ss_buffed&(target.time_to_die-remains)>12&(master_assassin_remains=0|!ticking&azerite.shrouded_suffocation.enabled)" );
    dot -> add_talent( this, "Crimson Tempest", "if=spell_targets>=2&remains<2+(spell_targets>=5)&combo_points>=4", "Crimson Tempest on multiple targets at 4+ CP when running out in 2s (up to 4 targets) or 3s (5+ targets)" );
    dot -> add_action( this, "Rupture", "if=!variable.skip_rupture&(combo_points>=4&refreshable|!ticking&(time>10|combo_points>=2))&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3+azerite.shrouded_suffocation.enabled)&(!exsanguinated|remains<=tick_time*2&spell_targets.fan_of_knives>=3+azerite.shrouded_suffocation.enabled)&target.time_to_die-remains>4", "Keep up Rupture at 4+ on all targets (when living long enough and not snapshot)" );
    dot -> add_action( this, "Rupture", "cycle_targets=1,if=!variable.skip_cycle_rupture&!variable.skip_rupture&target!=self.target&combo_points>=4&refreshable&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3+azerite.shrouded_suffocation.enabled)&(!exsanguinated|remains<=tick_time*2&spell_targets.fan_of_knives>=3+azerite.shrouded_suffocation.enabled)&target.time_to_die-remains>4" );
    dot -> add_talent( this, "Crimson Tempest", "if=spell_targets=1&combo_points>=(cp_max_spend-1)&refreshable&!exsanguinated&!debuff.shiv.up&master_assassin_remains=0&!azerite.twist_the_knife.enabled&target.time_to_die-remains>4", "Crimson Tempest on ST if in pandemic and it will do less damage than Envenom due to TB/MA/TtK" );
    dot -> add_action( "sepsis" );

    // Direct damage abilities
    action_priority_list_t* direct = get_action_priority_list( "direct", "Direct damage abilities" );
    direct -> add_action( this, "Envenom", "if=(combo_points>=4+talent.deeper_stratagem.enabled|combo_points=animacharged_cp)&(debuff.vendetta.up|debuff.shiv.up|energy.deficit<=25+variable.energy_regen_combined|!variable.single_target)&(!talent.exsanguinate.enabled|cooldown.exsanguinate.remains>2)", "Envenom at 4+ (5+ with DS) CP. Immediately on 2+ targets, with Vendetta, or with TB; otherwise wait for some energy. Also wait if Exsg combo is coming up." );
    direct -> add_action( "variable,name=use_filler,value=combo_points.deficit>1|energy.deficit<=25+variable.energy_regen_combined|!variable.single_target" );
    direct -> add_action( "serrated_bone_spike,target_if=min:dot.serrated_bone_spike_dot.ticking,if=!dot.serrated_bone_spike.ticking|variable.use_filler&(active_enemies=1&raid_event.adds.in>full_recharge_time|charges>2&target.time_to_die<5)" );
    direct -> add_action( this, "Fan of Knives", "if=variable.use_filler&azerite.echoing_blades.enabled&spell_targets.fan_of_knives>=2+(debuff.vendetta.up*(1+(azerite.echoing_blades.rank=1)))", "With Echoing Blades, Fan of Knives at 2+ targets, or 3-4+ targets when Vendetta is up" );
    direct -> add_action( this, "Fan of Knives", "if=variable.use_filler&(buff.hidden_blades.stack>=19|(!priority_rotation&spell_targets.fan_of_knives>=4+(azerite.double_dose.rank>2)+stealthed.rogue))", "Fan of Knives at 19+ stacks of Hidden Blades or against 4+ (5+ with Double Dose) targets." );
    direct -> add_action( this, "Fan of Knives", "target_if=!dot.deadly_poison_dot.ticking,if=variable.use_filler&spell_targets.fan_of_knives>=3", "Fan of Knives to apply Deadly Poison if inactive on any target at 3 targets." );
    direct -> add_action( "echoing_reprimand,if=variable.use_filler" );
    direct -> add_action( "slaughter,if=variable.use_filler" );
    direct -> add_action( this, "Ambush", "if=variable.use_filler" );
    direct -> add_action( this, "Mutilate", "target_if=!dot.deadly_poison_dot.ticking,if=variable.use_filler&spell_targets.fan_of_knives=2", "Tab-Mutilate to apply Deadly Poison at 2 targets" );
    direct -> add_action( this, "Mutilate", "if=variable.use_filler" );
  }
  else if ( specialization() == ROGUE_OUTLAW )
  {
    // Pre-Combat
    precombat -> add_action( this, "Stealth", "if=(!equipped.pocketsized_computation_device|!cooldown.cyclotronic_blast.duration|raid_event.invulnerable.exists)" );
    precombat -> add_action( this, "Roll the Bones", "precombat_seconds=1" );
    precombat -> add_action( this, "Slice and Dice", "precombat_seconds=1" );
    precombat -> add_action( "use_item,name=azsharas_font_of_power" );
    precombat -> add_action( "use_item,effect_name=cyclotronic_blast,if=!raid_event.invulnerable.exists" );

    // Main Rotation
    def -> add_action( "variable,name=rtb_reroll,value=0", "Currently not worth rerolling in SL in any known situation" );
    def -> add_action( "variable,name=ambush_condition,value=combo_points.deficit>=2+2*(talent.ghostly_strike.enabled&cooldown.ghostly_strike.remains<1)+buff.broadside.up&energy>60&!buff.skull_and_crossbones.up&!buff.keep_your_wits_about_you.up" );
    def -> add_action( "variable,name=blade_flurry_sync,value=spell_targets.blade_flurry<2&raid_event.adds.in>20|buff.blade_flurry.up", "With multiple targets, this variable is checked to decide whether some CDs should be synced with Blade Flurry" );
    def -> add_action( "call_action_list,name=stealth,if=stealthed.all" );
    def -> add_action( "call_action_list,name=cds" );
    def -> add_action( "run_action_list,name=finish,if=combo_points>=cp_max_spend-(buff.broadside.up+buff.opportunity.up)*(talent.quick_draw.enabled&(!talent.marked_for_death.enabled|cooldown.marked_for_death.remains>1))*(azerite.ace_up_your_sleeve.rank<2|!cooldown.between_the_eyes.up)|combo_points=animacharged_cp", "Finish at maximum CP. Substract one for each Broadside and Opportunity when Quick Draw is selected and MfD is not ready after the next second. Always max BtE with 2+ Ace." );
    def -> add_action( "call_action_list,name=build" );
    def -> add_action( "arcane_torrent,if=energy.deficit>=15+energy.regen" );
    def -> add_action( "arcane_pulse" );
    def -> add_action( "lights_judgment");
    def -> add_action( "bag_of_tricks");

    // Cooldowns
    action_priority_list_t* cds = get_action_priority_list( "cds", "Cooldowns" );
    cds -> add_action( "call_action_list,name=essences,if=!stealthed.all" );
    cds -> add_action( this, "Adrenaline Rush", "if=!buff.adrenaline_rush.up&(!equipped.azsharas_font_of_power|cooldown.latent_arcana.remains>20)" );
    cds -> add_action( this, "Roll the Bones", "if=buff.roll_the_bones.remains<=3|variable.rtb_reroll" );
    cds -> add_talent( this, "Marked for Death", "target_if=min:target.time_to_die,if=raid_event.adds.up&(target.time_to_die<combo_points.deficit|!stealthed.rogue&combo_points.deficit>=cp_max_spend-1)", "If adds are up, snipe the one with lowest TTD. Use when dying faster than CP deficit or without any CP." );
    cds -> add_talent( this, "Marked for Death", "if=raid_event.adds.in>30-raid_event.adds.duration&!stealthed.rogue&combo_points.deficit>=cp_max_spend-1", "If no adds will die within the next 30s, use MfD on boss without any CP." );
    cds -> add_action( this, "Blade Flurry", "if=spell_targets>=2&!buff.blade_flurry.up&(!raid_event.adds.exists|raid_event.adds.remains>8|raid_event.adds.in>(2-cooldown.blade_flurry.charges_fractional)*25)", "Blade Flurry on 2+ enemies. With adds: Use if they stay for 8+ seconds or if your next charge will be ready in time for the next wave." );
    cds -> add_action( this, "Blade Flurry", "if=spell_targets=1&raid_event.adds.in>(2-cooldown.blade_flurry.charges_fractional)*25", "Blade Flurry on 1T if your next charge will be ready in time for the next wave." );
    cds -> add_talent( this, "Ghostly Strike", "if=combo_points.deficit>=1+buff.broadside.up" );
    cds -> add_talent( this, "Killing Spree", "if=variable.blade_flurry_sync&spell_targets.blade_flurry>1&energy.time_to_max>2" );
    cds -> add_talent( this, "Blade Rush", "if=variable.blade_flurry_sync&energy.time_to_max>2" );
    cds -> add_action( this, "Vanish", "if=!stealthed.all&variable.ambush_condition", "Using Vanish/Ambush is only a very tiny increase, so in reality, you're absolutely fine to use it as a utility spell." );
    cds -> add_talent( this, "Dreadblades", "if=!stealthed.all&combo_points<=1" );
    cds -> add_action( "shadowmeld,if=!stealthed.all&variable.ambush_condition" );
    cds -> add_action( "sepsis,if=!stealthed.all" );

    // Non-spec stuff with lower prio
    cds -> add_action( potion_action );
    cds -> add_action( "blood_fury" );
    cds -> add_action( "berserking" );
    cds -> add_action( "fireblood" );
    cds -> add_action( "ancestral_call" );

    cds -> add_action( "use_item,effect_name=cyclotronic_blast,if=!stealthed.all&buff.adrenaline_rush.down&buff.memory_of_lucid_dreams.down&energy.time_to_max>4&rtb_buffs<5" );
    cds -> add_action( "use_item,name=azsharas_font_of_power,if=!buff.adrenaline_rush.up&!buff.blade_flurry.up&cooldown.adrenaline_rush.remains<15" );
    cds -> add_action( "use_item,name=ashvanes_razor_coral,if=debuff.razor_coral_debuff.down|debuff.conductive_ink_debuff.up&target.health.pct<32&target.health.pct>=30|!debuff.conductive_ink_debuff.up&(debuff.razor_coral_debuff.stack>=20-10*debuff.blood_of_the_enemy.up|target.time_to_die<60)&buff.adrenaline_rush.remains>18", "Very roughly rule of thumbified maths below: Use for Inkpod crit, otherwise with AR at 20+ stacks or 10+ with also Blood up." );
    cds -> add_action( "use_items,if=buff.bloodlust.react|fight_remains<=20|combo_points.deficit<=2", "Default fallback for usable items." );

    // Azerite Essences
    action_priority_list_t* essences = get_action_priority_list( "essences", "Essences" );
    essences->add_action( "concentrated_flame,if=energy.time_to_max>1&!buff.blade_flurry.up&(!dot.concentrated_flame_burn.ticking&!action.concentrated_flame.in_flight|full_recharge_time<gcd.max)" );
    essences->add_action( "blood_of_the_enemy,if=variable.blade_flurry_sync&cooldown.between_the_eyes.up&(spell_targets.blade_flurry>=2|raid_event.adds.in>45)" );
    essences->add_action( "guardian_of_azeroth" );
    essences->add_action( "focused_azerite_beam,if=spell_targets.blade_flurry>=2|raid_event.adds.in>60&!buff.adrenaline_rush.up" );
    essences->add_action( "purifying_blast,if=spell_targets.blade_flurry>=2|raid_event.adds.in>60" );
    essences->add_action( "the_unbound_force,if=buff.reckless_force.up|buff.reckless_force_counter.stack<10" );
    essences->add_action( "ripple_in_space" );
    essences->add_action( "worldvein_resonance" );
    essences->add_action( "memory_of_lucid_dreams,if=energy<45" );
    essences->add_action( reaping_flames_condition, reaping_flames_comment );
    essences->add_action( reaping_flames_action );

    // Stealth
    action_priority_list_t* stealth = get_action_priority_list( "stealth", "Stealth" );
    stealth -> add_action( this, "Cheap Shot", "target_if=min:debuff.prey_on_the_weak.remains,if=talent.prey_on_the_weak.enabled&!target.is_boss" );
    stealth -> add_action( "slaughter" );
    stealth -> add_action( this, "Ambush" );

    // Finishers
    action_priority_list_t* finish = get_action_priority_list( "finish", "Finishers" );
    finish -> add_action( this, "Between the Eyes", "", "BtE on cooldown to keep the Crit debuff up" );
    finish -> add_action( this, "Slice and Dice", "if=buff.slice_and_dice.remains<fight_remains&buff.slice_and_dice.remains<(1+combo_points)*1.8" );
    finish -> add_action( this, "Dispatch" );

    // Builders
    action_priority_list_t* build = get_action_priority_list( "build", "Builders" );
    build -> add_action( "echoing_reprimand" );
    build -> add_action( "serrated_bone_spike,target_if=min:dot.serrated_bone_spike_dot.ticking,if=!dot.serrated_bone_spike.ticking|active_enemies=1&raid_event.adds.in>full_recharge_time|charges>2&target.time_to_die<5" );
    build -> add_action( this, "Pistol Shot", "if=(talent.quick_draw.enabled|azerite.keep_your_wits_about_you.rank<2)&buff.opportunity.up&(buff.keep_your_wits_about_you.stack<14|energy<45)", "Use Pistol Shot if it won't cap combo points and the Opportunity buff is up. Avoid using when Keep Your Wits stacks are high or when using Weaponmaster, unless the Deadshot buff is up." );
    build -> add_action( this, "Pistol Shot", "if=buff.opportunity.up&(buff.deadshot.up|buff.greenskins_wickers.up|buff.concealed_blunderbuss.up)" );
    build -> add_action( this, "Sinister Strike" );
  }
  else if ( specialization() == ROGUE_SUBTLETY )
  {
    // Pre-Combat
    precombat -> add_action( this, "Stealth" );
    precombat -> add_talent( this, "Marked for Death", "precombat_seconds=15" );
    precombat -> add_action( this, "Slice and Dice", "precombat_seconds=1" );
    precombat -> add_action( "potion" );
    precombat -> add_action( "use_item,name=azsharas_font_of_power" );

    // Main Rotation
    def -> add_action( "call_action_list,name=cds", "Check CDs at first" );
    def -> add_action( "run_action_list,name=stealthed,if=stealthed.all", "Run fully switches to the Stealthed Rotation (by doing so, it forces pooling if nothing is available)." );
    def -> add_action( this, "Slice and Dice", "if=target.time_to_die>6&buff.slice_and_dice.remains<gcd.max&combo_points>=4-(time<10)*2", "Apply Slice and Dice at 2+ CP during the first 10 seconds, after that 4+ CP if it expires within the next GCD or is not up" );
    def -> add_action( "variable,name=use_priority_rotation,value=priority_rotation&spell_targets.shuriken_storm>=2", "Only change rotation if we have priority_rotation set and multiple targets up." );
    def -> add_action( "call_action_list,name=stealth_cds,if=variable.use_priority_rotation", "Priority Rotation? Let's give a crap about energy for the stealth CDs (builder still respect it). Yup, it can be that simple." );
    def -> add_action( "variable,name=stealth_threshold,value=25+talent.vigor.enabled*20+talent.master_of_shadows.enabled*20+talent.shadow_focus.enabled*25+talent.alacrity.enabled*20+25*(spell_targets.shuriken_storm>=4)", "Used to define when to use stealth CDs or builders" );
    def -> add_action( "variable,name=stealth_threshold,op=set,if=conduit.slaughter_scars.enabled&conduit.slaughter_scars.rank>=1+talent.weaponmaster.enabled,value=20+talent.vigor.enabled*5+talent.master_of_shadows.enabled*5+talent.shadow_focus.enabled*10+talent.alacrity.enabled*20+30*(spell_targets.shuriken_storm>=3)", "Redefine for Slaughter energy" );
    def -> add_action( "call_action_list,name=stealth_cds,if=energy.deficit<=variable.stealth_threshold", "Consider using a Stealth CD when reaching the energy threshold" );
    //def -> add_action( this, "Rupture", "if=azerite.nights_vengeance.enabled&!buff.nights_vengeance.up&combo_points.deficit>1&(spell_targets.shuriken_storm<2|variable.use_priority_rotation)&(cooldown.symbols_of_death.remains<=3|(azerite.nights_vengeance.rank>=2&buff.symbols_of_death.remains>3&!stealthed.all&cooldown.shadow_dance.charges_fractional>=0.9))", "Night's Vengeance: Rupture before Symbols at low CP to combine early refresh with getting the buff up. Also low CP during Symbols between Dances with 2+ NV." );
    def -> add_action( "call_action_list,name=finish,if=runeforge.deathly_shadows.equipped&dot.sepsis.ticking&dot.sepsis.remains<=2&combo_points>=2" );
    def -> add_action( "call_action_list,name=finish,if=combo_points=animacharged_cp" );
    def -> add_action( "call_action_list,name=finish,if=combo_points.deficit<=1|target.time_to_die<=1&combo_points>=3", "Finish at 4+ without DS, 5+ with DS (outside stealth)" );
    def -> add_action( "call_action_list,name=finish,if=spell_targets.shuriken_storm=4&combo_points>=4", "With DS also finish at 4+ against exactly 4 targets (outside stealth)" );
    def -> add_action( "call_action_list,name=build,if=energy.deficit<=variable.stealth_threshold", "Use a builder when reaching the energy threshold" );
    def -> add_action( "arcane_torrent,if=energy.deficit>=15+energy.regen", "Lowest priority in all of the APL because it causes a GCD" );
    def -> add_action( "arcane_pulse" );
    def -> add_action( "lights_judgment");
    def -> add_action( "bag_of_tricks");

    // Cooldowns
    action_priority_list_t* cds = get_action_priority_list( "cds", "Cooldowns" );
    cds -> add_action( this, "Shadow Dance", "use_off_gcd=1,if=!buff.shadow_dance.up&buff.shuriken_tornado.up&buff.shuriken_tornado.remains<=3.5", "Use Dance off-gcd before the first Shuriken Storm from Tornado comes in." );
    cds -> add_action( this, "Symbols of Death", "use_off_gcd=1,if=buff.shuriken_tornado.up&buff.shuriken_tornado.remains<=3.5", "(Unless already up because we took Shadow Focus) use Symbols off-gcd before the first Shuriken Storm from Tornado comes in." );
    cds -> add_action( this, "Vanish", "if=(runeforge.mark_of_the_master_assassin.equipped|runeforge.deathly_shadows.equipped)&buff.symbols_of_death.up&buff.shadow_dance.up&master_assassin_remains=0&buff.deathly_shadows.down&(combo_points<1|!runeforge.deathly_shadows.equipped)" );
    cds -> add_action( "call_action_list,name=essences,if=!stealthed.all&buff.slice_and_dice.up|essence.breath_of_the_dying.major&time>=2" );
    cds -> add_action( "pool_resource,for_next=1,if=!talent.shadow_focus.enabled", "Pool for Tornado pre-SoD with ShD ready when not running SF." );
    cds -> add_talent( this, "Shuriken Tornado", "if=energy>=60&buff.slice_and_dice.up&cooldown.symbols_of_death.up&cooldown.shadow_dance.charges>=1", "Use Tornado pre SoD when we have the energy whether from pooling without SF or just generally." );
    cds -> add_action( "serrated_bone_spike,cycle_targets=1,if=buff.slice_and_dice.up&!dot.serrated_bone_spike_dot.ticking|fight_remains<=5" );
    cds -> add_action( this, "Symbols of Death", "if=buff.slice_and_dice.up&!cooldown.shadow_blades.up&(talent.enveloping_shadows.enabled|cooldown.shadow_dance.charges>=1)&(!talent.shuriken_tornado.enabled|talent.shadow_focus.enabled|cooldown.shuriken_tornado.remains>2)&(!essence.blood_of_the_enemy.major|cooldown.blood_of_the_enemy.remains>2)", "Use Symbols on cooldown (after first SnD) unless we are going to pop Tornado and do not have Shadow Focus." );
    cds -> add_talent( this, "Marked for Death", "target_if=min:target.time_to_die,if=raid_event.adds.up&(target.time_to_die<combo_points.deficit|!stealthed.all&combo_points.deficit>=cp_max_spend)", "If adds are up, snipe the one with lowest TTD. Use when dying faster than CP deficit or not stealthed without any CP." );
    cds -> add_talent( this, "Marked for Death", "if=raid_event.adds.in>30-raid_event.adds.duration&!stealthed.all&combo_points.deficit>=cp_max_spend", "If no adds will die within the next 30s, use MfD on boss without any CP and no stealth." );
    cds -> add_action( this, "Shadow Blades", "if=!stealthed.all&buff.slice_and_dice.up&combo_points.deficit>=2" );
    cds -> add_talent( this, "Shuriken Tornado", "if=talent.shadow_focus.enabled&buff.slice_and_dice.up&buff.symbols_of_death.up", "With SF, if not already done, use Tornado with SoD up." );
    cds -> add_action( this, "Shadow Dance", "if=!buff.shadow_dance.up&target.time_to_die<=5+talent.subterfuge.enabled&!raid_event.adds.up" );

    // Non-spec stuff with lower prio
    cds -> add_action( potion_action );
    cds -> add_action( "blood_fury,if=buff.symbols_of_death.up" );
    cds -> add_action( "berserking,if=buff.symbols_of_death.up" );
    cds -> add_action( "fireblood,if=buff.symbols_of_death.up" );
    cds -> add_action( "ancestral_call,if=buff.symbols_of_death.up" );

    cds -> add_action( "use_item,effect_name=cyclotronic_blast,if=!stealthed.all&buff.slice_and_dice.up&!buff.symbols_of_death.up&energy.deficit>=30" );
    cds -> add_action( "use_item,name=azsharas_font_of_power,if=!buff.shadow_dance.up&cooldown.symbols_of_death.remains<10" );
    cds -> add_action( "use_item,name=ashvanes_razor_coral,if=debuff.razor_coral_debuff.down|debuff.conductive_ink_debuff.up&target.health.pct<32&target.health.pct>=30|!debuff.conductive_ink_debuff.up&(debuff.razor_coral_debuff.stack>=25-10*debuff.blood_of_the_enemy.up|target.time_to_die<40)&buff.symbols_of_death.remains>8", "Very roughly rule of thumbified maths below: Use for Inkpod crit, otherwise with SoD at 25+ stacks or 15+ with also Blood up." );
    cds -> add_action( "use_item,name=mydas_talisman" );
    cds -> add_action( "use_items,if=buff.symbols_of_death.up|target.time_to_die<20", "Default fallback for usable items: Use with Symbols of Death." );

    // Azerite Essences
    action_priority_list_t* essences = get_action_priority_list( "essences", "Essences" );
    essences->add_action( "concentrated_flame,if=energy.time_to_max>1&!buff.symbols_of_death.up&(!dot.concentrated_flame_burn.ticking&!action.concentrated_flame.in_flight|full_recharge_time<gcd.max)" );
    essences->add_action( "blood_of_the_enemy,if=!cooldown.shadow_blades.up&cooldown.symbols_of_death.up|target.time_to_die<=10" );
    essences->add_action( "guardian_of_azeroth" );
    essences->add_action( "focused_azerite_beam,if=(spell_targets.shuriken_storm>=2|raid_event.adds.in>60)&!cooldown.symbols_of_death.up&!buff.symbols_of_death.up&energy.deficit>=30" );
    essences->add_action( "purifying_blast,if=spell_targets.shuriken_storm>=2|raid_event.adds.in>60" );
    essences->add_action( "the_unbound_force,if=buff.reckless_force.up|buff.reckless_force_counter.stack<10" );
    essences->add_action( "ripple_in_space" );
    essences->add_action( "worldvein_resonance,if=cooldown.symbols_of_death.remains<5|target.time_to_die<18" );
    essences->add_action( "memory_of_lucid_dreams,if=energy<40&buff.symbols_of_death.up" );
    essences->add_action( reaping_flames_condition, reaping_flames_comment );
    essences->add_action( reaping_flames_action );

    // Stealth Cooldowns
    action_priority_list_t* stealth_cds = get_action_priority_list( "stealth_cds", "Stealth Cooldowns" );
    stealth_cds -> add_action( "variable,name=shd_threshold,value=cooldown.shadow_dance.charges_fractional>=1.75", "Helper Variable" );
    stealth_cds -> add_action( this, "Vanish", "if=!variable.shd_threshold&combo_points.deficit>1", "Vanish unless we are about to cap on Dance charges." );
    stealth_cds -> add_action( "sepsis,if=!variable.shd_threshold" );
    stealth_cds -> add_action( "pool_resource,for_next=1,extra_amount=40", "Pool for Shadowmeld + Shadowstrike unless we are about to cap on Dance charges. Only when Find Weakness is about to run out." );
    stealth_cds -> add_action( "shadowmeld,if=energy>=40&energy.deficit>=10&!variable.shd_threshold&combo_points.deficit>1&debuff.find_weakness.remains<1" );
    stealth_cds -> add_action( "variable,name=shd_combo_points,value=combo_points.deficit>=4", "CP requirement: Dance at low CP by default." );
    stealth_cds -> add_action( "variable,name=shd_combo_points,value=combo_points.deficit<=1,if=variable.use_priority_rotation", "CP requirement: Dance only before finishers if we have priority rotation." );
    stealth_cds -> add_action( this, "Shadow Dance", "if=variable.shd_combo_points&(variable.shd_threshold|buff.symbols_of_death.remains>=1.2|spell_targets.shuriken_storm>=4&cooldown.symbols_of_death.remains>10)", "Dance during Symbols or above threshold." );
    stealth_cds -> add_action( this, "Shadow Dance", "if=variable.shd_combo_points&target.time_to_die<cooldown.symbols_of_death.remains&!raid_event.adds.up", "Burn remaining Dances before the target dies if SoD won't be ready in time." );

    // Stealthed Rotation
    action_priority_list_t* stealthed = get_action_priority_list( "stealthed", "Stealthed Rotation" );
    stealthed -> add_action( "slaughter,if=time<1", "TODO: Update when we have proper slaughter poison buff with expiry to make this work as a buff application" );
    stealthed -> add_action( this, "Shadowstrike", "if=(buff.stealth.up|buff.vanish.up)", "If Stealth/vanish are up, use Shadowstrike to benefit from the passive bonus and Find Weakness, even if we are at max CP (from the precombat MfD)." );
    stealthed -> add_action( "call_action_list,name=finish,if=buff.shuriken_tornado.up&combo_points.deficit<=2", "Finish at 3+ CP without DS / 4+ with DS with Shuriken Tornado buff up to avoid some CP waste situations." );
    stealthed -> add_action( "call_action_list,name=finish,if=spell_targets.shuriken_storm=4&combo_points>=4", "Also safe to finish at 4+ CP with exactly 4 targets. (Same as outside stealth.)" );
    stealthed -> add_action( "call_action_list,name=finish,if=combo_points.deficit<=1-(talent.deeper_stratagem.enabled&buff.vanish.up)", "Finish at 4+ CP without DS, 5+ with DS, and 6 with DS after Vanish" );
    stealthed -> add_action( this, "Shadowstrike", "cycle_targets=1,if=talent.secret_technique.enabled&debuff.find_weakness.remains<1&spell_targets.shuriken_storm=2&target.time_to_die-remains>6", "At 2 targets with Secret Technique keep up Find Weakness by cycling Shadowstrike.");
    stealthed -> add_action( this, "Shadowstrike", "if=!talent.deeper_stratagem.enabled&azerite.blade_in_the_shadows.rank=3&spell_targets.shuriken_storm=3", "Without Deeper Stratagem and 3 Ranks of Blade in the Shadows it is worth using Shadowstrike on 3 targets." );
    stealthed -> add_action( this, "Shadowstrike", "if=variable.use_priority_rotation&(debuff.find_weakness.remains<1|talent.weaponmaster.enabled&spell_targets.shuriken_storm<=4|azerite.inevitability.enabled&buff.symbols_of_death.up&spell_targets.shuriken_storm<=3+azerite.blade_in_the_shadows.enabled)", "For priority rotation, use Shadowstrike over Storm 1) with WM against up to 4 targets, 2) if FW is running off (on any amount of targets), or 3) to maximize SoD extension with Inevitability on 3 targets (4 with BitS)." );
    stealthed -> add_action( this, "Shuriken Storm", "if=spell_targets>=3+(conduit.slaughter_scars.rank>12)" );
    stealthed -> add_action( this, "Shadowstrike", "if=debuff.find_weakness.remains<=1|cooldown.symbols_of_death.remains<18&debuff.find_weakness.remains<cooldown.symbols_of_death.remains", "Shadowstrike to refresh Find Weakness and to ensure we can carry over a full FW into the next SoD if possible." );
    stealthed -> add_action( "pool_resource,for_next=1" );
    stealthed -> add_action( "slaughter,if=!runeforge.akaaris_soul_fragment.equipped&conduit.slaughter_scars.enabled&conduit.slaughter_scars.rank>=1+talent.weaponmaster.enabled" );
    stealthed -> add_talent( this, "Gloomblade", "if=!runeforge.akaaris_soul_fragment.equipped&(azerite.perforate.rank>=2|buff.perforated_veins.stack>=3&conduit.perforated_veins.rank>=(3-conduit.deeper_daggers.enabled*2))" );
    stealthed -> add_action( this, "Backstab", "if=!runeforge.akaaris_soul_fragment.equipped&buff.perforated_veins.stack>=3&conduit.perforated_veins.rank>=8-talent.weaponmaster.enabled*2" );
    stealthed -> add_action( this, "Shadowstrike" );

    // Finishers
    action_priority_list_t* finish = get_action_priority_list( "finish", "Finishers" );
    finish -> add_action( this, "Slice and Dice", "if=(!talent.premeditation.enabled|!buff.shadow_dance.up)&buff.slice_and_dice.remains<target.time_to_die&buff.slice_and_dice.remains<(1+combo_points)*1.8" );
    finish -> add_action( this, "Rupture", "if=target.time_to_die-remains>6&refreshable", "Keep up Rupture if it is about to run out." );
    finish -> add_action( this, "Rupture", "cycle_targets=1,if=!variable.use_priority_rotation&spell_targets.shuriken_storm>=2&target.time_to_die>=(5+(2*combo_points))&refreshable", "Multidotting targets that will live for the duration of Rupture, refresh during pandemic." );
    finish -> add_action( this, "Rupture", "if=remains<cooldown.symbols_of_death.remains+10&cooldown.symbols_of_death.remains<=5&target.time_to_die-remains>cooldown.symbols_of_death.remains+5", "Refresh Rupture early if it will expire during Symbols. Do that refresh if SoD gets ready in the next 5s." );
    finish -> add_talent( this, "Secret Technique" );
    finish -> add_action( this, "Shadow Vault", "if=!variable.use_priority_rotation&spell_targets>=3" );
    finish -> add_action( this, "Eviscerate" );

    // Builders
    action_priority_list_t* build = get_action_priority_list( "build", "Builders" );
    build -> add_action( this, "Shuriken Storm", "if=spell_targets>=2+(talent.gloomblade.enabled&azerite.perforate.rank>=2&position_back)" );
    build -> add_action( "serrated_bone_spike,if=cooldown.serrated_bone_spike.charges_fractional>=2.75" );
    build -> add_action( "echoing_reprimand" );
    build -> add_talent( this, "Gloomblade" );
    build -> add_action( this, "Backstab" );
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// rogue_t::create_action  ==================================================

action_t* rogue_t::create_action( util::string_view name, const std::string& options_str )
{
  using namespace actions;

  if ( name == "adrenaline_rush"     ) return new adrenaline_rush_t     ( name, this, options_str );
  if ( name == "ambush"              ) return new ambush_t              ( name, this, options_str );
  if ( name == "apply_poison"        ) return new apply_poison_t        ( this, options_str );
  if ( name == "auto_attack"         ) return new auto_melee_attack_t   ( this, options_str );
  if ( name == "backstab"            ) return new backstab_t            ( name, this, options_str );
  if ( name == "between_the_eyes"    ) return new between_the_eyes_t    ( name, this, options_str );
  if ( name == "blade_flurry"        ) return new blade_flurry_t        ( name, this, options_str );
  if ( name == "blade_rush"          ) return new blade_rush_t          ( name, this, options_str );
  if ( name == "cheap_shot"          ) return new cheap_shot_t          ( name, this, options_str );
  if ( name == "crimson_tempest"     ) return new crimson_tempest_t     ( name, this, options_str );
  if ( name == "detection"           ) return new detection_t           ( name, this, options_str );
  if ( name == "dispatch"            ) return new dispatch_t            ( name, this, options_str );
  if ( name == "dreadblades"         ) return new dreadblades_t         ( name, this, options_str );
  if ( name == "echoing_reprimand"   ) return new echoing_reprimand_t   ( name, this, options_str );
  if ( name == "envenom"             ) return new envenom_t             ( name, this, options_str );
  if ( name == "eviscerate"          ) return new eviscerate_t          ( name, this, options_str );
  if ( name == "exsanguinate"        ) return new exsanguinate_t        ( name, this, options_str );
  if ( name == "fan_of_knives"       ) return new fan_of_knives_t       ( name, this, options_str );
  if ( name == "feint"               ) return new feint_t               ( name, this, options_str );
  if ( name == "garrote"             ) return new garrote_t             ( name, this, options_str );
  if ( name == "gouge"               ) return new gouge_t               ( name, this, options_str );
  if ( name == "ghostly_strike"      ) return new ghostly_strike_t      ( name, this, options_str );
  if ( name == "gloomblade"          ) return new gloomblade_t          ( name, this, options_str );
  if ( name == "kick"                ) return new kick_t                ( name, this, options_str );
  if ( name == "kidney_shot"         ) return new kidney_shot_t         ( name, this, options_str );
  if ( name == "killing_spree"       ) return new killing_spree_t       ( name, this, options_str );
  if ( name == "marked_for_death"    ) return new marked_for_death_t    ( name, this, options_str );
  if ( name == "mutilate"            ) return new mutilate_t            ( name, this, options_str );
  if ( name == "pistol_shot"         ) return new pistol_shot_t         ( name, this, options_str );
  if ( name == "poisoned_knife"      ) return new poisoned_knife_t      ( name, this, options_str );
  if ( name == "roll_the_bones"      ) return new roll_the_bones_t      ( name, this, options_str );
  if ( name == "rupture"             ) return new rupture_t             ( name, this, options_str );
  if ( name == "secret_technique"    ) return new secret_technique_t    ( name, this, options_str );
  if ( name == "sepsis"              ) return new sepsis_t              ( name, this, options_str );
  if ( name == "serrated_bone_spike" ) return new serrated_bone_spike_t ( name, this, options_str );
  if ( name == "shadow_blades"       ) return new shadow_blades_t       ( name, this, options_str );
  if ( name == "shadow_dance"        ) return new shadow_dance_t        ( name, this, options_str );
  if ( name == "shadowstep"          ) return new shadowstep_t          ( name, this, options_str );
  if ( name == "shadowstrike"        ) return new shadowstrike_t        ( name, this, options_str );
  if ( name == "shadow_vault"        ) return new shadow_vault_t        ( name, this, options_str );
  if ( name == "shuriken_storm"      ) return new shuriken_storm_t      ( name, this, options_str );
  if ( name == "shuriken_tornado"    ) return new shuriken_tornado_t    ( name, this, options_str );
  if ( name == "shuriken_toss"       ) return new shuriken_toss_t       ( name, this, options_str );
  if ( name == "sinister_strike"     ) return new sinister_strike_t     ( name, this, options_str );
  if ( name == "slaughter"           ) return new slaughter_t           ( name, this, options_str );
  if ( name == "slice_and_dice"      ) return new slice_and_dice_t      ( name, this, options_str );
  if ( name == "sprint"              ) return new sprint_t              ( name, this, options_str );
  if ( name == "stealth"             ) return new stealth_t             ( name, this, options_str );
  if ( name == "symbols_of_death"    ) return new symbols_of_death_t    ( name, this, options_str );
  if ( name == "shiv"                ) return new shiv_t                ( name, this, options_str );
  if ( name == "vanish"              ) return new vanish_t              ( name, this, options_str );
  if ( name == "vendetta"            ) return new vendetta_t            ( name, this, options_str );
  if ( name == "cancel_autoattack"   ) return new cancel_autoattack_t   ( this, options_str );
  if ( name == "swap_weapon"         ) return new weapon_swap_t         ( this, options_str );

  return player_t::create_action( name, options_str );
}

// rogue_t::create_expression ===============================================

std::unique_ptr<expr_t> rogue_t::create_expression( util::string_view name_str )
{
  auto split = util::string_split<util::string_view>( name_str, "." );

  if ( name_str == "combo_points" )
    return make_ref_expr( name_str, resources.current[ RESOURCE_COMBO_POINT ] );
  else if ( util::str_compare_ci( name_str, "cp_max_spend" ) )
  {
    return make_mem_fn_expr( name_str, *this, &rogue_t::consume_cp_max );
  }
  else if ( util::str_compare_ci( name_str, "animacharged_cp" ) )
  {
    if(!covenant.echoing_reprimand->ok() )
      return make_mem_fn_expr( name_str, *this, &rogue_t::consume_cp_max );

    return make_fn_expr( name_str, [ this ]() {
      if ( buffs.echoing_reprimand_2->check() )
        return 2;
      else if ( buffs.echoing_reprimand_3->check() )
        return 3;
      else if ( buffs.echoing_reprimand_4->check() )
        return 4;

      return as<int>( consume_cp_max() );
    } );
  }
  else if ( util::str_compare_ci( name_str, "master_assassin_remains" ) && !legendary.master_assassins_mark->ok() )
  {
    return make_fn_expr( name_str, [ this ]() {
      if ( buffs.master_assassin_aura->check() )
      {
        timespan_t nominal_master_assassin_duration = timespan_t::from_seconds( talent.master_assassin->effectN( 1 ).base_value() );
        timespan_t gcd_remains = timespan_t::from_seconds( std::max( ( gcd_ready - sim->current_time() ).total_seconds(), 0.0 ) );
        return gcd_remains + nominal_master_assassin_duration;
      }
      else if ( buffs.master_assassin->check() )
        return buffs.master_assassin->remains();
      else
        return timespan_t::from_seconds( 0.0 );
    } );
  }
  else if ( legendary.master_assassins_mark->ok() &&
    ( util::str_compare_ci( name_str, "master_assassins_mark_remains" ) || util::str_compare_ci( name_str, "master_assassin_remains" ) ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      if ( buffs.master_assassins_mark_aura->check() )
      {
        timespan_t nominal_master_assassin_duration = timespan_t::from_seconds( legendary.master_assassins_mark->effectN( 1 ).base_value() );
        timespan_t gcd_remains = timespan_t::from_seconds( std::max( ( gcd_ready - sim->current_time() ).total_seconds(), 0.0 ) );
        return gcd_remains + nominal_master_assassin_duration;
      }
      else if ( buffs.master_assassins_mark->check() )
        return buffs.master_assassins_mark->remains();
      else
        return timespan_t::from_seconds( 0.0 );
    } );
  }
  else if ( util::str_compare_ci( name_str, "poisoned" ) )
    return make_fn_expr( name_str, [ this ]() {
      rogue_td_t* tdata = get_target_data( target );
      return tdata -> is_lethal_poisoned();
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
             tdata -> dots.rupture -> is_ticking() +
             tdata -> dots.crimson_tempest -> is_ticking();
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
        if ( tdata -> is_lethal_poisoned() ) {
          poisoned_bleeds += tdata -> dots.garrote -> is_ticking() +
                             tdata -> dots.internal_bleeding -> is_ticking() +
                             tdata -> dots.rupture -> is_ticking();
        }
      }
      return poisoned_bleeds;
    } );
  }
  else if ( util::str_compare_ci( name_str, "non_ss_buffed_targets" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      int non_ss_buffed_targets = 0;
      for ( size_t i = 0, actors = sim -> target_non_sleeping_list.size(); i < actors; i++ )
      {
        player_t* t = sim -> target_non_sleeping_list[i];
        rogue_td_t* tdata = get_target_data( t );
        if ( ! tdata -> dots.garrote -> is_ticking() || 
             ! debug_cast<const actions::garrote_t::garrote_state_t*>( tdata -> dots.garrote -> state ) -> shrouded_suffocation )
        {
          non_ss_buffed_targets++;
        }
      }
      return non_ss_buffed_targets;
    } );
  }
  else if ( util::str_compare_ci( name_str, "ss_buffed_targets_above_pandemic" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      int ss_buffed_targets_above_pandemic = 0;
      for ( size_t i = 0, actors = sim -> target_non_sleeping_list.size(); i < actors; i++ )
      {
        player_t* t = sim -> target_non_sleeping_list[i];
        rogue_td_t* tdata = get_target_data( t );
        if ( tdata -> dots.garrote -> remains() > timespan_t::from_seconds( 5.4 )
             && debug_cast<const actions::garrote_t::garrote_state_t*>( tdata -> dots.garrote -> state ) -> shrouded_suffocation )
        {
          ss_buffed_targets_above_pandemic++;
        }
      }
      return ss_buffed_targets_above_pandemic;
    } );
  }
  else if ( util::str_compare_ci( name_str, "rtb_buffs" ) )
  {
    if ( specialization() != ROGUE_OUTLAW )
    {
      return expr_t::create_constant( name_str, 0 );
    }

    return make_fn_expr( name_str, [ this ]() {
      double n_buffs = 0;
      n_buffs += buffs.skull_and_crossbones -> check() != 0;
      n_buffs += buffs.grand_melee -> check() != 0;
      n_buffs += buffs.ruthless_precision -> check() != 0;
      n_buffs += buffs.true_bearing -> check() != 0;
      n_buffs += buffs.broadside -> check() != 0;
      n_buffs += buffs.buried_treasure -> check() != 0;
      return n_buffs;
    } );
  }
  else if ( util::str_compare_ci(name_str, "priority_rotation") )
  {
    return make_ref_expr(name_str, options.priority_rotation);
  }

  // Split expressions

  // stealthed.(rogue|mantle|all)
  // rogue: all rogue abilities are checked (stealth, vanish, shadow_dance, subterfuge)
  // mantle: all abilities that maintain Mantle of the Master Assassin aura are checked (stealth, vanish)
  // all: all abilities that allow stealth are checked (rogue + shadowmeld)
  if ( split.size() == 2 && util::str_compare_ci( split[ 0 ], "stealthed" ) )
  {
    if ( util::str_compare_ci( split[ 1 ], "rogue" ) )
    {
      return make_fn_expr( split[ 0 ], [ this ]() {
        return stealthed( STEALTH_BASIC | STEALTH_ROGUE );
      } );
    }
    else if ( util::str_compare_ci( split[ 1 ], "mantle" ) )
    {
      return make_fn_expr( split[ 0 ], [ this ]() {
        return stealthed( STEALTH_BASIC );
      } );
    }
    else if ( util::str_compare_ci( split[ 1 ], "all" ) )
    {
      return make_fn_expr( split[ 0 ], [ this ]() {
        return stealthed();
      } );
    }
  }
  // exsanguinated.(garrote|internal_bleeding|rupture|crimson_tempest)
  else if ( split.size() == 2 && util::str_compare_ci( split[ 0 ], "exsanguinated" ) &&
    ( util::str_compare_ci( split[ 1 ], "garrote" ) ||
      util::str_compare_ci( split[ 1 ], "internal_bleeding" ) ||
      util::str_compare_ci( split[ 1 ], "rupture" ) ||
      util::str_compare_ci( split[ 1 ], "crimson_tempest" ) ) )
  {
    action_t* action = find_action( split[ 1 ] );
    if ( !action )
    {
      return expr_t::create_constant( "exsanguinated_expr", 0 );
    }

    return std::make_unique<exsanguinated_expr_t>( action );
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
      buffs.broadside,
      buffs.buried_treasure,
      buffs.grand_melee,
      buffs.ruthless_precision,
      buffs.skull_and_crossbones,
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
      unsigned attack_x = util::to_unsigned( split[ 1 ] );
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
        range::sort( attacks );
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

  return player_t::create_expression( name_str );
}

std::unique_ptr<expr_t> rogue_t::create_resource_expression( util::string_view name_str )
{
  auto splits = util::string_split<util::string_view>( name_str, "." );
  if ( splits.empty() )
    return nullptr;

  resource_e r = util::parse_resource_type( splits[ 0 ] );
  if ( r == RESOURCE_ENERGY )
  {
    // Custom Rogue Energy Regen Functions
    // Handles things that are outside of the normal resource_regen_per_second flow
    if ( splits.size() == 2 && ( splits[ 1 ] == "regen" || splits[ 1 ] == "time_to_max" ) )
    {
      const bool regen = ( splits[ 1 ] == "regen" );
      return make_fn_expr( name_str, [ this, regen ] {
        const double energy_deficit = resources.max[ RESOURCE_ENERGY ] - resources.current[ RESOURCE_ENERGY ];
        double energy_regen_per_second = resource_regen_per_second( RESOURCE_ENERGY );
        
        if ( buffs.adrenaline_rush->check() )
        {
          energy_regen_per_second *= 1.0 + buffs.adrenaline_rush->data().effectN( 1 ).percent();
        }

        energy_regen_per_second += buffs.nothing_personal->check_value();
        energy_regen_per_second += buffs.buried_treasure->check_value();
        energy_regen_per_second += buffs.vendetta->check_value();

        if ( buffs.blade_rush->check() )
        {
          energy_regen_per_second += ( buffs.blade_rush->data().effectN( 1 ).base_value() / buffs.blade_rush->buff_period.total_seconds() );
        }

        // TODO - Add support for Venomous Vim, Master of Shadows, and potentially also estimated Combat Potency, ShT etc.
        //        Also consider if buffs such as Adrenaline Rush/Lucid should be prorated based on duration for time_to_max

        if ( player_t::buffs.memory_of_lucid_dreams->check() )
        {
          energy_regen_per_second *= 1.0 + player_t::buffs.memory_of_lucid_dreams->data().effectN( 1 ).percent();
        }

        return regen ? energy_regen_per_second : energy_deficit / energy_regen_per_second;
      } );
    }
  }

  return player_t::create_resource_expression( name_str );
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
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *= 1.0 + spec.combat_potency -> effectN( 1 ).percent();
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *= 1.0 + talent.vigor -> effectN( 2 ).percent();

  base_gcd = timespan_t::from_seconds( 1.0 );
  min_gcd  = timespan_t::from_seconds( 1.0 );

  if ( options.rogue_ready_trigger )
  {
    ready_type = READY_TRIGGER;
  }
}

// rogue_t::init_spells =====================================================

void rogue_t::init_spells()
{
  player_t::init_spells();

  // Shared
  spec.shadowstep           = find_specialization_spell( "Shadowstep" );

  // Generic
  spec.all_rogue            = find_spell( 137034 );
  spec.assassination_rogue  = find_specialization_spell( "Assassination Rogue" );
  spec.outlaw_rogue         = find_specialization_spell( "Outlaw Rogue" );
  spec.subtlety_rogue       = find_specialization_spell( "Subtlety Rogue" );

  // Assassination
  spec.deadly_poison        = find_specialization_spell( "Deadly Poison" );
  spec.envenom              = find_specialization_spell( "Envenom" );
  spec.improved_poisons     = find_specialization_spell( "Improved Poisons" );
  spec.improved_poisons_2   = find_rank_spell( "Improved Poisons", "Rank 2" );
  spec.seal_fate            = find_specialization_spell( "Seal Fate" );
  spec.venomous_wounds      = find_specialization_spell( "Venomous Wounds" );
  spec.vendetta             = find_specialization_spell( "Vendetta" );
  spec.master_assassin      = find_spell( 256735 );
  spec.garrote              = find_specialization_spell( "Garrote" );
  spec.garrote_2            = find_specialization_spell( 231719 );
  spec.shiv_2               = find_rank_spell( "Shiv", "Rank 2" );
  spec.shiv_2_debuff        = find_spell( 319504 );
  spec.slice_and_dice_2     = find_rank_spell( "Slice and Dice", "Rank 2" );
  spec.wound_poison_2       = find_rank_spell( "Wound Poison", "Rank 2" );

  // Outlaw
  spec.adrenaline_rush      = find_spell( 13750 ); // Needs to be generic due to Celerity
  spec.between_the_eyes     = find_specialization_spell( "Between the Eyes" );
  spec.between_the_eyes_2   = find_rank_spell( "Between the Eyes", "Rank 2" );
  spec.blade_flurry         = find_specialization_spell( "Blade Flurry" );
  spec.blade_flurry_2       = find_rank_spell( "Blade Flurry", "Rank 2" );
  spec.broadside            = find_spell( 193356 );
  spec.combat_potency       = find_specialization_spell( "Combat Potency" );
  spec.combat_potency_2     = find_rank_spell( "Combat Potency", "Rank 2" );
  spec.restless_blades      = find_specialization_spell( "Restless Blades" );
  spec.roll_the_bones       = find_specialization_spell( "Roll the Bones" );
  spec.ruthlessness         = find_specialization_spell( "Ruthlessness" );
  spec.ruthless_precision   = find_spell( 193357 );
  spec.sinister_strike      = find_specialization_spell( "Sinister Strike" );
  spec.sinister_strike_2    = find_rank_spell( "Sinister Strike", "Rank 2", ROGUE_OUTLAW );

  // Subtlety
  spec.backstab_2           = find_rank_spell( "Backstab", "Rank 2" );
  spec.deepening_shadows    = find_specialization_spell( "Deepening Shadows" );
  spec.find_weakness        = find_specialization_spell( "Find Weakness" );
  spec.relentless_strikes   = find_specialization_spell( "Relentless Strikes" );
  spec.shadow_blades        = find_specialization_spell( "Shadow Blades" );
  spec.shadow_dance         = find_specialization_spell( "Shadow Dance" );
  spec.shadow_techniques    = find_specialization_spell( "Shadow Techniques" );
  spec.shadow_techniques_effect = find_spell( 196911 );
  spec.symbols_of_death     = find_specialization_spell( "Symbols of Death" );
  spec.symbols_of_death_2   = find_rank_spell( "Symbols of Death", "Rank 2" );
  spec.symbols_of_death_autocrit = find_spell( 227151 );
  spec.eviscerate           = find_class_spell( "Eviscerate" );
  spec.eviscerate_2         = find_rank_spell( "Eviscerate", "Rank 2" );
  spec.shadowstrike         = find_specialization_spell( "Shadowstrike" );
  spec.shadowstrike_2       = find_spell( 245623 );
  spec.shuriken_storm_2     = find_rank_spell( "Shuriken Storm", "Rank 2" );

  // Masteries
  mastery.potent_assassin   = find_mastery_spell( ROGUE_ASSASSINATION );
  mastery.main_gauche       = find_mastery_spell( ROGUE_OUTLAW );
  mastery.executioner       = find_mastery_spell( ROGUE_SUBTLETY );

  // Misc spells
  spell.poison_bomb_driver            = find_spell( 255545 );
  spell.critical_strikes              = find_spell( 157442 );
  spell.fan_of_knives                 = find_class_spell( "Fan of Knives" );
  spell.fleet_footed                  = find_spell( 31209 );
  spell.master_of_shadows             = find_spell( 196980 );
  spell.nightstalker_dmg_amp          = find_spell( 130493 );
  spell.sprint                        = find_class_spell( "Sprint" );
  spell.sprint_2                      = find_spell( 231691 );
  spell.ruthlessness_driver           = find_spell( 14161 );
  spell.ruthlessness_cp               = spec.ruthlessness -> effectN( 1 ).trigger();
  spell.shadow_focus                  = find_spell( 112942 );
  spell.subterfuge                    = find_spell( 115192 );
  spell.relentless_strikes_energize   = find_spell( 98440 );
  spell.slice_and_dice                = find_class_spell( "Slice and Dice" );

  // Talents ================================================================

  // Shared
  talent.weaponmaster       = find_talent_spell( "Weaponmaster" ); // Note: this will return a different spell depending on the spec.

  talent.nightstalker       = find_talent_spell( "Nightstalker" );
  talent.subterfuge         = find_talent_spell( "Subterfuge" );

  talent.vigor              = find_talent_spell( "Vigor" );
  talent.deeper_stratagem   = find_talent_spell( "Deeper Stratagem" );
  talent.marked_for_death   = find_talent_spell( "Marked for Death" );

  talent.prey_on_the_weak   = find_talent_spell( "Prey on the Weak" );

  talent.alacrity           = find_talent_spell( "Alacrity" );

  // Assassination
  talent.master_poisoner    = find_talent_spell( "Master Poisoner" );
  talent.elaborate_planning = find_talent_spell( "Elaborate Planning" );
  talent.blindside          = find_talent_spell( "Blindside" );

  talent.master_assassin    = find_talent_spell( "Master Assassin" );

  talent.leeching_poison    = find_talent_spell( "Leeching Poison" );

  talent.internal_bleeding  = find_talent_spell( "Internal Bleeding" );

  talent.venom_rush         = find_talent_spell( "Venom Rush" );
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
  talent.dreadblades        = find_talent_spell( "Dreadblades" );

  talent.dancing_steel      = find_talent_spell( "Dancing Steel" );
  talent.blade_rush         = find_talent_spell( "Blade Rush" );
  talent.killing_spree      = find_talent_spell( "Killing Spree" );

  // Subtlety
  talent.premeditation      = find_talent_spell( "Premeditation" );
  talent.gloomblade         = find_talent_spell( "Gloomblade" );

  talent.shadow_focus       = find_talent_spell( "Shadow Focus" );

  talent.enveloping_shadows = find_talent_spell( "Enveloping Shadows" );
  talent.dark_shadow        = find_talent_spell( "Dark Shadow" );

  talent.master_of_shadows  = find_talent_spell( "Master of Shadows" );
  talent.secret_technique   = find_talent_spell( "Secret Technique" );
  talent.shuriken_tornado   = find_talent_spell( "Shuriken Tornado" );

  // Azerite Powers =========================================================

  azerite.ace_up_your_sleeve   = find_azerite_spell( "Ace Up Your Sleeve" );
  azerite.blade_in_the_shadows = find_azerite_spell( "Blade In The Shadows" );
  azerite.brigands_blitz       = find_azerite_spell( "Brigand's Blitz" );
  azerite.deadshot             = find_azerite_spell( "Deadshot" );
  azerite.double_dose          = find_azerite_spell( "Double Dose" );
  azerite.echoing_blades       = find_azerite_spell( "Echoing Blades" );
  azerite.inevitability        = find_azerite_spell( "Inevitability" );
  azerite.keep_your_wits_about_you = find_azerite_spell( "Keep Your Wits About You" );
  azerite.nights_vengeance     = find_azerite_spell( "Night's Vengeance" );
  azerite.nothing_personal     = find_azerite_spell( "Nothing Personal" );
  azerite.paradise_lost        = find_azerite_spell( "Paradise Lost" );
  azerite.perforate            = find_azerite_spell( "Perforate" );
  azerite.replicating_shadows  = find_azerite_spell( "Replicating Shadows" );
  azerite.scent_of_blood       = find_azerite_spell( "Scent of Blood" );
  azerite.shrouded_suffocation = find_azerite_spell( "Shrouded Suffocation" );
  azerite.snake_eyes           = find_azerite_spell( "Snake Eyes" );
  azerite.the_first_dance      = find_azerite_spell( "The First Dance" );
  azerite.twist_the_knife      = find_azerite_spell( "Twist the Knife" );

  // Azerite Essences =======================================================

  azerite.memory_of_lucid_dreams  = find_azerite_essence( "Memory of Lucid Dreams" );
  spell.memory_of_lucid_dreams    = azerite.memory_of_lucid_dreams.spell( 1u, essence_type::MINOR );

  azerite.vision_of_perfection            = find_azerite_essence( "Vision of Perfection" );
  azerite.vision_of_perfection_percentage = azerite.vision_of_perfection.spell( 1u, essence_type::MAJOR )->effectN( 1 ).percent();
  azerite.vision_of_perfection_percentage += azerite.vision_of_perfection.spell( 2u, essence_spell::UPGRADE, essence_type::MAJOR )->effectN( 1 ).percent();

  // Covenant Abilities =====================================================

  covenant.echoing_reprimand      = find_covenant_spell( "Echoing Reprimand" );
  covenant.sepsis                 = find_covenant_spell( "Sepsis" );
  covenant.serrated_bone_spike    = find_covenant_spell( "Serrated Bone Spike" );
  covenant.slaughter              = find_covenant_spell( "Slaughter" );

  // Conduits ===============================================================

  conduit.lethal_poisons          = find_conduit_spell( "Lethal Poisons" );
  conduit.maim_mangle             = find_conduit_spell( "Maim, Mangle" );
  conduit.poisoned_katar          = find_conduit_spell( "Poisoned Katar");
  conduit.well_placed_steel       = find_conduit_spell( "Well-Placed Steel" );

  conduit.ambidexterity           = find_conduit_spell( "Ambidexterity" );
  conduit.count_the_odds          = find_conduit_spell( "Count the Odds" );
  conduit.slight_of_hand          = find_conduit_spell( "Slight of Hand" );
  conduit.triple_threat           = find_conduit_spell( "Triple Threat" );

  conduit.deeper_daggers          = find_conduit_spell( "Deeper Daggers" );
  conduit.perforated_veins        = find_conduit_spell( "Perforated Veins" );
  conduit.planned_execution       = find_conduit_spell( "Planned Execution" );
  conduit.stiletto_staccato       = find_conduit_spell( "Stiletto Staccato" );

  conduit.reverberation           = find_conduit_spell( "Reverberation" );
  conduit.slaughter_scars         = find_conduit_spell( "Slaughter Scars" );
  conduit.sudden_fractures        = find_conduit_spell( "Sudden Fractures" );
  conduit.septic_shock            = find_conduit_spell( "Septic Shock" );

  // Legendary Items ========================================================

  // Generic
  legendary.essence_of_bloodfang      = find_runeforge_legendary( "Essence of Bloodfang" );
  legendary.master_assassins_mark     = find_runeforge_legendary( "Mark of the Master Assassin" );
  legendary.tiny_toxic_blades         = find_runeforge_legendary( "Tiny Toxic Blades" );
  legendary.invigorating_shadowdust   = find_runeforge_legendary( "Invigorating Shadowdust" );

  // Assassination
  legendary.dashing_scoundrel         = find_runeforge_legendary( "Dashing Scoundrel" );
  legendary.doomblade                 = find_runeforge_legendary( "Doomblade" );
  legendary.dustwalkers_patch         = find_runeforge_legendary( "Dustwalker's Patch" );
  legendary.zoldyck_insignia          = find_runeforge_legendary( "Zoldyck Insignia" );

  // Outlaw
  legendary.greenskins_wickers        = find_runeforge_legendary( "Greenskin's Wickers" );
  legendary.guile_charm               = find_runeforge_legendary( "Guile Charm" );
  legendary.celerity                  = find_runeforge_legendary( "Celerity" );
  legendary.concealed_blunderbuss     = find_runeforge_legendary( "Concealed Blunderbuss" );

  // Subtlety
  legendary.akaaris_soul_fragment     = find_runeforge_legendary( "Akaari's Soul Fragment" );
  legendary.deathly_shadows           = find_runeforge_legendary( "Deathly Shadows" );
  legendary.finality                  = find_runeforge_legendary( "Finality" );
  legendary.the_rotten                = find_runeforge_legendary( "The Rotten" );

  // Spell Setup
  if ( legendary.essence_of_bloodfang->ok() )
  {
    active.bloodfang = get_background_action<actions::bloodfang_t>( "bloodfang" );
  }

  if ( legendary.dashing_scoundrel->ok() )
  {
    legendary.dashing_scoundrel_gain = find_spell( 340426 )->effectN( 1 ).resource( RESOURCE_ENERGY );
  }

  if ( legendary.akaaris_soul_fragment->ok() )
  {
    active.akaaris_soul_fragment = get_secondary_trigger_action<actions::shadowstrike_t>( TRIGGER_AKAARIS_SOUL_FRAGMENT, "shadowstrike_akaaris_soul_fragment" );
    active.akaaris_soul_fragment->base_multiplier *= legendary.akaaris_soul_fragment->effectN( 2 ).percent();
  }

  if ( legendary.concealed_blunderbuss->ok() )
  {
    active.concealed_blunderbuss = get_secondary_trigger_action<actions::pistol_shot_t>( TRIGGER_CONCEALED_BLUNDERBUSS, "pistol_shot_concealed_blunderbuss" );
  }

  // Active Spells = ========================================================

  auto_attack = new actions::auto_melee_attack_t( this, "" );

  if ( mastery.main_gauche->ok() )
  {
    active.main_gauche = get_background_action<actions::main_gauche_t>( "main_gauche" );
  }

  if ( spec.blade_flurry->ok() )
  {
    active.blade_flurry = get_background_action<actions::blade_flurry_attack_t>( "blade_flurry_attack" );
  }

  if ( talent.poison_bomb->ok() )
  {
    active.poison_bomb = get_background_action<actions::poison_bomb_t>( "poison_bomb" );
  }

  if ( azerite.replicating_shadows.ok() )
  {
    active.replicating_shadows = get_background_action<actions::replicating_shadows_t>( "replicating_shadows" );
  }

  if ( talent.weaponmaster->ok() && specialization() == ROGUE_SUBTLETY )
  {
    active.weaponmaster.backstab = get_secondary_trigger_action<actions::backstab_t>( TRIGGER_WEAPONMASTER, "backstab_weaponmaster" );
    active.weaponmaster.shadowstrike = get_secondary_trigger_action<actions::shadowstrike_t>( TRIGGER_WEAPONMASTER, "shadowstrike_weaponmaster" );
  }

  if ( conduit.triple_threat.ok() && specialization() == ROGUE_OUTLAW )
  {
    active.triple_threat_mh = get_secondary_trigger_action<actions::sinister_strike_t>( TRIGGER_TRIPLE_THREAT, "sinister_strike_triple_threat_mh" );
    active.triple_threat_oh = get_secondary_trigger_action<actions::sinister_strike_t::triple_threat_t>( TRIGGER_TRIPLE_THREAT, "sinister_strike_triple_threat_oh" );
  }
}

// rogue_t::init_gains ======================================================

void rogue_t::init_gains()
{
  player_t::init_gains();

  gains.adrenaline_rush          = get_gain( "Adrenaline Rush"          );
  gains.adrenaline_rush_expiry   = get_gain( "Adrenaline Rush (Expiry)" );
  gains.blade_rush               = get_gain( "Blade Rush"               );
  gains.buried_treasure          = get_gain( "Buried Treasure"          );
  gains.combat_potency           = get_gain( "Combat Potency"           );
  gains.energy_refund            = get_gain( "Energy Refund"            );
  gains.seal_fate                = get_gain( "Seal Fate"                );
  gains.vendetta                 = get_gain( "Vendetta"                 );
  gains.venom_rush               = get_gain( "Venom Rush"               );
  gains.venomous_wounds          = get_gain( "Venomous Vim"             );
  gains.venomous_wounds_death    = get_gain( "Venomous Vim (Death)"     );
  gains.quick_draw               = get_gain( "Quick Draw"               );
  gains.broadside                = get_gain( "Broadside"                );
  gains.ruthlessness             = get_gain( "Ruthlessness"             );
  gains.shadow_techniques        = get_gain( "Shadow Techniques"        );
  gains.master_of_shadows        = get_gain( "Master of Shadows"        );
  gains.shadow_blades            = get_gain( "Shadow Blades"            );
  gains.relentless_strikes       = get_gain( "Relentless Strikes"       );
  gains.symbols_of_death         = get_gain( "Symbols of Death"         );
  gains.ace_up_your_sleeve       = get_gain( "Ace Up Your Sleeve"       );
  gains.shrouded_suffocation     = get_gain( "Shrouded Suffocation"     );
  gains.the_first_dance          = get_gain( "The First Dance"          );
  gains.nothing_personal         = get_gain( "Nothing Personal"         );
  gains.memory_of_lucid_dreams   = get_gain( "Memory of Lucid Dreams"   );
  gains.dashing_scoundrel        = get_gain( "Dashing Scoundrel"        );
  gains.the_rotten               = get_gain( "The Rotten"               );
  gains.deathly_shadows          = get_gain( "Deathly Shadows"          );
  gains.serrated_bone_spike      = get_gain( "Serrated Bone Spike"      );
  gains.dreadblades              = get_gain( "Dreadblades"              );
  gains.premeditation            = get_gain( "Premeditation"            );
}

// rogue_t::init_procs ======================================================

void rogue_t::init_procs()
{
  player_t::init_procs();

  procs.seal_fate           = get_proc( "Seal Fate"                    );
  procs.weaponmaster        = get_proc( "Weaponmaster"                 );

  procs.roll_the_bones_1    = get_proc( "Roll the Bones: 1 buff"       );
  procs.roll_the_bones_2    = get_proc( "Roll the Bones: 2 buffs"      );
  procs.roll_the_bones_3    = get_proc( "Roll the Bones: 3 buffs"      );
  procs.roll_the_bones_4    = get_proc( "Roll the Bones: 4 buffs"      );
  procs.roll_the_bones_5    = get_proc( "Roll the Bones: 5 buffs"      );
  procs.roll_the_bones_6    = get_proc( "Roll the Bones: 6 buffs"      );
  static_cast<buffs::roll_the_bones_t*>( buffs.roll_the_bones )->set_procs();

  procs.deepening_shadows   = get_proc( "Deepening Shadows"            );

  procs.echoing_reprimand_2 = get_proc( "Animacharged CP 2 Used"       );
  procs.echoing_reprimand_3 = get_proc( "Animacharged CP 3 Used"       );
  procs.echoing_reprimand_4 = get_proc( "Animacharged CP 4 Used"       );
  procs.count_the_odds      = get_proc( "Count the Odds"               );

  procs.dustwalker_patch    = get_proc( "Dustwalker Patch"             );
}

// rogue_t::init_scaling ====================================================

void rogue_t::init_scaling()
{
  player_t::init_scaling();

  scaling -> enable( STAT_WEAPON_OFFHAND_DPS );
  scaling -> disable( STAT_STRENGTH );

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

  resources.current[ RESOURCE_COMBO_POINT ] = options.initial_combo_points;
}

// rogue_t::init_buffs ======================================================

void rogue_t::create_buffs()
{
  player_t::create_buffs();

  // Baseline ===============================================================
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
  
  // TODO: Possibly refactor into buffs::slice_and_dice_t
  buffs.slice_and_dice        = make_buff( this, "slice_and_dice", spell.slice_and_dice )
                                ->set_default_value_from_effect_type( A_MOD_RANGED_AND_MELEE_ATTACK_SPEED )
                                ->apply_affecting_aura( spec.slice_and_dice_2 )
                                ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
                                ->add_invalidate( CACHE_ATTACK_SPEED );
  if ( legendary.celerity.ok() )
  {
    buffs.slice_and_dice->set_period( spell.slice_and_dice->effectN( 2 ).period() );
    buffs.slice_and_dice->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
      if ( rng().roll( legendary.celerity->effectN( 2 ).percent() ) )
      {
        timespan_t duration = timespan_t::from_seconds( legendary.celerity->effectN( 3 ).base_value() );
        if ( buffs.adrenaline_rush->check() )
          buffs.adrenaline_rush->extend_duration( this, duration );
        else
          buffs.adrenaline_rush->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
      }
    } );
  }

  // Assassination
  buffs.envenom               = make_buff( this, "envenom", spec.envenom )
                                ->set_default_value( spec.envenom->effectN( 2 ).percent() )
                                ->set_duration( timespan_t::min() )
                                ->set_period( timespan_t::zero() )
                                ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
  buffs.vendetta              = make_buff( this, "vendetta_energy", spec.vendetta->effectN( 4 ).trigger() )
                                ->set_default_value( spec.vendetta->effectN( 4 ).trigger()->effectN( 1 ).base_value() / 5.0 )
                                ->set_affects_regen( true );

  // Outlaw
  buffs.adrenaline_rush       = new buffs::adrenaline_rush_t( this );
  buffs.blade_flurry          = new buffs::blade_flurry_t( this );
  buffs.blade_rush            = make_buff( this, "blade_rush", find_spell( 271896 ) )
                                ->set_period( find_spell( 271896 )->effectN( 1 ).period() )
                                ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
                                  resource_gain( RESOURCE_ENERGY, b -> data().effectN( 1 ).base_value(), gains.blade_rush );
                                } );
  buffs.opportunity           = make_buff( this, "opportunity", find_spell( 195627 ) );
  // Roll the bones buffs
  buffs.broadside             = make_buff( this, "broadside", spec.broadside )
                                ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.buried_treasure       = make_buff( this, "buried_treasure", find_spell( 199600 ) )
                                ->set_affects_regen( true )
                                ->set_default_value( find_spell( 199600 )->effectN( 1 ).base_value() / 5.0 );
  buffs.grand_melee           = make_buff( this, "grand_melee", find_spell( 193358 ) )
                                ->add_invalidate( CACHE_LEECH )
                                ->set_default_value( find_spell( 193358 )->effectN( 1 ).base_value() );
  buffs.skull_and_crossbones  = make_buff( this, "skull_and_crossbones", find_spell( 199603 ) )
                                ->set_default_value( find_spell( 199603 )->effectN( 1 ).percent() );
  buffs.ruthless_precision    = make_buff( this, "ruthless_precision", spec.ruthless_precision )
                                ->set_default_value( spec.ruthless_precision->effectN( 1 ).percent() )
                                ->add_invalidate( CACHE_CRIT_CHANCE );
  buffs.true_bearing          = make_buff( this, "true_bearing", find_spell( 193359 ) )
                                ->set_default_value( find_spell( 193359 )->effectN( 1 ).base_value() * 0.1 );
  // Note, since I (navv) am a slacker, this needs to be constructed after the secondary buffs.
  buffs.roll_the_bones        = new buffs::roll_the_bones_t( this );
  
  // Subtlety
  buffs.shadow_blades         = make_buff( this, "shadow_blades", spec.shadow_blades )
                                -> set_cooldown( timespan_t::zero() )
                                -> set_default_value( spec.shadow_blades -> effectN( 1 ).percent() );
  buffs.shadow_dance          = new buffs::shadow_dance_t( this );
  buffs.symbols_of_death      = make_buff( this, "symbols_of_death", spec.symbols_of_death )
                                -> set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
                                -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.symbols_of_death_autocrit = make_buff( this, "symbols_of_death_autocrit", spec.symbols_of_death_autocrit )
                                -> add_invalidate( CACHE_CRIT_CHANCE )
                                -> set_default_value( spec.symbols_of_death_autocrit->effectN( 1 ).percent() );

  // Talents ================================================================
  // Shared
  buffs.alacrity                = make_buff( this, "alacrity", find_spell( 193538 ) )
                                  -> set_default_value( find_spell( 193538 ) -> effectN( 1 ).percent() )
                                  -> set_chance( talent.alacrity -> ok() )
                                  -> add_invalidate( CACHE_HASTE );
  buffs.subterfuge              = new buffs::subterfuge_t( this );

  // Assassination
  buffs.elaborate_planning      = make_buff( this, "elaborate_planning", talent.elaborate_planning -> effectN( 1 ).trigger() )
                                  -> set_default_value( 1.0 + talent.elaborate_planning -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
                                  -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.blindside               = make_buff( this, "blindside", find_spell( 121153 ) )
                                  -> set_default_value( find_spell( 121153 ) -> effectN( 2 ).percent() );
  buffs.master_assassin_aura    = make_buff( this, "master_assassin_aura", talent.master_assassin )
                                  ->set_default_value( spec.master_assassin->effectN( 1 ).percent() )
                                  ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
                                      if ( new_ == 0 )
                                        buffs.master_assassin->trigger();
                                      else
                                        buffs.master_assassin->expire();
                                    } );
  buffs.master_assassin         = make_buff( this, "master_assassin", talent.master_assassin )
                                  -> set_default_value( spec.master_assassin -> effectN( 1 ).percent() )
                                  -> set_duration( timespan_t::from_seconds( talent.master_assassin -> effectN( 1 ).base_value() ) );
  buffs.hidden_blades_driver    = make_buff( this, "hidden_blades_driver", talent.hidden_blades )
                                  -> set_period( talent.hidden_blades -> effectN( 1 ).period() )
                                  -> set_quiet( true )
                                  -> set_tick_callback( [this]( buff_t*, int, timespan_t ) { buffs.hidden_blades -> trigger(); } )
                                  -> set_tick_time_behavior( buff_tick_time_behavior::UNHASTED );
  buffs.hidden_blades           = make_buff( this, "hidden_blades", find_spell( 270070 ) )
                                  -> set_default_value( find_spell( 270070 ) -> effectN( 1 ).percent() );
  // Outlaw
  buffs.loaded_dice             = make_buff( this, "loaded_dice", talent.loaded_dice -> effectN( 1 ).trigger() );
  buffs.killing_spree           = make_buff( this, "killing_spree", talent.killing_spree )
                                  -> set_cooldown( timespan_t::zero() )
                                  -> set_duration( talent.killing_spree -> duration() );
  buffs.dreadblades             = make_buff( this, "dreadblades", talent.dreadblades )
                                  ->set_cooldown( timespan_t::zero() )
                                  ->set_default_value( find_spell( 343143 )->effectN( 1 ).base_value() );

  // Subtlety
  buffs.premeditation           = make_buff( this, "premeditation", find_spell( 343173 ) );
  buffs.master_of_shadows       = make_buff( this, "master_of_shadows", find_spell( 196980 ) )
                                  -> set_period( find_spell( 196980 ) -> effectN( 1 ).period() )
                                  -> set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
                                    resource_gain( RESOURCE_ENERGY, b -> data().effectN( 1 ).base_value(), gains.master_of_shadows );
                                  } )
                                  -> set_refresh_behavior( buff_refresh_behavior::DURATION );
  buffs.secret_technique        = make_buff( this, "secret_technique", talent.secret_technique )
                                  -> set_cooldown( timespan_t::zero() )
                                  -> set_quiet( true );
  buffs.shuriken_tornado        = new buffs::shuriken_tornado_t( this );


  // Azerite ================================================================

  buffs.blade_in_the_shadows               = make_buff( this, "blade_in_the_shadows", find_spell( 279754 ) )
                                             -> set_trigger_spell( azerite.blade_in_the_shadows.spell_ref().effectN( 1 ).trigger() )
                                             -> set_default_value( azerite.blade_in_the_shadows.value() );
  buffs.brigands_blitz                     = make_buff<stat_buff_t>( this, "brigands_blitz", find_spell( 277724 ) )
                                             -> add_stat( STAT_HASTE_RATING, azerite.brigands_blitz.value() )
                                             -> set_refresh_behavior( buff_refresh_behavior::DURATION );
  buffs.brigands_blitz_driver              = make_buff( this, "brigands_blitz_driver", find_spell( 277725 ) )
                                             -> set_trigger_spell( azerite.brigands_blitz.spell_ref().effectN( 1 ).trigger() )
                                             -> set_quiet( true )
                                             -> set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                                                  buffs.brigands_blitz -> trigger();
                                                });
  buffs.deadshot                           = make_buff( this, "deadshot", find_spell( 272940 ) )
                                             -> set_trigger_spell( azerite.deadshot.spell_ref().effectN( 1 ).trigger() )
                                             -> set_default_value( azerite.deadshot.value() );
  buffs.double_dose                        = make_buff( this, "double_dose", find_spell( 273009 ) )
                                             -> set_quiet( true );
  buffs.keep_your_wits_about_you           = make_buff( this, "keep_your_wits_about_you", find_spell( 288988 ) )
                                             -> set_trigger_spell( azerite.keep_your_wits_about_you.spell_ref().effectN( 1 ).trigger() )
                                             -> set_default_value( find_spell( 288988 ) -> effectN( 1 ).percent() );
  buffs.nights_vengeance                   = make_buff( this, "nights_vengeance", find_spell( 273424 ) )
                                             -> set_trigger_spell( azerite.nights_vengeance.spell_ref().effectN( 1 ).trigger() )
                                             -> set_default_value( azerite.nights_vengeance.value() );
  buffs.nothing_personal                   = make_buff( this, "nothing_personal", find_spell( 289467 ) )
                                             -> set_trigger_spell( azerite.nothing_personal.spell_ref().effectN( 1 ).trigger() )
                                             -> set_affects_regen( true )
                                             -> set_default_value( find_spell( 289467 ) -> effectN( 2 ).base_value() / 5.0 );
  buffs.paradise_lost                      = make_buff<stat_buff_t>( this, "paradise_lost", find_spell( 278962 ) )
                                             -> add_stat( STAT_AGILITY, azerite.paradise_lost.value() )
                                             -> set_refresh_behavior( buff_refresh_behavior::DURATION );
  buffs.perforate                          = make_buff( this, "perforate", find_spell( 277720 ) )
                                             -> set_trigger_spell( azerite.perforate.spell_ref().effectN( 1 ).trigger() )
                                             -> set_default_value( azerite.perforate.value() );
  buffs.scent_of_blood                     = make_buff<stat_buff_t>( this, "scent_of_blood", find_spell( 277731 ) )
                                             -> add_stat( STAT_AGILITY, azerite.scent_of_blood.value() )
                                             -> set_duration( timespan_t::zero() ); // Infinite aura
  buffs.snake_eyes                         = make_buff( this, "snake_eyes", find_spell( 275863 ) )
                                             -> set_trigger_spell( azerite.snake_eyes.spell_ref().effectN( 1 ).trigger() )
                                             -> set_default_value( azerite.snake_eyes.value() );
  buffs.the_first_dance                    = make_buff<stat_buff_t>( this, "the_first_dance", find_spell( 278981 ) )
                                             -> add_stat( STAT_HASTE_RATING, azerite.the_first_dance.value() );

  // Covenants ==============================================================

  buffs.echoing_reprimand_2 = make_buff( this, "echoing_reprimand_2", covenant.echoing_reprimand->ok() ?
                                         find_spell( 323558 ) : spell_data_t::not_found() )
                                          ->set_max_stack( 1 )
                                          ->set_default_value( 2 );
  buffs.echoing_reprimand_3 = make_buff( this, "echoing_reprimand_3", covenant.echoing_reprimand->ok() ?
                                         find_spell( 323559 ) : spell_data_t::not_found() )
                                          ->set_max_stack( 1 )
                                          ->set_default_value( 3 );
  buffs.echoing_reprimand_4 = make_buff( this, "echoing_reprimand_4", covenant.echoing_reprimand->ok() ?
                                         find_spell( 323560 ) : spell_data_t::not_found() )
                                          ->set_max_stack( 1 )
                                          ->set_default_value( 4 );

  // Conduits ===============================================================

  buffs.deeper_daggers = make_buff( this, "deeper_daggers", conduit.deeper_daggers->effectN( 1 ).trigger() )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    ->set_trigger_spell( conduit.deeper_daggers )
    ->set_default_value( conduit.deeper_daggers.percent() );

  buffs.perforated_veins = make_buff( this, "perforated_veins", conduit.perforated_veins->effectN( 1 ).trigger() )
    ->set_trigger_spell( conduit.perforated_veins )
    ->set_default_value( conduit.perforated_veins.percent() );

  if ( conduit.planned_execution.ok() )
    buffs.symbols_of_death->add_invalidate( CACHE_CRIT_CHANCE );

  // Legendary Items ========================================================

  buffs.deathly_shadows = make_buff( this, "deathly_shadows", legendary.deathly_shadows->effectN( 1 ).trigger() )
    ->set_default_value( legendary.deathly_shadows->effectN( 1 ).trigger()->effectN( 1 ).percent() );

  const spell_data_t* master_assassins_mark = legendary.master_assassins_mark->ok() ? find_spell( 340094 ) : spell_data_t::not_found();
  buffs.master_assassins_mark_aura = make_buff( this, "master_assassins_mark_aura", master_assassins_mark )
    ->add_invalidate( CACHE_CRIT_CHANCE )
    ->set_default_value( find_spell( 340094 )->effectN( 1 ).percent() )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
        if ( new_ == 0 )
          buffs.master_assassins_mark->trigger();
        else
          buffs.master_assassins_mark->expire();
      } );
  buffs.master_assassins_mark = make_buff( this, "master_assassins_mark", master_assassins_mark )
    ->add_invalidate( CACHE_CRIT_CHANCE )
    ->set_duration( timespan_t::from_seconds( legendary.master_assassins_mark->effectN( 1 ).base_value() ) )
    ->set_default_value( find_spell( 340094 )->effectN( 1 ).percent() );

  buffs.the_rotten = make_buff( this, "the_rotten", legendary.the_rotten->effectN( 1 ).trigger() );

  buffs.guile_charm_insight_1 = make_buff( this, "shallow_insight", find_spell( 340582 ) )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    ->set_default_value( find_spell( 340582 )->effectN( 1 ).percent() )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int ) {
      legendary.guile_charm_counter = 0;
    } );
  buffs.guile_charm_insight_2 = make_buff( this, "moderate_insight", find_spell( 340583 ) )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    ->set_default_value( find_spell( 340583 )->effectN( 1 ).percent() )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int ) {
      buffs.guile_charm_insight_1->expire();
      legendary.guile_charm_counter = 0; 
    } );
  buffs.guile_charm_insight_3 = make_buff( this, "deep_insight", find_spell( 340584 ) )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    ->set_default_value( find_spell( 340584 )->effectN( 1 ).percent() )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int ) {
      buffs.guile_charm_insight_2->expire();
      legendary.guile_charm_counter = 0;
    } );

  buffs.finality_eviscerate = make_buff( this, "finality_eviscerate", find_spell( 340600 ) )
    ->set_default_value( find_spell( 340600 )->effectN( 1 ).percent() );
  buffs.finality_rupture = make_buff( this, "finality_rupture", find_spell( 340601 ) )
    ->set_default_value( find_spell( 340601 )->effectN( 1 ).percent() );
  buffs.finality_shadow_vault = make_buff( this, "finality_shadow_vault", find_spell( 340603 ) )
    ->set_default_value( find_spell( 340603 )->effectN( 1 ).percent() );

  buffs.concealed_blunderbuss = make_buff( this, "concealed_blunderbuss", find_spell( 340587 ) )
    ->set_chance( legendary.concealed_blunderbuss->effectN( 1 ).percent() )
    ->set_default_value( legendary.concealed_blunderbuss->effectN( 2 ).base_value() );

  buffs.greenskins_wickers = make_buff( this, "greenskins_wickers", find_spell( 340573 ) )
    ->set_default_value( find_spell( 340573 )->effectN( 1 ).percent() );
}

// rogue_t::create_options ==================================================

static bool do_parse_secondary_weapon( rogue_t* rogue, util::string_view value, slot_e slot )
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

static bool parse_offhand_secondary( sim_t* sim, util::string_view /* name */, util::string_view value )
{
  rogue_t* rogue = static_cast<rogue_t*>( sim -> active_player );
  return do_parse_secondary_weapon( rogue, value, SLOT_OFF_HAND );
}

static bool parse_mainhand_secondary( sim_t* sim, util::string_view /* name */, util::string_view value )
{
  rogue_t* rogue = static_cast<rogue_t*>( sim -> active_player );
  return do_parse_secondary_weapon( rogue, value, SLOT_MAIN_HAND );
}

static bool parse_fixed_rtb( sim_t* sim, util::string_view /* name */, util::string_view value )
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
    sim -> error( "{}: No valid 'fixed_rtb' buffs given by string '{}'", sim -> active_player -> name(),
        value );
    return false;
  }

  debug_cast<rogue_t*>( sim->active_player )->options.fixed_rtb = buffs;

  return true;
}

static bool parse_fixed_rtb_odds( sim_t* sim, util::string_view /* name */, util::string_view value )
{
  auto odds = util::string_split<util::string_view>( value, "," );
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
    buff_chances[ i ] = util::to_double( odds[ i ] );
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

  debug_cast<rogue_t*>( sim->active_player )->options.fixed_rtb_odds = buff_chances;
  return true;
}

void rogue_t::create_options()
{
  player_t::create_options();

  // Overload default options but with a default true value
  add_option( opt_bool( "ready_trigger", options.rogue_ready_trigger ) );
  add_option( opt_bool( "optimize_expressions", options.rogue_optimize_expressions ) );

  add_option( opt_func( "off_hand_secondary", parse_offhand_secondary ) );
  add_option( opt_func( "main_hand_secondary", parse_mainhand_secondary ) );
  add_option( opt_int( "initial_combo_points", options.initial_combo_points ) );
  add_option( opt_func( "fixed_rtb", parse_fixed_rtb ) );
  add_option( opt_func( "fixed_rtb_odds", parse_fixed_rtb_odds ) );
  add_option( opt_bool( "priority_rotation", options.priority_rotation ) );
  add_option( opt_float( "memory_of_lucid_dreams_proc_chance", options.memory_of_lucid_dreams_proc_chance, 0.0, 1.0 ) );
}

// rogue_t::copy_from =======================================================

void rogue_t::copy_from( player_t* source )
{
  rogue_t* rogue = static_cast<rogue_t*>( source );
  player_t::copy_from( source );

  if ( !rogue->weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.options_str.empty() )
  {
    weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.options_str = \
      rogue->weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.options_str;
  }

  if ( !rogue->weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.options_str.empty() )
  {
    weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.options_str = \
      rogue->weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.options_str;
  }

  if ( rogue->options.initial_combo_points != 0 )
  {
    options.initial_combo_points = rogue->options.initial_combo_points;
  }

  options.fixed_rtb = rogue->options.fixed_rtb;
  options.fixed_rtb_odds = rogue->options.fixed_rtb_odds;
  options.rogue_optimize_expressions = rogue->options.rogue_optimize_expressions;
  options.rogue_ready_trigger = rogue->options.rogue_ready_trigger;
  options.priority_rotation = rogue->options.priority_rotation;
  options.memory_of_lucid_dreams_proc_chance = rogue->options.memory_of_lucid_dreams_proc_chance;
}

// rogue_t::create_profile  =================================================

std::string rogue_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  // Break out early if we are not saving everything, or gear
  if ( !(stype & SAVE_PLAYER ) && !(stype & SAVE_GEAR ) )
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

void rogue_t::init_items()
{
  player_t::init_items();

  // Initialize weapon swapping data structures for primary weapons here
  weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_PRIMARY ] = main_hand_weapon;
  weapon_data[ WEAPON_MAIN_HAND ].item_data[ WEAPON_PRIMARY ] = &( items[ SLOT_MAIN_HAND ] );
  weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_PRIMARY ] = off_hand_weapon;
  weapon_data[ WEAPON_OFF_HAND ].item_data[ WEAPON_PRIMARY ] = &( items[ SLOT_OFF_HAND ] );

  if ( ! weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.options_str.empty() )
  {
    weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.init();
    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_SECONDARY ] = main_hand_weapon;
    weapon_data[ WEAPON_MAIN_HAND ].item_data[ WEAPON_SECONDARY ] = &( weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data );

    // Restore primary main hand weapon after secondary weapon init
    main_hand_weapon = weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_PRIMARY ];
  }

  if ( ! weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.options_str.empty() )
  {
    weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.init();
    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_SECONDARY ] = off_hand_weapon;
    weapon_data[ WEAPON_OFF_HAND ].item_data[ WEAPON_SECONDARY ] = &( weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data );

    // Restore primary off hand weapon after secondary weapon init
    main_hand_weapon = weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_PRIMARY ];
  }
}

// rogue_t::init_special_effects ============================================

void rogue_t::init_special_effects()
{
  player_t::init_special_effects();

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

  // Memory of Lucid Dreams
  if ( options.memory_of_lucid_dreams_proc_chance < 0 )
  {
    switch ( specialization() )
    {
      case ROGUE_ASSASSINATION:
        options.memory_of_lucid_dreams_proc_chance = 0.15;
        break;
      case ROGUE_OUTLAW:
        options.memory_of_lucid_dreams_proc_chance = 0.15;
        break;
      case ROGUE_SUBTLETY:
        options.memory_of_lucid_dreams_proc_chance = 0.15;
        break;
	  default:
		  break;
    }
  }
}

// rogue_t::init_finished ===================================================

void rogue_t::init_finished()
{
  weapon_data[ WEAPON_MAIN_HAND ].initialize();
  weapon_data[ WEAPON_OFF_HAND ].initialize();

  player_t::init_finished();
}

// rogue_t::reset ===========================================================

void rogue_t::reset()
{
  player_t::reset();

  shadow_techniques = 0;

  last_rupture_target = nullptr;

  restealth_allowed = false;

  weapon_data[ WEAPON_MAIN_HAND ].reset();
  weapon_data[ WEAPON_OFF_HAND ].reset();
}

// rogue_t::activate ========================================================

struct restealth_callback_t
{
  rogue_t* r;
  restealth_callback_t( rogue_t* p ) : r( p )
  { }

  void operator()( player_t* )
  {
    // Special case: We allow stealth when the actor has no active targets
    // (which excludes invulnerable ones if ignore_invulnerable_targets is set like in the DungeonSlice fightstyle).
    // This allows us to better approximate restealthing in dungeons.
    if ( r->sim->target_non_sleeping_list.empty() )
    {
      r->restealth_allowed = true;
      r->cancel_auto_attack();
    }
  }
};

void rogue_t::activate()
{
  player_t::activate();

  sim->target_non_sleeping_list.register_callback( restealth_callback_t( this ) );
}

// rogue_t::is_target_poisoned ==============================================
// Due to how our DOT system functions, at the time when last_tick() is called
// for Deadly Poison, is_ticking() for the dot object will still return true.
// This breaks the is_ticking() check below, creating an inconsistent state in
// the sim, if Deadly Poison was the only poison up on the target. As a
// workaround, deliver the "Deadly Poison fade event" as an extra parameter.
bool rogue_t::is_target_poisoned( player_t* target, bool deadly_fade ) const
{
  const rogue_td_t* td = get_target_data( target );

  if ( !deadly_fade && td->dots.deadly_poison->is_ticking() )
    return true;

  if ( td->debuffs.wound_poison->check() )
    return true;

  if ( td->debuffs.crippling_poison->check() )
    return true;

  return false;
}

// rogue_t::break_stealth ===================================================

void rogue_t::break_stealth()
{
  // Expiry delayed by 1ms in order to have it processed on the next tick. This seems to be what the server does.
  if ( buffs.stealth->check() )
  {
    buffs.stealth->expire( timespan_t::from_millis( 1 ) );
  }

  if ( buffs.vanish->check() )
  {
    buffs.vanish->expire( timespan_t::from_millis( 1 ) );
  }
}

// rogue_t::cancel_auto_attack ==============================================

void rogue_t::cancel_auto_attack()
{
  // Stop autoattacks
  if ( main_hand_attack && main_hand_attack->execute_event )
    event_t::cancel( main_hand_attack->execute_event );

  if ( off_hand_attack && off_hand_attack->execute_event )
    event_t::cancel( off_hand_attack->execute_event );
}

// rogue_t::swap_weapon =====================================================

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

// rogue_t::stealthed =======================================================

bool rogue_t::stealthed( uint32_t stealth_mask ) const
{
  if ( ( stealth_mask & STEALTH_NORMAL ) && buffs.stealth->check() )
    return true;

  if ( ( stealth_mask & STEALTH_VANISH ) && buffs.vanish->check() )
    return true;

  if ( ( stealth_mask & STEALTH_SHADOWDANCE ) && buffs.shadow_dance->check() )
    return true;

  if ( ( stealth_mask & STEALTH_SUBTERFUGE ) && buffs.subterfuge->check() )
    return true;

  if ( ( stealth_mask & STEALTH_SHADOWMELD ) && player_t::buffs.shadowmeld->check() )
    return true;

  return false;
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
  if ( !sim->optimize_expressions && options.rogue_optimize_expressions )
  {
    for ( auto p : sim->player_list )
    {
      if ( !p->is_pet() && p->specialization() != ROGUE_ASSASSINATION && p->specialization() != ROGUE_OUTLAW && p->specialization() != ROGUE_SUBTLETY )
      {
        options.rogue_optimize_expressions = false;
        break;
      }
    }
    if ( options.rogue_optimize_expressions )
    {
      sim->optimize_expressions = true;
    }
  }

  player_t::combat_begin();

  if ( talent.hidden_blades -> ok() )
  {
    buffs.hidden_blades -> trigger( buffs.hidden_blades -> max_stack() );
    buffs.hidden_blades_driver -> trigger();
  }
}

// rogue_t::energy_regen_per_second =========================================

double rogue_t::resource_regen_per_second( resource_e r ) const
{
  double reg = player_t::resource_regen_per_second( r );

  // Slice and Dice does not have any energy effect in Shadowlands but keeping this commented out for now in case they change their minds.
  /*if ( r == RESOURCE_ENERGY )
  {
    if ( buffs.slice_and_dice->up() )
    {
      reg *= 1.0 + talent.slice_and_dice->effectN( 3 ).percent() * buffs.slice_and_dice->check_value();
    }
  }*/

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

  return temporary;
}

// rogue_t::passive_movement_modifier===================================

double rogue_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  ms += spell.fleet_footed -> effectN( 1 ).percent();
  ms += talent.hit_and_run -> effectN( 1 ).percent();

  if ( stealthed( (STEALTH_BASIC | STEALTH_SHADOWDANCE) ) ) // Check if nightstalker is temporary or passive.
  {
    ms += talent.nightstalker -> effectN( 1 ).percent();
  }

  return ms;
}

// rogue_t::regen ===========================================================

void rogue_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  // We handle some energy gain increases here instead of the resource_regen_per_second method in order to better track their benefits.
  // IMPORTANT NOTE: If anything is updated/added here, rogue_t::create_resource_expression() needs to be updated as well to reflect this
  if ( ! resources.is_infinite( RESOURCE_ENERGY ) )
  {
    if ( buffs.adrenaline_rush -> up() )
    {
      double energy_regen = periodicity.total_seconds() * resource_regen_per_second( RESOURCE_ENERGY ) * buffs.adrenaline_rush -> data().effectN( 1 ).percent();
      resource_gain( RESOURCE_ENERGY, energy_regen, gains.adrenaline_rush );
    }

    // Additional energy gains
    if ( buffs.nothing_personal -> up() )
      resource_gain( RESOURCE_ENERGY, buffs.nothing_personal -> check_value() * periodicity.total_seconds(), gains.nothing_personal );
    if ( buffs.buried_treasure -> up() )
      resource_gain( RESOURCE_ENERGY, buffs.buried_treasure -> check_value() * periodicity.total_seconds(), gains.buried_treasure );
    if ( buffs.vendetta -> up() )
      resource_gain( RESOURCE_ENERGY, buffs.vendetta -> check_value() * periodicity.total_seconds(), gains.vendetta );
  }
}

// rogue_t::resource_gain ===================================================

double rogue_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  // Memory of Lucid Dreams
  if ( resource_type == RESOURCE_ENERGY && player_t::buffs.memory_of_lucid_dreams->up() )
  {
    amount *= 1.0 + player_t::buffs.memory_of_lucid_dreams->data().effectN( 1 ).percent();
  }

  return player_t::resource_gain( resource_type, amount, source, action );
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

// rogue_t::vision_of_perfection_proc ========================================

void rogue_t::vision_of_perfection_proc()
{
  switch ( specialization() )
  {
    case ROGUE_ASSASSINATION:
    {
      rogue_td_t* td = this->get_target_data( this->target );
      const timespan_t duration = td->debuffs.vendetta->data().duration() * azerite.vision_of_perfection_percentage;
      if ( td->debuffs.vendetta->check() )
      {
        td->debuffs.vendetta->extend_duration( this, duration );
      }
      else
      {
        td->debuffs.vendetta->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
      }
      break;
    }

    case ROGUE_SUBTLETY:
    {
      const timespan_t duration = this->buffs.shadow_blades->data().duration() * azerite.vision_of_perfection_percentage;
      if ( this->buffs.shadow_blades->check() )
      {
        this->buffs.shadow_blades->extend_duration( this, duration );
      }
      else
      {
        this->buffs.shadow_blades->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
      }
      break;
    }
    case ROGUE_OUTLAW:
    {
      const timespan_t duration = this->buffs.adrenaline_rush->data().duration() * azerite.vision_of_perfection_percentage;
      if ( this->buffs.adrenaline_rush->check() )
      {
        this->buffs.adrenaline_rush->extend_duration( this, duration );
      }
      else
      {
        this->buffs.adrenaline_rush->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
      }
      break;
    }
	default:
		break;
  }
}

void rogue_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  action.apply_affecting_aura( spec.all_rogue );
  action.apply_affecting_aura( spec.assassination_rogue );
  action.apply_affecting_aura( spec.outlaw_rogue );
  action.apply_affecting_aura( spec.subtlety_rogue );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class rogue_report_t : public player_report_extension_t
{
public:
  rogue_report_t( rogue_t& player ) : p( player )
  {
  }

  void html_customsection( report::sc_html_stream& ) override
  {
    // Custom Class Section can be added here
    ( void )p;
  }
private:
  rogue_t& p;
};

// ROGUE MODULE INTERFACE ===================================================

class rogue_module_t : public module_t
{
public:
  rogue_module_t() : module_t( ROGUE ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new rogue_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new rogue_report_t( *p ) );
    return p;
  }

  bool valid() const override
  { return true; }

  void static_init() const override
  {
  }

  void register_hotfixes() const override
  {
  }

  void init( player_t* ) const override {}
  void combat_begin( sim_t* ) const override {}
  void combat_end( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::rogue()
{
  static rogue_module_t m;
  return &m;
}
