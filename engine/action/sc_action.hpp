// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "dbc/data_definitions.hh"
#include "player/target_specific.hpp"
#include "player/covenant.hpp"
#include "sc_enums.hpp"
#include "util/timespan.hpp"
#include "util/generic.hpp"
#include "util/string_view.hpp"
#include "util/format.hpp"

#include <array>
#include <vector>
#include <memory>
#include <string>

struct action_priority_t;
struct action_priority_list_t;
struct action_state_t;
struct spell_data_t;
struct cooldown_t;
struct dot_t;
struct event_t;
struct expr_t;
struct gain_t;
struct item_t;
struct option_t;
struct sim_t;
struct player_t;
struct proc_t;
namespace rng {
  struct rng_t;
}
struct spelleffect_data_t;
struct stats_t;
struct travel_event_t;
struct weapon_t;

// Action ===================================================================

struct action_t : private noncopyable
{
public:
  const spell_data_t* s_data;
  const spell_data_t* s_data_reporting;
  sim_t* sim;
  const action_e type;
  std::string name_str;
  player_t* const player;
  player_t* target;

  /// Item back-pointer for trinket-sourced actions so we can show proper tooltips in reports
  const item_t* item;

  /// Weapon used for this ability. If set extra weapon damage is calculated.
  weapon_t* weapon;

  /**
   * Default target is needed, otherwise there's a chance that cycle_targets
   * option will _MAJORLY_ mess up the action list for the actor, as there's no
   * guarantee cycle_targets will end up on the "initial target" when an
   * iteration ends.
   */
  player_t* default_target;

  /// What type of damage this spell does.
  school_e school;

  /// What base school components this spell has
  std::vector<school_e> base_schools;

  /// Spell id if available, 0 otherwise
  unsigned id;

  /**
   * @brief player & action-name unique id.
   *
   * Every action -- even actions without spelldata -- is given an internal_id
   */
  int internal_id;

  /// What resource does this ability use.
  resource_e resource_current;

  /// The amount of targets that an ability impacts on. -1 will hit all targets.
  int aoe;

  /// If set to true, this action will not be counted toward total amount of executes in reporting. Useful for abilities with parent/children attacks.
  bool dual;

  /// enables/disables proc callback system on the action, like trinkets, enchants, rppm.
  bool callbacks;

  /// Whether or not the spell uses the yellow attack hit table.
  bool special;

  /// Tells the sim to not perform any other actions, as the ability is channeled.
  bool channeled;

  /// mark this as a sequence_t action
  bool sequence;

  /**
   * @brief Disables/enables reporting of this action.
   *
   * When set to true, action will not show up in raid report or count towards executes.
   */
  bool quiet;

  /**
   * @brief Enables/Disables direct execution of the ability in an action list.
   *
   * Background actions cannot be executed via action list, but can be triggered by other actions.
   * Background actions do not count for executes.
   * Abilities with background = true can still be called upon by other actions,
   * example being deep wounds and how it is activated by devastate.
   */
  bool background;

  /**
   * @brief Check if action is executable between GCD's
   *
   * When set to true, will check every 100 ms to see if this action needs to be used,
   * rather than waiting until the next gcd.
   * Slows simulation down significantly.
   */
  bool use_off_gcd;

  /// True if ability should be used while casting another spell.
  bool use_while_casting;

  /// True if ability is usable while casting another spell
  bool usable_while_casting;

  /// true if channeled action does not reschedule autoattacks, used on abilities such as bladestorm.
  bool interrupt_auto_attack;

  /// Used for actions that will do awful things to the sim when a "false positive" skill roll happens.
  bool ignore_false_positive;

  /// Skill is now done per ability, with the default being set to the player option.
  double action_skill;

  /// Used with DoT Drivers, tells simc that the direct hit is actually a tick.
  bool direct_tick;

  /// Used for abilities that repeat themselves without user interaction, only used on autoattacks.
  bool repeating;

  /**
   * Simplified: Will the ability pull the boss if used.
   * Also determines whether ability can be used precombat without counting towards the 1 harmful spell limit
   */
  bool harmful;

  /**
   * @brief Whether or not this ability is a proc.
   *
   * Procs do not consume resources.
   */
  bool proc;

  /// Whether or not the action is an interrupt (specifically triggers PF2_CAST_INTERRUPT callbacks)
  bool is_interrupt;

  /// Whether the action is used from the precombat action list. Will be set up automatically by action_t::init_finished
  bool is_precombat;

  /// Is the action initialized? (action_t::init ran successfully)
  bool initialized;

  /// Self explanatory.
  bool may_hit, may_miss, may_dodge, may_parry, may_glance, may_block, may_crit, tick_may_crit;

  /// Whether or not the ability/dot ticks immediately on usage.
  bool tick_zero;

  /// Whether or not the ability/dot ticks when it is first applied, but not on refresh applications
  bool tick_on_application;

  /**
   * @brief Whether or not ticks scale with haste.
   *
   * Generally only used for bleeds that do not scale with haste,
   * or with ability drivers that deal damage every x number of seconds.
   */
  bool hasted_ticks;

  /// Need to consume per tick?
  bool consume_per_tick_;

  /// Split damage evenly between targets
  bool split_aoe_damage;

  /// Reduce damage to secondary targets based on total target count
  bool reduced_aoe_damage;

  /**
   * @brief Normalize weapon speed for weapon damage calculations
   *
   * \sa weapon_t::get_normalized_speed()
   */
  bool normalize_weapon_speed;

  /// This ability leaves a ticking dot on the ground, and doesn't move when the target moves. Used with original_x and original_y
  bool ground_aoe;

  /// Duration of the ground area trigger
  timespan_t ground_aoe_duration;

  /// Round spell base damage to integer before using
  bool round_base_dmg;

  /// Used with tick_action, tells tick_action to update state on every tick.
  bool dynamic_tick_action;

  /// Type of attack power used by the ability
  attack_power_type ap_type;

  /// Did a channel action have an interrupt_immediate used to cancel it on it
  bool interrupt_immediate_occurred;

  bool hit_any_target;

  /**
   * @brief Behavior of dot.
   *
   * Acceptable inputs are DOT_CLIP, DOT_REFRESH, and DOT_EXTEND.
   */
  dot_behavior_e dot_behavior;

  /// Ability specific extra player ready delay
  timespan_t ability_lag, ability_lag_stddev;

  /// The minimum gcd triggered no matter the haste.
  timespan_t min_gcd;

  /// Hasted GCD stat type. One of HASTE_NONE, HASTE_ATTACK, HASTE_SPELL, SPEED_ATTACK, SPEED_SPELL
  gcd_haste_type gcd_type;

  /// Length of unhasted gcd triggered when used.
  timespan_t trigger_gcd;

  /// This is how far away the target can be from the player, and still be hit or targeted.
  double range;

  /**
   * @brief AoE ability area of influence.
   *
   * Typically anything that has a radius, but not a range, is based on the players location.
   */
  double radius;

  /// Weapon AP multiplier.
  double weapon_power_mod;

  /// Attack/Spell power scaling of the ability.
  struct {
  double direct, tick;
  } attack_power_mod, spell_power_mod;

  /**
   * @brief full damage variation in %
   *
   * Amount of variation of the full raw damage (base damage + AP/SP modified).
   * Example: amount_delta = 0.1 means variation between 95% and 105%.
   * Parsed from spell data.
   */
  double amount_delta;

  /// Amount of time the ability uses to execute before modifiers.
  timespan_t base_execute_time;

  /// Amount of time the ability uses between ticks.
  timespan_t base_tick_time;

  /// Default full duration of dot.
  timespan_t dot_duration;

  // Maximum number of DoT stacks.
  int dot_max_stack;

  /// Cost of using the ability.
  std::array< double, RESOURCE_MAX > base_costs, secondary_costs;

  /// Cost of using ability per periodic effect tick.
  std::array< double, RESOURCE_MAX > base_costs_per_tick;

  /// Minimum base direct damage
  double base_dd_min;

  /// Maximum base direct damage
  double base_dd_max;

  /// Base tick damage
  double base_td;

  /// base direct damage multiplier
  double base_dd_multiplier;

  /// base tick damage multiplier
  double base_td_multiplier;

  /// base damage multiplier (direct and tick damage)
  double base_multiplier;

  double base_hit;
  double base_crit;
  double crit_multiplier;
  double crit_bonus_multiplier;
  double crit_bonus;
  double base_dd_adder;
  double base_ta_adder;

  /// Weapon damage for the ability.
  double weapon_multiplier;

  /// Damage modifier for chained/AoE, exponentially increased by number of target hit.
  double chain_multiplier;

  /// Damage modifier for chained/AoE, linearly increasing with each target hit.
  double chain_bonus_damage;

  /// Static reduction of damage for AoE
  double base_aoe_multiplier;

  /// Static action cooldown duration multiplier
  double base_recharge_multiplier;

  /// Maximum distance that the ability can travel. Used on abilities that instantly move you, or nearly instantly move you to a location.
  double base_teleport_distance;

  /// Missile travel speed in yards / second
  double travel_speed;

  // Amount of resource for the energize to grant.
  double energize_amount;

  /**
   * @brief Movement Direction
   * @code
   * movement_directionality = movement_direction::MOVEMENT_OMNI; // Can move in any direction, ex: Heroic Leap, Blink. Generally set movement skills to this.
   * movement_directionality = movement_direction::MOVEMENT_TOWARDS; // Can only be used towards enemy target. ex: Charge
   * movement_directionality = movement_direction::MOVEMENT_AWAY; // Can only be used away from target. Ex: ????
   * @endcode
   */
  movement_direction_type movement_directionality;

  /// This is used to cache/track spells that have a parent driver, such as most channeled/ground aoe abilities.
  dot_t* parent_dot;

  std::vector< action_t* > child_action;

  /// This action will execute every tick
  action_t* tick_action;

  /// This action will execute every execute
  action_t* execute_action;

  /// This action will execute every impact - Useful for AoE debuffs
  action_t* impact_action;

  // Gain object of the same name as the action
  gain_t* gain;

  // Sets the behavior for when this action's resource energize occur.
  action_energize energize_type;

  // Resource for the energize to grant.
  resource_e energize_resource;

  /**
   * @brief Used to manipulate cooldown duration and charges.
   *
   * @code
   * cooldown -> duration = timespan_t::from_seconds( 20 ); //Ability has a cooldown of 20 seconds.
   * cooldown -> charges = 3; // Ability has 3 charges.
   * @endcode
   */
  cooldown_t* cooldown, *internal_cooldown;

  /** action statistics, merged by action-name */
  stats_t* stats;

  /** Execute event (e.g., "casting" of the action) */
  event_t* execute_event;

  /** Queue delay event (for queueing cooldowned actions shortly before they execute. */
  event_t* queue_event;

  /** Last available, effectively used execute time */
  timespan_t time_to_execute;

  /** Last available, effectively used travel time */
  timespan_t time_to_travel;

  /** Last available, effectively used resource cost */
  double last_resource_cost;

  /** Last available number of targets effectively hit */
  int num_targets_hit;

  /** Marker for sample action priority list reporting */
  char marker;

  /* The last time the action was executed */
  timespan_t last_used;

  // Options
  struct options_t {
    /**
     * moving (default: -1), when different from -1, will flag the action as usable only when the players are moving
     * (moving=1) or not moving (moving=0). When left to -1, the action will be usable anytime. The players happen to
     * move either because of a "movement" raid event, or because of "start_moving" actions. Note that actions which are
     * not usable while moving do not need to be flagged with "move=0", Simulationcraft is already aware of those
     * restrictions.
     *
     */
    int moving;

    int wait_on_ready;

    int max_cycle_targets;

    int target_number;

    /** Interrupt option. If true, channeled action can get interrupted. */
    bool interrupt;

    /**
     * Chain can be used to re-cast a channeled spell at the beginning of its last tick. This has two advantages over
     * waiting for the channel to complete before re-casting: 1) the gcd finishes sooner, and 2) it avoids the roughly
     * 1/4 second delay between the end of a channel and the beginning of the next cast.
     */
    bool chain;

    int cycle_targets;

    bool cycle_players;

    bool interrupt_immediate;

    std::string if_expr_str;
    std::string target_if_str;
    std::string interrupt_if_expr_str;
    std::string early_chain_if_expr_str;
    std::string cancel_if_expr_str;
    std::string sync_str;
    std::string target_str;
    options_t();
  } option;

  bool interrupt_global;

  std::unique_ptr<expr_t> if_expr;

  enum target_if_mode_e
  {
    TARGET_IF_NONE,
    TARGET_IF_FIRST,
    TARGET_IF_MIN,
    TARGET_IF_MAX
  } target_if_mode;

  std::unique_ptr<expr_t> target_if_expr;
  std::unique_ptr<expr_t> interrupt_if_expr;
  std::unique_ptr<expr_t> early_chain_if_expr;
  std::unique_ptr<expr_t> cancel_if_expr;
  action_t* sync_action;
  std::string signature_str;
  target_specific_t<dot_t> target_specific_dot;
  action_priority_list_t* action_list;

  /**
   * @brief Resource starvation tracking.
   *
   * Tracking proc triggered on resource-starved ready() calls.
   * Can be overridden by class modules for tracking purposes.
   */
  proc_t* starved_proc;
  uint_least64_t total_executions;

  /**
   * @brief Cooldown for specific APL line.
   *
   * Tied to a action_t object, and not shared by action-name,
   * this cooldown helps articifally restricting usage of a specific line
   * in the APL.
   */
  std::unique_ptr<cooldown_t> line_cooldown;
  const action_priority_t* signature;


  /// State of the last execute()
  action_state_t* execute_state;

  /// Optional - if defined before execute(), will be copied into execute_state
  action_state_t* pre_execute_state;

  unsigned snapshot_flags;

  unsigned update_flags;

  /**
   * Target Cache System
   * - list: contains the cached target pointers
   * - callback: unique_Ptr to callback
   * - is_valid: gets invalidated by the callback from the target list source.
   *  When the target list is requested in action_t::target_list(), it gets recalculated if
   *  flag is false, otherwise cached version is used
   */
  struct target_cache_t {
    std::vector< player_t* > list;
    bool is_valid;
    target_cache_t() : is_valid( false ) {}
  } mutable target_cache;

private:
  std::vector<std::unique_ptr<option_t>> options;
  action_state_t* state_cache;
  std::vector<travel_event_t*> travel_events;
public:
  action_t( action_e type, util::string_view token, player_t* p );
  action_t( action_e type, util::string_view token, player_t* p, const spell_data_t* s );

  virtual ~action_t();

  void add_child( action_t* child );

  void add_option( std::unique_ptr<option_t> new_option );

  void   check_spec( specialization_e );

  void   check_spell( const spell_data_t* );

  /**
   * @brief Spell Data associated with the action.
   *
   * spell_data_t::nil() if no spell data is available,
   * spell_data_t::not_found if spell given was not found.
   *
   * This means that if no spell data is available/found (eg. talent not available),
   * all spell data fields will be filled with 0 and can thus be used directly
   * without checking specifically for the spell_data_t::ok()
   */
  const spell_data_t& data() const
  { return ( *s_data ); }

  // return s_data_reporting if available, otherwise fallback to s_data
  const spell_data_t& data_reporting() const;

  dot_t* find_dot( player_t* target ) const;

  bool is_aoe() const
  {
    const int num_targets = n_targets();
    return num_targets == -1 || num_targets > 0;
  }

  const char* name() const
  { return name_str.c_str(); }

  size_t num_travel_events() const
  { return travel_events.size(); }

  bool has_travel_events() const
  { return ! travel_events.empty(); }

  timespan_t shortest_travel_event() const;

  bool has_travel_events_for( const player_t* target ) const;

  /** Determine if the action can have a resulting damage/heal amount > 0 */
  bool has_amount_result() const
  {
    return attack_power_mod.direct > 0 || attack_power_mod.tick > 0
        || spell_power_mod.direct > 0 || spell_power_mod.tick > 0
        || (weapon && weapon_multiplier > 0);
  }

  void parse_spell_data( const spell_data_t& );

  void parse_target_str();

  void remove_travel_event( travel_event_t* e );

  void reschedule_queue_event();

  rng::rng_t& rng();

  rng::rng_t& rng() const;

  player_t* select_target_if_target();

  void apply_affecting_aura(const spell_data_t*);
  void apply_affecting_effect( const spelleffect_data_t& effect );
  void apply_affecting_conduit( const conduit_data_t& conduit, int effect_num = 1 );
  void apply_affecting_conduit_effect( const conduit_data_t& conduit, size_t effect_num );

  action_state_t* get_state( const action_state_t* = nullptr );

private:
  friend struct action_state_t;
  void release_state( action_state_t* );

public:

  // =======================
  // Const virtual functions
  // =======================

  virtual bool verify_actor_level() const;

  virtual bool verify_actor_spec() const;

  virtual bool verify_actor_weapon() const;

  virtual double cost() const;

  virtual double base_cost() const;

  virtual double cost_per_tick( resource_e ) const;

  virtual timespan_t cooldown_duration() const;

  virtual timespan_t gcd() const;

  virtual double false_positive_pct() const;

  virtual double false_negative_pct() const;

  virtual timespan_t execute_time() const
  { return base_execute_time; }

  virtual timespan_t tick_time( const action_state_t* state ) const;

  virtual timespan_t travel_time() const;

  virtual timespan_t distance_targeting_travel_time( action_state_t* s ) const;

  virtual result_e calculate_result( action_state_t* /* state */ ) const
  {
    assert( false );
    return RESULT_UNKNOWN;
  }

  virtual block_result_e calculate_block_result( action_state_t* s ) const;

  virtual double calculate_direct_amount( action_state_t* state ) const;

  virtual double calculate_tick_amount( action_state_t* state, double multiplier ) const;

  virtual double calculate_weapon_damage( double attack_power ) const;

  virtual double calculate_crit_damage_bonus( action_state_t* s ) const;

  virtual double target_armor(player_t* t) const;

  virtual double recharge_multiplier( const cooldown_t& ) const
  { return base_recharge_multiplier; }

  /** Cooldown base duration for action based cooldowns. */
  virtual timespan_t cooldown_base_duration( const cooldown_t& cd ) const;

  virtual resource_e current_resource() const
  { return resource_current; }

  virtual int n_targets() const
  { return aoe; }

  virtual double last_tick_factor( const dot_t* d, timespan_t time_to_tick, timespan_t duration ) const;

  virtual result_amount_type amount_type( const action_state_t* /* state */, bool /* periodic */ = false ) const
  { return result_amount_type::NONE; }

  virtual result_amount_type report_amount_type( const action_state_t* state ) const;

  /// Use when damage schools change during runtime.
  virtual school_e get_school() const
  { return school; }

  virtual double miss_chance( double /* hit */, player_t* /* target */ ) const
  { return 0; }

  virtual double dodge_chance( double /* expertise */, player_t* /* target */ ) const
  { return 0; }

  virtual double parry_chance( double /* expertise */, player_t* /* target */ ) const
  { return 0; }

  virtual double glance_chance( int /* delta_level */ ) const
  { return 0; }

  virtual double block_chance( action_state_t* /* state */ ) const
  { return 0; }

  virtual double crit_block_chance( action_state_t* /* state */  ) const
  { return 0; }

  virtual double total_crit_bonus( const action_state_t* /* state */ ) const; // Check if we want to move this into the stateless system.

  virtual int num_targets() const;

  virtual size_t available_targets( std::vector< player_t* >& ) const;

  virtual std::vector< player_t* >& target_list() const;

  virtual player_t* find_target_by_number( int number ) const;

  virtual bool execute_targeting( action_t* action ) const;

  virtual std::vector<player_t*> targets_in_range_list( std::vector< player_t* >& tl ) const;

  virtual std::vector<player_t*>& check_distance_targeting( std::vector< player_t* >& tl ) const;

  virtual double ppm_proc_chance( double PPM ) const;

  virtual bool usable_moving() const;

  virtual timespan_t composite_dot_duration( const action_state_t* ) const;

  virtual double attack_direct_power_coefficient( const action_state_t* ) const
  { return attack_power_mod.direct; }

  virtual double amount_delta_modifier( const action_state_t* ) const
  { return amount_delta; }

  virtual double attack_tick_power_coefficient( const action_state_t* ) const
  { return attack_power_mod.tick; }

  virtual double spell_direct_power_coefficient( const action_state_t* ) const
  { return spell_power_mod.direct; }

  virtual double spell_tick_power_coefficient( const action_state_t* ) const
  { return spell_power_mod.tick; }

  virtual double base_da_min( const action_state_t* ) const
  { return base_dd_min; }

  virtual double base_da_max( const action_state_t* ) const
  { return base_dd_max; }

  virtual double base_ta( const action_state_t* ) const
  { return base_td; }

  virtual double bonus_da( const action_state_t* ) const
  { return base_dd_adder; }

  virtual double bonus_ta( const action_state_t* ) const
  { return base_ta_adder; }

  virtual double range_() const
  { return range; }

  virtual double radius_() const
  { return radius; }

  virtual double action_multiplier() const
  { return base_multiplier; }

  virtual double action_da_multiplier() const
  { return base_dd_multiplier; }

  virtual double action_ta_multiplier() const
  { return base_td_multiplier; }

  virtual double composite_hit() const
  { return base_hit; }

  virtual double composite_crit_chance() const
  { return base_crit; }

  virtual double composite_crit_chance_multiplier() const
  { return 1.0; }

  virtual double composite_crit_damage_bonus_multiplier() const
  { return crit_bonus_multiplier; }

  virtual double composite_haste() const
  { return 1.0; }

  virtual attack_power_type get_attack_power_type() const
  { return ap_type; }

  virtual double composite_attack_power() const;

  virtual double composite_spell_power() const;

  virtual double composite_target_crit_chance( player_t* target ) const;

  virtual double composite_target_crit_damage_bonus_multiplier( player_t* ) const
  { return 1.0; }

  virtual double composite_target_multiplier( player_t* target ) const;

  virtual double composite_target_damage_vulnerability( player_t* target ) const;

  virtual double composite_versatility( const action_state_t* ) const
  { return 1.0; }

  virtual double composite_leech( const action_state_t* ) const;

  virtual double composite_run_speed() const;

  virtual double composite_avoidance() const;

  virtual double composite_corruption() const;

  virtual double composite_corruption_resistance() const;

  virtual double composite_total_corruption() const;

  /// Direct amount multiplier due to debuffs on the target
  virtual double composite_target_da_multiplier( player_t* target ) const
  { return composite_target_multiplier( target ); }

  /// Tick amount multiplier due to debuffs on the target
  virtual double composite_target_ta_multiplier( player_t* target ) const
  { return composite_target_multiplier( target ); }

  virtual double composite_da_multiplier(const action_state_t* /* s */) const;

  /// Normal ticking modifiers that are updated every tick
  virtual double composite_ta_multiplier(const action_state_t* /* s */) const;

  /// Persistent modifiers that are snapshot at the start of the spell cast
  virtual double composite_persistent_multiplier(const action_state_t*) const;

  /**
   * @brief Generic aoe multiplier for the action.
   *
   * Used in action_t::calculate_direct_amount, and applied after
   * other (passive) aoe multipliers are applied.
   */
  virtual double composite_aoe_multiplier( const action_state_t* ) const
  { return 1.0; }

  virtual double composite_target_mitigation(player_t* t, school_e s) const;

  virtual double composite_player_critical_multiplier(const action_state_t* s) const;

  /// Action proc type, needed for dynamic aoe stuff and such.
  virtual proc_types proc_type() const
  { return PROC1_INVALID; }

  virtual bool has_movement_directionality() const;

  virtual double composite_teleport_distance( const action_state_t* ) const
  { return base_teleport_distance; }

  virtual timespan_t calculate_dot_refresh_duration(const dot_t*,
      timespan_t triggered_duration) const;

  // Helper for dot refresh expression, overridable on action level
  virtual bool dot_refreshable( const dot_t* dot, timespan_t triggered_duration ) const;

  virtual double composite_energize_amount( const action_state_t* ) const
  { return energize_amount; }

  virtual resource_e energize_resource_() const
  { return energize_resource; }

  virtual action_energize energize_type_() const
  { return energize_type; }

  virtual gain_t* energize_gain( const action_state_t* /* state */ ) const
  { return gain; }

  // ==========================
  // mutating virtual functions
  // ==========================

  virtual void parse_effect_data( const spelleffect_data_t& );

  virtual void parse_options( util::string_view options_str );

  virtual void consume_resource();

  virtual void execute();

  virtual void tick(dot_t* d);

  virtual void last_tick(dot_t* d);

  virtual void assess_damage(result_amount_type, action_state_t* assess_state);

  virtual void record_data(action_state_t* data);

  virtual void schedule_execute(action_state_t* execute_state = nullptr);

  virtual void queue_execute( execute_type type );

  virtual void reschedule_execute(timespan_t time);

  virtual void start_gcd();

  virtual void update_ready(timespan_t cd_duration = timespan_t::min());

  /// Is the _ability_ ready based on spell characteristics
  virtual bool ready();
  /// Is the action ready, as a combination of ability characteristics and user input? Main
  /// ntry-point when selecting something to do for an actor.
  virtual bool action_ready();
  /// Select a target to execute on
  virtual bool select_target();
  /// Target readiness state checking
  virtual bool target_ready( player_t* target );

  /// Ability usable during an on-going cast
  virtual bool usable_during_current_cast() const;
  /// Ability usable during the on-going global cooldown
  virtual bool usable_during_current_gcd() const;

  virtual void init();

  virtual void init_finished();

  virtual void reset();

  virtual void cancel();

  virtual void interrupt_action();

  // Perform activation duties for the action. This is used to "enable" the action when the actor
  // becomes active.
  virtual void activate();

  virtual std::unique_ptr<expr_t> create_expression(util::string_view name);

  virtual action_state_t* new_state();

  virtual void do_schedule_travel( action_state_t*, timespan_t );

  virtual void schedule_travel( action_state_t* );

  virtual void impact( action_state_t* );

  virtual void trigger_dot( action_state_t* );

  virtual void snapshot_internal( action_state_t*, unsigned flags, result_amount_type );

  virtual void snapshot_state( action_state_t* s, result_amount_type rt )
  { snapshot_internal( s, snapshot_flags, rt ); }

  virtual void update_state( action_state_t* s, result_amount_type rt )
  { snapshot_internal( s, update_flags, rt ); }

  event_t* start_action_execute_event( timespan_t time, action_state_t* execute_state = nullptr );

  virtual bool consume_cost_per_tick( const dot_t& dot );

  virtual void do_teleport( action_state_t* );

  virtual dot_t* get_dot( player_t* = nullptr );

  virtual void acquire_target( retarget_source /* event */, player_t* /* context */, player_t* /* candidate_target */ );

  virtual void set_target( player_t* target );

  virtual void gain_energize_resource( resource_e resource_type, double amount, gain_t* gain );

  // ================
  // Static functions
  // ================

  static bool result_is_hit( result_e r )
  {
    return( r == RESULT_HIT        ||
            r == RESULT_CRIT       ||
            r == RESULT_GLANCE     ||
            r == RESULT_NONE       );
  }

  static bool result_is_miss( result_e r )
  {
    return( r == RESULT_MISS   ||
            r == RESULT_DODGE  ||
            r == RESULT_PARRY );
  }

  static bool result_is_block( block_result_e r )
  {
    return( r == BLOCK_RESULT_BLOCKED || r == BLOCK_RESULT_CRIT_BLOCKED );
  }

  friend void format_to( const action_t&, fmt::format_context::iterator );
};

struct call_action_list_t : public action_t
{
  action_priority_list_t* alist;

  call_action_list_t( player_t*, util::string_view );
  void execute() override
  { assert( 0 ); }
};

struct swap_action_list_t : public action_t
{
  action_priority_list_t* alist;

  swap_action_list_t( player_t* player, util::string_view options_str,
    util::string_view name = "swap_action_list" );

  void execute() override;
  bool ready() override;
};

struct run_action_list_t : public swap_action_list_t
{
  run_action_list_t( player_t* player, util::string_view options_str );

  void execute() override;
};
