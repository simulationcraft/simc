// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "util/generic.hpp"

#include "simulationcraft.hpp"

// ==========================================================================
// Enemy
// ==========================================================================

namespace
{  // UNNAMED NAMESPACE

enum class tank_dummy_e
{
  NONE = 0,
  WEAK,
  DUNGEON,
  RAID,
  HEROIC,
  MYTHIC,
  MAX
};

struct enemy_t : public player_t
{
  size_t enemy_id;
  double fixed_health, initial_health;
  double fixed_health_percentage, initial_health_percentage;
  double health_recalculation_dampening_exponent;
  timespan_t waiting_time;

  int current_target;
  int apply_damage_taken_debuff;
  double custom_armor_coeff;

  std::vector<buff_t*> buffs_health_decades;

  enemy_t( sim_t* s, util::string_view n, race_e r = RACE_HUMANOID, player_e type = ENEMY )
    : player_t( s, type, n, r ),
      enemy_id( s->target_list.size() ),
      fixed_health( 0 ),
      initial_health( 0 ),
      fixed_health_percentage( 0 ),
      initial_health_percentage( 100.0 ),
      health_recalculation_dampening_exponent( 1.0 ),
      waiting_time( timespan_t::from_seconds( 1.0 ) ),
      current_target( 0 ),
      apply_damage_taken_debuff( 0 ),
      custom_armor_coeff( 0 )
  {
    s->target_list.push_back( this );
    position_str = "front";
    combat_reach = 4.0;
  }

  role_e primary_role() const override
  {
    return ROLE_TANK;
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_HEALTH;
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
  void init_race() override;
  void init_base_stats() override;
  void init_defense() override;
  void create_buffs() override;
  void init_resources( bool force = false ) override;
  void init_target() override;
  virtual std::string generate_action_list();
  virtual void generate_heal_raid_event();
  void init_action_list() override;
  void init_stats() override;
  double resource_loss( resource_e, double, gain_t*, action_t* ) override;
  void create_options() override;
  pet_t* create_pet( util::string_view add_name, util::string_view pet_type = {} ) override;
  void create_pets() override;
  double health_percentage() const override;
  timespan_t time_to_percent( double ) const override;
  void combat_begin() override;
  void combat_end() override;
  virtual void recalculate_health();
  void demise() override;
  double armor_coefficient( int level, tank_dummy_e diff );
  std::unique_ptr<expr_t> create_expression( util::string_view expression_str ) override;
  timespan_t available() const override
  {
    return waiting_time;
  }

  void actor_changed() override
  {
    if ( !sim->overrides.target_health.empty() )
    {
      initial_health =
          static_cast<double>( sim->overrides.target_health[ enemy_id % sim->overrides.target_health.size() ] );
    }
    else
    {
      initial_health = fixed_health;
    }

    this->default_target = this->target = sim->player_no_pet_list[ sim->current_index ];
    range::for_each( action_list, [ this ]( action_t* action ) {
      if ( action->harmful )
      {
        action->default_target = action->target = sim->player_no_pet_list[ sim->current_index ];
      }
    } );
  }

  bool taunt( player_t* source ) override;
  void reset_auto_attacks( timespan_t delay, proc_t* proc = nullptr ) override;
  void delay_auto_attacks( timespan_t delay, proc_t* proc = nullptr ) override;

  std::string generate_tank_action_list( tank_dummy_e );
  void add_tank_heal_raid_event( tank_dummy_e );
};

namespace actions
{
// Enemy actions are generic to serve both enemy_t and enemy_add_t,
// so they can only rely on player_t and should have no knowledge of class definitions

template <typename ACTION_TYPE>
struct enemy_action_t : public ACTION_TYPE
{
  using action_type_t = ACTION_TYPE;
  using base_t        = enemy_action_t<ACTION_TYPE>;

  bool apply_debuff;
  std::string dmg_type_override;
  int num_debuff_stacks;
  double damage_range;
  timespan_t cooldown_;
  int aoe_tanks;

  enemy_action_t( util::string_view name, player_t* player )
    : action_type_t( name, player ),
      apply_debuff( false ),
      num_debuff_stacks( -1000000 ),
      damage_range( -1 ),
      cooldown_( timespan_t::zero() ),
      aoe_tanks( 0 )
  {
    this->add_option( opt_float( "damage", this->base_dd_min ) );
    this->add_option( opt_timespan( "attack_speed", this->base_execute_time ) );
    this->add_option( opt_int( "apply_debuff", num_debuff_stacks ) );
    this->add_option( opt_int( "aoe_tanks", aoe_tanks ) );
    this->add_option( opt_float( "range", damage_range ) );
    this->add_option( opt_timespan( "cooldown", cooldown_ ) );
    this->add_option( opt_string( "type", dmg_type_override ) );

    this->special     = true;
    dmg_type_override = "none";
  }

  virtual void set_name_string()
  {
    this->name_str = this->name_str + "_" + this->target->name();
  }

  // this is only used by helper structures
  std::string filter_options_list( util::string_view options_str )
  {
    auto splits = util::string_split<util::string_view>( options_str, "," );
    std::string filtered_options;
    for ( auto split : splits )
    {
      if ( !util::str_in_str_ci( split, "if=" ) )
      {
        if ( filtered_options.length() > 0 )
          filtered_options += ",";
        filtered_options += std::string( split );
      }
    }
    return filtered_options;
  }

  void init() override
  {
    action_type_t::init();

    set_name_string();
    this->cooldown = this->player->get_cooldown( this->name_str );

    this->stats         = this->player->get_stats( this->name_str, this );
    this->stats->school = this->school;

    if ( cooldown_ > timespan_t::zero() )
      this->cooldown->duration = cooldown_;

    // if the debuff increment size is specified in the options string, it takes precedence
    if ( num_debuff_stacks != -1e6 && num_debuff_stacks != 0 )
      apply_debuff = true;

    if ( enemy_t* e = dynamic_cast<enemy_t*>( this->player ) )
    {
      // if the debuff increment hasn't been specified at all, set it
      if ( num_debuff_stacks == -1e6 )
      {
        if ( e->apply_damage_taken_debuff == 0 )
          num_debuff_stacks = 0;
        else
        {
          apply_debuff      = true;
          num_debuff_stacks = e->apply_damage_taken_debuff;
        }
      }
    }
    if ( dmg_type_override != "none" )
      this->school = util::parse_school_type( dmg_type_override );

    if ( this->base_dd_max < this->base_dd_min )
      this->base_dd_max = this->base_dd_min;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    // TODO: This does not work for heals at all, as it presumes enemies in the
    // actor list.
    tl.clear();
    tl.push_back( this->target );

    if ( this->sim->single_actor_batch )
    {
      player_t* actor = this->sim->player_no_pet_list[ this->sim->current_index ];
      if ( !actor->is_sleeping() && !actor->is_enemy() && actor->primary_role() == ROLE_TANK && actor != this->target &&
           actor != this->sim->heal_target )
        tl.push_back( actor );
    }
    else
    {
      for ( size_t i = 0, actors = this->sim->actor_list.size(); i < actors; i++ )
      {
        player_t* actor = this->sim->actor_list[ i ];
        // only add non heal_target tanks to this list for now
        if ( !actor->is_sleeping() && !actor->is_enemy() && actor->primary_role() == ROLE_TANK &&
             actor != this->target && actor != this->sim->heal_target )
          tl.push_back( actor );
      }
    }
    // if we have no target (no tank), add the healing target as substitute
    if ( tl.empty() )
    {
      tl.push_back( this->sim->heal_target );
    }

    return tl.size();
  }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    // force boss attack size to vary regardless of whether the sim itself does
    int previous_average_range_state = this->sim->average_range;
    this->sim->average_range         = 0;

    double amount = action_type_t::calculate_direct_amount( s );

    this->sim->average_range = previous_average_range_state;

    return amount;
  }

  void impact( action_state_t* s ) override
  {
    if ( apply_debuff && num_debuff_stacks >= 0 )
      this->target->debuffs.damage_taken->trigger( num_debuff_stacks );
    else if ( apply_debuff )
      this->target->debuffs.damage_taken->decrement( -num_debuff_stacks );

    action_type_t::impact( s );
  }

  void tick( dot_t* d ) override
  {
    if ( apply_debuff && num_debuff_stacks >= 0 )
      this->target->debuffs.damage_taken->trigger( num_debuff_stacks );
    else if ( apply_debuff )
      this->target->debuffs.damage_taken->decrement( -num_debuff_stacks );

    action_type_t::tick( d );
  }

  bool ready() override
  {
    if ( this->sim->single_actor_batch == 1 && this->target->primary_role() != ROLE_TANK )
    {
      return false;
    }

    return action_type_t::ready();
  }
};

// enemy_action_driver_t handles all of the weird stuff necessary to aoe tanks
// it's basically a clone of whatever the child action is, but with extra fluff!

template <typename CHILD_ACTION_TYPE>
struct enemy_action_driver_t : public CHILD_ACTION_TYPE
{
  using child_action_type_t = CHILD_ACTION_TYPE;
  using base_t              = enemy_action_driver_t<CHILD_ACTION_TYPE>;

  int aoe_tanks;
  std::vector<child_action_type_t*> ch_list;
  size_t num_attacks;

  enemy_action_driver_t( player_t* player, util::string_view options_str )
    : child_action_type_t( player, options_str ), aoe_tanks( 0 ), num_attacks( 0 )
  {
    this->add_option( opt_int( "aoe_tanks", aoe_tanks ) );
    this->parse_options( options_str );

    // construct the target list
    std::vector<player_t*> target_list;

    if ( this->sim->single_actor_batch )
    {
      target_list.push_back( this->sim->player_no_pet_list[ this->sim->current_index ] );
    }
    else
    {
      for ( size_t i = 0; i < this->sim->player_no_pet_list.size(); i++ )
        if ( this->sim->player_no_pet_list[ i ]->primary_role() == ROLE_TANK )
          target_list.push_back( this->sim->player_no_pet_list[ i ] );
    }

    // create a separate action for each potential target
    for ( auto* i : target_list )
    {
      auto ch        = new child_action_type_t( player, this->filter_options_list( options_str ) );
      ch->target     = i;
      ch->background = true;
      ch_list.push_back( ch );
    }

    // handle the aoe_tanks flag: negative or 1 for all, 2+ represents 2+ random targets
    // Do this by defining num_attacks appropriately for later use
    if ( aoe_tanks == 1 || aoe_tanks < 0 )
      num_attacks = ch_list.size();
    else
      num_attacks = static_cast<size_t>( aoe_tanks );

    this->interrupt_auto_attack = false;
    // if there are no valid targets, disable
    if ( ch_list.empty() )
      this->background = true;
  }

  void schedule_execute( action_state_t* s ) override
  {
    // first, execute on the primary target
    child_action_type_t::schedule_execute( s );

    // if we're hitting more than one target, handle the children
    if ( num_attacks > 1 )
    {
      if ( num_attacks == ch_list.size() )
      {
        // hit everyone
        for ( size_t i = 0; i < ch_list.size(); i++ )
          if ( ch_list[ i ]->target != this->target )
            ch_list[ i ]->schedule_execute( s );
      }
      else
      {
        // hit a random subset
        std::vector<child_action_type_t*> rt_list;
        rt_list  = ch_list;
        size_t i = 1;
        while ( i < num_attacks )
        {
          size_t element = static_cast<size_t>( std::floor( this->rng().real() * rt_list.size() ) );
          if ( rt_list[ element ]->target != this->target )
          {
            rt_list[ element ]->schedule_execute( s );
            i++;
          }
          // remove this element
          rt_list.erase( rt_list.begin() + element );

          // infinte loop check
          if ( rt_list.empty() )
            break;
        }
      }
    }
  }

  bool ready() override
  {
    if ( this->sim->single_actor_batch == 1 && this->target->primary_role() != ROLE_TANK )
    {
      return false;
    }

    return child_action_type_t::ready();
  }
};

// Melee ====================================================================

struct melee_t : public enemy_action_t<melee_attack_t>
{
  action_t* driver;
  bool first;

  melee_t( util::string_view name, player_t* player, action_t* a, util::string_view options_str )
    : base_t( name, player ), driver( a ), first( false )
  {
    school            = SCHOOL_PHYSICAL;
    trigger_gcd       = timespan_t::zero();
    base_dd_min       = 1040;
    base_execute_time = timespan_t::from_seconds( 1.5 );
    may_crit = background = repeating = not_a_proc = true;
    may_dodge = may_parry = may_block = true;
    special                           = false;

    parse_options( options_str );
  }

  void init() override
  {
    base_t::init();

    // check that the specified damage range is sane
    if ( damage_range > base_dd_min || damage_range < 0 )
      damage_range = 0.1 * base_dd_min;  // if not, set to +/-10%

    // set damage range to mean +/- range
    base_dd_max = base_dd_min + damage_range;
    base_dd_min -= damage_range;

    // if the execute time is somehow less than 10 ms, set it back to the default of 1.5 seconds
    if ( base_execute_time < timespan_t::from_seconds( 0.01 ) )
      base_execute_time = timespan_t::from_seconds( 1.5 );
  }

  timespan_t execute_time() const override
  {
    timespan_t et = base_t::execute_time();

    if ( first )
    {
      et += this->base_execute_time / 2;
    }

    return et;
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public enemy_action_t<attack_t>
{
  std::vector<melee_t*> mh_list;

  // default constructor
  auto_attack_t( player_t* p, util::string_view options_str ) : base_t( "auto_attack", p ), mh_list( 0 )
  {
    parse_options( options_str );

    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();

    size_t num_attacks = 0;
    if ( this->sim->single_actor_batch )
      num_attacks = 1;
    else if ( aoe_tanks == 1 || aoe_tanks < 0 )
      num_attacks = this->player->sim->actor_list.size();
    else
      num_attacks = static_cast<size_t>( aoe_tanks );

    std::vector<player_t*> target_list;
    if ( aoe_tanks )
    {
      for ( auto* i : sim->player_no_pet_list )
        if ( target_list.size() < num_attacks && i->primary_role() == ROLE_TANK )
          target_list.push_back( i );
    }
    else
      target_list.push_back( target );

    for ( auto* t : target_list )
    {
      melee_t* mh = new melee_t( "melee_main_hand", p, this, options_str );
      mh->weapon  = &( p->main_hand_weapon );
      mh->target  = t;
      mh_list.push_back( mh );
    }

    if ( !mh_list.empty() )
      p->main_hand_attack = mh_list[ 0 ];
  }

  void set_name_string() override
  {
    if ( mh_list.size() > 1 )
      this->name_str = this->name_str + "_tanks";
    else
      base_t::set_name_string();
  }

  void init() override
  {
    base_t::init();

    if ( enemy_t* e = dynamic_cast<enemy_t*>( player ) )
    {
      for ( auto* mh : mh_list )
      {
        // if the number of debuff stacks hasn't been specified yet, set it
        if ( mh->num_debuff_stacks == -1e6 )
        {
          if ( e->apply_damage_taken_debuff == 0 )
            mh->num_debuff_stacks = 0;
          else
          {
            mh->apply_debuff      = true;
            mh->num_debuff_stacks = e->apply_damage_taken_debuff;
          }
        }
      }
    }
  }

  void execute() override
  {
    player->main_hand_attack = mh_list[ 0 ];
    for ( auto* mh : mh_list )
      mh->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() || !player->main_hand_attack )
      return false;
    if ( !base_t::ready() )
      return false;
    return ( player->main_hand_attack->execute_event == nullptr );  // not swinging
  }
};

// Auto Attack Off-Hand =======================================================

struct auto_attack_off_hand_t : public enemy_action_t<attack_t>
{
  std::vector<melee_t*> oh_list;

  // default constructor
  auto_attack_off_hand_t( player_t* player, util::string_view options_str )
    : base_t( "auto_attack_off_hand", player ), oh_list( 0 )
  {
    parse_options( options_str );

    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();

    size_t num_attacks = 0;
    if ( this->sim->single_actor_batch )
      num_attacks = 1;
    else if ( aoe_tanks == 1 || aoe_tanks < 0 )
      num_attacks = this->player->sim->actor_list.size();
    else
      num_attacks = static_cast<size_t>( aoe_tanks );

    std::vector<player_t*> target_list;
    if ( aoe_tanks )
    {
      for ( auto* p : sim->player_no_pet_list )
        if ( target_list.size() < num_attacks && p->primary_role() == ROLE_TANK )
          target_list.push_back( p );
    }
    else
      target_list.push_back( target );

    for ( auto* t : target_list )
    {
      melee_t* oh = new melee_t( "melee_off_hand", player, this, options_str );
      oh->weapon  = &( player->off_hand_weapon );
      oh->target  = t;
      oh_list.push_back( oh );
    }

    player->off_hand_attack = oh_list[ 0 ];
  }

  void init() override
  {
    base_t::init();

    if ( enemy_t* e = dynamic_cast<enemy_t*>( player ) )
    {
      for ( auto* oh : oh_list )
      {
        // if the number of debuff stacks hasn't been specified yet, set it
        if ( oh && oh->num_debuff_stacks == -1e6 )
        {
          if ( e->apply_damage_taken_debuff == 0 )
            oh->num_debuff_stacks = 0;
          else
          {
            oh->apply_debuff      = true;
            oh->num_debuff_stacks = e->apply_damage_taken_debuff;
          }
        }
      }
    }
  }

  void execute() override
  {
    player->off_hand_attack = oh_list[ 0 ];
    for ( auto* oh : oh_list )
    {
      oh->first = true;
      oh->schedule_execute();
    }
  }

  bool ready() override
  {
    if ( player->is_moving() )
      return false;
    return ( player->off_hand_attack->execute_event == nullptr );  // not swinging
  }
};

// Melee Nuke ===============================================================

struct melee_nuke_t : public enemy_action_t<melee_attack_t>
{
  // default constructor
  melee_nuke_t( player_t* p, util::string_view options_str ) : base_t( "melee_nuke", p )
  {
    school   = SCHOOL_PHYSICAL;
    may_miss = may_dodge = may_parry = false;
    may_block                        = true;
    base_execute_time                = timespan_t::from_seconds( 3.0 );
    base_dd_min                      = 2800;

    parse_options( options_str );
  }

  void init() override
  {
    base_t::init();

    // check that the specified damage range is sane
    if ( damage_range > base_dd_min || damage_range < 0 )
      damage_range = 0.1 * base_dd_min;  // if not, set to +/-10%

    base_dd_max = base_dd_min + damage_range;
    base_dd_min -= damage_range;

    if ( base_execute_time < timespan_t::zero() )
      base_execute_time = timespan_t::from_seconds( 3.0 );

    if ( base_execute_time < trigger_gcd )
    {
      trigger_gcd = base_execute_time;
      min_gcd     = base_execute_time;
    }
  }
};

struct melee_nuke_driver_t : public enemy_action_driver_t<melee_nuke_t>
{
  melee_nuke_driver_t( player_t* p, util::string_view options_str )
    : enemy_action_driver_t<melee_nuke_t>( p, options_str )
  {
  }
};

// Spell Nuke ===============================================================

struct spell_nuke_t : public enemy_action_t<spell_t>
{
  spell_nuke_t( player_t* p, util::string_view options_str ) : base_t( "spell_nuke", p )
  {
    school            = SCHOOL_FIRE;
    base_execute_time = timespan_t::from_seconds( 3.0 );
    base_dd_min       = 2400;

    parse_options( options_str );
  }

  void init() override
  {
    base_t::init();

    // check that the specified damage range is sane
    if ( damage_range > base_dd_min || damage_range < 0 )
      damage_range = 0.1 * base_dd_min;  // if not, set to +/-10%

    base_dd_max = base_dd_min + damage_range;
    base_dd_min -= damage_range;

    if ( base_execute_time < timespan_t::zero() )
      base_execute_time = timespan_t::from_seconds( 3.0 );

    if ( base_execute_time < trigger_gcd )
    {
      trigger_gcd = base_execute_time;
      min_gcd     = base_execute_time;
    }
  }
};

struct spell_nuke_driver_t : public enemy_action_driver_t<spell_nuke_t>
{
  spell_nuke_driver_t( player_t* p, util::string_view options_str )
    : enemy_action_driver_t<spell_nuke_t>( p, options_str )
  {
  }
};

// Spell DoT ================================================================

struct spell_dot_t : public enemy_action_t<spell_t>
{
  bool is_bleed;

  spell_dot_t( player_t* p, util::string_view options_str ) :
    base_t( "spell_dot", p ), is_bleed( false )
  {
    school            = SCHOOL_FIRE;
    base_tick_time    = timespan_t::from_seconds( 1.0 );
    base_execute_time = timespan_t::from_seconds( 1.0 );
    dot_duration      = timespan_t::from_seconds( 10.0 );
    base_td           = 200;
    tick_may_crit     = false;
    may_crit          = false;

    // Replace damage option
    add_option( opt_float( "damage", base_td ) );
    add_option( opt_timespan( "dot_duration", dot_duration ) );
    add_option( opt_timespan( "tick_time", base_tick_time ) );
    add_option( opt_bool( "bleed", is_bleed ) );
    parse_options( options_str );
  }

  void init() override
  {
    base_t::init();

    if ( is_bleed )
      school = SCHOOL_PHYSICAL;

    if ( base_tick_time < timespan_t::zero() )  // User input sanity check
      base_tick_time = timespan_t::from_seconds( 1.0 );

    if ( base_execute_time < trigger_gcd )
    {
      trigger_gcd = base_execute_time;
      min_gcd     = base_execute_time;
    }
  }

  void execute() override
  {
    target_cache.is_valid = false;

    base_t::execute();
  }
};

struct spell_dot_driver_t : public enemy_action_driver_t<spell_dot_t>
{
  spell_dot_driver_t( player_t* p, util::string_view options_str )
    : enemy_action_driver_t<spell_dot_t>( p, options_str )
  {
    base_tick_time = timespan_t::from_seconds( 1.0 );

    add_option( opt_float( "damage", base_td ) );
    add_option( opt_timespan( "dot_duration", dot_duration ) );
    add_option( opt_timespan( "tick_time", base_tick_time ) );
    add_option( opt_bool( "bleed", is_bleed ) );
    parse_options( options_str );
  }

  void schedule_execute( action_state_t* s ) override
  {
    target_cache.is_valid = false;
    base_t::schedule_execute( s );
  }
};

// Spell AoE ================================================================

struct spell_aoe_t : public enemy_action_t<spell_t>
{
  // default constructor
  spell_aoe_t( player_t* p, util::string_view options_str ) : base_t( "spell_aoe", p )
  {
    school            = SCHOOL_FIRE;
    base_execute_time = timespan_t::from_seconds( 3.0 );
    base_dd_min       = 200;
    aoe               = -1;
    may_crit          = false;

    parse_options( options_str );
  }

  void init() override
  {
    base_t::init();

    base_dd_max = base_dd_min;
    if ( base_execute_time < timespan_t::from_seconds( 0.01 ) )
      base_execute_time = timespan_t::from_seconds( 3.0 );
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    // TODO: This does not work for heals at all, as it presumes enemies in the
    // actor list.

    tl.clear();
    tl.push_back( target );

    if ( sim->single_actor_batch )
    {
      player_t* actor = sim->player_no_pet_list[ sim->current_index ];
      if ( !actor->is_sleeping() && !actor->is_enemy() && actor->primary_role() == ROLE_TANK && actor != this->target &&
           actor != this->sim->heal_target )
        tl.push_back( actor );
    }
    else
    {
      for ( size_t i = 0, actors = sim->actor_list.size(); i < actors; ++i )
      {
        if ( !sim->actor_list[ i ]->is_sleeping() && !sim->actor_list[ i ]->is_enemy() &&
             sim->actor_list[ i ] != target )
          tl.push_back( sim->actor_list[ i ] );
      }
    }

    return tl.size();
  }
};

// Summon Add ===============================================================

struct summon_add_t : public spell_t
{
  std::string add_name;
  timespan_t summoning_duration;
  pet_t* pet;

  summon_add_t( player_t* p, util::string_view options_str )
    : spell_t( "summon_add", p, spell_data_t::nil() ),
      add_name( "" ),
      summoning_duration( timespan_t::zero() ),
      pet( nullptr )
  {
    add_option( opt_string( "name", add_name ) );
    add_option( opt_timespan( "duration", summoning_duration ) );
    add_option( opt_timespan( "cooldown", cooldown->duration ) );
    parse_options( options_str );

    school = SCHOOL_PHYSICAL;
    pet    = p->find_pet( add_name );
    if ( !pet )
    {
      sim->error( "Player {} unable to find pet {} for summons.", p->name(), add_name );
      sim->cancel();
    }

    harmful = false;

    trigger_gcd = timespan_t::from_seconds( 1.5 );
  }

  void execute() override
  {
    spell_t::execute();

    player->summon_pet( add_name, summoning_duration );
  }

  bool ready() override
  {
    if ( !pet->is_sleeping() )
      return false;

    return spell_t::ready();
  }
};

// Pause Action ===============================================================

struct pause_action_t : public action_t
{
  timespan_t duration_stddev, duration_min, duration_max;
  timespan_t cooldown_stddev, cooldown_min, cooldown_max;

  pause_action_t( player_t* p, util::string_view options_str )
    : action_t( ACTION_OTHER, "pause_action", p, spell_data_t::nil() ),
      duration_stddev( 0_s ),
      duration_min( 0_s ),
      duration_max( 0_s ),
      cooldown_stddev( 0_s ),
      cooldown_min( 0_s ),
      cooldown_max( 0_s )
  {
    // Dummy action to help model a boss attacking a different tank
    // or just diverting their attention from auto attacking the player in general
    // (targeting another player for an ability, dialogue/animations, etc.)
    interrupt_auto_attack = special = true;

    // Use the same duration and cooldown min/max/stddev system as raid events
    add_option( opt_timespan( "duration", base_execute_time ) );
    add_option( opt_timespan( "duration_stddev", duration_stddev ) );
    add_option( opt_timespan( "duration_min", duration_min ) );
    add_option( opt_timespan( "duration_max", duration_max ) );

    timespan_t cooldown_duration;
    add_option( opt_timespan( "cooldown", cooldown_duration ) );
    add_option( opt_timespan( "cooldown_stddev", cooldown_stddev ) );
    add_option( opt_timespan( "cooldown_min", cooldown_min ) );
    add_option( opt_timespan( "cooldown_max", cooldown_max ) );

    // By default, only interrupts auto attack without resetting the swing timer, but that can be changed
    add_option( opt_bool( "reset_auto_attack", reset_auto_attack ) );
    // Name option to specify multiple pause actions on the same enemy
    add_option( opt_string( "name", name_str ) );

    parse_options( options_str );

    // Set the cooldown and stats' name to the action's custom name
    internal_id        = p->get_action_id( name_str );
    cooldown           = p->get_cooldown( name_str );
    cooldown->duration = cooldown_duration;
    stats              = p->get_stats( name_str, this );

    // Default duration and cooldown to 30s, and min/max to 0.5x and 1.5x.
    // Some sanity checks as well

    if ( base_execute_time <= 0_s )
    {
      sim->error( "Duration invalid or not set for action {}, setting to 30s", name() );
    }
    if ( duration_min <= 0_s )
      duration_min = base_execute_time * 0.5;
    if ( duration_max <= 0_s )
      duration_max = base_execute_time * 1.5;
    if ( base_execute_time <= duration_stddev )
    {
      sim->error( "Duration value for {} lower than standard deviation, setting stddev to 0", name() );
      duration_stddev = 0_s;
    }

    if ( cooldown->duration <= 0_s )
    {
      sim->error( "Cooldown invalid or not set action {}, setting to 30s", name() );
      cooldown->duration = 25_s;
    }
    if ( cooldown_min <= 0_s )
      cooldown_min = cooldown->duration * 0.5;
    if ( cooldown_max <= 0_s )
      cooldown_max = cooldown->duration * 1.5;
    if ( cooldown->duration <= cooldown_stddev )
    {
      sim->error( "Cooldown value for {} lower than standard deviation, setting stddev to 0", name() );
      cooldown_stddev = 0_s;
    }
  }

  // Don't trigger an assert related to result
  result_e calculate_result( action_state_t* ) const override
  {
    return RESULT_NONE;
  }

  timespan_t execute_time() const override
  {
    timespan_t duration = sim->rng().gauss( base_execute_time, duration_stddev );

    duration = clamp( duration, duration_min, duration_max );

    return duration;
  }

  void update_ready( timespan_t /* cd_duration */ ) override
  {
    timespan_t cd = sim->rng().gauss( cooldown->duration, cooldown_stddev );

    cd = clamp( cd, cooldown_min, cooldown_max );

    action_t::update_ready( cd );
  }
};
}  // end namespace actions

action_t* enemy_create_action( player_t* p, util::string_view name, util::string_view options_str )
{
  using namespace actions;

  if ( name == "auto_attack" )
    return new auto_attack_t( p, options_str );
  if ( name == "auto_attack_off_hand" )
    return new auto_attack_off_hand_t( p, options_str );
  if ( name == "melee_nuke" )
    return new melee_nuke_driver_t( p, options_str );
  if ( name == "spell_nuke" )
    return new spell_nuke_driver_t( p, options_str );
  if ( name == "spell_dot" )
    return new spell_dot_driver_t( p, options_str );
  if ( name == "spell_aoe" )
    return new spell_aoe_t( p, options_str );
  if ( name == "summon_add" )
    return new summon_add_t( p, options_str );
  if ( name == "pause_action" )
    return new pause_action_t( p, options_str );

  return nullptr;
}
// ==========================================================================
// Enemy Add
// ==========================================================================

struct add_t : public pet_t
{
  add_t( sim_t* s, enemy_t* o, util::string_view n, pet_e pt = PET_ENEMY ) : pet_t( s, o, n, pt )
  {
    true_level = s->max_player_level + 3;
  }

  void init_action_list() override
  {
    if ( action_list_str.empty() )
    {
      action_list_str += "/snapshot_stats";
    }

    pet_t::init_action_list();
  }

  double health_percentage() const override
  {
    timespan_t remainder = timespan_t::zero();
    timespan_t divisor   = timespan_t::zero();
    if ( duration > timespan_t::zero() )
    {
      remainder = expiration->remains();
      divisor   = duration;
    }
    else
    {
      remainder = std::max( timespan_t::zero(), sim->expected_iteration_time - sim->current_time() );
      divisor   = sim->expected_iteration_time;
    }

    return remainder / divisor * 100.0;
  }

  timespan_t time_to_percent( double percent ) const override
  {
    if ( duration > timespan_t::zero() )
    {
      if ( duration == expiration->remains() )
        return duration * ( 100.0 - percent ) * 0.01;

      const double health_pct = health_percentage();
      const double scale      = ( health_pct - percent ) / ( 100 - health_pct );
      return std::max( 0_ms, ( duration - expiration->remains() ) * scale );
    }

    return pet_t::time_to_percent( percent );
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_HEALTH;
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    action_t* a = enemy_create_action( this, name, options_str );

    if ( !a )
      a = pet_t::create_action( name, options_str );

    return a;
  }
};

// ==========================================================================
// Heal Enemy
// ==========================================================================

struct heal_enemy_t : public enemy_t
{
  heal_enemy_t( sim_t* s, util::string_view n, race_e r = RACE_HUMANOID ) : enemy_t( s, n, r, HEALING_ENEMY )
  {
    target     = this;
    true_level = s->max_player_level;  // set to default player level, not default+3
  }

  void init_resources( bool /* force */ ) override
  {
    resources.base[ RESOURCE_HEALTH ] = 20000000.0;

    player_t::init_resources( true );

    resources.current[ RESOURCE_HEALTH ] = resources.base[ RESOURCE_HEALTH ] / 1.5;
  }

  void init_base_stats() override
  {
    enemy_t::init_base_stats();

    collected_data.htps.change_mode( false );

    true_level = std::min( sim->max_player_level, true_level );
  }
  resource_e primary_resource() const override
  {
    return RESOURCE_HEALTH;
  }

  //  void init_action_list()
  //  { }
};

// ==========================================================================
// Tank Dummy Enemy
// ==========================================================================

struct tank_dummy_enemy_t : public enemy_t
{
  std::string tank_dummy_str;
  tank_dummy_e tank_dummy_enum;

  tank_dummy_enemy_t( sim_t* s, util::string_view n, race_e r = RACE_HUMANOID )
    : enemy_t( s, n, r, TANK_DUMMY ), tank_dummy_str( "none" ), tank_dummy_enum( tank_dummy_e::NONE )
  {
  }

  void create_options() override
  {
    add_option( opt_string( "tank_dummy_type", tank_dummy_str ) );
    enemy_t::create_options();
  }

  tank_dummy_e convert_tank_dummy_string( util::string_view tank_dummy_string )
  {
    if ( util::str_in_str_ci( tank_dummy_string, "none" ) )
      return tank_dummy_e::NONE;
    if ( util::str_in_str_ci( tank_dummy_string, "weak" ) )
      return tank_dummy_e::WEAK;
    if ( util::str_in_str_ci( tank_dummy_string, "dungeon" ) )
      return tank_dummy_e::DUNGEON;
    if ( util::str_in_str_ci( tank_dummy_string, "raid" ) )
      return tank_dummy_e::RAID;
    if ( util::str_in_str_ci( tank_dummy_string, "heroic" ) )
      return tank_dummy_e::HEROIC;
    if ( util::str_in_str_ci( tank_dummy_string, "mythic" ) )
      return tank_dummy_e::MYTHIC;

    if ( !tank_dummy_string.empty() )
      sim->print_debug( "Unknown Tank Dummy string input provided: {}", tank_dummy_string );

    return tank_dummy_e::NONE;
  }

  void init() override
  {
    tank_dummy_enum = convert_tank_dummy_string( tank_dummy_str );
    // Try parsing the name
    if ( tank_dummy_enum == tank_dummy_e::NONE )
      tank_dummy_enum = convert_tank_dummy_string( name_str );
    // if we still have no clue, pit them against the default value (Mythic)
    if ( tank_dummy_enum == tank_dummy_e::NONE )
      tank_dummy_enum = tank_dummy_e::MYTHIC;
  }

  void init_base_stats() override
  {
    enemy_t::init_base_stats();

    // override level
    switch ( tank_dummy_enum )
    {
      case tank_dummy_e::DUNGEON:
        true_level = sim->max_player_level + 2;
        break;
      case tank_dummy_e::WEAK:
        true_level = sim->max_player_level;
        break;
      default:
        true_level = sim->max_player_level + 3;
    }

    // override race
    race = RACE_HUMANOID;

    // Don't change the value if it's specified by the user - handled in enemy_t::init_base_stats()
    if ( custom_armor_coeff <= 0 )
    {
      base.armor_coeff = armor_coefficient( sim->max_player_level, tank_dummy_enum );
      sim->print_debug( "Enemy {} base armor coefficient set to {}.", *this, base.armor_coeff );
    }
  }

  std::string generate_action_list() override
  {
    return generate_tank_action_list( tank_dummy_enum );
  }

  void generate_heal_raid_event() override
  {
    return add_tank_heal_raid_event( tank_dummy_enum );
  }
};

// enemy_t::create_action ===================================================

action_t* enemy_t::create_action( util::string_view name, util::string_view options_str )
{
  action_t* a = enemy_create_action( this, name, options_str );

  if ( !a )
    a = player_t::create_action( name, options_str );

  return a;
}

void enemy_t::init_race()
{
  player_t::init_race();

  // target_race override
  if ( !sim->target_race.empty() )
  {
    race = util::parse_race_type( sim->target_race );
    if ( race == RACE_UNKNOWN )
    {
      throw std::invalid_argument(
          fmt::format( "{} could not parse race from sim target race '{}'.", name(), sim->target_race ) );
    }
  }
}
// enemy_t::init_base =======================================================

void enemy_t::init_base_stats()
{
  player_t::init_base_stats();

  resources.infinite_resource[ RESOURCE_HEALTH ] = false;

  if ( true_level == 0 )
    true_level = sim->max_player_level + 3;

  // target_level override
  if ( sim->target_level >= 0 )
    true_level = sim->target_level;
  else if ( sim->rel_target_level >= 0 && ( sim->max_player_level + sim->rel_target_level ) >= 0 )
    true_level = sim->max_player_level + sim->rel_target_level;
  else
    true_level = sim->max_player_level + 3;

  // waiting_time override
  waiting_time = timespan_t::from_seconds( 5.0 );
  if ( waiting_time < timespan_t::from_seconds( 1.0 ) )
    waiting_time = timespan_t::from_seconds( 1.0 );

  base.attack_crit_chance = 0.05;

  if ( !sim->overrides.target_health.empty() )
  {
    initial_health =
        static_cast<double>( sim->overrides.target_health[ enemy_id % sim->overrides.target_health.size() ] );
  }
  else
  {
    initial_health = fixed_health;
  }

  if ( this == sim->target )
  {
    if ( !sim->overrides.target_health.empty() || fixed_health > 0 )
    {
      sim->print_debug( "Setting vary_combat_length forcefully to 0.0 because fixed health simulation was detected." );

      sim->vary_combat_length = 0.0;
    }
  }

  if ( ( initial_health_percentage < 1 ) || ( initial_health_percentage > 100 ) )
  {
    initial_health_percentage = 100.0;
  }

  // Armor Coefficient, based on level (1054 @ 50; 2500 @ 60-63)
  base.armor_coeff = custom_armor_coeff > 0 ? custom_armor_coeff : armor_coefficient( level(), tank_dummy_e::MYTHIC );
  sim->print_debug( "{} base armor coefficient set to {}.", *this, base.armor_coeff );
}

void enemy_t::init_defense()
{
  player_t::init_defense();

  collected_data.health_changes_tmi.collect = false;
  collected_data.health_changes.collect     = false;

  if ( ( total_gear.armor ) <= 0 )
  {
    double& a = initial.stats.armor;

    a = dbc->npc_armor_value( level() );
  }
}

// enemy_t::init_buffs ======================================================

void enemy_t::create_buffs()
{
  player_t::create_buffs();

  if ( !sim->fixed_time && fixed_health_percentage == 0.0 )
  {
    for ( unsigned int i = 1; i <= 10; ++i )
    {
      buffs_health_decades.push_back(
          make_buff( this, fmt::format( "Health Decade ({} - {})", ( i - 1 ) * 10, i * 10 ) ) );
    }
  }
}

// enemy_t::init_resources ==================================================

void enemy_t::init_resources( bool /* force */ )
{
  double health_adjust = sim->iteration_time_adjust();

  resources.base[ RESOURCE_HEALTH ] = initial_health * health_adjust;

  player_t::init_resources( true );

  if ( initial_health_percentage > 0 )
  {
    resources.max[ RESOURCE_HEALTH ] *= 100.0 / initial_health_percentage;
  }

  resources.initial[ RESOURCE_HEALTH ] = std::ceil( resources.initial[ RESOURCE_HEALTH ] );
}

// enemy_t::init_target =====================================================

void enemy_t::init_target()
{
  if ( !target_str.empty() )
  {
    target = sim->find_player( target_str );
  }

  if ( target )
  {
    current_target = (int)target->actor_index;
    return;
  }

  for ( auto* p : sim->player_list )
  {
    if ( p->primary_role() != ROLE_TANK )
      continue;

    target = p;
    break;
  }

  if ( !target )
    target = sim->target;

  current_target = (int)target->actor_index;
}

// enemy_t::init_actions ====================================================

std::string enemy_t::generate_action_list()
{
  return generate_tank_action_list( tank_dummy_e::MYTHIC );
}

void enemy_t::generate_heal_raid_event()
{
  add_tank_heal_raid_event( tank_dummy_e::HEROIC );
}

std::string enemy_t::generate_tank_action_list( tank_dummy_e tank_dummy )
{
  std::string als;
  constexpr size_t numTankDummies = static_cast<size_t>( tank_dummy_e::MAX );
  //                               NONE, WEAK,           DUNGEON,  RAID,   HEROIC, MYTHIC
  //                               NONE, Normal Dungeon, Mythic 0, Normal, Heroic, Mythic
  // Level 70 Values
  // Defaulted to 20-man damage
  // Damage is normally increased from 10-man to 30-man by an average of 10% for every 5 players added.
  // 10-man -> 20-man = 20% increase; 20-man -> 30-man = 20% increase
  // Calculated by computing (M / <difficulty>) pre-mitigation (U) melee swings for all difficulties from Kazzara - Aberrus.
  // A Normal Dungeon estimate was then added below LFR. Prior to this update, it was approximately 3.9, but the rest of the
  // values were a bit lower, so this one was also lowered.
  std::array<double, numTankDummies> tank_dummy_index_scalar = { 0, 3.6, 2.8, 2.1, 1.5, 1};
  int aa_damage_base                                         = 830000;
  int dummy_strike_base                                      = aa_damage_base * 1.5;
  int background_spell_base                                  = aa_damage_base * 0.04;

  size_t tank_dummy_index = static_cast<size_t>( tank_dummy );
  als += "/auto_attack,damage=" + util::to_string( floor( aa_damage_base / tank_dummy_index_scalar[ tank_dummy_index ] ) ) +
         ",range=" + util::to_string( aa_damage_base / tank_dummy_index_scalar[ tank_dummy_index ] * 0.02 ) +
         ",attack_speed=2,aoe_tanks=1";
  als += "/melee_nuke,damage=" + util::to_string( floor( dummy_strike_base / tank_dummy_index_scalar[ tank_dummy_index ] ) ) +
         ",range=" + util::to_string( floor( dummy_strike_base / tank_dummy_index_scalar[ tank_dummy_index ] * 0.02 ) ) +
         ",attack_speed=2,cooldown=30,aoe_tanks=1";
  als += "/spell_dot,damage=" + util::to_string( floor( background_spell_base / tank_dummy_index_scalar[ tank_dummy_index ] ) ) +
         ",range=" + util::to_string( floor( background_spell_base / tank_dummy_index_scalar[ tank_dummy_index ] * 0.02 ) ) +
         ",tick_time=2,cooldown=60,aoe_tanks=1,dot_duration=30,bleed=1";
  // pause periodically to mimic a tank swap
  als += "/pause_action,duration=30,cooldown=30,if=time>=30";
  return als;
}

// enemy_t::add_tank_heal_raid_event() ======================================

void enemy_t::add_tank_heal_raid_event( tank_dummy_e tank_dummy )
{
  constexpr size_t numTankDummies = static_cast<size_t>( tank_dummy_e::MAX );
  //                                           NONE, WEAK, DUNGEON, RAID,  HEROIC, MYTHIC
  std::array<int, numTankDummies> heal_value = { 0, 12000, 24000, 36000, 48000, 60000 };
  size_t tank_dummy_index                    = static_cast<size_t>( tank_dummy );
  std::string heal_raid_event = fmt::format( "heal,name=tank_heal,amount={},period=0.5,duration=0,player_if=role.tank",
                                             heal_value[ tank_dummy_index ] );
  sim->raid_events_str += "/" + heal_raid_event;
  std::string::size_type cut_pt = heal_raid_event.find_first_of( ',' );
  auto heal_options             = heal_raid_event.substr( cut_pt + 1 );
  auto heal_name                = heal_raid_event.substr( 0, cut_pt );
  auto raid_event               = raid_event_t::create( sim, heal_name, heal_options );

  if ( raid_event->cooldown <= timespan_t::zero() )
  {
    throw std::invalid_argument( "Cooldown not set or negative." );
  }
  if ( raid_event->cooldown <= raid_event->cooldown_stddev )
  {
    throw std::invalid_argument( "Cooldown lower than cooldown standard deviation." );
  }

  sim->print_debug( "Successfully created '{}'.", *( raid_event.get() ) );
  sim->raid_events.push_back( std::move( raid_event ) );
}

void enemy_t::init_action_list()
{
  if ( !is_add() && is_enemy() )
  {
    // If the action list string is empty, automatically populate it
    if ( action_list_str.empty() )
    {
      std::string& precombat_list = get_action_priority_list( "precombat" )->action_list_str;
      precombat_list += "/snapshot_stats";

      // If targeting an player, use Fluffy Pillow or Tank Dummy boss as appropriate
      if ( !target->is_enemy() )
      {
        generate_heal_raid_event();

        action_list_str += generate_action_list();
      }

      // Otherwise just auto-attack the heal target
      else if ( sim->heal_target && this != sim->heal_target )
      {
        unsigned int healers = 0;
        for ( auto* p : sim->player_list )
          if ( !p->is_pet() && p->primary_role() == ROLE_HEAL )
            ++healers;

        double hps_per_healer = 90000;
        double swing_speed    = 2.0;
        action_list_str +=
            "/auto_attack,damage=" + util::to_string( hps_per_healer * healers * swing_speed * level() / 100.0 ) +
            ",attack_speed=" + util::to_string( swing_speed ) + ",target=" + sim->heal_target->name_str;
      }
      // Otherwise... do nothing?
      else
        action_list_str += "/";
    }
  }

  /* If we have more than one tank in the simulation, we do some fancy stuff.
     We take the default APL and clone it into a new APL for each tank, appending
     a "target=Tank_Name" to each ability. Then the default APL is replaced with
     a series of /run_action_list entries at the end. This is how we support tank swaps.
  */
  // only do this for enemies targeting players
  if ( sim->enable_taunts && !target->is_enemy() && this != sim->heal_target )
  {
    // count the number of tanks in the simulation
    std::vector<player_t*> tanks;
    for ( auto* p : sim->player_list )
    {
      if ( p->primary_role() == ROLE_TANK )
        tanks.push_back( p );
    }

    // If we have more than one tank, create a new action list for each
    // Only do this if the user hasn't specified additional action lists beyond precombat & default
    if ( tanks.size() > 1 && action_priority_list.size() < 3 )
    {
      std::string new_action_list_str;

      // split the action_list_str into individual actions so we can modify each later
      auto splits = util::string_split<util::string_view>( action_list_str, "/" );

      for ( const auto* tank : tanks )
      {
        // create a new APL sub-entry with the name of the tank in question
        std::string& tank_str = get_action_priority_list( tank->name_str )->action_list_str;

        // Reconstruct the action_list_str for this tank by appending ",target=Tank_Name"
        // to each action if it doesn't already specify a different target
        for ( auto& split : splits )
        {
          tank_str += fmt::format( "/{}", split );

          if ( !util::str_in_str_ci( "target=", split ) )
            tank_str += ",target=" + tank->name_str;
        }

        // add a /run_action_list line to the new default APL
        new_action_list_str +=
            "/run_action_list,name=" + tank->name_str + ",if=current_target=" + util::to_string( tank->actor_index );
      }

      // finish off the default action list with an instruction to run the default target's APL
      if ( !target->is_enemy() )
        new_action_list_str += "/run_action_list,name=" + target->name_str;
      else
        new_action_list_str += "/run_action_list,name=" + tanks[ 0 ]->name_str;

      // replace the default APL with our new one.
      action_list_str = new_action_list_str;
    }
  }
  player_t::init_action_list();
}

// Hack to get this executed after player_t::init_action_list.
void enemy_t::init_stats()
{
  player_t::init_stats();

  // Small hack to increase waiting time for target without any actions
  for ( size_t i = 0; i < action_list.size(); ++i )
  {
    action_t* action = action_list[ i ];
    if ( action->background )
      continue;
    if ( action->name_str == "snapshot_stats" )
      continue;
    if ( action->name_str.find( "auto_attack" ) != std::string::npos )
      continue;
    waiting_time = timespan_t::from_seconds( 1.0 );
    break;
  }
}

// enemy_t::resource_loss ===================================================

double enemy_t::resource_loss( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  // This mechanic compares pre and post health decade, and if it switches to a lower decade,
  // it triggers the respective trigger buff.
  // Starting decade ( 100% to 90% ) is triggered in combat_begin()

  int pre_health  = static_cast<int>( resources.pct( RESOURCE_HEALTH ) * 100 ) / 10;
  double r        = player_t::resource_loss( resource_type, amount, source, action );
  int post_health = static_cast<int>( resources.pct( RESOURCE_HEALTH ) * 100 ) / 10;

  if ( buffs_health_decades.size() && pre_health < 10 && pre_health > post_health && post_health >= 0 )
  {
    buffs_health_decades.at( post_health + 1 )->expire();
    buffs_health_decades.at( post_health )->trigger();
  }

  return r;
}

// enemy_t::create_options ==================================================

void enemy_t::create_options()
{
  // this first part handles enemy options that are sim-wide
  // TODO: FIXME, as this surely isn't working as intended in all use cases.
  add_option( opt_float( "enemy_health", fixed_health ) );
  add_option( opt_float( "enemy_initial_health_percentage", initial_health_percentage ) );
  add_option( opt_float( "enemy_fixed_health_percentage", fixed_health_percentage ) );
  add_option( opt_float( "health_recalculation_dampening_exponent", health_recalculation_dampening_exponent ) );
  add_option( opt_float( "enemy_height", height ) );
  add_option( opt_float( "enemy_reach", combat_reach ) );
  add_option( opt_string( "enemy_tank", target_str ) );
  add_option( opt_int( "apply_debuff", apply_damage_taken_debuff ) );

  // the next part handles actor-specific options for enemies
  player_t::create_options();

  add_option( opt_int( "level", true_level, 0, MAX_LEVEL + 3 ) );
  add_option( opt_float( "armor_coefficient", custom_armor_coeff ) );
}

// enemy_t::create_add ======================================================

pet_t* enemy_t::create_pet( util::string_view add_name, util::string_view /* pet_type */ )
{
  pet_t* p = find_pet( add_name );
  if ( !p )
    p = new add_t( sim, this, add_name, PET_ENEMY );
  return p;
}

// enemy_t::create_pets =====================================================

void enemy_t::create_pets()
{
  for ( int i = 0; i < sim->target_adds; i++ )
  {
    create_pet( fmt::format( "add{}", i ) );
  }
}

// enemy_t::health_percentage() =============================================

double enemy_t::health_percentage() const
{
  if ( fixed_health_percentage > 0 && sim->current_time() < sim->expected_iteration_time )
  {
    return fixed_health_percentage;
  }
  else if ( fixed_health_percentage > 0 )
    return 0.0;

  if ( resources.base[ RESOURCE_HEALTH ] == 0 || sim->fixed_time )  // first iteration or fixed time sim.
  {
    timespan_t remainder = std::max( timespan_t::zero(), ( sim->expected_iteration_time - sim->current_time() ) );

    return ( remainder / sim->expected_iteration_time ) * ( initial_health_percentage - death_pct ) + death_pct;
  }

  return resources.pct( RESOURCE_HEALTH ) * 100;
}

// enemy_t::time_to_percent() ===============================================

timespan_t enemy_t::time_to_percent( double percent ) const
{
  // First check current health, considering fixed_health_percentage and initial_health_percentage
  if ( health_percentage() <= percent )
    return timespan_t::zero();

  // time_to_pct_0, or time_to_die, can ignore fixed_health_percentage or initial_health_percentage
  if ( percent != 0 )
  {
    // Return unreachable values for percentages that can't possibly be encountered
    if ( fixed_health_percentage > percent || percent < death_pct || initial_health_percentage <= death_pct )
    {
      return sim->expected_iteration_time * 2;
    }

    // If using fixed_time with custom initial health settings, scale the input value
    if ( sim->fixed_time && initial_health_percentage != 100.0 )
    {
      percent = ( percent / initial_health_percentage ) * 100.0;
    }
  }

  return player_t::time_to_percent( percent );
}

// enemy_t::recalculate_health ==============================================

void enemy_t::recalculate_health()
{
  if ( sim->expected_iteration_time <= timespan_t::zero() || fixed_health > 0 )
    return;

  if ( initial_health == 0 )  // first iteration
  {
    initial_health = iteration_dmg_taken * ( sim->expected_iteration_time / sim->current_time() ) *
                     ( 1.0 / ( 1.0 - death_pct / 100 ) );
  }
  else
  {
    timespan_t delta_time = sim->current_time() - sim->expected_iteration_time;
    delta_time /= std::pow( ( sim->current_iteration + 1 ),
                            health_recalculation_dampening_exponent );  // dampening factor, by default 1/n
    double factor = 1.0 - ( delta_time / sim->expected_iteration_time );

    factor = clamp( factor, 0.5, 1.5 );

    if ( sim->current_time() > sim->expected_iteration_time &&
         this != sim->target )  // Special case for aoe targets that do not die before fluffy pillow.
      factor = 1;

    initial_health *= factor;
  }

  sim->print_debug( "Target {} initial health calculated to be {}. Damage was {}", name(), initial_health,
                    iteration_dmg_taken );
}

bool enemy_t::taunt( player_t* source )
{
  current_target = (int)source->actor_index;
  if ( main_hand_attack && main_hand_attack->execute_event )
    event_t::cancel( main_hand_attack->execute_event );
  if ( off_hand_attack && off_hand_attack->execute_event )
    event_t::cancel( off_hand_attack->execute_event );

  return true;
}

void enemy_t::reset_auto_attacks( timespan_t delay, proc_t* proc )
{
  player_t::reset_auto_attacks( delay, proc );

  using namespace actions;

  auto mh = dynamic_cast<melee_t*>( main_hand_attack );
  auto oh = dynamic_cast<melee_t*>( off_hand_attack );

  if ( mh )
  {
    auto driver = debug_cast<auto_attack_t*>( mh->driver );

    // the first element is main_hand_attack and it's already been rescheduled
    for ( size_t i = 1; i < driver->mh_list.size(); i++ )
    {
      auto melee = driver->mh_list[ i ];
      if ( melee->execute_event )
      {
        event_t::cancel( melee->execute_event );
        melee->schedule_execute();

        if ( delay > 0_ms )
          melee->execute_event->reschedule( melee->execute_event->remains() + delay );
      }
    }
  }

  if ( oh )
  {
    auto driver = debug_cast<auto_attack_off_hand_t*>( mh->driver );

    // the first element is off_hand_attack and it's already been rescheduled
    for ( size_t i = 1; i < driver->oh_list.size(); i++ )
    {
      auto melee = driver->oh_list[ i ];
      if ( melee->execute_event )
      {
        event_t::cancel( melee->execute_event );
        melee->schedule_execute();

        if ( delay > 0_ms )
          melee->execute_event->reschedule( melee->execute_event->remains() + delay );
      }
    }
  }
}

void enemy_t::delay_auto_attacks( timespan_t delay, proc_t* proc )
{
  player_t::delay_auto_attacks( delay, proc );

  if ( delay == 0_ms )
    return;

  using namespace actions;

  auto mh = dynamic_cast<melee_t*>( main_hand_attack );
  auto oh = dynamic_cast<melee_t*>( off_hand_attack );

  if ( mh )
  {
    auto driver = debug_cast<auto_attack_t*>( mh->driver );

    // the first element is main_hand_attack and it's already been rescheduled
    for ( size_t i = 1; i < driver->mh_list.size(); i++ )
    {
      auto event = driver->mh_list[ i ]->execute_event;
      if ( event )
        event->reschedule( event->remains() + delay );
    }
  }

  if ( oh )
  {
    auto driver = debug_cast<auto_attack_off_hand_t*>( mh->driver );

    // the first element is off_hand_attack and it's already been rescheduled
    for ( size_t i = 1; i < driver->oh_list.size(); i++ )
    {
      auto event = driver->oh_list[ i ]->execute_event;
      if ( event )
        event->reschedule( event->remains() + delay );
    }
  }
}

// enemy_t::create_expression ===============================================

std::unique_ptr<expr_t> enemy_t::create_expression( util::string_view expression_str )
{
  if ( expression_str == "adds" )
    return make_mem_fn_expr( expression_str, active_pets, &std::vector<pet_t*>::size );

  // override enemy health.pct expression
  if ( expression_str == "health.pct" )
    return make_mem_fn_expr( expression_str, *this, &enemy_t::health_percentage );

  // current target (for tank/taunting purposes)
  if ( expression_str == "current_target" )
    return make_ref_expr( expression_str, current_target );

  auto splits = util::string_split<util::string_view>( expression_str, "." );

  if ( splits[ 0 ] == "current_target" )
  {
    if ( splits.size() == 2 && splits[ 1 ] == "name" )
    {
      struct current_target_name_expr_t : public expr_t
      {
        enemy_t* boss;

        current_target_name_expr_t( enemy_t* e ) : expr_t( "current_target_name" ), boss( e )
        {
        }

        double evaluate() override
        {
          return (double)boss->sim->actor_list[ boss->current_target ]->actor_index;
        }
      };

      return std::make_unique<current_target_name_expr_t>( this );
    }
    else if ( splits.size() == 3 && splits[ 1 ] == "debuff" )
    {
      struct current_target_debuff_expr_t : public expr_t
      {
        enemy_t* boss;
        std::string debuff_str;

        current_target_debuff_expr_t( enemy_t* e, util::string_view debuff_str )
          : expr_t( "current_target_debuff" ), boss( e ), debuff_str( debuff_str )
        {
        }

        double evaluate() override
        {
          if ( debuff_str == "damage_taken" )
            return boss->sim->actor_list[ boss->current_target ]->debuffs.damage_taken->current_stack;
          // else if ( debuff_str == "vulnerable" )
          //  return boss -> sim -> actor_list[ boss -> current_target ] -> debuffs.vulnerable -> current_stack;
          // else if ( debuff_str == "mortal_wounds" )
          //  return boss -> sim -> actor_list[ boss -> current_target ] -> debuffs.mortal_wounds -> current_stack;
          // may add others here as desired
          else
            return 0;
        }
      };

      return std::make_unique<current_target_debuff_expr_t>( this, splits[ 2 ] );
    }
  }

  if ( splits.size() == 3 && splits[ 0 ] == "action" )
  {
    for ( size_t i = 0; i < action_list.size(); ++i )
    {
      action_t* action = action_list[ i ];
      if ( action->name_str == splits[ 1 ] )
        return action->create_expression( splits[ 2 ] );
    }
  }

  return player_t::create_expression( expression_str );
}

// enemy_t::combat_begin ====================================================

void enemy_t::combat_begin()
{
  player_t::combat_begin();

  if ( buffs_health_decades.size() )
    buffs_health_decades[ 9 ]->trigger();
}

// enemy_t::combat_end ======================================================

void enemy_t::combat_end()
{
  player_t::combat_end();

  if ( sim->overrides.target_health.empty() )
    recalculate_health();
}

void enemy_t::demise()
{
  if ( this == sim->target )
  {
    if ( sim->current_iteration != 0 || !sim->overrides.target_health.empty() || fixed_health > 0 )
      // For the main target, end simulation on death.
      sim->cancel_iteration();
  }

  player_t::demise();
}

double enemy_t::armor_coefficient( int level, tank_dummy_e dungeon_content )
{
  // Armor coefficient (colloquially called "k-value")
  // Max level enemies have different armor coefficient based on the difficulty setting and the area they are fought
  // in. The default value stored in spelldata only works for outdoor and generally "easy" content. New values are
  // added when new seasonal content (new raid, new M+ season) is released. ArmorConstantMod is pulled from the
  // ExpectedStatMod table. Values are a combination of the base "K-values" for the intended level, multiplied by the
  // ArmorConstantMod field.

  // In order to get the ArmorConstMod ID you first get the ID from the map you are looking for.
  // You then take the Map ID and search the ID within the MapDifficulty table. For raids, you will come back with
  // four records, one for each difficulty. Each of these will have a ContentTuningID. You then take the
  // ContentTuningID and search the ID in the ContentTuningXExpected table. Often you will come back with multiple
  // results but the one that is correct will generally be the one that has the ArmorConstMod value being greater
  // than 1.000.

  /*
    10.0 values here
    Level 70 Base/open world: 11766.000 (Level 70 Armor mitigation constants (K-values))
    Level 70 M0/M+: 12,824.94039274908 (ExpectedStatModID: 216; ArmorConstMod: 1.09000003338)
    Vault of the Incarnates LFR: 13,083.79186539696 (ExpectedStatModID: 212; ArmorConstMod: 1.11199998856)
    Vault of the Incarnates Normal: 14,025.07237027602 (ExpectedStatModID: 213; ArmorConstMod: 1.19200003147)
    Vault of the Incarnates Heroic: 15,084.01136040024 (ExpectedStatModID: 214; ArmorConstMod: 1.28199994564)
    Vault of the Incarnates Mythic: 16,284.14333792718 (ExpectedStatModID: 215; ArmorConstMod: 1.38399994373)
    Level 70 Season 2 M0/M+: 14,742.79824685068 (ExpectedStatModID: 234; ArmorConstMod: 1.25300002098)
    Aberrus, the Shadowed Crucible LFR: 15,084.01136040024 (ExpectedStatModID: 214; ArmorConstMod: 1.28199994564)
    Aberrus, the Shadowed Crucible Normal: 16,284.14333792718 (ExpectedStatModID: 215; ArmorConstMod: 1.38399994373)
    Aberrus, the Shadowed Crucible Heroic: 17,625.4683029745 (ExpectedStatModID: 227; ArmorConstMod: 1.49800002575)
    Aberrus, the Shadowed Crucible Mythic: 19,155.04824685068 (ExpectedStatModID: 228; ArmorConstMod: 1.62800002098)
  */
  double k = dbc->armor_mitigation_constant( level );

  switch ( dungeon_content )
  {
    case tank_dummy_e::DUNGEON:
      return k * 1.25300002098;  // M0/M+
      break;
    case tank_dummy_e::RAID:
      return k * 1.28199994564;  // Normal Raid
      break;
    case tank_dummy_e::HEROIC:
      return k * 1.49800002575;  // Heroic Raid
      break;
    case tank_dummy_e::MYTHIC:
      return k * 1.62800002098;  // Mythic Raid
      break;
    default:
      break;  // tank_dummy_e::NONE
  }

  return k;
}

// ENEMY MODULE INTERFACE ===================================================

struct enemy_module_t : public module_t
{
  enemy_module_t() : module_t( ENEMY )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e /* r = RACE_NONE */ ) const override
  {
    return new enemy_t( sim, name );
  }
  bool valid() const override
  {
    return true;
  }
  void init( player_t* ) const override
  {
  }
  void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
};

// HEAL ENEMY MODULE INTERFACE ==============================================

struct heal_enemy_module_t : public module_t
{
  heal_enemy_module_t() : module_t( HEALING_ENEMY )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e /* r = RACE_NONE */ ) const override
  {
    auto p = new heal_enemy_t( sim, name );
    return p;
  }
  bool valid() const override
  {
    return true;
  }
  void init( player_t* ) const override
  {
  }
  void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
};

// TANK DUMMY ENEMY MODULE INTERFACE ==============================================

struct tank_dummy_enemy_module_t : public module_t
{
  tank_dummy_enemy_module_t() : module_t( TANK_DUMMY )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e /* r = RACE_NONE */ ) const override
  {
    auto p = new tank_dummy_enemy_t( sim, name );
    return p;
  }
  bool valid() const override
  {
    return true;
  }
  void init( player_t* ) const override
  {
  }
  void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
};

}  // END UNNAMED NAMESPACE

const module_t* module_t::enemy()
{
  static enemy_module_t m = enemy_module_t();
  return &m;
}

const module_t* module_t::heal_enemy()
{
  static heal_enemy_module_t m = heal_enemy_module_t();
  return &m;
}

const module_t* module_t::tank_dummy_enemy()
{
  static tank_dummy_enemy_module_t m = tank_dummy_enemy_module_t();
  return &m;
}
