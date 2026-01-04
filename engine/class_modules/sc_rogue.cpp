// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "util/util.hpp"
#include "class_modules/apl/apl_rogue.hpp"

namespace { // UNNAMED NAMESPACE

// Forward Declarations
class rogue_t;

constexpr double COMBO_POINT_MAX = 5;

enum class secondary_trigger
{
  NONE = 0U,
  SINISTER_STRIKE,
  WEAPONMASTER,
  SECRET_TECHNIQUE,
  SECRET_TECHNIQUE_CLONE,
  SHURIKEN_TORNADO,
  INTERNAL_BLEEDING,
  MAIN_GAUCHE,
  DEATHMARK,
  HIDDEN_OPPORTUNITY,
  FAN_THE_HAMMER,
  COUP_DE_GRACE,
  HAND_OF_FATE,
};

enum stealth_type_e
{
  STEALTH_NORMAL = 0x01,
  STEALTH_VANISH = 0x02,
  STEALTH_SHADOWMELD = 0x04,
  STEALTH_SUBTERFUGE = 0x08,
  STEALTH_SHADOW_DANCE = 0x10,
  STEALTH_IMPROVED_GARROTE = 0x20,

  STEALTH_BASIC = ( STEALTH_NORMAL | STEALTH_VANISH ),            // Normal + Vanish
  STEALTH_ROGUE = ( STEALTH_SUBTERFUGE | STEALTH_SHADOW_DANCE ),  // Subterfuge + Shadowdance

  // All Stealth states that enable Stealth ability stance masks
  STEALTH_STANCE = ( STEALTH_BASIC | STEALTH_ROGUE | STEALTH_SHADOWMELD ),

  STEALTH_ALL = 0xFF
};

struct fatebound_t
{
  enum coinflip_e { HEADS, TAILS, EDGE };
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
  WEAPON_PRIMARY = 0U,
  WEAPON_SECONDARY
};

enum weapon_slot_e
{
  WEAPON_MAIN_HAND = 0U,
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

namespace buffs {
  struct rogue_buff_t : public buff_t
  {
    rogue_buff_t( player_t* p , util::string_view name, const spell_data_t* spell_data ) :
      buff_t( p->sim, p, p, name, spell_data, nullptr )
    {}
  };
}

// ==========================================================================
// Rogue Target Data
// ==========================================================================

class rogue_td_t : public actor_target_data_t
{
  std::vector<dot_t*> bleeds;
  std::vector<dot_t*> poison_dots;
  std::vector<buff_t*> poison_debuffs;

public:
  struct dots_t
  {
    dot_t* crimson_tempest;
    dot_t* deadly_poison;
    dot_t* deadly_poison_deathmark;
    dot_t* deathmark;
    dot_t* garrote;
    dot_t* garrote_deathmark;
    dot_t* internal_bleeding;
    dot_t* killing_spree; // Strictly speaking, this should probably be on player
    dot_t* kingsbane;
    dot_t* mutilated_flesh;
    dot_t* rupture;
    dot_t* rupture_deathmark;
    dot_t* soulrip;
  } dots;

  struct debuffs_t
  {
    buff_t* amplifying_poison;
    buff_t* amplifying_poison_deathmark;
    buff_t* atrophic_poison;
    buff_t* between_the_eyes;
    buff_t* caustic_spatter;
    buff_t* corrupt_the_blood;
    buff_t* crippling_poison;
    damage_buff_t* deathmark;
    buff_t* deathstalkers_mark;
    buff_t* fatal_intent;
    damage_buff_t* fazed;
    buff_t* find_weakness;
    buff_t* numbing_poison;
    buff_t* wound_poison;
  } debuffs;

  rogue_td_t( player_t* target, rogue_t* source );

  timespan_t lethal_poison_remains() const
  {
    if ( dots.deadly_poison->is_ticking() )
      return dots.deadly_poison->remains();

    if ( dots.deadly_poison_deathmark->is_ticking() )
      return dots.deadly_poison_deathmark->remains();

    if ( debuffs.wound_poison->check() )
      return debuffs.wound_poison->remains();

    if ( debuffs.amplifying_poison->check() )
      return debuffs.amplifying_poison->remains();

    if ( debuffs.amplifying_poison_deathmark->check() )
      return debuffs.amplifying_poison_deathmark->remains();

    return 0_s;
  }

  timespan_t non_lethal_poison_remains() const
  {
    if ( debuffs.atrophic_poison->check() )
      return debuffs.atrophic_poison->remains();

    if ( debuffs.crippling_poison->check() )
      return debuffs.crippling_poison->remains();

    if ( debuffs.numbing_poison->check() )
      return debuffs.numbing_poison->remains();

    return 0_s;
  }

  bool is_lethal_poisoned() const
  {
    return dots.deadly_poison->is_ticking() || dots.deadly_poison_deathmark->is_ticking() || 
           debuffs.amplifying_poison->check() || debuffs.amplifying_poison_deathmark->check() ||
           debuffs.wound_poison->check();
  }

  bool is_non_lethal_poisoned() const
  {
    return debuffs.atrophic_poison->check() || debuffs.crippling_poison->check() || debuffs.numbing_poison->check();
  }

  bool is_poisoned() const
  {
    return is_lethal_poisoned() || is_non_lethal_poisoned();
  }

  bool is_bleeding() const
  {
    for ( auto b : bleeds )
    {
      if ( b->is_ticking() )
        return true;
    }
    return false;
  }

  int total_bleeds() const
  {
    // TOCHECK -- Confirm all these things count as intended
    return as<int>( range::count_if( bleeds, []( dot_t* dot ) { return dot->is_ticking(); } ) );
  }

  int total_poisons() const
  {
    // TOCHECK -- Confirm all these things count as intended
    return as<int>( range::count_if( poison_dots, []( dot_t* d ) { return d->is_ticking(); } ) +
                    range::count_if( poison_debuffs, []( buff_t* b ) { return b->check(); } ) );
  }

  // Total count of active DoT effects that contribute towards the Lethal Dose talent
  int lethal_dose_count() const
  {
    return total_bleeds() + total_poisons();
  }
};

// ==========================================================================
// Rogue
// ==========================================================================

class rogue_t : public player_t
{
  // Ready trigger energy threshold
  double rogue_ready_trigger_threshold;

public:
  // Shadow Techniques swing counter;
  unsigned shadow_techniques_counter;

  // Danse Macabre Ability ID Tracker
  std::vector<unsigned> danse_macabre_tracker;

  buff_t* deathstalkers_mark_debuff;

  // Active
  struct
  {
    actions::rogue_poison_t* lethal_poison = nullptr;
    actions::rogue_poison_t* lethal_poison_dtb = nullptr;
    actions::rogue_poison_t* nonlethal_poison = nullptr;
    actions::rogue_poison_t* nonlethal_poison_dtb = nullptr;
    actions::rogue_attack_t* blade_flurry = nullptr;
    actions::rogue_attack_t* caustic_spatter = nullptr;
    actions::rogue_attack_t* echoing_reprimand = nullptr;
    actions::rogue_attack_t* fan_the_hammer = nullptr;
    actions::rogue_attack_t* internal_bleeding = nullptr;
    actions::rogue_attack_t* lingering_shadow = nullptr;
    actions::rogue_attack_t* main_gauche = nullptr;
    actions::rogue_attack_t* poison_bomb = nullptr;
    actions::shadow_blades_attack_t* shadow_blades_attack = nullptr;
    actions::rogue_spell_t* thistle_tea = nullptr;

    residual_action::residual_periodic_action_t<spell_t>* doomblade = nullptr;

    struct
    {
      actions::rogue_attack_t* backstab = nullptr;
      actions::rogue_attack_t* gloomblade = nullptr;
      actions::rogue_attack_t* shadowstrike = nullptr;
    } weaponmaster;
    struct
    {
      actions::rogue_attack_t* amplifying_poison = nullptr;
      actions::rogue_attack_t* deadly_poison_dot = nullptr;
      actions::rogue_attack_t* deadly_poison_instant = nullptr;
      actions::rogue_attack_t* garrote = nullptr;
      actions::rogue_attack_t* instant_poison = nullptr;
      actions::rogue_attack_t* rupture = nullptr;
      actions::rogue_attack_t* wound_poison = nullptr;
    } deathmark;
    struct
    {
      actions::rogue_attack_t* clear_the_witnesses = nullptr;
      actions::rogue_attack_t* clear_the_witnesses_tornado = nullptr;
      actions::rogue_attack_t* corrupt_the_blood = nullptr;
      actions::rogue_attack_t* deathstalkers_mark = nullptr;
      actions::rogue_attack_t* fatal_intent = nullptr;
      actions::rogue_attack_t* hunt_them_down = nullptr;
      actions::rogue_attack_t* singular_focus = nullptr;
    } deathstalker;
    struct
    {
      actions::rogue_attack_t* fatebound_coin_tails = nullptr;
      actions::rogue_attack_t* fate_intertwined = nullptr;
      actions::rogue_attack_t* lucky_coin = nullptr;
    } fatebound;
    struct
    {
      actions::rogue_attack_t* nimble_flurry = nullptr;
      actions::rogue_attack_t* unseen_blade = nullptr;
    } trickster;
    
    struct
    {
      actions::rogue_attack_t* ethereal_rampage = nullptr;
    } tww1;
  } active;

  // Autoattacks
  action_t* auto_attack;
  actions::melee_t* melee_main_hand;
  actions::melee_t* melee_off_hand;

  // Is using stealth during combat allowed? Relevant for Dungeon sims.
  bool restealth_allowed;

  // Experimental weapon swapping
  std::array<weapon_info_t, 2> weapon_data;

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
    // Outlaw
    buffs::rogue_buff_t* adrenaline_rush;
    buff_t* between_the_eyes;
    buffs::rogue_buff_t* blade_flurry;
    buff_t* blade_rush;
    buff_t* opportunity;
    buff_t* roll_the_bones;
    // Roll the bones buffs
    buff_t* one_of_a_kind;
    damage_buff_t* double_trouble;
    damage_buff_t* triple_threat;
    damage_buff_t* jackpot;
    // Subtlety
    buff_t* shadow_blades;
    damage_buff_t* shadow_dance;

    // Talents
    // Shared
    std::vector<buff_t*> supercharger;

    damage_buff_t* acrobatic_strikes;
    buff_t* subterfuge;
    buff_t* thistle_tea;
    buff_t* echoing_reprimand;

    // Hero
    // Deathstalker
    buff_t* clear_the_witnesses;
    damage_buff_t* deathstalkers_mark;
    buff_t* darkest_night;
    buff_t* lingering_darkness;
    damage_buff_t* momentum_of_despair;
    damage_buff_t* symbolic_victory;

    // Fatebound
    damage_buff_t* fatebound_coin_heads;
    buff_t* fatebound_coin_tails;
    stat_buff_t* fatebound_lucky_coin;

    // Trickster
    buff_t* cloud_cover;
    buff_t* disorienting_strikes;
    buff_t* escalating_blade;
    damage_buff_t* flawless_form;
    buff_t* unseen_blade_cd;
    
    // Assassination
    buff_t* blindside;
    buff_t* improved_garrote;
    buff_t* improved_garrote_aura;
    damage_buff_t* kingsbane;
    buff_t* scent_of_blood;

    // Outlaw
    buff_t* audacity;
    buff_t* killing_spree;
    buff_t* loaded_dice;
    buffs::rogue_buff_t* slice_and_dice;
    damage_buff_t* summarily_dispatched;

    // Subtlety
    damage_buff_t* danse_macabre;
    buff_t* goremaws_bite;
    buff_t* lingering_shadow;
    buff_t* master_of_shadows;
    damage_buff_t* perforated_veins;
    buff_t* perforated_veins_counter;
    buff_t* premeditation;
    buff_t* secret_technique;   // Only to simplify APL tracking
    buff_t* shadow_techniques;  // Internal tracking buff
    buff_t* shot_in_the_dark;
    buff_t* shuriken_tornado;
    buff_t* silent_storm;
    buff_t* the_first_dance;
    damage_buff_t* the_rotten;

    // Set Bonuses
    damage_buff_t* tww1_assassination_2pc;
    damage_buff_t* tww1_assassination_4pc;
    damage_buff_t* tww1_outlaw_4pc;
    damage_buff_t* tww1_subtlety_2pc;
    damage_buff_t* tww1_subtlety_4pc;

    damage_buff_t* tww2_assassination_2pc;
    damage_buff_t* tww2_assassination_4pc;
    damage_buff_t* tww2_outlaw_2pc;
    damage_buff_t* tww2_subtlety_2pc;

    damage_buff_t* tww3_deathstalker_2pc;
    buff_t* tww3_trickster_4pc;

  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* stealth;

    cooldown_t* adrenaline_rush;
    cooldown_t* between_the_eyes;
    cooldown_t* blade_flurry;
    cooldown_t* blade_rush;
    cooldown_t* blind;
    cooldown_t* cloak_of_shadows;
    cooldown_t* deathmark;
    cooldown_t* evasion;
    cooldown_t* feint;
    cooldown_t* garrote;
    cooldown_t* goremaws_bite;
    cooldown_t* gouge;
    cooldown_t* grappling_hook;
    cooldown_t* keep_it_rolling;
    cooldown_t* killing_spree;
    cooldown_t* kingsbane;
    cooldown_t* roll_the_bones;
    cooldown_t* secret_technique;
    cooldown_t* shadow_blades;
    cooldown_t* shadow_dance;
    cooldown_t* shadowstep;
    cooldown_t* shiv;
    cooldown_t* shuriken_tornado;
    cooldown_t* sprint;
    cooldown_t* thistle_tea;
    cooldown_t* unseen_blade_icd;
    cooldown_t* vanish;

    target_specific_cooldown_t* weaponmaster;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* adrenaline_rush;
    gain_t* adrenaline_rush_expiry;
    gain_t* blade_rush;
    gain_t* buried_treasure;
    gain_t* darkest_night;
    gain_t* dashing_scoundrel;
    gain_t* fatal_flourish;
    gain_t* energy_refund;
    gain_t* master_of_shadows;
    gain_t* venomous_wounds;
    gain_t* venomous_wounds_death;
    gain_t* relentless_strikes;
    gain_t* slice_and_dice;

    // CP Gains
    gain_t* ace_up_your_sleeve;
    gain_t* improved_adrenaline_rush;
    gain_t* improved_adrenaline_rush_expiry;
    gain_t* improved_ambush;
    gain_t* killing_spree;
    gain_t* premeditation;
    gain_t* quick_draw;
    gain_t* roll_the_bones;
    gain_t* ruthlessness;
    gain_t* seal_fate;
    gain_t* shadow_techniques;
    gain_t* shadow_techniques_shadowcraft;
    gain_t* shadow_blades;
    gain_t* shrouded_suffocation;
    gain_t* deal_fate;

  } gains;

  // Spell Data
  struct spells_t
  {
    // Core Class Spells
    const spell_data_t* ambush;
    const spell_data_t* cheap_shot;
    const spell_data_t* crimson_vial;           // No implementation
    const spell_data_t* crippling_poison;
    const spell_data_t* detection;
    const spell_data_t* distract;
    const spell_data_t* echoing_reprimand_buff;
    const spell_data_t* echoing_reprimand_damage;
    const spell_data_t* instant_poison;
    const spell_data_t* kick;
    const spell_data_t* kidney_shot;
    const spell_data_t* shadowstep;
    const spell_data_t* slice_and_dice;
    const spell_data_t* sprint;
    const spell_data_t* stealth;
    const spell_data_t* thistle_tea;
    const spell_data_t* vanish;
    const spell_data_t* wound_poison;
    const spell_data_t* feint;
    const spell_data_t* sap;

    // Class Passives
    const spell_data_t* all_rogue;
    const spell_data_t* critical_strikes;
    const spell_data_t* cut_to_the_chase;
    const spell_data_t* fleet_footed;           // DFALPHA: Duplicate passive?
    const spell_data_t* leather_specialization;

    // Background Spells
    const spell_data_t* leeching_poison_buff;
    const spell_data_t* recuperator_heal;
    const spell_data_t* shadowstep_buff;
    const spell_data_t* subterfuge_buff;
    const spell_data_t* thistle_tea_buff;
    const spell_data_t* vanish_buff;

    // Hero Spells
    const spell_data_t* clear_the_witnesses_buff;
    const spell_data_t* clear_the_witnesses_damage;
    const spell_data_t* cloud_cover_distract;
    const spell_data_t* corrupt_the_blood_damage;
    const spell_data_t* coup_de_grace;
    const spell_data_t* coup_de_grace_damage_1;
    const spell_data_t* coup_de_grace_damage_2;
    const spell_data_t* coup_de_grace_damage_3;
    const spell_data_t* coup_de_grace_damage_bonus_1;
    const spell_data_t* coup_de_grace_damage_bonus_2;
    const spell_data_t* coup_de_grace_damage_bonus_3;
    const spell_data_t* darkest_night_buff;
    const spell_data_t* deathstalkers_mark_buff;
    const spell_data_t* deathstalkers_mark_damage;
    const spell_data_t* deathstalkers_mark_debuff;
    const spell_data_t* escalating_blade_buff;
    const spell_data_t* fatal_intent_damage;
    const spell_data_t* fatal_intent_debuff;
    const spell_data_t* fatebound_coin_heads_buff;
    const spell_data_t* fatebound_coin_heads_initial_buff;
    const spell_data_t* fatebound_coin_tails_buff;
    const spell_data_t* fatebound_coin_tails;
    const spell_data_t* fatebound_lucky_coin_buff;
    const spell_data_t* fatebound_lucky_coin_damage;
    const spell_data_t* fatebound_fate_intertwined;
    const spell_data_t* fazed_debuff;
    const spell_data_t* flawless_form_buff;
    const spell_data_t* hunt_them_down_damage;
    const spell_data_t* lingering_darkness_buff;
    const spell_data_t* momentum_of_despair_buff;
    const spell_data_t* nimble_flurry_damage;
    const spell_data_t* singular_focus_damage;
    const spell_data_t* symbolic_victory_buff;
    const spell_data_t* unseen_blade;
    const spell_data_t* unseen_blade_buff;

  } spell;

  // Spec Spell Data
  struct spec_t
  {
    // Assassination Spells
    const spell_data_t* assassination_rogue;
    const spell_data_t* envenom;
    const spell_data_t* fan_of_knives;
    const spell_data_t* garrote;
    const spell_data_t* mutilate;
    const spell_data_t* poisoned_knife;

    const spell_data_t* amplifying_poison_debuff;
    const spell_data_t* blindside_buff;
    const spell_data_t* caustic_spatter_buff;
    const spell_data_t* caustic_spatter_damage;
    double dashing_scoundrel_gain = 0.0;
    const spell_data_t* deadly_poison_instant;
    const spell_data_t* improved_garrote_buff;
    const spell_data_t* internal_bleeding_debuff;
    const spell_data_t* kingsbane_buff;
    const spell_data_t* poison_bomb_driver;
    const spell_data_t* poison_bomb_damage;
    const spell_data_t* scent_of_blood_buff;
    const spell_data_t* zoldyck_insignia;

    const spell_data_t* deathmark_debuff;
    const spell_data_t* deathmark_amplifying_poison;
    const spell_data_t* deathmark_deadly_poison_dot;
    const spell_data_t* deathmark_deadly_poison_instant;
    const spell_data_t* deathmark_garrote;
    const spell_data_t* deathmark_instant_poison;
    const spell_data_t* deathmark_rupture;
    const spell_data_t* deathmark_wound_poison;

    // Outlaw Spells
    const spell_data_t* outlaw_rogue;
    const spell_data_t* between_the_eyes;
    const spell_data_t* dispatch;
    const spell_data_t* pistol_shot;
    const spell_data_t* sinister_strike;
    const spell_data_t* roll_the_bones;
    const spell_data_t* restless_blades;
    const spell_data_t* blade_flurry;

    const spell_data_t* acrobatic_strikes_buff;
    const spell_data_t* audacity_buff;
    const spell_data_t* blade_flurry_attack;
    const spell_data_t* blade_flurry_instant_attack;
    const spell_data_t* blade_rush_attack;
    const spell_data_t* blade_rush_energize;
    const spell_data_t* doomblade_debuff;
    const spell_data_t* hidden_opportunity_extra_attack;
    const spell_data_t* improved_adrenaline_rush_energize;
    const spell_data_t* killing_spree_mh_attack;
    const spell_data_t* killing_spree_oh_attack;
    const spell_data_t* killing_spree_energize;
    const spell_data_t* opportunity_buff;
    const spell_data_t* sinister_strike_extra_attack;
    const spell_data_t* summarily_dispatched_buff;
    const spell_data_t* ace_up_your_sleeve_energize;

    const spell_data_t* one_of_a_kind;
    const spell_data_t* double_trouble;
    const spell_data_t* triple_threat;
    const spell_data_t* jackpot;

    // Subtlety Spells
    const spell_data_t* subtlety_rogue;
    const spell_data_t* backstab;
    const spell_data_t* black_powder;
    const spell_data_t* eviscerate;
    const spell_data_t* secret_technique;
    const spell_data_t* shadow_dance;         // Baseline charge increase passive
    const spell_data_t* shadow_techniques;
    const spell_data_t* shadowstrike;
    const spell_data_t* shuriken_storm;    
    const spell_data_t* shuriken_toss;

    const spell_data_t* black_powder_shadow_attack;
    const spell_data_t* danse_macabre_buff;
    const spell_data_t* eviscerate_shadow_attack;
    const spell_data_t* find_weakness_debuff;
    const spell_data_t* goremaws_bite_buff;
    const spell_data_t* master_of_shadows_buff;
    const spell_data_t* perforated_veins_buff;
    const spell_data_t* perforated_veins_counter;
    const spell_data_t* premeditation_buff;
    const spell_data_t* relentless_strikes_energize;
    const spell_data_t* replicating_shadows_tick;
    const spell_data_t* lingering_shadow_attack;
    const spell_data_t* lingering_shadow_buff;
    const spell_data_t* secret_technique_attack;
    const spell_data_t* secret_technique_clone_attack;
    const spell_data_t* shadow_blades_attack;
    const spell_data_t* shadow_focus_buff;
    const spell_data_t* shadow_techniques_energize;
    const spell_data_t* shadowstrike_stealth_buff;
    const spell_data_t* shot_in_the_dark_buff;
    const spell_data_t* silent_storm_buff;
    const spell_data_t* the_first_dance_buff;

    // Multi-Spec
    const spell_data_t* rupture;      // Assassination + Subtlety
    const spell_data_t* shadowstep;   // Assassination + Subtlety, baseline charge increase passive

    // Set Bonuses
    const spell_data_t* tww1_assassination_2pc_buff;
    const spell_data_t* tww1_assassination_4pc_buff;
    const spell_data_t* tww1_outlaw_2pc_spell;
    const spell_data_t* tww1_outlaw_4pc_buff;
    const spell_data_t* tww2_assassination_4pc_buff;
    const spell_data_t* tww3_deathstalker_2pc_buff;

  } spec;

  // Talents
  struct talents_t
  {
    struct class_talents_t
    {
      player_talent_t shiv;
      player_talent_t blind;                    // No implementation
      player_talent_t cloak_of_shadows;         // No implementation

      player_talent_t evasion;                  // No implementation
      player_talent_t gouge;
      player_talent_t airborne_irritant;        // No implementation
      player_talent_t thrill_seeking;

      player_talent_t master_poisoner;          // No implementation
      player_talent_t elusiveness;              // No implementation
      player_talent_t cheat_death;              // No implementation
      player_talent_t blackjack;                // No implementation
      player_talent_t tricks_of_the_trade;      // No implementation

      player_talent_t improved_wound_poison;
      player_talent_t nimble_fingers;
      player_talent_t improved_sprint;
      player_talent_t shadowrunner;
      
      player_talent_t superior_mixture;         // No implementation
      player_talent_t fleet_footed;             // No implementation
      player_talent_t iron_stomach;             // No implementation
      player_talent_t unbreakable_stride;       // No implementation
      player_talent_t featherfoot;

      player_talent_t numbing_poison;
      player_talent_t atrophic_poison;
      player_talent_t deadened_nerves;          // No implementation
      player_talent_t graceful_guile;           // No implementation
      player_talent_t stillshroud;              // No implementation

      player_talent_t deadly_precision;
      player_talent_t virulent_poisons;
      player_talent_t improved_ambush;
      player_talent_t tight_spender;

      player_talent_t swift_slasher;

      player_talent_t leeching_poison;
      player_talent_t lethality;
      player_talent_t recuperator;
      player_talent_t alacrity;
      player_talent_t soothing_darkness;        // No implementation

      player_talent_t vigor;
      player_talent_t supercharger;
      player_talent_t subterfuge;

      player_talent_t thistle_tea;
      player_talent_t echoing_reprimand;
      player_talent_t forced_induction;
      player_talent_t deeper_stratagem;
      player_talent_t without_a_trace;
    } rogue;

    struct assassination_talents_t
    {
      player_talent_t deadly_poison;

      player_talent_t venomous_wounds;
      player_talent_t path_of_blood;

      player_talent_t rapid_injection;
      player_talent_t improved_poisons;
      player_talent_t bloody_mess;

      player_talent_t thrown_precision;
      player_talent_t seal_fate;
      player_talent_t caustic_spatter;
      player_talent_t internal_bleeding;
      player_talent_t improved_garrote;

      player_talent_t crimson_tempest;
      player_talent_t deathmark;

      player_talent_t flying_daggers;
      player_talent_t sanguine_stratagem;
      player_talent_t fatal_concoction;
      player_talent_t lethal_dose;
      player_talent_t intent_to_kill;
      player_talent_t iron_wire;                // No implementation

      player_talent_t systemic_failure;
      player_talent_t amplifying_poison;
      player_talent_t doomblade;

      player_talent_t blindside;
      player_talent_t dashing_scoundrel;
      player_talent_t shrouded_suffocation;

      player_talent_t zoldyck_recipe;
      player_talent_t poison_bomb;
      player_talent_t scent_of_blood;

      player_talent_t kingsbane;
      player_talent_t dragon_tempered_blades;
      player_talent_t sudden_demise;            // Partial NYI for "execute" mechanic
      
    } assassination;

    struct outlaw_talents_t
    {
      player_talent_t opportunity;

      player_talent_t adrenaline_rush;

      player_talent_t retractable_hook;
      player_talent_t combat_potency;
      player_talent_t combat_stamina;
      player_talent_t hit_and_run;

      player_talent_t blinding_powder;
      player_talent_t precision_shot;

      player_talent_t heavy_hitter;
      player_talent_t devious_stratagem;
      player_talent_t killing_spree;
      player_talent_t fatal_flourish;
      player_talent_t quick_draw;
      player_talent_t deft_maneuvers;

      player_talent_t acrobatic_strikes;

      player_talent_t ruthlessness;
      player_talent_t loaded_dice;
      player_talent_t sleight_of_hand;
      player_talent_t thiefs_versatility;
      player_talent_t improved_between_the_eyes;

      player_talent_t audacity;
      player_talent_t improved_adrenaline_rush;
      player_talent_t dancing_steel;

      player_talent_t ace_up_your_sleeve;
      player_talent_t blade_rush;

      player_talent_t summarily_dispatched;
      player_talent_t fan_the_hammer;

      player_talent_t hidden_opportunity;
      player_talent_t keep_it_rolling;

    } outlaw;

    struct subtlety_talents_t
    {
      player_talent_t find_weakness;

      player_talent_t improved_backstab;
      player_talent_t shadow_blades;
      player_talent_t improved_shuriken_storm;

      player_talent_t shot_in_the_dark;
      player_talent_t quick_decisions;
      player_talent_t ephemeral_bond;           // No implementation
      player_talent_t exhilarating_execution;   // No implementation

      player_talent_t shrouded_in_darkness;     // No implementation
      player_talent_t shadow_focus;
      player_talent_t fade_to_nothing;          // No implementation
      player_talent_t cloaked_in_shadow;        // No implementation
      player_talent_t night_terrors;            // No implementation
      player_talent_t terrifying_pace;          // No implementation

      player_talent_t gloomblade;
      player_talent_t relentless_strikes;
      player_talent_t silent_storm;

      player_talent_t premeditation;
      player_talent_t planned_execution;
      player_talent_t warning_signs;
      player_talent_t double_dance;
      player_talent_t shadowed_finishers;
      player_talent_t secret_stratagem;
      player_talent_t replicating_shadows;

      player_talent_t weaponmaster;
      player_talent_t the_first_dance;
      player_talent_t master_of_shadows;
      player_talent_t deepening_shadows;
      player_talent_t veiltouched;
      player_talent_t shuriken_tornado;

      player_talent_t perforated_veins;
      player_talent_t lingering_shadow;
      player_talent_t deeper_daggers;

      player_talent_t death_perception;
      player_talent_t dark_shadow;
      player_talent_t finality;

      player_talent_t the_rotten;
      player_talent_t shadowcraft;              // Partial NYI, TODO finisher interaction
      player_talent_t danse_macabre;
      player_talent_t goremaws_bite;
      player_talent_t dark_brew;

    } subtlety;

    struct deathstalker_talents_t
    {
      player_talent_t deathstalkers_mark;

      player_talent_t clear_the_witnesses;
      player_talent_t hunt_them_down;
      player_talent_t singular_focus;

      player_talent_t fatal_intent;
      player_talent_t corrupt_the_blood;
      player_talent_t lingering_darkness;
      player_talent_t symbolic_victory;

      player_talent_t ethereal_cloak;       // No implementation
      player_talent_t bait_and_switch;      // No implementation
      player_talent_t momentum_of_despair;
      player_talent_t follow_the_blood;
      player_talent_t shadewalker;
      player_talent_t shroud_of_night;      // No implementation

      player_talent_t darkest_night;

    } deathstalker;

    struct fatebound_talents_t
    {
      player_talent_t hand_of_fate;

      player_talent_t chosens_revelry;
      player_talent_t tempted_fate;     // TODO: Defensive
      player_talent_t mean_streak;
      player_talent_t inexorable_march; // No implementation
      player_talent_t deaths_arrival;   // NYI in-game

      player_talent_t deal_fate;
      player_talent_t edge_case;        // TODO: Double jeopardy + edge case stealth break overlap bug
      player_talent_t fate_intertwined;

      player_talent_t delivered_doom;
      player_talent_t destiny_defined;  // TODO: Outlaw also gets the poison proc rate the text says is for assa? Verify in-game.

    } fatebound;

    struct trickster_talents_t
    {
      player_talent_t unseen_blade;

      player_talent_t surprising_strikes; 
      player_talent_t smoke;              // No implementation
      player_talent_t mirrors;            // No implementation
      player_talent_t flawless_form;

      player_talent_t so_tricky;          // No implementation
      player_talent_t dont_be_suspicious;
      player_talent_t devious_distractions;
      player_talent_t thousand_cuts;
      player_talent_t flickerstrike;      // TODO: Add time-based trigger opt

      player_talent_t disorienting_strikes;
      player_talent_t cloud_cover;
      player_talent_t no_scruples;
      player_talent_t nimble_flurry;

      player_talent_t coup_de_grace;

    } trickster;

  } talent;

  // Masteries
  struct masteries_t
  {
    // Assassination
    const spell_data_t* potent_assassin;
    // Outlaw
    const spell_data_t* main_gauche;
    const spell_data_t* main_gauche_attack;
    // Subtlety
    const spell_data_t* executioner;
  } mastery;

  // Legendary effects
  struct legendary_t
  {
  } legendary;

  // Procs
  struct procs_t
  {
    // Shared
    proc_t* supercharger_wasted;

    // Assassination
    proc_t* amplifying_poison_consumed;
    proc_t* amplifying_poison_deathmark_consumed;

    // Subtlety
    proc_t* deepening_shadows;
    proc_t* weaponmaster;

    // Set Bonus
    proc_t* tww2_subtlety_4pc;

  } procs;

  // Set Bonus effects
  struct set_bonuses_t
  {
    const spell_data_t* tww1_assassination_2pc;
    const spell_data_t* tww1_assassination_4pc;
    const spell_data_t* tww1_outlaw_2pc;
    const spell_data_t* tww1_outlaw_4pc;
    const spell_data_t* tww1_subtlety_2pc;
    const spell_data_t* tww1_subtlety_4pc;

    const spell_data_t* tww2_assassination_2pc;
    const spell_data_t* tww2_assassination_4pc;
    const spell_data_t* tww2_outlaw_2pc;
    const spell_data_t* tww2_outlaw_4pc;
    const spell_data_t* tww2_subtlety_2pc;
    const spell_data_t* tww2_subtlety_4pc;

    const spell_data_t* tww3_deathstalker_2pc;
    const spell_data_t* tww3_deathstalker_4pc;
    const spell_data_t* tww3_fatebound_2pc;
    const spell_data_t* tww3_fatebound_4pc;
    const spell_data_t* tww3_trickster_2pc;
    const spell_data_t* tww3_trickster_4pc;

  } set_bonuses;

  // Options
  struct rogue_options_t
  {
    int fixed_rtb;
    std::vector<double> fixed_rtb_odds;
    int initial_combo_points = 0;
    int initial_shadow_techniques = -1;
    int initial_supercharged_cp = 0;
    bool rogue_ready_trigger = true;
    bool priority_rotation = false;
  } options;

  rogue_t( sim_t* sim, util::string_view name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, ROGUE, name, r ),
    rogue_ready_trigger_threshold( 25 ),
    shadow_techniques_counter( 0 ),
    deathstalkers_mark_debuff( nullptr ),
    auto_attack( nullptr ), melee_main_hand( nullptr ), melee_off_hand( nullptr ),
    restealth_allowed( false ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    spell( spells_t() ),
    spec( spec_t() ),
    talent( talents_t() ),
    mastery( masteries_t() ),
    legendary( legendary_t() ),
    procs( procs_t() ),
    set_bonuses( set_bonuses_t() ),
    options( rogue_options_t() )
  {
    // Cooldowns
    cooldowns.stealth                   = get_cooldown( "stealth" );

    cooldowns.adrenaline_rush           = get_cooldown( "adrenaline_rush" );
    cooldowns.between_the_eyes          = get_cooldown( "between_the_eyes" );
    cooldowns.blade_flurry              = get_cooldown( "blade_flurry" );
    cooldowns.blade_rush                = get_cooldown( "blade_rush" );
    cooldowns.blind                     = get_cooldown( "blind" );
    cooldowns.cloak_of_shadows          = get_cooldown( "cloak_of_shadows" );
    cooldowns.deathmark                 = get_cooldown( "deathmark" );
    cooldowns.evasion                   = get_cooldown( "evasion" );
    cooldowns.feint                     = get_cooldown( "feint" );
    cooldowns.garrote                   = get_cooldown( "garrote" );
    cooldowns.goremaws_bite             = get_cooldown( "goremaws_bite" );
    cooldowns.gouge                     = get_cooldown( "gouge" );
    cooldowns.grappling_hook            = get_cooldown( "grappling_hook" );
    cooldowns.keep_it_rolling           = get_cooldown( "keep_it_rolling" );
    cooldowns.killing_spree             = get_cooldown( "killing_spree" );
    cooldowns.kingsbane                 = get_cooldown( "kingsbane" );
    cooldowns.roll_the_bones            = get_cooldown( "roll_the_bones" );
    cooldowns.secret_technique          = get_cooldown( "secret_technique" );
    cooldowns.shadow_blades             = get_cooldown( "shadow_blades" );
    cooldowns.shadow_dance              = get_cooldown( "shadow_dance" );
    cooldowns.shadowstep                = get_cooldown( "shadowstep" );
    cooldowns.shiv                      = get_cooldown( "shiv" );
    cooldowns.shuriken_tornado          = get_cooldown( "shuriken_tornado" );
    cooldowns.sprint                    = get_cooldown( "sprint" );   
    cooldowns.thistle_tea               = get_cooldown( "thistle_tea" );
    cooldowns.unseen_blade_icd          = get_cooldown( "unseen_blade_icd" );
    cooldowns.vanish                    = get_cooldown( "vanish" );

    cooldowns.weaponmaster              = get_target_specific_cooldown( "weaponmaster" );

    resource_regeneration = regen_type::DYNAMIC;
    regen_caches[CACHE_HASTE] = true;
    regen_caches[CACHE_ATTACK_HASTE] = true;
  }

  // Character Definition
  void        init_spells() override;
  void        init_base_stats() override;
  void        init_talents() override;
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
  void        init_blizzard_action_list() override;
  std::vector<std::string> action_names_from_spell_id( unsigned int spell_id ) const override;
  parsed_assisted_combat_rule_t parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                            const assisted_combat_step_data_t& step ) const override;
  void        reset() override;
  void        activate() override;
  void        arise() override;
  void        combat_begin() override;
  timespan_t  available() const override;
  action_t*   create_action( util::string_view name, util::string_view options ) override;
  std::unique_ptr<expr_t> create_action_expression( action_t& action, std::string_view name_str ) override;
  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override;
  std::unique_ptr<expr_t> create_resource_expression( util::string_view name ) override;
  void        regen( timespan_t periodicity ) override;
  resource_e  primary_resource() const override { return RESOURCE_ENERGY; }
  role_e      primary_role() const override  { return ROLE_ATTACK; }
  stat_e      convert_hybrid_stat( stat_e s ) const override;

  // Default consumables
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;

  double    composite_attribute_multiplier( attribute_e attr ) const override;
  double    composite_melee_auto_attack_speed() const override;
  double    composite_melee_haste() const override;
  double    composite_melee_crit_chance() const override;
  double    composite_spell_crit_chance() const override;
  double    composite_spell_haste() const override;
  double    composite_damage_versatility() const override;
  double    composite_heal_versatility() const override;
  double    composite_leech() const override;
  double    matching_gear_multiplier( attribute_e attr ) const override;
  double    composite_player_multiplier( school_e school ) const override;
  double    composite_player_pet_damage_multiplier( const action_state_t*, bool ) const override;
  double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double    composite_player_target_crit_chance( player_t* target ) const override;
  double    composite_player_target_armor( player_t* target ) const override;
  double    resource_regen_per_second( resource_e ) const override;
  double    non_stacking_movement_modifier() const override;
  double    stacking_movement_modifier() const override;
  void      invalidate_cache( cache_e ) override;

  void break_stealth();
  void cancel_auto_attacks() override;
  void do_exsanguinate( dot_t* dot, double coeff );

  void trigger_venomous_wounds_death( player_t* ); // On-death trigger for Venomous Wounds energy replenish

  double consume_cp_max() const
  {
    return COMBO_POINT_MAX + as<double>( talent.rogue.deeper_stratagem->effectN( 2 ).base_value() +
                                         talent.assassination.sanguine_stratagem->effectN( 2 ).base_value() +
                                         talent.outlaw.devious_stratagem->effectN( 2 ).base_value() +
                                         talent.subtlety.secret_stratagem->effectN( 2 ).base_value() );
  }

  double current_cp( bool /* react */ = false ) const
  {
    return resources.current[ RESOURCE_COMBO_POINT ];
  }

  // Current number of effective combo points, considering Supercharger and Escalating Blade
  double current_effective_cp( bool use_supercharger = true, bool react = false ) const
  {
    double current_cp = std::min( this->current_cp( react ), consume_cp_max() );

    if ( use_supercharger && current_cp > 0 )
    {
      if ( range::any_of( buffs.supercharger, []( const buff_t* buff ) { return buff->check(); } ) )
        current_cp += talent.rogue.supercharger->effectN( 2 ).base_value() + talent.rogue.forced_induction->effectN( 1 ).base_value();
    }

    return current_cp;
  }

  // Extra attack proc chance for Outlaw mechanics (Sinister Strike, Audacity, Hidden Opportunity)
  double extra_attack_proc_chance() const
  {
    double proc_chance = spec.sinister_strike->effectN( 3 ).percent();
    if ( buffs.roll_the_bones->check() )
      proc_chance += spec.jackpot->effectN( 1 ).percent();
    if ( talent.fatebound.destiny_defined->ok() )
      proc_chance += talent.fatebound.destiny_defined->effectN( 2 ).percent();
    return proc_chance;
  }

  target_specific_t<rogue_td_t> target_data;

  const rogue_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

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
  T* find_background_action( util::string_view n = {} )
  {
    T* found_action = nullptr;
    for ( auto action : background_actions )
    {
      found_action = dynamic_cast<T*>( action );
      if ( found_action )
      {
        if ( n.empty() || found_action->name_str == n )
          break;
        else
          found_action = nullptr;
      }
    }
    return found_action;
  }

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
  T* find_secondary_trigger_action( secondary_trigger source, util::string_view n = {} )
  {
    T* found_action = nullptr;
    for ( auto action : secondary_trigger_actions )
    {
      found_action = dynamic_cast<T*>( action );
      if ( found_action )
      {
        if ( found_action->secondary_trigger_type == source && ( n.empty() || found_action->name_str == n ) )
          break;
        else
          found_action = nullptr;
      }
    }
    return found_action;
  }

  template <typename T, typename... Ts>
  T* get_secondary_trigger_action( secondary_trigger source, util::string_view n, Ts&&... args )
  {
    T* found_action = find_secondary_trigger_action<T>( source, n );
    if ( !found_action )
    {
      // Actions will follow form of foo_t( util::string_view name, rogue_t* p, ... )
      found_action = new T( n, this, std::forward<Ts>( args )... );
      found_action->background = found_action->dual = true;
      found_action->repeating = false;
      found_action->secondary_trigger_type = source;
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
      state->target = action_target;
      action->cast_state( state )->set_combo_points( cp, cp );
      // Calling snapshot_internal, snapshot_state would overwrite CP.
      action->snapshot_internal( state, action->snapshot_flags, action->amount_type( state ) );
    }
    
    assert( !action->pre_execute_state );

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

template<typename T_ACTION>
struct rogue_action_state_t : public action_state_t
{
private:
  T_ACTION* action;
  int base_cp;
  int total_cp;
  double exsanguinated_rate;
  bool exsanguinated;

public:
  rogue_action_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ),
    action( dynamic_cast<T_ACTION*>( action ) ),
    base_cp( 0 ), total_cp( 0 ), exsanguinated_rate( 1.0 ), exsanguinated( false )
  {}

  void initialize() override
  {
    action_state_t::initialize();
    base_cp = 0;
    total_cp = 0;
    exsanguinated_rate = 1.0;
    exsanguinated = false;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s ) << " base_cp=" << base_cp << " total_cp=" << total_cp << " exsanguinated_rate=" << exsanguinated_rate;
    return s;
  }

  void copy_state( const action_state_t* s, bool copy_exsang )
  {
    action_state_t::copy_state( s );
    const rogue_action_state_t* rs = debug_cast<const rogue_action_state_t*>( s );
    base_cp = rs->base_cp;
    total_cp = rs->total_cp;
    if ( copy_exsang )
    {
      exsanguinated_rate = rs->exsanguinated_rate;
      exsanguinated = rs->exsanguinated;
    }
    else
    {
      exsanguinated_rate = 1.0;
      exsanguinated = false;
    }
  }

  void copy_state( const action_state_t* s ) override
  {
    // Default copy behavior to not copy Exsanguianted DoT state
    this->copy_state( s, false );
  }

  T_ACTION* get_action() const
  {
    return action;
  }

  void set_combo_points( int base_cp, int total_cp )
  {
    this->base_cp = base_cp;
    this->total_cp = total_cp;
  }

  int get_combo_points( bool base_only = false ) const
  {
    if ( base_only )
      return base_cp;

    return total_cp;
  }

  void set_exsanguinated_rate( double rate )
  {
    exsanguinated_rate = rate;
    exsanguinated = true;
  }

  double get_exsanguinated_rate() const
  {
    return exsanguinated_rate;
  }

  double is_exsanguinated() const
  {
    return exsanguinated;
  }

  void clear_exsanguinated()
  {
    exsanguinated_rate = 1.0;
    exsanguinated = false;
  }

  proc_types2 cast_proc_type2() const override
  {
    if( action->secondary_trigger_type == secondary_trigger::WEAPONMASTER )
    {
      return PROC2_CAST_DAMAGE;
    }

    return action_state_t::cast_proc_type2();
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
  bool _breaks_stealth;

public:
  // Secondary triggered ability, due to Weaponmaster talent or Death from Above. Secondary
  // triggered abilities cost no resources or incur cooldowns.
  secondary_trigger secondary_trigger_type;

  proc_t* supercharged_cp_proc;
  proc_t* perforated_veins_consumed_proc;

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
    bool adrenaline_rush_gcd = false;
    bool audacity = false;              // Stance Mask
    bool blindside = false;             // Stance Mask
    bool danse_macabre = false;         // Trigger
    bool darkest_night = false;         // Damage
    bool darkest_night_crit = false;    // Crit%
    bool dashing_scoundrel = false;
    bool deathmark = false;             // Tuning Aura
    bool deepening_shadows = false;     // Trigger
    bool fazed_damage = false;
    bool fazed_crit_chance = false;
    bool fazed_crit_damage = false;
    bool goremaws_bite = false;         // Cost Reduction
    bool improved_ambush = false;
    bool lethal_dose = false;
    bool maim_mangle = false;           // Renamed Systemic Failure for DF talent
    bool momentum_of_despair = false;   // Crit Damage Multiplier
    bool planned_execution = false;     // Crit Damage Multiplier
    bool relentless_strikes = false;    // Trigger
    bool roll_the_bones_cp = false;
    bool ruthlessness = false;          // Trigger
    bool shadow_blades_cp = false;
    bool zoldyck_insignia = false;

    bool tww3_fatebound_4pc = false;

    damage_affect_data follow_the_blood;
    damage_affect_data mastery_executioner;
    damage_affect_data mastery_potent_assassin;
    damage_affect_data tww2_subtlety_4pc;
  } affected_by;

  std::vector<damage_buff_t*> direct_damage_buffs;
  std::vector<damage_buff_t*> periodic_damage_buffs;
  std::vector<damage_buff_t*> auto_attack_damage_buffs;
  std::vector<damage_buff_t*> crit_chance_buffs;
  
  struct consume_buff_t
  {
    buff_t* buff = nullptr;
    proc_t* proc = nullptr;
    timespan_t delay = timespan_t::zero();
    bool on_background = false;
    bool decrement = false;
  };
  std::vector<consume_buff_t> consume_buffs;

  // Init =====================================================================

  rogue_action_t( util::string_view n, rogue_t* p, const spell_data_t* s = spell_data_t::nil(),
                  util::string_view options = {} )
    : ab( n, p, s ),
    _requires_stealth( false ),
    _breaks_stealth( true ),
    secondary_trigger_type( secondary_trigger::NONE ),
    supercharged_cp_proc( nullptr ),
    perforated_veins_consumed_proc( nullptr )
  {
    ab::parse_options( options );
    parse_spell_data( s );

    // rogue_t sets base and min GCD to 1s by default and action_t sets 1s gcd attacks to non-hasted regardless of
    // ability flags. There should no longer be a need to explicitly enforced non-hasted GCDs.
    // ab::gcd_type = gcd_haste_type::NONE;

    // Dynamically affected flags
    // Special things like CP, Energy, Crit, etc.
    affected_by.improved_ambush = ab::data().affected_by( p->talent.rogue.improved_ambush->effectN( 1 ) );

    // Hero Talents
    if ( p->talent.deathstalker.momentum_of_despair->ok() )
    {
      affected_by.momentum_of_despair = ab::data().affected_by( p->spell.momentum_of_despair_buff->effectN( 2 ) );
    }

    if ( p->talent.trickster.unseen_blade->ok() )
    {
      affected_by.fazed_damage = ab::data().affected_by( p->spell.fazed_debuff->effectN( 1 ) );
      affected_by.fazed_crit_damage = ab::data().affected_by( p->spell.fazed_debuff->effectN( 4 ) );
      affected_by.fazed_crit_chance = ab::data().affected_by( p->spell.fazed_debuff->effectN( 5 ) );
    }

    if ( p->set_bonuses.tww3_fatebound_4pc->ok() && p->spell.fatebound_lucky_coin_buff->ok() )
    {
      // Cooldown whitelists are in the Lucky Coin buff and modified by the set bonus
      affected_by.tww3_fatebound_4pc = ( ab::data().affected_by( p->spell.fatebound_lucky_coin_buff->effectN( 2 ) ) ||
                                         ab::data().affected_by( p->spell.fatebound_lucky_coin_buff->effectN( 3 ) ) ||
                                         ab::data().affected_by( p->spell.fatebound_lucky_coin_buff->effectN( 4 ) ) ||
                                         ab::data().affected_by( p->spell.fatebound_lucky_coin_buff->effectN( 5 ) ) );
    }
    
    // Assassination
    affected_by.blindside = ab::data().affected_by( p->spec.blindside_buff->effectN( 1 ) );

    if ( p->talent.assassination.systemic_failure->ok() )
    {
      affected_by.maim_mangle = ab::data().affected_by( p->spec.garrote->effectN( 4 ) );
    }

    if ( p->talent.assassination.dashing_scoundrel->ok() )
    {
      affected_by.dashing_scoundrel = ab::data().affected_by( p->spec.envenom->effectN( 5 ) );
    }

    if ( p->talent.assassination.zoldyck_recipe->ok() )
    {
      // Not in spell data. Using the mastery whitelist as a baseline, most seem to apply (including VV)
      // 2025-08-03 -- Effect 4 with the plague damage talents is, as only Corrupt the Blood appears to work
      affected_by.zoldyck_insignia = ab::data().affected_by( p->mastery.potent_assassin->effectN( 1 ) ) ||
                                     ab::data().affected_by( p->mastery.potent_assassin->effectN( 2 ) ) ||
                                     ab::data().affected_by_label( p->mastery.potent_assassin->effectN( 3 ) );
    }

    if ( p->talent.assassination.lethal_dose->ok() )
    {
      // Not in spell data. Using the mastery whitelist as a baseline, most seem to apply
      affected_by.lethal_dose = ab::data().affected_by( p->mastery.potent_assassin->effectN( 1 ) ) ||
                                ab::data().affected_by( p->mastery.potent_assassin->effectN( 2 ) );
    }

    if ( p->talent.assassination.deathmark->ok() )
    {
      affected_by.deathmark = ab::data().affected_by( p->spec.deathmark_debuff->effectN( 1 ) ) ||
                              ab::data().affected_by( p->spec.deathmark_debuff->effectN( 2 ) );
    }

    // Outlaw
    affected_by.adrenaline_rush_gcd = ab::data().affected_by( p->talent.outlaw.adrenaline_rush->effectN( 3 ) );

    affected_by.roll_the_bones_cp = ab::data().affected_by( p->spec.jackpot->effectN( 2 ) );

    affected_by.audacity = ab::data().affected_by( p->spec.audacity_buff->effectN( 1 ) );

    // Subtlety
    affected_by.shadow_blades_cp = ( ab::data().affected_by( p->talent.subtlety.shadow_blades->effectN( 2 ) ) ||
                                     ab::data().affected_by( p->talent.subtlety.shadow_blades->effectN( 3 ) ) ||
                                     ab::data().affected_by( p->talent.subtlety.shadow_blades->effectN( 4 ) ) );

    affected_by.danse_macabre = ab::data().affected_by( p->spec.danse_macabre_buff->effectN( 1 ) );

    if ( p->talent.subtlety.goremaws_bite->ok() && p->spec.goremaws_bite_buff->ok() )
    {
      affected_by.goremaws_bite = ab::data().affected_by( p->spec.goremaws_bite_buff->effectN( 2 ) );
    }

    if ( p->talent.subtlety.planned_execution->ok() )
    {
      affected_by.planned_execution = ab::data().affected_by( p->spec.shadow_dance->effectN( 9 ) );
    }

    // Auto-parsing for damage affecting dynamic flags, this reads IF direct/periodic dmg is affected and stores by how much.
    // Still requires manual impl below but removes need to hardcode effect numbers.
    parse_damage_affecting_spell( p->mastery.executioner, affected_by.mastery_executioner );
    parse_damage_affecting_spell( p->mastery.potent_assassin, affected_by.mastery_potent_assassin );

    if ( p->set_bonuses.tww2_subtlety_4pc->ok() )
    {
      auto buff_spell = p->set_bonuses.tww2_subtlety_2pc->effectN( 1 ).trigger();

      affected_by.tww2_subtlety_4pc.direct_percent = p->set_bonuses.tww2_subtlety_4pc->effectN( 1 ).percent();
      affected_by.tww2_subtlety_4pc.direct = ab::data().affected_by( buff_spell->effectN( 2 ) ) ||
        ab::data().affected_by_label( buff_spell->effectN( 4 ) ) ||
        ab::data().affected_by_label( buff_spell->effectN( 6 ) );

      affected_by.tww2_subtlety_4pc.periodic_percent = p->set_bonuses.tww2_subtlety_4pc->effectN( 1 ).percent();
      affected_by.tww2_subtlety_4pc.periodic = ab::data().affected_by( buff_spell->effectN( 3 ) ) ||
        ab::data().affected_by_label( buff_spell->effectN( 5 ) ) ||
        ab::data().affected_by_label( buff_spell->effectN( 7 ) );
    }
  }

  void init() override
  {
    ab::init();
    
    if ( consumes_supercharger() )
    {
      supercharged_cp_proc = p()->get_proc( "Supercharger " + ab::name_str );
    }

    if ( p()->buffs.perforated_veins->is_affecting( &ab::data() ) )
    {
      perforated_veins_consumed_proc = p()->get_proc( "Perforated Veins " + ab::name_str );
    }

    auto register_damage_buff = [ this ]( damage_buff_t* buff ) {
      if ( buff->is_affecting_direct( ab::s_data ) )
        direct_damage_buffs.push_back( buff );

      if ( buff->is_affecting_periodic( ab::s_data ) )
        periodic_damage_buffs.push_back( buff );

      if ( ab::repeating && !ab::special && !ab::s_data->ok() && buff->auto_attack_mod.multiplier != 1.0 )
        auto_attack_damage_buffs.push_back( buff );

      if ( buff->is_affecting_crit_chance( ab::s_data ) )
        crit_chance_buffs.push_back( buff );
    };

    direct_damage_buffs.clear();
    periodic_damage_buffs.clear();
    auto_attack_damage_buffs.clear();
    crit_chance_buffs.clear();

    register_damage_buff( p()->buffs.acrobatic_strikes );
    register_damage_buff( p()->buffs.danse_macabre );
    register_damage_buff( p()->buffs.deathstalkers_mark );
    register_damage_buff( p()->buffs.fatebound_coin_heads );
    register_damage_buff( p()->buffs.flawless_form );
    register_damage_buff( p()->buffs.kingsbane );
    register_damage_buff( p()->buffs.momentum_of_despair );
    register_damage_buff( p()->buffs.perforated_veins );
    register_damage_buff( p()->buffs.shadow_dance );
    register_damage_buff( p()->buffs.summarily_dispatched );
    register_damage_buff( p()->buffs.symbolic_victory );
    register_damage_buff( p()->buffs.the_rotten );

    register_damage_buff( p()->buffs.double_trouble );
    register_damage_buff( p()->buffs.triple_threat );
    register_damage_buff( p()->buffs.jackpot );

    register_damage_buff( p()->buffs.tww1_assassination_2pc );
    register_damage_buff( p()->buffs.tww1_assassination_4pc );
    register_damage_buff( p()->buffs.tww1_outlaw_4pc );
    register_damage_buff( p()->buffs.tww1_subtlety_2pc );
    register_damage_buff( p()->buffs.tww1_subtlety_4pc );

    register_damage_buff( p()->buffs.tww2_assassination_2pc );
    register_damage_buff( p()->buffs.tww2_assassination_4pc );
    register_damage_buff( p()->buffs.tww2_outlaw_2pc );
    register_damage_buff( p()->buffs.tww2_subtlety_2pc );
    register_damage_buff( p()->buffs.tww3_deathstalker_2pc );

    if ( ab::base_costs[ RESOURCE_COMBO_POINT ] > 0 )
    {
      affected_by.deepening_shadows = true;
      affected_by.relentless_strikes = true;
      affected_by.ruthlessness = true;
    }

    // Auto-Consume Buffs on Execute
    auto register_consume_buff = [this]( buff_t* buff, bool condition, proc_t* proc = nullptr, timespan_t delay = timespan_t::zero(),
                                         bool on_background = false, bool decrement = false )
    {
      if ( condition )
      {
        consume_buffs.push_back( { buff, proc, delay, on_background, decrement } );
      }
    };

    register_consume_buff( p()->buffs.audacity, affected_by.audacity );
    register_consume_buff( p()->buffs.blindside, affected_by.blindside );
    register_consume_buff( p()->buffs.deathstalkers_mark, p()->buffs.deathstalkers_mark->is_affecting( &ab::data() ),
                           nullptr, 1_ms, true ); // Works with WM
    register_consume_buff( p()->buffs.goremaws_bite, affected_by.goremaws_bite );
    register_consume_buff( p()->buffs.perforated_veins, p()->buffs.perforated_veins->is_affecting( &ab::data() ),
                           perforated_veins_consumed_proc, 1_ms ); // TOCHECK -- Ensure this still affects WM procs like it used to
    register_consume_buff( p()->buffs.symbolic_victory, p()->buffs.symbolic_victory->is_affecting( &ab::data() ),
                           nullptr, p()->bugs ? 0_ms : 1_ms ); // 2024-08-12 -- Consumed immediatey, does not work with Shadowy Finishers
    register_consume_buff( p()->buffs.the_rotten, p()->buffs.the_rotten->is_affecting_direct( &ab::data() ), nullptr, 1_ms, false, true );
    
    register_consume_buff( p()->buffs.tww1_subtlety_2pc, p()->buffs.tww1_subtlety_2pc->is_affecting( &ab::data() ),
                           nullptr, 1.31_s ); // Appears to work on Clone Attacks
    register_consume_buff( p()->buffs.tww1_outlaw_4pc, p()->buffs.tww1_outlaw_4pc->is_affecting( &ab::data() ) );
  }

  // Type Wrappers ============================================================

  static const rogue_action_state_t<base_t>* cast_state( const action_state_t* st )
  { return debug_cast<const rogue_action_state_t<base_t>*>( st ); }

  static rogue_action_state_t<base_t>* cast_state( action_state_t* st )
  { return debug_cast<rogue_action_state_t<base_t>*>( st ); }

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

    if ( s->flags( spell_attribute::SX_NO_STEALTH_BREAK ) )
      _breaks_stealth = false;

    for ( size_t i = 1; i <= s->effect_count(); i++ )
    {
      const spelleffect_data_t& effect = s->effectN( i );

      if ( effect.type() == E_APPLY_AURA && effect.subtype() == A_PERIODIC_DAMAGE )
      {
        ab::base_td_adder = effect.bonus( p() );
      }
      else if ( effect.type() == E_SCHOOL_DAMAGE )
      {
        ab::base_dd_adder = effect.bonus( p() );
      }
    }
  }

  void parse_damage_affecting_spell( const spell_data_t* spell, damage_affect_data& flags )
  {
    if ( !spell->ok() )
      return;

    for ( const spelleffect_data_t& effect : spell->effects() )
    {
      if ( !effect.ok() || !( effect.type() == E_APPLY_AURA ||
                              effect.subtype() == A_ADD_PCT_MODIFIER ||
                              effect.subtype() == A_ADD_PCT_LABEL_MODIFIER ) )
        continue;

      if ( ab::data().affected_by_all( effect ) )
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
    return new rogue_action_state_t<base_t>( this, ab::target );
  }

  void update_state( action_state_t* state, unsigned flags, result_amount_type rt ) override
  {
    // Exsanguinated bleeds snapshot hasted tick time when the ticks are rescheduled.
    // This will make snapshot_internal on haste updates discard the new value.
    if ( cast_state( state )->is_exsanguinated() )
    {
      flags &= ~STATE_HASTE;
    }
    
    ab::update_state( state, flags, rt );
  }

  void snapshot_state( action_state_t* state, result_amount_type rt ) override
  {
    int consume_cp = as<int>( std::min( p()->current_cp(), p()->consume_cp_max() ) );
    int effective_cp = consume_cp;

    // Apply and Snapshot Supercharger Buffs
    if ( p()->talent.rogue.supercharger->ok() && consumes_supercharger() )
    {
      if ( range::any_of( p()->buffs.supercharger, []( const buff_t* buff ) { return buff->check(); } ) )
        effective_cp += as<int>( p()->talent.rogue.supercharger->effectN( 2 ).base_value() +
                                 p()->talent.rogue.forced_induction->effectN( 1 ).base_value() );
    }

    auto rs = cast_state( state );
    rs->set_combo_points( consume_cp, effective_cp );
    rs->clear_exsanguinated();

    ab::snapshot_state( state, rt );
  }

  // Secondary Trigger Functions ==============================================

  bool is_secondary_action() const
  { return secondary_trigger_type != secondary_trigger::NONE && ab::background == true; }

  virtual void trigger_secondary_action( player_t* target, int cp = 0, timespan_t delay = timespan_t::zero() )
  {
    assert( is_secondary_action() );
    make_event<secondary_action_trigger_t<base_t>>( *ab::sim, target, this, cp, delay );
  }

  virtual void trigger_secondary_action( player_t* target, timespan_t delay )
  {
    trigger_secondary_action( target, 0, delay );
  }

  virtual void trigger_secondary_action( action_state_t* s, timespan_t delay = timespan_t::zero() )
  {
    assert( is_secondary_action() && s->action == this );
    make_event<secondary_action_trigger_t<base_t>>( *ab::sim, s, delay );
  }

  // Residual Trigger Functions ===============================================

  virtual void trigger_residual_action( const action_state_t* s, double multiplier = 1.0, bool unmitigated = true, bool reverse_target_da_multiplier = true, player_t* override_target = nullptr, bool trigger_event = true )
  {
    // Depending on the ability, may use unmitigated or mitigated results
    const double base_damage = unmitigated ? s->result_total : s->result_amount;
    // Target multipliers may not replicate to secondary targets, which requires reversing them out
    const double target_da_multiplier = ( unmitigated && reverse_target_da_multiplier ) ? ( 1.0 / s->target_da_multiplier ) : 1.0;
    const double amount = base_damage * multiplier * target_da_multiplier;

    if ( amount <= 0 )
      return;

    player_t* primary_target = override_target ? override_target : s->target;

    p()->sim->print_debug( "{} triggers residual {} for {:.2f} damage ({:.2f} * {} * {:.3f}) on {}",
                           *p(), *this, amount, base_damage, multiplier, target_da_multiplier, *primary_target );

    if ( !ab::callbacks || !trigger_event )
    {
      ab::execute_on_target( primary_target, amount );
    }
    else
    {
      // Trigger as an event so that this happens after the impact for proc/RPPM targeting purposes
      make_event( *p()->sim, 0_ms, [ this, amount, primary_target ]() {
        ab::execute_on_target( primary_target, amount );
      } );
    }
  }

  virtual void trigger_residual_action( player_t* primary_target, double amount, bool trigger_event = true )
  {
    if ( amount <= 0 )
      return;

    p()->sim->print_debug( "{} triggers residual {} for {:.2f} damage on {}", *p(), *this, amount, *primary_target );

    if ( !ab::callbacks || !trigger_event )
    {
      ab::execute_on_target( primary_target, amount );
    }
    else
    {
      // Trigger as an event so that this happens after the impact for proc/RPPM targeting purposes
      make_event( *p()->sim, 0_ms, [ this, amount, primary_target ]() {
        ab::execute_on_target( primary_target, amount );
      } );
    }
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

      if ( affected_by.roll_the_bones_cp && p()->buffs.roll_the_bones->check_value() >= 2 )
      {
        cp += p()->spec.jackpot->effectN( 2 ).base_value();
      }

      if ( affected_by.shadow_blades_cp && p()->buffs.shadow_blades->check() )
      {
        cp += p()->buffs.shadow_blades->data().effectN( 2 ).base_value();
      }

      if ( affected_by.improved_ambush )
      {
        cp += p()->talent.rogue.improved_ambush->effectN( 1 ).base_value();
      }
    }

    return cp;
  }

  virtual bool procs_poison() const
  { return ab::weapon != nullptr && ab::has_amount_result(); }

  // 2022-10-23 -- As of the latest beta build it appears all expected abilities proc Deadly Poison
  virtual bool procs_deadly_poison() const
  { return procs_poison(); }

  // Generic rules for proccing Main Gauche, used by rogue_t::trigger_main_gauche()
  virtual bool procs_main_gauche() const
  { return false; }

  // Generic rules for proccing Fatal Flourish, used by rogue_t::trigger_fatal_flourish()
  virtual bool procs_fatal_flourish() const
  { return ab::callbacks && !ab::proc && ab::weapon != nullptr && ab::weapon->slot == SLOT_OFF_HAND; }

  // Generic rules for proccing Blade Flurry, used by rogue_t::trigger_blade_flurry()
  virtual bool procs_blade_flurry() const
  { return false; }

  // Generic rules for proccing Nimble Flurry, used by rogue_t::trigger_nimble_flurry()
  virtual bool procs_nimble_flurry() const
  { return false; }

  // Generic rules for proccing Shadow Blades, used by rogue_t::trigger_shadow_blades_attack()
  virtual bool procs_shadow_blades_damage() const
  { return true; }

  // Generic rules for proccing Caustic Spatter, used by rogue_t::trigger_caustic_spatter()
  virtual bool procs_caustic_spatter() const
  { return dbc::has_common_school( ab::get_school(), SCHOOL_NATURE ); }

  // Generic rules for proccing Seal Fate, used by rogue_t::trigger_seal_fate()
  virtual bool procs_seal_fate() const
  { return ab::energize_type != action_energize::NONE && ab::energize_resource == RESOURCE_COMBO_POINT && ab::energize_amount > 0; }
  
  // Placeholder for actions which additionally proc Deal Fate when they proc Seal Fate
  virtual bool procs_deal_fate() const
  { return false; }

  // Overridable wrapper for checking stealth requirement
  virtual bool requires_stealth() const
  {
    if ( affected_by.audacity && p()->buffs.audacity->check() )
      return false;

    if ( affected_by.blindside && p()->buffs.blindside->check() )
      return false;

    return _requires_stealth;
  }

  // Overridable wrapper for checking stealth breaking
  virtual bool breaks_stealth() const
  { return _breaks_stealth; }

  // Overridable function to determine whether a finisher is working with Supercharger
  virtual bool consumes_supercharger() const
  { return ab::base_costs[ RESOURCE_COMBO_POINT ] > 0 && ( ab::attack_power_mod.direct > 0.0 || ab::attack_power_mod.tick > 0.0 ); }

  double parry_chance( double exp, player_t* target ) const override
  {
    auto chance = ab::parry_chance(exp, target);
    if ( chance > 0.0 && td( target )->debuffs.fazed->up() )
    {
      chance += td( target )->debuffs.fazed->data().effectN( 2 ).percent();
    }
    return std::max(0.0, chance);
  }

public:
  // Ability triggers
  void spend_combo_points( const action_state_t* );
  void trigger_auto_attack( const action_state_t* );
  void trigger_poisons( const action_state_t* );
  void trigger_seal_fate( const action_state_t* );
  void trigger_main_gauche( const action_state_t* );
  void trigger_fatal_flourish( const action_state_t* );
  void trigger_energy_refund();
  void trigger_doomblade( const action_state_t* );
  void trigger_poison_bomb( const action_state_t* );
  void trigger_venomous_wounds( const action_state_t* );
  void trigger_blade_flurry( const action_state_t* );
  void trigger_ruthlessness_cp( const action_state_t* );
  void trigger_combo_point_gain( int, gain_t* gain = nullptr );
  void trigger_deepening_shadows( const action_state_t* );
  void trigger_shadow_techniques( const action_state_t* );
  void trigger_weaponmaster( const action_state_t*, rogue_attack_t* action );
  void trigger_opportunity( const action_state_t*, rogue_attack_t* action, double modifier = 1.0 );
  void trigger_restless_blades( const action_state_t* );
  void trigger_hand_of_fate( const action_state_t*, bool biased = false );
  void execute_fatebound_coinflip( const action_state_t* state, fatebound_t::coinflip_e result, timespan_t delay = timespan_t::zero() );
  void trigger_fatebound_edge_case( const action_state_t* state );
  void trigger_fate_intertwined( const action_state_t* );
  void trigger_relentless_strikes( const action_state_t* );
  void trigger_blindside( const action_state_t* );
  void trigger_shadow_blades_attack( const action_state_t* );
  void trigger_find_weakness( const action_state_t* state, timespan_t duration = timespan_t::min() );
  void trigger_master_of_shadows();
  void trigger_dashing_scoundrel( const action_state_t* state );
  void trigger_keep_it_rolling();
  void trigger_perforated_veins( const action_state_t* state );
  void trigger_lingering_shadow( const action_state_t* state );
  void trigger_danse_macabre( const action_state_t* state );
  void trigger_caustic_spatter( const action_state_t* state );
  void trigger_caustic_spatter_debuff( const action_state_t* state );
  void trigger_shadowcraft( const action_state_t* state );
  void trigger_cut_to_the_chase( const action_state_t* state );
  void trigger_cloud_cover( const action_state_t* state );
  void trigger_deathstalkers_mark( const action_state_t* state, bool ignore_cp = false );
  bool trigger_deathstalkers_mark_debuff( const action_state_t* state, bool from_darkest_night = false );
  void trigger_unseen_blade( const action_state_t* state );
  void trigger_nimble_flurry( const action_state_t* state );
  void trigger_supercharger();
  void trigger_echoing_reprimand( const action_state_t* state );
  void trigger_tww1_assassination_set_bonus( const action_state_t* state );
  void trigger_tww1_outlaw_set_bonus( const action_state_t* );
  void trigger_tww2_set_bonus_removal();

  // General Methods ==========================================================

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    if ( secondary_trigger_type != secondary_trigger::NONE )
    {
      cd_duration = timespan_t::zero();
    }

    if ( affected_by.tww3_fatebound_4pc && cd_duration != 0_ms && p()->buffs.fatebound_lucky_coin->check() )
    {
      if ( cd_duration == timespan_t::min() )
      {
        cd_duration = ab::cooldown_base_duration( *ab::cooldown );
      }

      for ( size_t i = 1; i <= p()->spell.fatebound_lucky_coin_buff->effect_count(); i++ )
      {
        if ( ab::data().affected_by( p()->spell.fatebound_lucky_coin_buff->effectN( i ) ) )
        {
          cd_duration += p()->spell.fatebound_lucky_coin_buff->effectN( i ).time_value();
        }
      }
    }

    ab::update_ready( cd_duration );
  }

  timespan_t gcd() const override
  {
    timespan_t t = ab::gcd();

    // As per alpha notes: Now increases global cooldown recovery rate equal to your Haste while active, up to 25%.
    // Cap is not currently reflected anywhere in spell data, dummy script periodic in AR effect 6 
    if ( affected_by.adrenaline_rush_gcd && t != timespan_t::zero() && p()->buffs.adrenaline_rush->check() )
    {
      double reduction_multiplier = std::clamp( ( 1.0 / p()->cache.attack_haste() ) - 1.0, 0.0, 0.25 ) / 0.25;
      t -= ( 200_ms * reduction_multiplier );
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

    // Registered Damage Buffs
    for ( auto damage_buff : direct_damage_buffs )
      m *= damage_buff->is_stacking ? damage_buff->stack_value_direct() : damage_buff->value_direct();
    
    // Mastery
    if ( affected_by.mastery_executioner.direct || affected_by.mastery_potent_assassin.direct )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    // Due to being scripted, Zoldyck and Lethal Dose are not affected by things that ignore target modifiers
    if ( affected_by.zoldyck_insignia &&
         state->target->health_percentage() < p()->spec.zoldyck_insignia->effectN( 2 ).base_value() )
    {
      m *= 1.0 + p()->spec.zoldyck_insignia->effectN( 1 ).percent();
    }

    if ( affected_by.lethal_dose )
    {
      m *= 1.0 + ( p()->talent.assassination.lethal_dose->effectN( 1 ).percent() *
                   td( state->target )->lethal_dose_count() );
    }

    // Follow the Blood
    if ( affected_by.follow_the_blood.direct )
    {
      if ( p()->get_active_dots( td( state->target )->dots.rupture ) >=
           as<unsigned int>( p()->talent.deathstalker.follow_the_blood->effectN( 2 ).base_value() ) )
      {
        m *= 1.0 + p()->talent.deathstalker.follow_the_blood->effectN( 1 ).percent() *
          ( 1.0 + p()->buffs.tww3_deathstalker_2pc->check() * p()->spec.tww3_deathstalker_2pc_buff->effectN( 2 ).percent() );
      }
    }

    // Darkest Night
    if ( affected_by.darkest_night )
    {
      if ( p()->buffs.darkest_night->up() && cast_state( state )->get_combo_points() >= p()->consume_cp_max() )
      {
        m *= 1.0 + p()->spell.darkest_night_buff->effectN( 2 ).percent() *
          ( 1.0 + p()->buffs.tww3_deathstalker_2pc->check() * p()->spec.tww3_deathstalker_2pc_buff->effectN( 3 ).percent() );
      }
    }

    // TWW2 Set Bonus
    if ( affected_by.tww2_subtlety_4pc.direct &&
         p()->buffs.shadow_dance->check() && p()->buffs.tww2_subtlety_2pc->check() )
    {
      m *= 1.0 + ( affected_by.tww2_subtlety_4pc.direct_percent * p()->buffs.tww2_subtlety_2pc->check() );
    }

    return m;
  }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = ab::composite_ta_multiplier( state );

    // Registered Damage Buffs
    for ( auto damage_buff : periodic_damage_buffs )
      m *= damage_buff->is_stacking ? damage_buff->stack_value_periodic() : damage_buff->value_periodic();

    // Masteries for Assassination and Subtlety have periodic damage in a separate effect. Just to be sure, use that instead of direct mastery_value.
    if ( affected_by.mastery_executioner.periodic )
    {
      m *= 1.0 + p()->cache.mastery() * p()->mastery.executioner->effectN( 2 ).mastery_value();
    }

    if ( affected_by.mastery_potent_assassin.periodic )
    {
      m *= 1.0 + p()->cache.mastery() * p()->mastery.potent_assassin->effectN( 2 ).mastery_value();
    }

    // Due to being scripted, Zoldyck and Lethal Dose are not affected by things that ignore target modifiers
    if ( affected_by.zoldyck_insignia &&
         state->target->health_percentage() < p()->spec.zoldyck_insignia->effectN( 2 ).base_value() )
    {
      m *= 1.0 + p()->spec.zoldyck_insignia->effectN( 1 ).percent();
    }

    if ( affected_by.lethal_dose )
    {
      m *= 1.0 + ( p()->talent.assassination.lethal_dose->effectN( 1 ).percent() *
                   td( state->target )->lethal_dose_count() );
    }

    // Follow the Blood
    if ( affected_by.follow_the_blood.periodic )
    {
      if ( p()->get_active_dots( td( state->target )->dots.rupture ) >=
           as<unsigned int>( p()->talent.deathstalker.follow_the_blood->effectN( 2 ).base_value() ) )
      {
        m *= 1.0 + p()->talent.deathstalker.follow_the_blood->effectN( 1 ).percent() *
          ( 1.0 + p()->buffs.tww3_deathstalker_2pc->check() * p()->spec.tww3_deathstalker_2pc_buff->effectN( 2 ).percent() );
      }
    }

    // TWW2 Set Bonus
    if ( affected_by.tww2_subtlety_4pc.periodic &&
         p()->buffs.shadow_dance->check() && p()->buffs.tww2_subtlety_2pc->check() )
    {
      m *= 1.0 + ( affected_by.tww2_subtlety_4pc.periodic_percent * p()->buffs.tww2_subtlety_2pc->check() );
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = ab::composite_target_multiplier( target );

    auto tdata = td( target );

    if ( affected_by.maim_mangle && tdata->dots.garrote->is_ticking() )
    {
      m *= 1.0 + p()->talent.assassination.systemic_failure->effectN( 1 ).percent();
    }

    if ( affected_by.deathmark )
    {
      m *= tdata->debuffs.deathmark->value_direct();
    }

    if ( affected_by.fazed_damage )
    {
      m *= tdata->debuffs.fazed->value_direct();
    }

    return m;
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = ab::composite_target_crit_chance( target );

    if ( affected_by.fazed_crit_chance && td( target )->debuffs.fazed->check() )
    {
      c += td( target )->debuffs.fazed->value_crit_chance();
    }

    return c;
  }

  double composite_crit_chance() const override
  {
    double c = ab::composite_crit_chance();

    // Registered Damage Buffs
    for ( auto crit_chance_buff : crit_chance_buffs )
      c += crit_chance_buff->stack_value_crit_chance();

    if ( affected_by.dashing_scoundrel && p()->buffs.envenom->check() )
    {
      c += p()->spec.envenom->effectN( 5 ).percent() * p()->buffs.envenom->check();
    }

    if ( affected_by.darkest_night_crit && p()->buffs.darkest_night->up() )
    {
      if ( ab::pre_execute_state && cast_state( ab::pre_execute_state )->get_combo_points() >= p()->consume_cp_max() )
      {
        c += 1.0 + p()->spell.darkest_night_buff->effectN( 4 ).percent();
      }
      // No CP state available this early as crit chance is calculated during state creation
      else if ( p()->current_effective_cp( true ) >= p()->consume_cp_max() )
      {
        c += 1.0 + p()->spell.darkest_night_buff->effectN( 4 ).percent();
      }
    }

    return c;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double cm = ab::composite_crit_damage_bonus_multiplier();

    if ( affected_by.momentum_of_despair && p()->buffs.momentum_of_despair->check() )
    {
      cm *= 1.0 + p()->spell.momentum_of_despair_buff->effectN( 2 ).percent() *
        ( 1.0 + p()->buffs.tww3_deathstalker_2pc->check() * p()->spec.tww3_deathstalker_2pc_buff->effectN( 3 ).percent() );
    }

    if ( affected_by.planned_execution && p()->buffs.shadow_dance->check() )
    {
      cm *= 1.0 + p()->talent.subtlety.planned_execution->effectN( 1 ).percent();
    }

    return cm;
  }

  double composite_target_crit_damage_bonus_multiplier( player_t* target ) const override
  {
    double cm = ab::composite_target_crit_damage_bonus_multiplier( target );

    if ( affected_by.fazed_crit_damage && td( target )->debuffs.fazed->check() )
    {
      cm *= 1.0 + p()->talent.trickster.surprising_strikes->effectN( 1 ).percent();
    }

    return cm;
  }

  timespan_t tick_time( const action_state_t* state ) const override
  {
    timespan_t tt = ab::tick_time( state );

    tt *= 1.0 / cast_state( state )->get_exsanguinated_rate();

    return tt;
  }

  virtual double composite_poison_flat_modifier( const action_state_t* ) const
  { return 0.0; }

  double cost_pct_multiplier() const override
  {
    double c = ab::cost_pct_multiplier();

    if ( p()->talent.subtlety.shadow_focus->ok() && p()->stealthed( STEALTH_BASIC | STEALTH_SHADOW_DANCE ) )
    {
      c *= 1.0 + p()->spec.shadow_focus_buff->effectN( 1 ).percent();
    }

    if ( affected_by.blindside )
    {
      c *= 1.0 + p()->buffs.blindside->check_value();
    }

    if ( affected_by.goremaws_bite )
    {
      c *= 1.0 + p()->buffs.goremaws_bite->check_value();
    }

    return c;
  }

  void consume_resource() override
  {
    // Abilities triggered as part of another ability (secondary triggers) do not consume resources
    if ( is_secondary_action() )
    {
      return;
    }

    ab::consume_resource();

    spend_combo_points( ab::execute_state );

    if ( ab::current_resource() == RESOURCE_ENERGY && ab::last_resource_cost > 0 )
    {
      if ( !ab::hit_any_target )
      {
        trigger_energy_refund();
      }
      else
      {
        // Energy Spend Mechanics
      }

      // 2024-09-07 -- Thistle Tea now triggers automatically when Energy drops low enough
      if ( p()->talent.rogue.thistle_tea->ok() && p()->cooldowns.thistle_tea->current_charge > 0 )
      {
        if ( p()->resources.current[ RESOURCE_ENERGY ] < p()->talent.rogue.thistle_tea->effectN( 2 ).resource( RESOURCE_ENERGY ) )
        {
          p()->active.thistle_tea->execute();
        }
      }
    }
  }

  void execute() override
  {
    ab::execute();

    if ( ab::hit_any_target )
    {
      trigger_auto_attack( ab::execute_state );
      trigger_ruthlessness_cp( ab::execute_state );

      if ( ab::energize_type != action_energize::NONE && ab::energize_resource == RESOURCE_COMBO_POINT )
      {
        if ( affected_by.shadow_blades_cp && p()->buffs.shadow_blades->up() )
        {
          trigger_combo_point_gain( as<int>( p()->buffs.shadow_blades->data().effectN( 2 ).base_value() ), p()->gains.shadow_blades );
        }

        if ( affected_by.roll_the_bones_cp && p()->buffs.roll_the_bones->check_value() >= 2 )
        {
          trigger_combo_point_gain( as<int>( p()->spec.jackpot->effectN( 2 ).base_value() ), p()->gains.roll_the_bones );
        }

        if ( affected_by.improved_ambush )
        {
          trigger_combo_point_gain( as<int>( p()->talent.rogue.improved_ambush->effectN( 1 ).base_value() ), p()->gains.improved_ambush );
        }

        if ( p()->buffs.premeditation->up() )
        {
          trigger_combo_point_gain( as<int>( p()->talent.subtlety.premeditation->effectN( 1 ).base_value() ), p()->gains.premeditation );
          p()->buffs.premeditation->expire();
        }

        // Shadow Techniques gains are the last thing to be evaluated in order to reduce buff stack consume waste
        if ( p()->spec.shadow_techniques->ok() && p()->buffs.shadow_techniques->up() && ab::energize_amount > 0 )
        {
          auto consume_stacks = std::min( p()->buffs.shadow_techniques->check(),
                                          std::max( 0, as<int>( p()->consume_cp_max() - p()->current_cp() ) ) );
          if ( consume_stacks > 0 )
          {
            trigger_combo_point_gain( consume_stacks, p()->gains.shadow_techniques );
            p()->buffs.shadow_techniques->decrement( consume_stacks );
          }
        }
      }

      trigger_danse_macabre( ab::execute_state );
      trigger_relentless_strikes( ab::execute_state );
      trigger_deepening_shadows( ab::execute_state );
      trigger_shadowcraft( ab::execute_state );
    }

    // Trigger the 1ms delayed breaking of all stealth buffs
    if ( p()->stealthed( STEALTH_BASIC | STEALTH_SHADOWMELD ) && breaks_stealth() )
    {
      p()->break_stealth();
    }

    // Expire On-Cast Fading Buffs
    for ( consume_buff_t& consume_buff : consume_buffs )
    {
      if ( consume_buff.on_background || !ab::background || ( is_secondary_action() && ab::not_a_proc ) )
      {
        if ( consume_buff.buff->check() )
        {
          if ( consume_buff.decrement )
          {
            if ( consume_buff.delay > 0_s )
              make_event( *p()->sim, consume_buff.delay, [ consume_buff ] { consume_buff.buff->decrement(); } );
            else
              consume_buff.buff->decrement();
          }
          else
          {
            consume_buff.buff->expire( consume_buff.delay );
          }
          if ( consume_buff.proc )
            consume_buff.proc->occur();
        }
      }
    }

    // Debugging
    if ( p()->sim->log && ab::execute_state )
    {
      const auto rs = cast_state( ab::execute_state );
      if ( rs->get_exsanguinated_rate() != 1.0 )
      {
        p()->sim->print_log( "{} casts {} with initial exsanguinated rate of {:.1f}", *p(), *this, rs->get_exsanguinated_rate() );
      }
    }
  }

  void schedule_travel( action_state_t* state ) override
  {
    ab::schedule_travel( state );

    trigger_seal_fate( state );
    trigger_poisons( state );
  }

  bool ready() override
  {
    if ( !ab::ready() )
      return false;

    if ( ab::base_costs[ RESOURCE_COMBO_POINT ] > 0 &&
      p()->current_cp() < ab::base_costs[ RESOURCE_COMBO_POINT ] )
      return false;

    if ( requires_stealth() && !p()->stealthed( STEALTH_STANCE ) )
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
                       util::string_view o = {} )
    : base_t( n, p, s, o )
  {
    harmful = false;
    set_target( p );
  }

  bool breaks_stealth() const override
  { return false; }
};

struct rogue_spell_t : public rogue_action_t<spell_t>
{
  rogue_spell_t( util::string_view n, rogue_t* p,
                        const spell_data_t* s = spell_data_t::nil(),
                        util::string_view o = {} )
    : base_t( n, p, s, o )
  {}
};

struct rogue_attack_t : public rogue_action_t<melee_attack_t>
{
  rogue_attack_t( util::string_view n, rogue_t* p,
                         const spell_data_t* s = spell_data_t::nil(),
                         util::string_view o = {} )
    : base_t( n, p, s, o )
  {
    special = true;
  }

  void impact( action_state_t* state ) override
  {
    base_t::impact( state );

    trigger_main_gauche( state );
    trigger_fatal_flourish( state );
    trigger_blade_flurry( state );
    trigger_nimble_flurry( state );
    trigger_shadow_blades_attack( state );
    trigger_dashing_scoundrel( state );
    trigger_caustic_spatter( state );
    trigger_cloud_cover( state );
    trigger_deathstalkers_mark( state );
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    trigger_shadow_blades_attack( d->state );
    trigger_dashing_scoundrel( d->state );
    trigger_caustic_spatter( d->state );
    trigger_cloud_cover( d->state );
    trigger_tww1_assassination_set_bonus( d->state );
  }
};

// ==========================================================================
// Poisons
// ==========================================================================

struct rogue_poison_t : public rogue_attack_t
{
  bool is_lethal;
  parsed_value_t<double> base_proc_chance;
  rogue_attack_t* deathmark_impact_action;

  rogue_poison_t( util::string_view name, rogue_t* p, const spell_data_t* s,
                  bool is_lethal = false, bool triggers_procs = false ) :
    actions::rogue_attack_t( name, p, s ),
    is_lethal( is_lethal ),
    base_proc_chance(),
    deathmark_impact_action( nullptr )
  {
    background = dual = true;
    proc = !triggers_procs;
    callbacks = triggers_procs;

    trigger_gcd = timespan_t::zero();

    base_proc_chance = p->get_passive_value( data(), "proc_chance", 0.01 );
  }

  timespan_t execute_time() const override
  {
    return timespan_t::zero();
  }

  virtual double proc_chance( const action_state_t* source_state ) const
  {
    if ( base_proc_chance == 0.0 )
      return 0.0;

    auto chance = base_proc_chance;
    chance += p()->buffs.envenom->stack_value();

    // Applied after Dragon-Tempered Blades' modifer for Thrown Precision and Poisoned Knife
    return chance.value() + rogue_t::cast_attack( source_state->action )->composite_poison_flat_modifier( source_state );
  }

  virtual void trigger( const action_state_t* source_state )
  {
    // 2025-05-01 -- Terrible hack due to the discovery that Thrown Precision can roll poisons twice
    if ( p()->bugs && p()->talent.assassination.thrown_precision->ok() && source_state->result == RESULT_CRIT &&
         rogue_t::cast_attack( source_state->action )->data().id() == p()->spec.fan_of_knives->id() )
    {
      sim->print_debug( "{} procs poison from thrown_precision {}, target={} source={}", *player, *this,
                        *source_state->target, *source_state->action );

      execute_on_target( source_state->target );
    }

    bool result = rng().roll( proc_chance( source_state ) );

    sim->print_debug( "{} attempts to proc poison {}, target={} source={} proc_chance={}: {}", *player, *this,
                      *source_state->target, *source_state->action, proc_chance( source_state ), result );

    if ( !result )
      return;

    execute_on_target( source_state->target );
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( is_lethal && state->result_amount > 0 )
    {
      if ( td( state->target )->dots.kingsbane->is_ticking() )
        p()->buffs.kingsbane->trigger();
    }

    if ( deathmark_impact_action && td( state->target )->dots.deathmark->is_ticking() )
    {
      deathmark_impact_action->trigger_secondary_action( state->target );
    }
  }
};

// Deadly Poison ============================================================

struct deadly_poison_t : public rogue_poison_t
{
  struct deadly_poison_dd_t : public rogue_poison_t
  {
    deadly_poison_dd_t( util::string_view name, rogue_t* p, const spell_data_t* s ) :
      rogue_poison_t( name, p, s, true, true )
    {
    }
  };

  struct deadly_poison_dot_t : public rogue_poison_t
  {
    deadly_poison_dot_t( util::string_view name, rogue_t* p, const spell_data_t* s ) :
      rogue_poison_t( name, p, s, true, true )
    {
    }
  };

  deadly_poison_dd_t* proc_instant;
  deadly_poison_dot_t* proc_dot;

  deadly_poison_t( util::string_view name, rogue_t* p ) :
    rogue_poison_t( name, p, p->talent.assassination.deadly_poison ),
    proc_instant( nullptr ), proc_dot( nullptr )
  {
    proc_instant = p->get_background_action<deadly_poison_dd_t>(
      "deadly_poison_instant", p->spec.deadly_poison_instant );
    proc_dot = p->get_background_action<deadly_poison_dot_t>(
      "deadly_poison_dot", p->talent.assassination.deadly_poison->effectN( 1 ).trigger() );

    add_child( proc_instant );
    add_child( proc_dot );
  }

  void impact( action_state_t* state ) override
  {
    rogue_poison_t::impact( state );

    rogue_td_t* tdata = td( state->target );
    if ( tdata->dots.deadly_poison->is_ticking() )
    {
      proc_instant->set_target( state->target );
      proc_instant->execute();
    }
    proc_dot->set_target( state->target );
    proc_dot->execute();

    if ( tdata->dots.deathmark->is_ticking() )
    {
      if ( tdata->dots.deadly_poison_deathmark->is_ticking() )
      {
        p()->active.deathmark.deadly_poison_instant->trigger_secondary_action( state->target );
      }
      p()->active.deathmark.deadly_poison_dot->trigger_secondary_action( state->target );
    }
  }
};

// Instant Poison ===========================================================

struct instant_poison_t : public rogue_poison_t
{
  struct instant_poison_dd_t : public rogue_poison_t
  {
    instant_poison_dd_t( util::string_view name, rogue_t* p, const spell_data_t* s ) :
      rogue_poison_t( name, p, s, true, true )
    {
    }
  };

  instant_poison_t( util::string_view name, rogue_t* p ) :
    rogue_poison_t( name, p, p->spell.instant_poison )
  {
    impact_action = p->get_background_action<instant_poison_dd_t>(
      "instant_poison", p->spell.instant_poison->effectN( 1 ).trigger() );

    if ( p->talent.assassination.deathmark->ok() )
    {
      deathmark_impact_action = p->active.deathmark.instant_poison;
    }
  }
};

// Wound Poison =============================================================

struct wound_poison_t : public rogue_poison_t
{
  struct wound_poison_dd_t : public rogue_poison_t
  {
    wound_poison_dd_t( util::string_view name, rogue_t* p, const spell_data_t* s ) :
      rogue_poison_t( name, p, s, true, true )
    {
    }

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );
      td( state->target )->debuffs.wound_poison->trigger();
      if ( !sim->overrides.mortal_wounds && state->target->debuffs.mortal_wounds )
      {
        state->target->debuffs.mortal_wounds->trigger( data().duration() );
      }
    }
  };

  wound_poison_t( util::string_view name, rogue_t* p ) :
    rogue_poison_t( name, p, p->spell.wound_poison )
  {
    impact_action = p->get_background_action<wound_poison_dd_t>(
      "wound_poison", p->spell.wound_poison->effectN( 1 ).trigger() );

    if ( p->talent.assassination.deathmark->ok() )
    {
      deathmark_impact_action = p->active.deathmark.wound_poison;
    }
  }
};

// Amplifying Poison ========================================================

struct amplifying_poison_t : public rogue_poison_t
{
  struct amplifying_poison_dd_t : public rogue_poison_t
  {
    amplifying_poison_dd_t( util::string_view name, rogue_t* p, const spell_data_t* s ) :
      rogue_poison_t( name, p, s, true, true )
    {
    }

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );

      if ( secondary_trigger_type == secondary_trigger::DEATHMARK )
      {
        td( state->target )->debuffs.amplifying_poison_deathmark->trigger();
      }
      else
      {
        td( state->target )->debuffs.amplifying_poison->trigger();
      }
    }
  };

  amplifying_poison_t( util::string_view name, rogue_t* p ) :
    rogue_poison_t( name, p, p->talent.assassination.amplifying_poison )
  {
    impact_action = p->get_background_action<amplifying_poison_dd_t>(
      "amplifying_poison", p->talent.assassination.amplifying_poison->effectN( 3 ).trigger() );

    if ( p->talent.assassination.deathmark->ok() )
    {
      deathmark_impact_action = p->active.deathmark.amplifying_poison;
    }
  }
};

// Atrophic Poison ==========================================================

struct atrophic_poison_t : public rogue_poison_t
{
  struct atrophic_poison_proc_t : public rogue_poison_t
  {
    atrophic_poison_proc_t( util::string_view name, rogue_t* p ) :
      rogue_poison_t( name, p, p->talent.rogue.atrophic_poison->effectN( 1 ).trigger(), false, true )
    {
    }

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );
      td( state->target )->debuffs.atrophic_poison->trigger();
    }
  };

  atrophic_poison_t( util::string_view name, rogue_t* p ) :
    rogue_poison_t( name, p, p->talent.rogue.atrophic_poison )
  {
    impact_action = p->get_background_action<atrophic_poison_proc_t>( "atrophic_poison" );
  }
};

// Crippling Poison =========================================================

struct crippling_poison_t : public rogue_poison_t
{
  struct crippling_poison_proc_t : public rogue_poison_t
  {
    crippling_poison_proc_t( util::string_view name, rogue_t* p ) :
      rogue_poison_t( name, p, p->spell.crippling_poison->effectN( 1 ).trigger(), false, true )
    { }

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );
      if ( !state->target->is_boss() )
      {
        td( state->target )->debuffs.crippling_poison->trigger();
      }
    }
  };

  crippling_poison_t( util::string_view name, rogue_t* p ) :
    rogue_poison_t( name, p, p->spell.crippling_poison )
  {
    impact_action = p->get_background_action<crippling_poison_proc_t>( "crippling_poison" );
  }
};

// Numbing Poison ===========================================================

struct numbing_poison_t : public rogue_poison_t
{
  struct numbing_poison_proc_t : public rogue_poison_t
  {
    numbing_poison_proc_t( util::string_view name, rogue_t* p ) :
      rogue_poison_t( name, p, p->talent.rogue.numbing_poison->effectN( 1 ).trigger() )
    { }

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );
      td( state->target )->debuffs.numbing_poison->trigger();
    }
  };

  numbing_poison_t( util::string_view name, rogue_t* p ) :
    rogue_poison_t( name, p, p->talent.rogue.numbing_poison )
  {
    impact_action = p->get_background_action<numbing_poison_proc_t>( "numbing_poison" );
  }
};

// Apply Poison =============================================================

struct apply_poison_t : public action_t
{
  bool executed;

  apply_poison_t( rogue_t* p, util::string_view options_str ) :
    action_t( ACTION_OTHER, "apply_poison", p ),
    executed( false )
  {
    std::string lethal_str;
    std::string lethal_dtb_str;
    std::string nonlethal_str;
    std::string nonlethal_dtb_str;

    add_option( opt_string( "lethal", lethal_str ) );
    add_option( opt_string( "lethal_dtb", lethal_dtb_str ) );
    add_option( opt_string( "nonlethal", nonlethal_str ) );
    add_option( opt_string( "nonlethal_dtb", nonlethal_dtb_str ) );
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;
    set_target( p );

    if ( p->main_hand_weapon.type != WEAPON_NONE || p->off_hand_weapon.type != WEAPON_NONE )
    {
      if ( !p->active.lethal_poison )
      {
        if ( lethal_str.empty() )
        {
          lethal_str = get_default_lethal_poison( p );
        }
        
        p->active.lethal_poison = get_poison( p, lethal_str );
        if ( p->active.lethal_poison )
        {
          sim->print_log( "{} applies lethal poison {}", *p, *p->active.lethal_poison );
        }
      }

      if ( !p->active.nonlethal_poison )
      {
        if ( nonlethal_str.empty() )
        {
          nonlethal_str = get_default_nonlethal_poison_dtb( p );;
        }

        p->active.nonlethal_poison = get_poison( p, nonlethal_str );
        if ( p->active.nonlethal_poison )
        {
          sim->print_log( "{} applies non-lethal poison {}", *p, *p->active.nonlethal_poison );
        }
      }

      if ( p->talent.assassination.dragon_tempered_blades->ok() )
      {
        if ( !p->active.lethal_poison_dtb )
        {
          if ( lethal_dtb_str.empty() || get_poison( p, lethal_dtb_str ) == p->active.lethal_poison )
          {
            lethal_dtb_str = get_default_lethal_poison_dtb( p );
          }

          p->active.lethal_poison_dtb = get_poison( p, lethal_dtb_str );
          if ( p->active.lethal_poison_dtb )
          {
            sim->print_log( "{} applies second lethal poison {}", *p, *p->active.lethal_poison_dtb );
          }

          if ( nonlethal_dtb_str.empty() || get_poison( p, nonlethal_dtb_str ) == p->active.nonlethal_poison )
          {
            nonlethal_dtb_str = get_default_nonlethal_poison_dtb( p );
          }

          p->active.nonlethal_poison_dtb = get_poison( p, nonlethal_dtb_str );
          if ( p->active.nonlethal_poison_dtb )
          {
            sim->print_log( "{} applies second non-lethal poison {}", *p, *p->active.nonlethal_poison_dtb );
          }
        }
      }
    }
  }

  std::string get_default_lethal_poison( rogue_t* p )
  {
    if ( p->talent.assassination.amplifying_poison->ok() && !p->talent.assassination.dragon_tempered_blades->ok() )
      return "amplifying";

    if ( p->talent.assassination.deadly_poison->ok() )
      return "deadly";

    return "instant";
  }

  std::string get_default_lethal_poison_dtb( rogue_t* p )
  {
    if ( p->active.lethal_poison && p->active.lethal_poison->data().id() == p->talent.assassination.deadly_poison->id() )
    {
      return p->talent.assassination.amplifying_poison->ok() ? "amplifying" : "instant";
    }
    else
    {
      return "deadly";
    }
  }

  std::string get_default_nonlethal_poison( rogue_t* )
  {
    return "crippling";
  }

  std::string get_default_nonlethal_poison_dtb( rogue_t* p )
  {
    if ( p->active.nonlethal_poison && p->active.nonlethal_poison->data().id() == p->spell.crippling_poison->id() )
    {
      if ( p->talent.rogue.numbing_poison->ok() )
        return "numbing";
      else if ( p->talent.rogue.atrophic_poison->ok() )
        return "atrophic";
      
      return "none";
    }
    else
    {
      return "crippling";
    }
  }

  rogue_poison_t* get_poison( rogue_t* p, util::string_view poison_name )
  {
    if ( poison_name == "none" || poison_name.empty() )
      return nullptr;
    else if ( poison_name == "deadly" )
      return p->get_background_action<deadly_poison_t>( "deadly_poison_driver" );
    else if ( poison_name == "instant" )
      return p->get_background_action<instant_poison_t>( "instant_poison_driver" );
    else if ( poison_name == "amplifying" )
      return p->get_background_action<amplifying_poison_t>( "amplifying_poison_driver" );
    else if ( poison_name == "wound" )
      return p->get_background_action<wound_poison_t>( "wound_poison_driver" );
    else if ( poison_name == "atrophic" )
      return p->get_background_action<atrophic_poison_t>( "atrophic_poison_driver" );
    else if ( poison_name == "crippling" )
      return p->get_background_action<crippling_poison_t>( "crippling_poison_driver" );
    else if ( poison_name == "numbing" )
      return p->get_background_action<numbing_poison_t>( "numbing_poison_driver" );

    sim->error( "Player {} attempting to apply unknown poison {}.", *p, poison_name );
    return nullptr;
  }

  void reset() override
  {
    action_t::reset();
    executed = false;
  }

  void execute() override
  {
    executed = true;
    sim->print_log( "{} performs {}", *player, *this );
  }

  bool ready() override
  {
    return !executed;
  }
};

// ==========================================================================
// Attacks
// ==========================================================================

// Melee Attack =============================================================

struct melee_t : public rogue_attack_t
{
  int sync_weapons;
  bool first;
  bool canceled;
  timespan_t prev_scheduled_time;

  melee_t( const char* name, const char* reporting_name, rogue_t* p, int sw ) :
    rogue_attack_t( name, p ), sync_weapons( sw ), first( true ), canceled( false ),
    prev_scheduled_time( timespan_t::zero() )
  {
    background = repeating = may_glance = may_crit = true;
    allow_class_ability_procs = not_a_proc = true;
    special = false;
    school = SCHOOL_PHYSICAL;
    trigger_gcd = timespan_t::zero();
    weapon_multiplier = 1.0;
    name_str_reporting = reporting_name;

    if ( p->dual_wield() )
      base_hit -= 0.19;
  }

  void reset() override
  {
    rogue_attack_t::reset();
    first = true;
    canceled = false;
    prev_scheduled_time = timespan_t::zero();
  }

  timespan_t execute_time() const override
  {
    timespan_t t = rogue_attack_t::execute_time();
    
    if ( first )
    {
      return ( weapon->slot == SLOT_OFF_HAND ) ? ( sync_weapons ? timespan_t::zero() : t / 2 ) : timespan_t::zero();
    }

    // If we cancel the swing timer mid-fight, use the previous swing timer
    if ( canceled )
    {
      return std::min( t, std::max( prev_scheduled_time - p()->sim->current_time(), timespan_t::zero() ) );
    }

    return t;
  }

  void schedule_execute( action_state_t* state ) override
  {
    rogue_attack_t::schedule_execute( state );

    if ( first )
    {
      first = false;
      p()->sim->print_log( "{} schedules AA start {} with {} swing timer", *p(), *this, time_to_execute );
    }

    if ( canceled )
    {
      canceled = false;
      prev_scheduled_time = timespan_t::zero();
      p()->sim->print_log( "{} schedules AA restart {} with {} swing timer remaining", *p(), *this, time_to_execute );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    trigger_shadow_techniques( state );

    if ( result_is_hit( state->result ) )
    {
      p()->buffs.acrobatic_strikes->trigger();

      if ( p()->active.deathstalker.hunt_them_down && p()->get_target_data( state->target )->debuffs.deathstalkers_mark->check() )
      {
        p()->active.deathstalker.hunt_them_down->execute_on_target( state->target );
      }
    }
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    m *= td( target )->debuffs.fazed->value_auto_attack();

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = rogue_attack_t::composite_crit_chance();
    return c;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    // Registered Damage Buffs
    for ( auto damage_buff : auto_attack_damage_buffs )
      m *= damage_buff->is_stacking ? damage_buff->stack_value_auto_attack() : damage_buff->value_auto_attack();

    // Class Passives
    m *= 1.0 + p()->spec.assassination_rogue->effectN( 22 ).percent();
    m *= 1.0 + p()->spec.assassination_rogue->effectN( 23 ).percent(); // Non-PvP
    m *= 1.0 + p()->spec.outlaw_rogue->effectN( 18 ).percent();
    m *= 1.0 + p()->spec.subtlety_rogue->effectN( 11 ).percent();
    m *= 1.0 + p()->spec.subtlety_rogue->effectN( 18 ).percent(); // Non-PvP

    return m;
  }

  bool procs_poison() const override
  { return true; }

  bool procs_main_gauche() const override
  { return weapon->slot == SLOT_MAIN_HAND; }

  bool procs_blade_flurry() const override
  { return true; }

  bool procs_nimble_flurry() const override
  { return true; }
};

// Auto Attack ==============================================================

struct auto_melee_attack_t : public action_t
{
  int sync_weapons;

  auto_melee_attack_t( rogue_t* p, util::string_view options_str ) :
    action_t( ACTION_OTHER, "auto_attack", p ),
    sync_weapons( 0 )
  {
    trigger_gcd = timespan_t::zero();
    name_str_reporting = "Auto Attack";

    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    parse_options( options_str );

    if ( p -> main_hand_weapon.type == WEAPON_NONE )
    {
      background = true;
      return;
    }

    p->melee_main_hand = debug_cast<melee_t*>( p->find_action( "auto_attack_mh" ) );
    if ( !p->melee_main_hand )
      p->melee_main_hand = new melee_t( "auto_attack_mh", "Auto Attack (Main Hand)", p, sync_weapons );

    add_child( p->melee_main_hand );

    p->main_hand_attack = p->melee_main_hand;
    p->main_hand_attack->weapon = &( p->main_hand_weapon );
    p->main_hand_attack->base_execute_time = p->main_hand_weapon.swing_time;

    if ( p->off_hand_weapon.type != WEAPON_NONE )
    {
      p->melee_off_hand = debug_cast<melee_t*>( p->find_action( "auto_attack_oh" ) );
      if ( !p->melee_off_hand )
        p->melee_off_hand = new melee_t( "auto_attack_oh", "Auto Attack (Off Hand)", p, sync_weapons );

      add_child( p->melee_off_hand );

      p->off_hand_attack = p->melee_off_hand;
      p->off_hand_attack->weapon = &( p->off_hand_weapon );
      p->off_hand_attack->base_execute_time = p->off_hand_weapon.swing_time;
      p->off_hand_attack->id = 1;
    }
  }

  void execute() override
  {
    stats->add_execute( 0_ms, target ); // log AA timer resets

    player->main_hand_attack->schedule_execute();

    if ( player->off_hand_attack )
      player->off_hand_attack->schedule_execute();
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
  timespan_t precombat_seconds;

  adrenaline_rush_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->talent.outlaw.adrenaline_rush ),
    precombat_seconds( 0_s )
  {
    add_option( opt_timespan( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = false;
    set_target( p );
  }

  void execute() override
  {
    rogue_spell_t::execute();

    // 2020-12-02 - Using over Celerity proc'ed AR does not extend but applies base duration.
    p()->buffs.adrenaline_rush->expire();
    p()->buffs.adrenaline_rush->trigger();
    if ( p()->talent.outlaw.loaded_dice->ok() )
      p()->buffs.loaded_dice->trigger();

    if ( precombat_seconds > 0_s && !p()->in_combat )
    {
      p()->cooldowns.adrenaline_rush->adjust( -precombat_seconds, false );
      p()->buffs.adrenaline_rush->extend_duration( p(), -precombat_seconds );
      p()->buffs.loaded_dice->extend_duration( p(), -precombat_seconds );
    }

    trigger_fatebound_edge_case( execute_state );
    trigger_supercharger();
  }
};

// Ambush ===================================================================

struct ambush_t : public rogue_attack_t
{
  struct hidden_opportunity_extra_attack_t : public rogue_attack_t
  {
    hidden_opportunity_extra_attack_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.hidden_opportunity_extra_attack )
    {
    }

    void impact( action_state_t* state ) override
    {
      rogue_attack_t::impact( state );
      trigger_tww1_outlaw_set_bonus( execute_state );
    }

    bool procs_main_gauche() const override
    { return true; }

    bool procs_blade_flurry() const override
    { return true; }
  };

  hidden_opportunity_extra_attack_t* extra_attack;
  ambush_t* audacity_extra_attack;

  ambush_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spell.ambush, options_str ),
    extra_attack( nullptr ), audacity_extra_attack( nullptr )
  {
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( p()->talent.outlaw.hidden_opportunity->ok() )
    {
      extra_attack = p()->get_secondary_trigger_action<hidden_opportunity_extra_attack_t>(
        secondary_trigger::HIDDEN_OPPORTUNITY, "ambush_hidden_opportunity" );

      if ( !is_secondary_action() )
      {
        add_child( extra_attack );

        // 2023-11-24 -- Self-trigger Ambush proc for Audacy override spell bug
        if ( p()->bugs && p()->talent.outlaw.audacity->ok() )
        {
          audacity_extra_attack = p()->get_secondary_trigger_action<ambush_t>(
            secondary_trigger::HIDDEN_OPPORTUNITY, "ambush_hidden_opportunity_audacity" );
          add_child( audacity_extra_attack );
        }
      }
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();
    trigger_blindside( execute_state );
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( !is_secondary_action() )
    {
      trigger_unseen_blade( state );
    }

    trigger_tww1_outlaw_set_bonus( execute_state );

    if ( p()->talent.outlaw.hidden_opportunity->ok() )
    {
      // 2023-11-24 -- The Audacity replacement spell currently triggers the normal Ambush spell from HO
      //               This allows it to trigger up to twice, rather than just a single time
      if ( p()->bugs && audacity_extra_attack && p()->buffs.audacity->check() )
        trigger_opportunity( state, audacity_extra_attack, p()->talent.outlaw.hidden_opportunity->effectN( 1 ).percent() );
      else
        trigger_opportunity( state, extra_attack, p()->talent.outlaw.hidden_opportunity->effectN( 1 ).percent() );
    }

    trigger_caustic_spatter_debuff( state );
    trigger_deathstalkers_mark_debuff( state );
  }

  bool procs_main_gauche() const override
  { return true; }

  bool procs_blade_flurry() const override
  { return true; }

  bool procs_deal_fate() const override
  { return true; }
};

// Backstab =================================================================

struct lingering_shadow_t : public rogue_attack_t
{
  lingering_shadow_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spec.lingering_shadow_attack )
  {
    base_dd_min = base_dd_max = 1; // Override from 0 for snapshot_flags
  }
};

struct backstab_t : public rogue_attack_t
{
  backstab_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spec.backstab, options_str )
  {
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( !is_secondary_action() )
    {
      if ( p()->active.weaponmaster.backstab )
      {
        add_child( p()->active.weaponmaster.backstab );
      }

      if ( p()->active.lingering_shadow && !p()->talent.subtlety.gloomblade->ok() )
      {
        add_child( p()->active.lingering_shadow );
      }

      if ( p()->active.echoing_reprimand && !p()->talent.subtlety.gloomblade->ok() )
      {
        add_child( p()->active.echoing_reprimand );
      }
    }
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_da_multiplier( state );

    if ( p()->position() == POSITION_BACK )
    {
      m *= 1.0 + data().effectN( 4 ).percent();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    trigger_perforated_veins( state );
    trigger_unseen_blade( state );
    trigger_weaponmaster( state, p()->active.weaponmaster.backstab );
    trigger_echoing_reprimand( state );

    if ( state->result == RESULT_CRIT && p()->talent.subtlety.improved_backstab->ok() && p()->position() == POSITION_BACK )
    {
      timespan_t duration = timespan_t::from_seconds( p()->talent.subtlety.improved_backstab->effectN( 1 ).base_value() );
      trigger_find_weakness( state, duration );
    }

    trigger_lingering_shadow( state );
  }

  bool verify_actor_spec() const override
  {
    if ( p()->talent.subtlety.gloomblade->ok() )
      return false;

    return rogue_attack_t::verify_actor_spec();
  }

  // 2024-9-04 -- Currently does not proc from Weaponmaster hits
  bool procs_nimble_flurry() const override
  { return !p()->bugs || !is_secondary_action(); }
};

// Dispatch =================================================================

struct dispatch_t: public rogue_attack_t
{
  dispatch_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spec.dispatch, options_str )
  {
  }

  double cost_flat_modifier() const override
  {
    double c = rogue_attack_t::cost_flat_modifier();

    if ( p()->buffs.summarily_dispatched->check() )
    {
      c += p()->buffs.summarily_dispatched->check() * p()->spec.summarily_dispatched_buff->effectN( 2 ).base_value();
    }

    return c;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p()->talent.outlaw.summarily_dispatched->ok() )
    {
      int cp = cast_state( execute_state )->get_combo_points();
      if ( cp >= p()->talent.outlaw.summarily_dispatched->effectN( 2 ).base_value() )
      {
        p()->buffs.summarily_dispatched->trigger();
      }
    }

    if ( !is_secondary_action() )
    {
      trigger_restless_blades( execute_state );
      trigger_hand_of_fate( execute_state, true );
      trigger_tww2_set_bonus_removal();
    }

    trigger_cut_to_the_chase( execute_state );
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );
    trigger_fate_intertwined( state );
  }

  bool ready() override
  {
    if ( p()->talent.trickster.coup_de_grace->ok() &&
         ( p()->buffs.escalating_blade->at_max_stacks() || p()->buffs.tww3_trickster_4pc->check() ) )
      return false;

    return rogue_attack_t::ready();
  }

  bool procs_main_gauche() const override
  { return true; }

  bool procs_blade_flurry() const override
  { return true; }
};

// Between the Eyes =========================================================

struct between_the_eyes_t : public rogue_attack_t
{
  between_the_eyes_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spec.between_the_eyes, options_str )
  {
    ap_type = attack_power_type::WEAPON_BOTH;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    trigger_restless_blades( execute_state );
    trigger_hand_of_fate( execute_state );

    if ( result_is_hit( execute_state->result ) )
    {
      const auto rs = cast_state( execute_state );
      const int cp_spend = rs->get_combo_points();

      // There is nothing about the buff duration in spell data, so we have to hardcode the 3s base.
      p()->buffs.between_the_eyes->trigger( 3_s * cp_spend );

      if ( p()->talent.outlaw.ace_up_your_sleeve->ok() )
      {
        if ( rng().roll( p()->talent.outlaw.ace_up_your_sleeve->effectN( 1 ).percent() * cp_spend ) )
        {
          trigger_combo_point_gain( as<int>( p()->spec.ace_up_your_sleeve_energize->effectN( 1 ).base_value() ),
                                    p()->gains.ace_up_your_sleeve );
        }
      }
    }
  }

  bool procs_poison() const override
  { return true; }

  bool procs_blade_flurry() const override
  { return true; }
};

// Blade Flurry =============================================================

struct blade_flurry_attack_t : public rogue_attack_t
{
  blade_flurry_attack_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spec.blade_flurry_attack )
  {
    radius = 5;
    range = -1.0;
    aoe = static_cast<int>( p->spec.blade_flurry->effectN( 3 ).base_value() +
                            p->talent.outlaw.dancing_steel->effectN( 3 ).base_value() );
    target_filter_callback = secondary_targets_only();
  }

  void init() override
  {
    rogue_attack_t::init();
    snapshot_flags |= STATE_TGT_MUL_DA;
  }

  bool procs_poison() const override
  { return true; }
};

struct blade_flurry_t : public rogue_attack_t
{
  struct blade_flurry_instant_attack_t : public rogue_attack_t
  {
    blade_flurry_instant_attack_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.blade_flurry_instant_attack )
    {
      range = -1.0;

      if ( p->talent.outlaw.deft_maneuvers->ok() )
      {
        energize_type = action_energize::PER_HIT;
        energize_resource = RESOURCE_COMBO_POINT;
        energize_amount = 1;
      }
    }

    double composite_da_multiplier( const action_state_t* state ) const override
    {
      double m = rogue_attack_t::composite_da_multiplier( state );

      if ( p()->talent.trickster.nimble_flurry->ok() && p()->buffs.flawless_form->check() )
      {
        m *= 1.0 + p()->spell.flawless_form_buff->effectN( 3 ).percent();
      }

      return m;
    }

    bool procs_main_gauche() const override
    { return true; }
  };

  blade_flurry_instant_attack_t* instant_attack;
  timespan_t precombat_seconds;

  blade_flurry_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spec.blade_flurry ),
    instant_attack( nullptr ),
    precombat_seconds( timespan_t::zero() )
  {
    add_option( opt_timespan( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );
    harmful = false;
    set_target( p ); // Does not require a target to use

    if ( p->spec.blade_flurry_attack->ok() )
    {
      add_child( p->active.blade_flurry );
    }

    if ( p->spec.blade_flurry_instant_attack->ok() )
    {
      instant_attack = p->get_background_action<blade_flurry_instant_attack_t>( "blade_flurry_instant_attack" );
      add_child( instant_attack );
    }
  }

  void execute() override
  {
    if ( precombat_seconds > timespan_t::zero() && !p()->in_combat )
    {
      p()->cooldowns.blade_flurry->adjust( -precombat_seconds, false );
    }

    timespan_t d = p()->buffs.blade_flurry->buff_duration();
    if ( precombat_seconds > timespan_t::zero() && !p()->in_combat )
      d -= precombat_seconds;

    rogue_attack_t::execute();
    p()->buffs.blade_flurry->trigger( d );

    // Don't trigger the attack if there are no targets to avoid breaking Stealth
    // Set target to invalidate the target cache prior to checking the list size
    if ( instant_attack && precombat_seconds <= timespan_t::zero() )
    {
      instant_attack->set_target( player->target );
      if ( !instant_attack->target_list().empty() )
        instant_attack->execute();
    }
  }

  bool breaks_stealth() const override
  { return false; }
};

// Blade Rush ===============================================================

struct blade_rush_t : public rogue_attack_t
{
  struct blade_rush_attack_t : public rogue_attack_t
  {
    blade_rush_attack_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.blade_rush_attack )
    {
      dual = true;
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
        m *= 1.0 + p()->talent.outlaw.blade_rush->effectN( 1 ).percent();

      return m;
    }

    bool procs_main_gauche() const override
    { return true; }
  };

  blade_rush_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.outlaw.blade_rush, options_str )
  {
    execute_action = p->get_background_action<blade_rush_attack_t>( "blade_rush_attack" );
    execute_action->stats = stats;
  }
};

// Crimson Tempest ==========================================================

struct crimson_tempest_t : public rogue_attack_t
{
  crimson_tempest_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.assassination.crimson_tempest, options_str )
  {
    aoe = -1;

    // 2025-04-01 -- Reduced AoE targets appear to be set at 8 despite the tooltip saying 5
    //               Value of 5 seems to only be used for the end of the bonus damage scaling
    reduced_aoe_targets = p->bugs ? 8 : data().effectN( 3 ).base_value();
    // 2025-04-01 -- Appears to now tick immediately on cast, not just on application
    tick_zero = p->bugs;

    if ( p->talent.deathstalker.follow_the_blood->ok() )
    {
      affected_by.follow_the_blood.periodic = true;
    }
  }

  double calculate_tick_amount( action_state_t* s, double m ) const override
  {
    auto n = std::clamp( as<double>( s->n_targets ), reduced_aoe_targets, 20.0 );

    m *= std::sqrt( reduced_aoe_targets / n );

    return rogue_attack_t::calculate_tick_amount( s, m );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    // 2023-10-24 -- Base duration was changed to 4s, scaling by 2s per CP (not in spell data)
    const auto rs = cast_state( s );
    timespan_t duration = data().duration() + ( 2_s * rs->get_combo_points() );
    duration *= 1.0 / rs->get_exsanguinated_rate();

    return duration;
  }

  double combo_point_da_multiplier( const action_state_t* s ) const override
  {
    return static_cast<double>( cast_state( s )->get_combo_points() ) + 1.0;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_persistent_multiplier( state );

    // Bonus damage from target count is snapshot into the action state at time of cast
    if ( state->n_targets > 1 )
    {
      auto num_targets = std::min( state->n_targets, as<unsigned>( data().effectN( 3 ).base_value() ) );
      m *= 1.0 + ( num_targets * data().effectN( 4 ).percent() );
    }

    return m;
  }

  bool procs_poison() const override
  { return true; }

  void execute() override
  {
    rogue_attack_t::execute();
    trigger_hand_of_fate( execute_state );
    trigger_tww2_set_bonus_removal();
  }
};

// Deathmark ================================================================

struct deathmark_t : public rogue_attack_t
{
  deathmark_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.assassination.deathmark, options_str )
  {
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( data().ok() )
    {
      add_child( p()->active.deathmark.garrote );
      add_child( p()->active.deathmark.rupture );
      add_child( p()->active.deathmark.amplifying_poison );
      add_child( p()->active.deathmark.deadly_poison_dot );
      add_child( p()->active.deathmark.deadly_poison_instant );
      add_child( p()->active.deathmark.instant_poison );
      add_child( p()->active.deathmark.wound_poison );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    auto tdata = td( state->target );
    tdata->debuffs.deathmark->trigger();
    
    // Deathmark clones all the relevant DoTs and debuffs present on the target when cast
    if ( tdata->dots.deadly_poison->is_ticking() )
      tdata->dots.deadly_poison->copy( state->target, DOT_COPY_CLONE, p()->active.deathmark.deadly_poison_dot );
    
    if ( tdata->dots.garrote->is_ticking() )
      tdata->dots.garrote->copy( state->target, DOT_COPY_CLONE, p()->active.deathmark.garrote );
    
    if ( tdata->dots.rupture->is_ticking() )
      tdata->dots.rupture->copy( state->target, DOT_COPY_CLONE, p()->active.deathmark.rupture );

    if ( tdata->debuffs.amplifying_poison->check() )
    {
      tdata->debuffs.amplifying_poison_deathmark->expire();
      tdata->debuffs.amplifying_poison_deathmark->trigger( tdata->debuffs.amplifying_poison->check(),
                                                           tdata->debuffs.amplifying_poison->remains() );
    }
  }

  void last_tick( dot_t* d ) override
  {
    rogue_attack_t::last_tick( d );

    // Debuffs triggered by Deathmark currently expire immediately when Deathmark fades
    auto tdata = td( d->target );
    tdata->dots.deadly_poison_deathmark->cancel();
    tdata->dots.garrote_deathmark->cancel();
    tdata->dots.rupture_deathmark->cancel();
    tdata->debuffs.amplifying_poison_deathmark->expire();

    p()->buffs.lingering_darkness->trigger();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    trigger_fatebound_edge_case( execute_state );
  }
};

// Detection ================================================================

// This ability does nothing but for some odd reasons throughout the history of Rogue spaghetti, we may want to look at using it. So, let's support it.
struct detection_t : public rogue_spell_t
{
  detection_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->spell.detection, options_str )
  {
    min_gcd = 750_ms; // Force 750ms min gcd because rogue player base has a 1s min.
    harmful = false;
    set_target( p );
  }
};

// Distract =================================================================

struct distract_t : public rogue_spell_t
{
  distract_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->talent.trickster.cloud_cover->ok() ? p->spell.cloud_cover_distract : p->spell.distract, options_str )
  {
    harmful = false;
    set_target( p );
  }

  void execute() override
  {
    rogue_spell_t::execute();
    p()->buffs.cloud_cover->trigger();
  }
};

// Envenom ==================================================================

struct envenom_t : public rogue_attack_t
{
  envenom_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spec.envenom, options_str )
  {
    dot_duration = timespan_t::zero();
    affected_by.lethal_dose = false;
    affected_by.darkest_night = affected_by.darkest_night_crit = true;

    if ( p->active.poison_bomb )
    {
      add_child( p->active.poison_bomb );
    }

    if ( p->active.fatebound.fate_intertwined )
    {
      add_child( p->active.fatebound.fate_intertwined );
    }
  }
  
  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_da_multiplier( state );

    if ( p()->talent.assassination.rapid_injection->ok() && p()->buffs.envenom->check() )
    {
      m *= 1.0 + p()->talent.assassination.rapid_injection->effectN( 1 ).percent() * p()->buffs.envenom->check();
    }

    // 2024-09-04 -- Although logically a target modifier, the mechanic is such that it consumes stacks on the target
    //               to deal increased base damage, rather than causing the target to take additional damage.
    if ( p()->talent.assassination.amplifying_poison->ok() )
    {
      const int consume_stacks = as<int>( p()->talent.assassination.amplifying_poison->effectN( 2 ).base_value() );
      rogue_td_t* tdata = td( state->target );

      if ( tdata->debuffs.amplifying_poison->stack() >= consume_stacks )
      {
        m *= 1.0 + p()->talent.assassination.amplifying_poison->effectN( 1 ).percent();
      }

      if ( tdata->debuffs.amplifying_poison_deathmark->stack() >= consume_stacks )
      {
        m *= 1.0 + p()->talent.assassination.amplifying_poison->effectN( 1 ).percent();
      }
    }

    return m;
  }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    rogue_attack_t::snapshot_internal( s, flags, rt );

    // 2025-04-25 -- Consume Amplifying Poison prior to trigger_poison in schedule_travel
    //               This prevents a case where we could consume stacks generated prior to impact
    // TOCHECK -- If this consumes on when dodged/parried
    if ( p()->talent.assassination.amplifying_poison->ok() )
    {
      const int consume_stacks = as<int>( p()->talent.assassination.amplifying_poison->effectN( 2 ).base_value() );
      rogue_td_t* tdata = td( target );

      if ( tdata->debuffs.amplifying_poison->stack() >= consume_stacks )
      {
        tdata->debuffs.amplifying_poison->decrement( consume_stacks );
        p()->procs.amplifying_poison_consumed->occur();
      }

      if ( tdata->debuffs.amplifying_poison_deathmark->stack() >= consume_stacks )
      {
        tdata->debuffs.amplifying_poison_deathmark->decrement( consume_stacks );
        p()->procs.amplifying_poison_deathmark_consumed->occur();
      }
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    trigger_poison_bomb( execute_state );
    trigger_hand_of_fate( execute_state, true );
    trigger_tww2_set_bonus_removal();
  }

  void impact( action_state_t* state ) override
  {
    // Trigger Envenom buff before impact() so that poison procs from Envenom itself benefit
    // 2023-10-05 -- Envenom spell no longer has a base 1s duration, hard code for now
    timespan_t envenom_duration = ( 1_s * cast_state( state )->get_combo_points() );

    // 2025-05-12 -- Envenom with Twist the Knife pandemics based on the stack it is replacing if at max stacks
    //               Needs custom duration because pandemics on async buffs is not supported in the core
    if ( p()->buffs.envenom->stack_behavior == buff_stack_behavior::ASYNCHRONOUS &&
         p()->buffs.envenom->expiration.size() == as<size_t>( p()->buffs.envenom->max_stack() ) )
    {
      auto current_remains = p()->buffs.envenom->expiration.front()->occurs() - sim->current_time();
      auto residual = std::min( envenom_duration * 0.3, current_remains );
      
      sim->print_debug( "{} {} carryover duration from ongoing async buff: {}, refresh_duration={} new_duration={}",
                        *p(), *p()->buffs.envenom, residual, envenom_duration, ( envenom_duration + residual ) );

      envenom_duration += residual;
    }

    p()->buffs.envenom->trigger( envenom_duration );

    rogue_attack_t::impact( state );

    trigger_cut_to_the_chase( state );
    trigger_fate_intertwined( state );
  }
};

// Eviscerate ===============================================================

struct eviscerate_t : public rogue_attack_t
{
  struct eviscerate_bonus_t : public rogue_attack_t
  {
    int last_cp;

    eviscerate_bonus_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.eviscerate_shadow_attack ),
      last_cp( 1 )
    {
      // 2024-08-12 -- Currently is not affected by the Darkest Night damage bonus
      affected_by.darkest_night = !p->bugs;
      // 2024-09-01 -- Note: This works but needs custom composite_crit_chance() handling below
      affected_by.darkest_night_crit = false;

      if ( p->talent.subtlety.shadowed_finishers->ok() )
      {
        // Spell has the full damage coefficient and is modified via talent scripting
        base_multiplier *= p->talent.subtlety.shadowed_finishers->effectN( 1 ).percent();
      }
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

    double composite_crit_chance() const override
    {
      double c = rogue_attack_t::composite_crit_chance();

      // Custom handling using last_eviscerate_cp snapshot CP value from the initial cast
      if ( p()->buffs.darkest_night->up() && last_cp >= p()->consume_cp_max() )
      {
        c += 1.0 + p()->spell.darkest_night_buff->effectN( 4 ).percent();
      }

      return c;
    }

    bool procs_nimble_flurry() const override
    { return true; }
  };

  eviscerate_bonus_t* bonus_attack;

  eviscerate_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ):
    rogue_attack_t( name, p, p->spec.eviscerate, options_str ),
    bonus_attack( nullptr )
  {
    affected_by.darkest_night = affected_by.darkest_night_crit = true;

    if ( p->talent.subtlety.shadowed_finishers->ok() )
    {
      bonus_attack = p->get_background_action<eviscerate_bonus_t>( "eviscerate_bonus" );
      add_child( bonus_attack );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    trigger_cut_to_the_chase( execute_state );
    trigger_tww2_set_bonus_removal();
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( bonus_attack && td( state->target )->debuffs.find_weakness->up() && result_is_hit( state->result ) )
    {
      bonus_attack->last_cp = cast_state( state )->get_combo_points();
      bonus_attack->execute_on_target( state->target );
    }
  }

  bool ready() override
  {
    if ( p()->talent.trickster.coup_de_grace->ok() &&
         ( p()->buffs.escalating_blade->at_max_stacks() || p()->buffs.tww3_trickster_4pc->check() ) )
      return false;

    return rogue_attack_t::ready();
  }

  bool procs_nimble_flurry() const override
  { return true; }
};

// Fan of Knives ============================================================

struct fan_of_knives_t: public rogue_attack_t
{
  fan_of_knives_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ):
    rogue_attack_t( name, p, p->spec.fan_of_knives, options_str )
  {
    energize_type     = action_energize::ON_HIT;
    energize_resource = RESOURCE_COMBO_POINT;
    energize_amount   = data().effectN( 2 ).base_value();
    // 2021-10-07 - Not in the whitelist but confirmed as working as of 9.1.5 PTR
    affected_by.shadow_blades_cp = true;

    aoe = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();

    if ( p->talent.deathstalker.clear_the_witnesses->ok() )
    {
      add_child( p->active.deathstalker.clear_the_witnesses );
    }

    if ( p->talent.deathstalker.follow_the_blood->ok() )
    {
      affected_by.follow_the_blood.direct = true;
    }
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p()->talent.assassination.flying_daggers.ok() )
    {
      if ( target_list().size() >= p()->talent.assassination.flying_daggers->effectN( 2 ).base_value() )
      {
        m *= 1.0 + p()->talent.assassination.flying_daggers->effectN( 1 ).percent();
      }
    }

    return m;
  }

  double composite_poison_flat_modifier( const action_state_t* state ) const override
  {
    if( !p()->bugs && p()->talent.assassination.thrown_precision->ok() && state->result == RESULT_CRIT )
      return 1.0;

    return rogue_attack_t::composite_poison_flat_modifier( state );
  }

  double composite_energize_amount( const action_state_t* state ) const override
  {
    double cp = rogue_attack_t::composite_energize_amount( state );
    cp += p()->buffs.clear_the_witnesses->check();
    return cp;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p()->buffs.clear_the_witnesses->check() )
    {
      p()->active.deathstalker.clear_the_witnesses->execute_on_target( execute_state->target );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( state->result == RESULT_CRIT && p()->talent.deathstalker.momentum_of_despair->ok() )
    {
      p()->buffs.momentum_of_despair->trigger();
    }
  }

  bool procs_poison() const override
  { return true; }

  bool procs_deal_fate() const override
  { return true; }
};

// Feint ====================================================================

struct feint_t : public rogue_attack_t
{
  feint_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ):
    rogue_attack_t( name, p, p->spell.feint, options_str )
  {
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p()->buffs.feint->trigger();
  }
};

// Garrote ==================================================================

struct garrote_t : public rogue_attack_t
{
  garrote_t( util::string_view name, rogue_t* p, const spell_data_t* s, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, s, options_str )
  {
  }

  void init() override
  {
    rogue_attack_t::init();

    // Secondary Deathmark triggers do not trigger base CP gain
    if ( secondary_trigger_type == secondary_trigger::DEATHMARK )
    {
      energize_type = action_energize::NONE;
      energize_amount = 0;
      energize_resource = RESOURCE_NONE;
    }
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_persistent_multiplier( state );

    if ( p()->talent.assassination.improved_garrote->ok() &&
         p()->stealthed( STEALTH_IMPROVED_GARROTE ) )
    {
      m *= 1.0 + p()->spec.improved_garrote_buff->effectN( 2 ).percent();
    }

    return m;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t duration = rogue_attack_t::composite_dot_duration( s );
    duration *= 1.0 / cast_state( s )->get_exsanguinated_rate();
    return duration;
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );
    trigger_venomous_wounds( d->state );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    // 2022-11-28 -- Currently does not work correctly at all without Improved Garrote
    //               Additionally works every global of Improved Garrote regardless of Subterfuge
    // 2023-11-14 -- Shrouded Suffocation is no longer triggered by Deathmark Garrotes
    if ( p()->talent.assassination.shrouded_suffocation->ok() && !is_secondary_action() &&
         ( !p()->bugs || p()->stealthed( STEALTH_IMPROVED_GARROTE ) ) &&
         ( p()->stealthed( STEALTH_BASIC | STEALTH_ROGUE ) ||
           ( p()->bugs && p()->stealthed( STEALTH_IMPROVED_GARROTE ) ) ) )
    {
      trigger_combo_point_gain( as<int>( p()->talent.assassination.shrouded_suffocation->effectN( 2 ).base_value() ),
                                p()->gains.shrouded_suffocation );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( !is_secondary_action() && td( state->target )->dots.deathmark->is_ticking() )
    {
      p()->active.deathmark.garrote->trigger_secondary_action( state->target );
    }
  }

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    if ( p()->talent.assassination.improved_garrote->ok() &&
         p()->stealthed( STEALTH_IMPROVED_GARROTE ) )
    {
      cd_duration = timespan_t::zero();
    }

    rogue_attack_t::update_ready( cd_duration );
  }
};

// Gouge =====================================================================

struct gouge_t : public rogue_attack_t
{
  gouge_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.rogue.gouge, options_str )
  {
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->is_boss() )
      return false;

    return rogue_attack_t::target_ready( candidate_target );
  }
};

// Gloomblade ===============================================================

struct gloomblade_t : public rogue_attack_t
{
  gloomblade_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.subtlety.gloomblade, options_str )
  {
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( !is_secondary_action() )
    {
      if ( p()->active.weaponmaster.gloomblade )
      {
        add_child( p()->active.weaponmaster.gloomblade );
      }

      if ( p()->active.lingering_shadow && p()->talent.subtlety.gloomblade->ok() )
      {
        add_child( p()->active.lingering_shadow );
      }

      if ( p()->active.echoing_reprimand && p()->talent.subtlety.gloomblade->ok() )
      {
        add_child( p()->active.echoing_reprimand );
      }
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    trigger_perforated_veins( state );
    trigger_unseen_blade( state );
    trigger_weaponmaster( state, p()->active.weaponmaster.gloomblade );
    trigger_echoing_reprimand( state );

    if ( state->result == RESULT_CRIT && p()->talent.subtlety.improved_backstab->ok() )
    {
      timespan_t duration = timespan_t::from_seconds( p()->talent.subtlety.improved_backstab->effectN( 1 ).base_value() );
      trigger_find_weakness( state, duration );
    }

    trigger_lingering_shadow( state );
  }

  bool procs_nimble_flurry() const override
  { return true; }
};

// Kick =====================================================================

struct kick_t : public rogue_attack_t
{
  kick_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) : 
    rogue_attack_t( name, p, p->spell.kick, options_str )
  {
    is_interrupt = true;
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target->debuffs.casting && !candidate_target->debuffs.casting->check() )
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
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( p()->talent.trickster.devious_distractions->ok() && weapon != nullptr && weapon->slot == SLOT_MAIN_HAND )
    {
      p()->get_target_data( state->target )->debuffs.fazed->trigger();
    }
  }

  bool procs_main_gauche() const override
  // 2025-07-14 -- TOCHECK: Killing Spree's offhand spell does not have an assigned weapon on PTR
  { return weapon != nullptr && weapon->slot == SLOT_MAIN_HAND; }

  // OH hits do not proc Combat Potency
  bool procs_fatal_flourish() const override
  { return false; }

  bool procs_blade_flurry() const override
  { return true; }
};

struct killing_spree_t : public rogue_attack_t
{
  melee_attack_t* attack_mh;
  melee_attack_t* attack_oh;

  killing_spree_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.outlaw.killing_spree, options_str ),
    attack_mh( nullptr ), attack_oh( nullptr )
  {
    channeled = tick_zero = true;
    interrupt_auto_attack = true; // 2025-06-28 -- TOCHECK: Auto attacks are now interrupted on PTR

    // Assume we can react to the ending of Killing Spree faster than the 250ms channel_lag setting
    // through the use of [nochannel] macros, or in some cases reacting to the combo point generation
    // to cancel it early with another action
    ability_lag = p->world_lag;

    attack_mh = p->get_background_action<killing_spree_tick_t>( "killing_spree_mh", p->spec.killing_spree_mh_attack );
    attack_oh = p->get_background_action<killing_spree_tick_t>( "killing_spree_oh", p->spec.killing_spree_oh_attack );
    add_child( attack_mh );
    add_child( attack_oh );

    snapshot_flags |= STATE_HASTE; // Core handling of this fails even though it is channeled due to no base dot_duration
  }

  timespan_t tick_time( const action_state_t* s ) const override
  { return data().effectN( 1 ).period() * s->haste; }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    auto rs = cast_state( s );
    int trigger_cp = rs->get_combo_points();

    // 2025-09-01 -- If Killing Spree consumes Supercharger, its duration loses an effective combo point
    //               So with Forced Induction, the duration is treated as +2 CPs as opposed to +3
    if ( p()->bugs && trigger_cp > rs->get_combo_points( true ) )
      trigger_cp -= 1;

    return tick_time( s ) * trigger_cp;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    trigger_restless_blades( execute_state );
    trigger_hand_of_fate( execute_state );
    p()->buffs.killing_spree->trigger( composite_dot_duration( execute_state ) );

    if ( p()->talent.trickster.flawless_form->ok() )
    {
      p()->buffs.flawless_form->execute();
    }

    if ( p()->talent.trickster.disorienting_strikes->ok() )
    {
      p()->buffs.disorienting_strikes->trigger();
    }
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    // 06-28-2025 -- TOCHECK: On 11.2 PTR both hits target random enemies
    // Additionally, the new damage spell 1248604 is not currently being used but contains the 0-9 yard cone
    attack_mh->execute_on_target( rng().range( sim->target_non_sleeping_list ) );
    attack_oh->execute_on_target( rng().range( sim->target_non_sleeping_list ) );

    if ( p()->spec.killing_spree_energize->ok() && d->current_tick > 0 )
    {
      trigger_combo_point_gain( as<int>( p()->spec.killing_spree_energize->effectN( 1 ).base_value() ), p()->gains.killing_spree );
    }
  }

  bool consumes_supercharger() const override
  { return true; }
};

// Kingsbane ================================================================

struct kingsbane_t : public rogue_attack_t
{
  kingsbane_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.assassination.kingsbane, options_str )
  {
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( result_is_hit( state->result ) && p()->set_bonuses.tww3_fatebound_2pc->ok() )
    {
      trigger_fatebound_edge_case( state );
    }

    trigger_supercharger();
  }

  void last_tick( dot_t* d ) override
  {
    rogue_attack_t::last_tick( d );
    p()->buffs.kingsbane->expire();
  }
};

// Pistol Shot ==============================================================

struct pistol_shot_t : public rogue_attack_t
{
  pistol_shot_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spec.pistol_shot, options_str )
  {
    ap_type = attack_power_type::WEAPON_BOTH;
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( !is_secondary_action() )
    {
      if ( p()->active.fan_the_hammer )
      {
        add_child( p()->active.fan_the_hammer );
      }
    }

    if ( secondary_trigger_type == secondary_trigger::FAN_THE_HAMMER )
    {
      energize_amount -= p()->talent.outlaw.fan_the_hammer->effectN( 3 ).base_value();
    }
  }

  double cost_pct_multiplier() const override
  {
    double c = rogue_attack_t::cost_pct_multiplier();

    if ( p()->buffs.opportunity->check() )
    {
      c *= 1.0 + p()->buffs.opportunity->data().effectN( 1 ).percent();
    }

    return c;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    m *= 1.0 + p()->buffs.opportunity->value();

    // 2023-09-26 Pistol shot says it does 20% less damage on followup fan the hammer hits, but currently does not
    if ( !p()->bugs && secondary_trigger_type == secondary_trigger::FAN_THE_HAMMER )
    {
      m *= 1.0 - p()->talent.outlaw.fan_the_hammer->effectN( 4 ).percent();
    }

    return m;
  }

  double generate_cp() const override
  {
    double g = rogue_attack_t::generate_cp();

    if ( p()->talent.outlaw.quick_draw->ok() && p()->buffs.opportunity->check() )
    {
      g += p()->talent.outlaw.quick_draw->effectN( 2 ).base_value();
    }

    return g;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    // Opportunity-Triggered Mechanics
    if ( p()->buffs.opportunity->check() )
    {
      if ( p()->talent.outlaw.quick_draw->ok() )
      {
        const int cp_gain = as<int>( p()->talent.outlaw.quick_draw->effectN( 2 ).base_value() );
        trigger_combo_point_gain( cp_gain, p()->gains.quick_draw );
      }

      // Audacity -- Currently on beta this can also trigger from Fan the Hammer procs
      if ( p()->talent.outlaw.audacity->ok() )
      {
        p()->buffs.audacity->trigger( 1, buff_t::DEFAULT_VALUE(), p()->extra_attack_proc_chance() );
      }
    }

    p()->buffs.opportunity->decrement();

    trigger_tww1_outlaw_set_bonus( execute_state );

    // Fan the Hammer
    if ( p()->active.fan_the_hammer && !is_secondary_action() )
    {
      unsigned int num_shots = std::min( p()->buffs.opportunity->check(),
                                         as<int>( p()->talent.outlaw.fan_the_hammer->effectN( 1 ).base_value() ) );
      for ( unsigned i = 0; i < num_shots; ++i )
      {
        p()->active.fan_the_hammer->trigger_secondary_action( execute_state->target, 0.1_s * ( 1 + i ) );
      }
    }
  }

  bool procs_fatal_flourish() const override
  { return true; }

  bool procs_poison() const override
  { return true; }

  bool procs_blade_flurry() const override
  { return true; }

  bool procs_seal_fate() const override
  { return false; }
};

// Main Gauche ==============================================================

struct main_gauche_t : public rogue_attack_t
{
  main_gauche_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->mastery.main_gauche_attack )
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

  bool procs_fatal_flourish() const override
  { return true; }

  bool procs_poison() const override
  { return true; }

  bool procs_blade_flurry() const override
  { return true; }
};

// Mutilate =================================================================

struct mutilate_t : public rogue_attack_t
{
  struct mutilate_strike_t : public rogue_attack_t
  {
    mutilate_strike_t( util::string_view name, rogue_t* p, const spell_data_t* s ) :
      rogue_attack_t( name, p, s )
    {
    }

    void impact( action_state_t* state ) override
    {
      rogue_attack_t::impact( state );
      trigger_doomblade( state );
    }

    bool procs_seal_fate() const override
    { return true; }

    bool procs_deal_fate() const override
    { return true; }
  };

  mutilate_strike_t* mh_strike;
  mutilate_strike_t* oh_strike;

  mutilate_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spec.mutilate, options_str ),
    mh_strike( nullptr ), oh_strike( nullptr )
  {
    if ( p->main_hand_weapon.type != WEAPON_DAGGER || p->off_hand_weapon.type != WEAPON_DAGGER )
    {
      sim->errorf( "Player %s attempting to execute Mutilate without two daggers equipped.", p->name() );
      background = true;
    }

    mh_strike = p->get_background_action<mutilate_strike_t>( "mutilate_mh", data().effectN( 3 ).trigger() );
    oh_strike = p->get_background_action<mutilate_strike_t>( "mutilate_oh", data().effectN( 4 ).trigger() );
    add_child( mh_strike );
    add_child( oh_strike );

    if ( p->active.doomblade )
    {
      add_child( p->active.doomblade );
    }

    if ( p->active.echoing_reprimand )
    {
      add_child( p->active.echoing_reprimand );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      // Triggered prior to secondary damage impacts
      trigger_caustic_spatter_debuff( execute_state );

      mh_strike->execute_on_target( execute_state->target );
      oh_strike->execute_on_target( execute_state->target );

      trigger_blindside( execute_state );
      trigger_echoing_reprimand( execute_state );
    }
  }

  bool has_amount_result() const override
  { return true; }

  bool ready() override
  {
    // Blindside overrides Mutilate with Ambush in the action bar when the buff is present
    if ( p()->buffs.blindside->check() )
      return false;

    return rogue_attack_t::ready();
  }
};

// Roll the Bones ===========================================================

struct roll_the_bones_t : public rogue_spell_t
{
  timespan_t precombat_seconds;

  roll_the_bones_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->spec.roll_the_bones ),
    precombat_seconds( 0_s )
  {
    add_option( opt_timespan( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = false;
    dot_duration = timespan_t::zero();
    set_target( p );
  }

  void execute() override
  {
    rogue_spell_t::execute();

    timespan_t d = p()->buffs.roll_the_bones->data().duration();
    if ( precombat_seconds > 0_s && !p()->in_combat )
      d -= precombat_seconds;

    p()->buffs.roll_the_bones->trigger( d );
  }
};

// Rupture ==================================================================

struct rupture_t : public rogue_attack_t
{
  struct replicating_shadows_tick_t : public rogue_attack_t
  {
    replicating_shadows_tick_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.replicating_shadows_tick )
    {
    }

    bool procs_poison() const override
    { return false; }

    result_amount_type amount_type( const action_state_t*, bool ) const override
    { return result_amount_type::DMG_OVER_TIME; } // Treat as Periodic flag
  };

  replicating_shadows_tick_t* replicating_shadows_tick;
  
  rupture_t( util::string_view name, rogue_t* p, const spell_data_t* s, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, s, options_str ),
    replicating_shadows_tick( nullptr )
  {
    if ( p->talent.subtlety.replicating_shadows->ok() )
    {
      // Manually apply AoE count as Replicating Shadows as talent has workaround +30 chain target value
      aoe = ( aoe == 0 ? 1 : 0 ) + as<int>( p->talent.subtlety.replicating_shadows->effectN( 4 ).base_value() );
      replicating_shadows_tick = p->get_background_action<replicating_shadows_tick_t>( "rupture_replicating_shadows" );
      add_child( replicating_shadows_tick );
    }
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( !is_secondary_action() && p()->talent.deathstalker.corrupt_the_blood->ok() )
    {
      add_child( p()->active.deathstalker.corrupt_the_blood );
    }
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    const auto rs = cast_state( s );
    timespan_t duration = data().duration() * ( 1 + rs->get_combo_points() );
    if ( p()->talent.deathstalker.corrupt_the_blood->ok() )
    {
      auto extra = p()->talent.deathstalker.corrupt_the_blood->effectN( 1 ).time_value();
      p()->sim->print_debug("{} duration adjusted from {} to {}", *this, duration, duration + extra);
      duration += extra;
    }
    duration *= 1.0 / rs->get_exsanguinated_rate();
    return duration;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_persistent_multiplier( state );
    return m;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    trigger_scent_of_blood();
    trigger_hand_of_fate( execute_state );

    if ( !is_secondary_action() && p()->set_bonuses.tww2_assassination_2pc->ok() )
    {
      trigger_tww2_set_bonus_removal();
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( !is_secondary_action() && td( state->target )->dots.deathmark->is_ticking() )
    {
      p()->active.deathmark.rupture->trigger_secondary_action( state->target,
                                                               cast_state( state )->get_combo_points() );
    }

    // 2023-10-05 -- Currently when triggerd by an ER cast, only uses base combo points
    if ( p()->active.internal_bleeding )
    {
      p()->active.internal_bleeding->trigger_secondary_action( state->target,
                                                               cast_state( state )->get_combo_points( p()->bugs ) );
    }

    // 2024-07-25 -- Rupture and Deathmark Rupture trigger one stack on cast
    if ( p()->active.deathstalker.corrupt_the_blood )
    {
      p()->active.deathstalker.corrupt_the_blood->execute_on_target( state->target );
    }
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );
    trigger_venomous_wounds( d->state );

    if ( replicating_shadows_tick )
    {
      replicating_shadows_tick->execute_on_target( d->target );
    }

    // 2024-07-25 -- Currently does not stack from Deathmark ticks
    if ( !is_secondary_action() && p()->active.deathstalker.corrupt_the_blood )
    {
      p()->active.deathstalker.corrupt_the_blood->execute_on_target( d->target );
    }
  }

  void last_tick( dot_t* d ) override
  {
    rogue_attack_t::last_tick( d );

    // Delay to allow the demise to reset() and update get_active_dots() count
    make_event( *p()->sim, [this] { trigger_scent_of_blood(); } );
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
          double duration = base_duration * ( 1.0 + std::min( cp_max_spend, rupture->p()->current_cp() ) );
          return std::min( duration * 1.3, duration + rupture->td( rupture->target )->dots.rupture->remains().total_seconds() );
        }
      };

      return std::make_unique<new_duration_expr_t>( this );
    }

    return rogue_attack_t::create_expression( name );
  }

  void trigger_scent_of_blood()
  {
    if ( !p()->talent.assassination.scent_of_blood->ok() )
      return;

    const int current_stacks = p()->buffs.scent_of_blood->check();
    int desired_stacks = p()->get_active_dots( td( this->target )->dots.rupture );
    if ( p()->active.deathmark.rupture )
    {
      desired_stacks += p()->get_active_dots( td( this->target )->dots.rupture_deathmark );
    }
    desired_stacks = std::min( p()->buffs.scent_of_blood->max_stack(),
                               desired_stacks * as<int>( p()->talent.assassination.scent_of_blood->effectN( 1 ).base_value() ) );

    if ( current_stacks != desired_stacks )
    {
      p()->sim->print_log( "{} adjusting Scent of Blood stacks from {} to {}",
                           *p(), current_stacks, desired_stacks );
    }

    if ( desired_stacks < current_stacks )
    {
      p()->buffs.scent_of_blood->decrement( current_stacks - desired_stacks );
    }
    else if ( desired_stacks > current_stacks )
    {
      p()->buffs.scent_of_blood->increment( desired_stacks - current_stacks );
    }
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
      aoe = -1;
      full_amount_targets = 1; // 2025-05-30 -- Primary target is not reduced by sqrt scaling
      reduced_aoe_targets = p->spec.secret_technique->effectN( 6 ).base_value() - 1;
    }

    double composite_player_multiplier( const action_state_t* state ) const override
    {
      double m = rogue_attack_t::composite_player_multiplier( state );

      // 2024-09-16 -- Lingering Darkness does not work on pet clone attacks
      if ( p()->bugs && secondary_trigger_type == secondary_trigger::SECRET_TECHNIQUE_CLONE &&
           p()->buffs.lingering_darkness->check() )
      {
        m /= 1.0 + p()->buffs.lingering_darkness->check_value() *
          ( 1.0 + p()->buffs.tww3_deathstalker_2pc->check() * p()->spec.tww3_deathstalker_2pc_buff->effectN( 2 ).percent() );
      }

      return m;
    }

    double composite_da_multiplier( const action_state_t* state ) const override
    {
      double m = rogue_attack_t::composite_da_multiplier( state );

      if ( secondary_trigger_type == secondary_trigger::SECRET_TECHNIQUE_CLONE )
      {
        // Secret Technique clones count as pets and benefit from pet modifiers
        m *= p()->cache.pet_damage_multiplier( state, true );
      }

      return m;
    }

    double combo_point_da_multiplier( const action_state_t* state ) const override
    {
      return static_cast<double>( cast_state( state )->get_combo_points() );
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = rogue_attack_t::composite_target_multiplier( target );

      if ( target != this->target )
        m *= p()->spec.secret_technique->effectN( 3 ).percent();

      return m;
    }

    void impact( action_state_t* state ) override
    {
      rogue_attack_t::impact( state );

      if ( p()->talent.trickster.devious_distractions->ok() )
      {
        p()->get_target_data( state->target )->debuffs.fazed->trigger();
      }
    }

    // 2023-10-02 -- Clone attacks do not trigger Shadow Blades proc damage
    // 2025-02-08 -- Fixed on 11.1 PTR
    bool procs_shadow_blades_damage() const override
    { return true; }
  };

  secret_technique_attack_t* player_attack;
  secret_technique_attack_t* clone_attack;

  secret_technique_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spec.secret_technique, options_str )
  {
    may_miss = false;

    player_attack = p->get_secondary_trigger_action<secret_technique_attack_t>(
      secondary_trigger::SECRET_TECHNIQUE, "secret_technique_player", p->spec.secret_technique_attack );
    player_attack->update_flags &= ~STATE_CRIT; // Hotfixed to snapshot in Cold Blood on delayed attacks
    clone_attack = p->get_secondary_trigger_action<secret_technique_attack_t>(
      secondary_trigger::SECRET_TECHNIQUE_CLONE, "secret_technique_clones", p->spec.secret_technique_clone_attack );
    clone_attack->update_flags &= ~STATE_CRIT; // Hotfixed to snapshot in Cold Blood on delayed clone attacks

    add_child( player_attack );
    add_child( clone_attack );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    int cp = cast_state( execute_state )->get_combo_points();

    // All attacks need to snapshot the state here due to the delayed attacks snapshotting Cold Blood
    // Hit of the main char happens right after the primary cast.
    auto player_state = player_attack->get_state();
    player_state->target = execute_state->target;
    player_attack->cast_state( player_state )->set_combo_points( cp, cp );
    player_attack->snapshot_internal( player_state, player_attack->snapshot_flags, player_attack->amount_type( player_state ) );
    player_attack->trigger_secondary_action( player_state );

    // The clones seem to hit 1s and 1.3s later (no time reference in spell data though)
    // Trigger tracking buff for APL conditions until final clone's damage
    auto clone_state = clone_attack->get_state();
    clone_state->target = execute_state->target;
    clone_attack->cast_state( clone_state )->set_combo_points( cp, cp );
    clone_attack->snapshot_internal( clone_state, clone_attack->snapshot_flags, clone_attack->amount_type( clone_state ) );
    auto clone_state_2 = clone_attack->get_state( clone_state );

    clone_attack->trigger_secondary_action( clone_state, 1_s );
    clone_attack->trigger_secondary_action( clone_state_2, 1.3_s );
    p()->buffs.secret_technique->trigger( 1.3_s );

    if ( p()->talent.trickster.flawless_form->ok() )
    {
      p()->buffs.flawless_form->execute();
    }

    if ( p()->talent.trickster.disorienting_strikes->ok() )
    {
      p()->buffs.disorienting_strikes->trigger();
    }

    p()->buffs.tww1_subtlety_4pc->trigger();
    trigger_tww2_set_bonus_removal();
  }

  bool consumes_supercharger() const override
  { return true; }
};

// Shadow Blades ============================================================

struct shadow_blades_attack_t : public rogue_attack_t
{
  shadow_blades_attack_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spec.shadow_blades_attack )
  {
    base_dd_min = base_dd_max = 1; // Override from 0 for snapshot_flags
    attack_power_mod.direct = 0; // Base 0.1 mod overwritten by proc damage
  }

  bool procs_shadow_blades_damage() const override
  { return false; }
};

struct shadow_blades_t : public rogue_spell_t
{
  timespan_t precombat_seconds;

  shadow_blades_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->talent.subtlety.shadow_blades ),
    precombat_seconds( 0_s )
  {
    add_option( opt_timespan( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = false;
    school = SCHOOL_SHADOW;
    set_target( p );

    add_child( p->active.shadow_blades_attack );
  }

  void execute() override
  {
    rogue_spell_t::execute();

    // 2022-02-07 -- Updated to extend existing buffs on hard-casts in 9.2
    p()->buffs.shadow_blades->extend_duration_or_trigger();

    if ( precombat_seconds > 0_s && !p()->in_combat )
    {
      p()->cooldowns.shadow_blades->adjust( -precombat_seconds, false );
      p()->buffs.shadow_blades->extend_duration( p(), -precombat_seconds );
    }
  }
};

// Shadow Dance =============================================================

struct shadow_dance_t : public rogue_spell_t
{
  shadow_dance_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->spec.shadow_dance, options_str )
  {
    harmful = false;
    dot_duration = timespan_t::zero(); // No need to have a tick here
    set_target( p );
  }

  void execute() override
  {
    rogue_spell_t::execute();
    p()->buffs.shadow_dance->trigger();
    trigger_master_of_shadows();
    trigger_supercharger();
  }

  bool ready() override
  {
    // Cannot dance during stealth. Vanish works.
    if ( p()->buffs.stealth->check() )
      return false;

    return rogue_spell_t::ready();
  }
};

// Shadowstep ===============================================================

struct shadowstep_t : public rogue_spell_t
{
  shadowstep_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->spell.shadowstep, options_str )
  {
    harmful = false;
    dot_duration = timespan_t::zero();
    base_teleport_distance = data().max_range();
    movement_directionality = movement_direction_type::OMNI;
  }

  void execute() override
  {
    rogue_spell_t::execute();
    p()->buffs.shadowstep->trigger();
  }

  void update_ready( timespan_t cd_duration ) override
  {
    if ( p()->talent.assassination.intent_to_kill->ok() )
    {
      if ( td( target )->dots.garrote->is_ticking() )
      {
        if ( cd_duration < 0_ms )
        {
          cd_duration = cooldown->duration;
        }
          
        cd_duration *= ( 1.0 - p()->talent.assassination.intent_to_kill->effectN( 1 ).percent() );
      }
    }

    rogue_spell_t::update_ready( cd_duration );
  }
};

// Shadowstrike =============================================================

struct shadowstrike_t : public rogue_attack_t
{
  shadowstrike_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spec.shadowstrike, options_str )
  {
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( !is_secondary_action() )
    {
      if ( p()->active.weaponmaster.shadowstrike )
        add_child( p()->active.weaponmaster.shadowstrike );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    trigger_unseen_blade( state );
    trigger_weaponmaster( state, p()->active.weaponmaster.shadowstrike );
    trigger_find_weakness( state );
    trigger_deathstalkers_mark_debuff( state );
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p()->stealthed( STEALTH_BASIC ) )
    {
      m *= 1.0 + p()->spec.shadowstrike_stealth_buff->effectN( 2 ).percent();
    }

    return m;
  }

  // TODO: Distance movement support with distance targeting, next to the target
  double composite_teleport_distance( const action_state_t* ) const override
  {
    if ( is_secondary_action() )
    {
      return 0;
    }

    if ( p()->stealthed( STEALTH_BASIC ) )
    {
      return range + p()->spec.shadowstrike_stealth_buff->effectN( 1 ).base_value();
    }

    return 0;
  }

  // 2024-9-04 -- Currently does not proc from Weaponmaster hits
  bool procs_nimble_flurry() const override
  { return !p()->bugs || !is_secondary_action(); }
};

// Black Powder =============================================================

struct black_powder_t: public rogue_attack_t
{
  struct black_powder_bonus_t : public rogue_attack_t
  {
    int last_cp;

    black_powder_bonus_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.black_powder_shadow_attack ),
      last_cp( 1 )
    {
      callbacks = false; // 2021-07-19 -- Does not appear to trigger normal procs
      aoe = -1;
      reduced_aoe_targets = p->spec.black_powder->effectN( 4 ).base_value();

      if ( p->talent.subtlety.shadowed_finishers->ok() )
      {
        // Spell has the full damage coefficient and is modified via talent scripting
        base_multiplier *= p->talent.subtlety.shadowed_finishers->effectN( 1 ).percent();
      }

      if ( p->talent.deathstalker.follow_the_blood->ok() )
      {
        affected_by.follow_the_blood.direct = !p->bugs; // 2024-08-12 -- Currently does not work
      }
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

    size_t available_targets( std::vector< player_t* >& tl ) const override
    {
      rogue_attack_t::available_targets( tl );

      // Can only hit targets with the Find Weakness debuff
      range::erase_remove( tl, [ this ]( player_t* t ) { return !td( t )->debuffs.find_weakness->check(); } );

      return tl.size();
    }

    void execute() override
    {
      // Invalidate target cache to force re-checking Find Weakness debuffs.
      // Don't attempt to execute this attack if it has no valid targets
      target_cache.is_valid = false;
      if ( !target_list().empty() )
      {
        rogue_attack_t::execute();
      }
    }
  };

  black_powder_bonus_t* bonus_attack;

  black_powder_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ):
    rogue_attack_t( name, p, p->spec.black_powder, options_str ),
    bonus_attack( nullptr )
  {
    aoe = -1;
    reduced_aoe_targets = p->spec.black_powder->effectN( 4 ).base_value();

    if ( p->talent.subtlety.shadowed_finishers->ok() )
    {
      bonus_attack = p->get_background_action<black_powder_bonus_t>( "black_powder_bonus" );
      add_child( bonus_attack );
    }

    if ( p->talent.deathstalker.follow_the_blood->ok() )
    {
      affected_by.follow_the_blood.direct = true;
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( bonus_attack )
    {
      bonus_attack->last_cp = cast_state( execute_state )->get_combo_points();
      bonus_attack->execute_on_target( execute_state->target );
    }

    trigger_tww2_set_bonus_removal();
  }

  bool procs_poison() const override
  { return true; }

  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override
  {
    if ( util::str_compare_ci( name_str, "fw_targets" ) )
    {
      return make_fn_expr( name_str, [ this ]() {
        auto count = range::count_if( rogue_attack_t::target_list(), [ this ]( player_t* t ) {
          return this->td( t )->debuffs.find_weakness->check();
        } );
        return count;
      } );
    }

    return rogue_attack_t::create_expression( name_str );
  }
};

// Shuriken Storm ===========================================================

struct shuriken_storm_t: public rogue_attack_t
{
  action_t* clear_the_witnesses;

  shuriken_storm_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ):
    rogue_attack_t( name, p, p->spec.shuriken_storm, options_str ),
    clear_the_witnesses( nullptr )
  {
    energize_type = action_energize::PER_HIT;
    energize_resource = RESOURCE_COMBO_POINT;
    energize_amount = 1;
    ap_type = attack_power_type::WEAPON_BOTH;
    // 2021-04-22- Not in the whitelist but confirmed as working in-game
    affected_by.shadow_blades_cp = true;

    aoe = -1;
    reduced_aoe_targets = data().effectN( 4 ).base_value();

    if ( p->talent.deathstalker.follow_the_blood->ok() )
    {
      affected_by.follow_the_blood.direct = true;
    }
  }

  void init() override
  {
    rogue_attack_t::init();

    clear_the_witnesses = secondary_trigger_type == secondary_trigger::SHURIKEN_TORNADO ?
      p()->active.deathstalker.clear_the_witnesses_tornado :
      p()->active.deathstalker.clear_the_witnesses;

    if ( clear_the_witnesses )
    {
      add_child( clear_the_witnesses );
    }
  }

  double composite_crit_chance() const override
  {
    double c = rogue_attack_t::composite_crit_chance();    
    c += p()->buffs.silent_storm->stack_value();
    return c;
  }

  double composite_energize_amount( const action_state_t* state ) const override
  {
    double cp = rogue_attack_t::composite_energize_amount( state );
    cp += p()->buffs.clear_the_witnesses->check();
    return cp;
  }

  void execute() override
  {
    rogue_attack_t::execute();
    
    p()->buffs.silent_storm->expire();

    if ( clear_the_witnesses && p()->buffs.clear_the_witnesses->check() )
    {
      clear_the_witnesses->execute_on_target( execute_state->target );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( state->result == RESULT_CRIT && p()->talent.subtlety.improved_shuriken_storm->ok() )
    {
      timespan_t duration = timespan_t::from_seconds( p()->talent.subtlety.improved_shuriken_storm->effectN( 1 ).base_value() );
      trigger_find_weakness( state, duration );
    }

    if ( state->result == RESULT_CRIT && p()->talent.deathstalker.momentum_of_despair->ok() )
    {
      p()->buffs.momentum_of_despair->trigger();
    }
  }

  bool procs_poison() const override
  { return true; }
};

// Shuriken Tornado =========================================================

struct shuriken_tornado_t : public rogue_spell_t
{
  shuriken_tornado_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->talent.subtlety.shuriken_tornado, options_str )
  {
    dot_duration = timespan_t::zero();
    aoe = -1;

    // Trigger action is created in the buff, but it can't be parented then, just look it up here
    action_t* trigger_action = p->find_secondary_trigger_action<shuriken_storm_t>( secondary_trigger::SHURIKEN_TORNADO );
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
  shuriken_toss_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spec.shuriken_toss, options_str )
  {
    ap_type = attack_power_type::WEAPON_BOTH;
  }

  bool procs_seal_fate() const override
  { return false; }
};

// Sinister Strike ==========================================================

struct sinister_strike_t : public rogue_attack_t
{
  struct sinister_strike_extra_attack_t : public rogue_attack_t
  {
    sinister_strike_extra_attack_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.sinister_strike_extra_attack )
    {
      energize_type = action_energize::ON_HIT;
      energize_resource = RESOURCE_COMBO_POINT;
    }

    double composite_energize_amount( const action_state_t* ) const override
    {
      // CP generation is not in the spell data and the extra SS procs seem script-driven
      return ( secondary_trigger_type == secondary_trigger::SINISTER_STRIKE ) ? 1 : 0;
    }

    void impact( action_state_t* state ) override
    {
      rogue_attack_t::impact( state );
      trigger_tww1_outlaw_set_bonus( execute_state );
    }

    bool procs_main_gauche() const override
    { return true; }

    bool procs_blade_flurry() const override
    { return true; }
  };

  sinister_strike_extra_attack_t* extra_attack;

  sinister_strike_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spec.sinister_strike, options_str )
  {
    extra_attack = p->get_secondary_trigger_action<sinister_strike_extra_attack_t>(
      secondary_trigger::SINISTER_STRIKE, "sinister_strike_extra_attack" );
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( !is_secondary_action() )
    {
      add_child( extra_attack );
      if ( p()->active.echoing_reprimand )
        add_child( p()->active.echoing_reprimand );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();
    trigger_unseen_blade( execute_state );
    trigger_opportunity( execute_state, extra_attack );
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );
    trigger_tww1_outlaw_set_bonus( execute_state );
    trigger_echoing_reprimand( state );
  }

  bool procs_main_gauche() const override
  { return true; }

  bool procs_blade_flurry() const override
  { return true; }
};

// Slice and Dice ===========================================================

struct slice_and_dice_t : public rogue_spell_t
{
  timespan_t precombat_seconds;

  slice_and_dice_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->spell.slice_and_dice ),
    precombat_seconds( 0_s )
  {
    add_option( opt_timespan( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = false;
    dot_duration = timespan_t::zero();
    set_target( p );
  }

  timespan_t get_triggered_duration( int cp )
  {
    return ( cp + 1 ) * p()->buffs.slice_and_dice->data().duration();
  }

  void execute() override
  {
    rogue_spell_t::execute();

    trigger_restless_blades( execute_state );
    trigger_hand_of_fate( execute_state );

    int cp = cast_state( execute_state )->get_combo_points();
    timespan_t snd_duration = get_triggered_duration( cp );

    if ( precombat_seconds > 0_s && !p()->in_combat )
      snd_duration -= precombat_seconds;

    p()->buffs.slice_and_dice->trigger( snd_duration );
  }

  bool ready() override
  {
    // Grand Melee prevents refreshing if there would be a reduction in the post-pandemic buff duration
    if ( p()->buffs.slice_and_dice->remains() > get_triggered_duration( as<int>( p()->current_effective_cp( false ) ) ) * 1.3 )
      return false;

    return rogue_spell_t::ready();
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override
  {
    if ( util::str_compare_ci( name_str, "refreshable" ) )
      return make_fn_expr( name_str, [ this ]() {
        return p()->buffs.slice_and_dice->remains() < get_triggered_duration( as<int>( p()->current_effective_cp( false ) ) ) * 0.3;
      } );

    return rogue_spell_t::create_expression( name_str );
  }
};

// Sprint ===================================================================

struct sprint_t : public rogue_spell_t
{
  sprint_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ):
    rogue_spell_t( name, p, p->spell.sprint, options_str )
  {
    harmful = callbacks = false;
    cooldown = p->cooldowns.sprint;
    set_target( p );
  }

  void execute() override
  {
    rogue_spell_t::execute();
    p()->buffs.sprint->trigger();
  }
};

// Shiv =====================================================================

struct shiv_t : public rogue_attack_t
{
  shiv_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.rogue.shiv, options_str )
  {
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p()->buffs.symbolic_victory->trigger();
  }

  void impact( action_state_t* s ) override
  {
    rogue_attack_t::impact( s );

    if ( p()->specialization() == ROGUE_ASSASSINATION )
    {
      p()->buffs.tww3_deathstalker_2pc->trigger();
      if ( p()->set_bonuses.tww3_deathstalker_4pc->ok() )
      {
        trigger_deathstalkers_mark( s, true );
      }
    }
  }

  bool procs_fatal_flourish() const override
  { return true; }

  bool procs_blade_flurry() const override
  { return true; }
};

// Vanish ===================================================================

struct vanish_t : public rogue_spell_t
{
  vanish_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->spell.vanish, options_str )
  {
    harmful = false;
    set_target( p );
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

// Stealth ==================================================================

struct stealth_t : public rogue_spell_t
{
  timespan_t precombat_seconds;

  stealth_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->spell.stealth ),
    precombat_seconds( 0_s )
  {
    add_option( opt_timespan( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );
    harmful = false;
    set_target( p );
  }

  void execute() override
  {
    rogue_spell_t::execute();
    p()->buffs.stealth->trigger();
    trigger_master_of_shadows();

    if ( precombat_seconds > 0_s && !p()->in_combat )
    {
      p()->cooldowns.stealth->adjust( -precombat_seconds, false );
      p()->buffs.stealth->cooldown->adjust( -precombat_seconds, false ); // buff cd somehow seperate from skill cd
    }
  }

  bool ready() override
  {
    if ( p()->stealthed( STEALTH_BASIC | STEALTH_ROGUE ) )
      return false;

    if ( !p()->in_combat )
      return rogue_spell_t::ready(); // respect cd

    // Allow restealth for Dungeon sims against non-boss targets as Shadowmeld drops combat against trash.
    if ( ( p()->sim->fight_style == FIGHT_STYLE_DUNGEON_SLICE || p()->sim->fight_style == FIGHT_STYLE_DUNGEON_ROUTE ) &&
         p()->player_t::buffs.shadowmeld->check() && !p()->target->is_boss() )
      return rogue_spell_t::ready(); // respect cd

    if ( !p()->restealth_allowed )
      return false;

    if ( !p()->sim->target_non_sleeping_list.empty() )
      return false;

    return rogue_spell_t::ready();
  }
};

// Internal Bleeding ========================================================

struct internal_bleeding_t : public rogue_attack_t
{
  internal_bleeding_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spec.internal_bleeding_debuff )
  {
    affected_by.lethal_dose = true;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t duration = rogue_attack_t::composite_dot_duration( s );
    duration *= 1.0 / cast_state( s )->get_exsanguinated_rate();
    return duration;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_persistent_multiplier( state );
    m *= std::max( 1, cast_state( state )->get_combo_points() );
    return m;
  }
};

// Kidney Shot ==============================================================

struct kidney_shot_t : public rogue_attack_t
{
  kidney_shot_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spell.kidney_shot, options_str )
  {
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( !state->target->is_boss() && p()->active.internal_bleeding )
    {
      // 2023-10-05 -- Currently when triggerd by an ER cast, only uses base combo points
      p()->active.internal_bleeding->trigger_secondary_action( state->target,
                                                               cast_state( state )->get_combo_points( p()->bugs ) );
    }
  }
};

// Cheap Shot ===============================================================

struct cheap_shot_t : public rogue_attack_t
{
  cheap_shot_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spell.cheap_shot, options_str )
  {
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();
    if ( p()->buffs.shot_in_the_dark->up() )
      return 0.0;
    return c;
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );
    trigger_find_weakness( state );
  }
};

// Doomblade ================================================================

// Note: Uses spell_t instead of rogue_spell_t to avoid action_state casting issues
struct doomblade_t : public residual_action::residual_periodic_action_t<spell_t>
{
  rogue_t* rogue;

  doomblade_t( util::string_view name, rogue_t* p ) :
    residual_action_t( name, p, p->spec.doomblade_debuff ), rogue( p )
  {
    dual = true;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = residual_action_t::composite_da_multiplier( state );

    // 2023-11-10 -- Doomblade is intended to be dynamically affected by Lethal Dose
    if ( rogue->talent.assassination.lethal_dose->ok() )
    {
      m *= 1.0 + ( rogue->talent.assassination.lethal_dose->effectN( 1 ).percent() *
                   rogue->get_target_data( state->target )->lethal_dose_count() );
    }

    return m;
  }
};

// Poison Bomb ==============================================================

struct poison_bomb_t : public rogue_attack_t
{
  poison_bomb_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spec.poison_bomb_damage )
  {
    aoe = -1;
  }
};

// Caustic Spatter ==========================================================

struct caustic_spatter_t : public rogue_attack_t
{
  caustic_spatter_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spec.caustic_spatter_damage )
  {
    aoe = -1;
    reduced_aoe_targets = p->spec.caustic_spatter_buff->effectN( 2 ).base_value();
    target_filter_callback = secondary_targets_only();
  }

  void init() override
  {
    rogue_attack_t::init();
    snapshot_flags |= STATE_TGT_MUL_DA;
  }

  bool procs_poison() const override
  { return false; }

  bool procs_caustic_spatter() const override
  { return false; }
};

// Poisoned Knife ===========================================================

struct poisoned_knife_t : public rogue_attack_t
{
  poisoned_knife_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spec.poisoned_knife, options_str )
  {
  }

  double composite_poison_flat_modifier( const action_state_t* ) const override
  { return 1.0; }

  bool procs_seal_fate() const override
  { return false; }
};

// Thistle Tea ==============================================================

struct thistle_tea_t : public rogue_spell_t
{
  double precombat_seconds;

  thistle_tea_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->spell.thistle_tea ),
    precombat_seconds( 0.0 )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = false;
    energize_type = action_energize::ON_CAST;

    // If Thistle Tea is not actually in the APL, use the thistle_tea_auto data instead
    if ( !util::str_compare_ci( name, "thistle_tea" ) && !p->find_action( "thistle_tea" ) )
    {
      p->cooldowns.thistle_tea->duration = cooldown->duration;
      p->cooldowns.thistle_tea->charges = cooldown->charges;
    }

    cooldown = p->cooldowns.thistle_tea;
    set_target( p );
  }

  void execute() override
  {
    rogue_spell_t::execute();

    if ( precombat_seconds && !p()->in_combat )
    {
      p()->cooldowns.thistle_tea->adjust( -timespan_t::from_seconds( precombat_seconds ), false );
    }

    p()->buffs.thistle_tea->trigger();
  }
};

// Keep it Rolling ==========================================================

struct keep_it_rolling_t : public rogue_spell_t
{
  keep_it_rolling_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->talent.outlaw.keep_it_rolling, options_str )
  {
    harmful = false;
    set_target( p );
  }

  void execute() override
  {
    rogue_spell_t::execute();
    trigger_keep_it_rolling();
  }
};

// Goremaw's Bite ===========================================================

struct goremaws_bite_t : public rogue_attack_t
{
  struct goremaws_bite_damage_t : public rogue_attack_t
  {
    goremaws_bite_damage_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
      rogue_attack_t( name, p, p->talent.subtlety.goremaws_bite->effectN( 1 ).trigger(), options_str )
    {
      dual = true;
    }
  };

  goremaws_bite_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.subtlety.goremaws_bite, options_str )
  {
    impact_action = p->get_background_action<goremaws_bite_damage_t>( "goremaws_bite_damage" );
    impact_action->stats = stats;
    stats->school = impact_action->school; // Fix HTML Reporting

    // 2023-10-02 -- CP gen is technically in the buff, move to the ability for expression and event handling
    affected_by.shadow_blades_cp = true; // Buff is affected by Shadow Blades label 
    affected_by.danse_macabre = true; // Triggers a stack, but damage spell is not in the whitelist
    energize_type = action_energize::ON_HIT;
    energize_resource = RESOURCE_COMBO_POINT;
    energize_amount = p->spec.goremaws_bite_buff->effectN( 1 ).base_value();
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p()->buffs.goremaws_bite->trigger();
  }

  bool procs_nimble_flurry() const override
  { return true; }
};

// Echoing Reprimand ========================================================

struct echoing_reprimand_t : public rogue_attack_t
{
  echoing_reprimand_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spell.echoing_reprimand_damage )
  {
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p()->buffs.echoing_reprimand->expire();
  }
};

// Deathstalker =============================================================

struct clear_the_witnesses_t : public rogue_attack_t
{
  clear_the_witnesses_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spell.clear_the_witnesses_damage )
  {
    aoe = -1;
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p()->buffs.clear_the_witnesses->expire();
  }

  bool procs_shadow_blades_damage() const override
  { return false; }
};

struct corrupt_the_blood_t : public rogue_attack_t
{
  corrupt_the_blood_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spell.corrupt_the_blood_damage )
  {
    affected_by.zoldyck_insignia = true; // 2025-03-05 -- Confirmed to work via testing
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_da_multiplier( state );

    // 2024-09-04 -- Appears based on how secondary effects that reverse target modifiers out that the
    //               stacking bonus is not handled as a target modifier, even if it logically should be
    m *= 1.0 + td( state->target )->debuffs.corrupt_the_blood->check();

    return m;
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );
    td( state->target )->debuffs.corrupt_the_blood->trigger();
  }

  bool procs_shadow_blades_damage() const override
  { return false; }
};

struct deathstalkers_mark_t : public rogue_attack_t
{
  deathstalkers_mark_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spell.deathstalkers_mark_damage )
  {
  }
};

struct fatal_intent_t : public rogue_attack_t
{
  fatal_intent_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spell.fatal_intent_damage )
  {
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    m *= td( target )->debuffs.fatal_intent->stack();

    return m;
  }

  void execute() override
  {
    rogue_attack_t::execute();
    td( execute_state->target )->debuffs.fatal_intent->expire();
  }

  bool procs_shadow_blades_damage() const override
  { return false; }

  bool procs_caustic_spatter() const override
  { return false; }
};

struct hunt_them_down_t : public rogue_attack_t
{
  hunt_them_down_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spell.hunt_them_down_damage )
  {
    p->auto_attack->add_child( this );
  }

  bool procs_shadow_blades_damage() const override
  { return false; }
};

struct singular_focus_t : public rogue_attack_t
{
  singular_focus_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spell.singular_focus_damage )
  {
    callbacks = false;
  }

  bool procs_shadow_blades_damage() const override
  { return false; }

  bool procs_caustic_spatter() const override
  { return false; }
};

// Fatebound ================================================================

// Note:: Dummy Hand of Fate container spell for reporting purposes, not functional!
struct hand_of_fate_t : public rogue_attack_t
{
  hand_of_fate_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->talent.fatebound.hand_of_fate )
  {
    background = true;
    add_child( p->active.fatebound.fatebound_coin_tails );
    add_child( p->active.fatebound.lucky_coin );
  }
};

struct fatebound_coin_tails_t : public rogue_attack_t
{
  fatebound_coin_tails_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spell.fatebound_coin_tails )
  {
  }

  double action_multiplier() const override
  {
    auto m = rogue_attack_t::action_multiplier();

    auto stacks = p()->buffs.fatebound_coin_tails->total_stack();
    m *= 1.0 + ( stacks * p()->spell.fatebound_coin_tails_buff->effectN( 1 ).percent() );

    if ( p()->talent.fatebound.delivered_doom->ok() )
    {
      auto num_targets = targets_in_range_list( target_list() ).size();
      if ( num_targets < 6 )
      {
        m *= 1.0 + p()->talent.fatebound.delivered_doom->effectN( 1 ).percent() -
          ( p()->talent.fatebound.delivered_doom->effectN( 2 ).percent() * ( num_targets - 1 ) );
      }
    }

    return m;
  }

  void execute() override
  {
    rogue_attack_t::execute();
    // Tail buff is always incremented after its damage instance
    p()->buffs.fatebound_coin_tails->increment();
  }

  bool procs_blade_flurry() const override
  { return true; }

  bool procs_poison() const override
  { return true; }
};

struct fatebound_lucky_coin_t : public rogue_attack_t
{
  fatebound_lucky_coin_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spell.fatebound_lucky_coin_damage )
  {
  }

  bool procs_blade_flurry() const override
  { return true; }
};

struct fate_intertwined_t : public rogue_attack_t
{
  fate_intertwined_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spell.fatebound_fate_intertwined )
  {
    base_dd_min = base_dd_max = 1;
    aoe = as<int>( p->talent.fatebound.fate_intertwined->effectN( 2 ).base_value() );
    radius = data().effectN( 1 ).radius();
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    rogue_attack_t::available_targets( tl );

    // Does not hit the primary target if it hits additional enemies
    if( tl.size() > 1 )
    {
      range::erase_remove( tl, target );
    }

    return tl.size();
  }
};

// Trickster ================================================================

struct unseen_blade_t : public rogue_attack_t
{
  unseen_blade_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spell.unseen_blade )
  {
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p()->buffs.escalating_blade->trigger();
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );
    p()->get_target_data( state->target )->debuffs.fazed->trigger();

    if ( p()->talent.trickster.flawless_form->ok() )
    {
      p()->buffs.flawless_form->execute();
    }
  }

  bool procs_main_gauche() const override
  { return true; }

  bool procs_blade_flurry() const override
  { return true; }

  bool procs_nimble_flurry() const override
  { return true; }
};

struct nimble_flurry_t : public rogue_attack_t
{
  nimble_flurry_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spell.nimble_flurry_damage )
  {
    aoe = as<int>( p->talent.trickster.nimble_flurry->effectN( 2 ).base_value() );
    affected_by.fazed_damage = true; // 2025-08-11 -- Not in whitelist or spell data
    target_filter_callback = secondary_targets_only();
  }

  // Currently does not trigger on either side, which is likely a bug
  bool procs_shadow_blades_damage() const override
  { return false; }
};

struct coup_de_grace_t : public rogue_attack_t
{
  struct coup_de_grace_bonus_t : public rogue_attack_t
  {
    int last_cp;

    coup_de_grace_bonus_t( util::string_view name, rogue_t* p, const spell_data_t* s ) :
      rogue_attack_t( name, p, s ),
      last_cp( 1 )
    {
      dual = true;

      if ( p->talent.subtlety.shadowed_finishers->ok() )
      {
        // Spell has the full damage coefficient and is modified via talent scripting
        base_multiplier *= p->talent.subtlety.shadowed_finishers->effectN( 1 ).percent();
      }
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

    bool procs_nimble_flurry() const override
    { return true; }
  };

  struct coup_de_grace_damage_t : public rogue_attack_t
  {
    coup_de_grace_bonus_t* bonus_attack;

    coup_de_grace_damage_t( util::string_view name, rogue_t* p, const spell_data_t* s, const spell_data_t* bonus = nullptr ) :
      rogue_attack_t( name, p, s ),
      bonus_attack( nullptr )
    {
      if ( p->talent.subtlety.shadowed_finishers->ok() )
      {
        auto formatted_name = fmt::format( "eviscerate_{}", name );
        util::replace_all( formatted_name, "damage", "bonus" );
        bonus_attack = p->get_background_action<coup_de_grace_bonus_t>( formatted_name, bonus);
      }
    }

    double combo_point_da_multiplier( const action_state_t* state ) const override
    {
      return cast_state( state )->get_combo_points();
    }

    double composite_da_multiplier( const action_state_t* state ) const override
    {
      double m = rogue_attack_t::composite_da_multiplier( state );

      // 2025-07-31 -- TOCHECK: 4pc damage bonus does not apply to Shadowed Finishers
      if ( p()->set_bonuses.tww3_trickster_4pc->ok() && !p()->buffs.tww3_trickster_4pc->check() )
      {
        m *= p()->buffs.tww3_trickster_4pc->default_value;
      }

      return m;
    }

    void impact( action_state_t* state ) override
    {
      // 2024-08-08 -- Due to the animation, Danse Macabre is triggered prior to the impact and is self-affecting
      trigger_danse_macabre( state );

      rogue_attack_t::impact( state );

      if ( bonus_attack && td( state->target )->debuffs.find_weakness->up() && result_is_hit( state->result ) )
      {
        bonus_attack->last_cp = cast_state( state )->get_combo_points();
        bonus_attack->execute_on_target( state->target );
      }
    }

    bool procs_main_gauche() const override
    { return true; }

    bool procs_blade_flurry() const override
    { return true; }

    bool procs_nimble_flurry() const override
    { return true; }
  };

  std::vector<coup_de_grace_damage_t*> attacks;

  coup_de_grace_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spell.coup_de_grace, options_str )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( p->get_secondary_trigger_action<coup_de_grace_damage_t>(
        secondary_trigger::COUP_DE_GRACE, fmt::format( "{}_damage", name ),
        p->spell.coup_de_grace_damage_1, p->spell.coup_de_grace_damage_bonus_1 ) );
      attacks.push_back( p->get_secondary_trigger_action<coup_de_grace_damage_t>(
        secondary_trigger::COUP_DE_GRACE, fmt::format( "{}_damage_2", name ),
        p->spell.coup_de_grace_damage_2, p->spell.coup_de_grace_damage_bonus_2 ) );
      attacks.push_back( p->get_secondary_trigger_action<coup_de_grace_damage_t>(
        secondary_trigger::COUP_DE_GRACE, fmt::format( "{}_damage_3", name ),
        p->spell.coup_de_grace_damage_3, p->spell.coup_de_grace_damage_bonus_3 ) );
    }
  }

  void init_finished() override
  {
    rogue_attack_t::init_finished();

    // Merge attacks for reporting display and DPET purposes
    for ( auto& attack : attacks )
    {
      attack->stats = stats;
      if ( attack->bonus_attack && attack->bonus_attack != attacks.front()->bonus_attack )
      {
        attack->bonus_attack->stats = attacks.front()->bonus_attack->stats;
      }
    }

    if ( attacks.front()->bonus_attack )
    {
      add_child( attacks.front()->bonus_attack );
    }
  }

  void snapshot_state( action_state_t* state, result_amount_type rt ) override
  {
    rogue_attack_t::snapshot_state( state, rt );

    // Adjust the CP in the action state so it works with Relentless Strikes, etc.
    auto rs = cast_state( state );
    const int trigger_cp = rs->get_combo_points() + as<int>( p()->talent.trickster.coup_de_grace->effectN( 3 ).base_value() );
    rs->set_combo_points( rs->get_combo_points( true ), trigger_cp );
  }

  void execute() override
  {
    rogue_attack_t::execute();
    
    if ( p()->talent.outlaw.summarily_dispatched->ok() )
    {
      int cp = cast_state( execute_state )->get_combo_points();
      if ( cp >= p()->talent.outlaw.summarily_dispatched->effectN( 2 ).base_value() )
      {
        p()->buffs.summarily_dispatched->trigger();
      }
    }

    // Extra Flawless Form stacks are currently granted prior to the impact, so self-affecting
    // Use Execute() instead of Trigger() to avoid async stack trigger aura delay
    if ( p()->get_target_data( execute_state->target )->debuffs.fazed->check() )
    {
      p()->buffs.flawless_form->execute( as<int>( p()->talent.trickster.coup_de_grace->effectN( 2 ).base_value() ) );
    }

    // Includes the adjusted bonus CP from snapshot_state above
    const int trigger_cp = cast_state( execute_state )->get_combo_points();
    attacks[ 0 ]->trigger_secondary_action( execute_state->target, trigger_cp );
    attacks[ 1 ]->trigger_secondary_action( execute_state->target, trigger_cp, 300_ms );
    attacks[ 2 ]->trigger_secondary_action( execute_state->target, trigger_cp, 1_s );

    trigger_restless_blades( execute_state );
    trigger_cut_to_the_chase( execute_state );
    trigger_tww2_set_bonus_removal();

    if ( p()->buffs.tww3_trickster_4pc->check() )
    {
      p()->buffs.tww3_trickster_4pc->expire();
    }
    else
    {
      p()->buffs.tww3_trickster_4pc->trigger();
    }

    p()->buffs.escalating_blade->expire();
  }

  bool ready() override
  {
    if ( p()->talent.trickster.coup_de_grace->ok() &&
         ( p()->buffs.escalating_blade->at_max_stacks() || p()->buffs.tww3_trickster_4pc->check() ) )
      return rogue_attack_t::ready();

    return false;
  }

  bool procs_main_gauche() const override
  { return true; }

  bool has_amount_result() const override
  { return true; }

  bool consumes_supercharger() const override
  { return true; }
};

// TWW1 Set Bonus ===========================================================

struct ethereal_rampage_t : public rogue_attack_t
{
  ethereal_rampage_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spec.tww1_outlaw_2pc_spell )
  {
  }
};

// ==========================================================================
// Cancel AutoAttack
// ==========================================================================

struct cancel_autoattack_t : public action_t
{
  rogue_t* rogue;
  cancel_autoattack_t( rogue_t* rogue_, util::string_view options_str = {} ) :
    action_t( ACTION_OTHER, "cancel_autoattack", rogue_ ),
    rogue( rogue_ )
  {
    parse_options( options_str );
    set_target( rogue );
    trigger_gcd = timespan_t::zero();
  }

  result_e calculate_result( action_state_t* ) const override
  { return RESULT_HIT; }

  block_result_e calculate_block_result( action_state_t* ) const override
  { return BLOCK_RESULT_UNBLOCKED; }

  void execute() override
  {
    action_t::execute();
    rogue->cancel_auto_attacks();
  }

  bool ready() override
  {
    if ( ( rogue->main_hand_attack && rogue->main_hand_attack->execute_event ) ||
         ( rogue->off_hand_attack && rogue->off_hand_attack->execute_event ) )
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

  weapon_swap_t( rogue_t* rogue_, util::string_view options_str ) :
    action_t( ACTION_OTHER, "weapon_swap", rogue_ ),
    swap_type( SWAP_MAIN_HAND ), swap_to_type( WEAPON_SECONDARY ),
    rogue( rogue_ )
  {
    may_miss = may_crit = may_dodge = may_parry = may_glance = callbacks = harmful = false;
    
    set_target( rogue );

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
        sim->errorf( "Player %s weapon_swap: No weapon info for %s/%s",
                     player->name(), slot_str.c_str(), swap_to_str.c_str() );
      }
    }
    else
    {
      if ( ! rogue -> weapon_data[ WEAPON_MAIN_HAND ].item_data[ swap_to_type ] ||
           ! rogue -> weapon_data[ WEAPON_OFF_HAND ].item_data[ swap_to_type ] )
      {
        background = true;
        sim->errorf( "Player %s weapon_swap: No weapon info for %s/%s",
                     player->name(), slot_str.c_str(), swap_to_str.c_str() );
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

    rogue->invalidate_cache( CACHE_WEAPON_DPS );
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
    ( ab::data().id() == 703 || ab::data().id() == 1943 ||
      this->name_str == "crimson_tempest" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      dot_t* d = ab::get_dot( ab::target );
      return d->is_ticking() && cast_state( d->state )->is_exsanguinated();
    } );
  }
  else if ( util::str_compare_ci( name_str, "exsanguinated_rate" ) &&
    ( ab::data().id() == 703 || ab::data().id() == 1943 ||
      this->name_str == "crimson_tempest" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      dot_t* d = ab::get_dot( ab::target );
      return d->is_ticking() ? cast_state( d->state )->get_exsanguinated_rate() : 1.0;
    } );
  }
  else if ( util::str_compare_ci( name_str, "will_lose_exsanguinate" ) &&
    ( ab::data().id() == 703 || ab::data().id() == 1943
      || this->name_str == "crimson_tempest" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      dot_t* d = ab::get_dot( ab::target );
      double refresh_rate = 1.0;
      return d->is_ticking() && cast_state( d->state )->get_exsanguinated_rate() > refresh_rate;
    } );
  }
  else if ( name_str == "effective_combo_points" )
  {
    return make_fn_expr( name_str, [ this ]() { return p()->current_effective_cp( consumes_supercharger(), true ); } );
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
  sim_t* sim = item_data[ WEAPON_PRIMARY ]->sim;
  for ( size_t i = 0, end = cb_data[ weapon ].size(); i < end; ++i )
  {
    if ( state )
    {
      cb_data[ weapon ][ i ]->activate();
      if ( cb_data[ weapon ][ i ]->rppm )
      {
        cb_data[ weapon ][ i ]->rppm->set_accumulated_blp( timespan_t::zero() );
        cb_data[ weapon ][ i ]->rppm->set_last_trigger_attempt( sim->current_time() );
      }
      sim->print_debug( "{} enabling callback {} on {}", *item_data[ WEAPON_PRIMARY ]->player,
                        cb_data[ weapon ][ i ]->effect, *item_data[ weapon ] );
    }
    else
    {
      cb_data[ weapon ][ i ] -> deactivate();
      sim->print_debug( "{} disabling callback {} on {}", *item_data[ WEAPON_PRIMARY ]->player,
                        cb_data[ weapon ][ i ]->effect, *item_data[ weapon ] );
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

      for ( auto* callback : rogue->callbacks.all_callbacks )
      {
        dbc_proc_callback_t* cb = debug_cast<dbc_proc_callback_t*>( callback );

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

      for (auto & callback : rogue -> callbacks.all_callbacks)
      {
        dbc_proc_callback_t* cb = debug_cast<dbc_proc_callback_t*>( callback );

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

struct adrenaline_rush_t : public rogue_buff_t
{
  adrenaline_rush_t( rogue_t* p ) :
    rogue_buff_t( p, "adrenaline_rush", p->talent.outlaw.adrenaline_rush )
  {
    set_cooldown( timespan_t::zero() );
    set_default_value_from_effect_type( A_MOD_RANGED_AND_MELEE_AUTO_ATTACK_SPEED );
    set_affects_regen( true );
    add_invalidate( CACHE_AUTO_ATTACK_SPEED );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    buff_t::start( stacks, value, duration );

    rogue_t* rogue = debug_cast<rogue_t*>( source );
    rogue->resources.temporary[ RESOURCE_ENERGY ] += data().effectN( 4 ).base_value();
    rogue->recalculate_resource_max( RESOURCE_ENERGY );

    if ( rogue->talent.outlaw.improved_adrenaline_rush->ok() )
    {
      const double cp_gain = rogue->resources.max[ RESOURCE_COMBO_POINT ];
      rogue->resource_gain( RESOURCE_COMBO_POINT, cp_gain, rogue->gains.improved_adrenaline_rush );
    }
  }

  void expire_override(int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    rogue_t* rogue = debug_cast<rogue_t*>( source );
    rogue->resources.temporary[ RESOURCE_ENERGY ] -= data().effectN( 4 ).base_value();
    rogue->recalculate_resource_max( RESOURCE_ENERGY, rogue->gains.adrenaline_rush_expiry );

    if ( rogue->talent.outlaw.improved_adrenaline_rush->ok() )
    {
      const double energy_gain = rogue->resources.max[ RESOURCE_ENERGY ];
      rogue->resource_gain( RESOURCE_ENERGY, energy_gain, rogue->gains.improved_adrenaline_rush_expiry );
    }
  }
};

struct blade_flurry_t : public rogue_buff_t
{
  rogue_t* p;

  blade_flurry_t( rogue_t* p ) :
    rogue_buff_t( p, "blade_flurry", p->spec.blade_flurry ),
    p( p )
  {
    set_cooldown( timespan_t::zero() );
    set_default_value_from_effect( 2 );
    set_refresh_behavior( buff_refresh_behavior::DURATION );
  }
};

struct subterfuge_t : public buff_t
{
  rogue_t* rogue;

  subterfuge_t( rogue_t* r ) :
    buff_t( r, "subterfuge", r->spell.subterfuge_buff ),
    rogue( r )
  {
  }
};

template <typename BuffBase>
struct stealth_like_buff_t : public BuffBase
{
  using base_t = stealth_like_buff_t<BuffBase>;
  rogue_t* rogue;

  stealth_like_buff_t( rogue_t* r, util::string_view name, const spell_data_t* spell ) :
    BuffBase( r, name, spell ), rogue( r )
  {
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    bb::execute( stacks, value, duration );

    if ( rogue->stealthed( STEALTH_BASIC | STEALTH_SHADOW_DANCE ) )
    {
      if ( rogue->talent.assassination.improved_garrote->ok() )
        rogue->buffs.improved_garrote_aura->trigger();

      if ( rogue->talent.subtlety.premeditation->ok() )
        rogue->buffs.premeditation->trigger();

      if ( rogue->talent.subtlety.shot_in_the_dark->ok() )
        rogue->buffs.shot_in_the_dark->trigger();

      if ( rogue->talent.subtlety.silent_storm->ok() )
        rogue->buffs.silent_storm->trigger();
    }

    if ( rogue->stealthed( STEALTH_BASIC | STEALTH_ROGUE ) )
    {
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    bb::expire_override( expiration_stacks, remaining_duration );

    // Don't swap these buffs around if we are still in stealth due to Vanish expiring
    if ( !rogue->stealthed( STEALTH_BASIC ) )
    {
      rogue->buffs.improved_garrote_aura->expire();

      // 2023-10-21 -- Premeditation does not persist into Subterfuge when Stealth expires
      if ( rogue->bugs )
      {
        rogue->buffs.premeditation->expire();
      }
    }
  }

private:
  using bb = BuffBase;
};

// Note, stealth buff is set a max time of half the nominal fight duration, so it can be
// forced to show in sample sequence tables.
struct stealth_t : public stealth_like_buff_t<buff_t>
{
  stealth_t( rogue_t* r ) :
    base_t( r, "stealth", r->spell.stealth )
  {
    set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
    set_duration( sim->max_time / 2 );
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    base_t::execute( stacks, value, duration );
    rogue->cancel_auto_attacks();

    // Activating stealth buff (via expiring Vanish) also removes Shadow Dance. Not an issue in general since Stealth cannot be used during Dance.
    rogue->buffs.shadow_dance->expire();
  }
};

// Vanish now acts like "stealth like abilities".
struct vanish_t : public stealth_like_buff_t<buff_t>
{
  vanish_t( rogue_t* r ) :
    base_t( r, "vanish", r->spell.vanish_buff )
  {
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    base_t::execute( stacks, value, duration );
    rogue->cancel_auto_attacks();

    // Vanish drops combat if in combat with non-bosses, relevant for some trinket effects
    if ( !rogue->in_boss_encounter )
      rogue->leave_combat();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    // Stealth proc if Vanish fully ends (i.e. isn't broken before the expiration)
    if ( remaining_duration == timespan_t::zero() )
    {
      rogue->buffs.stealth->trigger();
    }

    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// Shadow dance acts like "stealth like abilities"
struct shadow_dance_t : public stealth_like_buff_t<damage_buff_t>
{
  shadow_dance_t( rogue_t* p ) :
    base_t( p, "shadow_dance", p->spec.shadow_dance )
  {
    set_cooldown( timespan_t::zero() );
  }

  timespan_t buff_duration() const override
  {
    timespan_t bd = stealth_like_buff_t::buff_duration();

    if ( rogue->buffs.the_first_dance->up() )
    {
      bd += rogue->spec.the_first_dance_buff->effectN( 1 ).time_value();
    }

    return bd;
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    stealth_like_buff_t::execute( stacks, value, duration );

    if ( rogue->talent.subtlety.danse_macabre->ok() )
    {
      rogue->buffs.danse_macabre->expire();
      rogue->danse_macabre_tracker.clear();
      rogue->buffs.danse_macabre->trigger();
    }

    rogue->buffs.the_first_dance->expire();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    stealth_like_buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( rogue->talent.subtlety.lingering_shadow->ok() )
    {
      rogue->buffs.lingering_shadow->cancel();
      rogue->buffs.lingering_shadow->trigger( rogue->buffs.lingering_shadow->max_stack() );
    }

    if ( rogue->talent.subtlety.danse_macabre->ok() )
    {
      rogue->buffs.danse_macabre->expire();
      rogue->danse_macabre_tracker.clear();
    }

    // These buffs do not persist after Shadow Dance expires, unlike normal Stealth
    rogue->buffs.improved_garrote->expire();
  }
};

struct shuriken_tornado_t : public buff_t
{
  rogue_t* rogue;
  actions::shuriken_storm_t* shuriken_storm_action;

  shuriken_tornado_t( rogue_t* r ) :
    buff_t( r, "shuriken_tornado", r->talent.subtlety.shuriken_tornado ),
    rogue( r ),
    shuriken_storm_action( nullptr )
  {
    set_cooldown( timespan_t::zero() );
    set_period( timespan_t::from_seconds( 1.0 ) ); // Not explicitly in spell data

    shuriken_storm_action = r->get_secondary_trigger_action<actions::shuriken_storm_t>(
      secondary_trigger::SHURIKEN_TORNADO, "shuriken_storm_tornado" );
    shuriken_storm_action->not_a_proc = true; // 2023-10-19 -- Now appears to be a scripted foreground cast
    shuriken_storm_action->affected_by.shadow_blades_cp = false; // 2023-10-11 -- No longer generates increased CP
    set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
      shuriken_storm_action->trigger_secondary_action( rogue->target );
    } );
  }
};

struct slice_and_dice_t : public rogue_buff_t
{
  struct recuperator_t : public actions::rogue_heal_t
  {
    recuperator_t( util::string_view name, rogue_t* p ) :
      rogue_heal_t( name, p, p->spell.recuperator_heal )
    {
      // This is treated as direct triggered by the tick callback on SnD to avoid duration/refresh desync
      direct_tick = not_a_proc = true;
      may_crit = false;
      dot_duration = timespan_t::zero();
      base_pct_heal = p->talent.rogue.recuperator->effectN( 1 ).percent();
      base_dd_min = base_dd_max = 1; // HAX: Make it always heal as this procs things in-game even with 0 value
    }

    result_amount_type amount_type( const action_state_t*, bool ) const override
    { return result_amount_type::HEAL_OVER_TIME; }
  };

  rogue_t* rogue;
  recuperator_t* recuperator;

  slice_and_dice_t( rogue_t* p ) :
    rogue_buff_t( p, "slice_and_dice", p->spell.slice_and_dice ),
    rogue( p ),
    recuperator( nullptr )
  {
    set_default_value_from_effect_type( A_MOD_RANGED_AND_MELEE_AUTO_ATTACK_SPEED );
    set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
    add_invalidate( CACHE_AUTO_ATTACK_SPEED );
    set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

    if ( p->talent.rogue.recuperator->ok() )
    {
      set_period( p->spell.recuperator_heal->effectN( 1 ).period() );
      recuperator = p->get_background_action<recuperator_t>( "recuperator" );
    }

    set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
      if ( recuperator )
      {
        recuperator->set_target( rogue );
        recuperator->execute();
      }
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
    rogue_poison_buff_t( r, "wound_poison", debug_cast<rogue_t*>( r.source )->spell.wound_poison->effectN( 1 ).trigger() )
  {
  }
};

struct atrophic_poison_t : public rogue_poison_buff_t
{
  atrophic_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "atrophic_poison", debug_cast<rogue_t*>( r.source )->talent.rogue.atrophic_poison->effectN( 1 ).trigger() )
  {
  }
};

struct crippling_poison_t : public rogue_poison_buff_t
{
  crippling_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "crippling_poison", debug_cast<rogue_t*>( r.source )->spell.crippling_poison->effectN( 1 ).trigger() )
  { }
};

struct numbing_poison_t : public rogue_poison_buff_t
{
  numbing_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "numbing_poison", debug_cast<rogue_t*>( r.source )->talent.rogue.numbing_poison->effectN( 1 ).trigger() )
  { }
};

struct roll_the_bones_t : public buff_t
{
  rogue_t* rogue;
  std::array<buff_t*, 4> buffs;
  std::array<proc_t*, 4> procs;
  std::array<proc_t*, 4> loss_procs;

  roll_the_bones_t( rogue_t* r ) :
    buff_t( r, "roll_the_bones", r->spec.roll_the_bones ),
    rogue( r )
  {
    set_cooldown( timespan_t::zero() );
    set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

    buffs = {
      rogue->buffs.one_of_a_kind,
      rogue->buffs.double_trouble,
      rogue->buffs.triple_threat,
      rogue->buffs.jackpot
    };
  }

  void extend_secondary_buffs( timespan_t duration )
  {
    for ( auto buff : buffs )
    {
      // 2022-12-12 -- Keep it Rolling does not appear to be able to extend buffs past 60s
      auto extend_duration = rogue->bugs ?
        std::max( 0_s, std::min( duration, ( 60_s - buff->remains() ) ) ) :
        duration;

      buff->extend_duration( rogue, extend_duration );
    }
  }

  void expire_secondary_buffs()
  {
    for ( std::size_t i = 0; i < buffs.size(); i++ )
    {
      if ( buffs[ i ]->check() )
      {
        loss_procs[ i ]->occur();
        buffs[ i ]->expire();
      }
    }
  }

  unsigned random_roll( bool loaded_dice )
  {
    unsigned num_buffs = 0;

    if ( rogue->options.fixed_rtb_odds.empty() )
    {
      // RtB uses hardcoded probabilities even after the redesign
      // Current beta testing appears to show 50%, 40%, 9%, 1% for the four states
      // Loaded Dice appears to increase the rolled tier by one
      // Sleight of Hand is currently TBD, thought to mean a 20% increased chance to be elevated currently
      rogue->options.fixed_rtb_odds = { 50.0, 40.0, 9.0, 1.0 };
      rogue->sim->print_log( "{} {} odds set to {:.1f}% one, {:.1f}% two and {:.1f}% five buffs",
                             *rogue, *this, rogue->options.fixed_rtb_odds[ 0 ], rogue->options.fixed_rtb_odds[ 1 ], rogue->options.fixed_rtb_odds[ 4 ] );
    }

    if ( !rogue->options.fixed_rtb_odds.empty() )
    {
      std::vector<double> current_odds = rogue->options.fixed_rtb_odds;

      double odd_sum = 0.0;
      for ( const double& chance : current_odds )
        odd_sum += chance;
      double roll = rng().range( 0.0, odd_sum );
      
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

      if ( loaded_dice )
      {
        num_buffs += 1;
      }

      if ( rogue->talent.outlaw.sleight_of_hand->ok() && rogue->rng().roll( rogue->talent.outlaw.sleight_of_hand->effectN( 1 ).percent() ) )
      {
        num_buffs += 1;
      }
    }

    num_buffs = std::clamp( num_buffs, 1U, 4U );

    return num_buffs;
  }

  unsigned roll_the_bones( timespan_t duration )
  {
    unsigned rolled;
   
    if ( rogue->options.fixed_rtb > 0 )
    {
      rolled = random_roll( rogue->buffs.loaded_dice->up() );
    }
    else
    {
      rolled = rogue->options.fixed_rtb;
    }

    assert( rolled > 0 && rolled <= buffs.size() );

    buffs[ rolled - 1 ]->trigger( duration );
    return rolled;
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    const timespan_t roll_duration = remains();

    expire_secondary_buffs();

    const unsigned buffs_rolled = roll_the_bones( roll_duration );

    procs[ buffs_rolled - 1 ]->occur();
    rogue->buffs.loaded_dice->expire();

    current_value = buffs_rolled; // Store in the container buff

    if ( rogue->set_bonuses.tww2_outlaw_4pc->ok() )
    {
      rogue->buffs.tww2_outlaw_2pc->trigger( buffs_rolled );
    }
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

  if ( !talent.assassination.venomous_wounds->ok() )
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

  // 2025-04-12 -- The death effect was never updated to use the new VW value of 8 Energy, and still uses the old value of 10
  int replenish = bugs ? 10 : as<int>( talent.assassination.venomous_wounds->effectN( 2 ).base_value() );
  
  sim->print_log( "{} venomous_wounds replenish on death: full_ticks={}, ticks_left={}, vw_replenish={}, remaining_time={}",
                  *this, full_ticks_remaining, td->dots.rupture->ticks_left(), replenish, td->dots.rupture->remains() );

  resource_gain( RESOURCE_ENERGY, full_ticks_remaining * replenish, gains.venomous_wounds_death,
                 td->dots.rupture->current_action );
}

[[maybe_unused]] void rogue_t::do_exsanguinate( dot_t* dot, double rate )
{
  if ( !dot->is_ticking() )
    return;

  auto rs = actions::rogue_attack_t::cast_state( dot->state );
  const double new_rate = rs->get_exsanguinated_rate() * rate;
  const double coeff = 1.0 / rate;

  sim->print_log( "{} exsanguinates {} tick rate by {:.1f} from {:.1f} to {:.1f}",
                  *this, *dot, rate, rs->get_exsanguinated_rate(), new_rate );

  // Since the advent of hasted bleed exsanguinate works differently though.
  // Note: PTR testing shows Exsanguinated compound multiplicatively (2x -> 4x -> 8x -> etc.)
  rs->set_exsanguinated_rate( new_rate );
  dot->adjust_full_ticks( coeff );
}

template <typename Base>
void actions::rogue_action_t<Base>::spend_combo_points( const action_state_t* state )
{
  if ( ab::base_costs[ RESOURCE_COMBO_POINT ] == 0 )
    return;

  if ( !ab::hit_any_target )
    return;

  const auto rs = cast_state( state );
  double max_spend = std::min( p()->current_cp(), p()->consume_cp_max() );
  ab::stats->consume_resource( RESOURCE_COMBO_POINT, max_spend );
  p()->resource_loss( RESOURCE_COMBO_POINT, max_spend );

  p()->sim->print_log( "{} consumes {} {} for {} ({})", *p(), max_spend, util::resource_type_string( RESOURCE_COMBO_POINT ),
                       *this, p()->current_cp() );

  // Remove Supercharger Buffs
  if ( p()->talent.rogue.supercharger->ok() && consumes_supercharger() )
  {
    if ( rs->get_combo_points() > rs->get_combo_points( true ) )
    {
      // Consume from the end of the list
      for ( auto it = p()->buffs.supercharger.rbegin(); it != p()->buffs.supercharger.rend(); ++it )
      {
        if ( ( *it )->check() )
        {
          ( *it )->expire();
          supercharged_cp_proc->occur();
          p()->buffs.echoing_reprimand->trigger();
          break;
        }
      }
    }
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_auto_attack( const action_state_t* /* state */ )
{
  if ( !p()->main_hand_attack || p()->main_hand_attack->execute_event || !p()->off_hand_attack || p()->off_hand_attack->execute_event )
    return;

  if ( !ab::data().flags( spell_attribute::SX_MELEE_COMBAT_START ) )
    return;

  p()->auto_attack->execute();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_poisons( const action_state_t* state )
{
  if ( !ab::result_is_hit( state->result ) )
    return;

  auto trigger_lethal_poison = [this, state]( rogue_poison_t* poison ) {
    if ( poison )
    {
      // 2021-06-29 -- For reasons unknown, Deadly Poison has its own proc logic
      bool procs_lethal_poison = p()->specialization() == ROGUE_ASSASSINATION &&
        poison->data().id() == p()->talent.assassination.deadly_poison->id() ?
        procs_deadly_poison() : procs_poison();

      if ( procs_lethal_poison )
        poison->trigger( state );
    }
  };

  trigger_lethal_poison( p()->active.lethal_poison );
  trigger_lethal_poison( p()->active.lethal_poison_dtb );

  if ( procs_poison() )
  {
    if ( p()->active.nonlethal_poison )
      p()->active.nonlethal_poison->trigger( state );

    if ( p()->active.nonlethal_poison_dtb )
      p()->active.nonlethal_poison_dtb->trigger( state );
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_seal_fate( const action_state_t* state )
{
  if ( !p()->talent.assassination.seal_fate->ok() )
    return;

  if ( !procs_seal_fate() )
    return;

  if ( state->result != RESULT_CRIT )
    return;

  if ( !p()->rng().roll( p()->talent.assassination.seal_fate->effectN( 1 ).percent() ) )
    return;

  trigger_combo_point_gain( as<int>( p()->talent.assassination.seal_fate->effectN( 2 ).trigger()->effectN( 1 ).base_value() ),
                            p()->gains.seal_fate );

  if ( p()->talent.fatebound.deal_fate->ok() && procs_deal_fate() )
  {
    trigger_combo_point_gain( as<int>( p()->talent.fatebound.deal_fate->effectN( 1 ).base_value() ), p()->gains.deal_fate );
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_main_gauche( const action_state_t* state )
{
  if ( !p()->mastery.main_gauche->ok() )
    return;

  if ( !ab::has_amount_result() )
    return;

  if ( !ab::result_is_hit( state->result ) )
    return;

  if ( !procs_main_gauche() )
    return;

  double proc_chance = p()->mastery.main_gauche->proc_chance();

  if ( p()->buffs.blade_flurry->check() )
  {
    proc_chance += p()->spec.blade_flurry->effectN( 5 ).percent();
  }

  if ( !p()->rng().roll( proc_chance ) )
    return;

  p()->active.main_gauche->trigger_secondary_action( state->target );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_fatal_flourish( const action_state_t* state )
{
  if ( !p()->talent.outlaw.fatal_flourish->ok() || !ab::result_is_hit( state->result ) || !procs_fatal_flourish() )
    return;

  double chance = p()->talent.outlaw.fatal_flourish->effectN( 1 ).percent();
  if ( state->action != p()->active.main_gauche && state->action->weapon )
    chance *= state->action->weapon->swing_time.total_seconds() / 2.6;
  if ( !p()->rng().roll( chance ) )
    return;

  double gain = p()->talent.outlaw.fatal_flourish->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_ENERGY );

  p()->resource_gain( RESOURCE_ENERGY, gain, p()->gains.fatal_flourish, state->action );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_energy_refund()
{
  double energy_restored = ab::last_resource_cost * 0.80;
  p()->resource_gain( RESOURCE_ENERGY, energy_restored, p()->gains.energy_refund );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_doomblade( const action_state_t* state )
{
  if ( !p()->talent.assassination.doomblade->ok() || !ab::result_is_hit( state->result ) )
    return;

  const double dot_damage = state->result_amount * p()->talent.assassination.doomblade->effectN( 1 ).percent();
  residual_action::trigger( p()->active.doomblade, state->target, dot_damage );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_poison_bomb( const action_state_t* state )
{
  if ( !p()->talent.assassination.poison_bomb->ok() || !ab::result_is_hit( state->result ) )
    return;

  // They put 25 as value in spell data and divide it by 10 later, it's due to the int restriction.
  const auto rs = cast_state( state );
  if ( p()->rng().roll( p()->talent.assassination.poison_bomb->effectN( 1 ).percent() / 10 * rs->get_combo_points() ) )
  {
    make_event<ground_aoe_event_t>( *p()->sim, p(), ground_aoe_params_t()
      .target( state->target )
      .pulse_time( p()->spec.poison_bomb_driver->duration() / p()->talent.assassination.poison_bomb->effectN( 2 ).base_value() )
      .duration( p()->spec.poison_bomb_driver->duration() )
      .action( p()->active.poison_bomb ) );
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_venomous_wounds( const action_state_t* state )
{
  if ( !p()->talent.assassination.venomous_wounds->ok() )
    return;

  if ( !p()->get_target_data( state->target )->is_poisoned() )
    return;

  if ( !ab::result_is_hit( state->result ) )
    return;

  double chance = p()->talent.assassination.venomous_wounds->proc_chance();
  if ( !p()->rng().roll( chance ) )
    return;

  p()->resource_gain( RESOURCE_ENERGY, p()->talent.assassination.venomous_wounds->effectN( 2 ).base_value(),
                      p()->gains.venomous_wounds );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_blade_flurry( const action_state_t* state )
{
  if ( !procs_blade_flurry() )
    return;

  if ( state->result_total <= 0 )
    return;

  if ( !p()->buffs.blade_flurry->check() )
    return;

  if ( !ab::result_is_hit( state->result ) )
    return;

  if ( p()->sim->active_enemies == 1 )
    return;

  // Compute Blade Flurry modifier
  double multiplier = p()->buffs.blade_flurry->check_value();

  // 2024-08-12 -- This effect is multiplicative, even though it uses the same tooltip as additive mods
  if ( p()->talent.trickster.nimble_flurry->ok() && p()->buffs.flawless_form->check() )
  {
    multiplier *= 1.0 + p()->spell.flawless_form_buff->effectN( 3 ).percent();
  }

  p()->active.blade_flurry->trigger_residual_action( state, multiplier );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_combo_point_gain( int cp, gain_t* gain )
{
  p()->resource_gain( RESOURCE_COMBO_POINT, cp, gain, this );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_ruthlessness_cp( const action_state_t* state )
{
  if ( !p()->talent.outlaw.ruthlessness->ok() || !affected_by.ruthlessness || is_secondary_action() )
    return;

  if ( !ab::result_is_hit( state->result ) )
    return;

  int cp = cast_state( state )->get_combo_points();
  if ( cp == 0 )
    return;

  double cp_chance = p()->talent.outlaw.ruthlessness->effectN( 1 ).pp_combo_points() * cp / 100.0;
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
    // TOCHECK -- Technically triggers spell 139546
    trigger_combo_point_gain( cp_gain, p()->gains.ruthlessness );
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_deepening_shadows( const action_state_t* state )
{
  if ( !p()->talent.subtlety.deepening_shadows->ok() || !affected_by.deepening_shadows )
    return;

  int cp = cast_state( state )->get_combo_points();
  if ( cp == 0 )
    return;

  double cdr = p()->talent.subtlety.deepening_shadows->effectN( 1 ).base_value() / 10;
  timespan_t adjustment = timespan_t::from_seconds( -cdr * cp );
  p()->cooldowns.shadow_dance->adjust( adjustment, cp >= 5 );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_shadow_techniques( const action_state_t* state )
{
  if ( !p()->spec.shadow_techniques->ok() || !ab::result_is_hit( state->result ) )
    return;

  p()->sim->print_debug( "{} trigger_shadow_techniques increment from {} to {}", *p(), p()->shadow_techniques_counter, p()->shadow_techniques_counter + 1 );

  // 2021-04-22 -- Initial 9.1.0 testing appears to show the threshold is reduced to 4/3
  // 2023-10-02 -- 10.2.0 tooltip was adjusted to show "28%" for Shadow Techniques and "reduced by 40%" for Shadowcraft
  //               However, this is just an expansion of the probability and doesn't change the underlying counter mechanic
  const unsigned shadowcraft_adjustment = ( p()->talent.subtlety.shadowcraft->ok() && p()->buffs.shadow_dance->check() ) ? 1 : 0;
  const unsigned shadow_techniques_upper = 4 - shadowcraft_adjustment;
  const unsigned shadow_techniques_lower = 3 - shadowcraft_adjustment;
  if ( ++p()->shadow_techniques_counter >= shadow_techniques_upper || ( p()->shadow_techniques_counter == shadow_techniques_lower && p()->rng().roll( 0.5 ) ) )
  {
    p()->sim->print_debug( "{} trigger_shadow_techniques proc'd at {}, resetting counter to 0", *p(), p()->shadow_techniques_counter );
    p()->shadow_techniques_counter = 0;

    double energy_gain = p()->spec.shadow_techniques_energize->effectN( 2 ).base_value();

    p()->resource_gain( RESOURCE_ENERGY, energy_gain, p()->gains.shadow_techniques, state->action );
    // 2024-11-28 -- Shadowcraft's implementation appears to trigger the energize twice
    if ( p()->bugs && p()->talent.subtlety.shadowcraft->ok() && p()->buffs.shadow_dance->check() )
    {
      p()->resource_gain( RESOURCE_ENERGY, energy_gain, p()->gains.shadow_techniques, state->action );
    }
    p()->buffs.shadow_techniques->trigger( 1 + shadowcraft_adjustment ); // Combo Point storage
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_weaponmaster( const action_state_t* state, actions::rogue_attack_t* action )
{
  if ( !p()->talent.subtlety.weaponmaster->ok() || !ab::result_is_hit( state->result ) || !action )
    return;

  // 2022-02-24 -- 9.2 now allows this to trigger with a per-target ICD
  cooldown_t* tcd = p()->cooldowns.weaponmaster->get_cooldown( state->target );
  if ( !tcd || tcd->down() )
    return;

  if ( !p()->rng().roll( p()->talent.subtlety.weaponmaster->proc_chance() ) )
    return;

  p()->procs.weaponmaster->occur();
  tcd->start();

  p()->sim->print_log( "{} procs weaponmaster for {}", *p(), *this );

  // Direct damage re-computes on execute
  action->trigger_secondary_action( state->target, cast_state( state )->get_combo_points() );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_opportunity( const action_state_t* state, actions::rogue_attack_t* action, double modifier )
{
  if ( !p()->talent.outlaw.opportunity->ok() || !ab::result_is_hit( state->result ) || !action )
    return;

  const int stacks = 1 + as<int>( p()->talent.outlaw.fan_the_hammer->effectN( 1 ).base_value() );
  if ( p()->buffs.opportunity->trigger( stacks, buff_t::DEFAULT_VALUE(), p()->extra_attack_proc_chance() * modifier ) )
  {
    if ( p()->talent.fatebound.deal_fate->ok() )
    {
      trigger_combo_point_gain( as<int>( p()->talent.fatebound.deal_fate->effectN( 1 ).base_value() ),
                                p()->gains.deal_fate );
    }
    action->trigger_secondary_action( state->target, 300_ms );
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_restless_blades( const action_state_t* state )
{
  timespan_t v = timespan_t::from_seconds( p()->spec.restless_blades->effectN( 1 ).base_value() / 10.0 );
  
  if ( p()->buffs.roll_the_bones->check_value() >= 3 )
    v *= 1.0 + p()->spec.jackpot->effectN( 4 ).percent();

  v *= -cast_state( state )->get_combo_points();

  p()->cooldowns.adrenaline_rush->adjust( v, false );
  p()->cooldowns.between_the_eyes->adjust( v, false );
  p()->cooldowns.blade_flurry->adjust( v, false );
  p()->cooldowns.blade_rush->adjust( v, false );
  p()->cooldowns.grappling_hook->adjust( v, false );
  p()->cooldowns.keep_it_rolling->adjust( v, false );
  p()->cooldowns.killing_spree->adjust( v, false );
  p()->cooldowns.roll_the_bones->adjust( v, false );
  p()->cooldowns.sprint->adjust( v, false );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_hand_of_fate( const action_state_t* state, bool biased )
{
  if ( !p()->talent.fatebound.hand_of_fate->ok() )
    return;

  if ( is_secondary_action() )
    return; // You have to actually spend the CP to get the coin - no secondary action finishers grant flips

  if ( cast_state( state )->get_combo_points() < p()->talent.fatebound.hand_of_fate->effectN( 1 ).base_value() )
    return;

  fatebound_t::coinflip_e result;

  // No stacks of either buff or equal stacks of both buffs (thanks to only using edge case)
  // Nothing to bias, just flip the coin fairly
  if ( p()->buffs.fatebound_coin_tails->total_stack() == p()->buffs.fatebound_coin_heads->total_stack() )
  {
    result = p()->rng().roll( 0.5 ) ? fatebound_t::coinflip_e::HEADS : fatebound_t::coinflip_e::TAILS;
  }
  // Flip the coin, potentially with a bias toward matching the last face flipped
  else
  {
    double matching_odds = 0.5;
    if ( biased )
    {
      // TODO: Validate how these stack with the presumed base 50/50 chance and one another
      if ( p()->talent.fatebound.mean_streak->ok() )
      {
        matching_odds += matching_odds * p()->talent.fatebound.mean_streak->effectN( 1 ).percent();
      }
      if ( p()->talent.fatebound.destiny_defined->ok() )
      {
        matching_odds += p()->talent.fatebound.destiny_defined->effectN( 3 ).percent();
      }
    }

    // TODO: it's an assumption that if you have both buffs (thanks, edge case) the bias prefers the one with more stacks
    // (since the last one you hit before the edge case has to be the one with more stacks)
    const bool is_match = p()->rng().roll( matching_odds );
    const bool current_is_heads = p()->buffs.fatebound_coin_heads->check() > p()->buffs.fatebound_coin_tails->check();
    result = is_match ?
      current_is_heads ? fatebound_t::coinflip_e::HEADS : fatebound_t::coinflip_e::TAILS :
      current_is_heads ? fatebound_t::coinflip_e::TAILS : fatebound_t::coinflip_e::HEADS;
  }

  execute_fatebound_coinflip( state, result );
}

template <typename Base>
void actions::rogue_action_t<Base>::execute_fatebound_coinflip( const action_state_t* state, fatebound_t::coinflip_e result, timespan_t delay )
{
  auto coin_target = state->target->is_enemy() ? state->target : p()->target;
  make_event( *p()->sim, delay, [ this, coin_target, result ] {
    if ( result == fatebound_t::coinflip_e::HEADS || result == fatebound_t::coinflip_e::EDGE )
    {
      p()->buffs.fatebound_coin_heads->increment();
      if ( result != fatebound_t::coinflip_e::EDGE )
      {
        p()->buffs.fatebound_coin_tails->expire();
      }
    }
    if ( result == fatebound_t::coinflip_e::TAILS || result == fatebound_t::coinflip_e::EDGE )
    {
      // Don't fling tails coins at enemies precombat, since that'll start combat (assume the player knows not to have an enemy targeted)
      if ( !ab::is_precombat )
      {
        p()->active.fatebound.fatebound_coin_tails->trigger_secondary_action( coin_target );
      }
      if ( result != fatebound_t::coinflip_e::EDGE )
      {
        p()->buffs.fatebound_coin_heads->expire();
      }
    }
  } );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_fatebound_edge_case( const action_state_t* state )
{
  if ( !p()->talent.fatebound.edge_case->ok() )
    return;

  execute_fatebound_coinflip( state, fatebound_t::coinflip_e::EDGE );
  
  if ( p()->set_bonuses.tww3_fatebound_2pc->ok() )
  {
    execute_fatebound_coinflip( state, fatebound_t::coinflip_e::EDGE, 200_ms );
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_fate_intertwined( const action_state_t* state )
{
  if ( !p()->talent.fatebound.fate_intertwined->ok() )
    return;

  if ( state->result != RESULT_CRIT )
    return;

  const double multiplier = p()->talent.fatebound.fate_intertwined->effectN( 1 ).percent();
  p()->active.fatebound.fate_intertwined->trigger_residual_action( state, multiplier );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_relentless_strikes( const action_state_t* state )
{
  if ( !p()->talent.subtlety.relentless_strikes->ok() || !affected_by.relentless_strikes )
    return;

  double grant_energy = cast_state( state )->get_combo_points() *
    p()->spec.relentless_strikes_energize->effectN( 1 ).resource( RESOURCE_ENERGY );

  if ( grant_energy > 0 )
  {
    p()->resource_gain( RESOURCE_ENERGY, grant_energy, p()->gains.relentless_strikes, state->action );
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_blindside( const action_state_t* state )
{
  if ( !p()->talent.assassination.blindside->ok() || !ab::result_is_hit( state->result ) )
    return;

  double chance = p()->talent.assassination.blindside->effectN( 1 ).percent();
  if ( state->target->health_percentage() < p()->talent.assassination.blindside->effectN( 3 ).base_value() )
  {
    chance = p()->talent.assassination.blindside->effectN( 2 ).percent();
  }
  if ( p()->rng().roll( chance ) )
  {
    p()->buffs.blindside->trigger();
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_shadow_blades_attack( const action_state_t* state )
{
  if ( !p()->buffs.shadow_blades->check() || state->result_total <= 0 || !ab::result_is_hit( state->result ) || !procs_shadow_blades_damage() )
    return;

  p()->active.shadow_blades_attack->trigger_residual_action( state, p()->buffs.shadow_blades->check_value(),
                                                             false, true, nullptr, false );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_find_weakness( const action_state_t* state, timespan_t duration )
{
  if ( !ab::result_is_hit( state->result ) )
    return;

  // Subtlety duration-triggered Find Weakness applications do not require the talent
  if ( !( p()->talent.subtlety.find_weakness->ok() || duration > timespan_t::zero() ) )
    return;

  // If the new duration (e.g. from Backstab crits) is lower than the existing debuff duration, refresh without change.
  // Additionally, Subtlety-triggered debuffs are triggered with the full 30% value regardless of the talented state
  if ( duration > timespan_t::zero() )
  {
    if ( duration < td( state->target )->debuffs.find_weakness->remains() )
      duration = td( state->target )->debuffs.find_weakness->remains();

    const double trigger_value = p()->spec.find_weakness_debuff->effectN( 1 ).percent();
    td( state->target )->debuffs.find_weakness->trigger( 1, trigger_value, -1.0, duration );
  }
  else
  {
    td( state->target )->debuffs.find_weakness->trigger( duration );
  }  
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_master_of_shadows()
{
  // Since Stealth from expiring Vanish cannot trigger this but expire_override already treats vanish as gone, we have to do this manually using this function.
  if ( p()->in_combat && p()->talent.subtlety.master_of_shadows->ok() )
  {
    p()->buffs.master_of_shadows->trigger();
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_dashing_scoundrel( const action_state_t* state )
{
  if ( !affected_by.dashing_scoundrel )
    return;

  // 2021-02-21-- Use the Crit-modifier whitelist to control this as it currently matches
  if ( state->result != RESULT_CRIT || !p()->buffs.envenom->check() )
    return;

  p()->resource_gain( RESOURCE_ENERGY, p()->spec.dashing_scoundrel_gain, p()->gains.dashing_scoundrel );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_keep_it_rolling()
{
  if ( !p()->talent.outlaw.keep_it_rolling->ok() )
    return;

  timespan_t extend_duration = timespan_t::from_seconds( p()->talent.outlaw.keep_it_rolling->effectN( 1 ).base_value() );
  debug_cast<buffs::roll_the_bones_t*>( p()->buffs.roll_the_bones )->extend_secondary_buffs( extend_duration );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_perforated_veins( const action_state_t* state )
{
  if ( !p()->talent.subtlety.perforated_veins->ok() || !ab::result_is_hit( state->result ) )
    return;

  // 2023-10-03 -- Cannot accumulate stacks via WM procs while the damage buff is active
  if ( p()->buffs.perforated_veins->check() )
    return;

  p()->buffs.perforated_veins_counter->trigger();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_lingering_shadow( const action_state_t* state )
{
  if ( !p()->talent.subtlety.lingering_shadow->ok() || !ab::result_is_hit( state->result ) )
    return;

  // Testing appears to show this does not work on Weaponmaster attacks
  if ( is_secondary_action() )
    return;

  int stacks = p()->buffs.lingering_shadow->stack();
  if ( stacks < 1 )
    return;

  // Tooltips imply a bonus of 1% per stack, appears to use mitigated result
  double amount = state->result_mitigated * ( stacks / 100.0 );
  p()->active.lingering_shadow->trigger_residual_action( state->target, amount );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_danse_macabre( const action_state_t* )
{
  if ( !p()->talent.subtlety.danse_macabre->ok() )
    return;

  // 2023-10-19 -- Shuriken Tornado can now trigger DM stacks as a Shuriken Storm
  if ( ( ab::background || ab::trigger_gcd == 0_ms || !affected_by.danse_macabre ) &&
       secondary_trigger_type != secondary_trigger::SHURIKEN_TORNADO )
    return;

  if ( !p()->stealthed( STEALTH_SHADOW_DANCE ) )
    return;

  if ( !range::contains( p()->danse_macabre_tracker, ab::data().id() ) )
  {
    p()->danse_macabre_tracker.push_back( ab::data().id() );
    p()->buffs.danse_macabre->increment();
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_caustic_spatter( const action_state_t* state )
{
  if ( !p()->talent.assassination.caustic_spatter->ok() || state->result_total <= 0 || !ab::result_is_hit( state->result ) )
    return;

  if ( !procs_caustic_spatter() )
    return;

  if ( p()->sim->active_enemies == 1 )
    return;

  if ( !td( state->target )->debuffs.caustic_spatter->up() )
    return;

  double multiplier = p()->spec.caustic_spatter_buff->effectN( 1 ).percent();
  p()->active.caustic_spatter->trigger_residual_action( state, multiplier );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_caustic_spatter_debuff( const action_state_t* state )
{
  if ( !p()->talent.assassination.caustic_spatter->ok() || !ab::result_is_hit( state->result ) )
    return;

  // Caustic Spatter is checked after impacts have an opportunity to trigger poisons
  auto tdata = td( state->target );
  if ( tdata->dots.rupture->is_ticking() && ( tdata->dots.deadly_poison->is_ticking() ||
                                              tdata->debuffs.amplifying_poison->check() ||
                                              tdata->debuffs.amplifying_poison_deathmark->check() ) )
  {
    // Caustic Spatter debuff can only exist on one target at a time
    tdata->debuffs.caustic_spatter->trigger();
    for ( auto t : p()->sim->target_non_sleeping_list )
    {
      if ( t != state->target )
      {
        td( t )->debuffs.caustic_spatter->expire();
      }
    }
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_shadowcraft( const action_state_t* )
{
  if ( !p()->talent.subtlety.shadowcraft->ok() || !ab::hit_any_target )
    return;

  if ( !p()->buffs.shadow_dance->check() )
    return;

  if ( ab::base_costs[ RESOURCE_COMBO_POINT ] == 0 )
    return;

  // Shadowcraft refunds only trigger if the current available Shadow Techniques stacks will bring you to maximum
  // Needs to be delayed as consume_resource for finishers doesn't trigger until post-impact
  make_event( *p()->sim, [ this ] {
    const int current_deficit = as<int>( p()->consume_cp_max() - p()->current_cp() );
    if ( current_deficit > 0 && p()->buffs.shadow_techniques->check() >= current_deficit )
    {
      trigger_combo_point_gain( current_deficit, p()->gains.shadow_techniques_shadowcraft );
      p()->buffs.shadow_techniques->decrement( current_deficit );
    }
  } );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_cut_to_the_chase( const action_state_t* state )
{
  if ( !p()->spell.cut_to_the_chase->ok() || !ab::result_is_hit( state->result ) )
    return;

  if ( ab::base_costs[ RESOURCE_COMBO_POINT ] == 0 )
    return;

  double extend_duration = p()->spell.cut_to_the_chase->effectN( 1 ).base_value() * cast_state( state )->get_combo_points();
  // Max duration it extends to is the maximum possible full pandemic duration, i.e. around 46s without and 54s with Deeper Stratagem.
  timespan_t max_duration = ( p()->consume_cp_max() + 1 ) * p()->buffs.slice_and_dice->data().duration() * 1.3;
  timespan_t effective_extend = std::min( timespan_t::from_seconds( extend_duration ), max_duration - p()->buffs.slice_and_dice->remains() );
  p()->buffs.slice_and_dice->extend_duration_or_trigger( effective_extend, p() );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_cloud_cover( const action_state_t* state )
{
  if ( !p()->talent.trickster.cloud_cover->ok() || !ab::result_is_hit( state->result ) )
    return;

  if ( !p()->buffs.cloud_cover->check() )
    return;

  p()->get_target_data( state->target )->debuffs.fazed->trigger();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_deathstalkers_mark( const action_state_t* state, bool ignore_cp )
{
  if ( !p()->talent.deathstalker.deathstalkers_mark->ok() )
    return;

  if ( ab::base_costs[ RESOURCE_COMBO_POINT ] == 0 && !ignore_cp )
    return;

  // 2025-06-28 -- Deathstalker's Mark can be consumed via Symbols of Death with the TWW3 4pc set bonus
  player_t* mark_target = state->target->is_enemy() ? state->target : p()->target;

  // 2024-11-30 -- When Darkest Night is active, Deathstalker's Mark cannot be reduced below the application stacks
  const bool darkest_night_bug = ( p()->bugs && p()->buffs.darkest_night->check() &&
                                   ( p()->get_target_data( mark_target )->debuffs.deathstalkers_mark->check() <=
                                     p()->spell.darkest_night_buff->effectN( 3 ).base_value() ) );
  if ( !darkest_night_bug && p()->get_target_data( mark_target )->debuffs.deathstalkers_mark->check() &&
       ( ignore_cp || cast_state( state )->get_combo_points() >= as<int>( p()->talent.deathstalker.deathstalkers_mark->effectN( 2 ).base_value() ) ) )
  {
    p()->get_target_data( mark_target )->debuffs.deathstalkers_mark->decrement();
    p()->buffs.deathstalkers_mark->trigger();
    p()->active.deathstalker.deathstalkers_mark->execute_on_target( mark_target );

    if ( p()->talent.deathstalker.shadewalker->ok() )
    {
      p()->cooldowns.shadowstep->adjust( p()->talent.deathstalker.shadewalker->effectN( 1 ).time_value() );
    }

    if ( p()->talent.deathstalker.darkest_night->ok() && !p()->deathstalkers_mark_debuff->check() )
    {
      p()->resource_gain( RESOURCE_ENERGY, p()->spell.darkest_night_buff->effectN( 1 ).resource(), p()->gains.darkest_night );
      // Needs to be delayed so that Shadowed Finishers casts don't immediately benefit
      make_event( *p()->sim, 1_ms, [ this ] {
        p()->buffs.darkest_night->trigger();
      } );
      return; // Note: Cannot immediately trigger and consume Darkest Night
    }
  }

  if ( ( affected_by.darkest_night || affected_by.darkest_night_crit ) && p()->buffs.darkest_night->check() &&
       cast_state( state )->get_combo_points() >= p()->consume_cp_max() )
  {
    trigger_deathstalkers_mark_debuff( state, true );
    p()->buffs.darkest_night->expire( 1_ms ); // Expire with delay for potential Shadowy Finishers support
  }
}

template <typename Base>
bool actions::rogue_action_t<Base>::trigger_deathstalkers_mark_debuff( const action_state_t* state, bool from_darkest_night )
{
  if ( !p()->talent.deathstalker.deathstalkers_mark->ok() )
    return false;

  if ( !p()->stealthed( STEALTH_BASIC | STEALTH_ROGUE ) && !from_darkest_night )
    return false;

  buff_t*& debuff = p()->deathstalkers_mark_debuff;
  if ( debuff && debuff->check() )
  {
    // 2024-06-25 -- Can no longer be re-applied if the target has a Deathstalker's Mark
    if ( debuff->player == state->target )
      return false;

    debuff->expire();
  }

  const int stacks = as<int>( from_darkest_night ? p()->spell.darkest_night_buff->effectN( 3 ).base_value() :
                              p()->talent.deathstalker.deathstalkers_mark->effectN( 1 ).base_value() );

  debuff = p()->get_target_data( state->target )->debuffs.deathstalkers_mark;
  debuff->trigger( stacks );

  p()->buffs.clear_the_witnesses->trigger();

  return true;
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_unseen_blade( const action_state_t* state )
{
  if ( !p()->talent.trickster.unseen_blade->ok() || !ab::result_is_hit( state->result ) )
    return;

  if ( p()->cooldowns.unseen_blade_icd->down() )
    return;

  if ( p()->buffs.unseen_blade_cd->check() && !p()->buffs.disorienting_strikes->check() )
    return;

  assert( p()->active.trickster.unseen_blade );

  p()->cooldowns.unseen_blade_icd->start();

  p()->active.trickster.unseen_blade->execute_on_target( state->target );
  
  if ( p()->set_bonuses.tww3_trickster_2pc->ok() &&
       p()->rng().roll( p()->set_bonuses.tww3_trickster_2pc->effectN( 2 ).percent() ) )
  {
    p()->active.trickster.unseen_blade->execute_on_target( state->target );
  }

  if ( p()->buffs.disorienting_strikes->check() )
    p()->buffs.disorienting_strikes->decrement();
  else
    p()->buffs.unseen_blade_cd->trigger();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_nimble_flurry( const action_state_t* state )
{
  // Outlaw gains a bonus to Blade Flurry instead of triggering this effect
  if ( !p()->talent.trickster.nimble_flurry->ok() || p()->specialization() != ROGUE_SUBTLETY )
    return;

  if ( !procs_nimble_flurry() )
    return;

  if ( p()->sim->active_enemies == 1 )
    return;

  if ( !ab::result_is_hit( state->result ) || !p()->buffs.flawless_form->check() )
    return;

  double multiplier = p()->talent.trickster.nimble_flurry->effectN( 3 ).percent();

  // 2025-08-02 -- Not yet in spell data descriptions, but in patch notes and appears to work
  if ( p()->buffs.shadow_blades->check() )
  {
    multiplier *= 1.0 + p()->buffs.shadow_blades->check_value();
  }

  p()->active.trickster.nimble_flurry->trigger_residual_action( state, multiplier );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_supercharger()
{
  if ( !p()->talent.rogue.supercharger->ok() )
    return;

  double trigger_buffs = p()->talent.rogue.supercharger->effectN( 1 ).base_value();
  for ( buff_t* b : p()->buffs.supercharger )
  {
    if ( trigger_buffs <= 0 )
      return;

    if ( !b->check() )
    {
      b->trigger();
      trigger_buffs--;
    }
  }

  if ( trigger_buffs > 0 )
  {
    p()->procs.supercharger_wasted->occur();
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_echoing_reprimand( const action_state_t* state )
{
  if ( !p()->talent.rogue.echoing_reprimand->ok() )
    return;

  if ( !p()->buffs.echoing_reprimand->check() )
    return;

  p()->active.echoing_reprimand->execute_on_target( state->target );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_tww1_assassination_set_bonus( const action_state_t* state )
{
  if ( !p()->set_bonuses.tww1_assassination_2pc->ok() )
    return;

  if ( state->result_type != result_amount_type::DMG_OVER_TIME )
    return;

  // TOCHECK -- May require more specific whitelisting in the future but seems to match
  if ( ab::school != SCHOOL_PHYSICAL )
    return;

  p()->buffs.tww1_assassination_2pc->trigger();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_tww1_outlaw_set_bonus( const action_state_t* state )
{
  if ( !p()->set_bonuses.tww1_outlaw_2pc->ok() )
    return;

  if ( !p()->rng().roll( p()->set_bonuses.tww1_outlaw_2pc->effectN( 1 ).percent() ) )
    return;

  // TOCHECK -- Double-check target modifiers in the future
  p()->active.tww1.ethereal_rampage->trigger_residual_action( state, p()->set_bonuses.tww1_outlaw_2pc->effectN( 2 ).percent() );

  if ( p()->set_bonuses.tww1_outlaw_4pc->ok() )
    p()->buffs.tww1_outlaw_4pc->increment();
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_tww2_set_bonus_removal()
{
  // 2025-02-26 -- 4pc can self-proc when up, unclear if this is intentional or not
  if ( p()->set_bonuses.tww2_assassination_2pc->ok() &&
       ( p()->buffs.tww2_assassination_2pc->check() || ( p()->bugs && p()->buffs.tww2_assassination_4pc->check() ) ) &&
       p()->rng().roll( p()->set_bonuses.tww2_assassination_2pc->effectN( 1 ).percent() ) )
  {
    // 2025-02-08 -- Based on testing, 20% chance is nowhere in spell data currently
    // 2025-02-20 -- Updated to 66%
    if ( p()->set_bonuses.tww2_assassination_4pc->ok() && p()->rng().roll( 0.66 ) )
    {
      // Buff stack change callback for the 4pc handles expiration and restoration of 2pc buff stacks
      // If the 4pc refreshes itself, the restoration value (default_value) gets updated to 10 stacks
      if ( p()->buffs.tww2_assassination_4pc->check() )
      {
        p()->buffs.tww2_assassination_4pc->trigger( p()->buffs.tww2_assassination_4pc->max_stack() );
        p()->buffs.tww2_assassination_4pc->default_value = p()->buffs.tww2_assassination_4pc->max_stack();
      }
      else
      {
        p()->buffs.tww2_assassination_4pc->trigger( p()->buffs.tww2_assassination_4pc->max_stack() );
      }
      return;
    }

    p()->buffs.tww2_assassination_2pc->expire();
  }

  if ( p()->set_bonuses.tww2_outlaw_2pc->ok() && p()->buffs.tww2_outlaw_2pc->check() &&
       p()->rng().roll( p()->set_bonuses.tww2_outlaw_2pc->effectN( 1 ).percent() ) )
  {
    p()->buffs.tww2_outlaw_2pc->expire();
  }

  if ( p()->set_bonuses.tww2_subtlety_2pc->ok() && p()->buffs.tww2_subtlety_2pc->check() &&
       p()->rng().roll( p()->set_bonuses.tww2_subtlety_2pc->effectN( 1 ).percent() ) )
  {
    if ( p()->set_bonuses.tww2_subtlety_4pc->ok() && p()->buffs.shadow_dance->check() )
    {
      p()->procs.tww2_subtlety_4pc->occur();
      return;
    }

    p()->buffs.tww2_subtlety_2pc->expire();
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
  dots.crimson_tempest          = target->get_dot( "crimson_tempest", source );
  dots.deadly_poison            = target->get_dot( "deadly_poison_dot", source );
  dots.deadly_poison_deathmark  = target->get_dot( "deadly_poison_dot_deathmark", source );
  dots.deathmark                = target->get_dot( "deathmark", source );
  dots.garrote                  = target->get_dot( "garrote", source );
  dots.garrote_deathmark        = target->get_dot( "garrote_deathmark", source );
  dots.internal_bleeding        = target->get_dot( "internal_bleeding", source );
  dots.kingsbane                = target->get_dot( "kingsbane", source );
  dots.mutilated_flesh          = target->get_dot( "mutilated_flesh", source );
  dots.rupture                  = target->get_dot( "rupture", source );
  dots.rupture_deathmark        = target->get_dot( "rupture_deathmark", source );
  dots.soulrip                  = target->get_dot( "soulrip", source );

  debuffs.wound_poison          = new buffs::wound_poison_t( *this );
  debuffs.atrophic_poison       = new buffs::atrophic_poison_t( *this );
  debuffs.crippling_poison      = new buffs::crippling_poison_t( *this );
  debuffs.numbing_poison        = new buffs::numbing_poison_t( *this );

  debuffs.amplifying_poison = make_buff( *this, "amplifying_poison", source->spec.amplifying_poison_debuff );
  debuffs.amplifying_poison_deathmark = make_buff( *this, "amplifying_poison_deathmark", source->spec.deathmark_amplifying_poison );
  
  // 2023-10-25 -- Effects 3 and 4 are PvP-only, not in spell data
  debuffs.deathmark = make_buff<damage_buff_t>( *this, "deathmark", source->spec.deathmark_debuff, false )
    ->set_direct_mod( source->spec.deathmark_debuff, 1 )
    ->set_periodic_mod( source->spec.deathmark_debuff, 2 );
  debuffs.deathmark->set_cooldown( timespan_t::zero() );

  debuffs.caustic_spatter = make_buff( *this, "caustic_spatter", source->spec.caustic_spatter_buff )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION ); // TOCHECK
  debuffs.find_weakness = make_buff( *this, "find_weakness", source->spec.find_weakness_debuff )
    ->set_default_value( source->talent.subtlety.find_weakness->effectN( 1 ).percent() )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION );

  debuffs.corrupt_the_blood = make_buff( *this, "corrupt_the_blood", source->spell.corrupt_the_blood_damage )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  debuffs.deathstalkers_mark = make_buff( *this, "deathstalkers_mark", source->spell.deathstalkers_mark_debuff );
  debuffs.fatal_intent = make_buff( *this, "fatal_intent", source->spell.fatal_intent_debuff )
    ->set_default_value( source->talent.deathstalker.fatal_intent->effectN( 1 ).ap_coeff() );
  debuffs.fazed = make_buff<damage_buff_t>( *this, "fazed", source->spell.fazed_debuff );
  debuffs.fazed->set_refresh_duration_callback( []( const buff_t* b, timespan_t d ) {
    return std::min( b->remains() + d, 10_s );  // Capped to 10 seconds, not in spell data
  } );

  // Type-Based Tracking for Accumulators
  bleeds = { dots.deathmark, dots.garrote, dots.garrote_deathmark, dots.internal_bleeding,
             dots.rupture, dots.rupture_deathmark, dots.crimson_tempest, dots.mutilated_flesh };
  poison_dots = { dots.deadly_poison, dots.deadly_poison_deathmark, dots.kingsbane };
  poison_debuffs = { debuffs.atrophic_poison, debuffs.crippling_poison, debuffs.numbing_poison,
                     debuffs.wound_poison, debuffs.amplifying_poison, debuffs.amplifying_poison_deathmark };

  // Callbacks ================================================================

  // Venomous Wounds Energy Refund
  if ( source->talent.assassination.venomous_wounds->ok() )
  {
    target->register_on_demise_callback( source, [ source ]( player_t* target ) { 
      source->trigger_venomous_wounds_death( target );
    } );
  }

  // Darkest Night On-Death Buff Trigger
  if ( source->talent.deathstalker.darkest_night->ok() )
  {
    target->register_on_demise_callback( source, [ this, source ]( player_t* ) {
      if ( debuffs.deathstalkers_mark->check() )
      {
        source->resource_gain( RESOURCE_ENERGY, source->spell.darkest_night_buff->effectN( 1 ).resource(), source->gains.darkest_night );
        source->buffs.darkest_night->trigger();
      }
    } );
  }
}

// ==========================================================================
// Rogue Character Definition
// ==========================================================================

// rogue_t::composite_attribute_multiplier ==================================

double rogue_t::composite_attribute_multiplier( attribute_e a ) const
{
  double am = player_t::composite_attribute_multiplier( a );

  return am;
}

// rogue_t::composite_melee_auto_attack_speed ===============================

double rogue_t::composite_melee_auto_attack_speed() const
{
  double h = player_t::composite_melee_auto_attack_speed();

  if ( buffs.slice_and_dice->up() )
  {
    double buff_value = buffs.slice_and_dice->check_value() +
      ( talent.rogue.swift_slasher->ok() ? ( 1.0 / cache.attack_haste() ) - 1.0 : 0.0 );
    h *= 1.0 / ( 1.0 + buff_value );
  }

  if ( buffs.adrenaline_rush->check() )
  {
    h *= 1.0 / ( 1.0 + buffs.adrenaline_rush->check_value() );
  }

  if ( talent.subtlety.warning_signs->ok() && buffs.shadow_dance->up() )
  {
    h *= 1.0 + talent.subtlety.warning_signs->effectN( 1 ).percent();
  }

  return h;
}

// rogue_t::composite_melee_haste ==========================================

double rogue_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  return h;
}

// rogue_t::composite_spell_haste ==========================================

double rogue_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  return h;
}

// rogue_t::composite_melee_crit_chance =========================================

double rogue_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  return crit;
}

// rogue_t::composite_spell_crit_chance =========================================

double rogue_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  return crit;
}

// rogue_t::composite_damage_versatility ===================================

double rogue_t::composite_damage_versatility() const
{
  double cdv = player_t::composite_damage_versatility();

  return cdv;
}

// rogue_t::composite_heal_versatility =====================================

double rogue_t::composite_heal_versatility() const
{
  double chv = player_t::composite_heal_versatility();

  return chv;
}

// rogue_t::composite_leech ===============================================

double rogue_t::composite_leech() const
{
  double l = player_t::composite_leech();

  l += spell.leeching_poison_buff->effectN( 1 ).percent();

  if ( talent.fatebound.chosens_revelry->ok() ) {
    // a server side script enables leech effect in base buffs - value in the talent's effect is "50", which doesnt seem to reflect the 0.5% it promises and applies
    l += (buffs.fatebound_coin_heads->check() + buffs.fatebound_coin_tails->check()) * 0.005;
  }

  return l;
}

// rogue_t::matching_gear_multiplier ========================================

double rogue_t::matching_gear_multiplier( attribute_e attr ) const
{
  return player_t::matching_gear_multiplier( attr );
}

// rogue_t::composite_player_multiplier =====================================

double rogue_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( talent.deathstalker.lingering_darkness->ok() )
  {
    // 2024-08-12 -- Handled via hidden conditional aura for specializations
    auto effect = ( specialization() == ROGUE_ASSASSINATION ?
                    spell.lingering_darkness_buff->effectN( 1 ) :
                    spell.lingering_darkness_buff->effectN( 2 ) );

    if ( effect.has_common_school( school ) )
    {
      m *= 1.0 + buffs.lingering_darkness->value() *
        ( 1.0 + buffs.tww3_deathstalker_2pc->check() * spec.tww3_deathstalker_2pc_buff->effectN( 2 ).percent() );
    }
  }

  return m;
}

// rogue_t::composite_player_pet_damage_multiplier ==========================

double rogue_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );

  return m;
}

// rogue_t::composite_player_target_multiplier ==============================

double rogue_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  return m;
}

// rogue_t::composite_player_target_crit_chance =============================

double rogue_t::composite_player_target_crit_chance( player_t* target ) const
{
  double c = player_t::composite_player_target_crit_chance( target );

  return c;
}

// rogue_t::composite_player_target_armor ===================================

double rogue_t::composite_player_target_armor( player_t* target ) const
{
  double a = player_t::composite_player_target_armor( target );

  a *= 1.0 - get_target_data( target )->debuffs.find_weakness->value();

  return a;
}

// rogue_t::default_flask ===================================================

std::string rogue_t::default_flask() const
{
  return rogue_apl::flask( this );
}

// rogue_t::default_potion ==================================================

std::string rogue_t::default_potion() const
{
  return rogue_apl::potion( this );
}

// rogue_t::default_food ====================================================

std::string rogue_t::default_food() const
{
  return rogue_apl::food( this );
}

// rogue_t::default_rune ====================================================

std::string rogue_t::default_rune() const
{
  return rogue_apl::rune( this );
}

// rogue_t::default_temporary_enchant =======================================

std::string rogue_t::default_temporary_enchant() const
{
  return rogue_apl::temporary_enchant( this );
}

// rogue_t::init_actions ====================================================

void rogue_t::init_action_list()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim->errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();

  if ( specialization() == ROGUE_ASSASSINATION )
  {
    rogue_apl::assassination( this );
  }
  else if ( specialization() == ROGUE_OUTLAW )
  {
    rogue_apl::outlaw( this );
  }
  else if ( specialization() == ROGUE_SUBTLETY )
  {
    rogue_apl::subtlety( this );
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// rogue_t::action_names_from_spell_id ====================================================

std::vector<std::string> rogue_t::action_names_from_spell_id( unsigned int spell_id ) const
{
  auto name = find_spell( spell_id )->name_cstr();
  auto splits = util::string_split( util::tokenize_fn( name ), "_" );
  auto fragment = !splits.empty() ? splits[ 0 ] : "";

  std::string poison_type = "";
  switch ( spell_id )
  {
  case 2823:   // deadly poison
  case 315584: // instant poison
  case 381664: // amplifying poison
  case 8679:   // wound poison
    poison_type = "lethal";
    break;
  case 381637: // atrophic poison
  case 3408:   // crippling poison
  case 5761:   // numbing poison
    poison_type = "nonlethal";
    break;
  default:
    break;
  }

  // Just return nothing for apply_poison for now because the conditions are nonsense
  if ( !poison_type.empty() )
    return {}; // return { fmt::format( "apply_poison,{}={}", type, fragment ) };

  if ( spell_id == 196819 && specialization() == ROGUE_OUTLAW )
    return { "dispatch", "coup_de_grace" };
  if ( spell_id == 196819 && specialization() == ROGUE_SUBTLETY )
    return { "eviscerate", "coup_de_grace" };
  if ( spell_id == 196819 && specialization() == ROGUE_ASSASSINATION )
    return { "envenom", "coup_de_grace" };

  if ( spell_id == 1752 && specialization() == ROGUE_ASSASSINATION )
    return { "mutilate" };
  if ( spell_id == 1752 && specialization() == ROGUE_SUBTLETY )
    return { "backstab", "gloomblade" };

  return player_t::action_names_from_spell_id( spell_id );
}

// rogue_t::action_names_from_spell_id ====================================================

parsed_assisted_combat_rule_t rogue_t::parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                                   const assisted_combat_step_data_t& step ) const
{
  if ( rule.condition_type == AC_AURA_MISSING_PLAYER )
  {
    switch ( rule.condition_value_1 )
    {
      case 2823:    // deadly poison
      case 315584:  // instant poison
      case 381664:  // amplifying poison
      case 8679:    // wound poison
      case 381637:  // atrophic poison
      case 3408:    // crippling poison
      case 5761:    // numbing poison
        return { "1",
                 "apply_poison may only be executed once per iteration, so we do not need to condition based on poison "
                 "status." };
      default:
        break;
    }
  }

  if ( rule.condition_type == AC_TARGET_COUNT_NEAR_TARGET_GREATER ||
       rule.condition_type == AC_TARGET_COUNT_NEAR_PLAYER_GREATER )
    return fmt::format( "active_enemies>={}", rule.condition_value_1 );

  if ( rule.condition_type == AC_COMBO_POINTS_GREATER )
    return fmt::format( "effective_combo_points>={}", rule.condition_value_1 );

  if ( rule.condition_type == AC_COMBO_POINTS_LESS )
    return fmt::format( "effective_combo_points<={}", rule.condition_value_1 );

  // Stealth + Vanish checks
  if ( rule.condition_type == AC_AURA_ON_PLAYER && ( rule.condition_value_1 == 1784 || rule.condition_value_1 == 115191 || rule.condition_value_1 == 11327 ) )
    return { "stealthed.basic", "Generic stealth check replacement expression for multiple buff checks", true, false };

  if ( rule.condition_type == AC_AURA_ON_PLAYER && rule.condition_value_1 == 51667 )
    return { "1", "Checks if the automatically learned passive Cut to the Chase is known. Assumed to be strictly true." };

  if ( rule.condition_type == AC_AURA_MISSING_PLAYER && rule.condition_value_1 == 51667 )
    return { "0", "Checks if the automatically learned passive Cut to the Chase is known. Assumed to be strictly false." };

  if ( rule.condition_type == AC_AURA_ON_PLAYER && rule.condition_value_1 == 462128 )
    return "action.coup_de_grace.ready";

  return player_t::parse_assisted_combat_rule( rule, step );
}

// rogue_t::init_blizzard_action_list =====================================================

void rogue_t::init_blizzard_action_list()
{
  player_t::init_blizzard_action_list();

  // Replacement for Blizzard's nonsensical apply_poison logic
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  precombat->add_action( "apply_poison" );

  if ( use_cds_with_blizzard_action_list )
  {
    action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );

    switch ( specialization() )
    {
      case ROGUE_ASSASSINATION:
        cooldowns->add_action( "vanish,if=!stealthed.all" );
        cooldowns->add_action( "deathmark,if=!talent.kingsbane|cooldown.kingsbane.ready" );
        break;
      case ROGUE_OUTLAW:
        cooldowns->add_action( "adrenaline_rush,if=!buff.adrenaline_rush.up" );
        cooldowns->add_action( "vanish,if=!stealthed.all" );
        cooldowns->add_action( "keep_it_rolling,if=rtb_buffs>=4" );
        break;
      case ROGUE_SUBTLETY:
        cooldowns->add_action( "shadow_blades,if=buff.shadow_dance.up" );
        break;
      default:
        break;
    }
  }
}

// rogue_t::create_action  ==================================================

action_t* rogue_t::create_action( util::string_view name, util::string_view options_str )
{
  using namespace actions;

  if ( name == "adrenaline_rush"        ) return new adrenaline_rush_t        ( name, this, options_str );
  if ( name == "ambush"                 ) return new ambush_t                 ( name, this, options_str );
  if ( name == "apply_poison"           ) return new apply_poison_t           ( this, options_str );
  if ( name == "auto_attack"            ) return new auto_melee_attack_t      ( this, options_str );
  if ( name == "backstab"               ) return new backstab_t               ( name, this, options_str );
  if ( name == "between_the_eyes"       ) return new between_the_eyes_t       ( name, this, options_str );
  if ( name == "black_powder"           ) return new black_powder_t           ( name, this, options_str );
  if ( name == "blade_flurry"           ) return new blade_flurry_t           ( name, this, options_str );
  if ( name == "blade_rush"             ) return new blade_rush_t             ( name, this, options_str );
  if ( name == "cheap_shot"             ) return new cheap_shot_t             ( name, this, options_str );
  if ( name == "coup_de_grace"          ) return new coup_de_grace_t          ( name, this, options_str );
  if ( name == "crimson_tempest"        ) return new crimson_tempest_t        ( name, this, options_str );
  if ( name == "deathmark"              ) return new deathmark_t              ( name, this, options_str );
  if ( name == "detection"              ) return new detection_t              ( name, this, options_str );
  if ( name == "distract"               ) return new distract_t               ( name, this, options_str );
  if ( name == "dispatch"               ) return new dispatch_t               ( name, this, options_str );
  if ( name == "envenom"                ) return new envenom_t                ( name, this, options_str );
  if ( name == "eviscerate"             ) return new eviscerate_t             ( name, this, options_str );
  if ( name == "fan_of_knives"          ) return new fan_of_knives_t          ( name, this, options_str );
  if ( name == "feint"                  ) return new feint_t                  ( name, this, options_str );
  if ( name == "garrote"                ) return new garrote_t                ( name, this, spec.garrote, options_str );
  if ( name == "goremaws_bite"          ) return new goremaws_bite_t          ( name, this, options_str );
  if ( name == "gouge"                  ) return new gouge_t                  ( name, this, options_str );
  if ( name == "gloomblade"             ) return new gloomblade_t             ( name, this, options_str );
  if ( name == "keep_it_rolling"        ) return new keep_it_rolling_t        ( name, this, options_str );
  if ( name == "kick"                   ) return new kick_t                   ( name, this, options_str );
  if ( name == "kidney_shot"            ) return new kidney_shot_t            ( name, this, options_str );
  if ( name == "killing_spree"          ) return new killing_spree_t          ( name, this, options_str );
  if ( name == "kingsbane"              ) return new kingsbane_t              ( name, this, options_str );
  if ( name == "mutilate"               ) return new mutilate_t               ( name, this, options_str );
  if ( name == "pistol_shot"            ) return new pistol_shot_t            ( name, this, options_str );
  if ( name == "poisoned_knife"         ) return new poisoned_knife_t         ( name, this, options_str );
  if ( name == "roll_the_bones"         ) return new roll_the_bones_t         ( name, this, options_str );
  if ( name == "rupture"                ) return new rupture_t                ( name, this, spec.rupture, options_str );
  if ( name == "secret_technique"       ) return new secret_technique_t       ( name, this, options_str );
  if ( name == "shadow_blades"          ) return new shadow_blades_t          ( name, this, options_str );
  if ( name == "shadow_dance"           ) return new shadow_dance_t           ( name, this, options_str );
  if ( name == "shadowstep"             ) return new shadowstep_t             ( name, this, options_str );
  if ( name == "shadowstrike"           ) return new shadowstrike_t           ( name, this, options_str );
  if ( name == "shuriken_storm"         ) return new shuriken_storm_t         ( name, this, options_str );
  if ( name == "shuriken_tornado"       ) return new shuriken_tornado_t       ( name, this, options_str );
  if ( name == "shuriken_toss"          ) return new shuriken_toss_t          ( name, this, options_str );
  if ( name == "sinister_strike"        ) return new sinister_strike_t        ( name, this, options_str );
  if ( name == "slice_and_dice"         ) return new slice_and_dice_t         ( name, this, options_str );
  if ( name == "sprint"                 ) return new sprint_t                 ( name, this, options_str );
  if ( name == "stealth"                ) return new stealth_t                ( name, this, options_str );
  if ( name == "shiv"                   ) return new shiv_t                   ( name, this, options_str );
  if ( name == "thistle_tea"            ) return new thistle_tea_t            ( name, this, options_str );
  if ( name == "vanish"                 ) return new vanish_t                 ( name, this, options_str );
  if ( name == "cancel_autoattack"      ) return new cancel_autoattack_t      ( this, options_str );
  if ( name == "swap_weapon"            ) return new weapon_swap_t            ( this, options_str );

  return player_t::create_action( name, options_str );
}

// rogue_t::create_expression ===============================================

std::unique_ptr<expr_t> rogue_t::create_action_expression( action_t& action, std::string_view name_str )
{
  auto split = util::string_split<util::string_view>( name_str, "." );

  if ( util::str_compare_ci( name_str, "poisoned" ) )
  {
    return make_fn_expr( name_str, [ this, &action ]() {
      rogue_td_t* tdata = get_target_data( action.get_expression_target() );
      return tdata->is_lethal_poisoned();
    } );
  }
  else if ( util::str_compare_ci( name_str, "poison_remains" ) )
  {
    return make_fn_expr( name_str, [ this, &action ]() {
      rogue_td_t* tdata = get_target_data( action.get_expression_target() );
      return tdata->lethal_poison_remains();
    } );
  }
  else if ( util::str_compare_ci( name_str, "bleeds" ) )
  {
    return make_fn_expr( name_str, [ this, &action ]() {
      rogue_td_t* tdata = get_target_data( action.get_expression_target() );
      return tdata->total_bleeds();
    } );
  }
  else if ( util::str_compare_ci( name_str, "poisons" ) )
  {
    return make_fn_expr( name_str, [ this, &action ]() {
      rogue_td_t* tdata = get_target_data( action.get_expression_target() );
      return tdata->total_poisons();
    } );
  }
  // exsanguinated.(garrote|internal_bleeding|rupture|crimson_tempest)
  else if ( split.size() == 2 && util::str_compare_ci( split[ 0 ], "exsanguinated" ) &&
            ( util::str_compare_ci( split[ 1 ], "garrote" ) ||
              util::str_compare_ci( split[ 1 ], "internal_bleeding" ) ||
              util::str_compare_ci( split[ 1 ], "rupture" ) ||
              util::str_compare_ci( split[ 1 ], "crimson_tempest" ) ) )
  {
    action_t* dot_action = find_action( split[ 1 ] );
    if ( !dot_action )
    {
      return expr_t::create_constant( "exsanguinated_expr", 0 );
    }

    return make_fn_expr( name_str, [ &action, dot_action ]() {
      dot_t* d = dot_action->get_dot( action.get_expression_target() );
      return d->is_ticking() && actions::rogue_attack_t::cast_state( d->state )->is_exsanguinated();
    } );
  }
  else if ( util::str_compare_ci( name_str, "used_for_danse" ) )
  {
    return make_fn_expr( name_str, [ this, &action ]() {
      return range::contains( danse_macabre_tracker, action.data().id() );
    } );
  }
  else if ( split[ 0 ] == "buff" && split[ 1 ] == "envenom" && split[ 2 ] == "remains" && split.size() == 4 )
  {
    size_t buff_idx = as<size_t>( util::to_int( split[ 3 ] ) );
    return make_fn_expr( name_str, [ this, buff_idx ]() {
      if ( buffs.envenom->expiration.size() < buff_idx )
        return 0_s;
      else
        return buffs.envenom->expiration[ buff_idx - 1 ]->occurs() - sim->current_time();
    } );
  }

  return player_t::create_action_expression( action, name_str );
}

std::unique_ptr<expr_t> rogue_t::create_expression( util::string_view name_str )
{
  auto split = util::string_split<util::string_view>( name_str, "." );

  if ( split[ 0 ] == "combo_points" )
  {
    if ( split.size() == 1 )
    {
      return make_fn_expr( name_str, [ this ] { return this->current_cp( true ); } );
    }

    if(split.size() == 2 && split[1] == "deficit" )
    {
      return make_fn_expr( name_str, [ this ] { 
        return resources.max[ RESOURCE_COMBO_POINT ] - this->current_cp( true ); } );
    }
  }
  else if ( name_str == "effective_combo_points" )
  {
    return make_fn_expr( name_str, [ this ]() { return current_effective_cp( true, true ); } );
  }
  else if ( util::str_compare_ci( name_str, "cp_max_spend" ) )
  {
    return make_mem_fn_expr( name_str, *this, &rogue_t::consume_cp_max );
  }
  else if ( util::str_compare_ci( name_str, "improved_garrote_remains" ) )
  {
    if ( !talent.assassination.improved_garrote->ok() )
      return expr_t::create_constant( name_str, 0 );

    return make_fn_expr( name_str, [this]() {
      timespan_t remains;
      if ( buffs.improved_garrote_aura->check() )
      {
        timespan_t nominal_duration = buffs.improved_garrote->base_buff_duration;
        timespan_t gcd_remains = timespan_t::from_seconds( std::max( ( gcd_ready - sim->current_time() ).total_seconds(), 0.0 ) );
        remains = gcd_remains + nominal_duration;
      }
      remains = std::max( { remains, buffs.improved_garrote->remains() } );
      return remains;
    } );
  }
  else if ( util::str_compare_ci( name_str, "poisoned_bleeds" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      int poisoned_bleeds = 0;
      for ( auto p : sim->target_non_sleeping_list )
      {
        rogue_td_t* tdata = get_target_data( p );
        if ( tdata->is_lethal_poisoned() )
        {
          poisoned_bleeds +=
            tdata->dots.garrote->is_ticking() +
            tdata->dots.garrote_deathmark->is_ticking() +
            tdata->dots.rupture->is_ticking() +
            tdata->dots.rupture_deathmark->is_ticking();
        }
      }
      return poisoned_bleeds;
    } );
  }
  else if ( util::str_compare_ci( split[ 0 ], "rtb_buffs" ) )
  {
    if ( !spec.roll_the_bones->ok() )
      return expr_t::create_constant( name_str, 0 );

    buffs::roll_the_bones_t* primary = static_cast<buffs::roll_the_bones_t*>( buffs.roll_the_bones );
    return make_fn_expr( name_str, [ primary ]() {
      return primary->check_value();
    } );
  }
  else if ( util::str_compare_ci(name_str, "priority_rotation") )
  {
    return expr_t::create_constant( name_str, options.priority_rotation );
  }

  // Split expressions

  // stealthed.(rogue|mantle|basic|all)
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
    else if ( util::str_compare_ci( split[ 1 ], "mantle" ) || util::str_compare_ci( split[ 1 ], "basic" ) )
    {
      return make_fn_expr( split[ 0 ], [ this ]() {
        return stealthed( STEALTH_BASIC );
      } );
    }
    else if ( util::str_compare_ci( split[ 1 ], "improved_garrote" ) )
    {
      if ( !talent.assassination.improved_garrote->ok() )
        return expr_t::create_constant( name_str, false );

      return make_fn_expr( split[ 0 ], [this]() {
        return stealthed( STEALTH_IMPROVED_GARROTE );
      } );
    }
    else if ( util::str_compare_ci( split[ 1 ], "all" ) )
    {
      return make_fn_expr( split[ 0 ], [ this ]() {
        return stealthed();
      } );
    }
  }
  // time_to_sht.(1|2|3|4)
  // time_to_sht.(1|2|3|4).plus
  // x: returns time until we will do the xth attack since last ShT proc.
  // plus: denotes to return the timer for the next swing if we are past that counter
  if ( split[ 0 ] == "time_to_sht" )
  {
    unsigned attack_x = split.size() > 1 ? util::to_unsigned( split[ 1 ] ) : 4;
    bool plus = split.size() > 2 ? split[ 2 ] == "plus" : false;
    
    return make_fn_expr( split[ 0 ], [ this, attack_x, plus ]() {
      timespan_t return_value = timespan_t::from_seconds( 0.0 );

      if ( main_hand_attack && ( attack_x > shadow_techniques_counter || plus ) && attack_x <= 4 )
      {
        unsigned remaining_aa;
        if ( attack_x <= shadow_techniques_counter && plus )
        {
          remaining_aa = 1;
          sim->print_debug( "{} time_to_sht: attack_x = {}+, count at {}, returning next", *this, attack_x, shadow_techniques_counter );
        }
        else
        {
          remaining_aa = attack_x - shadow_techniques_counter;
          sim->print_debug( "{} time_to_sht: attack_x = {}, remaining_aa = {}", *this, attack_x, remaining_aa );
        }
        
        timespan_t mh_swing_time = main_hand_attack->execute_time();
        timespan_t mh_next_swing = timespan_t::from_seconds( 0.0 );
        if ( main_hand_attack->execute_event == nullptr )
        {
          mh_next_swing = mh_swing_time;
        }
        else
        {
          mh_next_swing = main_hand_attack->execute_event->remains();
        }
        sim->print_debug( "{} time_to_sht: MH swing_time: {}, next_swing in: {}", *this, mh_swing_time, mh_next_swing );

        timespan_t oh_swing_time = timespan_t::from_seconds( 0.0 );
        timespan_t oh_next_swing = timespan_t::from_seconds( 0.0 );
        if ( off_hand_attack )
        {
          oh_swing_time = off_hand_attack->execute_time();
          if ( off_hand_attack->execute_event == nullptr )
          {
            oh_next_swing = oh_swing_time;
          }
          else
          {
            oh_next_swing = off_hand_attack->execute_event->remains();
          }
          sim->print_debug( "{} time_to_sht: OH swing_time: {}, next_swing in: {}", *this, oh_swing_time, oh_next_swing );
        }
        else
        {
          sim->print_debug( "{} time_to_sht: OH attack not found, using only MH timers", *this );
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

        // Add player reaction time to the predicted value as players still need to react to the swing and proc
        timespan_t total_reaction_time = this->total_reaction_time();
        return_value = attacks.at( remaining_aa - 1 ) + total_reaction_time;
      }
      else if ( main_hand_attack == nullptr )
      {
        return_value = timespan_t::from_seconds( 0.0 );
        sim->print_debug( "{} time_to_sht: MH attack is required but was not found", *this );
      }
      else if ( attack_x > 4 )
      {
        return_value = timespan_t::from_seconds( 0.0 );
        sim->print_debug( "{} time_to_sht: Invalid value {} for attack_x (must be 4 or less)", *this, attack_x );
      }
      else
      {
        return_value = timespan_t::from_seconds( 0.0 );
        sim->print_debug( "{} time_to_sht: attack_x value {} is not greater than shadow techniques count {}", *this, attack_x, shadow_techniques_counter );
      }
      sim->print_debug( "{} time_to_sht: return value is: {}", *this, return_value );
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
    // Custom Rogue Energy Deficit
    // Ignores temporary energy max when calculating the current deficit
    if ( splits.size() == 2 && ( splits[ 1 ] == "base_deficit" ) )
    {
      return make_fn_expr( name_str, [this] {
        return std::max( resources.max[ RESOURCE_ENERGY ] -
                         resources.current[ RESOURCE_ENERGY ] -
                         resources.temporary[ RESOURCE_ENERGY ], 0.0 );
      } );
    }
    // Custom Rogue Energy Regen Functions
    // Optionally handles things that are outside of the normal resource_regen_per_second flow
    if ( splits.size() == 2 && ( splits[ 1 ] == "regen" || splits[ 1 ] == "regen_combined" ||
                                 splits[ 1 ] == "time_to_max" || splits[ 1 ] == "time_to_max_combined" || 
                                 splits[ 1 ] == "base_time_to_max" || splits[ 1 ] == "base_time_to_max_combined" ) )
    {
      const bool regen = util::str_prefix_ci( splits[ 1 ], "regen" );
      const bool combined = util::str_in_str_ci( splits[ 1 ], "_combined" );
      const bool base_max = util::str_prefix_ci( splits[ 1 ], "base" );

      return make_fn_expr( name_str, [ this, regen, combined, base_max] {
        double energy_deficit = resources.max[ RESOURCE_ENERGY ] - resources.current[ RESOURCE_ENERGY ];
        double energy_regen_per_second = resource_regen_per_second( RESOURCE_ENERGY );

        if ( base_max )
        {
          energy_deficit = std::max( energy_deficit - resources.temporary[ RESOURCE_ENERGY ], 0.0 );
        }

        if ( buffs.adrenaline_rush->check() )
        {
          energy_regen_per_second *= 1.0 + buffs.adrenaline_rush->data().effectN( 1 ).percent();
        }

        if ( buffs.blade_rush->check() )
        {
          energy_regen_per_second += ( buffs.blade_rush->data().effectN( 1 ).base_value() / buffs.blade_rush->buff_period.total_seconds() );
        }

        // Combined non-traditional or inconsistent regen sources
        if ( combined )
        {
          if ( specialization() == ROGUE_ASSASSINATION )
          {
            double poisoned_bleeds = 0;
            int lethal_poisons = 0;
            for ( auto p : sim->target_non_sleeping_list )
            {
              rogue_td_t* tdata = get_target_data( p );
              if ( tdata->is_lethal_poisoned() )
              {
                lethal_poisons++;
                auto bleeds = { tdata->dots.garrote, tdata->dots.garrote_deathmark,
                                tdata->dots.rupture, tdata->dots.rupture_deathmark };
                for ( auto bleed : bleeds )
                {
                  if ( bleed && bleed->is_ticking() )
                  {
                    // Multiply Venomous Wounds contribution by the Exsanguinated rate multiplier
                    poisoned_bleeds += ( 1.0 * cast_attack( bleed->current_action )->cast_state( bleed->state )->get_exsanguinated_rate() );
                  }
                }
              }
            }

            // Venomous Wounds
            const double dot_tick_rate = 2.0 * composite_spell_haste();
            energy_regen_per_second += ( poisoned_bleeds * talent.assassination.venomous_wounds->effectN( 2 ).base_value() ) / dot_tick_rate;

            // Dashing Scoundrel -- Estimate ~90% Envenom uptime for energy regen approximation
            if ( spec.dashing_scoundrel_gain > 0.0 )
            {
              energy_regen_per_second += ( 0.9 * lethal_poisons * active.lethal_poison->composite_crit_chance() * spec.dashing_scoundrel_gain ) / dot_tick_rate;
            }
          }
        }

        // TODO - Add support for Master of Shadows, and potentially also estimated Combat Potency, ShT etc.
        //        Also consider if buffs such as Adrenaline Rush should be prorated based on duration for time_to_max

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

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;
  base.spell_power_per_intellect = 1.0;

  player_t::init_base_stats();

  base_gcd = timespan_t::from_seconds( 1.0 );
  min_gcd  = timespan_t::from_seconds( 1.0 );

  if ( options.rogue_ready_trigger )
  {
    ready_type = READY_TRIGGER;
    rogue_ready_trigger_threshold = ( specialization() == ROGUE_OUTLAW ) ? 20 : 25;
  }
}

// rogue_t::init_spells =====================================================

void rogue_t::init_spells()
{
  player_t::init_spells();

  // Core Class Spells
  spell.ambush = find_class_spell( "Ambush" );
  spell.cheap_shot = find_class_spell( "Cheap Shot" );
  spell.crimson_vial = find_class_spell( "Crimson Vial" );
  spell.crippling_poison = find_class_spell( "Crippling Poison" );
  spell.detection = find_spell( 56814 ); // find_class_spell( "Detection" );
  spell.distract = find_class_spell( "Distract" );
  spell.instant_poison = find_class_spell( "Instant Poison" );
  spell.kick = find_class_spell( "Kick" );
  spell.kidney_shot = find_class_spell( "Kidney Shot" );
  spell.slice_and_dice = find_class_spell( "Slice and Dice" );
  spell.sprint = find_class_spell( "Sprint" );
  spell.stealth = find_class_spell( "Stealth" );
  spell.vanish = find_class_spell( "Vanish" );
  spell.wound_poison = find_class_spell( "Wound Poison" );
  spell.feint = find_class_spell( "Feint" );
  spell.sap = find_class_spell( "Sap" );

  // Class Passives
  spell.all_rogue = find_spell( 137034 );
  spell.critical_strikes = find_spell( 157442 );
  spell.cut_to_the_chase = find_specialization_spell( "Cut to the Chase");
  spell.fleet_footed = find_class_spell( "Fleet Footed" );
  spell.leather_specialization = find_spell( 86092 );

  // Assassination Spells
  spec.assassination_rogue = find_specialization_spell( "Assassination Rogue" );
  spec.envenom = find_specialization_spell( "Envenom" );
  spec.fan_of_knives = find_specialization_spell( "Fan of Knives" );
  spec.garrote = find_specialization_spell( "Garrote" );
  spec.mutilate = find_specialization_spell( "Mutilate" );
  spec.poisoned_knife = find_specialization_spell( "Poisoned Knife" );
  spec.rupture = find_specialization_spell( "Rupture" );

  // Outlaw Spells
  spec.outlaw_rogue = find_specialization_spell( "Outlaw Rogue" );
  spec.between_the_eyes = find_specialization_spell( "Between the Eyes" );
  spec.dispatch = find_specialization_spell( "Dispatch" );
  spec.pistol_shot = find_specialization_spell( "Pistol Shot" );
  spec.sinister_strike = find_specialization_spell( "Sinister Strike" );
  spec.roll_the_bones = find_specialization_spell("Roll the Bones");
  spec.restless_blades = find_specialization_spell( "Restless Blades" );
  spec.blade_flurry = find_specialization_spell( "Blade Flurry" );

  // Subtlety Spells
  spec.subtlety_rogue = find_specialization_spell( "Subtlety Rogue" );
  spec.backstab = find_specialization_spell( "Backstab" );
  spec.black_powder = find_specialization_spell( "Black Powder" );
  spec.eviscerate = find_class_spell( "Eviscerate" ); // Baseline spell for early leveling
  spec.secret_technique = find_specialization_spell( "Secret Technique" );
  spec.shadow_dance = find_specialization_spell( "Shadow Dance" );
  spec.shadow_techniques = find_specialization_spell( "Shadow Techniques" );
  spec.shadowstrike = find_specialization_spell( "Shadowstrike" );
  spec.shuriken_toss = find_specialization_spell( "Shuriken Toss" );
  spec.shuriken_storm = find_specialization_spell( "Shuriken Storm" );

  // Multi-Spec
  spec.shadowstep = find_specialization_spell( "Shadowstep" ); // Assassination + Subtlety

  // Masteries
  mastery.potent_assassin = find_mastery_spell( ROGUE_ASSASSINATION );
  mastery.main_gauche = find_mastery_spell( ROGUE_OUTLAW );
  mastery.main_gauche_attack = mastery.main_gauche->ok() ? find_spell( 86392 ) : spell_data_t::not_found();
  mastery.executioner = find_mastery_spell( ROGUE_SUBTLETY );

  // Class Talents
  talent.rogue.shiv = find_talent_spell( talent_tree::CLASS, "Shiv" );
  talent.rogue.blind = find_talent_spell( talent_tree::CLASS, "Blind" );
  talent.rogue.cloak_of_shadows = find_talent_spell( talent_tree::CLASS, "Cloak of Shadows" );

  talent.rogue.evasion = find_talent_spell( talent_tree::CLASS, "Evasion" );
  talent.rogue.gouge = find_talent_spell( talent_tree::CLASS, "Gouge" );
  talent.rogue.airborne_irritant = find_talent_spell( talent_tree::CLASS, "Airborne Irritant" );
  talent.rogue.thrill_seeking = find_talent_spell( talent_tree::CLASS, "Thrill Seeking" );

  talent.rogue.master_poisoner = find_talent_spell( talent_tree::CLASS, "Master Poisoner" );
  talent.rogue.cheat_death = find_talent_spell( talent_tree::CLASS, "Cheat Death" );
  talent.rogue.elusiveness = find_talent_spell( talent_tree::CLASS, "Elusiveness" );
  talent.rogue.tricks_of_the_trade = find_talent_spell( talent_tree::CLASS, "Tricks of the Trade" );
  talent.rogue.blackjack = find_talent_spell( talent_tree::CLASS, "Blackjack" );

  talent.rogue.improved_wound_poison = find_talent_spell( talent_tree::CLASS, "Improved Wound Poison" );
  talent.rogue.nimble_fingers = find_talent_spell( talent_tree::CLASS, "Nimble Fingers" );
  talent.rogue.improved_sprint = find_talent_spell( talent_tree::CLASS, "Improved Sprint" );
  talent.rogue.shadowrunner = find_talent_spell( talent_tree::CLASS, "Shadowrunner" );

  talent.rogue.superior_mixture = find_talent_spell( talent_tree::CLASS, "Superior Mixture" );
  talent.rogue.fleet_footed = find_talent_spell( talent_tree::CLASS, "Fleet Footed" );
  talent.rogue.iron_stomach = find_talent_spell( talent_tree::CLASS, "Iron Stomach" );
  talent.rogue.unbreakable_stride = find_talent_spell( talent_tree::CLASS, "Unbreakable Stride" );
  talent.rogue.featherfoot = find_talent_spell( talent_tree::CLASS, "Featherfoot" );
  
  talent.rogue.numbing_poison = find_talent_spell( talent_tree::CLASS, "Numbing Poison" );
  talent.rogue.atrophic_poison = find_talent_spell( talent_tree::CLASS, "Atrophic Poison" );
  talent.rogue.deadened_nerves = find_talent_spell( talent_tree::CLASS, "Deadened Nerves" );
  talent.rogue.graceful_guile = find_talent_spell( talent_tree::CLASS, "Graceful Guile" );
  talent.rogue.stillshroud = find_talent_spell( talent_tree::CLASS, "Stillshroud" );

  talent.rogue.deadly_precision = find_talent_spell( talent_tree::CLASS, "Deadly Precision" );
  talent.rogue.virulent_poisons = find_talent_spell( talent_tree::CLASS, "Virulent Poisons" );
  talent.rogue.improved_ambush = find_talent_spell( talent_tree::CLASS, "Improved Ambush" );
  talent.rogue.tight_spender = find_talent_spell( talent_tree::CLASS, "Tight Spender" );

  talent.rogue.leeching_poison = find_talent_spell( talent_tree::CLASS, "Leeching Poison" );
  talent.rogue.lethality = find_talent_spell( talent_tree::CLASS, "Lethality" );
  talent.rogue.recuperator = find_talent_spell( talent_tree::CLASS, "Recuperator" );
  talent.rogue.alacrity = find_talent_spell( talent_tree::CLASS, "Alacrity" );
  talent.rogue.soothing_darkness = find_talent_spell( talent_tree::CLASS, "Soothing Darkness" );

  talent.rogue.swift_slasher = find_talent_spell( talent_tree::CLASS, "Swift Slasher" );

  talent.rogue.vigor = find_talent_spell( talent_tree::CLASS, "Vigor" );
  talent.rogue.supercharger = find_talent_spell( talent_tree::CLASS, "Supercharger" );
  talent.rogue.subterfuge = find_talent_spell( talent_tree::CLASS, "Subterfuge" );

  talent.rogue.thistle_tea = find_talent_spell( talent_tree::CLASS, "Thistle Tea" );
  talent.rogue.echoing_reprimand = find_talent_spell( talent_tree::CLASS, "Echoing Reprimand" );
  talent.rogue.forced_induction = find_talent_spell( talent_tree::CLASS, "Forced Induction" );
  talent.rogue.deeper_stratagem = find_talent_spell( talent_tree::CLASS, "Deeper Stratagem" );
  talent.rogue.without_a_trace = find_talent_spell( talent_tree::CLASS, "Without a Trace" );

  // Assassination Talents
  talent.assassination.deadly_poison = find_talent_spell( talent_tree::SPECIALIZATION, "Deadly Poison" );

  talent.assassination.venomous_wounds = find_talent_spell( talent_tree::SPECIALIZATION, "Venomous Wounds" );
  talent.assassination.path_of_blood = find_talent_spell( talent_tree::SPECIALIZATION, "Path of Blood" );

  talent.assassination.rapid_injection = find_talent_spell( talent_tree::SPECIALIZATION, "Rapid Injection" );
  talent.assassination.improved_poisons = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Poisons" );
  talent.assassination.bloody_mess = find_talent_spell( talent_tree::SPECIALIZATION, "Bloody Mess" );

  talent.assassination.thrown_precision = find_talent_spell( talent_tree::SPECIALIZATION, "Thrown Precision" );
  talent.assassination.seal_fate = find_talent_spell( talent_tree::SPECIALIZATION, "Seal Fate" );
  talent.assassination.caustic_spatter = find_talent_spell( talent_tree::SPECIALIZATION, "Caustic Spatter" );
  talent.assassination.internal_bleeding = find_talent_spell( talent_tree::SPECIALIZATION, "Internal Bleeding" );
  talent.assassination.improved_garrote = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Garrote" );

  talent.assassination.crimson_tempest = find_talent_spell( talent_tree::SPECIALIZATION, "Crimson Tempest" );
  talent.assassination.deathmark = find_talent_spell( talent_tree::SPECIALIZATION, "Deathmark" );
  
  talent.assassination.flying_daggers = find_talent_spell( talent_tree::SPECIALIZATION, "Flying Daggers" );
  talent.assassination.sanguine_stratagem = find_talent_spell( talent_tree::SPECIALIZATION, "Sanguine Stratagem" );
  talent.assassination.fatal_concoction = find_talent_spell( talent_tree::SPECIALIZATION, "Fatal Concoction" );
  talent.assassination.lethal_dose = find_talent_spell( talent_tree::SPECIALIZATION, "Lethal Dose" );
  talent.assassination.intent_to_kill = find_talent_spell( talent_tree::SPECIALIZATION, "Intent to Kill" );
  talent.assassination.iron_wire = find_talent_spell( talent_tree::SPECIALIZATION, "Iron Wire" );

  talent.assassination.systemic_failure = find_talent_spell( talent_tree::SPECIALIZATION, "Systemic Failure" );
  talent.assassination.amplifying_poison = find_talent_spell( talent_tree::SPECIALIZATION, "Amplifying Poison" );
  talent.assassination.doomblade = find_talent_spell( talent_tree::SPECIALIZATION, "Doomblade" );

  talent.assassination.blindside = find_talent_spell( talent_tree::SPECIALIZATION, "Blindside" );
  talent.assassination.dashing_scoundrel = find_talent_spell( talent_tree::SPECIALIZATION, "Dashing Scoundrel" );
  talent.assassination.shrouded_suffocation = find_talent_spell( talent_tree::SPECIALIZATION, "Shrouded Suffocation" );

  talent.assassination.zoldyck_recipe = find_talent_spell( talent_tree::SPECIALIZATION, "Zoldyck Recipe" );
  talent.assassination.poison_bomb = find_talent_spell( talent_tree::SPECIALIZATION, "Poison Bomb" );
  talent.assassination.scent_of_blood = find_talent_spell( talent_tree::SPECIALIZATION, "Scent of Blood" );

  talent.assassination.kingsbane = find_talent_spell( talent_tree::SPECIALIZATION, "Kingsbane" );
  talent.assassination.dragon_tempered_blades = find_talent_spell( talent_tree::SPECIALIZATION, "Dragon-Tempered Blades" );
  talent.assassination.sudden_demise = find_talent_spell( talent_tree::SPECIALIZATION, "Sudden Demise" );

  // Outlaw Talents
  talent.outlaw.opportunity = find_talent_spell( talent_tree::SPECIALIZATION, "Opportunity" );

  talent.outlaw.adrenaline_rush = find_talent_spell( talent_tree::SPECIALIZATION, "Adrenaline Rush" );

  talent.outlaw.retractable_hook = find_talent_spell( talent_tree::SPECIALIZATION, "Retractable Hook" );
  talent.outlaw.combat_potency = find_talent_spell( talent_tree::SPECIALIZATION, "Combat Potency" );
  talent.outlaw.combat_stamina = find_talent_spell( talent_tree::SPECIALIZATION, "Combat Stamina" );
  talent.outlaw.hit_and_run = find_talent_spell( talent_tree::SPECIALIZATION, "Hit and Run" );

  talent.outlaw.blinding_powder = find_talent_spell( talent_tree::SPECIALIZATION, "Blinding Powder" );
  talent.outlaw.precision_shot = find_talent_spell( talent_tree::SPECIALIZATION, "Precision Shot" );

  talent.outlaw.heavy_hitter = find_talent_spell( talent_tree::SPECIALIZATION, "Heavy Hitter" );
  talent.outlaw.devious_stratagem = find_talent_spell( talent_tree::SPECIALIZATION, "Devious Stratagem" );
  talent.outlaw.killing_spree = find_talent_spell( talent_tree::SPECIALIZATION, "Killing Spree" );
  talent.outlaw.fatal_flourish = find_talent_spell( talent_tree::SPECIALIZATION, "Fatal Flourish" );
  talent.outlaw.quick_draw = find_talent_spell( talent_tree::SPECIALIZATION, "Quick Draw" );
  talent.outlaw.deft_maneuvers = find_talent_spell( talent_tree::SPECIALIZATION, "Deft Maneuvers" );

  talent.outlaw.acrobatic_strikes = find_talent_spell( talent_tree::SPECIALIZATION, "Acrobatic Strikes" );

  talent.outlaw.ruthlessness = find_talent_spell( talent_tree::SPECIALIZATION, "Ruthlessness" );
  talent.outlaw.loaded_dice = find_talent_spell( talent_tree::SPECIALIZATION, "Loaded Dice" );
  talent.outlaw.sleight_of_hand = find_talent_spell( talent_tree::SPECIALIZATION, "Sleight of Hand" );
  talent.outlaw.thiefs_versatility = find_talent_spell( talent_tree::SPECIALIZATION, "Thief's Versatility" );
  talent.outlaw.improved_between_the_eyes = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Between the Eyes" );

  talent.outlaw.audacity = find_talent_spell( talent_tree::SPECIALIZATION, "Audacity" );
  talent.outlaw.improved_adrenaline_rush = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Adrenaline Rush" );
  talent.outlaw.dancing_steel = find_talent_spell( talent_tree::SPECIALIZATION, "Dancing Steel" );

  talent.outlaw.ace_up_your_sleeve = find_talent_spell( talent_tree::SPECIALIZATION, "Ace Up Your Sleeve" );
  talent.outlaw.blade_rush = find_talent_spell( talent_tree::SPECIALIZATION, "Blade Rush" );

  talent.outlaw.summarily_dispatched = find_talent_spell( talent_tree::SPECIALIZATION, "Summarily Dispatched" );
  talent.outlaw.fan_the_hammer = find_talent_spell( talent_tree::SPECIALIZATION, "Fan the Hammer" );

  talent.outlaw.hidden_opportunity = find_talent_spell( talent_tree::SPECIALIZATION, "Hidden Opportunity" );
  talent.outlaw.keep_it_rolling = find_talent_spell( talent_tree::SPECIALIZATION, "Keep it Rolling" );

  // Subtlety Talents
  talent.subtlety.find_weakness = find_talent_spell( talent_tree::SPECIALIZATION, "Find Weakness" );

  talent.subtlety.improved_backstab = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Backstab" );
  talent.subtlety.shadow_blades = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow Blades" );
  talent.subtlety.improved_shuriken_storm = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Shuriken Storm" );

  talent.subtlety.shot_in_the_dark = find_talent_spell( talent_tree::SPECIALIZATION, "Shot in the Dark" );
  talent.subtlety.quick_decisions = find_talent_spell( talent_tree::SPECIALIZATION, "Quick Decisions" );
  talent.subtlety.ephemeral_bond = find_talent_spell( talent_tree::SPECIALIZATION, "Ephemeral Bonds" );
  talent.subtlety.exhilarating_execution = find_talent_spell( talent_tree::SPECIALIZATION, "Exhilarating Execution" );

  talent.subtlety.shrouded_in_darkness = find_talent_spell( talent_tree::SPECIALIZATION, "Shrouded in Darkness" );
  talent.subtlety.shadow_focus = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow Focus" );
  talent.subtlety.fade_to_nothing = find_talent_spell( talent_tree::SPECIALIZATION, "Fade to Nothing" );
  talent.subtlety.cloaked_in_shadow = find_talent_spell( talent_tree::SPECIALIZATION, "Cloaked in Shadow" );
  talent.subtlety.night_terrors = find_talent_spell( talent_tree::SPECIALIZATION, "Night Terrors" );
  talent.subtlety.terrifying_pace = find_talent_spell( talent_tree::SPECIALIZATION, "Terrifying Pace" );

  talent.subtlety.gloomblade = find_talent_spell( talent_tree::SPECIALIZATION, "Gloomblade" );
  talent.subtlety.relentless_strikes = find_talent_spell( talent_tree::SPECIALIZATION, "Relentless Strikes" );
  talent.subtlety.silent_storm = find_talent_spell( talent_tree::SPECIALIZATION, "Silent Storm" );

  talent.subtlety.premeditation = find_talent_spell( talent_tree::SPECIALIZATION, "Premeditation" );
  talent.subtlety.planned_execution = find_talent_spell( talent_tree::SPECIALIZATION, "Planned Execution" );
  talent.subtlety.warning_signs = find_talent_spell( talent_tree::SPECIALIZATION, "Warning Signs" );
  talent.subtlety.double_dance = find_talent_spell( talent_tree::SPECIALIZATION, "Double Dance" );
  talent.subtlety.shadowed_finishers = find_talent_spell( talent_tree::SPECIALIZATION, "Shadowed Finishers" );
  talent.subtlety.secret_stratagem = find_talent_spell( talent_tree::SPECIALIZATION, "Secret Stratagem" );
  talent.subtlety.replicating_shadows = find_talent_spell( talent_tree::SPECIALIZATION, "Replicating Shadows" );

  talent.subtlety.weaponmaster = find_talent_spell( talent_tree::SPECIALIZATION, "Weaponmaster", ROGUE_SUBTLETY );
  talent.subtlety.the_first_dance = find_talent_spell( talent_tree::SPECIALIZATION, "The First Dance" );
  talent.subtlety.master_of_shadows = find_talent_spell( talent_tree::SPECIALIZATION, "Master of Shadows" );
  talent.subtlety.deepening_shadows = find_talent_spell( talent_tree::SPECIALIZATION, "Deepening Shadows" );
  talent.subtlety.veiltouched = find_talent_spell( talent_tree::SPECIALIZATION, "Veiltouched" );
  talent.subtlety.shuriken_tornado = find_talent_spell( talent_tree::SPECIALIZATION, "Shuriken Tornado" );

  talent.subtlety.perforated_veins = find_talent_spell( talent_tree::SPECIALIZATION, "Perforated Veins" );
  talent.subtlety.lingering_shadow = find_talent_spell( talent_tree::SPECIALIZATION, "Lingering Shadow" );
  talent.subtlety.deeper_daggers = find_talent_spell( talent_tree::SPECIALIZATION, "Deeper Daggers" );

  talent.subtlety.death_perception = find_talent_spell( talent_tree::SPECIALIZATION, "Death Perception" );
  talent.subtlety.dark_shadow = find_talent_spell( talent_tree::SPECIALIZATION, "Dark Shadow" );
  talent.subtlety.finality = find_talent_spell( talent_tree::SPECIALIZATION, "Finality" );

  talent.subtlety.the_rotten = find_talent_spell( talent_tree::SPECIALIZATION, "The Rotten" );
  talent.subtlety.shadowcraft = find_talent_spell( talent_tree::SPECIALIZATION, "Shadowcraft" );
  talent.subtlety.danse_macabre = find_talent_spell( talent_tree::SPECIALIZATION, "Danse Macabre" );
  talent.subtlety.goremaws_bite = find_talent_spell( talent_tree::SPECIALIZATION, "Goremaw's Bite" );
  talent.subtlety.dark_brew = find_talent_spell( talent_tree::SPECIALIZATION, "Dark Brew" );

  // Shared Talents
  spell.shadowstep = find_spell( 36554 );     // Base spell with 0 charges

  /* None currently, but leaving this just in case...
  auto find_shared_talent = []( std::vector<player_talent_t*> talents ) {
    for ( const auto t : talents )
    {
      if ( t->ok() )
      {
        return *t;
      }
    }
    return *talents[ 0 ];
  };
  */

  // Deathstalker Talents
  talent.deathstalker.deathstalkers_mark = find_talent_spell( talent_tree::HERO, "Deathstalker's Mark" );
  
  talent.deathstalker.clear_the_witnesses = find_talent_spell( talent_tree::HERO, "Clear the Witnesses" );
  talent.deathstalker.hunt_them_down = find_talent_spell( talent_tree::HERO, "Hunt Them Down" );
  talent.deathstalker.singular_focus = find_talent_spell( talent_tree::HERO, "Singular Focus" );

  talent.deathstalker.fatal_intent = find_talent_spell( talent_tree::HERO, "Fatal Intent" );
  talent.deathstalker.corrupt_the_blood = find_talent_spell( talent_tree::HERO, "Corrupt the Blood" );
  talent.deathstalker.lingering_darkness = find_talent_spell( talent_tree::HERO, "Lingering Darkness" );
  talent.deathstalker.symbolic_victory = find_talent_spell( talent_tree::HERO, "Symbolic Victory" );

  talent.deathstalker.ethereal_cloak = find_talent_spell( talent_tree::HERO, "Ethereal Cloak" );
  talent.deathstalker.bait_and_switch = find_talent_spell( talent_tree::HERO, "Bait and Switch" );
  talent.deathstalker.momentum_of_despair = find_talent_spell( talent_tree::HERO, "Momentum of Despair" );
  talent.deathstalker.follow_the_blood = find_talent_spell( talent_tree::HERO, "Follow the Blood" );
  talent.deathstalker.shadewalker = find_talent_spell( talent_tree::HERO, "Shadewalker" );
  talent.deathstalker.shroud_of_night = find_talent_spell( talent_tree::HERO, "Shroud of Night" );

  talent.deathstalker.darkest_night = find_talent_spell( talent_tree::HERO, "Darkest Night" );

  // Fatebound Talents
  talent.fatebound.hand_of_fate = find_talent_spell( talent_tree::HERO, "Hand of Fate" );

  talent.fatebound.chosens_revelry = find_talent_spell( talent_tree::HERO, "Chosen's Revelry" );
  talent.fatebound.tempted_fate = find_talent_spell( talent_tree::HERO, "Tempted Fate" );
  talent.fatebound.mean_streak = find_talent_spell( talent_tree::HERO, "Mean Streak" );
  talent.fatebound.inexorable_march = find_talent_spell( talent_tree::HERO, "Inexorable March" );
  talent.fatebound.deaths_arrival = find_talent_spell( talent_tree::HERO, "Death's Arrival [NYI]" );

  talent.fatebound.deal_fate = find_talent_spell( talent_tree::HERO, "Deal Fate" );
  talent.fatebound.edge_case = find_talent_spell( talent_tree::HERO, "Edge Case" );
  talent.fatebound.fate_intertwined = find_talent_spell( talent_tree::HERO, "Fate Intertwined" );

  talent.fatebound.delivered_doom = find_talent_spell( talent_tree::HERO, "Delivered Doom" );
  talent.fatebound.destiny_defined = find_talent_spell( talent_tree::HERO, "Destiny Defined" );

  // Trickster Talents
  talent.trickster.unseen_blade = find_talent_spell( talent_tree::HERO, "Unseen Blade" );

  talent.trickster.surprising_strikes = find_talent_spell( talent_tree::HERO, "Surprising Strikes" );
  talent.trickster.smoke = find_talent_spell( talent_tree::HERO, "Smoke" );
  talent.trickster.mirrors = find_talent_spell( talent_tree::HERO, "Mirrors" );
  talent.trickster.flawless_form = find_talent_spell( talent_tree::HERO, "Flawless Form" );

  talent.trickster.so_tricky = find_talent_spell( talent_tree::HERO, "So Tricky" );
  talent.trickster.dont_be_suspicious = find_talent_spell( talent_tree::HERO, "Don't Be Suspicious" );
  talent.trickster.devious_distractions = find_talent_spell( talent_tree::HERO, "Devious Distractions" );
  talent.trickster.thousand_cuts = find_talent_spell( talent_tree::HERO, "Thousand Cuts" );
  talent.trickster.flickerstrike = find_talent_spell( talent_tree::HERO, "Flickerstrike" );

  talent.trickster.disorienting_strikes = find_talent_spell( talent_tree::HERO, "Disorienting Strikes" );
  talent.trickster.cloud_cover = find_talent_spell( talent_tree::HERO, "Cloud Cover" );
  talent.trickster.no_scruples = find_talent_spell( talent_tree::HERO, "No Scruples" );
  talent.trickster.nimble_flurry = find_talent_spell( talent_tree::HERO, "Nimble Flurry" );

  talent.trickster.coup_de_grace = find_talent_spell( talent_tree::HERO, "Coup de Grace" );

  // Class Background Spells
  spell.echoing_reprimand_buff = talent.rogue.echoing_reprimand->ok() ? find_spell( 470671 ) : spell_data_t::not_found();
  spell.echoing_reprimand_damage = talent.rogue.echoing_reprimand->ok() ? find_spell( 470672 ) : spell_data_t::not_found();
  spell.leeching_poison_buff = talent.rogue.leeching_poison->ok() ? find_spell( 108211 ) : spell_data_t::not_found();
  spell.recuperator_heal = talent.rogue.recuperator->ok() ? find_spell( 426605 ) : spell_data_t::not_found();
  spell.shadowstep_buff = spec.shadowstep->ok() ? find_spell( 36554 ) : spell_data_t::not_found();
  spell.subterfuge_buff = talent.rogue.subterfuge->ok() ? find_spell( 115192 ) : spell_data_t::not_found();
  spell.thistle_tea = talent.rogue.thistle_tea->ok() ? talent.rogue.thistle_tea->effectN( 1 ).trigger() : spell_data_t::not_found();
  spell.thistle_tea_buff = talent.rogue.thistle_tea->ok() ? find_spell( 381623 ) : spell_data_t::not_found();
  spell.vanish_buff = spell.vanish->ok() ? find_spell( 11327 ) : spell_data_t::not_found();

  // Hero Talent Background Spells
  // Deathstalker
  spell.clear_the_witnesses_buff = talent.deathstalker.clear_the_witnesses->ok() ? find_spell( 457178 ) : spell_data_t::not_found();
  spell.clear_the_witnesses_damage = talent.deathstalker.clear_the_witnesses->ok() ? find_spell( 457179 ) : spell_data_t::not_found();
  spell.corrupt_the_blood_damage = talent.deathstalker.corrupt_the_blood->ok() ? find_spell( 457133 ) : spell_data_t::not_found();
  spell.darkest_night_buff = talent.deathstalker.darkest_night->ok() ? find_spell( 457280 ) : spell_data_t::not_found();
  spell.deathstalkers_mark_buff = talent.deathstalker.deathstalkers_mark->ok() ? find_spell( 457160 ) : spell_data_t::not_found();
  spell.deathstalkers_mark_damage = talent.deathstalker.deathstalkers_mark->ok() ? find_spell( 457157 ) : spell_data_t::not_found();
  spell.deathstalkers_mark_debuff = talent.deathstalker.deathstalkers_mark->ok() ? find_spell( 457129 ) : spell_data_t::not_found();
  spell.fatal_intent_damage = talent.deathstalker.fatal_intent->ok() ? find_spell( 461984 ) : spell_data_t::not_found();
  spell.fatal_intent_debuff = talent.deathstalker.fatal_intent->ok() ? find_spell( 461981 ) : spell_data_t::not_found();
  spell.hunt_them_down_damage = talent.deathstalker.hunt_them_down->ok() ? find_spell( 457193 ) : spell_data_t::not_found();
  spell.lingering_darkness_buff = talent.deathstalker.lingering_darkness->ok() ? find_spell( 457273 ) : spell_data_t::not_found();
  spell.momentum_of_despair_buff = talent.deathstalker.momentum_of_despair->effectN( 1 ).trigger();
  spell.singular_focus_damage = talent.deathstalker.singular_focus->ok() ? find_spell( 457236 ) : spell_data_t::not_found();
  spell.symbolic_victory_buff = talent.deathstalker.symbolic_victory->ok() ? find_spell( 457167 ) : spell_data_t::not_found();
  
  // Fatebound
  spell.fatebound_coin_heads_buff = talent.fatebound.hand_of_fate->ok() ? find_spell( 452923 ) : spell_data_t::not_found();
  spell.fatebound_coin_heads_initial_buff = talent.fatebound.hand_of_fate->ok() ? find_spell( 456479 ) : spell_data_t::not_found();
  spell.fatebound_coin_tails_buff = talent.fatebound.hand_of_fate->ok() ? find_spell( 452917 ) : spell_data_t::not_found();
  spell.fatebound_coin_tails = talent.fatebound.hand_of_fate->ok() ? find_spell( 452538 ) : spell_data_t::not_found();
  spell.fatebound_lucky_coin_buff = spell_data_t::not_found(); // TODO Lucky Coin
  spell.fatebound_lucky_coin_damage = spell_data_t::not_found(); // TODO Lucky Coin
  spell.fatebound_fate_intertwined = talent.fatebound.fate_intertwined->ok() ? find_spell( 456306 ) : spell_data_t::not_found();

  // Trickster
  spell.cloud_cover_distract = talent.trickster.cloud_cover->ok() ? find_spell( as<unsigned>( talent.trickster.cloud_cover->effectN( 1 ).base_value() ) ) : spell_data_t::not_found();
  spell.coup_de_grace = talent.trickster.coup_de_grace->ok() ? find_spell( 441776 ) : spell_data_t::not_found();
  spell.coup_de_grace_damage_1 = talent.trickster.coup_de_grace->ok() ? ( specialization() == ROGUE_SUBTLETY ? find_spell( 462241 ) : find_spell( 462140 ) ) : spell_data_t::not_found();
  spell.coup_de_grace_damage_2 = talent.trickster.coup_de_grace->ok() ? ( specialization() == ROGUE_SUBTLETY ? find_spell( 462242 ) : find_spell( 462239 ) ) : spell_data_t::not_found();
  spell.coup_de_grace_damage_3 = talent.trickster.coup_de_grace->ok() ? ( specialization() == ROGUE_SUBTLETY ? find_spell( 462243 ) : find_spell( 462240 ) ) : spell_data_t::not_found();
  spell.coup_de_grace_damage_bonus_1 = talent.trickster.coup_de_grace->ok() && talent.subtlety.shadowed_finishers->ok() ? find_spell( 462244 ) : spell_data_t::not_found();
  spell.coup_de_grace_damage_bonus_2 = talent.trickster.coup_de_grace->ok() && talent.subtlety.shadowed_finishers->ok() ? find_spell( 462247 ) : spell_data_t::not_found();
  spell.coup_de_grace_damage_bonus_3 = talent.trickster.coup_de_grace->ok() && talent.subtlety.shadowed_finishers->ok() ? find_spell( 462248 ) : spell_data_t::not_found();
  spell.escalating_blade_buff = talent.trickster.coup_de_grace->ok() ? find_spell( 441786 ) : spell_data_t::not_found();
  spell.unseen_blade = talent.trickster.unseen_blade->ok() ? find_spell( 441144 ) : spell_data_t::not_found();
  spell.unseen_blade_buff = talent.trickster.unseen_blade->ok() ? find_spell( 459485 ) : spell_data_t::not_found();
  spell.fazed_debuff = talent.trickster.unseen_blade->ok() ? find_spell( 441224 ) : spell_data_t::not_found();
  spell.flawless_form_buff = ( talent.trickster.flawless_form->ok() || talent.trickster.coup_de_grace->ok() ) ? find_spell( 441326 ) : spell_data_t::not_found();
  spell.nimble_flurry_damage = talent.trickster.nimble_flurry->ok() ? find_spell( 459497 ) : spell_data_t::not_found();

  // Spec Background Spells
  // Assassination
  spec.amplifying_poison_debuff = talent.assassination.amplifying_poison->effectN( 3 ).trigger();
  spec.blindside_buff = talent.assassination.blindside->ok() ? find_spell( 121153 ) : spell_data_t::not_found();
  spec.caustic_spatter_buff = talent.assassination.caustic_spatter->ok() ? find_spell( 421976 ) : spell_data_t::not_found();
  spec.caustic_spatter_damage = talent.assassination.caustic_spatter->ok() ? find_spell( 421979 ) : spell_data_t::not_found();
  spec.dashing_scoundrel_gain = talent.assassination.dashing_scoundrel->ok() ? talent.assassination.dashing_scoundrel->effectN( 2 ).resource( RESOURCE_ENERGY ) : 0.0;
  spec.deadly_poison_instant = talent.assassination.deadly_poison->ok() ? find_spell( 113780 ) : spell_data_t::not_found();
  spec.doomblade_debuff = talent.assassination.doomblade->ok() ? find_spell( 394021 ) : spell_data_t::not_found();
  spec.improved_garrote_buff = talent.assassination.improved_garrote->ok() ? find_spell( 392401 ) : spell_data_t::not_found();
  spec.internal_bleeding_debuff = talent.assassination.internal_bleeding->ok() ? find_spell( 381628 ) : spell_data_t::not_found();
  spec.kingsbane_buff = talent.assassination.kingsbane->ok() ? find_spell( 394095 ) : spell_data_t::not_found();
  spec.poison_bomb_driver = talent.assassination.poison_bomb->ok() ? find_spell( 255545 ) : spell_data_t::not_found();
  spec.poison_bomb_damage = talent.assassination.poison_bomb->ok() ? find_spell( 255546 ) : spell_data_t::not_found();
  spec.scent_of_blood_buff = talent.assassination.scent_of_blood->ok() ? find_spell( 394080 ) : spell_data_t::not_found();
  spec.zoldyck_insignia = talent.assassination.zoldyck_recipe->ok() ? talent.assassination.zoldyck_recipe : spell_data_t::not_found();

  spec.deathmark_debuff = talent.assassination.deathmark->effectN( 2 ).trigger();
  spec.deathmark_amplifying_poison = talent.assassination.deathmark->ok() ? find_spell( 394328 ) : spell_data_t::not_found();
  spec.deathmark_deadly_poison_dot = talent.assassination.deathmark->ok() ? find_spell( 394324 ) : spell_data_t::not_found();
  spec.deathmark_deadly_poison_instant = talent.assassination.deathmark->ok() ? find_spell( 394325 ) : spell_data_t::not_found();
  spec.deathmark_garrote = talent.assassination.deathmark->ok() ? find_spell( 360830 ) : spell_data_t::not_found();
  spec.deathmark_instant_poison = talent.assassination.deathmark->ok() ? find_spell( 394326 ) : spell_data_t::not_found();
  spec.deathmark_rupture = talent.assassination.deathmark->ok() ? find_spell( 360826 ) : spell_data_t::not_found();
  spec.deathmark_wound_poison = talent.assassination.deathmark->ok() ? find_spell( 394327 ) : spell_data_t::not_found();

  // Outlaw
  spec.acrobatic_strikes_buff = talent.outlaw.acrobatic_strikes->ok() ? find_spell( 455144 ) : spell_data_t::not_found();
  spec.audacity_buff = talent.outlaw.audacity->ok() ? find_spell( 386270 ) : spell_data_t::not_found();
  spec.blade_flurry_attack = spec.blade_flurry->ok() ? find_spell( 22482 ) : spell_data_t::not_found();
  spec.blade_flurry_instant_attack = spec.blade_flurry->ok() ? 
    ( talent.outlaw.deft_maneuvers->ok() ? find_spell( 429951 ) : find_spell( 331850 ) ) : spell_data_t::not_found();
  spec.blade_rush_attack = talent.outlaw.blade_rush->ok() ? find_spell( 271881 ) : spell_data_t::not_found();
  spec.blade_rush_energize = talent.outlaw.blade_rush->ok() ? find_spell( 271896 ) : spell_data_t::not_found();
  spec.hidden_opportunity_extra_attack = talent.outlaw.hidden_opportunity->ok() ? find_spell( 385897 ) : spell_data_t::not_found();
  spec.improved_adrenaline_rush_energize = talent.outlaw.improved_adrenaline_rush->ok() ? find_spell( 395424 ) : spell_data_t::not_found();
  // TOCHECK -- Killing Spree spell ids could change over 11.2 PTR, new spell 1248604 exists but not used in logs yet
  spec.killing_spree_mh_attack = talent.outlaw.killing_spree->ok() ? find_spell( 57841 ) : spell_data_t::not_found();
  spec.killing_spree_oh_attack = talent.outlaw.killing_spree->ok() ? find_spell( 57842 ) : spell_data_t::not_found();
  spec.killing_spree_energize = talent.outlaw.killing_spree->ok() ? find_spell( 1235074 ) : spell_data_t::not_found();
  spec.opportunity_buff = talent.outlaw.opportunity->ok() ? find_spell( 195627 ) : spell_data_t::not_found();
  spec.sinister_strike_extra_attack = talent.outlaw.opportunity->ok() ? find_spell( 197834 ) : spell_data_t::not_found();
  spec.summarily_dispatched_buff = talent.outlaw.summarily_dispatched->ok() ? find_spell( 386868 ) : spell_data_t::not_found();
  spec.ace_up_your_sleeve_energize = talent.outlaw.ace_up_your_sleeve->ok() ? find_spell( 394120 ) : spell_data_t::not_found();

  spec.one_of_a_kind = spec.roll_the_bones->ok() ? find_spell( 1214933 ) : spell_data_t::not_found();
  spec.double_trouble = spec.roll_the_bones->ok() ? find_spell( 1214934 ) : spell_data_t::not_found();
  spec.triple_threat = spec.roll_the_bones->ok() ? find_spell( 1214935 ) : spell_data_t::not_found();
  spec.jackpot = spec.roll_the_bones->ok() ? find_spell( 1214937 ) : spell_data_t::not_found();

  // Subtlety
  spec.black_powder_shadow_attack = talent.subtlety.shadowed_finishers->ok() ? find_spell( 319190 ) : spell_data_t::not_found();
  spec.danse_macabre_buff = talent.subtlety.danse_macabre->ok() ? find_spell( 393969 ) : spell_data_t::not_found();
  spec.eviscerate_shadow_attack = talent.subtlety.shadowed_finishers->ok() ? find_spell( 328082 ) : spell_data_t::not_found();
  spec.find_weakness_debuff = talent.subtlety.find_weakness->ok() ? find_spell( 316220 ) : spell_data_t::not_found();
  spec.goremaws_bite_buff = talent.subtlety.goremaws_bite->effectN( 2 ).trigger();
  spec.lingering_shadow_attack = talent.subtlety.lingering_shadow->ok() ? find_spell( 386081 ) : spell_data_t::not_found();
  spec.lingering_shadow_buff = talent.subtlety.lingering_shadow->ok() ? find_spell( 385960 ) : spell_data_t::not_found();
  spec.master_of_shadows_buff = talent.subtlety.master_of_shadows->ok() ? find_spell( 196980 ) : spell_data_t::not_found();
  spec.perforated_veins_buff = talent.subtlety.perforated_veins->ok() ? find_spell( 426602 ) : spell_data_t::not_found();
  spec.perforated_veins_counter = talent.subtlety.perforated_veins->effectN( 1 ).trigger();
  spec.premeditation_buff = talent.subtlety.premeditation->ok() ? find_spell( 343173 ) : spell_data_t::not_found();
  spec.relentless_strikes_energize = talent.subtlety.relentless_strikes->ok() ? find_spell( 98440 ) : spell_data_t::not_found();
  spec.replicating_shadows_tick = talent.subtlety.replicating_shadows->ok() ? find_spell( 394031 ) : spell_data_t::not_found();
  spec.secret_technique_attack = spec.secret_technique->ok() ? find_spell( 280720 ) : spell_data_t::not_found();
  spec.secret_technique_clone_attack = spec.secret_technique->ok() ? find_spell( 282449 ) : spell_data_t::not_found();
  spec.shadowstrike_stealth_buff = spec.shadowstrike->ok() ? find_spell( 245623 ) : spell_data_t::not_found();
  spec.shadow_blades_attack = talent.subtlety.shadow_blades->ok() ? find_spell( 279043 ) : spell_data_t::not_found();
  spec.shadow_focus_buff = talent.subtlety.shadow_focus->ok() ? find_spell( 112942 ) : spell_data_t::not_found();
  spec.shadow_techniques_energize = spec.shadow_techniques->ok() ? find_spell( 196911 ) : spell_data_t::not_found();
  spec.shot_in_the_dark_buff = talent.subtlety.shot_in_the_dark->ok() ? find_spell( 257506 ) : spell_data_t::not_found();
  spec.silent_storm_buff = talent.subtlety.silent_storm->ok() ? find_spell( 385727 ) : spell_data_t::not_found();
  spec.the_first_dance_buff = talent.subtlety.the_first_dance->ok() ? find_spell( 470678 ) : spell_data_t::not_found();

  // Set Bonus Items ========================================================

  set_bonuses.tww1_assassination_2pc  = sets->set( ROGUE_ASSASSINATION, TWW1, B2 );
  set_bonuses.tww1_assassination_4pc  = sets->set( ROGUE_ASSASSINATION, TWW1, B4 );
  set_bonuses.tww1_outlaw_2pc         = sets->set( ROGUE_OUTLAW, TWW1, B2 );
  set_bonuses.tww1_outlaw_4pc         = sets->set( ROGUE_OUTLAW, TWW1, B4 );
  set_bonuses.tww1_subtlety_2pc       = sets->set( ROGUE_SUBTLETY, TWW1, B2 );
  set_bonuses.tww1_subtlety_4pc       = sets->set( ROGUE_SUBTLETY, TWW1, B4 );

  spec.tww1_assassination_2pc_buff = set_bonuses.tww1_assassination_2pc->ok() ? find_spell( 458475 ) : spell_data_t::not_found();
  spec.tww1_assassination_4pc_buff = set_bonuses.tww1_assassination_4pc->ok() ? find_spell( 458476 ) : spell_data_t::not_found();
  spec.tww1_outlaw_2pc_spell = set_bonuses.tww1_outlaw_2pc->ok() ? find_spell( 459002 ) : spell_data_t::not_found();
  spec.tww1_outlaw_4pc_buff = set_bonuses.tww1_outlaw_4pc->ok() ? find_spell( 458826 ) : spell_data_t::not_found();

  set_bonuses.tww2_assassination_2pc  = sets->set( ROGUE_ASSASSINATION, TWW2, B2 );
  set_bonuses.tww2_assassination_4pc  = sets->set( ROGUE_ASSASSINATION, TWW2, B4 );
  set_bonuses.tww2_outlaw_2pc         = sets->set( ROGUE_OUTLAW, TWW2, B2 );
  set_bonuses.tww2_outlaw_4pc         = sets->set( ROGUE_OUTLAW, TWW2, B4 );
  set_bonuses.tww2_subtlety_2pc       = sets->set( ROGUE_SUBTLETY, TWW2, B2 );
  set_bonuses.tww2_subtlety_4pc       = sets->set( ROGUE_SUBTLETY, TWW2, B4 );

  spec.tww2_assassination_4pc_buff = set_bonuses.tww2_assassination_4pc->ok() ? find_spell( 1219264 ) : spell_data_t::not_found();

  set_bonuses.tww3_deathstalker_2pc = sets->set( HERO_DEATHSTALKER, TWW3, B2 );
  set_bonuses.tww3_deathstalker_4pc = sets->set( HERO_DEATHSTALKER, TWW3, B4 );
  set_bonuses.tww3_fatebound_2pc    = sets->set( HERO_FATEBOUND, TWW3, B2 );
  set_bonuses.tww3_fatebound_4pc    = sets->set( HERO_FATEBOUND, TWW3, B4 );
  set_bonuses.tww3_trickster_2pc    = sets->set( HERO_TRICKSTER, TWW3, B2 );
  set_bonuses.tww3_trickster_4pc    = sets->set( HERO_TRICKSTER, TWW3, B4 );

  spec.tww3_deathstalker_2pc_buff = set_bonuses.tww3_deathstalker_2pc->ok() ?
    ( specialization() == ROGUE_ASSASSINATION ? find_spell( 1239231 ) : find_spell( 1239232 ) ) : spell_data_t::not_found();

  // Register passives ======================================================

  // Extra CP from improved ambush is reported separatedly and manually handled within the action
  deregister_passive_spell( talent.rogue.improved_ambush );

  // Corrupt the blood duration extension handled in action as rupture spell data has per CP duration
  deregister_passive_spell( talent.deathstalker.corrupt_the_blood );

  // Veiltouched is scripted to apply to physical abilities with their school changed by Dark Brew
  if ( talent.subtlety.dark_brew.ok() && talent.subtlety.veiltouched.ok() )
  {
    auto list_dd = affect_list_t( 1 );
    auto list_td = affect_list_t( 2 );
    bool prop;  // dummy

    // grab all affected spells
    auto veil_spells_dd_1 = spells_affected_by_passive( talent.subtlety.veiltouched->effectN( 1 ), prop );
    auto veil_spells_dd_2 = spells_affected_by_passive( talent.subtlety.veiltouched->effectN( 3 ), prop );
    auto veil_spells_dd_3 = spells_affected_by_passive( talent.subtlety.veiltouched->effectN( 6 ), prop );
    auto veil_spells_td_1 = spells_affected_by_passive( talent.subtlety.veiltouched->effectN( 2 ), prop );
    auto veil_spells_td_2 = spells_affected_by_passive( talent.subtlety.veiltouched->effectN( 4 ), prop );
    auto veil_spells_td_3 = spells_affected_by_passive( talent.subtlety.veiltouched->effectN( 5 ), prop );

    // build combined list
    std::vector<const spell_data_t*> spells_dd, spells_td;
    range::copy( veil_spells_dd_1, std::back_inserter( spells_dd ) );
    range::copy( veil_spells_dd_2, std::back_inserter( spells_dd ) );
    range::copy( veil_spells_dd_3, std::back_inserter( spells_dd ) );
    range::copy( veil_spells_td_1, std::back_inserter( spells_td ) );
    range::copy( veil_spells_td_2, std::back_inserter( spells_td ) );
    range::copy( veil_spells_td_3, std::back_inserter( spells_td ) );

    // build affect list
    auto dark_brew_spells = spells_affected_by_passive( talent.subtlety.dark_brew->effectN( 1 ), prop );
    for ( auto s : dark_brew_spells )
    {
      if ( !range::contains( spells_dd, s ) )
        list_dd.spell.push_back( s->id() );
      if ( !range::contains( spells_td, s ) )
        list_td.spell.push_back( s->id() );
    }

    register_passive_affect_list( talent.subtlety.veiltouched, list_dd );
    register_passive_affect_list( talent.subtlety.veiltouched, list_td );
  }

  parse_all_class_passives();
  parse_all_passive_talents();
  parse_all_passive_sets();

  // Active Spells ==========================================================

  auto_attack = new actions::auto_melee_attack_t( this, "" );

  if ( talent.rogue.echoing_reprimand->ok() )
  {
    active.echoing_reprimand = get_background_action<actions::echoing_reprimand_t>( "echoing_reprimand" );
  }

  if ( talent.rogue.thistle_tea->ok() )
  {
    active.thistle_tea = get_background_action<actions::thistle_tea_t>( "thistle_tea_auto" );
  }

  // Assassination
  if ( talent.assassination.deathmark->ok() )
  {
    active.deathmark.garrote = get_secondary_trigger_action<actions::garrote_t>(
      secondary_trigger::DEATHMARK, "garrote_deathmark", spec.deathmark_garrote );
    active.deathmark.rupture = get_secondary_trigger_action<actions::rupture_t>(
      secondary_trigger::DEATHMARK, "rupture_deathmark", spec.deathmark_rupture );

    active.deathmark.amplifying_poison = get_secondary_trigger_action<actions::amplifying_poison_t::amplifying_poison_dd_t>(
      secondary_trigger::DEATHMARK, "amplifying_poison_deathmark", spec.deathmark_amplifying_poison );
    active.deathmark.deadly_poison_dot = get_secondary_trigger_action<actions::deadly_poison_t::deadly_poison_dot_t>(
      secondary_trigger::DEATHMARK, "deadly_poison_dot_deathmark", spec.deathmark_deadly_poison_dot );
    active.deathmark.deadly_poison_instant = get_secondary_trigger_action<actions::deadly_poison_t::deadly_poison_dd_t>(
      secondary_trigger::DEATHMARK, "deadly_poison_instant_deathmark", spec.deathmark_deadly_poison_instant );
    active.deathmark.instant_poison = get_secondary_trigger_action<actions::instant_poison_t::instant_poison_dd_t>(
      secondary_trigger::DEATHMARK, "instant_poison_deathmark", spec.deathmark_instant_poison );
    active.deathmark.wound_poison = get_secondary_trigger_action<actions::wound_poison_t::wound_poison_dd_t>(
      secondary_trigger::DEATHMARK, "wound_poison_deathmark", spec.deathmark_wound_poison );
  }

  if ( talent.assassination.internal_bleeding->ok() )
  {
    active.internal_bleeding = get_secondary_trigger_action<actions::internal_bleeding_t>(
      secondary_trigger::INTERNAL_BLEEDING, "internal_bleeding" );
  }

  if ( talent.assassination.caustic_spatter->ok() )
  {
    active.caustic_spatter = get_background_action<actions::caustic_spatter_t>( "caustic_spatter" );
  }

  if ( talent.assassination.doomblade->ok() )
  {
    active.doomblade = get_background_action<actions::doomblade_t>( "mutilated_flesh" );
  }

  if ( talent.assassination.poison_bomb->ok() )
  {
    active.poison_bomb = get_background_action<actions::poison_bomb_t>( "poison_bomb" );
  }

  // Outlaw
  if ( mastery.main_gauche->ok() )
  {
    active.main_gauche = get_secondary_trigger_action<actions::main_gauche_t>(
      secondary_trigger::MAIN_GAUCHE, "main_gauche" );
  }

  if ( spec.blade_flurry_attack->ok() )
  {
    active.blade_flurry = get_background_action<actions::blade_flurry_attack_t>( "blade_flurry_attack" );
  }

  if ( talent.outlaw.fan_the_hammer->ok() )
  {
    active.fan_the_hammer = get_secondary_trigger_action<actions::pistol_shot_t>(
      secondary_trigger::FAN_THE_HAMMER, "pistol_shot_fan_the_hammer" );
    active.fan_the_hammer->not_a_proc = true; // Scripted foreground cast, can trigger cast procs
  }

  // Subtlety
  if ( talent.subtlety.weaponmaster->ok() )
  {
    cooldowns.weaponmaster->base_duration = talent.subtlety.weaponmaster->internal_cooldown();
    active.weaponmaster.backstab = get_secondary_trigger_action<actions::backstab_t>(
      secondary_trigger::WEAPONMASTER, "backstab_weaponmaster" );
    active.weaponmaster.gloomblade = get_secondary_trigger_action<actions::gloomblade_t>(
      secondary_trigger::WEAPONMASTER, "gloomblade_weaponmaster" );
    active.weaponmaster.shadowstrike = get_secondary_trigger_action<actions::shadowstrike_t>(
      secondary_trigger::WEAPONMASTER, "shadowstrike_weaponmaster" );
  }

  if ( talent.subtlety.lingering_shadow->ok() )
  {
    active.lingering_shadow = get_background_action<actions::lingering_shadow_t>( "lingering_shadow" );
  }

  if ( specialization() == ROGUE_SUBTLETY )
  {
    active.shadow_blades_attack = get_background_action<actions::shadow_blades_attack_t>( "shadow_blades_attack" );
  }

  // Deathstalker
  if ( talent.deathstalker.clear_the_witnesses->ok() )
  {
    active.deathstalker.clear_the_witnesses = get_background_action<actions::clear_the_witnesses_t>( "clear_the_witnesses" );
    if ( talent.subtlety.shuriken_tornado->ok() )
    {
      active.deathstalker.clear_the_witnesses_tornado = get_background_action<actions::clear_the_witnesses_t>( "clear_the_witnesses_tornado" );
    }
  }

  if ( talent.deathstalker.corrupt_the_blood->ok() )
  {
    active.deathstalker.corrupt_the_blood = get_background_action<actions::corrupt_the_blood_t>( "corrupt_the_blood" );
  }

  if ( talent.deathstalker.deathstalkers_mark->ok() )
  {
    active.deathstalker.deathstalkers_mark = get_background_action<actions::deathstalkers_mark_t>( "deathstalkers_mark" );
  }

  if ( talent.deathstalker.fatal_intent->ok() )
  {
    active.deathstalker.fatal_intent = get_background_action<actions::fatal_intent_t>( "fatal_intent" );
  }

  if ( talent.deathstalker.hunt_them_down->ok() )
  {
    active.deathstalker.hunt_them_down = get_background_action<actions::hunt_them_down_t>( "hunt_them_down" );
  }

  if ( talent.deathstalker.singular_focus->ok() )
  {
    active.deathstalker.singular_focus = get_background_action<actions::singular_focus_t>( "singular_focus" );
  }

  // Fatebound
  if ( talent.fatebound.hand_of_fate->ok() )
  {
    active.fatebound.fatebound_coin_tails =
      get_secondary_trigger_action<actions::fatebound_coin_tails_t>( secondary_trigger::HAND_OF_FATE, "fatebound_coin_tails" );
    active.fatebound.fatebound_coin_tails->not_a_proc = true; // Scripted foreground cast, can trigger cast procs

    active.fatebound.lucky_coin =
      get_secondary_trigger_action<actions::fatebound_lucky_coin_t>( secondary_trigger::HAND_OF_FATE, "lucky_coin" );

    // Stats wrapper to group these for reporting purposes
    get_background_action<actions::hand_of_fate_t>( "hand_of_fate" );
  }

  if ( talent.fatebound.fate_intertwined->ok() )
  {
    active.fatebound.fate_intertwined = get_background_action<actions::fate_intertwined_t>( "fate_intertwined" );
  }

  // Trickster
  if ( talent.trickster.unseen_blade->ok() )
  {
    cooldowns.unseen_blade_icd->duration = talent.trickster.unseen_blade->internal_cooldown();
    active.trickster.unseen_blade = get_background_action<actions::unseen_blade_t>( "unseen_blade" );
  }

  if ( talent.trickster.nimble_flurry->ok() )
  {
    active.trickster.nimble_flurry = get_background_action<actions::nimble_flurry_t>( "nimble_flurry" );
  }

  // TWW1 Set Bonus
  if ( set_bonuses.tww1_outlaw_2pc->ok() )
  {
    active.tww1.ethereal_rampage = get_background_action<actions::ethereal_rampage_t>( "ethereal_rampage" );
  }
}

// rogue_t::init_talents ====================================================

void rogue_t::init_talents()
{
  player_t::init_talents();
}

// rogue_t::init_gains ======================================================

void rogue_t::init_gains()
{
  player_t::init_gains();

  gains.ace_up_your_sleeve              = get_gain( "Ace Up Your Sleeve" );
  gains.adrenaline_rush                 = get_gain( "Adrenaline Rush" );
  gains.adrenaline_rush_expiry          = get_gain( "Adrenaline Rush (Expiry)" );
  gains.blade_rush                      = get_gain( "Blade Rush" );
  gains.buried_treasure                 = get_gain( "Buried Treasure" );
  gains.darkest_night                   = get_gain( "Darkest Night" );
  gains.dashing_scoundrel               = get_gain( "Dashing Scoundrel" );
  gains.deal_fate                       = get_gain( "Deal Fate" );
  gains.energy_refund                   = get_gain( "Energy Refund" );
  gains.fatal_flourish                  = get_gain( "Fatal Flourish" );
  gains.improved_adrenaline_rush        = get_gain( "Improved Adrenaline Rush" );
  gains.improved_adrenaline_rush_expiry = get_gain( "Improved Adrenaline Rush (Expiry)" );
  gains.improved_ambush                 = get_gain( "Improved Ambush" );
  gains.killing_spree                   = get_gain( "Killing Spree" );
  gains.master_of_shadows               = get_gain( "Master of Shadows" );
  gains.premeditation                   = get_gain( "Premeditation" );
  gains.quick_draw                      = get_gain( "Quick Draw" );
  gains.relentless_strikes              = get_gain( "Relentless Strikes" );
  gains.roll_the_bones                  = get_gain( "Roll the Bones" );
  gains.ruthlessness                    = get_gain( "Ruthlessness" );
  gains.seal_fate                       = get_gain( "Seal Fate" );
  gains.shadow_blades                   = get_gain( "Shadow Blades" );
  gains.shadow_techniques               = get_gain( "Shadow Techniques" );
  gains.shadow_techniques_shadowcraft   = get_gain( "Shadow Techniques (Shadowcraft)" );
  gains.shrouded_suffocation            = get_gain( "Shrouded Suffocation" );
  gains.slice_and_dice                  = get_gain( "Slice and Dice" );
  gains.venomous_wounds                 = get_gain( "Venomous Vim" );
  gains.venomous_wounds_death           = get_gain( "Venomous Vim (Death)" );
}

// rogue_t::init_procs ======================================================

void rogue_t::init_procs()
{
  player_t::init_procs();

  // Roll the Bones Procs, two loops for display purposes in the HTML report
  auto roll_the_bones = static_cast<buffs::roll_the_bones_t*>( buffs.roll_the_bones );
  for ( size_t i = 0; i < roll_the_bones->buffs.size(); i++ )
  {
    roll_the_bones->procs[ i ] = get_proc( fmt::format( "Roll the Bones Buffs: {}", i + 1 ) );
  }
  for ( size_t i = 0; i < roll_the_bones->buffs.size(); i++ )
  {
    roll_the_bones->loss_procs[ i ] = get_proc( "Roll the Bones Buff Lost: " + roll_the_bones->buffs[ i ]->name_str );
  }

  procs.supercharger_wasted                   = get_proc( "Supercharger Wasted" );

  procs.weaponmaster                          = get_proc( "Weaponmaster" );
  procs.deepening_shadows                     = get_proc( "Deepening Shadows" );

  procs.amplifying_poison_consumed            = get_proc( "Amplifying Poison Consumed" );
  procs.amplifying_poison_deathmark_consumed  = get_proc( "Amplifying Poison (Deathmark) Consumed" );

  procs.tww2_subtlety_4pc                     = get_proc( "Winning Streak During Shadow Dance" );
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

  if ( options.initial_combo_points > 0.0 )
    resources.current[ RESOURCE_COMBO_POINT ] = options.initial_combo_points;
}

// rogue_t::init_buffs ======================================================

void rogue_t::create_buffs()
{
  player_t::create_buffs();

  // Baseline ===============================================================
  // Shared

  buffs.feint = make_buff( this, "feint", spell.feint )
    ->set_cooldown( timespan_t::zero() )
    ->set_duration( spell.feint->duration() );

  buffs.shadowstep = make_buff( this, "shadowstep", spell.shadowstep_buff )
    ->set_cooldown( timespan_t::zero() )
    ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED )
    ->add_invalidate( CACHE_RUN_SPEED );

  buffs.sprint = make_buff( this, "sprint", spell.sprint )
    ->set_cooldown( timespan_t::zero() )
    ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED )
    ->add_invalidate( CACHE_RUN_SPEED );

  buffs.slice_and_dice = new buffs::slice_and_dice_t( this );
  buffs.stealth = new buffs::stealth_t( this );
  buffs.vanish = new buffs::vanish_t( this );

  // Assassination ==========================================================

  buffs.envenom = make_buff( this, "envenom", spec.envenom )
    ->set_default_value_from_effect_type( A_ADD_FLAT_MODIFIER, P_PROC_CHANCE )
    ->set_duration( timespan_t::min() )
    ->disable_ticking( true )
    ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  // Outlaw =================================================================

  buffs.between_the_eyes = make_buff<stat_buff_t>( this, "between_the_eyes", spec.between_the_eyes )
    ->set_cooldown( timespan_t::zero() )
    ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
    ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
    ->set_refresh_behavior( buff_refresh_behavior::MAX );
  buffs.adrenaline_rush = new buffs::adrenaline_rush_t( this );
  buffs.blade_flurry = new buffs::blade_flurry_t( this );

  buffs.blade_rush = make_buff( this, "blade_rush", spec.blade_rush_energize )
    ->set_default_value_from_effect_type( A_PERIODIC_ENERGIZE )
    ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
      resource_gain( RESOURCE_ENERGY, b->check_value(), gains.blade_rush );
    } );

  buffs.opportunity = make_buff( this, "opportunity", spec.opportunity_buff )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC );

  buffs.summarily_dispatched = make_buff<damage_buff_t>( this, "summarily_dispatched", spec.summarily_dispatched_buff );
  buffs.summarily_dispatched
    ->set_duration( timespan_t::from_seconds( talent.outlaw.summarily_dispatched->effectN( 3 ).base_value() ) )
    ->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  // Roll the Bones Buffs
  buffs.one_of_a_kind = make_buff( this, "one_of_a_kind", spec.one_of_a_kind );
  buffs.double_trouble = make_buff<damage_buff_t>( this, "double_trouble", spec.double_trouble );
  buffs.triple_threat = make_buff<damage_buff_t>( this, "triple_threat", spec.triple_threat );
  buffs.jackpot = make_buff<damage_buff_t>( this, "jackpot", spec.jackpot );
  buffs.jackpot->add_invalidate( CACHE_CRIT_CHANCE );

  buffs.roll_the_bones = new buffs::roll_the_bones_t( this );

  // Subtlety ===============================================================

  buffs.danse_macabre = make_buff<damage_buff_t>( this, "danse_macabre", spec.danse_macabre_buff );
  buffs.danse_macabre->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buffs.shadow_blades = make_buff( this, "shadow_blades", talent.subtlety.shadow_blades )
    ->set_default_value_from_effect( 1 ) // Bonus Damage%
    ->set_cooldown( timespan_t::zero() )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
      if ( new_ == 0 )
        buffs.lingering_darkness->trigger();
    } );

  buffs.shadow_dance = new buffs::shadow_dance_t( this );
  if ( talent.subtlety.warning_signs->ok() )
  {
    buffs.shadow_dance->add_invalidate( CACHE_AUTO_ATTACK_SPEED );
  }

  // Talents ================================================================
  // Shared

  buffs.acrobatic_strikes = make_buff<damage_buff_t>( this, "acrobatic_strikes", spec.acrobatic_strikes_buff );
  buffs.acrobatic_strikes->set_default_value_from_effect_type( A_MOD_SPEED_ALWAYS );

  buffs.subterfuge = new buffs::subterfuge_t( this );

  buffs.thistle_tea = make_buff( this, "thistle_tea", spell.thistle_tea_buff )
    ->set_cooldown( timespan_t::zero() )
    ->set_default_value_from_effect_type( A_MOD_MASTERY_PCT )
    ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY );

  buffs.echoing_reprimand = make_buff( this, "echoing_reprimand", spell.echoing_reprimand_buff );

  buffs.supercharger.clear();
  std::array<unsigned int, 7> supercharger_ids = { 470398, 470406, 470409, 470412, 470414, 470415, 470416 };
  for ( size_t i = 0; i < supercharger_ids.size(); i++ )
  {
    buffs.supercharger.emplace_back( make_buff( this, fmt::format( "supercharge_{}", i + 1 ), talent.rogue.supercharger->ok() ?
                                                find_spell( supercharger_ids[ i ] ) : spell_data_t::not_found() )
                                     ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT ) );
  }

  // Hero
  // Deathstalker

  buffs.clear_the_witnesses = make_buff( this, "clear_the_witnesses", spell.clear_the_witnesses_buff );

  buffs.darkest_night = make_buff( this, "darkest_night", spell.darkest_night_buff );

  buffs.deathstalkers_mark = make_buff<damage_buff_t>( this, "deathstalkers_mark_buff", spell.deathstalkers_mark_buff );

  buffs.lingering_darkness = make_buff( this, "lingering_darkness", spell.lingering_darkness_buff )
    ->set_default_value_from_effect( 1 )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.momentum_of_despair = make_buff<damage_buff_t>( this, "momentum_of_despair", spell.momentum_of_despair_buff );

  buffs.symbolic_victory = make_buff<damage_buff_t>( this, "symbolic_victory", spell.symbolic_victory_buff );

  // Fatebound

  buffs.fatebound_coin_heads = make_buff<damage_buff_t>( this, "fatebound_coin_heads", spell.fatebound_coin_heads_buff, false );
  if ( spell.fatebound_coin_heads_buff->ok() && spell.fatebound_coin_heads_initial_buff->ok() )
  {
    // Combine the 2% per additional stack buff (which we use as the stacking base buff) and 8% from initial stack buff
    buffs.fatebound_coin_heads->set_direct_mod( spell.fatebound_coin_heads_buff, 1, spell.fatebound_coin_heads_buff->effectN( 1 ).percent(),
                                                1.0 + ( spell.fatebound_coin_heads_initial_buff->effectN( 1 ).percent() ) );
    buffs.fatebound_coin_heads->set_periodic_mod( spell.fatebound_coin_heads_buff, 2, spell.fatebound_coin_heads_buff->effectN( 2 ).percent(),
                                                  1.0 + ( spell.fatebound_coin_heads_initial_buff->effectN( 2 ).percent() ) );
    buffs.fatebound_coin_heads->set_auto_attack_mod( spell.fatebound_coin_heads_buff, 5, spell.fatebound_coin_heads_buff->effectN( 5 ).percent(),
                                                      1.0 + spell.fatebound_coin_heads_initial_buff->effectN( 3 ).percent() );
  }
  buffs.fatebound_coin_heads
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  buffs.fatebound_coin_tails = make_buff( this, "fatebound_coin_tails", spell.fatebound_coin_tails_buff )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
    
  if ( talent.fatebound.chosens_revelry->ok() )
  {
    buffs.fatebound_coin_tails->add_invalidate( CACHE_LEECH );
    buffs.fatebound_coin_heads->add_invalidate( CACHE_LEECH );
  }
  
  buffs.fatebound_lucky_coin = make_buff<stat_buff_t>( this, "fatebound_lucky_coin", spell.fatebound_lucky_coin_buff );
  buffs.fatebound_lucky_coin->set_pct_buff_type( STAT_PCT_BUFF_AGILITY )
    ->set_default_value( spell.fatebound_lucky_coin_buff->effectN( 1 ).percent() )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
    ->set_stack_change_callback( [ this ]( buff_t* b, int old_, int ) {
      // 07-29-2025 -- Testing shows this also reduces the active cooldowns by the amount when the buff is triggered
      if ( old_ == 0 && this->bugs )
      {
        cooldowns.adrenaline_rush->adjust( b->data().effectN( 2 ).time_value() );
        cooldowns.deathmark->adjust( b->data().effectN( 3 ).time_value() );
        cooldowns.kingsbane->adjust( b->data().effectN( 5 ).time_value() );
      }
    } );

  // Trickster

  buffs.cloud_cover = make_buff( this, "cloud_cover", spell.cloud_cover_distract );

  // TOCHECK -- Find the proper buff spell someday? Still doesn't seem to exist...
  buffs.disorienting_strikes = make_buff( this, "disorienting_strikes", talent.trickster.disorienting_strikes );
  if ( talent.trickster.disorienting_strikes->ok() )
  {
    buffs.disorienting_strikes
      ->set_duration( timespan_t::zero() )
      ->set_max_stack( as<int>( talent.trickster.disorienting_strikes->effectN( 2 ).base_value() ) )
      ->set_initial_stack( as<int>( talent.trickster.disorienting_strikes->effectN( 2 ).base_value() ) )
      ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  }

  buffs.escalating_blade = make_buff( this, "escalating_blade", spell.escalating_blade_buff );

  buffs.flawless_form = make_buff<damage_buff_t>( this, "flawless_form", spell.flawless_form_buff );
  buffs.flawless_form->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );

  buffs.unseen_blade_cd = make_buff( this, "unseen_blade_cooldown", spell.unseen_blade_buff )
    ->set_quiet( true );

  // Assassination

  buffs.improved_garrote = make_buff( this, "improved_garrote", spec.improved_garrote_buff )
    ->set_default_value_from_effect( 2 )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.improved_garrote_aura = make_buff( this, "improved_garrote_aura", spec.improved_garrote_buff )
    ->set_default_value_from_effect( 2 )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
    ->set_duration( sim->max_time / 2 )
    ->set_stack_change_callback( [this]( buff_t*, int, int new_ ) {
      if ( new_ == 0 )
        buffs.improved_garrote->trigger();
      else
        buffs.improved_garrote->expire();
    } );

  buffs.blindside = make_buff( this, "blindside", spec.blindside_buff )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_RESOURCE_COST_1 );

  buffs.kingsbane = make_buff<damage_buff_t>( this, "kingsbane", spec.kingsbane_buff );
  buffs.kingsbane->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buffs.scent_of_blood = make_buff( this, "scent_of_blood", spec.scent_of_blood_buff )
    ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
    ->set_duration( sim->max_time / 2 ) // Duration is 24s in spell data, but not enforced
    ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY );

  // Outlaw

  buffs.audacity = make_buff( this, "audacity", spec.audacity_buff );

  buffs.killing_spree = make_buff( this, "killing_spree", talent.outlaw.killing_spree )
    ->set_cooldown( timespan_t::zero() )
    ->set_duration( talent.outlaw.killing_spree->duration() );

  buffs.loaded_dice = make_buff( this, "loaded_dice", talent.outlaw.loaded_dice->effectN( 1 ).trigger() );

  // Subtlety

  buffs.lingering_shadow = make_buff( this, "lingering_shadow", spec.lingering_shadow_buff )
    ->set_default_value( 1.0 )
    ->set_tick_zero( true )
    ->set_freeze_stacks( true )
    ->set_tick_callback( [this]( buff_t* b, int, timespan_t ) {
      // This is wonky and set up with a rolling partial stack reduction so it alternates per tick
      // Otherwise we could just use the normal set_reverse_stack_count() functionality
      double stack_reduction = b->max_stack() / talent.subtlety.lingering_shadow->effectN( 3 ).base_value();
      int target_stacks = as<int>( std::ceil( b->max_stack() - ( stack_reduction * b->current_tick ) ) ) - 1;
      make_event( sim, [ b, target_stacks ] { 
        b->decrement( b->check() - target_stacks ); 
      } );
    } );

  buffs.master_of_shadows = make_buff( this, "master_of_shadows", spec.master_of_shadows_buff )
    ->set_default_value_from_effect_type( A_PERIODIC_ENERGIZE )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
      resource_gain( RESOURCE_ENERGY, b->check_value(), gains.master_of_shadows );
    } )
    ->set_stack_change_callback( [ this ]( buff_t* b, int, int new_ ) {
      if ( new_ == 1 )
        resource_gain( RESOURCE_ENERGY, b->data().effectN( 2 ).resource(), gains.master_of_shadows );
    } );

  buffs.premeditation = make_buff( this, "premeditation", spec.premeditation_buff )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  buffs.shadow_techniques = make_buff( this, "shadow_techniques", spec.shadow_techniques_energize )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
    ->set_duration( sim->max_time / 2 );
  buffs.shadow_techniques->reactable = true;

  buffs.shot_in_the_dark = make_buff( this, "shot_in_the_dark", spec.shot_in_the_dark_buff );

  buffs.silent_storm = make_buff( this, "silent_storm", spec.silent_storm_buff )
    ->set_default_value_from_effect_type( A_ADD_FLAT_MODIFIER, P_CRIT )
    ->add_invalidate( CACHE_CRIT_CHANCE )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
    ->set_duration( sim->max_time / 2 );

  buffs.secret_technique = make_buff( this, "secret_technique", spec.secret_technique )
    ->set_cooldown( timespan_t::zero() )
    ->set_quiet( true );

  buffs.shuriken_tornado = new buffs::shuriken_tornado_t( this );

  buffs.the_first_dance = make_buff( this, "the_first_dance", spec.the_first_dance_buff )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  // TODO -- Add register_on_combat_state_callback out of combat behavior

  buffs.the_rotten = make_buff<damage_buff_t>( this, "the_rotten", talent.subtlety.the_rotten->effectN( 1 ).trigger() )
    ->set_is_stacking_mod( false );

  buffs.perforated_veins = make_buff<damage_buff_t>( this, "perforated_veins", spec.perforated_veins_buff );
  buffs.perforated_veins_counter = make_buff( this, "perforated_veins_counter", spec.perforated_veins_counter );
  if ( spec.perforated_veins_counter->ok() )
  {
    buffs.perforated_veins_counter
      ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
      ->set_expire_at_max_stack( true )
      ->set_stack_change_callback( [ this ]( buff_t* b, int, int ) {
      // 2023-10-02 -- Buff can trigger prior to a WM proc but is not immediately consumed nor does it benefit
      //               Delay application to avoid this timing issue, ensure we clear any errant counter stacks
      if ( b->at_max_stacks() )
        make_event( sim, 1_ms, [ this ] {
          buffs.perforated_veins->trigger();
          buffs.perforated_veins_counter->expire();
        } );
      } );
  }

  buffs.goremaws_bite = make_buff( this, "goremaws_bite", spec.goremaws_bite_buff )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_RESOURCE_COST_1 );
  buffs.goremaws_bite->set_initial_stack( buffs.goremaws_bite->max_stack() );

  // Set Bonus Items ========================================================

  buffs.tww1_assassination_2pc = make_buff<damage_buff_t>( this, "vile_tincture", spec.tww1_assassination_2pc_buff );
  if ( spec.tww1_assassination_2pc_buff->ok() )
  {
    buffs.tww1_assassination_2pc->set_chance( set_bonuses.tww1_assassination_2pc->proc_chance() );
    if ( set_bonuses.tww1_assassination_4pc->ok() )
    {
      const int trigger_stacks = as<int>( set_bonuses.tww1_assassination_4pc->effectN( 1 ).base_value() );
      buffs.tww1_assassination_2pc->set_stack_change_callback( [ this, trigger_stacks ]( buff_t*, int, int new_ ) {
        if ( new_ >= trigger_stacks )
          buffs.tww1_assassination_4pc->trigger();
        else
          buffs.tww1_assassination_4pc->expire();
      } );
    }
  }
  buffs.tww1_assassination_4pc = make_buff<damage_buff_t>( this, "thrombotic_tincture", spec.tww1_assassination_4pc_buff );
  buffs.tww1_assassination_4pc->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  buffs.tww1_outlaw_4pc = make_buff<damage_buff_t>( this, "ethereal_rampage", spec.tww1_outlaw_4pc_buff );
  buffs.tww1_outlaw_4pc->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  buffs.tww1_subtlety_2pc = make_buff<damage_buff_t>( this, "poised_shadows", set_bonuses.tww1_subtlety_2pc->effectN( 1 ).trigger() );
  buffs.tww1_subtlety_4pc = make_buff<damage_buff_t>( this, "bolstering_shadows", set_bonuses.tww1_subtlety_4pc->effectN( 1 ).trigger() );

  auto tww2_assassination_buff = set_bonuses.tww2_assassination_2pc->effectN( 1 ).trigger();
  buffs.tww2_assassination_2pc = make_buff<damage_buff_t>( this, "winning_streak_asssassination", tww2_assassination_buff );
  if ( set_bonuses.tww2_assassination_2pc->ok() )
  {
    buffs.tww2_assassination_2pc->set_chance( 1.0 ); // Proc rate chance on the buff is the removal percent, not application
  }
  // Clone buff for the 4pc and manually increase since it does not use an affecting whitelist
  buffs.tww2_assassination_4pc = make_buff<damage_buff_t>( this, "winning_streak_asssassination_cashout", tww2_assassination_buff,
                                                           tww2_assassination_buff->effectN( 1 ).percent() * ( 1.0 + spec.tww2_assassination_4pc_buff->effectN( 1 ).percent() ) );
  if ( set_bonuses.tww2_assassination_4pc->ok() )
  {
    buffs.tww2_assassination_4pc->set_chance( 1.0 ) // Proc rate chance on the buff is the removal percent, not application
      ->set_duration( spec.tww2_assassination_4pc_buff->duration() )
      ->set_stack_change_callback( [ this ]( buff_t* b, int old_, int new_ ) {
      // Restore the original 2pc stack value on expiration of the 4pc
      if ( new_ == 0 )
      {
        assert( b->default_value && !buffs.tww2_assassination_2pc->check() );
        buffs.tww2_assassination_2pc->trigger( as<int>( b->default_value ) );
      }
      else if ( old_ == 0 )
      {
        assert( buffs.tww2_assassination_2pc->check() );
        b->default_value = buffs.tww2_assassination_2pc->check();
        buffs.tww2_assassination_2pc->expire();
      }
    } );
  }
  
  buffs.tww2_outlaw_2pc = make_buff<damage_buff_t>( this, "winning_streak_outlaw", set_bonuses.tww2_outlaw_2pc->effectN( 1 ).trigger() );
  buffs.tww2_outlaw_2pc->set_chance( 1.0 ); // Proc rate chance on the buff is the removal percent, not application
  
  buffs.tww2_subtlety_2pc = make_buff<damage_buff_t>( this, "winning_streak_subtlety", set_bonuses.tww2_subtlety_2pc->effectN( 1 ).trigger(), false );
  buffs.tww2_subtlety_2pc->set_chance( 1.0 ); // Proc rate chance on the buff is the removal percent, not application
  buffs.tww2_subtlety_2pc->set_cooldown( 400_ms ); // TODO -- Update when value is not 400 seconds in spell data...
  if ( set_bonuses.tww2_subtlety_2pc->ok() )
  {
    buffs.tww2_subtlety_2pc->set_direct_mod( set_bonuses.tww2_subtlety_2pc->effectN( 1 ).trigger(), 1 );
  }

  buffs.tww3_deathstalker_2pc = make_buff<damage_buff_t>( this, "deaths_study", spec.tww3_deathstalker_2pc_buff );
  buffs.tww3_deathstalker_2pc->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  if ( spec.tww3_deathstalker_2pc_buff->ok() )
  {
    buffs.deathstalkers_mark->apply_dynamic_buff_multiplier( buffs.tww3_deathstalker_2pc );
    buffs.momentum_of_despair->apply_dynamic_buff_multiplier( buffs.tww3_deathstalker_2pc );
    buffs.symbolic_victory->apply_dynamic_buff_multiplier( buffs.tww3_deathstalker_2pc );
  }

  buffs.tww3_trickster_4pc = make_buff( this, "tww3_trickster_4pc", set_bonuses.tww3_trickster_4pc );
  if ( set_bonuses.tww3_trickster_4pc->ok() )
  {
    // Buff is active on Coup cast but does not begin counting down until 1s into the cast, effectively adding 1s to its duration
    buffs.tww3_trickster_4pc->set_duration( timespan_t::from_seconds( set_bonuses.tww3_trickster_4pc->effectN( 2 ).base_value() ) + 1_s )
      ->set_default_value_from_effect( 1 ) // Coup de Grace damage multiplier
      ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
        if ( new_ == 0 )
          buffs.escalating_blade->expire(); // Technically this is the same buff in-game
      } );
  }
}

// rogue_t::invalidate_cache =========================================

void rogue_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_HASTE:
      if ( talent.rogue.swift_slasher->ok() && buffs.slice_and_dice->check() )
      {
        invalidate_cache( CACHE_AUTO_ATTACK_SPEED );
      }
      break;
    default:
      break;
  }
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

static bool parse_fixed_rtb_odds( sim_t* sim, util::string_view /* name */, util::string_view value )
{
  auto odds = util::string_split<util::string_view>( value, "," );
  if ( odds.size() != 6 )
  {
    sim->errorf( "%s: Expected 6 comma-separated values for 'fixed_rtb_odds'", sim->active_player->name() );
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
    sim->errorf( "Warning: %s: 'fixed_rtb_odds' adding up to %f instead of 100, re-scaling accordingly", sim->active_player->name(), sum );
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

  add_option( opt_func( "off_hand_secondary", parse_offhand_secondary ) );
  add_option( opt_func( "main_hand_secondary", parse_mainhand_secondary ) );
  add_option( opt_int( "initial_combo_points", options.initial_combo_points ) );
  add_option( opt_int( "initial_shadow_techniques", options.initial_shadow_techniques, -1, 4 ) );
  add_option( opt_int( "initial_supercharged_cp", options.initial_supercharged_cp, 0, 2 ) );
  add_option( opt_int( "fixed_rtb", options.fixed_rtb, 0, 4 ) );
  add_option( opt_func( "fixed_rtb_odds", parse_fixed_rtb_odds ) );
  add_option( opt_bool( "priority_rotation", options.priority_rotation ) );
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

  options.initial_combo_points = rogue->options.initial_combo_points;
  options.initial_shadow_techniques = rogue->options.initial_shadow_techniques;
  options.initial_supercharged_cp = rogue->options.initial_supercharged_cp;

  options.fixed_rtb = rogue->options.fixed_rtb;
  options.fixed_rtb_odds = rogue->options.fixed_rtb_odds;
  options.rogue_ready_trigger = rogue->options.rogue_ready_trigger;
  options.priority_rotation = rogue->options.priority_rotation;
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
  
  if ( unique_gear::find_special_effect( this, 448000 ) )
  {
    std::vector<unsigned> poison_ids = {
      spell.instant_poison->effectN( 1 ).trigger()->id(),
      spell.crippling_poison->effectN( 1 ).trigger()->id(),
      spell.wound_poison->effectN( 1 ).trigger()->id(),
      talent.rogue.atrophic_poison->effectN( 1 ).trigger()->id(),
      talent.rogue.numbing_poison->effectN( 1 ).trigger()->id(),
      talent.assassination.deadly_poison->effectN( 1 ).trigger()->id(),
      talent.assassination.amplifying_poison->effectN( 3 ).trigger()->id()
    };
    range::erase_remove( poison_ids, 0 );

    callbacks.register_callback_trigger_function(
      448000, dbc_proc_callback_t::trigger_fn_type::CONDITION,
      [ poison_ids ]( const dbc_proc_callback_t*, action_t* a, const action_state_t* ) {
        return !a->special || range::contains( poison_ids, a->data().id() );
    } );
  }

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

  if ( talent.deathstalker.fatal_intent->ok() )
  {
    auto const fatal_intent_driver = new special_effect_t( this );
    fatal_intent_driver->name_str = "fatal_intent_driver";
    fatal_intent_driver->spell_id = talent.deathstalker.fatal_intent->id();
    fatal_intent_driver->proc_flags_ = talent.deathstalker.fatal_intent->proc_flags();
    fatal_intent_driver->proc_flags2_ = PF2_ALL_CAST;
    special_effects.push_back( fatal_intent_driver );

    struct fatal_intent_driver_cb_t : public dbc_proc_callback_t
    {
      rogue_t* rogue;
      double health_threshold;

      fatal_intent_driver_cb_t( rogue_t* p, const special_effect_t& e )
        : dbc_proc_callback_t( p, e ), rogue( p ), health_threshold( p->talent.deathstalker.fatal_intent->effectN( 3 ).base_value() )
      {
      }

      void execute( action_t* a, action_state_t* s ) override
      {
        dbc_proc_callback_t::execute( a, s );
        if ( s->target && s->target->is_enemy() )
        {
          if ( s->target->health_percentage() >= health_threshold )
          {
            rogue->get_target_data( s->target )->debuffs.fatal_intent->trigger();
          }
          else if ( rogue->get_target_data( s->target )->debuffs.fatal_intent->check() )
          {
            rogue->active.deathstalker.fatal_intent->execute_on_target( s->target );
          }
        }
      }
    };

    auto cb = new fatal_intent_driver_cb_t( this, *fatal_intent_driver );
    cb->initialize();
  }

  if ( talent.trickster.thousand_cuts->ok() )
  {
    auto const thousand_cuts_driver = new special_effect_t( this );
    thousand_cuts_driver->name_str = "thousand_cuts_driver";
    thousand_cuts_driver->spell_id = talent.trickster.thousand_cuts->id();
    thousand_cuts_driver->proc_flags_ = talent.trickster.thousand_cuts->proc_flags();
    thousand_cuts_driver->proc_flags2_ = PF2_ALL_HIT;
    special_effects.push_back( thousand_cuts_driver );

    struct thousand_cuts_cb_t : public dbc_proc_callback_t
    {
      rogue_t* rogue;

      thousand_cuts_cb_t( rogue_t* p, const special_effect_t& e )
        : dbc_proc_callback_t( p, e ), rogue( p )
      {
      }

      void execute( action_t* a, action_state_t* s ) override
      {
        dbc_proc_callback_t::execute( a, s );
        rogue->buffs.unseen_blade_cd->expire();
      }
    };

    auto cb = new thousand_cuts_cb_t( this, *thousand_cuts_driver );
    cb->initialize();
  }

  if ( talent.deathstalker.singular_focus->ok() )
  {
    auto const singular_focus_driver = new special_effect_t( this );
    singular_focus_driver->name_str = "singular_focus_driver";
    singular_focus_driver->spell_id = talent.deathstalker.singular_focus->id();
    singular_focus_driver->proc_flags_ = talent.deathstalker.singular_focus->proc_flags();
    singular_focus_driver->proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
    special_effects.push_back( singular_focus_driver );

    struct singular_focus_cb_t : public dbc_proc_callback_t
    {
      rogue_t* rogue;
      const double multiplier;

      singular_focus_cb_t( rogue_t* p, const special_effect_t& e )
        : dbc_proc_callback_t( p, e ), rogue( p ), multiplier( p->talent.deathstalker.singular_focus->effectN( 1 ).percent() )
      {
      }

      void execute( action_t* a, action_state_t* s ) override
      {
        dbc_proc_callback_t::execute( a, s );

        if ( rogue->sim->active_enemies == 1 )
          return;

        buff_t* debuff = rogue->deathstalkers_mark_debuff;
        if ( !debuff || !debuff->check() || debuff->player == s->target || debuff->player->is_sleeping() )
          return;

        if ( !rogue->active.deathstalker.singular_focus )
          return;

        rogue->active.deathstalker.singular_focus->trigger_residual_action( s, multiplier, false, false, debuff->player );
      }
    };

    auto cb = new singular_focus_cb_t( this, *singular_focus_driver );
    cb->initialize();
  }

  if ( set_bonuses.tww2_assassination_2pc->ok() )
  {
    auto const winning_streak_driver = new special_effect_t( this );
    winning_streak_driver->name_str = "winning_streak_driver";
    winning_streak_driver->spell_id = set_bonuses.tww2_assassination_2pc->id();
    winning_streak_driver->proc_flags_ = set_bonuses.tww2_assassination_2pc->proc_flags();
    winning_streak_driver->proc_flags2_ = PF2_ALL_CAST;
    special_effects.push_back( winning_streak_driver );

    struct winning_streak_driver_cb_t : public dbc_proc_callback_t
    {
      rogue_t* rogue;

      winning_streak_driver_cb_t( rogue_t* p, const special_effect_t& e ) : dbc_proc_callback_t( p, e ), rogue( p )
      {
      }

      void execute( action_t* a, action_state_t* s ) override
      {
        dbc_proc_callback_t::execute( a, s );
        // Don't trigger the 2pc buff if the 4pc is already present, as the 4pc buff restores stacks when it fades
        if ( !rogue->buffs.tww2_assassination_4pc->check() )
        {
          rogue->buffs.tww2_assassination_2pc->trigger();
        }
      }
    };

    auto cb = new winning_streak_driver_cb_t( this, *winning_streak_driver );
    cb->initialize();
  }

  if ( set_bonuses.tww2_outlaw_2pc->ok() )
  {
    auto const winning_streak_driver = new special_effect_t( this );
    winning_streak_driver->name_str = "winning_streak_driver";
    winning_streak_driver->spell_id = set_bonuses.tww2_outlaw_2pc->id();
    winning_streak_driver->proc_flags_ = set_bonuses.tww2_outlaw_2pc->proc_flags();
    winning_streak_driver->proc_flags2_ = PF2_ALL_CAST;
    special_effects.push_back( winning_streak_driver );

    struct winning_streak_driver_cb_t : public dbc_proc_callback_t
    {
      rogue_t* rogue;

      winning_streak_driver_cb_t( rogue_t* p, const special_effect_t& e ) : dbc_proc_callback_t( p, e ), rogue( p )
      {
      }

      void execute( action_t* a, action_state_t* s ) override
      {
        dbc_proc_callback_t::execute( a, s );

        // Proc-based stack limit is defined in a dummy effect on the 2pc bonus spell
        if ( rogue->buffs.tww2_outlaw_2pc->check() < rogue->set_bonuses.tww2_outlaw_2pc->effectN( 2 ).base_value() )
        {
          rogue->buffs.tww2_outlaw_2pc->trigger();
        }
      }
    };

    auto cb = new winning_streak_driver_cb_t( this, *winning_streak_driver );
    cb->initialize();
  }

  if ( set_bonuses.tww2_subtlety_2pc->ok() )
  {
    auto const winning_streak_driver = new special_effect_t( this );
    winning_streak_driver->name_str = "winning_streak_driver";
    winning_streak_driver->spell_id = set_bonuses.tww2_subtlety_2pc->id();
    winning_streak_driver->proc_flags_ = set_bonuses.tww2_subtlety_2pc->proc_flags();
    winning_streak_driver->proc_flags2_ = PF2_ALL_CAST;
    special_effects.push_back( winning_streak_driver );

    struct winning_streak_driver_cb_t : public dbc_proc_callback_t
    {
      rogue_t* rogue;

      winning_streak_driver_cb_t( rogue_t* p, const special_effect_t& e ) : dbc_proc_callback_t( p, e ), rogue( p )
      {
      }

      void execute( action_t* a, action_state_t* s ) override
      {
        dbc_proc_callback_t::execute( a, s );
        rogue->buffs.tww2_subtlety_2pc->trigger( rogue->set_bonuses.tww2_subtlety_4pc->ok() && rogue->buffs.shadow_dance->check() ? 2 : 1 );
      }
    };

    auto cb = new winning_streak_driver_cb_t( this, *winning_streak_driver );
    cb->initialize();
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

  if( options.initial_shadow_techniques >= 0 )
    shadow_techniques_counter = options.initial_shadow_techniques;
  else
    shadow_techniques_counter = rng().range( 0, 4 );

  danse_macabre_tracker.clear();
  deathstalkers_mark_debuff = nullptr;

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
      r->cancel_auto_attacks();
    }
  }
};

void rogue_t::activate()
{
  player_t::activate();

  sim->target_non_sleeping_list.register_callback( restealth_callback_t( this ) );
}

// rogue_t::break_stealth ===================================================

void rogue_t::break_stealth()
{
  restealth_allowed = false;

  // Trigger Subterfuge
  if ( talent.rogue.subterfuge->ok() && !buffs.subterfuge->check() && stealthed( STEALTH_BASIC ) )
  {
    timespan_t buff_duration = ( specialization() == ROGUE_SUBTLETY ? talent.rogue.subterfuge->effectN( 4 ).time_value() :
                                                                      talent.rogue.subterfuge->effectN( 2 ).time_value() );
    buffs.subterfuge->trigger( buff_duration );
  }

  // Expiry delayed by 1ms in order to have it processed on the next tick. This seems to be what the server does.
  if ( player_t::buffs.shadowmeld->check() )
  {
    player_t::buffs.shadowmeld->expire( 1_ms );
  }

  if ( buffs.stealth->check() )
  {
    buffs.stealth->expire( 1_ms );
  }

  if ( buffs.vanish->check() )
  {
    buffs.vanish->expire( 1_ms );
  }
}

// rogue_t::cancel_auto_attack ==============================================

void rogue_t::cancel_auto_attacks()
{
  // Cancel scheduled AA events and record the swing timer to reference on restart
  if ( melee_main_hand && melee_main_hand->execute_event )
  {
    melee_main_hand->canceled = true;
    melee_main_hand->prev_scheduled_time = melee_main_hand->execute_event->occurs();
  }

  if ( melee_off_hand && melee_off_hand->execute_event )
  {
    melee_off_hand->canceled = true;
    melee_off_hand->prev_scheduled_time = melee_off_hand->execute_event->occurs();
  }

  player_t::cancel_auto_attacks();
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

  sim->print_debug( "{} performing weapon swap from {} to {}", *this,
                    *weapon_data[ slot ].item_data[ !to_weapon ], *weapon_data[ slot ].item_data[ to_weapon ] );

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

  if ( ( stealth_mask & STEALTH_SHADOW_DANCE ) && buffs.shadow_dance->check() )
    return true;

  if ( ( stealth_mask & STEALTH_SUBTERFUGE ) && buffs.subterfuge->check() )
    return true;

  if ( ( stealth_mask & STEALTH_SHADOWMELD ) && player_t::buffs.shadowmeld->check() )
    return true;

  if ( ( stealth_mask & STEALTH_IMPROVED_GARROTE ) && talent.assassination.improved_garrote->ok() &&
       ( buffs.improved_garrote->check() || buffs.improved_garrote_aura->check() ) )
    return true;

  return false;
}

// rogue_t::arise ===========================================================

void rogue_t::arise()
{
  player_t::arise();

  if ( talent.rogue.supercharger->ok() && options.initial_supercharged_cp > 0 )
  {
    for ( size_t i = 0; i < as<size_t>( options.initial_supercharged_cp ); i++ )
    {
      buffs.supercharger[ i ]->trigger();
    }
  }

  if ( talent.subtlety.the_first_dance->ok() )
  {
    buffs.the_first_dance->trigger(); // Does not currently reset the timer on pull
  }
}

// rogue_t::combat_begin ====================================================

void rogue_t::combat_begin()
{
  player_t::combat_begin();
}

// rogue_t::energy_regen_per_second =========================================

double rogue_t::resource_regen_per_second( resource_e r ) const
{
  double reg = player_t::resource_regen_per_second( r );

  // We handle some energy gain increases in rogue_t::regen() instead of the resource_regen_per_second method in order to better track their benefits.
  // For simple implementation without separate tracking, can put stuff here.

  return reg;
}

// rogue_t::non_stacking_movement_modifier ==================================

double rogue_t::non_stacking_movement_modifier() const
{
  double ms = player_t::non_stacking_movement_modifier();

  if ( buffs.sprint->up() )
    ms = std::max( buffs.sprint->check_value(), ms );

  if ( buffs.shadowstep->up() )
    ms = std::max( buffs.shadowstep->check_value(), ms );

  return ms;
}

// rogue_t::stacking_movement_modifier===================================

double rogue_t::stacking_movement_modifier() const
{
  double ms = player_t::stacking_movement_modifier();

  ms += talent.rogue.fleet_footed->effectN( 1 ).percent();
  ms += spell.fleet_footed->effectN( 1 ).percent(); // DFALPHA: Duplicate passive?
  ms += talent.outlaw.hit_and_run->effectN( 1 ).percent();
  ms += buffs.acrobatic_strikes->check_stack_value();

  if ( stealthed( ( STEALTH_BASIC | STEALTH_ROGUE ) ) )
  {
    ms += talent.rogue.shadowrunner->effectN( 1 ).percent();
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
    // Multiplicative energy gains
    double mult_regen_base = periodicity.total_seconds() * resource_regen_per_second( RESOURCE_ENERGY );

    if ( buffs.adrenaline_rush->up() )
    {
      double energy_regen = mult_regen_base * buffs.adrenaline_rush->data().effectN( 1 ).percent();
      resource_gain( RESOURCE_ENERGY, energy_regen, gains.adrenaline_rush );
      mult_regen_base += energy_regen;
    }
  }
}

// rogue_t::available =======================================================

timespan_t rogue_t::available() const
{
  if ( ready_type == READY_POLL )
    return player_t::available();

  const double energy = resources.current[ RESOURCE_ENERGY ];
  if ( energy >= rogue_ready_trigger_threshold )
    return 100_ms;

  // TODO -- See if this is improved by considering more regen sources
  return std::max( 100_ms, timespan_t::from_seconds( ( rogue_ready_trigger_threshold - energy ) /
                                                     resource_regen_per_second( RESOURCE_ENERGY ) ) );
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

// ROGUE MODULE INTERFACE ===================================================

class rogue_module_t : public module_t
{
public:
  rogue_module_t() : module_t( ROGUE ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    return new rogue_t( sim, name, r );
  }

  bool valid() const override
  { return true; }

  void static_init() const override
  {
  }

  void register_hotfixes() const override
  {
    // 2025-07-29 -- Fatebound Lucky Coin expires 15s after leaving combat
    hotfix::register_effect( "Rogue", "2025-07-29", "Fatebound Lucky Coin Expiry", 1156957 )
        .field( "base_value" )
        .operation( hotfix::HOTFIX_SET )
        .modifier( 15 )
        .verification_value( 10 );
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
