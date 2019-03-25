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
  TRIGGER_SINISTER_STRIKE,
  TRIGGER_WEAPONMASTER,
  TRIGGER_SECRET_TECHNIQUE,
  TRIGGER_SHURIKEN_TORNADO,
  TRIGGER_REPLICATING_SHADOWS,
  TRIGGER_INTERNAL_BLEEDING
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

struct rogue_t;
namespace actions
{
struct rogue_attack_state_t;
struct residual_damage_state_t;
struct rogue_poison_t;
struct rogue_attack_t;
struct melee_t;
struct shadow_blades_attack_t;
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
    buff_t* rupture; // Hidden proxy for handling Scent of Blood azerite trait
    buff_t* toxic_blade;
    buff_t* find_weakness;
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
  action_t* replicating_shadows;

  // Autoattacks
  action_t* auto_attack;
  actions::melee_t* melee_main_hand;
  actions::melee_t* melee_off_hand;
  actions::shadow_blades_attack_t* shadow_blades_attack;

  // Track target of last manual Nightblade application for Replicating Shadows azerite power
  player_t* last_nightblade_target;

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
    buff_t* killing_spree;
    buff_t* loaded_dice;
    buff_t* slice_and_dice;
    // Subtlety
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
    cooldown_t* weaponmaster;
    cooldown_t* vendetta;
    cooldown_t* toxic_blade;
    cooldown_t* symbols_of_death;
    cooldown_t* secret_technique;
    cooldown_t* shadow_blades;
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
  } spec;

  // Spell Data
  struct spells_t
  {
    const spell_data_t* poison_bomb_driver;
    const spell_data_t* critical_strikes;
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
    const spell_data_t* find_weakness;
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
  struct azerite_powers_t
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
  } azerite;

  // Procs
  struct procs_t
  {
    // Assassination
    proc_t* seal_fate;

    // Outlaw
    proc_t* sinister_strike_extra_attack;
    proc_t* roll_the_bones_1;
    proc_t* roll_the_bones_2;
    proc_t* roll_the_bones_3;
    proc_t* roll_the_bones_4;
    proc_t* roll_the_bones_5;
    proc_t* roll_the_bones_6;

    // Subtlety
    proc_t* deepening_shadows;
    proc_t* weaponmaster;
  } procs;

  // Options
  int initial_combo_points;
  bool rogue_optimize_expressions;
  bool rogue_ready_trigger;
  bool priority_rotation;

  rogue_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, ROGUE, name, r ),
    shadow_techniques( 0 ),
    poisoned_enemies( 0 ),
    active_blade_flurry( nullptr ),
    active_lethal_poison( nullptr ),
    active_nonlethal_poison( nullptr ),
    active_main_gauche( nullptr ),
    poison_bomb( nullptr ),
    replicating_shadows( nullptr ),
    auto_attack( nullptr ), melee_main_hand( nullptr ), melee_off_hand( nullptr ),
    shadow_blades_attack( nullptr ),
    last_nightblade_target( nullptr ),
    restealth_allowed( false ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    spec( spec_t() ),
    spell( spells_t() ),
    talent( talents_t() ),
    mastery( masteries_t() ),
    procs( procs_t() ),
    initial_combo_points( 0 ),
    rogue_optimize_expressions( true ),
    rogue_ready_trigger( true ),
    priority_rotation( false )
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
    cooldowns.ghostly_strike           = get_cooldown( "ghostly_strike"           );
    cooldowns.grappling_hook           = get_cooldown( "grappling_hook"           );
    cooldowns.marked_for_death         = get_cooldown( "marked_for_death"         );
    cooldowns.riposte                  = get_cooldown( "riposte"                  );
    cooldowns.weaponmaster             = get_cooldown( "weaponmaster"             );
    cooldowns.vendetta                 = get_cooldown( "vendetta"                 );
    cooldowns.toxic_blade              = get_cooldown( "toxic_blade"              );
    cooldowns.symbols_of_death         = get_cooldown( "symbols_of_death"         );
    cooldowns.secret_technique         = get_cooldown( "secret_technique"         );
    cooldowns.shadow_blades            = get_cooldown( "shadow_blades"            );

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
  void      init_items() override;
  void      init_special_effects() override;
  void      init_finished() override;
  void      create_buffs() override;
  void      create_options() override;
  void      copy_from( player_t* source ) override;
  std::string      create_profile( save_e stype ) override;
  void      init_action_list() override;
  void      reset() override;
  void      activate() override;
  void      arise() override;
  void      combat_begin() override;
  void      regen( timespan_t periodicity ) override;
  timespan_t available() const override;
  action_t* create_action( const std::string& name, const std::string& options ) override;
  expr_t*   create_expression( const std::string& name_str ) override;
  resource_e primary_resource() const override { return RESOURCE_ENERGY; }
  role_e    primary_role() const override  { return ROLE_ATTACK; }
  stat_e    convert_hybrid_stat( stat_e s ) const override;

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
  void trigger_elaborate_planning( const action_state_t* );
  void trigger_alacrity( const action_state_t* );
  void trigger_deepening_shadows( const action_state_t* );
  void trigger_shadow_techniques( const action_state_t* );
  void trigger_weaponmaster( const action_state_t* );
  void trigger_restless_blades( const action_state_t* );
  void trigger_exsanguinate( const action_state_t* );
  void trigger_relentless_strikes( const action_state_t* );
  void trigger_shadow_blades_attack( action_state_t* );
  void trigger_inevitability( const action_state_t* state );

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
  bool stealthed( uint32_t stealth_mask = STEALTH_ALL ) const;
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
  weapon_e     requires_weapon_type;
  weapon_e     requires_weapon_group;

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
    bool broadside;
    bool master_assassin;
    bool toxic_blade;
  } affected_by;

  rogue_attack_t( const std::string& token, rogue_t* p,
                  const spell_data_t* s = spell_data_t::nil(),
                  const std::string& options = std::string() ):
    melee_attack_t( token, p, s ),
    requires_stealth( false ), requires_position( POSITION_NONE ),
    requires_weapon_type( WEAPON_NONE ), requires_weapon_group( WEAPON_NONE ),
    secondary_trigger( TRIGGER_NONE )
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
                                data().affected_by( p() -> spec.shadow_blades -> effectN( 3 ) );

    affected_by.ruthlessness = base_costs[ RESOURCE_COMBO_POINT ] > 0;
    affected_by.relentless_strikes = base_costs[ RESOURCE_COMBO_POINT ] > 0;
    affected_by.deepening_shadows = base_costs[ RESOURCE_COMBO_POINT ] > 0;
    affected_by.vendetta = data().affected_by( p() -> spec.vendetta -> effectN( 1 ) );
    affected_by.alacrity = base_costs[ RESOURCE_COMBO_POINT ] > 0;
    affected_by.adrenaline_rush_gcd = data().affected_by( p() -> buffs.adrenaline_rush -> data().effectN( 3 ) );
    affected_by.broadside = data().affected_by( p() -> buffs.broadside -> data().effectN( 4 ) );
    affected_by.master_assassin = data().affected_by( p() -> spec.master_assassin -> effectN( 1 ) );
    affected_by.toxic_blade = data().affected_by( p() -> talent.toxic_blade -> effectN( 4 ).trigger() -> effectN( 1 ) );
  }

  void snapshot_state( action_state_t* state, dmg_e rt ) override
  {
    double max_cp = std::min( player -> resources.current[ RESOURCE_COMBO_POINT ], p() -> consume_cp_max() );
    cast_state( state ) -> cp = static_cast<int>( max_cp );

    melee_attack_t::snapshot_state( state, rt );
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

    return t;
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
      if ( p() -> buffs.broadside -> check() )
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

  virtual double proc_chance_main_gauche() const
  { return p()->mastery.main_gauche->proc_chance(); }

  // Generic rules for snapshotting the Nightstalker pmultiplier, default to DoTs only
  virtual bool snapshots_nightstalker() const
  { 
    return dot_duration > timespan_t::zero() && base_tick_time > timespan_t::zero();
  }

  virtual double combo_point_da_multiplier(const action_state_t* s) const
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return static_cast<double>( cast_state( s ) -> cp );
    return 1.0;
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

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = melee_attack_t::composite_da_multiplier( state );

    m *= combo_point_da_multiplier( state );

    // Subtlety
    if ( p()->mastery.executioner->ok() && data().affected_by( p()->mastery.executioner->effectN( 1 ) ) )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( p()->buffs.symbols_of_death->up() && data().affected_by( p()->buffs.symbols_of_death->data().effectN( 1 ) ) )
    {
      m *= 1.0 + p()->buffs.symbols_of_death->data().effectN( 1 ).percent();
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
    
    if ( p()->buffs.elaborate_planning->up() && data().affected_by( p()->buffs.elaborate_planning->data().effectN( 1 ) ) )
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
      // 08/17/2018 - Mastery: Executioner has a different coefficient for periodic
      if ( p()->bugs )
        m *= 1.0 + p()->cache.mastery() * p()->mastery.executioner->effectN( 2 ).mastery_value();
      else
        m *= 1.0 + p()->cache.mastery_value();
    }

    if ( p()->buffs.symbols_of_death->up() && data().affected_by( p()->buffs.symbols_of_death->data().effectN( 2 ) ) )
    {
      m *= 1.0 + p()->buffs.symbols_of_death->data().effectN( 2 ).percent();
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

    if ( p()->buffs.elaborate_planning->up() && data().affected_by( p()->buffs.elaborate_planning->data().effectN( 2 ) ) )
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
    if ( p()->talent.nightstalker->ok() && snapshots_nightstalker() && p()->stealthed( STEALTH_BASIC | STEALTH_SHADOWDANCE ) )
    {
      m *= 1.0 + ( p()->talent.nightstalker->effectN( 2 ).percent() + p()->spec.subtlety_rogue->effectN( 4 ).percent() );
    }

    return m;
  }

  double action_multiplier() const override
  {
    double m = melee_attack_t::action_multiplier();

    if (affected_by.broadside && p() -> buffs.broadside -> up())
    {
      m *= 1.0 + p() -> buffs.broadside -> data().effectN( 4 ).percent();
    }

    // Apply Nightstalker as an Action Multiplier for things that don't snapshot
    if ( p()->talent.nightstalker->ok() && !snapshots_nightstalker() && p()->stealthed( STEALTH_BASIC | STEALTH_SHADOWDANCE ) )
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
    a *= 1.0 - td( target ) -> debuffs.find_weakness -> value();
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
    // Ensure target is still available and did not demise during delay.
    if ( ( state && state->target && state->target->is_sleeping() ) ||
         ( target && target->is_sleeping() ) )
      return;

    actions::rogue_attack_t* attack = rogue_t::cast_attack( action );
    auto bg = attack -> background, d = attack -> dual, r = attack -> repeating;

    attack -> background = attack -> dual = true;
    attack -> repeating = false;
    attack -> secondary_trigger = source;
    if ( state )
    {
      attack -> set_target( state -> target );
      attack -> pre_execute_state = state;
    }
    // No state, construct one and grab combo points from the event instead of current CP amount.
    else
    {
      attack -> set_target( target );
      action_state_t* s = attack -> get_state();
      actions::rogue_attack_t::cast_state( s ) -> cp = cp;
      // Calling snapshot_internal, snapshot_state would overwrite CP.
      attack -> snapshot_internal( s, attack -> snapshot_flags, attack -> amount_type( s ) );

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
    special = background = may_crit = true;
    proc = true; // it's proc; therefore it cannot trigger main_gauche for chain-procs
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    double ap = rogue_attack_t::attack_direct_power_coefficient( s );

    // Main Gauche is not set up like most masteries as it is a flat mod instead of a percent mod
    // Need to reference the 2nd effect and directly apply the sp_coeff instead of dividing by 100
    ap += p()->cache.mastery() * p()->mastery.main_gauche->effectN( 2 ).sp_coeff();

    return ap;
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

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    // As of 2018-12-08: Keep Your Wits stacks per BF hit per target.
    if ( p() -> azerite.keep_your_wits_about_you.ok() )
      p() -> buffs.keep_your_wits_about_you -> trigger();
  }
};

struct internal_bleeding_t : public rogue_attack_t
{
  internal_bleeding_t( rogue_t* p ) :
    rogue_attack_t( "internal_bleeding", p, p -> find_spell( 154953 ) )
  {
    background = true;
    hasted_ticks = true;
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

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_persistent_multiplier( state );
    m *= cast_state( state ) -> cp;
    return m;
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

    if ( p() -> azerite.double_dose.ok() && ( source_state -> action -> name_str == "mutilate_mh" || source_state -> action -> name_str == "mutilate_oh" ) )
      p() -> buffs.double_dose -> trigger( 1 );
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
  };

  struct deadly_poison_dot_t : public rogue_poison_t
  {
    deadly_poison_dot_t( rogue_t* p ) :
      rogue_poison_t( "deadly_poison_dot", p, p -> find_specialization_spell( "Deadly Poison" ) -> effectN( 1 ).trigger() )
    {
      may_crit       = false;
      proc           = false;
      harmful        = true;
      hasted_ticks   = true;
      callbacks      = true;
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
  p() -> trigger_shadow_blades_attack( state );

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

  if ( p()->talent.shadow_focus->ok() && p()->stealthed( STEALTH_BASIC | STEALTH_SHADOWDANCE ) )
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
}

// rogue_attack_t::execute ==================================================

void rogue_attack_t::execute()
{
  melee_attack_t::execute();

  if ( harmful )
    p() -> restealth_allowed = false;

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

  if ( harmful && p()->stealthed( STEALTH_BASIC | STEALTH_SHADOWMELD ) )
  {
    player -> buffs.shadowmeld -> expire();

    // Check stealthed again after shadowmeld is popped. If we're still stealthed, trigger subterfuge
    if ( p()->talent.subterfuge->ok() && !p()->buffs.subterfuge->check() && p()->stealthed( STEALTH_BASIC ) )
      p()->buffs.subterfuge->trigger();
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

  if ( requires_stealth && !p()->stealthed() )
  {
    return false;
  }

  if ( requires_position != POSITION_NONE )
    if ( p() -> position() != requires_position )
      return false;

  if ( requires_weapon_type != WEAPON_NONE )
  {
    if ( !weapon || weapon->type != requires_weapon_type )
      return false;
  }

  if ( requires_weapon_group != WEAPON_NONE )
  {
    if ( !weapon || weapon->group() != requires_weapon_group )
      return false;
  }

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
      m *= 1.0 + p()->buffs.shadow_dance->data().effectN( 2 ).percent()
        + p()->talent.dark_shadow->effectN( 1 ).percent();
    }

    // Assassination
    if ( p()->buffs.elaborate_planning->up() )
    {
      m *= 1.0 + p()->buffs.elaborate_planning->data().effectN( 3 ).percent();
    }

    return m;
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
  double precombat_seconds;

  adrenaline_rush_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "adrenaline_rush", p, p -> find_specialization_spell( "Adrenaline Rush" ) ),
    precombat_seconds( 0.0 )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = may_miss = may_crit = false;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.adrenaline_rush -> trigger();
    if ( p() -> talent.loaded_dice -> ok() )
      p() -> buffs.loaded_dice -> trigger();
    p() -> buffs.brigands_blitz_driver -> trigger();

    if ( precombat_seconds && ! p() -> in_combat ) {
      timespan_t precombat_lost_seconds = - timespan_t::from_seconds( precombat_seconds );
      p() -> cooldowns.adrenaline_rush -> adjust( precombat_lost_seconds, false );
      p() -> buffs.adrenaline_rush -> extend_duration( p(), precombat_lost_seconds );
      p() -> buffs.loaded_dice -> extend_duration( p(), precombat_lost_seconds );
      p() -> buffs.brigands_blitz_driver -> extend_duration( p(), precombat_lost_seconds );
      if ( p() -> azerite.brigands_blitz.ok() )
        p() -> buffs.brigands_blitz -> trigger( floor( -precombat_lost_seconds / p() -> buffs.brigands_blitz_driver -> buff_period ) );
    }
  }
};

// Ambush ===================================================================

struct ambush_t : public rogue_attack_t
{
  ambush_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "ambush", p, p -> find_specialization_spell( "Ambush" ), options_str )
  {
    may_dodge = may_block = may_parry = false;
    requires_stealth  = true;
  }

  bool procs_main_gauche() const override
  { return false; }

  void execute() override
  {
    rogue_attack_t::execute();
    if ( p() -> buffs.broadside -> up() )
    {
      p() -> trigger_combo_point_gain( p() -> buffs.broadside -> data().effectN( 2 ).base_value(),
          p() -> gains.broadside, this );
    }
  }
};

// Backstab =================================================================

struct backstab_t : public rogue_attack_t
{
  backstab_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "backstab", p, p -> find_specialization_spell( "Backstab" ), options_str )
  {
    requires_weapon_type = WEAPON_DAGGER;
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
    if ( p() -> talent.gloomblade -> ok() )
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
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    p() -> trigger_weaponmaster( state );
    p() -> trigger_inevitability( state );
  }
};

// Between the Eyes =========================================================

struct between_the_eyes_t : public rogue_attack_t
{
  between_the_eyes_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "between_the_eyes", p, p -> find_specialization_spell( "Between the Eyes" ),
                    options_str )
  {
    ap_type = AP_WEAPON_BOTH;
    crit_bonus_multiplier *= 1.0 + p -> find_specialization_spell( 235484 ) -> effectN( 1 ).percent();
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double b = rogue_attack_t::bonus_da( state );

    if ( p() -> azerite.ace_up_your_sleeve.ok() )
      b += p() -> azerite.ace_up_your_sleeve.value(); // CP Mult is applied as a mod later.

    return b;
  }

  double composite_crit_chance() const override
  {
    double c = rogue_attack_t::composite_crit_chance();

    if ( p() -> buffs.ruthless_precision -> up() )
    {
      c += p() -> buffs.ruthless_precision -> data().effectN( 2 ).percent();
    }

    return c;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> trigger_restless_blades( execute_state );

    const rogue_attack_state_t* rs = rogue_attack_t::cast_state( execute_state );
    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> buffs.deadshot -> trigger();

      if ( p() -> azerite.ace_up_your_sleeve.ok() && rng().roll( rs -> cp * p() -> azerite.ace_up_your_sleeve.spell_ref().effectN( 2 ).percent() ) )
        p() -> trigger_combo_point_gain( p() -> azerite.ace_up_your_sleeve.spell_ref().effectN( 3 ).base_value() , p() -> gains.ace_up_your_sleeve, this );
    }
  }
};

// Blade Flurry =============================================================

struct blade_flurry_t : public rogue_attack_t
{
  blade_flurry_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "blade_flurry", p, p -> find_specialization_spell( "Blade Flurry" ), options_str )
  {
    harmful = may_miss = may_crit = false;
    ignore_false_positive = true;
    internal_cooldown -> duration += p -> talent.dancing_steel -> effectN( 4 ).time_value();
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p() -> buffs.blade_flurry -> trigger();
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
    weapon = &( p->main_hand_weapon );
    requires_weapon_group = WEAPON_1H;
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
    requires_weapon_type = WEAPON_DAGGER;
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( ! p() -> buffs.blindside -> check() &&
         candidate_target -> health_percentage() >= data().effectN( 4 ).base_value() )
      return false;

    return rogue_attack_t::target_ready( candidate_target );
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();
    c *= 1.0 + p() -> buffs.blindside -> check_value();
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

  double combo_point_da_multiplier( const action_state_t* s ) const override
  { 
    return static_cast<double>( cast_state( s ) -> cp + 1 );
  }
};

// Dispatch =================================================================

struct dispatch_t: public rogue_attack_t
{
  dispatch_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "dispatch", p, p -> find_specialization_spell( "Dispatch" ), options_str )
  {
    requires_weapon_group = WEAPON_1H;
  }

  bool procs_main_gauche() const override
  {
    return false;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> trigger_restless_blades( execute_state );
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
    if ( p() -> azerite.twist_the_knife.ok() )
      b += p() -> azerite.twist_the_knife.value(); // CP Mult is applied as a mod later.
    return b;
  }

  virtual void execute() override
  {
    rogue_attack_t::execute();

    timespan_t envenom_duration = p() -> buffs.envenom -> data().duration() * ( 1 + cast_state( execute_state ) -> cp );

    if ( p() -> azerite.twist_the_knife.ok() && execute_state -> result == RESULT_CRIT )
      envenom_duration += p() -> azerite.twist_the_knife.spell_ref().effectN( 2 ).time_value();

    p() -> buffs.envenom -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, envenom_duration );

    p() -> trigger_poison_bomb( execute_state );
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
    b += p() -> buffs.nights_vengeance -> stack_value(); // CP Mult is applied as a mod later.
    return b;
  }

  void execute() override
  {
    rogue_attack_t::execute();

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
  struct echoing_blades_t : public rogue_attack_t
  {
    echoing_blades_t( rogue_t* p ) :
      rogue_attack_t( "echoing_blades", p, p -> find_spell(287653) )
    {
      aoe = -1;
      background  = true;
      may_miss = may_block = may_dodge = may_parry = false;
      base_dd_min = p -> azerite.echoing_blades.value( 6 );
      base_dd_max = p -> azerite.echoing_blades.value( 6 );
    }

    double composite_crit_chance() const override
    {
      return 1.0;
    }
  };

  echoing_blades_t* echoing_blades_attack;
  int echoing_blades_crit_count;

  fan_of_knives_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "fan_of_knives", p, p -> find_specialization_spell( "Fan of Knives" ), options_str ),
    echoing_blades_attack( nullptr ), echoing_blades_crit_count( 0 )
  {
    aoe = -1;
    energize_type     = ENERGIZE_ON_HIT;
    energize_resource = RESOURCE_COMBO_POINT;
    energize_amount   = data().effectN( 2 ).base_value();

    if ( p -> azerite.echoing_blades.ok() )
    {
      echoing_blades_attack = new echoing_blades_t( p );
      add_child( echoing_blades_attack );
    }
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

    if ( p() -> buffs.hidden_blades -> up() )
      p() -> buffs.hidden_blades -> expire();

    echoing_blades_crit_count = 0;
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

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( echoing_blades_attack && state -> result == RESULT_CRIT && ++echoing_blades_crit_count <= p() -> azerite.echoing_blades.spell_ref().effectN( 4 ).base_value() )
    {
      echoing_blades_attack -> set_target( state -> target );
      echoing_blades_attack -> execute();
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

struct garrote_state_t : public rogue_attack_state_t
{
  bool shrouded_suffocation;

  garrote_state_t( action_t* action, player_t* target ) :
    rogue_attack_state_t( action, target ), shrouded_suffocation( false )
  { }

  void initialize() override
  { rogue_attack_state_t::initialize(); shrouded_suffocation = false; }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    rogue_attack_state_t::debug_str( s ) << " shrouded_suffocation=" << shrouded_suffocation;
    return s;
  }

  void copy_state( const action_state_t* o ) override
  {
    rogue_attack_state_t::copy_state( o );
    const garrote_state_t* st = debug_cast<const garrote_state_t*>( o );
    shrouded_suffocation = st -> shrouded_suffocation;
  }
};

struct garrote_t : public rogue_attack_t
{
  garrote_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "garrote", p, p -> spec.garrote, options_str )
  {
    may_crit = false;
    may_dodge = may_block = may_parry = false;
    hasted_ticks = true;
  }

  action_state_t* new_state() override
  { return new garrote_state_t( this, target ); }

  void snapshot_state( action_state_t* state, dmg_e type ) override
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
        p() -> trigger_combo_point_gain( p() -> azerite.shrouded_suffocation.spell_ref().effectN( 2 ).base_value(), p() -> gains.shrouded_suffocation, this );
    }
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

    if ( result_is_hit (execute_state -> result ) && p() -> buffs.broadside -> up() )
    {
      p() -> trigger_combo_point_gain( p() -> buffs.broadside -> data().effectN( 2 ).base_value(),
          p() -> gains.broadside, this );
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

    if ( result_is_hit( execute_state -> result ) && p() -> buffs.broadside -> up() )
    {
      p() -> trigger_combo_point_gain( p() -> buffs.broadside -> data().effectN( 2 ).base_value(),
          p() -> gains.broadside, this );
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
    requires_weapon_type = WEAPON_DAGGER;
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
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );
    p() -> trigger_inevitability( state );
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
  killing_spree_tick_t( rogue_t* p, const char* name, const spell_data_t* s ) :
    rogue_attack_t( name, p, s )
  {
    may_dodge = may_parry = may_block = false;
    may_crit = true;
    background = true;
    direct_tick = true;
  }
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
      attack_oh = new killing_spree_tick_t( p, "killing_spree_oh", p -> find_spell( 57841 ) -> effectN( 1 ).trigger() );
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
    ap_type = AP_WEAPON_BOTH;
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
      if ( result_is_hit( execute_state -> result ) && p() -> buffs.broadside -> up() )
      {
        p() -> trigger_combo_point_gain( p() -> buffs.broadside -> data().effectN( 2 ).base_value(),
            p() -> gains.broadside, this );
      }

      if ( p() -> talent.quick_draw -> ok() && p() -> buffs.opportunity -> check() )
      {
        p() -> trigger_combo_point_gain( static_cast<int>( p() -> talent.quick_draw -> effectN( 2 ).base_value() ),
            p() -> gains.quick_draw, this );
      }
    }

    // Expire buffs.
    p() -> buffs.opportunity -> expire();
    p() -> buffs.deadshot -> expire();
  }
};

// Marked for Death =========================================================

struct marked_for_death_t : public rogue_attack_t
{
  double precombat_seconds;

  marked_for_death_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "marked_for_death", p, p -> find_talent_spell( "Marked for Death" ) ),
    precombat_seconds( 0.0 )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    may_miss = may_crit = harmful = callbacks = false;
    energize_type = ENERGIZE_ON_CAST;
    cooldown -> duration += timespan_t::from_millis( p -> spec.subtlety_rogue -> effectN( 6 ).base_value() );
    cooldown -> duration += timespan_t::from_millis( p -> spec.assassination_rogue -> effectN( 4 ).base_value() );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( precombat_seconds && ! p() -> in_combat ) {
      p() -> cooldowns.marked_for_death -> adjust( - timespan_t::from_seconds( precombat_seconds ), false );
    }
  }

  // Defined after marked_for_death_debuff_t. Sigh.
  void impact( action_state_t* state ) override;
};


// Mutilate =================================================================

struct mutilate_strike_t : public rogue_attack_t
{
  mutilate_strike_t( rogue_t* p, const char* name, const spell_data_t* s ) :
    rogue_attack_t( name, p, s )
  {
    background  = true;
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    p() -> trigger_seal_fate( state );
  }
};

struct double_dose_t : public rogue_attack_t
{
  double_dose_t( rogue_t* p ) :
    rogue_attack_t( "double_dose", p, p -> find_spell(273009) )
  {
    background  = true;
    may_miss = may_block = may_dodge = may_parry = false;
    base_dd_min = p -> azerite.double_dose.value();
    base_dd_max = p -> azerite.double_dose.value();
  }
};

struct mutilate_t : public rogue_attack_t
{
  rogue_attack_t* mh_strike;
  rogue_attack_t* oh_strike;
  rogue_attack_t* double_dose;

  mutilate_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "mutilate", p, p -> find_specialization_spell( "Mutilate" ), options_str ),
    mh_strike( nullptr ), oh_strike( nullptr ), double_dose( nullptr)
  {
    may_crit = false;

    if ( p -> main_hand_weapon.type != WEAPON_DAGGER ||
         p ->  off_hand_weapon.type != WEAPON_DAGGER )
    {
      sim -> errorf( "Player %s attempting to execute Mutilate without two daggers equipped.", p -> name() );
      background = true;
    }

    mh_strike = new mutilate_strike_t( p, "mutilate_mh", data().effectN( 3 ).trigger() );
    add_child( mh_strike );

    oh_strike = new mutilate_strike_t( p, "mutilate_oh", data().effectN( 4 ).trigger() );
    add_child( oh_strike );

    if ( p -> azerite.double_dose.ok() )
    {
      double_dose = new double_dose_t( p );
      add_child( double_dose );
    }
  }

  void execute() override
  {
    // Reset Double Dose tracker before anything happens.
    p() -> buffs.double_dose -> expire();

    rogue_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> talent.blindside -> ok() && rng().roll( p() -> talent.blindside -> effectN( 5 ).percent() ) )
        p() -> buffs.blindside -> trigger();

      mh_strike -> set_target( execute_state -> target );
      mh_strike -> execute();

      oh_strike -> set_target( execute_state -> target );
      oh_strike -> execute();

      if ( double_dose && p() -> buffs.double_dose -> stack() == p() -> buffs.double_dose -> max_stack() )
      {
        double_dose -> set_target( execute_state -> target );
        double_dose -> execute();
      }

      if ( p() -> talent.venom_rush-> ok() && p() -> get_target_data( execute_state -> target ) -> poisoned() )
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
    ap_type = AP_WEAPON_BOTH;

    if ( p ->  replicating_shadows )
      add_child( p ->  replicating_shadows );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t base_per_tick = data().effectN( 1 ).period();
    return data().duration() + base_per_tick * cast_state( s ) -> cp;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    // Copy the persistent multiplier from the origin of replications.
    if ( secondary_trigger == TRIGGER_REPLICATING_SHADOWS )
      return p() -> get_target_data( p() -> last_nightblade_target ) -> dots.nightblade -> state -> persistent_multiplier;
    return rogue_attack_t::composite_persistent_multiplier( state );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    // Check if this is a manually applied Nightblade that hits
    if ( secondary_trigger == TRIGGER_NONE && result_is_hit( execute_state -> result ) )
    {
      p() -> buffs.nights_vengeance -> trigger();

      // Save the target for Replicating Shadows
      p() -> last_nightblade_target = execute_state -> target;
    }
  }
};

struct replicating_shadows_t : public rogue_attack_t
{
  action_t* nightblade_action;

  replicating_shadows_t( rogue_t* p ) :
    rogue_attack_t( "replicating_shadows", p, p -> find_spell(286131) ),
    nightblade_action( nullptr )
  {
    background  = true;
    may_miss = may_block = may_dodge = may_parry = false;
    base_dd_min = p -> azerite.replicating_shadows.value();
    base_dd_max = p -> azerite.replicating_shadows.value();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( ! p() -> last_nightblade_target )
      return;

    // Get the last manually applied Nightblade as origin. Has to be still up.
    rogue_td_t* last_nb_tdata = p() -> get_target_data( p() -> last_nightblade_target );
    if ( last_nb_tdata -> dots.nightblade -> is_ticking() )
    {
      // Find the closest target to that manual target without a Nightblade debuff.
      double minDist = 0.0;
      player_t* minDistTarget = nullptr;
      for ( const auto enemy : sim -> target_non_sleeping_list )
      {
        rogue_td_t* tdata = p() -> get_target_data( enemy );
        if ( ! tdata -> dots.nightblade -> is_ticking() )
        {
          double dist = enemy -> get_position_distance( p() -> last_nightblade_target -> x_position, p() -> last_nightblade_target -> y_position );
          if ( ! minDistTarget || dist < minDist)
          {
            minDist = dist;
            minDistTarget = enemy;
          }
        }
      }

      // If it exists, trigger a new nightblade with 0 CP duration. We also copy the persistent multiplier in nightblade_t.
      // Estimated 10 yd spread radius.
      if ( minDistTarget && minDist < 10.0 )
      {
        if ( !nightblade_action )
          nightblade_action = p() -> find_action( "nightblade" );
        if ( nightblade_action )
          make_event<actions::secondary_ability_trigger_t>( *sim, minDistTarget, nightblade_action,
            cast_state( last_nb_tdata -> dots.nightblade -> state ) -> cp, TRIGGER_REPLICATING_SHADOWS );
      }
    }
  }
};

// Roll the Bones ===========================================================

struct roll_the_bones_t : public rogue_attack_t
{
  double precombat_seconds;

  roll_the_bones_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "roll_the_bones", p, p -> spec.roll_the_bones ),
    precombat_seconds( 0.0 )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = false;
    dot_duration = timespan_t::zero();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    // Restless Blades and thus True Bearing CDR triggers before buff roll.
    p() -> trigger_restless_blades( execute_state );

    int cp = cast_state( execute_state ) -> cp;
    timespan_t d = ( cp + 1 ) * p() -> buffs.roll_the_bones -> data().duration();

    if ( precombat_seconds && ! p() -> in_combat )
      d -= timespan_t::from_seconds( precombat_seconds );

    p() -> buffs.roll_the_bones -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, d );

    p() -> buffs.snake_eyes -> trigger( p() -> buffs.snake_eyes -> max_stack(), cp * p() -> azerite.snake_eyes.value() );
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

    // Run quiet proxy buff that handles Scent of Blood
    td( execute_state -> target ) -> debuffs.rupture -> trigger();
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

// Secret Technique =========================================================

struct secret_technique_t : public rogue_attack_t
{
  struct secret_technique_attack_t : public rogue_attack_t
  {
    secret_technique_attack_t( const std::string& n, rogue_t* p ) :
      rogue_attack_t( n, p, p -> find_spell( 280720 ) )
    {
      weapon = &(p -> main_hand_weapon);
      background = true;
      aoe = -1;
    }

    double combo_point_da_multiplier( const action_state_t* state ) const override
    {
      return static_cast<double>( cast_state( state ) -> cp );
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

  secret_technique_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "secret_technique", p, p -> talent.secret_technique, options_str )
  {
    requires_weapon_type = WEAPON_DAGGER;
    may_miss = false;
    aoe = -1;

    player_attack = new secret_technique_attack_t( "secret_technique_player", p );
    add_child( player_attack );
    clone_attack = new secret_technique_attack_t( "secret_technique_clones", p );
    add_child( clone_attack );

    radius = player_attack -> radius;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    int cp = cast_state( execute_state ) -> cp;

    // Hit of the main char happens right on cast.
    make_event<actions::secondary_ability_trigger_t>( *sim, execute_state -> target, player_attack, cp, TRIGGER_SECRET_TECHNIQUE );

    // The clones seem to hit 1s later (no time reference in spell data though)
    timespan_t delay = timespan_t::from_seconds( 1.0 );
    // Trigger tracking buff until clone damage
    p() -> buffs.secret_technique -> trigger( 1, buff_t::DEFAULT_VALUE(), (-1.0), delay );
    // Assuming effect #2 is the number of aditional clones
    for ( size_t i = 0; i < data().effectN( 2 ).base_value(); i++ )
    {
      make_event<actions::secondary_ability_trigger_t>( *sim, execute_state -> target, clone_attack, cp, TRIGGER_SECRET_TECHNIQUE, delay );
    }
  }
};

// Shadow Blades ============================================================

struct shadow_blades_attack_t : public rogue_attack_t
{
  shadow_blades_attack_t( rogue_t* p ) :
    rogue_attack_t( "shadow_blades_attack", p, p -> find_spell( 279043 ) )
  {
    background = true;
    may_crit = may_dodge = may_block = may_parry = false;
    attack_power_mod.direct = 0;
  }

  void init() override
  {
    rogue_attack_t::init();

    snapshot_flags = update_flags = 0;
  }
};

struct shadow_blades_t : public rogue_attack_t
{
  double precombat_seconds;

  shadow_blades_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadow_blades", p, p -> find_specialization_spell( "Shadow Blades" ) ),
    precombat_seconds( 0.0 )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = may_miss = may_crit = false;

    school = SCHOOL_SHADOW;
    add_child( p -> shadow_blades_attack );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.shadow_blades -> trigger();

    if ( precombat_seconds && ! p() -> in_combat ) {
      timespan_t precombat_lost_seconds = - timespan_t::from_seconds( precombat_seconds );
      p() -> cooldowns.shadow_blades -> adjust( precombat_lost_seconds, false );
      p() -> buffs.shadow_blades -> extend_duration( p(), precombat_lost_seconds );
    }
  }
};

// Shadow Dance =============================================================

struct shadow_dance_t : public rogue_attack_t
{
  cooldown_t* icd;

  shadow_dance_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadow_dance", p, p -> spec.shadow_dance ),
    icd( p -> get_cooldown( "shadow_dance_icd" ) )
  {
    parse_options( options_str );

    harmful = may_miss = may_crit = false;
    dot_duration = timespan_t::zero(); // No need to have a tick here
    icd -> duration = data().cooldown();
    if ( p -> talent.enveloping_shadows -> ok() )
    {
      cooldown -> charges += p -> talent.enveloping_shadows -> effectN( 2 ).base_value();
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.shadow_dance -> trigger();

    if ( p()->azerite.the_first_dance.ok() )
    {
      p()->buffs.the_first_dance->trigger();
      p() -> trigger_combo_point_gain( p()->buffs.the_first_dance->data().effectN( 3 ).resource( RESOURCE_COMBO_POINT ),
        p() -> gains.the_first_dance, this );
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
  shadowstrike_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadowstrike", p, p -> spec.shadowstrike, options_str )
  {
    may_dodge = may_block = may_parry = false;
    requires_weapon_type = WEAPON_DAGGER;
    requires_stealth = true;
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

    p() -> buffs.blade_in_the_shadows -> trigger();
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( p() -> talent.find_weakness ->ok() )
      td( state -> target ) -> debuffs.find_weakness -> trigger();

    p() -> trigger_weaponmaster( state );
    p() -> trigger_inevitability( state );
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
    ap_type = AP_WEAPON_BOTH;
  }

  void init() override
  {
    rogue_attack_t::init();

    // As of 2018-06-18 not in BfA spell data.
    affected_by.shadow_blades = true;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    // Stealth Buff
    if ( p()->stealthed( STEALTH_BASIC | STEALTH_SHADOWDANCE ) )
    {
      m *= 1.0 + data().effectN( 3 ).percent();
    }

    return m;
  }
};

// Shuriken Tornado =========================================================

struct shuriken_tornado_t : public rogue_attack_t
{
  shuriken_tornado_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shuriken_tornado", p, p -> talent.shuriken_tornado, options_str )
  {
    may_miss = false;
    dot_duration = timespan_t::zero();
    aoe = -1;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.shuriken_tornado -> trigger();
  }
};

// Shuriken Toss ============================================================

struct shuriken_toss_t : public rogue_attack_t
{
  shuriken_toss_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shuriken_toss", p, p -> find_specialization_spell( "Shuriken Toss" ), options_str )
  {
    ap_type = AP_WEAPON_BOTH;
  }
};

// Sinister Strike ==========================================================

struct sinister_strike_t : public rogue_attack_t
{
  timespan_t extra_attack_delay;

  sinister_strike_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "sinister_strike", p, p -> find_specialization_spell( "Sinister Strike" ), options_str ),
    extra_attack_delay( data().duration() )
  {
  }

  double cost() const override
  {
    if ( secondary_trigger == TRIGGER_SINISTER_STRIKE )
      return 0;

    return rogue_attack_t::cost();
  }

  double composite_energize_amount( const action_state_t* state ) const override
  {
    // Do not grant CP on extra proc event
    // This was changed back as of 2018-07-09, commenting it out in case the revert is reverted.
    /*if ( secondary_trigger == TRIGGER_SINISTER_STRIKE )
      return 0;*/
    return rogue_attack_t::composite_energize_amount( state );
  }

  double sinister_strike_proc_chance() const
  {
    double opportunity_proc_chance = data().effectN( 3 ).percent();
    opportunity_proc_chance += p() -> talent.weaponmaster -> effectN( 1 ).percent();
    opportunity_proc_chance += p() -> buffs.skull_and_crossbones -> stack_value();
    opportunity_proc_chance += p() -> buffs.keep_your_wits_about_you -> stack_value();
    return opportunity_proc_chance;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = rogue_attack_t::bonus_da( s );

    b += p() -> buffs.snake_eyes -> value();

    if ( secondary_trigger == TRIGGER_SINISTER_STRIKE )
      b += p() -> azerite.keep_your_wits_about_you.value( 2 );

    return b;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( ! result_is_hit( execute_state -> result ) )
    {
      return;
    }

    p() -> buffs.snake_eyes -> decrement();

    if ( secondary_trigger != TRIGGER_SINISTER_STRIKE &&
         ( p() -> buffs.opportunity -> trigger( 1, buff_t::DEFAULT_VALUE(), sinister_strike_proc_chance() ) ) )
    {
      make_event<actions::secondary_ability_trigger_t>( *sim, execute_state -> target, this, 0, TRIGGER_SINISTER_STRIKE, extra_attack_delay );
      p() -> procs.sinister_strike_extra_attack -> occur();
    }

    // secondary_trigger != TRIGGER_SINISTER_STRIKE &&
    if ( p() -> buffs.broadside -> up() )
    {
      p() -> trigger_combo_point_gain( p() -> buffs.broadside -> data().effectN( 2 ).base_value(),
          p() -> gains.broadside, this );
    }
  }
};

// Slice and Dice ===========================================================

struct slice_and_dice_t : public rogue_attack_t
{
  double precombat_seconds;

  slice_and_dice_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "slice_and_dice", p, p -> talent.slice_and_dice ),
    precombat_seconds( 0.0 )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    base_costs[ RESOURCE_COMBO_POINT ] = 1; // No resource cost in the spell .. sigh
    harmful = false;
    dot_duration = timespan_t::zero();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> trigger_restless_blades( execute_state );

    int cp = cast_state( execute_state ) -> cp;
    timespan_t snd_duration = ( cp + 1 ) * p() -> buffs.slice_and_dice -> data().duration();

    if ( precombat_seconds && ! p() -> in_combat )
      snd_duration -= timespan_t::from_seconds( precombat_seconds );

    double snd_mod = 1.0; // Multiplier for the SnD effects. Was changed in Legion for Loaded Dice artifact trait.
    p() -> buffs.slice_and_dice -> trigger( 1, snd_mod, -1.0, snd_duration );

    p() -> buffs.snake_eyes -> trigger( p() -> buffs.snake_eyes -> max_stack(), cp * p() -> azerite.snake_eyes.value() );

    if ( p() -> azerite.paradise_lost.ok() )
      p() -> buffs.paradise_lost -> trigger( 1, buff_t::DEFAULT_VALUE(), (-1.0), snd_duration );
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
    cooldown->duration = data().cooldown() + p->spell.sprint_2->effectN( 1 ).time_value();
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
    rogue_attack_t( "symbols_of_death", p, p -> spec.symbols_of_death )
  {
    parse_options( options_str );

    harmful = callbacks = false;
    requires_stealth = false;

    dot_duration = timespan_t::zero();
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.symbols_of_death -> trigger();
  }
};

// Toxic Blade ==============================================================

struct toxic_blade_t : public rogue_attack_t
{
  toxic_blade_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "toxic_blade", p, p -> talent.toxic_blade, options_str )
  {
    requires_weapon_type = WEAPON_DAGGER;
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
  struct nothing_personal_t : rogue_attack_t
  {
    nothing_personal_t( rogue_t* p ) :
      rogue_attack_t( "nothing_personal", p, p -> find_spell( 286581 ) )
    {
      may_dodge = may_parry = may_block = false;
      may_crit = false;
      tick_may_crit = true;
      background = true;
      harmful = true;
      hasted_ticks = true;
      base_td = p -> azerite.nothing_personal.value();
    }
  };

  double precombat_seconds;
  nothing_personal_t* nothing_personal_dot;

  vendetta_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "vendetta", p, p -> find_specialization_spell( "Vendetta" ) ),
    precombat_seconds( 0.0 ), nothing_personal_dot( nullptr )
  {
    add_option( opt_float( "precombat_seconds", precombat_seconds ) );
    parse_options( options_str );

    harmful = may_miss = may_crit = false;

    if ( p -> azerite.nothing_personal.ok() )
    {
      nothing_personal_dot = new nothing_personal_t( p );
      add_child( nothing_personal_dot );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.vendetta -> trigger();
    p() -> buffs.nothing_personal -> trigger();

    if ( nothing_personal_dot )
    {
      nothing_personal_dot -> set_target( execute_state -> target );
      nothing_personal_dot -> execute();
    }

    rogue_td_t* td = this -> td( execute_state -> target );
    td -> debuffs.vendetta -> trigger();

    if ( precombat_seconds && ! p() -> in_combat ) {
      timespan_t precombat_lost_seconds = - timespan_t::from_seconds( precombat_seconds );
      p() -> cooldowns.vendetta -> adjust( precombat_lost_seconds, false );
      p() -> buffs.vendetta -> extend_duration( p(), precombat_lost_seconds );
      td -> debuffs.vendetta -> extend_duration( p(), precombat_lost_seconds );
    }
  }
};

// ==========================================================================
// Stealth
// ==========================================================================

struct stealth_t : public rogue_attack_t
{
  stealth_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "stealth", p, p -> find_class_spell( "Stealth" ) )
  {
    may_miss = may_crit = harmful = false;
    ignore_false_positive = true;

    parse_options( options_str );
  }

  virtual void execute() override
  {
    rogue_attack_t::execute();

    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", p() -> name(), name() );

    p() -> buffs.stealth -> trigger();

    // Stop autoattacks
    if ( p() -> main_hand_attack && p() -> main_hand_attack -> execute_event )
      event_t::cancel( p() -> main_hand_attack -> execute_event );

    if ( p() -> off_hand_attack && p() -> off_hand_attack -> execute_event )
      event_t::cancel( p() -> off_hand_attack -> execute_event );
  }

  virtual bool ready() override
  {
    if ( p() -> stealthed( STEALTH_BASIC | STEALTH_ROGUE ) )
      return false;

    if ( ! p() -> in_combat )
      return true;

    // HAX: Allow restealth for DungeonSlice against non-"boss" targets because Shadowmeld drops combat against trash.
    if ( p()->sim->fight_style == "DungeonSlice" && p()->player_t::buffs.shadowmeld->check() && target->name_str.find("Boss") == std::string::npos )
      return true;

    if ( !p()->restealth_allowed )
      return false;

    return rogue_attack_t::ready();
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
      make_event<actions::secondary_ability_trigger_t>( *sim, state -> target, internal_bleeding, cast_state( state ) -> cp, TRIGGER_INTERNAL_BLEEDING );
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
  { }

  double composite_poison_flat_modifier( const action_state_t* ) const override
  { return 1.0; }
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
            ( data().id() == 703 || data().id() == 1943 || this -> name_str == "crimson_tempest" ) )
  {
    return new exsanguinated_expr_t( this );
  }
  else if ( util::str_compare_ci( name_str, "ss_buffed") )
  {
    return make_fn_expr( "ss_buffed", [ this ]() {
    rogue_td_t* td_ = td( target );
    if ( ! td_ -> dots.garrote -> is_ticking() )
    {
      return 0.0;
    }
    return debug_cast<const garrote_state_t*>( td_ -> dots.garrote -> state ) -> shrouded_suffocation ? 1.0 : 0.0;
  } );
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
    buff_t( p, "adrenaline_rush", p -> find_class_spell( "Adrenaline Rush" ) )
  { 
    set_cooldown( timespan_t::zero() );
    set_default_value( p -> find_class_spell( "Adrenaline Rush" ) -> effectN( 2 ).percent() );
    set_affects_regen( true );
    add_invalidate( CACHE_ATTACK_SPEED );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    buff_t::start( stacks, value, duration );

    rogue_t* rogue = debug_cast<rogue_t*>( source );
    rogue -> resources.temporary[ RESOURCE_ENERGY ] += data().effectN( 4 ).base_value();
    rogue -> recalculate_resource_max( RESOURCE_ENERGY );
  }

  void expire_override(int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    rogue_t* rogue = debug_cast<rogue_t*>( source );
    rogue -> resources.temporary[ RESOURCE_ENERGY ] -= data().effectN( 4 ).base_value();
    rogue -> recalculate_resource_max( RESOURCE_ENERGY );
  }
};

struct blade_flurry_t : public buff_t
{
  blade_flurry_t( rogue_t* p ) :
    buff_t( p, "blade_flurry", p -> spec.blade_flurry )
  {
    set_cooldown( timespan_t::zero() );
    set_duration( p -> spec.blade_flurry -> duration() + p -> talent.dancing_steel -> effectN( 2 ).time_value() );
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

    actions::break_stealth( rogue );
  }
};

struct stealth_like_buff_t : public buff_t
{
  rogue_t* rogue;

  stealth_like_buff_t( rogue_t* r, const std::string& name, const spell_data_t* spell ) :
    buff_t( r, name, spell ), rogue( r )
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
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( rogue->talent.master_assassin->ok() )
    {
      // Don't swap these buffs around if we are still in stealth due to Vanish expiring
      if ( !rogue->buffs.stealth->check() )
      {
        rogue->buffs.master_assassin_aura->expire();
        rogue->buffs.master_assassin->trigger();
      }
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

// Shadow dance acts like "stealth like abilities"
struct shadow_dance_t : public stealth_like_buff_t
{
  shadow_dance_t( rogue_t* p ) :
    stealth_like_buff_t( p, "shadow_dance", p -> spec.shadow_dance )
  {
    buff_duration += p -> talent.subterfuge -> effectN( 2 ).time_value();

    if ( p -> talent.dark_shadow -> ok() )
    {
      add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    stealth_like_buff_t::expire_override( expiration_stacks, remaining_duration );

    rogue -> buffs.the_first_dance -> expire();
  }
};

struct shuriken_tornado_t : public buff_t
{
  rogue_t* rogue;
  action_t* shuriken_storm_action;

  shuriken_tornado_t( rogue_t* r ) :
    buff_t( r, "shuriken_tornado", r -> talent.shuriken_tornado ),
    rogue( r ),
    shuriken_storm_action( nullptr )
  {
    set_cooldown( timespan_t::zero() );
    set_period( timespan_t::from_seconds( 1.0 ) ); // Not explicitly in spell data
    set_tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
      if ( !shuriken_storm_action )
        shuriken_storm_action = rogue -> find_action( "shuriken_storm" );
      if ( shuriken_storm_action )
        make_event<actions::secondary_ability_trigger_t>( *sim, rogue -> target, shuriken_storm_action, 0, TRIGGER_SHURIKEN_TORNADO );
    } );
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

    buffs[ 0 ] = rogue -> buffs.broadside;
    buffs[ 1 ] = rogue -> buffs.buried_treasure;
    buffs[ 2 ] = rogue -> buffs.grand_melee;
    buffs[ 3 ] = rogue -> buffs.ruthless_precision;
    buffs[ 4 ] = rogue -> buffs.skull_and_crossbones;
    buffs[ 5 ] = rogue -> buffs.true_bearing;
  }

  void expire_secondary_buffs()
  {
    rogue -> buffs.broadside -> expire();
    rogue -> buffs.buried_treasure -> expire();
    rogue -> buffs.grand_melee -> expire();
    rogue -> buffs.ruthless_precision -> expire();
    rogue -> buffs.skull_and_crossbones -> expire();
    rogue -> buffs.true_bearing -> expire();
    rogue -> buffs.paradise_lost -> expire();
  }

  std::vector<buff_t*> random_roll(bool loaded)
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
      std::vector<double> current_odds = rogue -> fixed_rtb_odds;
      if (loaded)
        current_odds[0] = 0.0;

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
    range::for_each( rogue -> fixed_rtb, [this, &rolled]( size_t idx )
    { rolled.push_back( buffs[ idx ] ); } );
    return rolled;
  }

  unsigned roll_the_bones( timespan_t duration )
  {
    std::vector<buff_t*> rolled;
    if ( rogue -> fixed_rtb.size() == 0 )
    {
      rolled = random_roll(rogue -> buffs.loaded_dice -> up());
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

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    expire_secondary_buffs();

    switch ( roll_the_bones( remains() ) )
    {
      case 1:
        rogue -> procs.roll_the_bones_1 -> occur();
        if ( rogue -> azerite.paradise_lost.ok() )
          rogue -> buffs.paradise_lost -> trigger( 1, buff_t::DEFAULT_VALUE(), (-1.0), remains() );
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

  if ( state -> action -> is_aoe() )
    return;

  if ( sim -> active_enemies == 1 )
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

  // Note: this changed to be 10 * seconds as of 2017-04-19
  int cdr = spec.deepening_shadows -> effectN( 1 ).base_value();
  if ( talent.enveloping_shadows -> ok() )
  {
    cdr += talent.enveloping_shadows -> effectN( 1 ).base_value();
  }
  timespan_t adjustment = timespan_t::from_seconds( -0.1 * cdr * s -> cp );

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

void rogue_t::trigger_restless_blades( const action_state_t* state )
{
  timespan_t v = timespan_t::from_seconds( spec.restless_blades -> effectN( 1 ).base_value() / 10.0 );
  v += timespan_t::from_seconds( buffs.true_bearing -> value() );
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

  // Original and "logical" implementation.
  // dot -> adjust( coeff );
  // Since the advent of hasted bleed exsanguinate works differently though.
  dot -> adjust_full_ticks( coeff );

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

void rogue_t::spend_combo_points( const action_state_t* state )
{
  if ( state -> action -> base_costs[ RESOURCE_COMBO_POINT ] == 0 )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  double max_spend = std::min( resources.current[ RESOURCE_COMBO_POINT ], consume_cp_max() );
  state -> action -> stats -> consume_resource( RESOURCE_COMBO_POINT, max_spend );
  resource_loss( RESOURCE_COMBO_POINT, max_spend, nullptr, state ? state -> action : nullptr );

  if ( sim -> log )
    sim -> out_log.printf( "%s consumes %.1f %s for %s (%.0f)", name(),
                   max_spend, util::resource_type_string( RESOURCE_COMBO_POINT ),
                   state -> action -> name(), resources.current[ RESOURCE_COMBO_POINT ] );

  if ( state ->action->name_str != "secret_technique" )
  {
    // As of 2018-06-28 on beta, Secret Technique does not reduce its own cooldown. May be a bug or the cdr happening before CD start.
    timespan_t sectec_cdr = timespan_t::from_seconds( talent.secret_technique -> effectN( 5 ).base_value() );
    sectec_cdr *= -max_spend;
    cooldowns.secret_technique -> adjust( sectec_cdr, false );
  }

  // Proc Replicating Shadows on the current target.
  if ( specialization() == ROGUE_SUBTLETY && replicating_shadows && rng().roll( max_spend * azerite.replicating_shadows.spell_ref().effectN( 2 ).percent() ) )
  {
    replicating_shadows -> set_target( state -> target );
    replicating_shadows -> execute();
  }
}

void rogue_t::trigger_shadow_blades_attack( action_state_t* state )
{
  if ( ! buffs.shadow_blades -> check() || state -> result_total <= 0 || ! state -> action -> result_is_hit( state -> result ) )
  {
    return;
  }

  const actions::rogue_attack_t* attack = cast_attack( state -> action );
  if ( ! attack -> affected_by.shadow_blades )
  {
    return;
  }

  double amount = state -> result_amount * buffs.shadow_blades -> check_value();
  shadow_blades_attack -> base_dd_min = amount;
  shadow_blades_attack -> base_dd_max = amount;
  shadow_blades_attack -> set_target( state -> target );
  shadow_blades_attack -> execute();
}

void rogue_t::trigger_inevitability( const action_state_t* state )
{
  if ( ! state -> action -> result_is_hit( state -> result ) || ! azerite.inevitability.ok() )
    return;

  buffs.symbols_of_death -> extend_duration( this, timespan_t::from_seconds( azerite.inevitability.spell_ref().effectN( 2 ).base_value() / 10.0 ) );
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
  dots.rupture            = target -> get_dot( "rupture", source );
  dots.crimson_tempest    = target -> get_dot( "crimson_tempest", source );
  dots.nightblade         = target -> get_dot( "nightblade", source );

  debuffs.marked_for_death = new buffs::marked_for_death_debuff_t( *this );
  debuffs.wound_poison = new buffs::wound_poison_t( *this );
  debuffs.crippling_poison = new buffs::crippling_poison_t( *this );
  debuffs.leeching_poison = new buffs::leeching_poison_t( *this );
  debuffs.rupture = new buffs::proxy_rupture_t( *this );
  debuffs.vendetta = make_buff( *this, "vendetta", source->spec.vendetta )
    -> set_cooldown( timespan_t::zero() )
    -> set_default_value( source->spec.vendetta->effectN( 1 ).percent() );
  debuffs.toxic_blade = make_buff( *this, "toxic_blade", source -> talent.toxic_blade -> effectN( 4 ).trigger() )
    -> set_default_value( source -> talent.toxic_blade -> effectN( 4 ).trigger() -> effectN( 1 ).percent() );
  debuffs.ghostly_strike = make_buff( *this, "ghostly_strike", source -> talent.ghostly_strike )
    -> set_default_value( source -> talent.ghostly_strike -> effectN( 3 ).percent() );
  const spell_data_t* fw_debuff = source -> talent.find_weakness -> effectN( 1 ).trigger();
  debuffs.find_weakness = make_buff( *this, "find_weakness", fw_debuff )
    -> set_default_value( fw_debuff -> effectN( 1 ).percent() );

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

  crit += buffs.ruthless_precision -> stack_value();

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

  crit += buffs.ruthless_precision -> stack_value();

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
  return ( true_level >  110 ) ? "currents" :
         ( true_level >  100 ) ? "seventh_demon" :
         ( true_level >= 90  ) ? "greater_draenic_agility_flask" :
         ( true_level >= 85  ) ? "spring_blossoms" :
         ( true_level >= 80  ) ? "winds" :
         "disabled";
}

// rogue_t::default_potion ==================================================

std::string rogue_t::default_potion() const
{
  return ( true_level > 110 ) ? "battle_potion_of_agility" :
         ( true_level > 100 ) ? "prolonged_power" :
         ( true_level >= 90 ) ? "draenic_agility" :
         ( true_level >= 85 ) ? "virmens_bite" :
         ( true_level >= 80 ) ? "tolvir" :
         "disabled";
}

// rogue_t::default_food ====================================================

std::string rogue_t::default_food() const
{
  return ( true_level >  110 ) ? "bountiful_captains_feast" :
         ( true_level >  100 ) ? "lavish_suramar_feast" :
         ( true_level >  90  ) ? "jumbo_sea_dog" :
         ( true_level >= 90  ) ? "sea_mist_rice_noodles" :
         ( true_level >= 80  ) ? "seafood_magnifique_feast" :
         "disabled";
}

std::string rogue_t::default_rune() const
{
  return ( true_level >= 120 ) ? "battle_scarred" :
         ( true_level >= 110 ) ? "defiled" :
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

  if ( specialization() == ROGUE_ASSASSINATION )
  {
    def -> add_action( "variable,name=energy_regen_combined,value=energy.regen+poisoned_bleeds*7%(2*spell_haste)" );
    def -> add_action( "variable,name=single_target,value=spell_targets.fan_of_knives<2" );
    def -> add_action( "call_action_list,name=stealthed,if=stealthed.rogue" );
    def -> add_action( "call_action_list,name=cds" );
    def -> add_action( "call_action_list,name=dot" );
    def -> add_action( "call_action_list,name=direct" );
    def -> add_action( "arcane_torrent,if=energy.deficit>=15+variable.energy_regen_combined" );
    def -> add_action( "arcane_pulse");
    def -> add_action( "lights_judgment");

    // Cooldowns
    action_priority_list_t* cds = get_action_priority_list( "cds", "Cooldowns" );
    cds -> add_action( potion_action );
    for ( size_t i = 0; i < items.size(); i++ )
    {
      if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
      {
        auto use_effect_id = items[i].special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) -> spell_id;
        if ( items[i].name_str == "galecallers_boon" )
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=cooldown.vendetta.remains>45" );
        else if ( use_effect_id == 271107 || use_effect_id == 277179 || use_effect_id == 277185 ) // Golden Luster, Gladiator's Medallion Gladiator's Badge
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=debuff.vendetta.up" );
        else // Default: Use on CD
          cds -> add_action( "use_item,name=" + items[i].name_str, "Default Trinket usage: Use on cooldown." );
      }
    }
    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "lights_judgment" || racial_actions[i] == "arcane_torrent" || racial_actions[i] == "arcane_pulse" )
        continue; // Manually added
      else
        cds -> add_action( racial_actions[i] + ",if=debuff.vendetta.up" );
    }
    cds -> add_talent( this, "Marked for Death", "target_if=min:target.time_to_die,if=raid_event.adds.up&(target.time_to_die<combo_points.deficit*1.5|combo_points.deficit>=cp_max_spend)", "If adds are up, snipe the one with lowest TTD. Use when dying faster than CP deficit or without any CP." );
    cds -> add_talent( this, "Marked for Death", "if=raid_event.adds.in>30-raid_event.adds.duration&combo_points.deficit>=cp_max_spend", "If no adds will die within the next 30s, use MfD on boss without any CP." );
    cds -> add_action( this, "Vendetta", "if=!stealthed.rogue&dot.rupture.ticking&(!talent.subterfuge.enabled|!azerite.shrouded_suffocation.enabled|dot.garrote.pmultiplier>1&(spell_targets.fan_of_knives<6|!cooldown.vanish.up))&(!talent.nightstalker.enabled|!talent.exsanguinate.enabled|cooldown.exsanguinate.remains<5-2*talent.deeper_stratagem.enabled)" );
    cds -> add_action( this, "Vanish", "if=talent.exsanguinate.enabled&(talent.nightstalker.enabled|talent.subterfuge.enabled&variable.single_target)&combo_points>=cp_max_spend&cooldown.exsanguinate.remains<1&(!talent.subterfuge.enabled|!azerite.shrouded_suffocation.enabled|dot.garrote.pmultiplier<=1)", "Vanish with Exsg + (Nightstalker, or Subterfuge only on 1T): Maximum CP and Exsg ready for next GCD" );
    cds -> add_action( this, "Vanish", "if=talent.nightstalker.enabled&!talent.exsanguinate.enabled&combo_points>=cp_max_spend&debuff.vendetta.up", "Vanish with Nightstalker + No Exsg: Maximum CP and Vendetta up" );
    cds -> add_action( "variable,name=ss_vanish_condition,value=azerite.shrouded_suffocation.enabled&(non_ss_buffed_targets>=1|spell_targets.fan_of_knives=3)&(ss_buffed_targets_above_pandemic=0|spell_targets.fan_of_knives>=6)", "See full comment on https://github.com/Ravenholdt-TC/Rogue/wiki/Assassination-APL-Research." );
    cds -> add_action( "pool_resource,for_next=1,extra_amount=45" );
    cds -> add_action( this, "Vanish", "if=talent.subterfuge.enabled&!stealthed.rogue&cooldown.garrote.up&(variable.ss_vanish_condition|!azerite.shrouded_suffocation.enabled&dot.garrote.refreshable)&combo_points.deficit>=((1+2*azerite.shrouded_suffocation.enabled)*spell_targets.fan_of_knives)>?4&raid_event.adds.in>12" );
    cds -> add_action( this, "Vanish", "if=talent.master_assassin.enabled&!stealthed.all&master_assassin_remains<=0&!dot.rupture.refreshable&dot.garrote.remains>3", "Vanish with Master Assasin: No stealth and no active MA buff, Rupture not in refresh range" );
    cds -> add_action( "shadowmeld,if=!stealthed.all&azerite.shrouded_suffocation.enabled&dot.garrote.refreshable&dot.garrote.pmultiplier<=1&combo_points.deficit>=1", "Shadowmeld for Shrouded Suffocation" );
    cds -> add_talent( this, "Exsanguinate", "if=dot.rupture.remains>4+4*cp_max_spend&!dot.garrote.refreshable", "Exsanguinate when both Rupture and Garrote are up for long enough" );
    cds -> add_talent( this, "Toxic Blade", "if=dot.rupture.ticking" );

    // Stealth
    action_priority_list_t* stealthed = get_action_priority_list( "stealthed", "Stealthed Actions" );
    stealthed -> add_action( this, "Rupture", "if=combo_points>=4&(talent.nightstalker.enabled|talent.subterfuge.enabled&(talent.exsanguinate.enabled&cooldown.exsanguinate.remains<=2|!ticking)&variable.single_target)&target.time_to_die-remains>6", "Nighstalker, or Subt+Exsg on 1T: Snapshot Rupture" );
    stealthed -> add_action( "pool_resource,for_next=1", "Subterfuge + Shrouded Suffocation: Ensure we use one global to apply Garrote to the main target if it is not snapshot yet, so all other main target abilities profit." );
    stealthed -> add_action( this, "Garrote", "if=azerite.shrouded_suffocation.enabled&buff.subterfuge.up&buff.subterfuge.remains<1.3&!ss_buffed" );
    stealthed -> add_action( "pool_resource,for_next=1", "Subterfuge: Apply or Refresh with buffed Garrotes" );
    stealthed -> add_action( this, "Garrote", "target_if=min:remains,if=talent.subterfuge.enabled&(remains<12|pmultiplier<=1)&target.time_to_die-remains>2" );
    stealthed -> add_action( this, "Rupture", "if=talent.subterfuge.enabled&azerite.shrouded_suffocation.enabled&!dot.rupture.ticking&variable.single_target", "Subterfuge + Shrouded Suffocation in ST: Apply early Rupture that will be refreshed for pandemic" );
    stealthed -> add_action( "pool_resource,for_next=1", "Subterfuge w/ Shrouded Suffocation: Reapply for bonus CP and/or extended snapshot duration." );
    stealthed -> add_action( this, "Garrote", "target_if=min:remains,if=talent.subterfuge.enabled&azerite.shrouded_suffocation.enabled&target.time_to_die>remains&(remains<18|!ss_buffed)" );
    stealthed -> add_action( "pool_resource,for_next=1", "Subterfuge + Exsg: Even override a snapshot Garrote right after Rupture before Exsanguination" );
    stealthed -> add_action( this, "Garrote", "if=talent.subterfuge.enabled&talent.exsanguinate.enabled&cooldown.exsanguinate.remains<1&prev_gcd.1.rupture&dot.rupture.remains>5+4*cp_max_spend" );

    // Damage over time abilities
    action_priority_list_t* dot = get_action_priority_list( "dot", "Damage over time abilities" );
    dot -> add_action( this, "Rupture", "if=talent.exsanguinate.enabled&((combo_points>=cp_max_spend&cooldown.exsanguinate.remains<1)|(!ticking&(time>10|combo_points>=2)))", "Special Rupture setup for Exsg" );
    dot -> add_action( "pool_resource,for_next=1", "Garrote upkeep, also tries to use it as a special generator for the last CP before a finisher" );
    dot -> add_action( this, "Garrote", "cycle_targets=1,if=(!talent.subterfuge.enabled|!(cooldown.vanish.up&cooldown.vendetta.remains<=4))&combo_points.deficit>=1+3*(azerite.shrouded_suffocation.enabled&cooldown.vanish.up)&refreshable&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3+azerite.shrouded_suffocation.enabled)&(!exsanguinated|remains<=tick_time*2&spell_targets.fan_of_knives>=3+azerite.shrouded_suffocation.enabled)&!ss_buffed&(target.time_to_die-remains>4&spell_targets.fan_of_knives<=1|target.time_to_die-remains>12)&(master_assassin_remains=0|!ticking)" );
    dot -> add_talent( this, "Crimson Tempest", "if=spell_targets>=2&remains<2+(spell_targets>=5)&combo_points>=4", "Crimson Tempest only on multiple targets at 4+ CP when running out in 2s (up to 4 targets) or 3s (5+ targets)" );
    dot -> add_action( this, "Rupture", "cycle_targets=1,if=combo_points>=4&refreshable&(pmultiplier<=1|remains<=tick_time&spell_targets.fan_of_knives>=3+azerite.shrouded_suffocation.enabled)&(!exsanguinated|remains<=tick_time*2&spell_targets.fan_of_knives>=3+azerite.shrouded_suffocation.enabled)&target.time_to_die-remains>4", "Keep up Rupture at 4+ on all targets (when living long enough and not snapshot)" );

    // Direct damage abilities
    action_priority_list_t* direct = get_action_priority_list( "direct", "Direct damage abilities" );
    direct -> add_action( this, "Envenom", "if=combo_points>=4+talent.deeper_stratagem.enabled&(debuff.vendetta.up|debuff.toxic_blade.up|energy.deficit<=25+variable.energy_regen_combined|!variable.single_target)&(!talent.exsanguinate.enabled|cooldown.exsanguinate.remains>2)", "Envenom at 4+ (5+ with DS) CP. Immediately on 2+ targets, with Vendetta, or with TB; otherwise wait for some energy. Also wait if Exsg combo is coming up." );
    direct -> add_action( "variable,name=use_filler,value=combo_points.deficit>1|energy.deficit<=25+variable.energy_regen_combined|!variable.single_target" );
    direct -> add_action( this, "Fan of Knives", "if=variable.use_filler&azerite.echoing_blades.enabled&spell_targets.fan_of_knives>=2", "With Echoing Blades, Fan of Knives at 2+ targets." );
    direct -> add_action( this, "Fan of Knives", "if=variable.use_filler&(buff.hidden_blades.stack>=19|spell_targets.fan_of_knives>=4+(azerite.double_dose.rank>2)+stealthed.rogue)", "Fan of Knives at 19+ stacks of Hidden Blades or against 4+ (5+ with Double Dose) targets." );
    direct -> add_action( this, "Fan of Knives", "target_if=!dot.deadly_poison_dot.ticking,if=variable.use_filler&spell_targets.fan_of_knives>=3", "Fan of Knives to apply Deadly Poison if inactive on any target at 3 targets." );
    direct -> add_talent( this, "Blindside", "if=variable.use_filler&(buff.blindside.up|!talent.venom_rush.enabled&!azerite.double_dose.enabled)" );
    direct -> add_action( this, "Mutilate", "target_if=!dot.deadly_poison_dot.ticking,if=variable.use_filler&spell_targets.fan_of_knives=2", "Tab-Mutilate to apply Deadly Poison at 2 targets" );
    direct -> add_action( this, "Mutilate", "if=variable.use_filler" );
  }
  else if ( specialization() == ROGUE_OUTLAW )
  {
    // Pre-Combat
    precombat -> add_action( this, "Roll the Bones", "precombat_seconds=2" );
    precombat -> add_talent( this, "Slice and Dice", "precombat_seconds=2" );
    precombat -> add_action( this, "Adrenaline Rush", "precombat_seconds=1" );

    // Main Rotation
    def -> add_action( "variable,name=rtb_reroll,value=rtb_buffs<2&(buff.loaded_dice.up|!buff.grand_melee.up&!buff.ruthless_precision.up)", "Reroll for 2+ buffs with Loaded Dice up. Otherwise reroll for 2+ or Grand Melee or Ruthless Precision." );
    def -> add_action( "variable,name=rtb_reroll,op=set,if=azerite.deadshot.enabled|azerite.ace_up_your_sleeve.enabled,value=rtb_buffs<2&(buff.loaded_dice.up|buff.ruthless_precision.remains<=cooldown.between_the_eyes.remains)", "Reroll for 2+ buffs or Ruthless Precision with Deadshot or Ace up your Sleeve." );
    def -> add_action( "variable,name=rtb_reroll,op=set,if=azerite.snake_eyes.rank>=2,value=rtb_buffs<2", "2+ Snake Eyes: Always reroll for 2+ buffs." );
    def -> add_action( "variable,name=rtb_reroll,op=reset,if=azerite.snake_eyes.rank>=2&buff.snake_eyes.stack>=2-buff.broadside.up", "2+ Snake Eyes: Do not reroll with 2+ stacks of the Snake Eyes buff (1+ stack with Broadside up)." );
    def -> add_action( "variable,name=ambush_condition,value=combo_points.deficit>=2+2*(talent.ghostly_strike.enabled&cooldown.ghostly_strike.remains<1)+buff.broadside.up&energy>60&!buff.skull_and_crossbones.up" );
    def -> add_action( "variable,name=blade_flurry_sync,value=spell_targets.blade_flurry<2&raid_event.adds.in>20|buff.blade_flurry.up", "With multiple targets, this variable is checked to decide whether some CDs should be synced with Blade Flurry" );
    def -> add_action( "call_action_list,name=stealth,if=stealthed.all" );
    def -> add_action( "call_action_list,name=cds" );
    def -> add_action( "run_action_list,name=finish,if=combo_points>=cp_max_spend-(buff.broadside.up+buff.opportunity.up)*(talent.quick_draw.enabled&(!talent.marked_for_death.enabled|cooldown.marked_for_death.remains>1))", "Finish at maximum CP. Substract one for each Broadside and Opportunity when Quick Draw is selected and MfD is not ready after the next second." );
    def -> add_action( "call_action_list,name=build" );
    def -> add_action( "arcane_torrent,if=energy.deficit>=15+energy.regen" );
    def -> add_action( "arcane_pulse" );
    def -> add_action( "lights_judgment");

    // Cooldowns
    action_priority_list_t* cds = get_action_priority_list( "cds", "Cooldowns" );
    cds -> add_action( potion_action );
    for ( size_t i = 0; i < items.size(); i++ )
    {
      if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
      {
        cds -> add_action( "use_item,name=" + items[i].name_str + ",if=buff.bloodlust.react|target.time_to_die<=20|combo_points.deficit<=2", "Falling back to default item usage" );
      }
    }
    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "lights_judgment" || racial_actions[i] == "arcane_torrent" || racial_actions[i] == "arcane_pulse" )
        continue; // Manually added
      else
        cds -> add_action( racial_actions[i] );
    }
    cds -> add_action( this, "Adrenaline Rush", "if=!buff.adrenaline_rush.up&energy.time_to_max>1" );
    cds -> add_talent( this, "Marked for Death", "target_if=min:target.time_to_die,if=raid_event.adds.up&(target.time_to_die<combo_points.deficit|!stealthed.rogue&combo_points.deficit>=cp_max_spend-1)", "If adds are up, snipe the one with lowest TTD. Use when dying faster than CP deficit or without any CP." );
    cds -> add_talent( this, "Marked for Death", "if=raid_event.adds.in>30-raid_event.adds.duration&!stealthed.rogue&combo_points.deficit>=cp_max_spend-1", "If no adds will die within the next 30s, use MfD on boss without any CP." );
    cds -> add_action( this, "Blade Flurry", "if=spell_targets>=2&!buff.blade_flurry.up&(!raid_event.adds.exists|raid_event.adds.remains>8|raid_event.adds.in>(2-cooldown.blade_flurry.charges_fractional)*25)", "Blade Flurry on 2+ enemies. With adds: Use if they stay for 8+ seconds or if your next charge will be ready in time for the next wave." );
    cds -> add_talent( this, "Ghostly Strike", "if=variable.blade_flurry_sync&combo_points.deficit>=1+buff.broadside.up" );
    cds -> add_talent( this, "Killing Spree", "if=variable.blade_flurry_sync&(energy.time_to_max>5|energy<15)" );
    cds -> add_talent( this, "Blade Rush", "if=variable.blade_flurry_sync&energy.time_to_max>1" );
    cds -> add_action( this, "Vanish", "if=!stealthed.all&variable.ambush_condition", "Using Vanish/Ambush is only a very tiny increase, so in reality, you're absolutely fine to use it as a utility spell." );
    cds -> add_action( "shadowmeld,if=!stealthed.all&variable.ambush_condition" );

    // Stealth
    action_priority_list_t* stealth = get_action_priority_list( "stealth", "Stealth" );
    stealth -> add_action( this, "Ambush" );

    // Finishers
    action_priority_list_t* finish = get_action_priority_list( "finish", "Finishers" );
    finish -> add_action( this, "Between the Eyes", "if=buff.ruthless_precision.up|(azerite.deadshot.enabled|azerite.ace_up_your_sleeve.enabled)&buff.roll_the_bones.up", "BtE over RtB rerolls with Deadshot/Ace traits or Ruthless Precision." );
    finish -> add_talent( this, "Slice and Dice", "if=buff.slice_and_dice.remains<target.time_to_die&buff.slice_and_dice.remains<(1+combo_points)*1.8" );
    finish -> add_action( this, "Roll the Bones", "if=buff.roll_the_bones.remains<=3|variable.rtb_reroll" );
    finish -> add_action( this, "Between the Eyes", "if=azerite.ace_up_your_sleeve.enabled|azerite.deadshot.enabled", "BtE with the Ace Up Your Sleeve or Deadshot traits." );
    finish -> add_action( this, "Dispatch" );

    // Builders
    action_priority_list_t* build = get_action_priority_list( "build", "Builders" );
    build -> add_action( this, "Pistol Shot", "if=buff.opportunity.up&(buff.keep_your_wits_about_you.stack<25|buff.deadshot.up)", "Use Pistol Shot if it won't cap combo points and the Oppotunity buff is up. Avoid using when Keep Your Wits stacks are high unless the Deadshot buff is also up." );
    build -> add_action( this, "Sinister Strike" );
  }
  else if ( specialization() == ROGUE_SUBTLETY )
  {
    // Pre-Combat
    precombat -> add_action( this, "Stealth" );
    precombat -> add_talent( this, "Marked for Death", "precombat_seconds=15" );
    precombat -> add_action( this, "Shadow Blades", "precombat_seconds=1" );
    precombat -> add_action( "potion" );

    // Main Rotation
    def -> add_action( "call_action_list,name=cds", "Check CDs at first" );
    def -> add_action( "run_action_list,name=stealthed,if=stealthed.all", "Run fully switches to the Stealthed Rotation (by doing so, it forces pooling if nothing is available)." );
    def -> add_action( this, "Nightblade", "if=target.time_to_die>6&remains<gcd.max&combo_points>=4-(time<10)*2", "Apply Nightblade at 2+ CP during the first 10 seconds, after that 4+ CP if it expires within the next GCD or is not up" );
    def -> add_action( "variable,name=use_priority_rotation,value=priority_rotation&spell_targets.shuriken_storm>=2", "Only change rotation if we have priority_rotation set and multiple targets up." );
    def -> add_action( "call_action_list,name=stealth_cds,if=variable.use_priority_rotation", "Priority Rotation? Let's give a crap about energy for the stealth CDs (builder still respect it). Yup, it can be that simple." );
    def -> add_action( "variable,name=stealth_threshold,value=25+talent.vigor.enabled*35+talent.master_of_shadows.enabled*25+talent.shadow_focus.enabled*20+talent.alacrity.enabled*10+15*(spell_targets.shuriken_storm>=3)", "Used to define when to use stealth CDs or builders" );
    def -> add_action( "call_action_list,name=stealth_cds,if=energy.deficit<=variable.stealth_threshold", "Consider using a Stealth CD when reaching the energy threshold" );
    def -> add_action( "call_action_list,name=finish,if=combo_points.deficit<=1|target.time_to_die<=1&combo_points>=3", "Finish at 4+ without DS, 5+ with DS (outside stealth)" );
    def -> add_action( "call_action_list,name=finish,if=spell_targets.shuriken_storm=4&combo_points>=4", "With DS also finish at 4+ against exactly 4 targets (outside stealth)" );
    def -> add_action( "call_action_list,name=build,if=energy.deficit<=variable.stealth_threshold", "Use a builder when reaching the energy threshold" );
    def -> add_action( "arcane_torrent,if=energy.deficit>=15+energy.regen", "Lowest priority in all of the APL because it causes a GCD" );
    def -> add_action( "arcane_pulse" );
    def -> add_action( "lights_judgment");

    // Cooldowns
    action_priority_list_t* cds = get_action_priority_list( "cds", "Cooldowns" );
    cds -> add_action( potion_action );
    for ( size_t i = 0; i < items.size(); i++ )
    {
      if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
      {
        // Use on-cd exceptions
        if ( items[i].name_str == "mydas_talisman" )
          cds -> add_action( "use_item,name=" + items[i].name_str, "Use on cooldown." );
        else // Use with Symbols default
          cds -> add_action( "use_item,name=" + items[i].name_str + ",if=buff.symbols_of_death.up|target.time_to_die<20", "Falling back to default item usage: Use with Symbols of Death." );
      }
    }
    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "lights_judgment" || racial_actions[i] == "arcane_torrent" )
        continue; // Manually added
      else
        cds -> add_action( racial_actions[i] + ",if=buff.symbols_of_death.up" );
    }
    cds -> add_action( this, "Shadow Dance", "use_off_gcd=1,if=!buff.shadow_dance.up&buff.shuriken_tornado.up&buff.shuriken_tornado.remains<=3.5", "Use Dance off-gcd before the first Shuriken Storm from Tornado comes in." );
    cds -> add_action( this, "Symbols of Death", "use_off_gcd=1,if=buff.shuriken_tornado.up&buff.shuriken_tornado.remains<=3.5", "(Unless already up because we took Shadow Focus) use Symbols off-gcd before the first Shuriken Storm from Tornado comes in." );
    cds -> add_action( this, "Symbols of Death", "if=dot.nightblade.ticking&(!talent.shuriken_tornado.enabled|talent.shadow_focus.enabled|spell_targets.shuriken_storm<3|!cooldown.shuriken_tornado.up)", "Use Symbols on cooldown (after first Nightblade) unless we are going to pop Tornado and do not have Shadow Focus." );
    cds -> add_talent( this, "Marked for Death", "target_if=min:target.time_to_die,if=raid_event.adds.up&(target.time_to_die<combo_points.deficit|!stealthed.all&combo_points.deficit>=cp_max_spend)", "If adds are up, snipe the one with lowest TTD. Use when dying faster than CP deficit or not stealthed without any CP." );
    cds -> add_talent( this, "Marked for Death", "if=raid_event.adds.in>30-raid_event.adds.duration&!stealthed.all&combo_points.deficit>=cp_max_spend", "If no adds will die within the next 30s, use MfD on boss without any CP and no stealth." );
    cds -> add_action( this, "Shadow Blades", "if=combo_points.deficit>=2+stealthed.all" );
    cds -> add_talent( this, "Shuriken Tornado", "if=spell_targets>=3&!talent.shadow_focus.enabled&dot.nightblade.ticking&!stealthed.all&cooldown.symbols_of_death.up&cooldown.shadow_dance.charges>=1", "At 3+ without Shadow Focus use Tornado with SoD and Dance ready. We will pop those before the first storm comes in." );
    cds -> add_talent( this, "Shuriken Tornado", "if=spell_targets>=3&talent.shadow_focus.enabled&dot.nightblade.ticking&buff.symbols_of_death.up", "At 3+ with Shadow Focus use Tornado with SoD already up." );
    cds -> add_action( this, "Shadow Dance", "if=!buff.shadow_dance.up&target.time_to_die<=5+talent.subterfuge.enabled&!raid_event.adds.up" );

    // Stealth Cooldowns
    action_priority_list_t* stealth_cds = get_action_priority_list( "stealth_cds", "Stealth Cooldowns" );
    stealth_cds -> add_action( "variable,name=shd_threshold,value=cooldown.shadow_dance.charges_fractional>=1.75", "Helper Variable" );
    stealth_cds -> add_action( this, "Vanish", "if=!variable.shd_threshold&combo_points.deficit>1&debuff.find_weakness.remains<1", "Vanish unless we are about to cap on Dance charges. Only when Find Weakness is about to run out." );
    stealth_cds -> add_action( "pool_resource,for_next=1,extra_amount=40", "Pool for Shadowmeld + Shadowstrike unless we are about to cap on Dance charges. Only when Find Weakness is about to run out." );
    stealth_cds -> add_action( "shadowmeld,if=energy>=40&energy.deficit>=10&!variable.shd_threshold&combo_points.deficit>1&debuff.find_weakness.remains<1" );
    stealth_cds -> add_action( "variable,name=shd_combo_points,value=combo_points.deficit>=4", "CP requirement: Dance at low CP by default." );
    stealth_cds -> add_action( "variable,name=shd_combo_points,value=combo_points.deficit<=1+2*azerite.the_first_dance.enabled,if=variable.use_priority_rotation&(talent.nightstalker.enabled|talent.dark_shadow.enabled)", "CP requirement: Dance only before finishers if we have amp talents and priority rotation." );
    stealth_cds -> add_action( this, "Shadow Dance", "if=variable.shd_combo_points&(!talent.dark_shadow.enabled|dot.nightblade.remains>=5+talent.subterfuge.enabled)&(variable.shd_threshold|buff.symbols_of_death.remains>=1.2|spell_targets.shuriken_storm>=4&cooldown.symbols_of_death.remains>10)", "With Dark Shadow only Dance when Nightblade will stay up. Use during Symbols or above threshold." );
    stealth_cds -> add_action( this, "Shadow Dance", "if=variable.shd_combo_points&target.time_to_die<cooldown.symbols_of_death.remains&!raid_event.adds.up", "Burn remaining Dances before the target dies if SoD won't be ready in time." );

    // Stealthed Rotation
    action_priority_list_t* stealthed = get_action_priority_list( "stealthed", "Stealthed Rotation" );
    stealthed -> add_action( this, "Shadowstrike", "if=(talent.find_weakness.enabled|spell_targets.shuriken_storm<3)&(buff.stealth.up|buff.vanish.up)", "If Stealth/vanish are up, use Shadowstrike to benefit from the passive bonus and Find Weakness, even if we are at max CP (from the precombat MfD)." );
    stealthed -> add_action( "call_action_list,name=finish,if=combo_points.deficit<=1-(talent.deeper_stratagem.enabled&(buff.vanish.up|azerite.the_first_dance.enabled&!talent.dark_shadow.enabled&!talent.subterfuge.enabled&spell_targets.shuriken_storm<3))", "Finish at 4+ CP without DS, 5+ with DS, and 6 with DS after Vanish or The First Dance and no Dark Shadow + no Subterfuge" );
    stealthed -> add_talent( this, "Gloomblade" , "if=azerite.perforate.rank>=2&spell_targets.shuriken_storm<=2&position_back", "Use Gloomblade over Shadowstrike and Storm with 2+ Perforate at 2 or less targets.");
    stealthed -> add_action( this, "Shadowstrike", "cycle_targets=1,if=talent.secret_technique.enabled&talent.find_weakness.enabled&debuff.find_weakness.remains<1&spell_targets.shuriken_storm=2&target.time_to_die-remains>6", "At 2 targets with Secret Technique keep up Find Weakness by cycling Shadowstrike.");
    stealthed -> add_action( this, "Shadowstrike", "if=!talent.deeper_stratagem.enabled&azerite.blade_in_the_shadows.rank=3&spell_targets.shuriken_storm=3", "Without Deeper Stratagem and 3 Ranks of Blade in the Shadows it is worth using Shadowstrike on 3 targets." );
    stealthed -> add_action( this, "Shadowstrike", "if=variable.use_priority_rotation&(talent.find_weakness.enabled&debuff.find_weakness.remains<1|talent.weaponmaster.enabled&spell_targets.shuriken_storm<=4|azerite.inevitability.enabled&buff.symbols_of_death.up&spell_targets.shuriken_storm<=3+azerite.blade_in_the_shadows.enabled)", "For priority rotation, use Shadowstrike over Storm 1) with WM against up to 4 targets, 2) if FW is running off (on any amount of targets), or 3) to maximize SoD extension with Inevitability on 3 targets (4 with BitS)." );
    stealthed -> add_action( this, "Shuriken Storm", "if=spell_targets>=3" );
    stealthed -> add_action( this, "Shadowstrike" );

    // Finishers
    action_priority_list_t* finish = get_action_priority_list( "finish", "Finishers" );
    finish -> add_action( this, "Eviscerate", "if=talent.shadow_focus.enabled&buff.nights_vengeance.up&spell_targets.shuriken_storm>=2+3*talent.secret_technique.enabled", "Eviscerate highest priority at 2+ targets with Shadow Focus (5+ with Secret Technique in addition) and Night's Vengeance up." );
    finish -> add_action( this, "Nightblade", "if=(!talent.dark_shadow.enabled|!buff.shadow_dance.up)&target.time_to_die-remains>6&remains<tick_time*2", "Keep up Nightblade if it is about to run out. Do not use NB during Dance, if talented into Dark Shadow." );
    finish -> add_action( this, "Nightblade", "cycle_targets=1,if=!variable.use_priority_rotation&spell_targets.shuriken_storm>=2&(azerite.nights_vengeance.enabled|!azerite.replicating_shadows.enabled|spell_targets.shuriken_storm-active_dot.nightblade>=2)&!buff.shadow_dance.up&target.time_to_die>=(5+(2*combo_points))&refreshable", "Multidotting outside Dance on targets that will live for the duration of Nightblade, refresh during pandemic. Multidot as long as 2+ targets do not have Nightblade up with Replicating Shadows (unless you have Night's Vengeance too)." );
    finish -> add_action( this, "Nightblade", "if=remains<cooldown.symbols_of_death.remains+10&cooldown.symbols_of_death.remains<=5&target.time_to_die-remains>cooldown.symbols_of_death.remains+5", "Refresh Nightblade early if it will expire during Symbols. Do that refresh if SoD gets ready in the next 5s." );
    finish -> add_talent( this, "Secret Technique", "if=buff.symbols_of_death.up&(!talent.dark_shadow.enabled|buff.shadow_dance.up)", "Secret Technique during Symbols. With Dark Shadow only during Shadow Dance (until threshold in next line)." );
    finish -> add_talent( this, "Secret Technique", "if=spell_targets.shuriken_storm>=2+talent.dark_shadow.enabled+talent.nightstalker.enabled", "With enough targets always use SecTec on CD." );
    finish -> add_action( this, "Eviscerate" );

    // Builders
    action_priority_list_t* build = get_action_priority_list( "build", "Builders" );
    build -> add_action( this, "Shuriken Storm", "if=spell_targets>=2+(talent.gloomblade.enabled&azerite.perforate.rank>=2&position_back)" );
    build -> add_talent( this, "Gloomblade" );
    build -> add_action( this, "Backstab" );
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
  if ( name == "secret_technique"    ) return new secret_technique_t   ( this, options_str );
  if ( name == "shadow_blades"       ) return new shadow_blades_t      ( this, options_str );
  if ( name == "shadow_dance"        ) return new shadow_dance_t       ( this, options_str );
  if ( name == "shadowstep"          ) return new shadowstep_t         ( this, options_str );
  if ( name == "shadowstrike"        ) return new shadowstrike_t       ( this, options_str );
  if ( name == "shuriken_storm"      ) return new shuriken_storm_t     ( this, options_str );
  if ( name == "shuriken_tornado"    ) return new shuriken_tornado_t   ( this, options_str );
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

// rogue_t::create_expression ===============================================

expr_t* rogue_t::create_expression( const std::string& name_str )
{
  std::vector<std::string> split = util::string_split( name_str, "." );

  if ( name_str == "combo_points" )
    return make_ref_expr( name_str, resources.current[ RESOURCE_COMBO_POINT ] );
  else if ( util::str_compare_ci( name_str, "cp_max_spend" ) )
  {
    return make_mem_fn_expr( name_str, *this, &rogue_t::consume_cp_max );
  }
  else if ( util::str_compare_ci(name_str, "master_assassin_remains") )
  {
    return make_fn_expr( name_str, [ this ]() {
      if ( buffs.master_assassin_aura -> check() )
      {
        timespan_t nominal_master_assassin_duration = timespan_t::from_seconds( talent.master_assassin -> effectN( 1 ).base_value() );
        timespan_t gcd_remains = timespan_t::from_seconds( std::max( ( gcd_ready - sim -> current_time() ).total_seconds(), 0.0 ) );
        return gcd_remains + nominal_master_assassin_duration;
      }
      else if ( buffs.master_assassin -> check() )
        return buffs.master_assassin -> remains();
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
        if ( tdata -> lethal_poisoned() ) {
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
        if ( ! tdata -> dots.garrote -> is_ticking() || ! debug_cast<const actions::garrote_state_t*>( tdata -> dots.garrote -> state ) -> shrouded_suffocation )
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
        if ( tdata -> dots.garrote -> remains() > timespan_t::from_seconds( 5.4 ) && debug_cast<const actions::garrote_state_t*>( tdata -> dots.garrote -> state ) -> shrouded_suffocation )
        {
          ss_buffed_targets_above_pandemic++;
        }
      }
      return ss_buffed_targets_above_pandemic;
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
    return make_ref_expr(name_str, priority_rotation);
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
  spell.sprint                        = find_class_spell( "Sprint" );
  spell.sprint_2                      = find_spell( 231691 );
  spell.ruthlessness_driver           = find_spell( 14161 );
  spell.ruthlessness_cp               = spec.ruthlessness -> effectN( 1 ).trigger();
  spell.shadow_focus                  = find_spell( 112942 );
  spell.subterfuge                    = find_spell( 115192 );
  spell.relentless_strikes_energize   = find_spell( 98440 );

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
  talent.find_weakness      = find_talent_spell( "Find Weakness" );
  talent.gloomblade         = find_talent_spell( "Gloomblade" );

  talent.shadow_focus       = find_talent_spell( "Shadow Focus" );

  talent.enveloping_shadows = find_talent_spell( "Enveloping Shadows" );
  talent.dark_shadow        = find_talent_spell( "Dark Shadow" );

  talent.master_of_shadows  = find_talent_spell( "Master of Shadows" );
  talent.secret_technique   = find_talent_spell( "Secret Technique" );
  talent.shuriken_tornado   = find_talent_spell( "Shuriken Tornado" );

  // Azerite Powers
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

  auto_attack = new actions::auto_melee_attack_t( this, "" );

  shadow_blades_attack = new actions::shadow_blades_attack_t( this );

  if ( mastery.main_gauche -> ok() )
    active_main_gauche = new actions::main_gauche_t( this );

  if ( spec.blade_flurry -> ok() )
    active_blade_flurry = new actions::blade_flurry_attack_t( this );

  if ( talent.poison_bomb -> ok() )
  {
    poison_bomb = new actions::poison_bomb_t( this );
  }

  if ( azerite.replicating_shadows.ok() )
  {
    replicating_shadows = new actions::replicating_shadows_t( this );
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
}

// rogue_t::init_procs ======================================================

void rogue_t::init_procs()
{
  player_t::init_procs();

  procs.seal_fate                    = get_proc( "Seal Fate"                    );
  procs.weaponmaster                 = get_proc( "Weaponmaster"                 );

  procs.sinister_strike_extra_attack = get_proc( "Sinister Strike Extra Attack" );
  procs.roll_the_bones_1             = get_proc( "Roll the Bones: 1 buff"       );
  procs.roll_the_bones_2             = get_proc( "Roll the Bones: 2 buffs"      );
  procs.roll_the_bones_3             = get_proc( "Roll the Bones: 3 buffs"      );
  procs.roll_the_bones_4             = get_proc( "Roll the Bones: 4 buffs"      );
  procs.roll_the_bones_5             = get_proc( "Roll the Bones: 5 buffs"      );
  procs.roll_the_bones_6             = get_proc( "Roll the Bones: 6 buffs"      );

  procs.deepening_shadows            = get_proc( "Deepening Shadows"            );
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

  resources.current[ RESOURCE_COMBO_POINT ] = initial_combo_points;
}

// rogue_t::init_buffs ======================================================

void rogue_t::create_buffs()
{
  player_t::create_buffs();

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
  buffs.blade_flurry          = new buffs::blade_flurry_t( this );
  buffs.blade_rush            = make_buff( this, "blade_rush", find_spell( 271896 ) )
                                -> set_period( find_spell( 271896 ) -> effectN( 1 ).period() )
                                -> set_tick_callback( [ this ]( buff_t* b, int, const timespan_t& ) {
                                  resource_gain( RESOURCE_ENERGY, b -> data().effectN( 1 ).base_value(), gains.blade_rush );
                                } );
  buffs.opportunity           = make_buff( this, "opportunity", find_spell( 195627 ) );
  // Roll the bones buffs
  buffs.broadside             = make_buff( this, "broadside", find_spell( 193356 ) )
                                -> add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.buried_treasure       = make_buff( this, "buried_treasure", find_spell( 199600 ) )
                                -> set_affects_regen( true )
                                -> set_default_value( find_spell( 199600 ) -> effectN( 1 ).base_value() / 5.0 );
  buffs.grand_melee           = make_buff( this, "grand_melee", find_spell( 193358 ) )
      -> add_invalidate( CACHE_ATTACK_SPEED )
      -> add_invalidate( CACHE_LEECH )
      -> set_default_value( 1.0 / ( 1.0 + find_spell( 193358 ) -> effectN( 1 ).percent() ) );
  buffs.skull_and_crossbones  = make_buff( this, "skull_and_crossbones", find_spell( 199603 ) )
                                -> set_default_value( find_spell( 199603 ) -> effectN( 1 ).percent() );
  buffs.ruthless_precision    = make_buff( this, "ruthless_precision", find_spell( 193357 ) )
                                -> set_default_value( find_spell( 193357 ) -> effectN( 1 ).percent() )
                                -> add_invalidate( CACHE_CRIT_CHANCE );
  buffs.true_bearing          = make_buff( this, "true_bearing", find_spell( 193359 ) )
                                -> set_default_value( find_spell( 193359 ) -> effectN( 1 ).base_value() * 0.1 );
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


  // Talents
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
  buffs.blindside                = make_buff( this, "blindside", find_spell( 121153 ) )
                                  -> set_default_value( find_spell( 121153 ) -> effectN( 2 ).percent() );
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
  buffs.secret_technique        = make_buff( this, "secret_technique", talent.secret_technique )
                                  -> set_cooldown( timespan_t::zero() )
                                  -> set_quiet( true );
  buffs.shuriken_tornado        = new buffs::shuriken_tornado_t( this );


  // Azerite
  buffs.blade_in_the_shadows               = make_buff( this, "blade_in_the_shadows", find_spell( 279754 ) )
                                             -> set_trigger_spell( azerite.blade_in_the_shadows.spell_ref().effectN( 1 ).trigger() )
                                             -> set_default_value( azerite.blade_in_the_shadows.value() );
  buffs.brigands_blitz                     = make_buff<stat_buff_t>( this, "brigands_blitz", find_spell( 277724 ) )
                                             -> add_stat( STAT_HASTE_RATING, azerite.brigands_blitz.value() )
                                             -> set_refresh_behavior( buff_refresh_behavior::DURATION );
  buffs.brigands_blitz_driver              = make_buff( this, "brigands_blitz_driver", find_spell( 277725 ) )
                                             -> set_trigger_spell( azerite.brigands_blitz.spell_ref().effectN( 1 ).trigger() )
                                             -> set_quiet( true )
                                             -> set_tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
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
  add_option( opt_bool( "rogue_optimize_expressions", rogue_optimize_expressions ) );
  add_option( opt_bool( "rogue_ready_trigger", rogue_ready_trigger ) );
  add_option( opt_bool( "priority_rotation", priority_rotation ) );

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

  fixed_rtb = rogue -> fixed_rtb;
  fixed_rtb_odds = rogue -> fixed_rtb_odds;
  priority_rotation = rogue -> priority_rotation;
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

  poisoned_enemies = 0;

  shadow_techniques = 0;

  last_nightblade_target = nullptr;

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
      r->restealth_allowed = true;
  }
};

void rogue_t::activate()
{
  player_t::activate();

  sim->target_non_sleeping_list.register_callback( restealth_callback_t( this ) );
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
  rogue_report_t( rogue_t& player ) : p( player )
  {
  }

  virtual void html_customsection( report::sc_html_stream& ) override
  {
    // Custom Class Section can be added here
    ( void )p;
  }
private:
  rogue_t& p;
};

// ROGUE MODULE INTERFACE ===================================================

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
