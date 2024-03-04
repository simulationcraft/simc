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
  TRIPLE_THREAT,
  FLAGELLATION,
  MAIN_GAUCHE,
  DEATHMARK,
  VICIOUS_VENOMS,
  HIDDEN_OPPORTUNITY,
  FAN_THE_HAMMER,
  CRACKSHOT,
  T31_SUBTLETY_2PC,
};

enum stealth_type_e
{
  STEALTH_NORMAL = 0x01,
  STEALTH_VANISH = 0x02,
  STEALTH_SHADOWMELD = 0x04,
  STEALTH_SUBTERFUGE = 0x08,
  STEALTH_SHADOW_DANCE = 0x10,
  STEALTH_SEPSIS = 0x20,
  STEALTH_IMPROVED_GARROTE = 0x40,

  STEALTH_BASIC = ( STEALTH_NORMAL | STEALTH_VANISH ),            // Normal + Vanish
  STEALTH_ROGUE = ( STEALTH_SUBTERFUGE | STEALTH_SHADOW_DANCE ),  // Subterfuge + Shadowdance

  // All Stealth states that enable Stealth ability stance masks
  STEALTH_STANCE = ( STEALTH_BASIC | STEALTH_ROGUE | STEALTH_SHADOWMELD | STEALTH_SEPSIS ),

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
  struct flagellation_damage_t;
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

    // Used by underhanded upper hand, it's not a *real* pause, but rather an application of a 100x slowdown via time mod
    rogue_buff_t* pause()
    {
      set_dynamic_time_duration_multiplier( 100.0 );
      return this;
    }

    rogue_buff_t* unpause()
    {
      set_dynamic_time_duration_multiplier( 1.0 );
      return this;
    }
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
    dot_t* sepsis;
    dot_t* serrated_bone_spike;
    dot_t* soulrip;
  } dots;

  struct debuffs_t
  {
    buff_t* amplifying_poison;
    buff_t* amplifying_poison_deathmark;
    buff_t* atrophic_poison;
    buff_t* between_the_eyes;
    buff_t* caustic_spatter;
    buff_t* crippling_poison;
    damage_buff_t* deathmark;
    buff_t* find_weakness;
    buff_t* flagellation;
    buff_t* ghostly_strike;
    buff_t* numbing_poison;
    buff_t* sting_like_a_bee;
    damage_buff_t* shiv;
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
public:
  // Shadow Techniques swing counter;
  unsigned shadow_techniques_counter;
  // Assassination T31 Natureblight energy accumulator
  double t31_assassination_2pc_accumulator;

  // Danse Macabre Ability ID Tracker
  std::vector<unsigned> danse_macabre_tracker;

  // Active
  struct
  {
    actions::rogue_poison_t* lethal_poison = nullptr;
    actions::rogue_poison_t* lethal_poison_dtb = nullptr;
    actions::rogue_poison_t* nonlethal_poison = nullptr;
    actions::rogue_poison_t* nonlethal_poison_dtb = nullptr;

    actions::rogue_attack_t* blade_flurry = nullptr;
    actions::rogue_attack_t* caustic_spatter = nullptr;
    actions::rogue_attack_t* fan_the_hammer = nullptr;
    actions::flagellation_damage_t* flagellation = nullptr;
    actions::rogue_attack_t* internal_bleeding = nullptr;
    actions::rogue_attack_t* lingering_shadow = nullptr;
    actions::rogue_attack_t* main_gauche = nullptr;
    actions::rogue_attack_t* poison_bomb = nullptr;
    actions::shadow_blades_attack_t* shadow_blades_attack = nullptr;
    actions::rogue_attack_t* triple_threat_mh = nullptr;
    actions::rogue_attack_t* triple_threat_oh = nullptr;
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
      actions::rogue_attack_t* mutilate_mh = nullptr;
      actions::rogue_attack_t* mutilate_oh = nullptr;
      actions::rogue_attack_t* ambush = nullptr;
    } vicious_venoms;
    struct
    {
      actions::rogue_attack_t* crimson_tempest = nullptr;
      actions::rogue_attack_t* garrote = nullptr;
      actions::rogue_attack_t* rupture = nullptr;
      actions::rogue_attack_t* deathmark_garrote = nullptr;
      actions::rogue_attack_t* deathmark_rupture = nullptr;
    } sanguine_blades;
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
    damage_buff_t* broadside;
    buff_t* buried_treasure;
    damage_buff_t* grand_melee;
    buff_t* skull_and_crossbones;
    damage_buff_t* ruthless_precision;
    buff_t* true_bearing;
    // Subtlety
    buff_t* shadow_blades;
    damage_buff_t* shadow_dance;
    damage_buff_t* symbols_of_death;

    // Talents
    // Shared
    std::vector<buff_t*> echoing_reprimand;

    buff_t* alacrity;
    damage_buff_t* cold_blood;
    damage_buff_t* nightstalker;
    buff_t* sepsis;
    buff_t* subterfuge;
    buff_t* thistle_tea;
    
    // Assassination
    buff_t* blindside;
    buff_t* improved_garrote;
    buff_t* improved_garrote_aura;
    buff_t* indiscriminate_carnage;
    buff_t* indiscriminate_carnage_aura;
    damage_buff_t* kingsbane;
    damage_buff_t* master_assassin;
    damage_buff_t* master_assassin_aura;
    buff_t* scent_of_blood;

    // Outlaw
    buff_t* audacity;
    buff_t* greenskins_wickers;
    buff_t* killing_spree;
    buff_t* loaded_dice;
    buffs::rogue_buff_t* slice_and_dice;
    buff_t* take_em_by_surprise;
    buff_t* take_em_by_surprise_aura;
    damage_buff_t* summarily_dispatched;

    // Subtlety
    damage_buff_t* danse_macabre;
    buff_t* deeper_daggers;
    damage_buff_t* finality_eviscerate;
    buff_t* finality_rupture;
    damage_buff_t* finality_black_powder;
    buff_t* flagellation;
    buff_t* flagellation_persist;
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
    damage_buff_t* the_rotten;

    // Set Bonuses
    damage_buff_t* t29_assassination_4pc;
    damage_buff_t* t29_outlaw_2pc;
    damage_buff_t* t29_outlaw_4pc;
    damage_buff_t* t29_subtlety_2pc;
    damage_buff_t* t29_subtlety_4pc;
    damage_buff_t* t29_subtlety_4pc_black_powder;
    buff_t* t30_assassination_4pc;
    buff_t* t30_outlaw_4pc;
    damage_buff_t* t31_assassination_2pc;

  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* adrenaline_rush;
    cooldown_t* between_the_eyes;
    cooldown_t* blade_flurry;
    cooldown_t* blade_rush;
    cooldown_t* blind;
    cooldown_t* cloak_of_shadows;
    cooldown_t* cold_blood;
    cooldown_t* deathmark;
    cooldown_t* echoing_reprimand;
    cooldown_t* evasion;
    cooldown_t* feint;
    cooldown_t* flagellation;
    cooldown_t* garrote;
    cooldown_t* ghostly_strike;
    cooldown_t* goremaws_bite;
    cooldown_t* gouge;
    cooldown_t* grappling_hook;
    cooldown_t* keep_it_rolling;
    cooldown_t* killing_spree;
    cooldown_t* kingsbane;
    cooldown_t* roll_the_bones;
    cooldown_t* secret_technique;
    cooldown_t* sepsis;
    cooldown_t* serrated_bone_spike;
    cooldown_t* shadow_blades;
    cooldown_t* shadow_dance;
    cooldown_t* shadowstep;
    cooldown_t* shiv;
    cooldown_t* shuriken_tornado;
    cooldown_t* sprint;
    cooldown_t* symbols_of_death;
    cooldown_t* thistle_tea;
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
    gain_t* dashing_scoundrel;
    gain_t* fatal_flourish;
    gain_t* energy_refund;
    gain_t* master_of_shadows;
    gain_t* venom_rush;
    gain_t* venomous_wounds;
    gain_t* venomous_wounds_death;
    gain_t* relentless_strikes;
    gain_t* symbols_of_death;
    gain_t* symbols_of_death_t30;
    gain_t* slice_and_dice;

    // CP Gains
    gain_t* ace_up_your_sleeve;
    gain_t* broadside;
    gain_t* improved_adrenaline_rush;
    gain_t* improved_adrenaline_rush_expiry;
    gain_t* improved_ambush;
    gain_t* premeditation;
    gain_t* quick_draw;
    gain_t* ruthlessness;
    gain_t* seal_fate;
    gain_t* serrated_bone_spike;
    gain_t* shadow_techniques;
    gain_t* shadow_techniques_shadowcraft;
    gain_t* shadow_blades;
    gain_t* shrouded_suffocation;
    gain_t* the_first_dance;
    gain_t* t31_subtlety_4pc;

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
    const spell_data_t* echoing_reprimand;
    const spell_data_t* instant_poison;
    const spell_data_t* kick;
    const spell_data_t* kidney_shot;
    const spell_data_t* shadow_dance;
    const spell_data_t* shadowstep;
    const spell_data_t* slice_and_dice;
    const spell_data_t* sprint;
    const spell_data_t* stealth;
    const spell_data_t* vanish;
    const spell_data_t* wound_poison;
    const spell_data_t* feint;
    const spell_data_t* sap;

    // Class Passives
    const spell_data_t* all_rogue;
    const spell_data_t* critical_strikes;
    const spell_data_t* fleet_footed;           // DFALPHA: Duplicate passive?
    const spell_data_t* leather_specialization;

    // Background Spells
    const spell_data_t* alacrity_buff;
    const spell_data_t* leeching_poison_buff;
    const spell_data_t* nightstalker_buff;
    const spell_data_t* recuperator_heal;
    const spell_data_t* sepsis_buff;
    const spell_data_t* sepsis_expire_damage;
    const spell_data_t* shadowstep_buff;
    const spell_data_t* subterfuge_buff;
    const spell_data_t* vanish_buff;

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
    const spell_data_t* dashing_scoundrel;
    double dashing_scoundrel_gain = 0.0;
    const spell_data_t* deadly_poison_instant;
    const spell_data_t* improved_garrote_buff;
    const spell_data_t* improved_shiv_debuff;
    const spell_data_t* indiscriminate_carnage_buff;
    const spell_data_t* indiscriminate_carnage_buff_aura;
    const spell_data_t* internal_bleeding_debuff;
    const spell_data_t* kingsbane_buff;
    const spell_data_t* master_assassin_buff;
    const spell_data_t* poison_bomb_driver;
    const spell_data_t* poison_bomb_damage;
    const spell_data_t* sanguine_blades_damage;
    const spell_data_t* serrated_bone_spike_energize;
    const spell_data_t* scent_of_blood_buff;
    const spell_data_t* venom_rush_energize;
    const spell_data_t* vicious_venoms_ambush;
    const spell_data_t* vicious_venoms_mutilate_mh;
    const spell_data_t* vicious_venoms_mutilate_oh;
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

    const spell_data_t* audacity_buff;
    const spell_data_t* blade_flurry_attack;
    const spell_data_t* blade_flurry_instant_attack;
    const spell_data_t* blade_rush_attack;
    const spell_data_t* blade_rush_energize;
    const spell_data_t* doomblade_debuff;
    const spell_data_t* greenskins_wickers;
    const spell_data_t* greenskins_wickers_buff;
    const spell_data_t* hidden_opportunity_extra_attack;
    const spell_data_t* improved_adrenaline_rush_energize;
    const spell_data_t* killing_spree_mh_attack;
    const spell_data_t* killing_spree_oh_attack;
    const spell_data_t* opportunity_buff;
    const spell_data_t* sinister_strike_extra_attack;
    const spell_data_t* sting_like_a_bee_debuff;
    const spell_data_t* summarily_dispatched_buff;
    const spell_data_t* take_em_by_surprise_buff;
    const spell_data_t* triple_threat_attack;
    const spell_data_t* ace_up_your_sleeve_energize;

    const spell_data_t* broadside;
    const spell_data_t* buried_treasure;
    const spell_data_t* grand_melee;
    const spell_data_t* skull_and_crossbones;
    const spell_data_t* ruthless_precision;
    const spell_data_t* true_bearing;

    // Subtlety Spells
    const spell_data_t* subtlety_rogue;
    const spell_data_t* backstab;
    const spell_data_t* black_powder;
    const spell_data_t* eviscerate;
    const spell_data_t* shadow_dance;         // Baseline charge increase passive
    const spell_data_t* shadow_techniques;
    const spell_data_t* shadowstrike;
    const spell_data_t* shuriken_storm;    
    const spell_data_t* shuriken_toss;
    const spell_data_t* symbols_of_death;

    const spell_data_t* black_powder_shadow_attack;
    const spell_data_t* danse_macabre_buff;
    const spell_data_t* deeper_daggers_buff;
    const spell_data_t* eviscerate_shadow_attack;
    const spell_data_t* find_weakness_debuff;
    const spell_data_t* finality_black_powder_buff;
    const spell_data_t* finality_eviscerate_buff;
    const spell_data_t* finality_rupture_buff;
    const spell_data_t* flagellation_buff;
    const spell_data_t* flagellation_persist_buff;
    const spell_data_t* flagellation_damage;
    const spell_data_t* goremaws_bite_buff;
    const spell_data_t* master_of_shadows_buff;
    const spell_data_t* perforated_veins_buff;
    const spell_data_t* perforated_veins_counter;
    const spell_data_t* premeditation_buff;
    const spell_data_t* relentless_strikes_energize;
    const spell_data_t* replicating_shadows_tick;
    const spell_data_t* invigorating_shadowdust_cdr;
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

    // Multi-Spec
    const spell_data_t* rupture;      // Assassination + Subtlety
    const spell_data_t* shadowstep;   // Assassination + Subtlety, baseline charge increase passive

    // Set Bonuses
    const spell_data_t* t30_assassination_2pc_tick;
    const spell_data_t* t30_assassination_4pc_buff;
    const spell_data_t* t30_outlaw_2pc_attack;
    const spell_data_t* t30_outlaw_4pc_attack;
    const spell_data_t* t30_outlaw_4pc_buff;
    const spell_data_t* t30_subtlety_4pc_buff;
    const spell_data_t* t31_assassination_2pc_buff;
    const spell_data_t* t31_assassination_4pc_attack;
    double t31_subtlety_2pc_chance = 0.33; // TOCHECK -- Hardcoded, not in spell data
    const spell_data_t* t31_subtlety_2pc_black_powder;
    const spell_data_t* t31_subtlety_2pc_eviscerate;
    const spell_data_t* t31_subtlety_2pc_rupture;

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
      player_talent_t shadowstep;

      player_talent_t master_poisoner;          // No implementation
      player_talent_t elusiveness;              // No implementation
      player_talent_t cheat_death;              // No implementation
      player_talent_t blackjack;                // No implementation
      player_talent_t tricks_of_the_trade;      // No implementation

      player_talent_t numbing_poison;
      player_talent_t atrophic_poison;
      player_talent_t nimble_fingers;
      player_talent_t improved_sprint;
      player_talent_t shadowrunner;

      player_talent_t improved_wound_poison;
      player_talent_t fleet_footed;             // No implementation
      player_talent_t iron_stomach;             // No implementation
      player_talent_t unbreakable_stride;       // No implementation
      player_talent_t featherfoot;
      player_talent_t rushed_setup;
      player_talent_t subterfuge;

      player_talent_t superior_mixture;         // No implementation
      player_talent_t deadened_nerves;          // No implementation
      player_talent_t graceful_guile;           // No implementation
      player_talent_t stillshroud;              // No implementation

      player_talent_t deadly_precision;
      player_talent_t virulent_poisons;
      player_talent_t recuperator;
      player_talent_t improved_ambush;
      player_talent_t tight_spender;

      player_talent_t leeching_poison;
      player_talent_t lethality;
      player_talent_t acrobatic_strikes;
      player_talent_t alacrity;
      player_talent_t soothing_darkness;        // No implementation

      player_talent_t vigor;
      player_talent_t echoing_reprimand;
      player_talent_t nightstalker;
      
      player_talent_t thistle_tea;
      player_talent_t cold_blood;
      player_talent_t resounding_clarity;
      player_talent_t reverberation;
      player_talent_t deeper_stratagem;
      player_talent_t shadow_dance;
    } rogue;

    struct assassination_talents_t
    {
      player_talent_t deadly_poison;

      player_talent_t improved_shiv;
      player_talent_t venomous_wounds;
      player_talent_t path_of_blood;

      player_talent_t cut_to_the_chase;
      player_talent_t improved_poisons;
      player_talent_t bloody_mess;

      player_talent_t thrown_precision;
      player_talent_t seal_fate;
      player_talent_t caustic_spatter;
      player_talent_t internal_bleeding;
      player_talent_t improved_garrote;

      player_talent_t crimson_tempest;
      player_talent_t lightweight_shiv;
      player_talent_t deathmark;
      player_talent_t sanguine_blades;
      player_talent_t master_assassin;

      player_talent_t flying_daggers;
      player_talent_t venom_rush;
      player_talent_t vicious_venoms;
      player_talent_t fatal_concoction;
      player_talent_t lethal_dose;
      player_talent_t intent_to_kill;
      player_talent_t iron_wire;                // No implementation

      player_talent_t systemic_failure;
      player_talent_t amplifying_poison;
      player_talent_t twist_the_knife;
      player_talent_t doomblade;

      player_talent_t blindside;
      player_talent_t tiny_toxic_blade;
      player_talent_t dashing_scoundrel;
      player_talent_t shrouded_suffocation;
      player_talent_t sepsis;
      player_talent_t serrated_bone_spike;

      player_talent_t zoldyck_recipe;
      player_talent_t poison_bomb;
      player_talent_t scent_of_blood;

      player_talent_t arterial_precision;
      player_talent_t kingsbane;
      player_talent_t dragon_tempered_blades;
      player_talent_t indiscriminate_carnage;
      player_talent_t sudden_demise;            // Partial NYI for "execute" mechanic
      
    } assassination;

    struct outlaw_talents_t
    {
      player_talent_t opportunity;

      player_talent_t adrenaline_rush;

      player_talent_t retractable_hook;
      player_talent_t dirty_tricks;
      player_talent_t combat_potency;
      player_talent_t combat_stamina;
      player_talent_t hit_and_run;

      player_talent_t blinding_powder;
      player_talent_t float_like_a_butterfly;
      player_talent_t sting_like_a_bee;
      player_talent_t riposte;                  // No implementation
      player_talent_t precision_shot;

      player_talent_t heavy_hitter;
      player_talent_t devious_stratagem;
      player_talent_t thiefs_versatility;
      player_talent_t fatal_flourish;
      player_talent_t ambidexterity;
      player_talent_t quick_draw;
      player_talent_t deft_maneuvers;

      player_talent_t ruthlessness;
      player_talent_t swift_slasher;
      player_talent_t loaded_dice;
      player_talent_t sleight_of_hand;
      player_talent_t ace_up_your_sleeve;
      player_talent_t improved_between_the_eyes;

      player_talent_t audacity;
      player_talent_t triple_threat;
      player_talent_t improved_adrenaline_rush;
      player_talent_t improved_main_gauche;
      player_talent_t dancing_steel;

      player_talent_t underhanded_upper_hand;
      player_talent_t sepsis;
      player_talent_t ghostly_strike;
      player_talent_t count_the_odds;
      player_talent_t blade_rush;
      player_talent_t precise_cuts;

      player_talent_t take_em_by_surprise;
      player_talent_t summarily_dispatched;
      player_talent_t fan_the_hammer;

      player_talent_t hidden_opportunity;
      player_talent_t crackshot;
      player_talent_t keep_it_rolling;
      player_talent_t killing_spree;
      player_talent_t greenskins_wickers;

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
      player_talent_t without_a_trace;
      player_talent_t fade_to_nothing;          // No implementation
      player_talent_t cloaked_in_shadow;        // No implementation
      player_talent_t night_terrors;            // No implementation
      player_talent_t terrifying_pace;          // No implementation

      player_talent_t swift_death;
      player_talent_t improved_shadow_techniques;
      player_talent_t gloomblade;
      player_talent_t the_first_dance;
      player_talent_t master_of_shadows;
      player_talent_t secret_technique;
      player_talent_t relentless_strikes;
      player_talent_t silent_storm;

      player_talent_t premeditation;
      player_talent_t planned_execution;
      player_talent_t warning_signs;
      player_talent_t improved_shadow_dance;
      player_talent_t shadowed_finishers;
      player_talent_t secret_stratagem;
      player_talent_t replicating_shadows;

      player_talent_t weaponmaster;
      player_talent_t shadow_focus;
      player_talent_t deepening_shadows;
      player_talent_t veiltouched;
      player_talent_t shuriken_tornado;

      player_talent_t inevitability;     
      player_talent_t sepsis;
      player_talent_t perforated_veins;
      player_talent_t lingering_shadow;
      player_talent_t deeper_daggers;
      player_talent_t flagellation;

      player_talent_t invigorating_shadowdust;
      player_talent_t dark_shadow;
      player_talent_t finality;

      player_talent_t the_rotten;
      player_talent_t shadowcraft;              // Partial NYI, TODO finisher interaction
      player_talent_t danse_macabre;
      player_talent_t goremaws_bite;
      player_talent_t dark_brew;

    } subtlety;

    struct shared_talents_t
    {
      player_talent_t sepsis;

    } shared;

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
    std::vector<proc_t*> echoing_reprimand;

    // Assassination
    proc_t* amplifying_poison_consumed;
    proc_t* amplifying_poison_deathmark_consumed;
    proc_t* serrated_bone_spike_refund;
    proc_t* serrated_bone_spike_waste;
    proc_t* serrated_bone_spike_waste_partial;

    // Outlaw
    proc_t* count_the_odds;
    proc_t* count_the_odds_capped;
    proc_t* count_the_odds_ambush;
    proc_t* count_the_odds_ss;
    proc_t* count_the_odds_dispatch;
    proc_t* count_the_odds_stealth;
    proc_t* roll_the_bones_1;
    proc_t* roll_the_bones_2;
    proc_t* roll_the_bones_3;
    proc_t* roll_the_bones_4;
    proc_t* roll_the_bones_5;
    proc_t* roll_the_bones_6;
    proc_t* roll_the_bones_wasted;
    proc_t* t31_buff_extended;
    proc_t* t31_buff_not_extended;

    // Subtlety
    proc_t* deepening_shadows;
    proc_t* flagellation_cp_spend;
    proc_t* weaponmaster;

    // Set Bonus

  } procs;

  // Set Bonus effects
  struct set_bonuses_t
  {
    const spell_data_t* t29_assassination_2pc;
    const spell_data_t* t29_assassination_4pc;
    const spell_data_t* t29_outlaw_2pc;
    const spell_data_t* t29_outlaw_4pc;
    const spell_data_t* t29_subtlety_2pc;
    const spell_data_t* t29_subtlety_4pc;

    const spell_data_t* t30_assassination_2pc;
    const spell_data_t* t30_assassination_4pc;
    const spell_data_t* t30_outlaw_2pc;
    const spell_data_t* t30_outlaw_4pc;
    const spell_data_t* t30_subtlety_2pc;
    const spell_data_t* t30_subtlety_4pc;

    const spell_data_t* t31_assassination_2pc;
    const spell_data_t* t31_assassination_4pc;
    const spell_data_t* t31_outlaw_2pc;
    const spell_data_t* t31_outlaw_4pc;
    const spell_data_t* t31_subtlety_2pc;
    const spell_data_t* t31_subtlety_4pc;
  } set_bonuses;

  // Options
  struct rogue_options_t
  {
    std::vector<size_t> fixed_rtb;
    std::vector<double> fixed_rtb_odds;
    int initial_combo_points = 0;
    int initial_shadow_techniques = -1;
    bool rogue_ready_trigger = true;
    bool prepull_shadowdust = false;
    bool priority_rotation = false;
  } options;

  rogue_t( sim_t* sim, util::string_view name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, ROGUE, name, r ),
    shadow_techniques_counter( 0 ),
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
    cooldowns.adrenaline_rush           = get_cooldown( "adrenaline_rush" );
    cooldowns.between_the_eyes          = get_cooldown( "between_the_eyes" );
    cooldowns.blade_flurry              = get_cooldown( "blade_flurry" );
    cooldowns.blade_rush                = get_cooldown( "blade_rush" );
    cooldowns.blind                     = get_cooldown( "blind" );
    cooldowns.cloak_of_shadows          = get_cooldown( "cloak_of_shadows" );
    cooldowns.cold_blood                = get_cooldown( "cold_blood" );
    cooldowns.deathmark                 = get_cooldown( "deathmark" );
    cooldowns.echoing_reprimand         = get_cooldown( "echoing_reprimand" );
    cooldowns.evasion                   = get_cooldown( "evasion" );
    cooldowns.feint                     = get_cooldown( "feint" );
    cooldowns.flagellation              = get_cooldown( "flagellation" );
    cooldowns.garrote                   = get_cooldown( "garrote" );
    cooldowns.ghostly_strike            = get_cooldown( "ghostly_strike" );
    cooldowns.goremaws_bite             = get_cooldown( "goremaws_bite" );
    cooldowns.gouge                     = get_cooldown( "gouge" );
    cooldowns.grappling_hook            = get_cooldown( "grappling_hook" );
    cooldowns.keep_it_rolling           = get_cooldown( "keep_it_rolling" );
    cooldowns.killing_spree             = get_cooldown( "killing_spree" );
    cooldowns.kingsbane                 = get_cooldown( "kingsbane" );
    cooldowns.roll_the_bones            = get_cooldown( "roll_the_bones" );
    cooldowns.secret_technique          = get_cooldown( "secret_technique" );
    cooldowns.sepsis                    = get_cooldown( "sepsis" );
    cooldowns.serrated_bone_spike       = get_cooldown( "serrated_bone_spike" );
    cooldowns.shadow_blades             = get_cooldown( "shadow_blades" );
    cooldowns.shadow_dance              = get_cooldown( "shadow_dance" );
    cooldowns.shadowstep                = get_cooldown( "shadowstep" );
    cooldowns.shiv                      = get_cooldown( "shiv" );
    cooldowns.shuriken_tornado          = get_cooldown( "shuriken_tornado" );
    cooldowns.sprint                    = get_cooldown( "sprint" );   
    cooldowns.symbols_of_death          = get_cooldown( "symbols_of_death" );
    cooldowns.thistle_tea               = get_cooldown( "thistle_tea" );
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
  double    composite_melee_speed() const override;
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
  double    passive_movement_modifier() const override;
  double    temporary_movement_modifier() const override;
  void      apply_affecting_auras( action_t& action ) override;
  void      invalidate_cache( cache_e ) override;

  void break_stealth();
  void cancel_auto_attacks() override;
  void do_exsanguinate( dot_t* dot, double coeff );

  void trigger_venomous_wounds_death( player_t* ); // On-death trigger for Venomous Wounds energy replenish

  double consume_cp_max() const
  {
    return COMBO_POINT_MAX + as<double>( talent.rogue.deeper_stratagem->effectN( 2 ).base_value() +
                                         talent.outlaw.devious_stratagem->effectN( 2 ).base_value() +
                                         talent.subtlety.secret_stratagem->effectN( 2 ).base_value() );
  }

  double current_cp( bool /* react */ = false ) const
  {
    return resources.current[ RESOURCE_COMBO_POINT ];
  }

  // Current number of effective combo points, considering Echoing Reprimand
  double current_effective_cp( bool use_echoing_reprimand = true, bool react = false ) const
  {
    double current_cp = this->current_cp( react );

    if ( use_echoing_reprimand && current_cp > 0 )
    {
      if ( range::any_of( buffs.echoing_reprimand, [ current_cp ]( const buff_t* buff ) { return buff->check_value() == current_cp; } ) )
        return spell.echoing_reprimand->effectN( 2 ).base_value();
    }

    return current_cp;
  }

  // Extra attack proc chance for Outlaw mechanics (Sinister Strike, Audacity, Hidden Opportunity)
  double extra_attack_proc_chance() const
  {
    double proc_chance = spec.sinister_strike->effectN( 3 ).percent();
    if ( set_bonuses.t31_outlaw_2pc->ok() )
      proc_chance += set_bonuses.t31_outlaw_2pc->effectN( 1 ).percent();
    proc_chance += buffs.skull_and_crossbones->stack_value();
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

  proc_t* animacharged_cp_proc;
  proc_t* cold_blood_consumed_proc;
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
    bool alacrity = false;
    bool audacity = false;              // Stance Mask
    bool blindside = false;             // Stance Mask
    bool broadside_cp = false;
    bool cold_blood = false;
    bool danse_macabre = false;         // Trigger
    bool dashing_scoundrel = false;
    bool deathmark = false;             // Tuning Aura
    bool deepening_shadows = false;     // Trigger
    bool dragon_tempered_blades = false;// Proc Reduction
    bool flagellation = false;
    bool ghostly_strike = false;
    bool goremaws_bite = false;         // Cost Reduction
    bool improved_ambush = false;
    bool improved_shiv = false;
    bool lethal_dose = false;
    bool maim_mangle = false;           // Renamed Systemic Failure for DF talent
    bool master_assassin = false;
    bool relentless_strikes = false;    // Trigger
    bool ruthlessness = false;          // Trigger
    bool sepsis = false;                // Stance Mask
    bool shadow_blades_cp = false;
    bool zoldyck_insignia = false;

    bool t29_assassination_2pc = false;
    bool t30_subtlety_4pc = false;
    bool t31_subtlety_4pc = false;      // CP Generation Proc

    damage_affect_data deeper_daggers;
    damage_affect_data mastery_executioner;
    damage_affect_data mastery_potent_assassin;
    damage_affect_data t30_assassination_4pc;
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
    animacharged_cp_proc( nullptr ),
    cold_blood_consumed_proc( nullptr ),
    perforated_veins_consumed_proc( nullptr )
  {
    ab::parse_options( options );
    parse_spell_data( s );

    // rogue_t sets base and min GCD to 1s by default but let's also enforce non-hasted GCDs.
    // Even for rogue abilities that can be considered spells, hasted GCDs seem to be an exception rather than rule.
    // Those should be set explicitly. (see Vendetta, Shadow Blades, Detection)
    ab::gcd_type = gcd_haste_type::NONE;
    
    // Affecting Passive Auras
    // Put ability specific ones here; class/spec wide ones with labels that can effect things like trinkets in rogue_t::apply_affecting_auras.

    // Affecting Passive Spells
    ab::apply_affecting_aura( p->spec.shadow_dance );
    ab::apply_affecting_aura( p->spec.shadowstep );

    // Affecting Passive Talents
    ab::apply_affecting_aura( p->talent.rogue.master_poisoner );
    ab::apply_affecting_aura( p->talent.rogue.nimble_fingers );
    ab::apply_affecting_aura( p->talent.rogue.rushed_setup );
    ab::apply_affecting_aura( p->talent.rogue.improved_sprint );
    ab::apply_affecting_aura( p->talent.rogue.shadowstep );
    ab::apply_affecting_aura( p->talent.rogue.deadly_precision );
    ab::apply_affecting_aura( p->talent.rogue.virulent_poisons );
    ab::apply_affecting_aura( p->talent.rogue.tight_spender );
    ab::apply_affecting_aura( p->talent.rogue.acrobatic_strikes );
    ab::apply_affecting_aura( p->talent.rogue.lethality );
    ab::apply_affecting_aura( p->talent.rogue.deeper_stratagem );
    ab::apply_affecting_aura( p->talent.rogue.reverberation );
    ab::apply_affecting_aura( p->talent.rogue.shadow_dance );
    ab::apply_affecting_aura( p->talent.rogue.subterfuge );

    ab::apply_affecting_aura( p->talent.assassination.bloody_mess );
    ab::apply_affecting_aura( p->talent.assassination.thrown_precision );
    ab::apply_affecting_aura( p->talent.assassination.lightweight_shiv );
    ab::apply_affecting_aura( p->talent.assassination.flying_daggers );
    ab::apply_affecting_aura( p->talent.assassination.vicious_venoms );
    ab::apply_affecting_aura( p->talent.assassination.fatal_concoction );
    ab::apply_affecting_aura( p->talent.assassination.tiny_toxic_blade );
    ab::apply_affecting_aura( p->talent.assassination.shrouded_suffocation );
    ab::apply_affecting_aura( p->talent.assassination.arterial_precision );
    ab::apply_affecting_aura( p->talent.assassination.sudden_demise );

    ab::apply_affecting_aura( p->talent.outlaw.retractable_hook );
    ab::apply_affecting_aura( p->talent.outlaw.blinding_powder );
    ab::apply_affecting_aura( p->talent.outlaw.improved_between_the_eyes );
    ab::apply_affecting_aura( p->talent.outlaw.dirty_tricks );
    ab::apply_affecting_aura( p->talent.outlaw.heavy_hitter );
    ab::apply_affecting_aura( p->talent.outlaw.devious_stratagem );
    ab::apply_affecting_aura( p->talent.outlaw.deft_maneuvers );
    ab::apply_affecting_aura( p->talent.outlaw.underhanded_upper_hand );
    ab::apply_affecting_aura( p->talent.outlaw.precision_shot );

    ab::apply_affecting_aura( p->talent.subtlety.improved_backstab );
    ab::apply_affecting_aura( p->talent.subtlety.improved_shuriken_storm );
    ab::apply_affecting_aura( p->talent.subtlety.quick_decisions );
    ab::apply_affecting_aura( p->talent.subtlety.veiltouched );
    ab::apply_affecting_aura( p->talent.subtlety.swift_death );
    ab::apply_affecting_aura( p->talent.subtlety.improved_shadow_dance );
    ab::apply_affecting_aura( p->talent.subtlety.without_a_trace );
    ab::apply_affecting_aura( p->talent.subtlety.secret_stratagem );
    ab::apply_affecting_aura( p->talent.subtlety.dark_brew );

    // Dynamically affected flags
    // Special things like CP, Energy, Crit, etc.
    affected_by.shadow_blades_cp = ab::data().affected_by( p->talent.subtlety.shadow_blades->effectN( 2 ) ) ||
      ab::data().affected_by( p->talent.subtlety.shadow_blades->effectN( 3 ) ) ||
      ab::data().affected_by( p->talent.subtlety.shadow_blades->effectN( 4 ) );
    affected_by.adrenaline_rush_gcd = ab::data().affected_by( p->talent.outlaw.adrenaline_rush->effectN( 3 ) );
    
    affected_by.broadside_cp = ab::data().affected_by( p->spec.broadside->effectN( 1 ) ) ||
      ab::data().affected_by( p->spec.broadside->effectN( 2 ) ) ||
      ab::data().affected_by( p->spec.broadside->effectN( 3 ) );

    affected_by.audacity = ab::data().affected_by( p->spec.audacity_buff->effectN( 1 ) );
    affected_by.blindside = ab::data().affected_by( p->spec.blindside_buff->effectN( 1 ) );
    affected_by.danse_macabre = ab::data().affected_by( p->spec.danse_macabre_buff->effectN( 1 ) );
    affected_by.improved_ambush = ab::data().affected_by( p->talent.rogue.improved_ambush->effectN( 1 ) );
    affected_by.master_assassin = ab::data().affected_by( p->spec.master_assassin_buff->effectN( 1 ) );
    affected_by.improved_shiv =
      ( p->talent.assassination.improved_shiv->ok() && ab::data().affected_by( p->spec.improved_shiv_debuff->effectN( 1 ) ) ) ||
      ( p->talent.assassination.arterial_precision->ok() && ab::data().affected_by( p->spec.improved_shiv_debuff->effectN( 3 ) ) );

    if ( p->spell.sepsis_buff->ok() )
    {
      affected_by.sepsis = ab::data().affected_by( p->spell.sepsis_buff->effectN( 1 ) );
    }

    if ( p->talent.assassination.systemic_failure->ok() )
    {
      affected_by.maim_mangle = ab::data().affected_by( p->spec.garrote->effectN( 4 ) );
    }

    if ( p->talent.assassination.dashing_scoundrel->ok() )
    {
      affected_by.dashing_scoundrel = ab::data().affected_by( p->spec.envenom->effectN( 3 ) );
    }

    if ( p->talent.assassination.zoldyck_recipe->ok() )
    {
      // Not in spell data. Using the mastery whitelist as a baseline, most seem to apply (including VV)
      affected_by.zoldyck_insignia = ab::data().affected_by( p->mastery.potent_assassin->effectN( 1 ) ) ||
                                     ab::data().affected_by( p->mastery.potent_assassin->effectN( 2 ) ) ||
                                     ab::data().affected_by( p->mastery.potent_assassin->effectN( 3 ) );
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

    if ( p->talent.assassination.dragon_tempered_blades->ok() )
    {
      affected_by.dragon_tempered_blades = ab::data().affected_by( p->talent.assassination.dragon_tempered_blades->effectN( 2 ) );
    }

    if ( p->talent.outlaw.ghostly_strike->ok() )
    {
      affected_by.ghostly_strike = ab::data().affected_by( p->talent.outlaw.ghostly_strike->effectN( 3 ) );
    }

    if ( p->talent.subtlety.goremaws_bite->ok() && p->spec.goremaws_bite_buff->ok() )
    {
      affected_by.goremaws_bite = ab::data().affected_by( p->spec.goremaws_bite_buff->effectN( 2 ) );
    }

    if ( p->set_bonuses.t29_assassination_2pc->ok() )
    {
      affected_by.t29_assassination_2pc = ab::data().affected_by( p->spec.envenom->effectN( 7 ) );
    }

    if ( p->set_bonuses.t30_subtlety_4pc->ok() )
    {
      affected_by.t30_subtlety_4pc = ab::data().affected_by( p->spec.t30_subtlety_4pc_buff->effectN( 1 ) );
    }

    // Auto-parsing for damage affecting dynamic flags, this reads IF direct/periodic dmg is affected and stores by how much.
    // Still requires manual impl below but removes need to hardcode effect numbers.
    parse_damage_affecting_spell( p->spec.deeper_daggers_buff, affected_by.deeper_daggers );
    parse_damage_affecting_spell( p->mastery.executioner, affected_by.mastery_executioner );
    parse_damage_affecting_spell( p->mastery.potent_assassin, affected_by.mastery_potent_assassin );
    parse_damage_affecting_spell( p->spec.t30_assassination_4pc_buff, affected_by.t30_assassination_4pc );

    // Manually apply scripted modifiers from Veiltouched and Deeper Daggers when affected by Dark Brew talent
    if ( p->talent.subtlety.dark_brew->ok() && ab::data().affected_by( p->talent.subtlety.dark_brew->effectN( 1 ) ) )
    {
      if ( p->talent.subtlety.veiltouched->ok() )
      {
        damage_affect_data passive_list;
        parse_damage_affecting_spell( p->talent.subtlety.veiltouched, passive_list );
        if ( !passive_list.direct )
        {
          const spelleffect_data_t& effect = p->talent.subtlety.veiltouched->effectN( 1 );
          ab::base_dd_multiplier *= ( 1 + effect.percent() );
          p->sim->print_debug( "{} {} is manually affected by Veiltouched (id={} - effect #{})", *p, *this,
                               effect.id(), effect.spell_effect_num() + 1 );
          p->sim->print_debug( "{} base_dd_multiplier modified by {}% to {}", *this, effect.base_value(), ab::base_dd_multiplier );
          ab::affecting_list.emplace_back( &effect, effect.percent() );
        }
        if ( !passive_list.periodic )
        {
          const spelleffect_data_t& effect = p->talent.subtlety.veiltouched->effectN( 2 );
          ab::base_td_multiplier *= ( 1 + effect.percent() );
          p->sim->print_debug( "{} {} is manually affected by Veiltouched (id={} - effect #{})", *p, *this,
                               effect.id(), effect.spell_effect_num() + 1 );
          p->sim->print_debug( "{} base_td_multiplier modified by {}% to {}", *this, effect.base_value(), ab::base_td_multiplier );
          ab::affecting_list.emplace_back( &effect, effect.percent() );
        }
      }

      if ( p->talent.subtlety.deeper_daggers->ok() )
      {
        if ( !affected_by.deeper_daggers.direct )
          affected_by.deeper_daggers.direct = true;
        if ( !affected_by.deeper_daggers.periodic )
          affected_by.deeper_daggers.periodic = true;
      }
    }
  }

  void init() override
  {
    ab::init();
    
    if ( consumes_echoing_reprimand() )
    {
      animacharged_cp_proc = p()->get_proc( "Echoing Reprimand " + ab::name_str );
    }

    if ( p()->buffs.cold_blood->is_affecting( &ab::data() ) )
    {
      cold_blood_consumed_proc = p()->get_proc( "Cold Blood " + ab::name_str );
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

    register_damage_buff( p()->buffs.broadside );
    register_damage_buff( p()->buffs.grand_melee );
    register_damage_buff( p()->buffs.cold_blood );
    register_damage_buff( p()->buffs.danse_macabre );
    register_damage_buff( p()->buffs.finality_eviscerate );
    register_damage_buff( p()->buffs.finality_black_powder );
    register_damage_buff( p()->buffs.kingsbane );
    register_damage_buff( p()->buffs.master_assassin );
    register_damage_buff( p()->buffs.master_assassin_aura );
    register_damage_buff( p()->buffs.perforated_veins );
    register_damage_buff( p()->buffs.ruthless_precision );
    register_damage_buff( p()->buffs.shadow_dance );
    register_damage_buff( p()->buffs.summarily_dispatched );
    register_damage_buff( p()->buffs.symbols_of_death );
    register_damage_buff( p()->buffs.the_rotten );

    register_damage_buff( p()->buffs.t29_assassination_4pc );
    register_damage_buff( p()->buffs.t29_outlaw_2pc );
    register_damage_buff( p()->buffs.t29_outlaw_4pc );
    register_damage_buff( p()->buffs.t29_subtlety_2pc );
    register_damage_buff( p()->buffs.t29_subtlety_4pc );
    register_damage_buff( p()->buffs.t29_subtlety_4pc_black_powder );
    register_damage_buff( p()->buffs.t31_assassination_2pc );

    if ( ab::base_costs[ RESOURCE_COMBO_POINT ] > 0 )
    {
      affected_by.alacrity = true;
      affected_by.deepening_shadows = true;
      affected_by.flagellation = true;
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
    register_consume_buff( p()->buffs.cold_blood,
                           p()->buffs.cold_blood->is_affecting( &ab::data() ) && ab::data().id() != p()->talent.subtlety.secret_technique->id(),
                           cold_blood_consumed_proc );
    register_consume_buff( p()->buffs.perforated_veins, p()->buffs.perforated_veins->is_affecting( &ab::data() ),
                           perforated_veins_consumed_proc, 1_ms ); // TOCHECK -- Ensure this still affects WM procs like it used to
    register_consume_buff( p()->buffs.the_rotten, p()->buffs.the_rotten->is_affecting_crit_chance( &ab::data() ), nullptr, 1_ms, false, true );
    register_consume_buff( p()->buffs.goremaws_bite, affected_by.goremaws_bite );
    register_consume_buff( p()->buffs.t29_outlaw_2pc, p()->buffs.t29_outlaw_2pc->is_affecting( &ab::data() ) );
    register_consume_buff( p()->buffs.t29_outlaw_4pc, p()->buffs.t29_outlaw_4pc->is_affecting( &ab::data() ) );
    register_consume_buff( p()->buffs.t29_subtlety_2pc, p()->buffs.t29_subtlety_2pc->is_affecting( &ab::data() ) &&
                                                        secondary_trigger_type != secondary_trigger::SHURIKEN_TORNADO );
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

    // Apply and Snapshot Echoing Reprimand Buffs
    if ( p()->spell.echoing_reprimand->ok() && consumes_echoing_reprimand() )
    {
      if ( range::any_of( p()->buffs.echoing_reprimand, [ consume_cp ]( const buff_t* buff ) { return buff->check_value() == consume_cp; } ) )
        effective_cp = as<int>( p()->spell.echoing_reprimand->effectN( 2 ).base_value() );
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

  // Helper Functions =========================================================

  // Helper function for expressions. Returns the number of guaranteed generated combo points for
  // this ability, taking into account any potential buffs.
  virtual double generate_cp() const
  {
    double cp = 0;

    if ( ab::energize_type != action_energize::NONE && ab::energize_resource == RESOURCE_COMBO_POINT )
    {
      cp += ab::energize_amount;

      if ( affected_by.broadside_cp )
      {
        cp += p()->buffs.broadside->check_value();
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

  // Generic rules for proccing Shadow Blades, used by rogue_t::trigger_shadow_blades_attack()
  virtual bool procs_shadow_blades_damage() const
  { return true; }

  // Generic rules for proccing Caustic Spatter, used by rogue_t::trigger_caustic_spatter()
  virtual bool procs_caustic_spatter() const
  { return ab::get_school() == SCHOOL_NATURE; }

  // Generic rules for proccing Seal Fate, used by rogue_t::trigger_seal_fate()
  virtual bool procs_seal_fate() const
  { return ab::energize_type != action_energize::NONE && ab::energize_resource == RESOURCE_COMBO_POINT && ab::energize_amount > 0; }

  // Generic rules for snapshotting the Nightstalker pmultiplier, default to false as this is a custom script.
  virtual bool snapshots_nightstalker() const
  { return false; }

  // Overridable wrapper for checking stealth requirement
  virtual bool requires_stealth() const
  {
    if ( affected_by.audacity && p()->buffs.audacity->check() )
      return false;

    if ( affected_by.blindside && p()->buffs.blindside->check() )
      return false;

    if ( affected_by.sepsis && p()->buffs.sepsis->check() )
      return false;

    return _requires_stealth;
  }

  // Overridable wrapper for checking stealth breaking
  virtual bool breaks_stealth() const
  { return _breaks_stealth; }

  // Overridable function to determine whether a finisher is working with Echoing Reprimand.
  virtual bool consumes_echoing_reprimand() const
  { return ab::base_costs[ RESOURCE_COMBO_POINT ] > 0 && ( ab::attack_power_mod.direct > 0.0 || ab::attack_power_mod.tick > 0.0 ); }

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
  void trigger_vicious_venoms( const action_state_t* state, rogue_attack_t* action );
  void trigger_blade_flurry( const action_state_t* );
  void trigger_ruthlessness_cp( const action_state_t* );
  void trigger_combo_point_gain( int, gain_t* gain = nullptr );
  void trigger_alacrity( const action_state_t* );
  void trigger_deepening_shadows( const action_state_t* );
  void trigger_shadow_techniques( const action_state_t* );
  void trigger_weaponmaster( const action_state_t*, rogue_attack_t* action );
  void trigger_opportunity( const action_state_t*, rogue_attack_t* action, double modifier = 1.0 );
  void trigger_restless_blades( const action_state_t* );
  void trigger_relentless_strikes( const action_state_t* );
  void trigger_venom_rush( const action_state_t* );
  void trigger_blindside( const action_state_t* );
  void trigger_shadow_blades_attack( const action_state_t* );
  void trigger_sting_like_a_bee( const action_state_t* state );
  void trigger_find_weakness( const action_state_t* state, timespan_t duration = timespan_t::min() );
  void trigger_master_of_shadows();
  void trigger_dashing_scoundrel( const action_state_t* state );
  void trigger_count_the_odds( const action_state_t* state, proc_t* source_proc );
  void trigger_keep_it_rolling();
  void trigger_flagellation( const action_state_t* state );
  void trigger_perforated_veins( const action_state_t* state );
  void trigger_inevitability( const action_state_t* state );
  void trigger_lingering_shadow( const action_state_t* state );
  void trigger_danse_macabre( const action_state_t* state );
  void trigger_sanguine_blades( const action_state_t* state, rogue_attack_t* action );
  void trigger_caustic_spatter( const action_state_t* state );
  void trigger_caustic_spatter_debuff( const action_state_t* state );
  void trigger_shadowcraft( const action_state_t* state );
  virtual bool trigger_t31_subtlety_set_bonus( const action_state_t* state, rogue_attack_t* action = nullptr );

  // General Methods ==========================================================

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    if ( secondary_trigger_type != secondary_trigger::NONE )
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

    // Registered Damage Buffs
    for ( auto damage_buff : direct_damage_buffs )
      m *= damage_buff->is_stacking ? damage_buff->stack_value_direct() : damage_buff->value_direct();
    
    // Mastery
    if ( affected_by.mastery_executioner.direct || affected_by.mastery_potent_assassin.direct )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    // Nightstalker
    if ( p()->talent.rogue.nightstalker->ok() && p()->stealthed( STEALTH_BASIC | STEALTH_ROGUE ) && 
         p()->buffs.nightstalker->is_affecting_direct( ab::s_data ) )
    {
      m *= p()->buffs.nightstalker->direct_mod.multiplier;
    }

    // Deeper Daggers
    if ( affected_by.deeper_daggers.direct )
    {
      m *= 1.0 + p()->buffs.deeper_daggers->stack_value();
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

    // Set Bonuses
    if ( affected_by.t29_assassination_2pc && p()->buffs.envenom->check() )
    {
      m *= 1.0 + p()->set_bonuses.t29_assassination_2pc->effectN( 1 ).percent();
    }

    if ( affected_by.t30_assassination_4pc.direct && p()->buffs.t30_assassination_4pc->up() )
    {
      m *= 1.0 + affected_by.t30_assassination_4pc.direct_percent;
    }

    return m;
  }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = ab::composite_ta_multiplier( state );

    // Registered Damage Buffs
    for ( auto damage_buff : periodic_damage_buffs )
      m *= damage_buff->is_stacking ? damage_buff->stack_value_periodic() : damage_buff->value_periodic();

    // Nightstalker -- Periodic mod when Stealth is active is different from the snapshot effect
    if ( p()->talent.rogue.nightstalker->ok() && p()->stealthed( STEALTH_BASIC | STEALTH_ROGUE ) &&
         p()->buffs.nightstalker->is_affecting_periodic( ab::s_data ) )
    {
      m *= p()->buffs.nightstalker->periodic_mod.multiplier;
    }

    // Deeper Daggers
    if ( affected_by.deeper_daggers.periodic )
    {
      m *= 1.0 + p()->buffs.deeper_daggers->stack_value();
    }

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

    // Set Bonuses
    if ( affected_by.t30_assassination_4pc.periodic && p()->buffs.t30_assassination_4pc->up() )
    {
      m *= 1.0 + affected_by.t30_assassination_4pc.periodic_percent;
    }

    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = ab::composite_target_multiplier( target );

    auto tdata = td( target );

    if ( affected_by.improved_shiv )
    {
      m *= tdata->debuffs.shiv->value_direct();
    }

    if ( affected_by.maim_mangle && tdata->dots.garrote->is_ticking() )
    {
      m *= 1.0 + p()->talent.assassination.systemic_failure->effectN( 1 ).percent();
    }

    if ( affected_by.deathmark )
    {
      m *= tdata->debuffs.deathmark->value_direct();
    }

    if ( affected_by.ghostly_strike )
    {
      m *= 1.0 + tdata->debuffs.ghostly_strike->stack_value();
    }

    return m;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = ab::composite_persistent_multiplier( state );

    // 2023-05-13 -- Nightstalker snapshots for specific DoT spells out of Stealth
    if ( p()->talent.rogue.nightstalker->ok() && snapshots_nightstalker() &&
         p()->stealthed( STEALTH_BASIC | STEALTH_ROGUE ) )
    {
      m *= p()->buffs.nightstalker->periodic_mod.multiplier;
    }

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = ab::composite_crit_chance();

    // Registered Damage Buffs
    for ( auto crit_chance_buff : crit_chance_buffs )
      c += crit_chance_buff->stack_value_crit_chance();

    if ( affected_by.dashing_scoundrel && p()->buffs.envenom->check() )
    {
      c += p()->spec.dashing_scoundrel->effectN( 1 ).percent();
    }

    return c;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double cm = ab::composite_crit_damage_bonus_multiplier();

    if ( affected_by.t30_subtlety_4pc && p()->buffs.symbols_of_death->check() )
    {
      cm += p()->spec.t30_subtlety_4pc_buff->effectN( 1 ).percent();
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

  double cost() const override
  {
    double c = ab::cost();

    if ( c <= 0 )
      return 0;

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

    if ( c <= 0 )
      c = 0;

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
        if ( p()->set_bonuses.t31_assassination_2pc->ok() )
        {
          p()->t31_assassination_2pc_accumulator += ab::last_resource_cost;
          const double threshold = p()->set_bonuses.t31_assassination_2pc->effectN( 3 ).base_value();
          int trigger_stacks = 0;
          while ( p()->t31_assassination_2pc_accumulator >= threshold )
          {
            p()->t31_assassination_2pc_accumulator -= threshold;
            trigger_stacks++;
          }
          if ( trigger_stacks > 0 )
          {
            p()->buffs.t31_assassination_2pc->trigger( trigger_stacks );
          }
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

        if ( affected_by.broadside_cp && p()->buffs.broadside->up() )
        {
          trigger_combo_point_gain( as<int>( p()->buffs.broadside->check_value() ), p()->gains.broadside );
        }

        if ( affected_by.improved_ambush )
        {
          trigger_combo_point_gain( as<int>( p()->talent.rogue.improved_ambush->effectN( 1 ).base_value() ), p()->gains.improved_ambush );
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
      trigger_alacrity( ab::execute_state );
      trigger_deepening_shadows( ab::execute_state );
      trigger_flagellation( ab::execute_state );
      trigger_shadowcraft( ab::execute_state );
    }

    // 2020-12-04 -- Hotfix notes this is no longer consumed "while under the effects Stealth, Vanish, Subterfuge, Shadow Dance, and Shadowmeld"
    // 2023-05-13 -- Fixed an issue that caused Sepsis' buff that enables use of a Stealth skill to be cancelled by Ambush when a Blindside proc was available.
    if ( affected_by.sepsis && !ab::background && p()->buffs.sepsis->check() &&
         !p()->stealthed( STEALTH_STANCE & ~STEALTH_SEPSIS ) &&
         !( affected_by.blindside && p()->buffs.blindside->check() ) )
    {
      // Expiry delayed by 1ms in order to allow Deathmark to benefit from the Improved Garrote effect
      p()->buffs.sepsis->expire( 1_ms );
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
    if ( p()->sim->log )
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
    trigger_shadow_blades_attack( state );
    trigger_dashing_scoundrel( state );
    trigger_caustic_spatter( state );
    trigger_t31_subtlety_set_bonus( state );
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    trigger_shadow_blades_attack( d->state );
    trigger_dashing_scoundrel( d->state );
    trigger_caustic_spatter( d->state );
  }
};

// ==========================================================================
// Poisons
// ==========================================================================

struct rogue_poison_t : public rogue_attack_t
{
  bool is_lethal;
  double base_proc_chance;
  rogue_attack_t* deathmark_impact_action;

  rogue_poison_t( util::string_view name, rogue_t* p, const spell_data_t* s,
                  bool is_lethal = false, bool triggers_procs = false ) :
    actions::rogue_attack_t( name, p, s ),
    is_lethal( is_lethal ),
    base_proc_chance( 0.0 ),
    deathmark_impact_action( nullptr )
  {
    background = dual = true;
    proc = !triggers_procs;
    callbacks = triggers_procs;

    trigger_gcd = timespan_t::zero();

    base_proc_chance = data().proc_chance();
    if ( s->affected_by( p->talent.assassination.improved_poisons->effectN( 1 ) ) )
    {
      base_proc_chance += p->talent.assassination.improved_poisons->effectN( 1 ).percent();
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

    double chance = base_proc_chance;
    chance += p()->buffs.envenom->stack_value();

    // Applies as a percent modifier, not a flat modifier
    if ( affected_by.dragon_tempered_blades )
    {
      chance *= 1.0 + p()->talent.assassination.dragon_tempered_blades->effectN( 2 ).percent();
    }

    // Applied after Dragon-Tempered Blades' modifer for Thrown Precision and Poisoned Knife
    chance += rogue_t::cast_attack( source_state->action )->composite_poison_flat_modifier( source_state );

    return chance;
  }

  virtual void trigger( const action_state_t* source_state )
  {
    bool result = rng().roll( proc_chance( source_state ) );

    sim->print_debug( "{} attempts to proc {}, target={} source={} proc_chance={}: {}", *player, *this,
                      *source_state->target, *source_state->action, proc_chance( source_state ), result );

    if ( !result )
      return;

    set_target( source_state->target );
    execute();
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( is_lethal && state->result_amount > 0 )
    {
      if ( td( state->target )->dots.kingsbane->is_ticking() )
        p()->buffs.kingsbane->trigger();

      if ( p()->set_bonuses.t29_assassination_4pc->ok() )
        p()->buffs.t29_assassination_4pc->trigger();
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
        sim->print_log( "{} applies lethal poison {}", *p, *p->active.lethal_poison );
      }

      if ( !p->active.nonlethal_poison )
      {
        if ( nonlethal_str.empty() )
        {
          nonlethal_str = get_default_nonlethal_poison_dtb( p );;
        }

        p->active.nonlethal_poison = get_poison( p, nonlethal_str );
        sim->print_log( "{} applies non-lethal poison {}", *p, *p->active.nonlethal_poison );
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
          sim->print_log( "{} applies second lethal poison {}", *p, *p->active.lethal_poison_dtb );

          if ( nonlethal_dtb_str.empty() || get_poison( p, nonlethal_dtb_str ) == p->active.nonlethal_poison )
          {
            nonlethal_dtb_str = get_default_nonlethal_poison_dtb( p );
          }

          p->active.nonlethal_poison_dtb = get_poison( p, nonlethal_dtb_str );
          sim->print_log( "{} applies second non-lethal poison {}", *p, *p->active.nonlethal_poison_dtb );
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

  melee_t( const char* name, rogue_t* p, int sw ) :
    rogue_attack_t( name, p ), sync_weapons( sw ), first( true ),
    canceled( false ), prev_scheduled_time( timespan_t::zero() )
  {
    background = repeating = may_glance = may_crit = true;
    allow_class_ability_procs = not_a_proc = true;
    special = false;
    school = SCHOOL_PHYSICAL;
    trigger_gcd = timespan_t::zero();
    weapon_multiplier = 1.0;

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
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = rogue_attack_t::composite_crit_chance();

    if ( p()->talent.assassination.master_assassin->ok() )
    {
      c += p()->buffs.master_assassin->stack_value();
      c += p()->buffs.master_assassin_aura->stack_value();
    }

    return c;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    // Registered Damage Buffs
    for ( auto damage_buff : auto_attack_damage_buffs )
      m *= damage_buff->is_stacking ? damage_buff->stack_value_auto_attack() : damage_buff->value_auto_attack();

    // Class Passives
    m *= 1.0 + p()->spec.outlaw_rogue->effectN( 18 ).percent();
    m *= 1.0 + p()->spec.assassination_rogue->effectN( 22 ).percent();
    m *= 1.0 + p()->spec.assassination_rogue->effectN( 23 ).percent();

    return m;
  }

  bool procs_poison() const override
  { return true; }

  bool procs_main_gauche() const override
  { return weapon->slot == SLOT_MAIN_HAND; }

  bool procs_blade_flurry() const override
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

    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    parse_options( options_str );

    if ( p -> main_hand_weapon.type == WEAPON_NONE )
    {
      background = true;
      return;
    }

    p->melee_main_hand = debug_cast<melee_t*>( p->find_action( "auto_attack_mh" ) );
    if ( !p->melee_main_hand )
      p->melee_main_hand = new melee_t( "auto_attack_mh", p, sync_weapons );

    p->main_hand_attack = p->melee_main_hand;
    p->main_hand_attack->weapon = &( p->main_hand_weapon );
    p->main_hand_attack->base_execute_time = p->main_hand_weapon.swing_time;

    if ( p->off_hand_weapon.type != WEAPON_NONE )
    {
      p->melee_off_hand = debug_cast<melee_t*>( p->find_action( "auto_attack_oh" ) );
      if ( !p->melee_off_hand )
        p->melee_off_hand = new melee_t( "auto_attack_oh", p, sync_weapons );

      p->off_hand_attack = p->melee_off_hand;
      p->off_hand_attack->weapon = &( p->off_hand_weapon );
      p->off_hand_attack->base_execute_time = p->off_hand_weapon.swing_time;
      p->off_hand_attack->id = 1;
    }
  }

  void execute() override
  {
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

    void execute() override
    {
      rogue_attack_t::execute();
      trigger_count_the_odds( execute_state, p()->procs.count_the_odds_ambush );
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
    if ( p->talent.assassination.vicious_venoms->ok() )
    {
      add_child( p->active.vicious_venoms.ambush );
    }
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
    trigger_count_the_odds( execute_state, p()->procs.count_the_odds_ambush );
    trigger_blindside( execute_state );
    trigger_venom_rush( execute_state );
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( p()->talent.outlaw.hidden_opportunity->ok() )
    {
      // 2023-11-24 -- The Audacity replacement spell currently triggers the normal Ambush spell from HO
      //               This allows it to trigger up to twice, rather than just a single time
      if ( p()->bugs && audacity_extra_attack && p()->buffs.audacity->check() )
        trigger_opportunity( state, audacity_extra_attack, p()->talent.outlaw.hidden_opportunity->effectN( 1 ).percent() );
      else
        trigger_opportunity( state, extra_attack, p()->talent.outlaw.hidden_opportunity->effectN( 1 ).percent() );
    }

    trigger_vicious_venoms( state, p()->active.vicious_venoms.ambush );
    trigger_caustic_spatter_debuff( state );
  }

  bool procs_main_gauche() const override
  { return true; }

  bool procs_blade_flurry() const override
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
    trigger_weaponmaster( state, p()->active.weaponmaster.backstab );

    if ( state->result == RESULT_CRIT && p()->talent.subtlety.improved_backstab->ok() && p()->position() == POSITION_BACK )
    {
      timespan_t duration = timespan_t::from_seconds( p()->talent.subtlety.improved_backstab->effectN( 1 ).base_value() );
      trigger_find_weakness( state, duration );
    }

    trigger_lingering_shadow( state );
    trigger_inevitability( state );

    if ( state->result == RESULT_CRIT && p()->set_bonuses.t29_subtlety_4pc->ok() )
    {
      p()->buffs.t29_subtlety_4pc->trigger();
      p()->buffs.t29_subtlety_4pc_black_powder->trigger();
    }
  }

  bool verify_actor_spec() const override
  {
    if ( p()->talent.subtlety.gloomblade->ok() )
      return false;

    return rogue_attack_t::verify_actor_spec();
  }
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

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( secondary_trigger_type == secondary_trigger::CRACKSHOT )
    {
      m *= p()->talent.outlaw.crackshot->effectN( 1 ).percent();
    }

    return m;
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

    if ( p()->set_bonuses.t29_outlaw_2pc->ok() )
    {
      p()->buffs.t29_outlaw_2pc->expire();
      p()->buffs.t29_outlaw_2pc->trigger( cast_state( execute_state )->get_combo_points() );
    }

    if ( !is_secondary_action() )
    {
      trigger_restless_blades( execute_state );
    }
    trigger_count_the_odds( execute_state, p()->procs.count_the_odds_dispatch );
  }

  bool procs_main_gauche() const override
  { return true; }

  bool procs_blade_flurry() const override
  { return true; }
};

// Between the Eyes =========================================================

struct between_the_eyes_t : public rogue_attack_t
{
  struct soulreave_t : public rogue_attack_t
  {
    soulreave_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.t30_outlaw_4pc_attack )
    {
      ignores_armor = true; // Not currently in spell data
    }

    bool procs_poison() const override
    { return false; }
  };

  soulreave_t* soulreave_attack;
  dispatch_t* dispatch;

  between_the_eyes_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spec.between_the_eyes, options_str ),
    soulreave_attack( nullptr ), dispatch( nullptr )
  {
    ap_type = attack_power_type::WEAPON_BOTH;

    if ( p->talent.outlaw.crackshot->ok() )
    {
      dispatch = p->get_secondary_trigger_action<dispatch_t>( secondary_trigger::CRACKSHOT, "dispatch_crackshot" );
      dispatch->not_a_proc = true; // Scripted foreground cast, can trigger cast procs
      add_child( dispatch );
    }
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( p()->set_bonuses.t30_outlaw_4pc->ok() )
    {
      soulreave_attack = p()->get_background_action<soulreave_t>( "soulreave" );
      action_t* soulrip = p()->find_action( "soulrip" );
      if ( soulrip )
      {
        soulrip->add_child( soulreave_attack );
      }
    }
  }

  double composite_crit_chance() const override
  {
    double c = rogue_attack_t::composite_crit_chance();

    if ( p()->buffs.ruthless_precision->check() )
    {
      c += p()->buffs.ruthless_precision->data().effectN( 2 ).percent();
    }

    return c;
  }

  void execute() override
  {
    // Soulreave trigger and consume happens prior to the impact of the triggering BtE cast
    // Each Soulreave is triggered independently based on the Soulrip on the target
    if ( p()->set_bonuses.t30_outlaw_4pc->ok() )
    {
      for ( auto t : sim->target_non_sleeping_list )
      {
        rogue_td_t* tdata = p()->get_target_data( t );
        if ( tdata->dots.soulrip->is_ticking() )
        {
          auto dot_state = debug_cast<residual_action::residual_periodic_state_t*>( tdata->dots.soulrip->state );
          double amount = dot_state->tick_amount * tdata->dots.soulrip->ticks_left();
          amount *= p()->set_bonuses.t30_outlaw_4pc->effectN( 1 ).percent();
          soulreave_attack->execute_on_target( t, amount );
          tdata->dots.soulrip->cancel();
        }
      }

      p()->buffs.t30_outlaw_4pc->trigger();
    }

    rogue_attack_t::execute();

    trigger_restless_blades( execute_state );

    if ( result_is_hit( execute_state->result ) )
    {
      const auto rs = cast_state( execute_state );
      const int cp_spend = rs->get_combo_points();

      // There is nothing about the buff duration in spell data, so we have to hardcode the 3s base.
      p()->buffs.between_the_eyes->trigger( 3_s * cp_spend );

      if ( p()->spec.greenskins_wickers->ok() &&
           rng().roll( p()->spec.greenskins_wickers->effectN( 1 ).percent() * cp_spend ) )
      {
        p()->buffs.greenskins_wickers->trigger();
      }

      if ( p()->talent.outlaw.ace_up_your_sleeve->ok() )
      {
        if ( rng().roll( p()->talent.outlaw.ace_up_your_sleeve->effectN( 1 ).percent() * cp_spend ) )
        {
          trigger_combo_point_gain( as<int>( p()->spec.ace_up_your_sleeve_energize->effectN( 1 ).base_value() ),
                                    p()->gains.ace_up_your_sleeve );
        }
      }

      if ( p()->talent.outlaw.crackshot->ok() && p()->stealthed( STEALTH_BASIC | STEALTH_ROGUE | STEALTH_SEPSIS ) )
      {
        p()->cooldowns.between_the_eyes->reset( false );
        dispatch->trigger_secondary_action( execute_state->target, cp_spend );
      }
    }
  }

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
  }

  void init() override
  {
    rogue_attack_t::init();
    snapshot_flags |= STATE_TGT_MUL_DA;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    rogue_attack_t::available_targets( tl );

    // Cannot hit the original target.
    range::erase_remove( tl, target );

    return tl.size();
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
        affected_by.broadside_cp = true; // 2023-10-25 -- Not in the whitelist but confirmed as working in-game
      }
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

    timespan_t d = p()->buffs.blade_flurry->data().duration();
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

// Cold Blood ===============================================================

struct cold_blood_t : public rogue_spell_t
{
  double precombat_seconds;

  cold_blood_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->talent.rogue.cold_blood ),
    precombat_seconds( 0.0 )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = false;
    set_target( p );
  }

  void execute() override
  {
    rogue_spell_t::execute();

    if ( precombat_seconds && !p()->in_combat )
    {
      p()->cooldowns.cold_blood->adjust( -timespan_t::from_seconds( precombat_seconds ), false );
    }

    p()->buffs.cold_blood->trigger();
  }
};

// Crimson Tempest ==========================================================

struct crimson_tempest_t : public rogue_attack_t
{
  struct poisoned_edges_t : public rogue_attack_t
  {
    poisoned_edges_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.t30_assassination_2pc_tick )
    {
      base_dd_min = base_dd_max = 1; // Override from 0 for snapshot_flags
    }

    bool procs_poison() const override
    { return false; }

    bool procs_caustic_spatter() const override
    { return false; }
  };

  poisoned_edges_t* poisoned_edges_damage;

  crimson_tempest_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.assassination.crimson_tempest, options_str ),
    poisoned_edges_damage( nullptr )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();

    if ( p->set_bonuses.t30_assassination_2pc->ok() )
    {
      poisoned_edges_damage = p->get_background_action<poisoned_edges_t>( "crimson_tempest_t30" );
      add_child( poisoned_edges_damage );
    }
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( p()->talent.assassination.sanguine_blades->ok() )
    {
      add_child( p()->active.sanguine_blades.crimson_tempest );
    }
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

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_ta_multiplier( state );

    // Deals bonus damage per target with the DoT ticking, up to 5 targets
    auto num_targets = std::min( p()->get_active_dots( td( state->target )->dots.crimson_tempest ),
                                 as<unsigned>( reduced_aoe_targets ) );
    if ( num_targets > 1 )
    {
      m *= 1.0 + ( num_targets * data().effectN( 4 ).percent() );
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( poisoned_edges_damage )
    {
      double multiplier = p()->set_bonuses.t30_assassination_2pc->effectN( 2 ).percent();
      double damage = state->result_total * multiplier;
      poisoned_edges_damage->execute_on_target( state->target, damage );
    }
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    trigger_sanguine_blades( d->state, p()->active.sanguine_blades.crimson_tempest );

    if ( poisoned_edges_damage )
    {
      double multiplier = p()->set_bonuses.t30_assassination_2pc->effectN( 2 ).percent();
      double damage = d->state->result_total * multiplier;
      poisoned_edges_damage->execute_on_target( d->target, damage );
    }
  }

  bool procs_poison() const override
  { return true; }

  bool snapshots_nightstalker() const override
  { return true; }
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

    p()->buffs.t30_assassination_4pc->trigger();
  }
};

// Detection ================================================================

// This ability does nothing but for some odd reasons throughout the history of Rogue spaghetti, we may want to look at using it. So, let's support it.
struct detection_t : public rogue_spell_t
{
  detection_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->spell.detection, options_str )
  {
    gcd_type = gcd_haste_type::ATTACK_HASTE;
    min_gcd = 750_ms; // Force 750ms min gcd because rogue player base has a 1s min.
    harmful = false;
    set_target( p );
  }
};

// Envenom ==================================================================

struct envenom_t : public rogue_attack_t
{
  struct envenomous_explosion_t : public rogue_attack_t
  {
    envenomous_explosion_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.t31_assassination_4pc_attack )
    {
      aoe = -1;
      affected_by.lethal_dose = false;
      affected_by.zoldyck_insignia = false;
      reduced_aoe_targets = p->set_bonuses.t31_assassination_4pc->effectN( 2 ).base_value();
    }

    double action_multiplier() const override
    {
      double m = rogue_attack_t::action_multiplier();

      if ( target_list().size() == 1 )
      {
        m *= 1.0 + p()->set_bonuses.t31_assassination_4pc->effectN( 3 ).percent();
      }

      return m;
    }

    bool procs_poison() const override
    { return false; }

    bool procs_caustic_spatter() const override
    { return true; } // TOCHECK
  };

  envenomous_explosion_t* envenomous_explosion;

  envenom_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spec.envenom, options_str ),
    envenomous_explosion( nullptr )
  {
    dot_duration = timespan_t::zero();
    affected_by.lethal_dose = false;

    if ( p->set_bonuses.t31_assassination_4pc->ok() )
    {
      envenomous_explosion = p->get_background_action<envenomous_explosion_t>( "envenomous_explosion" );
      add_child( envenomous_explosion );
    }

    if ( p->active.poison_bomb )
    {
      add_child( p->active.poison_bomb );
    }
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_multiplier( target );

    if ( p()->talent.assassination.amplifying_poison->ok() )
    {
      const int consume_stacks = as<int>( p()->talent.assassination.amplifying_poison->effectN( 2 ).base_value() );
      rogue_td_t* tdata = td( target );

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

  void execute() override
  {
    rogue_attack_t::execute();
    trigger_poison_bomb( execute_state );

    // TOCHECK -- If this consumes on execute or impact when parried
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

  void impact( action_state_t* state ) override
  {
    // T31 checks before applying Envenom, otherwise this would always succeed
    if ( envenomous_explosion && p()->buffs.envenom->check() )
    {
      envenomous_explosion->execute_on_target( state->target );
    }

    // Trigger Envenom buff before impact() so that poison procs from Envenom itself benefit
    // 2023-10-05 -- Envenom spell no longer has a base 1s duration, hard code for now
    timespan_t envenom_duration = ( 1_s * cast_state( state )->get_combo_points() ) +
                                  p()->talent.assassination.twist_the_knife->effectN( 1 ).time_value();
    p()->buffs.envenom->trigger( envenom_duration );

    rogue_attack_t::impact( state );

    if ( p()->talent.assassination.cut_to_the_chase->ok() && p()->buffs.slice_and_dice->check() )
    {
      double extend_duration = p()->talent.assassination.cut_to_the_chase->effectN( 1 ).base_value() * cast_state( state )->get_combo_points();
      // Max duration it extends to is the maximum possible full pandemic duration, i.e. around 46s without and 54s with Deeper Stratagem.
      timespan_t max_duration = ( p()->consume_cp_max() + 1 ) * p()->buffs.slice_and_dice->data().duration() * 1.3;
      timespan_t effective_extend = std::min( timespan_t::from_seconds( extend_duration ), max_duration - p()->buffs.slice_and_dice->remains() );
      p()->buffs.slice_and_dice->extend_duration( p(), effective_extend );
    }
  }
};

// Eviscerate ===============================================================

struct eviscerate_t : public rogue_attack_t
{
  struct eviscerate_bonus_t : public rogue_attack_t
  {
    int last_eviscerate_cp;

    eviscerate_bonus_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.eviscerate_shadow_attack ),
      last_eviscerate_cp( 1 )
    {
      if ( p->talent.subtlety.shadowed_finishers->ok() )
      {
        // Spell has the full damage coefficient and is modified via talent scripting
        base_multiplier *= p->talent.subtlety.shadowed_finishers->effectN( 1 ).percent();
      }
    }

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

  struct shadow_eviscerate_t : public rogue_attack_t
  {
    shadow_eviscerate_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.t31_subtlety_2pc_eviscerate )
    {
    }

    void trigger_clone_attack( const action_state_t* state )
    {
      // Damage is residual at the time of cast, benefitting from all three Shadow talents
      double damage_amount = state->result_amount * p()->set_bonuses.t31_subtlety_2pc->effectN( 1 ).percent();
      damage_amount *= 1.0 + p()->set_bonuses.t31_subtlety_4pc->effectN( 1 ).percent();
      damage_amount *= 1.0 + p()->talent.subtlety.dark_brew->effectN( 4 ).percent();
      damage_amount *= 1.0 + p()->talent.subtlety.veiltouched->effectN( 3 ).percent();
      damage_amount *= 1.0 + p()->buffs.deeper_daggers->stack_value();

      base_dd_min = base_dd_max = damage_amount;
      set_target( state->target );
      auto damage_state = get_state();
      damage_state->target = state->target;
      snapshot_state( damage_state, result_amount_type::DMG_DIRECT );
      trigger_secondary_action( damage_state, 1_s );
    }

    bool procs_poison() const override
    { return false; }
  };

  eviscerate_bonus_t* bonus_attack;
  shadow_eviscerate_t* shadow_eviscerate_attack;

  eviscerate_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ):
    rogue_attack_t( name, p, p->spec.eviscerate, options_str ),
    bonus_attack( nullptr ), shadow_eviscerate_attack( nullptr )
  {
    affected_by.t31_subtlety_4pc = true;

    if ( p->talent.subtlety.shadowed_finishers->ok() )
    {
      bonus_attack = p->get_background_action<eviscerate_bonus_t>( "eviscerate_bonus" );
      add_child( bonus_attack );
    }

    if ( p->set_bonuses.t31_subtlety_2pc->ok() )
    {
      shadow_eviscerate_attack = p->get_secondary_trigger_action<shadow_eviscerate_t>(
        secondary_trigger::T31_SUBTLETY_2PC, "shadow_eviscerate" );
      add_child( shadow_eviscerate_attack );
    }
  }

  void execute() override
  {
    p()->buffs.deeper_daggers->trigger();

    rogue_attack_t::execute();

    if ( p()->spec.finality_eviscerate_buff->ok() )
    {
      if ( p()->buffs.finality_eviscerate->check() )
        p()->buffs.finality_eviscerate->expire();
      else
        p()->buffs.finality_eviscerate->trigger();
    }

    if ( p()->set_bonuses.t29_subtlety_2pc->ok() )
    {
      p()->buffs.t29_subtlety_2pc->expire();
      p()->buffs.t29_subtlety_2pc->trigger( cast_state( execute_state )->get_combo_points() );
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( bonus_attack && td( state->target )->debuffs.find_weakness->up() && result_is_hit( state->result ) )
    {
      bonus_attack->last_eviscerate_cp = cast_state( state )->get_combo_points();
      bonus_attack->execute_on_target( state->target );
    }
  }

  bool trigger_t31_subtlety_set_bonus( const action_state_t* state, rogue_attack_t* ) override
  {
    if ( shadow_eviscerate_attack && rogue_attack_t::trigger_t31_subtlety_set_bonus( state, shadow_eviscerate_attack ) )
    {
      shadow_eviscerate_attack->trigger_clone_attack( state );
      return true;
    }

    return false;
  }
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
    if( p()->talent.assassination.thrown_precision->ok() && state->result == RESULT_CRIT )
      return 1.0;

    return rogue_attack_t::composite_poison_flat_modifier( state );
  }

  bool procs_poison() const override
  { return true; }
};

// Feint ====================================================================

struct feint_t : public rogue_attack_t
{
  feint_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ):
    rogue_attack_t( name, p, p->spell.feint, options_str )
  { }

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
    // 2023-01-28 -- Sepsis buff is consumed by Garrote when used for Improved Garrote
    affected_by.sepsis = p->talent.assassination.improved_garrote->ok();
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

    if ( p()->talent.assassination.sanguine_blades->ok() )
    {
      add_child( secondary_trigger_type == secondary_trigger::DEATHMARK ?
                 p()->active.sanguine_blades.deathmark_garrote :
                 p()->active.sanguine_blades.garrote );
    }
  }

  int n_targets() const override
  {
    int n = rogue_attack_t::n_targets();

    if ( !is_secondary_action() && ( p()->buffs.indiscriminate_carnage->check() ||
                                     p()->buffs.indiscriminate_carnage_aura->check() ) )
    {
      n = as<int>( p()->talent.assassination.indiscriminate_carnage->effectN( 1 ).base_value() );
    }

    return n;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    rogue_attack_t::available_targets( tl );

    // Indiscriminate Carnage smart-targets beyond the primary target, preferring lowest duration
    if ( tl.size() > 1 && !is_secondary_action() && ( p()->buffs.indiscriminate_carnage->check() ||
                                                      p()->buffs.indiscriminate_carnage_aura->check() ) )
    {
      auto it = std::partition( tl.begin(), tl.end(), [ this ]( player_t* t ) {
        return t == this->target;
      } );
      std::sort( it, tl.end(), [ this ]( player_t* a, player_t* b ) {
        return td( a )->dots.garrote->remains() < td( b )->dots.garrote->remains();
      } );
    }

    return tl.size();
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
    trigger_sanguine_blades( d->state, secondary_trigger_type == secondary_trigger::DEATHMARK ?
                                       p()->active.sanguine_blades.deathmark_garrote :
                                       p()->active.sanguine_blades.garrote );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    // 2022-11-28 -- Currently does not work correctly at all without Improved Garrote
    //               Additionally works every global of Improved Garrote regardless of Subterfuge
    // 2023-01-28 -- However, this does not work with Improved Garrote triggered by Sepsis
    // 2023-11-14 -- Shrouded Suffocation is no longer triggered by Deathmark Garrotes
    if ( p()->talent.assassination.shrouded_suffocation->ok() && !is_secondary_action() &&
         ( !p()->bugs || p()->stealthed( STEALTH_IMPROVED_GARROTE ) ) &&
         ( p()->stealthed( STEALTH_BASIC | STEALTH_ROGUE ) ||
           ( p()->bugs && p()->stealthed( STEALTH_IMPROVED_GARROTE ) && !p()->buffs.sepsis->check() ) ) )
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
    else if ( p()->bugs && p()->buffs.sepsis->check() )
    {
      // 2023-01-28 -- Sepsis buff causes Garrote to have no cooldown regardless of Improved Garrote
      cd_duration = timespan_t::zero();
    }

    rogue_attack_t::update_ready( cd_duration );
  }

  bool snapshots_nightstalker() const override
  { return true; }
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

// Ghostly Strike ===========================================================

struct ghostly_strike_t : public rogue_attack_t
{
  ghostly_strike_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.outlaw.ghostly_strike, options_str )
  {
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( result_is_hit( state->result ) )
    {
      td( state->target )->debuffs.ghostly_strike->trigger();
    }
  }

  bool procs_main_gauche() const override
  { return true; }

  bool procs_blade_flurry() const override
  { return true; }
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
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    trigger_perforated_veins( state );
    trigger_weaponmaster( state, p()->active.weaponmaster.gloomblade );

    if ( state->result == RESULT_CRIT && p()->talent.subtlety.improved_backstab->ok() )
    {
      timespan_t duration = timespan_t::from_seconds( p()->talent.subtlety.improved_backstab->effectN( 1 ).base_value() );
      trigger_find_weakness( state, duration );
    }

    trigger_lingering_shadow( state );
    trigger_inevitability( state );

    if ( state->result == RESULT_CRIT && p()->set_bonuses.t29_subtlety_4pc->ok() )
    {
      p()->buffs.t29_subtlety_4pc->trigger();
      p()->buffs.t29_subtlety_4pc_black_powder->trigger();
    }
  }
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
    direct_tick = true;
  }

  bool procs_main_gauche() const override
  { return weapon->slot == SLOT_MAIN_HAND; }

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
    interrupt_auto_attack = false;

    attack_mh = p->get_background_action<killing_spree_tick_t>( "killing_spree_mh", p->spec.killing_spree_mh_attack );
    attack_oh = p->get_background_action<killing_spree_tick_t>( "killing_spree_oh", p->spec.killing_spree_oh_attack );
    add_child( attack_mh );
    add_child( attack_oh );

    snapshot_flags |= STATE_HASTE;
  }

  timespan_t tick_time( const action_state_t* s ) const override
  { return data().effectN( 1 ).period() * s->haste; }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return tick_time( s ) * cast_state( s )->get_combo_points();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    trigger_restless_blades( execute_state );
    p()->buffs.killing_spree->trigger( composite_dot_duration( execute_state ) );
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

// Kingsbane ================================================================

struct kingsbane_t : public rogue_attack_t
{
  kingsbane_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.assassination.kingsbane, options_str )
  {
  }

  void last_tick( dot_t* d ) override
  {
    rogue_attack_t::last_tick( d );
    p()->buffs.kingsbane->expire();
  }

  bool snapshots_nightstalker() const override
  { return true; }
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

    m *= 1.0 + p()->buffs.opportunity->value();
    m *= 1.0 + p()->buffs.greenskins_wickers->value();

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

      if ( p()->set_bonuses.t29_outlaw_4pc->ok() )
      {
        p()->buffs.t29_outlaw_4pc->trigger();
      }
    }

    p()->buffs.opportunity->decrement();
    p()->buffs.greenskins_wickers->expire();

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
    rogue_attack_t* vicious_venoms_attack;

    mutilate_strike_t( util::string_view name, rogue_t* p, const spell_data_t* s, rogue_attack_t* vicious_venoms_attack ) :
      rogue_attack_t( name, p, s ), vicious_venoms_attack( vicious_venoms_attack )
    {
    }

    void impact( action_state_t* state ) override
    {
      rogue_attack_t::impact( state );
      trigger_doomblade( state );
      trigger_vicious_venoms( state, vicious_venoms_attack );
    }

    bool procs_seal_fate() const override
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

    mh_strike = p->get_background_action<mutilate_strike_t>( "mutilate_mh", data().effectN( 3 ).trigger(),
                                                             p->active.vicious_venoms.mutilate_mh );
    oh_strike = p->get_background_action<mutilate_strike_t>( "mutilate_oh", data().effectN( 4 ).trigger(),
                                                             p->active.vicious_venoms.mutilate_oh );
    add_child( mh_strike );
    add_child( oh_strike );

    if ( p->talent.assassination.vicious_venoms->ok() )
    {
      add_child( p->active.vicious_venoms.mutilate_mh );
      add_child( p->active.vicious_venoms.mutilate_oh );
    }

    if ( p->active.doomblade )
    {
      add_child( p->active.doomblade );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      mh_strike->execute_on_target( execute_state->target );
      oh_strike->execute_on_target( execute_state->target );

      trigger_blindside( execute_state );
      trigger_venom_rush( execute_state );
      trigger_caustic_spatter_debuff( execute_state );
    }
  }

  bool has_amount_result() const override
  { return true; }
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

  struct poisoned_edges_t : public rogue_attack_t
  {
    poisoned_edges_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.t30_assassination_2pc_tick )
    {
      base_dd_min = base_dd_max = 1; // Override from 0 for snapshot_flags
    }

    bool procs_poison() const override
    { return false; }

    bool procs_caustic_spatter() const override
    { return false; }
  };

  struct shadow_rupture_t : public rogue_attack_t
  {
    shadow_rupture_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.t31_subtlety_2pc_rupture )
    {
    }

    void trigger_clone_attack( const action_state_t* state )
    {
      // Damage is fully residual at the time of cast, across the DoT duration
      // Currently does not benefit from Dark Brew or Deeper Daggers
      const auto dot = td( state->target )->dots.rupture;
      assert( dot && dot->is_ticking() );

      // Takes the entire remaining DoT damage as if it were dealt instantly with the current state
      double damage_amount = state->action->calculate_tick_amount( dot->state, 1.0 );
      damage_amount *= ( dot->remains() / state->action->tick_time( dot->state ) );

      // Stretch the damage across the total ticks used for the DoT then apply the set bonus multiplier
      double num_ticks = ( composite_dot_duration( state ) / tick_time( state ) );
      damage_amount *= ( 1.0 / num_ticks ) * p()->set_bonuses.t31_subtlety_2pc->effectN( 1 ).percent();
      damage_amount *= 1.0 + p()->set_bonuses.t31_subtlety_4pc->effectN( 1 ).percent();
      if ( !p()->talent.subtlety.dark_brew->ok() )
      {
        // Gains from Veiltouched and Deeper Daggers directly if not talented into Dark Brew
        // Ignored with Dark Brew as Rupture's base shadow damage is already affected
        damage_amount *= 1.0 + p()->talent.subtlety.veiltouched->effectN( 4 ).percent();
        damage_amount *= 1.0 + p()->buffs.deeper_daggers->stack_value();
      }

      base_td = damage_amount;
      set_target( state->target );
      auto damage_state = get_state();
      damage_state->target = state->target;
      snapshot_state( damage_state, result_amount_type::DMG_OVER_TIME );
      trigger_secondary_action( damage_state, 1_s );
    }
  };

  replicating_shadows_tick_t* replicating_shadows_tick;
  poisoned_edges_t* poisoned_edges_damage;
  shadow_rupture_t* shadow_rupture_attack;
  
  rupture_t( util::string_view name, rogue_t* p, const spell_data_t* s, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, s, options_str ),
    replicating_shadows_tick( nullptr ), poisoned_edges_damage( nullptr ), shadow_rupture_attack( nullptr )
  {
    affected_by.t31_subtlety_4pc = true;

    if ( p->talent.subtlety.replicating_shadows->ok() )
    {
      // Manually apply AoE count as Replicating Shadows as talent has workaround +30 chain target value
      aoe = ( aoe == 0 ? 1 : 0 ) + as<int>( p->talent.subtlety.replicating_shadows->effectN( 4 ).base_value() );
      replicating_shadows_tick = p->get_background_action<replicating_shadows_tick_t>( "rupture_replicating_shadows" );
      add_child( replicating_shadows_tick );
    }

    if ( p->set_bonuses.t31_subtlety_2pc->ok() )
    {
      shadow_rupture_attack = p->get_secondary_trigger_action<shadow_rupture_t>(
        secondary_trigger::T31_SUBTLETY_2PC, "shadow_rupture" );
      add_child( shadow_rupture_attack );
    }
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( p()->set_bonuses.t30_assassination_2pc->ok() )
    {
      poisoned_edges_damage = p()->get_background_action<poisoned_edges_t>(
        secondary_trigger_type == secondary_trigger::DEATHMARK ? "rupture_deathmark_t30" :
                                                                 "rupture_t30" );
      add_child( poisoned_edges_damage );
    }

    if ( p()->talent.assassination.sanguine_blades->ok() )
    {
      add_child( secondary_trigger_type == secondary_trigger::DEATHMARK ?
                 p()->active.sanguine_blades.deathmark_rupture :
                 p()->active.sanguine_blades.rupture );
    }
  }

  int n_targets() const override
  {
    int n = rogue_attack_t::n_targets();

    if ( !is_secondary_action() && ( p()->buffs.indiscriminate_carnage->check() ||
                                     p()->buffs.indiscriminate_carnage_aura->check() ) )
    {
      n = as<int>( p()->talent.assassination.indiscriminate_carnage->effectN( 1 ).base_value() );
    }

    return n;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    rogue_attack_t::available_targets( tl );

    // Indiscriminate Carnage smart-targets beyond the primary target, preferring lowest duration
    // Replicating Shadows also smart-targets in a similar fashion as of 10.2.0
    if ( is_aoe() && tl.size() > 1 && !is_secondary_action() )
    {      
      if ( p()->talent.subtlety.replicating_shadows->ok() ||
           p()->buffs.indiscriminate_carnage->check() ||
           p()->buffs.indiscriminate_carnage_aura->check() )
      {
        auto it = std::partition( tl.begin(), tl.end(), [ this ]( player_t* t ) {
          return t == this->target;
        } );
        std::sort( it, tl.end(), [ this ]( player_t* a, player_t* b ) {
          return td( a )->dots.rupture->remains() < td( b )->dots.rupture->remains();
        } );
      }
    }

    return tl.size();
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    const auto rs = cast_state( s );
    timespan_t duration = data().duration() * ( 1 + rs->get_combo_points() );
    duration *= 1.0 / rs->get_exsanguinated_rate();
    return duration;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_persistent_multiplier( state );
    m *= 1.0 + p()->buffs.finality_rupture->value();
    return m;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    trigger_scent_of_blood();

    if ( p()->spec.finality_rupture_buff->ok() )
    {
      if ( p()->buffs.finality_rupture->check() )
        p()->buffs.finality_rupture->expire();
      else
        p()->buffs.finality_rupture->trigger();
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

    // TOCHECK -- Double check this interacts as expected for cleaved Ruptures
    // 2023-10-05 -- Currently when triggerd by an ER cast, only uses base combo points
    if ( p()->active.internal_bleeding )
    {
      p()->active.internal_bleeding->trigger_secondary_action( state->target,
                                                               cast_state( state )->get_combo_points( p()->bugs ) );
    }
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );
    trigger_venomous_wounds( d->state );
    trigger_sanguine_blades( d->state, secondary_trigger_type == secondary_trigger::DEATHMARK ?
                                       p()->active.sanguine_blades.deathmark_rupture :
                                       p()->active.sanguine_blades.rupture );

    if ( replicating_shadows_tick )
    {
      replicating_shadows_tick->execute_on_target( d->target );
    }

    if ( poisoned_edges_damage )
    {
      double multiplier = p()->set_bonuses.t30_assassination_2pc->effectN( 1 ).percent();
      double damage = d->state->result_total * multiplier;
      poisoned_edges_damage->execute_on_target( d->target, damage );
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

  bool trigger_t31_subtlety_set_bonus( const action_state_t* state, rogue_attack_t* ) override
  {
    if ( shadow_rupture_attack && rogue_attack_t::trigger_t31_subtlety_set_bonus( state, shadow_rupture_attack ) )
    {
      shadow_rupture_attack->trigger_clone_attack( state );
      return true;
    }

    return false;
  }

  bool snapshots_nightstalker() const override
  { return true; }
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
      reduced_aoe_targets = p->talent.subtlety.secret_technique->effectN( 6 ).base_value();
    }

    double composite_da_multiplier( const action_state_t* state ) const override
    {
      double m = rogue_attack_t::composite_da_multiplier( state );

      if ( secondary_trigger_type == secondary_trigger::SECRET_TECHNIQUE_CLONE )
      {
        // Secret Technique clones count as pets and benefit from pet modifiers
        m *= p()->composite_player_pet_damage_multiplier( state, true );

        // 2023-10-06 -- T31 4pc bonus affects SecTec, not just clone attacks from the 2pc
        m *= 1.0 + p()->set_bonuses.t31_subtlety_4pc->effectN( 1 ).percent();
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
        m *= p()->talent.subtlety.secret_technique->effectN( 3 ).percent();

      return m;
    }

    // 2023-10-02 -- Clone attacks do not trigger Shadow Blades proc damage
    bool procs_shadow_blades_damage() const override
    { return secondary_trigger_type != secondary_trigger::SECRET_TECHNIQUE_CLONE; }
  };

  secret_technique_attack_t* player_attack;
  secret_technique_attack_t* clone_attack;

  secret_technique_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.subtlety.secret_technique, options_str )
  {
    may_miss = false;
    affected_by.t31_subtlety_4pc = true;

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

    p()->buffs.secret_technique->trigger( 1.3_s );
    clone_attack->trigger_secondary_action( clone_state, 1_s );
    clone_attack->trigger_secondary_action( clone_state_2, 1.3_s );

    // Manually expire Cold Blood due to special handling above
    if ( p()->buffs.cold_blood->check() )
    {
      p()->buffs.cold_blood->expire();
      cold_blood_consumed_proc->occur();
    }
  }
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
    rogue_spell_t( name, p, p->spell.shadow_dance, options_str )
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

    if ( p()->talent.subtlety.the_first_dance->ok() )
    {
      trigger_combo_point_gain( as<int>( p()->talent.subtlety.the_first_dance->effectN( 1 ).base_value() ),
                                p()->gains.the_first_dance );
    }

    if ( p()->set_bonuses.t30_subtlety_2pc->ok() )
    {
      // 2023-05-01 -- This currently acts like a normal cast, including The Rotten and generating Energy
      // 2023-05-17 -- Updated to no longer decrease the duration of an existing buff, but still pandemics otherwise
      timespan_t symbols_duration = timespan_t::from_seconds( p()->set_bonuses.t30_subtlety_2pc->effectN( 1 ).base_value() );
      if ( p()->buffs.symbols_of_death->remains_lt( p()->buffs.symbols_of_death->refresh_duration( symbols_duration ) ) )
      {
        p()->buffs.symbols_of_death->trigger( symbols_duration );
      }
      p()->buffs.the_rotten->trigger( as<int>( p()->talent.subtlety.the_rotten->effectN( 1 ).base_value() ) );
      p()->resource_gain( RESOURCE_ENERGY, p()->spec.symbols_of_death->effectN( 5 ).resource(),
                          p()->gains.symbols_of_death_t30 );

      // Extends Rupture DoTs on all active targets
      timespan_t extend_duration = timespan_t::from_seconds( p()->set_bonuses.t30_subtlety_2pc->effectN( 2 ).base_value() );
      for ( auto t : sim->target_non_sleeping_list )
      {
        rogue_td_t* tdata = p()->get_target_data( t );
        if ( tdata->dots.rupture->is_ticking() )
        {
          // Pass in state_flags of 0 as this does not change the snapshot
          tdata->dots.rupture->adjust_duration( extend_duration );
        }
      }
    }
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

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p()->buffs.premeditation->up() )
    {
      if ( p()->buffs.slice_and_dice->check() )
      {
        trigger_combo_point_gain( as<int>( p()->talent.subtlety.premeditation->effectN( 2 ).base_value() ), p()->gains.premeditation );
      }

      timespan_t premed_duration = timespan_t::from_seconds( p()->talent.subtlety.premeditation->effectN( 1 ).base_value() );
      // Slice and Dice extension from Premeditation appears to be capped at 46.8s in any build, which equals 5CP duration with full pandemic.
      premed_duration = std::min( premed_duration, 46.8_s - p()->buffs.slice_and_dice->remains() );
      if ( premed_duration > timespan_t::zero() )
        p()->buffs.slice_and_dice->extend_duration_or_trigger( premed_duration );

      p()->buffs.premeditation->expire();
    }
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    trigger_weaponmaster( state, p()->active.weaponmaster.shadowstrike );
    trigger_find_weakness( state );
    trigger_inevitability( state );

    if ( state->result == RESULT_CRIT && p()->set_bonuses.t29_subtlety_4pc->ok() )
    {
      p()->buffs.t29_subtlety_4pc->trigger();
      p()->buffs.t29_subtlety_4pc_black_powder->trigger();
    }
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
      callbacks = false; // 2021-07-19-- Does not appear to trigger normal procs
      aoe = -1;
      reduced_aoe_targets = p->spec.black_powder->effectN( 4 ).base_value();

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

  struct shadow_powder_t : public rogue_attack_t
  {
    shadow_powder_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.t31_subtlety_2pc_black_powder )
    {
      aoe = -1;
    }

    void trigger_clone_attack( const action_state_t* state )
    {
      // Damage is residual at the time of cast, benefitting from Dark Brew and Deeper Daggers
      // For Black Powder, this appears to take the critical damage state of a random target
      double damage_amount = state->result_amount * p()->set_bonuses.t31_subtlety_2pc->effectN( 1 ).percent();
      damage_amount *= 1.0 + p()->set_bonuses.t31_subtlety_4pc->effectN( 1 ).percent();
      damage_amount *= 1.0 + p()->talent.subtlety.dark_brew->effectN( 4 ).percent();
      damage_amount *= 1.0 + p()->talent.subtlety.veiltouched->effectN( 3 ).percent();
      damage_amount *= 1.0 + p()->buffs.deeper_daggers->stack_value();

      base_dd_min = base_dd_max = damage_amount;
      set_target( state->target );
      auto damage_state = get_state();
      damage_state->target = state->target;
      snapshot_state( damage_state, result_amount_type::DMG_DIRECT );
      trigger_secondary_action( damage_state, 1_s );
    }

    bool procs_poison() const override
    { return false; }
  };

  black_powder_bonus_t* bonus_attack;
  shadow_powder_t* shadow_powder_attack;

  black_powder_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ):
    rogue_attack_t( name, p, p->spec.black_powder, options_str ),
    bonus_attack( nullptr ), shadow_powder_attack( nullptr )
  {
    aoe = -1;
    reduced_aoe_targets = p->spec.black_powder->effectN( 4 ).base_value();
    affected_by.t31_subtlety_4pc = true;

    if ( p->talent.subtlety.shadowed_finishers->ok() )
    {
      bonus_attack = p->get_background_action<black_powder_bonus_t>( "black_powder_bonus" );
      add_child( bonus_attack );
    }

    if ( p->set_bonuses.t31_subtlety_2pc->ok() )
    {
      shadow_powder_attack = p->get_secondary_trigger_action<shadow_powder_t>(
        secondary_trigger::T31_SUBTLETY_2PC, "shadow_powder" );
      add_child( shadow_powder_attack );
    }
  }

  void execute() override
  {
    p()->buffs.deeper_daggers->trigger();

    rogue_attack_t::execute();

    // BUG: Finality BP seems to affect every instance of shadow damage due to, err, spaghetti with the bonus attack trigger order and travel time?
    // See https://github.com/SimCMinMax/WoW-BugTracker/issues/747
    bool triggered_finality = false;
    if ( p()->spec.finality_black_powder_buff->ok() && !p()->buffs.finality_black_powder->check() )
    {
      p()->buffs.finality_black_powder->trigger();
      triggered_finality = true;
    }

    if ( bonus_attack )
    {
      bonus_attack->last_cp = cast_state( execute_state )->get_combo_points();
      bonus_attack->execute_on_target( execute_state->target );
    }

    // See bug above.
    if ( !triggered_finality )
    {
      p()->buffs.finality_black_powder->expire();
    }

    if ( p()->set_bonuses.t29_subtlety_2pc->ok() )
    {
      p()->buffs.t29_subtlety_2pc->expire();
      p()->buffs.t29_subtlety_2pc->trigger( cast_state( execute_state )->get_combo_points() );
    }
  }

  bool trigger_t31_subtlety_set_bonus( const action_state_t* state, rogue_attack_t* ) override
  {
    if ( shadow_powder_attack && rogue_attack_t::trigger_t31_subtlety_set_bonus( state, shadow_powder_attack ) )
    {
      shadow_powder_attack->trigger_clone_attack( state );
      return true;
    }

    return false;
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
  shuriken_storm_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ):
    rogue_attack_t( name, p, p->spec.shuriken_storm, options_str )
  {
    energize_type = action_energize::PER_HIT;
    energize_resource = RESOURCE_COMBO_POINT;
    energize_amount = 1;
    ap_type = attack_power_type::WEAPON_BOTH;
    // 2021-04-22- Not in the whitelist but confirmed as working in-game
    affected_by.shadow_blades_cp = true;

    aoe = -1;
    reduced_aoe_targets = data().effectN( 4 ).base_value();
  }

  double composite_crit_chance() const override
  {
    double c = rogue_attack_t::composite_crit_chance();    
    c += p()->buffs.silent_storm->stack_value();
    return c;
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p()->buffs.silent_storm->expire();
  }

  void impact(action_state_t* state) override
  {
    rogue_attack_t::impact( state );

    if ( state->result == RESULT_CRIT && p()->talent.subtlety.improved_shuriken_storm->ok() )
    {
      timespan_t duration = timespan_t::from_seconds( p()->talent.subtlety.improved_shuriken_storm->effectN( 1 ).base_value() );
      trigger_find_weakness( state, duration );
    }

    // 2023-01-31 -- Tornado-triggered Shuriken Storms do not activate 4pc
    // TOCHECK with the new foreground behavior on the PTR
    if ( p()->set_bonuses.t29_subtlety_4pc->ok() && state->result == RESULT_CRIT &&
         secondary_trigger_type != secondary_trigger::SHURIKEN_TORNADO )
    {
      p()->buffs.t29_subtlety_4pc->trigger();
      p()->buffs.t29_subtlety_4pc_black_powder->trigger();
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
  struct triple_threat_t : public rogue_attack_t
  {
    triple_threat_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.triple_threat_attack )
    {
    }

    void execute() override
    {
      rogue_attack_t::execute();
      p()->active.triple_threat_mh->trigger_secondary_action( execute_state->target );
    }

    bool procs_main_gauche() const override
    { return true; }

    bool procs_blade_flurry() const override
    { return true; }
  };

  struct sinister_strike_extra_attack_t : public rogue_attack_t
  {
    double triple_threat_chance;

    sinister_strike_extra_attack_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spec.sinister_strike_extra_attack ),
      triple_threat_chance( 0.0 )
    {
      energize_type = action_energize::ON_HIT;
      energize_resource = RESOURCE_COMBO_POINT;

      triple_threat_chance = p->talent.outlaw.triple_threat->effectN( 1 ).percent();
    }

    double composite_energize_amount( const action_state_t* ) const override
    {
      // CP generation is not in the spell data and the extra SS procs seem script-driven
      // Triple Threat procs of this spell don't give combo points, however
      return ( secondary_trigger_type == secondary_trigger::SINISTER_STRIKE ) ? 1 : 0;
    }

    void execute() override
    {
      rogue_attack_t::execute();

      // Triple Threat procs do not appear to be able to chain-proc based on testing
      if ( secondary_trigger_type == secondary_trigger::SINISTER_STRIKE &&
           p()->active.triple_threat_oh && p()->rng().roll( triple_threat_chance ) )
      {
        p()->active.triple_threat_oh->trigger_secondary_action( execute_state->target, 300_ms );
      }

      trigger_count_the_odds( execute_state, p()->procs.count_the_odds_ss );
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
      if ( p()->active.triple_threat_mh )
        add_child( p()->active.triple_threat_mh );
      if ( p()->active.triple_threat_oh )
        add_child( p()->active.triple_threat_oh );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();
    trigger_opportunity( execute_state, extra_attack );
    trigger_count_the_odds( execute_state, p()->procs.count_the_odds_ss );
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

// Symbols of Death =========================================================

struct symbols_of_death_t : public rogue_spell_t
{
  symbols_of_death_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->spec.symbols_of_death, options_str )
  {
    harmful = callbacks = false;
    dot_duration = timespan_t::zero();
    set_target( p );
  }

  void execute() override
  {
    rogue_spell_t::execute();

    p()->buffs.symbols_of_death->trigger();
    p()->buffs.the_rotten->trigger( as<int>( p()->talent.subtlety.the_rotten->effectN( 1 ).base_value() ) );
  }
};

// Shiv =====================================================================

struct shiv_t : public rogue_attack_t
{
  shiv_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.rogue.shiv, options_str )
  {
  }

  void impact( action_state_t* s ) override
  {
    rogue_attack_t::impact( s );

    if ( result_is_hit( s->result ) && p()->talent.assassination.improved_shiv->ok() )
    {
      td( s->target )->debuffs.shiv->trigger();
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
  stealth_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_spell_t( name, p, p->spell.stealth, options_str )
  {
    harmful = false;
    set_target( p );
  }

  void execute() override
  {
    rogue_spell_t::execute();
    p()->buffs.stealth->trigger();
    trigger_master_of_shadows();
  }

  bool ready() override
  {
    if ( p()->stealthed( STEALTH_BASIC | STEALTH_ROGUE ) )
      return false;

    if ( !p()->in_combat )
      return true;

    // Allow restealth for Dungeon sims against non-boss targets as Shadowmeld drops combat against trash.
    if ( ( p()->sim->fight_style == FIGHT_STYLE_DUNGEON_SLICE || p()->sim->fight_style == FIGHT_STYLE_DUNGEON_ROUTE ) &&
         p()->player_t::buffs.shadowmeld->check() && !p()->target->is_boss() )
      return true;

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

  bool snapshots_nightstalker() const override
  { return true; }
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

    trigger_sting_like_a_bee( state );

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
    trigger_sting_like_a_bee( state );
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

  bool procs_caustic_spatter() const override
  { return false; }
};

// Caustic Spatter ==========================================================

struct caustic_spatter_t : public rogue_attack_t
{
  caustic_spatter_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spec.caustic_spatter_damage )
  {
    aoe = -1;
    reduced_aoe_targets = p->spec.caustic_spatter_buff->effectN( 2 ).base_value();
  }

  void init() override
  {
    rogue_attack_t::init();
    snapshot_flags |= STATE_TGT_MUL_DA;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    rogue_attack_t::available_targets( tl );

    // Cannot hit the original target.
    range::erase_remove( tl, target );

    return tl.size();
  }

  bool procs_poison() const override
  { return false; }

  bool procs_caustic_spatter() const override
  { return false; }
};

// Sanguine Blades ==========================================================

struct sanguine_blades_t : public rogue_attack_t
{
  sanguine_blades_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spec.sanguine_blades_damage )
  {
    ignores_armor = true;           // Not in spell data
    base_dd_min = base_dd_max = 1;  // Override from 0 for snapshot_flags
  }
};

// Vicious Venoms ===========================================================

struct vicious_venoms_t : public rogue_attack_t
{
  bool triggers_doomblade;

  vicious_venoms_t( util::string_view name, rogue_t* p, const spell_data_t* s, bool from_multilate ) :
    rogue_attack_t( name, p, s ), triggers_doomblade( from_multilate )
  {
    base_dd_min = base_dd_max = 1;  // Override from 0 for snapshot_flags
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );
    if ( triggers_doomblade )
    {
      trigger_doomblade( state );
    }
  }

  bool procs_poison() const override
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
    rogue_spell_t( name, p, p->talent.rogue.thistle_tea ),
    precombat_seconds( 0.0 )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = false;
    energize_type = action_energize::ON_CAST;
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
};

// Echoing Reprimand ========================================================

struct echoing_reprimand_t : public rogue_attack_t
{
  size_t random_min, random_max;

  echoing_reprimand_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->spell.echoing_reprimand, options_str ),
    random_min( 1 ), random_max( 4 ) // Randomizes between 3CP and 5CP buffs
  {
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( result_is_hit( state->result ) )
    {
      // Casting Echoing Reprimand cancels all existing buffs. Relevant for CDR trait in AoE.
      for ( buff_t* b : p()->buffs.echoing_reprimand )
        b->expire();

      if ( p()->talent.rogue.resounding_clarity->ok() )
      {
        // 2022-12-16 -- Resounding Clarity now animacharges 2 instead of 3 additional buffs
        // This was accomplished by simply removing the chance to generate a 5 CP buff
        for ( size_t i = random_min; i < random_max; i++ )
        {
          p()->buffs.echoing_reprimand[ i ]->trigger();
        }
      }
      else
      {
        unsigned buff_idx = static_cast<int>( rng().range( random_min, random_max ) );
        p()->buffs.echoing_reprimand[ buff_idx ]->trigger();
      }
    }
  }

  bool procs_blade_flurry() const override
  { return true; }
};

// Flagellation =============================================================

struct flagellation_damage_t : public rogue_attack_t
{
  buff_t* debuff;

  flagellation_damage_t( util::string_view name, rogue_t* p ) :
    rogue_attack_t( name, p, p->spec.flagellation_damage ),
    debuff( nullptr )
  {
    dual = true;
  }

  double combo_point_da_multiplier( const action_state_t* s ) const override
  {
    return as<double>( cast_state( s )->get_combo_points() );
  }
};

struct flagellation_t : public rogue_attack_t
{
  flagellation_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.subtlety.flagellation, options_str )
  {
    dot_duration = timespan_t::zero();

    if ( p->active.flagellation )
    {
      add_child( p->active.flagellation );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p()->active.flagellation->debuff = td( execute_state->target )->debuffs.flagellation;
    p()->active.flagellation->debuff->trigger();

    p()->buffs.flagellation->trigger();
  }
};

// Sepsis ===================================================================

struct sepsis_t : public rogue_attack_t
{
  struct sepsis_expire_damage_t : public rogue_attack_t
  {
    sepsis_expire_damage_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->spell.sepsis_expire_damage )
    {
      dual = true;
      affected_by.lethal_dose = false; // 2023-06-11 -- Recent testing indicates this is no longer affected
      affected_by.zoldyck_insignia = false;
    }
  };

  sepsis_expire_damage_t* sepsis_expire_damage;

  sepsis_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.shared.sepsis, options_str )
  {
    affected_by.broadside_cp = true; // 2021-04-22 -- Not in the whitelist but confirmed as working in-game
    affected_by.lethal_dose = true;
    affected_by.zoldyck_insignia = false;
    sepsis_expire_damage = p->get_background_action<sepsis_expire_damage_t>( "sepsis_expire_damage" );
    sepsis_expire_damage->stats = stats;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_persistent_multiplier( state );

    // 2023-10-08 -- Sepsis is now scripted to snapshot Perforated Veins and The Rotten
    if ( p()->buffs.perforated_veins->check() )
      m *= p()->buffs.perforated_veins->direct_mod.multiplier;

    if ( p()->buffs.the_rotten->check() )
      m *= p()->buffs.the_rotten->direct_mod.multiplier;

    return m;
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p()->buffs.sepsis->trigger();
    p()->buffs.perforated_veins->decrement();
    p()->buffs.the_rotten->decrement();
  }

  void last_tick( dot_t* d ) override
  {
    rogue_attack_t::last_tick( d );
    sepsis_expire_damage->execute_on_target( d->target );
    p()->buffs.sepsis->trigger();
  }

  bool snapshots_nightstalker() const override
  { return true; }
};

// Serrated Bone Spike ======================================================

struct serrated_bone_spike_t : public rogue_attack_t
{
  struct serrated_bone_spike_dot_t : public rogue_attack_t
  {
    serrated_bone_spike_dot_t( util::string_view name, rogue_t* p ) :
      rogue_attack_t( name, p, p->talent.assassination.serrated_bone_spike->effectN( 2 ).trigger() )
    {
      aoe = 0;
      hasted_ticks = true; // Uses the SX_DOT_HASTED_MELEE
      dot_duration = timespan_t::from_seconds( sim->expected_max_time() * 3 );
    }

    bool procs_poison() const override
    { return false; }
  };

  int base_impact_cp;
  serrated_bone_spike_dot_t* serrated_bone_spike_dot;

  serrated_bone_spike_t( util::string_view name, rogue_t* p, util::string_view options_str = {} ) :
    rogue_attack_t( name, p, p->talent.assassination.serrated_bone_spike, options_str )
  {
    // Combo Point generation is in a secondary spell due to scripting logic
    // However, leave the ON_HIT entry as it can trigger things like Seal Fate
    energize_type = action_energize::ON_HIT;
    energize_resource = RESOURCE_COMBO_POINT;
    energize_amount = 0.0;
    base_impact_cp = as<int>( p->spec.serrated_bone_spike_energize->effectN( 1 ).base_value() );
    affected_by.lethal_dose = false;
    affected_by.zoldyck_insignia = false; // Does not affect direct, does affect periodic

    serrated_bone_spike_dot = p->get_background_action<serrated_bone_spike_dot_t>( "serrated_bone_spike_dot" );
    add_child( serrated_bone_spike_dot );
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( !t ) t = target;
    if ( !t ) return nullptr;
    return td( t )->dots.serrated_bone_spike;
  }

  double generate_cp() const override
  {
    double cp = rogue_attack_t::generate_cp();

    cp += base_impact_cp;
    cp += p()->get_active_dots( td( target )->dots.serrated_bone_spike );

    return cp;
  }

  void impact( action_state_t* state ) override
  {
    const unsigned active_dots = p()->get_active_dots( get_dot( state->target ) );

    rogue_attack_t::impact( state );
    serrated_bone_spike_dot->execute_on_target( state->target );

    // 2022-01-26 -- Deathspike logs shows this only happens on impact for the primary target
    if ( state->chain_target == 0 )
    {
      trigger_combo_point_gain( base_impact_cp + active_dots, p()->gains.serrated_bone_spike );
    }
  }
};

// Outlaw T30 Set Bonus =====================================================

struct soulrip_cb_t : public dbc_proc_callback_t
{
  // Note: Uses spell_t instead of rogue_spell_t to avoid action_state casting issues
  struct soulrip_damage_t : public residual_action::residual_periodic_action_t<spell_t>
  {
    soulrip_damage_t( util::string_view name, rogue_t* p ) :
      residual_action_t( name, p, p->spec.t30_outlaw_2pc_attack )
    {
      dual = true;
    }
  };

  soulrip_damage_t* damage;
  rogue_t* rogue;

  soulrip_cb_t( rogue_t* p, const special_effect_t& e )
    : dbc_proc_callback_t( p, e ), damage( nullptr ), rogue( p )
  {
    damage = rogue->get_background_action<soulrip_damage_t>( "soulrip" );
  }

  void execute( action_t* a, action_state_t* s ) override
  {
    dbc_proc_callback_t::execute( a, s );
    const double amount = s->result_amount * rogue->set_bonuses.t30_outlaw_2pc->effectN( 1 ).percent();
    residual_action::trigger( damage, s->target, amount );
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
      this->name_str == "crimson_tempest" || this->name_str == "serrated_bone_spike" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      dot_t* d = ab::get_dot( ab::target );
      return d->is_ticking() && cast_state( d->state )->is_exsanguinated();
    } );
  }
  else if ( util::str_compare_ci( name_str, "exsanguinated_rate" ) &&
    ( ab::data().id() == 703 || ab::data().id() == 1943 ||
      this->name_str == "crimson_tempest" || this->name_str == "serrated_bone_spike" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      dot_t* d = ab::get_dot( ab::target );
      return d->is_ticking() ? cast_state( d->state )->get_exsanguinated_rate() : 1.0;
    } );
  }
  else if ( util::str_compare_ci( name_str, "will_lose_exsanguinate" ) &&
    ( ab::data().id() == 703 || ab::data().id() == 1943
      || this->name_str == "crimson_tempest" || this->name_str == "serrated_bone_spike" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      dot_t* d = ab::get_dot( ab::target );
      double refresh_rate = 1.0;
      return d->is_ticking() && cast_state( d->state )->get_exsanguinated_rate() > refresh_rate;
    } );
  }
  else if ( name_str == "effective_combo_points" )
  {
    return make_fn_expr( name_str, [ this ]() { return p()->current_effective_cp( consumes_echoing_reprimand(), true ); } );
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
    set_default_value_from_effect_type( A_MOD_RANGED_AND_MELEE_ATTACK_SPEED );
    set_affects_regen( true );
    add_invalidate( CACHE_ATTACK_SPEED );
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

    // "Pause" blade flurry
    if ( rogue->talent.outlaw.underhanded_upper_hand->ok()  )
    {
      rogue->buffs.blade_flurry->pause();
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

    if ( rogue->talent.outlaw.underhanded_upper_hand->ok() )
    {
      rogue->buffs.blade_flurry->unpause();
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
    apply_affecting_aura( p->talent.outlaw.dancing_steel );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    buff_t::start( stacks, value, duration );

    if ( p->talent.outlaw.underhanded_upper_hand->ok() )
    {
      p->buffs.slice_and_dice->pause();
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( p->talent.outlaw.underhanded_upper_hand->ok() )
    {
      p->buffs.slice_and_dice->unpause();
    }
  }
};

struct subterfuge_t : public buff_t
{
  rogue_t* rogue;

  subterfuge_t( rogue_t* r ) :
    buff_t( r, "subterfuge", r->spell.subterfuge_buff ),
    rogue( r )
  {
    apply_affecting_aura( r->talent.rogue.subterfuge );
    apply_affecting_aura( r->talent.outlaw.underhanded_upper_hand );
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    if ( rogue->talent.outlaw.underhanded_upper_hand->ok() )
    {
      rogue->buffs.adrenaline_rush->pause();
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( rogue->talent.outlaw.underhanded_upper_hand->ok() && !rogue->stealthed( STEALTH_BASIC | STEALTH_ROGUE ) )
    {
      rogue->buffs.adrenaline_rush->unpause();
    }
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
      if ( rogue->talent.assassination.master_assassin->ok() )
        rogue->buffs.master_assassin_aura->trigger();

      if ( rogue->talent.assassination.improved_garrote->ok() )
        rogue->buffs.improved_garrote_aura->trigger();

      if ( rogue->talent.assassination.indiscriminate_carnage->ok() )
        rogue->buffs.indiscriminate_carnage_aura->trigger();

      if ( rogue->talent.outlaw.take_em_by_surprise->ok() )
      {
        // 2023-11-04 -- Shadow Dance does not trigger the aura buff if the duration buff is already active
        if ( !rogue->bugs || !( rogue->stealthed( STEALTH_SHADOW_DANCE ) && rogue->buffs.take_em_by_surprise->check() ) )
          rogue->buffs.take_em_by_surprise_aura->trigger();
      }

      if ( rogue->talent.subtlety.premeditation->ok() )
        rogue->buffs.premeditation->trigger();

      if ( rogue->talent.subtlety.shot_in_the_dark->ok() )
        rogue->buffs.shot_in_the_dark->trigger();

      if ( rogue->talent.subtlety.silent_storm->ok() )
        rogue->buffs.silent_storm->trigger();
    }

    if ( rogue->stealthed( STEALTH_BASIC | STEALTH_ROGUE ) && rogue->talent.outlaw.underhanded_upper_hand->ok() )
    {
      rogue->buffs.adrenaline_rush->pause();
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    bb::expire_override( expiration_stacks, remaining_duration );

    // Don't swap these buffs around if we are still in stealth due to Vanish expiring
    if ( !rogue->stealthed( STEALTH_BASIC ) )
    {
      rogue->buffs.improved_garrote_aura->expire();
      rogue->buffs.master_assassin_aura->expire();
      rogue->buffs.indiscriminate_carnage_aura->expire();
      rogue->buffs.take_em_by_surprise_aura->expire();

      // 2023-10-21 -- Premeditation does not persist into Subterfuge when Stealth expires
      if ( rogue->bugs )
      {
        rogue->buffs.premeditation->expire();
      }
    }

    if ( rogue->talent.outlaw.underhanded_upper_hand->ok() && !rogue->stealthed( STEALTH_BASIC | STEALTH_ROGUE ) )
    {
      rogue->buffs.adrenaline_rush->unpause();
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
  std::vector<cooldown_t*> shadowdust_cooldowns;
  const timespan_t shadowdust_reduction;

  vanish_t( rogue_t* r ) :
    base_t( r, "vanish", r->spell.vanish_buff ),
    shadowdust_reduction( timespan_t::from_seconds( r->spec.invigorating_shadowdust_cdr->effectN( 1 ).base_value() ) )
  {
    if ( r->talent.subtlety.invigorating_shadowdust || r->options.prepull_shadowdust )
    {
      shadowdust_cooldowns = { r->cooldowns.blind, r->cooldowns.cloak_of_shadows, r->cooldowns.cold_blood, r->cooldowns.echoing_reprimand,
        r->cooldowns.flagellation, r->cooldowns.goremaws_bite, r->cooldowns.gouge, r->cooldowns.secret_technique, r->cooldowns.sepsis,
        r->cooldowns.shadow_blades, r->cooldowns.shadow_dance, r->cooldowns.shadowstep, r->cooldowns.shiv, r->cooldowns.shuriken_tornado,
        r->cooldowns.sprint, r->cooldowns.symbols_of_death, r->cooldowns.thistle_tea };
    }
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    base_t::execute( stacks, value, duration );
    rogue->cancel_auto_attacks();

    // Vanish drops combat if in combat with non-bosses, relevant for some trinket effects
    if ( !rogue->in_boss_encounter )
      rogue->leave_combat();

    // Confirmed on early beta that Invigorating Shadowdust triggers from Vanish buff (via old Sepsis), not just Vanish casts
    if ( rogue->talent.subtlety.invigorating_shadowdust ||
         ( rogue->options.prepull_shadowdust && rogue->sim->current_time() == 0_s ) )
    {
      for ( cooldown_t* c : shadowdust_cooldowns )
      {
        if ( c && ( c->down() || c->current_charge_remains() > 0_s ) )
          c->adjust( -shadowdust_reduction, false );
      }
    }
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
    base_t( p, "shadow_dance", p->spell.shadow_dance )
  {
    set_cooldown( timespan_t::zero() );
    apply_affecting_aura( p->talent.subtlety.dark_shadow );
    apply_affecting_aura( p->talent.subtlety.improved_shadow_dance );
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
    rogue->buffs.indiscriminate_carnage->expire();
    rogue->buffs.master_assassin->expire();

    // 2023-11-04 -- This expires now as of PTR, but didn't previously on live. Unclear if intended.
    if ( rogue->bugs )
    {
      rogue->buffs.take_em_by_surprise->expire();
    }
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
    set_default_value_from_effect_type( A_MOD_RANGED_AND_MELEE_ATTACK_SPEED );
    set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
    add_invalidate( CACHE_ATTACK_SPEED );
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
    apply_affecting_aura( debug_cast<rogue_t*>( r.source )->talent.rogue.improved_wound_poison );
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
  std::array<buff_t*, 6> buffs;
  std::array<proc_t*, 6> procs;
  std::array<proc_t*, 6> loss_procs;

  struct overflow_state
  {
    buff_t* buff;
    timespan_t remains;
  };
  std::vector<overflow_state> overflow_states;

  roll_the_bones_t( rogue_t* r ) :
    buff_t( r, "roll_the_bones", r->spec.roll_the_bones ),
    rogue( r )
  {
    set_cooldown( timespan_t::zero() );
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
        // If we don't have a buff in the overflow list, assume it is lost
        if ( !range::contains( overflow_states, buffs[ i ], []( const auto& state ) { return state.buff; } ) )
        {
          loss_procs[ i ]->occur();
        }

        buffs[ i ]->expire();
      }
    }
  }

  void count_the_odds_trigger( timespan_t duration )
  {
    if ( !rogue->talent.outlaw.count_the_odds->ok() )
      return;

    std::vector<buff_t*> inactive_buffs;
    for ( buff_t* buff : buffs )
    {
      if ( !buff->check() )
        inactive_buffs.push_back( buff );
    }

    if ( inactive_buffs.empty() )
    {
      rogue->procs.count_the_odds_capped->occur();
      return;
    }

    unsigned buff_idx = static_cast<int>( rng().range( 0, as<double>( inactive_buffs.size() ) ) );
    inactive_buffs[ buff_idx ]->trigger( duration );
  }

  void overflow_expire( bool save_remains )
  {
    overflow_states.clear();

    if ( !save_remains )
      return;

    for ( buff_t* buff : buffs )
    {
      if ( buff->check() )
      {
        // 2022-06-19 -- 9.2.5: Recent testing shows only buffs with longer duration than existing RtB buffs are preserved
        if ( buff->remains() < remains() )
        {
          rogue->procs.roll_the_bones_wasted->occur();
        }
        else if ( buff->remains() > remains() )
        {
          overflow_states.push_back( { buff, buff->remains() } );
        }
      }
    }
  }

  void overflow_restore()
  {
    if ( overflow_states.empty() )
      return;

    // 2021-03-08 -- 9.0.5: If the same roll as an existing partial buff is in the result, the partial buff is lost
    // 2022-06-09 -- TOCHECK: This may be the cause of odd reroll behavior, need to investgate
    for ( auto &state : overflow_states )
    {
      if ( !state.buff->check() )
      {
        state.buff->trigger( state.remains );
      }
    }
  }

  std::vector<buff_t*> random_roll( bool loaded_dice, std::vector<unsigned> pool )
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

      if ( rogue->talent.outlaw.sleight_of_hand->ok() )
      {
        rogue->options.fixed_rtb_odds[ 0 ] -= rogue->talent.outlaw.sleight_of_hand->effectN( 1 ).base_value();
        rogue->options.fixed_rtb_odds[ 1 ] += rogue->talent.outlaw.sleight_of_hand->effectN( 1 ).base_value();
      }
    }

    if ( !rogue->options.fixed_rtb_odds.empty() )
    {
      std::vector<double> current_odds = rogue->options.fixed_rtb_odds;
      if ( loaded_dice )
      {
        // Loaded Dice converts 1 buff chance directly into two buff chance. (2020-03-09)
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

  unsigned roll_the_bones( timespan_t duration, std::vector<unsigned> pool )
  {
    std::vector<buff_t*> rolled;
    if ( rogue->options.fixed_rtb.empty() )
    {
      rolled = random_roll( rogue->buffs.loaded_dice->up(), pool );
    }
    else
    {
      rolled = fixed_roll();
    }

    for ( auto buff : rolled )
    {
      buff->trigger( duration );
    }

    return as<unsigned>( rolled.size() );
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    // 2020-11-21 -- Count the Odds buffs are kept if rerolling in some situations
    overflow_expire( true );

    buff_t::execute( stacks, value, duration );

    const timespan_t roll_duration = remains();
    std::vector<unsigned> pool = { 0, 1, 2, 3, 4, 5 };

    if ( rogue->set_bonuses.t31_outlaw_4pc->ok() ) 
    {
      std::vector<buff_t*> active_buffs;
      for ( auto buff : buffs )
      {
        if ( buff->check() )
          active_buffs.push_back( buff );
      }

      if ( active_buffs.empty() )
      {
        expire_secondary_buffs();
        rogue->procs.t31_buff_not_extended->occur();
      }
      else
      {
        rogue->procs.t31_buff_extended->occur();

        unsigned active_buff_idx = static_cast<int>( rng().range( 0, as<double>( active_buffs.size() ) ) );
        buff_t* extended = active_buffs[ active_buff_idx ];
        unsigned buff_idx = 0;
        for ( auto buff : buffs )
        {
          if ( buff == extended )
          {
            pool.erase( pool.begin() + buff_idx ); // remove the extended buff as a possible roll for the actual roll
            break;
          }
          buff_idx++;
        }
        expire_secondary_buffs();
        extended->trigger( roll_duration );
      }
    }
    else
    {
      expire_secondary_buffs();
    }

    const unsigned buffs_rolled = roll_the_bones( roll_duration, pool );

    procs[ buffs_rolled - 1 ]->occur();
    rogue->buffs.loaded_dice->expire();

    overflow_restore();
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
  int replenish = as<int>( talent.assassination.venomous_wounds->effectN( 2 ).base_value() );

  sim->print_debug( "{} venomous_wounds replenish on death: full_ticks={}, ticks_left={}, vw_replenish={}, remaining_time={}",
                    *this, full_ticks_remaining, td->dots.rupture->ticks_left(), replenish, td->dots.rupture->remains() );

  resource_gain( RESOURCE_ENERGY, full_ticks_remaining * replenish, gains.venomous_wounds_death,
                 td->dots.rupture->current_action );
}

void rogue_t::do_exsanguinate( dot_t* dot, double rate )
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

  double proc_chance = p()->mastery.main_gauche->proc_chance() +
                       p()->talent.outlaw.improved_main_gauche->effectN( 1 ).percent();

  if ( p()->buffs.blade_flurry->check() )
  {
    proc_chance += p()->talent.outlaw.ambidexterity->effectN( 1 ).percent();
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
void actions::rogue_action_t<Base>::trigger_vicious_venoms( const action_state_t* state, rogue_attack_t* action )
{
  if ( !p()->talent.assassination.vicious_venoms->ok() || !ab::result_is_hit( state->result ) )
    return;

  if ( !action || action->secondary_trigger_type != secondary_trigger::VICIOUS_VENOMS )
    return;

  double damage_amount = state->result_amount * p()->talent.assassination.vicious_venoms->effectN( 1 ).percent();
  action->base_dd_min = action->base_dd_max = damage_amount;
  action->set_target( state->target );
  auto damage_state = action->get_state();
  damage_state->target = state->target;
  action->snapshot_state( damage_state, result_amount_type::DMG_DIRECT );
  action->trigger_secondary_action( damage_state );
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

  // Grand Melee buff is additive with Killing Spree 100% base value
  if ( p()->buffs.grand_melee->up() )
  {
    multiplier += p()->spec.grand_melee->effectN( 2 ).percent();
  }

  if ( p()->talent.outlaw.precise_cuts->ok() )
  {
    // Already ignores the main target due to the target_list() being filtered
    const auto num_targets = as<int>( p()->active.blade_flurry->targets_in_range_list( p()->active.blade_flurry->target_list() ).size() );
    const auto max_targets = p()->active.blade_flurry->aoe;
    if ( num_targets < max_targets )
    {
      multiplier += p()->talent.outlaw.precise_cuts->effectN( 1 ).percent() * ( max_targets - num_targets );
    }
  }

  // Target multipliers do not replicate to secondary targets, need to reverse them out
  const double target_da_multiplier = ( 1.0 / state->target_da_multiplier );

  // Note: Unmitigated damage as Blade Flurry target mitigation is handled on each impact
  double damage = state->result_total * multiplier * target_da_multiplier;
  player_t* primary_target = state->target;

  p()->sim->print_debug( "{} flurries {} for {:.2f} damage ({:.2f} * {} * {:.3f})", *p(), *this, damage, state->result_total, multiplier, target_da_multiplier );

  // Trigger as an event so that this happens after the impact for proc/RPPM targeting purposes
  // Can't use schedule_execute() since multiple flurries can trigger at the same time due to Main Gauche
  make_event( *p()->sim, 0_ms, [ this, damage, primary_target ]() {
    if ( p()->sim->active_enemies > 1 )
    {
      p()->active.blade_flurry->execute_on_target( primary_target, damage );
    }
  } );
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
  const unsigned shadowcraft_adjustment = ( p()->talent.subtlety.shadowcraft->ok() && p()->buffs.symbols_of_death->check() ) ? 1 : 0;
  const unsigned shadow_techniques_upper = 4 - shadowcraft_adjustment;
  const unsigned shadow_techniques_lower = 3 - shadowcraft_adjustment;
  if ( ++p()->shadow_techniques_counter >= shadow_techniques_upper || ( p()->shadow_techniques_counter == shadow_techniques_lower && p()->rng().roll( 0.5 ) ) )
  {
    p()->sim->print_debug( "{} trigger_shadow_techniques proc'd at {}, resetting counter to 0", *p(), p()->shadow_techniques_counter );
    p()->shadow_techniques_counter = 0;

    double energy_gain = p()->spec.shadow_techniques_energize->effectN( 2 ).base_value() +
                         p()->talent.subtlety.improved_shadow_techniques->effectN( 1 ).base_value();
    p()->resource_gain( RESOURCE_ENERGY, energy_gain, p()->gains.shadow_techniques, state->action );
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
    action->trigger_secondary_action( state->target, 300_ms );
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_alacrity( const action_state_t* state )
{
  if ( !p()->talent.rogue.alacrity->ok() || !affected_by.alacrity )
    return;

  double chance = p()->talent.rogue.alacrity->effectN( 2 ).percent() * cast_state( state )->get_combo_points();
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
  v *= -cast_state( state )->get_combo_points();

  p()->cooldowns.adrenaline_rush->adjust( v, false );
  p()->cooldowns.between_the_eyes->adjust( v, false );
  p()->cooldowns.blade_flurry->adjust( v, false );
  p()->cooldowns.blade_rush->adjust( v, false );
  p()->cooldowns.ghostly_strike->adjust( v, false );
  p()->cooldowns.grappling_hook->adjust( v, false );
  p()->cooldowns.keep_it_rolling->adjust( v, false );
  p()->cooldowns.killing_spree->adjust( v, false );
  p()->cooldowns.roll_the_bones->adjust( v, false );
  p()->cooldowns.sprint->adjust( v, false );
  p()->cooldowns.vanish->adjust( v, false );

  if ( p()->talent.outlaw.float_like_a_butterfly->ok() )
  {
    p()->cooldowns.evasion->adjust( v, false );
    p()->cooldowns.feint->adjust( v, false );
  }
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
void actions::rogue_action_t<Base>::trigger_venom_rush( const action_state_t* state )
{
  if ( !p()->talent.assassination.venom_rush->ok() )
    return;

  if ( !ab::result_is_hit( state->result ) || !p()->get_target_data( state->target )->is_poisoned() )
    return;
  
  p()->resource_gain( RESOURCE_ENERGY, p()->spec.venom_rush_energize->effectN( 1 ).resource( RESOURCE_ENERGY ), p()->gains.venom_rush );
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

  // 2018-06-28 -- Secret Technique does not reduce its own cooldown
  // 2022-11-18 -- Updated to work with Echoing Reprimand via hotfix
  if ( p()->talent.subtlety.secret_technique->ok() && ab::data().id() != p()->talent.subtlety.secret_technique->id() )
  {
    timespan_t sectec_cdr = timespan_t::from_seconds( p()->talent.subtlety.secret_technique->effectN( 5 ).base_value() );
    sectec_cdr *= rs->get_combo_points();
    p()->cooldowns.secret_technique->adjust( -sectec_cdr, false );
  }

  // Remove Echoing Reprimand Buffs
  if ( p()->spell.echoing_reprimand->ok() && consumes_echoing_reprimand() )
  {
    int base_cp = rs->get_combo_points( true );
    if ( rs->get_combo_points() > base_cp )
    {
      auto it = range::find_if( p()->buffs.echoing_reprimand, [ base_cp ]( const buff_t* buff ) { return buff->check_value() == base_cp; } );
      assert( it != p()->buffs.echoing_reprimand.end() );
      ( *it )->expire();
      p()->procs.echoing_reprimand[ it - p()->buffs.echoing_reprimand.begin() ]->occur();
      animacharged_cp_proc->occur();
    }
  }
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_shadow_blades_attack( const action_state_t* state )
{
  if ( !p()->buffs.shadow_blades->check() || state->result_total <= 0 || !ab::result_is_hit( state->result ) || !procs_shadow_blades_damage() )
    return;

  double amount = state->result_amount * p()->buffs.shadow_blades->check_value();
  p()->active.shadow_blades_attack->execute_on_target( state->target, amount );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_sting_like_a_bee( const action_state_t* state )
{
  if ( !state->target->is_boss() || !ab::result_is_hit( state->result ) || !p()->talent.outlaw.sting_like_a_bee->ok() )
    return;

  td( state->target )->debuffs.sting_like_a_bee->trigger();
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
void actions::rogue_action_t<Base>::trigger_count_the_odds( const action_state_t* state, proc_t* source_proc )
{
  if ( !p()->talent.outlaw.count_the_odds->ok() )
    return;

  if ( !ab::result_is_hit( state->result ) )
    return;

  // Confirmed via logs this works with Shadowmeld and Shadow Dance
  double stealth_bonus = 1.0;
  if ( p()->stealthed( STEALTH_BASIC | STEALTH_SHADOWMELD | STEALTH_SHADOW_DANCE ) )
  {
    stealth_bonus += p()->talent.outlaw.count_the_odds->effectN( 3 ).percent();
  }

  double trigger_chance = p()->talent.outlaw.count_the_odds->effectN( 1 ).percent();
  timespan_t trigger_duration = timespan_t::from_seconds( p()->talent.outlaw.count_the_odds->effectN( 2 ).base_value() );

  if ( !p()->rng().roll( trigger_chance * stealth_bonus ) )
    return;

  debug_cast<buffs::roll_the_bones_t*>( p()->buffs.roll_the_bones )->count_the_odds_trigger( trigger_duration * stealth_bonus );
  p()->procs.count_the_odds->occur();
  source_proc->occur();
  if ( stealth_bonus > 1.0 )
  {
    p()->procs.count_the_odds_stealth->occur();
  }
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
void actions::rogue_action_t<Base>::trigger_flagellation( const action_state_t* state )
{
  if ( !affected_by.flagellation )
    return;

  if ( !p()->talent.subtlety.flagellation->ok() )
    return;

  buff_t* debuff = p()->active.flagellation->debuff;
  if ( !debuff || !debuff->up() )
    return;

  const auto rs = cast_state( state );
  const int cp_spend = rs->get_combo_points();
  if ( cp_spend <= 0 )
    return;

  p()->buffs.flagellation->trigger( cp_spend );
  
  // 2022-02-06 -- Testing shows that Outlaw 4pc procs trigger buff stacks but not damage/CDR
  if ( is_secondary_action() )
    return;

  p()->active.flagellation->trigger_secondary_action( debuff->player, cp_spend, 0.75_s );

  for ( int i = 0; i < cp_spend; i++ )
  {
    p()->procs.flagellation_cp_spend->occur();
  }
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
void actions::rogue_action_t<Base>::trigger_inevitability( const action_state_t* state )
{
  if ( !p()->talent.subtlety.inevitability->ok() || !ab::result_is_hit( state->result ) )
    return;

  // Testing appears to show this does not work on Weaponmaster attacks
  if ( is_secondary_action() )
    return;

  timespan_t extend_duration = timespan_t::from_seconds( p()->talent.subtlety.inevitability->effectN( 2 ).base_value() / 10.0 );
  p()->buffs.symbols_of_death->extend_duration( p(), extend_duration );
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
  p()->active.lingering_shadow->execute_on_target( state->target, amount );
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
void actions::rogue_action_t<Base>::trigger_sanguine_blades( const action_state_t* state, actions::rogue_attack_t* action )
{
  if ( !p()->talent.assassination.sanguine_blades->ok() || state->result_total <= 0 || !ab::result_is_hit( state->result ) )
    return;

  if ( !action )
    return;

  if ( ( p()->resources.current[ RESOURCE_ENERGY ] / p()->resources.max[ RESOURCE_ENERGY ] ) <=
       p()->talent.assassination.sanguine_blades->effectN( 1 ).percent() )
    return;

  const double additional_cost = p()->talent.assassination.sanguine_blades->effectN( 2 ).base_value();
  p()->resource_loss( RESOURCE_ENERGY, additional_cost, nullptr, action );
  action->stats->consume_resource( RESOURCE_ENERGY, additional_cost );
  p()->sim->print_log( "{} consumes {} {} for {} ({})", *p(), additional_cost, RESOURCE_ENERGY,
                       *action, p()->resources.current[ RESOURCE_ENERGY ] );

  double amount = state->result_amount * p()->talent.assassination.sanguine_blades->effectN( 3 ).percent();
  action->execute_on_target( state->target, amount );
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

  // Target multipliers do not replicate to secondary targets, need to reverse them out
  double target_da_multiplier = ( 1.0 / state->target_da_multiplier );

  // Note: Unmitigated damage as target mitigation is handled on each impact
  double damage = state->result_total * multiplier * target_da_multiplier;
  player_t* primary_target = state->target;

  p()->sim->print_debug( "{} spatters {} for {:.2f} damage ({:.2f} * {} * {:.3f})", *p(), *this, damage, state->result_total, multiplier, target_da_multiplier );

  // Trigger as an event so that this happens after the impact for proc/RPPM targeting purposes
  // Can't use schedule_execute() since multiple damage events can trigger at the same time
  make_event( *p()->sim, 0_ms, [ this, damage, primary_target ]() {
    if ( p()->sim->active_enemies > 1 )
    {
      p()->active.caustic_spatter->execute_on_target( primary_target, damage );
    }
  } );
}

template <typename Base>
void actions::rogue_action_t<Base>::trigger_caustic_spatter_debuff( const action_state_t* state )
{
  if ( !p()->talent.assassination.caustic_spatter->ok() || !ab::result_is_hit( state->result ) )
    return;

  // Caustic Spatter is checked after impacts have an opportunity to trigger poisons
  auto tdata = td( state->target );
  if ( tdata->dots.rupture->is_ticking() && tdata->dots.deadly_poison->is_ticking() )
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

  if ( !p()->buffs.symbols_of_death->check() )
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
bool actions::rogue_action_t<Base>::trigger_t31_subtlety_set_bonus( const action_state_t* state, rogue_attack_t* action )
{
  if ( !p()->set_bonuses.t31_subtlety_2pc->ok() )
    return false;

  if ( !( action && action->secondary_trigger_type == secondary_trigger::T31_SUBTLETY_2PC ||
          affected_by.t31_subtlety_4pc ) )
    return false;

  // Only triggers on a single impact for Black Powder
  if ( state->chain_target > 0 )
    return false;

  int num_clones = 0;
  if ( action && p()->rng().roll( p()->spec.t31_subtlety_2pc_chance ) )
  {
    num_clones = 1; // Only count if we are spawning a clone attack
  }
  else if ( affected_by.t31_subtlety_4pc && ab::data().id() == p()->talent.subtlety.secret_technique->id() )
  {
    num_clones = 2; // Both SecTec clones always count for the set bonus
  }

  if ( p()->set_bonuses.t31_subtlety_4pc->ok() && affected_by.t31_subtlety_4pc && num_clones > 0 )
  {
    // Needs to be delayed as consume_resource for finishers doesn't trigger until post-impact
    const int cp_gain = as<int>( p()->set_bonuses.t31_subtlety_4pc->effectN( 2 ).base_value() ) * num_clones;
    make_event( *p()->sim, [ this, cp_gain ] {
      trigger_combo_point_gain( cp_gain, p()->gains.t31_subtlety_4pc );
    } );
  }

  return ( num_clones > 0 );
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
  dots.sepsis                   = target->get_dot( "sepsis", source );
  dots.serrated_bone_spike      = target->get_dot( "serrated_bone_spike_dot", source );
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
  debuffs.shiv = make_buff<damage_buff_t>( *this, "shiv", source->spec.improved_shiv_debuff, false )
    ->set_direct_mod( source->spec.improved_shiv_debuff->effectN( 1 ).percent() );
  debuffs.ghostly_strike = make_buff( *this, "ghostly_strike", source->talent.outlaw.ghostly_strike )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER_SPELLS )
    ->set_tick_behavior( buff_tick_behavior::NONE )
    ->set_cooldown( timespan_t::zero() );
  debuffs.find_weakness = make_buff( *this, "find_weakness", source->spec.find_weakness_debuff )
    ->set_default_value( source->talent.subtlety.find_weakness->effectN( 1 ).percent() )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION );
  debuffs.sting_like_a_bee = make_buff( *this, "sting_like_a_bee", source->spec.sting_like_a_bee_debuff )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN );
  debuffs.flagellation = make_buff( *this, "flagellation", source->spec.flagellation_buff )
    ->set_initial_stack( 1 )
    ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
    ->set_period( timespan_t::zero() )
    ->set_cooldown( timespan_t::zero() );

  // Type-Based Tracking for Accumulators
  bleeds = { dots.deathmark, dots.garrote, dots.garrote_deathmark, dots.internal_bleeding,
             dots.rupture, dots.rupture_deathmark, dots.crimson_tempest, dots.mutilated_flesh,
             dots.serrated_bone_spike };
  poison_dots = { dots.deadly_poison, dots.deadly_poison_deathmark, dots.sepsis, dots.kingsbane };
  poison_debuffs = { debuffs.atrophic_poison, debuffs.crippling_poison, debuffs.numbing_poison,
                     debuffs.wound_poison, debuffs.amplifying_poison, debuffs.amplifying_poison_deathmark };

  // Callbacks ================================================================

  // Venomous Wounds Energy Refund
  if ( source->talent.assassination.venomous_wounds->ok() )
  {
    target->register_on_demise_callback( source, [source](player_t* target) { source->trigger_venomous_wounds_death( target ); } );
  }

  // Sepsis Cooldown Reduction
  if ( source->talent.shared.sepsis->ok() )
  {
    target->register_on_demise_callback( source, [ this, source ]( player_t* /*target*/ ) {
      if ( dots.sepsis->is_ticking() )
      {
        source->cooldowns.sepsis->adjust( -timespan_t::from_seconds( source->talent.shared.sepsis->effectN( 3 ).base_value() ) );
      }
    } );
  }

  // Serrated Bone Spike Charge Refund
  if ( source->talent.assassination.serrated_bone_spike->ok() )
  {
    target->register_on_demise_callback( source, [ this, source ]( player_t* ) {
      if ( dots.serrated_bone_spike->is_ticking() )
      {
        double refund_max = source->cooldowns.serrated_bone_spike->charges - source->cooldowns.serrated_bone_spike->charges_fractional();
        if ( refund_max > 1 )
          source->procs.serrated_bone_spike_refund->occur();
        else if ( refund_max <= 0 )
          source->procs.serrated_bone_spike_waste->occur();
        else
          source->procs.serrated_bone_spike_waste_partial->occur();

        source->cooldowns.serrated_bone_spike->reset( false, 1 );
      }
    } );
  }

  // Flagellation On-Death Buff Trigger
  if ( source->talent.subtlety.flagellation->ok() )
  {
    target->register_on_demise_callback( source, [ this, source ]( player_t* ) {
      if ( debuffs.flagellation->check() )
      {
        source->buffs.flagellation->increment( 10 );
        source->buffs.flagellation->expire(); // Triggers persist buff
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

  if ( a == ATTR_STAMINA )
  {
    // TOCHECK -- This may be a base attribute multiplier now
    am *= 1.0 + talent.outlaw.combat_stamina->effectN( 1 ).percent();
  }

  return am;
}

// rogue_t::composite_melee_speed ===========================================

double rogue_t::composite_melee_speed() const
{
  double h = player_t::composite_melee_speed();

  if ( buffs.slice_and_dice->up() )
  {
    double buff_value = buffs.slice_and_dice->check_value() +
      ( talent.outlaw.swift_slasher->ok() ? ( 1.0 / cache.attack_haste() ) - 1.0 : 0.0 );
    h *= 1.0 / ( 1.0 + buff_value );
  }

  if ( buffs.adrenaline_rush->check() )
  {
    h *= 1.0 / ( 1.0 + buffs.adrenaline_rush->check_value() );
  }

  h *= 1.0 / ( 1.0 + buffs.t31_assassination_2pc->check_stack_value() );

  return h;
}

// rogue_t::composite_melee_haste ==========================================

double rogue_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( talent.subtlety.warning_signs->ok() && buffs.symbols_of_death->up() )
  {
    h /= 1.0 + talent.subtlety.warning_signs->effectN( 1 ).percent();
  }

  return h;
}

// rogue_t::composite_spell_haste ==========================================

double rogue_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( talent.subtlety.warning_signs->ok() && buffs.symbols_of_death->up() )
  {
    h /= 1.0 + talent.subtlety.warning_signs->effectN( 1 ).percent();
  }

  return h;
}

// rogue_t::composite_melee_crit_chance =========================================

double rogue_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  crit += spell.critical_strikes->effectN( 1 ).percent();
  crit += talent.rogue.lethality->effectN( 1 ).percent();

  if ( talent.subtlety.planned_execution->ok() && buffs.symbols_of_death->up() )
  {
    crit += talent.subtlety.planned_execution->effectN( 1 ).percent();
  }

  return crit;
}

// rogue_t::composite_spell_crit_chance =========================================

double rogue_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  crit += spell.critical_strikes->effectN( 1 ).percent();
  crit += talent.rogue.lethality->effectN( 1 ).percent();

  if ( talent.subtlety.planned_execution->ok() && buffs.symbols_of_death->up() )
  {
    crit += talent.subtlety.planned_execution->effectN( 1 ).percent();
  }

  return crit;
}

// rogue_t::composite_damage_versatility ===================================

double rogue_t::composite_damage_versatility() const
{
  double cdv = player_t::composite_damage_versatility();

  cdv += talent.outlaw.thiefs_versatility->effectN( 1 ).percent();

  return cdv;
}

// rogue_t::composite_heal_versatility =====================================

double rogue_t::composite_heal_versatility() const
{
  double chv = player_t::composite_heal_versatility();

  chv += talent.outlaw.thiefs_versatility->effectN( 1 ).percent();

  return chv;
}

// rogue_t::composite_leech ===============================================

double rogue_t::composite_leech() const
{
  double l = player_t::composite_leech();

  l += spell.leeching_poison_buff->effectN( 1 ).percent();

  return l;
}

// rogue_t::matching_gear_multiplier ========================================

double rogue_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_AGILITY )
    return spell.leather_specialization->effectN( 1 ).percent();

  return 0.0;
}

// rogue_t::composite_player_multiplier =====================================

double rogue_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );
  return m;
}

// rogue_t::composite_player_pet_damage_multiplier ==========================

double rogue_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );

  m *= 1.0 + spec.assassination_rogue->effectN( 6 ).percent();
  m *= 1.0 + spec.outlaw_rogue->effectN( 3 ).percent();
  m *= 1.0 + spec.subtlety_rogue->effectN( 8 ).percent();

  return m;
}

// rogue_t::composite_player_target_multiplier ==============================

double rogue_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  rogue_td_t* tdata = get_target_data( target );

  m *= 1.0 + tdata->debuffs.sting_like_a_bee->stack_value();

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
  if ( name == "cold_blood"             ) return new cold_blood_t             ( name, this, options_str );
  if ( name == "crimson_tempest"        ) return new crimson_tempest_t        ( name, this, options_str );
  if ( name == "deathmark"              ) return new deathmark_t              ( name, this, options_str );
  if ( name == "detection"              ) return new detection_t              ( name, this, options_str );
  if ( name == "dispatch"               ) return new dispatch_t               ( name, this, options_str );
  if ( name == "echoing_reprimand"      ) return new echoing_reprimand_t      ( name, this, options_str );
  if ( name == "envenom"                ) return new envenom_t                ( name, this, options_str );
  if ( name == "eviscerate"             ) return new eviscerate_t             ( name, this, options_str );
  if ( name == "fan_of_knives"          ) return new fan_of_knives_t          ( name, this, options_str );
  if ( name == "feint"                  ) return new feint_t                  ( name, this, options_str );
  if ( name == "flagellation"           ) return new flagellation_t           ( name, this, options_str );
  if ( name == "garrote"                ) return new garrote_t                ( name, this, spec.garrote, options_str );
  if ( name == "goremaws_bite"          ) return new goremaws_bite_t          ( name, this, options_str );
  if ( name == "gouge"                  ) return new gouge_t                  ( name, this, options_str );
  if ( name == "ghostly_strike"         ) return new ghostly_strike_t         ( name, this, options_str );
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
  if ( name == "sepsis"                 ) return new sepsis_t                 ( name, this, options_str );
  if ( name == "serrated_bone_spike"    ) return new serrated_bone_spike_t    ( name, this, options_str );
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
  if ( name == "symbols_of_death"       ) return new symbols_of_death_t       ( name, this, options_str );
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
              util::str_compare_ci( split[ 1 ], "crimson_tempest" ) ||
              util::str_compare_ci( split[ 1 ], "serrated_bone_spike" ) ) )
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
  else if ( util::str_compare_ci( name_str, "master_assassin_remains" ) )
  {
    if ( !talent.assassination.master_assassin->ok() )
      return expr_t::create_constant( name_str, 0 );

    return make_fn_expr( name_str, [ this ]() {
      if ( buffs.master_assassin_aura->check() )
      {
        // Shadow Dance has no lingering effect
        if ( buffs.shadow_dance->check() )
          return buffs.shadow_dance->remains();

        timespan_t nominal_duration = timespan_t::from_seconds( talent.assassination.master_assassin->effectN( 1 ).base_value() );
        timespan_t gcd_remains = timespan_t::from_seconds( std::max( ( gcd_ready - sim->current_time() ).total_seconds(), 0.0 ) );
        return gcd_remains + nominal_duration;
      }
      return buffs.master_assassin->remains();
    } );
  }
  else if ( util::str_compare_ci( name_str, "indiscriminate_carnage_remains" ) )
  {
    if ( !talent.assassination.indiscriminate_carnage->ok() )
      return expr_t::create_constant( name_str, 0 );

    return make_fn_expr( name_str, [ this ]() {
      if ( buffs.indiscriminate_carnage_aura->check() )
      {
        // Shadow Dance has no lingering effect
        if ( buffs.shadow_dance->check() )
          return buffs.shadow_dance->remains();

        timespan_t nominal_duration = spec.indiscriminate_carnage_buff->duration();
        timespan_t gcd_remains = timespan_t::from_seconds( std::max( ( gcd_ready - sim->current_time() ).total_seconds(), 0.0 ) );
        return gcd_remains + nominal_duration;
      }
      return buffs.indiscriminate_carnage->remains();
    } );
  }
  else if ( util::str_compare_ci( name_str, "improved_garrote_remains" ) )
  {
    if ( !talent.assassination.improved_garrote->ok() )
      return expr_t::create_constant( name_str, 0 );

    return make_fn_expr( name_str, [this]() {
      timespan_t remains;
      if ( buffs.improved_garrote_aura->check() )
      {
        // Shadow Dance has no lingering effect
        if ( buffs.shadow_dance->check() )
        {
          remains = buffs.shadow_dance->remains();
        }
        else
        {
          timespan_t nominal_duration = buffs.improved_garrote->base_buff_duration;
          timespan_t gcd_remains = timespan_t::from_seconds( std::max( ( gcd_ready - sim->current_time() ).total_seconds(), 0.0 ) );
          remains = gcd_remains + nominal_duration;
        }
      }
      remains = std::max( { remains, buffs.improved_garrote->remains(), buffs.sepsis->remains() } );
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
  else if ( util::str_compare_ci( name_str, "active_bone_spikes" ) )
  {
    if ( !talent.assassination.serrated_bone_spike->ok() )
    {
      return expr_t::create_constant( name_str, 0 );
    }

    action_t* action = find_background_action<actions::serrated_bone_spike_t::serrated_bone_spike_dot_t>( "serrated_bone_spike_dot" );

    if ( !action )
    {
      return expr_t::create_constant( name_str, 0 );
    }

    return make_fn_expr( name_str, [ this, action ]() {
      return get_active_dots( action->get_dot() );
    } );
  }
  else if ( util::str_compare_ci( split[ 0 ], "rtb_buffs" ) )
  {
    if ( !spec.roll_the_bones->ok() )
      return expr_t::create_constant( name_str, 0 );

    buffs::roll_the_bones_t* primary = static_cast<buffs::roll_the_bones_t*>( buffs.roll_the_bones );
    std::function<bool( buff_t*, buff_t* )> normal_pred =
      []( buff_t* primary, buff_t* buff ) { return buff->check() && buff->remains() == primary->remains(); };
    std::function<bool( buff_t*, buff_t* )> longer_pred =
      []( buff_t* primary, buff_t* buff ) { return buff->check() && buff->remains_gt( primary->remains() ); };
    std::function<bool( buff_t*, buff_t* )> shorter_pred =
      []( buff_t* primary, buff_t* buff ) { return buff->check() && buff->remains_lt( primary->remains() ); };
    std::function<bool( buff_t*, buff_t* )> will_lose_pred =
      []( buff_t* primary, buff_t* buff ) { return buff->check() && !buff->remains_gt( primary->remains() ); };

    if ( split.size() == 1 || ( split.size() == 2 && util::str_compare_ci( split[ 1 ], "total" ) ) )
    {
      // Return the total amount of RtB buffs regardless of duration
      return make_fn_expr( name_str, [ primary ]() {
        double n_buffs = 0;
        for ( auto buff : primary->buffs )
          n_buffs += buff->check();
        return n_buffs;
      } );
    }
    else if ( split.size() == 2 && util::str_compare_ci( split[ 1 ], "max_remains" ) )
    {
      return make_fn_expr( name_str, [ primary ]() {
        timespan_t remains = 0_s;
        for ( auto buff : primary->buffs )
          remains = std::max( remains, buff->remains() );
        return remains.total_seconds();
      } );
    }
    else if ( split.size() == 2 && util::str_compare_ci( split[ 1 ], "min_remains" ) )
    {
      return make_fn_expr( name_str, [ primary ]() {
        timespan_t remains = 0_s;
        for ( auto buff : primary->buffs )
          if ( remains == 0_s || buff->remains_lt( remains ) )
            remains = buff->remains();
        return remains.total_seconds();
      } );
    }
    else if ( ( util::str_compare_ci( split[ 1 ], "longer" ) ||
                util::str_compare_ci( split[ 1 ], "shorter" ) ||
                util::str_compare_ci( split[ 1 ], "normal" ) ||
                util::str_compare_ci( split[ 1 ], "will_lose" ) ||
                util::str_compare_ci( split[ 1 ], "will_retain" ) ) )
    {
      buff_t* filter_buff = nullptr;
      if ( split.size() == 3 )
      {
        util::string_view buff_name = split[ 2 ];
        auto it = range::find_if( primary->buffs, [buff_name]( const buff_t* buff ) {
          return util::str_compare_ci( buff->name_str, buff_name ); } );

        if ( it == primary->buffs.end() )
          throw std::invalid_argument( fmt::format( "Invalid rtb_buffs.{} buff name given '{}'.", split[ 1 ], buff_name ) );
        else
          filter_buff = ( *it );
      }

      std::function<bool( buff_t*, buff_t* )> pred = normal_pred;
      if ( util::str_compare_ci( split[ 1 ], "shorter" ) )
        pred = shorter_pred;
      else if ( util::str_compare_ci( split[ 1 ], "longer" ) || util::str_compare_ci( split[ 1 ], "will_retain" ) )
        pred = longer_pred;
      else if ( util::str_compare_ci( split[ 1 ], "will_lose" ) )
        pred = will_lose_pred;

      return make_fn_expr( name_str, [ primary, pred, filter_buff ]() {
        double n_buffs = 0;
        for ( auto buff : primary->buffs )
          n_buffs += ( !filter_buff || filter_buff == buff ) && pred( primary, buff );
        return n_buffs;
      } );
    }
  }
  else if ( util::str_compare_ci(name_str, "priority_rotation") )
  {
    return expr_t::create_constant( name_str, options.priority_rotation );
  }
  else if ( util::str_compare_ci( name_str, "prepull_shadowdust" ) )
  {
    return expr_t::create_constant( name_str, options.prepull_shadowdust );
  }

  // Split expressions

  // stealthed.(rogue|mantle|basic|sepsis|all)
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
    else if ( util::str_compare_ci( split[ 1 ], "sepsis" ) )
    {
      return make_fn_expr( split[ 0 ], [ this ]() {
        return stealthed( STEALTH_SEPSIS );
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
  // rtb_list.<buffs>
  else if ( split.size() == 3 && util::str_compare_ci( split[ 0 ], "rtb_list" ) && ! split[ 1 ].empty() )
  {
    enum class rtb_list_type
    {
      NONE = 0U,
      ANY,
      ALL
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
    rtb_list_type type = rtb_list_type::NONE;
    if ( util::str_compare_ci( split[ 1 ], "any" ) )
    {
      type = rtb_list_type::ANY;
    }
    else if ( util::str_compare_ci( split[ 1 ], "all" ) )
    {
      type = rtb_list_type::ALL;
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
    if ( type != rtb_list_type::NONE && !list_values.empty() )
    {
      return make_fn_expr( split[ 0 ], [ type, rtb_buffs, list_values ]() {
        for ( unsigned int list_value : list_values )
        {
          if ( type == rtb_list_type::ANY && rtb_buffs[ list_value ]->check() )
          {
            return 1;
          }
          else if ( type == rtb_list_type::ALL && !rtb_buffs[ list_value ]->check() )
          {
            return 0;
          }
        }

        return type == rtb_list_type::ANY ? 0 : 1;
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

        energy_regen_per_second += buffs.buried_treasure->check_value();

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

  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;
  base.spell_power_per_intellect = 1.0;

  resources.base[ RESOURCE_COMBO_POINT ] = 5;
  resources.base[ RESOURCE_COMBO_POINT ] += talent.rogue.deeper_stratagem->effectN( 2 ).base_value();
  resources.base[ RESOURCE_COMBO_POINT ] += talent.outlaw.devious_stratagem->effectN( 2 ).base_value();
  resources.base[ RESOURCE_COMBO_POINT ] += talent.subtlety.secret_stratagem->effectN( 2 ).base_value();

  resources.base[ RESOURCE_ENERGY ] = 100;
  resources.base[ RESOURCE_ENERGY ] += talent.rogue.vigor->effectN( 1 ).base_value();
  resources.base[ RESOURCE_ENERGY ] += talent.assassination.path_of_blood->effectN( 1 ).base_value();
  resources.base[ RESOURCE_ENERGY ] += spec.assassination_rogue->effectN( 5 ).base_value();

  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *= 1.0 + talent.outlaw.combat_potency->effectN( 1 ).percent();
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *= 1.0 + talent.rogue.vigor->effectN( 2 ).percent();

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

  // Core Class Spells
  spell.ambush = find_class_spell( "Ambush" );
  spell.cheap_shot = find_class_spell( "Cheap Shot" );
  spell.crimson_vial = find_class_spell( "Crimson Vial" );
  spell.crippling_poison = find_class_spell( "Crippling Poison" );
  spell.detection = find_spell( 56814 ); // find_class_spell( "Detection" );
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
  spell.fleet_footed = find_class_spell( "Fleet Footed" );
  spell.leather_specialization = find_spell( 86092 );

  // Assassination Spells
  spec.assassination_rogue = find_specialization_spell( "Assassination Rogue" );
  spec.envenom = find_specialization_spell( "Envenom" );
  spec.fan_of_knives = find_specialization_spell( "Fan of Knives" );
  spec.garrote = find_specialization_spell( "Garrote" );
  spec.mutilate = find_specialization_spell( "Mutilate" );
  spec.poisoned_knife = find_specialization_spell( "Poisoned Knife" );

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
  spec.shadow_dance = find_specialization_spell( "Shadow Dance" );
  spec.shadow_techniques = find_specialization_spell( "Shadow Techniques" );
  spec.shadowstrike = find_specialization_spell( "Shadowstrike" );
  spec.shuriken_toss = find_specialization_spell( "Shuriken Toss" );
  spec.shuriken_storm = find_specialization_spell( "Shuriken Storm" );
  spec.symbols_of_death = find_specialization_spell( "Symbols of Death" );

  // Multi-Spec
  spec.rupture = find_specialization_spell( "Rupture" ); // Assassination + Subtlety
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
  talent.rogue.shadowstep = find_talent_spell( talent_tree::CLASS, "Shadowstep" );

  talent.rogue.master_poisoner = find_talent_spell( talent_tree::CLASS, "Master Poisoner" );
  talent.rogue.cheat_death = find_talent_spell( talent_tree::CLASS, "Cheat Death" );
  talent.rogue.elusiveness = find_talent_spell( talent_tree::CLASS, "Elusiveness" );
  talent.rogue.tricks_of_the_trade = find_talent_spell( talent_tree::CLASS, "Tricks of the Trade" );
  talent.rogue.blackjack = find_talent_spell( talent_tree::CLASS, "Blackjack" );

  talent.rogue.numbing_poison = find_talent_spell( talent_tree::CLASS, "Numbing Poison" );
  talent.rogue.atrophic_poison = find_talent_spell( talent_tree::CLASS, "Atrophic Poison" );
  talent.rogue.nimble_fingers = find_talent_spell( talent_tree::CLASS, "Nimble Fingers" );
  talent.rogue.improved_sprint = find_talent_spell( talent_tree::CLASS, "Improved Sprint" );
  talent.rogue.shadowrunner = find_talent_spell( talent_tree::CLASS, "Shadowrunner" );


  talent.rogue.improved_wound_poison = find_talent_spell( talent_tree::CLASS, "Improved Wound Poison" );
  talent.rogue.fleet_footed = find_talent_spell( talent_tree::CLASS, "Fleet Footed" );
  talent.rogue.iron_stomach = find_talent_spell( talent_tree::CLASS, "Iron Stomach" );
  talent.rogue.unbreakable_stride = find_talent_spell( talent_tree::CLASS, "Unbreakable Stride" );
  talent.rogue.featherfoot = find_talent_spell( talent_tree::CLASS, "Featherfoot" );
  talent.rogue.rushed_setup = find_talent_spell( talent_tree::CLASS, "Rushed Setup" );
  talent.rogue.subterfuge = find_talent_spell( talent_tree::CLASS, "Subterfuge" );

  talent.rogue.superior_mixture = find_talent_spell( talent_tree::CLASS, "Superior Mixture" );
  talent.rogue.deadened_nerves = find_talent_spell( talent_tree::CLASS, "Deadened Nerves" );
  talent.rogue.graceful_guile = find_talent_spell( talent_tree::CLASS, "Graceful Guile" );
  talent.rogue.stillshroud = find_talent_spell( talent_tree::CLASS, "Stillshroud" );

  talent.rogue.deadly_precision = find_talent_spell( talent_tree::CLASS, "Deadly Precision" );
  talent.rogue.virulent_poisons = find_talent_spell( talent_tree::CLASS, "Virulent Poisons" );
  talent.rogue.recuperator = find_talent_spell( talent_tree::CLASS, "Recuperator" );
  talent.rogue.improved_ambush = find_talent_spell( talent_tree::CLASS, "Improved Ambush" );
  talent.rogue.tight_spender = find_talent_spell( talent_tree::CLASS, "Tight Spender" );

  talent.rogue.leeching_poison = find_talent_spell( talent_tree::CLASS, "Leeching Poison" );
  talent.rogue.lethality = find_talent_spell( talent_tree::CLASS, "Lethality" );
  talent.rogue.acrobatic_strikes = find_talent_spell( talent_tree::CLASS, "Acrobatic Strikes" );
  talent.rogue.alacrity = find_talent_spell( talent_tree::CLASS, "Alacrity" );
  talent.rogue.soothing_darkness = find_talent_spell( talent_tree::CLASS, "Soothing Darkness" );

  talent.rogue.vigor = find_talent_spell( talent_tree::CLASS, "Vigor" );
  talent.rogue.echoing_reprimand = find_talent_spell( talent_tree::CLASS, "Echoing Reprimand" );
  talent.rogue.nightstalker = find_talent_spell( talent_tree::CLASS, "Nightstalker" );

  talent.rogue.thistle_tea = find_talent_spell( talent_tree::CLASS, "Thistle Tea" );
  talent.rogue.cold_blood = find_talent_spell( talent_tree::CLASS, "Cold Blood" );
  talent.rogue.resounding_clarity = find_talent_spell( talent_tree::CLASS, "Resounding Clarity" );
  talent.rogue.reverberation = find_talent_spell( talent_tree::CLASS, "Reverberation" );
  talent.rogue.deeper_stratagem = find_talent_spell( talent_tree::CLASS, "Deeper Stratagem" );
  talent.rogue.shadow_dance = find_talent_spell( talent_tree::CLASS, "Shadow Dance" );

  // Assassination Talents
  talent.assassination.deadly_poison = find_talent_spell( talent_tree::SPECIALIZATION, "Deadly Poison" );

  talent.assassination.improved_shiv = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Shiv" );
  talent.assassination.venomous_wounds = find_talent_spell( talent_tree::SPECIALIZATION, "Venomous Wounds" );
  talent.assassination.path_of_blood = find_talent_spell( talent_tree::SPECIALIZATION, "Path of Blood" );

  talent.assassination.cut_to_the_chase = find_talent_spell( talent_tree::SPECIALIZATION, "Cut to the Chase" );
  talent.assassination.improved_poisons = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Poisons" );
  talent.assassination.bloody_mess = find_talent_spell( talent_tree::SPECIALIZATION, "Bloody Mess" );

  talent.assassination.thrown_precision = find_talent_spell( talent_tree::SPECIALIZATION, "Thrown Precision" );
  talent.assassination.seal_fate = find_talent_spell( talent_tree::SPECIALIZATION, "Seal Fate" );
  talent.assassination.caustic_spatter = find_talent_spell( talent_tree::SPECIALIZATION, "Caustic Spatter" );
  talent.assassination.internal_bleeding = find_talent_spell( talent_tree::SPECIALIZATION, "Internal Bleeding" );
  talent.assassination.improved_garrote = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Garrote" );

  talent.assassination.crimson_tempest = find_talent_spell( talent_tree::SPECIALIZATION, "Crimson Tempest" );
  talent.assassination.lightweight_shiv = find_talent_spell( talent_tree::SPECIALIZATION, "Lightweight Shiv" );
  talent.assassination.deathmark = find_talent_spell( talent_tree::SPECIALIZATION, "Deathmark" );
  talent.assassination.sanguine_blades = find_talent_spell( talent_tree::SPECIALIZATION, "Sanguine Blades" );
  talent.assassination.master_assassin = find_talent_spell( talent_tree::SPECIALIZATION, "Master Assassin" );

  talent.assassination.flying_daggers = find_talent_spell( talent_tree::SPECIALIZATION, "Flying Daggers" );
  talent.assassination.venom_rush = find_talent_spell( talent_tree::SPECIALIZATION, "Venom Rush" );
  talent.assassination.vicious_venoms = find_talent_spell( talent_tree::SPECIALIZATION, "Vicious Venoms" );
  talent.assassination.fatal_concoction = find_talent_spell( talent_tree::SPECIALIZATION, "Fatal Concoction" );
  talent.assassination.lethal_dose = find_talent_spell( talent_tree::SPECIALIZATION, "Lethal Dose" );
  talent.assassination.intent_to_kill = find_talent_spell( talent_tree::SPECIALIZATION, "Intent to Kill" );
  talent.assassination.iron_wire = find_talent_spell( talent_tree::SPECIALIZATION, "Iron Wire" );

  talent.assassination.systemic_failure = find_talent_spell( talent_tree::SPECIALIZATION, "Systemic Failure" );
  talent.assassination.amplifying_poison = find_talent_spell( talent_tree::SPECIALIZATION, "Amplifying Poison" );
  talent.assassination.twist_the_knife = find_talent_spell( talent_tree::SPECIALIZATION, "Twist the Knife" );
  talent.assassination.doomblade = find_talent_spell( talent_tree::SPECIALIZATION, "Doomblade" );

  talent.assassination.blindside = find_talent_spell( talent_tree::SPECIALIZATION, "Blindside" );
  talent.assassination.tiny_toxic_blade = find_talent_spell( talent_tree::SPECIALIZATION, "Tiny Toxic Blade" );
  talent.assassination.dashing_scoundrel = find_talent_spell( talent_tree::SPECIALIZATION, "Dashing Scoundrel" );
  talent.assassination.shrouded_suffocation = find_talent_spell( talent_tree::SPECIALIZATION, "Shrouded Suffocation" );
  talent.assassination.sepsis = find_talent_spell( talent_tree::SPECIALIZATION, "Sepsis", ROGUE_ASSASSINATION );
  talent.assassination.serrated_bone_spike = find_talent_spell( talent_tree::SPECIALIZATION, "Serrated Bone Spike" );

  talent.assassination.zoldyck_recipe = find_talent_spell( talent_tree::SPECIALIZATION, "Zoldyck Recipe" );
  talent.assassination.poison_bomb = find_talent_spell( talent_tree::SPECIALIZATION, "Poison Bomb" );
  talent.assassination.scent_of_blood = find_talent_spell( talent_tree::SPECIALIZATION, "Scent of Blood" );

  talent.assassination.arterial_precision = find_talent_spell( talent_tree::SPECIALIZATION, "Arterial Precision" );
  talent.assassination.kingsbane = find_talent_spell( talent_tree::SPECIALIZATION, "Kingsbane" );
  talent.assassination.dragon_tempered_blades = find_talent_spell( talent_tree::SPECIALIZATION, "Dragon-Tempered Blades" );
  talent.assassination.indiscriminate_carnage = find_talent_spell( talent_tree::SPECIALIZATION, "Indiscriminate Carnage" );
  talent.assassination.sudden_demise = find_talent_spell( talent_tree::SPECIALIZATION, "Sudden Demise" );

  // Outlaw Talents
  talent.outlaw.opportunity = find_talent_spell( talent_tree::SPECIALIZATION, "Opportunity" );

  talent.outlaw.adrenaline_rush = find_talent_spell( talent_tree::SPECIALIZATION, "Adrenaline Rush" );

  talent.outlaw.retractable_hook = find_talent_spell( talent_tree::SPECIALIZATION, "Retractable Hook" );
  talent.outlaw.dirty_tricks = find_talent_spell( talent_tree::SPECIALIZATION, "Dirty Tricks" );
  talent.outlaw.combat_potency = find_talent_spell( talent_tree::SPECIALIZATION, "Combat Potency" );
  talent.outlaw.combat_stamina = find_talent_spell( talent_tree::SPECIALIZATION, "Combat Stamina" );
  talent.outlaw.hit_and_run = find_talent_spell( talent_tree::SPECIALIZATION, "Hit and Run" );

  talent.outlaw.blinding_powder = find_talent_spell( talent_tree::SPECIALIZATION, "Blinding Powder" );
  talent.outlaw.float_like_a_butterfly = find_talent_spell( talent_tree::SPECIALIZATION, "Float Like a Butterfly" );
  talent.outlaw.sting_like_a_bee = find_talent_spell( talent_tree::SPECIALIZATION, "Sting Like A Bee" );
  talent.outlaw.riposte = find_talent_spell( talent_tree::SPECIALIZATION, "Riposte" );
  talent.outlaw.precision_shot = find_talent_spell( talent_tree::SPECIALIZATION, "Precision Shot" );

  talent.outlaw.heavy_hitter = find_talent_spell( talent_tree::SPECIALIZATION, "Heavy Hitter" );
  talent.outlaw.devious_stratagem = find_talent_spell( talent_tree::SPECIALIZATION, "Devious Stratagem" );
  talent.outlaw.thiefs_versatility = find_talent_spell( talent_tree::SPECIALIZATION, "Thief's Versatility" );
  talent.outlaw.fatal_flourish = find_talent_spell( talent_tree::SPECIALIZATION, "Fatal Flourish" );
  talent.outlaw.ambidexterity = find_talent_spell( talent_tree::SPECIALIZATION, "Ambidexterity" );
  talent.outlaw.quick_draw = find_talent_spell( talent_tree::SPECIALIZATION, "Quick Draw" );
  talent.outlaw.deft_maneuvers = find_talent_spell( talent_tree::SPECIALIZATION, "Deft Maneuvers" );

  talent.outlaw.ruthlessness = find_talent_spell( talent_tree::SPECIALIZATION, "Ruthlessness" );
  talent.outlaw.swift_slasher = find_talent_spell( talent_tree::SPECIALIZATION, "Swift Slasher" );
  talent.outlaw.loaded_dice = find_talent_spell( talent_tree::SPECIALIZATION, "Loaded Dice" );
  talent.outlaw.sleight_of_hand = find_talent_spell( talent_tree::SPECIALIZATION, "Sleight of Hand" );
  talent.outlaw.ace_up_your_sleeve = find_talent_spell( talent_tree::SPECIALIZATION, "Ace Up Your Sleeve" );
  talent.outlaw.improved_between_the_eyes = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Between the Eyes" );

  talent.outlaw.audacity = find_talent_spell( talent_tree::SPECIALIZATION, "Audacity" );
  talent.outlaw.triple_threat = find_talent_spell( talent_tree::SPECIALIZATION, "Triple Threat" );
  talent.outlaw.improved_adrenaline_rush = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Adrenaline Rush" );
  talent.outlaw.improved_main_gauche = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Main Gauche" );
  talent.outlaw.dancing_steel = find_talent_spell( talent_tree::SPECIALIZATION, "Dancing Steel" );

  talent.outlaw.underhanded_upper_hand = find_talent_spell( talent_tree::SPECIALIZATION, "Underhanded Upper Hand" );
  talent.outlaw.sepsis = find_talent_spell( talent_tree::SPECIALIZATION, "Sepsis", ROGUE_OUTLAW );
  
  // TODO: ghostly_strike is duplicated in `trait_data_ptr.inc`, making the talent unable to be loaded by name (since there are two `Ghostly Strike` talents!)
  //  `find_talent_spell` normally uses the id of the first match, but in this case, it's broken, and it's the second match that works. So we do the data
  //  load manually to prefer the second, functional, talent node.
  talent.outlaw.ghostly_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Ghostly Strike" );
  if ( specialization() == ROGUE_OUTLAW && !talent.outlaw.ghostly_strike->ok() )
  {
    uint32_t class_idx, spec_idx;
    dbc->spec_idx( ROGUE_OUTLAW, class_idx, spec_idx );
    auto traits = trait_data_t::find_by_spell( talent_tree::SPECIALIZATION, 196937, class_idx, ROGUE_OUTLAW, dbc->ptr );
    for ( auto trait : traits )
    {
      auto it = range::find_if( player_traits, [ trait ]( const auto& entry ) {
        return std::get<1>( entry ) == trait->id_trait_node_entry;
      } );

      if ( it != player_traits.end() && std::get<2>( *it ) != 0U )
      {
        talent.outlaw.ghostly_strike = find_talent_spell( trait->id_trait_node_entry, ROGUE_OUTLAW );
        break;
      }
    }
  }

  talent.outlaw.count_the_odds = find_talent_spell( talent_tree::SPECIALIZATION, "Count the Odds" );
  talent.outlaw.blade_rush = find_talent_spell( talent_tree::SPECIALIZATION, "Blade Rush" );
  talent.outlaw.precise_cuts = find_talent_spell( talent_tree::SPECIALIZATION, "Precise Cuts" );

  talent.outlaw.take_em_by_surprise = find_talent_spell( talent_tree::SPECIALIZATION, "Take 'em by Surprise" );
  talent.outlaw.summarily_dispatched = find_talent_spell( talent_tree::SPECIALIZATION, "Summarily Dispatched" );
  talent.outlaw.fan_the_hammer = find_talent_spell( talent_tree::SPECIALIZATION, "Fan the Hammer" );

  talent.outlaw.hidden_opportunity = find_talent_spell( talent_tree::SPECIALIZATION, "Hidden Opportunity" );
  talent.outlaw.crackshot = find_talent_spell( talent_tree::SPECIALIZATION, "Crackshot" );
  talent.outlaw.keep_it_rolling = find_talent_spell( talent_tree::SPECIALIZATION, "Keep it Rolling" );
  talent.outlaw.killing_spree = find_talent_spell( talent_tree::SPECIALIZATION, "Killing Spree" );
  talent.outlaw.greenskins_wickers = find_talent_spell( talent_tree::SPECIALIZATION, "Greenskin's Wickers" );

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
  talent.subtlety.without_a_trace = find_talent_spell( talent_tree::SPECIALIZATION, "Without a Trace" );
  talent.subtlety.fade_to_nothing = find_talent_spell( talent_tree::SPECIALIZATION, "Fade to Nothing" );
  talent.subtlety.cloaked_in_shadow = find_talent_spell( talent_tree::SPECIALIZATION, "Cloaked in Shadow" );
  talent.subtlety.night_terrors = find_talent_spell( talent_tree::SPECIALIZATION, "Night Terrors" );
  talent.subtlety.terrifying_pace = find_talent_spell( talent_tree::SPECIALIZATION, "Terrifying Pace" );

  talent.subtlety.swift_death = find_talent_spell( talent_tree::SPECIALIZATION, "Swift Death" );
  talent.subtlety.improved_shadow_techniques = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Shadow Techniques" );
  talent.subtlety.gloomblade = find_talent_spell( talent_tree::SPECIALIZATION, "Gloomblade" );
  talent.subtlety.the_first_dance = find_talent_spell( talent_tree::SPECIALIZATION, "The First Dance" );
  talent.subtlety.master_of_shadows = find_talent_spell( talent_tree::SPECIALIZATION, "Master of Shadows" );
  talent.subtlety.secret_technique = find_talent_spell( talent_tree::SPECIALIZATION, "Secret Technique" );
  talent.subtlety.relentless_strikes = find_talent_spell( talent_tree::SPECIALIZATION, "Relentless Strikes" );
  talent.subtlety.silent_storm = find_talent_spell( talent_tree::SPECIALIZATION, "Silent Storm" );

  talent.subtlety.premeditation = find_talent_spell( talent_tree::SPECIALIZATION, "Premeditation" );
  talent.subtlety.planned_execution = find_talent_spell( talent_tree::SPECIALIZATION, "Planned Execution" );
  talent.subtlety.warning_signs = find_talent_spell( talent_tree::SPECIALIZATION, "Warning Signs" );
  talent.subtlety.improved_shadow_dance = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Shadow Dance" );
  talent.subtlety.shadowed_finishers = find_talent_spell( talent_tree::SPECIALIZATION, "Shadowed Finishers" );
  talent.subtlety.secret_stratagem = find_talent_spell( talent_tree::SPECIALIZATION, "Secret Stratagem" );
  talent.subtlety.replicating_shadows = find_talent_spell( talent_tree::SPECIALIZATION, "Replicating Shadows" );

  talent.subtlety.weaponmaster = find_talent_spell( talent_tree::SPECIALIZATION, "Weaponmaster", ROGUE_SUBTLETY );
  talent.subtlety.shadow_focus = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow Focus" );
  talent.subtlety.deepening_shadows = find_talent_spell( talent_tree::SPECIALIZATION, "Deepening Shadows" );
  talent.subtlety.veiltouched = find_talent_spell( talent_tree::SPECIALIZATION, "Veiltouched" );
  talent.subtlety.shuriken_tornado = find_talent_spell( talent_tree::SPECIALIZATION, "Shuriken Tornado" );

  talent.subtlety.inevitability = find_talent_spell( talent_tree::SPECIALIZATION, "Inevitability" );
  talent.subtlety.sepsis = find_talent_spell( talent_tree::SPECIALIZATION, "Sepsis", ROGUE_SUBTLETY );
  talent.subtlety.perforated_veins = find_talent_spell( talent_tree::SPECIALIZATION, "Perforated Veins" );
  talent.subtlety.lingering_shadow = find_talent_spell( talent_tree::SPECIALIZATION, "Lingering Shadow" );
  talent.subtlety.deeper_daggers = find_talent_spell( talent_tree::SPECIALIZATION, "Deeper Daggers" );
  talent.subtlety.flagellation = find_talent_spell( talent_tree::SPECIALIZATION, "Flagellation" );

  talent.subtlety.invigorating_shadowdust = find_talent_spell( talent_tree::SPECIALIZATION, "Invigorating Shadowdust" );
  talent.subtlety.dark_shadow = find_talent_spell( talent_tree::SPECIALIZATION, "Dark Shadow" );
  talent.subtlety.finality = find_talent_spell( talent_tree::SPECIALIZATION, "Finality" );

  talent.subtlety.the_rotten = find_talent_spell( talent_tree::SPECIALIZATION, "The Rotten" );
  talent.subtlety.shadowcraft = find_talent_spell( talent_tree::SPECIALIZATION, "Shadowcraft" );
  talent.subtlety.danse_macabre = find_talent_spell( talent_tree::SPECIALIZATION, "Danse Macabre" );
  talent.subtlety.goremaws_bite = find_talent_spell( talent_tree::SPECIALIZATION, "Goremaw's Bite" );
  talent.subtlety.dark_brew = find_talent_spell( talent_tree::SPECIALIZATION, "Dark Brew" );

  // Shared Talents
  spell.shadow_dance = find_spell( 185313 );  // Base spell with 0 charges
  spell.shadowstep = find_spell( 36554 );     // Base spell with 0 charges

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

  talent.shared.sepsis = find_shared_talent( { &talent.assassination.sepsis, &talent.outlaw.sepsis, &talent.subtlety.sepsis } );

  // Class Background Spells
  spell.alacrity_buff = talent.rogue.alacrity->ok() ? find_spell( 193538 ) : spell_data_t::not_found();
  spell.echoing_reprimand = talent.rogue.echoing_reprimand;
  spell.leeching_poison_buff = talent.rogue.leeching_poison->ok() ? find_spell( 108211 ) : spell_data_t::not_found();
  spell.nightstalker_buff = talent.rogue.nightstalker->ok() ? find_spell( 130493 ) : spell_data_t::not_found();
  spell.recuperator_heal = talent.rogue.recuperator->ok() ? find_spell( 426605 ) : spell_data_t::not_found();
  spell.sepsis_buff = talent.shared.sepsis->ok() ? find_spell( 375939 ) : spell_data_t::not_found();
  spell.sepsis_expire_damage = talent.shared.sepsis->ok() ? find_spell( 394026 ) : spell_data_t::not_found();
  spell.shadowstep_buff = ( talent.rogue.shadowstep->ok() || spec.shadowstep->ok() ) ? find_spell( 36554 ) : spell_data_t::not_found();
  spell.subterfuge_buff = ( talent.rogue.subterfuge->ok() || talent.outlaw.underhanded_upper_hand->ok() ) ? find_spell( 115192 ) : spell_data_t::not_found();
  spell.vanish_buff = spell.vanish->ok() ? find_spell( 11327 ) : spell_data_t::not_found();

  // Spec Background Spells
  // Assassination
  spec.amplifying_poison_debuff = talent.assassination.amplifying_poison->effectN( 3 ).trigger();
  spec.blindside_buff = talent.assassination.blindside->ok() ? find_spell( 121153 ) : spell_data_t::not_found();
  spec.caustic_spatter_buff = talent.assassination.caustic_spatter->ok() ? find_spell( 421976 ) : spell_data_t::not_found();
  spec.caustic_spatter_damage = talent.assassination.caustic_spatter->ok() ? find_spell( 421979 ) : spell_data_t::not_found();
  spec.dashing_scoundrel = talent.assassination.dashing_scoundrel->ok() ? talent.assassination.dashing_scoundrel : spell_data_t::not_found();
  spec.dashing_scoundrel_gain = spec.dashing_scoundrel->ok() ? find_spell( 340426 )->effectN( 1 ).resource( RESOURCE_ENERGY ) : 0.0;
  spec.deadly_poison_instant = talent.assassination.deadly_poison->ok() ? find_spell( 113780 ) : spell_data_t::not_found();
  spec.doomblade_debuff = talent.assassination.doomblade->ok() ? find_spell( 381672 ) : spell_data_t::not_found();
  spec.improved_garrote_buff = talent.assassination.improved_garrote->ok() ? find_spell( 392401 ) : spell_data_t::not_found();
  spec.improved_shiv_debuff = ( talent.assassination.improved_shiv->ok() || talent.assassination.arterial_precision->ok() ) ? find_spell( 319504 ) : spell_data_t::not_found();
  spec.indiscriminate_carnage_buff = talent.assassination.indiscriminate_carnage->ok() ? find_spell( 385747 ) : spell_data_t::not_found();
  spec.indiscriminate_carnage_buff_aura = talent.assassination.indiscriminate_carnage->ok() ? find_spell( 385754 ) : spell_data_t::not_found();
  spec.internal_bleeding_debuff = talent.assassination.internal_bleeding->ok() ? find_spell( 381628 ) : spell_data_t::not_found();
  spec.kingsbane_buff = talent.assassination.kingsbane->ok() ? find_spell( 394095 ) : spell_data_t::not_found();
  spec.master_assassin_buff = talent.assassination.master_assassin->ok() ? find_spell( 256735 ) : spell_data_t::not_found();
  spec.poison_bomb_driver = talent.assassination.poison_bomb->ok() ? find_spell( 255545 ) : spell_data_t::not_found();
  spec.poison_bomb_damage = talent.assassination.poison_bomb->ok() ? find_spell( 255546 ) : spell_data_t::not_found();
  spec.sanguine_blades_damage = talent.assassination.sanguine_blades->ok() ? find_spell( 423193 ) : spell_data_t::not_found();
  spec.serrated_bone_spike_energize = talent.assassination.serrated_bone_spike->ok() ? find_spell( 328548 ) : spell_data_t::not_found();
  spec.scent_of_blood_buff = talent.assassination.scent_of_blood->ok() ? find_spell( 394080 ) : spell_data_t::not_found();
  spec.venom_rush_energize = talent.assassination.venom_rush->ok() ? find_spell( 256522 ) : spell_data_t::not_found();
  spec.vicious_venoms_ambush = talent.assassination.vicious_venoms->ok() ? find_spell( 385794 ) : spell_data_t::not_found();
  spec.vicious_venoms_mutilate_mh = talent.assassination.vicious_venoms->ok() ? find_spell( 385806 ) : spell_data_t::not_found();
  spec.vicious_venoms_mutilate_oh = talent.assassination.vicious_venoms->ok() ? find_spell( 385802 ) : spell_data_t::not_found();
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
  spec.audacity_buff = talent.outlaw.audacity->ok() ? find_spell( 386270 ) : spell_data_t::not_found();
  spec.blade_flurry_attack = spec.blade_flurry->ok() ? find_spell( 22482 ) : spell_data_t::not_found();
  spec.blade_flurry_instant_attack = spec.blade_flurry->ok() ? 
    ( talent.outlaw.deft_maneuvers->ok() ? find_spell( 429951 ) : find_spell( 331850 ) ) : spell_data_t::not_found();
  spec.blade_rush_attack = talent.outlaw.blade_rush->ok() ? find_spell( 271881 ) : spell_data_t::not_found();
  spec.blade_rush_energize = talent.outlaw.blade_rush->ok() ? find_spell( 271896 ) : spell_data_t::not_found();
  spec.greenskins_wickers = talent.outlaw.greenskins_wickers;
  spec.greenskins_wickers_buff = talent.outlaw.greenskins_wickers->ok() ? find_spell( 394131 ) : spell_data_t::not_found();
  spec.hidden_opportunity_extra_attack = talent.outlaw.hidden_opportunity->ok() ? find_spell( 385897 ) : spell_data_t::not_found();
  spec.improved_adrenaline_rush_energize = talent.outlaw.improved_adrenaline_rush->ok() ? find_spell( 395424 ) : spell_data_t::not_found();
  spec.killing_spree_mh_attack = talent.outlaw.killing_spree->ok() ? find_spell( 57841 ) : spell_data_t::not_found();
  spec.killing_spree_oh_attack = spec.killing_spree_mh_attack->effectN( 1 ).trigger();
  spec.opportunity_buff = talent.outlaw.opportunity->ok() ? find_spell( 195627 ) : spell_data_t::not_found();
  spec.sinister_strike_extra_attack = talent.outlaw.opportunity->ok() ? find_spell( 197834 ) : spell_data_t::not_found();
  spec.summarily_dispatched_buff = talent.outlaw.summarily_dispatched->ok() ? find_spell( 386868 ) : spell_data_t::not_found();
  spec.take_em_by_surprise_buff = talent.outlaw.take_em_by_surprise->ok() ? find_spell( 385907 ) : spell_data_t::not_found();
  spec.triple_threat_attack = talent.outlaw.triple_threat->ok() ? find_spell( 341541 ) : spell_data_t::not_found();
  spec.ace_up_your_sleeve_energize = talent.outlaw.ace_up_your_sleeve->ok() ? find_spell( 394120 ) : spell_data_t::not_found();
  spec.sting_like_a_bee_debuff = talent.outlaw.sting_like_a_bee->ok() ? find_spell( 255909 ) : spell_data_t::not_found();

  spec.broadside = spec.roll_the_bones->ok() ? find_spell( 193356 ) : spell_data_t::not_found();
  spec.buried_treasure = spec.roll_the_bones->ok() ? find_spell( 199600 ) : spell_data_t::not_found();
  spec.grand_melee = spec.roll_the_bones->ok() ? find_spell( 193358 ) : spell_data_t::not_found();
  spec.skull_and_crossbones = spec.roll_the_bones->ok() ? find_spell( 199603 ) : spell_data_t::not_found();
  spec.ruthless_precision = spec.roll_the_bones->ok() ? find_spell( 193357 ) : spell_data_t::not_found();
  spec.true_bearing = spec.roll_the_bones->ok() ? find_spell( 193359 ) : spell_data_t::not_found();

  // Subtlety
  spec.black_powder_shadow_attack = talent.subtlety.shadowed_finishers->ok() ? find_spell( 319190 ) : spell_data_t::not_found();
  spec.danse_macabre_buff = talent.subtlety.danse_macabre->ok() ? find_spell( 393969 ) : spell_data_t::not_found();
  spec.deeper_daggers_buff = talent.subtlety.deeper_daggers->effectN( 1 ).trigger();
  spec.eviscerate_shadow_attack = talent.subtlety.shadowed_finishers->ok() ? find_spell( 328082 ) : spell_data_t::not_found();
  spec.find_weakness_debuff = talent.subtlety.find_weakness->ok() ? find_spell( 316220 ) : spell_data_t::not_found();
  spec.finality_black_powder_buff = talent.subtlety.finality->ok() ? find_spell( 385948 ) : spell_data_t::not_found();
  spec.finality_eviscerate_buff = talent.subtlety.finality->ok() ? find_spell( 385949 ) : spell_data_t::not_found();
  spec.finality_rupture_buff = talent.subtlety.finality->ok() ? find_spell( 385951 ) : spell_data_t::not_found();
  spec.flagellation_buff = talent.subtlety.flagellation;
  spec.flagellation_persist_buff = talent.subtlety.flagellation->ok() ? find_spell( 394758 ) : spell_data_t::not_found();
  spec.flagellation_damage = talent.subtlety.flagellation->ok() ? find_spell( 394757 ) : spell_data_t::not_found();
  spec.goremaws_bite_buff = talent.subtlety.goremaws_bite->effectN( 2 ).trigger();
  spec.invigorating_shadowdust_cdr = talent.subtlety.invigorating_shadowdust;
  spec.lingering_shadow_attack = talent.subtlety.lingering_shadow->ok() ? find_spell( 386081 ) : spell_data_t::not_found();
  spec.lingering_shadow_buff = talent.subtlety.lingering_shadow->ok() ? find_spell( 385960 ) : spell_data_t::not_found();
  spec.master_of_shadows_buff = talent.subtlety.master_of_shadows->ok() ? find_spell( 196980 ) : spell_data_t::not_found();
  spec.perforated_veins_buff = talent.subtlety.perforated_veins->ok() ? find_spell( 426602 ) : spell_data_t::not_found();
  spec.perforated_veins_counter = talent.subtlety.perforated_veins->effectN( 1 ).trigger();
  spec.premeditation_buff = talent.subtlety.premeditation->ok() ? find_spell( 343173 ) : spell_data_t::not_found();
  spec.relentless_strikes_energize = talent.subtlety.relentless_strikes->ok() ? find_spell( 98440 ) : spell_data_t::not_found();
  spec.replicating_shadows_tick = talent.subtlety.replicating_shadows->ok() ? find_spell( 394031 ) : spell_data_t::not_found();
  spec.secret_technique_attack = talent.subtlety.secret_technique->ok() ? find_spell( 280720 ) : spell_data_t::not_found();
  spec.secret_technique_clone_attack = talent.subtlety.secret_technique->ok() ? find_spell( 282449 ) : spell_data_t::not_found();
  spec.shadowstrike_stealth_buff = spec.shadowstrike->ok() ? find_spell( 196911 ) : spell_data_t::not_found();
  spec.shadow_blades_attack = talent.subtlety.shadow_blades->ok() ? find_spell( 279043 ) : spell_data_t::not_found();
  spec.shadow_focus_buff = talent.subtlety.shadow_focus->ok() ? find_spell( 112942 ) : spell_data_t::not_found();
  spec.shadow_techniques_energize = spec.shadow_techniques->ok() ? find_spell( 196911 ) : spell_data_t::not_found();
  spec.shot_in_the_dark_buff = talent.subtlety.shot_in_the_dark->ok() ? find_spell( 257506 ) : spell_data_t::not_found();
  spec.silent_storm_buff = talent.subtlety.silent_storm->ok() ? find_spell( 385727 ) : spell_data_t::not_found();

  // Set Bonus Items ========================================================

  set_bonuses.t29_assassination_2pc = sets->set( ROGUE_ASSASSINATION, T29, B2 );
  set_bonuses.t29_assassination_4pc = sets->set( ROGUE_ASSASSINATION, T29, B4 );
  set_bonuses.t29_outlaw_2pc        = sets->set( ROGUE_OUTLAW, T29, B2 );
  set_bonuses.t29_outlaw_4pc        = sets->set( ROGUE_OUTLAW, T29, B4 );
  set_bonuses.t29_subtlety_2pc      = sets->set( ROGUE_SUBTLETY, T29, B2 );
  set_bonuses.t29_subtlety_4pc      = sets->set( ROGUE_SUBTLETY, T29, B4 );

  set_bonuses.t30_assassination_2pc = sets->set( ROGUE_ASSASSINATION, T30, B2 );
  set_bonuses.t30_assassination_4pc = sets->set( ROGUE_ASSASSINATION, T30, B4 );
  set_bonuses.t30_outlaw_2pc        = sets->set( ROGUE_OUTLAW, T30, B2 );
  set_bonuses.t30_outlaw_4pc        = sets->set( ROGUE_OUTLAW, T30, B4 );
  set_bonuses.t30_subtlety_2pc      = sets->set( ROGUE_SUBTLETY, T30, B2 );
  set_bonuses.t30_subtlety_4pc      = sets->set( ROGUE_SUBTLETY, T30, B4 );

  set_bonuses.t31_assassination_2pc = sets->set( ROGUE_ASSASSINATION, T31, B2 );
  set_bonuses.t31_assassination_4pc = sets->set( ROGUE_ASSASSINATION, T31, B4 );
  set_bonuses.t31_outlaw_2pc        = sets->set( ROGUE_OUTLAW, T31, B2 );
  set_bonuses.t31_outlaw_4pc        = sets->set( ROGUE_OUTLAW, T31, B4 );
  set_bonuses.t31_subtlety_2pc      = sets->set( ROGUE_SUBTLETY, T31, B2 );
  set_bonuses.t31_subtlety_4pc      = sets->set( ROGUE_SUBTLETY, T31, B4 );

  spec.t30_assassination_2pc_tick = set_bonuses.t30_assassination_2pc->ok() ? find_spell( 409483 ) : spell_data_t::not_found();
  spec.t30_assassination_4pc_buff = set_bonuses.t30_assassination_4pc->ok() ? find_spell( 409587 ) : spell_data_t::not_found();
  spec.t30_outlaw_2pc_attack = set_bonuses.t30_outlaw_2pc->ok() ? find_spell( 409604 ) : spell_data_t::not_found();
  spec.t30_outlaw_4pc_attack = set_bonuses.t30_outlaw_4pc->ok() ? find_spell( 409605 ) : spell_data_t::not_found();
  spec.t30_outlaw_4pc_buff = set_bonuses.t30_outlaw_4pc->ok() ? find_spell( 409606 ) : spell_data_t::not_found();
  spec.t30_subtlety_4pc_buff = set_bonuses.t30_subtlety_4pc->ok() ? find_spell( 409987 ) : spell_data_t::not_found();
  spec.t31_assassination_2pc_buff = set_bonuses.t31_assassination_2pc->ok() ? find_spell( 426568 ) : spell_data_t::not_found();
  spec.t31_assassination_4pc_attack = set_bonuses.t31_assassination_4pc->ok() ? find_spell( 426581 ) : spell_data_t::not_found();
  spec.t31_subtlety_2pc_black_powder = set_bonuses.t31_subtlety_2pc->ok() ? find_spell( 424492 ) : spell_data_t::not_found();
  spec.t31_subtlety_2pc_eviscerate = set_bonuses.t31_subtlety_2pc->ok() ? find_spell( 424491 ) : spell_data_t::not_found();
  spec.t31_subtlety_2pc_rupture = set_bonuses.t31_subtlety_2pc->ok() ? find_spell( 424493 ) : spell_data_t::not_found();

  // Active Spells ==========================================================

  auto_attack = new actions::auto_melee_attack_t( this, "" );

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

  if ( talent.assassination.sanguine_blades->ok() )
  {
    active.sanguine_blades.crimson_tempest = get_background_action<actions::sanguine_blades_t>( "sanguine_blades_crimson_tempest" );
    active.sanguine_blades.garrote = get_background_action<actions::sanguine_blades_t>( "sanguine_blades_garrote" );
    active.sanguine_blades.rupture = get_background_action<actions::sanguine_blades_t>( "sanguine_blades_rupture" );
    active.sanguine_blades.deathmark_garrote = get_background_action<actions::sanguine_blades_t>( "sanguine_blades_deathmark_garrote" );
    active.sanguine_blades.deathmark_rupture = get_background_action<actions::sanguine_blades_t>( "sanguine_blades_deathmark_rupture" );
  }

  if ( talent.assassination.vicious_venoms->ok() )
  {
    active.vicious_venoms.ambush = get_secondary_trigger_action<actions::vicious_venoms_t>(
      secondary_trigger::VICIOUS_VENOMS, "ambush_vicious_venoms", spec.vicious_venoms_ambush, false );
    active.vicious_venoms.mutilate_mh = get_secondary_trigger_action<actions::vicious_venoms_t>(
      secondary_trigger::VICIOUS_VENOMS, "mutilate_mh_vicious_venoms", spec.vicious_venoms_mutilate_mh, true );
    active.vicious_venoms.mutilate_oh = get_secondary_trigger_action<actions::vicious_venoms_t>(
      secondary_trigger::VICIOUS_VENOMS, "mutilate_oh_vicious_venoms", spec.vicious_venoms_mutilate_oh, true );
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

  if ( talent.outlaw.triple_threat->ok() )
  {
    active.triple_threat_mh = get_secondary_trigger_action<actions::sinister_strike_t::sinister_strike_extra_attack_t>(
      secondary_trigger::TRIPLE_THREAT, "sinister_strike_triple_threat_mh" );
    active.triple_threat_oh = get_secondary_trigger_action<actions::sinister_strike_t::triple_threat_t>(
      secondary_trigger::TRIPLE_THREAT, "sinister_strike_triple_threat_oh" );
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

  if ( spec.flagellation_damage->ok() )
  {
    active.flagellation = get_secondary_trigger_action<actions::flagellation_damage_t>(
      secondary_trigger::FLAGELLATION, "flagellation_damage" );
  }
}

// rogue_t::init_talents ====================================================

void rogue_t::init_talents()
{
  player_t::init_talents();

  // 2023-10-04 -- Terrible hack to work around the dirty talent tree data and duplicated entries
  // If we find that we are parsing the old Exsanguinate talent, inject the Sanguine Blades talent at the same rank
  const trait_data_t* trait_obj_old = trait_data_t::find_tokenized( talent_tree::SPECIALIZATION, "exsanguinate",
                                                                    util::class_id( type ), specialization(), dbc->ptr );
  auto it = range::find_if( player_traits, [ trait_obj_old ]( const auto& entry ) {
    return std::get<1>( entry ) == trait_obj_old->id_trait_node_entry;
  } );

  unsigned old_rank = ( it != player_traits.end() ) ? std::get<2>( *it ) : 0;
  if ( old_rank > 0 )
  {
    const trait_data_t* trait_obj_new = trait_data_t::find_tokenized( talent_tree::SPECIALIZATION, "sanguine_blades",
                                                                      util::class_id( type ), specialization(), dbc->ptr );
    auto it = range::find_if( player_traits, [ trait_obj_new ]( const auto& entry ) {
      return std::get<1>( entry ) == trait_obj_new->id_trait_node_entry;
    } );

    // Only inject this entry if we didn't actually parse a valid one already
    if ( it == player_traits.end() )
    {
      auto entry = std::make_tuple( talent_tree::SPECIALIZATION, trait_obj_new->id_trait_node_entry, old_rank );
      player_traits.push_back( entry );
    }
  }
}

// rogue_t::init_gains ======================================================

void rogue_t::init_gains()
{
  player_t::init_gains();

  gains.ace_up_your_sleeve              = get_gain( "Ace Up Your Sleeve" );
  gains.adrenaline_rush                 = get_gain( "Adrenaline Rush" );
  gains.adrenaline_rush_expiry          = get_gain( "Adrenaline Rush (Expiry)" );
  gains.blade_rush                      = get_gain( "Blade Rush" );
  gains.broadside                       = get_gain( "Broadside" );
  gains.buried_treasure                 = get_gain( "Buried Treasure" );
  gains.dashing_scoundrel               = get_gain( "Dashing Scoundrel" );
  gains.energy_refund                   = get_gain( "Energy Refund" );
  gains.fatal_flourish                  = get_gain( "Fatal Flourish" );
  gains.improved_adrenaline_rush        = get_gain( "Improved Adrenaline Rush" );
  gains.improved_adrenaline_rush_expiry = get_gain( "Improved Adrenaline Rush (Expiry)" );
  gains.improved_ambush                 = get_gain( "Improved Ambush" );
  gains.master_of_shadows               = get_gain( "Master of Shadows" );
  gains.premeditation                   = get_gain( "Premeditation" );
  gains.quick_draw                      = get_gain( "Quick Draw" );
  gains.relentless_strikes              = get_gain( "Relentless Strikes" );
  gains.ruthlessness                    = get_gain( "Ruthlessness" );
  gains.seal_fate                       = get_gain( "Seal Fate" );
  gains.serrated_bone_spike             = get_gain( "Serrated Bone Spike" );
  gains.shadow_blades                   = get_gain( "Shadow Blades" );
  gains.shadow_techniques               = get_gain( "Shadow Techniques" );
  gains.shadow_techniques_shadowcraft   = get_gain( "Shadow Techniques (Shadowcraft)" );
  gains.shrouded_suffocation            = get_gain( "Shrouded Suffocation" );
  gains.slice_and_dice                  = get_gain( "Slice and Dice" );
  gains.symbols_of_death                = get_gain( "Symbols of Death" );
  gains.symbols_of_death_t30            = get_gain( "Symbols of Death (T30)" );
  gains.the_first_dance                 = get_gain( "The First Dance " );
  gains.venom_rush                      = get_gain( "Venom Rush" );
  gains.venomous_wounds                 = get_gain( "Venomous Vim" );
  gains.venomous_wounds_death           = get_gain( "Venomous Vim (Death)" );
  
  gains.t31_subtlety_4pc                = get_gain( "T31 Subtlety 4pc Bonus" );
}

// rogue_t::init_procs ======================================================

void rogue_t::init_procs()
{
  player_t::init_procs();

  procs.weaponmaster        = get_proc( "Weaponmaster"                 );

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

  procs.deepening_shadows   = get_proc( "Deepening Shadows"            );

  procs.echoing_reprimand.clear();
  procs.echoing_reprimand.push_back( get_proc( "Animacharged CP 2 Used" ) );
  procs.echoing_reprimand.push_back( get_proc( "Animacharged CP 3 Used" ) );
  procs.echoing_reprimand.push_back( get_proc( "Animacharged CP 4 Used" ) );
  procs.echoing_reprimand.push_back( get_proc( "Animacharged CP 5 Used" ) );

  procs.flagellation_cp_spend = get_proc( "CP Spent During Flagellation" );

  procs.serrated_bone_spike_refund            = get_proc( "Serrated Bone Spike Refund" );
  procs.serrated_bone_spike_waste             = get_proc( "Serrated Bone Spike Refund Wasted" );
  procs.serrated_bone_spike_waste_partial     = get_proc( "Serrated Bone Spike Refund Wasted (Partial)" );

  procs.count_the_odds          = get_proc( "Count the Odds" );
  procs.count_the_odds_ambush   = get_proc( "Count the Odds (Ambush)" );
  procs.count_the_odds_ss       = get_proc( "Count the Odds (Sinister Strike)" );
  procs.count_the_odds_dispatch = get_proc( "Count the Odds (Dispatch)" );
  procs.count_the_odds_stealth  = get_proc( "Count the Odds (Stealthed)" );
  procs.count_the_odds_capped   = get_proc( "Count the Odds Capped" );
  procs.roll_the_bones_wasted   = get_proc( "Roll the Bones Wasted" );
  procs.t31_buff_extended       = get_proc( "(T31) Roll the Bones Buff Extended" );
  procs.t31_buff_not_extended   = get_proc( "(T31) Roll the Bones Buff Not Extended" );

  procs.amplifying_poison_consumed           = get_proc( "Amplifying Poison Consumed" );
  procs.amplifying_poison_deathmark_consumed = get_proc( "Amplifying Poison (Deathmark) Consumed" );
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
    ->add_invalidate( CACHE_RUN_SPEED )
    ->apply_affecting_aura( talent.rogue.featherfoot );

  buffs.slice_and_dice = new buffs::slice_and_dice_t( this );
  buffs.stealth = new buffs::stealth_t( this );
  buffs.vanish = new buffs::vanish_t( this );

  // Assassination ==========================================================

  buffs.envenom = make_buff( this, "envenom", spec.envenom )
    ->set_default_value_from_effect_type( A_ADD_FLAT_MODIFIER, P_PROC_CHANCE )
    ->set_duration( timespan_t::min() )
    ->set_period( timespan_t::zero() )
    ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  // Outlaw =================================================================

  buffs.between_the_eyes = make_buff<damage_buff_t>( this, "between_the_eyes", spec.between_the_eyes )
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
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC )
    ->apply_affecting_aura( talent.outlaw.quick_draw )
    ->apply_affecting_aura( talent.outlaw.fan_the_hammer );

  buffs.take_em_by_surprise = make_buff( this, "take_em_by_surprise", spec.take_em_by_surprise_buff )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
    ->apply_affecting_aura( talent.outlaw.take_em_by_surprise ) // Label modifier on talent
    ->set_duration( timespan_t::from_seconds( talent.outlaw.take_em_by_surprise->effectN( 1 ).base_value() ) );
  buffs.take_em_by_surprise_aura = make_buff( this, "take_em_by_surprise_aura", spec.take_em_by_surprise_buff )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
    ->apply_affecting_aura( talent.outlaw.take_em_by_surprise ) // Label modifier on talent
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
    ->set_duration( sim->max_time / 2 ) // So it appears in sample sequence table
    ->set_stack_change_callback( [this]( buff_t*, int, int new_ ) {
    if ( new_ == 0 )
      buffs.take_em_by_surprise->trigger();
    else
      buffs.take_em_by_surprise->expire();
  } );

  buffs.summarily_dispatched = make_buff<damage_buff_t>( this, "summarily_dispatched", spec.summarily_dispatched_buff );
  buffs.summarily_dispatched
    ->set_duration( timespan_t::from_seconds( talent.outlaw.summarily_dispatched->effectN( 3 ).base_value() ) )
    ->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  // Roll the Bones Buffs
  buffs.broadside = make_buff<damage_buff_t>( this, "broadside", spec.broadside );
  buffs.broadside->set_default_value_from_effect( 1, 1.0 ); // Extra CP Gain

  buffs.buried_treasure = make_buff( this, "buried_treasure", spec.buried_treasure )
    ->set_default_value_from_effect_type( A_RESTORE_POWER )
    ->set_affects_regen( true );

  buffs.grand_melee = make_buff<damage_buff_t>( this, "grand_melee", spec.grand_melee );

  buffs.skull_and_crossbones = make_buff( this, "skull_and_crossbones", spec.skull_and_crossbones )
    ->set_default_value_from_effect( 1, 0.01 ); // Flat Modifier to Proc%

  buffs.ruthless_precision = make_buff<damage_buff_t>( this, "ruthless_precision", spec.ruthless_precision, false )
    ->set_crit_chance_mod( spec.ruthless_precision, 1 ); // Non-BtE Crit% Modifier

  buffs.true_bearing = make_buff( this, "true_bearing", spec.true_bearing )
    ->set_default_value_from_effect( 1, 0.1 );  // CDR Seconds

  buffs.roll_the_bones = new buffs::roll_the_bones_t( this );

  // Subtlety ===============================================================

  buffs.danse_macabre = make_buff<damage_buff_t>( this, "danse_macabre", spec.danse_macabre_buff );
  buffs.danse_macabre->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buffs.shadow_blades = make_buff( this, "shadow_blades", talent.subtlety.shadow_blades )
    ->set_default_value_from_effect( 1 ) // Bonus Damage%
    ->set_cooldown( timespan_t::zero() );

  buffs.shadow_dance = new buffs::shadow_dance_t( this );

  buffs.symbols_of_death = make_buff<damage_buff_t>( this, "symbols_of_death", spec.symbols_of_death );
  buffs.symbols_of_death->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
  if ( talent.subtlety.planned_execution->ok() )
  {
    buffs.symbols_of_death->add_invalidate( CACHE_CRIT_CHANCE );
  }
  if ( talent.subtlety.warning_signs->ok() )
  {
    buffs.symbols_of_death->add_invalidate( CACHE_HASTE );
  }

  // Talents ================================================================
  // Shared

  buffs.alacrity = make_buff( this, "alacrity", spell.alacrity_buff )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
    ->set_chance( talent.rogue.alacrity->ok() );

  buffs.cold_blood = make_buff<damage_buff_t>( this, "cold_blood", talent.rogue.cold_blood );
  buffs.cold_blood
    ->set_cooldown( timespan_t::zero() )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
    ->set_duration( sim->max_time / 2 );

  buffs.nightstalker = make_buff<damage_buff_t>( this, "nightstalker", spell.nightstalker_buff );
  buffs.nightstalker->direct_mod.multiplier = 1.0 + talent.rogue.nightstalker->effectN( 1 ).percent();
  buffs.nightstalker->periodic_mod.multiplier = 1.0 + talent.rogue.nightstalker->effectN( 1 ).percent();

  buffs.subterfuge = new buffs::subterfuge_t( this );

  buffs.thistle_tea = make_buff( this, "thistle_tea", talent.rogue.thistle_tea )
    ->set_cooldown( timespan_t::zero() )
    ->set_default_value_from_effect_type( A_MOD_MASTERY_PCT )
    ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY );

  buffs.echoing_reprimand.clear();
  buffs.echoing_reprimand.push_back( make_buff( this, "echoing_reprimand_2", spell.echoing_reprimand->ok() ?
                                                find_spell( 323558 ) : spell_data_t::not_found() )
                                     ->set_max_stack( 1 )
                                     ->set_default_value( 2 ) );
  buffs.echoing_reprimand.push_back( make_buff( this, "echoing_reprimand_3", spell.echoing_reprimand->ok() ?
                                                find_spell( 323559 ) : spell_data_t::not_found() )
                                     ->set_max_stack( 1 )
                                     ->set_default_value( 3 ) );
  buffs.echoing_reprimand.push_back( make_buff( this, "echoing_reprimand_4", spell.echoing_reprimand->ok() ?
                                                find_spell( 323560 ) : spell_data_t::not_found() )
                                     ->set_max_stack( 1 )
                                     ->set_default_value( 4 ) );
  buffs.echoing_reprimand.push_back( make_buff( this, "echoing_reprimand_5", spell.echoing_reprimand->ok() ?
                                                find_spell( 354838 ) : spell_data_t::not_found() )
                                     ->set_max_stack( 1 )
                                     ->set_default_value( 5 ) );

  buffs.sepsis = make_buff( this, "sepsis_buff", spell.sepsis_buff );
  if ( spell.sepsis_buff->ok() )
    buffs.sepsis->set_initial_stack( as<int>( talent.shared.sepsis->effectN( 6 ).base_value() ) );

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
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_RESOURCE_COST );

  buffs.indiscriminate_carnage = make_buff( this, "indiscriminate_carnage", spec.indiscriminate_carnage_buff );
  buffs.indiscriminate_carnage_aura = make_buff( this, "indiscriminate_carnage_aura", spec.indiscriminate_carnage_buff_aura );
  buffs.indiscriminate_carnage_aura
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
    ->set_duration( sim->max_time / 2 ) // So it appears in sample sequence table
    ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
    if ( new_ == 0 )
      buffs.indiscriminate_carnage->trigger();
    else
      buffs.indiscriminate_carnage->expire();
  } );

  buffs.kingsbane = make_buff<damage_buff_t>( this, "kingsbane", spec.kingsbane_buff );
  buffs.kingsbane->set_refresh_behavior( buff_refresh_behavior::NONE );

  buffs.master_assassin = make_buff<damage_buff_t>( this, "master_assassin", spec.master_assassin_buff );
  buffs.master_assassin
    ->set_duration( timespan_t::from_seconds( talent.assassination.master_assassin->effectN( 1 ).base_value() ) );
  buffs.master_assassin_aura = make_buff<damage_buff_t>( this, "master_assassin_aura", spec.master_assassin_buff );
  buffs.master_assassin_aura
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
    ->set_duration( sim->max_time / 2 ) // So it appears in sample sequence table
    ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
      if ( new_ == 0 )
        buffs.master_assassin->trigger();
      else
        buffs.master_assassin->expire();
    } );

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

  buffs.greenskins_wickers = make_buff( this, "greenskins_wickers", spec.greenskins_wickers_buff )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC );

  // Subtlety

  buffs.lingering_shadow = make_buff( this, "lingering_shadow", spec.lingering_shadow_buff )
    ->apply_affecting_aura( talent.subtlety.lingering_shadow ) // Max Stack Modifier
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
    ->apply_affecting_aura( talent.rogue.deeper_stratagem )     // Max stack increase
    ->apply_affecting_aura( talent.subtlety.secret_stratagem )  // Max stack increase
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
    ->set_duration( sim->max_time / 2 );
  buffs.shadow_techniques->reactable = true;

  buffs.shot_in_the_dark = make_buff( this, "shot_in_the_dark", spec.shot_in_the_dark_buff );

  buffs.silent_storm = make_buff( this, "silent_storm", spec.silent_storm_buff )
    ->set_default_value_from_effect_type( A_ADD_FLAT_MODIFIER, P_CRIT )
    ->add_invalidate( CACHE_CRIT_CHANCE )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
    ->set_duration( sim->max_time / 2 );

  buffs.secret_technique = make_buff( this, "secret_technique", talent.subtlety.secret_technique )
    ->set_cooldown( timespan_t::zero() )
    ->set_quiet( true );

  buffs.shuriken_tornado = new buffs::shuriken_tornado_t( this );

  buffs.the_rotten = make_buff<damage_buff_t>( this, "the_rotten", talent.subtlety.the_rotten->effectN( 1 ).trigger() )
    ->set_is_stacking_mod( false );

  buffs.flagellation = make_buff( this, "flagellation_buff", spec.flagellation_buff )
    ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
    ->set_cooldown( timespan_t::zero() )
    ->set_period( timespan_t::zero() )
    ->set_default_value_from_effect_type( A_MOD_MASTERY_PCT )
    ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
    ->set_stack_change_callback( [this]( buff_t*, int old_, int new_ ) {
      if ( new_ == 0 )
        buffs.flagellation_persist->trigger( old_, buffs.flagellation->default_value );
    } );
  buffs.flagellation_persist = make_buff( this, "flagellation_persist", spec.flagellation_persist_buff )
    ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY );

  buffs.deeper_daggers = make_buff( this, "deeper_daggers", spec.deeper_daggers_buff )
    ->set_default_value_from_effect( 1 );

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

  // Talent ranks override the value of the buffs via dummy effects
  buffs.finality_black_powder = make_buff<damage_buff_t>( this, "finality_black_powder", spec.finality_black_powder_buff,
                                                          talent.subtlety.finality->effectN( 1 ).percent() );
  buffs.finality_black_powder->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  buffs.finality_eviscerate = make_buff<damage_buff_t>( this, "finality_eviscerate", spec.finality_eviscerate_buff,
                                                        talent.subtlety.finality->effectN( 1 ).percent() );
  buffs.finality_eviscerate->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  buffs.finality_rupture = make_buff( this, "finality_rupture", spec.finality_rupture_buff )
    ->set_default_value( talent.subtlety.finality->effectN( 1 ).percent() )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  buffs.goremaws_bite = make_buff( this, "goremaws_bite", spec.goremaws_bite_buff )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_RESOURCE_COST );
  buffs.goremaws_bite->set_initial_stack( buffs.goremaws_bite->max_stack() );

  // Set Bonus Items ========================================================
  
  buffs.t29_assassination_4pc = make_buff<damage_buff_t>( this, "septic_wounds", set_bonuses.t29_assassination_4pc->ok() ?
                                                          find_spell( 394845 ) : spell_data_t::not_found() );
  buffs.t29_assassination_4pc->set_trigger_spell( set_bonuses.t29_assassination_4pc ); // Proc Rate

  buffs.t29_outlaw_2pc = make_buff<damage_buff_t>( this, "vicious_followup", set_bonuses.t29_outlaw_2pc->ok() ?
                                                   find_spell( 394879 ) : spell_data_t::not_found() );
  buffs.t29_outlaw_2pc->set_max_stack( as<int>( consume_cp_max() ) );

  buffs.t29_outlaw_4pc = make_buff<damage_buff_t>( this, "brutal_opportunist", set_bonuses.t29_outlaw_4pc->ok() ?
                                                   find_spell( 394888 ) : spell_data_t::not_found() );

  buffs.t29_subtlety_2pc = make_buff<damage_buff_t>( this, "honed_blades", set_bonuses.t29_subtlety_2pc->ok() ?
                                                     find_spell( 394894 ) : spell_data_t::not_found() );
  buffs.t29_subtlety_2pc->set_max_stack( as<int>( consume_cp_max() ) );

  buffs.t30_assassination_4pc = make_buff( this, "poisoned_edges", spec.t30_assassination_4pc_buff )
    ->set_default_value_from_effect( 1 );

  buffs.t30_outlaw_4pc = make_buff( this, "soulripper", spec.t30_outlaw_4pc_buff )
    ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
    ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY );

  // Cannot fully auto-parse as a single damage_buff_t since it has different mod for Black Powder
  const spell_data_t* t29_buff = ( set_bonuses.t29_subtlety_4pc->ok() ?
                                   find_spell( 395003 ) : spell_data_t::not_found() );
  buffs.t29_subtlety_4pc = make_buff<damage_buff_t>( this, "masterful_finish", t29_buff, false )
    ->set_direct_mod( t29_buff, 1 )
    ->set_periodic_mod( t29_buff, 3 );
  buffs.t29_subtlety_4pc_black_powder = make_buff<damage_buff_t>( this, "masterful_finish_bp", t29_buff, false )
    ->set_direct_mod( t29_buff, 2 );
  buffs.t29_subtlety_4pc_black_powder->set_quiet( true );

  buffs.t31_assassination_2pc = make_buff<damage_buff_t>( this, "natureblight", spec.t31_assassination_2pc_buff )
    ->set_is_stacking_mod( true );
  buffs.t31_assassination_2pc->set_default_value_from_effect_type( A_MOD_ATTACKSPEED )
    ->add_invalidate( CACHE_ATTACK_SPEED )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
}

// rogue_t::invalidate_cache =========================================

void rogue_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_HASTE:
      if ( talent.outlaw.swift_slasher->ok() && buffs.slice_and_dice->check() )
      {
        invalidate_cache( CACHE_ATTACK_SPEED );
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
      sim->errorf( "%s: Duplicate 'fixed_rtb' buff %c", sim->active_player->name(), *it );
      return false;
    }

    buffs.push_back( buff_index );
  }

  if ( buffs.empty() || buffs.size() > 6 )
  {
    sim->error( "{}: No valid 'fixed_rtb' buffs given by string '{}'", sim->active_player->name(), value );
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
  add_option( opt_func( "fixed_rtb", parse_fixed_rtb ) );
  add_option( opt_func( "fixed_rtb_odds", parse_fixed_rtb_odds ) );
  add_option( opt_bool( "prepull_shadowdust", options.prepull_shadowdust ) );
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

  options.fixed_rtb = rogue->options.fixed_rtb;
  options.fixed_rtb_odds = rogue->options.fixed_rtb_odds;
  options.rogue_ready_trigger = rogue->options.rogue_ready_trigger;
  options.prepull_shadowdust = rogue->options.prepull_shadowdust;
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

  if ( set_bonuses.t30_outlaw_2pc->ok() )
  {

    auto const soulrip_driver = new special_effect_t( this );
    soulrip_driver->name_str = "soulrip_driver";
    soulrip_driver->spell_id = set_bonuses.t30_outlaw_2pc->id();
    soulrip_driver->proc_flags_ = set_bonuses.t30_outlaw_2pc->proc_flags();
    soulrip_driver->proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
    special_effects.push_back( soulrip_driver );

    auto cb = new actions::soulrip_cb_t( this, *soulrip_driver );
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

  t31_assassination_2pc_accumulator = 0;
  danse_macabre_tracker.clear();

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
  if ( ( talent.rogue.subterfuge->ok() || talent.outlaw.underhanded_upper_hand->ok() ) && !buffs.subterfuge->check() && stealthed( STEALTH_BASIC ) )
  {
    buffs.subterfuge->trigger();
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

  if ( ( stealth_mask & STEALTH_SEPSIS ) && buffs.sepsis->check() )
    return true;

  // Sepsis gives all the benefits of Improved Garrote including CDR on PTR, even without the buff
  if ( ( stealth_mask & STEALTH_IMPROVED_GARROTE ) && talent.assassination.improved_garrote->ok() &&
       ( buffs.improved_garrote->check() || buffs.improved_garrote_aura->check() || buffs.sepsis->check() ) )
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

// rogue_t::temporary_movement_modifier ==================================

double rogue_t::temporary_movement_modifier() const
{
  double temporary = player_t::temporary_movement_modifier();

  if ( buffs.sprint->up() )
    temporary = std::max( buffs.sprint->check_value(), temporary );

  if ( buffs.shadowstep->up() )
    temporary = std::max( buffs.shadowstep->check_value(), temporary );

  return temporary;
}

// rogue_t::passive_movement_modifier===================================

double rogue_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  ms += talent.rogue.fleet_footed->effectN( 1 ).percent();
  ms += spell.fleet_footed->effectN( 1 ).percent(); // DFALPHA: Duplicate passive?
  ms += talent.outlaw.hit_and_run->effectN( 1 ).percent();

  if ( stealthed( ( STEALTH_BASIC | STEALTH_SHADOW_DANCE ) ) )
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

    // Additive energy gains
    if ( buffs.buried_treasure->up() )
      resource_gain( RESOURCE_ENERGY, buffs.buried_treasure -> check_value() * periodicity.total_seconds(), gains.buried_treasure );
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

void rogue_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  action.apply_affecting_aura( spell.all_rogue );
  action.apply_affecting_aura( spec.assassination_rogue );
  action.apply_affecting_aura( spec.outlaw_rogue );
  action.apply_affecting_aura( spec.subtlety_rogue );
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
